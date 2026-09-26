#!/usr/bin/env python3
"""Ground-side decoder and checker for HK-TM telemetry captured from the target.

Reads a console log from the target (hk_tm_sink_console.c writes one
"TM <hex>" line per packet) and checks every packet against
design/data-model.md, independently of the flight encoder: CCSDS primary
header, PUS-C TM[3,25] secondary header, SID, parameter order, stale markers,
and the packet error control (CRC-16/CCITT-FALSE, computed here with Python's
binascii). It also checks the command/telemetry ordering of the scripted
ground sequence in src/app/rtems/init.c and reports the inter-report
interval jitter, an indicative figure only for HKS-SR-140 under emulation.

Exit status is 0 only if every check passes.
"""

import argparse
import binascii
import re
import sys

APID = 0x0CA
PARAM_COUNT = 32
PACKET_LEN = 6 + 11 + 1 + 4 * PARAM_COUNT + 2
STALE_MARKER = 0xFFFFFFFF
PARAM_IDS = [(sub << 8) | n for sub in (1, 2, 3, 4) for n in range(1, 9)]
REPORT_PERIOD_S = 4.0
JITTER_LIMIT_S = 0.10 * REPORT_PERIOD_S  # HKS-SR-140

TC_ENABLE = re.compile(r"\[cmd\] telecommand 1 accepted")
TC_DISABLE = re.compile(r"\[cmd\] telecommand 2 accepted")
TC_UNKNOWN_REJECTED = re.compile(r"\[cmd\] telecommand 127 REJECTED")


def check_packet(pkt, errors, idx):
    def fail(msg):
        errors.append(f"packet {idx}: {msg}")

    if len(pkt) != PACKET_LEN:
        fail(f"length {len(pkt)}, expected {PACKET_LEN}")
        return None
    word0 = int.from_bytes(pkt[0:2], "big")
    word1 = int.from_bytes(pkt[2:4], "big")
    data_len = int.from_bytes(pkt[4:6], "big")
    if word0 >> 13 != 0 or (word0 >> 12) & 1 != 0 or (word0 >> 11) & 1 != 1:
        fail(f"primary header version/type/sec-hdr wrong: {word0:#06x}")
    if word0 & 0x7FF != APID:
        fail(f"APID {word0 & 0x7FF:#05x}, expected {APID:#05x}")
    if word1 >> 14 != 3:
        fail("sequence flags not 'unsegmented'")
    if data_len != len(pkt) - 7:
        fail(f"data length field {data_len}, expected {len(pkt) - 7}")
    if pkt[6] != 0x20 or pkt[7] != 3 or pkt[8] != 25:
        fail(f"secondary header {pkt[6]:#04x} [{pkt[7]},{pkt[8]}], expected 0x20 [3,25]")
    crc = binascii.crc_hqx(pkt[:-2], 0xFFFF)
    if crc != int.from_bytes(pkt[-2:], "big"):
        fail(f"PEC {pkt[-2:].hex()} != computed {crc:04x}")
    if pkt[17] != 1:
        fail(f"SID {pkt[17]}, expected 1")

    coarse = int.from_bytes(pkt[11:15], "big")
    fine = int.from_bytes(pkt[15:17], "big")
    values = [int.from_bytes(pkt[18 + 4 * i: 22 + 4 * i], "big") for i in range(PARAM_COUNT)]
    stale = [PARAM_IDS[i] for i, v in enumerate(values) if v == STALE_MARKER]
    cycles = set()
    for i, v in enumerate(values):
        if v == STALE_MARKER:
            continue
        if v >> 16 != PARAM_IDS[i]:  # simulated source encodes its id (SR-050 order)
            fail(f"value {i} carries id {v >> 16:#06x}, expected {PARAM_IDS[i]:#06x}")
        cycles.add(v & 0xFFFF)
    if len(cycles) > 1:
        fail(f"values from mixed collection cycles {sorted(cycles)}")
    return {
        "seq": word1 & 0x3FFF,
        "time": coarse + fine / 65536.0,
        "cycle": cycles.pop() if cycles else None,
        "stale": stale,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("log", help="target console log")
    ap.add_argument("--expect-packets", type=int, default=4)
    args = ap.parse_args()

    lines = open(args.log, encoding="utf-8", errors="replace").read().splitlines()
    errors, packets = [], []
    enable_at = disable_at = None
    unknown_rejected = False
    for n, line in enumerate(lines):
        if TC_ENABLE.search(line) and enable_at is None:
            enable_at = n
        if TC_DISABLE.search(line):
            disable_at = n
        if TC_UNKNOWN_REJECTED.search(line):
            unknown_rejected = True
        if line.startswith("TM "):
            try:
                pkt = bytes.fromhex(line[3:].strip())
            except ValueError:
                errors.append(f"line {n + 1}: malformed TM line")
                continue
            info = check_packet(pkt, errors, len(packets))
            if info is not None:
                info["line"] = n
                packets.append(info)

    print(f"{'#':>2} {'seq':>4} {'CUC time (s)':>13} {'cycle':>5}  stale parameters")
    for i, p in enumerate(packets):
        stale = ", ".join(f"{s:#06x}" for s in p["stale"]) or "-"
        print(f"{i:>2} {p['seq']:>4} {p['time']:>13.6f} {str(p['cycle']):>5}  {stale}")

    # SR-120/100/110: nothing before enable, nothing after disable
    if enable_at is None:
        errors.append("no accepted ENABLE_REPORTS telecommand in the log")
    if not unknown_rejected:
        errors.append("unknown telecommand 0x7F was not reported REJECTED (SR-130)")
    for p in packets:
        if enable_at is not None and p["line"] < enable_at:
            errors.append(f"packet seq {p['seq']} emitted before ENABLE (SR-120)")
        if disable_at is not None and p["line"] > disable_at:
            errors.append(f"packet seq {p['seq']} emitted after DISABLE (SR-110)")
    if len(packets) != args.expect_packets:
        errors.append(f"{len(packets)} packets, expected {args.expect_packets}")

    for a, b in zip(packets, packets[1:]):
        if b["seq"] != (a["seq"] + 1) & 0x3FFF:
            errors.append(f"sequence gap {a['seq']} -> {b['seq']}")

    intervals = [b["time"] - a["time"] for a, b in zip(packets, packets[1:])]
    if intervals:
        jitter = max(abs(t - REPORT_PERIOD_S) for t in intervals)
        verdict = "within" if jitter <= JITTER_LIMIT_S else "EXCEEDS"
        print(f"report intervals (s): {', '.join(f'{t:.6f}' for t in intervals)}")
        print(f"max jitter {jitter * 1000:.3f} ms, {verdict} the {JITTER_LIMIT_S * 1000:.0f} ms "
              "limit (HKS-SR-140, indicative only under emulation)")
        if jitter > JITTER_LIMIT_S:
            errors.append("report jitter exceeds 10% of the generation interval")

    if errors:
        print(f"FAIL: {len(errors)} problem(s)")
        for e in errors:
            print(f"  - {e}")
        return 1
    print(f"PASS: {len(packets)} TM[3,25] packets decoded and checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Ground-side decoder and checker for HK-TM telemetry captured from the target.

Reads a console log from the target (hk_tm_sink_console.c writes one
"TM <hex>" line per packet) and checks every packet against
design/data-model.md, independently of the flight encoder: CCSDS primary
header, PUS-C TM[3,25] secondary header, SID, stale markers, and the packet
error control (CRC-16/CCITT-FALSE, computed here with Python's binascii).
Each value is checked against this tool's own copy of the parameter limits
(a ground-side mission database, independent of src/fdir/src/hk_limits.c).
The simulated source makes every value mid soft band + (cycle % 64), so the
offsets also show the parameter order and that one packet holds one
collection. FDIR event reports, PUS TM[5,s] on their own APID, are decoded
and counted (HKS-FD-040). It also checks the command/telemetry ordering of the scripted
ground sequence in src/app/rtems/init.c and reports the inter-report
interval jitter, an indicative figure only for HKS-SR-140 under emulation.

Exit status is 0 only if every check passes.
"""

import argparse
import binascii
import re
import sys

APID = 0x0CA
EVT_APID = 0x0CB
PARAM_COUNT = 32
PACKET_LEN = 6 + 11 + 1 + 4 * PARAM_COUNT + 2
EVT_PACKET_LEN = 6 + 11 + 5 + 2
STALE_MARKER = 0xFFFFFFFF
PARAM_IDS = [(sub << 8) | n for sub in (1, 2, 3, 4) for n in range(1, 9)]
SIM_CYCLE_SPAN = 64

# Ground copy of the limits, report order: (low_crit, low_warn, high_warn, high_crit)
LIMITS = [
    (2600, 2800, 3300, 3500), (2400, 2600, 3400, 3600), (400, 800, 3300, 3700),
    (1500, 1650, 1850, 2000), (1800, 1900, 2200, 2300), (2000, 2150, 2450, 2600),
    (150, 300, 2800, 3400), (500, 900, 3200, 3600),
    (700, 1000, 3000, 3300), (650, 950, 3050, 3350), (1200, 1500, 2600, 2900),
    (600, 900, 3100, 3400), (300, 600, 3500, 3800), (320, 620, 3480, 3780),
    (350, 650, 3450, 3750), (900, 1200, 2800, 3100),
    (1600, 1700, 1900, 2000), (1500, 1600, 1800, 1900), (200, 400, 2400, 3000),
    (800, 1100, 2900, 3200), (1900, 1950, 2150, 2200), (1550, 1650, 1950, 2050),
    (1700, 1800, 2300, 2400), (800, 1000, 3000, 3300),
    (300, 600, 3500, 3800), (300, 600, 3500, 3800), (300, 600, 3500, 3800),
    (1600, 1700, 2100, 2200), (1650, 1750, 2050, 2150), (400, 700, 3400, 3700),
    (400, 700, 3400, 3700), (1700, 1800, 2200, 2300),
]
EVT_SEVERITY = {1: "info", 2: "low", 3: "medium", 4: "high"}
EVT_NAMES = {1: "limit", 2: "stale", 3: "safe mode"}
LIMIT_STATES = {1: "warn low", 2: "warn high", 3: "crit low", 4: "crit high"}
REPORT_PERIOD_S = 4.0
JITTER_LIMIT_S = 0.10 * REPORT_PERIOD_S  # HKS-SR-140

TC_ENABLE = re.compile(r"\[cmd\] telecommand 1 accepted")
TC_DISABLE = re.compile(r"\[cmd\] telecommand 2 accepted")
TC_UNKNOWN_REJECTED = re.compile(r"\[cmd\] telecommand 127 REJECTED")


def check_header(pkt, fail, apid, length):
    word0 = int.from_bytes(pkt[0:2], "big")
    word1 = int.from_bytes(pkt[2:4], "big")
    data_len = int.from_bytes(pkt[4:6], "big")
    if len(pkt) != length:
        fail(f"length {len(pkt)}, expected {length}")
        return None
    if word0 >> 13 != 0 or (word0 >> 12) & 1 != 0 or (word0 >> 11) & 1 != 1:
        fail(f"primary header version/type/sec-hdr wrong: {word0:#06x}")
    if word0 & 0x7FF != apid:
        fail(f"APID {word0 & 0x7FF:#05x}, expected {apid:#05x}")
    if word1 >> 14 != 3:
        fail("sequence flags not 'unsegmented'")
    if data_len != len(pkt) - 7:
        fail(f"data length field {data_len}, expected {len(pkt) - 7}")
    if binascii.crc_hqx(pkt[:-2], 0xFFFF) != int.from_bytes(pkt[-2:], "big"):
        fail("PEC does not match")
    return word1 & 0x3FFF


def check_packet(pkt, errors, idx):
    def fail(msg):
        errors.append(f"packet {idx}: {msg}")

    seq = check_header(pkt, fail, APID, PACKET_LEN)
    if seq is None:
        return None
    if pkt[6] != 0x20 or pkt[7] != 3 or pkt[8] != 25:
        fail(f"secondary header {pkt[6]:#04x} [{pkt[7]},{pkt[8]}], expected 0x20 [3,25]")
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
        _, low_warn, high_warn, _ = LIMITS[i]
        if not low_warn <= v <= high_warn:
            fail(f"value {i} ({PARAM_IDS[i]:#06x}) = {v} outside soft limits "
                 f"[{low_warn}, {high_warn}]")
        offset = v - (low_warn + high_warn) // 2  # sim: mid + cycle % 64 (SR-050 order)
        if not 0 <= offset < SIM_CYCLE_SPAN:
            fail(f"value {i} ({PARAM_IDS[i]:#06x}) = {v} is not this parameter's")
        cycles.add(offset)
    if len(cycles) > 1:
        fail(f"values from mixed collection cycles {sorted(cycles)}")
    return {
        "seq": seq,
        "time": coarse + fine / 65536.0,
        "cycle": cycles.pop() if cycles else None,
        "stale": stale,
    }


def check_event(pkt, errors, idx):
    def fail(msg):
        errors.append(f"event {idx}: {msg}")

    seq = check_header(pkt, fail, EVT_APID, EVT_PACKET_LEN)
    if seq is None:
        return None
    if pkt[6] != 0x20 or pkt[7] != 5 or pkt[8] not in EVT_SEVERITY:
        fail(f"secondary header {pkt[6]:#04x} [{pkt[7]},{pkt[8]}], expected 0x20 [5,1..4]")
    evt_id = int.from_bytes(pkt[17:19], "big")
    if evt_id not in EVT_NAMES:
        fail(f"unknown event id {evt_id}")
    return {
        "seq": seq,
        "time": int.from_bytes(pkt[11:15], "big") + int.from_bytes(pkt[15:17], "big") / 65536.0,
        "severity": EVT_SEVERITY.get(pkt[8], "?"),
        "event": EVT_NAMES.get(evt_id, "?"),
        "param": int.from_bytes(pkt[19:21], "big"),
        "condition": LIMIT_STATES.get(pkt[21], str(pkt[21])) if evt_id == 1 else str(pkt[21]),
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("log", help="target console log")
    ap.add_argument("--expect-packets", type=int, default=4)
    ap.add_argument("--expect-events", type=int, default=0,
                    help="FDIR event reports expected (nominal run: 0)")
    args = ap.parse_args()

    lines = open(args.log, encoding="utf-8", errors="replace").read().splitlines()
    errors, packets, events = [], [], []
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
            if len(pkt) >= 2 and int.from_bytes(pkt[0:2], "big") & 0x7FF == EVT_APID:
                info = check_event(pkt, errors, len(events))
                if info is not None:
                    events.append(info)
                continue
            info = check_packet(pkt, errors, len(packets))
            if info is not None:
                info["line"] = n
                packets.append(info)

    print(f"{'#':>2} {'seq':>4} {'CUC time (s)':>13} {'cycle':>5}  stale parameters")
    for i, p in enumerate(packets):
        stale = ", ".join(f"{s:#06x}" for s in p["stale"]) or "-"
        print(f"{i:>2} {p['seq']:>4} {p['time']:>13.6f} {str(p['cycle']):>5}  {stale}")

    for e in events:
        print(f"event seq {e['seq']} t={e['time']:.6f} [{e['severity']}] {e['event']} "
              f"param {e['param']:#06x} condition {e['condition']}")
    if len(events) != args.expect_events:
        errors.append(f"{len(events)} FDIR events, expected {args.expect_events}")

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
    print(f"PASS: {len(packets)} TM[3,25] packets and {len(events)} TM[5,x] events "
          "decoded and checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())

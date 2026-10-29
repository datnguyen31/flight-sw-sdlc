#!/usr/bin/env python3
"""Telemetry capture and pass/fail checker for the HK-TM system tests ST-01..ST-10.

Reads the target console log of the system test image (test/system/st_main.c)
and checks each scenario of verification/system-test-spec.md against what the
service emitted: TM packets (decoded with the ground decoder's header and
event checks, tools/hk_tm_decode.py), command verification lines ("[cmd]") and
the collection task's cycle status lines ("[hk] cycle n: ..."). The rig's own
"[ST]" markers only split the log into scenarios, and its "[fault]" lines only
say when a fault started; no verdict is taken from rig output.

A TM line is printed during a collection release, before that release's
"[hk] cycle n" line, so each packet is attributed to the next cycle line.

Exit status is 0 only if every scenario and every global check passes.
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools"))
from hk_tm_decode import (  # noqa: E402  (ground decoder, reused for header/PEC/event checks)
    APID, EVT_APID, LIMITS, PACKET_LEN, PARAM_COUNT, PARAM_IDS, STALE_MARKER,
    check_event, check_header)

HK_DEBOUNCE_N = 3      # HKS-FD-030 default
HK_STALE_M = 3         # HKS-FD-070 default
REPORT_DIVIDER = 4     # HKS-SR-060: 4000 ms / 1000 ms
TIME_TOLERANCE_S = 0.010

RE_MARK = re.compile(r"^\[ST\] (ST-\d\d)\b.*: (begin|end)$")
RE_FAULT = re.compile(r"^\[fault\] param 0x([0-9A-F]{4}) := (\S+)$")
RE_HK = re.compile(r"^\[hk\] cycle\s+(\d+): (.*?) \(overruns (\d+)\)( \[SAFE MODE\])?$")
RE_CMD = re.compile(r"^\[cmd\] (.*)$")
RE_TIMING = re.compile(r"^\[timing\] (.*)$")


def nominal(i):
    """The rig provider's nominal value for report position i (st_param_provider.c)."""
    _, low_warn, high_warn, _ = LIMITS[i]
    return (low_warn + high_warn) // 2 + i


def parse(path):
    """The log as a list of items, each a dict with 'kind' and the line number."""
    items, decode_errors = [], []
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = [ln.rstrip("\r\n") for ln in f]
    for n, line in enumerate(lines, 1):
        m = RE_MARK.match(line)
        if m:
            items.append({"kind": m.group(2), "st": m.group(1), "line": n})
            continue
        m = RE_FAULT.match(line)
        if m:
            items.append({"kind": "fault", "param": int(m.group(1), 16), "what": m.group(2),
                          "line": n})
            continue
        m = RE_HK.match(line)
        if m:
            items.append({"kind": "hk", "cycle": int(m.group(1)), "step": m.group(2),
                          "overruns": int(m.group(3)), "safe": m.group(4) is not None,
                          "line": n})
            continue
        m = RE_CMD.match(line)
        if m:
            items.append({"kind": "cmd", "text": m.group(1), "line": n})
            continue
        m = RE_TIMING.match(line)
        if m:
            items.append({"kind": "timing", "text": m.group(1), "line": n})
            continue
        if line.startswith("TM "):
            try:
                pkt = bytes.fromhex(line[3:].strip())
            except ValueError:
                decode_errors.append(f"line {n}: malformed TM line")
                continue
            items.append(decode_tm(pkt, n, decode_errors))

    # Attribute each packet to the collection release that emitted it.
    pending = []
    for it in items:
        if it["kind"] in ("hkpkt", "evt"):
            pending.append(it)
        elif it["kind"] == "hk":
            for p in pending:
                p["cycle"] = it["cycle"]
            pending = []
    return items, decode_errors


def decode_tm(pkt, n, errors):
    def fail(msg):
        errors.append(f"line {n}: {msg}")

    apid = int.from_bytes(pkt[0:2], "big") & 0x7FF if len(pkt) >= 2 else None
    if apid == EVT_APID:
        info = check_event(pkt, errors, f"at line {n}") or {}
        info.update({"kind": "evt", "line": n, "raw": pkt})
        return info
    seq = check_header(pkt, fail, APID, PACKET_LEN)
    info = {"kind": "hkpkt", "line": n, "raw": pkt, "seq": seq}
    if seq is None:
        return info
    if pkt[6] != 0x20 or pkt[7] != 3 or pkt[8] != 25:
        fail(f"secondary header {pkt[6]:#04x} [{pkt[7]},{pkt[8]}], expected 0x20 [3,25]")
    info["time"] = int.from_bytes(pkt[11:15], "big") + int.from_bytes(pkt[15:17], "big") / 65536.0
    info["sid"] = pkt[17]
    info["values"] = [int.from_bytes(pkt[18 + 4 * i: 22 + 4 * i], "big") for i in range(PARAM_COUNT)]
    info["stale"] = [PARAM_IDS[i] for i, v in enumerate(info["values"]) if v == STALE_MARKER]
    return info


class Scenario:
    def __init__(self, st, items):
        self.st = st
        self.items = items
        self.failures = []
        self.evidence = []

    def of(self, kind):
        return [it for it in self.items if it["kind"] == kind]

    def check(self, cond, msg):
        if not cond:
            self.failures.append(msg)
        return cond

    def cmd_line(self, text):
        """The first command verification line containing text, or None."""
        return next((c for c in self.of("cmd") if text in c["text"]), None)

    def hk_between(self, first_line, last_line):
        return [h for h in self.of("hk") if first_line < h["line"] < last_line]

    def fault(self, what=None):
        return [f for f in self.of("fault") if what is None or f["what"] == what]


def st03(s):
    hk = s.of("hk")
    ticks = [h for h in hk if h["step"] == "report tick, reporting disabled"]
    s.check(len(hk) >= 2 * REPORT_DIVIDER, f"only {len(hk)} collection cycles observed")
    s.check(not s.of("hkpkt"), f"{len(s.of('hkpkt'))} HK packets emitted with no command")
    s.check(not s.of("evt"), f"{len(s.of('evt'))} events emitted with nominal values")
    s.check(not s.of("cmd"), "a telecommand was processed")
    s.check(len(ticks) >= 2, f"{len(ticks)} report ticks seen, expected >= 2 suppressed")
    s.evidence.append(f"{len(hk)} cycles (1..{hk[-1]['cycle'] if hk else '-'}), "
                      f"{len(ticks)} report ticks suppressed, 0 TM packets")


def st01(s):
    acc = s.cmd_line("telecommand 1 accepted")
    s.check(acc is not None, "ENABLE_REPORTS not accepted")
    pkts = s.of("hkpkt")
    s.check(len(pkts) >= 3, f"{len(pkts)} HK packets, expected >= 3")
    cycles = [p.get("cycle") for p in pkts]
    s.check(all(b - a == REPORT_DIVIDER for a, b in zip(cycles, cycles[1:])),
            f"packet cycles {cycles} are not every {REPORT_DIVIDER}th")
    sent = [h["cycle"] for h in s.of("hk") if h["step"] == "report sent"]
    s.check(sent == cycles, f"'report sent' cycles {sent} differ from packet cycles {cycles}")
    others = [h for h in s.of("hk") if acc and h["line"] > acc["line"] and h["cycle"] not in cycles]
    s.check(all(h["step"] == "collected" for h in others),
            "a non-report cycle did something other than collect")
    for p in pkts:
        s.check(p["raw"][7:9] == bytes([3, 25]), f"packet seq {p['seq']} is not TM[3,25]")
    times = [p["time"] for p in pkts]
    intervals = [b - a for a, b in zip(times, times[1:])]
    s.check(all(abs(t - 4.0) <= 0.4 for t in intervals), f"intervals {intervals}")
    s.check(not s.of("evt"), "unexpected event")
    s.evidence.append(f"ENABLE accepted; {len(pkts)} TM[3,25] at cycles {cycles}, "
                      f"seq {[p['seq'] for p in pkts]}, intervals "
                      f"{', '.join(f'{t:.6f}' for t in intervals)} s")


def st05(s, prev_seq):
    pkts = s.of("hkpkt")
    if not s.check(len(pkts) == 1, f"{len(pkts)} HK packets, expected exactly 1"):
        return
    p = pkts[0]
    raw = p["raw"]
    s.check(len(raw) == 148, f"length {len(raw)}")
    s.check(raw[0:2] == bytes([0x08, 0xCA]), f"packet ID {raw[0:2].hex()}, expected 08ca")
    s.check(raw[2] >> 6 == 3, "sequence flags not unsegmented")
    s.check(prev_seq is None or p["seq"] == (prev_seq + 1) & 0x3FFF,
            f"seq {p['seq']} does not follow {prev_seq}")
    s.check(int.from_bytes(raw[4:6], "big") == 141, "packet data length not 141")
    s.check(raw[6] == 0x20, f"PUS version byte {raw[6]:#04x}")
    s.check(raw[7:9] == bytes([3, 25]), f"service/subtype {raw[7]}/{raw[8]}")
    s.check(raw[9:11] == bytes([0, 0]), "message type counter not 0")
    s.check(p["sid"] == 1, f"SID {p['sid']}")
    # Collection time: release k is at about k - 1 s since boot (CUC epoch is boot).
    expect_t = p["cycle"] - 1
    s.check(abs(p["time"] - expect_t) <= TIME_TOLERANCE_S,
            f"CUC {p['time']:.6f} s not the collection time of cycle {p['cycle']} (~{expect_t} s)")
    wrong = [i for i in range(PARAM_COUNT) if p["values"][i] != nominal(i)]
    s.check(not wrong, f"values out of order / wrong at positions {wrong}")
    s.evidence.append(f"seq {p['seq']}, packet ID 0x08CA, length field 141, PUS 0x20 [3,25], "
                      f"msg counter 0, SID 1, CUC {p['time']:.6f} s for cycle {p['cycle']}, "
                      f"32 values = nominal(i) in report order, PEC valid")


def st04(s):
    mal = s.cmd_line("malformed telecommand (1 bytes) REJECTED")
    unk = s.cmd_line("telecommand 127 REJECTED")
    s.check(mal is not None, "truncated frame not reported REJECTED")
    s.check(unk is not None, "unknown telecommand 0x7F not reported REJECTED")
    s.check(not s.cmd_line("accepted"), "a telecommand was accepted")
    last = max(mal["line"] if mal else 0, unk["line"] if unk else 0)
    after = [p for p in s.of("hkpkt") if p["line"] > last]
    s.check(len(after) >= 1, "reporting stopped after the rejected telecommands")
    ticks = [h for h in s.of("hk") if h["step"].startswith("report")]
    s.check(all(h["step"] == "report sent" for h in ticks),
            f"report ticks {[h['step'] for h in ticks]}: reporting state changed")
    s.check(not any(h["safe"] for h in s.of("hk")), "safe mode changed")
    s.evidence.append(f"truncated frame and 0x7F both REJECTED; "
                      f"{len(after)} HK packet(s) after them (seq "
                      f"{[p['seq'] for p in after]}), every report tick sent")


def st02(s):
    acc = s.cmd_line("telecommand 2 accepted")
    if not s.check(acc is not None, "DISABLE_REPORTS not accepted"):
        return
    late = [p for p in s.of("hkpkt") if p["line"] > acc["line"]]
    s.check(not late, f"{len(late)} HK packets after DISABLE")
    after = [h for h in s.of("hk") if h["line"] > acc["line"]]
    ticks = [i for i, h in enumerate(after) if h["step"].startswith("report")]
    s.check(len(ticks) >= 2, f"{len(ticks)} report ticks after DISABLE, expected >= 2")
    if ticks:
        s.check(ticks[0] < REPORT_DIVIDER, "first report tick after DISABLE not within one interval")
        s.check(all(after[i]["step"] == "report tick, reporting disabled" for i in ticks),
                "a report tick after DISABLE was not suppressed")
        s.evidence.append(f"DISABLE accepted; first report tick {ticks[0] + 1} cycle(s) later "
                          f"(cycle {after[ticks[0]]['cycle']}) suppressed; {len(ticks)} ticks "
                          "suppressed, 0 packets after DISABLE")


def fault_window(s):
    """(start fault item, clear fault item, hk cycles sampled under the fault)."""
    start = next((f for f in s.of("fault") if f["what"] != "nominal"), None)
    clear = next((f for f in s.fault("nominal")), None)
    if start is None or clear is None:
        s.failures.append("fault injection lines missing")
        return None, None, []
    return start, clear, s.hk_between(start["line"], clear["line"])


def st10(s):
    """One single out-of-limit sample, then a run of N - 1: neither may confirm."""
    starts = [f for f in s.of("fault") if f["what"] != "nominal"]
    clears = s.fault("nominal")
    if not s.check(len(starts) == 2 and len(clears) == 2, "expected two fault windows"):
        return
    runs = [len(s.hk_between(a["line"], b["line"])) for a, b in zip(starts, clears)]
    s.check(runs == [1, HK_DEBOUNCE_N - 1],
            f"samples under fault {runs}, expected [1, {HK_DEBOUNCE_N - 1}]")
    s.check(not s.of("evt"), f"{len(s.of('evt'))} events from transients shorter than N")
    s.check(not any(h["safe"] for h in s.of("hk")), "safe mode entered on a transient")
    s.evidence.append(f"param {starts[0]['param']:#06x} := {starts[0]['what']} (beyond hard "
                      f"limit) for {runs[0]} sample, then for {runs[1]} consecutive samples "
                      "(N - 1); 0 events, safe mode inactive")


def first_event_after(s, start, evts, n):
    """Cycles sampled under the fault before the cycle that emitted the event (expect n - 1)."""
    return len(s.hk_between(start["line"], evts[0]["line"])) if evts else None


def st06(s):
    s.check(s.cmd_line("telecommand 1 accepted") is not None, "ENABLE_REPORTS not accepted")
    start, clear, under = fault_window(s)
    if start is None:
        return
    evts = s.of("evt")
    s.check(len(evts) == 1, f"{len(evts)} events, expected 1 stale event")
    if evts:
        e = evts[0]
        s.check((e.get("event"), e.get("param"), e.get("severity"), e.get("condition")) ==
                ("stale", start["param"], "medium", "1"),
                f"event {e.get('event')} param {e.get('param')} [{e.get('severity')}] "
                f"condition {e.get('condition')}")
        before = first_event_after(s, start, evts, HK_STALE_M)
        s.check(before == HK_STALE_M - 1,
                f"stale event on stale sample {before + 1}, expected sample {HK_STALE_M}")
    marked = [p for p in s.of("hkpkt") if start["line"] < p["line"] < clear["line"]]
    s.check(len(marked) >= 1, "no HK packet during the stale window")
    s.check(all(p["stale"] == [start["param"]] for p in marked),
            f"stale markers {[p['stale'] for p in marked]}, expected only {start['param']:#06x}")
    after = [p for p in s.of("hkpkt") if p["line"] > clear["line"]]
    s.check(len(after) >= 1 and not after[-1]["stale"], "stale marker not gone after recovery")
    s.check(not any(h["safe"] for h in s.of("hk")), "safe mode entered on a stale parameter")
    if evts:
        s.evidence.append(
            f"param {start['param']:#06x} stale for {len(under)} cycles; packets seq "
            f"{[p['seq'] for p in marked]} carry 0xFFFFFFFF for it only; one TM[5,3] stale event "
            f"(cycle {evts[0].get('cycle')}, stale sample {HK_STALE_M}); marker gone in seq "
            f"{after[-1]['seq'] if after else '-'}")


def limit_scenario(s, severity, cond, expect_safe):
    start, clear, under = fault_window(s)
    if start is None:
        return
    evts = s.of("evt")
    want = 2 if expect_safe else 1
    s.check(len(evts) == want, f"{len(evts)} events, expected {want}")
    if evts:
        e = evts[0]
        s.check((e.get("event"), e.get("param"), e.get("severity"), e.get("condition")) ==
                ("limit", start["param"], severity, cond),
                f"event {e.get('event')} param {e.get('param')} [{e.get('severity')}] "
                f"condition {e.get('condition')}")
        before = first_event_after(s, start, evts, HK_DEBOUNCE_N)
        s.check(before == HK_DEBOUNCE_N - 1,
                f"limit event on out-of-limit sample {before + 1}, expected {HK_DEBOUNCE_N}")
    hk = s.of("hk")
    if expect_safe and len(evts) >= 2:
        e0, e1 = evts[0], evts[1]
        s.check((e1.get("event"), e1.get("severity"), e1.get("param")) == ("safe mode", "high", 0),
                f"second event {e1.get('event')} [{e1.get('severity')}]")
        s.check(e1.get("time") == e0.get("time"), "safe-mode event not stamped with the limit event")
        cyc = e0.get("cycle")
        s.check(all(h["safe"] == (h["cycle"] >= cyc) for h in hk),
                "safe-mode indicator not set exactly from the confirming cycle on")
        held = [h for h in hk if h["line"] > clear["line"]]
        s.check(len(held) >= HK_DEBOUNCE_N and all(h["safe"] for h in held),
                "safe mode did not hold after the condition cleared")
        s.evidence.append(
            f"param {start['param']:#06x} := {start['what']} for {len(under)} cycles; TM[5,4] "
            f"limit crit high and TM[5,4] safe-mode event, same CUC, at cycle {cyc} "
            f"(sample {HK_DEBOUNCE_N}); [SAFE MODE] from cycle {cyc}, still set "
            f"{len(held)} cycles after the value returned to nominal")
    elif evts and not expect_safe:
        s.check(not any(h["safe"] for h in hk), "safe mode entered on a soft limit")
        s.evidence.append(
            f"param {start['param']:#06x} := {start['what']} for {len(under)} cycles; one TM[5,2] "
            f"limit warn high at cycle {evts[0].get('cycle')} (sample {HK_DEBOUNCE_N}); "
            "no safe-mode event, safe mode inactive; no event on recovery")


def st09(s):
    hk = s.of("hk")
    acc = s.cmd_line("telecommand 3 accepted")
    s.check(bool(hk) and hk[0]["safe"], "safe mode not active at entry (ST-08 precondition)")
    if not s.check(acc is not None, "CLEAR_SAFE_MODE not accepted"):
        return
    after = [h for h in hk if h["line"] > acc["line"]]
    s.check(len(after) >= 2 and not any(h["safe"] for h in after),
            "safe mode still indicated after CLEAR_SAFE_MODE")
    s.check(not s.of("evt"), "unexpected event")
    s.evidence.append(f"[SAFE MODE] at entry; CLEAR_SAFE_MODE accepted; {len(after)} following "
                      "cycles without [SAFE MODE]; no events")


SPEC = [
    ("ST-01", "SR-020, 060, 100", "Enable reporting: one valid TM[3,25] every 4th cycle"),
    ("ST-02", "SR-110", "Disable reporting: stops within one generation interval"),
    ("ST-03", "SR-120", "Power-on, no command: no packets"),
    ("ST-04", "SR-130", "Malformed and unknown telecommands rejected, state unchanged"),
    ("ST-05", "SR-050, 070, 080", "HK packet fields, SID, timestamp, parameter order"),
    ("ST-06", "SR-040, FD-070", "Stale marker in packet, then one stale event after M cycles"),
    ("ST-07", "FD-030, FD-050", "Soft limit for N cycles: one warning event, no safe mode"),
    ("ST-08", "FD-050, 060, 080", "Hard limit for N cycles: one critical event, safe mode"),
    ("ST-09", "FD-080", "Clear-safe-mode command: safe mode inactive"),
    ("ST-10", "FD-030", "Out-of-limit for 1 and for N-1 samples: no event (debounce holds)"),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("log", help="target console log of the system test image")
    ap.add_argument("--markdown", help="also write the results table to this file")
    args = ap.parse_args()

    items, global_errors = parse(args.log)

    # Split into scenarios by the rig's markers.
    segs, cur = {}, None
    for it in items:
        if it["kind"] == "begin":
            cur = it["st"]
            segs[cur] = []
        elif it["kind"] == "end":
            cur = None
        elif cur is not None:
            segs[cur].append(it)

    results = {}
    hk_seq_before_st05 = None
    for st, _, _ in SPEC:
        s = Scenario(st, segs.get(st, []))
        if st not in segs:
            s.failures.append("scenario not run (no begin marker)")
        elif st == "ST-01":
            st01(s)
            pk = s.of("hkpkt")
            hk_seq_before_st05 = pk[-1]["seq"] if pk else None
        elif st == "ST-05":
            st05(s, hk_seq_before_st05)
        else:
            {"ST-02": st02, "ST-03": st03, "ST-04": st04, "ST-06": st06, "ST-09": st09,
             "ST-10": st10,
             "ST-07": lambda x: limit_scenario(x, "low", "warn high", False),
             "ST-08": lambda x: limit_scenario(x, "high", "crit high", True)}[st](s)
        results[st] = s

    # Global checks over the whole run.
    hk_pkts = [it for it in items if it["kind"] == "hkpkt" and it["seq"] is not None]
    evts = [it for it in items if it["kind"] == "evt" and it.get("seq") is not None]
    for name, pk in (("HK", hk_pkts), ("event", evts)):
        for a, b in zip(pk, pk[1:]):
            if b["seq"] != (a["seq"] + 1) & 0x3FFF:
                global_errors.append(f"{name} sequence gap {a['seq']} -> {b['seq']}")
    overruns = max((it["overruns"] for it in items if it["kind"] == "hk"), default=0)
    if overruns:
        global_errors.append(f"{overruns} collection period overruns")
    timing = [it["text"] for it in items if it["kind"] == "timing"]

    rows = []
    for st, reqs, title in SPEC:
        s = results[st]
        verdict = "PASS" if not s.failures else "FAIL"
        rows.append((st, reqs, title, verdict, "; ".join(s.evidence + s.failures)))
        print(f"{st} [{verdict}] {title} ({reqs})")
        for e in s.evidence:
            print(f"    {e}")
        for f in s.failures:
            print(f"    FAIL: {f}")
    print(f"global: {len(hk_pkts)} HK packets, {len(evts)} events, header/PEC/sequence checks "
          f"{'PASS' if not global_errors else 'FAIL'}; max overruns {overruns}")
    for e in global_errors:
        print(f"    FAIL: {e}")
    if timing:
        print(f"timing (last record): {timing[-1]}")

    if args.markdown:
        with open(args.markdown, "w", encoding="utf-8") as f:
            f.write("| ID | Requirements | Verdict | Observed |\n|---|---|---|---|\n")
            for st, reqs, _, verdict, ev in rows:
                f.write(f"| {st} | {reqs} | **{verdict}** | {ev} |\n")

    failed = [st for st, s in results.items() if s.failures]
    if failed or global_errors:
        print(f"FAIL: {len(failed)} scenario(s) failed {failed}, {len(global_errors)} global error(s)")
        return 1
    print(f"PASS: all {len(SPEC)} system test scenarios")
    return 0


if __name__ == "__main__":
    sys.exit(main())

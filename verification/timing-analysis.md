# HK-TM Timing Analysis (HKS-SR-150, HKS-FD-090)

HKS-SR-150 requires a collection cycle to complete within 50 ms, with
verification method Analysis. This document satisfies it by summing
conservative worst-case bounds for each operation in a cycle, per the
detailed design in `design/detailed-design.md`, and showing that the total
is well under budget. Measured confirmation follows during integration
(Session 9). Until then, this analysis is the evidence that closes the
requirement by analysis.

**Revision 2 (Session 8, FDIR increment).** The FDIR monitor
(`design/fdir-design.md`) runs on every collection cycle, so HKS-SR-150 is
re-verified here with it included, not assumed to still hold. The same
analysis verifies the timing half of HKS-FD-090. The Session 5 figures are
unchanged except for one arithmetic correction, noted below.

**Session 9 note (measurement).** `verification/timing-measured.md`
measured the running system under Renode. The total is corroborated: the
worst release is 1.30 ms. The split is not: sampling and monitoring take
about a quarter of their bound here, and the output path takes two to four
times its bound, because the target's console sink formats each byte to a
polled UART, not the 50 µs queued handoff assumed below (SYS-4). A report
tick with an event measured 1300 µs against the 1286 µs these rows give,
equal within the ±10 µs measurement resolution. No
requirement is at risk.

**Revision 3 (Session 10, closes SYS-4).** The sink handoff is now bounded
per sink, in the section "Sink handoff per sink" below. The worst-case
table is unchanged: it is the bound for the flight configuration, a queued
handoff to a downlink. The console stand-in gets its own bound, from the
Session 9 measurement and from the UART line rate.

## Assumptions

- Baseline parameter count N = 32. Not yet fixed by the data model; this
  is the lever to re-run the analysis against if the parameter table
  grows.
- Collection interval 1000 ms (HKS-SR-020 default).
- Generation interval 4000 ms (HKS-SR-060 default), so a report is
  emitted every 4th collection cycle (`REPORT_DIVIDER = 4`).
- A representative onboard computer, RTEMS on the target processor
  (DD-06).
- A conservative per-parameter read bound of 20 µs through the
  parameter-source interface (`hk_param_source_read`), covering both the
  fresh and stale/unknown paths.
- FDIR: classification of one value against four limits (at most four
  comparisons) is bounded at 4 µs. The update of the four run counters and
  the level evaluation (`count_sample`, `next_level`) is bounded at
  1.5 µs. A stale sample takes the stale path instead of these, and that
  path is cheaper, so it is covered by the same bound. An event report is
  a 24-byte packet: encode and PEC are bounded at 30 µs, and the sink
  handoff at 50 µs, the same as for the housekeeping packet.

## Worst-case bound per operation

| Operation | Count | Unit WCET | Subtotal |
|---|---|---|---|
| Parameter reads (Sampler, via interface) | 32 | 20 µs | 640 µs |
| Report assembly (Builder) | 32 | 5 µs | 160 µs |
| Timestamp read and write | 1 | 10 µs | 10 µs |
| FDIR: limit classification (`hk_limit_classify`) | 32 | 4 µs | 128 µs |
| FDIR: run counters and level evaluation | 32 | 1.5 µs | 48 µs |
| **Collection cycle, worst case, no new condition (HKS-SR-150 scope)** | | | **986 µs** |
| PUS / CCSDS encode (report tick only) | 1 | 150 µs | 150 µs |
| Telemetry-sink handoff (report tick only) | 1 | 50 µs | 50 µs |
| State and loop overhead | 1 | 20 µs | 20 µs |
| **Report cycle, worst case (full tick, every 4th)** | | | **1206 µs** |
| FDIR event burst (only on a cycle where conditions confirm): encode + handoff | ≤ 33 | 80 µs | ≤ 2640 µs |
| FDIR safe-mode entry, critical section | ≤ 1 | 10 µs | ≤ 10 µs |
| **Pathological release: report tick + every parameter confirming critical at once** | | | **≤ 3856 µs** |

The encode and handoff rows apply only on the one release in four that
also builds and emits a report. Every other release stops after the
collection-cycle subtotal.

The event rows apply only on a release where a parameter's level rises
(`design/fdir-design.md` §2.2). Each parameter can rise at most one level
per cycle, a stale event excludes a limit event for that parameter in the
same cycle, and safe-mode entry reports at most once. So a single release
emits at most 32 + 1 = 33 events. `test_worst_case_event_burst_is_bounded`
in `test/unit/test_hk_monitor.c` checks this bound.

**Monitor cost against the Session 8 brief.** The brief estimated
~160 µs for the monitor (130 µs classification, 30 µs counters), giving
~970 µs per cycle. This design keeps four run counters and a level per
parameter, not one counter (DD-13), so its bound is 176 µs, giving
~986 µs. The difference is immaterial against the budget, and the
analysis uses the figure for the code as built.

**Correction to revision 1.** Revision 1 gave the report cycle as
1060 µs. The rows it listed sum to 1030 µs (810 + 150 + 50 + 20). The
conclusion did not change.

## Sink handoff per sink (revision 3)

The telemetry-sink interface (`hk_tm_sink.h`, DD-03) has more than one
implementation, and they do not cost the same. Revisions 1 and 2 assumed
only one of them.

| Sink | Where | Handoff assumption | HK packet (148 B): encode + PEC + sink | Event packet (24 B) |
|---|---|---|---|---|
| Queued downlink | Flight configuration (not yet built; DD-03) | 50 µs: copy into a downlink queue, no formatting, no wait on hardware | 220 µs (150 + 50 + 20, as in the table above) | 80 µs |
| Capture sink | Host unit tests | Memory copy | Not timed: host only | Not timed |
| Console stand-in, emulated | `hk_tm_sink_console.c` on Renode | Measured: about 6 µs per byte, encode included (`timing-measured.md` §3). Bounded here at 8 µs per byte, a third over the measurement | 1184 µs | 192 µs |
| Console stand-in, real UART | `hk_tm_sink_console.c` on LEON3 hardware | Line-rate bound. Each packet is one line, `TM` + space, two hex characters per byte and a newline, so 2*n* + 4 characters, sent by polled `printk`. At 115200 baud (8N1) one character takes 86.8 µs | 300 characters, about 26.0 ms | 52 characters, about 4.5 ms |

With the console stand-in on Renode, the heaviest ordinary release is
986 + 1184 + 20 = about 2.19 ms, and the pathological release is
2.19 + 33 × 0.192 + 0.01 = about 8.5 ms. Both are inside 50 ms. The
measured 1.30 ms falls under the 2.19 ms bound.

**The console on a real UART does not fit the budget in every release.**
A report tick costs about 26 ms of output alone, still inside 50 ms and
the 1000 ms period. One event adds about 4.5 ms, and the pathological
burst of 33 events adds about 149 ms. That burst is not "nominal load",
so it does not break HKS-SR-150 as written, but that release would take
about 176 ms, a sixth of the period. The console sink is a stand-in for
emulation and bring-up, and **must not be the flight sink.** A timing
claim on flight-representative hardware needs either the queued downlink
or the console at a much higher line rate. The table above assumes the
queued downlink, and so does every conclusion below.

The per-byte bound for the emulated console is from one measurement, on
an emulator that does not model the UART line (`timing-measured.md` §4).
It is there to explain the Session 9 figures, not to predict hardware.

## Conclusion

The collection cycle, with the FDIR monitor, completes in about 0.99 ms
against the 50 ms budget in HKS-SR-150, which leaves about 98% margin.
HKS-SR-150 is re-verified by analysis, and the timing half of HKS-FD-090
holds.

The heaviest ordinary release, one that also encodes and emits a report,
takes about 1.21 ms. The pathological release, a report tick on which
every parameter confirms critical at once, is bounded by about 3.9 ms.
That is under 8% of the 50 ms budget and under 0.4% of the 1000 ms
collection period.

Because the collection task is released by the RTEMS rate-monotonic
period service, release jitter is bounded to about one clock tick. That is
far inside the 400 ms (10% of the 4000 ms generation interval) allowed by
HKS-SR-140. Two critical sections are short enough not to materially
affect either bound: the command task's (`design/detailed-design.md`
§1.6) and the safe-mode indicator's (`design/fdir-design.md` §4).

The margin is wide enough that the assumptions above, not the
arithmetic, are what to revisit if the parameter set or the collection
interval changes.

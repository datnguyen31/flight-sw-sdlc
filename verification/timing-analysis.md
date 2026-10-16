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

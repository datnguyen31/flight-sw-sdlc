# HK-TM Timing Analysis (HKS-SR-150)

HKS-SR-150 requires a collection cycle to complete within 50 ms, with
verification method Analysis. This document satisfies it by summing
conservative worst-case bounds for each operation in a cycle, per the
detailed design in `design/detailed-design.md`, and showing the total sits
well under budget. Measured confirmation follows during integration
(Session 9); this analysis is the evidence that closes the requirement by
analysis in the meantime.

## Assumptions

- Baseline parameter count N = 32. Not yet fixed by the data model; this is
  the lever to re-run the analysis against if the parameter table grows.
- Collection interval 1000 ms (HKS-SR-020 default).
- Generation interval 4000 ms (HKS-SR-060 default), so a report is emitted
  every 4th collection cycle (`REPORT_DIVIDER = 4`).
- A representative onboard computer, RTEMS on the target processor (DD-06).
- A conservative per-parameter read bound of 20 µs through the
  parameter-source interface (`hk_param_source_read`), covering both the
  fresh and stale/unknown paths.

## Worst-case bound per operation

| Operation | Count | Unit WCET | Subtotal |
|---|---|---|---|
| Parameter reads (Sampler, via interface) | 32 | 20 µs | 640 µs |
| Report assembly (Builder) | 32 | 5 µs | 160 µs |
| Timestamp read and write | 1 | 10 µs | 10 µs |
| **Collection cycle, worst case (HKS-SR-150 scope)** | | | **810 µs** |
| PUS / CCSDS encode (report tick only) | 1 | 150 µs | 150 µs |
| Telemetry-sink handoff (report tick only) | 1 | 50 µs | 50 µs |
| State and loop overhead | 1 | 20 µs | 20 µs |
| **Report cycle, worst case (full tick, every 4th)** | | | **1060 µs** |

The encode and handoff rows apply only on the one release in four that also
builds and emits a report; every other release stops after the
collection-cycle subtotal.

## Conclusion

The collection cycle completes in about 0.81 ms against the 50 ms budget in
HKS-SR-150 — over 98% margin. Even the heaviest release, one that also
encodes and emits a report, is about 1.06 ms against the 1000 ms collection
period, roughly 0.1% processor utilization. Because the collection task is
released by the RTEMS rate-monotonic period service, release jitter is
bounded to the order of one clock tick — far inside the 400 ms (10% of the
4000 ms generation interval) allowed by HKS-SR-140. The command task's
critical section (`design/detailed-design.md` §1.6) is a few instructions
and does not materially affect either bound.

The margin is wide enough that the assumptions above, not the arithmetic,
are what to revisit if the parameter set or the collection interval
changes.

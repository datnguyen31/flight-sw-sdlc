# Housekeeping Telemetry — FDIR Increment Requirements

The first increment after the baseline (`docs/software-development-plan.md`
§4, Session 8): onboard limit monitoring, event reporting, and an autonomous
safe-mode transition. These requirements are added alongside the baseline
in `requirements/hk-tm-baseline.md`. They do not modify any baseline
requirement; the baseline stays frozen at `req-baseline-v0.1`.

Verification methods: **T** = Test, **A** = Analysis, **I** = Inspection, **R** = Review of design.

## System-level context requirement

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-SYS-040 | The spacecraft shall autonomously detect and respond to out-of-limit housekeeping conditions. | Faults can develop faster than a ground contact allows operators to react. | T | Mission |

## Software requirements: FDIR

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-FD-010 | The FDIR function shall maintain, for each monitored parameter, a soft and a hard limit on both the low and high side. | Two severities let the response be proportionate: report a drift, act on a danger. | T | SYS-040 |
| HKS-FD-020 | The FDIR function shall check each monitored parameter against its limits once per collection cycle. | Monitoring at the sampling rate means no sample goes unchecked and adds no new timing source. | T | SYS-040 |
| HKS-FD-030 | The FDIR function shall confirm a limit condition only after N consecutive out-of-limit samples, N configurable, default 3. | A single noisy sample must not trip a fault response. | T | SYS-040 |
| HKS-FD-040 | On a confirmed limit condition, the FDIR function shall emit a PUS Service 5 event report identifying the parameter and the condition. | Operators learn of the condition from the event, without having to find it in the housekeeping stream. | T | SYS-040 |
| HKS-FD-050 | The FDIR function shall classify a confirmed condition as warning (soft limit) or critical (hard limit) and set the event severity accordingly. | Ground tools filter and alarm on severity. | T | SYS-040 |
| HKS-FD-060 | On a confirmed critical condition, the FDIR function shall request a transition to safe mode. | A hard-limit violation is the case that cannot wait for ground. | T | SYS-040 |
| HKS-FD-070 | The FDIR function shall treat a parameter stale for M consecutive cycles (default 3) as a fault and emit an event. | A source that stays silent is itself a fault, and limit checking cannot see it (HKS-SR-040 records the stale marker but raises nothing). | T | SYS-040 |
| HKS-FD-080 | Entering safe mode shall set a system safe-mode indicator and emit a safe-mode event; recovery from safe mode shall be by ground command only. | See note below. | T | SYS-040 |
| HKS-FD-090 | The FDIR function shall perform no dynamic allocation and shall execute within the collection task timing budget. | Keeps HKS-SR-150 and HKS-SR-160 true with FDIR included. | A, I | SYS-040 |

**Note on HKS-FD-080.** Recovery is deliberately ground-commanded, not
automatic. If recovery were autonomous, the spacecraft could go in and out
of safe mode repeatedly while a real fault persists. That is worse than
staying safe and waiting for operators.

**Scope boundary.** Entering safe mode here sets the indicator and reports
it. The actual safing actions, such as powering down non-essential loads or
reorienting, are system-level behaviour beyond this software unit. The code
marks the hook where a real system would invoke them.

## Baseline requirements affected

No baseline requirement text changes. HKS-SR-150 (collection cycle within
50 ms) must be re-verified, because the monitor adds work to every
collection cycle (`verification/timing-analysis.md`). HKS-SR-130 (reject
unknown or malformed telecommands) still applies unchanged to the command
set, which gains one opcode, `CLEAR_SAFE_MODE` (HKS-FD-080).

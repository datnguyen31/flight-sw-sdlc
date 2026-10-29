# HK-TM Verification and Validation Report (v1.0)

This report answers one question for a reviewer: **is every requirement
verified, and by what?** It also answers the question before that one:
**were these the right requirements for the mission?** (§6).

The report adds no new verification. It brings together evidence already
in the repository, produced from Session 3 to Session 9, and states what
that evidence closes and what it leaves open. The requirement-by-requirement
detail is in `verification/traceability-matrix.md`. This report summarizes
it and does not replace it.

| Item | Value |
|---|---|
| Product | Housekeeping Telemetry Service (HK-TM), with the FDIR increment |
| Baseline under report | branch `session10-closure`; flight and test sources identical to tag `vv-v0.1` (`git diff vv-v0.1 -- src test tools cmake CMakeLists.txt` is empty) |
| Requirements | `requirements/hk-tm-baseline.md` (tag `req-baseline-v0.1`), `requirements/fdir-increment.md` (tag `fdir-v0.1`): 4 system, 18 software, 9 FDIR, **31 in total** |
| Target | RTEMS 7, BSP sparc/leon3, on Renode 1.17.0 (DD-09). No flight-representative hardware |
| Author | Dat Nguyen (developer) |
| Review authority | Mentor |
| Date | 2026-10-29 |

## 1. Verdict

| Status | Count | Requirements |
|---|---|---|
| Verified | **29** | HKS-SYS-010 to 040; HKS-SR-010 to 130, 150, 160, 180; HKS-FD-010 to 090 |
| Partial | **2** | HKS-SR-140 (jitter: hardware measurement outstanding); HKS-SR-170 (coding standard: MISRA rule check not in the gate) |
| Failed | 0 | |
| Independently checked (ISVV) | 0 | ISVV is planned for 5 items. Execution is with the mentor (§4) |

Nothing is claimed as verified beyond what the evidence shows. The two
partial items and the pending ISVV are recorded in §5 with a closure
action each. They are not closed quietly.

## 2. Verification by requirement group

| Requirement group | Assigned method | Evidence | Status |
|---|---|---|---|
| System needs HKS-SYS-010 to 040 | Test | System tests ST-01 to ST-10 on target (Session 9) | Verified |
| Acquisition HKS-SR-010 to 040 | R, T, R, T | Design review (S4, PR #2); unit tests (S7); ST-01, ST-06 | Verified |
| Reporting HKS-SR-050 to 090 | R, T, T, I, R | Data model review (S4); encoder inspection against CCSDS 133.0-B and ECSS-E-ST-70-41C (S4, S7); byte-exact unit vector; independent ground decoder on every packet; ST-01, ST-05 | Verified |
| Command and state HKS-SR-100 to 130 | Test | Unit tests (S6, S8); ST-01 to ST-04 | Verified |
| Jitter HKS-SR-140 | Test | Rate-monotonic release by design; 0 µs measured at 10 µs resolution over 480 releases on Renode, 0 overruns (S9). Emulated time is deterministic, so this is indicative only | **Partial** |
| Collection timing HKS-SR-150 | Analysis | `verification/timing-analysis.md` rev. 1 (S5), rev. 2 with FDIR (S8), rev. 3 per-sink (S10); measured worst cycle 0.57 ms against 50 ms (S9) | Verified, corroborated by measurement |
| No dynamic allocation HKS-SR-160 | Inspection | Inspection of `src/app`, `src/fdir`; static task and queue storage; Renode `malloc` hook: no call after startup | Verified |
| Coding standard HKS-SR-170 | Inspection | Strict compiler flags with `-Werror` on all flight code; `cppcheck --enable=all --inconclusive` in CI, clean. MISRA addon not in CI (see below) | **Partial** |
| Tasks HKS-SR-180 | Review | Task design review (S4, S5, PRs #2, #3); inspection of `tasks/hk_tasks.h`, `rtems/init.c` | Verified |
| FDIR HKS-FD-010 to 080 | Test | Four FDIR unit suites plus FDIR cases in two others (S8); ST-06 to ST-10 | Verified |
| FDIR resources HKS-FD-090 | Analysis, Inspection | Inspection of `src/fdir`; timing analysis rev. 2 and 3; measured releases include FDIR events | Verified |

The assigned method is the one each requirement was baselined with. Where
tests were added on top of a Review or Inspection, the requirement is still
closed by its assigned method, and the tests are supplementary
(`verification/traceability-matrix.md`, "Verification-method control").

**Why HKS-SR-170 is Partial.** `docs/coding-standard.md` §3 names
`cppcheck --addon=misra` and `clang-tidy` as the gate. CI runs cppcheck
without the MISRA addon, and installs clang-tidy without running it. For
this report the MISRA addon was run by hand (cppcheck 2.7, no rule texts,
`src/app` and `src/fdir`):

| Category | Findings | Rules |
|---|---|---|
| Required | 6 | 8.4 and 21.8 in `rtems/init.c` (RTEMS `Init` entry point has no prior declaration; it calls `exit()`); 10.8 ×2 and 12.2 ×2 in `hk_pus_encoder.c` (cast of a composite expression; shift amount not provably in range to the tool) |
| Advisory | 99 | 2.5, 8.7, 15.5, 13.3, 8.9, 5.9, 12.1, 20.1 |

The coding standard requires every Required finding to be fixed or carried
as a logged deviation with a reviewer sign-off, and its deviation log is
empty. None of the six findings is a defect in behaviour: the encoder's
output is checked byte-exact by unit test and by an independent decoder on
every packet, and the `init.c` findings are RTEMS integration conventions.
They are still non-conformances against the written standard, so the
requirement is Partial (OI-2).

## 3. Evidence inventory

| Level | Evidence | Where | Result |
|---|---|---|---|
| Unit | 10 Unity suites, 66 test cases, host build | `test/unit/`, `ctest --test-dir build` | 66/66 pass (re-run 2026-10-29) |
| Static | Compiler `-Wall -Wextra -Werror -Wconversion -Wshadow -Wpedantic`; cppcheck gate | `CMakeLists.txt`, `.github/workflows/ci.yml` | Clean (re-run 2026-10-29) |
| Integration | Flight image end to end on Renode, scripted ground station, independent decoder | `tools/renode/run_hk_app.sh`, `tools/hk_tm_decode.py` | 4 packets, 0 events, 0 jitter (S9) |
| System | ST-01 to ST-10, one boot, host-side capture | `verification/system-test-spec.md`, `verification/system-test-results.md` | 10/10 pass (S9); re-run 2026-10-29 at this baseline, see §3.1 |
| System, negative | Five tampered logs and one code mutant (`HK_DEBOUNCE_N` = 2), to check the checker | `verification/system-test-results.md` §3 | Every tamper and the mutant detected |
| Analysis | Worst-case timing, rev. 3 | `verification/timing-analysis.md` | 0.99 ms against 50 ms |
| Measurement | Instrumented flight task, 480 releases | `verification/timing-measured.md` | Worst cycle 0.57 ms, worst release 1.30 ms, 0 overruns |
| Inspection, review | Design reviews through PRs #1 to #6; code inspection for SR-160, FD-090 | Pull requests; traceability matrix | Closed |
| ISVV | Risk-based sample I-1 to I-4 | `verification/isvv-record.md` | Plan ready; execution pending (mentor) |

### 3.1 Release confirmation run

The sources have not changed since `vv-v0.1`. For this report the target
image was rebuilt and the system tests re-run at the closure baseline, to
confirm the recorded result still reproduces:

```
RENODE=... test/system/run_system_tests.sh build-target/test/system/hk_system_test.exe
```

Result, 2026-10-29: **10 / 10 PASS**, global checks PASS, 0 overruns.
Every scenario's observation matches the recorded run in
`verification/system-test-results.md` §5 (16 HK packets, 4 events, the same cycles, sequence counts and CUC
values). The run's own timing record, at the default 100 µs quantum:

```
global: 16 HK packets, 4 events, header/PEC/sequence checks PASS; max overruns 0
timing (last record): cycles 60 overruns 0 collect_min 200000 ns collect_max 400000 ns (cycle 54) report_max 1100000 ns (cycle 12) jitter_max 0 ns (cycle 0)
PASS: all 10 system test scenarios
```

## 4. Independent verification (ISVV)

The ISVV sample (`verification/isvv-record.md`) was chosen by risk: the
encoder (I-1, every packet depends on it), the debounce (I-2, an
off-by-one makes FDIR fire early or never), safe mode (I-3, worst
consequence and shared between tasks), and a statement review of the
FDIR requirements (I-4). The developer wrote the plan and the
independence rules. The result columns are the mentor's, and they are
empty.

This report therefore claims **no requirement as independently checked**.
The traceability matrix shows "pending" in its ISVV column for the same
reason. If ISVV raises a defect, it reopens the affected requirement, and
v1.0 is not tagged until it is dispositioned.

## 5. Closure statement and open items

**Closure statement.** All 31 requirements of the baseline and the FDIR
increment have verification evidence by their assigned method. 29 are
verified. Two are partial, and each has a defined closure action. No
requirement failed. Four integration findings were raised during system
verification: one defect, fixed (SYS-1); one weakness in the test
specification, fixed (SYS-2); one analysis discrepancy, closed by
analysis revision 3 (SYS-4); and one operational gap that needs a
decision (SYS-3). The service is verified **for the scope stated below**,
on an emulated target.

| ID | Item | Why it is open | Closure action | Blocks v1.0? |
|---|---|---|---|---|
| OI-1 | HKS-SR-140 jitter, absolute figure | Emulated time is deterministic: 0 µs jitter says nothing about interrupt latency or contention on a real LEON3 | Run the flight image on flight-representative LEON3 hardware with a queued downlink sink, and read the DD-17 `[timing]` record (already in the flight build). Pass if `jitter_max` < 400 ms and `overruns` = 0; record the absolute SR-150 figures from the same run | No. Carried openly |
| OI-2 | HKS-SR-170 MISRA gate | MISRA addon and clang-tidy are not in CI; 6 Required findings, deviation log empty | Fix the four encoder findings (casts and shift ranges), log 8.4 and 21.8 in `init.c` as deviations with rationale and sign-off, then add `--addon=misra` with an error exit to the CI gate | No. Carried openly |
| OI-3 | ISVV execution | The plan is ready; the result columns are for the mentor | Mentor executes I-1 to I-4 and fills `verification/isvv-record.md` | Yes, if ISVV raises a defect |
| OI-4 | SYS-3, safe-mode state not in telemetry | HKS-FD-080 is met as written, but ground can confirm a clear only from command verification and the console | Decision: an informative TM[5,1] "safe mode exited" event (new requirement, no change to the HK report), or a status parameter in the report (changes the frozen SR-050 structure). Recommended: the event | No. It is a requirements gap, not a defect |
| OI-5 | Target build in CI | The `target-build` job is skipped until a toolchain image is published (`RTEMS_TOOLCHAIN_IMAGE`) | Publish the image from `tools/docker/rtems7-leon3.Dockerfile` and set the variable. Until then, the target build and system tests are run by hand, as in §3.1 | No |

**Scope limitations.** These are boundaries of what the capstone claims,
not defects:

- The 32-parameter set, the limits (raw 12-bit counts, DD-14) and the
  parameter providers are representative training values. The target's
  provider is a simulator.
- The telemetry sink on the target is a console stand-in (DD-03). It is
  not a flight sink: on a real UART it would cost about 26 ms per HK
  packet (`timing-analysis.md` rev. 3).
- Safe mode sets an indicator and reports it. The safing actions (load
  shedding, attitude) are system-level behaviour beyond this unit; the
  code marks the hook (`requirements/fdir-increment.md`, "Scope
  boundary").
- Timing figures from Renode are indicative. Renode models no caches,
  wait states or bus contention, and its UART transmits instantly.
- The BSP is the standard RTEMS leon3 BSP with two boot-loader duties done
  by the Renode script (DD-09), not a mission BSP.

## 6. Validation

Verification asked whether the service meets its requirements. Validation
asks whether those were the right requirements: does the service, as
built, give the mission what it needed? The yardstick is the four
system-level needs, which were written from the mission side
(Session 3 for SYS-010 to 030, Session 8 for SYS-040).

| Mission need | Demonstrated behaviour | Evidence | Validated |
|---|---|---|---|
| HKS-SYS-010: operators can monitor platform health | A well-formed housekeeping report every 4 s while enabled, 32 parameters, decodable by a ground decoder written independently of the flight encoder. A failed source shows as a stale marker, and reporting carries on | ST-01, ST-05, ST-06; `tools/hk_tm_decode.py` on every packet | Yes |
| HKS-SYS-020: operators control the telemetry stream | The service starts quiet, starts reporting on ENABLE, stops on DISABLE, and rejects malformed or unknown commands visibly without a state change | ST-01 to ST-04 | Yes |
| HKS-SYS-030: telemetry is placed on the mission timeline | Every report carries the CUC time of the collection it contains, and FDIR events carry the time of the report that confirmed them (DD-12), so an event and its data line up on the ground | ST-05, ST-07, ST-08 | Yes |
| HKS-SYS-040: autonomous response to out-of-limit health | Confirmed limit conditions are reported by severity; a transient does not trip FDIR; a critical condition puts the unit in safe mode, which holds until ground clears it | ST-06 to ST-10 | Yes, with the gap in OI-4 |

**What validation found that verification did not.** Every requirement
under HKS-SYS-040 passes. But an operator who sends CLEAR_SAFE_MODE gets
"accepted" back and nothing in telemetry that shows safe mode was left
(SYS-3, OI-4). The requirement does not ask for it, so verification could
not catch it. It took asking what the operator needs. This is the kind of
finding validation exists for, and it is carried as an open item with a
recommended requirement change, not patched under the existing one.

**Who validated.** The developer did, against the written mission needs,
with the mentor as review authority. No operator or ground-segment user
took part. On a real project that is the first thing to add.

**What is deliberately out of scope**, so that expectations are set
correctly: this is one onboard function, not a flight software suite.
There is no real downlink, no configurable report definitions (deferred
by DD-05), no FDIR beyond limit and staleness monitoring, and safe mode
signals rather than performs system safing.

**Validation result.** Within the boundary above, the service does what
the mission asked of it. The right thing was built. One operational gap
is known and has a proposed fix.

## 7. Sign-off

| Role | Name | Decision | Date |
|---|---|---|---|
| Developer | Dat Nguyen | Submitted for closure review | 2026-10-29 |
| Review authority | *mentor* | | |
| Independent verifier (ISVV) | *mentor* | | |

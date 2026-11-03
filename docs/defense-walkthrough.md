# Oral Defense Walkthrough (v1.0)

Speaker notes for the capstone defense, a qualification review in
miniature. The traceability matrix is the spine: start from a
requirement, follow it to design, code, test and result, and the
structure of the whole program shows in one move. The aim is to defend
decisions, not describe features.

About 20 minutes of walkthrough, then questions.

## 1. Opening (2 min)

> I built an onboard housekeeping telemetry service under a full,
> traceable life cycle, with fault management and an independent
> verification plan. It runs on RTEMS on an emulated LEON3. 31
> requirements: 29 verified, 2 partial with closure actions, none
> failed. I'll show you one requirement end to end, then the parts I'd
> expect you to push on.

Show: `docs/technical-report.md` §3 figure.

## 2. One requirement, end to end (6 min)

Use **HKS-FD-030** (debounce, N = 3). It crosses every level and has the
best story.

| Step | Show | Say |
|---|---|---|
| Requirement | `requirements/fdir-increment.md` | "Confirm only after N consecutive samples, N configurable, default 3. Rationale: one noisy sample must not trip a response." |
| Design | `design/fdir-design.md` §2.1, DD-13 | Four saturating run counters and a confirmed level. Why I departed from the reference sketch: taken literally, it re-reports every cycle and cannot escalate. |
| Code | `src/fdir/src/hk_monitor.c`, `HK_DEBOUNCE_N` in `hk_monitor.h` | Requirement ID in the header comment. |
| Unit test | `test/unit/test_hk_monitor.c`: `test_debounce_confirms_on_Nth_sample`, `test_interrupted_run_does_not_confirm` | Boundaries at N − 1, N, and an interrupted run. |
| System test | ST-07, ST-08 (fires on sample 3), ST-10 (1 and N − 1 samples: nothing) | On the target, verdict from telemetry only. |
| The test of the test | `verification/system-test-results.md` §3 | Mutant N = 2: the brief's ST-10 still passed. That is SYS-2. I extended ST-10, and the mutant now fails. |
| ISVV | I-2 in `verification/isvv-record.md` | Independent boundary tests, without access to my tests. |
| Status | Traceability matrix row FD-030 | Verified (Test), I-2 pending. |

## 3. Three things I want you to see (6 min)

1. **Measurement corrected the analysis** (SYS-4). The total agreed, the
   split did not. Revision 3 bounds the sink per implementation and shows
   the console stand-in would cost about 26 ms per packet on a real UART.
   `verification/timing-measured.md` §3, `verification/timing-analysis.md`
   "Sink handoff per sink".
2. **Validation found what verification could not** (SYS-3). Every FDIR
   requirement passes, yet an operator cannot see in telemetry that safe
   mode was cleared. It is carried as a requirements gap with a
   recommended fix (OI-4), not patched quietly.
3. **The report names its limits.** SR-140 and SR-170 are Partial, ISVV
   is unexecuted, and the target build is not in CI yet. Each has a
   closure action. `docs/verification-report.md` §5.

## 4. Likely questions

| Question | Answer | Evidence |
|---|---|---|
| Why one collection task, not two? | DD-01. One period, one timing source; a divider makes each report align to a completed collection, by construction. Two tasks would need synchronization to get the same guarantee. | DD-01; `hk_collection.c`; ST-01 intervals 4.000000 s |
| How do you know the encoder is right? | Inspection against the standards (assigned method), a byte-exact 148-byte unit vector, and a ground decoder written separately that checks every packet on the target. I-1 asks the mentor to derive a packet by hand. | SR-080 row; `test_encoder_matches_known_vector`; `tools/hk_tm_decode.py` |
| Why doesn't the command handler set the flag directly? | DD-07. The manager owns its state and its critical section. One owner for the one piece of shared state, behind a priority-inheritance semaphore (DD-11). | DD-07, DD-11; `hk_manager.c` |
| What stops a transient from tripping FDIR? | Debounce N = 3 (FD-030). ST-07 and ST-08 show it fires on the 3rd sample; ST-10 shows 1 and 2 samples do nothing. Both directions are needed: either alone passes a wrong N. | ST-07, ST-08, ST-10; SYS-2 |
| Why is safe-mode recovery not automatic? | FD-080. Automatic recovery while a fault persists makes the spacecraft oscillate in and out of safe mode. Staying safe and waiting for ground is the lesser harm. | FD-080 note; ST-09; `test_clear_during_persisting_condition_holds` |
| How far do you trust your timing numbers? | The analysis closes SR-150 with 98 % margin. The measurement corroborates the total but comes from an emulator with deterministic time, no caches and an instant UART. Absolute figures and jitter need hardware: OI-1. | `timing-analysis.md`; `timing-measured.md` §4 |
| Why is SR-170 only Partial? | The six Required findings are fixed in code, not deviated, because each had a compliant form with the same behaviour. A MISRA gate in CI now fails on any Mandatory or Required finding. It stays Partial because cppcheck's addon checks only part of the rule set and clang-tidy is not run, so a clean gate is not a full conformance claim. | Verification report §2, OI-2; `ci/misra_gate.py` |
| Your ISVV column is empty. Is anything independently verified? | No, and the report says so. The plan, sample and independence rules are ready; the results are the verifier's to write. If it raises a defect, v1.0 waits. | `isvv-record.md`; OI-3 |
| Why Renode, not QEMU? | Both model LEON3. Renode gave what the verification needed: reproducible instruction-driven time (two system-test runs three weeks apart match exactly), scriptable boot state and hooks (the `malloc` hook is the SR-160 evidence), and a platform described as text, so adding a peripheral is easy. Costs: two boot-loader duties and memory growth on long runs. Neither emulator models caches or contention, so OI-1 stands either way. Nothing in the flight code depends on Renode. | DD-09, DD-18; `tools/renode/hk_leon3.resc`; verification report §3.1 |
| Why a separate APID for events? | DD-12. CCSDS counts sequence per APID. Sharing 0x0CA would have changed the verified HK stream. | DD-12 |
| What would you do before flight? | Measure on hardware with a queued downlink sink (OI-1); full MISRA conformance with a qualified checker and clang-tidy (OI-2); execute ISVV (OI-3); decide SYS-3 (OI-4); a mission BSP; system-level safing behind the hook. | Verification report §5; technical report §8 |

## 5. Close (1 min)

> The code shows I can build it. The life cycle shows I can lead the
> building: requirements that carry their reasons, decisions with their
> alternatives, tests that were tested, and a report that names what it
> cannot claim.

## Materials to have open

- `verification/traceability-matrix.md`
- `docs/verification-report.md`
- `docs/technical-report.md`
- A terminal on the repository, to run `ctest --test-dir build` live
  and, if time allows, `test/system/run_system_tests.sh` (about 90 s of
  virtual time on Renode)

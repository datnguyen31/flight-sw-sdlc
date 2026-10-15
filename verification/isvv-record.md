# HK-TM ISVV Record (Session 9)

Independent software verification and validation of a risk-based sample.
Everything else in `verification/` was verified by the developer. This
record holds checks made by someone else, by independent means.

| Item | Value |
|---|---|
| Developer | Dat Nguyen |
| Independent verifier | *mentor (name)* |
| Baseline under ISVV | tag `vv-v0.1` (branch `session9-vandv`) |
| Status | **Plan prepared by the developer. Execution and results: mentor.** |

**Independence rule for this record.** The developer wrote the
*Sample*, *Why* and *Independent method* columns and the materials list,
because they follow from the Session 9 brief. The developer wrote nothing
in the *Result*, *Finding* or *Verdict* columns, and must not. The
verifier fills them in. A row the developer filled would not be
independent.

## 1. Risk-based sample

| # | Item | Why it is in the sample | Independent method | Inputs the verifier uses | Inputs the verifier must not use |
|---|---|---|---|---|---|
| I-1 | HKS-SR-080 encoder | A format error corrupts every packet and is easy to get subtly wrong. | Derive the expected 148-byte TM[3,25] packet by hand from the standard (CCSDS 133.0-B, ECSS-E-ST-70-41C) and `design/data-model.md`, including the CRC. Compare with a packet the target emitted, e.g. ST-05 in `test/system/st_uart.log`. | Standards; data model; a captured packet | `test/unit/test_hk_pus_encoder.c` (the developer's vector); `tools/hk_tm_decode.py` (the developer's decoder) |
| I-2 | HKS-FD-030 debounce | An off-by-one in the counter makes FDIR fire early or never. | Write independent boundary tests at *N* − 1, *N* and *N* + 1 consecutive samples, on both a soft and a hard limit, plus an interrupted run. Run them against `hk_monitor_check()` on the host, or as a rig scenario on the target. | `requirements/fdir-increment.md`; `src/fdir/include/hk_monitor.h`, `hk_limits.h` (the interface only) | `test/unit/test_hk_monitor.c`; `test/system/st_capture.py` ST-07/08/10 checks |
| I-3 | HKS-FD-060 / 080 safe mode | Worst consequence; the safe-mode flag is shared between the collection task and the command task. | (a) An independent scenario on target: the verifier's own fault and command sequence in a copy of `test/system/st_main.c`'s table. Suggested: CLEAR_SAFE_MODE while the critical condition persists, and a relapse after a clear. (b) A read of the critical-section handling: `src/fdir/src/hk_safemode.c`, `src/app/src/hk_critical_rtems.c`, `src/app/src/hk_manager.c`, the task priorities in `src/app/tasks/hk_tasks.h`. | Requirements; `design/fdir-design.md` §4; the rig | The developer's scenario table (only its mechanism) |
| I-4 | FDIR requirements HKS-FD-010 to 090 | New requirements may be ambiguous or untestable. | Static review of each statement: single, unambiguous, testable, traceable to HKS-SYS-040. | `requirements/fdir-increment.md` | The design and test documents, until the review of each statement is written |

## 2. Execution log

Fill one row per check. **Verdict** is one of: **Pass** (confirmed),
**Discrepancy** (needs investigation; may be the item or the method),
**Defect** (raise a repository issue, record its number, close it with a
referenced fix).

| # | Date | Verifier | Independence basis (what was not shared with the developer) | Method as executed | Result | Finding | Verdict | Issue |
|---|---|---|---|---|---|---|---|---|
| I-1 | | | | | | | | |
| I-2 | | | | | | | | |
| I-3a | | | | | | | | |
| I-3b | | | | | | | | |
| I-4 | | | | | | | | |

## 3. Developer's known open points (for the verifier's information)

The developer's own findings from the system tests, listed so the verifier
does not spend time rediscovering them. They are not ISVV findings, and the
verifier may disagree with how they were dispositioned.

- SYS-1, malformed frames rejected silently: fixed
  (`verification/system-test-results.md` §2).
- SYS-3, safe-mode state not observable in telemetry: open, decision
  needed. This bears on I-3 and I-4: is HKS-FD-080 testable from the
  ground as written?

## 4. Closure

| Item | Value |
|---|---|
| Sample complete | *(yes/no)* |
| Defects raised / closed | |
| Verifier's statement | |

An ISVV that finds nothing on its first run deserves mild suspicion, not
celebration. If every row is **Pass**, note here why the checks were
independent in fact and not only in name.

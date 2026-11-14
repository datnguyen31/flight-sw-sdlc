# Release Notes: HK-TM v1.0

Capstone release of the Housekeeping Telemetry Service: complete,
verified and validated within the scope stated in
`docs/verification-report.md`.

## Contents

- **Baseline function** (HKS-SR-010 to 180): 32 parameters sampled every
  1000 ms; PUS TM[3,25] report in a CCSDS space packet every 4000 ms, with
  CUC time and CRC; ground enable and disable; quiet start; malformed and
  unknown telecommands rejected visibly.
- **FDIR increment** (HKS-FD-010 to 090): soft and hard limits, debounce
  N = 3, PUS Service 5 events by severity on APID 0x0CB, stale-source
  detection, safe mode with ground-only recovery.
- **Target**: RTEMS 7, sparc/leon3, run on Renode 1.17.0. A system test
  image with fault injection, ST-01 to ST-10.
- **Documents**: requirements, architecture, detailed and FDIR design,
  18 recorded design decisions, coding standard, timing analysis
  (revision 3), measured timing, system test specification and results,
  ISVV plan and record, verification and validation report, technical
  report.

## Verification status

| | Count |
|---|---|
| Requirements | 31 (4 system, 18 software, 9 FDIR) |
| Verified | 29 |
| Partial | 2: HKS-SR-140, HKS-SR-170 |
| Failed | 0 |
| ISVV | Completed by the mentor on 2026-11-11: no blocking software defect |
| Unit tests | 66/66 pass (10 suites) |
| System tests | 10/10 pass on target, re-run at this baseline |

## Open items

- **OI-1, HKS-SR-140**: jitter and absolute cycle time need a measurement
  on flight-representative hardware. This is the main open item.
- OI-2, HKS-SR-170: the six MISRA Required findings are corrected and
  the MISRA gate is in CI. Still open: clang-tidy, and the rules cppcheck
  does not check.
- OI-3, ISVV observations (non-blocking, for a future increment): clarify
  the wording of HKS-FD-030, FD-070 and FD-080, including the behaviour of
  CLEAR_SAFE_MODE while a critical condition persists; preserve a raw
  TM[3,25] packet as qualification evidence. ISVV execution itself is
  complete (`verification/isvv-record.md`).
- OI-4: SYS-3, safe-mode state not in telemetry, is carried as a future
  requirements and validation improvement.
- OI-5: target build and system tests in CI, once the toolchain image
  is published.

## Changes since vv-v0.1

Source, for the PR #8 review (OI-2). The six MISRA C:2012 Required
findings were corrected, with no change in behaviour:
- `src/app/src/hk_pus_encoder.c`: the CCSDS header flags are written as
  16-bit literals (rule 12.2), and the PEC pointer differences are held
  in `ptrdiff_t` before the cast (rule 10.8).
- `src/app/rtems/init.c`: `Init` has a prototype before its definition
  (rule 8.4), and the scripted run ends with `rtems_shutdown_executive(0)`
  instead of `exit()` (rule 21.8).

CI: a MISRA gate, `ci/misra_gate.py`, fails the build on any Mandatory or
Required finding. Unit tests (66/66), the end-to-end run, and system tests
(10/10, the same observations as the recorded run) were re-run after the
change.

Documents: verification and validation report, technical report (with
the code-size method and the Renode rationale), DD-18 (Renode, not QEMU),
coding standard §1 and §3, defense walkthrough, these notes, timing
analysis revision 3 (closes SYS-4), SR-170 status corrected from "Defined"
to "Partial", README.

## Baselines

`req-baseline-v0.1` → `design-baseline-v0.1` → `design-detailed-v0.1` →
`impl-v0.1` → `impl-v0.2` → `fdir-v0.1` → `vv-v0.1` → `v1.0`

This is a capstone release. It is not a claim of flight qualification.

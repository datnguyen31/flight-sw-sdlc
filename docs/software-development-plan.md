# Software Development Plan (SDP)

## 1. Scope

This project develops a flight-representative **Housekeeping Telemetry
Service** for a satellite onboard computer, under a complete and documented
software development life cycle. The objective is not the size of the code
but the completeness and defensibility of the process wrapped around it:
requirements, design, implementation, fault management, verification, and
product assurance, each traceable to the next.

Target RTOS: **RTEMS**.

## 2. Applicable Standards

| Standard | Role |
|---|---|
| ECSS-E-ST-40C | Software engineering process and life cycle phases (backbone of the phase model) |
| ECSS-Q-ST-80C | Software product assurance — reviews, verification records, defect handling, coding standards |
| ECSS-E-ST-70-41 (PUS) | Telemetry and telecommand service definitions. Service 3 (housekeeping) is the anchor function |
| NASA NPR 7150.2 | Reference/cross-check only, for software classification |
| MISRA C:2012 (subset) | Coding standard basis for the C implementation, enforced by static analysis |

## 3. Life Cycle Model

A tailored, incremental **V-model**. Each increment carries one function or
capability down the left side of the V (requirements, design, code) and back
up the right side (unit, integration, system verification), so verification
is planned against requirements from the outset.

- First increment: baseline HK-TM function.
- Later increments: FDIR, then configurable reporting.

## 4. Phase and Session Mapping

| Session(s) | Phase | Key Outputs |
|---|---|---|
| 3 (now) | Requirements engineering, project planning | SDP, requirements baseline |
| 4–5 | Architecture and detailed design | Design description, interface definitions |
| 6–7 | Implementation of the baseline function | Source code, unit tests |
| 8 | FDIR increment (monitoring, safe mode) | FDIR design, code, tests |
| 9–10 | Integration and system verification, ISVV | Verification report, traceability |
| Capstone | Qualification review and defense | Technical report, oral defense |

## 5. Configuration Management

- Everything is versioned in the `flight-sw-sdlc` repository.
- `main` is protected and always represents a reviewed, consistent state.
- Work happens on short-lived feature branches merged by pull request.
- Every commit that satisfies or changes a requirement references the
  requirement ID (e.g. `HKS-SR-040: add periodic report scheduler`).
- Baselines are marked with annotated git tags (e.g. `req-baseline-v0.1`).
- Documents are Markdown in `docs/`, `requirements/`, and `verification/` so
  they diff and review like code.

## 6. Quality Assurance Approach

- **Reviews**: each artifact (requirements, design, code) is reviewed before
  being baselined. Mentor acts as reviewer/product assurance. Findings are
  logged as repo issues.
- **Coding standard and static analysis**: C code follows the MISRA C:2012
  subset, checked with `cppcheck` and `clang-tidy` in CI.
- **Traceability**: a requirements traceability matrix links each requirement
  to its design element and verifying test. No requirement is closed without
  a verification result.
- **Defect handling**: defects are recorded, classified by severity, and
  closed with a reference to the fixing commit.

## 7. Verification and Validation (V&V) Approach

Four verification methods are referenced by letter throughout the
requirements:

| Letter | Method |
|---|---|
| T | Test |
| A | Analysis |
| I | Inspection |
| R | Review of design |

Verification runs at three levels — unit, integration, system — against
requirements, on target or on the RTEMS/QEMU emulator.

Independent software verification and validation (**ISVV**) is introduced
later, where the mentor re-verifies a sample of requirements independently.

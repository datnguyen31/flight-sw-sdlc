# Flight Software SDLC

Flight software project, developed under a formal SDLC with traceability from
requirements through design, implementation, and verification.

## File Structure

1. `requirements`: System and software requirements, traceability to the mission and to tests
2. `design`: Architecture, interface definitions, and design decisions (and justification possibly)
3. `src/app`: The onboard application software
4. `src/fdir`: Fault detection, isolation, and recovery logic kept deliberately separate for clarity and review
5. `test/`: Unit, integration, and system tests, structured to produce evidence
6. `verification/`: Verification and validation plans, review records, and ISVV artifacts (QA/QC evidence)
7. `docs/`: SDLC plan, coding standard, and the living technical report for your capstone defense
8. `ci/`: Continuous integration pipeline definitions for automated build and test

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

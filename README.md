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

Host build (unit tests via ctest):

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Cross-build for RTEMS 6, BSP arm/beagleboneblack (DD-08), against the
toolchain installed at `/opt/rtems6`:

```sh
cmake -S . -B build-rtems -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems-beagleboneblack.cmake
cmake --build build-rtems
```

This produces `src/app/rtems/rtems_smoke`, a build-integration smoke image,
not the HK-TM application itself (Session 6/7). The RTEMS build does not
run here: beagleboneblack boots over TFTP on real hardware, not under
QEMU. Unit tests only build for the host and are skipped when
cross-compiling.

If `mkimage` is available (Debian/Ubuntu: `sudo apt install u-boot-tools`),
the same build also produces `rtems_smoke.img`: the ELF stripped to a raw
binary, gzipped, and wrapped as a U-Boot loadable kernel image, ready to
TFTP-boot on real beagleboneblack hardware. Without `mkimage` this step is
skipped with a status message; the ELF build is unaffected either way.

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

Cross-build for RTEMS 7, BSP sparc/leon3 (DD-09), against the toolchain
installed at `/opt/rtems7`, and run end to end on Renode:

```sh
cmake -S . -B build-target -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems.cmake
cmake --build build-target
RENODE=/path/to/renode tools/renode/run_hk_app.sh   # or: cmake --build build-target --target run_renode
```

This produces `build-target/src/app/rtems/hk_app.exe`, the HK-TM service
image, then runs it for 27 s of virtual time on Renode's LEON3 platform. The
image's `Init` plays a scripted ground station: ENABLE_REPORTS, an unknown
telecommand, then DISABLE_REPORTS. `tools/hk_tm_decode.py` then decodes every
`TM` packet on the console independently of the flight encoder and checks
it. To watch it interactively instead:

```sh
renode -e '$bin=@build-target/src/app/rtems/hk_app.exe; include @tools/renode/hk_leon3.resc; showAnalyzer uart; start'
```

Other installed BSPs use the same toolchain file, e.g. `-DRTEMS_BSP=erc32`
(runs under `sparc-rtems7-sis -r`). For RTEMS 6 arm/beagleboneblack on real
hardware (DD-08), use the preset
`-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems-beagleboneblack.cmake`. If
`mkimage` is available (`sudo apt install u-boot-tools`), that build also
produces `hk_app.img`, a U-Boot loadable image ready to TFTP-boot.

Unit tests only build for the host and are skipped when cross-compiling.
CI runs the target build and the Renode run in the `target-build` job, using
the image from `tools/docker/rtems7-leon3.Dockerfile`. That job is skipped
until the repository variable `RTEMS_TOOLCHAIN_IMAGE` names a published image.

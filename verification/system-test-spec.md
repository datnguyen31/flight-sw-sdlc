# HK-TM System Test Specification (Session 9)

The system tests verify the assembled HK-TM service with FDIR against its
requirements, end to end, on the target. The service runs as built for
flight: the same modules, tasks, startup sequence and RTEMS configuration.
It is driven only through its external interfaces and observed only
through what it emits, as a ground system would see it. Results are in
`verification/system-test-results.md`.

## 1. Test levels

| Level | What it checks | Evidence | Status |
|---|---|---|---|
| Unit | Each module against its detailed design, in isolation | 10 Unity suites, `ctest --test-dir build` (Sessions 6 to 8) | Done |
| Integration | The real modules together through their real interfaces, on the target | This rig: every scenario runs the integrated image (§2) | This session |
| System | The whole service against its requirements, end to end | ST-01 to ST-10 (§4) | This session |
| ISVV | A risk-based sample re-verified independently of the developer | `verification/isvv-record.md` | Started this session (mentor) |

## 2. Test rig

The service is driven and observed only through its interfaces; faults
enter through the parameter provider.

```
                      RTEMS 7 sparc/leon3 on Renode (DD-09)
 Command injector  ──telecommands──►  ┌───────────────────────────┐
 (st_main.c)                          │ HK-TM service + FDIR      │──TM packets──► Telemetry capture
 Fault injector ─► Parameter provider │ (system under test)       │  + [cmd]/[hk]   (st_capture.py, host)
 (st_main.c)       (st_param_provider.c) ──sampled values──►      │   console
                                      └───────────────────────────┘
```

| Rig element | File | Role |
|---|---|---|
| System test image | `test/system/CMakeLists.txt` → `hk_system_test.exe` | Links `hk_service` (the flight tasks, startup, console telemetry sink and RTEMS time, shared with `hk_app`) and the `app` and `fdir` libraries. Only the parameter provider and the Init script differ from the flight image. |
| Representative parameter provider | `test/system/st_param_provider.c` | Implements `hk_param_source_read()` (HKS-SR-030). Nominal value of report position *i* = middle of its soft band + *i*: in limits, constant, and tells the capture which position it came from. |
| Fault injector | `test/system/st_param_provider.c`, `st_rig.h` | On cue from the sequencer, one parameter returns a scripted value (out of limits) or `HK_PARAM_STALE`, until it is put back. |
| Command injector and sequencer | `test/system/st_main.c` | Runs in Init after `hk_service_start()`. A table of steps at fixed times since boot uplinks telecommands through the flight uplink entry points (`hk_command_uplink`, `hk_command_uplink_frame`) and drives the fault injector. Every step is at a half-second, between two collection releases (release *k* is at about *k* − 1 s). |
| Telemetry capture | `test/system/st_capture.py` (host) | Decodes every TM packet from the target console with the ground decoder's header, PEC and event checks (`tools/hk_tm_decode.py`), then asserts each scenario's pass condition. Exit status 0 only if all pass. |
| Runner | `test/system/run_system_tests.sh` | Runs the image headless on Renode for 90 s of virtual time, then the capture. |

**What the verdicts rest on.** The service emits three things: TM packets
(HK TM[3,25] on APID `0x0CA`, events TM[5,s] on APID `0x0CB`), command
verification lines (`[cmd] telecommand n accepted|REJECTED`,
`[cmd] malformed telecommand (n bytes) REJECTED`), and one status line per
collection release (`[hk] cycle n: <step result>`, with `[SAFE MODE]` while
the indicator is set). The rig also prints `[ST]` begin/end markers and one
`[fault]` or `[ground]` line per step. The capture uses rig lines only to
split the log into scenarios and to know when a fault started. Every
verdict is taken from service output.

**Timing of observations.** A packet is printed during a collection
release, before that release's `[hk]` line. The capture attributes each
packet to the next `[hk]` line. A debounce is then measured as the number
of releases between the `[fault]` line and the event packet: an event on
the *N*th sample has *N* − 1 releases before it.

**Order and dependence.** One boot runs all ten scenarios, in the order
of §3. Each one starts from the state the previous one left, as its entry
condition states. A scenario that leaves the wrong state can therefore
make later ones fail too: one defect can produce several failures, and the
first failing scenario in run order is the one to investigate. ST-03 must
come first, because it needs the state at power-on.

**Configuration under test.** Collection 1000 ms, report divider 4
(4000 ms), `HK_DEBOUNCE_N` = 3, `HK_STALE_M` = 3, the flight limit table
`src/fdir/src/hk_limits.c`.

**How to run.**

```sh
cmake -S . -B build-target -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-rtems.cmake
cmake --build build-target
RENODE=/path/to/renode test/system/run_system_tests.sh   # or --target run_system_tests
```

## 3. Timeline

| Time since boot (s) | Step | Scenario |
|---|---|---|
| 0 | power-on, no command | ST-03 |
| 9.5 | uplink ENABLE_REPORTS | ST-01 |
| 21.5 | (observe one packet) | ST-05 |
| 25.5, 26.5 | uplink a 1-byte frame, then unknown opcode 0x7F | ST-04 |
| 33.5 | uplink DISABLE_REPORTS | ST-02 |
| 43.5 → 44.5; 45.5 → 47.5 | 0x0201 := 3400 (past hard high 3300) for 1 sample; then for 2 samples | ST-10 |
| 49.5; 50.5 → 55.5 | ENABLE_REPORTS; 0x0203 stale for 5 samples | ST-06 |
| 60.5 → 65.5 | 0x0101 := 3400 (soft high 3300, hard high 3500) for 5 samples | ST-07 |
| 70.5 → 75.5 | 0x0102 := 3700 (hard high 3600) for 5 samples | ST-08 |
| 80.5 | uplink CLEAR_SAFE_MODE | ST-09 |
| 84.5 → | soak with reporting enabled (used for timing) | |

## 4. Test cases

Each case: entry condition, stimulus, the observable pass condition as
`st_capture.py` checks it, and the requirements it exercises.

| ID | Entry → stimulus | Pass condition (all must hold) | Requirements |
|---|---|---|---|
| ST-01 | Reporting disabled → ENABLE_REPORTS | ENABLE accepted. At least 3 HK packets, each a valid TM[3,25] (header, PEC). The emitting releases are exactly 4 apart and match the `report sent` lines. Every other release only collects. CUC intervals are 4 s ± 10 %. No events. | HKS-SR-020, 060, 100 |
| ST-02 | Reporting enabled → DISABLE_REPORTS | DISABLE accepted. No HK packet after it. The first report tick after it is within 4 releases and is `report tick, reporting disabled`, as is every later one (at least 2). | HKS-SR-110 |
| ST-03 | Power-on → nothing | At least 8 releases. 0 TM packets of either kind. No command processed. At least 2 report ticks, all suppressed. | HKS-SR-120 |
| ST-04 | Reporting enabled → a 1-byte frame (the first byte of DISABLE as uplinked), then opcode 0x7F | `malformed telecommand (1 bytes) REJECTED` and `telecommand 127 REJECTED`. Nothing accepted. At least 1 HK packet after both rejections. Every report tick in the window is `report sent` (state unchanged). Safe mode unchanged. | HKS-SR-130 |
| ST-05 | Reporting enabled → one report | Exactly 1 packet. Length 148. Packet ID `0x08CA`. Sequence flags unsegmented, and the count follows ST-01's last packet. Data length field 141. PUS byte `0x20`, service 3/25, message counter 0. SID 1. CUC within 10 ms of the emitting release's time. Value *i* equals the provider's nominal value for position *i*, for all 32 (parameter order). PEC valid. | HKS-SR-050, 070, 080 |
| ST-06 | Reporting enabled, all nominal → 0x0203 stale for 5 releases | Exactly 1 event: TM[5,3], stale, parameter 0x0203, condition 1 (STALE), emitted on the *M*th = 3rd stale release. Every HK packet in the stale window carries `0xFFFFFFFF` for 0x0203 and for no other parameter. The last packet after recovery has no stale marker. No safe mode. | HKS-SR-040, FD-070 |
| ST-07 | All nominal → 0x0101 in its warning band for 5 releases | Exactly 1 event: TM[5,2], limit, parameter 0x0101, warn high, on the *N*th = 3rd sample. No safe-mode event. `[SAFE MODE]` never shown. No event on recovery. | HKS-FD-030, FD-050 |
| ST-08 | All nominal → 0x0102 past its hard limit for 5 releases | Exactly 2 events: TM[5,4] limit, parameter 0x0102, crit high, on the 3rd sample, then TM[5,4] safe mode with the same CUC. `[SAFE MODE]` exactly from that release on. It is still set for at least 3 releases after the value returns to nominal (no autonomous recovery). | HKS-FD-050, 060, 080 |
| ST-09 | Safe mode active → CLEAR_SAFE_MODE | `[SAFE MODE]` at entry. CLEAR accepted. No `[SAFE MODE]` on any later release (at least 2). No events. | HKS-FD-080 |
| ST-10 | All nominal → 0x0201 past its **hard** limit for 1 sample, then again for *N* − 1 = 2 consecutive samples | Fault windows of exactly 1 and 2 samples. 0 events. No safe mode. | HKS-FD-030 |
| Global | whole run | Every packet passes header and PEC checks. HK and event sequence counts are contiguous. 0 period overruns. | HKS-SR-080, 090 |

**ST-10 goes beyond the Session 9 brief.** The brief's ST-10 is a single
out-of-limit sample. A single sample cannot distinguish *N* = 3 from
*N* = 2, and a mutation run showed it. With `HK_DEBOUNCE_N` set to 2 in
the target build, the single-sample ST-10 passed and only ST-07 and ST-08
failed. ST-10 therefore adds an *N* − 1 run. With the same mutant, ST-10
itself now fails. The transient is past the **hard** limit, so a debounce
defect would show as the worst outcome, a spurious safe mode.

**Checking the checker.** A system test that cannot fail proves nothing.
`system-test-results.md` §3 records the negative runs: tampered logs and
a code mutant, each of which the capture rejects.

## 5. Coverage of behavioural requirements

| Requirement | System test |
|---|---|
| HKS-SR-020 | ST-01 |
| HKS-SR-040 | ST-06 |
| HKS-SR-050 | ST-05 |
| HKS-SR-060 | ST-01 |
| HKS-SR-070 | ST-05 |
| HKS-SR-080 | ST-05, Global |
| HKS-SR-090 | Global (every packet reaches ground through the sink) |
| HKS-SR-100 | ST-01 |
| HKS-SR-110 | ST-02 |
| HKS-SR-120 | ST-03 |
| HKS-SR-130 | ST-04 |
| HKS-SR-140 | Measured: `verification/timing-measured.md` |
| HKS-SR-150 | Measured: `verification/timing-measured.md` |
| HKS-FD-020 | ST-06, 07, 08, 10 (the monitor runs every release, including while reporting is disabled: ST-10) |
| HKS-FD-030 | ST-07, ST-08, ST-10 |
| HKS-FD-040 | ST-07, ST-08 (event identifies parameter and condition) |
| HKS-FD-050 | ST-07, ST-08 |
| HKS-FD-060 | ST-08 |
| HKS-FD-070 | ST-06 |
| HKS-FD-080 | ST-08, ST-09 |

Every behavioural requirement is covered at least once at system level.
HKS-SR-010, 030, 160 to 180 and HKS-FD-010 and 090 are verified by review,
inspection or analysis (`verification/traceability-matrix.md`). The system
run exercises them but does not add evidence for them.

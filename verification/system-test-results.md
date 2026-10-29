# HK-TM System Test Results (Session 9)

Execution record for `verification/system-test-spec.md`.

| Item | Value |
|---|---|
| Baseline | branch `session9-vandv`, from `fdir-v0.1` (Session 8). Session 8 was not yet merged to `main` when this session started. |
| Image | `build-target/test/system/hk_system_test.exe`, RTEMS 7.0.0 (b6d95403), sparc-rtems7-gcc 15.2.0, BSP sparc/leon3 |
| Platform | Renode 1.17.0, LEON3 platform (`tools/renode/hk_leon3.resc`), CPU model 100 MIPS, default time quantum (100 µs), 90 s of virtual time. The Session 9 brief says QEMU; this project's emulated target has been Renode since Session 7 (DD-09). |
| Command | `RENODE=... test/system/run_system_tests.sh` |
| Date | 2026-10-16 |
| Verdict | **10 / 10 PASS**, global checks PASS, 0 period overruns |

## 1. Results

| ID | Requirements | Verdict | Observed (capture output) |
|---|---|---|---|
| ST-01 | SR-020, 060, 100 | **PASS** | ENABLE accepted. 3 TM[3,25] at cycles 12, 16, 20 (seq 0, 1, 2). Intervals 4.000000 s, 4.000000 s. Cycles in between: `collected`. |
| ST-02 | SR-110 | **PASS** | DISABLE accepted. The first report tick, 2 releases later (cycle 36), was suppressed, and so was the next. 0 packets after DISABLE. |
| ST-03 | SR-120 | **PASS** | Cycles 1 to 10 with no command. 2 report ticks (4, 8), both suppressed. 0 TM packets. |
| ST-04 | SR-130 | **PASS** | `malformed telecommand (1 bytes) REJECTED`, `telecommand 127 REJECTED`, nothing accepted. 2 HK packets after them (seq 4, 5): reporting state unchanged. Safe mode unchanged. |
| ST-05 | SR-050, 070, 080 | **PASS** | seq 3 (follows ST-01's 2). Packet ID 0x08CA, unsegmented, data length 141, 148 bytes. PUS 0x20 [3,25], message counter 0, SID 1. CUC 22.999954 s for cycle 24 (released at about 23 s). All 32 values equal the provider's nominal value for their position. PEC valid. |
| ST-06 | SR-040, FD-070 | **PASS** | 0x0203 stale for 5 releases. HK packets seq 6 and 7 carry 0xFFFFFFFF for 0x0203 and for no other parameter. One TM[5,3] stale event (param 0x0203, condition 1) at cycle 54, the 3rd stale release. The marker is gone in seq 8. |
| ST-07 | FD-030, FD-050 | **PASS** | 0x0101 := 3400 for 5 releases. One TM[5,2] limit event, warn high, at cycle 64, the 3rd sample. No safe-mode event, safe mode never indicated, no event on recovery. |
| ST-08 | FD-050, 060, 080 | **PASS** | 0x0102 := 3700 for 5 releases. At cycle 74, the 3rd sample: TM[5,4] limit crit high, then TM[5,4] safe mode with the same CUC. `[SAFE MODE]` from cycle 74, still set 4 releases after the value returned to nominal. |
| ST-09 | FD-080 | **PASS** | `[SAFE MODE]` at entry. CLEAR_SAFE_MODE accepted. 4 following releases without `[SAFE MODE]`. No events. |
| ST-10 | FD-030 | **PASS** | 0x0201 := 3400 (past its **hard** limit) for 1 sample, then for 2 consecutive samples (*N* − 1). 0 events. Safe mode never indicated. |
| Global | SR-080, 090 | **PASS** | 16 HK packets and 4 events. Every header and PEC is valid. HK seq 0 to 15 and event seq 0 to 3 contiguous. 0 overruns. |

Event packets as captured (APID 0x0CB, 24 bytes):

```
cycle 54  TM 08CBC0000011200503000000000034FFFD00020203019F09   stale      0x0203 cond 1
cycle 64  TM 08CBC001001120050200000000003EFFFD00010101023F86   limit      0x0101 warn high
cycle 74  TM 08CBC0020011200504000000000048FFFD000101020443B3   limit      0x0102 crit high
cycle 74  TM 08CBC0030011200504000000000048FFFD00030000006112   safe mode
```

## 2. Integration findings

These were found while assembling and running the integrated image. They
are the developer's own findings. The independent ones are in
`verification/isvv-record.md`.

| ID | Type | Finding | Disposition |
|---|---|---|---|
| SYS-1 | Defect (fixed) | **A malformed uplink frame was rejected silently.** The command task dropped a frame of the wrong length without a trace, so the SR-130 rejection of a *malformed* telecommand was true but unobservable: ground got no rejection, and no test could tell "rejected" from "never arrived". Only *unknown* opcodes were reported. | Fixed this session. `tasks/hk_command_task.c` reports `malformed telecommand (n bytes) REJECTED` and does not dispatch. `hk_command_uplink_frame()` is the uplink entry point for raw frames, and `hk_command_uplink()` now goes through it. ST-04 verifies this. Issue text: §4. |
| SYS-2 | Spec weakness (fixed in spec) | **The brief's ST-10 cannot detect an off-by-one in N.** A single out-of-limit sample passes with *N* = 2 as well as with *N* = 3. A mutation run proved it: with `HK_DEBOUNCE_N` 2, the single-sample ST-10 passed. | ST-10 extended with an *N* − 1 run (spec §4). The same mutant now fails ST-10 (§3). |
| SYS-3 | Observation (open, needs a decision) | **Safe-mode state is not in telemetry.** Entering safe mode emits an event, but nothing in TM shows the indicator's current state, and clearing it emits nothing. Ground can confirm CLEAR_SAFE_MODE only from command verification ("accepted") and the console status line, not from telemetry. ST-09 therefore uses the `[hk]` status line as its observable. HKS-FD-080 does not require more, so this is not a defect against the requirements. It is a gap a ground operator would notice. | Raised for decision (issue text §4). Options: an informative TM[5,1] event on exit from safe mode, or a status parameter in the report (that changes the frozen SR-050 structure, so it needs a new requirement). No code change this session. |
| SYS-4 | Analysis discrepancy | **The console sink costs far more than the 50 µs sink handoff the timing analysis assumes.** Measured by difference, the output path costs about 6 µs per byte (console hex formatting to a polled UART): about 880 µs per HK packet against 220 µs assumed, and about 150 µs per event packet against 80 µs. The total still agrees with the analysis: the worst report-plus-event release measured 1.30 ms against 1.29 ms, a difference within the ±10 µs measurement resolution, so no exceedance of the total is claimed. | See `verification/timing-measured.md` §3. The margin to 50 ms is unaffected. The analysis assumption holds for a queue handoff to a downlink, not for the polled-UART stand-in. **Closed in Session 10** by `timing-analysis.md` revision 3 (sink handoff bounded per sink). |

## 3. Negative runs: checking the checker

Every scenario passed on its first run. A first run with no failures
should raise suspicion, so these runs check that the capture fails when
the service misbehaves.

**Tampered logs** (copies of a passing capture, edited):

| Tampering | Capture verdict |
|---|---|
| ENABLE verification line removed | ST-01 FAIL: ENABLE_REPORTS not accepted |
| Sequence-control bytes of the first HK packet zeroed | Global FAIL: sequence flags not unsegmented; PEC does not match |
| Safe-mode event packet removed | ST-08 FAIL: 1 events, expected 2 |
| `[SAFE MODE]` removed from one release after the ST-08 condition cleared | ST-08 FAIL: indicator not set exactly from the confirming cycle on; safe mode did not hold |
| Stale event moved one release earlier | ST-06 FAIL: stale event on stale sample 2, expected sample 3 |

**Code mutant**, built and run on the target: `HK_DEBOUNCE_N` changed from 3
to 2 in `src/fdir/include/hk_monitor.h`.

| Spec version | Result |
|---|---|
| Brief's ST-10 (single sample) | ST-07 and ST-08 FAIL (event on sample 2); **ST-10 PASS**, which is SYS-2 |
| Extended ST-10 (1 and *N* − 1) | ST-10 FAIL (2 events from transients, spurious safe mode). The safe mode it entered carried into ST-06 and ST-07 as well, as spec §2 notes for one boot |

The mutant was reverted, and the recorded run above is from the
unmodified source.

## 4. Issue texts

`gh` is not installed in this environment, so these are ready to open on
GitHub. Reference the issue numbers from the PR.

**Issue: Command task drops malformed uplink frames silently (SYS-1)**

> A telecommand frame whose length is not `sizeof(hk_command_id_t)` was
> discarded by `hk_command_task` without any report. HKS-SR-130 requires
> rejection without a state change, which held, but the rejection was not
> observable, so neither ground nor a test could verify it. Found while
> preparing system test ST-04. Fixed on `session9-vandv`: the task now
> logs `malformed telecommand (n bytes) REJECTED`, and
> `hk_command_uplink_frame()` takes raw frames. Verified by ST-04.

**Issue: Safe-mode state is not observable in telemetry (SYS-3)**

> Safe-mode entry emits TM[5,4], but the indicator's current state is not
> in any TM packet and leaving safe mode emits nothing. After
> CLEAR_SAFE_MODE, ground can confirm recovery only from command
> verification and the console status line. HKS-FD-080 is met as written.
> Decision needed: add an informative TM[5,1] "safe mode exited" event
> (new requirement under HKS-FD-080; no change to the HK report), or a
> safe-mode status parameter in the HK report (changes the frozen
> HKS-SR-050 structure). Found during system test ST-09.

## 5. Raw evidence

The full console capture of the recorded run is reproduced by the command
above. The capture's summary output for this run:

```
ST-01 [PASS] ... 3 TM[3,25] at cycles [12, 16, 20], seq [0, 1, 2], intervals 4.000000, 4.000000 s
ST-02 [PASS] ... first report tick 2 cycle(s) later (cycle 36) suppressed; 2 ticks suppressed, 0 packets after DISABLE
ST-03 [PASS] ... 10 cycles (1..10), 2 report ticks suppressed, 0 TM packets
ST-04 [PASS] ... truncated frame and 0x7F both REJECTED; 2 HK packet(s) after them (seq [4, 5]), every report tick sent
ST-05 [PASS] ... seq 3, packet ID 0x08CA, length field 141, PUS 0x20 [3,25], msg counter 0, SID 1, CUC 22.999954 s for cycle 24, 32 values = nominal(i) in report order, PEC valid
ST-06 [PASS] ... packets seq [6, 7] carry 0xFFFFFFFF for it only; one TM[5,3] stale event (cycle 54, stale sample 3); marker gone in seq 8
ST-07 [PASS] ... one TM[5,2] limit warn high at cycle 64 (sample 3); no safe-mode event, safe mode inactive; no event on recovery
ST-08 [PASS] ... TM[5,4] limit crit high and TM[5,4] safe-mode event, same CUC, at cycle 74 (sample 3); [SAFE MODE] from cycle 74, still set 4 cycles after the value returned to nominal
ST-09 [PASS] ... [SAFE MODE] at entry; CLEAR_SAFE_MODE accepted; 4 following cycles without [SAFE MODE]; no events
ST-10 [PASS] ... 1 sample, then for 2 consecutive samples (N - 1); 0 events, safe mode inactive
global: 16 HK packets, 4 events, header/PEC/sequence checks PASS; max overruns 0
PASS: all 10 system test scenarios
```

The flight image's own end-to-end run (`tools/renode/run_hk_app.sh`) still
passes with the Session 9 changes: 4 packets, 0 events, 0.000 ms jitter.

# HK-TM Requirements Traceability Matrix

| Requirement | Assigned method (baseline) | Design element | Verifying test (unit, Sessions 6 to 8) | System test (Session 9) | ISVV | Status |
|---|---|---|---|---|---|---|
| HKS-SYS-010 | T (Test) | Satisfied via child software requirements (see HKS-SR-020, 060, etc.) |  | ST-01, ST-05 (periodic, decodable HK on target) |  | **Verified (Test, system)** |
| HKS-SYS-020 | T (Test) | Satisfied via child software requirements (see HKS-SR-100, 110, 120, 130) |  | ST-01 to ST-04 |  | **Verified (Test, system)** |
| HKS-SYS-030 | T (Test) | Satisfied via child software requirements (see HKS-SR-070) |  | ST-05 (CUC = collection time) |  | **Verified (Test, system)** |
| HKS-SYS-040 | T (Test) | Satisfied via child software requirements HKS-FD-010 to 090 (requirements/fdir-increment.md) |  | ST-06 to ST-10 |  | **Verified (Test, system)** |
| HKS-SR-010 | R (Review) | Fixed parameter table `g_param_ids` (src/app/src/hk_param_table.c; data-model.md) | test_param_table_ids_are_unique; test_collect_reads_every_param_once_in_order |  |  | **Verified (Review)**; Test added as supplementary evidence |
| HKS-SR-020 | T (Test) | Collection task rate-monotonic period, 1000 ms (tasks/hk_collection_task.c); Sampler (hk_sampler.c; detailed-design.md §1.1, §1.2) | test_collect_reads_every_param_once_in_order; test_enabled_reports_every_divider_cycles; Renode E2E: one cycle per 1 s | ST-01 |  | **Verified (Test)** |
| HKS-SR-030 | R (Review) | Sampler reads only via `hk_param_source_read` (hk_param_source.h; hk_sampler.c) | test_collect_reads_every_param_once_in_order (stub source) |  |  | **Verified (Review)**; Test added as supplementary evidence |
| HKS-SR-040 | T (Test) | Stale marker `0xFFFFFFFF` for any non-OK read, cycle continues (hk_sampler.c) | test_failing_sources_write_stale_marker_and_continue; test_recovered_source_clears_stale_marker; Renode E2E: 0x0203 stale in packets 0 and 3 | ST-06 |  | **Verified (Test)** |
| HKS-SR-050 | R (Review) | Report Builder fixed SID; fixed value order (hk_report_builder.c; hk_report.h) | test_build_stamps_sid_and_time; SID and value order in test_encoder_matches_known_vector | ST-05 |  | **Verified (Review)**; Test and system test added as supplementary evidence |
| HKS-SR-060 | T (Test) | Report divider = 4 in `hk_collection_step` (hk_collection.c; DD-01, DD-10) | test_enabled_reports_every_divider_cycles; test_enable_midway_keeps_report_alignment; Renode E2E: 4.000 s report interval | ST-01 |  | **Verified (Test)** |
| HKS-SR-070 | T (Test) | CUC 4+2 collection time (hk_report_builder.c; hk_time_rtems.c; data-model.md) | test_build_stamps_sid_and_time; CUC field in test_encoder_matches_known_vector and test_sink_receives_encoded_packet | ST-05 |  | **Verified (Test)** |
| HKS-SR-080 | I (Inspection) | PUS/CCSDS Encoder, TM[3,25] + PEC (hk_pus_encoder.c; detailed-design.md §1.4) | test_encoder_matches_known_vector (byte-exact, 148 bytes); test_crc16_ccitt_check_value; Renode E2E: packets checked by independent decoder tools/hk_tm_decode.py | ST-05; every packet of the run (global check) | I-1 (pending, mentor) | **Verified (Inspection)**; Test and system test added as supplementary evidence; independent inspection I-1 pending |
| HKS-SR-090 | R (Review) | Telemetry-sink interface (hk_tm_sink.h); hand-off in hk_collection.c; console sink on target | test_sink_receives_encoded_packet (capture sink); test_sink_rejection_reported_and_counted | Global: all 16 HK + 4 event packets reach ground via the sink |  | **Verified (Review)**; Test and system test added as supplementary evidence |
| HKS-SR-100 | T (Test) | Command Handler dispatch; HK Manager state flag (detailed-design.md §1.5; DD-07) | test_enable_accepted | ST-01 |  | **Verified (Test)** |
| HKS-SR-110 | T (Test) | Command Handler dispatch; HK Manager state flag (detailed-design.md §1.5; DD-07) | test_disable_accepted | ST-02 |  | **Verified (Test)** |
| HKS-SR-120 | T (Test) | `hk_manager_init` default disabled (DD-04) | test_default_state_is_disabled | ST-03 |  | **Verified (Test)** |
| HKS-SR-130 | T (Test) | Command Handler reject path (detailed-design.md §1.5); still the default case with CLEAR_SAFE_MODE added (fdir-design.md §1); frame-length check and rejection report in the command task (tasks/hk_command_task.c, SYS-1) | test_unknown_rejected_no_change; test_unknown_rejected_safe_mode_unchanged; test_only_clear_command_leaves_safe_mode | ST-04 (malformed frame and unknown opcode; SYS-1 fixed) |  | **Verified (Test)** |
| HKS-SR-140 | T (Test) | Rate-monotonic release of the collection task (tasks/hk_collection_task.c; verification/timing-analysis.md) | Indicative only: Renode E2E max report jitter 0.000 ms, 0 period overruns; emulator timing is not representative. Final: measurement on target hardware | Measured (Renode, 480 releases): jitter 0 µs at 10 µs resolution, 0 overruns (verification/timing-measured.md). Indicative only: emulated time is deterministic |  | Partial (HW measurement pending) |
| HKS-SR-150 | A (Analysis) | Verified by Analysis. Evidence: verification/timing-analysis.md. Revision 2 (Session 8) includes the FDIR monitor: 0.99 ms collection cycle, 3.9 ms pathological event burst, against the 50 ms budget. Pending measured confirmation in Session 9. |  | Measured (Renode): worst collection cycle 0.57 ms (two FDIR events), worst release 1.30 ms (report + event), against 50 ms (verification/timing-measured.md). Output path exceeds its analysis bound (SYS-4); total corroborated |  | **Verified (Analysis), corroborated by measurement**; HW measurement pending |
| HKS-SR-160 | I (Inspection) | Static report and packet buffers (hk_collection.c); task storage and queue buffers static via `rtems_*_construct` (tasks/); no malloc in src/app (DD-02) | Inspection of src/app (no allocation calls); RTEMS object counts fixed in rtems/init.c. Supporting run: a Renode hook on `malloc` saw 8 calls, all from RTEMS console/newlib initialization before the tasks start, and none in the 25 s of operation after |  |  | **Verified (Inspection)** |
| HKS-SR-170 | I (Inspection) | Coding standard defined in docs/coding-standard.md. Verification by Inspection, enforced in CI from Session 6. |  |  |  | **Defined** |
| HKS-SR-180 | R (Review) | Two tasks, static priorities 10/20 and stacks (tasks/hk_tasks.h), created once at startup (rtems/init.c) | Inspection of tasks/hk_tasks.h and rtems/init.c; Renode E2E exercises both tasks |  |  | **Verified (Review)**; Inspection of code and Renode run added as supplementary evidence |
| HKS-FD-010 | T (Test) | Limit table `g_limits`, four ordered limits per parameter, and `hk_limit_classify` (src/fdir/src/hk_limits.c; fdir-design.md §7) | test_classify_at_and_across_every_threshold; test_classify_without_soft_band; test_flight_table_rows_are_ordered; test_flight_table_holds_simulator_values |  | I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-020 | T (Test) | `hk_monitor_check` called once per cycle in `hk_collection_step`, after the build, before the gate (hk_collection.c; fdir-design.md §1) | test_monitor_checks_every_cycle_while_reporting_disabled; test_events_leave_hk_stream_unchanged; test_nominal_report_raises_nothing | ST-06, 07, 08, 10 (ST-10 with reporting disabled) | I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-030 | T (Test) | Saturating run counters, HK_DEBOUNCE_N = 3 (hk_monitor.h, hk_monitor.c; fdir-design.md §2.1; DD-13) | test_debounce_confirms_on_Nth_sample; test_interrupted_run_does_not_confirm; test_mixed_soft_and_hard_samples; test_recovery_debounced_then_rearmed | ST-07, ST-08, ST-10 (1 and N-1 samples) | I-2 (pending, mentor); I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-040 | T (Test) | Limit event on each rise in level, TM[5,s] on APID 0x0CB (hk_monitor.c, hk_event.c; data-model.md "Event report"; DD-12) | test_limit_event_matches_known_vector (byte-exact); test_soft_limit_is_warning (parameter and condition); test_sustained_condition_reported_once; test_event_stamped_with_report_time | ST-07, ST-08 | I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-050 | T (Test) | Level WARNING or CRITICAL; event subtype 2 (low) or 4 (high) (hk_event.c; fdir-design.md §3) | test_severity_subtypes; test_soft_limit_is_warning; test_hard_limit_is_critical; test_warning_escalates_to_critical | ST-07, ST-08 | I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-060 | T (Test) | Rise to CRITICAL calls `hk_safemode_enter` (hk_monitor.c) | test_debounce_confirms_on_Nth_sample; test_hard_limit_is_critical; test_soft_limit_is_warning (no safe mode); test_critical_steps_down_and_relapse_is_reported | ST-08 | I-3 (pending, mentor); I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-070 | T (Test) | Stale run counter, HK_STALE_M = 3, one medium-severity event per run (hk_monitor.c; fdir-design.md §2.3) | test_stale_for_M_cycles_emits_once; test_fresh_sample_ends_stale_run; test_stale_sample_does_not_break_limit_run | ST-06 | I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-080 | T (Test) | Safe-mode indicator under the critical section, idempotent entry with one event, cleared only by `HK_CMD_CLEAR_SAFE_MODE` (hk_safemode.c, hk_command.c; fdir-design.md §4) | test_entry_sets_indicator_and_emits_event; test_repeated_entry_reports_once; test_ground_clear_then_reentry; test_only_clear_command_leaves_safe_mode; test_clear_safe_mode_accepted; test_clear_during_persisting_condition_holds | ST-08, ST-09 (state observed on console status line; SYS-3) | I-3 (pending, mentor); I-4 statement review (pending, mentor) | **Verified (Test)** |
| HKS-FD-090 | A, I (Analysis, Inspection) | All FDIR state static (src/fdir: monitor state, 24-byte event buffer, flag); no allocation calls in src/fdir. Timing: verification/timing-analysis.md revision 2 | Inspection of src/fdir; timing analysis (monitor 176 µs per cycle, ≤ 33 events per cycle: test_worst_case_event_burst_is_bounded) | Timing corroborated: FDIR events inside every measured release (verification/timing-measured.md) | I-4 statement review (pending, mentor) | **Verified (Analysis/Inspection)** |

## Verification-method control

The "Assigned method" column is the method each requirement was baselined
with (`requirements/hk-tm-baseline.md`, `requirements/fdir-increment.md`).
It is not changed by later evidence. Where tests were added on top of a
Review or Inspection, the Status names the assigned method first and lists
the tests as supplementary evidence. The assigned method stays the basis
for closing the requirement.

| Requirement | Assigned method | How it was satisfied |
|---|---|---|
| HKS-SR-010 | R | Review of the parameter set in `design/data-model.md` and `design/architecture.md`, baselined through the Session 4 review (PR #2). |
| HKS-SR-030 | R | Review of the parameter-source interface (`hk_param_source.h`, `design/architecture.md`), baselined through the Session 4 review (PR #2). |
| HKS-SR-050 | R | Review of the fixed report structure in `design/data-model.md` (Session 4, PR #2) and the Report Builder in `design/detailed-design.md` (Session 5, PR #3). |
| HKS-SR-080 | I | Inspection of the field-by-field packet mapping in `design/data-model.md` against CCSDS 133.0-B and ECSS-E-ST-70-41C (Session 4, PR #2), and of the encoder against that mapping (Session 7, PR #5). Independent inspection I-1 is pending (`verification/isvv-record.md`). |
| HKS-SR-090 | R | Review of the telemetry-sink interface (`hk_tm_sink.h`, `design/architecture.md`), baselined through the Session 4 review (PR #2). |
| HKS-SR-180 | R | Review of the task design (`design/architecture.md`, `design/detailed-design.md`, Sessions 4 and 5, PRs #2 and #3). The earlier status "Verified (Inspection)" named the supplementary code inspection, not the assigned method. |
| HKS-SR-160, 170 | I | As listed in the table. Assigned and recorded methods agree. |
| HKS-SR-150 | A | As listed in the table. Measurement corroborates and does not replace the analysis. |
| HKS-FD-090 | A, I | As listed in the table. Assigned and recorded methods agree. |

Session 7 status: HKS-SR-010 to 090 verified by host unit test (six Unity
suites, `ctest --test-dir build`) and demonstrated end to end on RTEMS 7
sparc/leon3 under Renode (`tools/renode/run_hk_app.sh`, DD-09). With Session 6
(SR-100 to 130) and Session 5 (SR-150 by analysis), the baseline is verified
except HKS-SR-140's hardware measurement and the system-level (HKS-SYS) and
ISVV activities of Sessions 9 to 10.

Session 8 status: the FDIR increment (requirements/fdir-increment.md)
adds HKS-SYS-040 and HKS-FD-010 to 090. FD-010 to 080 are verified by host
unit test (four new Unity suites, `test_hk_limits`, `test_hk_monitor`,
`test_hk_event`, `test_hk_safemode`, plus FDIR cases added to
`test_hk_collection` and `test_hk_command`). FD-090 is verified by
inspection and analysis, and HKS-SR-150 is re-verified by analysis with
the monitor included. The Renode end-to-end run still passes with FDIR in
the image: four housekeeping packets and no events, since the simulated
values are nominal. Exercising fault scenarios end to end on the target is
Session 9.

Session 9 status: the assembled service, with FDIR, was verified at system
level on the RTEMS 7 sparc/leon3 target under Renode. System tests ST-01 to
ST-10 (verification/system-test-spec.md) all pass
(verification/system-test-results.md), so every behavioural requirement now
has system-level evidence on top of its unit evidence. HKS-SYS-010 to 040
are closed through it. Two integration findings were raised: SYS-1, a
malformed uplink frame was rejected silently (fixed), and SYS-3, safe-mode
state is not observable in telemetry (open, decision needed). The timing
was measured on the running system (verification/timing-measured.md). This
corroborates HKS-SR-150 and found the output path costing more than its
analysis bound (SYS-4, analysis revision 3 due). HKS-SR-140 and the
absolute SR-150 figures still need flight-representative hardware.

ISVV (verification/isvv-record.md) is planned for HKS-SR-080, HKS-FD-030,
HKS-FD-060/080 and a statement review of HKS-FD-010 to 090. The mentor
performs it. Until results are recorded there, the ISVV column reads
"pending", and no requirement is claimed as independently checked.

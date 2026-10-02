# HK-TM Requirements Traceability Matrix

| Requirement | Design element | Verifying test | Status |
|---|---|---|---|
| HKS-SYS-010 | Satisfied via child software requirements (see HKS-SR-020, 060, etc.) | | Open |
| HKS-SYS-020 | Satisfied via child software requirements (see HKS-SR-100, 110, 120, 130) | | Open |
| HKS-SYS-030 | Satisfied via child software requirements (see HKS-SR-070) | | Open |
| HKS-SYS-040 | Satisfied via child software requirements HKS-FD-010 to 090 (requirements/fdir-increment.md) | | Open (system-level fault scenarios, Session 9) |
| HKS-SR-010 | Fixed parameter table `g_param_ids` (src/app/src/hk_param_table.c; data-model.md) | test_param_table_ids_are_unique; test_collect_reads_every_param_once_in_order | **Verified (Test)** |
| HKS-SR-020 | Collection task rate-monotonic period, 1000 ms (tasks/hk_collection_task.c); Sampler (hk_sampler.c; detailed-design.md §1.1, §1.2) | test_collect_reads_every_param_once_in_order; test_enabled_reports_every_divider_cycles; Renode E2E: one cycle per 1 s | **Verified (Test)** |
| HKS-SR-030 | Sampler reads only via `hk_param_source_read` (hk_param_source.h; hk_sampler.c) | test_collect_reads_every_param_once_in_order (stub source) | **Verified (Test)** |
| HKS-SR-040 | Stale marker `0xFFFFFFFF` for any non-OK read, cycle continues (hk_sampler.c) | test_failing_sources_write_stale_marker_and_continue; test_recovered_source_clears_stale_marker; Renode E2E: 0x0203 stale in packets 0 and 3 | **Verified (Test)** |
| HKS-SR-050 | Report Builder fixed SID; fixed value order (hk_report_builder.c; hk_report.h) | test_build_stamps_sid_and_time; SID and value order in test_encoder_matches_known_vector | **Verified (Test)** |
| HKS-SR-060 | Report divider = 4 in `hk_collection_step` (hk_collection.c; DD-01, DD-10) | test_enabled_reports_every_divider_cycles; test_enable_midway_keeps_report_alignment; Renode E2E: 4.000 s report interval | **Verified (Test)** |
| HKS-SR-070 | CUC 4+2 collection time (hk_report_builder.c; hk_time_rtems.c; data-model.md) | test_build_stamps_sid_and_time; CUC field in test_encoder_matches_known_vector and test_sink_receives_encoded_packet | **Verified (Test)** |
| HKS-SR-080 | PUS/CCSDS Encoder, TM[3,25] + PEC (hk_pus_encoder.c; detailed-design.md §1.4) | test_encoder_matches_known_vector (byte-exact, 148 bytes); test_crc16_ccitt_check_value; Renode E2E: packets checked by independent decoder tools/hk_tm_decode.py | **Verified (Test)** |
| HKS-SR-090 | Telemetry-sink interface (hk_tm_sink.h); hand-off in hk_collection.c; console sink on target | test_sink_receives_encoded_packet (capture sink); test_sink_rejection_reported_and_counted | **Verified (Test)** |
| HKS-SR-100 | Command Handler dispatch; HK Manager state flag (detailed-design.md §1.5; DD-07) | test_enable_accepted | **Verified (Test)** |
| HKS-SR-110 | Command Handler dispatch; HK Manager state flag (detailed-design.md §1.5; DD-07) | test_disable_accepted | **Verified (Test)** |
| HKS-SR-120 | `hk_manager_init` default disabled (DD-04) | test_default_state_is_disabled | **Verified (Test)** |
| HKS-SR-130 | Command Handler reject path (detailed-design.md §1.5); still the default case with CLEAR_SAFE_MODE added (fdir-design.md §1) | test_unknown_rejected_no_change; test_unknown_rejected_safe_mode_unchanged; test_only_clear_command_leaves_safe_mode | **Verified (Test)** |
| HKS-SR-140 | Rate-monotonic release of the collection task (tasks/hk_collection_task.c; verification/timing-analysis.md) | Indicative only: Renode E2E max report jitter 0.000 ms, 0 period overruns; emulator timing is not representative. Final: measurement on target hardware | Partial (HW measurement pending) |
| HKS-SR-150 | Verified by Analysis. Evidence: verification/timing-analysis.md. Revision 2 (Session 8) includes the FDIR monitor: 0.99 ms collection cycle, 3.9 ms pathological event burst, against the 50 ms budget. Pending measured confirmation in Session 9. | | **Re-verified (Analysis)** |
| HKS-SR-160 | Static report and packet buffers (hk_collection.c); task storage and queue buffers static via `rtems_*_construct` (tasks/); no malloc in src/app (DD-02) | Inspection of src/app (no allocation calls); RTEMS object counts fixed in rtems/init.c. Supporting run: a Renode hook on `malloc` saw 8 calls, all from RTEMS console/newlib initialization before the tasks start, and none in the 25 s of operation after | **Verified (Inspection)** |
| HKS-SR-170 | Coding standard defined in docs/coding-standard.md. Verification by Inspection, enforced in CI from Session 6. | | **Defined** |
| HKS-SR-180 | Two tasks, static priorities 10/20 and stacks (tasks/hk_tasks.h), created once at startup (rtems/init.c) | Inspection of tasks/hk_tasks.h and rtems/init.c; Renode E2E exercises both tasks | **Verified (Inspection)** |
| HKS-FD-010 | Limit table `g_limits`, four ordered limits per parameter, and `hk_limit_classify` (src/fdir/src/hk_limits.c; fdir-design.md §7) | test_classify_at_and_across_every_threshold; test_classify_without_soft_band; test_flight_table_rows_are_ordered; test_flight_table_holds_simulator_values | **Verified (Test)** |
| HKS-FD-020 | `hk_monitor_check` called once per cycle in `hk_collection_step`, after the build, before the gate (hk_collection.c; fdir-design.md §1) | test_monitor_checks_every_cycle_while_reporting_disabled; test_events_leave_hk_stream_unchanged; test_nominal_report_raises_nothing | **Verified (Test)** |
| HKS-FD-030 | Saturating run counters, HK_DEBOUNCE_N = 3 (hk_monitor.h, hk_monitor.c; fdir-design.md §2.1; DD-13) | test_debounce_confirms_on_Nth_sample; test_interrupted_run_does_not_confirm; test_mixed_soft_and_hard_samples; test_recovery_debounced_then_rearmed | **Verified (Test)** |
| HKS-FD-040 | Limit event on each rise in level, TM[5,s] on APID 0x0CB (hk_monitor.c, hk_event.c; data-model.md "Event report"; DD-12) | test_limit_event_matches_known_vector (byte-exact); test_soft_limit_is_warning (parameter and condition); test_sustained_condition_reported_once; test_event_stamped_with_report_time | **Verified (Test)** |
| HKS-FD-050 | Level WARNING or CRITICAL; event subtype 2 (low) or 4 (high) (hk_event.c; fdir-design.md §3) | test_severity_subtypes; test_soft_limit_is_warning; test_hard_limit_is_critical; test_warning_escalates_to_critical | **Verified (Test)** |
| HKS-FD-060 | Rise to CRITICAL calls `hk_safemode_enter` (hk_monitor.c) | test_debounce_confirms_on_Nth_sample; test_hard_limit_is_critical; test_soft_limit_is_warning (no safe mode); test_critical_steps_down_and_relapse_is_reported | **Verified (Test)** |
| HKS-FD-070 | Stale run counter, HK_STALE_M = 3, one medium-severity event per run (hk_monitor.c; fdir-design.md §2.3) | test_stale_for_M_cycles_emits_once; test_fresh_sample_ends_stale_run; test_stale_sample_does_not_break_limit_run | **Verified (Test)** |
| HKS-FD-080 | Safe-mode indicator under the critical section, idempotent entry with one event, cleared only by `HK_CMD_CLEAR_SAFE_MODE` (hk_safemode.c, hk_command.c; fdir-design.md §4) | test_entry_sets_indicator_and_emits_event; test_repeated_entry_reports_once; test_ground_clear_then_reentry; test_only_clear_command_leaves_safe_mode; test_clear_safe_mode_accepted; test_clear_during_persisting_condition_holds | **Verified (Test)** |
| HKS-FD-090 | All FDIR state static (src/fdir: monitor state, 24-byte event buffer, flag); no allocation calls in src/fdir. Timing: verification/timing-analysis.md revision 2 | Inspection of src/fdir; timing analysis (monitor 176 µs per cycle, ≤ 33 events per cycle: test_worst_case_event_burst_is_bounded) | **Verified (Analysis/Inspection)** |

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

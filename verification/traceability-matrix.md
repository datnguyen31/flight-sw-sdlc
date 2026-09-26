# HK-TM Requirements Traceability Matrix

| Requirement | Design element | Verifying test | Status |
|---|---|---|---|
| HKS-SYS-010 | Satisfied via child software requirements (see HKS-SR-020, 060, etc.) | | Open |
| HKS-SYS-020 | Satisfied via child software requirements (see HKS-SR-100, 110, 120, 130) | | Open |
| HKS-SYS-030 | Satisfied via child software requirements (see HKS-SR-070) | | Open |
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
| HKS-SR-130 | Command Handler reject path (detailed-design.md §1.5) | test_unknown_rejected_no_change | **Verified (Test)** |
| HKS-SR-140 | Rate-monotonic release of the collection task (tasks/hk_collection_task.c; verification/timing-analysis.md) | Indicative only: Renode E2E max report jitter 0.000 ms, 0 period overruns; emulator timing is not representative. Final: measurement on target hardware | Partial (HW measurement pending) |
| HKS-SR-150 | Verified by Analysis. Evidence: verification/timing-analysis.md (0.81 ms collection cycle against 50 ms budget). Pending measured confirmation in Session 9. | | **Verified (Analysis)** |
| HKS-SR-160 | Static report and packet buffers (hk_collection.c); task storage and queue buffers static via `rtems_*_construct` (tasks/); no malloc in src/app (DD-02) | Inspection of src/app (no allocation calls); RTEMS object counts fixed in rtems/init.c. Supporting run: a Renode hook on `malloc` saw 8 calls, all from RTEMS console/newlib initialization before the tasks start, and none in the 25 s of operation after | **Verified (Inspection)** |
| HKS-SR-170 | Coding standard defined in docs/coding-standard.md. Verification by Inspection, enforced in CI from Session 6. | | **Defined** |
| HKS-SR-180 | Two tasks, static priorities 10/20 and stacks (tasks/hk_tasks.h), created once at startup (rtems/init.c) | Inspection of tasks/hk_tasks.h and rtems/init.c; Renode E2E exercises both tasks | **Verified (Inspection)** |

Session 7 status: HKS-SR-010 to 090 verified by host unit test (six Unity
suites, `ctest --test-dir build`) and demonstrated end to end on RTEMS 7
sparc/leon3 under Renode (`tools/renode/run_hk_app.sh`, DD-09). With Session 6
(SR-100 to 130) and Session 5 (SR-150 by analysis), the baseline is verified
except HKS-SR-140's hardware measurement and the system-level (HKS-SYS) and
ISVV activities of Sessions 9 to 10.

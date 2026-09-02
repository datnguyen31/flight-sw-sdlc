# HK-TM Requirements Traceability Matrix

| Requirement | Design element | Verifying test | Status |
|---|---|---|---|
| HKSSYS-010 | Satisfied via child software requirements (see HKSSR-020, 060, etc.) | | Open |
| HKSSYS-020 | Satisfied via child software requirements (see HKSSR-100, 110, 120, 130) | | Open |
| HKSSYS-030 | Satisfied via child software requirements (see HKSSR-070) | | Open |
| HKSSR-010 | Parameter set and `hk_param_id_t` definitions (data-model.md) | | Open |
| HKSSR-020 | Scheduler periodic tick; Sampler read loop | | Open |
| HKSSR-030 | Parameter-source interface (hk_param_source.h) | | Open |
| HKSSR-040 | `HK_PARAM_STALE` handling in Sampler | | Open |
| HKSSR-050 | Report Builder fixed structure; SID field | | Open |
| HKSSR-060 | Scheduler report-divider counter (DD-01) | | Open |
| HKSSR-070 | CUC time field in report (data-model.md) | | Open |
| HKSSR-080 | PUS/CCSDS Encoder, TM[3,25] | | Open |
| HKSSR-090 | Telemetry-sink interface (hk_tm_sink.h) | | Open |
| HKSSR-100 | Command Handler dispatch; HK Manager state flag | | Open |
| HKSSR-110 | Command Handler dispatch; HK Manager state flag | | Open |
| HKSSR-120 | `hk_manager_init` default disabled (DD-04) | | Open |
| HKSSR-130 | Command Handler reject path | | Open |
| HKSSR-140 | Rate-monotonic period on collection task (RTEMS) | | Open |
| HKSSR-150 | Small per-tick work; timing analysis deferred to Session 5 | | Open |
| HKSSR-160 | Static buffers only (DD-02) | | Open |
| HKSSR-170 | Coding standard; detailed in Session 5 | | Open |
| HKSSR-180 | Two-task model with static priorities and stacks | | Open |
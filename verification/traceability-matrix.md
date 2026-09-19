# HK-TM Requirements Traceability Matrix

| Requirement | Design element | Verifying test | Status |
|---|---|---|---|
| HKS-SYS-010 | Satisfied via child software requirements (see HKS-SR-020, 060, etc.) | | Open |
| HKS-SYS-020 | Satisfied via child software requirements (see HKS-SR-100, 110, 120, 130) | | Open |
| HKS-SYS-030 | Satisfied via child software requirements (see HKS-SR-070) | | Open |
| HKS-SR-010 | Parameter set and `hk_param_id_t` definitions (data-model.md) | | Open |
| HKS-SR-020 | Scheduler periodic tick; Sampler read loop | | Open |
| HKS-SR-030 | Parameter-source interface (hk_param_source.h) | | Open |
| HKS-SR-040 | `HK_PARAM_STALE` handling in Sampler | | Open |
| HKS-SR-050 | Report Builder fixed structure; SID field | | Open |
| HKS-SR-060 | Scheduler report-divider counter (DD-01) | | Open |
| HKS-SR-070 | CUC time field in report (data-model.md) | | Open |
| HKS-SR-080 | PUS/CCSDS Encoder, TM[3,25] | | Open |
| HKS-SR-090 | Telemetry-sink interface (hk_tm_sink.h) | | Open |
| HKS-SR-100 | Command Handler dispatch; HK Manager state flag | | Open |
| HKS-SR-110 | Command Handler dispatch; HK Manager state flag | | Open |
| HKS-SR-120 | `hk_manager_init` default disabled (DD-04) | | Open |
| HKS-SR-130 | Command Handler reject path | | Open |
| HKS-SR-140 | Rate-monotonic period on collection task (RTEMS) | | Open |
| HKS-SR-150 | Small per-tick work; timing analysis deferred to Session 5 | | Open |
| HKS-SR-160 | Static buffers only (DD-02) | | Open |
| HKS-SR-170 | Coding standard; detailed in Session 5 | | Open |
| HKS-SR-180 | Two-task model with static priorities and stacks | | Open |

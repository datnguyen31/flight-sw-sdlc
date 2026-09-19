# HK-TM Requirements Traceability Matrix

| Requirement | Design element | Verifying test | Status |
|---|---|---|---|
| HKS-SYS-010 | Satisfied via child software requirements (see HKS-SR-020, 060, etc.) | | Open |
| HKS-SYS-020 | Satisfied via child software requirements (see HKS-SR-100, 110, 120, 130) | | Open |
| HKS-SYS-030 | Satisfied via child software requirements (see HKS-SR-070) | | Open |
| HKS-SR-010 | Parameter set and `hk_param_id_t` definitions (data-model.md) | | Open |
| HKS-SR-020 | Scheduler periodic tick; Sampler read loop (detailed-design.md §1.1, §1.2) | | Open |
| HKS-SR-030 | Parameter-source interface (hk_param_source.h); Sampler (detailed-design.md §1.2) | | Open |
| HKS-SR-040 | `HK_PARAM_STALE`/`HK_PARAM_UNKNOWN` handling in Sampler (detailed-design.md §1.2) | | Open |
| HKS-SR-050 | Report Builder fixed structure; SID field (detailed-design.md §1.3) | | Open |
| HKS-SR-060 | Scheduler report-divider counter (DD-01; detailed-design.md §1.1) | | Open |
| HKS-SR-070 | CUC time field in report (data-model.md; detailed-design.md §1.3) | | Open |
| HKS-SR-080 | PUS/CCSDS Encoder, TM[3,25] (detailed-design.md §1.4) | | Open |
| HKS-SR-090 | Telemetry-sink interface (hk_tm_sink.h) | | Open |
| HKS-SR-100 | Command Handler dispatch; HK Manager state flag (detailed-design.md §1.5; DD-07) | | Open |
| HKS-SR-110 | Command Handler dispatch; HK Manager state flag (detailed-design.md §1.5; DD-07) | | Open |
| HKS-SR-120 | `hk_manager_init` default disabled (DD-04) | | Open |
| HKS-SR-130 | Command Handler reject path (detailed-design.md §1.5) | | Open |
| HKS-SR-140 | Jitter argument documented via RTEMS rate-monotonic release (verification/timing-analysis.md); final verification by Test during integration | | Open |
| HKS-SR-150 | Verified by Analysis. Evidence: verification/timing-analysis.md (0.81 ms collection cycle against 50 ms budget). Pending measured confirmation in Session 9. | | **Verified (Analysis)** |
| HKS-SR-160 | Static buffers only (DD-02; detailed-design.md §1.3) | | Open |
| HKS-SR-170 | Coding standard defined in docs/coding-standard.md. Verification by Inspection, enforced in CI from Session 6. | | **Defined** |
| HKS-SR-180 | Two-task model with static priorities and stacks (architecture.md; detailed-design.md §1.6) | | Open |

# Design Decisions

| ID | Decision and Rationale |
|---|---|
| DD-01 | A single periodic collection task with a report divider, rather than two independent tasks. One rate-monotonic period drives collection; a counter decides when a report is emitted. This guarantees each report aligns to a completed collection cycle (HKS-SR-060) and keeps timing reasoning simple. |
| DD-02 | Static allocation only. All buffers, including the report buffer, are sized at build time. No allocation after initialization (HKS-SR-160), avoiding fragmentation and non-deterministic timing. |
| DD-03 | Parameter access and telemetry output go through pure C interfaces with swappable implementations. Real providers and a real downlink in flight, stubs in unit test (HKS-SR-030, HKS-SR-090). Testability is designed in, not retrofitted. |
| DD-04 | Reporting state is a single flag, disabled at init. Enable and disable commands set it; the report step consults it (HKS-SR-100, 110, 120). A quiet start is easier to verify than autonomous output. |
| DD-05 | Fixed APID and fixed structure identifier for the baseline. Configurable report definitions are deferred to a later increment, keeping the first structure simple (HKS-SR-050). |
| DD-06 | Target RTOS: **RTEMS**, using its rate-monotonic period service for the collection task. Chosen for consistency with prior RTEMS background and alignment with the QEMU emulation environment set up in Session 2. Architecture, interfaces, and data model are RTOS-agnostic; only the task/timing primitives are RTEMS-specific. |
# Housekeeping Telemetry — Baseline Requirements

Verification methods: **T** = Test, **A** = Analysis, **I** = Inspection, **R** = Review of design.

## System-level context requirements

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-SYS-010 | The spacecraft shall periodically report housekeeping telemetry for ground-based health monitoring. | Operators must observe platform health throughout the mission. | T | Mission |
| HKS-SYS-020 | The spacecraft shall allow ground operators to enable and disable housekeeping telemetry generation. | Telemetry volume and downlink budget must be controllable from ground. | T | Mission |
| HKS-SYS-030 | Housekeeping telemetry shall be time-stamped with onboard time. | Telemetry is only diagnostic if each sample can be placed on the mission timeline. | T | Mission |

## Software requirements: acquisition

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-SR-010 | The HK service shall maintain a defined set of housekeeping parameters, each identified by a unique parameter identifier. | A stable parameter set is the basis for report structure and later monitoring. | R | SYS-010 |
| HKS-SR-020 | The HK service shall sample all defined housekeeping parameters at a configurable collection interval with a default of 1000 ms. | Periodic sampling decouples acquisition rate from report rate. | T | SYS-010 |
| HKS-SR-030 | The HK service shall obtain each parameter value through a defined parameter-source interface and shall not depend on the internal implementation of any parameter provider. | Decoupling enables unit testing with stubbed sources and future FDIR insertion. | R | SYS-010 |
| HKS-SR-040 | If a parameter source does not provide a value within one collection cycle, the HK service shall record a defined stale marker for that parameter and continue operation. | A single failing source must not stop health reporting; precursor to FDIR. | T | SYS-010 |

## Software requirements: reporting

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-SR-050 | The HK service shall assemble sampled parameters into a housekeeping report of fixed structure for the baseline increment. | A fixed structure keeps the first increment simple; configurability comes later. | R | SYS-010 |
| HKS-SR-060 | The HK service shall generate a periodic housekeeping report at a configurable generation interval, default 4000 ms, which shall be an integer multiple of the collection interval. | Aligning rates guarantees each report contains a complete, consistent sample set. | T | SYS-010 |
| HKS-SR-070 | Each housekeeping report shall carry an onboard timestamp corresponding to the collection time of the parameters it contains. | Places every value on the mission timeline. | T | SYS-030 |
| HKS-SR-080 | The HK service shall encode each housekeeping report as a CCSDS space packet consistent with the PUS Service 3 housekeeping report structure. | Standard encoding enables integration with any compliant ground segment. | I | SYS-010 |
| HKS-SR-090 | The HK service shall emit reports through a defined telemetry-sink interface, independent of the underlying transport. | Allows testing against a capture stub now and a real downlink later. | R | SYS-010 |

## Software requirements: command and control

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-SR-100 | The HK service shall accept a telecommand to enable periodic housekeeping report generation. | Gives ground control of the telemetry stream. | T | SYS-020 |
| HKS-SR-110 | The HK service shall accept a telecommand to disable periodic housekeeping report generation. | Conserve downlink budget and reduce noise during quiet operations. | T | SYS-020 |
| HKS-SR-120 | On initialization the HK service shall default to periodic report generation disabled. | A quiet, deterministic start is safer and easier to verify than autonomous chatter. | T | SYS-020 |
| HKS-SR-130 | The HK service shall reject a malformed or unrecognized telecommand without changing its reporting state. | Robustness against corrupted uplink; precursor to fault handling. | T | SYS-020 |

## Software requirements: timing, resources, and constraints

| ID | Statement | Rationale | Method | Parent |
|---|---|---|---|---|
| HKS-SR-140 | The HK service shall generate periodic reports with a timing jitter not exceeding 10 percent of the configured generation interval. | Bounded jitter keeps telemetry cadence predictable for ground processing. | T | SYS-010 |
| HKS-SR-150 | The HK service shall complete one collection cycle within 50 ms under nominal load. | A bounded, short cycle fits an RTOS task budget and leaves margin. | A | SYS-010 |
| HKS-SR-160 | The HK service shall not perform dynamic memory allocation after initialization. | Standard flight software practice; avoids fragmentation and non-deterministic timing. | I | SYS-010 |
| HKS-SR-170 | The HK service shall be implemented in C conforming to the project MISRA C:2012 subset defined in the coding standard. | Enforceable, analyzable, and consistent with the target environment. | I | SYS-010 |
| HKS-SR-180 | The HK service shall execute as one or more RTOS tasks with statically defined priorities and stack sizes, targeting RTEMS. | Deterministic scheduling and resource use, verifiable by inspection. | R | SYS-010 |

# HK-TM Data Model and PUS Packet Mapping

A report is a set of parameter values plus the onboard time at which they
were collected. On the wire it is a PUS Service 3, subtype 25 housekeeping
parameter report, carried in a CCSDS space packet. The layout below is the
baseline; field widths follow the standard, with simplifications noted.

## CCSDS Space Packet Primary Header

| Field | Size | Contents |
|---|---|---|
| Packet ID | 2 bytes | Version, type = TM, secondary header flag = 1, APID (fixed for baseline). |
| Packet sequence control | 2 bytes | Sequence flags and packet sequence count. |
| Packet data length | 2 bytes | Length of everything after the primary header, minus one. |

## PUS TM Secondary Header

| Field | Size | Contents |
|---|---|---|
| PUS version and flags | 1 byte | TM PUS version number and spacecraft time reference status. |
| Service / subtype | 2 bytes | Service type = 3, message subtype = 25 (housekeeping parameter report). |
| Message counter | 2 bytes | Telemetry message type counter. Not maintained in the baseline: always 0. |
| Time | 6 bytes | Onboard collection time, CUC 4+2 (HKS-SR-070); see below. |

## Source Data (Housekeeping Report)

| Field | Size | Contents |
|---|---|---|
| Structure identifier | 1 byte | Fixed SID for the baseline report definition (HKS-SR-050). |
| Parameter values | 4 bytes x 32 | The 32 defined parameters (`src/app/src/hk_param_table.c`) in fixed order, each an unsigned 32-bit value; a stale value carries the stale marker `0xFFFFFFFF` (HKS-SR-040). |

## Packet Error Control

| Field | Size | Contents |
|---|---|---|
| PEC | 2 bytes | CRC-16/CCITT-FALSE (polynomial 0x1021, initial value 0xFFFF, no reflection, no final XOR), the ECSS-E-ST-70-41C packet error control, over every preceding byte of the packet. |

## Baseline field values

The encoder (`src/app/src/hk_pus_encoder.c`) fixes these values; the
byte-exact vector in `test/unit/test_hk_pus_encoder.c` pins them.

| Field | Value |
|---|---|
| Packet ID | `0x08CA`: version 0, TM, secondary header present, APID `0x0CA` (DD-05) |
| Sequence control | `0b11` (unsegmented) + 14-bit count, incremented per generated packet |
| Packet data length | 141 (`0x008D`): 142 bytes follow the primary header |
| PUS version byte | `0x20`: PUS-C (version 2), time reference status 0 |
| Service / subtype | 3 / 25 |
| SID | 1 |
| Total packet length | 148 bytes |

**Time format.** CUC with 4 octets of coarse time (seconds) and 2 of fine
time (units of 2^-16 s), big-endian, preamble field implicit (not
transmitted). In memory the report holds it as one integer,
`coarse << 16 | fine`. The epoch is processor boot: the baseline has no time
correlation with ground, which a later increment adds without changing the
field layout.

**Simplifications against PUS-C.** The TM secondary header here omits the
message type counter's per-type maintenance (sent as 0) and the destination
ID field; both are acceptable for a single-report, single-ground baseline
and are recorded here so a ground decoder is configured to match.

## Event report (FDIR, PUS Service 5)

FDIR event reports (HKS-FD-040, design/fdir-design.md §3) use the same
primary header, secondary header and PEC as the housekeeping report.
`hk_pus_put_tm_header()` and `hk_pus_put_pec()` write both, so the two
packet kinds share one implementation. The source data is an event
definition id followed by fixed auxiliary data.

| Field | Size | Contents |
|---|---|---|
| Event definition id | 2 bytes | 1 = limit, 2 = stale parameter, 3 = safe mode entered. |
| Parameter id | 2 bytes | The parameter concerned; `0x0000` for safe mode. |
| Condition | 1 byte | Limit event: `hk_limit_state_t` (1 warn low, 2 warn high, 3 crit low, 4 crit high). Stale event: the parameter's last `hk_param_status_t` (1 stale, 2 unknown). Safe mode: 0. |

| Field | Value |
|---|---|
| Packet ID | `0x08CB`: version 0, TM, secondary header present, APID `0x0CB` (DD-12) |
| Sequence control | `0b11` (unsegmented) + 14-bit count, separate from the housekeeping count, incremented per generated event |
| Packet data length | 17 (`0x0011`): 18 bytes follow the primary header |
| Service / subtype | 5 / severity: 1 informative, 2 low (warning limit), 3 medium (stale), 4 high (critical limit, safe mode) |
| Time | Collection time of the report that confirmed the event, CUC 4+2 |
| Total packet length | 24 bytes |

The byte-exact vector in `test/unit/test_hk_event.c` pins this layout.

**Notes:**
- Endianness for multi-byte fields is big-endian per CCSDS.
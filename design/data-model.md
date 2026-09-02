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
| Message counter | 2 bytes | Telemetry message type counter. |
| Time | N bytes | Onboard collection time in CUC format (HKS-SR-070). |

## Source Data (Housekeeping Report)

| Field | Size | Contents |
|---|---|---|
| Structure identifier | 1 byte | Fixed SID for the baseline report definition (HKS-SR-050). |
| Parameter values | M bytes | The defined parameters in fixed order; a stale value carries its defined stale marker (HKS-SR-040). |

**Notes:**
- The stale marker and exact parameter widths are pinned down in Session 5
  detailed design, alongside the timing budget.
- Endianness for multi-byte fields is big-endian per CCSDS.
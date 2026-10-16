#ifndef HK_PUS_ENCODER_H
#define HK_PUS_ENCODER_H

/*
 * PUS / CCSDS Encoder (HKS-SR-080; header helpers also HKS-FD-040)
 *
 * Serializes a report as a CCSDS space packet carrying a PUS TM[3,25]
 * housekeeping parameter report, big-endian, per design/data-model.md.
 * Pure: report in, byte buffer out. The header and PEC writers are public
 * so every TM packet the service emits (the FDIR event reports in
 * src/fdir/hk_event.c) shares one header discipline.
 */

#include <stddef.h>
#include <stdint.h>

#include "hk_report.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HK_APID        0x0CAu /* fixed for the baseline (DD-05) */
#define HK_PUS_SVC     3u
#define HK_PUS_SUBTYPE 25u
#define HK_TIME_BYTES  6u /* CUC: 4 coarse + 2 fine (data-model) */

/* Primary header, PUS TM secondary header, source data, PEC. */
#define HK_PRIMARY_HDR_BYTES   6u
#define HK_SECONDARY_HDR_BYTES (1u + 2u + 2u + HK_TIME_BYTES)
#define HK_SOURCE_DATA_BYTES   (1u + (4u * HK_PARAM_COUNT))
#define HK_PEC_BYTES           2u
#define HK_TM_HEADER_BYTES     (HK_PRIMARY_HDR_BYTES + HK_SECONDARY_HDR_BYTES)
#define HK_MAX_PACKET          (HK_TM_HEADER_BYTES + HK_SOURCE_DATA_BYTES + HK_PEC_BYTES)

    /* The variable fields of a TM packet's primary and secondary headers. */
    typedef struct
    {
        uint16_t apid;         /* 11 bits used */
        uint16_t seq;          /* packet sequence count, 14 bits used */
        uint8_t  service;      /* PUS service type */
        uint8_t  subtype;      /* PUS message subtype */
        uint16_t packet_bytes; /* total packet length, headers through PEC */
        uint64_t time_cuc;     /* CUC 4+2 (data-model.md) */
    } hk_tm_header_t;

    /* CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF), the ECSS-E-ST-70-41C
     * packet error control. Returns 0xFFFF if data is NULL. */
    uint16_t hk_crc16_ccitt(const uint8_t* data, size_t length);

    /* Write v big-endian at p; returns the byte after it. */
    uint8_t* hk_pus_put16(uint8_t* p, uint16_t v);

    /* Write the CCSDS primary and PUS-C TM secondary headers at buf, which
     * must hold HK_TM_HEADER_BYTES. Returns the first source-data byte. */
    uint8_t* hk_pus_put_tm_header(uint8_t* buf, const hk_tm_header_t* hdr);

    /* Append the PEC over [buf, end) at end, which must leave room for
     * HK_PEC_BYTES. Returns the total packet length. */
    size_t hk_pus_put_pec(const uint8_t* buf, uint8_t* end);

    /* Encode report as TM[3,25] with sequence count seq (14 bits used).
     * Returns the packet length, or 0 if an argument is NULL or cap is below
     * HK_MAX_PACKET, in which case buf is left untouched. */
    size_t hk_pus_encode(const hk_report_t* report, uint16_t seq, uint8_t* buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* HK_PUS_ENCODER_H */

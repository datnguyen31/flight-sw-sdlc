#ifndef HK_PUS_ENCODER_H
#define HK_PUS_ENCODER_H

/*
 * PUS / CCSDS Encoder (HKS-SR-080)
 *
 * Serializes a report as a CCSDS space packet carrying a PUS TM[3,25]
 * housekeeping parameter report, big-endian, per design/data-model.md.
 * Pure: report in, byte buffer out.
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
#define HK_MAX_PACKET                                                                              \
    (HK_PRIMARY_HDR_BYTES + HK_SECONDARY_HDR_BYTES + HK_SOURCE_DATA_BYTES + HK_PEC_BYTES)

    /* CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF), the ECSS-E-ST-70-41C
     * packet error control. Returns 0xFFFF if data is NULL. */
    uint16_t hk_crc16_ccitt(const uint8_t* data, size_t length);

    /* Encode report as TM[3,25] with sequence count seq (14 bits used).
     * Returns the packet length, or 0 if an argument is NULL or cap is below
     * HK_MAX_PACKET, in which case buf is left untouched. */
    size_t hk_pus_encode(const hk_report_t* report, uint16_t seq, uint8_t* buf, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* HK_PUS_ENCODER_H */

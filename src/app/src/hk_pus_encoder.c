/*
 * hk_pus_encoder.c
 *
 * PUS / CCSDS Encoder (HKS-SR-080). Writes the CCSDS primary header, the
 * PUS-C TM secondary header for service 3 subtype 25, the source data, and
 * the packet error control, all big-endian, and never writes past the
 * caller's buffer. The layout is design/data-model.md; the byte-exact
 * vector in test/unit/test_hk_pus_encoder.c pins it down. The header and
 * PEC writers are shared with the FDIR event encoder (src/fdir/hk_event.c).
 */

#include "hk_pus_encoder.h"

#define CCSDS_SEC_HDR_FLAG    (1u << 11) /* version 0, type TM (0) */
#define CCSDS_SEQ_UNSEGMENTED (3u << 14)
#define CCSDS_SEQ_COUNT_MASK  0x3FFFu
#define CCSDS_APID_MASK       0x7FFu
#define PUS_TM_VERSION_BYTE   (2u << 4) /* PUS-C, time-ref status 0 */
#define CRC16_CCITT_POLY      0x1021u
#define CRC16_CCITT_INIT      0xFFFFu

uint8_t* hk_pus_put16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
    return &p[2];
}

static uint8_t* put32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
    return &p[4];
}

uint16_t hk_crc16_ccitt(const uint8_t* data, size_t length)
{
    uint16_t crc = CRC16_CCITT_INIT;

    if (data == NULL)
    {
        return crc;
    }

    for (size_t i = 0u; i < length; i++)
    {
        crc = (uint16_t)(crc ^ (uint16_t)((uint16_t)data[i] << 8));
        for (uint8_t bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x8000u) != 0u)
            {
                crc = (uint16_t)((uint16_t)(crc << 1) ^ CRC16_CCITT_POLY);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

uint8_t* hk_pus_put_tm_header(uint8_t* buf, const hk_tm_header_t* hdr)
{
    /* Packet data length counts everything after the primary header. */
    const uint16_t data_len = (uint16_t)(hdr->packet_bytes - HK_PRIMARY_HDR_BYTES - 1u);
    uint8_t*       p        = buf;

    /* CCSDS primary header: version 0, TM, sec-hdr=1, APID; unsegmented. */
    p = hk_pus_put16(p, (uint16_t)(CCSDS_SEC_HDR_FLAG | (hdr->apid & CCSDS_APID_MASK)));
    p = hk_pus_put16(p, (uint16_t)(CCSDS_SEQ_UNSEGMENTED | (hdr->seq & CCSDS_SEQ_COUNT_MASK)));
    p = hk_pus_put16(p, data_len);

    /* PUS TM secondary header (PUS-C, data-model.md) */
    *p++ = (uint8_t)PUS_TM_VERSION_BYTE;
    *p++ = hdr->service;
    *p++ = hdr->subtype;
    p    = hk_pus_put16(p, 0u); /* message type counter: not maintained in baseline */
    for (uint8_t b = 0u; b < HK_TIME_BYTES; b++) /* CUC, big-endian (SR-070) */
    {
        const uint8_t shift = (uint8_t)(8u * (HK_TIME_BYTES - 1u - b));
        *p++                = (uint8_t)(hdr->time_cuc >> shift);
    }
    return p;
}

size_t hk_pus_put_pec(const uint8_t* buf, uint8_t* end)
{
    /* Packet error control over everything before it */
    const uint8_t* p = hk_pus_put16(end, hk_crc16_ccitt(buf, (size_t)(end - buf)));
    return (size_t)(p - buf);
}

size_t hk_pus_encode(const hk_report_t* report, uint16_t seq, uint8_t* buf, size_t cap)
{
    if ((report == NULL) || (buf == NULL) || (cap < HK_MAX_PACKET))
    {
        return 0u; /* never overflow the caller's buffer */
    }

    const hk_tm_header_t hdr = {.apid         = HK_APID,
                                .seq          = seq,
                                .service      = HK_PUS_SVC,
                                .subtype      = HK_PUS_SUBTYPE,
                                .packet_bytes = HK_MAX_PACKET,
                                .time_cuc     = report->collection_time_cuc};
    uint8_t*             p   = hk_pus_put_tm_header(buf, &hdr);

    /* Source data: SID then values in fixed order (SR-050) */
    *p++ = report->structure_id;
    for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++)
    {
        p = put32(p, report->values[i]);
    }

    return hk_pus_put_pec(buf, p);
}

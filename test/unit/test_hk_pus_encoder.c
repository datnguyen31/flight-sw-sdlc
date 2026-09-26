/* test/unit/test_hk_pus_encoder.c (verifies HKS-SR-080, SR-050, SR-070) */
#include "unity.h"
#include "hk_report.h"
#include "hk_pus_encoder.h"

#include <string.h>

void setUp(void) { }
void tearDown(void) { }

static hk_report_t vector_report(void) {
  hk_report_t r;
  memset(&r, 0, sizeof r);
  r.structure_id = HK_BASELINE_SID;
  r.collection_time_cuc = 0x0000000102030405u; /* coarse 0x00010203, fine 0x0405 */
  r.values[0] = 0x11223344u;
  return r;
}

/* The complete on-the-wire packet, derived by hand from design/data-model.md.
 * The PEC was computed independently of this code base (Python
 * binascii.crc_hqx, init 0xFFFF == CRC-16/CCITT-FALSE). */
static const uint8_t k_expected[HK_MAX_PACKET] = {
    /* CCSDS primary header */
    0x08, 0xCA,             /* ver 0, TM, sec-hdr 1, APID 0x0CA */
    0xC0, 0x00,             /* unsegmented, seq count 0 */
    0x00, 0x8D,             /* data length: 142 bytes follow, minus 1 */
    /* PUS-C TM secondary header */
    0x20,                   /* PUS version 2, time-ref status 0 */
    0x03, 0x19,             /* service 3, subtype 25 */
    0x00, 0x00,             /* message type counter */
    0x00, 0x01, 0x02, 0x03, /* CUC coarse */
    0x04, 0x05,             /* CUC fine */
    /* source data */
    0x01,                   /* SID */
    0x11, 0x22, 0x33, 0x44, /* values[0] */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[1..4]   */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[5..8]   */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[9..12]  */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[13..16] */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[17..20] */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[21..24] */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* values[25..28] */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,                /* values[29..31] */
    /* packet error control */
    0xBB, 0x3B,
};

/* ECSS PEC is CRC-16/CCITT-FALSE: standard check value over "123456789" */
void test_crc16_ccitt_check_value(void) {
  const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TEST_ASSERT_EQUAL_HEX16(0x29B1u, hk_crc16_ccitt(check, sizeof check));
}

/* SR-080: the whole packet matches the known vector byte for byte */
void test_encoder_matches_known_vector(void) {
  hk_report_t r = vector_report();
  uint8_t buf[256];

  size_t n = hk_pus_encode(&r, 0u, buf, sizeof buf);

  TEST_ASSERT_EQUAL_size_t(sizeof k_expected, n);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(k_expected, buf, sizeof k_expected);
}

/* The packet data length field agrees with the bytes actually written */
void test_encoder_length_field_consistent(void) {
  hk_report_t r = vector_report();
  uint8_t buf[HK_MAX_PACKET];

  size_t n = hk_pus_encode(&r, 0u, buf, sizeof buf);

  TEST_ASSERT_EQUAL_size_t(HK_MAX_PACKET, n);
  TEST_ASSERT_EQUAL_UINT16(n - 6u - 1u, (uint16_t)((buf[4] << 8) | buf[5]));
}

/* Sequence count occupies 14 bits; the flags are never disturbed */
void test_encoder_sequence_count_masked_to_14_bits(void) {
  hk_report_t r = vector_report();
  uint8_t buf[HK_MAX_PACKET];

  (void)hk_pus_encode(&r, 0x4001u, buf, sizeof buf);

  TEST_ASSERT_EQUAL_HEX8(0xC0, buf[2]);
  TEST_ASSERT_EQUAL_HEX8(0x01, buf[3]);
}

/* Never writes past the caller's buffer: too small -> 0, buffer untouched */
void test_encoder_refuses_short_buffer(void) {
  hk_report_t r = vector_report();
  uint8_t buf[HK_MAX_PACKET];
  memset(buf, 0x5A, sizeof buf);

  size_t n = hk_pus_encode(&r, 0u, buf, HK_MAX_PACKET - 1u);

  TEST_ASSERT_EQUAL_size_t(0u, n);
  for (size_t i = 0u; i < sizeof buf; i++) {
    TEST_ASSERT_EQUAL_HEX8(0x5A, buf[i]);
  }
}

/* Coding standard rule 7: invalid arguments rejected */
void test_encoder_rejects_null_arguments(void) {
  hk_report_t r = vector_report();
  uint8_t buf[HK_MAX_PACKET];

  TEST_ASSERT_EQUAL_size_t(0u, hk_pus_encode(NULL, 0u, buf, sizeof buf));
  TEST_ASSERT_EQUAL_size_t(0u, hk_pus_encode(&r, 0u, NULL, sizeof buf));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_crc16_ccitt_check_value);
  RUN_TEST(test_encoder_matches_known_vector);
  RUN_TEST(test_encoder_length_field_consistent);
  RUN_TEST(test_encoder_sequence_count_masked_to_14_bits);
  RUN_TEST(test_encoder_refuses_short_buffer);
  RUN_TEST(test_encoder_rejects_null_arguments);
  return UNITY_END();
}

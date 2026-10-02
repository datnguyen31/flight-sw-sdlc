/* test/unit/test_hk_report_builder.c (verifies HKS-SR-050, SR-070) */
#include "unity.h"
#include "hk_report_builder.h"

#include <string.h>

void setUp(void) { }
void tearDown(void) { }

/* HKS-SR-050: fixed structure id; HKS-SR-070: collection timestamp */
void test_build_stamps_sid_and_time(void) {
  hk_report_t r;
  memset(&r, 0, sizeof r);

  hk_report_build(&r, 0x0000123456789ABCu);

  TEST_ASSERT_EQUAL_UINT8(HK_BASELINE_SID, r.structure_id);
  TEST_ASSERT_TRUE(r.collection_time_cuc == 0x0000123456789ABCu);
}

/* The builder stamps only; collected values and status are untouched */
void test_build_leaves_values_untouched(void) {
  hk_report_t r;
  memset(&r, 0, sizeof r);
  r.values[0] = 0x11111111u;
  r.values[HK_PARAM_COUNT - 1u] = HK_STALE_MARKER;
  r.status[3] = 1u;

  hk_report_build(&r, 1u);

  TEST_ASSERT_EQUAL_HEX32(0x11111111u, r.values[0]);
  TEST_ASSERT_EQUAL_HEX32(HK_STALE_MARKER, r.values[HK_PARAM_COUNT - 1u]);
  TEST_ASSERT_EQUAL_UINT8(1u, r.status[3]);
}

/* A later build replaces the timestamp (each report its own collection) */
void test_rebuild_updates_time(void) {
  hk_report_t r;
  memset(&r, 0, sizeof r);

  hk_report_build(&r, 100u);
  hk_report_build(&r, 200u);

  TEST_ASSERT_TRUE(r.collection_time_cuc == 200u);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_build_stamps_sid_and_time);
  RUN_TEST(test_build_leaves_values_untouched);
  RUN_TEST(test_rebuild_updates_time);
  return UNITY_END();
}

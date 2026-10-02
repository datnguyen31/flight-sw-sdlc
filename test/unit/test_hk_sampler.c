/* test/unit/test_hk_sampler.c (verifies HKS-SR-010, 020, 030, 040) */
#include "unity.h"
#include "hk_sampler.h"
#include "hk_test_doubles.h"

#include <string.h>

static hk_report_t s_r;

void setUp(void) {
  hk_stub_source_reset();
  memset(&s_r, 0, sizeof s_r);
}
void tearDown(void) { }

/* HKS-SR-010: a defined set, each parameter uniquely identified */
void test_param_table_ids_are_unique(void) {
  for (size_t i = 0u; i < HK_PARAM_COUNT; i++) {
    TEST_ASSERT_NOT_EQUAL(0u, g_param_ids[i]);
    for (size_t j = i + 1u; j < HK_PARAM_COUNT; j++) {
      TEST_ASSERT_NOT_EQUAL(g_param_ids[i], g_param_ids[j]);
    }
  }
}

/* HKS-SR-020, SR-030: one cycle reads every parameter once, in table
 * order, through the parameter-source interface */
void test_collect_reads_every_param_once_in_order(void) {
  hk_sampler_collect(&s_r);

  TEST_ASSERT_EQUAL_size_t(HK_PARAM_COUNT, hk_stub_source_read_count());
  for (size_t i = 0u; i < HK_PARAM_COUNT; i++) {
    TEST_ASSERT_EQUAL_UINT16(g_param_ids[i], hk_stub_source_read_id(i));
    TEST_ASSERT_EQUAL_HEX32(hk_stub_value(g_param_ids[i]), s_r.values[i]);
    TEST_ASSERT_EQUAL_UINT8(HK_PARAM_OK, s_r.status[i]);
  }
}

/* HKS-SR-040: stale and unknown sources record the stale marker and the
 * cycle continues past them */
void test_failing_sources_write_stale_marker_and_continue(void) {
  hk_stub_source_set_status(g_param_ids[5], HK_PARAM_STALE);
  hk_stub_source_set_status(g_param_ids[20], HK_PARAM_UNKNOWN);

  hk_sampler_collect(&s_r);

  TEST_ASSERT_EQUAL_size_t(HK_PARAM_COUNT, hk_stub_source_read_count());
  TEST_ASSERT_EQUAL_HEX32(HK_STALE_MARKER, s_r.values[5]);
  TEST_ASSERT_EQUAL_UINT8(HK_PARAM_STALE, s_r.status[5]);
  TEST_ASSERT_EQUAL_HEX32(HK_STALE_MARKER, s_r.values[20]);
  TEST_ASSERT_EQUAL_UINT8(HK_PARAM_UNKNOWN, s_r.status[20]);
  /* neighbours, and the parameter after the last failure, are fresh */
  TEST_ASSERT_EQUAL_HEX32(hk_stub_value(g_param_ids[6]), s_r.values[6]);
  TEST_ASSERT_EQUAL_HEX32(hk_stub_value(g_param_ids[31]), s_r.values[31]);
}

/* A value recovered on the next cycle replaces the stale marker */
void test_recovered_source_clears_stale_marker(void) {
  hk_stub_source_set_status(g_param_ids[0], HK_PARAM_STALE);
  hk_sampler_collect(&s_r);
  hk_stub_source_reset();

  hk_sampler_collect(&s_r);

  TEST_ASSERT_EQUAL_HEX32(hk_stub_value(g_param_ids[0]), s_r.values[0]);
  TEST_ASSERT_EQUAL_UINT8(HK_PARAM_OK, s_r.status[0]);
}

/* Coding standard rule 7: NULL report is rejected without reading */
void test_collect_null_report_is_ignored(void) {
  hk_sampler_collect(NULL);
  TEST_ASSERT_EQUAL_size_t(0u, hk_stub_source_read_count());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_param_table_ids_are_unique);
  RUN_TEST(test_collect_reads_every_param_once_in_order);
  RUN_TEST(test_failing_sources_write_stale_marker_and_continue);
  RUN_TEST(test_recovered_source_clears_stale_marker);
  RUN_TEST(test_collect_null_report_is_ignored);
  return UNITY_END();
}

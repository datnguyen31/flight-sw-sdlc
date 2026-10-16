/* test/unit/test_hk_limits.c (verifies HKS-FD-010) */
#include "unity.h"
#include "hk_limits.h"

#include <stddef.h>

#define ADC_FULL_SCALE 4095u
#define SIM_SPAN 64u /* hk_param_source_sim.c: nominal + (cycle % 64) */

void setUp(void) { }
void tearDown(void) { }

static const hk_limit_t k_row = {100u, 200u, 300u, 400u};

/* Every threshold, approached from both sides, on one row */
void test_classify_at_and_across_every_threshold(void) {
  TEST_ASSERT_EQUAL(HK_CRIT_LOW, hk_limit_classify(0u, &k_row));
  TEST_ASSERT_EQUAL(HK_CRIT_LOW, hk_limit_classify(100u, &k_row));
  TEST_ASSERT_EQUAL(HK_WARN_LOW, hk_limit_classify(101u, &k_row));
  TEST_ASSERT_EQUAL(HK_WARN_LOW, hk_limit_classify(199u, &k_row));
  TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(200u, &k_row));
  TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(250u, &k_row));
  TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(300u, &k_row));
  TEST_ASSERT_EQUAL(HK_WARN_HIGH, hk_limit_classify(301u, &k_row));
  TEST_ASSERT_EQUAL(HK_WARN_HIGH, hk_limit_classify(399u, &k_row));
  TEST_ASSERT_EQUAL(HK_CRIT_HIGH, hk_limit_classify(400u, &k_row));
  TEST_ASSERT_EQUAL(HK_CRIT_HIGH, hk_limit_classify(0xFFFFFFFFu, &k_row));
}

/* A row with no soft band on one side goes straight to critical */
void test_classify_without_soft_band(void) {
  const hk_limit_t row = {100u, 101u, 300u, 301u};
  TEST_ASSERT_EQUAL(HK_CRIT_LOW, hk_limit_classify(100u, &row));
  TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(101u, &row));
  TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(300u, &row));
  TEST_ASSERT_EQUAL(HK_CRIT_HIGH, hk_limit_classify(301u, &row));
}

/* Coding standard rule 7: no limits means nothing to flag */
void test_classify_null_limits_is_ok(void) {
  TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(0u, NULL));
}

/* FD-010: every parameter has four ordered limits, inside the 12-bit ADC
 * range, so a stuck-at-zero or full-scale reading is critical */
void test_flight_table_rows_are_ordered(void) {
  for (size_t i = 0u; i < HK_PARAM_COUNT; i++) {
    const hk_limit_t *l = &g_limits[i];
    TEST_ASSERT_TRUE_MESSAGE(l->low_crit < l->low_warn, "low_crit < low_warn");
    TEST_ASSERT_TRUE_MESSAGE(l->low_warn <= l->high_warn, "low_warn <= high_warn");
    TEST_ASSERT_TRUE_MESSAGE(l->high_warn < l->high_crit, "high_warn < high_crit");
    TEST_ASSERT_TRUE_MESSAGE(l->high_crit <= ADC_FULL_SCALE, "within ADC range");
    TEST_ASSERT_EQUAL(HK_CRIT_LOW, hk_limit_classify(0u, l));
    TEST_ASSERT_EQUAL(HK_CRIT_HIGH, hk_limit_classify(ADC_FULL_SCALE, l));
  }
}

/* The target simulator's values, mid soft band plus up to SIM_SPAN - 1,
 * stay nominal for every parameter */
void test_flight_table_holds_simulator_values(void) {
  for (size_t i = 0u; i < HK_PARAM_COUNT; i++) {
    const hk_limit_t *l = &g_limits[i];
    const uint32_t mid = (l->low_warn + l->high_warn) / 2u;
    TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(mid, l));
    TEST_ASSERT_EQUAL(HK_LIMIT_OK, hk_limit_classify(mid + SIM_SPAN - 1u, l));
  }
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_classify_at_and_across_every_threshold);
  RUN_TEST(test_classify_without_soft_band);
  RUN_TEST(test_classify_null_limits_is_ok);
  RUN_TEST(test_flight_table_rows_are_ordered);
  RUN_TEST(test_flight_table_holds_simulator_values);
  return UNITY_END();
}

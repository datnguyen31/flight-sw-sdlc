/* test/unit/test_hk_monitor.c (verifies HKS-FD-020, FD-030, FD-040,
 * FD-050, FD-060, FD-070) */
#include "unity.h"
#include "hk_event.h"
#include "hk_limits.h"
#include "hk_monitor.h"
#include "hk_safemode.h"
#include "hk_sampler.h"
#include "hk_test_doubles.h"

static hk_report_t s_r;

/* Event packet fields (design/data-model.md, "Event report") */
#define EVT_SUBTYPE 8u
#define EVT_ID 18u
#define EVT_PARAM 19u
#define EVT_COND 21u

/* Parameter 0 (0x0101) relative to its limits */
#define P0 0u
#define BELOW_HARD_LOW (g_limits[P0].low_crit)
#define BELOW_SOFT_LOW (g_limits[P0].low_warn - 1u)
#define ABOVE_SOFT_HIGH (g_limits[P0].high_warn + 1u)
#define ABOVE_HARD_HIGH (g_limits[P0].high_crit)
#define IN_LIMITS (hk_stub_value(g_param_ids[P0]))

void setUp(void) {
  hk_monitor_init();
  hk_safemode_init();
  hk_event_init();
  hk_capture_sink_reset();
  s_r.structure_id = HK_BASELINE_SID;
  s_r.collection_time_cuc = 0u;
  for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++) {
    s_r.values[i] = hk_stub_value(g_param_ids[i]);
    s_r.status[i] = (uint8_t)HK_PARAM_OK;
  }
}
void tearDown(void) { }

static void check_n(uint32_t value, uint32_t cycles) {
  s_r.values[P0] = value;
  for (uint32_t k = 0u; k < cycles; k++) {
    hk_monitor_check(&s_r);
  }
}

static size_t events(void) { return hk_capture_sink_count_service(HK_EVT_SVC); }

static size_t events_with_id(hk_event_id_t id) {
  size_t n = 0u;
  for (size_t i = 0u; i < events(); i++) {
    if (hk_capture_sink_packet(i)[EVT_ID] == (uint8_t)id) {
      n++;
    }
  }
  return n;
}

static const uint8_t *last_limit_event(void) {
  const uint8_t *last = NULL;
  for (size_t i = 0u; i < events(); i++) {
    if (hk_capture_sink_packet(i)[EVT_ID] == (uint8_t)HK_EVT_ID_LIMIT) {
      last = hk_capture_sink_packet(i);
    }
  }
  TEST_ASSERT_NOT_NULL(last);
  return last;
}

/* FD-020: nominal values, checked every cycle, raise nothing */
void test_nominal_report_raises_nothing(void) {
  check_n(IN_LIMITS, 10u);
  TEST_ASSERT_EQUAL_size_t(0u, events());
  TEST_ASSERT_FALSE(hk_safemode_active());
}

/* FD-030, FD-060: N - 1 out-of-limit samples emit nothing; the Nth
 * confirms, emits exactly one limit event, and enters safe mode */
void test_debounce_confirms_on_Nth_sample(void) {
  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N - 1u);
  TEST_ASSERT_EQUAL_size_t(0u, events());
  TEST_ASSERT_FALSE(hk_safemode_active());

  check_n(BELOW_HARD_LOW, 1u);
  TEST_ASSERT_EQUAL_size_t(1u, events_with_id(HK_EVT_ID_LIMIT));
  TEST_ASSERT_EQUAL_size_t(1u, events_with_id(HK_EVT_ID_SAFE_MODE));
  TEST_ASSERT_TRUE(hk_safemode_active());
}

/* FD-030: an in-limits sample breaks the run, so a transient does not
 * confirm */
void test_interrupted_run_does_not_confirm(void) {
  for (uint32_t k = 0u; k < 4u; k++) {
    check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N - 1u);
    check_n(IN_LIMITS, 1u);
  }
  TEST_ASSERT_EQUAL_size_t(0u, events());
}

/* A sustained condition is acted on once, not every cycle */
void test_sustained_condition_reported_once(void) {
  check_n(BELOW_HARD_LOW, 10u * HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(1u, events_with_id(HK_EVT_ID_LIMIT));
  TEST_ASSERT_EQUAL_size_t(1u, events_with_id(HK_EVT_ID_SAFE_MODE));
}

/* FD-040, FD-050: a soft-limit condition is a warning: low severity, the
 * parameter and the condition in the event, no safe mode */
void test_soft_limit_is_warning(void) {
  check_n(BELOW_SOFT_LOW, HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(1u, events());
  const uint8_t *e = last_limit_event();
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_LOW, e[EVT_SUBTYPE]);
  TEST_ASSERT_EQUAL_HEX8(0x01, e[EVT_PARAM]);
  TEST_ASSERT_EQUAL_HEX8(0x01, e[EVT_PARAM + 1u]);
  TEST_ASSERT_EQUAL_HEX8(HK_WARN_LOW, e[EVT_COND]);
  TEST_ASSERT_FALSE(hk_safemode_active());
}

/* FD-050: a hard-limit condition is critical: high severity */
void test_hard_limit_is_critical(void) {
  check_n(ABOVE_HARD_HIGH, HK_DEBOUNCE_N);
  const uint8_t *e = last_limit_event();
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_HIGH, e[EVT_SUBTYPE]);
  TEST_ASSERT_EQUAL_HEX8(HK_CRIT_HIGH, e[EVT_COND]);
  TEST_ASSERT_TRUE(hk_safemode_active());
}

/* Fig. 1 ladder: a confirmed warning that worsens to the hard limit
 * escalates to critical, with its own event and safe mode */
void test_warning_escalates_to_critical(void) {
  check_n(ABOVE_SOFT_HIGH, HK_DEBOUNCE_N + 2u);
  TEST_ASSERT_EQUAL_HEX8(HK_WARN_HIGH, last_limit_event()[EVT_COND]);
  TEST_ASSERT_FALSE(hk_safemode_active());

  check_n(ABOVE_HARD_HIGH, HK_DEBOUNCE_N - 1u);
  TEST_ASSERT_EQUAL_size_t(1u, events());
  check_n(ABOVE_HARD_HIGH, 1u);
  TEST_ASSERT_EQUAL_size_t(2u, events_with_id(HK_EVT_ID_LIMIT));
  TEST_ASSERT_EQUAL_HEX8(HK_CRIT_HIGH, last_limit_event()[EVT_COND]);
  TEST_ASSERT_TRUE(hk_safemode_active());
}

/* FD-030: N consecutive out-of-limit samples confirm at least a warning
 * even when they are a mix of soft and hard; the hard run confirms
 * critical once it reaches N too */
void test_mixed_soft_and_hard_samples(void) {
  check_n(BELOW_SOFT_LOW, 1u);
  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N - 1u);
  TEST_ASSERT_EQUAL_size_t(1u, events());
  TEST_ASSERT_EQUAL_HEX8(HK_WARN_LOW, last_limit_event()[EVT_COND]);
  TEST_ASSERT_FALSE(hk_safemode_active());

  check_n(BELOW_HARD_LOW, 1u);
  TEST_ASSERT_EQUAL_HEX8(HK_CRIT_LOW, last_limit_event()[EVT_COND]);
  TEST_ASSERT_TRUE(hk_safemode_active());
}

/* Recovery takes N in-limits samples too; once NOMINAL, a new condition
 * is confirmed and reported again */
void test_recovery_debounced_then_rearmed(void) {
  check_n(BELOW_SOFT_LOW, HK_DEBOUNCE_N);
  check_n(IN_LIMITS, HK_DEBOUNCE_N - 1u);
  check_n(BELOW_SOFT_LOW, HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(1u, events()); /* never left WARNING */

  check_n(IN_LIMITS, HK_DEBOUNCE_N);
  check_n(BELOW_SOFT_LOW, HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(2u, events());
}

/* A critical condition easing to the soft band steps down to WARNING
 * without an event, so a relapse to the hard limit is reported again */
void test_critical_steps_down_and_relapse_is_reported(void) {
  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N);
  hk_safemode_clear();
  check_n(BELOW_SOFT_LOW, HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(1u, events_with_id(HK_EVT_ID_LIMIT));

  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(2u, events_with_id(HK_EVT_ID_LIMIT));
  TEST_ASSERT_EQUAL_size_t(2u, events_with_id(HK_EVT_ID_SAFE_MODE));
  TEST_ASSERT_TRUE(hk_safemode_active());
}

/* FD-080: clearing safe mode while the critical condition persists does
 * not re-enter it; the condition was already acted on */
void test_clear_during_persisting_condition_holds(void) {
  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N);
  hk_safemode_clear();
  check_n(BELOW_HARD_LOW, 5u * HK_DEBOUNCE_N);
  TEST_ASSERT_FALSE(hk_safemode_active());
  TEST_ASSERT_EQUAL_size_t(2u, events());
}

/* FD-070: stale for M - 1 cycles is nothing; the Mth emits one stale
 * event (medium severity), once per stale run, and no safe mode */
void test_stale_for_M_cycles_emits_once(void) {
  s_r.status[P0] = (uint8_t)HK_PARAM_STALE;
  s_r.values[P0] = HK_STALE_MARKER;
  for (uint32_t k = 0u; k < HK_STALE_M - 1u; k++) {
    hk_monitor_check(&s_r);
  }
  TEST_ASSERT_EQUAL_size_t(0u, events());

  for (uint32_t k = 0u; k < 5u * HK_STALE_M; k++) {
    hk_monitor_check(&s_r);
  }
  TEST_ASSERT_EQUAL_size_t(1u, events());
  const uint8_t *e = hk_capture_sink_packet(0u);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_MEDIUM, e[EVT_SUBTYPE]);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_ID_STALE, e[EVT_ID]);
  TEST_ASSERT_EQUAL_HEX8(HK_PARAM_STALE, e[EVT_COND]);
  TEST_ASSERT_FALSE(hk_safemode_active());
}

/* FD-070: one fresh sample ends the stale run; a new run of M reports
 * again. The stale marker itself is never limit-checked. */
void test_fresh_sample_ends_stale_run(void) {
  for (uint32_t run = 0u; run < 2u; run++) {
    s_r.status[P0] = (uint8_t)HK_PARAM_UNKNOWN;
    s_r.values[P0] = HK_STALE_MARKER;
    for (uint32_t k = 0u; k < HK_STALE_M; k++) {
      hk_monitor_check(&s_r);
    }
    s_r.status[P0] = (uint8_t)HK_PARAM_OK;
    check_n(IN_LIMITS, 1u);
  }
  TEST_ASSERT_EQUAL_size_t(2u, events_with_id(HK_EVT_ID_STALE));
  TEST_ASSERT_EQUAL_size_t(0u, events_with_id(HK_EVT_ID_LIMIT));
}

/* A stale sample carries no value, so it neither counts toward nor
 * breaks a limit run */
void test_stale_sample_does_not_break_limit_run(void) {
  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N - 1u);
  s_r.status[P0] = (uint8_t)HK_PARAM_STALE;
  hk_monitor_check(&s_r);
  TEST_ASSERT_EQUAL_size_t(0u, events());
  s_r.status[P0] = (uint8_t)HK_PARAM_OK;
  check_n(BELOW_HARD_LOW, 1u);
  TEST_ASSERT_TRUE(hk_safemode_active());
}

/* Each parameter carries its own state; safe mode is entered once */
void test_parameters_are_independent(void) {
  s_r.values[5] = g_limits[5].high_crit;
  check_n(BELOW_HARD_LOW, HK_DEBOUNCE_N);
  TEST_ASSERT_EQUAL_size_t(2u, events_with_id(HK_EVT_ID_LIMIT));
  TEST_ASSERT_EQUAL_size_t(1u, events_with_id(HK_EVT_ID_SAFE_MODE));
  TEST_ASSERT_EQUAL_HEX8(0x06, last_limit_event()[EVT_PARAM + 1u]); /* 0x0106 */
}

/* Timing analysis bound: the worst cycle confirms every parameter
 * critical at once, HK_PARAM_COUNT limit events plus one safe-mode event */
void test_worst_case_event_burst_is_bounded(void) {
  for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++) {
    s_r.values[i] = g_limits[i].high_crit;
  }
  for (uint32_t k = 0u; k < HK_DEBOUNCE_N - 1u; k++) {
    hk_monitor_check(&s_r);
  }
  TEST_ASSERT_EQUAL_size_t(0u, events());
  hk_monitor_check(&s_r);
  TEST_ASSERT_EQUAL_size_t(HK_PARAM_COUNT + 1u, events());
}

/* Events carry the collection time of the report that confirmed them */
void test_event_stamped_with_report_time(void) {
  s_r.collection_time_cuc = 0x0000123456789ABCu;
  check_n(BELOW_SOFT_LOW, HK_DEBOUNCE_N);
  const uint8_t cuc[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(cuc, &last_limit_event()[11], sizeof cuc);
}

/* Coding standard rule 7: a NULL report is ignored */
void test_null_report_is_ignored(void) {
  hk_monitor_check(NULL);
  TEST_ASSERT_EQUAL_size_t(0u, hk_capture_sink_count());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_nominal_report_raises_nothing);
  RUN_TEST(test_debounce_confirms_on_Nth_sample);
  RUN_TEST(test_interrupted_run_does_not_confirm);
  RUN_TEST(test_sustained_condition_reported_once);
  RUN_TEST(test_soft_limit_is_warning);
  RUN_TEST(test_hard_limit_is_critical);
  RUN_TEST(test_warning_escalates_to_critical);
  RUN_TEST(test_mixed_soft_and_hard_samples);
  RUN_TEST(test_recovery_debounced_then_rearmed);
  RUN_TEST(test_critical_steps_down_and_relapse_is_reported);
  RUN_TEST(test_clear_during_persisting_condition_holds);
  RUN_TEST(test_stale_for_M_cycles_emits_once);
  RUN_TEST(test_fresh_sample_ends_stale_run);
  RUN_TEST(test_stale_sample_does_not_break_limit_run);
  RUN_TEST(test_parameters_are_independent);
  RUN_TEST(test_worst_case_event_burst_is_bounded);
  RUN_TEST(test_event_stamped_with_report_time);
  RUN_TEST(test_null_report_is_ignored);
  return UNITY_END();
}

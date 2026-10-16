/* test/support/hk_test_doubles.c */
#include "hk_test_doubles.h"

#include "hk_limits.h"
#include "hk_sampler.h"
#include "hk_tm_sink.h"

#define MAX_OVERRIDES 8u
#define MAX_READ_LOG 256u

static struct {
  hk_param_id_t id;
  hk_param_status_t status;
  uint32_t value;
} s_overrides[MAX_OVERRIDES];
static size_t s_override_count;
static hk_param_id_t s_read_log[MAX_READ_LOG];
static size_t s_read_count;

static uint8_t s_last[HK_MAX_PACKET];
static size_t s_last_len;
static uint8_t s_log[HK_CAPTURE_LOG][HK_MAX_PACKET];
static size_t s_sink_count;
static bool s_sink_accept = true;

void hk_stub_source_reset(void) {
  s_override_count = 0u;
  s_read_count = 0u;
}

static void set_override(hk_param_id_t id, hk_param_status_t status,
                         uint32_t value) {
  size_t i = 0u;
  while ((i < s_override_count) && (s_overrides[i].id != id)) {
    i++;
  }
  if (i == MAX_OVERRIDES) {
    return;
  }
  if (i == s_override_count) {
    s_override_count++;
  }
  s_overrides[i].id = id;
  s_overrides[i].status = status;
  s_overrides[i].value = value;
}

void hk_stub_source_set_status(hk_param_id_t id, hk_param_status_t status) {
  set_override(id, status, 0xDEADBEEFu); /* garbage the sampler must not keep */
}

void hk_stub_source_set_value(hk_param_id_t id, uint32_t value) {
  set_override(id, HK_PARAM_OK, value);
}

/* Middle of the soft band: nominal, so the FDIR monitor stays quiet. */
uint32_t hk_stub_value(hk_param_id_t id) {
  for (size_t i = 0u; i < HK_PARAM_COUNT; i++) {
    if (g_param_ids[i] == id) {
      return (g_limits[i].low_warn + g_limits[i].high_warn) / 2u;
    }
  }
  return 0u;
}

size_t hk_stub_source_read_count(void) { return s_read_count; }

hk_param_id_t hk_stub_source_read_id(size_t call_index) {
  return (call_index < MAX_READ_LOG) ? s_read_log[call_index] : 0u;
}

hk_param_status_t hk_param_source_read(hk_param_id_t id, uint32_t *out_value) {
  if (s_read_count < MAX_READ_LOG) {
    s_read_log[s_read_count] = id;
  }
  s_read_count++;
  for (size_t i = 0u; i < s_override_count; i++) {
    if (s_overrides[i].id == id) {
      *out_value = s_overrides[i].value;
      return s_overrides[i].status;
    }
  }
  *out_value = hk_stub_value(id);
  return HK_PARAM_OK;
}

void hk_capture_sink_reset(void) {
  s_last_len = 0u;
  s_sink_count = 0u;
  s_sink_accept = true;
}

void hk_capture_sink_set_accept(bool accept) { s_sink_accept = accept; }

size_t hk_capture_sink_count(void) { return s_sink_count; }

size_t hk_capture_sink_last_len(void) { return s_last_len; }

const uint8_t *hk_capture_sink_last(void) { return s_last; }

const uint8_t *hk_capture_sink_packet(size_t index) {
  return ((index < s_sink_count) && (index < HK_CAPTURE_LOG)) ? s_log[index]
                                                              : NULL;
}

size_t hk_capture_sink_count_service(uint8_t svc) {
  size_t n = 0u;
  for (size_t i = 0u; (i < s_sink_count) && (i < HK_CAPTURE_LOG); i++) {
    if (s_log[i][7] == svc) {
      n++;
    }
  }
  return n;
}

bool hk_tm_sink_send(const uint8_t *packet, size_t length) {
  s_last_len = (length <= sizeof s_last) ? length : sizeof s_last;
  for (size_t i = 0u; i < s_last_len; i++) {
    s_last[i] = packet[i];
    if (s_sink_count < HK_CAPTURE_LOG) {
      s_log[s_sink_count][i] = packet[i];
    }
  }
  s_sink_count++;
  return s_sink_accept;
}

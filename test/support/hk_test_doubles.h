#ifndef HK_TEST_DOUBLES_H
#define HK_TEST_DOUBLES_H

/*
 * Test doubles for the two external interfaces (DD-03), linked only into
 * the host tests: a scriptable parameter source (hk_param_source.h) and a
 * capture telemetry sink (hk_tm_sink.h).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hk_param_source.h"
#include "hk_pus_encoder.h"

/* Stub source: every id reads HK_PARAM_OK with value hk_stub_value(id)
 * unless overridden. Reads are logged in call order. */
void hk_stub_source_reset(void);
void hk_stub_source_set_status(hk_param_id_t id, hk_param_status_t status);
uint32_t hk_stub_value(hk_param_id_t id);
size_t hk_stub_source_read_count(void);
hk_param_id_t hk_stub_source_read_id(size_t call_index);

/* Capture sink: keeps a copy of the last packet and a call count. */
void hk_capture_sink_reset(void);
void hk_capture_sink_set_accept(bool accept);
size_t hk_capture_sink_count(void);
size_t hk_capture_sink_last_len(void);
const uint8_t *hk_capture_sink_last(void);

#endif /* HK_TEST_DOUBLES_H */

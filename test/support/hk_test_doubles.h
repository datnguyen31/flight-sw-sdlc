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

/* Stub source: every id reads HK_PARAM_OK with value hk_stub_value(id), a
 * nominal value inside the id's soft limits (g_limits), unless overridden.
 * An override sets a status (the value is garbage) or an in-range value
 * read as HK_PARAM_OK; a second override of the same id replaces the first.
 * Reads are logged in call order. */
void hk_stub_source_reset(void);
void hk_stub_source_set_status(hk_param_id_t id, hk_param_status_t status);
void hk_stub_source_set_value(hk_param_id_t id, uint32_t value);
uint32_t hk_stub_value(hk_param_id_t id);
size_t hk_stub_source_read_count(void);
hk_param_id_t hk_stub_source_read_id(size_t call_index);

/* Capture sink: keeps a copy of the first HK_CAPTURE_LOG packets and of
 * the last one, and a call count. */
#define HK_CAPTURE_LOG 64u
void hk_capture_sink_reset(void);
void hk_capture_sink_set_accept(bool accept);
size_t hk_capture_sink_count(void);
size_t hk_capture_sink_last_len(void);
const uint8_t *hk_capture_sink_last(void);
/* Packet index (0 = first sent since reset), or NULL past the log. */
const uint8_t *hk_capture_sink_packet(size_t index);
/* Packets sent with PUS service type svc (byte 7), within the log. */
size_t hk_capture_sink_count_service(uint8_t svc);

#endif /* HK_TEST_DOUBLES_H */

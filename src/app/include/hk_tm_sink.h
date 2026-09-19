#ifndef HK_TM_SINK_H
#define HK_TM_SINK_H

/*
 * Telemetry-Sink Interface (HKS-SR-090)
 *
 * Abstracts the downlink so the encoder can hand off a completed packet
 * without knowing how (or whether, in test) it is actually transmitted.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Accept one fully encoded TM packet for downlink. Returns true if accepted. */
bool hk_tm_sink_send(const uint8_t *packet, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* HK_TM_SINK_H */

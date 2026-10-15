#ifndef ST_RIG_H
#define ST_RIG_H

/*
 * System test rig (Session 9; verification/system-test-spec.md)
 *
 * The fault injector half of the representative parameter provider
 * (st_param_provider.c). The scenario sequencer (st_main.c) calls these
 * between collection cycles to make one parameter read a scripted value
 * or go stale, and to put it back. Everything else about the service is
 * the flight code, driven only through its interfaces.
 */

#include <stdint.h>

#include "hk_param_source.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* From the next collection cycle, parameter id reads value. */
    void st_fault_value(hk_param_id_t id, uint32_t value);

    /* From the next collection cycle, parameter id reads HK_PARAM_STALE. */
    void st_fault_stale(hk_param_id_t id);

    /* From the next collection cycle, parameter id reads nominal again. */
    void st_fault_clear(hk_param_id_t id);

    /* The nominal value of parameter index i (0..HK_PARAM_COUNT-1). */
    uint32_t st_nominal_value(uint8_t i);

#ifdef __cplusplus
}
#endif

#endif /* ST_RIG_H */

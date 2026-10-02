/*
 * hk_param_table.c
 *
 * The baseline housekeeping parameter set (HKS-SR-010): a fixed, explicit
 * table of unique parameter identifiers, in report order (HKS-SR-050).
 * The high byte groups parameters by subsystem; identifiers are
 * illustrative for this capstone. HK_PARAM_COUNT is the size the Session 5
 * timing budget assumed.
 */

#include "hk_sampler.h"

const hk_param_id_t g_param_ids[HK_PARAM_COUNT] = {
    /* EPS: bus voltages, currents, battery */
    0x0101u, 0x0102u, 0x0103u, 0x0104u, 0x0105u, 0x0106u, 0x0107u, 0x0108u,
    /* Thermal: board and panel temperatures */
    0x0201u, 0x0202u, 0x0203u, 0x0204u, 0x0205u, 0x0206u, 0x0207u, 0x0208u,
    /* OBC: counters and health */
    0x0301u, 0x0302u, 0x0303u, 0x0304u, 0x0305u, 0x0306u, 0x0307u, 0x0308u,
    /* AOCS: sensor and actuator status */
    0x0401u, 0x0402u, 0x0403u, 0x0404u, 0x0405u, 0x0406u, 0x0407u, 0x0408u,
};

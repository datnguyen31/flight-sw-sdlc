#ifndef HK_SAFEMODE_H
#define HK_SAFEMODE_H

/*
 * Safe-mode controller (HKS-FD-060, HKS-FD-080)
 *
 * Holds the system safe-mode indicator. The collection task sets it on a
 * confirmed critical condition; the command task clears it on the ground
 * CLEAR_SAFE_MODE telecommand, and nothing else does. It is shared between
 * the two tasks, so every access is under the critical section
 * (hk_critical.h; design/fdir-design.md).
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Safe mode inactive. Call once at startup, after hk_critical_init(). */
    void hk_safemode_init(void);

    /* Enter safe mode (FD-060). Idempotent: only the first entry emits the
     * safe-mode event, stamped now_cuc (FD-080). Collection task only. */
    void hk_safemode_enter(uint64_t now_cuc);

    bool hk_safemode_active(void);

    /* Leave safe mode. Ground-commanded recovery only (FD-080). */
    void hk_safemode_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* HK_SAFEMODE_H */

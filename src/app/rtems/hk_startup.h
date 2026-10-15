#ifndef HK_STARTUP_H
#define HK_STARTUP_H

/*
 * HK-TM service startup (target build only; HKS-SR-120, HKS-SR-180)
 *
 * The one startup sequence every target image runs: the flight image
 * (init.c) and the system test image (test/system/st_main.c). Sharing it
 * means the system tests boot the service exactly as flight does.
 *
 * Order matters: hk_critical_init() creates the mutex the HK Manager and
 * the safe-mode controller use, so it runs before hk_manager_init() and
 * hk_safemode_init(); all module state is initialized before either task
 * starts (hk_critical.h). A failure is fatal: there is no partial service.
 */

#include <rtems.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define HK_FATAL_STARTUP 0x484B494Eu /* 'HKIN' */

    /* Print what failed and stop with HK_FATAL_STARTUP unless sc succeeded. */
    void hk_startup_check(rtems_status_code sc, const char* what);

    /* Initialize every module, then start the command and collection
     * tasks. Reporting is disabled (SR-120) and safe mode inactive. */
    void hk_service_start(void);

#ifdef __cplusplus
}
#endif

#endif /* HK_STARTUP_H */

#ifndef HK_TASKS_H
#define HK_TASKS_H

/*
 * HK-TM RTEMS tasks (HKS-SR-180; design/architecture.md task model)
 *
 * Two statically defined tasks. Priorities and stack sizes are fixed here
 * and inspected against the architecture (SR-180); task storage and the
 * command queue buffers are statically allocated and handed to RTEMS with
 * the *_construct directives, so no task resource comes from the heap
 * (SR-160). RTEMS priorities: lower number is more urgent.
 */

#include <rtems.h>
#include <stdint.h>

#include "hk_command.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Command task above collection task (architecture.md). */
#define HK_COMMAND_TASK_PRIORITY    10u
#define HK_COLLECTION_TASK_PRIORITY 20u
#define HK_TASK_STACK_SIZE          (2u * RTEMS_MINIMUM_STACK_SIZE)
#define HK_TASK_MAX_TLS_SIZE        256u /* = CONFIGURE_MAXIMUM_THREAD_LOCAL_STORAGE_SIZE */

#define HK_COLLECTION_PERIOD_MS 1000u /* SR-020 default */
#define HK_COMMAND_QUEUE_DEPTH  4u

    /* Construct the command queue and start the command task. */
    rtems_status_code hk_command_task_start(void);

    /* Construct and start the collection task. */
    rtems_status_code hk_collection_task_start(void);

    /* Uplink ingress: queue one raw uplink frame for the command task. A
     * frame longer than a telecommand is refused (RTEMS_INVALID_SIZE); a
     * shorter one is rejected by the command task (SR-130). */
    rtems_status_code hk_command_uplink_frame(const void* frame, size_t size);

    /* Uplink ingress: queue one well-formed telecommand. */
    rtems_status_code hk_command_uplink(hk_command_id_t cmd);

    /* Collection-cycle timing measured on the running system, kept and
     * printed by the collection task (verification/timing-measured.md).
     * Times in nanoseconds of RTEMS uptime. */
    typedef struct
    {
        uint32_t releases;       /* collection cycles run */
        uint32_t overruns;       /* rate-monotonic period overruns */
        uint64_t collect_min_ns;    /* best release that sent no report */
        uint64_t collect_max_ns;    /* worst release that sent no report */
        uint32_t collect_max_cycle; /* ... and the cycle it was */
        uint64_t report_max_ns;     /* worst release that encoded and sent a report */
        uint32_t report_max_cycle;  /* ... and the cycle it was */
        uint64_t jitter_max_ns;     /* worst |release - ideal release| */
        uint32_t jitter_max_cycle;  /* ... and the cycle it was */
    } hk_timing_t;

#ifdef __cplusplus
}
#endif

#endif /* HK_TASKS_H */

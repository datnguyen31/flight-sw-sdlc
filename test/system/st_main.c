/*
 * st_main.c (system test image only)
 *
 * Scenario sequencer and command injector for the system tests ST-01 to
 * ST-10 (verification/system-test-spec.md). The service is started exactly
 * as in flight (hk_service_start(), hk_rtems_config.h); this file replaces
 * only the flight image's ground script.
 *
 * One boot runs every scenario in order, each starting from the state the
 * previous one left, as the spec's entry conditions state. The script is a
 * table of steps at fixed times since boot. Collection releases fall on
 * whole seconds (release k at about k - 1 s), so every step is at a
 * half-second: a command or fault lands between two releases and takes
 * effect from the next one.
 *
 * The rig prints "[ST] ST-xx begin/end" markers and one "[fault]" or
 * "[ground]" line per step. These are test-harness output, not service
 * output; test/system/st_capture.py uses them only to split the log into
 * scenarios and to know when a fault started. Pass and fail are decided on
 * what the service itself emits: TM packets, command verification and its
 * cycle status lines.
 *
 * After the last scenario the image keeps running with reporting enabled
 * (the soak used for verification/timing-measured.md) until the emulator
 * stops it.
 */

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/version.h>

#include "hk_command.h"
#include "hk_startup.h"
#include "hk_tasks.h"
#include "st_rig.h"

#define NS_PER_MS 1000000u

/* ST-04: an opcode no telecommand uses, and a frame too short to be one.
 * The short frame is the first byte of DISABLE_REPORTS as uplinked: if
 * the command task read it as a telecommand at all, reporting would stop. */
#define ST_UNKNOWN_TC ((hk_command_id_t)0x7F)
static const uint8_t k_truncated_frame[1] = {0x02u};

/* Fault values against src/fdir/src/hk_limits.c */
#define P_OBC_BOARD_T  0x0201u /* hard high 3300 */
#define P_BATT_PACK_T  0x0203u
#define P_MAIN_BUS_V   0x0101u /* soft high 3300, hard high 3500 */
#define P_BATT_V       0x0102u /* hard high 3600 */
#define V_BEYOND_HARD  3400u   /* 0x0201: past its hard limit */
#define V_SOFT_BAND    3400u   /* 0x0101: past soft, inside hard */
#define V_HARD_BATT_V  3700u   /* 0x0102: past its hard limit */

typedef enum
{
    STEP_BEGIN,  /* scenario marker; text = scenario id and title */
    STEP_END,    /* scenario marker; text = scenario id */
    STEP_TC,     /* uplink telecommand arg */
    STEP_FRAME,  /* uplink the truncated frame */
    STEP_VALUE,  /* parameter id reads value */
    STEP_STALE,  /* parameter id goes stale */
    STEP_CLEAR,  /* parameter id back to nominal */
    STEP_SOAK    /* scenarios done */
} step_kind_t;

typedef struct
{
    uint32_t      at_ms; /* since boot */
    step_kind_t   kind;
    uint32_t      arg; /* telecommand, or parameter id */
    uint32_t      value;
    const char*   text;
} step_t;

/* clang-format off */
static const step_t k_script[] = {
    {    0u, STEP_BEGIN, 0u, 0u, "ST-03 power-on, no telecommand" },
    { 9500u, STEP_END,   0u, 0u, "ST-03" },

    { 9500u, STEP_BEGIN, 0u, 0u, "ST-01 enable reporting" },
    { 9500u, STEP_TC,    HK_CMD_ENABLE_REPORTS, 0u, "ENABLE_REPORTS" },
    {21500u, STEP_END,   0u, 0u, "ST-01" },

    {21500u, STEP_BEGIN, 0u, 0u, "ST-05 packet format" },
    {25500u, STEP_END,   0u, 0u, "ST-05" },

    {25500u, STEP_BEGIN, 0u, 0u, "ST-04 malformed and unknown telecommands" },
    {25500u, STEP_FRAME, 0u, 0u, "truncated 1-byte frame" },
    {26500u, STEP_TC,    (uint32_t)ST_UNKNOWN_TC, 0u, "unknown telecommand 0x7F" },
    {33500u, STEP_END,   0u, 0u, "ST-04" },

    {33500u, STEP_BEGIN, 0u, 0u, "ST-02 disable reporting" },
    {33500u, STEP_TC,    HK_CMD_DISABLE_REPORTS, 0u, "DISABLE_REPORTS" },
    {42500u, STEP_END,   0u, 0u, "ST-02" },

    {42500u, STEP_BEGIN, 0u, 0u, "ST-10 out-of-limit transients of 1 and N-1 samples" },
    {43500u, STEP_VALUE, P_OBC_BOARD_T, V_BEYOND_HARD, NULL },
    {44500u, STEP_CLEAR, P_OBC_BOARD_T, 0u, NULL },
    {45500u, STEP_VALUE, P_OBC_BOARD_T, V_BEYOND_HARD, NULL }, /* N - 1 samples */
    {47500u, STEP_CLEAR, P_OBC_BOARD_T, 0u, NULL },
    {49500u, STEP_END,   0u, 0u, "ST-10" },

    {49500u, STEP_BEGIN, 0u, 0u, "ST-06 stale parameter" },
    {49500u, STEP_TC,    HK_CMD_ENABLE_REPORTS, 0u, "ENABLE_REPORTS" },
    {50500u, STEP_STALE, P_BATT_PACK_T, 0u, NULL },
    {55500u, STEP_CLEAR, P_BATT_PACK_T, 0u, NULL },
    {59500u, STEP_END,   0u, 0u, "ST-06" },

    {59500u, STEP_BEGIN, 0u, 0u, "ST-07 soft limit for N cycles" },
    {60500u, STEP_VALUE, P_MAIN_BUS_V, V_SOFT_BAND, NULL },
    {65500u, STEP_CLEAR, P_MAIN_BUS_V, 0u, NULL },
    {69500u, STEP_END,   0u, 0u, "ST-07" },

    {69500u, STEP_BEGIN, 0u, 0u, "ST-08 hard limit for N cycles" },
    {70500u, STEP_VALUE, P_BATT_V, V_HARD_BATT_V, NULL },
    {75500u, STEP_CLEAR, P_BATT_V, 0u, NULL },
    {79500u, STEP_END,   0u, 0u, "ST-08" },

    {79500u, STEP_BEGIN, 0u, 0u, "ST-09 clear safe mode" },
    {80500u, STEP_TC,    HK_CMD_CLEAR_SAFE_MODE, 0u, "CLEAR_SAFE_MODE" },
    {84500u, STEP_END,   0u, 0u, "ST-09" },

    {84500u, STEP_SOAK,  0u, 0u, NULL },
};
/* clang-format on */

static void wait_until_ms(uint32_t at_ms)
{
    const uint64_t now_ms = rtems_clock_get_uptime_nanoseconds() / NS_PER_MS;

    if (now_ms < at_ms)
    {
        (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS((uint32_t)(at_ms - now_ms)));
    }
}

static void run_step(const step_t* s)
{
    switch (s->kind)
    {
        case STEP_BEGIN:
            printk("[ST] %s: begin\n", s->text);
            break;
        case STEP_END:
            printk("[ST] %s: end\n", s->text);
            break;
        case STEP_TC:
            printk("[ground] uplink %s\n", s->text);
            hk_startup_check(hk_command_uplink((hk_command_id_t)s->arg), "uplink");
            break;
        case STEP_FRAME:
            printk("[ground] uplink %s\n", s->text);
            hk_startup_check(hk_command_uplink_frame(k_truncated_frame, sizeof k_truncated_frame),
                             "uplink");
            break;
        case STEP_VALUE:
            printk("[fault] param 0x%04lX := %lu\n", (unsigned long)s->arg, (unsigned long)s->value);
            st_fault_value((hk_param_id_t)s->arg, s->value);
            break;
        case STEP_STALE:
            printk("[fault] param 0x%04lX := STALE\n", (unsigned long)s->arg);
            st_fault_stale((hk_param_id_t)s->arg);
            break;
        case STEP_CLEAR:
            printk("[fault] param 0x%04lX := nominal\n", (unsigned long)s->arg);
            st_fault_clear((hk_param_id_t)s->arg);
            break;
        default:
            printk("[ST] scenarios complete; soak with reporting enabled\n");
            break;
    }
}

rtems_task Init(rtems_task_argument arg)
{
    (void)arg;
    printk("\n[init] HK-TM system test image (RTEMS %s)\n", rtems_version());

    hk_service_start();

    for (size_t i = 0u; i < (sizeof k_script / sizeof k_script[0]); i++)
    {
        wait_until_ms(k_script[i].at_ms);
        run_step(&k_script[i]);
    }

    /* Soak: the service keeps running; Init has nothing more to do. */
    (void)rtems_task_suspend(RTEMS_SELF);
}

#include "hk_rtems_config.h"

/*
 * hk_limits.c
 *
 * Limit table and classification (HKS-FD-010). Values are raw 12-bit ADC
 * counts (0..4095), as sampled; calibration to engineering units is a
 * ground function. A reading pinned at 0 or 4095 (open or shorted sensor)
 * is beyond every hard limit. Limits are illustrative for this capstone.
 * test_hk_limits.c checks every row is ordered, and that each soft band
 * is wide enough for the target simulator's nominal values
 * (src/app/rtems/hk_param_source_sim.c).
 */

#include "hk_limits.h"

#include <stddef.h>

/* clang-format off */
const hk_limit_t g_limits[HK_PARAM_COUNT] = {
    /* low_crit low_warn high_warn high_crit */
    /* EPS */
    { 2600u, 2800u, 3300u, 3500u }, /* 0x0101 main bus voltage */
    { 2400u, 2600u, 3400u, 3600u }, /* 0x0102 battery voltage */
    {  400u,  800u, 3300u, 3700u }, /* 0x0103 battery current, bipolar */
    { 1500u, 1650u, 1850u, 2000u }, /* 0x0104 3V3 rail voltage */
    { 1800u, 1900u, 2200u, 2300u }, /* 0x0105 5V rail voltage */
    { 2000u, 2150u, 2450u, 2600u }, /* 0x0106 12V rail voltage */
    {  150u,  300u, 2800u, 3400u }, /* 0x0107 bus load current */
    {  500u,  900u, 3200u, 3600u }, /* 0x0108 charge regulator current */
    /* Thermal */
    {  700u, 1000u, 3000u, 3300u }, /* 0x0201 OBC board */
    {  650u,  950u, 3050u, 3350u }, /* 0x0202 EPS board */
    { 1200u, 1500u, 2600u, 2900u }, /* 0x0203 battery pack */
    {  600u,  900u, 3100u, 3400u }, /* 0x0204 radio power amplifier */
    {  300u,  600u, 3500u, 3800u }, /* 0x0205 panel +X */
    {  320u,  620u, 3480u, 3780u }, /* 0x0206 panel -X */
    {  350u,  650u, 3450u, 3750u }, /* 0x0207 panel +Y */
    {  900u, 1200u, 2800u, 3100u }, /* 0x0208 star tracker optics */
    /* OBC */
    { 1600u, 1700u, 1900u, 2000u }, /* 0x0301 1V8 rail voltage */
    { 1500u, 1600u, 1800u, 1900u }, /* 0x0302 1V2 core voltage */
    {  200u,  400u, 2400u, 3000u }, /* 0x0303 OBC input current */
    {  800u, 1100u, 2900u, 3200u }, /* 0x0304 FPGA temperature */
    { 1900u, 1950u, 2150u, 2200u }, /* 0x0305 oscillator reference */
    { 1550u, 1650u, 1950u, 2050u }, /* 0x0306 memory supply voltage */
    { 1700u, 1800u, 2300u, 2400u }, /* 0x0307 watchdog reference */
    {  800u, 1000u, 3000u, 3300u }, /* 0x0308 processor temperature */
    /* AOCS */
    {  300u,  600u, 3500u, 3800u }, /* 0x0401 reaction wheel 1 current */
    {  300u,  600u, 3500u, 3800u }, /* 0x0402 reaction wheel 2 current */
    {  300u,  600u, 3500u, 3800u }, /* 0x0403 reaction wheel 3 current */
    { 1600u, 1700u, 2100u, 2200u }, /* 0x0404 magnetometer supply */
    { 1650u, 1750u, 2050u, 2150u }, /* 0x0405 gyro supply */
    {  400u,  700u, 3400u, 3700u }, /* 0x0406 magnetorquer X current */
    {  400u,  700u, 3400u, 3700u }, /* 0x0407 magnetorquer Y current */
    { 1700u, 1800u, 2200u, 2300u }, /* 0x0408 star tracker supply */
};
/* clang-format on */

hk_limit_state_t hk_limit_classify(uint32_t value, const hk_limit_t* lim)
{
    hk_limit_state_t s;

    if (lim == NULL)
    {
        s = HK_LIMIT_OK;
    }
    else if (value <= lim->low_crit)
    {
        s = HK_CRIT_LOW;
    }
    else if (value < lim->low_warn)
    {
        s = HK_WARN_LOW;
    }
    else if (value >= lim->high_crit)
    {
        s = HK_CRIT_HIGH;
    }
    else if (value > lim->high_warn)
    {
        s = HK_WARN_HIGH;
    }
    else
    {
        s = HK_LIMIT_OK;
    }
    return s;
}

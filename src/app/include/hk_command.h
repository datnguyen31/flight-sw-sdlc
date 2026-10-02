#ifndef HK_COMMAND_H
#define HK_COMMAND_H

/*
 * Command Interface (HKS-SR-100, HKS-SR-110, HKS-SR-120, HKS-SR-130,
 * HKS-FD-080)
 *
 * Validates and dispatches telecommands that control reporting state and
 * clear safe mode.
 */

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        HK_CMD_ENABLE_REPORTS  = 1,
        HK_CMD_DISABLE_REPORTS = 2,
        HK_CMD_CLEAR_SAFE_MODE = 3 /* ground recovery from safe mode (FD-080) */
    } hk_command_id_t;

    typedef enum
    {
        HK_CMD_ACCEPTED = 0,
        HK_CMD_REJECTED = 1 /* malformed/unknown: no state change (SR-130) */
    } hk_command_result_t;

    hk_command_result_t hk_command_dispatch(hk_command_id_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* HK_COMMAND_H */

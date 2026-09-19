#ifndef STM32TOOLS_AT_CHANNEL_RECOVERY_H
#define STM32TOOLS_AT_CHANNEL_RECOVERY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Shared recovery progression. Module adapters translate bytes to these
 * confirmed events. No hardware, business publishing, sleeps or storage
 * operations belong in this state machine. */
typedef enum {
  AT_RECOVERY_IDLE = 0,
  AT_RECOVERY_WAIT_MODE,
  AT_RECOVERY_RESET_DUE,
  AT_RECOVERY_WAIT_READY,
  AT_RECOVERY_PROBE_DUE,
  AT_RECOVERY_WAIT_PROBE,
  AT_RECOVERY_ECHO_DUE,
  AT_RECOVERY_WAIT_ECHO,
  AT_RECOVERY_COMPLETE
} AtRecoveryState;

typedef enum {
  AT_RECOVERY_COMMAND_NONE = 0,
  AT_RECOVERY_COMMAND_RESET,
  AT_RECOVERY_COMMAND_PROBE,
  AT_RECOVERY_COMMAND_ECHO
} AtRecoveryCommand;

typedef struct {
  AtRecoveryState state;
  uint32_t started_ms;
} AtChannelRecovery;

static inline void AtChannelRecovery_Init(AtChannelRecovery *recovery)
{
  if (recovery == NULL) {
    return;
  }
  recovery->state = AT_RECOVERY_IDLE;
  recovery->started_ms = 0U;
}

static inline void AtChannelRecovery_Quarantine(
    AtChannelRecovery *recovery, uint8_t command_mode_known)
{
  if (recovery == NULL) {
    return;
  }
  recovery->state = (command_mode_known != 0U) ? AT_RECOVERY_RESET_DUE
                                               : AT_RECOVERY_WAIT_MODE;
}

static inline AtRecoveryCommand AtChannelRecovery_Next(
    const AtChannelRecovery *recovery)
{
  if (recovery == NULL) {
    return AT_RECOVERY_COMMAND_NONE;
  }
  switch (recovery->state) {
  case AT_RECOVERY_RESET_DUE:
    return AT_RECOVERY_COMMAND_RESET;
  case AT_RECOVERY_PROBE_DUE:
    return AT_RECOVERY_COMMAND_PROBE;
  case AT_RECOVERY_ECHO_DUE:
    return AT_RECOVERY_COMMAND_ECHO;
  default:
    return AT_RECOVERY_COMMAND_NONE;
  }
}

static inline void AtChannelRecovery_Sent(AtChannelRecovery *recovery,
                                          AtRecoveryCommand command,
                                          uint32_t now_ms)
{
  if (recovery == NULL) {
    return;
  }
  recovery->started_ms = now_ms;
  switch (command) {
  case AT_RECOVERY_COMMAND_RESET:
    recovery->state = AT_RECOVERY_WAIT_READY;
    break;
  case AT_RECOVERY_COMMAND_PROBE:
    recovery->state = AT_RECOVERY_WAIT_PROBE;
    break;
  case AT_RECOVERY_COMMAND_ECHO:
    recovery->state = AT_RECOVERY_WAIT_ECHO;
    break;
  default:
    break;
  }
}

static inline void AtChannelRecovery_ModuleReady(
    AtChannelRecovery *recovery)
{
  if (recovery != NULL) {
    recovery->state = AT_RECOVERY_PROBE_DUE;
  }
}

static inline void AtChannelRecovery_BodyFinished(
    AtChannelRecovery *recovery)
{
  if ((recovery != NULL) && (recovery->state == AT_RECOVERY_WAIT_MODE)) {
    recovery->state = AT_RECOVERY_RESET_DUE;
  }
}

static inline uint8_t AtChannelRecovery_WaitingResult(
    const AtChannelRecovery *recovery)
{
  return ((recovery != NULL) &&
          ((recovery->state == AT_RECOVERY_WAIT_PROBE) ||
           (recovery->state == AT_RECOVERY_WAIT_ECHO)))
             ? 1U
             : 0U;
}

static inline void AtChannelRecovery_Result(AtChannelRecovery *recovery,
                                            uint8_t ok)
{
  if (AtChannelRecovery_WaitingResult(recovery) == 0U) {
    return;
  }
  if (ok == 0U) {
    recovery->state = AT_RECOVERY_RESET_DUE;
  } else {
    recovery->state = (recovery->state == AT_RECOVERY_WAIT_PROBE)
                          ? AT_RECOVERY_ECHO_DUE
                          : AT_RECOVERY_COMPLETE;
  }
}

static inline uint8_t AtChannelRecovery_Timeout(
    AtChannelRecovery *recovery, uint32_t now_ms, uint32_t command_timeout_ms,
    uint32_t reset_timeout_ms)
{
  uint32_t timeout_ms;

  if (recovery == NULL) {
    return 0U;
  }
  timeout_ms = (recovery->state == AT_RECOVERY_WAIT_READY)
                   ? reset_timeout_ms
                   : command_timeout_ms;
  if (((recovery->state == AT_RECOVERY_WAIT_READY) ||
       (AtChannelRecovery_WaitingResult(recovery) != 0U)) &&
      ((uint32_t)(now_ms - recovery->started_ms) >= timeout_ms)) {
    recovery->state = AT_RECOVERY_RESET_DUE;
    return 1U;
  }
  return 0U;
}

static inline uint8_t AtChannelRecovery_TakeComplete(
    AtChannelRecovery *recovery)
{
  if ((recovery == NULL) || (recovery->state != AT_RECOVERY_COMPLETE)) {
    return 0U;
  }
  recovery->state = AT_RECOVERY_IDLE;
  return 1U;
}

#ifdef __cplusplus
}
#endif

#endif /* STM32TOOLS_AT_CHANNEL_RECOVERY_H */

#include <AT/AtChannelRecovery.h>

#include <assert.h>
#include <stdio.h>

static void test_known_command_mode_success(void)
{
  AtChannelRecovery recovery;

  AtChannelRecovery_Init(&recovery);
  assert(recovery.state == AT_RECOVERY_IDLE);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_NONE);

  AtChannelRecovery_Quarantine(&recovery, 1U);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_RESET);
  AtChannelRecovery_Sent(&recovery, AT_RECOVERY_COMMAND_RESET, 100U);
  assert(recovery.state == AT_RECOVERY_WAIT_READY);
  assert(AtChannelRecovery_Timeout(&recovery, 200U, 50U, 1000U) == 0U);

  AtChannelRecovery_ModuleReady(&recovery);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_PROBE);
  AtChannelRecovery_Sent(&recovery, AT_RECOVERY_COMMAND_PROBE, 300U);
  assert(AtChannelRecovery_WaitingResult(&recovery) != 0U);
  AtChannelRecovery_Result(&recovery, 1U);

  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_ECHO);
  AtChannelRecovery_Sent(&recovery, AT_RECOVERY_COMMAND_ECHO, 400U);
  AtChannelRecovery_Result(&recovery, 1U);
  assert(AtChannelRecovery_TakeComplete(&recovery) != 0U);
  assert(recovery.state == AT_RECOVERY_IDLE);
  assert(AtChannelRecovery_TakeComplete(&recovery) == 0U);
}

static void test_unknown_input_mode_waits_for_body_end(void)
{
  AtChannelRecovery recovery;

  AtChannelRecovery_Init(&recovery);
  AtChannelRecovery_Quarantine(&recovery, 0U);
  assert(recovery.state == AT_RECOVERY_WAIT_MODE);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_NONE);

  AtChannelRecovery_BodyFinished(&recovery);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_RESET);
}

static void test_failure_and_timeout_restart_reset(void)
{
  AtChannelRecovery recovery;

  AtChannelRecovery_Init(&recovery);
  AtChannelRecovery_Quarantine(&recovery, 1U);
  AtChannelRecovery_Sent(&recovery, AT_RECOVERY_COMMAND_RESET, 0xFFFFFFF0UL);
  assert(AtChannelRecovery_Timeout(&recovery, 0x00000020UL, 10U, 40U) != 0U);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_RESET);

  AtChannelRecovery_ModuleReady(&recovery);
  AtChannelRecovery_Sent(&recovery, AT_RECOVERY_COMMAND_PROBE, 1000U);
  AtChannelRecovery_Result(&recovery, 0U);
  assert(AtChannelRecovery_Next(&recovery) == AT_RECOVERY_COMMAND_RESET);
}

static void test_null_arguments_are_safe(void)
{
  assert(AtChannelRecovery_Next(NULL) == AT_RECOVERY_COMMAND_NONE);
  assert(AtChannelRecovery_WaitingResult(NULL) == 0U);
  assert(AtChannelRecovery_Timeout(NULL, 0U, 0U, 0U) == 0U);
  assert(AtChannelRecovery_TakeComplete(NULL) == 0U);
  AtChannelRecovery_Init(NULL);
  AtChannelRecovery_Quarantine(NULL, 1U);
  AtChannelRecovery_Sent(NULL, AT_RECOVERY_COMMAND_RESET, 0U);
  AtChannelRecovery_ModuleReady(NULL);
  AtChannelRecovery_BodyFinished(NULL);
  AtChannelRecovery_Result(NULL, 1U);
}

int main(void)
{
  test_known_command_mode_success();
  test_unknown_input_mode_waits_for_body_end();
  test_failure_and_timeout_restart_reset();
  test_null_arguments_are_safe();
  puts("AT channel recovery tests passed");
  return 0;
}

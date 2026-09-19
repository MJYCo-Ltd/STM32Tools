#include <Auxiliary.h>
#include "main.h"
#include <assert.h>
#include <stdio.h>
static RTC_HandleTypeDef rtc;
static UART_HandleTypeDef uart;
static unsigned stops, standby, armed, deactivated, clocks, writes;
static HAL_StatusTypeDef arm_result, deactivate_result;
static AuxiliaryStatus clock_result;
size_t xPortGetFreeHeapSize(void) { return 40960U; }
uint32_t HAL_RCC_GetHCLKFreq(void) { return 480000000U; }
void HAL_PWR_EnterSLEEPMode(uint32_t r,uint32_t e) { (void)r;(void)e; }
void HAL_PWR_EnterSTOPMode(uint32_t r,uint32_t e) { (void)r;(void)e;++stops; }
void HAL_PWR_EnterSTANDBYMode(void) { ++standby; }
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *u,const uint8_t *d,uint16_t n,uint32_t t)
{ assert(u==&uart && d && n && t==30U);++writes;return HAL_OK; }
HAL_StatusTypeDef HAL_RTCEx_DeactivateWakeUpTimer(RTC_HandleTypeDef *r)
{ assert(r==&rtc);++deactivated;return deactivate_result; }
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *r,uint32_t n,uint32_t c)
{ assert(r==&rtc);(void)n;(void)c;++armed;return arm_result; }
static AuxiliaryStatus Restore(void *context) { assert(context==&rtc);++clocks;return clock_result; }
int main(void)
{
  AuxiliaryConfig config={&rtc,&uart,Restore,&rtc};
  STMSTATUS s=GetStatus();
  assert(s.unRamFree==40960U && s.unRamTotal==65536U && s.unCPUFrequency==480U);
  Auxiliary_Configure(NULL);
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP,1U,0U)==AUXILIARY_ERR_UNCONFIGURED && stops==0U);
  SendDebugInfo((const uint8_t *)"a",1U); assert(writes==0U);
  Auxiliary_Configure(&config);
  assert(Auxiliary_EnterLowPower((LOW_POWER_MODE)4,1U,0U)==AUXILIARY_ERR_PARAM && armed==0U);
  arm_result=HAL_ERROR;
  EnterLowPowerMode(LP_MODE_STOP,1U,0U);
  assert(Auxiliary_LastError()==AUXILIARY_ERR_IO && stops==0U);
  arm_result=HAL_OK; deactivate_result=HAL_ERROR;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP,1U,0U)==AUXILIARY_ERR_IO && stops==0U);
  deactivate_result=HAL_OK;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP,1U,0U)==AUXILIARY_OK);
  assert(stops==1U && clocks==1U && deactivated==4U);
  clock_result=AUXILIARY_ERR_IO;
  assert(Auxiliary_EnterLowPower(LP_MODE_STOP,1U,0U)==AUXILIARY_ERR_IO);
  SendDebugInfo((const uint8_t *)"a",1U); assert(writes==1U);
  assert(Auxiliary_EnterLowPower(LP_MODE_STANDBY,1U,0U)==AUXILIARY_ERR_IO && standby==1U);
  uint8_t *memory=RequestSpace(64U); assert(memory);for(unsigned i=0;i<64U;++i)assert(!memory[i]);RecycleSpace(memory);
  puts("Auxiliary explicit dependencies, heap width, RTC error contracts passed");return 0;
}

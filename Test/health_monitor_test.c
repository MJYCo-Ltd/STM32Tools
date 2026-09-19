#include <Services/HealthMonitor.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
  HealthMonitor h;
  uint32_t times[HEALTH_MONITOR_CAPACITY]={100U,200U};
  assert(HealthMonitor_Init(&h,3U,times,0U));
  assert(!HealthMonitor_Stable(&h,0U,30U));
  HealthMonitor_Report(&h,0U,1U);HealthMonitor_Report(&h,1U,1U);
  assert(!HealthMonitor_Stable(&h,1U,30U));
  HealthMonitor_Report(&h,0U,2U);HealthMonitor_Report(&h,1U,2U);
  assert(!HealthMonitor_Stable(&h,2U,30U));
  assert(HealthMonitor_Stable(&h,32U,30U));
  assert(HealthMonitor_Faults(&h,102U)==1U);
  HealthMonitor_Report(&h,0U,103U);
  assert(HealthMonitor_Faults(&h,103U)==1U && !HealthMonitor_Stable(&h,103U,30U));
  assert(HealthMonitor_Init(&h,3U,times,UINT32_MAX-50U));
  HealthMonitor_Report(&h,0U,20U);HealthMonitor_Report(&h,1U,20U);
  assert(!HealthMonitor_Faults(&h,21U));
  HealthMonitor_Report(&h,0U,121U); /* Late report itself must latch. */
  assert(HealthMonitor_Faults(&h,121U)&1U);
  assert(HealthMonitor_Init(&h,3U,times,0U));
  HealthMonitor_Report(&h,0U,10U);HealthMonitor_Report(&h,0U,90U);
  assert(HealthMonitor_Faults(&h,200U)==3U);
  assert(!HealthMonitor_Init(NULL,3U,times,0U));
  puts("Health deadlines, repeated progress, trial window, fault latching and wrap passed");return 0;
}

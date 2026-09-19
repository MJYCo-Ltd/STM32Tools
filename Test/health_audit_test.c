#include "System/HealthMonitor.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
static uint8_t Init(HealthMonitor *h, uint32_t now)
{
  if (!h) return 0U;
  HealthMonitor_Init(h);
  return HealthMonitor_Register(h,0U,100U,now) &&
         HealthMonitor_Register(h,1U,200U,now) && HealthMonitor_Arm(h,3U,now);
}
int main(void)
{
  HealthMonitor h;
  assert(Init(&h,0U));
  assert(!HealthMonitor_StableFor(&h,0U,30U));
  assert(HealthMonitor_Progress(&h,0U,1U));
  assert(HealthMonitor_Progress(&h,1U,1U));
  assert(!HealthMonitor_StableFor(&h,1U,30U));
  assert(HealthMonitor_Progress(&h,0U,2U));
  assert(HealthMonitor_Progress(&h,1U,2U));
  assert(!HealthMonitor_StableFor(&h,2U,30U));
  assert(HealthMonitor_StableFor(&h,32U,30U));
  assert(!HealthMonitor_Check(&h,102U) && h.fault_mask==1U);
  assert(!HealthMonitor_Progress(&h,0U,103U));
  assert(!HealthMonitor_StableFor(&h,103U,30U) && h.fault_mask==1U);
  assert(Init(&h,UINT32_MAX-50U));
  assert(HealthMonitor_Progress(&h,0U,20U));
  assert(HealthMonitor_Progress(&h,1U,20U));
  assert(HealthMonitor_Check(&h,21U));
  assert(!HealthMonitor_Progress(&h,0U,121U) && (h.fault_mask&1U));
  assert(Init(&h,0U));
  assert(HealthMonitor_Progress(&h,0U,10U));
  assert(HealthMonitor_Progress(&h,0U,90U));
  assert(!HealthMonitor_Check(&h,200U) && h.fault_mask==3U);
  assert(!Init(NULL,0U));
  puts("Audit health deadlines, sticky faults and tick wrap: passed");
  return 0;
}

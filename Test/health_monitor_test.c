#include <System/HealthMonitor.h>
#include <assert.h>
#include <stdio.h>
static void Init(HealthMonitor *m, uint32_t now) {
  HealthMonitor_Init(m);
  assert(HealthMonitor_Register(m, 0, 100U, now));
  assert(HealthMonitor_Register(m, 1, 200U, now));
  assert(!HealthMonitor_Arm(m, 1U, now));
  assert(HealthMonitor_Arm(m, 3U, now));
}
int main(void) {
  HealthMonitor m;
  Init(&m, 0);
  assert(HealthMonitor_Check(&m, 0));
  assert(!HealthMonitor_StableFor(&m, 0, 0));
  for (uint32_t t = 10; t <= 80; t += 10) {
    assert(HealthMonitor_Progress(&m, 0, t));
    assert(HealthMonitor_Progress(&m, 1, t));
  }
  assert(HealthMonitor_StableFor(&m, 80, 50));
  assert(!HealthMonitor_Check(&m, 180));
  assert(m.fault_mask == 1U);
  assert(!HealthMonitor_Progress(&m, 0, 181));
  assert(!HealthMonitor_Check(&m, 181));
  Init(&m, UINT32_MAX - 50U);
  assert(HealthMonitor_Progress(&m, 0, 0U));
  assert(HealthMonitor_Progress(&m, 1, 0U));
  assert(HealthMonitor_Check(&m, 20U));
  assert(!HealthMonitor_Check(&m, 100U));
  Init(&m, 0U);
  assert(HealthMonitor_BeginLease(&m, 0U, 400U, 10U));
  assert(!HealthMonitor_BeginLease(&m, 0U, 400U, 20U));
  assert(!HealthMonitor_Progress(&m, 0U, 20U));
  assert(HealthMonitor_Progress(&m, 1U, 100U));
  assert(HealthMonitor_Check(&m, 250U));
  assert(HealthMonitor_Progress(&m, 1U, 250U));
  assert(HealthMonitor_EndLease(&m, 0U, 300U));
  assert(HealthMonitor_Check(&m, 399U));
  assert(!HealthMonitor_Check(&m, 400U));
  Init(&m, 0U);
  assert(HealthMonitor_BeginLease(&m, 0U, 400U, 0U));
  assert(!HealthMonitor_Check(&m, 201U));
  assert(m.fault_mask == 2U); /* other task cannot hide behind lease */
  Init(&m, 0U);
  assert(HealthMonitor_BeginLease(&m, 0U, 400U, 0U));
  for (unsigned t = 50; t < 400; t += 50)
    assert(HealthMonitor_Progress(&m, 1U, t));
  assert(!HealthMonitor_EndLease(&m, 0U, 400U));
  assert(m.fault_mask == 1U);
  assert(!HealthMonitor_BeginLease(&m, 0, UINT32_MAX, 0));
  puts("HealthMonitor liveness/stability/leases/latching/wrap: passed");
  return 0;
}

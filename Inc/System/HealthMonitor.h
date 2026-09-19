#ifndef STM32TOOLS_HEALTH_MONITOR_H
#define STM32TOOLS_HEALTH_MONITOR_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define HEALTH_MONITOR_TASKS 8U
#define HEALTH_MONITOR_MAX_LEASE_MS 60000U

typedef struct {
  uint32_t last_progress_ms, deadline_ms, lease_start_ms, lease_ms;
  uint8_t observations, leased;
} HealthTask;
typedef struct {
  HealthTask tasks[HEALTH_MONITOR_TASKS];
  uint32_t healthy_since_ms;
  uint8_t required_mask, fault_mask, armed, stable;
} HealthMonitor;

/** Allocation-free policy core. Caller serializes access; time is monotonic
 * modulo uint32, intervals and observation gaps must be below 2^31 ms.
 * Offline networks and valid waiting loops are progress, not task failures.
 * A missed deadline latches until Init; a late heartbeat cannot hide a stall.
 */
void HealthMonitor_Init(HealthMonitor *monitor);
uint8_t HealthMonitor_Register(HealthMonitor *monitor, uint8_t id, uint32_t deadline_ms,
                               uint32_t now_ms);
uint8_t HealthMonitor_Arm(HealthMonitor *monitor, uint8_t expected_mask, uint32_t now_ms);
uint8_t HealthMonitor_Check(HealthMonitor *monitor, uint32_t now_ms);
uint8_t HealthMonitor_Progress(HealthMonitor *monitor, uint8_t id, uint32_t now_ms);
uint8_t HealthMonitor_BeginLease(HealthMonitor *monitor, uint8_t id, uint32_t duration_ms,
                                 uint32_t now_ms);
uint8_t HealthMonitor_EndLease(HealthMonitor *monitor, uint8_t id, uint32_t now_ms);
uint8_t HealthMonitor_StableFor(HealthMonitor *monitor, uint32_t now_ms, uint32_t duration_ms);
#ifdef __cplusplus
}
#endif
#endif

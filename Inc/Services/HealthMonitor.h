#ifndef STM32TOOLS_HEALTH_MONITOR_H
#define STM32TOOLS_HEALTH_MONITOR_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define HEALTH_MONITOR_CAPACITY 8U

typedef struct {
  uint32_t last[HEALTH_MONITOR_CAPACITY];
  uint32_t timeout[HEALTH_MONITOR_CAPACITY];
  uint8_t reports[HEALTH_MONITOR_CAPACITY];
  uint32_t required, fault, started, stable_since;
  uint8_t stable;
} HealthMonitor;

/* Pure state only; owner supplies monotonic milliseconds and serialization.
 * Report only after bounded work OR a deliberate quiescent/retry checkpoint.
 * This does not require network connectivity or successful sensor readings. */
static inline uint8_t HealthMonitor_Init(HealthMonitor *h, uint32_t required,
                                         const uint32_t *timeouts, uint32_t now)
{
  unsigned i;
  if (!h || !timeouts || !required || required >= (1UL << HEALTH_MONITOR_CAPACITY)) return 0U;
  memset(h, 0, sizeof(*h));
  h->required = required; h->started = now;
  for (i = 0U; i < HEALTH_MONITOR_CAPACITY; ++i) {
    if ((required & (1UL << i)) && (!timeouts[i] || timeouts[i] > INT32_MAX)) return 0U;
    h->timeout[i] = timeouts[i]; h->last[i] = now;
  }
  return 1U;
}
static inline void HealthMonitor_Report(HealthMonitor *h, unsigned id, uint32_t now)
{
  if (!h || id >= HEALTH_MONITOR_CAPACITY || !(h->required & (1UL << id))) return;
  /* A late heartbeat cannot retroactively hide a missed deadline. */
  if (now - h->last[id] >= h->timeout[id]) h->fault |= 1UL << id;
  h->last[id] = now;
  if (h->reports[id] < 2U) ++h->reports[id];
}
static inline uint32_t HealthMonitor_Faults(HealthMonitor *h, uint32_t now)
{
  unsigned i;
  if (!h || !h->required) return UINT32_MAX;
  for (i = 0U; i < HEALTH_MONITOR_CAPACITY; ++i)
    if ((h->required & (1UL << i)) && now - h->last[i] >= h->timeout[i]) h->fault |= 1UL << i;
  return h->fault;
}
static inline uint8_t HealthMonitor_Stable(HealthMonitor *h, uint32_t now, uint32_t window)
{
  unsigned i;
  if (HealthMonitor_Faults(h, now)) { if (h) h->stable = 0U; return 0U; }
  for (i = 0U; i < HEALTH_MONITOR_CAPACITY; ++i)
    if ((h->required & (1UL << i)) && h->reports[i] < 2U) { h->stable = 0U; return 0U; }
  if (!h->stable) { h->stable = 1U; h->stable_since = now; }
  return now - h->stable_since >= window;
}
#endif

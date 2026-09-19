#include "System/HealthMonitor.h"
#include <stddef.h>
#include <string.h>

static uint8_t Registered(const HealthMonitor *m, uint8_t id) {
  return m != NULL && id < HEALTH_MONITOR_TASKS && (m->required_mask & (1U << id));
}
void HealthMonitor_Init(HealthMonitor *m) {
  if (m != NULL)
    memset(m, 0, sizeof(*m));
}
uint8_t HealthMonitor_Register(HealthMonitor *m, uint8_t id, uint32_t deadline, uint32_t now) {
  if (m == NULL || m->armed || id >= HEALTH_MONITOR_TASKS || deadline == 0U ||
      deadline >= 0x80000000UL || Registered(m, id))
    return 0U;
  m->tasks[id] = (HealthTask){.last_progress_ms = now, .deadline_ms = deadline};
  m->required_mask |= (uint8_t)(1U << id);
  return 1U;
}
uint8_t HealthMonitor_Arm(HealthMonitor *m, uint8_t expected, uint32_t now) {
  if (m == NULL || m->armed || expected == 0U || m->required_mask != expected || m->fault_mask)
    return 0U;
  m->armed = 1U;
  m->stable = 0U;
  /* Every task gets one bounded first-observation interval after handoff. */
  for (unsigned i = 0; i < HEALTH_MONITOR_TASKS; ++i)
    if (Registered(m, (uint8_t)i)) {
      m->tasks[i].last_progress_ms = now;
      m->tasks[i].observations = 0U;
    }
  return 1U;
}
uint8_t HealthMonitor_Check(HealthMonitor *m, uint32_t now) {
  uint8_t observed = 1U;
  if (m == NULL || !m->armed)
    return 0U;
  for (unsigned i = 0; i < HEALTH_MONITOR_TASKS; ++i)
    if (Registered(m, (uint8_t)i)) {
      HealthTask *t = &m->tasks[i];
      if ((t->leased && now - t->lease_start_ms >= t->lease_ms) ||
          (!t->leased && now - t->last_progress_ms >= t->deadline_ms))
        m->fault_mask |= (uint8_t)(1U << i);
      if (t->observations < 2U)
        observed = 0U;
    }
  if (m->fault_mask) {
    m->stable = 0U;
    return 0U;
  }
  if (observed && !m->stable) {
    m->stable = 1U;
    m->healthy_since_ms = now;
  }
  return 1U;
}
uint8_t HealthMonitor_Progress(HealthMonitor *m, uint8_t id, uint32_t now) {
  if (!Registered(m, id) || !HealthMonitor_Check(m, now))
    return 0U;
  HealthTask *t = &m->tasks[id];
  if (t->leased)
    return 0U; /* neither heartbeats nor polling can extend a lease */
  t->last_progress_ms = now;
  if (t->observations < 2U)
    ++t->observations;
  return 1U;
}
uint8_t HealthMonitor_BeginLease(HealthMonitor *m, uint8_t id, uint32_t duration,
                                 uint32_t now) {
  if (!Registered(m, id) || duration == 0U || duration > HEALTH_MONITOR_MAX_LEASE_MS ||
      !HealthMonitor_Check(m, now) || m->tasks[id].leased)
    return 0U;
  m->tasks[id].lease_start_ms = now;
  m->tasks[id].lease_ms = duration;
  m->tasks[id].leased = 1U;
  return 1U;
}
uint8_t HealthMonitor_EndLease(HealthMonitor *m, uint8_t id, uint32_t now) {
  if (!Registered(m, id) || !m->tasks[id].leased || !HealthMonitor_Check(m, now))
    return 0U;
  HealthTask *task = &m->tasks[id];
  task->last_progress_ms = now;
  task->leased = 0U;
  if (task->observations < 2U)
    ++task->observations;
  return 1U;
}
uint8_t HealthMonitor_StableFor(HealthMonitor *m, uint32_t now, uint32_t duration) {
  if (!HealthMonitor_Check(m, now) || !m->stable || duration >= 0x80000000UL ||
      now - m->healthy_since_ms < duration)
    return 0U;
  for (unsigned i = 0; i < HEALTH_MONITOR_TASKS; ++i)
    if (Registered(m, (uint8_t)i) && m->tasks[i].leased)
      return 0U;
  return 1U;
}

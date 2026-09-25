#include <Net/CommAdapter.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t Online(void) { return 1U; }
static uint8_t Ready(void) { return 1U; }
static CommAdapterState OnlineState(void) { return COMM_ADAPTER_STATE_ONLINE; }
static CommAdapterReason NoReason(void) { return COMM_ADAPTER_REASON_NONE; }
static uint8_t Publish(const char *t, const char *p, uint8_t q, uint8_t r)
{ (void)t; (void)p; (void)q; (void)r; return 1U; }

int main(void)
{
  static const CommAdapterOps items[] = {
      {
          .id = 1U,
          .name = "a",
          .mqtt_command_channel = 1U,
          .download_priority = 20U,
          .is_online = Online,
          .is_selectable = Ready,
          .get_state = OnlineState,
          .get_reason = NoReason,
          .mqtt_publish = Publish,
      },
      {
          .id = 2U,
          .name = "b",
          .mqtt_command_channel = 2U,
          .download_priority = 10U,
          .is_online = Online,
          .is_selectable = Ready,
          .mqtt_publish = Publish,
      },
  };
  const CommAdapterRegistry registry = {items, 2U};
  const CommAdapterOps *a;
  const CommAdapterOps *b;

  assert(CommAdapterRegistry_GetCount(&registry) == 2U);
  a = CommAdapterRegistry_FindById(&registry, 1U);
  b = CommAdapterRegistry_GetAt(&registry, 1U);
  assert(a != NULL && b != NULL);
  assert(strcmp(a->name, "a") == 0);
  assert(strcmp(b->name, "b") == 0);
  assert(b->download_priority < a->download_priority);
  assert(a->get_state() == COMM_ADAPTER_STATE_ONLINE);
  assert(a->get_reason() == COMM_ADAPTER_REASON_NONE);
  assert(CommAdapterRegistry_FindById(&registry, 0U) == NULL);
  assert(CommAdapterRegistry_GetAt(&registry, 2U) == NULL);
  assert(CommAdapterRegistry_GetCount(NULL) == 0U);
  puts("comm adapter portable registry contract: PASS");
  return 0;
}

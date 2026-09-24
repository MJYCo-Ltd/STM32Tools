#include <Net/CommAdapter.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t Online(void) { return 1U; }
static uint8_t Ready(void) { return 1U; }
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

  assert(CommAdapterRegistry_Count(&registry) == 2U);
  a = CommAdapterRegistry_Find(&registry, 1U);
  b = CommAdapterRegistry_At(&registry, 1U);
  assert(a != NULL && b != NULL);
  assert(strcmp(a->name, "a") == 0);
  assert(strcmp(b->name, "b") == 0);
  assert(b->download_priority < a->download_priority);
  assert(CommAdapterRegistry_Find(&registry, 0U) == NULL);
  assert(CommAdapterRegistry_At(&registry, 2U) == NULL);
  assert(CommAdapterRegistry_Count(NULL) == 0U);
  puts("comm adapter portable registry contract: PASS");
  return 0;
}

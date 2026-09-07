#ifndef STM32TOOLS_AT_SESSION_H
#define STM32TOOLS_AT_SESSION_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  char *buffer;
  uint16_t capacity;
  uint16_t length;
  uint32_t started_at;
  uint8_t pending;
  uint8_t ready;
  uint8_t overflow;
} AtSession;

void AtSession_Init(AtSession *session, char *buffer, size_t capacity);
void AtSession_Reset(AtSession *session);
void AtSession_Start(AtSession *session, uint32_t now);
void AtSession_Cancel(AtSession *session);
void AtSession_Complete(AtSession *session);
uint16_t AtSession_Append(AtSession *session, const uint8_t *data,
                          uint16_t length);
uint8_t AtSession_IsBusy(const AtSession *session);
uint8_t AtSession_IsPending(const AtSession *session);
uint8_t AtSession_TakeReady(AtSession *session);
uint8_t AtSession_HasTimedOut(const AtSession *session, uint32_t now,
                              uint32_t timeout_ms);
uint8_t AtSession_Overflowed(const AtSession *session);
uint16_t AtSession_Length(const AtSession *session);
const char *AtSession_Data(const AtSession *session);

#endif /* STM32TOOLS_AT_SESSION_H */

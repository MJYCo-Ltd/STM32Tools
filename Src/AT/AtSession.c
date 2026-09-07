#include <AT/AtSession.h>

#include <limits.h>
#include <string.h>

void AtSession_Init(AtSession *session, char *buffer, size_t capacity)
{
  if (session == NULL) {
    return;
  }
  memset(session, 0, sizeof(*session));
  if ((buffer == NULL) || (capacity < 2U) || (capacity > UINT16_MAX)) {
    return;
  }
  session->buffer = buffer;
  session->capacity = (uint16_t)capacity;
  session->buffer[0] = '\0';
}

void AtSession_Reset(AtSession *session)
{
  if ((session == NULL) || (session->buffer == NULL)) {
    return;
  }
  session->length = 0U;
  session->pending = 0U;
  session->ready = 0U;
  session->overflow = 0U;
  session->buffer[0] = '\0';
}

void AtSession_Start(AtSession *session, uint32_t now)
{
  if ((session == NULL) || (session->buffer == NULL)) {
    return;
  }
  session->started_at = now;
  session->pending = 1U;
}

void AtSession_Cancel(AtSession *session)
{
  if (session != NULL) {
    session->pending = 0U;
    session->ready = 0U;
  }
}

void AtSession_Complete(AtSession *session)
{
  if (session != NULL) {
    session->pending = 0U;
    session->ready = 1U;
  }
}

uint16_t AtSession_Append(AtSession *session, const uint8_t *data,
                          uint16_t length)
{
  size_t available;
  size_t copy_length;

  if ((session == NULL) || (session->buffer == NULL) || (data == NULL) ||
      (length == 0U)) {
    return 0U;
  }
  available = (size_t)session->capacity - session->length - 1U;
  copy_length = length;
  if (copy_length > available) {
    copy_length = available;
    session->overflow = 1U;
  }
  if (copy_length > 0U) {
    memcpy(&session->buffer[session->length], data, copy_length);
    session->length = (uint16_t)(session->length + copy_length);
    session->buffer[session->length] = '\0';
  }
  return (uint16_t)copy_length;
}

uint8_t AtSession_IsBusy(const AtSession *session)
{
  return ((session != NULL) &&
          ((session->pending != 0U) || (session->ready != 0U)))
             ? 1U
             : 0U;
}

uint8_t AtSession_IsPending(const AtSession *session)
{
  return ((session != NULL) && (session->pending != 0U)) ? 1U : 0U;
}

uint8_t AtSession_TakeReady(AtSession *session)
{
  if ((session == NULL) || (session->ready == 0U)) {
    return 0U;
  }
  session->ready = 0U;
  return 1U;
}

uint8_t AtSession_HasTimedOut(const AtSession *session, uint32_t now,
                              uint32_t timeout_ms)
{
  return ((session != NULL) && (session->pending != 0U) &&
          ((now - session->started_at) >= timeout_ms))
             ? 1U
             : 0U;
}

uint8_t AtSession_Overflowed(const AtSession *session)
{
  return ((session != NULL) && (session->overflow != 0U)) ? 1U : 0U;
}

uint16_t AtSession_Length(const AtSession *session)
{
  return (session != NULL) ? session->length : 0U;
}

const char *AtSession_Data(const AtSession *session)
{
  return ((session != NULL) && (session->buffer != NULL)) ? session->buffer
                                                           : "";
}

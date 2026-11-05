#include <Net/ConnectTransfer.h>
#include <Common.h>
#include <string.h>

void ConnectTransfer_Reset(ConnectTransfer *s) { memset(s, 0, sizeof(*s)); }
void ConnectTransfer_Cancel(ConnectTransfer *s) { s->state = CONNECT_TRANSFER_CANCELED; }
uint32_t ConnectTransfer_Crc(const ConnectTransfer *s) { return s->running_crc ^ UINT32_MAX; }

ConnectTransferResult ConnectTransfer_Begin(ConnectTransfer *s,
    uint32_t total, uint32_t crc, uint32_t now)
{
  if (!total) return CONNECT_TRANSFER_INVALID;
  if (s->state == CONNECT_TRANSFER_RECEIVING || s->state == CONNECT_TRANSFER_VERIFIED)
    return s->total == total && s->expected_crc == crc ?
        CONNECT_TRANSFER_DUPLICATE : CONNECT_TRANSFER_CONFLICT;
  *s = (ConnectTransfer){.total=total, .expected_crc=crc,
      .running_crc=UINT32_MAX, .progress_tick=now, .state=CONNECT_TRANSFER_RECEIVING};
  return CONNECT_TRANSFER_OK;
}

ConnectTransferResult ConnectTransfer_Accept(ConnectTransfer *s,
    const ConnectTransferSink *sink, uint32_t offset, const uint8_t *bytes,
    uint32_t length, uint32_t now)
{
  if ((s->state != CONNECT_TRANSFER_RECEIVING && s->state != CONNECT_TRANSFER_VERIFIED) ||
      !sink || !sink->write || !bytes || !length || offset > s->total || length > s->total-offset)
    return CONNECT_TRANSFER_INVALID;
  if (offset < s->received) {
    if (length > s->received-offset) return CONNECT_TRANSFER_OFFSET;
    if (!sink->matches || !sink->matches(sink->context,offset,bytes,length))
      return CONNECT_TRANSFER_CONFLICT;
    return CONNECT_TRANSFER_DUPLICATE; /* Replay cannot extend the receive deadline. */
  }
  if (offset != s->received) return CONNECT_TRANSFER_OFFSET;
  if (s->state != CONNECT_TRANSFER_RECEIVING) return CONNECT_TRANSFER_INVALID;
  const ConnectTransferResult result = sink->write(sink->context,offset,bytes,length);
  if (result != CONNECT_TRANSFER_OK) return result;
  s->running_crc = CalCRC32Update(s->running_crc,bytes,length);
  s->received += length;
  s->progress_tick = now;
  return CONNECT_TRANSFER_OK;
}

typedef struct { uint8_t *bytes; } BufferSink;
static ConnectTransferResult BufferWrite(void *context,uint32_t offset,
    const uint8_t *bytes,uint32_t length)
{
  BufferSink *buffer=context;
  memmove(buffer->bytes+offset,bytes,length);
  return CONNECT_TRANSFER_OK;
}
static uint8_t BufferMatches(void *context,uint32_t offset,
    const uint8_t *bytes,uint32_t length)
{
  BufferSink *buffer=context;
  return memcmp(buffer->bytes+offset,bytes,length)==0;
}
ConnectTransferResult ConnectTransfer_AcceptBuffer(ConnectTransfer *s,
    uint8_t *buffer,uint32_t capacity,uint32_t offset,const uint8_t *bytes,
    uint32_t length,uint32_t now)
{
  if (!buffer || s->total>capacity) return CONNECT_TRANSFER_INVALID;
  BufferSink context={buffer};
  const ConnectTransferSink sink={&context,BufferWrite,BufferMatches};
  return ConnectTransfer_Accept(s,&sink,offset,bytes,length,now);
}

ConnectTransferResult ConnectTransfer_Complete(ConnectTransfer *s)
{
  if (s->state != CONNECT_TRANSFER_RECEIVING && s->state != CONNECT_TRANSFER_VERIFIED)
    return CONNECT_TRANSFER_INVALID;
  if (s->received != s->total) return CONNECT_TRANSFER_INCOMPLETE;
  if (ConnectTransfer_Crc(s) != s->expected_crc) return CONNECT_TRANSFER_CRC;
  s->state=CONNECT_TRANSFER_VERIFIED;
  return CONNECT_TRANSFER_OK;
}
ConnectTransferResult ConnectTransfer_Event(ConnectTransfer *s,
    const ConnectTransferSink *sink,const ConnectDownloadEvent *event,uint32_t now)
{
  if (!event || event->content_length != s->total) return CONNECT_TRANSFER_INVALID;
  if (event->type == CONNECT_DOWNLOAD_EVENT_DATA) {
    if (event->data_length>sizeof(event->data)) return CONNECT_TRANSFER_INVALID;
    return ConnectTransfer_Accept(s,sink,event->offset,event->data,event->data_length,now);
  }
  if (event->type == CONNECT_DOWNLOAD_EVENT_COMPLETE) {
    if (event->data_length || event->offset != s->total) return CONNECT_TRANSFER_INVALID;
    return ConnectTransfer_Complete(s);
  }
  return CONNECT_TRANSFER_INVALID;
}
ConnectTransferResult ConnectTransfer_ConfirmDurable(ConnectTransfer *s,uint32_t position)
{
  if ((s->state != CONNECT_TRANSFER_RECEIVING && s->state != CONNECT_TRANSFER_VERIFIED) ||
      position < s->durable || position > s->received) return CONNECT_TRANSFER_INVALID;
  s->durable=position;
  return CONNECT_TRANSFER_OK;
}
uint8_t ConnectTransfer_TimedOut(const ConnectTransfer *s,uint32_t now,uint32_t timeout)
{
  return s->state==CONNECT_TRANSFER_RECEIVING && timeout &&
      (uint32_t)(now-s->progress_tick)>=timeout;
}

uint8_t ConnectTransfer_Decode(const uint8_t *bytes,size_t length,ConnectTransferEnvelope *out)
{
  if (!bytes || !out || length<CONNECT_TRANSFER_HEADER_SIZE || memcmp(bytes,"BTF1",4U) ||
      bytes[7] || ReadLE16(bytes+22U) || !bytes[6] || bytes[6]>CONNECT_TRANSFER_IDENTITY_MAX)
    return 0U;
  const uint16_t count=ReadLE16(bytes+20U);
  const uint32_t total=ReadLE32(bytes+8U),offset=ReadLE32(bytes+12U);
  if (!total || offset>total || count>CONNECT_TRANSFER_BLOCK_MAX ||
      length!=CONNECT_TRANSFER_HEADER_SIZE+bytes[6]+(size_t)count || count>total-offset) return 0U;
  switch (bytes[5]) {
    case CONNECT_TRANSFER_BEGIN: if (offset || count) return 0U; break;
    case CONNECT_TRANSFER_DATA:
      if (!count || offset%CONNECT_TRANSFER_BLOCK_MAX ||
          count != (total-offset<CONNECT_TRANSFER_BLOCK_MAX ? total-offset : CONNECT_TRANSFER_BLOCK_MAX)) return 0U;
      break;
    case CONNECT_TRANSFER_COMMIT: if (offset!=total || count) return 0U; break;
    default: return 0U;
  }
  *out=(ConnectTransferEnvelope){.kind=bytes[4],.operation=bytes[5],.total=total,
      .offset=offset,.crc=ReadLE32(bytes+16U),.identity_length=bytes[6],
      .identity=bytes+CONNECT_TRANSFER_HEADER_SIZE,.length=count,
      .data=bytes+CONNECT_TRANSFER_HEADER_SIZE+bytes[6]};
  return 1U;
}

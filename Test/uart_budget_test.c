#include "UartReceive.h"
#include "cmsis_os.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct { UartQueueInfo data[UART_RECEIVE_QUEUE_DEPTH]; unsigned head,tail,count; } Queue;
static Queue queues[2];
static unsigned next_queue, calls[2];
static uint8_t *dma_buffer[2];
static uint8_t continuous;
static DMA_HandleTypeDef dma;
static UART_HandleTypeDef ports[2]={{.hdmarx=&dma,.id=0U},{.hdmarx=&dma,.id=1U}};
void *RequestSpace(size_t n) { return calloc(1U,n); }
void RecycleSpace(void *p) { free(p); }
osMessageQueueId_t osMessageQueueNew(uint32_t count,uint32_t size,const void *attr)
{ (void)attr; assert(count==UART_RECEIVE_QUEUE_DEPTH && size==sizeof(UartQueueInfo) && next_queue<2U); return &queues[next_queue++]; }
osStatus_t osMessageQueueDelete(osMessageQueueId_t id) { (void)id; return osOK; }
osStatus_t osMessageQueuePut(osMessageQueueId_t id,const void *p,uint8_t priority,uint32_t ticks)
{ Queue *q=id; (void)priority; assert(ticks==0U); if(q->count==UART_RECEIVE_QUEUE_DEPTH)return osError; q->data[q->head]=*(const UartQueueInfo*)p; q->head=(q->head+1U)%UART_RECEIVE_QUEUE_DEPTH; ++q->count; return osOK; }
osStatus_t osMessageQueueGet(osMessageQueueId_t id,void *p,uint8_t *priority,uint32_t ticks)
{ Queue *q=id;(void)priority; assert(ticks==0U); if(!q->count)return osError; *(UartQueueInfo*)p=q->data[q->tail]; q->tail=(q->tail+1U)%UART_RECEIVE_QUEUE_DEPTH; --q->count; return osOK; }
uint32_t osMessageQueueGetCount(osMessageQueueId_t id) { return ((Queue*)id)->count; }
HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *p,uint8_t *buffer,uint16_t length)
{ assert(length==UART_RECEIVE_BUFFER_LENGTH); dma_buffer[p->id]=buffer; p->RxState=HAL_UART_STATE_BUSY_RX; return HAL_OK; }
void HAL_UART_DMAStop(UART_HandleTypeDef *p) { p->RxState=HAL_UART_STATE_READY; }
static void receive(unsigned id)
{ assert(dma_buffer[id]); dma_buffer[id][0]=(uint8_t)id; ports[id].RxState=HAL_UART_STATE_READY; HAL_UARTEx_RxEventCallback(&ports[id],1U); }
static void callback(UART_HandleTypeDef *p,uint8_t *bytes,uint16_t length)
{ assert(length==1U && bytes[0]==p->id); ++calls[p->id]; if(continuous && p->id==0U) { assert(calls[0]<=UART_RECEIVE_PROCESS_BUDGET); receive(0U); } }
int main(void)
{
  InitUartCount(2U);
  assert(AddUart(&ports[0],callback)==1U && AddUart(&ports[1],callback)==2U);
  assert(BeginReceiveUartInfo(1U)==HAL_OK && BeginReceiveUartInfo(2U)==HAL_OK);
  receive(0U); receive(1U); continuous=1U;
  ProcessUart();
  assert(calls[0]==UART_RECEIVE_PROCESS_BUDGET && calls[1]==1U);
  assert(queues[0].count==1U && queues[1].count==0U);
  continuous=0U; ProcessUart();
  assert(calls[0]==UART_RECEIVE_PROCESS_BUDGET+1U && calls[1]==1U);
  assert(GetUartIOInfo(1U)->unDealCount==calls[0]);
  puts("actual UART RX dispatcher: continuous producer bounded; second port serviced");
  return 0;
}

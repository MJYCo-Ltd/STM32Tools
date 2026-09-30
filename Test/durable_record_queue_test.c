/* Production queue + DualBankStore + NOR storage layer, no Agriculture/HAL.
 * Each instance uses a nonzero physical base and caller-selected partition. */
#include <Flash/DurableRecordQueue.h>
#include <Common.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SECTOR 4096U
#define MEDIA_SIZE (7U * SECTOR)
#define DATA_PHYSICAL (3U * SECTOR)

typedef struct {
  uint8_t flash[MEDIA_SIZE], summaries[4];
  StorageBackend backend;
  StoragePartDesc parts[5];
  StoragePartitionMap map;
  DurableRecordQueueConfig config;
  DurableRecordQueue queue;
  uint32_t depth, maximum_depth, begins, ends, reads, writes, erases;
  uint32_t fail_write, fail_read;
  uint8_t reject_lock, fail_ack_read, partial_erase;
} Environment;
static Environment a, b;
static uint32_t record[16], out[16];

static Storage_Status Lock(void *ctx)
{
  Environment *e=ctx;
  if (e->reject_lock) return STORAGE_ERR_BUSY;
  ++e->begins; ++e->depth;
  if (e->depth>e->maximum_depth) e->maximum_depth=e->depth;
  return STORAGE_OK;
}
static void Unlock(void *ctx)
{
  Environment *e=ctx; assert(e->depth!=0U); --e->depth; ++e->ends;
}
static Storage_Status Read(void *ctx, uint32_t address, void *data, uint32_t length)
{
  Environment *e=ctx;
  assert(e->depth && address<=MEDIA_SIZE && length<=MEDIA_SIZE-address);
  ++e->reads;
  if (address==e->fail_read) return STORAGE_ERR_IO;
  memcpy(data,e->flash+address,length); return STORAGE_OK;
}
static Storage_Status Write(void *ctx, uint32_t address, const void *data, uint32_t length)
{
  Environment *e=ctx; const uint8_t *bytes=data;
  assert(e->depth && address<=MEDIA_SIZE && length<=MEDIA_SIZE-address);
  ++e->writes;
  const uint32_t count=address==e->fail_write ? length/2U : length;
  for(uint32_t i=0U;i<count;++i) {
    assert((e->flash[address+i]&bytes[i])==bytes[i]);
    e->flash[address+i]&=bytes[i];
  }
  if(e->fail_ack_read && address>=DATA_PHYSICAL && address%64U==60U && length==4U)
    e->fail_read=address-60U;
  return address==e->fail_write ? STORAGE_ERR_IO : STORAGE_OK;
}
static Storage_Status Erase(void *ctx, uint32_t address)
{
  Environment *e=ctx;
  assert(e->depth && address>=SECTOR && address%SECTOR==0U && address<=MEDIA_SIZE-SECTOR);
  ++e->erases;
  memset(e->flash+address,255,e->partial_erase ? SECTOR/2U : SECTOR);
  return e->partial_erase ? STORAGE_ERR_IO : STORAGE_OK;
}
static Storage_Status Init(Environment *e)
{
  const DualBankTransaction transaction={e,Lock,Unlock};
  return DurableRecordQueue_InitEx(&e->queue,&e->config,&e->map,4U,&transaction);
}
static void Reset(Environment *e)
{
  memset(e,0,sizeof(*e)); memset(e->flash,255,sizeof(e->flash));
  e->fail_write=e->fail_read=UINT32_MAX;
  e->backend=(StorageBackend){.ctx=e,.read=Read,.write=Write,.erase_sector=Erase};
  e->parts[4]=(StoragePartDesc){.base=SECTOR,.size=6U*SECTOR};
  e->map=(StoragePartitionMap){.backend=&e->backend,.parts=e->parts,.part_count=5U,.media_size=MEDIA_SIZE};
  e->config=(DurableRecordQueueConfig){2U*SECTOR,256U,0U,SECTOR,0x12345678U,e->summaries,NULL};
  assert(Init(e)==STORAGE_OK && e->queue.ready);
  assert(!e->depth && e->begins==e->ends && e->maximum_depth>=2U);
  assert(e->flash[SECTOR]!=255U && e->flash[DATA_PHYSICAL]==255U);
}
static Storage_Status Peek(Environment *e)
{
  Storage_Status st=DurableRecordQueue_Peek(&e->queue,out);
  return st==STORAGE_ERR_BUSY ? DurableRecordQueue_Peek(&e->queue,out) : st;
}
static void Append(Environment *e, unsigned count)
{
  for(unsigned i=0U;i<count;++i) assert(DurableRecordQueue_Append(&e->queue,record)==STORAGE_OK);
}
static void Ack(Environment *e)
{
  assert(Peek(e)==STORAGE_OK);
  assert(DurableRecordQueue_Acknowledge(&e->queue,out[1],out[2])==STORAGE_OK);
}

static void TestPortAndLegacy(void)
{
  Reset(&a); Reset(&b);
  Append(&a,1U);
  assert(Peek(&a)==STORAGE_OK && Peek(&b)==STORAGE_ERR_NOT_FOUND);
  uint32_t legacy[16]={0x12345678U,17U,99U,1700000000U,
    0xFFFFFFDDU,UINT32_MAX,1U,10U,1234U,0x00010002U,3U,0U,0U,0U,0x434F4D31U,UINT32_MAX};
  legacy[13]=CalCRC32((const uint8_t *)legacy,52U);
  memcpy(b.flash+DATA_PHYSICAL,legacy,sizeof(legacy));
  assert(Init(&b)==STORAGE_OK && b.queue.epoch==18U);
  assert(Peek(&b)==STORAGE_OK && memcmp(out,legacy,sizeof(legacy))==0);
  assert(memcmp(b.flash+DATA_PHYSICAL,legacy,sizeof(legacy))==0);
  Ack(&b); assert(Peek(&b)==STORAGE_ERR_NOT_FOUND);
  assert(Peek(&a)==STORAGE_OK); /* no singletons, partition/global contamination */
  const uint32_t writes=a.writes, reads=a.reads, ends=a.ends;
  a.reject_lock=1U;
  assert(DurableRecordQueue_Append(&a.queue,record)==STORAGE_ERR_BUSY);
  assert(a.writes==writes && a.reads==reads && a.ends==ends && !a.depth);
  a.reject_lock=0U; Ack(&a);
  const DualBankTransaction invalid={&a,Lock,NULL};
  assert(DurableRecordQueue_InitEx(&a.queue,&a.config,&a.map,4U,&invalid)==STORAGE_ERR_PARAM);
}
static void TestFullAndPartial(void)
{
  Reset(&a); Append(&a,256U);
  const uint32_t erases=a.erases;
  assert(DurableRecordQueue_Append(&a.queue,record)==STORAGE_ERR_NO_SPACE);
  assert(DurableRecordQueue_ReclaimOne(&a.queue)==STORAGE_ERR_NO_SPACE && a.erases==erases);
  for(unsigned i=0U;i<64U;++i) Ack(&a);
  a.partial_erase=1U;
  assert(DurableRecordQueue_ReclaimOne(&a.queue)==STORAGE_ERR_IO);
  a.partial_erase=0U;
  assert(DurableRecordQueue_ReclaimOne(&a.queue)==STORAGE_ERR_NO_SPACE);
  assert(Init(&a)==STORAGE_OK && Peek(&a)==STORAGE_OK && out[2]==65U);
  Reset(&a); a.fail_write=DATA_PHYSICAL;
  assert(DurableRecordQueue_Append(&a.queue,record)==STORAGE_ERR_IO);
  a.fail_write=UINT32_MAX; Append(&a,192U);
  assert(DurableRecordQueue_Append(&a.queue,record)==STORAGE_ERR_NO_SPACE);
  assert(!a.depth && a.begins==a.ends);
}
static void TestUncertainAck(void)
{
  Reset(&a); Append(&a,64U);
  for(unsigned i=0U;i<63U;++i) Ack(&a);
  assert(Peek(&a)==STORAGE_OK && out[2]==64U);
  const uint32_t epoch=out[1], sequence=out[2];
  a.fail_ack_read=1U;
  assert(DurableRecordQueue_Acknowledge(&a.queue,epoch,sequence)==STORAGE_ERR_IO);
  a.fail_ack_read=0U; a.fail_read=UINT32_MAX;
  Append(&a,192U); /* evict first sector from both detailed caches */
  assert(Peek(&a)==STORAGE_OK && out[2]==65U);
  const uint32_t erases=a.erases, writes=a.writes;
  assert(DurableRecordQueue_ReclaimOne(&a.queue)==STORAGE_ERR_NO_SPACE && a.erases==erases);
  assert(DurableRecordQueue_Acknowledge(&a.queue,epoch,sequence+1U)==STORAGE_ERR_NOT_FOUND);
  assert(DurableRecordQueue_Acknowledge(&a.queue,epoch,sequence)==STORAGE_OK && a.writes==writes);
  assert(DurableRecordQueue_ReclaimOne(&a.queue)==STORAGE_OK && a.erases==erases+1U);
  assert(Peek(&a)==STORAGE_OK && out[2]==65U);
  assert(!a.depth && a.begins==a.ends);
}
int main(void)
{
  TestPortAndLegacy(); TestFullAndPartial(); TestUncertainAck();
  puts("durable core: injected partition/recursive transaction, real epoch banks, legacy ABI, faults and ACK pin PASS");
  return 0;
}

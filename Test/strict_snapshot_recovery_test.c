/* Real scanner, bank switching and NOR-style writes. Faults are injected at
 * exact write/erase boundaries; this is host coverage, not hardware testing. */
#include <Flash/DualBankStore.h>
#include <Flash/storage_bank.h>
#include <Flash/storage_record.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define BANK_SIZE 8192U
static uint8_t flash[BANK_SIZE*2U];
static uint32_t writes,erases,fail_write,fail_erase,partial_erase;
static uint8_t commit_then_error;
static uint32_t lock_depth;
static StorageBackend backend;
static StoragePartDesc part={0U,sizeof(flash),0U,{0}};
static StoragePartitionMap map;
typedef struct { uint32_t value; uint8_t padding[5996]; } Snapshot;
static Snapshot payload,scratch_a,scratch_b;

static Storage_Status Read(void *ctx,uint32_t offset,void *out,uint32_t length)
{
  (void)ctx;assert(offset+length<=sizeof(flash));
  memcpy(out,flash+offset,length);return STORAGE_OK;
}
static Storage_Status Write(void *ctx,uint32_t offset,const void *raw,uint32_t length)
{
  (void)ctx;assert(offset+length<=sizeof(flash));++writes;
  if(writes==fail_write && !commit_then_error)return STORAGE_ERR_INJECT;
  const uint8_t *bytes=raw;
  for(uint32_t i=0;i<length;++i)flash[offset+i]&=bytes[i];
  return writes==fail_write ? STORAGE_ERR_INJECT : STORAGE_OK;
}
static Storage_Status Erase(void *ctx,uint32_t offset)
{
  (void)ctx;assert(offset%NOR_FLASH_SECTOR_SIZE==0 && offset+NOR_FLASH_SECTOR_SIZE<=sizeof(flash));
  ++erases;
  if(erases==fail_erase) {
    if(partial_erase)memset(flash+offset,0xff,partial_erase);
    return STORAGE_ERR_INJECT;
  }
  memset(flash+offset,0xff,NOR_FLASH_SECTOR_SIZE);return STORAGE_OK;
}
static Storage_Status Begin(void *ctx)
{ (void)ctx;assert(!lock_depth);++lock_depth;return STORAGE_OK; }
static void End(void *ctx)
{ (void)ctx;assert(lock_depth==1);--lock_depth; }
static const DualBankTransaction transaction={NULL,Begin,End};
static uint8_t Valid(const void *raw)
{ const Snapshot *s=raw;return s->value>=1U && s->value<=3U; }
static void Reset(void)
{
  memset(flash,0xff,sizeof(flash));writes=erases=fail_write=fail_erase=partial_erase=0;
  commit_then_error=0;lock_depth=0;memset(&payload,0,sizeof(payload));
  backend=(StorageBackend){.read=Read,.write=Write,.erase_sector=Erase};
  assert(StoragePartition_Init(&map,&backend,&part,1,sizeof(flash))==STORAGE_OK);
}
static Storage_Status Init(DualBankStore *store,uint8_t strict)
{
  return strict ? DualBankStore_InitStrict(store,&transaction,&map,0,0,BANK_SIZE,BANK_SIZE,
      sizeof(payload),Valid,&scratch_a,&scratch_b) :
      DualBankStore_Init(store,&transaction,&map,0,0,BANK_SIZE,BANK_SIZE,
      sizeof(payload),Valid,&scratch_a,&scratch_b);
}
static void SmallRecord(uint32_t offset,uint32_t sequence,uint32_t value)
{ assert(StorageRecord_WriteAt(&map,0,offset,BANK_SIZE,sequence,&value,sizeof(value))==STORAGE_OK); }
static Storage_Status Scan(uint8_t strict,StorageRecordLoc *loc)
{
  return strict ? StorageRecord_FindLatestStrict(&map,0,0,BANK_SIZE,loc,NULL,0) :
      StorageRecord_FindLatest(&map,0,0,BANK_SIZE,loc,NULL,0);
}

static void TestCommittedCorruption(void)
{
  const uint32_t corrupt_offsets[]={offsetof(StorageRecordHeader,magic),
      offsetof(StorageRecordHeader,format_version),offsetof(StorageRecordHeader,header_size),
      offsetof(StorageRecordHeader,sequence),offsetof(StorageRecordHeader,payload_length),
      offsetof(StorageRecordHeader,payload_crc32),offsetof(StorageRecordHeader,header_crc32),
      sizeof(StorageRecordHeader)};
  StorageRecordLoc loc;
  for(unsigned n=0;n<sizeof(corrupt_offsets)/sizeof(corrupt_offsets[0]);++n) {
    Reset();SmallRecord(0,1,1);SmallRecord(32,2,2);
    flash[32+corrupt_offsets[n]]^=1;
    const Storage_Status status=Scan(1,&loc);
    assert(status==STORAGE_ERR_STATE || status==STORAGE_ERR_CRC);
    assert(Scan(0,&loc)==STORAGE_OK && loc.sequence==1); /* Legacy policy unchanged. */
  }
  Reset();SmallRecord(0,1,1);SmallRecord(32,2,2);
  memset(flash+32,0xff,sizeof(uint32_t)); /* Erased-looking magic cannot hide committed marker. */
  assert(Scan(1,&loc)==STORAGE_ERR_STATE);
}
static void TestTornAndWrap(void)
{
  StorageRecordLoc loc;
  for(uint32_t fail=1;fail<=3;++fail) {
    Reset();SmallRecord(0,UINT32_MAX,1);writes=0;fail_write=fail;
    uint32_t next=2;
    assert(StorageRecord_WriteAt(&map,0,32,BANK_SIZE,0,&next,sizeof(next))==STORAGE_ERR_INJECT);
    fail_write=0;
    assert(Scan(1,&loc)==STORAGE_OK && loc.sequence==UINT32_MAX);
  }
  Reset();SmallRecord(0,UINT32_MAX,1);SmallRecord(32,0,2);SmallRecord(64,UINT32_MAX-1U,3);
  assert(Scan(1,&loc)==STORAGE_OK && loc.sequence==0 && loc.offset==32);
  /* A damaged commit word carries no proof of commitment and is deliberately
   * handled like a torn write; strict recovery cannot promise arbitrary
   * corruption detection without an independent durable commit witness. */
  Reset();SmallRecord(0,1,1);SmallRecord(32,2,2);
  flash[32+offsetof(StorageRecordHeader,commit_marker)]^=1;
  assert(Scan(1,&loc)==STORAGE_OK && loc.sequence==1);
  /* Uncommitted valid-header payload can contain header-looking words. Its
   * independently CRC-checked extent must be skipped without false corruption. */
  Reset();SmallRecord(0,1,1);
  uint8_t fake[64]={0};
  StorageRecordHeader *embedded=(StorageRecordHeader *)(void *)fake;
  embedded->magic=STORAGE_RECORD_MAGIC;embedded->commit_marker=STORAGE_COMMIT_MARKER;
  writes=0;fail_write=3;
  assert(StorageRecord_WriteAt(&map,0,32,BANK_SIZE,2,fake,sizeof(fake))==STORAGE_ERR_INJECT);
  fail_write=0;
  assert(Scan(1,&loc)==STORAGE_OK && loc.sequence==1);
}
static void TestStrictStore(void)
{
  DualBankStore store;
  Reset();assert(Init(&store,1)==STORAGE_ERR_NOT_FOUND && store.strict_recovery);
  payload.value=1;assert(DualBankStore_Save(&store,&payload)==STORAGE_OK);
  flash[sizeof(StorageRecordHeader)]^=1;
  assert(DualBankStore_Load(&store,&scratch_a)==STORAGE_ERR_CRC);
  const uint32_t before=writes;
  assert(DualBankStore_Save(&store,&payload)==STORAGE_ERR_CRC && writes==before && !lock_depth);
  assert(Init(&store,1)==STORAGE_ERR_CRC && !store.have_snapshot);
  Reset();payload.value=1;
  assert(StorageRecord_WriteAt(&map,0,0,BANK_SIZE,UINT32_MAX,&payload,sizeof(payload))==STORAGE_OK);
  payload.value=2;
  assert(StorageRecord_WriteAt(&map,0,BANK_SIZE,sizeof(flash),0,&payload,sizeof(payload))==STORAGE_OK);
  assert(Init(&store,1)==STORAGE_OK && store.active_bank==1 && store.next_sequence==1);
  assert(DualBankStore_Load(&store,&scratch_a)==STORAGE_OK && scratch_a.value==2);
  flash[BANK_SIZE+sizeof(StorageRecordHeader)]^=1;
  assert(Init(&store,1)==STORAGE_ERR_CRC && !store.have_snapshot);
  assert(Init(&store,0)==STORAGE_OK && scratch_a.value==1 && !store.strict_recovery);
}
static void TestGcEraseFaults(void)
{
  DualBankStore store;
  /* Two sectors per bank, 6000-byte payload, so every second Save switches
   * banks. Sweep failure before each destination/old-bank sector erase. */
  for(uint32_t fail=1;fail<=4;++fail) {
    Reset();assert(Init(&store,1)==STORAGE_ERR_NOT_FOUND);
    payload.value=1;assert(DualBankStore_Save(&store,&payload)==STORAGE_OK);
    payload.value=2;fail_erase=fail;
    assert(DualBankStore_Save(&store,&payload)==STORAGE_ERR_INJECT && !lock_depth);
    fail_erase=0;
    assert(Init(&store,1)==STORAGE_OK);
    assert(DualBankStore_Load(&store,&scratch_a)==STORAGE_OK);
    assert(scratch_a.value==(fail<=2 ? 1U : 2U));
  }
  /* A partial old-bank header erase is ambiguous even with a valid newer
   * other bank: strict recovery reports it rather than trusting generation. */
  Reset();assert(Init(&store,1)==STORAGE_ERR_NOT_FOUND);
  payload.value=1;assert(DualBankStore_Save(&store,&payload)==STORAGE_OK);
  payload.value=2;fail_erase=3;partial_erase=1;
  assert(DualBankStore_Save(&store,&payload)==STORAGE_ERR_INJECT);
  fail_erase=0;
  assert(Init(&store,1)==STORAGE_ERR_STATE && !store.have_snapshot);
  assert(Init(&store,0)==STORAGE_OK && scratch_a.value==2);
  /* If the old first sector was wholly erased, its remaining tail cannot
   * resurrect an old record; the complete new bank remains usable. */
  Reset();assert(Init(&store,1)==STORAGE_ERR_NOT_FOUND);
  payload.value=1;assert(DualBankStore_Save(&store,&payload)==STORAGE_OK);
  payload.value=2;fail_erase=4;partial_erase=128;
  assert(DualBankStore_Save(&store,&payload)==STORAGE_ERR_INJECT);
  fail_erase=0;
  assert(Init(&store,1)==STORAGE_OK && scratch_a.value==2);
}
static void TestGcWriteFaults(void)
{
  DualBankStore store;
  for(uint32_t fail=1;fail<=3;++fail) {
    Reset();assert(Init(&store,1)==STORAGE_ERR_NOT_FOUND);
    payload.value=1;assert(DualBankStore_Save(&store,&payload)==STORAGE_OK);
    writes=0;fail_write=fail;payload.value=2;
    assert(DualBankStore_Save(&store,&payload)==STORAGE_ERR_INJECT);
    fail_write=0;
    assert(Init(&store,1)==STORAGE_OK && scratch_a.value==1);
  }
  Reset();assert(Init(&store,1)==STORAGE_ERR_NOT_FOUND);
  payload.value=1;assert(DualBankStore_Save(&store,&payload)==STORAGE_OK);
  writes=0;fail_write=3;commit_then_error=1;payload.value=2;
  assert(DualBankStore_Save(&store,&payload)==STORAGE_ERR_INJECT);
  fail_write=0;commit_then_error=0;
  assert(Init(&store,1)==STORAGE_OK && scratch_a.value==2);
}
int main(void)
{
  TestCommittedCorruption();TestTornAndWrap();TestStrictStore();TestGcEraseFaults();TestGcWriteFaults();
  puts("strict snapshot recovery: committed corruption fails closed; torn writes, wrap and GC fault sweep PASS");
  return 0;
}

#include <Flash/storage_log.h>
#include <Flash/storage_commit.h>
#include <Flash/nor_flash.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static uint8_t flash[8192];
static uint32_t fail_read=UINT32_MAX, fail_write=UINT32_MAX, reads, erases;
static StoragePartitionMap map;
void StorageBackend_Poll(const StorageBackend *b) { (void)b; }
Storage_Status Storage_Read(const StoragePartitionMap *m,uint32_t p,uint32_t o,void *b,uint32_t n)
{ (void)m;(void)p;assert(o<=sizeof(flash)&&n<=sizeof(flash)-o);++reads;
  if(o==fail_read)return STORAGE_ERR_IO;memcpy(b,flash+o,n);return STORAGE_OK; }
Storage_Status Storage_Write(const StoragePartitionMap *m,uint32_t p,uint32_t o,const void *b,uint32_t n)
{ (void)m;(void)p;assert(o<=sizeof(flash)&&n<=sizeof(flash)-o);
  uint32_t count=o==fail_write?n/2:n;const uint8_t *src=b;
  for(uint32_t i=0;i<count;++i)flash[o+i]&=src[i];return o==fail_write?STORAGE_ERR_IO:STORAGE_OK; }
Storage_Status Storage_EraseSector(const StoragePartitionMap *m,uint32_t p,uint32_t o)
{ (void)m;(void)p;assert(o%4096==0&&o+4096<=sizeof(flash));++erases;memset(flash+o,255,4096);return STORAGE_OK; }
static Storage_Status Next(StorageLog *l,StorageLogCursor *c,StorageLogReceipt *r,uint8_t *b)
{ Storage_Status st;uint32_t n=0;unsigned budget=2000;
  do {assert(budget--);st=StorageLog_NextPending(l,c,r,b,152,&n);}while(st==STORAGE_ERR_BUSY);
  if(st==STORAGE_OK)assert(n==152);return st; }
static void Reset(StorageLog *l)
{ memset(flash,255,sizeof(flash));fail_read=fail_write=UINT32_MAX;reads=erases=0;
  assert(StorageLog_Init(l,&map,0,0,sizeof(flash))==STORAGE_OK); }
int main(void)
{
  StorageLog l;StorageLogCursor c={0};StorageLogReceipt r,first;uint8_t b[152]={0},out[152];
  Reset(&l);
  for(unsigned i=1;i<=30;++i){b[0]=(uint8_t)i;assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_OK);}
  unsigned erase_before=erases;
  for(unsigned i=1;i<=30;++i){
    assert(Next(&l,&c,&r,out)==STORAGE_OK&&out[0]==i);
    if(i==1){first=r;b[0]=31;assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_OK);}
    assert(StorageLog_Acknowledge(&l,&r)==STORAGE_OK);
    assert(StorageLog_Acknowledge(&l,&r)==STORAGE_OK); /* idempotent */
  }
  assert(Next(&l,&c,&r,out)==STORAGE_OK&&out[0]==31); /* added during upload */
  assert(StorageLog_Acknowledge(&l,&r)==STORAGE_OK);
  assert(erases==erase_before); /* ACK never erases the whole partition. */
  const uint32_t sequence=l.next_sequence, offset=l.write_offset_in_sector;
  assert(StorageLog_Init(&l,&map,0,0,sizeof(flash))==STORAGE_OK);
  assert(l.next_sequence==sequence&&l.write_offset_in_sector==offset);
  memset(&c,0,sizeof(c));assert(Next(&l,&c,&r,out)==STORAGE_ERR_NOT_FOUND);
  b[0]=32;assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_OK);
  assert(Next(&l,&c,&r,out)==STORAGE_OK&&out[0]==32&&r.record.sequence==sequence);
  /* An interrupted ACK is not lost; reboot can retry it without old payload copies. */
  fail_write=r.record.offset+offsetof(StorageRecordHeader,commit_marker);
  assert(StorageLog_Acknowledge(&l,&r)==STORAGE_ERR_IO);fail_write=UINT32_MAX;
  assert(StorageLog_Init(&l,&map,0,0,sizeof(flash))==STORAGE_OK);
  memset(&c,0,sizeof(c));assert(Next(&l,&c,&r,out)==STORAGE_OK&&out[0]==32);
  assert(StorageLog_Acknowledge(&l,&r)==STORAGE_OK);
  /* Reusing a ring sector invalidates old receipts even when offsets match. */
  for(unsigned i=0;i<50;++i)assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_OK);
  assert(StorageLog_Acknowledge(&l,&first)==STORAGE_ERR_NOT_FOUND);
  Reset(&l);memset(&c,0,sizeof(c));b[0]=99;
  assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_OK);
  fail_read=sizeof(StorageLogSectorHeader)+sizeof(StorageRecordHeader);
  assert(Next(&l,&c,&r,out)==STORAGE_ERR_IO);fail_read=UINT32_MAX;
  assert(Next(&l,&c,&r,out)==STORAGE_OK&&out[0]==99); /* read error did not advance */
  /* Explicit clear must not make a stale receipt identify the next record. */
  first=r;
  assert(StorageLog_Clear(&l)==STORAGE_OK);
  assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_OK);
  assert(StorageLog_Acknowledge(&l,&first)==STORAGE_ERR_NOT_FOUND);
  /* A torn original commit (a superset, rather than subset, of COMMIT) is not delivered. */
  Reset(&l);fail_write=sizeof(StorageLogSectorHeader)+offsetof(StorageRecordHeader,commit_marker);
  assert(StorageLog_Append(&l,b,sizeof(b))==STORAGE_ERR_IO);fail_write=UINT32_MAX;
  assert(StorageLog_Init(&l,&map,0,0,sizeof(flash))==STORAGE_OK);
  memset(&c,0,sizeof(c));assert(Next(&l,&c,&r,out)==STORAGE_ERR_NOT_FOUND);
  puts("stream log: >24 records, concurrent append, reboot, torn ACK/commit, stale receipt and read faults passed");
  return 0;
}

// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct spinlock bucketlock[BNUM];
  struct buf buf[NBUF];

  // Linked lists of buffers in different buckets, through prev/next.
  struct buf head[BNUM];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for (int i = 0; i < BNUM; i++) {
    initlock(bcache.bucketlock + i, "bcache");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next  = &bcache.head[i];

  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketno = blockno % BNUM;

  // printf("acquring get lock %d\n", bucketno);
  acquire(bcache.bucketlock + bucketno);

  // Is the block already cached?
  for(b = bcache.head[bucketno].next; b != &bcache.head[bucketno]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(bcache.bucketlock + bucketno);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Find closest bucket's LRU buffer
  for(int i = (bucketno + 1) % BNUM; i != bucketno; i = (i + 1) % BNUM) {
    uint targetticks = 0x7fffffff;
    struct buf *targetbuf = 0;
    // printf("acquring other lock %d\n", i);
    acquire(bcache.bucketlock + i);
    for (b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev) {
      if (b->refcnt == 0 && b->ticks < targetticks) {
        targetticks = b->ticks;
        targetbuf = b;
      }
    }

    if(targetbuf != 0) {
      targetbuf->dev = dev;
      targetbuf->blockno = blockno;
      targetbuf->valid = 0;
      targetbuf->refcnt = 1;
      targetbuf->next->prev = targetbuf->prev;
      targetbuf->prev->next = targetbuf->next;
      targetbuf->next = bcache.head[bucketno].next;
      targetbuf->prev = &bcache.head[bucketno];
      bcache.head[bucketno].next->prev = targetbuf;
      bcache.head[bucketno].next = targetbuf;
      release(bcache.bucketlock + i);
      release(bcache.bucketlock + bucketno);
      acquiresleep(&targetbuf->lock);
      return targetbuf;
    }
    release(bcache.bucketlock + i);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*

bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucketno = b->blockno & BNUM;
  acquire(&bcache.lock + bucketno);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->ticks = ticks;
  }
  
  release(&bcache.lock + bucketno);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}



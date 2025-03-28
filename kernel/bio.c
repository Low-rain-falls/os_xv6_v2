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

// buffer cache structure similar to buffer structure
struct {
  struct spinlock lock;
  struct buf buf[NBUF]; // array of all buffers in cache

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head; // first node in linked list of buffers
} bcache;

//initialize cache 
void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache"); //initialize lock named "bcache"

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer"); // init sleeplock to synchronize access individually
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// find buffer or allcate new buffer if cannot find
// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // Is the block already cached? (find exist buffer)
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    //recycle buffer which has re
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers"); //if there are no available buffer call panic.
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  //check if the data is valid or not
  if(!b->valid) {
    virtio_disk_rw(b, 0); //write data into buffer
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
// Synchronize the contents of buffer b with the block on disk.
void
bwrite(struct buf *b)
{
  //check if buffer is locked by instance process
  if(!holdingsleep(&b->lock))
    panic("bwrite"); //call panic
  virtio_disk_rw(b, 1); // write data into buffer
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  //check if buffer is lock
  if(!holdingsleep(&b->lock))
    panic("brelse"); // call panic
  //release lock buffer
  releasesleep(&b->lock);

  //reduce refcnt
  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it and move it to LRU
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}

//pin buffer to prevent buffer from reusing
void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

//unpin buffer
void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}



// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

uint64 kfree_memsize(void);
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

//pages have a run as a node in linked list
struct run {
  struct run *next;
};

//memory structure contain:
struct {
  struct spinlock lock; //synchronize access when allocation or releasing
  struct run *freelist; //pointer to free list of page
} kmem;

// initialize
void
kinit()
{
  initlock(&kmem.lock, "kmem"); //initialize a lock named "kmem"
  freerange(end, (void*)PHYSTOP); //release a range of page from "end" to phystop = put a range to free list pf page
}

// release range of page 
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start); // round the value
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) // loop through each page
    kfree(p); //relase each page = put each page into free list of page
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  //check valid: size of page, in ragnge from end to phystop
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree"); //stop the system

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  //put page pa to free list
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  //get the first page in the free list
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk to easily detect errors if there is misuse.
  return (void*)r;
}

// coun the number of free memory
uint64 kfree_memsize(void) {
  acquire(&kmem.lock);

  uint64 free_mem = 0;
  struct run *r = kmem.freelist;


  while(r) {
    free_mem += PGSIZE;
    r = r->next;
  }

  release(&kmem.lock);
  return free_mem;
}
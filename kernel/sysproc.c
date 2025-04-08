#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgpte(void)
{
  uint64 va;
  struct proc *p;  

  p = myproc();
  argaddr(0, &va);
  pte_t *pte = pgpte(p->pagetable, va);
  if(pte != 0) {
      return (uint64) *pte;
  }
  return 0;
}
#endif

#ifdef LAB_PGTBL
int
sys_kpgtbl(void)
{
  struct proc *p;  

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif


uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_pgaccess(void) {
  uint64 va; // virtual address
  int num_page;
  uint64 mask_addr; // buffer for bitmask
  
  vmprint(myproc()->pagetable);

  argaddr(0, &va);
  argint(1, &num_page);
  argaddr(2, &mask_addr);

  if (num_page > 64 || num_page <= 0 || mask_addr == 0 || va == 0) {
    return -1;
  }

  struct proc *p = myproc();
  uint64 bitmask = 0;

  for (int i = 0; i < num_page; i++) {
    pte_t *pte = walk(p->pagetable, va + i * PGSIZE, 0);

    // check valid
    if (pte && (*pte & PTE_V)) {
      // check access
      if (*pte & PTE_A) {
        bitmask |= (1ULL << 1); // set bit
        *pte &= ~PTE_A;
      }
    }
  }

  if (copyout(p->pagetable, mask_addr, (char*)&bitmask, sizeof(bitmask)) < 0) {
    return -1;
  }

  return 0;
}

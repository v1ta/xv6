#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0) return -1;

  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);

      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Halt (shutdown) the system by sending a special
// signal to QEMU.
// Based on: http://pdos.csail.mit.edu/6.828/2012/homework/xv6-syscall.html
// and: https://github.com/t3rm1n4l/pintos/blob/master/devices/shutdown.c
int
sys_halt(void)
{
  char *p = "Shutdown";
  for( ; *p; p++)
    outw(0xB004, 0x2000);
  return 0;
}

int
sys_clone(void)
{ 
  int fn, arg, stack;

  //error checking
  if (argint(0, &fn) < 0)  return -1;
  else if (argint(1, &arg) < 0)  return -1;
  else if (argint(2, &stack) < 0)  return -1;

  return clone((void *) fn, (void *) arg, (void *) stack);
}

int
sys_join(void)
{
  int pid, stack, ret;

  //error checking
  if (argint(0, &pid) < 0)  return -1;
  else if (argint(1, &stack) < 0)  return -1;
  else if (argint(2, &ret) < 0)  return -1;

  return join(pid, (void **) stack, (void **) ret);
}

int
sys_texit(void)
{
  int ret;

  if (argint(0, &ret) < 0) return -1;

  proc->ret_val = (void *)ret;
  exit();

  return 0;
}

int 
sys_mutex_unlock(void)
{
  int id;
   
  //error checking 
  if (argint(0,&id)<0)  return -1;
  else if (id>31 || id<0)  return -1;
  else if (proc->mtable[id].active == 0)  return -1;
  
  acquire(&(proc->parent->mtable[id].sl));
  
  proc->parent->mtable[id].locked = 0;
  wakeup(proc->parent->mtable[id].chan);
  release(&(proc->parent->mtable[id].sl));
   
  return 0;
}

int 
sys_mutex_destroy(void) 
{
  int id;

  // error checking 
  if (argint(0,&id)<0) return -1;
  else if (id>31 || id<0) return -1;
  else if (proc->mtable[id].locked == 1) return -1;
  else if (proc->mtable[id].active == 0) return -1;
    
  proc->mtable[id].active = 0;
  return 0;
}

int 
sys_mutex_lock(void)
{
  int mutex_id;
  
  //error checking 
  if (argint(0,&mutex_id)<0) return -1; 
  else if (mutex_id>31 || mutex_id<0) return -1;
  else if (proc->mtable[mutex_id].active == 0)  return -1;

  acquire(&(proc->parent->mtable[mutex_id].sl));

  while(proc->parent->mtable[mutex_id].locked == 1) {
    sleep(proc->parent->mtable[mutex_id].chan,&(proc->parent->mtable[mutex_id].sl));
  }
  
  proc->parent->mtable[mutex_id].locked = 1;
  release(&(proc->parent->mtable[mutex_id].sl));

  return 0;   
}

int 
sys_mutex_init(void)
{
  int i;

  for(i = 0; i < 32; i++) {
    if (proc->mtable[i].active == 0){
      proc->mtable[i].active = 1;
      proc->mtable[i].mid = i;
      proc->mtable[i].chan = &(proc->mtable[i]);

      return i;
    }
  }
  return -1;
}

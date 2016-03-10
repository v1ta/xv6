#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "param.h"
#include "signal.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  int i;
  struct proc *p;
  if(tf->trapno == T_SYSCALL){
    if(proc->killed)
      exit();
    proc->tf = tf;
    syscall();
    if(proc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
  	 for (i = 0; i < NPROC; i++) {
	     p = (struct proc*) get_process(i);
               if (p && p->pending > 0) {
		     p->alarm_ticks++;
		     if (p->alarm_ticks >= p->pending) {
			     p->pending = 0;
			     p->alarm_ticks = 0;
			     struct siginfo info; 
			     info.signum = SIGALRM;
			     *((siginfo_t*)(proc->tf->esp - 4)) = info;
                             cprintf("&info is %d, info is %d, info.signum is %d\n", &info, info, info.signum); 
	 		     p->tf->esp -= 8;
	 		     p->tf->eip = p->handlers[1];
		    }
	    }
	  }
    if(cpu->id == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpu->id, tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_DIVIDE:
    ;
    uint old_eip  = tf->eip +4;
    uint old_esp  = tf->esp;
    uint old_eax  = tf->eax;
    uint old_edx  = tf->edx;
    uint old_ecx  = tf->ecx;
    uint old = (int) proc->old;


    asm volatile (
      "movl %1, (%%eax)\t \n" //addr of old vals -> stack
      "movl $0, 4(%%eax)\t \n"//SIGFPE -> stack
      "movl %2, 8(%%eax)\t \n"//edx -> stack
      "movl %3, 12(%%eax)\t \n"//ecx -> stack
      "movl %4, 16(%%eax)\t \n"//eax -> stack
      "movl %5, 20(%%eax)\t \n"//old eip -> stack
      "addl $24, %%eax\t \n" //grow stack
      :  :
      "r" (old_esp),
      "r" (old),
      "r" (old_edx),
      "r" (old_ecx),
      "r" (old_eax),
      "r" (old_eip));

    tf->eip = (int) proc->handlers[0];
    break;
  //PAGEBREAK: 13
  default:
    if(proc == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpu->id, tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip,
            rcr2());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();
}


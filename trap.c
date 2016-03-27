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
    if(cpu->id == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
      tick_alarms();
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
    if (proc->handlers[0] >= 0) {
    siginfo_t info = { .signum = SIGFPE };
    uint oldeip = proc->tf->eip;
    if (proc->skip == 1) {
      proc->tf->eip = proc->tf->eip + 4;
      return;
    } 
      proc->tf->eip = (uint) proc->handlers[0];

      *((uint*) (proc->tf->esp - 4)) = oldeip;
      *((uint*) (proc->tf->esp - 8)) = proc->tf->eax;
      *((uint*) (proc->tf->esp - 12)) = proc->tf->ecx;
      *((uint*) (proc->tf->esp - 16)) = proc->tf->edx;
      *((siginfo_t*)(proc->tf->esp - 20)) = info;  
      *((uint*) (proc->tf->esp - 24)) = proc->trampoline;
      proc->tf->esp -= 24;

      return;
    }
   
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip, 
            rcr2());
    proc->killed = 1;

    if (proc->killed)
      exit();
    return;
      /* cprintf("heya!!");
      uint old_eip  = tf->eip +4;
      uint old_esp  = tf->esp;
      uint old_eax  = tf->eax;
      uint old_edx  = tf->edx;
      uint old_ecx  = tf->ecx;
      uint trampoline = proc->trampoline;


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
        "r" (trampoline),
        "r" (old_edx),
        "r" (old_ecx),
        "r" (old_eax),
        "r" (old_eip));

      tf->eip = (int) proc->handlers[0];
    }
    */
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

    if(proc && proc->pending == 1){
		  if (proc->alarm_ticks == 0) {
			  proc->pending = 0;
			  siginfo_t info = (siginfo_t) { .signum = SIGALRM };
			  uint oldeip = proc->tf->eip;
			  proc->tf->eip = (uint) proc->handlers[1];

			  *((uint*) (proc->tf->esp - 4)) = oldeip;
			  *((uint*) (proc->tf->esp - 8)) = proc->tf->eax;
			  *((uint*) (proc->tf->esp - 12)) = proc->tf->ecx;
			  *((uint*) (proc->tf->esp - 16)) = proc->tf->edx;
			  *((siginfo_t*)(proc->tf->esp - 20)) = info;

			  *((uint*) (proc->tf->esp - 24)) = proc->trampoline;

	 		  proc->tf->esp -= 24;
		  }
	}

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();
}

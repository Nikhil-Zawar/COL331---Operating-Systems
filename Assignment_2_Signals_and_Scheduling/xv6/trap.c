#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

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
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);

      if(myproc() && myproc()->state == RUNNING){
        myproc()->ticks_used++;

        if(myproc()->exec_time != -1){
          if(myproc()->ticks_used > myproc()->exec_time){
            // cprintf("Killing pid");
            kill(myproc()->pid);
          }
        }
      }
      
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
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  case T_PGFLT:
    uint f_address = rcr2();
    if (f_address == 0xDEADC0DE && myproc() && myproc()->within_handler==1 && myproc()->handler_exist==1){

      myproc()->within_handler=0;
      tf->esp = myproc()->esp_old;
      tf->edx = myproc()->edx_old;
      tf->ebp = myproc()->ebp_old;
      tf->eflags = myproc()->eflags_old;
      tf->ebx = myproc()->ebx_old;
      tf->ecx = myproc()->ecx_old;
      tf->esi = myproc()->esi_old;
      tf->eip = myproc()->eip_old;
      tf->eax = myproc()->eax_old;
      tf->edi = myproc()->edi_old;
      break;
    }

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  if(myproc() && myproc()->signal_pending == 1 && myproc()->within_handler == 0 && (tf->cs&3) == DPL_USER) {

      myproc()->signal_pending = 0; // Set the pending signal flag to zero
      myproc()->within_handler = 1;

      // *(uint*)myproc()->tf->esp = myproc()->tf->eip;  // Save the current instruction pointer

      // myproc()->tf->eip = (uint)myproc()->handler;  // Set the instruction pointer to the address of the signal handler
          
      myproc()->edx_old = tf->edx;
      myproc()->esp_old = tf->esp;
      myproc()->eflags_old = tf->eflags;
      myproc()->eax_old = tf->eax;
      myproc()->ebx_old = tf->ebx;
      myproc()->esi_old = tf->esi;
      myproc()->eip_old = tf->eip;
      myproc()->ecx_old = tf->ecx;
      myproc()->ebp_old = tf->ebp;
      myproc()->edi_old = tf->edi;

      myproc()->tf->esp -= 4;  // Push the return address to the user stack

      uint r_address = 0xDEADC0DE;
      if(copyout(myproc()->pgdir, tf->esp, &r_address, sizeof(r_address)) < 0){
        myproc()->within_handler = 0;
        return;
      }
      myproc()->tf->eip = (uint)myproc()->handler;
  } 
}

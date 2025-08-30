#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

// jab jab process banega - maintain history array
// when sys_gethistory is called it should print the array in acscening pid

int sys_gethistory(void) {
  struct history_entry temp;    
    // Sort history in ascending order of PID
  for (int i = 0; i < history_count - 1; i++) {
      for (int j = 0; j < history_count - i - 1; j++) {
          if (process_history[j].pid > process_history[j + 1].pid) {
              temp = process_history[j];
              process_history[j] = process_history[j + 1];
              process_history[j + 1] = temp;
          }
      }
  }
  for (int i = 0; i < history_count; i++) {
      cprintf("%d %s %d\n", process_history[i].pid, process_history[i].name, process_history[i].mem_usage);
  }
  return 0;
}

int sys_block(void){
  int syscall_id;
  if(argint(0, &syscall_id) < 0)
    return -1;
  if(syscall_id == 1 || syscall_id == 2)
    return -1;
  struct proc *curproc = myproc();
  curproc->blocked_syscalls[syscall_id] = 1;
  return 0;
}

int sys_unblock(void){
  int syscall_id;
  if(argint(0, &syscall_id) < 0)
    return -1;
  struct proc *curproc = myproc();
  curproc->blocked_syscalls[syscall_id] = 0;
  return 0;
}

int sys_fork(void){
  return fork();
}

int sys_exit(void){
  exit();
  return 0;  // not reached
}

int sys_wait(void){
  return wait();
}

int sys_kill(void){
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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

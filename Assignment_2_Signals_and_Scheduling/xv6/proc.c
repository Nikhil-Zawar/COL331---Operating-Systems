#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  // Initialising the in_bgd bit to 0 and handler pointer to -1
  p->in_bgd = 0;
  p->handler_exist = 0;
  p->handler = -1;
  p->signal_pending = 0;
  p->within_handler = 0;

  p->start_later= 0;
  p->exec_time = -1;
  p->ticks_used = 0;

  p->creation_time = ticks;
  p->start_time = 0;
  p->termination_time = 0;
  p->context_switches = 0;
  p->has_started = 0;

  p->base_priority = INIT_PRIORITY;
  p->dynamic_priority = INIT_PRIORITY;
  p->cpu_ticks = 0;
  p->waiting_ticks = 0;
  p->last_run_time = 0;
  p->total_bgd_time = 0;
  p->last_bgd_stamp = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

    // Child inherits parent's priority state
    np->base_priority = curproc->base_priority;
    np->dynamic_priority = curproc->dynamic_priority;
    np->cpu_ticks = 0;
    np->waiting_ticks = 0;
    np->last_run_time = 0;
    np->total_bgd_time = 0;
    np->last_bgd_stamp = 0;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Record termination time
  curproc->termination_time = ticks;
  int tat = curproc->termination_time - curproc->creation_time;
  int wt = (curproc->termination_time - curproc->start_time) - (curproc->ticks_used) - (curproc->total_bgd_time);
  if (wt < 0){
    wt = 0;
  }

  int rt = curproc->start_time - curproc->creation_time;
  
  // Print metrics
  cprintf("PID: %d\n", curproc->pid);
  cprintf("TAT: %d\n", tat);
  cprintf("WT: %d\n", wt);
  cprintf("RT: %d\n", rt);
  cprintf("#CS: %d\n", curproc->context_switches);
//   show_priorities();

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;

        // Ignoring the backgrounded processes in counting the child for the starting init shell
        if (p->in_bgd == 1 && p->parent->pid == 2){
            continue;
        }

      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct proc *highest_pri_proc;
  struct proc *prev_sched = 0;

  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    // Find process with highest priority
    highest_pri_proc = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

        if (p->state == ZOMBIE){
            kfree(p->kstack);
            p->kstack = 0;
            freevm(p->pgdir);
            p->pid = 0;
            p->parent = 0;
            p->name[0] = 0;
            p->killed = 0;
            p->state = UNUSED;
        }
        // Ignoring the backgrounded processes in scheduling
        if (p->in_bgd == 1){
            continue;
        }

      if(p->state != RUNNABLE)
        continue;

      if(p->start_later == 1){
        continue;
      }

      // Track first execution time (for response time)
      if(!p->has_started) {
        p->start_time = ticks;
        p->has_started = 1;
      }

      // Update waiting time for all runnable processes
      if(p->last_run_time > 0){
        p->waiting_ticks += (ticks - p->last_run_time);
      }
      else{
        p->waiting_ticks += (ticks - p->start_time);
      }

      update_priority(p);

      // Select process with highest priority (lowest PID for ties)
      if(!highest_pri_proc || 
        (p->dynamic_priority > highest_pri_proc->dynamic_priority) ||
        (p->dynamic_priority == highest_pri_proc->dynamic_priority && 
         p->pid < highest_pri_proc->pid)) {
       highest_pri_proc = p;
     }

      // // Switch to chosen process.  It is the process's job
      // // to release ptable.lock and then reacquire it
      // // before jumping back to us.
      // p->context_switches++;
      // c->proc = p;
      // switchuvm(p);
      // p->state = RUNNING;

      // swtch(&(c->scheduler), p->context);
      // switchkvm();

      // // Process is done running for now.
      // // It should have changed its p->state before coming back.
      // c->proc = 0;
    }

    if(highest_pri_proc) {
    //   cprintf("highest priority PID: %d\n", highest_pri_proc->pid);
      // Run the selected process
      if (highest_pri_proc != prev_sched){
        highest_pri_proc->context_switches++;
      }
      prev_sched = highest_pri_proc;

      c->proc = highest_pri_proc;
      switchuvm(highest_pri_proc);
      highest_pri_proc->state = RUNNING;
      
      // Update process tracking variables
      highest_pri_proc->last_run_time = ticks;
      highest_pri_proc->waiting_ticks = 0;
      
      swtch(&(c->scheduler), highest_pri_proc->context);
      switchkvm();

      // Process has yielded, update CPU usage
      if(c->proc) {
        c->proc->cpu_ticks += ticks - c->proc->last_run_time;
        c->proc = 0;
      }
    }

    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;

      // Process is necessary to be runnable when killing
      if (p->in_bgd == 1){
        p->total_bgd_time += ticks - p->last_bgd_stamp;
      }
      p->in_bgd = 0;
      
      p->start_later = 0;

      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    // New state name for backgrounded processes
    if (p->in_bgd == 1){
        state = "bgd";
    }

    if (p->start_later == 1){
        state = "start_later";
    }

    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

// Definition of new functions for signal handling

// Signal Interrupt
void sigint(){
    cprintf("Ctrl-C is detected by xv6\n");
    struct  proc *p;
    
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED)
          continue;

        if (p->state == ZOMBIE){
            kfree(p->kstack);
            p->kstack = 0;
            freevm(p->pgdir);
            p->pid = 0;
            p->parent = 0;
            p->name[0] = 0;
            p->killed = 0;
            p->state = UNUSED;
        }

        else if (p->pid > 2){
            kill(p->pid);
            // cprintf("Process with pid %d killed\n", p->pid);
        }
      }
    
    // cprintf("Ctrl-C handling completed\n");
}

// Signal Background
void sigbg(){
    cprintf("Ctrl-B is detected by xv6\n");

    struct  proc *p;
    struct proc* pshell;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED)
          continue;

        if (p->state == ZOMBIE){
            kfree(p->kstack);
            p->kstack = 0;
            freevm(p->pgdir);
            p->pid = 0;
            p->parent = 0;
            p->name[0] = 0;
            p->killed = 0;
            p->state = UNUSED;
        }

        if (p->in_bgd == 1){
            continue;
        }
        
        if (p->pid == 2){
            pshell = p;
        }

        else if (p->pid > 2){
            acquire(&ptable.lock);
            p->in_bgd = 1;
            p->last_bgd_stamp = ticks;
            release(&ptable.lock);
            // cprintf("Process with pid %d suspended\n", p->pid);
        }
      }

    wakeup(pshell); 
    // cprintf("Ctrl-B handling completed\n");
}

// Singal Foreground
void sigfg(){
    cprintf("Ctrl-F is detected by xv6\n");
    struct  proc *p;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED)
          continue;

        if (p->state == ZOMBIE){
            kfree(p->kstack);
            p->kstack = 0;
            freevm(p->pgdir);
            p->pid = 0;
            p->parent = 0;
            p->name[0] = 0;
            p->killed = 0;
            p->state = UNUSED;
        }

        if (p->killed == 1 || p->in_bgd == 0){
            continue;
        }
        else if (p->pid > 2){
            acquire(&ptable.lock);
            p->in_bgd = 0;
            p->total_bgd_time += ticks - p->last_bgd_stamp;
            release(&ptable.lock);
            // cprintf("Process with pid %d revived\n", p->pid);
        }
      }
    // cprintf("Ctrl-F handling completed\n");
}

// Signal Custom
void sigcustom(){
    cprintf("Ctrl-G is detected by xv6\n");
    struct  proc *p;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->pid > 2){
            if (p->handler_exist == 0){
                // cprintf("Signal handler is not registered\n");
                // cprintf("Ctrl-G handling completed\n");
                return;
            }
            p->signal_pending = 1;
        }
      }

    // cprintf("Ctrl-G handling completed\n");
}


// Signal register function called by the system call identifier
int sys_signal(void)
{
    sighandler_t handler; 

    argstr(0, &handler);

    struct  proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->pid > 2){
            p->handler_exist = 1;
            p->handler = handler;
        }
      }
    return 0;
}

int custom_fork(int start_later, int exec_time){
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  np->start_later = start_later;
  np->exec_time = exec_time;
  np->ticks_used = 0;
//   cprintf("Forked process variables: start_later: %d, exec_time: %d, ticks_used: %d\n", np->start_later, np->exec_time, np->ticks_used);

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);
  return pid;
}



int scheduler_start(void){
  struct proc *p;
  int found = 0;

  acquire(&ptable.lock);

  // scan through process table looking for processes to start
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED && p->start_later){
      p->state = RUNNABLE;
      p->start_later = 0;
      p->total_bgd_time = 0;
      p->last_bgd_stamp = 0;
      found = 1;

      // If this is the first process being started, we might need to wake up the scheduler
      // if(p->pid > 1) {  // Skip init process
      //   wakeup1(&ptable.lock);  // Wake up other CPUs if any
      // }
    }
  }
  release(&ptable.lock);
  if(found){
    return 42;
  }
  return -54;
}

void
update_priority(struct proc *p)
{
  // πi(t) = πi(0) − α·Ci(t) + β·Wi(t)
  if(p->waiting_ticks > WAIT_THRESHOLD){
    p->dynamic_priority = MAX_PRIORITY;
  }else{
    p->dynamic_priority = p->base_priority - (ALPHA * p->cpu_ticks) + (BETA * p->waiting_ticks);
  }
  // Ensure priority doesn't go below minimum
  // if(p->dynamic_priority < 1)
  //   p->dynamic_priority = 1;
  if(p->dynamic_priority > MAX_PRIORITY){
    p->dynamic_priority = MAX_PRIORITY;
  }
}

void
show_priorities(void)
{
  struct proc *p;
  
  cprintf("PID\tState\tDyn\tBase\tCPU\tWait\n");
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == 0)
    continue;
    cprintf("%d\t%d\t%d\t%d\t%d\t%d\n", p->pid, p->state, p->dynamic_priority, p->base_priority, p->cpu_ticks, p->waiting_ticks);
  }
    
  release(&ptable.lock);
}
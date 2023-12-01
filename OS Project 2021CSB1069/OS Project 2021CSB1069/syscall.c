#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "trace.h"

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  struct proc *curproc = myproc();

  if(addr >= curproc->sz || addr+4 > curproc->sz)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;
  struct proc *curproc = myproc();

  if(addr >= curproc->sz)
    return -1;
  *pp = (char*)addr;
  ep = (char*)curproc->sz;
  for(s = *pp; s < ep; s++){
    if(*s == 0)
      return s - *pp;
  }
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint((myproc()->tf->esp) + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;
  struct proc *curproc = myproc();
 
  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= curproc->sz || (uint)i+size > curproc->sz)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}

extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
extern int sys_exec(void);
extern int sys_exit(void);
extern int sys_fork(void);
extern int sys_fstat(void);
extern int sys_getpid(void);
extern int sys_kill(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_mknod(void);
extern int sys_open(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_unlink(void);
extern int sys_wait(void);
extern int sys_write(void);
extern int sys_uptime(void);
extern int sys_ps(void);
extern int sys_history(void);
extern int sys_wait2(void);


/*Changes for Q1 start*/

int sys_trace() {
	static int n; // nth 32-bit system call argument.
	argint(0, &n);
	struct proc *curproc = myproc();
	curproc->traced = (n & SYSCALL_TRACE) ? n : 0; // Procedure for trace system call that runs in kernel if flag SYSCALL_TRACE is true
	return 0;
}



/*Changes for Q1 end*/
static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_ps]      sys_ps,
[SYS_history] sys_history,
[SYS_wait2]   sys_wait2,
};

static char *syscall_names[] = {
[SYS_fork]    "fork",
[SYS_exit]    "exit",
[SYS_wait]    "wait",
[SYS_pipe]    "pipe",
[SYS_read]    "read",
[SYS_kill]    "kill",
[SYS_exec]    "exec",
[SYS_fstat]   "fstat",
[SYS_chdir]   "chdir",
[SYS_dup]     "dup",
[SYS_getpid]  "getpid",
[SYS_sbrk]    "sbrk",
[SYS_sleep]   "sleep",
[SYS_uptime]  "uptime",
[SYS_open]    "open",
[SYS_write]   "write",
[SYS_mknod]   "mknod",
[SYS_unlink]  "unlink",
[SYS_link]    "link",
[SYS_mkdir]   "mkdir",
[SYS_close]   "close",
[SYS_trace]   "trace",
[SYS_ps]       "ps",
[SYS_history]  "history",
[SYS_wait2]    "wait2",
};

void
syscall(void)
{
  int num, i;
  struct proc *curproc = myproc();
  
  int is_traced = (curproc->traced & SYSCALL_TRACE);
  
  char procname[16];

  for(i=0; curproc->name[i] != 0; i++) {
    procname[i] = curproc->name[i];
  }
  procname[i] = curproc->name[i];

  num = curproc->tf->eax;
  
  
  /*Handling case 1 when the syscall used is exit() . In that case we do not print return vaue as it gets ended ere only*/
  
  // but this case can never arise as we dont have exit() syscall so we can comment it out as well
  
  //if(num == SYS_exit && is_traced) 
  //{
  //cprintf("System Call %s traced with pid = %d and process name = %s\n",syscall_names[num],curproc->pid,procname);
  //}
  
  
  /*Handling other cases that actially give return value*/
  
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) 
  {
    curproc->tf->eax = syscalls[num](); // making sure that valid system call has been made
    
    if (is_traced) {
      
      cprintf((num == SYS_exec && curproc->tf->eax == 0) ?
        "System Call %s traced with pid = %d and process name = %s\n" :
        "System Call %s traced with pid = %d, process name = %s and return val = %d\n",syscall_names[num], curproc->pid,procname,curproc->tf->eax);
	
// first condition if return val is 0 of the system call exec() and second for all other

/*The following can be commented out to see result in format the ques is asked*/
	
	//cprintf("%d: syscall %s -> %d\n", curproc->pid, syscall_names[num], curproc->tf->eax);
    }
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}



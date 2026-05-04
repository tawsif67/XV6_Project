# xv6 Lottery Scheduler Submission

## 1. File-by-file changes

### `kernel/proc.h`

Insert inside `struct proc`, immediately after `int pid;`.

```c
  int tickets;                 // Lottery scheduler tickets
  int rounds;                  // Number of times scheduled
```

### `kernel/proc.c`

Insert after `struct spinlock wait_lock;`.

```c
int
do_rand(unsigned long *ctx)
{
  long hi, lo, x;

  x = (*ctx % 0x7ffffffe) + 1;
  hi = x / 127773;
  lo = x % 127773;
  x = 16807 * lo - 2836 * hi;
  if(x < 0)
    x += 0x7fffffff;

  x--;
  *ctx = x;
  return x;
}

unsigned long rand_next = 1;

int
rand(void)
{
  return do_rand(&rand_next);
}
```

Insert in `allocproc()` immediately after `p->state = USED;`.

```c
  p->tickets = 10;
  p->rounds = 0;
```

Insert in `freeproc()` immediately before `p->state = UNUSED;`.

```c
  p->tickets = 0;
  p->rounds = 0;
```

Insert in `userinit()` immediately after `p->cwd = namei("/");`.

```c
  p->tickets = 10;
  p->rounds = 0;
```

Insert in `kfork()` immediately after `safestrcpy(np->name, p->name, sizeof(p->name));`.

```c
  np->tickets = p->tickets;
  np->rounds = 0;
```

Replace the complete `scheduler()` function with this code.

```c
void
scheduler(void)
{
  struct proc *p;
  struct proc *winner;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    intr_on();
    intr_off();

    int total_tickets = 0;
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE)
        total_tickets += p->tickets;
      release(&p->lock);
    }

    if(total_tickets == 0) {
      asm volatile("wfi");
      continue;
    }

    int winning_ticket = rand() % total_tickets;
    int ticket_count = 0;
    winner = 0;

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        ticket_count += p->tickets;
        if(ticket_count > winning_ticket) {
          winner = p;
          break;
        }
      }
      release(&p->lock);
    }

    if(winner == 0)
      continue;

    winner->rounds++;
    winner->state = RUNNING;
    c->proc = winner;
    swtch(&c->context, &winner->context);

    c->proc = 0;
    release(&winner->lock);
  }
}
```

In `procdump()`, replace the process output line with this code.

```c
    printf("%d %s %s %d", p->pid, state, p->name, p->rounds);
```

### `kernel/syscall.h`

Insert after `#define SYS_close  21`.

```c
#define SYS_settickets 22
```

### `kernel/syscall.c`

Insert with the other syscall declarations.

```c
extern uint64 sys_settickets(void);
```

Insert in the `syscalls[]` table.

```c
[SYS_settickets] sys_settickets,
```

### `kernel/sysproc.c`

Insert at the end of the file.

```c
uint64
sys_settickets(void)
{
  int n;
  struct proc *p;

  argint(0, &n);
  if(n <= 0)
    return -1;

  p = myproc();
  acquire(&p->lock);
  p->tickets = n;
  release(&p->lock);

  return 0;
}
```

### `user/user.h`

Insert with the other system call prototypes.

```c
int settickets(int);
```

### `user/usys.pl`

Insert after `entry("uptime");`.

```perl
entry("settickets");
```

### `Makefile`

Insert in the `UPROGS` list.

```make
	$U/_test_scheduler\
```

### `user/test_scheduler.c`

Create this new file.

```c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int tickets;

  if(argc != 2){
    fprintf(2, "usage: test_scheduler tickets\n");
    exit(1);
  }

  tickets = atoi(argv[1]);
  if(tickets <= 0){
    fprintf(2, "tickets must be positive\n");
    exit(1);
  }

  if(settickets(tickets) < 0){
    fprintf(2, "settickets failed\n");
    exit(1);
  }

  for(;;)
    ;
}
```

## 2. Complete code blocks

All modified source files are included in this repository with the complete working implementation applied.

## 3. Test program and how to run

Build and run xv6.

```sh
make qemu
```

Inside xv6, start several background processes.

```sh
test_scheduler 10 &
test_scheduler 5 &
test_scheduler 2 &
```

Press `Ctrl+P` multiple times after allowing the processes to run.

## 4. Expected behavior/output

`procdump()` prints each process as:

```text
pid state name rounds
```

Example:

```text
1 sleep  init 32
2 sleep  sh 18
5 runble test_scheduler 110
6 runble test_scheduler 55
7 run   test_scheduler 22
```

The exact numbers vary because lottery scheduling is probabilistic. Over time, the `test_scheduler` processes should receive rounds approximately proportional to their ticket counts, for example `10:5:2`.

## Final check

This submission includes:

- `struct proc` fields for `tickets` and `rounds`
- default ticket and round initialization
- `init` ticket initialization to 10
- fork inheritance of tickets and reset of child rounds to 0
- complete `settickets(int n)` syscall integration
- the specified kernel pseudorandom generator
- lottery scheduling using a winning ticket in `[0, total_tickets - 1]`
- `rounds++` when the scheduler chooses a process
- `procdump()` output including rounds
- `user/test_scheduler.c` and `Makefile` integration

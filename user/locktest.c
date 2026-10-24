// Layer 3 checkpoint: N cloned threads each increment a shared
// counter M times. Without a lock the increment's
// load/add/store isn't atomic, so interleaved threads clobber each
// other's updates and the final count comes up short; with the
// uthread_mutex around it, the final count must be exactly N*M.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread_lock.h"

#define NTHREADS 4
#define ITERS    50000
#define STACKSIZE 4096

struct uthread_mutex lock;
int counter_locked = 0;
int counter_unlocked = 0;

char stacks[NTHREADS][STACKSIZE] __attribute__((aligned(16)));

void
run_locked(int unused)
{
  for (int i = 0; i < ITERS; i++) {
    uthread_mutex_lock(&lock);
    counter_locked = counter_locked + 1;
    uthread_mutex_unlock(&lock);
  }
  exit(0);
}

void
run_unlocked(int unused)
{
  for (int i = 0; i < ITERS; i++) {
    counter_unlocked = counter_unlocked + 1;
  }
  exit(0);
}

// Spawn NTHREADS clones of body (0 == run_locked, 1 == run_unlocked),
// join them all, and return once every thread has exited.
void
spawn_and_join(int which)
{
  int pids[NTHREADS];

  for (int i = 0; i < NTHREADS; i++) {
    int pid = clone(stacks[i] + STACKSIZE);
    if (pid < 0) {
      printf("locktest: clone failed\n");
      exit(1);
    }
    if (pid == 0) {
      if (which == 0)
        run_locked(0);
      else
        run_unlocked(0);
    }
    pids[i] = pid;
  }
  for (int i = 0; i < NTHREADS; i++) {
    if (join(pids[i]) != pids[i]) {
      printf("locktest: join failed\n");
      exit(1);
    }
  }
}

int
main(int argc, char *argv[])
{
  uthread_mutex_init(&lock);

  spawn_and_join(0);
  int want = NTHREADS * ITERS;
  if (counter_locked != want) {
    printf("locktest: with lock, counter_locked=%d, want %d -- mutex is "
           "broken\n",
           counter_locked, want);
    exit(1);
  }
  printf("locktest: with lock, counter == %d as expected\n", counter_locked);

  spawn_and_join(1);
  printf("locktest: without lock, counter == %d (expected %d; a mismatch "
         "demonstrates the race the lock fixes above)\n",
         counter_unlocked, want);

  printf("locktest: ALL TESTS PASSED\n");
  exit(0);
}

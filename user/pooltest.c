// Layer 5 checkpoint: submit many small tasks with an uneven initial
// distribution (every single one lands on worker 0's own deque, so
// the other POOL_WORKERS-1 workers start with nothing), run the
// pool, and check (a) every task ran exactly once and (b) stealing
// actually happened -- i.e. the load really did balance across
// workers rather than worker 0 just grinding through all of it.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/pool.h"

#define NTASKS 2000

int ran[NTASKS];
volatile int sink; // so the compiler can't optimize task()'s busy-loop away

void
task(void *argp)
{
  int idx = (int)(uint64)argp;

  // A little real work, so worker 0 can't drain all 2000 tasks
  // before the other workers even finish starting up -- without
  // this, the run finishes too fast for stealing to have a chance
  // to happen at all. The accumulator has to be volatile: a plain
  // local's final value is a compile-time-constant triangular
  // number, so -O just folds the whole loop away to one store and
  // the "work" vanishes entirely.
  volatile int x = 0;
  for (int i = 0; i < 20000; i++)
    x = x + 1;
  sink = x;

  // Exactly-once check: if the pool ever handed this task to two
  // workers (a stolen task that also got popped, say), this would
  // observably become 2 somewhere.
  __atomic_fetch_add(&ran[idx], 1, __ATOMIC_RELAXED);
}

int
main(int argc, char *argv[])
{
  pool_init();

  // Uneven on purpose: all NTASKS tasks go to worker 0's deque.
  for (int i = 0; i < NTASKS; i++)
    pool_submit(0, task, (void *)(uint64)i);

  pool_start();
  pool_wait();

  for (int i = 0; i < NTASKS; i++) {
    if (ran[i] != 1) {
      printf("pooltest: task %d ran %d times, want exactly 1\n", i, ran[i]);
      exit(1);
    }
  }

  int steals = pool_total_steals();
  printf("pooltest: %d/%d tasks were stolen off worker 0's deque by an "
         "idle worker\n",
         steals, NTASKS);
  if (steals == 0) {
    printf("pooltest: no stealing occurred -- load never balanced across "
           "workers\n");
    exit(1);
  }

  printf("pooltest: ALL TESTS PASSED\n");
  exit(0);
}

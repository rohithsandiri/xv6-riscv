// Work-stealing thread pool built on Layers 1-4: POOL_WORKERS
// threads (Layer 4's thread_create), each spinning on its own
// Chase-Lev deque (deque.c) -- pop its own work first, and only
// steal from a peer when its own deque comes up empty. tasks_left is
// the shared completion signal: every task decrements it when done,
// and a worker stops once it hits 0 and one full round of
// pop+steal-from-everyone comes up empty.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"
#include "user/deque.h"
#include "user/pool.h"

static Deque deques[POOL_WORKERS];
static int tids[POOL_WORKERS];
static int tasks_left;
static int total_steals;

void
pool_init(void)
{
  for (int i = 0; i < POOL_WORKERS; i++)
    deque_init(&deques[i]);
  tasks_left = 0;
  total_steals = 0;
}

int
pool_total_steals(void)
{
  return total_steals;
}

void
pool_submit(int w, void (*fn)(void *), void *arg)
{
  Task t;
  t.fn = fn;
  t.arg = arg;
  deque_push(&deques[w], t);
  __atomic_fetch_add(&tasks_left, 1, __ATOMIC_RELAXED);
}

static void
worker(void *argp)
{
  int id = (int)(uint64)argp;
  Task t;

  for (;;) {
    if (deque_pop(&deques[id], &t)) {
      t.fn(t.arg);
      __atomic_fetch_sub(&tasks_left, 1, __ATOMIC_RELEASE);
      continue;
    }

    int stole = 0;
    for (int i = 1; i < POOL_WORKERS; i++) {
      int victim = (id + i) % POOL_WORKERS;
      if (deque_steal(&deques[victim], &t)) {
        t.fn(t.arg);
        __atomic_fetch_sub(&tasks_left, 1, __ATOMIC_RELEASE);
        __atomic_fetch_add(&total_steals, 1, __ATOMIC_RELAXED);
        stole = 1;
        break;
      }
    }
    if (stole)
      continue;

    // Found nothing in our own deque or anyone else's this round.
    // If the shared counter is already at 0, every task that will
    // ever exist has already run, so there's nothing left to wait
    // for; otherwise some other worker is still mid-task and might
    // (indirectly) create more work for us to steal, so keep trying.
    if (__atomic_load_n(&tasks_left, __ATOMIC_ACQUIRE) == 0)
      break;
  }
  exit(0);
}

void
pool_start(void)
{
  for (int i = 0; i < POOL_WORKERS; i++)
    tids[i] = thread_create(worker, (void *)(uint64)i);
}

void
pool_wait(void)
{
  for (int i = 0; i < POOL_WORKERS; i++)
    thread_join(tids[i]);
}

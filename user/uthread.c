// Thread library: a thin wrapper over the raw clone()/join()
// syscalls so callers (the work-stealing pool, in particular) deal
// in (fn, arg) pairs instead of hand-rolling clone's "returns twice"
// dispatch and stack bookkeeping every time.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"

#define STACKSIZE 4096
#define MAXTHREADS 64

// clone()d threads share our address space, so this table -- like
// any other global -- is visible to all of them; only the parent
// (the thread that calls thread_create/thread_join) ever touches it,
// so it needs no lock of its own.
static struct {
  int tid;
  void *stack;
} live[MAXTHREADS];

static void
record(int tid, void *stack)
{
  for (int i = 0; i < MAXTHREADS; i++) {
    if (live[i].tid == 0) {
      live[i].tid = tid;
      live[i].stack = stack;
      return;
    }
  }
}

static void *
take(int tid)
{
  for (int i = 0; i < MAXTHREADS; i++) {
    if (live[i].tid == tid) {
      live[i].tid = 0;
      return live[i].stack;
    }
  }
  return 0;
}

int
thread_create(void (*fn)(void *), void *arg)
{
  void *stack = malloc(STACKSIZE);
  if (stack == 0)
    return -1;

  int pid = clone((char *)stack + STACKSIZE);
  if (pid < 0) {
    free(stack);
    return -1;
  }
  if (pid == 0) {
    // New thread: run the requested body, then exit -- clone()
    // itself only forked execution onto this stack; it has no idea
    // fn/arg exist.
    fn(arg);
    exit(0);
  }

  record(pid, stack);
  return pid;
}

int
thread_join(int tid)
{
  int r = join(tid);
  if (r < 0)
    return -1;

  void *stack = take(tid);
  if (stack)
    free(stack);

  return r;
}

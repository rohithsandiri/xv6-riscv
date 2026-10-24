// Layer 4 checkpoint: thread_create several workers that each do
// something observable, thread_join them all, and check nothing
// leaked (no error return, and -- loosely -- that a stack-leak bug
// would show up as sbrk's break not returning to roughly where it
// started once every thread is joined and its stack freed).

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/uthread.h"

#define NWORKERS 8

int results[NWORKERS];

void
worker(void *argp)
{
  int id = (int)(uint64)argp;
  results[id] = id * id;
  exit(0);
}

int
main(int argc, char *argv[])
{
  int tids[NWORKERS];

  for (int i = 0; i < NWORKERS; i++) {
    tids[i] = thread_create(worker, (void *)(uint64)i);
    if (tids[i] < 0) {
      printf("uthreadtest: thread_create failed\n");
      exit(1);
    }
  }

  for (int i = 0; i < NWORKERS; i++) {
    if (thread_join(tids[i]) != tids[i]) {
      printf("uthreadtest: thread_join failed\n");
      exit(1);
    }
  }

  for (int i = 0; i < NWORKERS; i++) {
    if (results[i] != i * i) {
      printf("uthreadtest: worker %d result %d, want %d\n", i, results[i],
             i * i);
      exit(1);
    }
  }

  // malloc/free (user/umalloc.c, the classic K&R allocator) never
  // shrinks the heap via sbrk(-n) on free -- it only returns memory
  // to its own free list for reuse, same as a real allocator. So a
  // single create/join cycle growing the break proves nothing. What
  // DOES prove thread_join's free(stack) actually works: after the
  // first cycle primes the free list, repeating the cycle many more
  // times must reuse that same freed memory instead of asking sbrk
  // for more each time -- if thread_join leaked stacks, the break
  // would climb every single cycle instead of holding steady.
  for (int i = 0; i < NWORKERS; i++) {
    int tid = thread_create(worker, (void *)(uint64)i);
    thread_join(tid);
  }
  uint64 sz_before = (uint64)sbrk(0);
  for (int cycle = 0; cycle < 200; cycle++) {
    int tid = thread_create(worker, 0);
    if (tid < 0 || thread_join(tid) != tid) {
      printf("uthreadtest: create/join cycle %d failed\n", cycle);
      exit(1);
    }
  }
  uint64 sz_after = (uint64)sbrk(0);
  if (sz_after != sz_before) {
    printf("uthreadtest: heap break grew from %p to %p over 200 "
           "create/join cycles -- thread_join is leaking stacks\n",
           (void *)sz_before, (void *)sz_after);
    exit(1);
  }

  printf("uthreadtest: ALL TESTS PASSED\n");
  exit(0);
}

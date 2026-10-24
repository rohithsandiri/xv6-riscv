// A minimal user-space spinlock, built entirely on the same atomic
// builtins xv6's own kernel spinlocks (kernel/spinlock.c) use --
// test-and-set needs no kernel help, so this needs no new syscall.

#include "kernel/types.h"
#include "user/uthread_lock.h"

void
uthread_mutex_init(struct uthread_mutex *m)
{
  m->locked = 0;
}

void
uthread_mutex_lock(struct uthread_mutex *m)
{
  // __sync_lock_test_and_set atomically writes 1 and returns the
  // PREVIOUS value. If that was 0, we just flipped it from unlocked
  // to locked ourselves, so the lock is ours; if it was already 1,
  // someone else holds it and we spin and try again. The builtin
  // also acts as an acquire barrier, so no load/store below this
  // point can be reordered before the lock is actually held.
  while (__sync_lock_test_and_set(&m->locked, 1) != 0)
    ; // busy-wait
}

void
uthread_mutex_unlock(struct uthread_mutex *m)
{
  // __sync_lock_release is a release barrier: every write made
  // while holding the lock becomes visible to the next thread that
  // successfully test-and-sets it, before it observes locked==0.
  __sync_lock_release(&m->locked);
}

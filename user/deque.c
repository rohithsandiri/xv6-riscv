// Chase-Lev work-stealing deque (bounded, fixed-size variant --
// no resizing, since DEQUE_SIZE is generous for this project's test
// workloads and resizing correctly under concurrent steals is a
// substantial complication on its own).

#include "kernel/types.h"
#include "user/deque.h"

void
deque_init(Deque *q)
{
  q->top = 0;
  q->bottom = 0;
}

// Only the owner calls this, and only the owner ever moves bottom
// upward, so no CAS is needed here -- the race this whole structure
// exists to handle is between pop() and steal(), not between two
// pushes.
void
deque_push(Deque *q, Task t)
{
  long b = q->bottom;
  q->buf[b % DEQUE_SIZE] = t;
  __atomic_store_n(&q->bottom, b + 1, __ATOMIC_RELEASE);
}

// Owner pops from the bottom (LIFO order, for cache locality on the
// owner's own recently-pushed work).
int
deque_pop(Deque *q, Task *out)
{
  long b = q->bottom - 1;
  __atomic_store_n(&q->bottom, b, __ATOMIC_RELAXED);
  // Full fence: the store to bottom above must be visible to any
  // thief before we read top below, or we could both decide the
  // same single remaining task is ours.
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  long t = __atomic_load_n(&q->top, __ATOMIC_RELAXED);

  if (t > b) {
    // Deque was already empty. Canonical empty state is top==bottom;
    // restore that (not b+1, which would be stale if a thief has
    // since moved top further) before giving up.
    q->bottom = t;
    return 0;
  }

  *out = q->buf[b % DEQUE_SIZE];
  if (t == b) {
    // Exactly one item was left: a thief could be racing us for it.
    // Whoever wins the CAS on top gets it. Either way there's
    // nothing left, so bottom's new canonical empty position is t+1.
    long expect = t;
    int won = __atomic_compare_exchange_n(&q->top, &expect, t + 1, 0,
                                           __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    q->bottom = t + 1;
    return won;
  }
  // t < b: more than one item remains. Leave bottom at b -- that's
  // exactly the point of decrementing it up front, so do NOT restore
  // it to b+1 here, or this pop becomes a no-op that returns the
  // same item forever.
  return 1;
}

// Any worker but the owner steals from the top (FIFO order, so a
// thief takes the victim's OLDEST work, leaving its freshest -- and
// most cache-warm -- work for the owner itself).
int
deque_steal(Deque *q, Task *out)
{
  long t = __atomic_load_n(&q->top, __ATOMIC_ACQUIRE);
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
  long b = __atomic_load_n(&q->bottom, __ATOMIC_ACQUIRE);

  if (t >= b)
    return 0; // empty

  *out = q->buf[t % DEQUE_SIZE];
  long expect = t;
  if (!__atomic_compare_exchange_n(&q->top, &expect, t + 1, 0,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
    return 0; // lost the race to another thief, or to the owner's pop
  return 1;
}

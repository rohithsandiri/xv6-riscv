#ifndef DEQUE_H
#define DEQUE_H

#define DEQUE_SIZE 4096

typedef struct {
  void (*fn)(void *);
  void *arg;
} Task;

// A bounded Chase-Lev work-stealing deque. The owning worker pushes
// and pops its own end (bottom), treating it as a LIFO stack; any
// other worker may steal from the opposite end (top), treating it
// as a FIFO queue. Only pop() and steal() can race each other (two
// threads never call push() on the same deque), and both use a CAS
// on `top` to arbitrate who actually gets the last remaining task.
typedef struct {
  Task buf[DEQUE_SIZE];
  long top;    // next slot a thief steals
  long bottom; // next slot the owner pushes to
} Deque;

void deque_init(Deque *q);
void deque_push(Deque *q, Task t);
int deque_pop(Deque *q, Task *out);   // owner only
int deque_steal(Deque *q, Task *out); // any other worker

#endif

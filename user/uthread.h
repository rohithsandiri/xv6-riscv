#ifndef UTHREAD_H
#define UTHREAD_H

// Spawn a new thread running fn(arg) in the caller's address space.
// Returns the new thread's id (its pid, as far as the kernel is
// concerned) or -1 on failure.
int thread_create(void (*fn)(void *), void *arg);

// Wait for thread tid to finish and reclaim its stack. Returns tid
// on success, -1 on failure.
int thread_join(int tid);

#endif

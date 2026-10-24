#ifndef POOL_H
#define POOL_H

#define POOL_WORKERS 4

// Must be called once before any pool_submit/pool_start.
void pool_init(void);

// Push a task directly onto worker `w`'s own deque. Only safe to
// call before pool_start -- once workers are running, only the
// owning worker thread may push to its own deque.
void pool_submit(int w, void (*fn)(void *), void *arg);

// Spawn POOL_WORKERS threads, each running the steal loop, to drain
// every worker's deque (whatever pool_submit seeded it with).
void pool_start(void);

// Join all worker threads. Returns once every submitted task has
// run exactly once.
void pool_wait(void);

// How many tasks were picked up via deque_steal (as opposed to a
// worker's own deque_pop) across the whole run. Meaningful only
// after pool_wait returns.
int pool_total_steals(void);

#endif

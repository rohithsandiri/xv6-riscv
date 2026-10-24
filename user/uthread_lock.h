#ifndef UTHREAD_LOCK_H
#define UTHREAD_LOCK_H

struct uthread_mutex {
  int locked;
};

void uthread_mutex_init(struct uthread_mutex *m);
void uthread_mutex_lock(struct uthread_mutex *m);
void uthread_mutex_unlock(struct uthread_mutex *m);

#endif

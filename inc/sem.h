#include <pthread.h>

#ifndef __sem
#define __sem
struct Semaphome {
  int value;
  pthread_mutex_t lock;
  pthread_cond_t wait_queue;
};
#endif

void init_sem(struct Semaphome *sem, int value);

void P(struct Semaphome *sem);

void V(struct Semaphome *sem);
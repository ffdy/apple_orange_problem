#include "sem.h"

void initSem(struct Semaphome *sem, int value) {
  sem->value = value;
  pthread_mutex_init(&sem->lock, NULL);
  pthread_cond_init(&sem->waitQueue, NULL);
}

void P(struct Semaphome *sem) {
  pthread_mutex_lock(&sem->lock);
  sem->value--;
  if (sem->value < 0)
    pthread_cond_wait(&sem->waitQueue, &sem->lock);
  pthread_mutex_unlock(&sem->lock);
}

void V(struct Semaphome *sem) {
  pthread_mutex_lock(&sem->lock);
  sem->value++;
  if (sem->value <= 0)
    pthread_cond_signal(&sem->waitQueue);
  pthread_mutex_unlock(&sem->lock);
}

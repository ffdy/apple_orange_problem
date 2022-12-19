#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 信号量
#include "sem.h"
#include "data.h"
#include "proc.h"

// 内存锁
struct Semaphome memLock[N];
// 同步锁
struct Semaphome appleLock[N];
struct Semaphome orangeLock[N];
// 临界区锁
struct Semaphome lock;
// 线程
pthread_t threadAppleProducer[N], threadOrangeProducer[N];
pthread_t threadAppleConsumer[N], threadOrangeConsumer[N];

int id[N];

void *appleProducer(void *arg) {
  int id = *(int *)arg;
  while (1) {
    P(&lock);
    printf("apple producer%d: free\n", id);
    producer_consumer_state[id][0] = 0;
    V(&lock);
    sleep(free_time[id][0]);

    P(&lock);
    printf("apple producer%d: wait memory\n", id);
    producer_consumer_state[id][0] = 1;
    V(&lock);
    P(&memLock[id]);

    P(&lock);
    printf("apple producer%d: start to produce\n", id);
    producer_consumer_state[id][0] = 2;
    mem_state[id] = 1;
    mem_host[id] = id;
    producer_consumer_target[id][0] = id;
    V(&lock);
    sleep(work_time[id][0]);

    P(&lock);
    printf("apple producer%d: done\n", id);
    mem_state[id] = 2;
    V(&lock);
    V(&appleLock[id]);
  }
}

void *orangeProducer(void *arg) {
  int id = *(int *)arg;
  while (1) {
    P(&lock);
    printf("orange producer%d: free\n", id);
    producer_consumer_state[id][1] = 0;
    V(&lock);
    sleep(free_time[id][1]);

    P(&lock);
    printf("orange producer%d: wait memory\n", id);
    producer_consumer_state[id][1] = 1;
    V(&lock);
    P(&memLock[id]);

    P(&lock);
    printf("orange producer%d: start to produce\n", id);
    producer_consumer_state[id][1] = 2;
    mem_state[id] = 3;
    mem_host[id] = id;
    producer_consumer_target[id][1] = id;
    V(&lock);
    sleep(work_time[id][1]);

    P(&lock);
    printf("orange producer%d: done\n", id);
    mem_state[id] = 4;
    V(&lock);
    V(&orangeLock[id]);
  }
}

void *appleConsumer(void *arg) {
  int id = *(int *)arg;
  while (1) {
    P(&lock);
    printf("apple consumer%d: free\n", id);
    producer_consumer_state[id][2] = 0;
    V(&lock);
    sleep(free_time[id][2]);

    P(&lock);
    printf("apple comsumer%d: wait apple\n", id);
    producer_consumer_state[id][2] = 1;
    V(&lock);
    P(&appleLock[id]);

    P(&lock);
    printf("apple consumer%d: start to consume\n", id);
    producer_consumer_state[id][2] = 2;
    mem_host[id] = id;
    producer_consumer_target[id][2] = id;
    V(&lock);
    sleep(work_time[id][2]);

    P(&lock);
    printf("apple consumer%d: done\n", id);
    mem_state[id] = 0;
    V(&lock);
    V(&memLock[id]);
  }
}

void *orangeConsumer(void *arg) {
  int id = *(int *)arg;
  while (1) {
    P(&lock);
    printf("orange consumer%d: free\n", id);
    producer_consumer_state[id][3] = 0;
    V(&lock);
    sleep(free_time[id][3]);

    P(&lock);
    printf("orange comsumer%d: wait orange\n", id);
    producer_consumer_state[id][3] = 1;
    V(&lock);
    P(&orangeLock[id]);

    P(&lock);
    printf("orange consumer%d: start to consume\n", id);
    producer_consumer_state[id][3] = 2;
    mem_host[id] = id;
    producer_consumer_target[id][3] = id;
    V(&lock);
    sleep(work_time[id][3]);

    P(&lock);
    printf("orange consumer%d: done\n", id);
    mem_state[id] = 0;
    V(&lock);
    V(&memLock[id]);
  }
}

void proc_start() {

  // 初始化生产者消费者的生产时间
  for (int i = 0; i < N; i++) {
    work_time[i][0] = rand() % 6 + 1;
    work_time[i][1] = rand() % 6 + 1;
    work_time[i][2] = rand() % 2 + 1;
    work_time[i][3] = rand() % 2 + 1;

    free_time[i][0] = rand() % 2 + 1;
    free_time[i][1] = rand() % 2 + 1;
    free_time[i][2] = rand() % 2 + 1;
    free_time[i][3] = rand() % 2 + 1;
  }

  // 初始化信号量
  initSem(&lock, 1);
  for (int i = 0; i < N; i++) {
    initSem(&memLock[i], 1);
    initSem(&appleLock[i], 0);
    initSem(&orangeLock[i], 0);
  }

  for (int i = 0; i < N; i++) {
    // id[N]不能是局部
    id[i] = i;
    pthread_create(&threadAppleProducer[i], NULL, appleProducer, &id[i]);
    pthread_create(&threadAppleConsumer[i], NULL, appleConsumer, &id[i]);
    pthread_create(&threadOrangeProducer[i], NULL, orangeProducer, &id[i]);
    pthread_create(&threadOrangeConsumer[i], NULL, orangeConsumer, &id[i]);
  }
}

// 销毁线程
void proc_done() {
  for (int i = 0; i < N; i++) {
    pthread_cancel(threadAppleProducer[i]);
    pthread_cancel(threadAppleConsumer[i]);
    pthread_cancel(threadOrangeProducer[i]);
    pthread_cancel(threadOrangeConsumer[i]);
  }
}
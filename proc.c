#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 信号量
#include "sem.h"
#include "data.h"
#include "proc.h"

// 内存锁
struct Semaphome mem_lock[N];
// 同步锁
struct Semaphome apple_lock[N];
struct Semaphome orange_lock[N];
// 临界区锁
struct Semaphome lock;
// 线程
pthread_t thread_apple_producer[N], thread_orange_producer[N];
pthread_t thread_apple_consumer[N], thread_orange_consumer[N];

int self_id[N];

void proc_start() {

  // 初始化生产者消费者的生产时间
  for (int i = 0; i < N; i++) {
    work_time[i][APPLE_PRODUCER] = rand() % 6 + 1;
    work_time[i][ORANGE_PRODUCER] = rand() % 6 + 1;
    work_time[i][APPLE_CONSUMER] = rand() % 2 + 1;
    work_time[i][ORANGE_CONSUMER] = rand() % 2 + 1;

    free_time[i][APPLE_PRODUCER] = rand() % 2 + 1;
    free_time[i][ORANGE_PRODUCER] = rand() % 2 + 1;
    free_time[i][APPLE_CONSUMER] = rand() % 2 + 1;
    free_time[i][ORANGE_CONSUMER] = rand() % 2 + 1;
  }

  // 初始化信号量
  init_sem(&lock, 1);
  for (int i = 0; i < N; i++) {
    init_sem(&mem_lock[i], 1);
    init_sem(&apple_lock[i], 0);
    init_sem(&orange_lock[i], 0);
  }

void Ps(int *memId, int stateCode, int resultCode, int type) {
  while (1) {
    // *memId = (*memId + 1);
    if (type == 0)
      *memId = (*memId + 1) % (N / 2);
    else if (type == 1) {
      *memId = (*memId + 1) % N;
      if (!(*memId))
        *memId += N / 2;
    }
    if (memState[*memId] != stateCode)
      continue;
    P(&lock);
    if (memState[*memId] != stateCode) {
      V(&lock);
      continue;
    }
    if (stateCode == 0) {
      P(&memLock[*memId]);
    } else if (stateCode == 2) {
      P(&appleLock[*memId]);
    } else if (stateCode == 4) {
      P(&orangeLock[*memId]);
    }
    memState[*memId] = resultCode;
    V(&lock);
    break;
  }
}

void *apple_producer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("apple producer%d: free\n", id);
    producer_consumer_state[id][APPLE_PRODUCER] = FREE;
    V(&lock);
    sleep(free_time[id][APPLE_PRODUCER]);

    P(&lock);
    printf("apple producer%d: wait memory\n", id);
    producer_consumer_state[id][APPLE_PRODUCER] = WAITING;
    V(&lock);
    // P(&memLock[id]);
    Ps(&memId, 0, 1);

    P(&lock);
    printf("apple producer%d: start to produce\n", id);
    producer_consumer_state[id][APPLE_PRODUCER] = PRODUCING;
    mem_state[id] = MEM_APPLE_PRODUCE;
    mem_host[id] = id;
    producer_consumer_target[id][APPLE_PRODUCER] = id;
    V(&lock);
    sleep(work_time[id][APPLE_PRODUCER]);

    P(&lock);
    printf("apple producer%d: done\n", id);
    mem_state[id] = MEM_APPLE_WAITING;
    V(&lock);
    V(&apple_lock[id]);
  }
}

void *orange_producer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("orange producer%d: free\n", id);
    producer_consumer_state[id][ORANGE_PRODUCER] = FREE;
    V(&lock);
    sleep(free_time[id][ORANGE_PRODUCER]);

    P(&lock);
    printf("orange producer%d: wait memory\n", id);
    producer_consumer_state[id][ORANGE_PRODUCER] = WAITING;
    V(&lock);
    // P(&memLock[id]);
    Ps(&memId, 0, 3);

    P(&lock);
    printf("orange producer%d: start to produce\n", id);
    producer_consumer_state[id][ORANGE_PRODUCER] = PRODUCING;
    mem_state[id] = MEM_ORANGE_PRODUCE;
    mem_host[id] = id;
    producer_consumer_target[id][ORANGE_PRODUCER] = id;
    V(&lock);
    sleep(work_time[id][ORANGE_PRODUCER]);

    P(&lock);
    printf("orange producer%d: done\n", id);
    mem_state[id] = MEM_ORANGE_WAITING;
    V(&lock);
    V(&orange_lock[id]);
  }
}

void *apple_consumer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("apple consumer%d: free\n", id);
    producer_consumer_state[id][APPLE_CONSUMER] = FREE;
    V(&lock);
    sleep(free_time[id][APPLE_CONSUMER]);

    P(&lock);
    printf("apple comsumer%d: wait apple\n", id);
    producer_consumer_state[id][APPLE_CONSUMER] = WAITING;
    V(&lock);
    // P(&appleLock[id]);
    P(&appleSum);
    Ps(&memId, 2, 5);

    P(&lock);
    printf("apple consumer%d: start to consume\n", id);
    producer_consumer_state[id][APPLE_CONSUMER] = CONSUMING;
    producer_consumer_target[id][APPLE_CONSUMER] = id;
    mem_host[id] = id;
    mem_state[id] = MEM_APPLE_CONSUME;
    V(&lock);
    sleep(work_time[id][APPLE_CONSUMER]);

    P(&lock);
    printf("apple consumer%d: done\n", id);
    mem_state[id] = FREE;
    V(&lock);
    V(&mem_lock[id]);
  }
}

void *orange_consumer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("orange consumer%d: free\n", id);
    producer_consumer_state[id][ORANGE_CONSUMER] = FREE;
    V(&lock);
    sleep(free_time[id][ORANGE_CONSUMER]);

    P(&lock);
    printf("orange comsumer%d: wait orange\n", id);
    producer_consumer_state[id][ORANGE_CONSUMER] = WAITING;
    V(&lock);
    // P(&orangeLock[id]);
    P(&orangeSum);
    Ps(&memId, 4, 6);

    P(&lock);
    printf("orange consumer%d: start to consume\n", id);
    producer_consumer_state[id][ORANGE_CONSUMER] = CONSUMING;
    producer_consumer_target[id][ORANGE_CONSUMER] = id;
    mem_host[id] = id;
    mem_state[id] = MEM_ORANGE_CONSUME;
    V(&lock);
    sleep(work_time[id][ORANGE_CONSUMER]);

    P(&lock);
    printf("orange consumer%d: done\n", id);
    mem_state[id] = FREE;
    V(&lock);
    V(&mem_lock[id]);
  }
}
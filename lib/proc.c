#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG

// 信号量
#include "sem.h"
#include "data.h"
#include "proc.h"

// 内存锁
struct Semaphome mem_lock[N];
// 同步锁
struct Semaphome apple_lock[N], apple_sum;
struct Semaphome orange_lock[N], orange_sum;
// 临界区锁
struct Semaphome lock;
// 线程
pthread_t thread_apple_producer[N], thread_orange_producer[N];
pthread_t thread_apple_consumer[N], thread_orange_consumer[N];

int self_id[N];

void proc_start() {

  // 初始化生产者消费者的生产时间
  for (int i = 0; i < N; i++) {
    work_time[i][APPLE_PRODUCER] = rand() % 6 + 2;
    work_time[i][ORANGE_PRODUCER] = rand() % 6 + 2;
    work_time[i][APPLE_CONSUMER] = rand() % 6 + 2;
    work_time[i][ORANGE_CONSUMER] = rand() % 6 + 2;

    free_time[i][APPLE_PRODUCER] = rand() % 2 + 2;
    free_time[i][ORANGE_PRODUCER] = rand() % 2 + 2;
    free_time[i][APPLE_CONSUMER] = rand() % 2 + 2;
    free_time[i][ORANGE_CONSUMER] = rand() % 2 + 2;
  }

  // 初始化信号量
  init_sem(&lock, 1);
  for (int i = 0; i < N; i++) {
    init_sem(&mem_lock[i], 1);
    init_sem(&apple_lock[i], 0);
    init_sem(&orange_lock[i], 0);
  }
  init_sem(&apple_sum, 0);
  init_sem(&orange_sum, 0);

  for (int i = 0; i < N; i++) {
    // self_id[N]不能是局部
    self_id[i] = i;
    pthread_create(&thread_apple_producer[i], NULL, apple_producer, &self_id[i]);
    pthread_create(&thread_apple_consumer[i], NULL, apple_consumer, &self_id[i]);
    pthread_create(&thread_orange_producer[i], NULL, orange_producer,
                   &self_id[i]);
    pthread_create(&thread_orange_consumer[i], NULL, orange_consumer,
                   &self_id[i]);
  }
}

// 销毁线程
void proc_done() {
  for (int i = 0; i < N; i++) {
    pthread_cancel(thread_apple_producer[i]);
    pthread_cancel(thread_apple_consumer[i]);
    pthread_cancel(thread_orange_producer[i]);
    pthread_cancel(thread_orange_consumer[i]);
  }
}

void find_and_lock_mem(int producer_consumer_id, int *mem_id, int expect_state,
                       int result_state) {
  while (1) {
    *mem_id = (*mem_id + 1) % N;
    if (mem_state[*mem_id] != expect_state)
      continue;
    P(&lock);
    if (mem_state[*mem_id] != expect_state) {
      V(&lock);
      continue;
    }
    if (expect_state == MEM_FREE) {
      P(&mem_lock[*mem_id]);
    } else if (expect_state == MEM_APPLE_WAITING) {
      P(&apple_lock[*mem_id]);
    } else if (expect_state == MEM_ORANGE_WAITING) {
      P(&orange_lock[*mem_id]);
    }
    mem_state[*mem_id] = result_state;
    mem_host[*mem_id] = producer_consumer_id;
    V(&lock);
    break;
  }
}

void *apple_producer(void *arg) {
  int id = *(int *)arg;
  int mem_id = id - 1;
  while (1) {
    P(&lock);
    #ifdef DEBUG
    printf("apple producer%d: free\n", id);
    #endif
    producer_consumer_state[id][APPLE_PRODUCER] = FREE;
    V(&lock);
    sleep(free_time[id][APPLE_PRODUCER]);

    P(&lock);
    #ifdef DEBUG
    printf("apple producer%d: wait memory\n", id);
    #endif
    producer_consumer_state[id][APPLE_PRODUCER] = WAITING;
    V(&lock);
    find_and_lock_mem(id, &mem_id, MEM_FREE, MEM_APPLE_PRODUCE);

    P(&lock);
    #ifdef DEBUG
    printf("apple producer%d: start to produce in mem%d\n", id, mem_id);
    #endif
    producer_consumer_state[id][APPLE_PRODUCER] = PRODUCING;
    producer_consumer_target[id][APPLE_PRODUCER] = mem_id;
    V(&lock);
    sleep(work_time[id][APPLE_PRODUCER]);

    P(&lock);
    #ifdef DEBUG
    printf("apple producer%d: done\n", id);
    #endif
    mem_state[mem_id] = MEM_APPLE_WAITING;
    V(&lock);
    V(&apple_lock[mem_id]);
    // 总苹果数加一
    V(&apple_sum);
  }
}

void *orange_producer(void *arg) {
  int id = *(int *)arg;
  int mem_id = id - 1;
  while (1) {
    P(&lock);
    #ifdef DEBUG
    printf("orange producer%d: free\n", id);
    #endif
    producer_consumer_state[id][ORANGE_PRODUCER] = FREE;
    V(&lock);
    sleep(free_time[id][ORANGE_PRODUCER]);

    P(&lock);
    #ifdef DEBUG
    printf("orange producer%d: wait memory\n", id);
    #endif
    producer_consumer_state[id][ORANGE_PRODUCER] = WAITING;
    V(&lock);
    find_and_lock_mem(id, &mem_id, MEM_FREE, MEM_ORANGE_PRODUCE);

    P(&lock);
    #ifdef DEBUG
    printf("orange producer%d: start to produce in mem%d\n", id, mem_id);
    #endif
    producer_consumer_state[id][ORANGE_PRODUCER] = PRODUCING;
    producer_consumer_target[id][ORANGE_PRODUCER] = mem_id;
    V(&lock);
    sleep(work_time[id][ORANGE_PRODUCER]);

    P(&lock);
    #ifdef DEBUG
    printf("orange producer%d: done\n", id);
    #endif
    mem_state[mem_id] = MEM_ORANGE_WAITING;
    V(&lock);
    V(&orange_lock[mem_id]);
    V(&orange_sum);
  }
}

void *apple_consumer(void *arg) {
  int id = *(int *)arg;
  int mem_id = id - 1;
  while (1) {
    P(&lock);
    #ifdef DEBUG
    printf("apple consumer%d: free\n", id);
    #endif
    producer_consumer_state[id][APPLE_CONSUMER] = FREE;
    V(&lock);
    sleep(free_time[id][APPLE_CONSUMER]);

    P(&lock);
    #ifdef DEBUG
    printf("apple comsumer%d: wait apple\n", id);
    #endif
    producer_consumer_state[id][APPLE_CONSUMER] = WAITING;
    V(&lock);
    P(&apple_sum);
    find_and_lock_mem(id, &mem_id, MEM_APPLE_WAITING, MEM_APPLE_CONSUME);

    P(&lock);
    #ifdef DEBUG
    printf("apple consumer%d: start to consume in mem%d\n", id, mem_id);
    #endif
    producer_consumer_state[id][APPLE_CONSUMER] = CONSUMING;
    producer_consumer_target[id][APPLE_CONSUMER] = mem_id;
    V(&lock);
    sleep(work_time[id][APPLE_CONSUMER]);

    P(&lock);
    #ifdef DEBUG
    printf("apple consumer%d: done\n", id);
    #endif
    mem_state[mem_id] = MEM_FREE;
    V(&lock);
    V(&mem_lock[mem_id]);
  }
}

void *orange_consumer(void *arg) {
  int id = *(int *)arg;
  int mem_id = id - 1;
  while (1) {
    P(&lock);
    #ifdef DEBUG
    printf("orange consumer%d: free\n", id);
    #endif
    producer_consumer_state[id][ORANGE_CONSUMER] = FREE;
    V(&lock);
    sleep(free_time[id][ORANGE_CONSUMER]);

    P(&lock);
    #ifdef DEBUG
    printf("orange comsumer%d: wait orange\n", id);
    #endif
    producer_consumer_state[id][ORANGE_CONSUMER] = WAITING;
    V(&lock);
    // P(&orange_lock[id]);
    P(&orange_sum);
    find_and_lock_mem(id, &mem_id, MEM_ORANGE_WAITING, MEM_ORANGE_CONSUME);

    P(&lock);
    #ifdef DEBUG
    printf("orange consumer%d: start to consume in mem%d\n", id, mem_id);
    #endif
    producer_consumer_state[id][ORANGE_CONSUMER] = CONSUMING;
    producer_consumer_target[id][ORANGE_CONSUMER] = mem_id;
    V(&lock);
    sleep(work_time[id][ORANGE_CONSUMER]);

    P(&lock);
    #ifdef DEBUG
    printf("orange consumer%d: done\n", id);
    #endif
    mem_state[mem_id] = MEM_FREE;
    V(&lock);
    V(&mem_lock[mem_id]);
  }
}
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
    work_time[i][APPLE_CONSUMER] = rand() % 2 + 2;
    work_time[i][ORANGE_CONSUMER] = rand() % 2 + 2;

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
    pthread_create(&thread_orange_producer[i], NULL, orange_producer, &self_id[i]);
    pthread_create(&thread_orange_consumer[i], NULL, orange_consumer, &self_id[i]);
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

// 记录两类生产消费者所占用的内存的边界
int apple_mem_border_id = 0, orange_mem_border_id = N - 1;

void find_and_lock_mem(int *mem_id, int expect_state, int result_state, int type) {
  *mem_id = (type ? N : -1);
  while (1) {
    if (type == 0) { // apple
      // *mem_id = (*mem_id + 1) % (apple_mem_border_id + 1);
      (*mem_id)++;
      if (*mem_id > apple_mem_border_id)
        *mem_id = 0;
      if (mem_state[*mem_id] != expect_state)
        continue;
      P(&lock);
      if (mem_state[*mem_id] != expect_state) {
        V(&lock);
        continue;
      }
      if (expect_state == 0) {
        P(&mem_lock[*mem_id]);
        if ((*mem_id == apple_mem_border_id) && apple_mem_border_id < N - 1 && apple_mem_border_id < orange_mem_border_id)
          apple_mem_border_id++;
      } else if (expect_state == 2) {
        P(&apple_lock[*mem_id]);
        // if ((*mem_id == apple_mem_border_id) && apple_mem_border_id)
        //   apple_mem_border_id--;
      }
    } else if (type == 1) { // orange
      (*mem_id)--;
      if (*mem_id < orange_mem_border_id)
        *mem_id = N - 1;
      if (mem_state[*mem_id] != expect_state)
        continue;
      P(&lock);
      if (mem_state[*mem_id] != expect_state) {
        V(&lock);
        continue;
      }
      if (expect_state == 0) {
        P(&mem_lock[*mem_id]);
        if ((*mem_id == orange_mem_border_id) && orange_mem_border_id && apple_mem_border_id < orange_mem_border_id)
          orange_mem_border_id--;
      } else if (expect_state == 4) {
        P(&orange_lock[*mem_id]);
        // if ((*mem_id == orange_mem_border_id) && orange_mem_border_id < N - 1)
        //   orange_mem_border_id++;
      }
    }
    mem_state[*mem_id] = result_state;
    V(&lock);
    printf("%d %d\n", apple_mem_border_id, orange_mem_border_id);
    break;
  }
}

void *apple_producer(void *arg) {
  int id = *(int *)arg;
  int mem_id = -1;
  while (1) {
    P(&lock);
    printf("apple producer%d: free\n", id);
    producer_consumer_state[id][0] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("apple producer%d: wait memory\n", id);
    producer_consumer_state[id][0] = 1;
    V(&lock);
    // P(&mem_lock[id]);
    find_and_lock_mem(&mem_id, 0, 1, 0);

    P(&lock);
    printf("apple producer%d: start to produce in mem%d\n", id, mem_id);
    producer_consumer_state[id][0] = 2;
    // mem_state[id] = 1;
    V(&lock);
    sleep(work_time[id][0]);

    P(&lock);
    printf("apple producer%d: done\n", id);
    mem_state[mem_id] = 2;
    V(&lock);
    V(&apple_lock[mem_id]);
    V(&apple_sum);
  }
}

void *orange_producer(void *arg) {
  int id = *(int *)arg;
  int mem_id = N / 2 - 1;
  while (1) {
    P(&lock);
    printf("orange producer%d: free\n", id);
    producer_consumer_state[id][1] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("orange producer%d: wait memory\n", id);
    producer_consumer_state[id][1] = 1;
    V(&lock);
    // P(&mem_lock[id]);
    find_and_lock_mem(&mem_id, 0, 3, 1);

    P(&lock);
    printf("orange producer%d: start to produce in mem%d\n", id, mem_id);
    producer_consumer_state[id][1] = 2;
    // mem_state[mem_id] = 3;
    V(&lock);
    sleep(work_time[id][1]);

    P(&lock);
    printf("orange producer%d: done\n", id);
    mem_state[mem_id] = 4;
    V(&lock);
    V(&orange_lock[mem_id]);
    V(&orange_sum);
  }
}

void *apple_consumer(void *arg) {
  int id = *(int *)arg;
  int mem_id = -1;
  while (1) {
    P(&lock);
    printf("apple consumer%d: free\n", id);
    producer_consumer_state[id][2] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("apple comsumer%d: wait apple\n", id);
    producer_consumer_state[id][2] = 1;
    V(&lock);
    // P(&apple_lock[id]);
    P(&apple_sum);
    find_and_lock_mem(&mem_id, 2, 5, 0);

    P(&lock);
    printf("apple consumer%d: start to consume in mem%d\n", id, mem_id);
    producer_consumer_state[id][2] = 2;
    V(&lock);
    sleep(work_time[id][2]);

    P(&lock);
    printf("apple consumer%d: done\n", id);
    mem_state[mem_id] = 0;
    if ((mem_id == apple_mem_border_id) && apple_mem_border_id && mem_state[mem_id - 1] == 0)
      apple_mem_border_id--;
    V(&lock);
    V(&mem_lock[mem_id]);
  }
}

void *orange_consumer(void *arg) {
  int id = *(int *)arg;
  int mem_id = N / 2 - 1;
  while (1) {
    P(&lock);
    printf("orange consumer%d: free\n", id);
    producer_consumer_state[id][3] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("orange comsumer%d: wait orange\n", id);
    producer_consumer_state[id][3] = 1;
    V(&lock);
    // P(&orange_lock[id]);
    P(&orange_sum);
    find_and_lock_mem(&mem_id, 4, 6, 1);

    P(&lock);
    printf("orange consumer%d: start to consume in mem%d\n", id, mem_id);
    producer_consumer_state[id][3] = 2;
    V(&lock);
    sleep(work_time[id][3]);

    P(&lock);
    printf("orange consumer%d: done\n", id);
    mem_state[mem_id] = 0;
    if ((mem_id == orange_mem_border_id) && orange_mem_border_id < N - 1 && mem_state[mem_id + 1] == 0)
      orange_mem_border_id++;
    V(&lock);
    V(&mem_lock[mem_id]);
  }
}
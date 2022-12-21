#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 信号量
#include "sem.h"
#include "list.h"
#include "data.h"
#include "proc.h"

// 临界区锁
struct Semaphome lock;
// 线程
pthread_t thread_apple_producer[N], thread_orange_producer[N];
pthread_t thread_apple_consumer[N], thread_orange_consumer[N];

// 内存管理器的链表
struct List free_mem_list, apple_mem_list, orange_mem_list;
struct Node mem_list_node[N];

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

  // 初始化内存管理器
  init_list(&free_mem_list);
  init_list(&apple_mem_list);
  init_list(&orange_mem_list);

  // 初始化链表使用的节点
  for (int i = 0; i < N; i++) {
    mem_list_node[i].mem_id = i;
    mem_list_node[i].tail = NULL;
    V_list(&free_mem_list, &mem_list_node[i]);
  }

  for (int i = 0; i < N; i++) {
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

void *apple_producer(void *arg) {
  int id = *(int *)arg;
  int mem_id;
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
    P_list(&free_mem_list, &mem_id);

    P(&lock);
    printf("apple producer%d: start to produce in mem%d\n", id, mem_id);
    producer_consumer_state[id][APPLE_PRODUCER] = PRODUCING;
    producer_consumer_target[id][APPLE_PRODUCER] = mem_id;
    mem_state[mem_id] = MEM_APPLE_PRODUCE;
    mem_host[mem_id] = id;
    V(&lock);
    sleep(work_time[id][APPLE_PRODUCER]);

    P(&lock);
    printf("apple producer%d: done\n", id);
    mem_state[mem_id] = MEM_APPLE_WAITING;
    V(&lock);
    V_list(&apple_mem_list, &mem_list_node[mem_id]);
  }
}

void *orange_producer(void *arg) {
  int id = *(int *)arg;
  int mem_id;
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
    P_list(&free_mem_list, &mem_id);

    P(&lock);
    printf("orange producer%d: start to produce in mem%d\n", id, mem_id);
    producer_consumer_state[id][ORANGE_PRODUCER] = CONSUMING;
    producer_consumer_target[id][ORANGE_PRODUCER] = mem_id;
    mem_state[mem_id] = MEM_ORANGE_PRODUCE;
    mem_host[mem_id] = id;
    V(&lock);
    sleep(work_time[id][ORANGE_PRODUCER]);

    P(&lock);
    printf("orange producer%d: done\n", id);
    mem_state[mem_id] = MEM_ORANGE_WAITING;
    V(&lock);
    V_list(&orange_mem_list, &mem_list_node[mem_id]);
  }
}

void *apple_consumer(void *arg) {
  int id = *(int *)arg;
  int mem_id;
  while (1) {
    P(&lock);
    printf("apple consumer%d: free\n", id);
    producer_consumer_state[id][APPLE_CONSUMER] = FREE;
    V(&lock);
    sleep(free_time[id][ORANGE_PRODUCER]);

    P(&lock);
    printf("apple comsumer%d: wait apple\n", id);
    producer_consumer_state[id][APPLE_CONSUMER] = WAITING;
    V(&lock);
    P_list(&apple_mem_list, &mem_id);

    P(&lock);
    printf("apple consumer%d: start to consume in mem%d\n", id, mem_id);
    producer_consumer_state[id][APPLE_CONSUMER] = CONSUMING;
    producer_consumer_target[id][APPLE_CONSUMER] = mem_id;
    mem_state[mem_id] = MEM_APPLE_CONSUME;
    mem_host[mem_id] = id;
    V(&lock);
    sleep(work_time[id][APPLE_CONSUMER]);

    P(&lock);
    printf("apple consumer%d: done\n", id);
    mem_state[mem_id] = MEM_FREE;
    V(&lock);
    V_list(&free_mem_list, &mem_list_node[mem_id]);
  }
}

void *orange_consumer(void *arg) {
  int id = *(int *)arg;
  int mem_id;
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
    P_list(&orange_mem_list, &mem_id);

    P(&lock);
    printf("orange consumer%d: start to consume in mem%d\n", id, mem_id);
    producer_consumer_state[id][ORANGE_CONSUMER] = CONSUMING;
    producer_consumer_target[id][ORANGE_CONSUMER] = mem_id;
    mem_state[mem_id] = MEM_ORANGE_CONSUME;
    mem_host[mem_id] = id;
    V(&lock);
    sleep(work_time[id][ORANGE_CONSUMER]);

    P(&lock);
    printf("orange consumer%d: done\n", id);
    mem_state[mem_id] = MEM_FREE;
    V(&lock);
    V_list(&free_mem_list, &mem_list_node[mem_id]);
  }
}
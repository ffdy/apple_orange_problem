#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 信号量
#include "lib/sem.h"
#include "lib/list.h"
#include "data.h"
#include "proc.h"

// 临界区锁
struct Semaphome lock;
// 线程
pthread_t threadAppleProducer[N], threadOrangeProducer[N];
pthread_t threadAppleConsumer[N], threadOrangeConsumer[N];

// 内存管理器的链表
struct List freeMem, appleMem, orangeMem;
struct Node memNode[N];

int id[N];

void *appleProducer(void *arg) {
  int id = *(int *)arg;
  int memId;
  while (1) {
    P(&lock);
    printf("apple producer%d: free\n", id);
    pcState[id][0] = 0;
    printf("apple producer%d: wait memory\n", id);
    pcState[id][0] = 1;
    V(&lock);
    PList(&freeMem, &memId);

    P(&lock);
    printf("apple producer%d: start to produce in mem%d\n", id, memId);
    pcState[id][0] = 2;
    memState[memId] = 1;
    V(&lock);
    sleep(workTime[id][0]);

    P(&lock);
    printf("apple producer%d: done\n", id);
    memState[memId] = 2;
    V(&lock);
    VList(&appleMem, &memNode[memId]);
  }
}

void *orangeProducer(void *arg) {
  int id = *(int *)arg;
  int memId;
  while (1) {
    P(&lock);
    printf("orange producer%d: free\n", id);
    pcState[id][1] = 0;
    printf("orange producer%d: wait memory\n", id);
    pcState[id][1] = 1;
    V(&lock);
    PList(&freeMem, &memId);

    P(&lock);
    printf("orange producer%d: start to produce in mem%d\n", id, memId);
    pcState[id][1] = 2;
    memState[memId] = 3;
    V(&lock);
    sleep(workTime[id][1]);

    P(&lock);
    printf("orange producer%d: done\n", id);
    memState[memId] = 4;
    V(&lock);
    VList(&orangeMem, &memNode[memId]);
  }
}

void *appleConsumer(void *arg) {
  int id = *(int *)arg;
  int memId;
  while (1) {
    P(&lock);
    printf("apple consumer%d: free\n", id);
    pcState[id][2] = 0;
    printf("apple comsumer%d: wait apple\n", id);
    pcState[id][2] = 1;
    V(&lock);
    PList(&appleMem, &memId);

    P(&lock);
    printf("apple consumer%d: start to consume in mem%d\n", id, memId);
    pcState[id][2] = 2;
    memState[memId] = 5;
    V(&lock);
    sleep(workTime[id][2]);

    P(&lock);
    printf("apple consumer%d: done\n", id);
    memState[memId] = 0;
    V(&lock);
    VList(&freeMem, &memNode[memId]);
  }
}

void *orangeConsumer(void *arg) {
  int id = *(int *)arg;
  int memId;
  while (1) {
    P(&lock);
    printf("orange consumer%d: free\n", id);
    pcState[id][3] = 0;
    printf("orange comsumer%d: wait orange\n", id);
    pcState[id][3] = 1;
    V(&lock);
    PList(&orangeMem, &memId);

    P(&lock);
    printf("orange consumer%d: start to consume in mem%d\n", id, memId);
    pcState[id][3] = 2;
    memState[memId] = 6;
    V(&lock);
    sleep(workTime[id][3]);

    P(&lock);
    printf("orange consumer%d: done\n", id);
    memState[memId] = 0;
    V(&lock);
    VList(&freeMem, &memNode[memId]);
  }
}

void proc_start() {

  // 初始化生产者消费者的生产时间
  for (int i = 0; i < N; i++) {
    workTime[i][0] = rand() % 2 + 2;
    workTime[i][1] = rand() % 2 + 2;
    workTime[i][2] = rand() % 2 + 2;
    workTime[i][3] = rand() % 2 + 2;
  }

  // 初始化信号量
  initSem(&lock, 1);
  // for (int i = 0; i < N; i++) {
  //   initSem(&memLock[i], 1);
  //   initSem(&appleLock[i], 0);
  //   initSem(&orangeLock[i], 0);
  // }

  // 初始化内存管理器
  initList(&freeMem);
  initList(&appleMem);
  initList(&orangeMem);
  for (int i = 0; i < N; i++) {
    memNode[i].memId = i;
    memNode[i].tail = NULL;
    VList(&freeMem, &memNode[i]);
  }

  // ShowList(&freeMem, "fM");
  // ShowList(&appleMem, "aM");
  // ShowList(&orangeMem, "oM");

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
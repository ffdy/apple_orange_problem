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

// 内存锁
struct Semaphome memLock[N];
// 同步锁
struct Semaphome appleLock[N], appleSum;
struct Semaphome orangeLock[N], orangeSum;
// 临界区锁
struct Semaphome lock;
// 线程
pthread_t threadAppleProducer[N], threadOrangeProducer[N];
pthread_t threadAppleConsumer[N], threadOrangeConsumer[N];

// 内存管理器的链表
struct List freeMem, appleMem, orangeMem;
struct Node memNode[N];

int id[N];

void Ps(int *memId, int stateCode, int resultCode) {
  while (1) {
    // printf("find %d\n", stateCode);
    *memId = (*memId + 1) % N;
    if (memState[*memId] != stateCode)
      continue;
    P(&lock);
    // printf("get lock\n");
    if (memState[*memId] != stateCode) {
      V(&lock);
      // printf("free lock1\n");
      continue;
    }
    if (stateCode == 0) {
      // printf("\ttype 0 %d\n", memState[*memState]);
      P(&memLock[*memId]);
      // printf("\tget 0\n");
    } else if (stateCode == 2) {
      // printf("\ttype 2\n");
      P(&appleLock[*memId]);
      // printf("\tget 2\n");
    } else if (stateCode == 4) {
      // printf("\ttype 4\n");
      P(&orangeLock[*memId]);
      // printf("\tget 4\n");
    }
    memState[*memId] = resultCode;
    V(&lock);
    // printf("free lock2\n");
    break;
  }
}

void *appleProducer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("apple producer%d: free\n", id);
    pcState[id][0] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("apple producer%d: wait memory\n", id);
    pcState[id][0] = 1;
    V(&lock);
    // P(&memLock[id]);
    Ps(&memId, 0, 1);

    P(&lock);
    printf("apple producer%d: start to produce in mem%d\n", id, memId);
    pcState[id][0] = 2;
    // memState[id] = 1;
    V(&lock);
    sleep(workTime[id][0]);

    P(&lock);
    printf("apple producer%d: done\n", id);
    memState[memId] = 2;
    V(&lock);
    V(&appleLock[memId]);
    V(&appleSum);
  }
}

void *orangeProducer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("orange producer%d: free\n", id);
    pcState[id][1] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("orange producer%d: wait memory\n", id);
    pcState[id][1] = 1;
    V(&lock);
    // P(&memLock[id]);
    Ps(&memId, 0, 3);

    P(&lock);
    printf("orange producer%d: start to produce in mem%d\n", id, memId);
    pcState[id][1] = 2;
    // memState[memId] = 3;
    V(&lock);
    sleep(workTime[id][1]);

    P(&lock);
    printf("orange producer%d: done\n", id);
    memState[memId] = 4;
    V(&lock);
    V(&orangeLock[memId]);
    V(&orangeSum);
  }
}

void *appleConsumer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("apple consumer%d: free\n", id);
    pcState[id][2] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("apple comsumer%d: wait apple\n", id);
    pcState[id][2] = 1;
    V(&lock);
    // P(&appleLock[id]);
    P(&appleSum);
    Ps(&memId, 2, 5);

    P(&lock);
    printf("apple consumer%d: start to consume in mem%d\n", id, memId);
    pcState[id][2] = 2;
    V(&lock);
    sleep(workTime[id][2]);

    P(&lock);
    printf("apple consumer%d: done\n", id);
    memState[memId] = 0;
    V(&lock);
    V(&memLock[memId]);
  }
}

void *orangeConsumer(void *arg) {
  int id = *(int *)arg;
  int memId = id - 1;
  while (1) {
    P(&lock);
    printf("orange consumer%d: free\n", id);
    pcState[id][3] = 0;
    V(&lock);
    sleep(rand() % 5 + 3);

    P(&lock);
    printf("orange comsumer%d: wait orange\n", id);
    pcState[id][3] = 1;
    V(&lock);
    // P(&orangeLock[id]);
    P(&orangeSum);
    Ps(&memId, 4, 6);

    P(&lock);
    printf("orange consumer%d: start to consume in mem%d\n", id, memId);
    pcState[id][3] = 2;
    V(&lock);
    sleep(workTime[id][3]);

    P(&lock);
    printf("orange consumer%d: done\n", id);
    memState[memId] = 0;
    V(&lock);
    V(&memLock[memId]);
  }
}

void proc_start() {

  // 初始化生产者消费者的生产时间
  for (int i = 0; i < N; i++) {
    workTime[i][0] = rand() % 6 + 30;
    workTime[i][1] = rand() % 6 + 30;
    workTime[i][2] = rand() % 2 + 4;
    workTime[i][3] = rand() % 2 + 4;
  }

  // 初始化信号量
  initSem(&lock, 1);
  for (int i = 0; i < N; i++) {
    initSem(&memLock[i], 1);
    initSem(&appleLock[i], 0);
    initSem(&orangeLock[i], 0);
  }
  initSem(&appleSum, 0);
  initSem(&orangeSum, 0);

  // 初始化内存管理器
  initList(&freeMem);
  initList(&appleMem);
  initList(&orangeMem);
  for (int i = 0; i < N; i++) {
    memNode[i].memId = i;
    memNode[i].tail = NULL;
    VList(&freeMem, &memNode[i]);
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
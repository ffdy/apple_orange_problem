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
    ShowList(&freeMem, "afM1", id, memId);
    PList(&freeMem, &memId);
    ShowList(&freeMem, "afM2", id, memId);

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
    ShowList(&appleMem, "aaM1", id, memId);
    VList(&appleMem, memId);
    ShowList(&appleMem, "aaM2", id, memId);
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
    ShowList(&freeMem, "ofM1", id, memId);
    PList(&freeMem, &memId);
    ShowList(&freeMem, "ofM2", id, memId);

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
    ShowList(&orangeMem, "ooM1", id, memId);
    VList(&orangeMem, memId);
    ShowList(&orangeMem, "ooM2", id, memId);
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
    ShowList(&appleMem, "acaM1", id, memId);
    PList(&appleMem, &memId);
    ShowList(&appleMem, "acaM2", id, memId);

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
    ShowList(&freeMem, "acfM1", id, memId);
    VList(&freeMem, memId);
    ShowList(&freeMem, "acfM2", id, memId);
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
    ShowList(&orangeMem, "ocoM1", id, memId);
    PList(&orangeMem, &memId);
    ShowList(&orangeMem, "ocoM2", id, memId);

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
    ShowList(&freeMem, "ocfM1", id, memId);
    VList(&freeMem, memId);
    ShowList(&freeMem, "ocfM2", id, memId);
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
    VList(&freeMem, i);
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
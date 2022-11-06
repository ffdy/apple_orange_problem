#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 信号量
#include "lib/sem.h"
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

int id[N];

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

void *appleProducer(void *arg) {
  int id = *(int *)arg;
  int memId = -1;
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
    Ps(&memId, 0, 1, 0);

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
  int memId = N / 2 - 1;
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
    Ps(&memId, 0, 3, 1);

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
  int memId = -1;
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
    Ps(&memId, 2, 5, 0);

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
  int memId = N / 2 - 1;
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
    Ps(&memId, 4, 6, 1);

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
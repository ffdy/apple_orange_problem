#include "sem.h"

#ifndef __safe_list
#define __safe_list
struct Node {
  int memId;
  struct Node *tail;
};

struct List {
  struct Semaphome lock;
  struct Semaphome memSum;
  struct Node head;
  struct Node *tail;
};
#endif

void initList(struct List *);
void PList(struct List *, int *);
void VList(struct List *, struct Node *);
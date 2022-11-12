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

void initList(struct List *list);
void PList(struct List *list, int *elem);
void VList(struct List *list, struct Node *elem);
void ShowList(struct List *list, char *tag, int id, int memId);
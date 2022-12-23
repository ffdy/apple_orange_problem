#include "sem.h"

#ifndef __safe_list
#define __safe_list
struct Node {
  int mem_id;
  struct Node *tail;
};

struct List {
  struct Semaphome lock;
  struct Semaphome mem_sum;
  struct Node head;
  struct Node *tail;
};
#endif

void init_list(struct List *list);
void P_list(struct List *list, int *elem);
void V_list(struct List *list, struct Node *elem);
void show_list(struct List *list, char *tag, int id, int mem_id);
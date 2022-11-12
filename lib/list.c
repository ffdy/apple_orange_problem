#include "list.h"

void initList(struct List *list) {
  initSem(&list->lock, 1);
  initSem(&list->memSum, 0);
  list->head.tail = NULL;
  list->tail = &list->head;
}

void PList(struct List *list, int *elem) {
  P(&list->memSum);
  P(&list->lock);
  *elem = list->head.tail->memId;
  list->head.tail = list->head.tail->tail;
  V(&list->lock);
}

void VList(struct List *list, struct Node *elem) {
  P(&list->lock);
  list->tail->tail = elem;
  list->tail = elem;
  V(&list->lock);
  V(&list->memSum);
}
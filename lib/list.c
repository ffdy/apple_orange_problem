#include "list.h"
#include <stdio.h>

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
  if (*elem == list->tail->memId)
    list->tail = &list->head;
  V(&list->lock);
}

void VList(struct List *list, struct Node *elem) {
  P(&list->lock);
  list->tail->tail = elem;
  list->tail = elem;
  V(&list->lock);
  V(&list->memSum);
}

void ShowList(struct List *list, char *tag, int id, int memId) {
  printf("***%s%3d%3d: %3d,", tag, id, memId, list->memSum.value);
  for (struct Node *i = list->head.tail; i != NULL; i = i->tail) {
    printf("%3d", i->memId);
    if (i == list->tail)
      break;
  }
  printf("\n");
}
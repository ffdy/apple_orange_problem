#include "list.h"
#include <stdio.h>
#include <malloc.h>

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
  struct Node *node = list->head.tail;
  list->head.tail = list->head.tail->tail;
  if (node == list->tail)
    list->tail = &list->head;
  free(node);
  V(&list->lock);
}

void VList(struct List *list, int elem) {
  P(&list->lock);
  ShowList(list, "<", -1, -1);
  printf("%p %p %p\n", list->head.tail, list->tail, &list->head);
  struct Node *node = (struct Node *)malloc(sizeof(struct Node));
  while (node == NULL)
    node = (struct Node *)malloc(sizeof(struct Node));
  node->memId = elem;
  node->tail = NULL;
  // printf("node:%3d %3d", node->memId)
  list->tail->tail = node;
  list->tail = node;
  ShowList(list, ">", -1, -1);
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
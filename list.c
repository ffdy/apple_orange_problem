#include "list.h"
#include <stdio.h>

void init_list(struct List *list) {
  init_sem(&list->lock, 1);
  init_sem(&list->mem_sum, 0);
  list->head.tail = NULL;
  list->tail = &list->head;
}

void P_list(struct List *list, int *elem) {
  P(&list->mem_sum);
  P(&list->lock);
  *elem = list->head.tail->mem_id;
  list->head.tail = list->head.tail->tail;
  if (*elem == list->tail->mem_id)
    list->tail = &list->head;
  V(&list->lock);
}

void V_list(struct List *list, struct Node *elem) {
  P(&list->lock);
  list->tail->tail = elem;
  list->tail = elem;
  V(&list->lock);
  V(&list->mem_sum);
}

void show_list(struct List *list, char *tag, int id, int mem_id) {
  printf("***%s%3d%3d: %3d,", tag, id, mem_id, list->mem_sum.value);
  for (struct Node *i = list->head.tail; i != NULL; i = i->tail) {
    printf("%3d", i->mem_id);
    if (i == list->tail)
      break;
  }
  printf("\n");
}
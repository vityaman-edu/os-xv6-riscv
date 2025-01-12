#include "../core/type.h"
#include "../defs.h"

#include "list.h"

void lst_init(struct list* lst) {
  lst->next = lst;
  lst->prev = lst;
}

int lst_empty(struct list* lst) { return lst->next == lst; }

void lst_remove(struct list* node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

void* lst_pop(struct list* lst) {
  if (lst->next == lst) {
    panic("lst_pop on en empty list");
  }
  struct list* p = lst->next;
  lst_remove(p);
  return (void*)p;
}

void lst_push(struct list* lst, void* p) {
  struct list* e = (struct list*)p;
  e->next = lst->next;
  e->prev = lst;
  lst->next->prev = p;
  lst->next = e;
}

void lst_print(struct list* lst) {
  for (struct list* node = lst->next; node != lst; node = node->next) {
    printf(" %p", node);
  }
  printf("\n");
}

#ifndef XV6_KERNEL_LIST_H
#define XV6_KERNEL_LIST_H

/// Double-linked, circular list. double-linked makes remove
/// fast. circular simplifies code, because don't have to check for
/// empty list in insert and remove.

struct list {
  struct list* next;
  struct list* prev;
};

void lst_init(struct list* lst);

int lst_empty(struct list* lst);

void lst_remove(struct list* node);

void* lst_pop(struct list* lst);

void lst_push(struct list* lst, void* p);

void lst_print(struct list* lst);

#endif // XV6_KERNEL_LIST_H
#ifndef XV6_KERNEL_PROC_STORAGE_H
#define XV6_KERNEL_PROC_STORAGE_H

#include "kernel/lib/list.h"
#include "kernel/process/proc.h"
#include "kernel/sync/spinlock.h"

typedef struct {
  struct list list;
  struct spinlock lock;
} ProcessQueue;

typedef struct {
  struct list list;
  struct proc process;
} ProcessQueue$Node;

void ProcessQueue$New(ProcessQueue* self);

void ProcessQueue$Push(ProcessQueue* self, ProcessQueue$Node* node);

#define ProcessQueue$ForEach(self) LST_FOR_EACH(it, &(self)->list)

#define ProcessQueue$Iterator$Process(it) (&((ProcessQueue$Node*)(it))->process)

#endif // XV6_KERNEL_PROC_STORAGE_H
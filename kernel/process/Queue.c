#include "kernel/process/Queue.h"
#include "kernel/lib/list.h"
#include "kernel/sync/spinlock.h"

void ProcessQueue$New(ProcessQueue* self) {
  lst_init(&self->list);
  initlock(&self->lock, "ProccessQueue$Lock");
}

void ProcessQueue$Push(ProcessQueue* self, ProcessQueue$Node* node) {
  acquire(&self->lock);
  lst_push(&self->list, node);
  release(&self->lock);
}

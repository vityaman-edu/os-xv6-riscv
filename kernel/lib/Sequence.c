#include "kernel/lib/Sequence.h"
#include "kernel/sync/spinlock.h"

void Sequence$New(Sequence* self) {
  initlock(&self->lock, "SequenceLock");
  self->next = 0;
}

Sequence$Number Sequence$Next(Sequence* self) {
  acquire(&self->lock);
  Sequence$Number number = self->next;
  self->next += 1;
  release(&self->lock);
  return number;
}

Sequence$Number Sequence$Last(Sequence* self) {
  acquire(&self->lock);
  Sequence$Number number = self->next;
  release(&self->lock);
  return number;
}
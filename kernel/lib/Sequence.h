#ifndef XV6_KERNEL_SEQUENCE_H
#define XV6_KERNEL_SEQUENCE_H

#include "kernel/sync/spinlock.h"

typedef int Sequence$Number;

typedef struct {
  Sequence$Number next;
  struct spinlock lock;
} Sequence;

void Sequence$New(Sequence* self);

Sequence$Number Sequence$Next(Sequence* self);

Sequence$Number Sequence$Last(Sequence* self);

#endif
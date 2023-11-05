#include "kernel/process/CPU.h"

static CPU CPU$Array[NCPU];

int CPU$Id() {
  return r_tp();
}

CPU* CPU$Current() {
  return &CPU$Array[CPU$Id()];
}
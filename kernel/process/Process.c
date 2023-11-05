#include "kernel/process/Process.h"
#include "kernel/process/CPU.h"
#include "kernel/sync/Interrupt.h"

Process* Process$Current() {
  Interrupts$Disable();
  CPU* cpu = CPU$Current();
  Process* proc = cpu->proc;
  Interrupts$Enable();
  return proc;
}

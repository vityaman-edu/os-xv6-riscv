#include "kernel/sync/Interrupt.h"

void push_off(void);
void pop_off(void);

void Interrupts$Enable() {
  pop_off();
}

void Interrupts$Disable() {
  push_off();
}

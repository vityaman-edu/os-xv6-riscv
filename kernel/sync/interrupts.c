#include <kernel/defs.h>
#include <kernel/hw/arch/riscv/riscv.h>
#include <kernel/process/proc.h>
#include <kernel/sync/interrupts.h>

void interrupts_enable() {
  intr_on();
}

void interrupts_disable() {
  intr_off();
}

bool interrupts_enabled() {
  return intr_get();
}

void interrupts_disable_push() {
  int old = interrupts_enabled();

  interrupts_disable();

  if (mycpu()->noff == 0) {
    mycpu()->intena = old;
  }
  
  mycpu()->noff += 1;
}

void interrupts_disable_pop() {
  struct cpu* cpu = mycpu();

  if (interrupts_enabled()) {
    panic("pop_off - interruptible");
  }

  if (cpu->noff < 1) {
    panic("pop_off");
  }

  cpu->noff -= 1;

  if (cpu->noff == 0 && cpu->intena) {
    interrupts_enable();
  }
}

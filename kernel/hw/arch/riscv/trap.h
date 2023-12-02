#pragma once

#include <kernel/core/type.h>
#include <kernel/process/proc.h>

typedef enum {
  SCAUSE_INSTRUCTION_ADDRESS_MISALIGNED = 0,
  SCAUSE_INSTRUCTION_ACCESS_FAULT = 1,
  SCAUSE_ILLEGAL_INSTRUCTION = 2,
  SCAUSE_BREAKPOINT = 3,
  SCAUSE_RESERVED_4 = 4,
  SCAUSE_LOAD_ACCESS_FAULT = 5,
  SCAUSE_AMO_ADDRESS_MISALIGNED = 6,
  SCAUSE_STORE_OR_AMO_ACCESS_FAULT = 7,
  SCAUSE_ENVIRONMENT_CALL = 8,
  SCAUSE_RESERVED_9 = 9,
  SCAUSE_RESERVED_10 = 10,
  SCAUSE_RESERVED_11 = 11,
  SCAUSE_INSTRUCTION_PAGE_FAULT = 12,
  SCAUSE_LOAD_PAGE_FAULT = 13,
  SCAUSE_RESERVED_14 = 14,
  SCAUSE_STORE_OR_AMO_PAGE_FAULT = 15,

  SCAUSE_INTERRUPT_TIMER = 0x8000000000000001L,
  SCAUSE_INTERRUPT_PLIC,

  SCAUSE_UNKNOWN,
} scause_kind;

scause_kind scause_kind_parse(uint64 scause);

void kerneltrap();
void interrupt_plic();
void interrupt_timer();

void usertrap();
void usertrap_return();

void usertrap_syscall(struct proc* caller);
void usertrap_page_fault(struct proc* caller, virt virt);

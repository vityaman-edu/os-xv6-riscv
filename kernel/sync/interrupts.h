#pragma once

#include <kernel/core/type.h>

void interrupts_enable();

void interrupts_disable();

bool interrupts_enabled();

/// push_off/pop_off are like intr_off()/intr_on() except that they
/// are matched: it takes two pop_off()s to undo two push_off()s.
/// Also, if interrupts are initially off, then push_off, pop_off
/// leaves them off.

void interrupts_disable_push();

void interrupts_disable_pop();

#pragma once

#include <kernel/core/type.h>

typedef struct {
  byte* as_bytes;
} bits;

typedef struct {
  bits bits;
  int len;
} fat_bits;

bool bits_is_set(bits bits, int index);

void bits_set(bits bits, int index);

void bits_clear(bits bits, int index);

void bits_switch(bits bits, int index);

void bits_print(fat_bits bits);

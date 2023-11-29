#include <kernel/alloc/bits.h>
#include <kernel/defs.h>

/// Return 1 if bit at position index in array is set to 1
bool bits_is_set(bits bits, int index) {
  const byte cell = bits.as_bytes[index / 8];
  const byte mask = (1 << (index % 8));
  return (cell & mask) == mask;
}

/// Set bit at position index in array to 1
void bits_set(bits bits, int index) {
  const byte cell = bits.as_bytes[index / 8];
  const byte mask = (1 << (index % 8));
  bits.as_bytes[index / 8] = (cell | mask);
}

/// Clear bit at position index in array
void bits_clear(bits bits, int index) {
  const byte cell = bits.as_bytes[index / 8];
  const byte mask = (1 << (index % 8));
  bits.as_bytes[index / 8] = (cell & ~mask);
}

void bits_switch(bits bits, int index) {
  if (bits_is_set(bits, index)) {
    bits_clear(bits, index);
  } else {
    bits_set(bits, index);
  }
}

/// Print a bit vector as a list of ranges of 1 bits
void bits_print(fat_bits bits) {
  int last_bit = 1;
  int last_bit_index = 0;

  for (int bit_index = 0; bit_index < bits.len; bit_index++) {
    if (last_bit == bits_is_set(bits.bits, bit_index)) {
      continue;
    }

    if (last_bit == 1) {
      printf(" [%d, %d)", last_bit_index, bit_index);
    }

    last_bit_index = bit_index;
    last_bit = bits_is_set(bits.bits, bit_index);
  }

  if (last_bit_index == 0 || last_bit == 1) {
    printf(" [%d, %d)", last_bit_index, bits.len);
  }

  printf("\n");
}

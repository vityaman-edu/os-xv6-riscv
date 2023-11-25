#pragma once

#include <kernel/types.h>

int UserVirtMemoryCopy(
    pagetable_t src, pagetable_t dst, UInt64 size
);

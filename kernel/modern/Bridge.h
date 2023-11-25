#pragma once

#include <kernel/types.h>

int UserVityalMemoryCopy(
    pagetable_t src, pagetable_t dst, UInt64 size
);

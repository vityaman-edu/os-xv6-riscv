#pragma once

/// Result status. Either OK or some error code.
typedef enum {
  OK = 0,    // Success.
  BAD_ALLOC, // Allocation failed.
  UNKNOWN,   // Unknown error.
} rstatus_t;

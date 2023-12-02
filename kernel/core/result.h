#pragma once

/// Result status. Either OK or some error code.
typedef enum {
  OK = 0,            // Success.
  BAD_ALLOC,         // Allocation failed.
  NOT_FOUND,         // Something does not exist.
  PERMISSION_DENIED, // User have no permission.
  UNKNOWN,           // Unknown error.
} rstatus_t;

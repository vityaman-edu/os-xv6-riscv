#pragma once

/// Readable functions to check flags status.
///
/// Made as macros as flags can be represented as 
/// different int types, but there are no templates
/// in C.

/// Checks that `flags` contain `flag`.
#define FLAG_ENABLED(flags, flag) (((flags) & (flag)) != (0))

/// Checks that `flags` do not contain `flag`.
#define FLAG_DISABLED(flags, flag) (!(FLAG_ENABLED(flags, flag)))

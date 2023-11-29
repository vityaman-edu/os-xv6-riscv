// Create a zombie process that
// must be reparented at exit.

#include "kernel/legacy/types.h"
#include "kernel/legacy/stat.h"
#include "user/user.h"

int main(void) {
  if (fork() > 0)
    sleep(5); // Let child exit before parent.
  exit(0);
}

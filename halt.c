#include "defs.h"

  // Use virt test device to terminate:
  //   SUCCESS: *0x100000 = 0x5555
  //      FAIL: *0x100000 = 0x3333
  //

void halt(void) {
  // Use virt test device to terminate
  *(int*)0x100000 = 0x5555; // success
  for (;;) continue; // just in case
}

void panic(const char * msg) {
  printf("PANIC! %s\n", msg);
  *(int*)0x100000 = 0x3333; // fail
  for (;;) continue; // just in case
}

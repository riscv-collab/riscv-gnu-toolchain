<<<<<<< HEAD
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
=======
// halt.c
//

#include "halt.h"
#include "trap.h"
#include "console.h"

#include <stdint.h>

// The halt_success and halt_failure functions use the virt test device to
// terminate. Will not work on real hardware.

void halt_success(void) {
	*(int*)0x100000 = 0x5555; // success
	for (;;) continue; // just in case
}

void halt_failure(void) {
	*(int*)0x100000 = 0x3333; // failure
	for (;;) continue; // just in case
}

void panic(const char * msg) {
	if (msg != NULL)
		console_puts(msg);
	
	halt_failure();
}

// fault_handler() is called from trap.s by _trap_entry

void fault_handler(int code, struct trap_frame * tfr) {
    kprintf("PANIC Unhandled fault %d at 0x%lx\n", code, (long)tfr->mepc);
    panic(NULL);
}
>>>>>>> release/mp1

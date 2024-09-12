// trap.h - Trap entry and initialization
// 

#ifndef _TRAP_H_
#define _TRAP_H_

#include <stdint.h>

// All traps are handled by _trap_entry defined in trap.s, which saves the
// current context in a struct trap_frame on the stack and dispatches to one of
// the handlers listed below. It arranges to restore the saved context when the
// handler returns.

struct trap_frame {
    uint64_t x[32]; // x[0] unused
    uint64_t mstatus;
    uint64_t mepc;
};

// _trap_entry dispatches to fault_handler in halt.c (for now) or intr_handler
// in intr.c. The code argument is the exception code (mcause[62:0]).

extern void fault_handler(int code, struct trap_frame * tfr);
extern void intr_handler(int code);

#endif // _TRAP_H_
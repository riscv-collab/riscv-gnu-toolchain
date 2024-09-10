// intr.c - Interrupt management
// 

#include "intr.h"
#include "trap.h"
#include "halt.h"
#include "csr.h"

#include <stddef.h>

extern void timer_intr_handler(void) __attribute__ ((weak));

// EXPORTED GLOBAL VARIABLE DEFINITIONS
// 

int intr_initialized = 0;

// EXPORTED FUNCTION DEFINITIONS
//

void intr_init(void) {
    intr_disable(); // should be disabled already
    csrw_mip(0); // clear all pending interrupts

    intr_initialized = 1;
}

// INTERNAL FUNCTION DEFINITIONS
//

// intr_handler() is called from trap.s to handle an interrupt

void intr_handler(int code) {
    switch (code) {
    case RISCV_MCAUSE_EXCODE_MTI:
        timer_intr_handler();
        break;
    default:
        panic("unhandled interrupt");
        break;
    }
}

void timer_intr_handler(void) {
    // do nothing
}
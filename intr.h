// intr.h - Interrupt management
// 

#ifndef _INTR_H_
#define _INTR_H_

#include "csr.h"

// EXPORTED GLOBAL VARIABLE DECLARATIONS
// 

extern int intr_initialized;

// EXPORTED FUNCTION DECLARATIONS
// 

extern void intr_init(void);

static inline int intr_enable(void);
static inline int intr_disable(void);
static inline void intr_restore(int saved);

// intr_enabled and intr_disabled return the current status of interrupts

static inline int intr_enabled(void);
static inline int intr_disabled(void);

extern void intr_register_isr (
    int irqno, int prio, void (*isr)(int irqno, void * aux), void * isr_aux);

extern void intr_enable_irq(int irqno);
extern void intr_disable_irq(int irqno);

// INLINE FUNCTION DEFINITIONS
//

static inline int intr_enable(void) {
    int64_t mstatus;

    asm volatile (
    "csrrsi  %0, mstatus, %1"
    : "=r" (mstatus) : "I" (RISCV_MSTATUS_MIE));

    return (int)mstatus;
}

static inline int intr_disable(void) {
    int64_t mstatus;

    asm (
    "csrrci %0, mstatus, %1"
    : "=r" (mstatus) : "I" (RISCV_MSTATUS_MIE));

    return (int)mstatus;
}

static inline void intr_restore(int saved_intr_state) {
    asm (
    "csrrs  zero, mstatus, %0"
    :: "r" (saved_intr_state & RISCV_MSTATUS_MIE));
}

static int intr_enabled(void) {
    return ((csrr_mstatus() & RISCV_MSTATUS_MIE) != 0);
}

static int intr_disabled(void) {
    return ((csrr_mstatus() & RISCV_MSTATUS_MIE) == 0);
}

#endif // _INTR_H_
#ifndef RISCV_SYSCFG_H
#define RISCV_SYSCFG_H

//------------------------------------------------------------------------
// RISC-V minimum and maximum vector length
//------------------------------------------------------------------------

#define RISCV_SYSCFG_VLEN_MIN 4
#define RISCV_SYSCFG_VLEN_MAX 32

//------------------------------------------------------------------------
// Threads
//------------------------------------------------------------------------

// Size of the various stacks
#define RISCV_SYSCFG_USER_THREAD_STACK_SIZE   0x00010000
#define RISCV_SYSCFG_KERNEL_THREAD_STACK_SIZE 0x00010000
#define RISCV_SYSCFG_UT_THREAD_STACK_SIZE     0x00001000

#define RISCV_SYSCFG_MAX_PROCS 64 // maximum number of processors in the system
#define RISCV_SYSCFG_MAX_KEYS  64 // maximum number of unique keys per thread

#endif // RISCV_SYSCFG_H

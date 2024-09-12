// csr.h - Access to RISC-V CSRs
//

#ifndef _CSR_H_
#define _CSR_H_

#include <stdint.h>

#define RISCV_MCAUSE_EXCODE_SSI 1
#define RISCV_MCAUSE_EXCODE_MSI 3
#define RISCV_MCAUSE_EXCODE_STI 5
#define RISCV_MCAUSE_EXCODE_MTI 7
#define RISCV_MCAUSE_EXCODE_SEI 9
#define RISCV_MCAUSE_EXCODE_MEI 11

static inline intptr_t csrr_mcause(void) {
    intptr_t val;
    asm inline ("csrr %0, mcause" : "=r" (val));
    return val;
}

static inline void csrw_mtvec(void (*handler)(void)) {
    asm inline volatile ("csrw mtvec, %0" :: "r" (handler));
}

static inline void csrw_mscratch(intptr_t val) {
    asm inline volatile("csrw mscratch, %0" :: "r" (val));
}

#define RISCV_MIE_SSIE (1 << 1)
#define RISCV_MIE_MSIE (1 << 3)
#define RISCV_MIE_STIE (1 << 5)
#define RISCV_MIE_MTIE (1 << 7)
#define RISCV_MIE_SEIE (1 << 9)
#define RISCV_MIE_MEIE (1 << 11)

static inline void csrw_mie(intptr_t mask) {
    asm inline volatile("csrw mie, %0" :: "r" (mask));
}

static inline void csrs_mie(intptr_t mask) {
    asm inline volatile ("csrrs zero, mie, %0" :: "r" (mask));
}

static inline void csrc_mie(intptr_t mask) {
    asm inline volatile ("csrrc %0, mie, %0" :: "r" (mask));
}
#define RISCV_MIP_SSIP (1 << 1)
#define RISCV_MIP_MSIP (1 << 3)
#define RISCV_MIP_STIP (1 << 5)
#define RISCV_MIP_MTIP (1 << 7)
#define RISCV_MIP_SEIP (1 << 9)
#define RISCV_MIP_MEIP (1 << 11)

static inline void csrw_mip(intptr_t mask) {
    asm inline volatile("csrw mip, %0" :: "r" (mask));
}

static inline void csrs_mip(intptr_t mask) {
    asm inline volatile ("csrrs zero, mip, %0" :: "r" (mask));
}

static inline void csrc_mip(intptr_t mask) {
    asm inline volatile ("csrrc %0, mip, %0" :: "r" (mask));
}

#define RISCV_MSTATUS_SIE (1 << 1)
#define RISCV_MSTATUS_MIE (1 << 3)
#define RISCV_MSTATUS_SPIE (1 << 3)
#define RISCV_MSTATUS_MPIE (1 << 3)
#define RISCV_MSTATUS_SPP (1 << 8)
#define RISCV_MSTATUS_MPP_shift 11

static inline intptr_t csrr_mstatus(void) {
    intptr_t val;

    asm inline volatile ("csrr %0, mstatus" : "=r" (val));
    return val;
}

static inline void csrs_mstatus(intptr_t mask) {
    asm inline volatile ("csrrs zero, mstatus, %0" :: "r" (mask));
}

static inline void csrc_mstatus(intptr_t mask) {
    asm inline volatile ("csrrc zero, mstatus, %0" :: "r" (mask));
}

#endif // _CSR_H_
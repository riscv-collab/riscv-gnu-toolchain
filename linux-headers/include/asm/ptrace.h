#ifndef _ASM_RISCV_PTRACE_H
#define _ASM_RISCV_PTRACE_H

#include <asm/csr.h>

#ifndef __ASSEMBLY__

typedef struct pt_regs {
	unsigned long zero;
	unsigned long ra;
	unsigned long s[12];
	unsigned long sp;
	unsigned long tp;
	unsigned long v[2];
	unsigned long a[8];
	unsigned long t[5];
	unsigned long gp;
	/* PCRs */
	unsigned long status;
	unsigned long epc;
	unsigned long badvaddr;
	unsigned long cause;
	/* For restarting system calls */
	unsigned long syscallno;
} pt_regs;

#define user_mode(regs) (((regs)->status & SR_PS) == 0)


/* Helpers for working with the instruction pointer */
#define GET_IP(regs) ((regs)->epc)
#define SET_IP(regs, val) (GET_IP(regs) = (val))

static __inline__ unsigned long instruction_pointer(struct pt_regs *regs)
{
	return GET_IP(regs);
}
static __inline__ void instruction_pointer_set(struct pt_regs *regs,
                                           unsigned long val)
{
	SET_IP(regs, val);
}

#define profile_pc(regs) instruction_pointer(regs)

/* Helpers for working with the user stack pointer */
#define GET_USP(regs) ((regs)->sp)
#define SET_USP(regs, val) (GET_USP(regs) = (val))

static __inline__ unsigned long user_stack_pointer(struct pt_regs *regs)
{
	return GET_USP(regs);
}
static __inline__ void user_stack_pointer_set(struct pt_regs *regs,
                                          unsigned long val)
{
	SET_USP(regs, val);
}

/* Helpers for working with the frame pointer */
#define GET_FP(regs) ((regs)->s[0])
#define SET_FP(regs, val) (GET_FP(regs) = (val))

static __inline__ unsigned long frame_pointer(struct pt_regs *regs)
{
	return GET_FP(regs);
}
static __inline__ void frame_pointer_set(struct pt_regs *regs,
                                     unsigned long val)
{
	SET_FP(regs, val);
}

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PTRACE_H */

#ifndef _ASM_RISCV_USER_H
#define _ASM_RISCV_USER_H

/* Mirror pt_regs from ptrace.h */

typedef struct user_regs_struct {
	unsigned long zero;
	unsigned long ra;
	unsigned long s[12];
	unsigned long sp;
	unsigned long tp;
	unsigned long v[2];
	unsigned long a[8];
	unsigned long t[5];
	unsigned long gp;
	unsigned long status;
} user_regs_struct;

#endif /* _ASM_RISCV_USER_H */

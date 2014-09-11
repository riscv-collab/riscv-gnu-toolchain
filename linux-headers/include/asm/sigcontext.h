#ifndef __ASM_RISCV_SIGCONTEXT_H
#define __ASM_RISCV_SIGCONTEXT_H

/* This struct is saved by setup_frame in signal.c, to keep the current
 * context while a signal handler is executed. It is restored by sys_sigreturn.
 */

struct sigcontext {
	unsigned long zero;
	unsigned long ra;
	unsigned long s[12];
	unsigned long sp;
	unsigned long tp;
	unsigned long v[2];
	unsigned long a[8];
	unsigned long t[5];
	unsigned long gp;
	unsigned long epc;
};

#endif /* __ASM_RISCV_SIGCONTEXT_H */

#ifndef _ASM_RISCV_SIGCONTEXT_H
#define _ASM_RISCV_SIGCONTEXT_H

#include <asm/ptrace.h>

/* Signal context structure
 *
 * This contains the context saved before a signal handler is invoked;
 * it is restored by sys_sigreturn / sys_rt_sigreturn.
 */
struct sigcontext {
	struct user_regs_struct sc_regs;
	struct user_fpregs_struct sc_fpregs;
};

#endif /* _ASM_RISCV_SIGCONTEXT_H */

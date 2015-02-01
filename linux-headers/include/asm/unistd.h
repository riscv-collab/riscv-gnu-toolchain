#include <asm-generic/unistd.h>

#define __NR_sysriscv  __NR_arch_specific_syscall
#ifndef __riscv_atomic
__SYSCALL(__NR_sysriscv, sys_sysriscv)
#endif

#define RISCV_ATOMIC_CMPXCHG    1
#define RISCV_ATOMIC_CMPXCHG64  2

#if !defined(_ASM_RISCV_UNISTD_H) || defined(__SYSCALL)
#define _ASM_RISCV_UNISTD_H

#define __ARCH_HAVE_MMU
#define __ARCH_WANT_SYSCALL_NO_AT
#define __ARCH_WANT_SYSCALL_DEPRECATED
#define __ARCH_WANT_SYSCALL_OFF_T
#define __ARCH_WANT_SYSCALL_NO_FLAGS

#define __ARCH_WANT_SYS_EXECVE
#define __ARCH_WANT_SYS_CLONE
#define __ARCH_WANT_SYS_VFORK
#define __ARCH_WANT_SYS_FORK

#include <asm-generic/unistd.h>

#define __NR_sysriscv  __NR_arch_specific_syscall
#ifdef CONFIG_RV_SYSRISCV_ATOMIC
__SYSCALL(__NR_sysriscv, sys_sysriscv)
#endif

#define __NR_ipc 1080
#undef  __NR_syscalls
#define __NR_syscalls (__NR_ipc + 1)

#define RISCV_ATOMIC_CMPXCHG    1
#define RISCV_ATOMIC_CMPXCHG64  2

#endif /* _ASM_RISCV_UNISTD_H */

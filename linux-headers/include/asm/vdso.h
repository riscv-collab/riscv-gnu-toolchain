#ifndef _ASM_RISCV_VDSO_H
#define _ASM_RISCV_VDSO_H

#include <linux/types.h>

struct vdso_data {
};

#define VDSO_SYMBOL(base, name)					\
({								\
	extern const char __vdso_##name[];			\
	(void *)((unsigned long)(base) + __vdso_##name);	\
})

#endif /* _ASM_RISCV_VDSO_H */

#ifndef _ASM_RISCV_CSR_H
#define _ASM_RISCV_CSR_H

#include <linux/const.h>

/* Status register flags */
#define SR_S    _AC(0x00000001,UL) /* Supervisor */
#define SR_PS   _AC(0x00000002,UL) /* Previous supervisor */
#define SR_EI   _AC(0x00000004,UL) /* Enable interrupts */
#define SR_PEI  _AC(0x00000008,UL) /* Previous EI */
#define SR_EF   _AC(0x00000010,UL) /* Enable floating-point */
#define SR_U64  _AC(0x00000020,UL) /* RV64 user mode */
#define SR_S64  _AC(0x00000040,UL) /* RV64 supervisor mode */
#define SR_VM   _AC(0x00000080,UL) /* Enable virtual memory */
#define SR_IM   _AC(0x00FF0000,UL) /* Interrupt mask */
#define SR_IP   _AC(0xFF000000,UL) /* Pending interrupts */

#define SR_IM_SHIFT     16
#define SR_IM_MASK(n)   ((_AC(1,UL)) << ((n) + SR_IM_SHIFT))

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_SYSCALL             6
#define EXC_LOAD_MISALIGNED     8
#define EXC_STORE_MISALIGNED    9
#define EXC_LOAD_ACCESS         10
#define EXC_STORE_ACCESS        11

#ifndef __ASSEMBLY__

#define CSR_ZIMM(val) \
	(__builtin_constant_p(val) && ((unsigned long)(val) < 0x20))

#define csr_swap(csr,val)					\
({								\
	typeof(val) __v = (val);				\
	if (CSR_ZIMM(__v)) { 					\
		__asm__ __volatile__ (				\
			"csrrw %0, " #csr ", %1"		\
			: "=r" (__v) : "i" (__v));		\
	} else {						\
		__asm__ __volatile__ (				\
			"csrrw %0, " #csr ", %1"		\
			: "=r" (__v) : "r" (__v));		\
	}							\
	__v;							\
})

#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ __volatile__ (					\
		"csrr %0, " #csr : "=r" (__v));			\
	__v;							\
})

#define csr_write(csr,val)					\
({								\
	typeof(val) __v = (val);				\
	if (CSR_ZIMM(__v)) {					\
		__asm__ __volatile__ (				\
			"csrw " #csr ", %0" : : "i" (__v));	\
	} else {						\
		__asm__ __volatile__ (				\
			"csrw " #csr ", %0" : : "r" (__v));	\
	}							\
})

#define csr_read_set(csr,val)					\
({								\
	typeof(val) __v = (val);				\
	if (CSR_ZIMM(val)) {					\
		__asm__ __volatile__ (				\
			"csrrs %0, " #csr ", %1"		\
			: "=r" (__v) : "i" (__v));		\
	} else {						\
		__asm__ __volatile__ (				\
			"csrrs %0, " #csr ", %1"		\
			: "=r" (__v) : "r" (__v));		\
	}							\
	__v;							\
})

#define csr_set(csr,val)					\
({								\
	typeof(val) __v = (val);				\
	if (CSR_ZIMM(__v)) {					\
		__asm__ __volatile__ (				\
			"csrs " #csr ", %0" : : "i" (__v));	\
	} else {						\
		__asm__ __volatile__ (				\
			"csrs " #csr ", %0" : : "r" (__v));	\
	}							\
})

#define csr_read_clear(csr,val)					\
({								\
	typeof(val) __v = (val);				\
	if (CSR_ZIMM(__v)) {					\
		__asm__ __volatile__ (				\
			"csrrc %0, " #csr ", %1"		\
			: "=r" (__v) : "i" (__v));		\
	} else {						\
		__asm__ __volatile__ (				\
			"csrrc %0, " #csr ", %1"		\
			: "=r" (__v) : "r" (__v));		\
	}							\
	__v;							\
})

#define csr_clear(csr,val)					\
({								\
	typeof(val) __v = (val);				\
	if (CSR_ZIMM(__v)) {					\
		__asm__ __volatile__ (				\
			"csrc " #csr ", %0" : : "i" (__v));	\
	} else {						\
		__asm__ __volatile__ (				\
			"csrc " #csr ", %0" : : "r" (__v));	\
	}							\
})

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_CSR_H */

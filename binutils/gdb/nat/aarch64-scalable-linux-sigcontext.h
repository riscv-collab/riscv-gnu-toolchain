/* Linux Kernel sigcontext definitions for AArch64 Scalable Extensions
   (SVE/SME).

   Copyright (C) 2018-2024 Free Software Foundation, Inc.
   Contributed by Arm Ltd.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef NAT_AARCH64_SCALABLE_LINUX_SIGCONTEXT_H
#define NAT_AARCH64_SCALABLE_LINUX_SIGCONTEXT_H

#ifndef SVE_SIG_ZREGS_SIZE

#define SVE_MAGIC	0x53564501


struct sve_context {
	struct _aarch64_ctx head;
	__u16 vl;
	/* Holds flags.  This field was defined for SME support.  Prior to it,
	   this used to be a reserved 16-bit value.  */
	__u16 flags;
	__u16 __reserved[2];
};

/*
 * The SVE architecture leaves space for future expansion of the
 * vector length beyond its initial architectural limit of 2048 bits
 * (16 quadwords).
 *
 * See linux/Documentation/arm64/sve.txt for a description of the VL/VQ
 * terminology.
 */
#define SVE_VQ_BYTES		16	/* number of bytes per quadword */

#define SVE_VQ_MIN		1
#define SVE_VQ_MAX		512

#define SVE_VL_MIN		(SVE_VQ_MIN * SVE_VQ_BYTES)
#define SVE_VL_MAX		(SVE_VQ_MAX * SVE_VQ_BYTES)

#define SVE_NUM_ZREGS		32
#define SVE_NUM_PREGS		16

#define sve_vl_valid(vl) \
	((vl) % SVE_VQ_BYTES == 0 && (vl) >= SVE_VL_MIN && (vl) <= SVE_VL_MAX)

/*
 * If the SVE registers are currently live for the thread at signal delivery,
 * sve_context.head.size >=
 *	SVE_SIG_CONTEXT_SIZE(sve_vq_from_vl(sve_context.vl))
 * and the register data may be accessed using the SVE_SIG_*() macros.
 *
 * If sve_context.head.size <
 *	SVE_SIG_CONTEXT_SIZE(sve_vq_from_vl(sve_context.vl)),
 * the SVE registers were not live for the thread and no register data
 * is included: in this case, the SVE_SIG_*() macros should not be
 * used except for this check.
 *
 * The same convention applies when returning from a signal: a caller
 * will need to remove or resize the sve_context block if it wants to
 * make the SVE registers live when they were previously non-live or
 * vice-versa.  This may require the caller to allocate fresh
 * memory and/or move other context blocks in the signal frame.
 *
 * Changing the vector length during signal return is not permitted:
 * sve_context.vl must equal the thread's current vector length when
 * doing a sigreturn.
 *
 *
 * Note: for all these macros, the "vq" argument denotes the SVE
 * vector length in quadwords (i.e., units of 128 bits).
 *
 * The correct way to obtain vq is to use sve_vq_from_vl(vl).  The
 * result is valid if and only if sve_vl_valid(vl) is true.  This is
 * guaranteed for a struct sve_context written by the kernel.
 *
 *
 * Additional macros describe the contents and layout of the payload.
 * For each, SVE_SIG_x_OFFSET(args) is the start offset relative to
 * the start of struct sve_context, and SVE_SIG_x_SIZE(args) is the
 * size in bytes:
 *
 *	x	type				description
 *	-	----				-----------
 *	REGS					the entire SVE context
 *
 *	ZREGS	__uint128_t[SVE_NUM_ZREGS][vq]	all Z-registers
 *	ZREG	__uint128_t[vq]			individual Z-register Zn
 *
 *	PREGS	uint16_t[SVE_NUM_PREGS][vq]	all P-registers
 *	PREG	uint16_t[vq]			individual P-register Pn
 *
 *	FFR	uint16_t[vq]			first-fault status register
 *
 * Additional data might be appended in the future.
 */

#define SVE_SIG_ZREG_SIZE(vq)	((__u32)(vq) * SVE_VQ_BYTES)
#define SVE_SIG_PREG_SIZE(vq)	((__u32)(vq) * (SVE_VQ_BYTES / 8))
#define SVE_SIG_FFR_SIZE(vq)	SVE_SIG_PREG_SIZE(vq)

#define SVE_SIG_REGS_OFFSET					\
	((sizeof(struct sve_context) + (SVE_VQ_BYTES - 1))	\
		/ SVE_VQ_BYTES * SVE_VQ_BYTES)

#define SVE_SIG_ZREGS_OFFSET	SVE_SIG_REGS_OFFSET
#define SVE_SIG_ZREG_OFFSET(vq, n) \
	(SVE_SIG_ZREGS_OFFSET + SVE_SIG_ZREG_SIZE(vq) * (n))
#define SVE_SIG_ZREGS_SIZE(vq) \
	(SVE_SIG_ZREG_OFFSET(vq, SVE_NUM_ZREGS) - SVE_SIG_ZREGS_OFFSET)

#define SVE_SIG_PREGS_OFFSET(vq) \
	(SVE_SIG_ZREGS_OFFSET + SVE_SIG_ZREGS_SIZE(vq))
#define SVE_SIG_PREG_OFFSET(vq, n) \
	(SVE_SIG_PREGS_OFFSET(vq) + SVE_SIG_PREG_SIZE(vq) * (n))
#define SVE_SIG_PREGS_SIZE(vq) \
	(SVE_SIG_PREG_OFFSET(vq, SVE_NUM_PREGS) - SVE_SIG_PREGS_OFFSET(vq))

#define SVE_SIG_FFR_OFFSET(vq) \
	(SVE_SIG_PREGS_OFFSET(vq) + SVE_SIG_PREGS_SIZE(vq))

#define SVE_SIG_REGS_SIZE(vq) \
	(SVE_SIG_FFR_OFFSET(vq) + SVE_SIG_FFR_SIZE(vq) - SVE_SIG_REGS_OFFSET)

#define SVE_SIG_CONTEXT_SIZE(vq) (SVE_SIG_REGS_OFFSET + SVE_SIG_REGS_SIZE(vq))

/* SVE/FP/SIMD state (NT_ARM_SVE and NT_ARM_SSVE) */

struct user_sve_header {
	__u32 size; /* total meaningful regset content in bytes */
	__u32 max_size; /* maximum possible size for this thread */
	__u16 vl; /* current vector length */
	__u16 max_vl; /* maximum possible vector length */
	__u16 flags;
	__u16 __reserved;
};

/* Definitions for user_sve_header.flags: */
#define SVE_PT_REGS_MASK		(1 << 0)

#define SVE_PT_REGS_FPSIMD		0
#define SVE_PT_REGS_SVE			SVE_PT_REGS_MASK

/*
 * Common SVE_PT_* flags:
 * These must be kept in sync with prctl interface in <linux/ptrace.h>
 */
#define SVE_PT_VL_INHERIT		(PR_SVE_VL_INHERIT >> 16)
#define SVE_PT_VL_ONEXEC		(PR_SVE_SET_VL_ONEXEC >> 16)


/*
 * The remainder of the SVE state follows struct user_sve_header.  The
 * total size of the SVE state (including header) depends on the
 * metadata in the header:  SVE_PT_SIZE(vq, flags) gives the total size
 * of the state in bytes, including the header.
 *
 * Refer to <asm/sigcontext.h> for details of how to pass the correct
 * "vq" argument to these macros.
 */

/* Offset from the start of struct user_sve_header to the register data */
#define SVE_PT_REGS_OFFSET					\
	((sizeof(struct user_sve_header) + (SVE_VQ_BYTES - 1))	\
		/ SVE_VQ_BYTES * SVE_VQ_BYTES)

/*
 * The register data content and layout depends on the value of the
 * flags field.
 */

/*
 * (flags & SVE_PT_REGS_MASK) == SVE_PT_REGS_FPSIMD case:
 *
 * The payload starts at offset SVE_PT_FPSIMD_OFFSET, and is of type
 * struct user_fpsimd_state.  Additional data might be appended in the
 * future: use SVE_PT_FPSIMD_SIZE(vq, flags) to compute the total size.
 * SVE_PT_FPSIMD_SIZE(vq, flags) will never be less than
 * sizeof(struct user_fpsimd_state).
 */

#define SVE_PT_FPSIMD_OFFSET		SVE_PT_REGS_OFFSET

#define SVE_PT_FPSIMD_SIZE(vq, flags)	(sizeof(struct user_fpsimd_state))

/*
 * (flags & SVE_PT_REGS_MASK) == SVE_PT_REGS_SVE case:
 *
 * The payload starts at offset SVE_PT_SVE_OFFSET, and is of size
 * SVE_PT_SVE_SIZE(vq, flags).
 *
 * Additional macros describe the contents and layout of the payload.
 * For each, SVE_PT_SVE_x_OFFSET(args) is the start offset relative to
 * the start of struct user_sve_header, and SVE_PT_SVE_x_SIZE(args) is
 * the size in bytes:
 *
 *	x	type				description
 *	-	----				-----------
 *	ZREGS		\
 *	ZREG		|
 *	PREGS		| refer to <asm/sigcontext.h>
 *	PREG		|
 *	FFR		/
 *
 *	FPSR	uint32_t			FPSR
 *	FPCR	uint32_t			FPCR
 *
 * Additional data might be appended in the future.
 */

#define SVE_PT_SVE_ZREG_SIZE(vq)	SVE_SIG_ZREG_SIZE(vq)
#define SVE_PT_SVE_PREG_SIZE(vq)	SVE_SIG_PREG_SIZE(vq)
#define SVE_PT_SVE_FFR_SIZE(vq)		SVE_SIG_FFR_SIZE(vq)
#define SVE_PT_SVE_FPSR_SIZE		sizeof(__u32)
#define SVE_PT_SVE_FPCR_SIZE		sizeof(__u32)

#define __SVE_SIG_TO_PT(offset) \
	((offset) - SVE_SIG_REGS_OFFSET + SVE_PT_REGS_OFFSET)

#define SVE_PT_SVE_OFFSET		SVE_PT_REGS_OFFSET

#define SVE_PT_SVE_ZREGS_OFFSET \
	__SVE_SIG_TO_PT(SVE_SIG_ZREGS_OFFSET)
#define SVE_PT_SVE_ZREG_OFFSET(vq, n) \
	__SVE_SIG_TO_PT(SVE_SIG_ZREG_OFFSET(vq, n))
#define SVE_PT_SVE_ZREGS_SIZE(vq) \
	(SVE_PT_SVE_ZREG_OFFSET(vq, SVE_NUM_ZREGS) - SVE_PT_SVE_ZREGS_OFFSET)

#define SVE_PT_SVE_PREGS_OFFSET(vq) \
	__SVE_SIG_TO_PT(SVE_SIG_PREGS_OFFSET(vq))
#define SVE_PT_SVE_PREG_OFFSET(vq, n) \
	__SVE_SIG_TO_PT(SVE_SIG_PREG_OFFSET(vq, n))
#define SVE_PT_SVE_PREGS_SIZE(vq) \
	(SVE_PT_SVE_PREG_OFFSET(vq, SVE_NUM_PREGS) - \
		SVE_PT_SVE_PREGS_OFFSET(vq))

/* For streaming mode SVE (SSVE) FFR must be read and written as zero.  */
#define SVE_PT_SVE_FFR_OFFSET(vq) \
	__SVE_SIG_TO_PT(SVE_SIG_FFR_OFFSET(vq))

#define SVE_PT_SVE_FPSR_OFFSET(vq)				\
	((SVE_PT_SVE_FFR_OFFSET(vq) + SVE_PT_SVE_FFR_SIZE(vq) +	\
			(SVE_VQ_BYTES - 1))			\
		/ SVE_VQ_BYTES * SVE_VQ_BYTES)
#define SVE_PT_SVE_FPCR_OFFSET(vq) \
	(SVE_PT_SVE_FPSR_OFFSET(vq) + SVE_PT_SVE_FPSR_SIZE)

/*
 * Any future extension appended after FPCR must be aligned to the next
 * 128-bit boundary.
 */

#define SVE_PT_SVE_SIZE(vq, flags)					\
	((SVE_PT_SVE_FPCR_OFFSET(vq) + SVE_PT_SVE_FPCR_SIZE		\
			- SVE_PT_SVE_OFFSET + (SVE_VQ_BYTES - 1))	\
		/ SVE_VQ_BYTES * SVE_VQ_BYTES)

#define SVE_PT_SIZE(vq, flags)						\
	 (((flags) & SVE_PT_REGS_MASK) == SVE_PT_REGS_SVE ?		\
		  SVE_PT_SVE_OFFSET + SVE_PT_SVE_SIZE(vq, flags)	\
		: SVE_PT_FPSIMD_OFFSET + SVE_PT_FPSIMD_SIZE(vq, flags))

#endif /* SVE_SIG_ZREGS_SIZE */

/* Scalable Matrix Extensions (SME) definitions.  */

/* Make sure we only define these if the kernel header doesn't.  */
#ifndef ZA_PT_SIZE

/* ZA state (NT_ARM_ZA) */
struct user_za_header {
	__u32 size; /* total meaningful regset content in bytes */
	__u32 max_size; /* maximum possible size for this thread */
	__u16 vl; /* current vector length */
	__u16 max_vl; /* maximum possible vector length */
	__u16 flags;
	__u16 __reserved;
};

/* The remainder of the ZA state follows struct user_za_header.  The
   total size of the ZA state (including header) depends on the
   metadata in the header:  ZA_PT_SIZE(vq, flags) gives the total size
   of the state in bytes, including the header.

   Refer to arch/arm64/include/uapi/asm/sigcontext.h from the Linux kernel
   for details of how to pass the correct "vq" argument to these macros.  */

/* Offset from the start of struct user_za_header to the register data */
#define ZA_PT_ZA_OFFSET						  \
	((sizeof (struct user_za_header) + (SVE_VQ_BYTES - 1))  \
		/ SVE_VQ_BYTES * SVE_VQ_BYTES)

/* The payload starts at offset ZA_PT_ZA_OFFSET, and is of size
   ZA_PT_ZA_SIZE(vq, flags).

   The ZA array is stored as a sequence of horizontal vectors ZAV of SVL/8
   bytes each, starting from vector 0.

   Additional data might be appended in the future.

   The ZA matrix is represented in memory in an endianness-invariant layout
   which differs from the layout used for the FPSIMD V-registers on big-endian
   systems: see sigcontext.h for more explanation.  */

#define ZA_PT_ZAV_OFFSET(vq, n)				\
	(ZA_PT_ZA_OFFSET + ((vq * SVE_VQ_BYTES) * n))

#define ZA_PT_ZA_SIZE(vq) ((vq * SVE_VQ_BYTES) * (vq * SVE_VQ_BYTES))

#define ZA_PT_SIZE(vq)			      \
	(ZA_PT_ZA_OFFSET + ZA_PT_ZA_SIZE(vq))
#endif /* ZA_PT_SIZE */

#endif /* NAT_AARCH64_SCALABLE_LINUX_SIGCONTEXT_H */

/* Target-dependent definitions for AMD64.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.
   Contributed by Jiri Smid, SuSE Labs.

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

#ifndef AMD64_TDEP_H
#define AMD64_TDEP_H

struct gdbarch;
class frame_info_ptr;
struct regcache;

#include "i386-tdep.h"
#include "infrun.h"

/* Register numbers of various important registers.  */

enum amd64_regnum
{
  AMD64_RAX_REGNUM,		/* %rax */
  AMD64_RBX_REGNUM,		/* %rbx */
  AMD64_RCX_REGNUM,		/* %rcx */
  AMD64_RDX_REGNUM,		/* %rdx */
  AMD64_RSI_REGNUM,		/* %rsi */
  AMD64_RDI_REGNUM,		/* %rdi */
  AMD64_RBP_REGNUM,		/* %rbp */
  AMD64_RSP_REGNUM,		/* %rsp */
  AMD64_R8_REGNUM,		/* %r8 */
  AMD64_R9_REGNUM,		/* %r9 */
  AMD64_R10_REGNUM,		/* %r10 */
  AMD64_R11_REGNUM,		/* %r11 */
  AMD64_R12_REGNUM,		/* %r12 */
  AMD64_R13_REGNUM,		/* %r13 */
  AMD64_R14_REGNUM,		/* %r14 */
  AMD64_R15_REGNUM,		/* %r15 */
  AMD64_RIP_REGNUM,		/* %rip */
  AMD64_EFLAGS_REGNUM,		/* %eflags */
  AMD64_CS_REGNUM,		/* %cs */
  AMD64_SS_REGNUM,		/* %ss */
  AMD64_DS_REGNUM,		/* %ds */
  AMD64_ES_REGNUM,		/* %es */
  AMD64_FS_REGNUM,		/* %fs */
  AMD64_GS_REGNUM,		/* %gs */
  AMD64_ST0_REGNUM = 24,	/* %st0 */
  AMD64_ST1_REGNUM,		/* %st1 */
  AMD64_FCTRL_REGNUM = AMD64_ST0_REGNUM + 8,
  AMD64_FSTAT_REGNUM = AMD64_ST0_REGNUM + 9,
  AMD64_FTAG_REGNUM = AMD64_ST0_REGNUM + 10,
  AMD64_XMM0_REGNUM = 40,	/* %xmm0 */
  AMD64_XMM1_REGNUM,		/* %xmm1 */
  AMD64_MXCSR_REGNUM = AMD64_XMM0_REGNUM + 16,
  AMD64_YMM0H_REGNUM,		/* %ymm0h */
  AMD64_YMM15H_REGNUM = AMD64_YMM0H_REGNUM + 15,
  AMD64_BND0R_REGNUM = AMD64_YMM15H_REGNUM + 1,
  AMD64_BND3R_REGNUM = AMD64_BND0R_REGNUM + 3,
  AMD64_BNDCFGU_REGNUM,
  AMD64_BNDSTATUS_REGNUM,
  AMD64_XMM16_REGNUM,
  AMD64_XMM31_REGNUM = AMD64_XMM16_REGNUM + 15,
  AMD64_YMM16H_REGNUM,
  AMD64_YMM31H_REGNUM = AMD64_YMM16H_REGNUM + 15,
  AMD64_K0_REGNUM,
  AMD64_K7_REGNUM = AMD64_K0_REGNUM + 7,
  AMD64_ZMM0H_REGNUM,
  AMD64_ZMM31H_REGNUM = AMD64_ZMM0H_REGNUM + 31,
  AMD64_PKRU_REGNUM,
  AMD64_FSBASE_REGNUM,
  AMD64_GSBASE_REGNUM
};

/* Number of general purpose registers.  */
#define AMD64_NUM_GREGS		24

#define AMD64_NUM_REGS		(AMD64_GSBASE_REGNUM + 1)

extern displaced_step_copy_insn_closure_up amd64_displaced_step_copy_insn
  (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to,
   struct regcache *regs);
extern void amd64_displaced_step_fixup
  (struct gdbarch *gdbarch, displaced_step_copy_insn_closure *closure,
   CORE_ADDR from, CORE_ADDR to, struct regcache *regs, bool completed_p);

/* Initialize the ABI for amd64.  Uses DEFAULT_TDESC as fallback
   tdesc, if INFO does not specify one.  */
extern void amd64_init_abi (struct gdbarch_info info,
			    struct gdbarch *gdbarch,
			    const target_desc *default_tdesc);

/* Initialize the ABI for x32.  Uses DEFAULT_TDESC as fallback tdesc,
   if INFO does not specify one.  */
extern void amd64_x32_init_abi (struct gdbarch_info info,
				struct gdbarch *gdbarch,
				const target_desc *default_tdesc);
extern const struct target_desc *amd64_target_description (uint64_t xcr0,
							   bool segments);

/* Fill register REGNUM in REGCACHE with the appropriate
   floating-point or SSE register value from *FXSAVE.  If REGNUM is
   -1, do this for all registers.  This function masks off any of the
   reserved bits in *FXSAVE.  */

extern void amd64_supply_fxsave (struct regcache *regcache, int regnum,
				 const void *fxsave);

/* Similar to amd64_supply_fxsave, but use XSAVE extended state.  */
extern void amd64_supply_xsave (struct regcache *regcache, int regnum,
				const void *xsave);

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value from REGCACHE.  If REGNUM is -1, do this for
   all registers.  This function doesn't touch any of the reserved
   bits in *FXSAVE.  */

extern void amd64_collect_fxsave (const struct regcache *regcache, int regnum,
				  void *fxsave);
/* Similar to amd64_collect_fxsave, but use XSAVE extended state.  */
extern void amd64_collect_xsave (const struct regcache *regcache,
				 int regnum, void *xsave, int gcore);

/* Floating-point register set. */
extern const struct regset amd64_fpregset;

/* Variables exported from amd64-linux-tdep.c.  */
extern int amd64_linux_gregset_reg_offset[];

/* Variables exported from amd64-netbsd-tdep.c.  */
extern int amd64nbsd_r_reg_offset[];

/* Variables exported from amd64-obsd-tdep.c.  */
extern int amd64obsd_r_reg_offset[];

#endif /* amd64-tdep.h */

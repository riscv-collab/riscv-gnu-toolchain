/* Target-dependent code for the i386.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#ifndef I386_TDEP_H
#define I386_TDEP_H

#include "gdbarch.h"
#include "infrun.h"
#include "expression.h"
#include "gdbsupport/x86-xstate.h"

class frame_info_ptr;
struct gdbarch;
struct reggroup;
struct regset;
struct regcache;

/* GDB's i386 target supports both the 32-bit Intel Architecture
   (IA-32) and the 64-bit AMD x86-64 architecture.  Internally it uses
   a similar register layout for both.

   - General purpose registers
   - FPU data registers
   - FPU control registers
   - SSE data registers
   - SSE control register

   The general purpose registers for the x86-64 architecture are quite
   different from IA-32.  Therefore, gdbarch_fp0_regnum
   determines the register number at which the FPU data registers
   start.  The number of FPU data and control registers is the same
   for both architectures.  The number of SSE registers however,
   differs and is determined by the num_xmm_regs member of `struct
   gdbarch_tdep'.  */

/* Convention for returning structures.  */

enum struct_return
{
  pcc_struct_return,		/* Return "short" structures in memory.  */
  reg_struct_return		/* Return "short" structures in registers.  */
};

/* i386 architecture specific information.  */
struct i386_gdbarch_tdep : gdbarch_tdep_base
{
  /* General-purpose registers.  */
  int *gregset_reg_offset = 0;
  int gregset_num_regs = 0;
  size_t sizeof_gregset = 0;

  /* Floating-point registers.  */
  size_t sizeof_fpregset = 0;

  /* Register number for %st(0).  The register numbers for the other
     registers follow from this one.  Set this to -1 to indicate the
     absence of an FPU.  */
  int st0_regnum = 0;

  /* Number of MMX registers.  */
  int num_mmx_regs = 0;

  /* Register number for %mm0.  Set this to -1 to indicate the absence
     of MMX support.  */
  int mm0_regnum = 0;

  /* Number of pseudo YMM registers.  */
  int num_ymm_regs = 0;

  /* Register number for %ymm0.  Set this to -1 to indicate the absence
     of pseudo YMM register support.  */
  int ymm0_regnum = 0;

  /* Number of AVX512 OpMask registers (K-registers)  */
  int num_k_regs = 0;

  /* Register number for %k0.  Set this to -1 to indicate the absence
     of AVX512 OpMask register support.  */
  int k0_regnum = 0;

  /* Number of pseudo ZMM registers ($zmm0-$zmm31).  */
  int num_zmm_regs = 0;

  /* Register number for %zmm0.  Set this to -1 to indicate the absence
     of pseudo ZMM register support.  */
  int zmm0_regnum = 0;

  /* Number of byte registers.  */
  int num_byte_regs = 0;

  /* Register pseudo number for %al.  */
  int al_regnum = 0;

  /* Number of pseudo word registers.  */
  int num_word_regs = 0;

  /* Register number for %ax.  */
  int ax_regnum = 0;

  /* Number of pseudo dword registers.  */
  int num_dword_regs = 0;

  /* Register number for %eax.  Set this to -1 to indicate the absence
     of pseudo dword register support.  */
  int eax_regnum = 0;

  /* Number of core registers.  */
  int num_core_regs = 0;

  /* Number of SSE registers.  */
  int num_xmm_regs = 0;

  /* Number of SSE registers added in AVX512.  */
  int num_xmm_avx512_regs = 0;

  /* Register number of XMM16, the first XMM register added in AVX512.  */
  int xmm16_regnum = 0;

  /* Number of YMM registers added in AVX512.  */
  int num_ymm_avx512_regs = 0;

  /* Register number of YMM16, the first YMM register added in AVX512.  */
  int ymm16_regnum = 0;

  /* Bits of the extended control register 0 (the XFEATURE_ENABLED_MASK
     register), excluding the x87 bit, which are supported by this GDB.  */

  uint64_t xcr0 = 0;

  /* Offset of XCR0 in XSAVE extended state.  */
  int xsave_xcr0_offset = 0;

  /* Layout of the XSAVE area extended region.  */
  x86_xsave_layout xsave_layout;

  /* Register names.  */
  const char * const *register_names = nullptr;

  /* Register number for %ymm0h.  Set this to -1 to indicate the absence
     of upper YMM register support.  */
  int ymm0h_regnum = 0;

  /* Upper YMM register names.  Only used for tdesc_numbered_register.  */
  const char * const *ymmh_register_names = nullptr;

  /* Register number for %ymm16h.  Set this to -1 to indicate the absence
  of support for YMM16-31.  */
  int ymm16h_regnum = 0;

  /* YMM16-31 register names.  Only used for tdesc_numbered_register.  */
  const char * const *ymm16h_register_names = nullptr;

  /* Register number for %bnd0r.  Set this to -1 to indicate the absence
     bound registers.  */
  int bnd0r_regnum = 0;

  /* Register number for pseudo register %bnd0.  Set this to -1 to indicate the absence
     bound registers.  */
  int bnd0_regnum = 0;

  /* Register number for %bndcfgu. Set this to -1 to indicate the absence
     bound control registers.  */
  int bndcfgu_regnum = 0;

  /* MPX register names.  Only used for tdesc_numbered_register.  */
  const char * const *mpx_register_names = nullptr;

  /* Register number for %zmm0h.  Set this to -1 to indicate the absence
     of ZMM_HI256 register support.  */
  int zmm0h_regnum = 0;

  /* OpMask register names.  */
  const char * const *k_register_names = nullptr;

  /* ZMM register names.  Only used for tdesc_numbered_register.  */
  const char * const *zmmh_register_names = nullptr;

  /* XMM16-31 register names.  Only used for tdesc_numbered_register.  */
  const char * const *xmm_avx512_register_names = nullptr;

  /* YMM16-31 register names.  Only used for tdesc_numbered_register.  */
  const char * const *ymm_avx512_register_names = nullptr;

  /* Number of PKEYS registers.  */
  int num_pkeys_regs = 0;

  /* Register number for PKRU register.  */
  int pkru_regnum = 0;

  /* PKEYS register names.  */
  const char * const *pkeys_register_names = nullptr;

  /* Register number for %fsbase.  Set this to -1 to indicate the
     absence of segment base registers.  */
  int fsbase_regnum = 0;

  /* Target description.  */
  const struct target_desc *tdesc = nullptr;

  /* Register group function.  */
  gdbarch_register_reggroup_p_ftype *register_reggroup_p = nullptr;

  /* Offset of saved PC in jmp_buf.  */
  int jb_pc_offset = 0;

  /* Convention for returning structures.  */
  enum struct_return struct_return {};

  /* Address range where sigtramp lives.  */
  CORE_ADDR sigtramp_start = 0;
  CORE_ADDR sigtramp_end = 0;

  /* Detect sigtramp.  */
  int (*sigtramp_p) (frame_info_ptr) = nullptr;

  /* Get address of sigcontext for sigtramp.  */
  CORE_ADDR (*sigcontext_addr) (frame_info_ptr) = nullptr;

  /* Offset of registers in `struct sigcontext'.  */
  int *sc_reg_offset = 0;
  int sc_num_regs = 0;

  /* Offset of saved PC and SP in `struct sigcontext'.  Usage of these
     is deprecated, please use `sc_reg_offset' instead.  */
  int sc_pc_offset = 0;
  int sc_sp_offset = 0;

  /* ISA-specific data types.  */
  struct type *i386_mmx_type = nullptr;
  struct type *i386_ymm_type = nullptr;
  struct type *i386_zmm_type = nullptr;
  struct type *i387_ext_type = nullptr;
  struct type *i386_bnd_type = nullptr;

  /* Process record/replay target.  */
  /* The map for registers because the AMD64's registers order
     in GDB is not same as I386 instructions.  */
  const int *record_regmap = nullptr;
  /* Parse intx80 args.  */
  int (*i386_intx80_record) (struct regcache *regcache) = nullptr;
  /* Parse sysenter args.  */
  int (*i386_sysenter_record) (struct regcache *regcache) = nullptr;
  /* Parse syscall args.  */
  int (*i386_syscall_record) (struct regcache *regcache) = nullptr;

  /* Regsets. */
  const struct regset *fpregset = nullptr;
};

/* Floating-point registers.  */

/* All FPU control registers (except for FIOFF and FOOFF) are 16-bit
   (at most) in the FPU, but are zero-extended to 32 bits in GDB's
   register cache.  */

/* Return non-zero if REGNUM matches the FP register and the FP
   register set is active.  */
extern int i386_fp_regnum_p (struct gdbarch *, int);
extern int i386_fpc_regnum_p (struct gdbarch *, int);

/* Register numbers of various important registers.  */

enum i386_regnum
{
  I386_EAX_REGNUM,		/* %eax */
  I386_ECX_REGNUM,		/* %ecx */
  I386_EDX_REGNUM,		/* %edx */
  I386_EBX_REGNUM,		/* %ebx */
  I386_ESP_REGNUM,		/* %esp */
  I386_EBP_REGNUM,		/* %ebp */
  I386_ESI_REGNUM,		/* %esi */
  I386_EDI_REGNUM,		/* %edi */
  I386_EIP_REGNUM,		/* %eip */
  I386_EFLAGS_REGNUM,		/* %eflags */
  I386_CS_REGNUM,		/* %cs */
  I386_SS_REGNUM,		/* %ss */
  I386_DS_REGNUM,		/* %ds */
  I386_ES_REGNUM,		/* %es */
  I386_FS_REGNUM,		/* %fs */
  I386_GS_REGNUM,		/* %gs */
  I386_ST0_REGNUM,		/* %st(0) */
  I386_MXCSR_REGNUM = 40,	/* %mxcsr */
  I386_YMM0H_REGNUM,		/* %ymm0h */
  I386_YMM7H_REGNUM = I386_YMM0H_REGNUM + 7,
  I386_BND0R_REGNUM,
  I386_BND3R_REGNUM = I386_BND0R_REGNUM + 3,
  I386_BNDCFGU_REGNUM,
  I386_BNDSTATUS_REGNUM,
  I386_K0_REGNUM,		/* %k0 */
  I386_K7_REGNUM = I386_K0_REGNUM + 7,
  I386_ZMM0H_REGNUM,		/* %zmm0h */
  I386_ZMM7H_REGNUM = I386_ZMM0H_REGNUM + 7,
  I386_PKRU_REGNUM,
  I386_FSBASE_REGNUM,
  I386_GSBASE_REGNUM
};

/* Register numbers of RECORD_REGMAP.  */

enum record_i386_regnum
{
  X86_RECORD_REAX_REGNUM,
  X86_RECORD_RECX_REGNUM,
  X86_RECORD_REDX_REGNUM,
  X86_RECORD_REBX_REGNUM,
  X86_RECORD_RESP_REGNUM,
  X86_RECORD_REBP_REGNUM,
  X86_RECORD_RESI_REGNUM,
  X86_RECORD_REDI_REGNUM,
  X86_RECORD_R8_REGNUM,
  X86_RECORD_R9_REGNUM,
  X86_RECORD_R10_REGNUM,
  X86_RECORD_R11_REGNUM,
  X86_RECORD_R12_REGNUM,
  X86_RECORD_R13_REGNUM,
  X86_RECORD_R14_REGNUM,
  X86_RECORD_R15_REGNUM,
  X86_RECORD_REIP_REGNUM,
  X86_RECORD_EFLAGS_REGNUM,
  X86_RECORD_CS_REGNUM,
  X86_RECORD_SS_REGNUM,
  X86_RECORD_DS_REGNUM,
  X86_RECORD_ES_REGNUM,
  X86_RECORD_FS_REGNUM,
  X86_RECORD_GS_REGNUM,
};

#define I386_NUM_GREGS	16
#define I386_NUM_XREGS  9

#define I386_SSE_NUM_REGS	(I386_MXCSR_REGNUM + 1)
#define I386_AVX_NUM_REGS	(I386_YMM7H_REGNUM + 1)
#define I386_MPX_NUM_REGS	(I386_BNDSTATUS_REGNUM + 1)
#define I386_AVX512_NUM_REGS	(I386_ZMM7H_REGNUM + 1)
#define I386_PKEYS_NUM_REGS	(I386_PKRU_REGNUM + 1)
#define I386_NUM_REGS		(I386_GSBASE_REGNUM + 1)

/* Size of the largest register.  */
#define I386_MAX_REGISTER_SIZE	64

/* Types for i386-specific registers.  */
extern struct type *i387_ext_type (struct gdbarch *gdbarch);

/* Checks of different registers.  */
extern int i386_byte_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_word_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_dword_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_xmm_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_xmm_avx512_regnum_p (struct gdbarch * gdbarch, int regnum);
extern int i386_ymm_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_ymm_avx512_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_bnd_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_k_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_zmm_regnum_p (struct gdbarch *gdbarch, int regnum);
extern int i386_zmmh_regnum_p (struct gdbarch *gdbarch, int regnum);
extern bool i386_pkru_regnum_p (struct gdbarch *gdbarch, int regnum);

extern const char *i386_pseudo_register_name (struct gdbarch *gdbarch,
					      int regnum);
extern struct type *i386_pseudo_register_type (struct gdbarch *gdbarch,
					       int regnum);

extern value *i386_pseudo_register_read_value (gdbarch *gdbarch,
					       frame_info_ptr next_frame,
					       int regnum);

extern void i386_pseudo_register_write (gdbarch *gdbarch,
					frame_info_ptr next_frame, int regnum,
					gdb::array_view<const gdb_byte> buf);

extern int i386_ax_pseudo_register_collect (struct gdbarch *gdbarch,
					    struct agent_expr *ax,
					    int regnum);

/* Segment selectors.  */
#define I386_SEL_RPL	0x0003  /* Requester's Privilege Level mask.  */
#define I386_SEL_UPL	0x0003	/* User Privilege Level.  */
#define I386_SEL_KPL	0x0000	/* Kernel Privilege Level.  */

/* The length of the longest i386 instruction (according to
   include/asm-i386/kprobes.h in Linux 2.6.  */
#define I386_MAX_INSN_LEN (16)

/* Functions exported from i386-tdep.c.  */
extern CORE_ADDR i386_pe_skip_trampoline_code (frame_info_ptr frame,
					       CORE_ADDR pc, char *name);
extern CORE_ADDR i386_skip_main_prologue (struct gdbarch *gdbarch,
					  CORE_ADDR pc);

/* The "push_dummy_call" gdbarch method, optionally with the thiscall
   calling convention.  */
extern CORE_ADDR i386_thiscall_push_dummy_call (struct gdbarch *gdbarch,
						struct value *function,
						struct regcache *regcache,
						CORE_ADDR bp_addr,
						int nargs, struct value **args,
						CORE_ADDR sp,
						function_call_return_method
						return_method,
						CORE_ADDR struct_addr,
						bool thiscall);

/* Return whether the THIS_FRAME corresponds to a sigtramp routine.  */
extern int i386_sigtramp_p (frame_info_ptr this_frame);

/* Return non-zero if REGNUM is a member of the specified group.  */
extern int i386_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				     const struct reggroup *group);

/* Supply register REGNUM from the general-purpose register set REGSET
   to register cache REGCACHE.  If REGNUM is -1, do this for all
   registers in REGSET.  */
extern void i386_supply_gregset (const struct regset *regset,
				 struct regcache *regcache, int regnum,
				 const void *gregs, size_t len);

/* General-purpose register set. */
extern const struct regset i386_gregset;

/* Floating-point register set. */
extern const struct regset i386_fpregset;

/* Default iterator over core file register note sections.  */
extern void
  i386_iterate_over_regset_sections (struct gdbarch *gdbarch,
				     iterate_over_regset_sections_cb *cb,
				     void *cb_data,
				     const struct regcache *regcache);

typedef buf_displaced_step_copy_insn_closure
  i386_displaced_step_copy_insn_closure;

extern displaced_step_copy_insn_closure_up i386_displaced_step_copy_insn
  (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to,
   struct regcache *regs);
extern void i386_displaced_step_fixup
  (struct gdbarch *gdbarch, displaced_step_copy_insn_closure *closure,
   CORE_ADDR from, CORE_ADDR to, regcache *regs, bool completed_p);

/* Initialize a basic ELF architecture variant.  */
extern void i386_elf_init_abi (struct gdbarch_info, struct gdbarch *);

/* Initialize a SVR4 architecture variant.  */
extern void i386_svr4_init_abi (struct gdbarch_info, struct gdbarch *);

/* Convert SVR4 register number REG to the appropriate register number
   used by GDB.  */
extern int i386_svr4_reg_to_regnum (struct gdbarch *gdbarch, int reg);

extern int i386_process_record (struct gdbarch *gdbarch,
				struct regcache *regcache, CORE_ADDR addr);
extern const struct target_desc *i386_target_description (uint64_t xcr0,
							  bool segments);

/* Return true iff the current target is MPX enabled.  */
extern int i386_mpx_enabled (void);


/* Functions and variables exported from i386-bsd-tdep.c.  */

extern void i386bsd_init_abi (struct gdbarch_info, struct gdbarch *);
extern CORE_ADDR i386obsd_sigtramp_start_addr;
extern CORE_ADDR i386obsd_sigtramp_end_addr;
extern int i386obsd_sc_reg_offset[];
extern int i386bsd_sc_reg_offset[];

/* SystemTap related functions.  */

extern int i386_stap_is_single_operand (struct gdbarch *gdbarch,
					const char *s);

extern expr::operation_up i386_stap_parse_special_token
     (struct gdbarch *gdbarch, struct stap_parse_info *p);

#endif /* i386-tdep.h */

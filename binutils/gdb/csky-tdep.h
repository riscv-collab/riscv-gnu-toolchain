/* Target-dependent code for the CSKY architecture, for GDB.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

#ifndef CSKY_TDEP_H
#define CSKY_TDEP_H

#include "gdbarch.h"

/* How to interpret the contents of the link register.  */
enum lr_type_t
{
  LR_TYPE_R15,
  LR_TYPE_EPC,
  LR_TYPE_FPC
};

/* Target-dependent structure in gdbarch.  */
struct csky_gdbarch_tdep : gdbarch_tdep_base
{
  /* Save FPU, VDSP ABI.  */
  unsigned int fpu_abi;
  unsigned int fpu_hardfp;
  unsigned int vdsp_version;

  /* Save fv_pseudo_registers_count.  */
  unsigned int has_vr0;
  unsigned int fv_pseudo_registers_count;
};

/* Instruction sizes.  */
enum csky_insn_size_t
{
  CSKY_INSN_SIZE16 = 2,
  CSKY_INSN_SIZE32 = 4
};

/* CSKY register numbers.  */
enum csky_regnum
{
  CSKY_R0_REGNUM = 0, /* General registers.  */
  CSKY_R15_REGNUM = 15,
  CSKY_HI_REGNUM = 36,
  CSKY_LO_REGNUM = 37,
  CSKY_PC_REGNUM = 72,
  CSKY_AR0_REGNUM = 73,
  CSKY_CR0_REGNUM = 89,
  CSKY_VBR_REGNUM = CSKY_CR0_REGNUM + 1,
  CSKY_EPSR_REGNUM = CSKY_CR0_REGNUM + 2,
  CSKY_FPSR_REGNUM = CSKY_CR0_REGNUM + 3,
  CSKY_EPC_REGNUM = CSKY_CR0_REGNUM + 4,
  CSKY_FPC_REGNUM = CSKY_CR0_REGNUM + 5,

  /* Float register 0.  */
  CSKY_FR0_REGNUM = 40,
  CSKY_FR16_REGNUM = 1172,
  CSKY_FCR_REGNUM = 121,
  CSKY_MMU_REGNUM = 128,
  CSKY_PROFCR_REGNUM = 140,
  CSKY_PROFGR_REGNUM = 144,
  CSKY_FP_REGNUM = 8,

  /* Vector register 0.  */
  CSKY_VR0_REGNUM = 56,

  /* m32r calling convention.  */
  CSKY_SP_REGNUM = CSKY_R0_REGNUM + 14,
  CSKY_RET_REGNUM = CSKY_R0_REGNUM,

  /* Argument registers.  */
  CSKY_ABI_A0_REGNUM = 0,
  CSKY_ABI_LAST_ARG_REGNUM = 3,

  /* Link register, r15.  */
  CSKY_LR_REGNUM = CSKY_R15_REGNUM,

  /* Processor status register, cr0.  */
  CSKY_PSR_REGNUM = CSKY_CR0_REGNUM,

  CSKY_MAX_REGISTER_SIZE = 16,

  /* Actually, the max regs number should be 1187. But if the
     gdb stub does not send a tdesc-xml file to gdb, 253 works. */
  CSKY_MAX_REGS = 253
};

/* ICE registers.  */
#define CSKY_CRBANK_NUM_REGS 32

/* Tdesc registers essential check.  */
#define CSKY_TDESC_REGS_PC_NUMBERED             (1 << 0)
#define CSKY_TDESC_REGS_SP_NUMBERED             (1 << 1)
#define CSKY_TDESC_REGS_LR_NUMBERED             (1 << 2)
#define CSKY_TDESC_REGS_ESSENTIAL_VALUE         (7)

/* For fr0~fr15, fr16~fr31, vr0~vr15 check.  */
#define CSKY_FULL16_ONEHOT_VALUE   0xffff

/* Define for CSKY FV pseudo regs for dwarf regs. */
#define FV_PSEUDO_REGNO_FIRST  74
#define FV_PSEUDO_REGNO_LAST   201

/* Number of processor registers w/o ICE registers.  */
#define CSKY_NUM_REGS (CSKY_MAX_REGS - CSKY_CRBANK_NUM_REGS)

/* size.  */
#define CSKY_16_ST_SIZE(insn) (1 << ((insn & 0x1800) >> 11))
/* rx.  */
#define CSKY_16_ST_ADDR_REGNUM(insn) ((insn & 0x700) >> 8)
/* disp.  */
#define CSKY_16_ST_OFFSET(insn) ((insn & 0x1f) << ((insn & 0x1800) >> 11))
/* ry.  */
#define CSKY_16_ST_VAL_REGNUM(insn) ((insn & 0xe0) >> 5)

/* st16.w rz, (sp, disp).  */
#define CSKY_16_IS_STWx0(insn) ((insn & 0xf800) == 0xb800)
#define CSKY_16_STWx0_VAL_REGNUM(insn) CSKY_16_ST_ADDR_REGNUM (insn)

/* disp.  */
#define CSKY_16_STWx0_OFFSET(insn) \
  ((((insn & 0x700) >> 3) + (insn & 0x1f)) << 2)

/* Check ld16 but not ld16 sp.  */
#define CSKY_16_IS_LD(insn) \
  (((insn & 0xe000) == 0x8000) && (insn & 0x1800) != 0x1800)
/* size.  */
#define CSKY_16_LD_SIZE(insn) CSKY_16_ST_SIZE (insn)
/* rx.  */
#define CSKY_16_LD_ADDR_REGNUM(insn) CSKY_16_ST_ADDR_REGNUM (insn)
/* disp.  */
#define CSKY_16_LD_OFFSET(insn) CSKY_16_ST_OFFSET (insn)

/* ld16.w rz,(sp,disp).  */
#define CSKY_16_IS_LDWx0(insn) ((insn & 0xf800) == 0x9800)
/*disp.  */
#define CSKY_16_LDWx0_OFFSET(insn) CSKY_16_STWx0_OFFSET (insn)

/* st32.b/h/w/d.  */
#define CSKY_32_IS_ST(insn) ((insn & 0xfc00c000) == 0xdc000000)

/* size: b/h/w/d.  */
#define CSKY_32_ST_SIZE(insn) (1 << ((insn & 0x3000) >> 12))
/* rx.  */
#define CSKY_32_ST_ADDR_REGNUM(insn) ((insn & 0x001f0000) >> 16)
/* disp.  */
#define CSKY_32_ST_OFFSET(insn) ((insn & 0xfff) << ((insn & 0x3000) >> 12))
/* ry.  */
#define CSKY_32_ST_VAL_REGNUM(insn) ((insn & 0x03e00000) >> 21)

/* stw ry, (sp, disp).  */
#define CSKY_32_IS_STWx0(insn) ((insn & 0xfc1ff000) == 0xdc0e2000)

/* stm32 ry-rz, (rx).  */
#define CSKY_32_IS_STM(insn) ((insn & 0xfc00ffe0) == 0xd4001c20)
/* rx.  */
#define CSKY_32_STM_ADDR_REGNUM(insn) CSKY_32_ST_ADDR_REGNUM (insn)
/* Count of registers.  */
#define CSKY_32_STM_SIZE(insn) (insn & 0x1f)
/* ry.  */
#define CSKY_32_STM_VAL_REGNUM(insn) ((insn & 0x03e00000) >> 21)
/* stm32 ry-rz, (sp).  */
#define CSKY_32_IS_STMx0(insn) ((insn & 0xfc1fffe0) == 0xd40e1c20)

/* str32.b/h/w rz, (rx, ry << offset).  */
#define CSKY_32_IS_STR(insn) \
  (((insn & 0xfc000000) == 0xd4000000) && !(CSKY_32_IS_STM (insn)))
/* rx.  */
#define CSKY_32_STR_X_REGNUM(insn) CSKY_32_ST_ADDR_REGNUM (insn)
/* ry.  */
#define CSKY_32_STR_Y_REGNUM(insn) ((insn >> 21) & 0x1f)
/* size: b/h/w.  */
#define CSKY_32_STR_SIZE(insn) (1 << ((insn & 0x0c00) >> 10))
/* imm (for rx + ry * imm).  */
#define CSKY_32_STR_OFFSET(insn) ((insn & 0x000003e0) >> 5)

/* stex32.w rz, (rx, disp).  */
#define CSKY_32_IS_STEX(insn) ((insn & 0xfc00f000) == 0xdc007000)
/* rx.  */
#define CSKY_32_STEX_ADDR_REGNUM(insn) ((insn & 0x1f0000) >> 16)
/* disp.  */
#define CSKY_32_STEX_OFFSET(insn) ((insn & 0x0fff) << 2)

/* ld.b/h/w.  */
#define CSKY_32_IS_LD(insn) ((insn & 0xfc00c000) == 0xd8000000)
/* size.  */
#define CSKY_32_LD_SIZE(insn) CSKY_32_ST_SIZE (insn)
/* rx.  */
#define CSKY_32_LD_ADDR_REGNUM(insn) CSKY_32_ST_ADDR_REGNUM (insn)
/* disp.  */
#define CSKY_32_LD_OFFSET(insn) CSKY_32_ST_OFFSET (insn)
#define CSKY_32_IS_LDM(insn) ((insn & 0xfc00ffe0) == 0xd0001c20)
/* rx.  */
#define CSKY_32_LDM_ADDR_REGNUM(insn) CSKY_32_STM_ADDR_REGNUM (insn)
/* Count of registers.  */
#define CSKY_32_LDM_SIZE(insn) CSKY_32_STM_SIZE (insn)

/* ldr32.b/h/w rz, (rx, ry << offset).  */
#define CSKY_32_IS_LDR(insn) \
  (((insn & 0xfc00fe00) == 0xd0000000) && !(CSKY_32_IS_LDM (insn)))
/* rx.  */
#define CSKY_32_LDR_X_REGNUM(insn) CSKY_32_STR_X_REGNUM (insn)
/* ry.  */
#define CSKY_32_LDR_Y_REGNUM(insn) CSKY_32_STR_Y_REGNUM (insn)
/* size: b/h/w.  */
#define CSKY_32_LDR_SIZE(insn) CSKY_32_STR_SIZE (insn)
/* imm (for rx + ry*imm).  */
#define CSKY_32_LDR_OFFSET(insn) CSKY_32_STR_OFFSET (insn)

#define CSKY_32_IS_LDEX(insn) ((insn & 0xfc00f000) == 0xd8007000)
/* rx.  */
#define CSKY_32_LDEX_ADDR_REGNUM(insn) CSKY_32_STEX_ADDR_REGNUM (insn)
/* disp.  */
#define CSKY_32_LDEX_OFFSET(insn) CSKY_32_STEX_OFFSET (insn)

/* subi.sp sp, disp.  */
#define CSKY_16_IS_SUBI0(insn) ((insn & 0xfce0) == 0x1420)
/* disp.  */
#define CSKY_16_SUBI_IMM(insn) ((((insn & 0x300) >> 3) + (insn & 0x1f)) << 2)

/* subi32 sp,sp,oimm12.  */
#define CSKY_32_IS_SUBI0(insn) ((insn & 0xfffff000) == 0xe5ce1000)
/* oimm12.  */
#define CSKY_32_SUBI_IMM(insn) ((insn & 0xfff) + 1)

/* push16.  */
#define CSKY_16_IS_PUSH(insn) ((insn & 0xffe0) == 0x14c0)
#define CSKY_16_IS_PUSH_R15(insn) ((insn & 0x10) == 0x10)
#define CSKY_16_PUSH_LIST1(insn) (insn & 0xf) /* r4 - r11.  */

/* pop16.  */
#define CSKY_16_IS_POP(insn) ((insn & 0xffe0) == 0x1480)
#define CSKY_16_IS_POP_R15(insn) CSKY_16_IS_PUSH_R15 (insn)
#define CSKY_16_POP_LIST1(insn) CSKY_16_PUSH_LIST1 (insn) /* r4 - r11.  */

/* push32.  */
#define CSKY_32_IS_PUSH(insn) ((insn & 0xfffffe00) == 0xebe00000)
#define CSKY_32_IS_PUSH_R29(insn) ((insn & 0x100) == 0x100)
#define CSKY_32_IS_PUSH_R15(insn) ((insn & 0x10) == 0x10)
#define CSKY_32_PUSH_LIST1(insn) (insn & 0xf)	 /* r4 - r11.  */
#define CSKY_32_PUSH_LIST2(insn) ((insn & 0xe0) >> 5) /* r16 - r17.  */

/* pop32.  */
#define CSKY_32_IS_POP(insn) ((insn & 0xfffffe00) == 0xebc00000)
#define CSKY_32_IS_POP_R29(insn) CSKY_32_IS_PUSH_R29 (insn)
#define CSKY_32_IS_POP_R15(insn) CSKY_32_IS_PUSH_R15 (insn)
#define CSKY_32_POP_LIST1(insn) CSKY_32_PUSH_LIST1 (insn) /* r4 - r11.  */
#define CSKY_32_POP_LIST2(insn) CSKY_32_PUSH_LIST2 (insn) /* r16 - r17.  */

/* Adjust sp by r4(l0).  */
/* lrw r4, literal.  */
#define CSKY_16_IS_LRW4(x) (((x) &0xfce0) == 0x1080)
/* movi r4, imm8.  */
#define CSKY_16_IS_MOVI4(x) (((x) &0xff00) == 0x3400)

/* addi r4, oimm8.  */
#define CSKY_16_IS_ADDI4(x) (((x) &0xff00) == 0x2400)
/* subi r4, oimm8.  */
#define CSKY_16_IS_SUBI4(x) (((x) &0xff00) == 0x2c00)

/* nor16 r4, r4.  */
#define CSKY_16_IS_NOR4(x) ((x) == 0x6d12)

/* lsli r4, r4, imm5.  */
#define CSKY_16_IS_LSLI4(x) (((x) &0xffe0) == 0x4480)
/* bseti r4, imm5.  */
#define CSKY_16_IS_BSETI4(x) (((x) &0xffe0) == 0x3ca0)
/* bclri r4, imm5.  */
#define CSKY_16_IS_BCLRI4(x) (((x) &0xffe0) == 0x3c80)

/* subu sp, r4.  */
#define CSKY_16_IS_SUBU4(x) ((x) == 0x6392)

#define CSKY_16_IS_R4_ADJUSTER(x) \
  (CSKY_16_IS_ADDI4 (x) || CSKY_16_IS_SUBI4 (x) || CSKY_16_IS_BSETI4 (x) \
   || CSKY_16_IS_BCLRI4 (x) || CSKY_16_IS_NOR4 (x) || CSKY_16_IS_LSLI4 (x))

/* lrw r4, literal.  */
#define CSKY_32_IS_LRW4(x) (((x) &0xffff0000) == 0xea840000)
/* movi r4, imm16.  */
#define CSKY_32_IS_MOVI4(x) (((x) &0xffff0000) == 0xea040000)
/* movih r4, imm16.  */
#define CSKY_32_IS_MOVIH4(x) (((x) &0xffff0000) == 0xea240000)
/* bmaski r4, oimm5.  */
#define CSKY_32_IS_BMASKI4(x) (((x) &0xfc1fffff) == 0xc4005024)
/* addi r4, r4, oimm12.  */
#define CSKY_32_IS_ADDI4(x) (((x) &0xfffff000) == 0xe4840000)
/* subi r4, r4, oimm12.  */
#define CSKY_32_IS_SUBI4(x) (((x) &0xfffff000) == 0xe4810000)

/* nor32 r4, r4, r4.  */
#define CSKY_32_IS_NOR4(x) ((x) == 0xc4842484)
/* rotli r4, r4, imm5.  */
#define CSKY_32_IS_ROTLI4(x) (((x) &0xfc1fffff) == 0xc4044904)
/* lsli r4, r4, imm5.  */
#define CSKY_32_IS_LISI4(x) (((x) &0xfc1fffff) == 0xc4044824)
/* bseti32 r4, r4, imm5.  */
#define CSKY_32_IS_BSETI4(x) (((x) &0xfc1fffff) == 0xc4042844)
/* bclri32 r4, r4, imm5.  */
#define CSKY_32_IS_BCLRI4(x) (((x) &0xfc1fffff) == 0xc4042824)
/* ixh r4, r4, r4.  */
#define CSKY_32_IS_IXH4(x) ((x) == 0xc4840824)
/* ixw r4, r4, r4.  */
#define CSKY_32_IS_IXW4(x) ((x) == 0xc4840844)
/* subu32 sp, sp, r4.  */
#define CSKY_32_IS_SUBU4(x) ((x) == 0xc48e008e)

#define CSKY_32_IS_R4_ADJUSTER(x) \
  (CSKY_32_IS_ADDI4 (x) || CSKY_32_IS_SUBI4 (x) || CSKY_32_IS_ROTLI4 (x) \
   || CSKY_32_IS_IXH4 (x) || CSKY_32_IS_IXW4 (x) || CSKY_32_IS_NOR4 (x) \
   || CSKY_32_IS_BSETI4 (x) || CSKY_32_IS_BCLRI4 (x) || CSKY_32_IS_LISI4 (x))

#define CSKY_IS_R4_ADJUSTER(x)						\
  (CSKY_32_IS_R4_ADJUSTER (x) || CSKY_16_IS_R4_ADJUSTER (x))
#define CSKY_IS_SUBU4(x) (CSKY_32_IS_SUBU4 (x) || CSKY_16_IS_SUBU4 (x))

/* mfcr rz, epsr.  */
#define CSKY_32_IS_MFCR_EPSR(insn) ((insn & 0xffffffe0) == 0xc0026020)
/* mfcr rz, fpsr.  */
#define CSKY_32_IS_MFCR_FPSR(insn) ((insn & 0xffffffe0) == 0xc0036020)
/* mfcr rz, epc.  */
#define CSKY_32_IS_MFCR_EPC(insn) ((insn & 0xffffffe0) == 0xc0046020)
/* mfcr rz, fpc.  */
#define CSKY_32_IS_MFCR_FPC(insn) ((insn & 0xffffffe0) == 0xc0056020)

#define CSKY_32_IS_RTE(insn) (insn == 0xc0004020)
#define CSKY_32_IS_RFI(insn) (insn == 0xc0004420)
#define CSKY_32_IS_JMP(insn) ((insn & 0xffe0ffff) == 0xe8c00000)
#define CSKY_16_IS_JMP(insn) ((insn & 0xffc3) == 0x7800)
#define CSKY_32_IS_JMPI(insn) ((insn & 0xffff0000) == 0xeac00000)
#define CSKY_32_IS_JMPIX(insn) ((insn & 0xffe0fffc) == 0xe9e00000)
#define CSKY_16_IS_JMPIX(insn) ((insn & 0xf8fc) == 0x38e0)

#define CSKY_16_IS_BR(insn) ((insn & 0xfc00) == 0x0400)
#define CSKY_32_IS_BR(insn) ((insn & 0xffff0000) == 0xe8000000)
#define CSKY_16_IS_MOV_FP_SP(insn) (insn == 0x6e3b)     /* mov r8, r14.  */
#define CSKY_32_IS_MOV_FP_SP(insn) (insn == 0xc40e4828) /* mov r8, r14.  */
#define CSKY_16_IS_MOV_SP_FP(insn) (insn == 0x6fa3)     /* mov r14, r8.  */
#define CSKY_32_INSN_MASK 0xc000
#define CSKY_BKPT_INSN 0x0
#define CSKY_NUM_GREGS 32
/* 32 general regs + 4.  */
#define CSKY_NUM_GREGS_SAVED_GREGS (CSKY_NUM_GREGS + 4)

/* CSKY software bkpt write-mode.  */
#define CSKY_WR_BKPT_MODE 4

/* Define insns for parse rt_sigframe.  */
/* There are three words(sig, pinfo, puc) before siginfo.  */
#define CSKY_SIGINFO_OFFSET 0xc

/* Size of struct siginfo.  */
#define CSKY_SIGINFO_SIZE 0x80

/* There are five words(uc_flags, uc_link, and three for uc_stack)
   in struct ucontext before sigcontext.  */
#define CSKY_UCONTEXT_SIGCONTEXT 0x14

/* There is a word(sc_mask) before sc_usp.  */
#define CSKY_SIGCONTEXT_SC_USP 0x4

/* There is a word(sc_usp) before sc_a0.  */
#define CSKY_SIGCONTEXT_SC_A0 0x4

#define CSKY_MOVI_R7_173 0x00adea07
#define CSKY_TRAP_0 0x2020c000

/* Sizeof (tls) */
#define CSKY_SIGCONTEXT_PT_REGS_TLS  4

/* Macro for kernel 4.x  */
#define CSKY_MOVI_R7_139 0x008bea07

/* Macro for check long branch.  */
#define CSKY_JMPI_PC_4      0x1eac0
#define CSKY_LRW_T1_PC_8    0x2ea8d
#define CSKY_JMP_T1_VS_NOP  0x6c037834

#endif

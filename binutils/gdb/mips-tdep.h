/* Target-dependent header for the MIPS architecture, for GDB, the GNU Debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#ifndef MIPS_TDEP_H
#define MIPS_TDEP_H

#include "objfiles.h"
#include "gdbarch.h"

struct gdbarch;

/* All the possible MIPS ABIs.  */
enum mips_abi
  {
    MIPS_ABI_UNKNOWN = 0,
    MIPS_ABI_N32,
    MIPS_ABI_O32,
    MIPS_ABI_N64,
    MIPS_ABI_O64,
    MIPS_ABI_EABI32,
    MIPS_ABI_EABI64,
    MIPS_ABI_LAST
  };

/* Return the MIPS ABI associated with GDBARCH.  */
enum mips_abi mips_abi (struct gdbarch *gdbarch);

/* Base and compressed MIPS ISA variations.  */
enum mips_isa
  {
    ISA_MIPS = -1,		/* mips_compression_string depends on it.  */
    ISA_MIPS16,
    ISA_MICROMIPS
  };

/* Corresponding MSYMBOL_TARGET_FLAG aliases.  */
#define MSYMBOL_TARGET_FLAG_MIPS16(sym) \
	(sym)->target_flag_1 ()

#define SET_MSYMBOL_TARGET_FLAG_MIPS16(sym) \
	(sym)->set_target_flag_1 (true)

#define MSYMBOL_TARGET_FLAG_MICROMIPS(sym) \
	(sym)->target_flag_2 ()

#define SET_MSYMBOL_TARGET_FLAG_MICROMIPS(sym) \
	(sym)->set_target_flag_2 (true)

/* Return the MIPS ISA's register size.  Just a short cut to the BFD
   architecture's word size.  */
extern int mips_isa_regsize (struct gdbarch *gdbarch);

/* Return the current index for various MIPS registers.  */
struct mips_regnum
{
  int pc;
  int fp0;
  int fp_implementation_revision;
  int fp_control_status;
  int badvaddr;		/* Bad vaddr for addressing exception.  */
  int cause;		/* Describes last exception.  */
  int hi;		/* Multiply/divide temp.  */
  int lo;		/* ...  */
  int dspacc;		/* SmartMIPS/DSP accumulators.  */
  int dspctl;		/* DSP control.  */
};
extern const struct mips_regnum *mips_regnum (struct gdbarch *gdbarch);

/* Some MIPS boards don't support floating point while others only
   support single-precision floating-point operations.  */

enum mips_fpu_type
{
  MIPS_FPU_DOUBLE,		/* Full double precision floating point.  */
  MIPS_FPU_SINGLE,		/* Single precision floating point (R4650).  */
  MIPS_FPU_NONE			/* No floating point.  */
};

/* MIPS specific per-architecture information.  */
struct mips_gdbarch_tdep : gdbarch_tdep_base
{
  /* from the elf header */
  int elf_flags = 0;

  /* mips options */
  enum mips_abi mips_abi {};
  enum mips_abi found_abi {};
  enum mips_isa mips_isa {};
  enum mips_fpu_type mips_fpu_type {};
  int mips_last_arg_regnum = 0;
  int mips_last_fp_arg_regnum = 0;
  int default_mask_address_p = 0;
  /* Is the target using 64-bit raw integer registers but only
     storing a left-aligned 32-bit value in each?  */
  int mips64_transfers_32bit_regs_p = 0;
  /* Indexes for various registers.  IRIX and embedded have
     different values.  This contains the "public" fields.  Don't
     add any that do not need to be public.  */
  const struct mips_regnum *regnum = nullptr;
  /* Register names table for the current register set.  */
  const char * const *mips_processor_reg_names = nullptr;

  /* The size of register data available from the target, if known.
     This doesn't quite obsolete the manual
     mips64_transfers_32bit_regs_p, since that is documented to force
     left alignment even for big endian (very strange).  */
  int register_size_valid_p = 0;
  int register_size = 0;

  /* Return the expected next PC if FRAME is stopped at a syscall
     instruction.  */
  CORE_ADDR (*syscall_next_pc) (frame_info_ptr frame) = nullptr;
};

/* Register numbers of various important registers.  */

enum
{
  MIPS_ZERO_REGNUM = 0,		/* Read-only register, always 0.  */
  MIPS_AT_REGNUM = 1,
  MIPS_V0_REGNUM = 2,		/* Function integer return value.  */
  MIPS_A0_REGNUM = 4,		/* Loc of first arg during a subr call.  */
  MIPS_S2_REGNUM = 18,		/* Contains return address in MIPS16 thunks. */
  MIPS_T9_REGNUM = 25,		/* Contains address of callee in PIC.  */
  MIPS_GP_REGNUM = 28,
  MIPS_SP_REGNUM = 29,
  MIPS_RA_REGNUM = 31,
  MIPS_PS_REGNUM = 32,		/* Contains processor status.  */
  MIPS_EMBED_LO_REGNUM = 33,
  MIPS_EMBED_HI_REGNUM = 34,
  MIPS_EMBED_BADVADDR_REGNUM = 35,
  MIPS_EMBED_CAUSE_REGNUM = 36,
  MIPS_EMBED_PC_REGNUM = 37,
  MIPS_EMBED_FP0_REGNUM = 38,
  MIPS_UNUSED_REGNUM = 73,	/* Never used, FIXME.  */
  MIPS_FIRST_EMBED_REGNUM = 74,	/* First CP0 register for embedded use.  */
  MIPS_PRID_REGNUM = 89,	/* Processor ID.  */
  MIPS_LAST_EMBED_REGNUM = 89	/* Last one.  */
};

/* Instruction sizes and other useful constants.  */
enum
{
  MIPS_INSN16_SIZE = 2,
  MIPS_INSN32_SIZE = 4,
  /* The number of floating-point or integer registers.  */
  MIPS_NUMREGS = 32
};

/* Single step based on where the current instruction will take us.  */
extern std::vector<CORE_ADDR> mips_software_single_step
  (struct regcache *regcache);

/* Strip the ISA (compression) bit off from ADDR.  */
extern CORE_ADDR mips_unmake_compact_addr (CORE_ADDR addr);

/* Tell if the program counter value in MEMADDR is in a standard
   MIPS function.  */
extern int mips_pc_is_mips (CORE_ADDR memaddr);

/* Tell if the program counter value in MEMADDR is in a MIPS16
   function.  */
extern int mips_pc_is_mips16 (struct gdbarch *gdbarch, CORE_ADDR memaddr);

/* Tell if the program counter value in MEMADDR is in a microMIPS
   function.  */
extern int mips_pc_is_micromips (struct gdbarch *gdbarch, CORE_ADDR memaddr);

/* Return the currently configured (or set) saved register size.  */
extern unsigned int mips_abi_regsize (struct gdbarch *gdbarch);

/* Make PC the address of the next instruction to execute.  */
extern void mips_write_pc (struct regcache *regcache, CORE_ADDR pc);

/* Target descriptions which only indicate the size of general
   registers.  */
extern struct target_desc *mips_tdesc_gp32;
extern struct target_desc *mips_tdesc_gp64;

/* Return non-zero if PC is in a MIPS SVR4 lazy binding stub section.  */

static inline int
in_mips_stubs_section (CORE_ADDR pc)
{
  return pc_in_section (pc, ".MIPS.stubs");
}

#endif /* MIPS_TDEP_H */

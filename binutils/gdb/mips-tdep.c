/* Target-dependent code for the MIPS architecture, for GDB, the GNU Debugger.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

   Contributed by Alessandro Forin(af@cs.cmu.edu) at CMU
   and by Per Bothner(bothner@cs.wisc.edu) at U.Wisconsin.

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

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "language.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "target.h"
#include "arch-utils.h"
#include "regcache.h"
#include "osabi.h"
#include "mips-tdep.h"
#include "block.h"
#include "reggroups.h"
#include "opcode/mips.h"
#include "elf/mips.h"
#include "elf-bfd.h"
#include "symcat.h"
#include "sim-regno.h"
#include "dis-asm.h"
#include "disasm.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "infcall.h"
#include "remote.h"
#include "target-descriptions.h"
#include "dwarf2/frame.h"
#include "user-regs.h"
#include "valprint.h"
#include "ax.h"
#include "target-float.h"
#include <algorithm>

static struct type *mips_register_type (struct gdbarch *gdbarch, int regnum);

static int mips32_instruction_has_delay_slot (struct gdbarch *gdbarch,
					      ULONGEST inst);
static int micromips_instruction_has_delay_slot (ULONGEST insn, int mustbe32);
static int mips16_instruction_has_delay_slot (unsigned short inst,
					      int mustbe32);

static int mips32_insn_at_pc_has_delay_slot (struct gdbarch *gdbarch,
					     CORE_ADDR addr);
static int micromips_insn_at_pc_has_delay_slot (struct gdbarch *gdbarch,
						CORE_ADDR addr, int mustbe32);
static int mips16_insn_at_pc_has_delay_slot (struct gdbarch *gdbarch,
					     CORE_ADDR addr, int mustbe32);

static void mips_print_float_info (struct gdbarch *, struct ui_file *,
				   frame_info_ptr, const char *);

/* A useful bit in the CP0 status register (MIPS_PS_REGNUM).  */
/* This bit is set if we are emulating 32-bit FPRs on a 64-bit chip.  */
#define ST0_FR (1 << 26)

/* The sizes of floating point registers.  */

enum
{
  MIPS_FPU_SINGLE_REGSIZE = 4,
  MIPS_FPU_DOUBLE_REGSIZE = 8
};

enum
{
  MIPS32_REGSIZE = 4,
  MIPS64_REGSIZE = 8
};

static const char *mips_abi_string;

static const char *const mips_abi_strings[] = {
  "auto",
  "n32",
  "o32",
  "n64",
  "o64",
  "eabi32",
  "eabi64",
  NULL
};

/* Enum describing the different kinds of breakpoints.  */

enum mips_breakpoint_kind
{
  /* 16-bit MIPS16 mode breakpoint.  */
  MIPS_BP_KIND_MIPS16 = 2,

  /* 16-bit microMIPS mode breakpoint.  */
  MIPS_BP_KIND_MICROMIPS16 = 3,

  /* 32-bit standard MIPS mode breakpoint.  */
  MIPS_BP_KIND_MIPS32 = 4,

  /* 32-bit microMIPS mode breakpoint.  */
  MIPS_BP_KIND_MICROMIPS32 = 5,
};

/* For backwards compatibility we default to MIPS16.  This flag is
   overridden as soon as unambiguous ELF file flags tell us the
   compressed ISA encoding used.  */
static const char mips_compression_mips16[] = "mips16";
static const char mips_compression_micromips[] = "micromips";
static const char *const mips_compression_strings[] =
{
  mips_compression_mips16,
  mips_compression_micromips,
  NULL
};

static const char *mips_compression_string = mips_compression_mips16;

/* The standard register names, and all the valid aliases for them.  */
struct register_alias
{
  const char *name;
  int regnum;
};

/* Aliases for o32 and most other ABIs.  */
const struct register_alias mips_o32_aliases[] = {
  { "ta0", 12 },
  { "ta1", 13 },
  { "ta2", 14 },
  { "ta3", 15 }
};

/* Aliases for n32 and n64.  */
const struct register_alias mips_n32_n64_aliases[] = {
  { "ta0", 8 },
  { "ta1", 9 },
  { "ta2", 10 },
  { "ta3", 11 }
};

/* Aliases for ABI-independent registers.  */
const struct register_alias mips_register_aliases[] = {
  /* The architecture manuals specify these ABI-independent names for
     the GPRs.  */
#define R(n) { "r" #n, n }
  R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7),
  R(8), R(9), R(10), R(11), R(12), R(13), R(14), R(15),
  R(16), R(17), R(18), R(19), R(20), R(21), R(22), R(23),
  R(24), R(25), R(26), R(27), R(28), R(29), R(30), R(31),
#undef R

  /* k0 and k1 are sometimes called these instead (for "kernel
     temp").  */
  { "kt0", 26 },
  { "kt1", 27 },

  /* This is the traditional GDB name for the CP0 status register.  */
  { "sr", MIPS_PS_REGNUM },

  /* This is the traditional GDB name for the CP0 BadVAddr register.  */
  { "bad", MIPS_EMBED_BADVADDR_REGNUM },

  /* This is the traditional GDB name for the FCSR.  */
  { "fsr", MIPS_EMBED_FP0_REGNUM + 32 }
};

const struct register_alias mips_numeric_register_aliases[] = {
#define R(n) { #n, n }
  R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7),
  R(8), R(9), R(10), R(11), R(12), R(13), R(14), R(15),
  R(16), R(17), R(18), R(19), R(20), R(21), R(22), R(23),
  R(24), R(25), R(26), R(27), R(28), R(29), R(30), R(31),
#undef R
};

#ifndef MIPS_DEFAULT_FPU_TYPE
#define MIPS_DEFAULT_FPU_TYPE MIPS_FPU_DOUBLE
#endif
static int mips_fpu_type_auto = 1;
static enum mips_fpu_type mips_fpu_type = MIPS_DEFAULT_FPU_TYPE;

static unsigned int mips_debug = 0;

/* Properties (for struct target_desc) describing the g/G packet
   layout.  */
#define PROPERTY_GP32 "internal: transfers-32bit-registers"
#define PROPERTY_GP64 "internal: transfers-64bit-registers"

struct target_desc *mips_tdesc_gp32;
struct target_desc *mips_tdesc_gp64;

/* The current set of options to be passed to the disassembler.  */
static char *mips_disassembler_options;

/* Implicit disassembler options for individual ABIs.  These tell
   libopcodes to use general-purpose register names corresponding
   to the ABI we have selected, perhaps via a `set mips abi ...'
   override, rather than ones inferred from the ABI set in the ELF
   headers of the binary file selected for debugging.  */
static const char mips_disassembler_options_o32[] = "gpr-names=32";
static const char mips_disassembler_options_n32[] = "gpr-names=n32";
static const char mips_disassembler_options_n64[] = "gpr-names=64";

const struct mips_regnum *
mips_regnum (struct gdbarch *gdbarch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  return tdep->regnum;
}

static int
mips_fpa0_regnum (struct gdbarch *gdbarch)
{
  return mips_regnum (gdbarch)->fp0 + 12;
}

/* Return 1 if REGNUM refers to a floating-point general register, raw
   or cooked.  Otherwise return 0.  */

static int
mips_float_register_p (struct gdbarch *gdbarch, int regnum)
{
  int rawnum = regnum % gdbarch_num_regs (gdbarch);

  return (rawnum >= mips_regnum (gdbarch)->fp0
	  && rawnum < mips_regnum (gdbarch)->fp0 + 32);
}

static bool
mips_eabi (gdbarch *arch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (arch);
  return (tdep->mips_abi == MIPS_ABI_EABI32 \
	  || tdep->mips_abi == MIPS_ABI_EABI64);
}

static int
mips_last_fp_arg_regnum (gdbarch *arch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (arch);
  return tdep->mips_last_fp_arg_regnum;
}

static int
mips_last_arg_regnum (gdbarch *arch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (arch);
  return tdep->mips_last_arg_regnum;
}

static enum mips_fpu_type
mips_get_fpu_type (gdbarch *arch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (arch);
  return tdep->mips_fpu_type;
}

/* Return the MIPS ABI associated with GDBARCH.  */
enum mips_abi
mips_abi (struct gdbarch *gdbarch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  return tdep->mips_abi;
}

int
mips_isa_regsize (struct gdbarch *gdbarch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

  /* If we know how big the registers are, use that size.  */
  if (tdep->register_size_valid_p)
    return tdep->register_size;

  /* Fall back to the previous behavior.  */
  return (gdbarch_bfd_arch_info (gdbarch)->bits_per_word
	  / gdbarch_bfd_arch_info (gdbarch)->bits_per_byte);
}

/* Max saved register size.  */
#define MAX_MIPS_ABI_REGSIZE 8

/* Return the currently configured (or set) saved register size.  */

unsigned int
mips_abi_regsize (struct gdbarch *gdbarch)
{
  switch (mips_abi (gdbarch))
    {
    case MIPS_ABI_EABI32:
    case MIPS_ABI_O32:
      return 4;
    case MIPS_ABI_N32:
    case MIPS_ABI_N64:
    case MIPS_ABI_O64:
    case MIPS_ABI_EABI64:
      return 8;
    case MIPS_ABI_UNKNOWN:
    case MIPS_ABI_LAST:
    default:
      internal_error (_("bad switch"));
    }
}

/* MIPS16/microMIPS function addresses are odd (bit 0 is set).  Here
   are some functions to handle addresses associated with compressed
   code including but not limited to testing, setting, or clearing
   bit 0 of such addresses.  */

/* Return one iff compressed code is the MIPS16 instruction set.  */

static int
is_mips16_isa (struct gdbarch *gdbarch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  return tdep->mips_isa == ISA_MIPS16;
}

/* Return one iff compressed code is the microMIPS instruction set.  */

static int
is_micromips_isa (struct gdbarch *gdbarch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  return tdep->mips_isa == ISA_MICROMIPS;
}

/* Return one iff ADDR denotes compressed code.  */

static int
is_compact_addr (CORE_ADDR addr)
{
  return ((addr) & 1);
}

/* Return one iff ADDR denotes standard ISA code.  */

static int
is_mips_addr (CORE_ADDR addr)
{
  return !is_compact_addr (addr);
}

/* Return one iff ADDR denotes MIPS16 code.  */

static int
is_mips16_addr (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return is_compact_addr (addr) && is_mips16_isa (gdbarch);
}

/* Return one iff ADDR denotes microMIPS code.  */

static int
is_micromips_addr (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return is_compact_addr (addr) && is_micromips_isa (gdbarch);
}

/* Strip the ISA (compression) bit off from ADDR.  */

static CORE_ADDR
unmake_compact_addr (CORE_ADDR addr)
{
  return ((addr) & ~(CORE_ADDR) 1);
}

/* Add the ISA (compression) bit to ADDR.  */

static CORE_ADDR
make_compact_addr (CORE_ADDR addr)
{
  return ((addr) | (CORE_ADDR) 1);
}

/* Extern version of unmake_compact_addr; we use a separate function
   so that unmake_compact_addr can be inlined throughout this file.  */

CORE_ADDR
mips_unmake_compact_addr (CORE_ADDR addr)
{
  return unmake_compact_addr (addr);
}

/* Functions for setting and testing a bit in a minimal symbol that
   marks it as MIPS16 or microMIPS function.  The MSB of the minimal
   symbol's "info" field is used for this purpose.

   gdbarch_elf_make_msymbol_special tests whether an ELF symbol is
   "special", i.e. refers to a MIPS16 or microMIPS function, and sets
   one of the "special" bits in a minimal symbol to mark it accordingly.
   The test checks an ELF-private flag that is valid for true function
   symbols only; for synthetic symbols such as for PLT stubs that have
   no ELF-private part at all the MIPS BFD backend arranges for this
   information to be carried in the asymbol's udata field instead.

   msymbol_is_mips16 and msymbol_is_micromips test the "special" bit
   in a minimal symbol.  */

static void
mips_elf_make_msymbol_special (asymbol * sym, struct minimal_symbol *msym)
{
  elf_symbol_type *elfsym = (elf_symbol_type *) sym;
  unsigned char st_other;

  if ((sym->flags & BSF_SYNTHETIC) == 0)
    st_other = elfsym->internal_elf_sym.st_other;
  else if ((sym->flags & BSF_FUNCTION) != 0)
    st_other = sym->udata.i;
  else
    return;

  if (ELF_ST_IS_MICROMIPS (st_other))
    {
      SET_MSYMBOL_TARGET_FLAG_MICROMIPS (msym);
      CORE_ADDR fixed = CORE_ADDR (msym->unrelocated_address ()) | 1;
      msym->set_unrelocated_address (unrelocated_addr (fixed));
    }
  else if (ELF_ST_IS_MIPS16 (st_other))
    {
      SET_MSYMBOL_TARGET_FLAG_MIPS16 (msym);
      CORE_ADDR fixed = CORE_ADDR (msym->unrelocated_address ()) | 1;
      msym->set_unrelocated_address (unrelocated_addr (fixed));
    }
}

/* Return one iff MSYM refers to standard ISA code.  */

static int
msymbol_is_mips (struct minimal_symbol *msym)
{
  return !(MSYMBOL_TARGET_FLAG_MIPS16 (msym)
	   || MSYMBOL_TARGET_FLAG_MICROMIPS (msym));
}

/* Return one iff MSYM refers to MIPS16 code.  */

static int
msymbol_is_mips16 (struct minimal_symbol *msym)
{
  return MSYMBOL_TARGET_FLAG_MIPS16 (msym);
}

/* Return one iff MSYM refers to microMIPS code.  */

static int
msymbol_is_micromips (struct minimal_symbol *msym)
{
  return MSYMBOL_TARGET_FLAG_MICROMIPS (msym);
}

/* Set the ISA bit in the main symbol too, complementing the corresponding
   minimal symbol setting and reflecting the run-time value of the symbol.
   The need for comes from the ISA bit having been cleared as code in
   `_bfd_mips_elf_symbol_processing' separated it into the ELF symbol's
   `st_other' STO_MIPS16 or STO_MICROMIPS annotation, making the values
   of symbols referring to compressed code different in GDB to the values
   used by actual code.  That in turn makes them evaluate incorrectly in
   expressions, producing results different to what the same expressions
   yield when compiled into the program being debugged.  */

static void
mips_make_symbol_special (struct symbol *sym, struct objfile *objfile)
{
  if (sym->aclass () == LOC_BLOCK)
    {
      /* We are in symbol reading so it is OK to cast away constness.  */
      struct block *block = (struct block *) sym->value_block ();
      CORE_ADDR compact_block_start;
      struct bound_minimal_symbol msym;

      compact_block_start = block->start () | 1;
      msym = lookup_minimal_symbol_by_pc (compact_block_start);
      if (msym.minsym && !msymbol_is_mips (msym.minsym))
	{
	  block->set_start (compact_block_start);
	}
    }
}

/* XFER a value from the big/little/left end of the register.
   Depending on the size of the value it might occupy the entire
   register or just part of it.  Make an allowance for this, aligning
   things accordingly.  */

static void
mips_xfer_register (struct gdbarch *gdbarch, struct regcache *regcache,
		    int reg_num, int length,
		    enum bfd_endian endian, gdb_byte *in,
		    const gdb_byte *out, int buf_offset)
{
  int reg_offset = 0;

  gdb_assert (reg_num >= gdbarch_num_regs (gdbarch));
  /* Need to transfer the left or right part of the register, based on
     the targets byte order.  */
  switch (endian)
    {
    case BFD_ENDIAN_BIG:
      reg_offset = register_size (gdbarch, reg_num) - length;
      break;
    case BFD_ENDIAN_LITTLE:
      reg_offset = 0;
      break;
    case BFD_ENDIAN_UNKNOWN:	/* Indicates no alignment.  */
      reg_offset = 0;
      break;
    default:
      internal_error (_("bad switch"));
    }
  if (mips_debug)
    gdb_printf (gdb_stderr,
		"xfer $%d, reg offset %d, buf offset %d, length %d, ",
		reg_num, reg_offset, buf_offset, length);
  if (mips_debug && out != NULL)
    {
      int i;
      gdb_printf (gdb_stdlog, "out ");
      for (i = 0; i < length; i++)
	gdb_printf (gdb_stdlog, "%02x", out[buf_offset + i]);
    }
  if (in != NULL)
    regcache->cooked_read_part (reg_num, reg_offset, length, in + buf_offset);
  if (out != NULL)
    regcache->cooked_write_part (reg_num, reg_offset, length, out + buf_offset);
  if (mips_debug && in != NULL)
    {
      int i;
      gdb_printf (gdb_stdlog, "in ");
      for (i = 0; i < length; i++)
	gdb_printf (gdb_stdlog, "%02x", in[buf_offset + i]);
    }
  if (mips_debug)
    gdb_printf (gdb_stdlog, "\n");
}

/* Determine if a MIPS3 or later cpu is operating in MIPS{1,2} FPU
   compatiblity mode.  A return value of 1 means that we have
   physical 64-bit registers, but should treat them as 32-bit registers.  */

static int
mips2_fp_compat (frame_info_ptr frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  /* MIPS1 and MIPS2 have only 32 bit FPRs, and the FR bit is not
     meaningful.  */
  if (register_size (gdbarch, mips_regnum (gdbarch)->fp0) == 4)
    return 0;

#if 0
  /* FIXME drow 2002-03-10: This is disabled until we can do it consistently,
     in all the places we deal with FP registers.  PR gdb/413.  */
  /* Otherwise check the FR bit in the status register - it controls
     the FP compatiblity mode.  If it is clear we are in compatibility
     mode.  */
  if ((get_frame_register_unsigned (frame, MIPS_PS_REGNUM) & ST0_FR) == 0)
    return 1;
#endif

  return 0;
}

#define VM_MIN_ADDRESS (CORE_ADDR)0x400000

static CORE_ADDR heuristic_proc_start (struct gdbarch *, CORE_ADDR);

/* The list of available "set mips " and "show mips " commands.  */

static struct cmd_list_element *setmipscmdlist = NULL;
static struct cmd_list_element *showmipscmdlist = NULL;

/* Integer registers 0 thru 31 are handled explicitly by
   mips_register_name().  Processor specific registers 32 and above
   are listed in the following tables.  */

enum
{ NUM_MIPS_PROCESSOR_REGS = (90 - 32) };

/* Generic MIPS.  */

static const char * const mips_generic_reg_names[NUM_MIPS_PROCESSOR_REGS] = {
  "sr", "lo", "hi", "bad", "cause", "pc",
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  "fsr", "fir",
};

/* Names of tx39 registers.  */

static const char * const mips_tx39_reg_names[NUM_MIPS_PROCESSOR_REGS] = {
  "sr", "lo", "hi", "bad", "cause", "pc",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "config", "cache", "debug", "depc", "epc",
};

/* Names of registers with Linux kernels.  */
static const char * const mips_linux_reg_names[NUM_MIPS_PROCESSOR_REGS] = {
  "sr", "lo", "hi", "bad", "cause", "pc",
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  "fsr", "fir"
};


/* Return the name of the register corresponding to REGNO.  */
static const char *
mips_register_name (struct gdbarch *gdbarch, int regno)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  /* GPR names for all ABIs other than n32/n64.  */
  static const char *mips_gpr_names[] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
  };

  /* GPR names for n32 and n64 ABIs.  */
  static const char *mips_n32_n64_gpr_names[] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
  };

  enum mips_abi abi = mips_abi (gdbarch);

  /* Map [gdbarch_num_regs .. 2*gdbarch_num_regs) onto the raw registers, 
     but then don't make the raw register names visible.  This (upper)
     range of user visible register numbers are the pseudo-registers.

     This approach was adopted accommodate the following scenario:
     It is possible to debug a 64-bit device using a 32-bit
     programming model.  In such instances, the raw registers are
     configured to be 64-bits wide, while the pseudo registers are
     configured to be 32-bits wide.  The registers that the user
     sees - the pseudo registers - match the users expectations
     given the programming model being used.  */
  int rawnum = regno % gdbarch_num_regs (gdbarch);
  if (regno < gdbarch_num_regs (gdbarch))
    return "";

  /* The MIPS integer registers are always mapped from 0 to 31.  The
     names of the registers (which reflects the conventions regarding
     register use) vary depending on the ABI.  */
  if (0 <= rawnum && rawnum < 32)
    {
      if (abi == MIPS_ABI_N32 || abi == MIPS_ABI_N64)
	return mips_n32_n64_gpr_names[rawnum];
      else
	return mips_gpr_names[rawnum];
    }
  else if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, rawnum);
  else if (32 <= rawnum && rawnum < gdbarch_num_regs (gdbarch))
    {
      gdb_assert (rawnum - 32 < NUM_MIPS_PROCESSOR_REGS);
      if (tdep->mips_processor_reg_names[rawnum - 32])
	return tdep->mips_processor_reg_names[rawnum - 32];
      return "";
    }
  else
    internal_error (_("mips_register_name: bad register number %d"), rawnum);
}

/* Return the groups that a MIPS register can be categorised into.  */

static int
mips_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  const struct reggroup *reggroup)
{
  int vector_p;
  int float_p;
  int raw_p;
  int rawnum = regnum % gdbarch_num_regs (gdbarch);
  int pseudo = regnum / gdbarch_num_regs (gdbarch);
  if (reggroup == all_reggroup)
    return pseudo;
  vector_p = register_type (gdbarch, regnum)->is_vector ();
  float_p = register_type (gdbarch, regnum)->code () == TYPE_CODE_FLT;
  /* FIXME: cagney/2003-04-13: Can't yet use gdbarch_num_regs
     (gdbarch), as not all architectures are multi-arch.  */
  raw_p = rawnum < gdbarch_num_regs (gdbarch);
  if (gdbarch_register_name (gdbarch, regnum)[0] == '\0')
    return 0;
  if (reggroup == float_reggroup)
    return float_p && pseudo;
  if (reggroup == vector_reggroup)
    return vector_p && pseudo;
  if (reggroup == general_reggroup)
    return (!vector_p && !float_p) && pseudo;
  /* Save the pseudo registers.  Need to make certain that any code
     extracting register values from a saved register cache also uses
     pseudo registers.  */
  if (reggroup == save_reggroup)
    return raw_p && pseudo;
  /* Restore the same pseudo register.  */
  if (reggroup == restore_reggroup)
    return raw_p && pseudo;
  return 0;
}

/* Return the groups that a MIPS register can be categorised into.
   This version is only used if we have a target description which
   describes real registers (and their groups).  */

static int
mips_tdesc_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				const struct reggroup *reggroup)
{
  int rawnum = regnum % gdbarch_num_regs (gdbarch);
  int pseudo = regnum / gdbarch_num_regs (gdbarch);
  int ret;

  /* Only save, restore, and display the pseudo registers.  Need to
     make certain that any code extracting register values from a
     saved register cache also uses pseudo registers.

     Note: saving and restoring the pseudo registers is slightly
     strange; if we have 64 bits, we should save and restore all
     64 bits.  But this is hard and has little benefit.  */
  if (!pseudo)
    return 0;

  ret = tdesc_register_in_reggroup_p (gdbarch, rawnum, reggroup);
  if (ret != -1)
    return ret;

  return mips_register_reggroup_p (gdbarch, regnum, reggroup);
}

/* Map the symbol table registers which live in the range [1 *
   gdbarch_num_regs .. 2 * gdbarch_num_regs) back onto the corresponding raw
   registers.  Take care of alignment and size problems.  */

static enum register_status
mips_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int cookednum, gdb_byte *buf)
{
  int rawnum = cookednum % gdbarch_num_regs (gdbarch);
  gdb_assert (cookednum >= gdbarch_num_regs (gdbarch)
	      && cookednum < 2 * gdbarch_num_regs (gdbarch));
  if (register_size (gdbarch, rawnum) == register_size (gdbarch, cookednum))
    return regcache->raw_read (rawnum, buf);
  else if (register_size (gdbarch, rawnum) >
	   register_size (gdbarch, cookednum))
    {
      mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

      if (tdep->mips64_transfers_32bit_regs_p)
	return regcache->raw_read_part (rawnum, 0, 4, buf);
      else
	{
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  LONGEST regval;
	  enum register_status status;

	  status = regcache->raw_read (rawnum, &regval);
	  if (status == REG_VALID)
	    store_signed_integer (buf, 4, byte_order, regval);
	  return status;
	}
    }
  else
    internal_error (_("bad register size"));
}

static void
mips_pseudo_register_write (struct gdbarch *gdbarch,
			    struct regcache *regcache, int cookednum,
			    const gdb_byte *buf)
{
  int rawnum = cookednum % gdbarch_num_regs (gdbarch);
  gdb_assert (cookednum >= gdbarch_num_regs (gdbarch)
	      && cookednum < 2 * gdbarch_num_regs (gdbarch));
  if (register_size (gdbarch, rawnum) == register_size (gdbarch, cookednum))
    regcache->raw_write (rawnum, buf);
  else if (register_size (gdbarch, rawnum) >
	   register_size (gdbarch, cookednum))
    {
      mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

      if (tdep->mips64_transfers_32bit_regs_p)
	regcache->raw_write_part (rawnum, 0, 4, buf);
      else
	{
	  /* Sign extend the shortened version of the register prior
	     to placing it in the raw register.  This is required for
	     some mips64 parts in order to avoid unpredictable behavior.  */
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  LONGEST regval = extract_signed_integer (buf, 4, byte_order);
	  regcache_raw_write_signed (regcache, rawnum, regval);
	}
    }
  else
    internal_error (_("bad register size"));
}

static int
mips_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				 struct agent_expr *ax, int reg)
{
  int rawnum = reg % gdbarch_num_regs (gdbarch);
  gdb_assert (reg >= gdbarch_num_regs (gdbarch)
	      && reg < 2 * gdbarch_num_regs (gdbarch));

  ax_reg_mask (ax, rawnum);

  return 0;
}

static int
mips_ax_pseudo_register_push_stack (struct gdbarch *gdbarch,
				    struct agent_expr *ax, int reg)
{
  int rawnum = reg % gdbarch_num_regs (gdbarch);
  gdb_assert (reg >= gdbarch_num_regs (gdbarch)
	      && reg < 2 * gdbarch_num_regs (gdbarch));
  if (register_size (gdbarch, rawnum) >= register_size (gdbarch, reg))
    {
      ax_reg (ax, rawnum);

      if (register_size (gdbarch, rawnum) > register_size (gdbarch, reg))
	{
	  mips_gdbarch_tdep *tdep
	    = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

	  if (!tdep->mips64_transfers_32bit_regs_p
	      || gdbarch_byte_order (gdbarch) != BFD_ENDIAN_BIG)
	    {
	      ax_const_l (ax, 32);
	      ax_simple (ax, aop_lsh);
	    }
	  ax_const_l (ax, 32);
	  ax_simple (ax, aop_rsh_signed);
	}
    }
  else
    internal_error (_("bad register size"));

  return 0;
}

/* Table to translate 3-bit register field to actual register number.  */
static const signed char mips_reg3_to_reg[8] = { 16, 17, 2, 3, 4, 5, 6, 7 };

/* Heuristic_proc_start may hunt through the text section for a long
   time across a 2400 baud serial line.  Allows the user to limit this
   search.  */

static int heuristic_fence_post = 0;

/* Number of bytes of storage in the actual machine representation for
   register N.  NOTE: This defines the pseudo register type so need to
   rebuild the architecture vector.  */

static bool mips64_transfers_32bit_regs_p = false;

static void
set_mips64_transfers_32bit_regs (const char *args, int from_tty,
				 struct cmd_list_element *c)
{
  struct gdbarch_info info;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    {
      mips64_transfers_32bit_regs_p = 0;
      error (_("32-bit compatibility mode not supported"));
    }
}

/* Convert to/from a register and the corresponding memory value.  */

/* This predicate tests for the case of an 8 byte floating point
   value that is being transferred to or from a pair of floating point
   registers each of which are (or are considered to be) only 4 bytes
   wide.  */
static int
mips_convert_register_float_case_p (struct gdbarch *gdbarch, int regnum,
				    struct type *type)
{
  return (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
	  && register_size (gdbarch, regnum) == 4
	  && mips_float_register_p (gdbarch, regnum)
	  && type->code () == TYPE_CODE_FLT && type->length () == 8);
}

/* This predicate tests for the case of a value of less than 8
   bytes in width that is being transfered to or from an 8 byte
   general purpose register.  */
static int
mips_convert_register_gpreg_case_p (struct gdbarch *gdbarch, int regnum,
				    struct type *type)
{
  int num_regs = gdbarch_num_regs (gdbarch);

  return (register_size (gdbarch, regnum) == 8
	  && regnum % num_regs > 0 && regnum % num_regs < 32
	  && type->length () < 8);
}

static int
mips_convert_register_p (struct gdbarch *gdbarch,
			 int regnum, struct type *type)
{
  return (mips_convert_register_float_case_p (gdbarch, regnum, type)
	  || mips_convert_register_gpreg_case_p (gdbarch, regnum, type));
}

static int
mips_register_to_value (frame_info_ptr frame, int regnum,
			struct type *type, gdb_byte *to,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);

  if (mips_convert_register_float_case_p (gdbarch, regnum, type))
    {
      get_frame_register (frame, regnum + 0, to + 4);
      get_frame_register (frame, regnum + 1, to + 0);

      if (!get_frame_register_bytes (next_frame, regnum + 0, 0, { to + 4, 4 },
				     optimizedp, unavailablep))
	return 0;

      if (!get_frame_register_bytes (next_frame, regnum + 1, 0, { to + 0, 4 },
				     optimizedp, unavailablep))
	return 0;
      *optimizedp = *unavailablep = 0;
      return 1;
    }
  else if (mips_convert_register_gpreg_case_p (gdbarch, regnum, type))
    {
      size_t len = type->length ();
      CORE_ADDR offset;

      offset = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 8 - len : 0;
      if (!get_frame_register_bytes (next_frame, regnum, offset, { to, len },
				     optimizedp, unavailablep))
	return 0;

      *optimizedp = *unavailablep = 0;
      return 1;
    }
  else
    {
      internal_error (_("mips_register_to_value: unrecognized case"));
    }
}

static void
mips_value_to_register (frame_info_ptr frame, int regnum,
			struct type *type, const gdb_byte *from)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  if (mips_convert_register_float_case_p (gdbarch, regnum, type))
    {
      auto from_view = gdb::make_array_view (from, 8);
      frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);
      put_frame_register (next_frame, regnum, from_view.slice (4));
      put_frame_register (next_frame, regnum + 1, from_view.slice (0, 4));
    }
  else if (mips_convert_register_gpreg_case_p (gdbarch, regnum, type))
    {
      gdb_byte fill[8];
      size_t len = type->length ();
      frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);

      /* Sign extend values, irrespective of type, that are stored to 
	 a 64-bit general purpose register.  (32-bit unsigned values
	 are stored as signed quantities within a 64-bit register.
	 When performing an operation, in compiled code, that combines
	 a 32-bit unsigned value with a signed 64-bit value, a type
	 conversion is first performed that zeroes out the high 32 bits.)  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	{
	  if (from[0] & 0x80)
	    store_signed_integer (fill, 8, BFD_ENDIAN_BIG, -1);
	  else
	    store_signed_integer (fill, 8, BFD_ENDIAN_BIG, 0);
	  put_frame_register_bytes (next_frame, regnum, 0, {fill, 8 - len});
	  put_frame_register_bytes (next_frame, regnum, 8 - len, {from, len});
	}
      else
	{
	  if (from[len-1] & 0x80)
	    store_signed_integer (fill, 8, BFD_ENDIAN_LITTLE, -1);
	  else
	    store_signed_integer (fill, 8, BFD_ENDIAN_LITTLE, 0);
	  put_frame_register_bytes (next_frame, regnum, 0, {from, len});
	  put_frame_register_bytes (next_frame, regnum, len, {fill, 8 - len});
	}
    }
  else
    {
      internal_error (_("mips_value_to_register: unrecognized case"));
    }
}

/* Return the GDB type object for the "standard" data type of data in
   register REG.  */

static struct type *
mips_register_type (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (regnum >= 0 && regnum < 2 * gdbarch_num_regs (gdbarch));
  if (mips_float_register_p (gdbarch, regnum))
    {
      /* The floating-point registers raw, or cooked, always match
	 mips_isa_regsize(), and also map 1:1, byte for byte.  */
      if (mips_isa_regsize (gdbarch) == 4)
	return builtin_type (gdbarch)->builtin_float;
      else
	return builtin_type (gdbarch)->builtin_double;
    }
  else if (regnum < gdbarch_num_regs (gdbarch))
    {
      /* The raw or ISA registers.  These are all sized according to
	 the ISA regsize.  */
      if (mips_isa_regsize (gdbarch) == 4)
	return builtin_type (gdbarch)->builtin_int32;
      else
	return builtin_type (gdbarch)->builtin_int64;
    }
  else
    {
      int rawnum = regnum - gdbarch_num_regs (gdbarch);
      mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

      /* The cooked or ABI registers.  These are sized according to
	 the ABI (with a few complications).  */
      if (rawnum == mips_regnum (gdbarch)->fp_control_status
	  || rawnum == mips_regnum (gdbarch)->fp_implementation_revision)
	return builtin_type (gdbarch)->builtin_int32;
      else if (gdbarch_osabi (gdbarch) != GDB_OSABI_LINUX
	       && rawnum >= MIPS_FIRST_EMBED_REGNUM
	       && rawnum <= MIPS_LAST_EMBED_REGNUM)
	/* The pseudo/cooked view of the embedded registers is always
	   32-bit.  The raw view is handled below.  */
	return builtin_type (gdbarch)->builtin_int32;
      else if (tdep->mips64_transfers_32bit_regs_p)
	/* The target, while possibly using a 64-bit register buffer,
	   is only transfering 32-bits of each integer register.
	   Reflect this in the cooked/pseudo (ABI) register value.  */
	return builtin_type (gdbarch)->builtin_int32;
      else if (mips_abi_regsize (gdbarch) == 4)
	/* The ABI is restricted to 32-bit registers (the ISA could be
	   32- or 64-bit).  */
	return builtin_type (gdbarch)->builtin_int32;
      else
	/* 64-bit ABI.  */
	return builtin_type (gdbarch)->builtin_int64;
    }
}

/* Return the GDB type for the pseudo register REGNUM, which is the
   ABI-level view.  This function is only called if there is a target
   description which includes registers, so we know precisely the
   types of hardware registers.  */

static struct type *
mips_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  const int num_regs = gdbarch_num_regs (gdbarch);
  int rawnum = regnum % num_regs;
  struct type *rawtype;

  gdb_assert (regnum >= num_regs && regnum < 2 * num_regs);

  /* Absent registers are still absent.  */
  rawtype = gdbarch_register_type (gdbarch, rawnum);
  if (rawtype->length () == 0)
    return rawtype;

  /* Present the floating point registers however the hardware did;
     do not try to convert between FPU layouts.  */
  if (mips_float_register_p (gdbarch, rawnum))
    return rawtype;

  /* Floating-point control registers are always 32-bit even though for
     backwards compatibility reasons 64-bit targets will transfer them
     as 64-bit quantities even if using XML descriptions.  */
  if (rawnum == mips_regnum (gdbarch)->fp_control_status
      || rawnum == mips_regnum (gdbarch)->fp_implementation_revision)
    return builtin_type (gdbarch)->builtin_int32;

  /* Use pointer types for registers if we can.  For n32 we can not,
     since we do not have a 64-bit pointer type.  */
  if (mips_abi_regsize (gdbarch)
      == builtin_type (gdbarch)->builtin_data_ptr->length())
    {
      if (rawnum == MIPS_SP_REGNUM
	  || rawnum == mips_regnum (gdbarch)->badvaddr)
	return builtin_type (gdbarch)->builtin_data_ptr;
      else if (rawnum == mips_regnum (gdbarch)->pc)
	return builtin_type (gdbarch)->builtin_func_ptr;
    }

  if (mips_abi_regsize (gdbarch) == 4 && rawtype->length () == 8
      && ((rawnum >= MIPS_ZERO_REGNUM && rawnum <= MIPS_PS_REGNUM)
	  || rawnum == mips_regnum (gdbarch)->lo
	  || rawnum == mips_regnum (gdbarch)->hi
	  || rawnum == mips_regnum (gdbarch)->badvaddr
	  || rawnum == mips_regnum (gdbarch)->cause
	  || rawnum == mips_regnum (gdbarch)->pc
	  || (mips_regnum (gdbarch)->dspacc != -1
	      && rawnum >= mips_regnum (gdbarch)->dspacc
	      && rawnum < mips_regnum (gdbarch)->dspacc + 6)))
    return builtin_type (gdbarch)->builtin_int32;

  /* The pseudo/cooked view of embedded registers is always
     32-bit, even if the target transfers 64-bit values for them.
     New targets relying on XML descriptions should only transfer
     the necessary 32 bits, but older versions of GDB expected 64,
     so allow the target to provide 64 bits without interfering
     with the displayed type.  */
  if (gdbarch_osabi (gdbarch) != GDB_OSABI_LINUX
      && rawnum >= MIPS_FIRST_EMBED_REGNUM
      && rawnum <= MIPS_LAST_EMBED_REGNUM)
    return builtin_type (gdbarch)->builtin_int32;

  /* For all other registers, pass through the hardware type.  */
  return rawtype;
}

/* Should the upper word of 64-bit addresses be zeroed?  */
static enum auto_boolean mask_address_var = AUTO_BOOLEAN_AUTO;

static int
mips_mask_address_p (mips_gdbarch_tdep *tdep)
{
  switch (mask_address_var)
    {
    case AUTO_BOOLEAN_TRUE:
      return 1;
    case AUTO_BOOLEAN_FALSE:
      return 0;
      break;
    case AUTO_BOOLEAN_AUTO:
      return tdep->default_mask_address_p;
    default:
      internal_error (_("mips_mask_address_p: bad switch"));
      return -1;
    }
}

static void
show_mask_address (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  const char *additional_text = "";
  if (mask_address_var == AUTO_BOOLEAN_AUTO)
    {
      if (gdbarch_bfd_arch_info (current_inferior ()->arch ())->arch
	  != bfd_arch_mips)
	additional_text = _(" (current architecture is not MIPS)");
      else
	{
	  mips_gdbarch_tdep *tdep
	    = gdbarch_tdep<mips_gdbarch_tdep> (current_inferior ()->arch  ());

	  if (mips_mask_address_p (tdep))
	    additional_text = _(" (currently \"on\")");
	  else
	    additional_text = _(" (currently \"off\")");
	}
    }

  gdb_printf (file, _("Zeroing of upper 32 bits of 64-bit addresses is \"%s\"%s.\n"),
	      value, additional_text);
}

/* Tell if the program counter value in MEMADDR is in a standard ISA
   function.  */

int
mips_pc_is_mips (CORE_ADDR memaddr)
{
  struct bound_minimal_symbol sym;

  /* Flags indicating that this is a MIPS16 or microMIPS function is
     stored by elfread.c in the high bit of the info field.  Use this
     to decide if the function is standard MIPS.  Otherwise if bit 0
     of the address is clear, then this is a standard MIPS function.  */
  sym = lookup_minimal_symbol_by_pc (make_compact_addr (memaddr));
  if (sym.minsym)
    return msymbol_is_mips (sym.minsym);
  else
    return is_mips_addr (memaddr);
}

/* Tell if the program counter value in MEMADDR is in a MIPS16 function.  */

int
mips_pc_is_mips16 (struct gdbarch *gdbarch, CORE_ADDR memaddr)
{
  struct bound_minimal_symbol sym;

  /* A flag indicating that this is a MIPS16 function is stored by
     elfread.c in the high bit of the info field.  Use this to decide
     if the function is MIPS16.  Otherwise if bit 0 of the address is
     set, then ELF file flags will tell if this is a MIPS16 function.  */
  sym = lookup_minimal_symbol_by_pc (make_compact_addr (memaddr));
  if (sym.minsym)
    return msymbol_is_mips16 (sym.minsym);
  else
    return is_mips16_addr (gdbarch, memaddr);
}

/* Tell if the program counter value in MEMADDR is in a microMIPS function.  */

int
mips_pc_is_micromips (struct gdbarch *gdbarch, CORE_ADDR memaddr)
{
  struct bound_minimal_symbol sym;

  /* A flag indicating that this is a microMIPS function is stored by
     elfread.c in the high bit of the info field.  Use this to decide
     if the function is microMIPS.  Otherwise if bit 0 of the address
     is set, then ELF file flags will tell if this is a microMIPS
     function.  */
  sym = lookup_minimal_symbol_by_pc (make_compact_addr (memaddr));
  if (sym.minsym)
    return msymbol_is_micromips (sym.minsym);
  else
    return is_micromips_addr (gdbarch, memaddr);
}

/* Tell the ISA type of the function the program counter value in MEMADDR
   is in.  */

static enum mips_isa
mips_pc_isa (struct gdbarch *gdbarch, CORE_ADDR memaddr)
{
  struct bound_minimal_symbol sym;

  /* A flag indicating that this is a MIPS16 or a microMIPS function
     is stored by elfread.c in the high bit of the info field.  Use
     this to decide if the function is MIPS16 or microMIPS or normal
     MIPS.  Otherwise if bit 0 of the address is set, then ELF file
     flags will tell if this is a MIPS16 or a microMIPS function.  */
  sym = lookup_minimal_symbol_by_pc (make_compact_addr (memaddr));
  if (sym.minsym)
    {
      if (msymbol_is_micromips (sym.minsym))
	return ISA_MICROMIPS;
      else if (msymbol_is_mips16 (sym.minsym))
	return ISA_MIPS16;
      else
	return ISA_MIPS;
    }
  else
    {
      if (is_mips_addr (memaddr))
	return ISA_MIPS;
      else if (is_micromips_addr (gdbarch, memaddr))
	return ISA_MICROMIPS;
      else
	return ISA_MIPS16;
    }
}

/* Set the ISA bit correctly in the PC, used by DWARF-2 machinery.
   The need for comes from the ISA bit having been cleared, making
   addresses in FDE, range records, etc. referring to compressed code
   different to those in line information, the symbol table and finally
   the PC register.  That in turn confuses many operations.  */

static CORE_ADDR
mips_adjust_dwarf2_addr (CORE_ADDR pc)
{
  pc = unmake_compact_addr (pc);
  return mips_pc_is_mips (pc) ? pc : make_compact_addr (pc);
}

/* Recalculate the line record requested so that the resulting PC has
   the ISA bit set correctly, used by DWARF-2 machinery.  The need for
   this adjustment comes from some records associated with compressed
   code having the ISA bit cleared, most notably at function prologue
   ends.  The ISA bit is in this context retrieved from the minimal
   symbol covering the address requested, which in turn has been
   constructed from the binary's symbol table rather than DWARF-2
   information.  The correct setting of the ISA bit is required for
   breakpoint addresses to correctly match against the stop PC.

   As line entries can specify relative address adjustments we need to
   keep track of the absolute value of the last line address recorded
   in line information, so that we can calculate the actual address to
   apply the ISA bit adjustment to.  We use PC for this tracking and
   keep the original address there.

   As such relative address adjustments can be odd within compressed
   code we need to keep track of the last line address with the ISA
   bit adjustment applied too, as the original address may or may not
   have had the ISA bit set.  We use ADJ_PC for this tracking and keep
   the adjusted address there.

   For relative address adjustments we then use these variables to
   calculate the address intended by line information, which will be
   PC-relative, and return an updated adjustment carrying ISA bit
   information, which will be ADJ_PC-relative.  For absolute address
   adjustments we just return the same address that we store in ADJ_PC
   too.

   As the first line entry can be relative to an implied address value
   of 0 we need to have the initial address set up that we store in PC
   and ADJ_PC.  This is arranged with a call from `dwarf_decode_lines_1'
   that sets PC to 0 and ADJ_PC accordingly, usually 0 as well.  */

static CORE_ADDR
mips_adjust_dwarf2_line (CORE_ADDR addr, int rel)
{
  static CORE_ADDR adj_pc;
  static CORE_ADDR pc;
  CORE_ADDR isa_pc;

  pc = rel ? pc + addr : addr;
  isa_pc = mips_adjust_dwarf2_addr (pc);
  addr = rel ? isa_pc - adj_pc : isa_pc;
  adj_pc = isa_pc;
  return addr;
}

/* Various MIPS16 thunk (aka stub or trampoline) names.  */

static const char mips_str_mips16_call_stub[] = "__mips16_call_stub_";
static const char mips_str_mips16_ret_stub[] = "__mips16_ret_";
static const char mips_str_call_fp_stub[] = "__call_stub_fp_";
static const char mips_str_call_stub[] = "__call_stub_";
static const char mips_str_fn_stub[] = "__fn_stub_";

/* This is used as a PIC thunk prefix.  */

static const char mips_str_pic[] = ".pic.";

/* Return non-zero if the PC is inside a call thunk (aka stub or
   trampoline) that should be treated as a temporary frame.  */

static int
mips_in_frame_stub (CORE_ADDR pc)
{
  CORE_ADDR start_addr;
  const char *name;

  /* Find the starting address of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_call_stub_*, this is a call/return stub.  */
  if (startswith (name, mips_str_mips16_call_stub))
    return 1;
  /* If the PC is in __call_stub_*, this is a call/return or a call stub.  */
  if (startswith (name, mips_str_call_stub))
    return 1;
  /* If the PC is in __fn_stub_*, this is a call stub.  */
  if (startswith (name, mips_str_fn_stub))
    return 1;

  return 0;			/* Not a stub.  */
}

/* MIPS believes that the PC has a sign extended value.  Perhaps the
   all registers should be sign extended for simplicity?  */

static CORE_ADDR
mips_read_pc (readable_regcache *regcache)
{
  int regnum = gdbarch_pc_regnum (regcache->arch ());
  LONGEST pc;

  regcache->cooked_read (regnum, &pc);
  return pc;
}

static CORE_ADDR
mips_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  CORE_ADDR pc;

  pc = frame_unwind_register_signed (next_frame, gdbarch_pc_regnum (gdbarch));
  /* macro/2012-04-20: This hack skips over MIPS16 call thunks as
     intermediate frames.  In this case we can get the caller's address
     from $ra, or if $ra contains an address within a thunk as well, then
     it must be in the return path of __mips16_call_stub_{s,d}{f,c}_{0..10}
     and thus the caller's address is in $s2.  */
  if (frame_relative_level (next_frame) >= 0 && mips_in_frame_stub (pc))
    {
      pc = frame_unwind_register_signed
	     (next_frame, gdbarch_num_regs (gdbarch) + MIPS_RA_REGNUM);
      if (mips_in_frame_stub (pc))
	pc = frame_unwind_register_signed
	       (next_frame, gdbarch_num_regs (gdbarch) + MIPS_S2_REGNUM);
    }
  return pc;
}

static CORE_ADDR
mips_unwind_sp (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  return frame_unwind_register_signed
	   (next_frame, gdbarch_num_regs (gdbarch) + MIPS_SP_REGNUM);
}

/* Assuming THIS_FRAME is a dummy, return the frame ID of that
   dummy frame.  The frame ID's base needs to match the TOS value
   saved by save_dummy_frame_tos(), and the PC match the dummy frame's
   breakpoint.  */

static struct frame_id
mips_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  return frame_id_build
	   (get_frame_register_signed (this_frame,
				       gdbarch_num_regs (gdbarch)
				       + MIPS_SP_REGNUM),
	    get_frame_pc (this_frame));
}

/* Implement the "write_pc" gdbarch method.  */

void
mips_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  int regnum = gdbarch_pc_regnum (regcache->arch ());

  regcache_cooked_write_unsigned (regcache, regnum, pc);
}

/* Fetch and return instruction from the specified location.  Handle
   MIPS16/microMIPS as appropriate.  */

static ULONGEST
mips_fetch_instruction (struct gdbarch *gdbarch,
			enum mips_isa isa, CORE_ADDR addr, int *errp)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[MIPS_INSN32_SIZE];
  int instlen;
  int err;

  switch (isa)
    {
    case ISA_MICROMIPS:
    case ISA_MIPS16:
      instlen = MIPS_INSN16_SIZE;
      addr = unmake_compact_addr (addr);
      break;
    case ISA_MIPS:
      instlen = MIPS_INSN32_SIZE;
      break;
    default:
      internal_error (_("invalid ISA"));
      break;
    }
  err = target_read_memory (addr, buf, instlen);
  if (errp != NULL)
    *errp = err;
  if (err != 0)
    {
      if (errp == NULL)
	memory_error (TARGET_XFER_E_IO, addr);
      return 0;
    }
  return extract_unsigned_integer (buf, instlen, byte_order);
}

/* These are the fields of 32 bit mips instructions.  */
#define mips32_op(x) (x >> 26)
#define itype_op(x) (x >> 26)
#define itype_rs(x) ((x >> 21) & 0x1f)
#define itype_rt(x) ((x >> 16) & 0x1f)
#define itype_immediate(x) (x & 0xffff)

#define jtype_op(x) (x >> 26)
#define jtype_target(x) (x & 0x03ffffff)

#define rtype_op(x) (x >> 26)
#define rtype_rs(x) ((x >> 21) & 0x1f)
#define rtype_rt(x) ((x >> 16) & 0x1f)
#define rtype_rd(x) ((x >> 11) & 0x1f)
#define rtype_shamt(x) ((x >> 6) & 0x1f)
#define rtype_funct(x) (x & 0x3f)

/* MicroMIPS instruction fields.  */
#define micromips_op(x) ((x) >> 10)

/* 16-bit/32-bit-high-part instruction formats, B and S refer to the lowest
   bit and the size respectively of the field extracted.  */
#define b0s4_imm(x) ((x) & 0xf)
#define b0s5_imm(x) ((x) & 0x1f)
#define b0s5_reg(x) ((x) & 0x1f)
#define b0s7_imm(x) ((x) & 0x7f)
#define b0s10_imm(x) ((x) & 0x3ff)
#define b1s4_imm(x) (((x) >> 1) & 0xf)
#define b1s9_imm(x) (((x) >> 1) & 0x1ff)
#define b2s3_cc(x) (((x) >> 2) & 0x7)
#define b4s2_regl(x) (((x) >> 4) & 0x3)
#define b5s5_op(x) (((x) >> 5) & 0x1f)
#define b5s5_reg(x) (((x) >> 5) & 0x1f)
#define b6s4_op(x) (((x) >> 6) & 0xf)
#define b7s3_reg(x) (((x) >> 7) & 0x7)

/* 32-bit instruction formats, B and S refer to the lowest bit and the size
   respectively of the field extracted.  */
#define b0s6_op(x) ((x) & 0x3f)
#define b0s11_op(x) ((x) & 0x7ff)
#define b0s12_imm(x) ((x) & 0xfff)
#define b0s16_imm(x) ((x) & 0xffff)
#define b0s26_imm(x) ((x) & 0x3ffffff)
#define b6s10_ext(x) (((x) >> 6) & 0x3ff)
#define b11s5_reg(x) (((x) >> 11) & 0x1f)
#define b12s4_op(x) (((x) >> 12) & 0xf)

/* Return the size in bytes of the instruction INSN encoded in the ISA
   instruction set.  */

static int
mips_insn_size (enum mips_isa isa, ULONGEST insn)
{
  switch (isa)
    {
    case ISA_MICROMIPS:
      if ((micromips_op (insn) & 0x4) == 0x4
	  || (micromips_op (insn) & 0x7) == 0x0)
	return 2 * MIPS_INSN16_SIZE;
      else
	return MIPS_INSN16_SIZE;
    case ISA_MIPS16:
      if ((insn & 0xf800) == 0xf000)
	return 2 * MIPS_INSN16_SIZE;
      else
	return MIPS_INSN16_SIZE;
    case ISA_MIPS:
	return MIPS_INSN32_SIZE;
    }
  internal_error (_("invalid ISA"));
}

static LONGEST
mips32_relative_offset (ULONGEST inst)
{
  return ((itype_immediate (inst) ^ 0x8000) - 0x8000) << 2;
}

/* Determine the address of the next instruction executed after the INST
   floating condition branch instruction at PC.  COUNT specifies the
   number of the floating condition bits tested by the branch.  */

static CORE_ADDR
mips32_bc1_pc (struct gdbarch *gdbarch, struct regcache *regcache,
	       ULONGEST inst, CORE_ADDR pc, int count)
{
  int fcsr = mips_regnum (gdbarch)->fp_control_status;
  int cnum = (itype_rt (inst) >> 2) & (count - 1);
  int tf = itype_rt (inst) & 1;
  int mask = (1 << count) - 1;
  ULONGEST fcs;
  int cond;

  if (fcsr == -1)
    /* No way to handle; it'll most likely trap anyway.  */
    return pc;

  fcs = regcache_raw_get_unsigned (regcache, fcsr);
  cond = ((fcs >> 24) & 0xfe) | ((fcs >> 23) & 0x01);

  if (((cond >> cnum) & mask) != mask * !tf)
    pc += mips32_relative_offset (inst);
  else
    pc += 4;

  return pc;
}

/* Return nonzero if the gdbarch is an Octeon series.  */

static int
is_octeon (struct gdbarch *gdbarch)
{
  const struct bfd_arch_info *info = gdbarch_bfd_arch_info (gdbarch);

  return (info->mach == bfd_mach_mips_octeon
	 || info->mach == bfd_mach_mips_octeonp
	 || info->mach == bfd_mach_mips_octeon2);
}

/* Return true if the OP represents the Octeon's BBIT instruction.  */

static int
is_octeon_bbit_op (int op, struct gdbarch *gdbarch)
{
  if (!is_octeon (gdbarch))
    return 0;
  /* BBIT0 is encoded as LWC2: 110 010.  */
  /* BBIT032 is encoded as LDC2: 110 110.  */
  /* BBIT1 is encoded as SWC2: 111 010.  */
  /* BBIT132 is encoded as SDC2: 111 110.  */
  if (op == 50 || op == 54 || op == 58 || op == 62)
    return 1;
  return 0;
}


/* Determine where to set a single step breakpoint while considering
   branch prediction.  */

static CORE_ADDR
mips32_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  unsigned long inst;
  int op;
  inst = mips_fetch_instruction (gdbarch, ISA_MIPS, pc, NULL);
  op = itype_op (inst);
  if ((inst & 0xe0000000) != 0)		/* Not a special, jump or branch
					   instruction.  */
    {
      if (op >> 2 == 5)
	/* BEQL, BNEL, BLEZL, BGTZL: bits 0101xx */
	{
	  switch (op & 0x03)
	    {
	    case 0:		/* BEQL */
	      goto equal_branch;
	    case 1:		/* BNEL */
	      goto neq_branch;
	    case 2:		/* BLEZL */
	      goto less_branch;
	    case 3:		/* BGTZL */
	      goto greater_branch;
	    default:
	      pc += 4;
	    }
	}
      else if (op == 17 && itype_rs (inst) == 8)
	/* BC1F, BC1FL, BC1T, BC1TL: 010001 01000 */
	pc = mips32_bc1_pc (gdbarch, regcache, inst, pc + 4, 1);
      else if (op == 17 && itype_rs (inst) == 9
	       && (itype_rt (inst) & 2) == 0)
	/* BC1ANY2F, BC1ANY2T: 010001 01001 xxx0x */
	pc = mips32_bc1_pc (gdbarch, regcache, inst, pc + 4, 2);
      else if (op == 17 && itype_rs (inst) == 10
	       && (itype_rt (inst) & 2) == 0)
	/* BC1ANY4F, BC1ANY4T: 010001 01010 xxx0x */
	pc = mips32_bc1_pc (gdbarch, regcache, inst, pc + 4, 4);
      else if (op == 29)
	/* JALX: 011101 */
	/* The new PC will be alternate mode.  */
	{
	  unsigned long reg;

	  reg = jtype_target (inst) << 2;
	  /* Add 1 to indicate 16-bit mode -- invert ISA mode.  */
	  pc = ((pc + 4) & ~(CORE_ADDR) 0x0fffffff) + reg + 1;
	}
      else if (is_octeon_bbit_op (op, gdbarch))
	{
	  int bit, branch_if;

	  branch_if = op == 58 || op == 62;
	  bit = itype_rt (inst);

	  /* Take into account the *32 instructions.  */
	  if (op == 54 || op == 62)
	    bit += 32;

	  if (((regcache_raw_get_signed (regcache,
					 itype_rs (inst)) >> bit) & 1)
	      == branch_if)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;        /* After the delay slot.  */
	}

      else
	pc += 4;		/* Not a branch, next instruction is easy.  */
    }
  else
    {				/* This gets way messy.  */

      /* Further subdivide into SPECIAL, REGIMM and other.  */
      switch (op & 0x07)	/* Extract bits 28,27,26.  */
	{
	case 0:		/* SPECIAL */
	  op = rtype_funct (inst);
	  switch (op)
	    {
	    case 8:		/* JR */
	    case 9:		/* JALR */
	      /* Set PC to that address.  */
	      pc = regcache_raw_get_signed (regcache, rtype_rs (inst));
	      break;
	    case 12:            /* SYSCALL */
	      {
		mips_gdbarch_tdep *tdep
		  = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

		if (tdep->syscall_next_pc != NULL)
		  pc = tdep->syscall_next_pc (get_current_frame ());
		else
		  pc += 4;
	      }
	      break;
	    default:
	      pc += 4;
	    }

	  break;		/* end SPECIAL */
	case 1:			/* REGIMM */
	  {
	    op = itype_rt (inst);	/* branch condition */
	    switch (op)
	      {
	      case 0:		/* BLTZ */
	      case 2:		/* BLTZL */
	      case 16:		/* BLTZAL */
	      case 18:		/* BLTZALL */
	      less_branch:
		if (regcache_raw_get_signed (regcache, itype_rs (inst)) < 0)
		  pc += mips32_relative_offset (inst) + 4;
		else
		  pc += 8;	/* after the delay slot */
		break;
	      case 1:		/* BGEZ */
	      case 3:		/* BGEZL */
	      case 17:		/* BGEZAL */
	      case 19:		/* BGEZALL */
		if (regcache_raw_get_signed (regcache, itype_rs (inst)) >= 0)
		  pc += mips32_relative_offset (inst) + 4;
		else
		  pc += 8;	/* after the delay slot */
		break;
	      case 0x1c:	/* BPOSGE32 */
	      case 0x1e:	/* BPOSGE64 */
		pc += 4;
		if (itype_rs (inst) == 0)
		  {
		    unsigned int pos = (op & 2) ? 64 : 32;
		    int dspctl = mips_regnum (gdbarch)->dspctl;

		    if (dspctl == -1)
		      /* No way to handle; it'll most likely trap anyway.  */
		      break;

		    if ((regcache_raw_get_unsigned (regcache,
						    dspctl) & 0x7f) >= pos)
		      pc += mips32_relative_offset (inst);
		    else
		      pc += 4;
		  }
		break;
		/* All of the other instructions in the REGIMM category */
	      default:
		pc += 4;
	      }
	  }
	  break;		/* end REGIMM */
	case 2:		/* J */
	case 3:		/* JAL */
	  {
	    unsigned long reg;
	    reg = jtype_target (inst) << 2;
	    /* Upper four bits get never changed...  */
	    pc = reg + ((pc + 4) & ~(CORE_ADDR) 0x0fffffff);
	  }
	  break;
	case 4:		/* BEQ, BEQL */
	equal_branch:
	  if (regcache_raw_get_signed (regcache, itype_rs (inst)) ==
	      regcache_raw_get_signed (regcache, itype_rt (inst)))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 5:		/* BNE, BNEL */
	neq_branch:
	  if (regcache_raw_get_signed (regcache, itype_rs (inst)) !=
	      regcache_raw_get_signed (regcache, itype_rt (inst)))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 6:		/* BLEZ, BLEZL */
	  if (regcache_raw_get_signed (regcache, itype_rs (inst)) <= 0)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 7:
	default:
	greater_branch:	/* BGTZ, BGTZL */
	  if (regcache_raw_get_signed (regcache, itype_rs (inst)) > 0)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	}			/* switch */
    }				/* else */
  return pc;
}				/* mips32_next_pc */

/* Extract the 7-bit signed immediate offset from the microMIPS instruction
   INSN.  */

static LONGEST
micromips_relative_offset7 (ULONGEST insn)
{
  return ((b0s7_imm (insn) ^ 0x40) - 0x40) << 1;
}

/* Extract the 10-bit signed immediate offset from the microMIPS instruction
   INSN.  */

static LONGEST
micromips_relative_offset10 (ULONGEST insn)
{
  return ((b0s10_imm (insn) ^ 0x200) - 0x200) << 1;
}

/* Extract the 16-bit signed immediate offset from the microMIPS instruction
   INSN.  */

static LONGEST
micromips_relative_offset16 (ULONGEST insn)
{
  return ((b0s16_imm (insn) ^ 0x8000) - 0x8000) << 1;
}

/* Return the size in bytes of the microMIPS instruction at the address PC.  */

static int
micromips_pc_insn_size (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  ULONGEST insn;

  insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, pc, NULL);
  return mips_insn_size (ISA_MICROMIPS, insn);
}

/* Calculate the address of the next microMIPS instruction to execute
   after the INSN coprocessor 1 conditional branch instruction at the
   address PC.  COUNT denotes the number of coprocessor condition bits
   examined by the branch.  */

static CORE_ADDR
micromips_bc1_pc (struct gdbarch *gdbarch, struct regcache *regcache,
		  ULONGEST insn, CORE_ADDR pc, int count)
{
  int fcsr = mips_regnum (gdbarch)->fp_control_status;
  int cnum = b2s3_cc (insn >> 16) & (count - 1);
  int tf = b5s5_op (insn >> 16) & 1;
  int mask = (1 << count) - 1;
  ULONGEST fcs;
  int cond;

  if (fcsr == -1)
    /* No way to handle; it'll most likely trap anyway.  */
    return pc;

  fcs = regcache_raw_get_unsigned (regcache, fcsr);
  cond = ((fcs >> 24) & 0xfe) | ((fcs >> 23) & 0x01);

  if (((cond >> cnum) & mask) != mask * !tf)
    pc += micromips_relative_offset16 (insn);
  else
    pc += micromips_pc_insn_size (gdbarch, pc);

  return pc;
}

/* Calculate the address of the next microMIPS instruction to execute
   after the instruction at the address PC.  */

static CORE_ADDR
micromips_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ULONGEST insn;

  insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, pc, NULL);
  pc += MIPS_INSN16_SIZE;
  switch (mips_insn_size (ISA_MICROMIPS, insn))
    {
    /* 32-bit instructions.  */
    case 2 * MIPS_INSN16_SIZE:
      insn <<= 16;
      insn |= mips_fetch_instruction (gdbarch, ISA_MICROMIPS, pc, NULL);
      pc += MIPS_INSN16_SIZE;
      switch (micromips_op (insn >> 16))
	{
	case 0x00: /* POOL32A: bits 000000 */
	  switch (b0s6_op (insn))
	    {
	    case 0x3c: /* POOL32Axf: bits 000000 ... 111100 */
	      switch (b6s10_ext (insn))
		{
		case 0x3c:  /* JALR:     000000 0000111100 111100 */
		case 0x7c:  /* JALR.HB:  000000 0001111100 111100 */
		case 0x13c: /* JALRS:    000000 0100111100 111100 */
		case 0x17c: /* JALRS.HB: 000000 0101111100 111100 */
		  pc = regcache_raw_get_signed (regcache,
						b0s5_reg (insn >> 16));
		  break;
		case 0x22d: /* SYSCALL:  000000 1000101101 111100 */
		  {
		    mips_gdbarch_tdep *tdep
		      = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

		    if (tdep->syscall_next_pc != NULL)
		      pc = tdep->syscall_next_pc (get_current_frame ());
		  }
		  break;
		}
	      break;
	    }
	  break;

	case 0x10: /* POOL32I: bits 010000 */
	  switch (b5s5_op (insn >> 16))
	    {
	    case 0x00: /* BLTZ: bits 010000 00000 */
	    case 0x01: /* BLTZAL: bits 010000 00001 */
	    case 0x11: /* BLTZALS: bits 010000 10001 */
	      if (regcache_raw_get_signed (regcache,
					   b0s5_reg (insn >> 16)) < 0)
		pc += micromips_relative_offset16 (insn);
	      else
		pc += micromips_pc_insn_size (gdbarch, pc);
	      break;

	    case 0x02: /* BGEZ: bits 010000 00010 */
	    case 0x03: /* BGEZAL: bits 010000 00011 */
	    case 0x13: /* BGEZALS: bits 010000 10011 */
	      if (regcache_raw_get_signed (regcache,
					   b0s5_reg (insn >> 16)) >= 0)
		pc += micromips_relative_offset16 (insn);
	      else
		pc += micromips_pc_insn_size (gdbarch, pc);
	      break;

	    case 0x04: /* BLEZ: bits 010000 00100 */
	      if (regcache_raw_get_signed (regcache,
					   b0s5_reg (insn >> 16)) <= 0)
		pc += micromips_relative_offset16 (insn);
	      else
		pc += micromips_pc_insn_size (gdbarch, pc);
	      break;

	    case 0x05: /* BNEZC: bits 010000 00101 */
	      if (regcache_raw_get_signed (regcache,
					   b0s5_reg (insn >> 16)) != 0)
		pc += micromips_relative_offset16 (insn);
	      break;

	    case 0x06: /* BGTZ: bits 010000 00110 */
	      if (regcache_raw_get_signed (regcache,
					   b0s5_reg (insn >> 16)) > 0)
		pc += micromips_relative_offset16 (insn);
	      else
		pc += micromips_pc_insn_size (gdbarch, pc);
	      break;

	    case 0x07: /* BEQZC: bits 010000 00111 */
	      if (regcache_raw_get_signed (regcache,
					   b0s5_reg (insn >> 16)) == 0)
		pc += micromips_relative_offset16 (insn);
	      break;

	    case 0x14: /* BC2F: bits 010000 10100 xxx00 */
	    case 0x15: /* BC2T: bits 010000 10101 xxx00 */
	      if (((insn >> 16) & 0x3) == 0x0)
		/* BC2F, BC2T: don't know how to handle these.  */
		break;
	      break;

	    case 0x1a: /* BPOSGE64: bits 010000 11010 */
	    case 0x1b: /* BPOSGE32: bits 010000 11011 */
	      {
		unsigned int pos = (b5s5_op (insn >> 16) & 1) ? 32 : 64;
		int dspctl = mips_regnum (gdbarch)->dspctl;

		if (dspctl == -1)
		  /* No way to handle; it'll most likely trap anyway.  */
		  break;

		if ((regcache_raw_get_unsigned (regcache,
						dspctl) & 0x7f) >= pos)
		  pc += micromips_relative_offset16 (insn);
		else
		  pc += micromips_pc_insn_size (gdbarch, pc);
	      }
	      break;

	    case 0x1c: /* BC1F: bits 010000 11100 xxx00 */
		       /* BC1ANY2F: bits 010000 11100 xxx01 */
	    case 0x1d: /* BC1T: bits 010000 11101 xxx00 */
		       /* BC1ANY2T: bits 010000 11101 xxx01 */
	      if (((insn >> 16) & 0x2) == 0x0)
		pc = micromips_bc1_pc (gdbarch, regcache, insn, pc,
				       ((insn >> 16) & 0x1) + 1);
	      break;

	    case 0x1e: /* BC1ANY4F: bits 010000 11110 xxx01 */
	    case 0x1f: /* BC1ANY4T: bits 010000 11111 xxx01 */
	      if (((insn >> 16) & 0x3) == 0x1)
		pc = micromips_bc1_pc (gdbarch, regcache, insn, pc, 4);
	      break;
	    }
	  break;

	case 0x1d: /* JALS: bits 011101 */
	case 0x35: /* J: bits 110101 */
	case 0x3d: /* JAL: bits 111101 */
	    pc = ((pc | 0x7fffffe) ^ 0x7fffffe) | (b0s26_imm (insn) << 1);
	  break;

	case 0x25: /* BEQ: bits 100101 */
	    if (regcache_raw_get_signed (regcache, b0s5_reg (insn >> 16))
		== regcache_raw_get_signed (regcache, b5s5_reg (insn >> 16)))
	      pc += micromips_relative_offset16 (insn);
	    else
	      pc += micromips_pc_insn_size (gdbarch, pc);
	  break;

	case 0x2d: /* BNE: bits 101101 */
	  if (regcache_raw_get_signed (regcache, b0s5_reg (insn >> 16))
		!= regcache_raw_get_signed (regcache, b5s5_reg (insn >> 16)))
	      pc += micromips_relative_offset16 (insn);
	  else
	      pc += micromips_pc_insn_size (gdbarch, pc);
	  break;

	case 0x3c: /* JALX: bits 111100 */
	    pc = ((pc | 0xfffffff) ^ 0xfffffff) | (b0s26_imm (insn) << 2);
	  break;
	}
      break;

    /* 16-bit instructions.  */
    case MIPS_INSN16_SIZE:
      switch (micromips_op (insn))
	{
	case 0x11: /* POOL16C: bits 010001 */
	  if ((b5s5_op (insn) & 0x1c) == 0xc)
	    /* JR16, JRC, JALR16, JALRS16: 010001 011xx */
	    pc = regcache_raw_get_signed (regcache, b0s5_reg (insn));
	  else if (b5s5_op (insn) == 0x18)
	    /* JRADDIUSP: bits 010001 11000 */
	    pc = regcache_raw_get_signed (regcache, MIPS_RA_REGNUM);
	  break;

	case 0x23: /* BEQZ16: bits 100011 */
	  {
	    int rs = mips_reg3_to_reg[b7s3_reg (insn)];

	    if (regcache_raw_get_signed (regcache, rs) == 0)
	      pc += micromips_relative_offset7 (insn);
	    else
	      pc += micromips_pc_insn_size (gdbarch, pc);
	  }
	  break;

	case 0x2b: /* BNEZ16: bits 101011 */
	  {
	    int rs = mips_reg3_to_reg[b7s3_reg (insn)];

	    if (regcache_raw_get_signed (regcache, rs) != 0)
	      pc += micromips_relative_offset7 (insn);
	    else
	      pc += micromips_pc_insn_size (gdbarch, pc);
	  }
	  break;

	case 0x33: /* B16: bits 110011 */
	  pc += micromips_relative_offset10 (insn);
	  break;
	}
      break;
    }

  return pc;
}

/* Decoding the next place to set a breakpoint is irregular for the
   mips 16 variant, but fortunately, there fewer instructions.  We have
   to cope ith extensions for 16 bit instructions and a pair of actual
   32 bit instructions.  We dont want to set a single step instruction
   on the extend instruction either.  */

/* Lots of mips16 instruction formats */
/* Predicting jumps requires itype,ritype,i8type
   and their extensions      extItype,extritype,extI8type.  */
enum mips16_inst_fmts
{
  itype,			/* 0  immediate 5,10 */
  ritype,			/* 1   5,3,8 */
  rrtype,			/* 2   5,3,3,5 */
  rritype,			/* 3   5,3,3,5 */
  rrrtype,			/* 4   5,3,3,3,2 */
  rriatype,			/* 5   5,3,3,1,4 */
  shifttype,			/* 6   5,3,3,3,2 */
  i8type,			/* 7   5,3,8 */
  i8movtype,			/* 8   5,3,3,5 */
  i8mov32rtype,			/* 9   5,3,5,3 */
  i64type,			/* 10  5,3,8 */
  ri64type,			/* 11  5,3,3,5 */
  jalxtype,			/* 12  5,1,5,5,16 - a 32 bit instruction */
  exiItype,			/* 13  5,6,5,5,1,1,1,1,1,1,5 */
  extRitype,			/* 14  5,6,5,5,3,1,1,1,5 */
  extRRItype,			/* 15  5,5,5,5,3,3,5 */
  extRRIAtype,			/* 16  5,7,4,5,3,3,1,4 */
  EXTshifttype,			/* 17  5,5,1,1,1,1,1,1,5,3,3,1,1,1,2 */
  extI8type,			/* 18  5,6,5,5,3,1,1,1,5 */
  extI64type,			/* 19  5,6,5,5,3,1,1,1,5 */
  extRi64type,			/* 20  5,6,5,5,3,3,5 */
  extshift64type		/* 21  5,5,1,1,1,1,1,1,5,1,1,1,3,5 */
};
/* I am heaping all the fields of the formats into one structure and
   then, only the fields which are involved in instruction extension.  */
struct upk_mips16
{
  CORE_ADDR offset;
  unsigned int regx;		/* Function in i8 type.  */
  unsigned int regy;
};


/* The EXT-I, EXT-ri nad EXT-I8 instructions all have the same format
   for the bits which make up the immediate extension.  */

static CORE_ADDR
extended_offset (unsigned int extension)
{
  CORE_ADDR value;

  value = (extension >> 16) & 0x1f;	/* Extract 15:11.  */
  value = value << 6;
  value |= (extension >> 21) & 0x3f;	/* Extract 10:5.  */
  value = value << 5;
  value |= extension & 0x1f;		/* Extract 4:0.  */

  return value;
}

/* Only call this function if you know that this is an extendable
   instruction.  It won't malfunction, but why make excess remote memory
   references?  If the immediate operands get sign extended or something,
   do it after the extension is performed.  */
/* FIXME: Every one of these cases needs to worry about sign extension
   when the offset is to be used in relative addressing.  */

static unsigned int
fetch_mips_16 (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];

  pc = unmake_compact_addr (pc);	/* Clear the low order bit.  */
  target_read_memory (pc, buf, 2);
  return extract_unsigned_integer (buf, 2, byte_order);
}

static void
unpack_mips16 (struct gdbarch *gdbarch, CORE_ADDR pc,
	       unsigned int extension,
	       unsigned int inst,
	       enum mips16_inst_fmts insn_format, struct upk_mips16 *upk)
{
  CORE_ADDR offset;
  int regx;
  int regy;
  switch (insn_format)
    {
    case itype:
      {
	CORE_ADDR value;
	if (extension)
	  {
	    value = extended_offset ((extension << 16) | inst);
	    value = (value ^ 0x8000) - 0x8000;		/* Sign-extend.  */
	  }
	else
	  {
	    value = inst & 0x7ff;
	    value = (value ^ 0x400) - 0x400;		/* Sign-extend.  */
	  }
	offset = value;
	regx = -1;
	regy = -1;
      }
      break;
    case ritype:
    case i8type:
      {				/* A register identifier and an offset.  */
	/* Most of the fields are the same as I type but the
	   immediate value is of a different length.  */
	CORE_ADDR value;
	if (extension)
	  {
	    value = extended_offset ((extension << 16) | inst);
	    value = (value ^ 0x8000) - 0x8000;		/* Sign-extend.  */
	  }
	else
	  {
	    value = inst & 0xff;			/* 8 bits */
	    value = (value ^ 0x80) - 0x80;		/* Sign-extend.  */
	  }
	offset = value;
	regx = (inst >> 8) & 0x07;			/* i8 funct */
	regy = -1;
	break;
      }
    case jalxtype:
      {
	unsigned long value;
	unsigned int nexthalf;
	value = ((inst & 0x1f) << 5) | ((inst >> 5) & 0x1f);
	value = value << 16;
	nexthalf = mips_fetch_instruction (gdbarch, ISA_MIPS16, pc + 2, NULL);
						/* Low bit still set.  */
	value |= nexthalf;
	offset = value;
	regx = -1;
	regy = -1;
	break;
      }
    default:
      internal_error (_("bad switch"));
    }
  upk->offset = offset;
  upk->regx = regx;
  upk->regy = regy;
}


/* Calculate the destination of a branch whose 16-bit opcode word is at PC,
   and having a signed 16-bit OFFSET.  */

static CORE_ADDR
add_offset_16 (CORE_ADDR pc, int offset)
{
  return pc + (offset << 1) + 2;
}

static CORE_ADDR
extended_mips16_next_pc (regcache *regcache, CORE_ADDR pc,
			 unsigned int extension, unsigned int insn)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int op = (insn >> 11);
  switch (op)
    {
    case 2:			/* Branch */
      {
	struct upk_mips16 upk;
	unpack_mips16 (gdbarch, pc, extension, insn, itype, &upk);
	pc = add_offset_16 (pc, upk.offset);
	break;
      }
    case 3:			/* JAL , JALX - Watch out, these are 32 bit
				   instructions.  */
      {
	struct upk_mips16 upk;
	unpack_mips16 (gdbarch, pc, extension, insn, jalxtype, &upk);
	pc = ((pc + 2) & (~(CORE_ADDR) 0x0fffffff)) | (upk.offset << 2);
	if ((insn >> 10) & 0x01)	/* Exchange mode */
	  pc = pc & ~0x01;	/* Clear low bit, indicate 32 bit mode.  */
	else
	  pc |= 0x01;
	break;
      }
    case 4:			/* beqz */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (gdbarch, pc, extension, insn, ritype, &upk);
	reg = regcache_raw_get_signed (regcache, mips_reg3_to_reg[upk.regx]);
	if (reg == 0)
	  pc = add_offset_16 (pc, upk.offset);
	else
	  pc += 2;
	break;
      }
    case 5:			/* bnez */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (gdbarch, pc, extension, insn, ritype, &upk);
	reg = regcache_raw_get_signed (regcache, mips_reg3_to_reg[upk.regx]);
	if (reg != 0)
	  pc = add_offset_16 (pc, upk.offset);
	else
	  pc += 2;
	break;
      }
    case 12:			/* I8 Formats btez btnez */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (gdbarch, pc, extension, insn, i8type, &upk);
	/* upk.regx contains the opcode */
	/* Test register is 24 */
	reg = regcache_raw_get_signed (regcache, 24);
	if (((upk.regx == 0) && (reg == 0))	/* BTEZ */
	    || ((upk.regx == 1) && (reg != 0)))	/* BTNEZ */
	  pc = add_offset_16 (pc, upk.offset);
	else
	  pc += 2;
	break;
      }
    case 29:			/* RR Formats JR, JALR, JALR-RA */
      {
	struct upk_mips16 upk;
	/* upk.fmt = rrtype; */
	op = insn & 0x1f;
	if (op == 0)
	  {
	    int reg;
	    upk.regx = (insn >> 8) & 0x07;
	    upk.regy = (insn >> 5) & 0x07;
	    if ((upk.regy & 1) == 0)
	      reg = mips_reg3_to_reg[upk.regx];
	    else
	      reg = 31;		/* Function return instruction.  */
	    pc = regcache_raw_get_signed (regcache, reg);
	  }
	else
	  pc += 2;
	break;
      }
    case 30:
      /* This is an instruction extension.  Fetch the real instruction
	 (which follows the extension) and decode things based on
	 that.  */
      {
	pc += 2;
	pc = extended_mips16_next_pc (regcache, pc, insn,
				      fetch_mips_16 (gdbarch, pc));
	break;
      }
    default:
      {
	pc += 2;
	break;
      }
    }
  return pc;
}

static CORE_ADDR
mips16_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  unsigned int insn = fetch_mips_16 (gdbarch, pc);
  return extended_mips16_next_pc (regcache, pc, 0, insn);
}

/* The mips_next_pc function supports single_step when the remote
   target monitor or stub is not developed enough to do a single_step.
   It works by decoding the current instruction and predicting where a
   branch will go.  This isn't hard because all the data is available.
   The MIPS32, MIPS16 and microMIPS variants are quite different.  */
static CORE_ADDR
mips_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();

  if (mips_pc_is_mips16 (gdbarch, pc))
    return mips16_next_pc (regcache, pc);
  else if (mips_pc_is_micromips (gdbarch, pc))
    return micromips_next_pc (regcache, pc);
  else
    return mips32_next_pc (regcache, pc);
}

/* Return non-zero if the MIPS16 instruction INSN is a compact branch
   or jump.  */

static int
mips16_instruction_is_compact_branch (unsigned short insn)
{
  switch (insn & 0xf800)
    {
    case 0xe800:
      return (insn & 0x009f) == 0x80;	/* JALRC/JRC */
    case 0x6000:
      return (insn & 0x0600) == 0;	/* BTNEZ/BTEQZ */
    case 0x2800:			/* BNEZ */
    case 0x2000:			/* BEQZ */
    case 0x1000:			/* B */
      return 1;
    default:
      return 0;
    }
}

/* Return non-zero if the microMIPS instruction INSN is a compact branch
   or jump.  */

static int
micromips_instruction_is_compact_branch (unsigned short insn)
{
  switch (micromips_op (insn))
    {
    case 0x11:			/* POOL16C: bits 010001 */
      return (b5s5_op (insn) == 0x18
				/* JRADDIUSP: bits 010001 11000 */
	      || b5s5_op (insn) == 0xd);
				/* JRC: bits 010011 01101 */
    case 0x10:			/* POOL32I: bits 010000 */
      return (b5s5_op (insn) & 0x1d) == 0x5;
				/* BEQZC/BNEZC: bits 010000 001x1 */
    default:
      return 0;
    }
}

struct mips_frame_cache
{
  CORE_ADDR base;
  trad_frame_saved_reg *saved_regs;
};

/* Set a register's saved stack address in temp_saved_regs.  If an
   address has already been set for this register, do nothing; this
   way we will only recognize the first save of a given register in a
   function prologue.

   For simplicity, save the address in both [0 .. gdbarch_num_regs) and
   [gdbarch_num_regs .. 2*gdbarch_num_regs).
   Strictly speaking, only the second range is used as it is only second
   range (the ABI instead of ISA registers) that comes into play when finding
   saved registers in a frame.  */

static void
set_reg_offset (struct gdbarch *gdbarch, struct mips_frame_cache *this_cache,
		int regnum, CORE_ADDR offset)
{
  if (this_cache != NULL
      && this_cache->saved_regs[regnum].is_realreg ()
      && this_cache->saved_regs[regnum].realreg () == regnum)
    {
      this_cache->saved_regs[regnum + 0
			     * gdbarch_num_regs (gdbarch)].set_addr (offset);
      this_cache->saved_regs[regnum + 1
			     * gdbarch_num_regs (gdbarch)].set_addr (offset);
    }
}


/* Fetch the immediate value from a MIPS16 instruction.
   If the previous instruction was an EXTEND, use it to extend
   the upper bits of the immediate value.  This is a helper function
   for mips16_scan_prologue.  */

static int
mips16_get_imm (unsigned short prev_inst,	/* previous instruction */
		unsigned short inst,	/* current instruction */
		int nbits,	/* number of bits in imm field */
		int scale,	/* scale factor to be applied to imm */
		int is_signed)	/* is the imm field signed?  */
{
  int offset;

  if ((prev_inst & 0xf800) == 0xf000)	/* prev instruction was EXTEND? */
    {
      offset = ((prev_inst & 0x1f) << 11) | (prev_inst & 0x7e0);
      if (offset & 0x8000)	/* check for negative extend */
	offset = 0 - (0x10000 - (offset & 0xffff));
      return offset | (inst & 0x1f);
    }
  else
    {
      int max_imm = 1 << nbits;
      int mask = max_imm - 1;
      int sign_bit = max_imm >> 1;

      offset = inst & mask;
      if (is_signed && (offset & sign_bit))
	offset = 0 - (max_imm - offset);
      return offset * scale;
    }
}


/* Analyze the function prologue from START_PC to LIMIT_PC. Builds
   the associated FRAME_CACHE if not null.
   Return the address of the first instruction past the prologue.  */

static CORE_ADDR
mips16_scan_prologue (struct gdbarch *gdbarch,
		      CORE_ADDR start_pc, CORE_ADDR limit_pc,
		      frame_info_ptr this_frame,
		      struct mips_frame_cache *this_cache)
{
  int prev_non_prologue_insn = 0;
  int this_non_prologue_insn;
  int non_prologue_insns = 0;
  CORE_ADDR prev_pc;
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0;	/* Value of $r17, used as frame pointer.  */
  CORE_ADDR sp;
  long frame_offset = 0;        /* Size of stack frame.  */
  long frame_adjust = 0;        /* Offset of FP from SP.  */
  int frame_reg = MIPS_SP_REGNUM;
  unsigned short prev_inst = 0;	/* saved copy of previous instruction.  */
  unsigned inst = 0;		/* current instruction */
  unsigned entry_inst = 0;	/* the entry instruction */
  unsigned save_inst = 0;	/* the save instruction */
  int prev_delay_slot = 0;
  int in_delay_slot;
  int reg, offset;

  int extend_bytes = 0;
  int prev_extend_bytes = 0;
  CORE_ADDR end_prologue_addr;

  /* Can be called when there's no process, and hence when there's no
     THIS_FRAME.  */
  if (this_frame != NULL)
    sp = get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch)
				    + MIPS_SP_REGNUM);
  else
    sp = 0;

  if (limit_pc > start_pc + 200)
    limit_pc = start_pc + 200;
  prev_pc = start_pc;

  /* Permit at most one non-prologue non-control-transfer instruction
     in the middle which may have been reordered by the compiler for
     optimisation.  */
  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS_INSN16_SIZE)
    {
      this_non_prologue_insn = 0;
      in_delay_slot = 0;

      /* Save the previous instruction.  If it's an EXTEND, we'll extract
	 the immediate offset extension from it in mips16_get_imm.  */
      prev_inst = inst;

      /* Fetch and decode the instruction.  */
      inst = (unsigned short) mips_fetch_instruction (gdbarch, ISA_MIPS16,
						      cur_pc, NULL);

      /* Normally we ignore extend instructions.  However, if it is
	 not followed by a valid prologue instruction, then this
	 instruction is not part of the prologue either.  We must
	 remember in this case to adjust the end_prologue_addr back
	 over the extend.  */
      if ((inst & 0xf800) == 0xf000)    /* extend */
	{
	  extend_bytes = MIPS_INSN16_SIZE;
	  continue;
	}

      prev_extend_bytes = extend_bytes;
      extend_bytes = 0;

      if ((inst & 0xff00) == 0x6300	/* addiu sp */
	  || (inst & 0xff00) == 0xfb00)	/* daddiu sp */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 1);
	  if (offset < 0)	/* Negative stack adjustment?  */
	    frame_offset -= offset;
	  else
	    /* Exit loop if a positive stack adjustment is found, which
	       usually means that the stack cleanup code in the function
	       epilogue is reached.  */
	    break;
	}
      else if ((inst & 0xf800) == 0xd000)	/* sw reg,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  reg = mips_reg3_to_reg[(inst & 0x700) >> 8];
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0xf900)	/* sd reg,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips_reg3_to_reg[(inst & 0xe0) >> 5];
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0x6200)	/* sw $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	}
      else if ((inst & 0xff00) == 0xfa00)	/* sd $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 0);
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	}
      else if (inst == 0x673d)	/* move $s1, $sp */
	{
	  frame_addr = sp;
	  frame_reg = 17;
	}
      else if ((inst & 0xff00) == 0x0100)	/* addiu $s1,sp,n */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  frame_addr = sp + offset;
	  frame_reg = 17;
	  frame_adjust = offset;
	}
      else if ((inst & 0xFF00) == 0xd900)	/* sw reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 4, 0);
	  reg = mips_reg3_to_reg[(inst & 0xe0) >> 5];
	  set_reg_offset (gdbarch, this_cache, reg, frame_addr + offset);
	}
      else if ((inst & 0xFF00) == 0x7900)	/* sd reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips_reg3_to_reg[(inst & 0xe0) >> 5];
	  set_reg_offset (gdbarch, this_cache, reg, frame_addr + offset);
	}
      else if ((inst & 0xf81f) == 0xe809
	       && (inst & 0x700) != 0x700)	/* entry */
	entry_inst = inst;	/* Save for later processing.  */
      else if ((inst & 0xff80) == 0x6480)	/* save */
	{
	  save_inst = inst;	/* Save for later processing.  */
	  if (prev_extend_bytes)		/* extend */
	    save_inst |= prev_inst << 16;
	}
      else if ((inst & 0xff1c) == 0x6704)	/* move reg,$a0-$a3 */
	{
	  /* This instruction is part of the prologue, but we don't
	     need to do anything special to handle it.  */
	}
      else if (mips16_instruction_has_delay_slot (inst, 0))
						/* JAL/JALR/JALX/JR */
	{
	  /* The instruction in the delay slot can be a part
	     of the prologue, so move forward once more.  */
	  in_delay_slot = 1;
	  if (mips16_instruction_has_delay_slot (inst, 1))
						/* JAL/JALX */
	    {
	      prev_extend_bytes = MIPS_INSN16_SIZE;
	      cur_pc += MIPS_INSN16_SIZE;	/* 32-bit instruction */
	    }
	}
      else
	{
	  this_non_prologue_insn = 1;
	}

      non_prologue_insns += this_non_prologue_insn;

      /* A jump or branch, or enough non-prologue insns seen?  If so,
	 then we must have reached the end of the prologue by now.  */
      if (prev_delay_slot || non_prologue_insns > 1
	  || mips16_instruction_is_compact_branch (inst))
	break;

      prev_non_prologue_insn = this_non_prologue_insn;
      prev_delay_slot = in_delay_slot;
      prev_pc = cur_pc - prev_extend_bytes;
    }

  /* The entry instruction is typically the first instruction in a function,
     and it stores registers at offsets relative to the value of the old SP
     (before the prologue).  But the value of the sp parameter to this
     function is the new SP (after the prologue has been executed).  So we
     can't calculate those offsets until we've seen the entire prologue,
     and can calculate what the old SP must have been.  */
  if (entry_inst != 0)
    {
      int areg_count = (entry_inst >> 8) & 7;
      int sreg_count = (entry_inst >> 6) & 3;

      /* The entry instruction always subtracts 32 from the SP.  */
      frame_offset += 32;

      /* Now we can calculate what the SP must have been at the
	 start of the function prologue.  */
      sp += frame_offset;

      /* Check if a0-a3 were saved in the caller's argument save area.  */
      for (reg = 4, offset = 0; reg < areg_count + 4; reg++)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset += mips_abi_regsize (gdbarch);
	}

      /* Check if the ra register was pushed on the stack.  */
      offset = -4;
      if (entry_inst & 0x20)
	{
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if the s0 and s1 registers were pushed on the stack.  */
      for (reg = 16; reg < sreg_count + 16; reg++)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}
    }

  /* The SAVE instruction is similar to ENTRY, except that defined by the
     MIPS16e ASE of the MIPS Architecture.  Unlike with ENTRY though, the
     size of the frame is specified as an immediate field of instruction
     and an extended variation exists which lets additional registers and
     frame space to be specified.  The instruction always treats registers
     as 32-bit so its usefulness for 64-bit ABIs is questionable.  */
  if (save_inst != 0 && mips_abi_regsize (gdbarch) == 4)
    {
      static int args_table[16] = {
	0, 0, 0, 0, 1, 1, 1, 1,
	2, 2, 2, 0, 3, 3, 4, -1,
      };
      static int astatic_table[16] = {
	0, 1, 2, 3, 0, 1, 2, 3,
	0, 1, 2, 4, 0, 1, 0, -1,
      };
      int aregs = (save_inst >> 16) & 0xf;
      int xsregs = (save_inst >> 24) & 0x7;
      int args = args_table[aregs];
      int astatic = astatic_table[aregs];
      long frame_size;

      if (args < 0)
	{
	  warning (_("Invalid number of argument registers encoded in SAVE."));
	  args = 0;
	}
      if (astatic < 0)
	{
	  warning (_("Invalid number of static registers encoded in SAVE."));
	  astatic = 0;
	}

      /* For standard SAVE the frame size of 0 means 128.  */
      frame_size = ((save_inst >> 16) & 0xf0) | (save_inst & 0xf);
      if (frame_size == 0 && (save_inst >> 16) == 0)
	frame_size = 16;
      frame_size *= 8;
      frame_offset += frame_size;

      /* Now we can calculate what the SP must have been at the
	 start of the function prologue.  */
      sp += frame_offset;

      /* Check if A0-A3 were saved in the caller's argument save area.  */
      for (reg = MIPS_A0_REGNUM, offset = 0; reg < args + 4; reg++)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset += mips_abi_regsize (gdbarch);
	}

      offset = -4;

      /* Check if the RA register was pushed on the stack.  */
      if (save_inst & 0x40)
	{
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if the S8 register was pushed on the stack.  */
      if (xsregs > 6)
	{
	  set_reg_offset (gdbarch, this_cache, 30, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	  xsregs--;
	}
      /* Check if S2-S7 were pushed on the stack.  */
      for (reg = 18 + xsregs - 1; reg > 18 - 1; reg--)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if the S1 register was pushed on the stack.  */
      if (save_inst & 0x10)
	{
	  set_reg_offset (gdbarch, this_cache, 17, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}
      /* Check if the S0 register was pushed on the stack.  */
      if (save_inst & 0x20)
	{
	  set_reg_offset (gdbarch, this_cache, 16, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if A0-A3 were pushed on the stack.  */
      for (reg = MIPS_A0_REGNUM + 3; reg > MIPS_A0_REGNUM + 3 - astatic; reg--)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}
    }

  if (this_cache != NULL)
    {
      this_cache->base =
	(get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch) + frame_reg)
	 + frame_offset - frame_adjust);
      /* FIXME: brobecker/2004-10-10: Just as in the mips32 case, we should
	 be able to get rid of the assignment below, evetually.  But it's
	 still needed for now.  */
      this_cache->saved_regs[gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->pc]
	= this_cache->saved_regs[gdbarch_num_regs (gdbarch) + MIPS_RA_REGNUM];
    }

  /* Set end_prologue_addr to the address of the instruction immediately
     after the last one we scanned.  Unless the last one looked like a
     non-prologue instruction (and we looked ahead), in which case use
     its address instead.  */
  end_prologue_addr = (prev_non_prologue_insn || prev_delay_slot
		       ? prev_pc : cur_pc - prev_extend_bytes);

  return end_prologue_addr;
}

/* Heuristic unwinder for 16-bit MIPS instruction set (aka MIPS16).
   Procedures that use the 32-bit instruction set are handled by the
   mips_insn32 unwinder.  */

static struct mips_frame_cache *
mips_insn16_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct mips_frame_cache *cache;

  if ((*this_cache) != NULL)
    return (struct mips_frame_cache *) (*this_cache);
  cache = FRAME_OBSTACK_ZALLOC (struct mips_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Analyze the function prologue.  */
  {
    const CORE_ADDR pc = get_frame_address_in_block (this_frame);
    CORE_ADDR start_addr;

    find_pc_partial_function (pc, NULL, &start_addr, NULL);
    if (start_addr == 0)
      start_addr = heuristic_proc_start (gdbarch, pc);
    /* We can't analyze the prologue if we couldn't find the begining
       of the function.  */
    if (start_addr == 0)
      return cache;

    mips16_scan_prologue (gdbarch, start_addr, pc, this_frame,
			  (struct mips_frame_cache *) *this_cache);
  }
  
  /* gdbarch_sp_regnum contains the value and not the address.  */
  cache->saved_regs[gdbarch_num_regs (gdbarch)
		    + MIPS_SP_REGNUM].set_value (cache->base);

  return (struct mips_frame_cache *) (*this_cache);
}

static void
mips_insn16_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			   struct frame_id *this_id)
{
  struct mips_frame_cache *info = mips_insn16_frame_cache (this_frame,
							   this_cache);
  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;
  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
mips_insn16_frame_prev_register (frame_info_ptr this_frame,
				 void **this_cache, int regnum)
{
  struct mips_frame_cache *info = mips_insn16_frame_cache (this_frame,
							   this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static int
mips_insn16_frame_sniffer (const struct frame_unwind *self,
			   frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (mips_pc_is_mips16 (gdbarch, pc))
    return 1;
  return 0;
}

static const struct frame_unwind mips_insn16_frame_unwind =
{
  "mips insn16 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_insn16_frame_this_id,
  mips_insn16_frame_prev_register,
  NULL,
  mips_insn16_frame_sniffer
};

static CORE_ADDR
mips_insn16_frame_base_address (frame_info_ptr this_frame,
				void **this_cache)
{
  struct mips_frame_cache *info = mips_insn16_frame_cache (this_frame,
							   this_cache);
  return info->base;
}

static const struct frame_base mips_insn16_frame_base =
{
  &mips_insn16_frame_unwind,
  mips_insn16_frame_base_address,
  mips_insn16_frame_base_address,
  mips_insn16_frame_base_address
};

static const struct frame_base *
mips_insn16_frame_base_sniffer (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (mips_pc_is_mips16 (gdbarch, pc))
    return &mips_insn16_frame_base;
  else
    return NULL;
}

/* Decode a 9-bit signed immediate argument of ADDIUSP -- -2 is mapped
   to -258, -1 -- to -257, 0 -- to 256, 1 -- to 257 and other values are
   interpreted directly, and then multiplied by 4.  */

static int
micromips_decode_imm9 (int imm)
{
  imm = (imm ^ 0x100) - 0x100;
  if (imm > -3 && imm < 2)
    imm ^= 0x100;
  return imm << 2;
}

/* Analyze the function prologue from START_PC to LIMIT_PC.  Return
   the address of the first instruction past the prologue.  */

static CORE_ADDR
micromips_scan_prologue (struct gdbarch *gdbarch,
			 CORE_ADDR start_pc, CORE_ADDR limit_pc,
			 frame_info_ptr this_frame,
			 struct mips_frame_cache *this_cache)
{
  CORE_ADDR end_prologue_addr;
  int prev_non_prologue_insn = 0;
  int frame_reg = MIPS_SP_REGNUM;
  int this_non_prologue_insn;
  int non_prologue_insns = 0;
  long frame_offset = 0;	/* Size of stack frame.  */
  long frame_adjust = 0;	/* Offset of FP from SP.  */
  int prev_delay_slot = 0;
  int in_delay_slot;
  CORE_ADDR prev_pc;
  CORE_ADDR cur_pc;
  ULONGEST insn;		/* current instruction */
  CORE_ADDR sp;
  long offset;
  long sp_adj;
  long v1_off = 0;		/* The assumption is LUI will replace it.  */
  int reglist;
  int breg;
  int dreg;
  int sreg;
  int treg;
  int loc;
  int op;
  int s;
  int i;

  /* Can be called when there's no process, and hence when there's no
     THIS_FRAME.  */
  if (this_frame != NULL)
    sp = get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch)
				    + MIPS_SP_REGNUM);
  else
    sp = 0;

  if (limit_pc > start_pc + 200)
    limit_pc = start_pc + 200;
  prev_pc = start_pc;

  /* Permit at most one non-prologue non-control-transfer instruction
     in the middle which may have been reordered by the compiler for
     optimisation.  */
  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += loc)
    {
      this_non_prologue_insn = 0;
      in_delay_slot = 0;
      sp_adj = 0;
      loc = 0;
      insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, cur_pc, NULL);
      loc += MIPS_INSN16_SIZE;
      switch (mips_insn_size (ISA_MICROMIPS, insn))
	{
	/* 32-bit instructions.  */
	case 2 * MIPS_INSN16_SIZE:
	  insn <<= 16;
	  insn |= mips_fetch_instruction (gdbarch,
					  ISA_MICROMIPS, cur_pc + loc, NULL);
	  loc += MIPS_INSN16_SIZE;
	  switch (micromips_op (insn >> 16))
	    {
	    /* Record $sp/$fp adjustment.  */
	    /* Discard (D)ADDU $gp,$jp used for PIC code.  */
	    case 0x0: /* POOL32A: bits 000000 */
	    case 0x16: /* POOL32S: bits 010110 */
	      op = b0s11_op (insn);
	      sreg = b0s5_reg (insn >> 16);
	      treg = b5s5_reg (insn >> 16);
	      dreg = b11s5_reg (insn);
	      if (op == 0x1d0
				/* SUBU: bits 000000 00111010000 */
				/* DSUBU: bits 010110 00111010000 */
		  && dreg == MIPS_SP_REGNUM && sreg == MIPS_SP_REGNUM
		  && treg == 3)
				/* (D)SUBU $sp, $v1 */
		    sp_adj = v1_off;
	      else if (op != 0x150
				/* ADDU: bits 000000 00101010000 */
				/* DADDU: bits 010110 00101010000 */
		       || dreg != 28 || sreg != 28 || treg != MIPS_T9_REGNUM)
		this_non_prologue_insn = 1;
	      break;

	    case 0x8: /* POOL32B: bits 001000 */
	      op = b12s4_op (insn);
	      breg = b0s5_reg (insn >> 16);
	      reglist = sreg = b5s5_reg (insn >> 16);
	      offset = (b0s12_imm (insn) ^ 0x800) - 0x800;
	      if ((op == 0x9 || op == 0xc)
				/* SWP: bits 001000 1001 */
				/* SDP: bits 001000 1100 */
		  && breg == MIPS_SP_REGNUM && sreg < MIPS_RA_REGNUM)
				/* S[DW]P reg,offset($sp) */
		{
		  s = 4 << ((b12s4_op (insn) & 0x4) == 0x4);
		  set_reg_offset (gdbarch, this_cache,
				  sreg, sp + offset);
		  set_reg_offset (gdbarch, this_cache,
				  sreg + 1, sp + offset + s);
		}
	      else if ((op == 0xd || op == 0xf)
				/* SWM: bits 001000 1101 */
				/* SDM: bits 001000 1111 */
		       && breg == MIPS_SP_REGNUM
				/* SWM reglist,offset($sp) */
		       && ((reglist >= 1 && reglist <= 9)
			   || (reglist >= 16 && reglist <= 25)))
		{
		  int sreglist = std::min(reglist & 0xf, 8);

		  s = 4 << ((b12s4_op (insn) & 0x2) == 0x2);
		  for (i = 0; i < sreglist; i++)
		    set_reg_offset (gdbarch, this_cache, 16 + i, sp + s * i);
		  if ((reglist & 0xf) > 8)
		    set_reg_offset (gdbarch, this_cache, 30, sp + s * i++);
		  if ((reglist & 0x10) == 0x10)
		    set_reg_offset (gdbarch, this_cache,
				    MIPS_RA_REGNUM, sp + s * i++);
		}
	      else
		this_non_prologue_insn = 1;
	      break;

	    /* Record $sp/$fp adjustment.  */
	    /* Discard (D)ADDIU $gp used for PIC code.  */
	    case 0xc: /* ADDIU: bits 001100 */
	    case 0x17: /* DADDIU: bits 010111 */
	      sreg = b0s5_reg (insn >> 16);
	      dreg = b5s5_reg (insn >> 16);
	      offset = (b0s16_imm (insn) ^ 0x8000) - 0x8000;
	      if (sreg == MIPS_SP_REGNUM && dreg == MIPS_SP_REGNUM)
				/* (D)ADDIU $sp, imm */
		sp_adj = offset;
	      else if (sreg == MIPS_SP_REGNUM && dreg == 30)
				/* (D)ADDIU $fp, $sp, imm */
		{
		  frame_adjust = offset;
		  frame_reg = 30;
		}
	      else if (sreg != 28 || dreg != 28)
				/* (D)ADDIU $gp, imm */
		this_non_prologue_insn = 1;
	      break;

	    /* LUI $v1 is used for larger $sp adjustments.  */
	    /* Discard LUI $gp used for PIC code.  */
	    case 0x10: /* POOL32I: bits 010000 */
	      if (b5s5_op (insn >> 16) == 0xd
				/* LUI: bits 010000 001101 */
		  && b0s5_reg (insn >> 16) == 3)
				/* LUI $v1, imm */
		v1_off = ((b0s16_imm (insn) << 16) ^ 0x80000000) - 0x80000000;
	      else if (b5s5_op (insn >> 16) != 0xd
				/* LUI: bits 010000 001101 */
		       || b0s5_reg (insn >> 16) != 28)
				/* LUI $gp, imm */
		this_non_prologue_insn = 1;
	      break;

	    /* ORI $v1 is used for larger $sp adjustments.  */
	    case 0x14: /* ORI: bits 010100 */
	      sreg = b0s5_reg (insn >> 16);
	      dreg = b5s5_reg (insn >> 16);
	      if (sreg == 3 && dreg == 3)
				/* ORI $v1, imm */
		v1_off |= b0s16_imm (insn);
	      else
		this_non_prologue_insn = 1;
	      break;

	    case 0x26: /* SWC1: bits 100110 */
	    case 0x2e: /* SDC1: bits 101110 */
	      breg = b0s5_reg (insn >> 16);
	      if (breg != MIPS_SP_REGNUM)
				/* S[DW]C1 reg,offset($sp) */
		this_non_prologue_insn = 1;
	      break;

	    case 0x36: /* SD: bits 110110 */
	    case 0x3e: /* SW: bits 111110 */
	      breg = b0s5_reg (insn >> 16);
	      sreg = b5s5_reg (insn >> 16);
	      offset = (b0s16_imm (insn) ^ 0x8000) - 0x8000;
	      if (breg == MIPS_SP_REGNUM)
				/* S[DW] reg,offset($sp) */
		set_reg_offset (gdbarch, this_cache, sreg, sp + offset);
	      else
		this_non_prologue_insn = 1;
	      break;

	    default:
	      /* The instruction in the delay slot can be a part
		 of the prologue, so move forward once more.  */
	      if (micromips_instruction_has_delay_slot (insn, 0))
		in_delay_slot = 1;
	      else
		this_non_prologue_insn = 1;
	      break;
	    }
	  insn >>= 16;
	  break;

	/* 16-bit instructions.  */
	case MIPS_INSN16_SIZE:
	  switch (micromips_op (insn))
	    {
	    case 0x3: /* MOVE: bits 000011 */
	      sreg = b0s5_reg (insn);
	      dreg = b5s5_reg (insn);
	      if (sreg == MIPS_SP_REGNUM && dreg == 30)
				/* MOVE  $fp, $sp */
		frame_reg = 30;
	      else if ((sreg & 0x1c) != 0x4)
				/* MOVE  reg, $a0-$a3 */
		this_non_prologue_insn = 1;
	      break;

	    case 0x11: /* POOL16C: bits 010001 */
	      if (b6s4_op (insn) == 0x5)
				/* SWM: bits 010001 0101 */
		{
		  offset = ((b0s4_imm (insn) << 2) ^ 0x20) - 0x20;
		  reglist = b4s2_regl (insn);
		  for (i = 0; i <= reglist; i++)
		    set_reg_offset (gdbarch, this_cache, 16 + i, sp + 4 * i);
		  set_reg_offset (gdbarch, this_cache,
				  MIPS_RA_REGNUM, sp + 4 * i++);
		}
	      else
		this_non_prologue_insn = 1;
	      break;

	    case 0x13: /* POOL16D: bits 010011 */
	      if ((insn & 0x1) == 0x1)
				/* ADDIUSP: bits 010011 1 */
		sp_adj = micromips_decode_imm9 (b1s9_imm (insn));
	      else if (b5s5_reg (insn) == MIPS_SP_REGNUM)
				/* ADDIUS5: bits 010011 0 */
				/* ADDIUS5 $sp, imm */
		sp_adj = (b1s4_imm (insn) ^ 8) - 8;
	      else
		this_non_prologue_insn = 1;
	      break;

	    case 0x32: /* SWSP: bits 110010 */
	      offset = b0s5_imm (insn) << 2;
	      sreg = b5s5_reg (insn);
	      set_reg_offset (gdbarch, this_cache, sreg, sp + offset);
	      break;

	    default:
	      /* The instruction in the delay slot can be a part
		 of the prologue, so move forward once more.  */
	      if (micromips_instruction_has_delay_slot (insn << 16, 0))
		in_delay_slot = 1;
	      else
		this_non_prologue_insn = 1;
	      break;
	    }
	  break;
	}
      if (sp_adj < 0)
	frame_offset -= sp_adj;

      non_prologue_insns += this_non_prologue_insn;

      /* A jump or branch, enough non-prologue insns seen or positive
	 stack adjustment?  If so, then we must have reached the end
	 of the prologue by now.  */
      if (prev_delay_slot || non_prologue_insns > 1 || sp_adj > 0
	  || micromips_instruction_is_compact_branch (insn))
	break;

      prev_non_prologue_insn = this_non_prologue_insn;
      prev_delay_slot = in_delay_slot;
      prev_pc = cur_pc;
    }

  if (this_cache != NULL)
    {
      this_cache->base =
	(get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch) + frame_reg)
	 + frame_offset - frame_adjust);
      /* FIXME: brobecker/2004-10-10: Just as in the mips32 case, we should
	 be able to get rid of the assignment below, evetually. But it's
	 still needed for now.  */
      this_cache->saved_regs[gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->pc]
	= this_cache->saved_regs[gdbarch_num_regs (gdbarch) + MIPS_RA_REGNUM];
    }

  /* Set end_prologue_addr to the address of the instruction immediately
     after the last one we scanned.  Unless the last one looked like a
     non-prologue instruction (and we looked ahead), in which case use
     its address instead.  */
  end_prologue_addr
    = prev_non_prologue_insn || prev_delay_slot ? prev_pc : cur_pc;

  return end_prologue_addr;
}

/* Heuristic unwinder for procedures using microMIPS instructions.
   Procedures that use the 32-bit instruction set are handled by the
   mips_insn32 unwinder.  Likewise MIPS16 and the mips_insn16 unwinder. */

static struct mips_frame_cache *
mips_micro_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct mips_frame_cache *cache;

  if ((*this_cache) != NULL)
    return (struct mips_frame_cache *) (*this_cache);

  cache = FRAME_OBSTACK_ZALLOC (struct mips_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Analyze the function prologue.  */
  {
    const CORE_ADDR pc = get_frame_address_in_block (this_frame);
    CORE_ADDR start_addr;

    find_pc_partial_function (pc, NULL, &start_addr, NULL);
    if (start_addr == 0)
      start_addr = heuristic_proc_start (get_frame_arch (this_frame), pc);
    /* We can't analyze the prologue if we couldn't find the begining
       of the function.  */
    if (start_addr == 0)
      return cache;

    micromips_scan_prologue (gdbarch, start_addr, pc, this_frame,
			     (struct mips_frame_cache *) *this_cache);
  }

  /* gdbarch_sp_regnum contains the value and not the address.  */
  cache->saved_regs[gdbarch_num_regs (gdbarch)
		    + MIPS_SP_REGNUM].set_value (cache->base);

  return (struct mips_frame_cache *) (*this_cache);
}

static void
mips_micro_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			  struct frame_id *this_id)
{
  struct mips_frame_cache *info = mips_micro_frame_cache (this_frame,
							  this_cache);
  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;
  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
mips_micro_frame_prev_register (frame_info_ptr this_frame,
				void **this_cache, int regnum)
{
  struct mips_frame_cache *info = mips_micro_frame_cache (this_frame,
							  this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static int
mips_micro_frame_sniffer (const struct frame_unwind *self,
			  frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);

  if (mips_pc_is_micromips (gdbarch, pc))
    return 1;
  return 0;
}

static const struct frame_unwind mips_micro_frame_unwind =
{
  "mips micro prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_micro_frame_this_id,
  mips_micro_frame_prev_register,
  NULL,
  mips_micro_frame_sniffer
};

static CORE_ADDR
mips_micro_frame_base_address (frame_info_ptr this_frame,
			       void **this_cache)
{
  struct mips_frame_cache *info = mips_micro_frame_cache (this_frame,
							  this_cache);
  return info->base;
}

static const struct frame_base mips_micro_frame_base =
{
  &mips_micro_frame_unwind,
  mips_micro_frame_base_address,
  mips_micro_frame_base_address,
  mips_micro_frame_base_address
};

static const struct frame_base *
mips_micro_frame_base_sniffer (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);

  if (mips_pc_is_micromips (gdbarch, pc))
    return &mips_micro_frame_base;
  else
    return NULL;
}

/* Mark all the registers as unset in the saved_regs array
   of THIS_CACHE.  Do nothing if THIS_CACHE is null.  */

static void
reset_saved_regs (struct gdbarch *gdbarch, struct mips_frame_cache *this_cache)
{
  if (this_cache == NULL || this_cache->saved_regs == NULL)
    return;

  {
    const int num_regs = gdbarch_num_regs (gdbarch);
    int i;

    /* Reset the register values to their default state.  Register i's value
       is in register i.  */
    for (i = 0; i < num_regs; i++)
      this_cache->saved_regs[i].set_realreg (i);
  }
}

/* Analyze the function prologue from START_PC to LIMIT_PC.  Builds
   the associated FRAME_CACHE if not null.  
   Return the address of the first instruction past the prologue.  */

static CORE_ADDR
mips32_scan_prologue (struct gdbarch *gdbarch,
		      CORE_ADDR start_pc, CORE_ADDR limit_pc,
		      frame_info_ptr this_frame,
		      struct mips_frame_cache *this_cache)
{
  int prev_non_prologue_insn;
  int this_non_prologue_insn;
  int non_prologue_insns;
  CORE_ADDR frame_addr = 0; /* Value of $r30. Used by gcc for
			       frame-pointer.  */
  int prev_delay_slot;
  CORE_ADDR prev_pc;
  CORE_ADDR cur_pc;
  CORE_ADDR sp;
  long frame_offset;
  int  frame_reg = MIPS_SP_REGNUM;

  CORE_ADDR end_prologue_addr;
  int seen_sp_adjust = 0;
  int load_immediate_bytes = 0;
  int in_delay_slot;
  int regsize_is_64_bits = (mips_abi_regsize (gdbarch) == 8);

  /* Can be called when there's no process, and hence when there's no
     THIS_FRAME.  */
  if (this_frame != NULL)
    sp = get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch)
				    + MIPS_SP_REGNUM);
  else
    sp = 0;

  if (limit_pc > start_pc + 200)
    limit_pc = start_pc + 200;

restart:
  prev_non_prologue_insn = 0;
  non_prologue_insns = 0;
  prev_delay_slot = 0;
  prev_pc = start_pc;

  /* Permit at most one non-prologue non-control-transfer instruction
     in the middle which may have been reordered by the compiler for
     optimisation.  */
  frame_offset = 0;
  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS_INSN32_SIZE)
    {
      unsigned long inst, high_word;
      long offset;
      int reg;

      this_non_prologue_insn = 0;
      in_delay_slot = 0;

      /* Fetch the instruction.  */
      inst = (unsigned long) mips_fetch_instruction (gdbarch, ISA_MIPS,
						     cur_pc, NULL);

      /* Save some code by pre-extracting some useful fields.  */
      high_word = (inst >> 16) & 0xffff;
      offset = ((inst & 0xffff) ^ 0x8000) - 0x8000;
      reg = high_word & 0x1f;

      if (high_word == 0x27bd		/* addiu $sp,$sp,-i */
	  || high_word == 0x23bd	/* addi $sp,$sp,-i */
	  || high_word == 0x67bd)	/* daddiu $sp,$sp,-i */
	{
	  if (offset < 0)		/* Negative stack adjustment?  */
	    frame_offset -= offset;
	  else
	    /* Exit loop if a positive stack adjustment is found, which
	       usually means that the stack cleanup code in the function
	       epilogue is reached.  */
	    break;
	  seen_sp_adjust = 1;
	}
      else if (((high_word & 0xFFE0) == 0xafa0) /* sw reg,offset($sp) */
	       && !regsize_is_64_bits)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	}
      else if (((high_word & 0xFFE0) == 0xffa0)	/* sd reg,offset($sp) */
	       && regsize_is_64_bits)
	{
	  /* Irix 6.2 N32 ABI uses sd instructions for saving $gp and $ra.  */
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	}
      else if (high_word == 0x27be)	/* addiu $30,$sp,size */
	{
	  /* Old gcc frame, r30 is virtual frame pointer.  */
	  if (offset != frame_offset)
	    frame_addr = sp + offset;
	  else if (this_frame && frame_reg == MIPS_SP_REGNUM)
	    {
	      unsigned alloca_adjust;

	      frame_reg = 30;
	      frame_addr = get_frame_register_signed
		(this_frame, gdbarch_num_regs (gdbarch) + 30);
	      frame_offset = 0;

	      alloca_adjust = (unsigned) (frame_addr - (sp + offset));
	      if (alloca_adjust > 0)
		{
		  /* FP > SP + frame_size.  This may be because of
		     an alloca or somethings similar.  Fix sp to
		     "pre-alloca" value, and try again.  */
		  sp += alloca_adjust;
		  /* Need to reset the status of all registers.  Otherwise,
		     we will hit a guard that prevents the new address
		     for each register to be recomputed during the second
		     pass.  */
		  reset_saved_regs (gdbarch, this_cache);
		  goto restart;
		}
	    }
	}
      /* move $30,$sp.  With different versions of gas this will be either
	 `addu $30,$sp,$zero' or `or $30,$sp,$zero' or `daddu 30,sp,$0'.
	 Accept any one of these.  */
      else if (inst == 0x03A0F021 || inst == 0x03a0f025 || inst == 0x03a0f02d)
	{
	  /* New gcc frame, virtual frame pointer is at r30 + frame_size.  */
	  if (this_frame && frame_reg == MIPS_SP_REGNUM)
	    {
	      unsigned alloca_adjust;

	      frame_reg = 30;
	      frame_addr = get_frame_register_signed
		(this_frame, gdbarch_num_regs (gdbarch) + 30);

	      alloca_adjust = (unsigned) (frame_addr - sp);
	      if (alloca_adjust > 0)
		{
		  /* FP > SP + frame_size.  This may be because of
		     an alloca or somethings similar.  Fix sp to
		     "pre-alloca" value, and try again.  */
		  sp = frame_addr;
		  /* Need to reset the status of all registers.  Otherwise,
		     we will hit a guard that prevents the new address
		     for each register to be recomputed during the second
		     pass.  */
		  reset_saved_regs (gdbarch, this_cache);
		  goto restart;
		}
	    }
	}
      else if ((high_word & 0xFFE0) == 0xafc0 	/* sw reg,offset($30) */
	       && !regsize_is_64_bits)
	{
	  set_reg_offset (gdbarch, this_cache, reg, frame_addr + offset);
	}
      else if ((high_word & 0xFFE0) == 0xE7A0 /* swc1 freg,n($sp) */
	       || (high_word & 0xF3E0) == 0xA3C0 /* sx reg,n($s8) */
	       || (inst & 0xFF9F07FF) == 0x00800021 /* move reg,$a0-$a3 */
	       || high_word == 0x3c1c /* lui $gp,n */
	       || high_word == 0x279c /* addiu $gp,$gp,n */
	       || high_word == 0x679c /* daddiu $gp,$gp,n */
	       || inst == 0x0399e021 /* addu $gp,$gp,$t9 */
	       || inst == 0x033ce021 /* addu $gp,$t9,$gp */
	       || inst == 0x0399e02d /* daddu $gp,$gp,$t9 */
	       || inst == 0x033ce02d /* daddu $gp,$t9,$gp */
	      )
	{
	  /* These instructions are part of the prologue, but we don't
	     need to do anything special to handle them.  */
	}
      /* The instructions below load $at or $t0 with an immediate
	 value in preparation for a stack adjustment via
	 subu $sp,$sp,[$at,$t0].  These instructions could also
	 initialize a local variable, so we accept them only before
	 a stack adjustment instruction was seen.  */
      else if (!seen_sp_adjust
	       && !prev_delay_slot
	       && (high_word == 0x3c01 /* lui $at,n */
		   || high_word == 0x3c08 /* lui $t0,n */
		   || high_word == 0x3421 /* ori $at,$at,n */
		   || high_word == 0x3508 /* ori $t0,$t0,n */
		   || high_word == 0x3401 /* ori $at,$zero,n */
		   || high_word == 0x3408 /* ori $t0,$zero,n */
		  ))
	{
	  load_immediate_bytes += MIPS_INSN32_SIZE;		/* FIXME!  */
	}
      /* Check for branches and jumps.  The instruction in the delay
	 slot can be a part of the prologue, so move forward once more.  */
      else if (mips32_instruction_has_delay_slot (gdbarch, inst))
	{
	  in_delay_slot = 1;
	}
      /* This instruction is not an instruction typically found
	 in a prologue, so we must have reached the end of the
	 prologue.  */
      else
	{
	  this_non_prologue_insn = 1;
	}

      non_prologue_insns += this_non_prologue_insn;

      /* A jump or branch, or enough non-prologue insns seen?  If so,
	 then we must have reached the end of the prologue by now.  */
      if (prev_delay_slot || non_prologue_insns > 1)
	break;

      prev_non_prologue_insn = this_non_prologue_insn;
      prev_delay_slot = in_delay_slot;
      prev_pc = cur_pc;
    }

  if (this_cache != NULL)
    {
      this_cache->base = 
	(get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch) + frame_reg)
	 + frame_offset);
      /* FIXME: brobecker/2004-09-15: We should be able to get rid of
	 this assignment below, eventually.  But it's still needed
	 for now.  */
      this_cache->saved_regs[gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->pc]
	= this_cache->saved_regs[gdbarch_num_regs (gdbarch)
				 + MIPS_RA_REGNUM];
    }

  /* Set end_prologue_addr to the address of the instruction immediately
     after the last one we scanned.  Unless the last one looked like a
     non-prologue instruction (and we looked ahead), in which case use
     its address instead.  */
  end_prologue_addr
    = prev_non_prologue_insn || prev_delay_slot ? prev_pc : cur_pc;
     
  /* In a frameless function, we might have incorrectly
     skipped some load immediate instructions.  Undo the skipping
     if the load immediate was not followed by a stack adjustment.  */
  if (load_immediate_bytes && !seen_sp_adjust)
    end_prologue_addr -= load_immediate_bytes;

  return end_prologue_addr;
}

/* Heuristic unwinder for procedures using 32-bit instructions (covers
   both 32-bit and 64-bit MIPS ISAs).  Procedures using 16-bit
   instructions (a.k.a. MIPS16) are handled by the mips_insn16
   unwinder.  Likewise microMIPS and the mips_micro unwinder. */

static struct mips_frame_cache *
mips_insn32_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct mips_frame_cache *cache;

  if ((*this_cache) != NULL)
    return (struct mips_frame_cache *) (*this_cache);

  cache = FRAME_OBSTACK_ZALLOC (struct mips_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Analyze the function prologue.  */
  {
    const CORE_ADDR pc = get_frame_address_in_block (this_frame);
    CORE_ADDR start_addr;

    find_pc_partial_function (pc, NULL, &start_addr, NULL);
    if (start_addr == 0)
      start_addr = heuristic_proc_start (gdbarch, pc);
    /* We can't analyze the prologue if we couldn't find the begining
       of the function.  */
    if (start_addr == 0)
      return cache;

    mips32_scan_prologue (gdbarch, start_addr, pc, this_frame,
			  (struct mips_frame_cache *) *this_cache);
  }
  
  /* gdbarch_sp_regnum contains the value and not the address.  */
  cache->saved_regs[gdbarch_num_regs (gdbarch)
		    + MIPS_SP_REGNUM].set_value (cache->base);

  return (struct mips_frame_cache *) (*this_cache);
}

static void
mips_insn32_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			   struct frame_id *this_id)
{
  struct mips_frame_cache *info = mips_insn32_frame_cache (this_frame,
							   this_cache);
  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;
  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
mips_insn32_frame_prev_register (frame_info_ptr this_frame,
				 void **this_cache, int regnum)
{
  struct mips_frame_cache *info = mips_insn32_frame_cache (this_frame,
							   this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static int
mips_insn32_frame_sniffer (const struct frame_unwind *self,
			   frame_info_ptr this_frame, void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (mips_pc_is_mips (pc))
    return 1;
  return 0;
}

static const struct frame_unwind mips_insn32_frame_unwind =
{
  "mips insn32 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_insn32_frame_this_id,
  mips_insn32_frame_prev_register,
  NULL,
  mips_insn32_frame_sniffer
};

static CORE_ADDR
mips_insn32_frame_base_address (frame_info_ptr this_frame,
				void **this_cache)
{
  struct mips_frame_cache *info = mips_insn32_frame_cache (this_frame,
							   this_cache);
  return info->base;
}

static const struct frame_base mips_insn32_frame_base =
{
  &mips_insn32_frame_unwind,
  mips_insn32_frame_base_address,
  mips_insn32_frame_base_address,
  mips_insn32_frame_base_address
};

static const struct frame_base *
mips_insn32_frame_base_sniffer (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (mips_pc_is_mips (pc))
    return &mips_insn32_frame_base;
  else
    return NULL;
}

static struct trad_frame_cache *
mips_stub_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  CORE_ADDR pc;
  CORE_ADDR start_addr;
  CORE_ADDR stack_addr;
  struct trad_frame_cache *this_trad_cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int num_regs = gdbarch_num_regs (gdbarch);

  if ((*this_cache) != NULL)
    return (struct trad_frame_cache *) (*this_cache);
  this_trad_cache = trad_frame_cache_zalloc (this_frame);
  (*this_cache) = this_trad_cache;

  /* The return address is in the link register.  */
  trad_frame_set_reg_realreg (this_trad_cache,
			      gdbarch_pc_regnum (gdbarch),
			      num_regs + MIPS_RA_REGNUM);

  /* Frame ID, since it's a frameless / stackless function, no stack
     space is allocated and SP on entry is the current SP.  */
  pc = get_frame_pc (this_frame);
  find_pc_partial_function (pc, NULL, &start_addr, NULL);
  stack_addr = get_frame_register_signed (this_frame,
					  num_regs + MIPS_SP_REGNUM);
  trad_frame_set_id (this_trad_cache, frame_id_build (stack_addr, start_addr));

  /* Assume that the frame's base is the same as the
     stack-pointer.  */
  trad_frame_set_this_base (this_trad_cache, stack_addr);

  return this_trad_cache;
}

static void
mips_stub_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			 struct frame_id *this_id)
{
  struct trad_frame_cache *this_trad_cache
    = mips_stub_frame_cache (this_frame, this_cache);
  trad_frame_get_id (this_trad_cache, this_id);
}

static struct value *
mips_stub_frame_prev_register (frame_info_ptr this_frame,
			       void **this_cache, int regnum)
{
  struct trad_frame_cache *this_trad_cache
    = mips_stub_frame_cache (this_frame, this_cache);
  return trad_frame_get_register (this_trad_cache, this_frame, regnum);
}

static int
mips_stub_frame_sniffer (const struct frame_unwind *self,
			 frame_info_ptr this_frame, void **this_cache)
{
  gdb_byte dummy[4];
  CORE_ADDR pc = get_frame_address_in_block (this_frame);
  struct bound_minimal_symbol msym;

  /* Use the stub unwinder for unreadable code.  */
  if (target_read_memory (get_frame_pc (this_frame), dummy, 4) != 0)
    return 1;

  if (in_plt_section (pc) || in_mips_stubs_section (pc))
    return 1;

  /* Calling a PIC function from a non-PIC function passes through a
     stub.  The stub for foo is named ".pic.foo".  */
  msym = lookup_minimal_symbol_by_pc (pc);
  if (msym.minsym != NULL
      && msym.minsym->linkage_name () != NULL
      && startswith (msym.minsym->linkage_name (), ".pic."))
    return 1;

  return 0;
}

static const struct frame_unwind mips_stub_frame_unwind =
{
  "mips stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_stub_frame_this_id,
  mips_stub_frame_prev_register,
  NULL,
  mips_stub_frame_sniffer
};

static CORE_ADDR
mips_stub_frame_base_address (frame_info_ptr this_frame,
			      void **this_cache)
{
  struct trad_frame_cache *this_trad_cache
    = mips_stub_frame_cache (this_frame, this_cache);
  return trad_frame_get_this_base (this_trad_cache);
}

static const struct frame_base mips_stub_frame_base =
{
  &mips_stub_frame_unwind,
  mips_stub_frame_base_address,
  mips_stub_frame_base_address,
  mips_stub_frame_base_address
};

static const struct frame_base *
mips_stub_frame_base_sniffer (frame_info_ptr this_frame)
{
  if (mips_stub_frame_sniffer (&mips_stub_frame_unwind, this_frame, NULL))
    return &mips_stub_frame_base;
  else
    return NULL;
}

/* mips_addr_bits_remove - remove useless address bits  */

static CORE_ADDR
mips_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

  if (mips_mask_address_p (tdep) && (((ULONGEST) addr) >> 32 == 0xffffffffUL))
    /* This hack is a work-around for existing boards using PMON, the
       simulator, and any other 64-bit targets that doesn't have true
       64-bit addressing.  On these targets, the upper 32 bits of
       addresses are ignored by the hardware.  Thus, the PC or SP are
       likely to have been sign extended to all 1s by instruction
       sequences that load 32-bit addresses.  For example, a typical
       piece of code that loads an address is this:

       lui $r2, <upper 16 bits>
       ori $r2, <lower 16 bits>

       But the lui sign-extends the value such that the upper 32 bits
       may be all 1s.  The workaround is simply to mask off these
       bits.  In the future, gcc may be changed to support true 64-bit
       addressing, and this masking will have to be disabled.  */
    return addr &= 0xffffffffUL;
  else
    return addr;
}


/* Checks for an atomic sequence of instructions beginning with a LL/LLD
   instruction and ending with a SC/SCD instruction.  If such a sequence
   is found, attempt to step through it.  A breakpoint is placed at the end of 
   the sequence.  */

/* Instructions used during single-stepping of atomic sequences, standard
   ISA version.  */
#define LL_OPCODE 0x30
#define LLD_OPCODE 0x34
#define SC_OPCODE 0x38
#define SCD_OPCODE 0x3c

static std::vector<CORE_ADDR>
mips_deal_with_atomic_sequence (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR breaks[2] = {CORE_ADDR_MAX, CORE_ADDR_MAX};
  CORE_ADDR loc = pc;
  CORE_ADDR branch_bp; /* Breakpoint at branch instruction's destination.  */
  ULONGEST insn;
  int insn_count;
  int index;
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */  
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */

  insn = mips_fetch_instruction (gdbarch, ISA_MIPS, loc, NULL);
  /* Assume all atomic sequences start with a ll/lld instruction.  */
  if (itype_op (insn) != LL_OPCODE && itype_op (insn) != LLD_OPCODE)
    return {};

  /* Assume that no atomic sequence is longer than "atomic_sequence_length" 
     instructions.  */
  for (insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      int is_branch = 0;
      loc += MIPS_INSN32_SIZE;
      insn = mips_fetch_instruction (gdbarch, ISA_MIPS, loc, NULL);

      /* Assume that there is at most one branch in the atomic
	 sequence.  If a branch is found, put a breakpoint in its
	 destination address.  */
      switch (itype_op (insn))
	{
	case 0: /* SPECIAL */
	  if (rtype_funct (insn) >> 1 == 4) /* JR, JALR */
	    return {}; /* fallback to the standard single-step code.  */
	  break;
	case 1: /* REGIMM */
	  is_branch = ((itype_rt (insn) & 0xc) == 0 /* B{LT,GE}Z* */
		       || ((itype_rt (insn) & 0x1e) == 0
			   && itype_rs (insn) == 0)); /* BPOSGE* */
	  break;
	case 2: /* J */
	case 3: /* JAL */
	  return {}; /* fallback to the standard single-step code.  */
	case 4: /* BEQ */
	case 5: /* BNE */
	case 6: /* BLEZ */
	case 7: /* BGTZ */
	case 20: /* BEQL */
	case 21: /* BNEL */
	case 22: /* BLEZL */
	case 23: /* BGTTL */
	  is_branch = 1;
	  break;
	case 17: /* COP1 */
	  is_branch = ((itype_rs (insn) == 9 || itype_rs (insn) == 10)
		       && (itype_rt (insn) & 0x2) == 0);
	  if (is_branch) /* BC1ANY2F, BC1ANY2T, BC1ANY4F, BC1ANY4T */
	    break;
	  [[fallthrough]];
	case 18: /* COP2 */
	case 19: /* COP3 */
	  is_branch = (itype_rs (insn) == 8); /* BCzF, BCzFL, BCzT, BCzTL */
	  break;
	}
      if (is_branch)
	{
	  branch_bp = loc + mips32_relative_offset (insn) + 4;
	  if (last_breakpoint >= 1)
	    return {}; /* More than one branch found, fallback to the
			  standard single-step code.  */
	  breaks[1] = branch_bp;
	  last_breakpoint++;
	}

      if (itype_op (insn) == SC_OPCODE || itype_op (insn) == SCD_OPCODE)
	break;
    }

  /* Assume that the atomic sequence ends with a sc/scd instruction.  */
  if (itype_op (insn) != SC_OPCODE && itype_op (insn) != SCD_OPCODE)
    return {};

  loc += MIPS_INSN32_SIZE;

  /* Insert a breakpoint right after the end of the atomic sequence.  */
  breaks[0] = loc;

  /* Check for duplicated breakpoints.  Check also for a breakpoint
     placed (branch instruction's destination) in the atomic sequence.  */
  if (last_breakpoint && pc <= breaks[1] && breaks[1] <= breaks[0])
    last_breakpoint = 0;

  std::vector<CORE_ADDR> next_pcs;

  /* Effectively inserts the breakpoints.  */
  for (index = 0; index <= last_breakpoint; index++)
    next_pcs.push_back (breaks[index]);

  return next_pcs;
}

static std::vector<CORE_ADDR>
micromips_deal_with_atomic_sequence (struct gdbarch *gdbarch,
				     CORE_ADDR pc)
{
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */
  CORE_ADDR breaks[2] = {CORE_ADDR_MAX, CORE_ADDR_MAX};
  CORE_ADDR branch_bp = 0; /* Breakpoint at branch instruction's
			      destination.  */
  CORE_ADDR loc = pc;
  int sc_found = 0;
  ULONGEST insn;
  int insn_count;
  int index;

  /* Assume all atomic sequences start with a ll/lld instruction.  */
  insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, loc, NULL);
  if (micromips_op (insn) != 0x18)	/* POOL32C: bits 011000 */
    return {};
  loc += MIPS_INSN16_SIZE;
  insn <<= 16;
  insn |= mips_fetch_instruction (gdbarch, ISA_MICROMIPS, loc, NULL);
  if ((b12s4_op (insn) & 0xb) != 0x3)	/* LL, LLD: bits 011000 0x11 */
    return {};
  loc += MIPS_INSN16_SIZE;

  /* Assume all atomic sequences end with an sc/scd instruction.  Assume
     that no atomic sequence is longer than "atomic_sequence_length"
     instructions.  */
  for (insn_count = 0;
       !sc_found && insn_count < atomic_sequence_length;
       ++insn_count)
    {
      int is_branch = 0;

      insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, loc, NULL);
      loc += MIPS_INSN16_SIZE;

      /* Assume that there is at most one conditional branch in the
	 atomic sequence.  If a branch is found, put a breakpoint in
	 its destination address.  */
      switch (mips_insn_size (ISA_MICROMIPS, insn))
	{
	/* 32-bit instructions.  */
	case 2 * MIPS_INSN16_SIZE:
	  switch (micromips_op (insn))
	    {
	    case 0x10: /* POOL32I: bits 010000 */
	      if ((b5s5_op (insn) & 0x18) != 0x0
				/* BLTZ, BLTZAL, BGEZ, BGEZAL: 010000 000xx */
				/* BLEZ, BNEZC, BGTZ, BEQZC: 010000 001xx */
		  && (b5s5_op (insn) & 0x1d) != 0x11
				/* BLTZALS, BGEZALS: bits 010000 100x1 */
		  && ((b5s5_op (insn) & 0x1e) != 0x14
		      || (insn & 0x3) != 0x0)
				/* BC2F, BC2T: bits 010000 1010x xxx00 */
		  && (b5s5_op (insn) & 0x1e) != 0x1a
				/* BPOSGE64, BPOSGE32: bits 010000 1101x */
		  && ((b5s5_op (insn) & 0x1e) != 0x1c
		      || (insn & 0x3) != 0x0)
				/* BC1F, BC1T: bits 010000 1110x xxx00 */
		  && ((b5s5_op (insn) & 0x1c) != 0x1c
		      || (insn & 0x3) != 0x1))
				/* BC1ANY*: bits 010000 111xx xxx01 */
		break;
	      [[fallthrough]];

	    case 0x25: /* BEQ: bits 100101 */
	    case 0x2d: /* BNE: bits 101101 */
	      insn <<= 16;
	      insn |= mips_fetch_instruction (gdbarch,
					      ISA_MICROMIPS, loc, NULL);
	      branch_bp = (loc + MIPS_INSN16_SIZE
			   + micromips_relative_offset16 (insn));
	      is_branch = 1;
	      break;

	    case 0x00: /* POOL32A: bits 000000 */
	      insn <<= 16;
	      insn |= mips_fetch_instruction (gdbarch,
					      ISA_MICROMIPS, loc, NULL);
	      if (b0s6_op (insn) != 0x3c
				/* POOL32Axf: bits 000000 ... 111100 */
		  || (b6s10_ext (insn) & 0x2bf) != 0x3c)
				/* JALR, JALR.HB: 000000 000x111100 111100 */
				/* JALRS, JALRS.HB: 000000 010x111100 111100 */
		break;
	      [[fallthrough]];

	    case 0x1d: /* JALS: bits 011101 */
	    case 0x35: /* J: bits 110101 */
	    case 0x3d: /* JAL: bits 111101 */
	    case 0x3c: /* JALX: bits 111100 */
	      return {}; /* Fall back to the standard single-step code. */

	    case 0x18: /* POOL32C: bits 011000 */
	      if ((b12s4_op (insn) & 0xb) == 0xb)
				/* SC, SCD: bits 011000 1x11 */
		sc_found = 1;
	      break;
	    }
	  loc += MIPS_INSN16_SIZE;
	  break;

	/* 16-bit instructions.  */
	case MIPS_INSN16_SIZE:
	  switch (micromips_op (insn))
	    {
	    case 0x23: /* BEQZ16: bits 100011 */
	    case 0x2b: /* BNEZ16: bits 101011 */
	      branch_bp = loc + micromips_relative_offset7 (insn);
	      is_branch = 1;
	      break;

	    case 0x11: /* POOL16C: bits 010001 */
	      if ((b5s5_op (insn) & 0x1c) != 0xc
				/* JR16, JRC, JALR16, JALRS16: 010001 011xx */
		  && b5s5_op (insn) != 0x18)
				/* JRADDIUSP: bits 010001 11000 */
		break;
	      return {}; /* Fall back to the standard single-step code. */

	    case 0x33: /* B16: bits 110011 */
	      return {}; /* Fall back to the standard single-step code. */
	    }
	  break;
	}
      if (is_branch)
	{
	  if (last_breakpoint >= 1)
	    return {}; /* More than one branch found, fallback to the
			  standard single-step code.  */
	  breaks[1] = branch_bp;
	  last_breakpoint++;
	}
    }
  if (!sc_found)
    return {};

  /* Insert a breakpoint right after the end of the atomic sequence.  */
  breaks[0] = loc;

  /* Check for duplicated breakpoints.  Check also for a breakpoint
     placed (branch instruction's destination) in the atomic sequence */
  if (last_breakpoint && pc <= breaks[1] && breaks[1] <= breaks[0])
    last_breakpoint = 0;

  std::vector<CORE_ADDR> next_pcs;

  /* Effectively inserts the breakpoints.  */
  for (index = 0; index <= last_breakpoint; index++)
    next_pcs.push_back (breaks[index]);

  return next_pcs;
}

static std::vector<CORE_ADDR>
deal_with_atomic_sequence (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (mips_pc_is_mips (pc))
    return mips_deal_with_atomic_sequence (gdbarch, pc);
  else if (mips_pc_is_micromips (gdbarch, pc))
    return micromips_deal_with_atomic_sequence (gdbarch, pc);
  else
    return {};
}

/* mips_software_single_step() is called just before we want to resume
   the inferior, if we want to single-step it but there is no hardware
   or kernel single-step support (MIPS on GNU/Linux for example).  We find
   the target of the coming instruction and breakpoint it.  */

std::vector<CORE_ADDR>
mips_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR pc, next_pc;

  pc = regcache_read_pc (regcache);
  std::vector<CORE_ADDR> next_pcs = deal_with_atomic_sequence (gdbarch, pc);

  if (!next_pcs.empty ())
    return next_pcs;

  next_pc = mips_next_pc (regcache, pc);

  return {next_pc};
}

/* Test whether the PC points to the return instruction at the
   end of a function.  */

static int
mips_about_to_return (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  ULONGEST insn;
  ULONGEST hint;

  /* This used to check for MIPS16, but this piece of code is never
     called for MIPS16 functions.  And likewise microMIPS ones.  */
  gdb_assert (mips_pc_is_mips (pc));

  insn = mips_fetch_instruction (gdbarch, ISA_MIPS, pc, NULL);
  hint = 0x7c0;
  return (insn & ~hint) == 0x3e00008;			/* jr(.hb) $ra */
}


/* This fencepost looks highly suspicious to me.  Removing it also
   seems suspicious as it could affect remote debugging across serial
   lines.  */

static CORE_ADDR
heuristic_proc_start (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR start_pc;
  CORE_ADDR fence;
  int instlen;
  int seen_adjsp = 0;
  struct inferior *inf;

  pc = gdbarch_addr_bits_remove (gdbarch, pc);
  start_pc = pc;
  fence = start_pc - heuristic_fence_post;
  if (start_pc == 0)
    return 0;

  if (heuristic_fence_post == -1 || fence < VM_MIN_ADDRESS)
    fence = VM_MIN_ADDRESS;

  instlen = mips_pc_is_mips (pc) ? MIPS_INSN32_SIZE : MIPS_INSN16_SIZE;

  inf = current_inferior ();

  /* Search back for previous return.  */
  for (start_pc -= instlen;; start_pc -= instlen)
    if (start_pc < fence)
      {
	/* It's not clear to me why we reach this point when
	   stop_soon, but with this test, at least we
	   don't print out warnings for every child forked (eg, on
	   decstation).  22apr93 rich@cygnus.com.  */
	if (inf->control.stop_soon == NO_STOP_QUIETLY)
	  {
	    static int blurb_printed = 0;

	    warning (_("GDB can't find the start of the function at %s."),
		     paddress (gdbarch, pc));

	    if (!blurb_printed)
	      {
		/* This actually happens frequently in embedded
		   development, when you first connect to a board
		   and your stack pointer and pc are nowhere in
		   particular.  This message needs to give people
		   in that situation enough information to
		   determine that it's no big deal.  */
		gdb_printf ("\n\
    GDB is unable to find the start of the function at %s\n\
and thus can't determine the size of that function's stack frame.\n\
This means that GDB may be unable to access that stack frame, or\n\
the frames below it.\n\
    This problem is most likely caused by an invalid program counter or\n\
stack pointer.\n\
    However, if you think GDB should simply search farther back\n\
from %s for code which looks like the beginning of a\n\
function, you can increase the range of the search using the `set\n\
heuristic-fence-post' command.\n",
			    paddress (gdbarch, pc), paddress (gdbarch, pc));
		blurb_printed = 1;
	      }
	  }

	return 0;
      }
    else if (mips_pc_is_mips16 (gdbarch, start_pc))
      {
	unsigned short inst;

	/* On MIPS16, any one of the following is likely to be the
	   start of a function:
	   extend save
	   save
	   entry
	   addiu sp,-n
	   daddiu sp,-n
	   extend -n followed by 'addiu sp,+n' or 'daddiu sp,+n'.  */
	inst = mips_fetch_instruction (gdbarch, ISA_MIPS16, start_pc, NULL);
	if ((inst & 0xff80) == 0x6480)		/* save */
	  {
	    if (start_pc - instlen >= fence)
	      {
		inst = mips_fetch_instruction (gdbarch, ISA_MIPS16,
					       start_pc - instlen, NULL);
		if ((inst & 0xf800) == 0xf000)	/* extend */
		  start_pc -= instlen;
	      }
	    break;
	  }
	else if (((inst & 0xf81f) == 0xe809
		  && (inst & 0x700) != 0x700)	/* entry */
		 || (inst & 0xff80) == 0x6380	/* addiu sp,-n */
		 || (inst & 0xff80) == 0xfb80	/* daddiu sp,-n */
		 || ((inst & 0xf810) == 0xf010 && seen_adjsp))	/* extend -n */
	  break;
	else if ((inst & 0xff00) == 0x6300	/* addiu sp */
		 || (inst & 0xff00) == 0xfb00)	/* daddiu sp */
	  seen_adjsp = 1;
	else
	  seen_adjsp = 0;
      }
    else if (mips_pc_is_micromips (gdbarch, start_pc))
      {
	ULONGEST insn;
	int stop = 0;
	long offset;
	int dreg;
	int sreg;

	/* On microMIPS, any one of the following is likely to be the
	   start of a function:
	   ADDIUSP -imm
	   (D)ADDIU $sp, -imm
	   LUI $gp, imm  */
	insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, pc, NULL);
	switch (micromips_op (insn))
	  {
	  case 0xc: /* ADDIU: bits 001100 */
	  case 0x17: /* DADDIU: bits 010111 */
	    sreg = b0s5_reg (insn);
	    dreg = b5s5_reg (insn);
	    insn <<= 16;
	    insn |= mips_fetch_instruction (gdbarch, ISA_MICROMIPS,
					    pc + MIPS_INSN16_SIZE, NULL);
	    offset = (b0s16_imm (insn) ^ 0x8000) - 0x8000;
	    if (sreg == MIPS_SP_REGNUM && dreg == MIPS_SP_REGNUM
				/* (D)ADDIU $sp, imm */
		&& offset < 0)
	      stop = 1;
	    break;

	  case 0x10: /* POOL32I: bits 010000 */
	    if (b5s5_op (insn) == 0xd
				/* LUI: bits 010000 001101 */
		&& b0s5_reg (insn >> 16) == 28)
				/* LUI $gp, imm */
	      stop = 1;
	    break;

	  case 0x13: /* POOL16D: bits 010011 */
	    if ((insn & 0x1) == 0x1)
				/* ADDIUSP: bits 010011 1 */
	      {
		offset = micromips_decode_imm9 (b1s9_imm (insn));
		if (offset < 0)
				/* ADDIUSP -imm */
		  stop = 1;
	      }
	    else
				/* ADDIUS5: bits 010011 0 */
	      {
		dreg = b5s5_reg (insn);
		offset = (b1s4_imm (insn) ^ 8) - 8;
		if (dreg == MIPS_SP_REGNUM && offset < 0)
				/* ADDIUS5  $sp, -imm */
		  stop = 1;
	      }
	    break;
	  }
	if (stop)
	  break;
      }
    else if (mips_about_to_return (gdbarch, start_pc))
      {
	/* Skip return and its delay slot.  */
	start_pc += 2 * MIPS_INSN32_SIZE;
	break;
      }

  return start_pc;
}

struct mips_objfile_private
{
  bfd_size_type size;
  char *contents;
};

/* According to the current ABI, should the type be passed in a
   floating-point register (assuming that there is space)?  When there
   is no FPU, FP are not even considered as possible candidates for
   FP registers and, consequently this returns false - forces FP
   arguments into integer registers.  */

static int
fp_register_arg_p (struct gdbarch *gdbarch, enum type_code typecode,
		   struct type *arg_type)
{
  return ((typecode == TYPE_CODE_FLT
	   || (mips_eabi (gdbarch)
	       && (typecode == TYPE_CODE_STRUCT
		   || typecode == TYPE_CODE_UNION)
	       && arg_type->num_fields () == 1
	       && check_typedef (arg_type->field (0).type ())->code ()
	       == TYPE_CODE_FLT))
	  && mips_get_fpu_type (gdbarch) != MIPS_FPU_NONE);
}

/* On o32, argument passing in GPRs depends on the alignment of the type being
   passed.  Return 1 if this type must be aligned to a doubleword boundary.  */

static int
mips_type_needs_double_align (struct type *type)
{
  enum type_code typecode = type->code ();

  if (typecode == TYPE_CODE_FLT && type->length () == 8)
    return 1;
  else if (typecode == TYPE_CODE_STRUCT)
    {
      if (type->num_fields () < 1)
	return 0;
      return mips_type_needs_double_align (type->field (0).type ());
    }
  else if (typecode == TYPE_CODE_UNION)
    {
      int i, n;

      n = type->num_fields ();
      for (i = 0; i < n; i++)
	if (mips_type_needs_double_align (type->field (i).type ()))
	  return 1;
      return 0;
    }
  return 0;
}

/* Adjust the address downward (direction of stack growth) so that it
   is correctly aligned for a new stack frame.  */
static CORE_ADDR
mips_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 16);
}

/* Implement the "push_dummy_code" gdbarch method.  */

static CORE_ADDR
mips_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp,
		      CORE_ADDR funaddr, struct value **args,
		      int nargs, struct type *value_type,
		      CORE_ADDR *real_pc, CORE_ADDR *bp_addr,
		      struct regcache *regcache)
{
  static gdb_byte nop_insn[] = { 0, 0, 0, 0 };
  CORE_ADDR nop_addr;
  CORE_ADDR bp_slot;

  /* Reserve enough room on the stack for our breakpoint instruction.  */
  bp_slot = sp - sizeof (nop_insn);

  /* Return to microMIPS mode if calling microMIPS code to avoid
     triggering an address error exception on processors that only
     support microMIPS execution.  */
  *bp_addr = (mips_pc_is_micromips (gdbarch, funaddr)
	      ? make_compact_addr (bp_slot) : bp_slot);

  /* The breakpoint layer automatically adjusts the address of
     breakpoints inserted in a branch delay slot.  With enough
     bad luck, the 4 bytes located just before our breakpoint
     instruction could look like a branch instruction, and thus
     trigger the adjustement, and break the function call entirely.
     So, we reserve those 4 bytes and write a nop instruction
     to prevent that from happening.  */
  nop_addr = bp_slot - sizeof (nop_insn);
  write_memory (nop_addr, nop_insn, sizeof (nop_insn));
  sp = mips_frame_align (gdbarch, nop_addr);

  /* Inferior resumes at the function entry point.  */
  *real_pc = funaddr;

  return sp;
}

static CORE_ADDR
mips_eabi_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			   struct regcache *regcache, CORE_ADDR bp_addr,
			   int nargs, struct value **args, CORE_ADDR sp,
			   function_call_return_method return_method,
			   CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int arg_space = 0;
  int stack_offset = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);
  int abi_regsize = mips_abi_regsize (gdbarch);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  We allocate more
     than necessary for EABI, because the first few arguments are
     passed in registers, but that's OK.  */
  for (argnum = 0; argnum < nargs; argnum++)
    arg_space += align_up (args[argnum]->type ()->length (),
			   abi_regsize);
  sp -= align_up (arg_space, 16);

  if (mips_debug)
    gdb_printf (gdb_stdlog,
		"mips_eabi_push_dummy_call: sp=%s allocated %ld\n",
		paddress (gdbarch, sp),
		(long) align_up (arg_space, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (return_method == return_method_struct)
    {
      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_eabi_push_dummy_call: "
		    "struct_return reg=%d %s\n",
		    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      /* This holds the address of structures that are passed by
	 reference.  */
      gdb_byte ref_valbuf[MAX_MIPS_ABI_REGSIZE];
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (arg->type ());
      int len = arg_type->length ();
      enum type_code typecode = arg_type->code ();

      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_eabi_push_dummy_call: %d len=%d type=%d",
		    argnum + 1, len, (int) typecode);

      /* The EABI passes structures that do not fit in a register by
	 reference.  */
      if (len > abi_regsize
	  && (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION))
	{
	  gdb_assert (abi_regsize <= ARRAY_SIZE (ref_valbuf));
	  store_unsigned_integer (ref_valbuf, abi_regsize, byte_order,
				  arg->address ());
	  typecode = TYPE_CODE_PTR;
	  len = abi_regsize;
	  val = ref_valbuf;
	  if (mips_debug)
	    gdb_printf (gdb_stdlog, " push");
	}
      else
	val = arg->contents ().data ();

      /* 32-bit ABIs always start floating point arguments in an
	 even-numbered floating point register.  Round the FP register
	 up before the check to see if there are any FP registers
	 left.  Non MIPS_EABI targets also pass the FP in the integer
	 registers so also round up normal registers.  */
      if (abi_regsize < 8 && fp_register_arg_p (gdbarch, typecode, arg_type))
	{
	  if ((float_argreg & 1))
	    float_argreg++;
	}

      /* Floating point arguments passed in registers have to be
	 treated specially.  On 32-bit architectures, doubles
	 are passed in register pairs; the even register gets
	 the low word, and the odd register gets the high word.
	 On non-EABI processors, the first two floating point arguments are
	 also copied to general registers, because MIPS16 functions
	 don't use float registers for arguments.  This duplication of
	 arguments in general registers can't hurt non-MIPS16 functions
	 because those registers are normally skipped.  */
      /* MIPS_EABI squeezes a struct that contains a single floating
	 point value into an FP register instead of pushing it onto the
	 stack.  */
      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && float_argreg <= mips_last_fp_arg_regnum (gdbarch))
	{
	  /* EABI32 will pass doubles in consecutive registers, even on
	     64-bit cores.  At one time, we used to check the size of
	     `float_argreg' to determine whether or not to pass doubles
	     in consecutive registers, but this is not sufficient for
	     making the ABI determination.  */
	  if (len == 8 && mips_abi (gdbarch) == MIPS_ABI_EABI32)
	    {
	      int low_offset = gdbarch_byte_order (gdbarch)
			       == BFD_ENDIAN_BIG ? 4 : 0;
	      long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_signed_integer (val + low_offset,
					       4, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg, phex (regval, 4));
	      regcache_cooked_write_signed (regcache, float_argreg++, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_signed_integer (val + 4 - low_offset,
					       4, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg, phex (regval, 4));
	      regcache_cooked_write_signed (regcache, float_argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
		 in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
		 above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_signed_integer (val, len, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg, phex (regval, len));
	      regcache_cooked_write_signed (regcache, float_argreg++, regval);
	    }
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of abi_regsize
	     are treated specially: Irix cc passes
	     them in registers where gcc sometimes puts them on the
	     stack.  For maximum compatibility, we will put them in
	     both places.  */
	  int odd_sized_struct = (len > abi_regsize && len % abi_regsize != 0);

	  /* Note: Floating-point values that didn't fit into an FP
	     register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = (len < abi_regsize ? len : abi_regsize);

	      if (mips_debug)
		gdb_printf (gdb_stdlog, " -- partial=%d",
			    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > mips_last_arg_regnum (gdbarch)
		  || odd_sized_struct
		  || fp_register_arg_p (gdbarch, typecode, arg_type))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		    {
		      if (abi_regsize == 8
			  && (typecode == TYPE_CODE_INT
			      || typecode == TYPE_CODE_PTR
			      || typecode == TYPE_CODE_FLT) && len <= 4)
			longword_offset = abi_regsize - len;
		      else if ((typecode == TYPE_CODE_STRUCT
				|| typecode == TYPE_CODE_UNION)
			       && arg_type->length () < abi_regsize)
			longword_offset = abi_regsize - len;
		    }

		  if (mips_debug)
		    {
		      gdb_printf (gdb_stdlog, " - stack_offset=%s",
				  paddress (gdbarch, stack_offset));
		      gdb_printf (gdb_stdlog, " longword_offset=%s",
				  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      gdb_printf (gdb_stdlog, " @%s ",
				  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  gdb_printf (gdb_stdlog, "%02x",
				      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
		 structs may go thru BOTH paths.  Floating point
		 arguments will not.  */
	      /* Write this portion of the argument to a general
		 purpose register.  */
	      if (argreg <= mips_last_arg_regnum (gdbarch)
		  && !fp_register_arg_p (gdbarch, typecode, arg_type))
		{
		  LONGEST regval =
		    extract_signed_integer (val, partial_len, byte_order);

		  if (mips_debug)
		    gdb_printf (gdb_stdlog, " - reg=%d val=%s",
				argreg,
				phex (regval, abi_regsize));
		  regcache_cooked_write_signed (regcache, argreg, regval);
		  argreg++;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
		 copy the next parameter.

		 In the new EABI (and the NABI32), the stack_offset
		 only needs to be adjusted when it has been used.  */

	      if (stack_used_p)
		stack_offset += align_up (partial_len, abi_regsize);
	    }
	}
      if (mips_debug)
	gdb_printf (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

/* Determine the return value convention being used.  */

static enum return_value_convention
mips_eabi_return_value (struct gdbarch *gdbarch, struct value *function,
			struct type *type, struct regcache *regcache,
			gdb_byte *readbuf, const gdb_byte *writebuf)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  int fp_return_type = 0;
  int offset, regnum, xfer;

  if (type->length () > 2 * mips_abi_regsize (gdbarch))
    return RETURN_VALUE_STRUCT_CONVENTION;

  /* Floating point type?  */
  if (tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      if (type->code () == TYPE_CODE_FLT)
	fp_return_type = 1;
      /* Structs with a single field of float type 
	 are returned in a floating point register.  */
      if ((type->code () == TYPE_CODE_STRUCT
	   || type->code () == TYPE_CODE_UNION)
	  && type->num_fields () == 1)
	{
	  struct type *fieldtype = type->field (0).type ();

	  if (check_typedef (fieldtype)->code () == TYPE_CODE_FLT)
	    fp_return_type = 1;
	}
    }

  if (fp_return_type)      
    {
      /* A floating-point value belongs in the least significant part
	 of FP0/FP1.  */
      if (mips_debug)
	gdb_printf (gdb_stderr, "Return float in $fp0\n");
      regnum = mips_regnum (gdbarch)->fp0;
    }
  else 
    {
      /* An integer value goes in V0/V1.  */
      if (mips_debug)
	gdb_printf (gdb_stderr, "Return scalar in $v0\n");
      regnum = MIPS_V0_REGNUM;
    }
  for (offset = 0;
       offset < type->length ();
       offset += mips_abi_regsize (gdbarch), regnum++)
    {
      xfer = mips_abi_regsize (gdbarch);
      if (offset + xfer > type->length ())
	xfer = type->length () - offset;
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch) + regnum, xfer,
			  gdbarch_byte_order (gdbarch), readbuf, writebuf,
			  offset);
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* N32/N64 ABI stuff.  */

/* Search for a naturally aligned double at OFFSET inside a struct
   ARG_TYPE.  The N32 / N64 ABIs pass these in floating point
   registers.  */

static int
mips_n32n64_fp_arg_chunk_p (struct gdbarch *gdbarch, struct type *arg_type,
			    int offset)
{
  int i;

  if (arg_type->code () != TYPE_CODE_STRUCT)
    return 0;

  if (mips_get_fpu_type (gdbarch) != MIPS_FPU_DOUBLE)
    return 0;

  if (arg_type->length () < offset + MIPS64_REGSIZE)
    return 0;

  for (i = 0; i < arg_type->num_fields (); i++)
    {
      int pos;
      struct type *field_type;

      /* We're only looking at normal fields.  */
      if (arg_type->field (i).is_static ()
	  || (arg_type->field (i).loc_bitpos () % 8) != 0)
	continue;

      /* If we have gone past the offset, there is no double to pass.  */
      pos = arg_type->field (i).loc_bitpos () / 8;
      if (pos > offset)
	return 0;

      field_type = check_typedef (arg_type->field (i).type ());

      /* If this field is entirely before the requested offset, go
	 on to the next one.  */
      if (pos + field_type->length () <= offset)
	continue;

      /* If this is our special aligned double, we can stop.  */
      if (field_type->code () == TYPE_CODE_FLT
	  && field_type->length () == MIPS64_REGSIZE)
	return 1;

      /* This field starts at or before the requested offset, and
	 overlaps it.  If it is a structure, recurse inwards.  */
      return mips_n32n64_fp_arg_chunk_p (gdbarch, field_type, offset - pos);
    }

  return 0;
}

static CORE_ADDR
mips_n32n64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			     struct regcache *regcache, CORE_ADDR bp_addr,
			     int nargs, struct value **args, CORE_ADDR sp,
			     function_call_return_method return_method,
			     CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int arg_space = 0;
  int stack_offset = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    arg_space += align_up (args[argnum]->type ()->length (),
			   MIPS64_REGSIZE);
  sp -= align_up (arg_space, 16);

  if (mips_debug)
    gdb_printf (gdb_stdlog,
		"mips_n32n64_push_dummy_call: sp=%s allocated %ld\n",
		paddress (gdbarch, sp),
		(long) align_up (arg_space, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (return_method == return_method_struct)
    {
      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_n32n64_push_dummy_call: "
		    "struct_return reg=%d %s\n",
		    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (arg->type ());
      int len = arg_type->length ();
      enum type_code typecode = arg_type->code ();

      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_n32n64_push_dummy_call: %d len=%d type=%d",
		    argnum + 1, len, (int) typecode);

      val = arg->contents ().data ();

      /* A 128-bit long double value requires an even-odd pair of
	 floating-point registers.  */
      if (len == 16
	  && fp_register_arg_p (gdbarch, typecode, arg_type)
	  && (float_argreg & 1))
	{
	  float_argreg++;
	  argreg++;
	}

      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && argreg <= mips_last_arg_regnum (gdbarch))
	{
	  /* This is a floating point value that fits entirely
	     in a single register or a pair of registers.  */
	  int reglen = (len <= MIPS64_REGSIZE ? len : MIPS64_REGSIZE);
	  LONGEST regval = extract_unsigned_integer (val, reglen, byte_order);
	  if (mips_debug)
	    gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			float_argreg, phex (regval, reglen));
	  regcache_cooked_write_unsigned (regcache, float_argreg, regval);

	  if (mips_debug)
	    gdb_printf (gdb_stdlog, " - reg=%d val=%s",
			argreg, phex (regval, reglen));
	  regcache_cooked_write_unsigned (regcache, argreg, regval);
	  float_argreg++;
	  argreg++;
	  if (len == 16)
	    {
	      regval = extract_unsigned_integer (val + reglen,
						 reglen, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg, phex (regval, reglen));
	      regcache_cooked_write_unsigned (regcache, float_argreg, regval);

	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - reg=%d val=%s",
			    argreg, phex (regval, reglen));
	      regcache_cooked_write_unsigned (regcache, argreg, regval);
	      float_argreg++;
	      argreg++;
	    }
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* For N32/N64, structs, unions, or other composite types are
	     treated as a sequence of doublewords, and are passed in integer
	     or floating point registers as though they were simple scalar
	     parameters to the extent that they fit, with any excess on the
	     stack packed according to the normal memory layout of the
	     object.
	     The caller does not reserve space for the register arguments;
	     the callee is responsible for reserving it if required.  */
	  /* Note: Floating-point values that didn't fit into an FP
	     register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = (len < MIPS64_REGSIZE ? len : MIPS64_REGSIZE);

	      if (mips_debug)
		gdb_printf (gdb_stdlog, " -- partial=%d",
			    partial_len);

	      if (fp_register_arg_p (gdbarch, typecode, arg_type))
		gdb_assert (argreg > mips_last_arg_regnum (gdbarch));

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > mips_last_arg_regnum (gdbarch))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		    {
		      if ((typecode == TYPE_CODE_INT
			   || typecode == TYPE_CODE_PTR)
			  && len <= 4)
			longword_offset = MIPS64_REGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      gdb_printf (gdb_stdlog, " - stack_offset=%s",
				  paddress (gdbarch, stack_offset));
		      gdb_printf (gdb_stdlog, " longword_offset=%s",
				  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      gdb_printf (gdb_stdlog, " @%s ",
				  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  gdb_printf (gdb_stdlog, "%02x",
				      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
		 structs may go thru BOTH paths.  */
	      /* Write this portion of the argument to a general
		 purpose register.  */
	      if (argreg <= mips_last_arg_regnum (gdbarch))
		{
		  LONGEST regval;

		  /* Sign extend pointers, 32-bit integers and signed
		     16-bit and 8-bit integers; everything else is taken
		     as is.  */

		  if ((partial_len == 4
		       && (typecode == TYPE_CODE_PTR
			   || typecode == TYPE_CODE_INT))
		      || (partial_len < 4
			  && typecode == TYPE_CODE_INT
			  && !arg_type->is_unsigned ()))
		    regval = extract_signed_integer (val, partial_len,
						     byte_order);
		  else
		    regval = extract_unsigned_integer (val, partial_len,
						       byte_order);

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.  */

		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		      && partial_len < MIPS64_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT
			  || typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS64_REGSIZE - partial_len)
				* TARGET_CHAR_BIT);

		  if (mips_debug)
		    gdb_printf (gdb_stdlog, " - reg=%d val=%s",
				argreg,
				phex (regval, MIPS64_REGSIZE));
		  regcache_cooked_write_unsigned (regcache, argreg, regval);

		  if (mips_n32n64_fp_arg_chunk_p (gdbarch, arg_type,
						  arg_type->length () - len))
		    {
		      if (mips_debug)
			gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg,
				    phex (regval, MIPS64_REGSIZE));
		      regcache_cooked_write_unsigned (regcache, float_argreg,
						      regval);
		    }

		  float_argreg++;
		  argreg++;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
		 copy the next parameter.

		 In N32 (N64?), the stack_offset only needs to be
		 adjusted when it has been used.  */

	      if (stack_used_p)
		stack_offset += align_up (partial_len, MIPS64_REGSIZE);
	    }
	}
      if (mips_debug)
	gdb_printf (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

static enum return_value_convention
mips_n32n64_return_value (struct gdbarch *gdbarch, struct value *function,
			  struct type *type, struct regcache *regcache,
			  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

  /* From MIPSpro N32 ABI Handbook, Document Number: 007-2816-004

     Function results are returned in $2 (and $3 if needed), or $f0 (and $f2
     if needed), as appropriate for the type.  Composite results (struct,
     union, or array) are returned in $2/$f0 and $3/$f2 according to the
     following rules:

     * A struct with only one or two floating point fields is returned in $f0
     (and $f2 if necessary).  This is a generalization of the Fortran COMPLEX
     case.

     * Any other composite results of at most 128 bits are returned in
     $2 (first 64 bits) and $3 (remainder, if necessary).

     * Larger composite results are handled by converting the function to a
     procedure with an implicit first parameter, which is a pointer to an area
     reserved by the caller to receive the result.  [The o32-bit ABI requires
     that all composite results be handled by conversion to implicit first
     parameters.  The MIPS/SGI Fortran implementation has always made a
     specific exception to return COMPLEX results in the floating point
     registers.]

     From MIPSpro Assembly Language Programmer's Guide, Document Number:
     007-2418-004

	      Software
     Register Name(from
     Name     fgregdef.h) Use and Linkage
     -----------------------------------------------------------------
     $f0, $f2 fv0, fv1    Hold results of floating-point type function
			  ($f0) and complex type function ($f0 has the
			  real part, $f2 has the imaginary part.)  */

  if (type->length () > 2 * MIPS64_REGSIZE)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else if ((type->code () == TYPE_CODE_COMPLEX
	    || (type->code () == TYPE_CODE_FLT && type->length () == 16))
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A complex value of up to 128 bits in width as well as a 128-bit
	 floating-point value goes in both $f0 and $f2.  A single complex
	 value is held in the lower halves only of the respective registers.
	 The two registers are used in the same as memory order, so the
	 bytes with the lower memory address are in $f0.  */
      if (mips_debug)
	gdb_printf (gdb_stderr, "Return float in $f0 and $f2\n");
      mips_xfer_register (gdbarch, regcache,
			  (gdbarch_num_regs (gdbarch)
			   + mips_regnum (gdbarch)->fp0),
			  type->length () / 2, gdbarch_byte_order (gdbarch),
			  readbuf, writebuf, 0);
      mips_xfer_register (gdbarch, regcache,
			  (gdbarch_num_regs (gdbarch)
			   + mips_regnum (gdbarch)->fp0 + 2),
			  type->length () / 2, gdbarch_byte_order (gdbarch),
			  readbuf ? readbuf + type->length () / 2 : readbuf,
			  (writebuf
			   ? writebuf + type->length () / 2 : writebuf), 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (type->code () == TYPE_CODE_FLT
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A single or double floating-point value that fits in FP0.  */
      if (mips_debug)
	gdb_printf (gdb_stderr, "Return float in $fp0\n");
      mips_xfer_register (gdbarch, regcache,
			  (gdbarch_num_regs (gdbarch)
			   + mips_regnum (gdbarch)->fp0),
			  type->length (),
			  gdbarch_byte_order (gdbarch),
			  readbuf, writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (type->code () == TYPE_CODE_STRUCT
	   && type->num_fields () <= 2
	   && type->num_fields () >= 1
	   && ((type->num_fields () == 1
		&& (check_typedef (type->field (0).type ())->code ()
		    == TYPE_CODE_FLT))
	       || (type->num_fields () == 2
		   && (check_typedef (type->field (0).type ())->code ()
		       == TYPE_CODE_FLT)
		   && (check_typedef (type->field (1).type ())->code ()
		       == TYPE_CODE_FLT))))
    {
      /* A struct that contains one or two floats.  Each value is part
	 in the least significant part of their floating point
	 register (or GPR, for soft float).  */
      int regnum;
      int field;
      for (field = 0, regnum = (tdep->mips_fpu_type != MIPS_FPU_NONE
				? mips_regnum (gdbarch)->fp0
				: MIPS_V0_REGNUM);
	   field < type->num_fields (); field++, regnum += 2)
	{
	  int offset = type->field (field).loc_bitpos () / TARGET_CHAR_BIT;
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return float struct+%d\n",
			offset);
	  if (type->field (field).type ()->length () == 16)
	    {
	      /* A 16-byte long double field goes in two consecutive
		 registers.  */
	      mips_xfer_register (gdbarch, regcache,
				  gdbarch_num_regs (gdbarch) + regnum,
				  8,
				  gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, offset);
	      mips_xfer_register (gdbarch, regcache,
				  gdbarch_num_regs (gdbarch) + regnum + 1,
				  8,
				  gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, offset + 8);
	    }
	  else
	    mips_xfer_register (gdbarch, regcache,
				gdbarch_num_regs (gdbarch) + regnum,
				type->field (field).type ()->length (),
				gdbarch_byte_order (gdbarch),
				readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (type->code () == TYPE_CODE_STRUCT
	   || type->code () == TYPE_CODE_UNION
	   || type->code () == TYPE_CODE_ARRAY)
    {
      /* A composite type.  Extract the left justified value,
	 regardless of the byte order.  I.e. DO NOT USE
	 mips_xfer_lower.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < type->length ();
	   offset += register_size (gdbarch, regnum), regnum++)
	{
	  int xfer = register_size (gdbarch, regnum);
	  if (offset + xfer > type->length ())
	    xfer = type->length () - offset;
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return struct+%d:%d in $%d\n",
			offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      xfer, BFD_ENDIAN_UNKNOWN, readbuf, writebuf,
			      offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else
    {
      /* A scalar extract each part but least-significant-byte
	 justified.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < type->length ();
	   offset += register_size (gdbarch, regnum), regnum++)
	{
	  int xfer = register_size (gdbarch, regnum);
	  if (offset + xfer > type->length ())
	    xfer = type->length () - offset;
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return scalar+%d:%d in $%d\n",
			offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      xfer, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Which registers to use for passing floating-point values between
   function calls, one of floating-point, general and both kinds of
   registers.  O32 and O64 use different register kinds for standard
   MIPS and MIPS16 code; to make the handling of cases where we may
   not know what kind of code is being used (e.g. no debug information)
   easier we sometimes use both kinds.  */

enum mips_fval_reg
{
  mips_fval_fpr,
  mips_fval_gpr,
  mips_fval_both
};

/* O32 ABI stuff.  */

static CORE_ADDR
mips_o32_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			  struct regcache *regcache, CORE_ADDR bp_addr,
			  int nargs, struct value **args, CORE_ADDR sp,
			  function_call_return_method return_method,
			  CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int arg_space = 0;
  int stack_offset = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct type *arg_type = check_typedef (args[argnum]->type ());

      /* Align to double-word if necessary.  */
      if (mips_type_needs_double_align (arg_type))
	arg_space = align_up (arg_space, MIPS32_REGSIZE * 2);
      /* Allocate space on the stack.  */
      arg_space += align_up (arg_type->length (), MIPS32_REGSIZE);
    }
  sp -= align_up (arg_space, 16);

  if (mips_debug)
    gdb_printf (gdb_stdlog,
		"mips_o32_push_dummy_call: sp=%s allocated %ld\n",
		paddress (gdbarch, sp),
		(long) align_up (arg_space, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (return_method == return_method_struct)
    {
      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_o32_push_dummy_call: "
		    "struct_return reg=%d %s\n",
		    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
      stack_offset += MIPS32_REGSIZE;
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (arg->type ());
      int len = arg_type->length ();
      enum type_code typecode = arg_type->code ();

      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_o32_push_dummy_call: %d len=%d type=%d",
		    argnum + 1, len, (int) typecode);

      val = arg->contents ().data ();

      /* 32-bit ABIs always start floating point arguments in an
	 even-numbered floating point register.  Round the FP register
	 up before the check to see if there are any FP registers
	 left.  O32 targets also pass the FP in the integer registers
	 so also round up normal registers.  */
      if (fp_register_arg_p (gdbarch, typecode, arg_type))
	{
	  if ((float_argreg & 1))
	    float_argreg++;
	}

      /* Floating point arguments passed in registers have to be
	 treated specially.  On 32-bit architectures, doubles are
	 passed in register pairs; the even FP register gets the
	 low word, and the odd FP register gets the high word.
	 On O32, the first two floating point arguments are also
	 copied to general registers, following their memory order,
	 because MIPS16 functions don't use float registers for
	 arguments.  This duplication of arguments in general
	 registers can't hurt non-MIPS16 functions, because those
	 registers are normally skipped.  */

      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && float_argreg <= mips_last_fp_arg_regnum (gdbarch))
	{
	  if (register_size (gdbarch, float_argreg) < 8 && len == 8)
	    {
	      int freg_offset = gdbarch_byte_order (gdbarch)
				== BFD_ENDIAN_BIG ? 1 : 0;
	      unsigned long regval;

	      /* First word.  */
	      regval = extract_unsigned_integer (val, 4, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg + freg_offset,
			    phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache,
					      float_argreg++ + freg_offset,
					      regval);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - reg=%d val=%s",
			    argreg, phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);

	      /* Second word.  */
	      regval = extract_unsigned_integer (val + 4, 4, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg - freg_offset,
			    phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache,
					      float_argreg++ - freg_offset,
					      regval);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - reg=%d val=%s",
			    argreg, phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
		 in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
		 above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_unsigned_integer (val, len, byte_order);
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			    float_argreg, phex (regval, len));
	      regcache_cooked_write_unsigned (regcache,
					      float_argreg++, regval);
	      /* Although two FP registers are reserved for each
		 argument, only one corresponding integer register is
		 reserved.  */
	      if (mips_debug)
		gdb_printf (gdb_stdlog, " - reg=%d val=%s",
			    argreg, phex (regval, len));
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);
	    }
	  /* Reserve space for the FP register.  */
	  stack_offset += align_up (len, MIPS32_REGSIZE);
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of MIPS32_REGSIZE
	     are treated specially: Irix cc passes
	     them in registers where gcc sometimes puts them on the
	     stack.  For maximum compatibility, we will put them in
	     both places.  */
	  int odd_sized_struct = (len > MIPS32_REGSIZE
				  && len % MIPS32_REGSIZE != 0);
	  /* Structures should be aligned to eight bytes (even arg registers)
	     on MIPS_ABI_O32, if their first member has double precision.  */
	  if (mips_type_needs_double_align (arg_type))
	    {
	      if ((argreg & 1))
		{
		  argreg++;
		  stack_offset += MIPS32_REGSIZE;
		}
	    }
	  while (len > 0)
	    {
	      int partial_len = (len < MIPS32_REGSIZE ? len : MIPS32_REGSIZE);

	      if (mips_debug)
		gdb_printf (gdb_stdlog, " -- partial=%d",
			    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > mips_last_arg_regnum (gdbarch)
		  || odd_sized_struct)
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;

		  if (mips_debug)
		    {
		      gdb_printf (gdb_stdlog, " - stack_offset=%s",
				  paddress (gdbarch, stack_offset));
		      gdb_printf (gdb_stdlog, " longword_offset=%s",
				  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      gdb_printf (gdb_stdlog, " @%s ",
				  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  gdb_printf (gdb_stdlog, "%02x",
				      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
		 structs may go thru BOTH paths.  */
	      /* Write this portion of the argument to a general
		 purpose register.  */
	      if (argreg <= mips_last_arg_regnum (gdbarch))
		{
		  LONGEST regval = extract_signed_integer (val, partial_len,
							   byte_order);
		  /* Value may need to be sign extended, because
		     mips_isa_regsize() != mips_abi_regsize().  */

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.

		     Also don't do this adjustment on O64 binaries.

		     cagney/2001-07-23: gdb/179: Also, GCC, when
		     outputting LE O32 with sizeof (struct) <
		     mips_abi_regsize(), generates a left shift
		     as part of storing the argument in a register
		     (the left shift isn't generated when
		     sizeof (struct) >= mips_abi_regsize()).  Since
		     it is quite possible that this is GCC
		     contradicting the LE/O32 ABI, GDB has not been
		     adjusted to accommodate this.  Either someone
		     needs to demonstrate that the LE/O32 ABI
		     specifies such a left shift OR this new ABI gets
		     identified as such and GDB gets tweaked
		     accordingly.  */

		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		      && partial_len < MIPS32_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT
			  || typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS32_REGSIZE - partial_len)
				* TARGET_CHAR_BIT);

		  if (mips_debug)
		    gdb_printf (gdb_stdlog, " - reg=%d val=%s",
				argreg,
				phex (regval, MIPS32_REGSIZE));
		  regcache_cooked_write_unsigned (regcache, argreg, regval);
		  argreg++;

		  /* Prevent subsequent floating point arguments from
		     being passed in floating point registers.  */
		  float_argreg = mips_last_fp_arg_regnum (gdbarch) + 1;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
		 copy the next parameter.

		 In older ABIs, the caller reserved space for
		 registers that contained arguments.  This was loosely
		 referred to as their "home".  Consequently, space is
		 always allocated.  */

	      stack_offset += align_up (partial_len, MIPS32_REGSIZE);
	    }
	}
      if (mips_debug)
	gdb_printf (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

static enum return_value_convention
mips_o32_return_value (struct gdbarch *gdbarch, struct value *function,
		       struct type *type, struct regcache *regcache,
		       gdb_byte *readbuf, const gdb_byte *writebuf)
{
  CORE_ADDR func_addr = function ? find_function_addr (function, NULL) : 0;
  int mips16 = mips_pc_is_mips16 (gdbarch, func_addr);
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  enum mips_fval_reg fval_reg;

  fval_reg = readbuf ? mips16 ? mips_fval_gpr : mips_fval_fpr : mips_fval_both;
  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_ARRAY)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else if (type->code () == TYPE_CODE_FLT
	   && type->length () == 4 && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A single-precision floating-point value.  If reading in or copying,
	 then we get it from/put it to FP0 for standard MIPS code or GPR2
	 for MIPS16 code.  If writing out only, then we put it to both FP0
	 and GPR2.  We do not support reading in with no function known, if
	 this safety check ever triggers, then we'll have to try harder.  */
      gdb_assert (function || !readbuf);
      if (mips_debug)
	switch (fval_reg)
	  {
	  case mips_fval_fpr:
	    gdb_printf (gdb_stderr, "Return float in $fp0\n");
	    break;
	  case mips_fval_gpr:
	    gdb_printf (gdb_stderr, "Return float in $2\n");
	    break;
	  case mips_fval_both:
	    gdb_printf (gdb_stderr, "Return float in $fp0 and $2\n");
	    break;
	  }
      if (fval_reg != mips_fval_gpr)
	mips_xfer_register (gdbarch, regcache,
			    (gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->fp0),
			    type->length (),
			    gdbarch_byte_order (gdbarch),
			    readbuf, writebuf, 0);
      if (fval_reg != mips_fval_fpr)
	mips_xfer_register (gdbarch, regcache,
			    gdbarch_num_regs (gdbarch) + 2,
			    type->length (),
			    gdbarch_byte_order (gdbarch),
			    readbuf, writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (type->code () == TYPE_CODE_FLT
	   && type->length () == 8 && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A double-precision floating-point value.  If reading in or copying,
	 then we get it from/put it to FP1 and FP0 for standard MIPS code or
	 GPR2 and GPR3 for MIPS16 code.  If writing out only, then we put it
	 to both FP1/FP0 and GPR2/GPR3.  We do not support reading in with
	 no function known, if this safety check ever triggers, then we'll
	 have to try harder.  */
      gdb_assert (function || !readbuf);
      if (mips_debug)
	switch (fval_reg)
	  {
	  case mips_fval_fpr:
	    gdb_printf (gdb_stderr, "Return float in $fp1/$fp0\n");
	    break;
	  case mips_fval_gpr:
	    gdb_printf (gdb_stderr, "Return float in $2/$3\n");
	    break;
	  case mips_fval_both:
	    gdb_printf (gdb_stderr,
			"Return float in $fp1/$fp0 and $2/$3\n");
	    break;
	  }
      if (fval_reg != mips_fval_gpr)
	{
	  /* The most significant part goes in FP1, and the least significant
	     in FP0.  */
	  switch (gdbarch_byte_order (gdbarch))
	    {
	    case BFD_ENDIAN_LITTLE:
	      mips_xfer_register (gdbarch, regcache,
				  (gdbarch_num_regs (gdbarch)
				   + mips_regnum (gdbarch)->fp0 + 0),
				  4, gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, 0);
	      mips_xfer_register (gdbarch, regcache,
				  (gdbarch_num_regs (gdbarch)
				   + mips_regnum (gdbarch)->fp0 + 1),
				  4, gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, 4);
	      break;
	    case BFD_ENDIAN_BIG:
	      mips_xfer_register (gdbarch, regcache,
				  (gdbarch_num_regs (gdbarch)
				   + mips_regnum (gdbarch)->fp0 + 1),
				  4, gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, 0);
	      mips_xfer_register (gdbarch, regcache,
				  (gdbarch_num_regs (gdbarch)
				   + mips_regnum (gdbarch)->fp0 + 0),
				  4, gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, 4);
	      break;
	    default:
	      internal_error (_("bad switch"));
	    }
	}
      if (fval_reg != mips_fval_fpr)
	{
	  /* The two 32-bit parts are always placed in GPR2 and GPR3
	     following these registers' memory order.  */
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + 2,
			      4, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, 0);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + 3,
			      4, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, 4);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
#if 0
  else if (type->code () == TYPE_CODE_STRUCT
	   && type->num_fields () <= 2
	   && type->num_fields () >= 1
	   && ((type->num_fields () == 1
		&& (TYPE_CODE (type->field (0).type ())
		    == TYPE_CODE_FLT))
	       || (type->num_fields () == 2
		   && (TYPE_CODE (type->field (0).type ())
		       == TYPE_CODE_FLT)
		   && (TYPE_CODE (type->field (1).type ())
		       == TYPE_CODE_FLT)))
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A struct that contains one or two floats.  Each value is part
	 in the least significant part of their floating point
	 register..  */
      int regnum;
      int field;
      for (field = 0, regnum = mips_regnum (gdbarch)->fp0;
	   field < type->num_fields (); field++, regnum += 2)
	{
	  int offset = (type->fields ()[field].loc_bitpos () / TARGET_CHAR_BIT);
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return float struct+%d\n",
			offset);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      TYPE_LENGTH (type->field (field).type ()),
			      gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
#endif
#if 0
  else if (type->code () == TYPE_CODE_STRUCT
	   || type->code () == TYPE_CODE_UNION)
    {
      /* A structure or union.  Extract the left justified value,
	 regardless of the byte order.  I.e. DO NOT USE
	 mips_xfer_lower.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < type->length ();
	   offset += register_size (gdbarch, regnum), regnum++)
	{
	  int xfer = register_size (gdbarch, regnum);
	  if (offset + xfer > type->length ())
	    xfer = type->length () - offset;
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return struct+%d:%d in $%d\n",
			offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum, xfer,
			      BFD_ENDIAN_UNKNOWN, readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
#endif
  else
    {
      /* A scalar extract each part but least-significant-byte
	 justified.  o32 thinks registers are 4 byte, regardless of
	 the ISA.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < type->length ();
	   offset += MIPS32_REGSIZE, regnum++)
	{
	  int xfer = MIPS32_REGSIZE;
	  if (offset + xfer > type->length ())
	    xfer = type->length () - offset;
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return scalar+%d:%d in $%d\n",
			offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum, xfer,
			      gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* O64 ABI.  This is a hacked up kind of 64-bit version of the o32
   ABI.  */

static CORE_ADDR
mips_o64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			  struct regcache *regcache, CORE_ADDR bp_addr,
			  int nargs,
			  struct value **args, CORE_ADDR sp,
			  function_call_return_method return_method, CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int arg_space = 0;
  int stack_offset = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct type *arg_type = check_typedef (args[argnum]->type ());

      /* Allocate space on the stack.  */
      arg_space += align_up (arg_type->length (), MIPS64_REGSIZE);
    }
  sp -= align_up (arg_space, 16);

  if (mips_debug)
    gdb_printf (gdb_stdlog,
		"mips_o64_push_dummy_call: sp=%s allocated %ld\n",
		paddress (gdbarch, sp),
		(long) align_up (arg_space, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (return_method == return_method_struct)
    {
      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_o64_push_dummy_call: "
		    "struct_return reg=%d %s\n",
		    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
      stack_offset += MIPS64_REGSIZE;
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (arg->type ());
      int len = arg_type->length ();
      enum type_code typecode = arg_type->code ();

      if (mips_debug)
	gdb_printf (gdb_stdlog,
		    "mips_o64_push_dummy_call: %d len=%d type=%d",
		    argnum + 1, len, (int) typecode);

      val = arg->contents ().data ();

      /* Floating point arguments passed in registers have to be
	 treated specially.  On 32-bit architectures, doubles are
	 passed in register pairs; the even FP register gets the
	 low word, and the odd FP register gets the high word.
	 On O64, the first two floating point arguments are also
	 copied to general registers, because MIPS16 functions
	 don't use float registers for arguments.  This duplication
	 of arguments in general registers can't hurt non-MIPS16
	 functions because those registers are normally skipped.  */

      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && float_argreg <= mips_last_fp_arg_regnum (gdbarch))
	{
	  LONGEST regval = extract_unsigned_integer (val, len, byte_order);
	  if (mips_debug)
	    gdb_printf (gdb_stdlog, " - fpreg=%d val=%s",
			float_argreg, phex (regval, len));
	  regcache_cooked_write_unsigned (regcache, float_argreg++, regval);
	  if (mips_debug)
	    gdb_printf (gdb_stdlog, " - reg=%d val=%s",
			argreg, phex (regval, len));
	  regcache_cooked_write_unsigned (regcache, argreg, regval);
	  argreg++;
	  /* Reserve space for the FP register.  */
	  stack_offset += align_up (len, MIPS64_REGSIZE);
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of MIPS64_REGSIZE
	     are treated specially: Irix cc passes them in registers
	     where gcc sometimes puts them on the stack.  For maximum
	     compatibility, we will put them in both places.  */
	  int odd_sized_struct = (len > MIPS64_REGSIZE
				  && len % MIPS64_REGSIZE != 0);
	  while (len > 0)
	    {
	      int partial_len = (len < MIPS64_REGSIZE ? len : MIPS64_REGSIZE);

	      if (mips_debug)
		gdb_printf (gdb_stdlog, " -- partial=%d",
			    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > mips_last_arg_regnum (gdbarch)
		  || odd_sized_struct)
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		    {
		      if ((typecode == TYPE_CODE_INT
			   || typecode == TYPE_CODE_PTR
			   || typecode == TYPE_CODE_FLT)
			  && len <= 4)
			longword_offset = MIPS64_REGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      gdb_printf (gdb_stdlog, " - stack_offset=%s",
				  paddress (gdbarch, stack_offset));
		      gdb_printf (gdb_stdlog, " longword_offset=%s",
				  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      gdb_printf (gdb_stdlog, " @%s ",
				  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  gdb_printf (gdb_stdlog, "%02x",
				      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
		 structs may go thru BOTH paths.  */
	      /* Write this portion of the argument to a general
		 purpose register.  */
	      if (argreg <= mips_last_arg_regnum (gdbarch))
		{
		  LONGEST regval = extract_signed_integer (val, partial_len,
							   byte_order);
		  /* Value may need to be sign extended, because
		     mips_isa_regsize() != mips_abi_regsize().  */

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.  */

		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		      && partial_len < MIPS64_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT
			  || typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS64_REGSIZE - partial_len)
				* TARGET_CHAR_BIT);

		  if (mips_debug)
		    gdb_printf (gdb_stdlog, " - reg=%d val=%s",
				argreg,
				phex (regval, MIPS64_REGSIZE));
		  regcache_cooked_write_unsigned (regcache, argreg, regval);
		  argreg++;

		  /* Prevent subsequent floating point arguments from
		     being passed in floating point registers.  */
		  float_argreg = mips_last_fp_arg_regnum (gdbarch) + 1;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
		 copy the next parameter.

		 In older ABIs, the caller reserved space for
		 registers that contained arguments.  This was loosely
		 referred to as their "home".  Consequently, space is
		 always allocated.  */

	      stack_offset += align_up (partial_len, MIPS64_REGSIZE);
	    }
	}
      if (mips_debug)
	gdb_printf (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

static enum return_value_convention
mips_o64_return_value (struct gdbarch *gdbarch, struct value *function,
		       struct type *type, struct regcache *regcache,
		       gdb_byte *readbuf, const gdb_byte *writebuf)
{
  CORE_ADDR func_addr = function ? find_function_addr (function, NULL) : 0;
  int mips16 = mips_pc_is_mips16 (gdbarch, func_addr);
  enum mips_fval_reg fval_reg;

  fval_reg = readbuf ? mips16 ? mips_fval_gpr : mips_fval_fpr : mips_fval_both;
  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_ARRAY)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else if (fp_register_arg_p (gdbarch, type->code (), type))
    {
      /* A floating-point value.  If reading in or copying, then we get it
	 from/put it to FP0 for standard MIPS code or GPR2 for MIPS16 code.
	 If writing out only, then we put it to both FP0 and GPR2.  We do
	 not support reading in with no function known, if this safety
	 check ever triggers, then we'll have to try harder.  */
      gdb_assert (function || !readbuf);
      if (mips_debug)
	switch (fval_reg)
	  {
	  case mips_fval_fpr:
	    gdb_printf (gdb_stderr, "Return float in $fp0\n");
	    break;
	  case mips_fval_gpr:
	    gdb_printf (gdb_stderr, "Return float in $2\n");
	    break;
	  case mips_fval_both:
	    gdb_printf (gdb_stderr, "Return float in $fp0 and $2\n");
	    break;
	  }
      if (fval_reg != mips_fval_gpr)
	mips_xfer_register (gdbarch, regcache,
			    (gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->fp0),
			    type->length (),
			    gdbarch_byte_order (gdbarch),
			    readbuf, writebuf, 0);
      if (fval_reg != mips_fval_fpr)
	mips_xfer_register (gdbarch, regcache,
			    gdbarch_num_regs (gdbarch) + 2,
			    type->length (),
			    gdbarch_byte_order (gdbarch),
			    readbuf, writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else
    {
      /* A scalar extract each part but least-significant-byte
	 justified.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < type->length ();
	   offset += MIPS64_REGSIZE, regnum++)
	{
	  int xfer = MIPS64_REGSIZE;
	  if (offset + xfer > type->length ())
	    xfer = type->length () - offset;
	  if (mips_debug)
	    gdb_printf (gdb_stderr, "Return scalar+%d:%d in $%d\n",
			offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      xfer, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Floating point register management.

   Background: MIPS1 & 2 fp registers are 32 bits wide.  To support
   64bit operations, these early MIPS cpus treat fp register pairs
   (f0,f1) as a single register (d0).  Later MIPS cpu's have 64 bit fp
   registers and offer a compatibility mode that emulates the MIPS2 fp
   model.  When operating in MIPS2 fp compat mode, later cpu's split
   double precision floats into two 32-bit chunks and store them in
   consecutive fp regs.  To display 64-bit floats stored in this
   fashion, we have to combine 32 bits from f0 and 32 bits from f1.
   Throw in user-configurable endianness and you have a real mess.

   The way this works is:
     - If we are in 32-bit mode or on a 32-bit processor, then a 64-bit
       double-precision value will be split across two logical registers.
       The lower-numbered logical register will hold the low-order bits,
       regardless of the processor's endianness.
     - If we are on a 64-bit processor, and we are looking for a
       single-precision value, it will be in the low ordered bits
       of a 64-bit GPR (after mfc1, for example) or a 64-bit register
       save slot in memory.
     - If we are in 64-bit mode, everything is straightforward.

   Note that this code only deals with "live" registers at the top of the
   stack.  We will attempt to deal with saved registers later, when
   the raw/cooked register interface is in place.  (We need a general
   interface that can deal with dynamic saved register sizes -- fp
   regs could be 32 bits wide in one frame and 64 on the frame above
   and below).  */

/* Copy a 32-bit single-precision value from the current frame
   into rare_buffer.  */

static void
mips_read_fp_register_single (frame_info_ptr frame, int regno,
			      gdb_byte *rare_buffer)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int raw_size = register_size (gdbarch, regno);
  gdb_byte *raw_buffer = (gdb_byte *) alloca (raw_size);

  if (!deprecated_frame_register_read (frame, regno, raw_buffer))
    error (_("can't read register %d (%s)"),
	   regno, gdbarch_register_name (gdbarch, regno));
  if (raw_size == 8)
    {
      /* We have a 64-bit value for this register.  Find the low-order
	 32 bits.  */
      int offset;

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = 4;
      else
	offset = 0;

      memcpy (rare_buffer, raw_buffer + offset, 4);
    }
  else
    {
      memcpy (rare_buffer, raw_buffer, 4);
    }
}

/* Copy a 64-bit double-precision value from the current frame into
   rare_buffer.  This may include getting half of it from the next
   register.  */

static void
mips_read_fp_register_double (frame_info_ptr frame, int regno,
			      gdb_byte *rare_buffer)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int raw_size = register_size (gdbarch, regno);

  if (raw_size == 8 && !mips2_fp_compat (frame))
    {
      /* We have a 64-bit value for this register, and we should use
	 all 64 bits.  */
      if (!deprecated_frame_register_read (frame, regno, rare_buffer))
	error (_("can't read register %d (%s)"),
	       regno, gdbarch_register_name (gdbarch, regno));
    }
  else
    {
      int rawnum = regno % gdbarch_num_regs (gdbarch);

      if ((rawnum - mips_regnum (gdbarch)->fp0) & 1)
	internal_error (_("mips_read_fp_register_double: bad access to "
			"odd-numbered FP register"));

      /* mips_read_fp_register_single will find the correct 32 bits from
	 each register.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	{
	  mips_read_fp_register_single (frame, regno, rare_buffer + 4);
	  mips_read_fp_register_single (frame, regno + 1, rare_buffer);
	}
      else
	{
	  mips_read_fp_register_single (frame, regno, rare_buffer);
	  mips_read_fp_register_single (frame, regno + 1, rare_buffer + 4);
	}
    }
}

static void
mips_print_fp_register (struct ui_file *file, frame_info_ptr frame,
			int regnum)
{				/* Do values for FP (float) regs.  */
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte *raw_buffer;
  std::string flt_str, dbl_str;

  const struct type *flt_type = builtin_type (gdbarch)->builtin_float;
  const struct type *dbl_type = builtin_type (gdbarch)->builtin_double;

  raw_buffer
    = ((gdb_byte *)
       alloca (2 * register_size (gdbarch, mips_regnum (gdbarch)->fp0)));

  gdb_printf (file, "%s:", gdbarch_register_name (gdbarch, regnum));
  gdb_printf (file, "%*s",
	      4 - (int) strlen (gdbarch_register_name (gdbarch, regnum)),
	      "");

  if (register_size (gdbarch, regnum) == 4 || mips2_fp_compat (frame))
    {
      struct value_print_options opts;

      /* 4-byte registers: Print hex and floating.  Also print even
	 numbered registers as doubles.  */
      mips_read_fp_register_single (frame, regnum, raw_buffer);
      flt_str = target_float_to_string (raw_buffer, flt_type, "%-17.9g");

      get_formatted_print_options (&opts, 'x');
      print_scalar_formatted (raw_buffer,
			      builtin_type (gdbarch)->builtin_uint32,
			      &opts, 'w', file);

      gdb_printf (file, " flt: %s", flt_str.c_str ());

      if ((regnum - gdbarch_num_regs (gdbarch)) % 2 == 0)
	{
	  mips_read_fp_register_double (frame, regnum, raw_buffer);
	  dbl_str = target_float_to_string (raw_buffer, dbl_type, "%-24.17g");

	  gdb_printf (file, " dbl: %s", dbl_str.c_str ());
	}
    }
  else
    {
      struct value_print_options opts;

      /* Eight byte registers: print each one as hex, float and double.  */
      mips_read_fp_register_single (frame, regnum, raw_buffer);
      flt_str = target_float_to_string (raw_buffer, flt_type, "%-17.9g");

      mips_read_fp_register_double (frame, regnum, raw_buffer);
      dbl_str = target_float_to_string (raw_buffer, dbl_type, "%-24.17g");

      get_formatted_print_options (&opts, 'x');
      print_scalar_formatted (raw_buffer,
			      builtin_type (gdbarch)->builtin_uint64,
			      &opts, 'g', file);

      gdb_printf (file, " flt: %s", flt_str.c_str ());
      gdb_printf (file, " dbl: %s", dbl_str.c_str ());
    }
}

static void
mips_print_register (struct ui_file *file, frame_info_ptr frame,
		     int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct value_print_options opts;
  struct value *val;

  if (mips_float_register_p (gdbarch, regnum))
    {
      mips_print_fp_register (file, frame, regnum);
      return;
    }

  val = get_frame_register_value (frame, regnum);

  gdb_puts (gdbarch_register_name (gdbarch, regnum), file);

  /* The problem with printing numeric register names (r26, etc.) is that
     the user can't use them on input.  Probably the best solution is to
     fix it so that either the numeric or the funky (a2, etc.) names
     are accepted on input.  */
  if (regnum < MIPS_NUMREGS)
    gdb_printf (file, "(r%d): ", regnum);
  else
    gdb_printf (file, ": ");

  get_formatted_print_options (&opts, 'x');
  value_print_scalar_formatted (val, &opts, 0, file);
}

/* Print IEEE exception condition bits in FLAGS.  */

static void
print_fpu_flags (struct ui_file *file, int flags)
{
  if (flags & (1 << 0))
    gdb_puts (" inexact", file);
  if (flags & (1 << 1))
    gdb_puts (" uflow", file);
  if (flags & (1 << 2))
    gdb_puts (" oflow", file);
  if (flags & (1 << 3))
    gdb_puts (" div0", file);
  if (flags & (1 << 4))
    gdb_puts (" inval", file);
  if (flags & (1 << 5))
    gdb_puts (" unimp", file);
  gdb_putc ('\n', file);
}

/* Print interesting information about the floating point processor
   (if present) or emulator.  */

static void
mips_print_float_info (struct gdbarch *gdbarch, struct ui_file *file,
		      frame_info_ptr frame, const char *args)
{
  int fcsr = mips_regnum (gdbarch)->fp_control_status;
  enum mips_fpu_type type = mips_get_fpu_type (gdbarch);
  ULONGEST fcs = 0;
  int i;

  if (fcsr == -1 || !read_frame_register_unsigned (frame, fcsr, &fcs))
    type = MIPS_FPU_NONE;

  gdb_printf (file, "fpu type: %s\n",
	      type == MIPS_FPU_DOUBLE ? "double-precision"
	      : type == MIPS_FPU_SINGLE ? "single-precision"
	      : "none / unused");

  if (type == MIPS_FPU_NONE)
    return;

  gdb_printf (file, "reg size: %d bits\n",
	      register_size (gdbarch, mips_regnum (gdbarch)->fp0) * 8);

  gdb_puts ("cond    :", file);
  if (fcs & (1 << 23))
    gdb_puts (" 0", file);
  for (i = 1; i <= 7; i++)
    if (fcs & (1 << (24 + i)))
      gdb_printf (file, " %d", i);
  gdb_putc ('\n', file);

  gdb_puts ("cause   :", file);
  print_fpu_flags (file, (fcs >> 12) & 0x3f);
  fputs ("mask    :", stdout);
  print_fpu_flags (file, (fcs >> 7) & 0x1f);
  fputs ("flags   :", stdout);
  print_fpu_flags (file, (fcs >> 2) & 0x1f);

  gdb_puts ("rounding: ", file);
  switch (fcs & 3)
    {
    case 0: gdb_puts ("nearest\n", file); break;
    case 1: gdb_puts ("zero\n", file); break;
    case 2: gdb_puts ("+inf\n", file); break;
    case 3: gdb_puts ("-inf\n", file); break;
    }

  gdb_puts ("flush   :", file);
  if (fcs & (1 << 21))
    gdb_puts (" nearest", file);
  if (fcs & (1 << 22))
    gdb_puts (" override", file);
  if (fcs & (1 << 24))
    gdb_puts (" zero", file);
  if ((fcs & (0xb << 21)) == 0)
    gdb_puts (" no", file);
  gdb_putc ('\n', file);

  gdb_printf (file, "nan2008 : %s\n", fcs & (1 << 18) ? "yes" : "no");
  gdb_printf (file, "abs2008 : %s\n", fcs & (1 << 19) ? "yes" : "no");
  gdb_putc ('\n', file);

  default_print_float_info (gdbarch, file, frame, args);
}

/* Replacement for generic do_registers_info.
   Print regs in pretty columns.  */

static int
print_fp_register_row (struct ui_file *file, frame_info_ptr frame,
		       int regnum)
{
  gdb_printf (file, " ");
  mips_print_fp_register (file, frame, regnum);
  gdb_printf (file, "\n");
  return regnum + 1;
}


/* Print a row's worth of GP (int) registers, with name labels above.  */

static int
print_gp_register_row (struct ui_file *file, frame_info_ptr frame,
		       int start_regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  /* Do values for GP (int) regs.  */
  const gdb_byte *raw_buffer;
  struct value *value;
  int ncols = (mips_abi_regsize (gdbarch) == 8 ? 4 : 8);    /* display cols
							       per row.  */
  int col, byte;
  int regnum;

  /* For GP registers, we print a separate row of names above the vals.  */
  for (col = 0, regnum = start_regnum;
       col < ncols && regnum < gdbarch_num_cooked_regs (gdbarch);
       regnum++)
    {
      if (*gdbarch_register_name (gdbarch, regnum) == '\0')
	continue;		/* unused register */
      if (mips_float_register_p (gdbarch, regnum))
	break;			/* End the row: reached FP register.  */
      /* Large registers are handled separately.  */
      if (register_size (gdbarch, regnum) > mips_abi_regsize (gdbarch))
	{
	  if (col > 0)
	    break;		/* End the row before this register.  */

	  /* Print this register on a row by itself.  */
	  mips_print_register (file, frame, regnum);
	  gdb_printf (file, "\n");
	  return regnum + 1;
	}
      if (col == 0)
	gdb_printf (file, "     ");
      gdb_printf (file,
		  mips_abi_regsize (gdbarch) == 8 ? "%17s" : "%9s",
		  gdbarch_register_name (gdbarch, regnum));
      col++;
    }

  if (col == 0)
    return regnum;

  /* Print the R0 to R31 names.  */
  if ((start_regnum % gdbarch_num_regs (gdbarch)) < MIPS_NUMREGS)
    gdb_printf (file, "\n R%-4d",
		start_regnum % gdbarch_num_regs (gdbarch));
  else
    gdb_printf (file, "\n      ");

  /* Now print the values in hex, 4 or 8 to the row.  */
  for (col = 0, regnum = start_regnum;
       col < ncols && regnum < gdbarch_num_cooked_regs (gdbarch);
       regnum++)
    {
      if (*gdbarch_register_name (gdbarch, regnum) == '\0')
	continue;		/* unused register */
      if (mips_float_register_p (gdbarch, regnum))
	break;			/* End row: reached FP register.  */
      if (register_size (gdbarch, regnum) > mips_abi_regsize (gdbarch))
	break;			/* End row: large register.  */

      /* OK: get the data in raw format.  */
      value = get_frame_register_value (frame, regnum);
      if (value->optimized_out ()
	  || !value->entirely_available ())
	{
	  gdb_printf (file, "%*s ",
		      (int) mips_abi_regsize (gdbarch) * 2,
		      (mips_abi_regsize (gdbarch) == 4 ? "<unavl>"
		       : "<unavailable>"));
	  col++;
	  continue;
	}
      raw_buffer = value->contents_all ().data ();
      /* pad small registers */
      for (byte = 0;
	   byte < (mips_abi_regsize (gdbarch)
		   - register_size (gdbarch, regnum)); byte++)
	gdb_printf (file, "  ");
      /* Now print the register value in hex, endian order.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	for (byte =
	     register_size (gdbarch, regnum) - register_size (gdbarch, regnum);
	     byte < register_size (gdbarch, regnum); byte++)
	  gdb_printf (file, "%02x", raw_buffer[byte]);
      else
	for (byte = register_size (gdbarch, regnum) - 1;
	     byte >= 0; byte--)
	  gdb_printf (file, "%02x", raw_buffer[byte]);
      gdb_printf (file, " ");
      col++;
    }
  if (col > 0)			/* ie. if we actually printed anything...  */
    gdb_printf (file, "\n");

  return regnum;
}

/* MIPS_DO_REGISTERS_INFO(): called by "info register" command.  */

static void
mips_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file,
			   frame_info_ptr frame, int regnum, int all)
{
  if (regnum != -1)		/* Do one specified register.  */
    {
      gdb_assert (regnum >= gdbarch_num_regs (gdbarch));
      if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	error (_("Not a valid register for the current processor type"));

      mips_print_register (file, frame, regnum);
      gdb_printf (file, "\n");
    }
  else
    /* Do all (or most) registers.  */
    {
      regnum = gdbarch_num_regs (gdbarch);
      while (regnum < gdbarch_num_cooked_regs (gdbarch))
	{
	  if (mips_float_register_p (gdbarch, regnum))
	    {
	      if (all)		/* True for "INFO ALL-REGISTERS" command.  */
		regnum = print_fp_register_row (file, frame, regnum);
	      else
		regnum += MIPS_NUMREGS;	/* Skip floating point regs.  */
	    }
	  else
	    regnum = print_gp_register_row (file, frame, regnum);
	}
    }
}

static int
mips_single_step_through_delay (struct gdbarch *gdbarch,
				frame_info_ptr frame)
{
  CORE_ADDR pc = get_frame_pc (frame);
  enum mips_isa isa;
  ULONGEST insn;
  int size;

  if ((mips_pc_is_mips (pc)
       && !mips32_insn_at_pc_has_delay_slot (gdbarch, pc))
      || (mips_pc_is_micromips (gdbarch, pc)
	  && !micromips_insn_at_pc_has_delay_slot (gdbarch, pc, 0))
      || (mips_pc_is_mips16 (gdbarch, pc)
	  && !mips16_insn_at_pc_has_delay_slot (gdbarch, pc, 0)))
    return 0;

  isa = mips_pc_isa (gdbarch, pc);
  /* _has_delay_slot above will have validated the read.  */
  insn = mips_fetch_instruction (gdbarch, isa, pc, NULL);
  size = mips_insn_size (isa, insn);

  const address_space *aspace = get_frame_address_space (frame);

  return breakpoint_here_p (aspace, pc + size) != no_breakpoint_here;
}

/* To skip prologues, I use this predicate.  Returns either PC itself
   if the code at PC does not look like a function prologue; otherwise
   returns an address that (if we're lucky) follows the prologue.  If
   LENIENT, then we must skip everything which is involved in setting
   up the frame (it's OK to skip more, just so long as we don't skip
   anything which might clobber the registers which are being saved.
   We must skip more in the case where part of the prologue is in the
   delay slot of a non-prologue instruction).  */

static CORE_ADDR
mips_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR limit_pc;
  CORE_ADDR func_addr;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 100;          /* Magic.  */

  if (mips_pc_is_mips16 (gdbarch, pc))
    return mips16_scan_prologue (gdbarch, pc, limit_pc, NULL, NULL);
  else if (mips_pc_is_micromips (gdbarch, pc))
    return micromips_scan_prologue (gdbarch, pc, limit_pc, NULL, NULL);
  else
    return mips32_scan_prologue (gdbarch, pc, limit_pc, NULL, NULL);
}

/* Implement the stack_frame_destroyed_p gdbarch method (32-bit version).
   This is a helper function for mips_stack_frame_destroyed_p.  */

static int
mips32_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      /* The MIPS epilogue is max. 12 bytes long.  */
      CORE_ADDR addr = func_end - 12;

      if (addr < func_addr + 4)
	addr = func_addr + 4;
      if (pc < addr)
	return 0;

      for (; pc < func_end; pc += MIPS_INSN32_SIZE)
	{
	  unsigned long high_word;
	  unsigned long inst;

	  inst = mips_fetch_instruction (gdbarch, ISA_MIPS, pc, NULL);
	  high_word = (inst >> 16) & 0xffff;

	  if (high_word != 0x27bd	/* addiu $sp,$sp,offset */
	      && high_word != 0x67bd	/* daddiu $sp,$sp,offset */
	      && inst != 0x03e00008	/* jr $ra */
	      && inst != 0x00000000)	/* nop */
	    return 0;
	}

      return 1;
    }

  return 0;
}

/* Implement the stack_frame_destroyed_p gdbarch method (microMIPS version).
   This is a helper function for mips_stack_frame_destroyed_p.  */

static int
micromips_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0;
  CORE_ADDR func_end = 0;
  CORE_ADDR addr;
  ULONGEST insn;
  long offset;
  int dreg;
  int sreg;
  int loc;

  if (!find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    return 0;

  /* The microMIPS epilogue is max. 12 bytes long.  */
  addr = func_end - 12;

  if (addr < func_addr + 2)
    addr = func_addr + 2;
  if (pc < addr)
    return 0;

  for (; pc < func_end; pc += loc)
    {
      loc = 0;
      insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, pc, NULL);
      loc += MIPS_INSN16_SIZE;
      switch (mips_insn_size (ISA_MICROMIPS, insn))
	{
	/* 32-bit instructions.  */
	case 2 * MIPS_INSN16_SIZE:
	  insn <<= 16;
	  insn |= mips_fetch_instruction (gdbarch,
					  ISA_MICROMIPS, pc + loc, NULL);
	  loc += MIPS_INSN16_SIZE;
	  switch (micromips_op (insn >> 16))
	    {
	    case 0xc: /* ADDIU: bits 001100 */
	    case 0x17: /* DADDIU: bits 010111 */
	      sreg = b0s5_reg (insn >> 16);
	      dreg = b5s5_reg (insn >> 16);
	      offset = (b0s16_imm (insn) ^ 0x8000) - 0x8000;
	      if (sreg == MIPS_SP_REGNUM && dreg == MIPS_SP_REGNUM
			    /* (D)ADDIU $sp, imm */
		  && offset >= 0)
		break;
	      return 0;

	    default:
	      return 0;
	    }
	  break;

	/* 16-bit instructions.  */
	case MIPS_INSN16_SIZE:
	  switch (micromips_op (insn))
	    {
	    case 0x3: /* MOVE: bits 000011 */
	      sreg = b0s5_reg (insn);
	      dreg = b5s5_reg (insn);
	      if (sreg == 0 && dreg == 0)
				/* MOVE $zero, $zero aka NOP */
		break;
	      return 0;

	    case 0x11: /* POOL16C: bits 010001 */
	      if (b5s5_op (insn) == 0x18
				/* JRADDIUSP: bits 010011 11000 */
		  || (b5s5_op (insn) == 0xd
				/* JRC: bits 010011 01101 */
		      && b0s5_reg (insn) == MIPS_RA_REGNUM))
				/* JRC $ra */
		break;
	      return 0;

	    case 0x13: /* POOL16D: bits 010011 */
	      offset = micromips_decode_imm9 (b1s9_imm (insn));
	      if ((insn & 0x1) == 0x1
				/* ADDIUSP: bits 010011 1 */
		  && offset > 0)
		break;
	      return 0;

	    default:
	      return 0;
	    }
	}
    }

  return 1;
}

/* Implement the stack_frame_destroyed_p gdbarch method (16-bit version).
   This is a helper function for mips_stack_frame_destroyed_p.  */

static int
mips16_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      /* The MIPS epilogue is max. 12 bytes long.  */
      CORE_ADDR addr = func_end - 12;

      if (addr < func_addr + 4)
	addr = func_addr + 4;
      if (pc < addr)
	return 0;

      for (; pc < func_end; pc += MIPS_INSN16_SIZE)
	{
	  unsigned short inst;

	  inst = mips_fetch_instruction (gdbarch, ISA_MIPS16, pc, NULL);

	  if ((inst & 0xf800) == 0xf000)	/* extend */
	    continue;

	  if (inst != 0x6300		/* addiu $sp,offset */
	      && inst != 0xfb00		/* daddiu $sp,$sp,offset */
	      && inst != 0xe820		/* jr $ra */
	      && inst != 0xe8a0		/* jrc $ra */
	      && inst != 0x6500)	/* nop */
	    return 0;
	}

      return 1;
    }

  return 0;
}

/* Implement the stack_frame_destroyed_p gdbarch method.

   The epilogue is defined here as the area at the end of a function,
   after an instruction which destroys the function's stack frame.  */

static int
mips_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (mips_pc_is_mips16 (gdbarch, pc))
    return mips16_stack_frame_destroyed_p (gdbarch, pc);
  else if (mips_pc_is_micromips (gdbarch, pc))
    return micromips_stack_frame_destroyed_p (gdbarch, pc);
  else
    return mips32_stack_frame_destroyed_p (gdbarch, pc);
}

/* Commands to show/set the MIPS FPU type.  */

static void
show_mipsfpu_command (const char *args, int from_tty)
{
  const char *fpu;

  if (gdbarch_bfd_arch_info (current_inferior ()->arch ())->arch
      != bfd_arch_mips)
    {
      gdb_printf
	("The MIPS floating-point coprocessor is unknown "
	 "because the current architecture is not MIPS.\n");
      return;
    }

  switch (mips_get_fpu_type (current_inferior ()->arch  ()))
    {
    case MIPS_FPU_SINGLE:
      fpu = "single-precision";
      break;
    case MIPS_FPU_DOUBLE:
      fpu = "double-precision";
      break;
    case MIPS_FPU_NONE:
      fpu = "absent (none)";
      break;
    default:
      internal_error (_("bad switch"));
    }
  if (mips_fpu_type_auto)
    gdb_printf ("The MIPS floating-point coprocessor "
		"is set automatically (currently %s)\n",
		fpu);
  else
    gdb_printf
      ("The MIPS floating-point coprocessor is assumed to be %s\n", fpu);
}


static void
set_mipsfpu_single_command (const char *args, int from_tty)
{
  struct gdbarch_info info;
  mips_fpu_type = MIPS_FPU_SINGLE;
  mips_fpu_type_auto = 0;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (_("set mipsfpu failed"));
}

static void
set_mipsfpu_double_command (const char *args, int from_tty)
{
  struct gdbarch_info info;
  mips_fpu_type = MIPS_FPU_DOUBLE;
  mips_fpu_type_auto = 0;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (_("set mipsfpu failed"));
}

static void
set_mipsfpu_none_command (const char *args, int from_tty)
{
  struct gdbarch_info info;
  mips_fpu_type = MIPS_FPU_NONE;
  mips_fpu_type_auto = 0;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (_("set mipsfpu failed"));
}

static void
set_mipsfpu_auto_command (const char *args, int from_tty)
{
  mips_fpu_type_auto = 1;
}

/* Just like reinit_frame_cache, but with the right arguments to be
   callable as an sfunc.  */

static void
reinit_frame_cache_sfunc (const char *args, int from_tty,
			  struct cmd_list_element *c)
{
  reinit_frame_cache ();
}

static int
gdb_print_insn_mips (bfd_vma memaddr, struct disassemble_info *info)
{
  gdb_disassemble_info *di
    = static_cast<gdb_disassemble_info *> (info->application_data);
  struct gdbarch *gdbarch = di->arch ();

  /* FIXME: cagney/2003-06-26: Is this even necessary?  The
     disassembler needs to be able to locally determine the ISA, and
     not rely on GDB.  Otherwize the stand-alone 'objdump -d' will not
     work.  */
  if (mips_pc_is_mips16 (gdbarch, memaddr))
    info->mach = bfd_mach_mips16;
  else if (mips_pc_is_micromips (gdbarch, memaddr))
    info->mach = bfd_mach_mips_micromips;

  /* Round down the instruction address to the appropriate boundary.  */
  memaddr &= (info->mach == bfd_mach_mips16
	      || info->mach == bfd_mach_mips_micromips) ? ~1 : ~3;

  return default_print_insn (memaddr, info);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
mips_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  CORE_ADDR pc = *pcptr;

  if (mips_pc_is_mips16 (gdbarch, pc))
    {
      *pcptr = unmake_compact_addr (pc);
      return MIPS_BP_KIND_MIPS16;
    }
  else if (mips_pc_is_micromips (gdbarch, pc))
    {
      ULONGEST insn;
      int status;

      *pcptr = unmake_compact_addr (pc);
      insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, pc, &status);
      if (status || (mips_insn_size (ISA_MICROMIPS, insn) == 2))
	return MIPS_BP_KIND_MICROMIPS16;
      else
	return MIPS_BP_KIND_MICROMIPS32;
    }
  else
    return MIPS_BP_KIND_MIPS32;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
mips_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);

  switch (kind)
    {
    case MIPS_BP_KIND_MIPS16:
      {
	static gdb_byte mips16_big_breakpoint[] = { 0xe8, 0xa5 };
	static gdb_byte mips16_little_breakpoint[] = { 0xa5, 0xe8 };

	*size = 2;
	if (byte_order_for_code == BFD_ENDIAN_BIG)
	  return mips16_big_breakpoint;
	else
	  return mips16_little_breakpoint;
      }
    case MIPS_BP_KIND_MICROMIPS16:
      {
	static gdb_byte micromips16_big_breakpoint[] = { 0x46, 0x85 };
	static gdb_byte micromips16_little_breakpoint[] = { 0x85, 0x46 };

	*size = 2;

	if (byte_order_for_code == BFD_ENDIAN_BIG)
	  return micromips16_big_breakpoint;
	else
	  return micromips16_little_breakpoint;
      }
    case MIPS_BP_KIND_MICROMIPS32:
      {
	static gdb_byte micromips32_big_breakpoint[] = { 0, 0x5, 0, 0x7 };
	static gdb_byte micromips32_little_breakpoint[] = { 0x5, 0, 0x7, 0 };

	*size = 4;
	if (byte_order_for_code == BFD_ENDIAN_BIG)
	  return micromips32_big_breakpoint;
	else
	  return micromips32_little_breakpoint;
      }
    case MIPS_BP_KIND_MIPS32:
      {
	static gdb_byte big_breakpoint[] = { 0, 0x5, 0, 0xd };
	static gdb_byte little_breakpoint[] = { 0xd, 0, 0x5, 0 };

	*size = 4;
	if (byte_order_for_code == BFD_ENDIAN_BIG)
	  return big_breakpoint;
	else
	  return little_breakpoint;
      }
    default:
      gdb_assert_not_reached ("unexpected mips breakpoint kind");
    };
}

/* Return non-zero if the standard MIPS instruction INST has a branch
   delay slot (i.e. it is a jump or branch instruction).  This function
   is based on mips32_next_pc.  */

static int
mips32_instruction_has_delay_slot (struct gdbarch *gdbarch, ULONGEST inst)
{
  int op;
  int rs;
  int rt;

  op = itype_op (inst);
  if ((inst & 0xe0000000) != 0)
    {
      rs = itype_rs (inst);
      rt = itype_rt (inst);
      return (is_octeon_bbit_op (op, gdbarch) 
	      || op >> 2 == 5	/* BEQL, BNEL, BLEZL, BGTZL: bits 0101xx  */
	      || op == 29	/* JALX: bits 011101  */
	      || (op == 17
		  && (rs == 8
				/* BC1F, BC1FL, BC1T, BC1TL: 010001 01000  */
		      || (rs == 9 && (rt & 0x2) == 0)
				/* BC1ANY2F, BC1ANY2T: bits 010001 01001  */
		      || (rs == 10 && (rt & 0x2) == 0))));
				/* BC1ANY4F, BC1ANY4T: bits 010001 01010  */
    }
  else
    switch (op & 0x07)		/* extract bits 28,27,26  */
      {
      case 0:			/* SPECIAL  */
	op = rtype_funct (inst);
	return (op == 8		/* JR  */
		|| op == 9);	/* JALR  */
	break;			/* end SPECIAL  */
      case 1:			/* REGIMM  */
	rs = itype_rs (inst);
	rt = itype_rt (inst);	/* branch condition  */
	return ((rt & 0xc) == 0
				/* BLTZ, BLTZL, BGEZ, BGEZL: bits 000xx  */
				/* BLTZAL, BLTZALL, BGEZAL, BGEZALL: 100xx  */
		|| ((rt & 0x1e) == 0x1c && rs == 0));
				/* BPOSGE32, BPOSGE64: bits 1110x  */
	break;			/* end REGIMM  */
      default:			/* J, JAL, BEQ, BNE, BLEZ, BGTZ  */
	return 1;
	break;
      }
}

/* Return non-zero if a standard MIPS instruction at ADDR has a branch
   delay slot (i.e. it is a jump or branch instruction).  */

static int
mips32_insn_at_pc_has_delay_slot (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  ULONGEST insn;
  int status;

  insn = mips_fetch_instruction (gdbarch, ISA_MIPS, addr, &status);
  if (status)
    return 0;

  return mips32_instruction_has_delay_slot (gdbarch, insn);
}

/* Return non-zero if the microMIPS instruction INSN, comprising the
   16-bit major opcode word in the high 16 bits and any second word
   in the low 16 bits, has a branch delay slot (i.e. it is a non-compact
   jump or branch instruction).  The instruction must be 32-bit if
   MUSTBE32 is set or can be any instruction otherwise.  */

static int
micromips_instruction_has_delay_slot (ULONGEST insn, int mustbe32)
{
  ULONGEST major = insn >> 16;

  switch (micromips_op (major))
    {
    /* 16-bit instructions.  */
    case 0x33:			/* B16: bits 110011 */
    case 0x2b:			/* BNEZ16: bits 101011 */
    case 0x23:			/* BEQZ16: bits 100011 */
      return !mustbe32;
    case 0x11:			/* POOL16C: bits 010001 */
      return (!mustbe32
	      && ((b5s5_op (major) == 0xc
				/* JR16: bits 010001 01100 */
		  || (b5s5_op (major) & 0x1e) == 0xe)));
				/* JALR16, JALRS16: bits 010001 0111x */
    /* 32-bit instructions.  */
    case 0x3d:			/* JAL: bits 111101 */
    case 0x3c:			/* JALX: bits 111100 */
    case 0x35:			/* J: bits 110101 */
    case 0x2d:			/* BNE: bits 101101 */
    case 0x25:			/* BEQ: bits 100101 */
    case 0x1d:			/* JALS: bits 011101 */
      return 1;
    case 0x10:			/* POOL32I: bits 010000 */
      return ((b5s5_op (major) & 0x1c) == 0x0
				/* BLTZ, BLTZAL, BGEZ, BGEZAL: 010000 000xx */
	      || (b5s5_op (major) & 0x1d) == 0x4
				/* BLEZ, BGTZ: bits 010000 001x0 */
	      || (b5s5_op (major) & 0x1d) == 0x11
				/* BLTZALS, BGEZALS: bits 010000 100x1 */
	      || ((b5s5_op (major) & 0x1e) == 0x14
		  && (major & 0x3) == 0x0)
				/* BC2F, BC2T: bits 010000 1010x xxx00 */
	      || (b5s5_op (major) & 0x1e) == 0x1a
				/* BPOSGE64, BPOSGE32: bits 010000 1101x */
	      || ((b5s5_op (major) & 0x1e) == 0x1c
		  && (major & 0x3) == 0x0)
				/* BC1F, BC1T: bits 010000 1110x xxx00 */
	      || ((b5s5_op (major) & 0x1c) == 0x1c
		  && (major & 0x3) == 0x1));
				/* BC1ANY*: bits 010000 111xx xxx01 */
    case 0x0:			/* POOL32A: bits 000000 */
      return (b0s6_op (insn) == 0x3c
				/* POOL32Axf: bits 000000 ... 111100 */
	      && (b6s10_ext (insn) & 0x2bf) == 0x3c);
				/* JALR, JALR.HB: 000000 000x111100 111100 */
				/* JALRS, JALRS.HB: 000000 010x111100 111100 */
    default:
      return 0;
    }
}

/* Return non-zero if a microMIPS instruction at ADDR has a branch delay
   slot (i.e. it is a non-compact jump instruction).  The instruction
   must be 32-bit if MUSTBE32 is set or can be any instruction otherwise.  */

static int
micromips_insn_at_pc_has_delay_slot (struct gdbarch *gdbarch,
				     CORE_ADDR addr, int mustbe32)
{
  ULONGEST insn;
  int status;
  int size;

  insn = mips_fetch_instruction (gdbarch, ISA_MICROMIPS, addr, &status);
  if (status)
    return 0;
  size = mips_insn_size (ISA_MICROMIPS, insn);
  insn <<= 16;
  if (size == 2 * MIPS_INSN16_SIZE)
    {
      insn |= mips_fetch_instruction (gdbarch, ISA_MICROMIPS, addr, &status);
      if (status)
	return 0;
    }

  return micromips_instruction_has_delay_slot (insn, mustbe32);
}

/* Return non-zero if the MIPS16 instruction INST, which must be
   a 32-bit instruction if MUSTBE32 is set or can be any instruction
   otherwise, has a branch delay slot (i.e. it is a non-compact jump
   instruction).  This function is based on mips16_next_pc.  */

static int
mips16_instruction_has_delay_slot (unsigned short inst, int mustbe32)
{
  if ((inst & 0xf89f) == 0xe800)	/* JR/JALR (16-bit instruction)  */
    return !mustbe32;
  return (inst & 0xf800) == 0x1800;	/* JAL/JALX (32-bit instruction)  */
}

/* Return non-zero if a MIPS16 instruction at ADDR has a branch delay
   slot (i.e. it is a non-compact jump instruction).  The instruction
   must be 32-bit if MUSTBE32 is set or can be any instruction otherwise.  */

static int
mips16_insn_at_pc_has_delay_slot (struct gdbarch *gdbarch,
				  CORE_ADDR addr, int mustbe32)
{
  unsigned short insn;
  int status;

  insn = mips_fetch_instruction (gdbarch, ISA_MIPS16, addr, &status);
  if (status)
    return 0;

  return mips16_instruction_has_delay_slot (insn, mustbe32);
}

/* Calculate the starting address of the MIPS memory segment BPADDR is in.
   This assumes KSSEG exists.  */

static CORE_ADDR
mips_segment_boundary (CORE_ADDR bpaddr)
{
  CORE_ADDR mask = CORE_ADDR_MAX;
  int segsize;

  if (sizeof (CORE_ADDR) == 8)
    /* Get the topmost two bits of bpaddr in a 32-bit safe manner (avoid
       a compiler warning produced where CORE_ADDR is a 32-bit type even
       though in that case this is dead code).  */
    switch (bpaddr >> ((sizeof (CORE_ADDR) << 3) - 2) & 3)
      {
      case 3:
	if (bpaddr == (bfd_signed_vma) (int32_t) bpaddr)
	  segsize = 29;			/* 32-bit compatibility segment  */
	else
	  segsize = 62;			/* xkseg  */
	break;
      case 2:				/* xkphys  */
	segsize = 59;
	break;
      default:				/* xksseg (1), xkuseg/kuseg (0)  */
	segsize = 62;
	break;
      }
  else if (bpaddr & 0x80000000)		/* kernel segment  */
    segsize = 29;
  else
    segsize = 31;			/* user segment  */
  mask <<= segsize;
  return bpaddr & mask;
}

/* Move the breakpoint at BPADDR out of any branch delay slot by shifting
   it backwards if necessary.  Return the address of the new location.  */

static CORE_ADDR
mips_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr)
{
  CORE_ADDR prev_addr;
  CORE_ADDR boundary;
  CORE_ADDR func_addr;

  /* If a breakpoint is set on the instruction in a branch delay slot,
     GDB gets confused.  When the breakpoint is hit, the PC isn't on
     the instruction in the branch delay slot, the PC will point to
     the branch instruction.  Since the PC doesn't match any known
     breakpoints, GDB reports a trap exception.

     There are two possible fixes for this problem.

     1) When the breakpoint gets hit, see if the BD bit is set in the
     Cause register (which indicates the last exception occurred in a
     branch delay slot).  If the BD bit is set, fix the PC to point to
     the instruction in the branch delay slot.

     2) When the user sets the breakpoint, don't allow him to set the
     breakpoint on the instruction in the branch delay slot.  Instead
     move the breakpoint to the branch instruction (which will have
     the same result).

     The problem with the first solution is that if the user then
     single-steps the processor, the branch instruction will get
     skipped (since GDB thinks the PC is on the instruction in the
     branch delay slot).

     So, we'll use the second solution.  To do this we need to know if
     the instruction we're trying to set the breakpoint on is in the
     branch delay slot.  */

  boundary = mips_segment_boundary (bpaddr);

  /* Make sure we don't scan back before the beginning of the current
     function, since we may fetch constant data or insns that look like
     a jump.  Of course we might do that anyway if the compiler has
     moved constants inline. :-(  */
  if (find_pc_partial_function (bpaddr, NULL, &func_addr, NULL)
      && func_addr > boundary && func_addr <= bpaddr)
    boundary = func_addr;

  if (mips_pc_is_mips (bpaddr))
    {
      if (bpaddr == boundary)
	return bpaddr;

      /* If the previous instruction has a branch delay slot, we have
	 to move the breakpoint to the branch instruction. */
      prev_addr = bpaddr - 4;
      if (mips32_insn_at_pc_has_delay_slot (gdbarch, prev_addr))
	bpaddr = prev_addr;
    }
  else
    {
      int (*insn_at_pc_has_delay_slot) (struct gdbarch *, CORE_ADDR, int);
      CORE_ADDR addr, jmpaddr;
      int i;

      boundary = unmake_compact_addr (boundary);

      /* The only MIPS16 instructions with delay slots are JAL, JALX,
	 JALR and JR.  An absolute JAL/JALX is always 4 bytes long,
	 so try for that first, then try the 2 byte JALR/JR.
	 The microMIPS ASE has a whole range of jumps and branches
	 with delay slots, some of which take 4 bytes and some take
	 2 bytes, so the idea is the same.
	 FIXME: We have to assume that bpaddr is not the second half
	 of an extended instruction.  */
      insn_at_pc_has_delay_slot = (mips_pc_is_micromips (gdbarch, bpaddr)
				   ? micromips_insn_at_pc_has_delay_slot
				   : mips16_insn_at_pc_has_delay_slot);

      jmpaddr = 0;
      addr = bpaddr;
      for (i = 1; i < 4; i++)
	{
	  if (unmake_compact_addr (addr) == boundary)
	    break;
	  addr -= MIPS_INSN16_SIZE;
	  if (i == 1 && insn_at_pc_has_delay_slot (gdbarch, addr, 0))
	    /* Looks like a JR/JALR at [target-1], but it could be
	       the second word of a previous JAL/JALX, so record it
	       and check back one more.  */
	    jmpaddr = addr;
	  else if (i > 1 && insn_at_pc_has_delay_slot (gdbarch, addr, 1))
	    {
	      if (i == 2)
		/* Looks like a JAL/JALX at [target-2], but it could also
		   be the second word of a previous JAL/JALX, record it,
		   and check back one more.  */
		jmpaddr = addr;
	      else
		/* Looks like a JAL/JALX at [target-3], so any previously
		   recorded JAL/JALX or JR/JALR must be wrong, because:

		   >-3: JAL
		    -2: JAL-ext (can't be JAL/JALX)
		    -1: bdslot (can't be JR/JALR)
		     0: target insn

		   Of course it could be another JAL-ext which looks
		   like a JAL, but in that case we'd have broken out
		   of this loop at [target-2]:

		    -4: JAL
		   >-3: JAL-ext
		    -2: bdslot (can't be jmp)
		    -1: JR/JALR
		     0: target insn  */
		jmpaddr = 0;
	    }
	  else
	    {
	      /* Not a jump instruction: if we're at [target-1] this
		 could be the second word of a JAL/JALX, so continue;
		 otherwise we're done.  */
	      if (i > 1)
		break;
	    }
	}

      if (jmpaddr)
	bpaddr = jmpaddr;
    }

  return bpaddr;
}

/* Return non-zero if SUFFIX is one of the numeric suffixes used for MIPS16
   call stubs, one of 1, 2, 5, 6, 9, 10, or, if ZERO is non-zero, also 0.  */

static int
mips_is_stub_suffix (const char *suffix, int zero)
{
  switch (suffix[0])
   {
   case '0':
     return zero && suffix[1] == '\0';
   case '1':
     return suffix[1] == '\0' || (suffix[1] == '0' && suffix[2] == '\0');
   case '2':
   case '5':
   case '6':
   case '9':
     return suffix[1] == '\0';
   default:
     return 0;
   }
}

/* Return non-zero if MODE is one of the mode infixes used for MIPS16
   call stubs, one of sf, df, sc, or dc.  */

static int
mips_is_stub_mode (const char *mode)
{
  return ((mode[0] == 's' || mode[0] == 'd')
	  && (mode[1] == 'f' || mode[1] == 'c'));
}

/* Code at PC is a compiler-generated stub.  Such a stub for a function
   bar might have a name like __fn_stub_bar, and might look like this:

      mfc1    $4, $f13
      mfc1    $5, $f12
      mfc1    $6, $f15
      mfc1    $7, $f14

   followed by (or interspersed with):

      j       bar

   or:

      lui     $25, %hi(bar)
      addiu   $25, $25, %lo(bar)
      jr      $25

   ($1 may be used in old code; for robustness we accept any register)
   or, in PIC code:

      lui     $28, %hi(_gp_disp)
      addiu   $28, $28, %lo(_gp_disp)
      addu    $28, $28, $25
      lw      $25, %got(bar)
      addiu   $25, $25, %lo(bar)
      jr      $25

   In the case of a __call_stub_bar stub, the sequence to set up
   arguments might look like this:

      mtc1    $4, $f13
      mtc1    $5, $f12
      mtc1    $6, $f15
      mtc1    $7, $f14

   followed by (or interspersed with) one of the jump sequences above.

   In the case of a __call_stub_fp_bar stub, JAL or JALR is used instead
   of J or JR, respectively, followed by:

      mfc1    $2, $f0
      mfc1    $3, $f1
      jr      $18

   We are at the beginning of the stub here, and scan down and extract
   the target address from the jump immediate instruction or, if a jump
   register instruction is used, from the register referred.  Return
   the value of PC calculated or 0 if inconclusive.

   The limit on the search is arbitrarily set to 20 instructions.  FIXME.  */

static CORE_ADDR
mips_get_mips16_fn_stub_pc (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int addrreg = MIPS_ZERO_REGNUM;
  CORE_ADDR start_pc = pc;
  CORE_ADDR target_pc = 0;
  CORE_ADDR addr = 0;
  CORE_ADDR gp = 0;
  int status = 0;
  int i;

  for (i = 0;
       status == 0 && target_pc == 0 && i < 20;
       i++, pc += MIPS_INSN32_SIZE)
    {
      ULONGEST inst = mips_fetch_instruction (gdbarch, ISA_MIPS, pc, NULL);
      CORE_ADDR imm;
      int rt;
      int rs;
      int rd;

      switch (itype_op (inst))
	{
	case 0:		/* SPECIAL */
	  switch (rtype_funct (inst))
	    {
	    case 8:		/* JR */
	    case 9:		/* JALR */
	      rs = rtype_rs (inst);
	      if (rs == MIPS_GP_REGNUM)
		target_pc = gp;				/* Hmm...  */
	      else if (rs == addrreg)
		target_pc = addr;
	      break;

	    case 0x21:		/* ADDU */
	      rt = rtype_rt (inst);
	      rs = rtype_rs (inst);
	      rd = rtype_rd (inst);
	      if (rd == MIPS_GP_REGNUM
		  && ((rs == MIPS_GP_REGNUM && rt == MIPS_T9_REGNUM)
		      || (rs == MIPS_T9_REGNUM && rt == MIPS_GP_REGNUM)))
		gp += start_pc;
	      break;
	    }
	  break;

	case 2:		/* J */
	case 3:		/* JAL */
	  target_pc = jtype_target (inst) << 2;
	  target_pc += ((pc + 4) & ~(CORE_ADDR) 0x0fffffff);
	  break;

	case 9:		/* ADDIU */
	  rt = itype_rt (inst);
	  rs = itype_rs (inst);
	  if (rt == rs)
	    {
	      imm = (itype_immediate (inst) ^ 0x8000) - 0x8000;
	      if (rt == MIPS_GP_REGNUM)
		gp += imm;
	      else if (rt == addrreg)
		addr += imm;
	    }
	  break;

	case 0xf:	/* LUI */
	  rt = itype_rt (inst);
	  imm = ((itype_immediate (inst) ^ 0x8000) - 0x8000) << 16;
	  if (rt == MIPS_GP_REGNUM)
	    gp = imm;
	  else if (rt != MIPS_ZERO_REGNUM)
	    {
	      addrreg = rt;
	      addr = imm;
	    }
	  break;

	case 0x23:	/* LW */
	  rt = itype_rt (inst);
	  rs = itype_rs (inst);
	  imm = (itype_immediate (inst) ^ 0x8000) - 0x8000;
	  if (gp != 0 && rs == MIPS_GP_REGNUM)
	    {
	      gdb_byte buf[4];

	      memset (buf, 0, sizeof (buf));
	      status = target_read_memory (gp + imm, buf, sizeof (buf));
	      addrreg = rt;
	      addr = extract_signed_integer (buf, sizeof (buf), byte_order);
	    }
	  break;
	}
    }

  return target_pc;
}

/* If PC is in a MIPS16 call or return stub, return the address of the
   target PC, which is either the callee or the caller.  There are several
   cases which must be handled:

   * If the PC is in __mips16_ret_{d,s}{f,c}, this is a return stub
     and the target PC is in $31 ($ra).
   * If the PC is in __mips16_call_stub_{1..10}, this is a call stub
     and the target PC is in $2.
   * If the PC at the start of __mips16_call_stub_{s,d}{f,c}_{0..10},
     i.e. before the JALR instruction, this is effectively a call stub
     and the target PC is in $2.  Otherwise this is effectively
     a return stub and the target PC is in $18.
   * If the PC is at the start of __call_stub_fp_*, i.e. before the
     JAL or JALR instruction, this is effectively a call stub and the
     target PC is buried in the instruction stream.  Otherwise this
     is effectively a return stub and the target PC is in $18.
   * If the PC is in __call_stub_* or in __fn_stub_*, this is a call
     stub and the target PC is buried in the instruction stream.

   See the source code for the stubs in gcc/config/mips/mips16.S, or the
   stub builder in gcc/config/mips/mips.c (mips16_build_call_stub) for the
   gory details.  */

static CORE_ADDR
mips_skip_mips16_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  CORE_ADDR start_addr;
  const char *name;
  size_t prefixlen;

  /* Find the starting address and name of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_ret_{d,s}{f,c}, this is a return stub
     and the target PC is in $31 ($ra).  */
  prefixlen = strlen (mips_str_mips16_ret_stub);
  if (strncmp (name, mips_str_mips16_ret_stub, prefixlen) == 0
      && mips_is_stub_mode (name + prefixlen)
      && name[prefixlen + 2] == '\0')
    return get_frame_register_signed
	     (frame, gdbarch_num_regs (gdbarch) + MIPS_RA_REGNUM);

  /* If the PC is in __mips16_call_stub_*, this is one of the call
     call/return stubs.  */
  prefixlen = strlen (mips_str_mips16_call_stub);
  if (strncmp (name, mips_str_mips16_call_stub, prefixlen) == 0)
    {
      /* If the PC is in __mips16_call_stub_{1..10}, this is a call stub
	 and the target PC is in $2.  */
      if (mips_is_stub_suffix (name + prefixlen, 0))
	return get_frame_register_signed
		 (frame, gdbarch_num_regs (gdbarch) + MIPS_V0_REGNUM);

      /* If the PC at the start of __mips16_call_stub_{s,d}{f,c}_{0..10},
	 i.e. before the JALR instruction, this is effectively a call stub
	 and the target PC is in $2.  Otherwise this is effectively
	 a return stub and the target PC is in $18.  */
      else if (mips_is_stub_mode (name + prefixlen)
	       && name[prefixlen + 2] == '_'
	       && mips_is_stub_suffix (name + prefixlen + 3, 0))
	{
	  if (pc == start_addr)
	    /* This is the 'call' part of a call stub.  The return
	       address is in $2.  */
	    return get_frame_register_signed
		     (frame, gdbarch_num_regs (gdbarch) + MIPS_V0_REGNUM);
	  else
	    /* This is the 'return' part of a call stub.  The return
	       address is in $18.  */
	    return get_frame_register_signed
		     (frame, gdbarch_num_regs (gdbarch) + MIPS_S2_REGNUM);
	}
      else
	return 0;		/* Not a stub.  */
    }

  /* If the PC is in __call_stub_* or __fn_stub*, this is one of the
     compiler-generated call or call/return stubs.  */
  if (startswith (name, mips_str_fn_stub)
      || startswith (name, mips_str_call_stub))
    {
      if (pc == start_addr)
	/* This is the 'call' part of a call stub.  Call this helper
	   to scan through this code for interesting instructions
	   and determine the final PC.  */
	return mips_get_mips16_fn_stub_pc (frame, pc);
      else
	/* This is the 'return' part of a call stub.  The return address
	   is in $18.  */
	return get_frame_register_signed
		 (frame, gdbarch_num_regs (gdbarch) + MIPS_S2_REGNUM);
    }

  return 0;			/* Not a stub.  */
}

/* Return non-zero if the PC is inside a return thunk (aka stub or trampoline).
   This implements the IN_SOLIB_RETURN_TRAMPOLINE macro.  */

static int
mips_in_return_stub (struct gdbarch *gdbarch, CORE_ADDR pc, const char *name)
{
  CORE_ADDR start_addr;
  size_t prefixlen;

  /* Find the starting address of the function containing the PC.  */
  if (find_pc_partial_function (pc, NULL, &start_addr, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_call_stub_{s,d}{f,c}_{0..10} but not at
     the start, i.e. after the JALR instruction, this is effectively
     a return stub.  */
  prefixlen = strlen (mips_str_mips16_call_stub);
  if (pc != start_addr
      && strncmp (name, mips_str_mips16_call_stub, prefixlen) == 0
      && mips_is_stub_mode (name + prefixlen)
      && name[prefixlen + 2] == '_'
      && mips_is_stub_suffix (name + prefixlen + 3, 1))
    return 1;

  /* If the PC is in __call_stub_fp_* but not at the start, i.e. after
     the JAL or JALR instruction, this is effectively a return stub.  */
  prefixlen = strlen (mips_str_call_fp_stub);
  if (pc != start_addr
      && strncmp (name, mips_str_call_fp_stub, prefixlen) == 0)
    return 1;

  /* Consume the .pic. prefix of any PIC stub, this function must return
     true when the PC is in a PIC stub of a __mips16_ret_{d,s}{f,c} stub
     or the call stub path will trigger in handle_inferior_event causing
     it to go astray.  */
  prefixlen = strlen (mips_str_pic);
  if (strncmp (name, mips_str_pic, prefixlen) == 0)
    name += prefixlen;

  /* If the PC is in __mips16_ret_{d,s}{f,c}, this is a return stub.  */
  prefixlen = strlen (mips_str_mips16_ret_stub);
  if (strncmp (name, mips_str_mips16_ret_stub, prefixlen) == 0
      && mips_is_stub_mode (name + prefixlen)
      && name[prefixlen + 2] == '\0')
    return 1;

  return 0;			/* Not a stub.  */
}

/* If the current PC is the start of a non-PIC-to-PIC stub, return the
   PC of the stub target.  The stub just loads $t9 and jumps to it,
   so that $t9 has the correct value at function entry.  */

static CORE_ADDR
mips_skip_pic_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct bound_minimal_symbol msym;
  int i;
  gdb_byte stub_code[16];
  int32_t stub_words[4];

  /* The stub for foo is named ".pic.foo", and is either two
     instructions inserted before foo or a three instruction sequence
     which jumps to foo.  */
  msym = lookup_minimal_symbol_by_pc (pc);
  if (msym.minsym == NULL
      || msym.value_address () != pc
      || msym.minsym->linkage_name () == NULL
      || !startswith (msym.minsym->linkage_name (), ".pic."))
    return 0;

  /* A two-instruction header.  */
  if (msym.minsym->size () == 8)
    return pc + 8;

  /* A three-instruction (plus delay slot) trampoline.  */
  if (msym.minsym->size () == 16)
    {
      if (target_read_memory (pc, stub_code, 16) != 0)
	return 0;
      for (i = 0; i < 4; i++)
	stub_words[i] = extract_unsigned_integer (stub_code + i * 4,
						  4, byte_order);

      /* A stub contains these instructions:
	 lui	t9, %hi(target)
	 j	target
	  addiu	t9, t9, %lo(target)
	 nop

	 This works even for N64, since stubs are only generated with
	 -msym32.  */
      if ((stub_words[0] & 0xffff0000U) == 0x3c190000
	  && (stub_words[1] & 0xfc000000U) == 0x08000000
	  && (stub_words[2] & 0xffff0000U) == 0x27390000
	  && stub_words[3] == 0x00000000)
	return ((((stub_words[0] & 0x0000ffff) << 16)
		 + (stub_words[2] & 0x0000ffff)) ^ 0x8000) - 0x8000;
    }

  /* Not a recognized stub.  */
  return 0;
}

static CORE_ADDR
mips_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  CORE_ADDR requested_pc = pc;
  CORE_ADDR target_pc;
  CORE_ADDR new_pc;

  do
    {
      target_pc = pc;

      new_pc = mips_skip_mips16_trampoline_code (frame, pc);
      if (new_pc)
	pc = new_pc;

      new_pc = find_solib_trampoline_target (frame, pc);
      if (new_pc)
	pc = new_pc;

      new_pc = mips_skip_pic_trampoline_code (frame, pc);
      if (new_pc)
	pc = new_pc;
    }
  while (pc != target_pc);

  return pc != requested_pc ? pc : 0;
}

/* Convert a dbx stab register number (from `r' declaration) to a GDB
   [1 * gdbarch_num_regs .. 2 * gdbarch_num_regs) REGNUM.  */

static int
mips_stab_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  int regnum;
  if (num >= 0 && num < 32)
    regnum = num;
  else if (num >= 38 && num < 70)
    regnum = num + mips_regnum (gdbarch)->fp0 - 38;
  else if (num == 70)
    regnum = mips_regnum (gdbarch)->hi;
  else if (num == 71)
    regnum = mips_regnum (gdbarch)->lo;
  else if (mips_regnum (gdbarch)->dspacc != -1 && num >= 72 && num < 78)
    regnum = num + mips_regnum (gdbarch)->dspacc - 72;
  else
    return -1;
  return gdbarch_num_regs (gdbarch) + regnum;
}


/* Convert a dwarf, dwarf2, or ecoff register number to a GDB [1 *
   gdbarch_num_regs .. 2 * gdbarch_num_regs) REGNUM.  */

static int
mips_dwarf_dwarf2_ecoff_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  int regnum;
  if (num >= 0 && num < 32)
    regnum = num;
  else if (num >= 32 && num < 64)
    regnum = num + mips_regnum (gdbarch)->fp0 - 32;
  else if (num == 64)
    regnum = mips_regnum (gdbarch)->hi;
  else if (num == 65)
    regnum = mips_regnum (gdbarch)->lo;
  else if (mips_regnum (gdbarch)->dspacc != -1 && num >= 66 && num < 72)
    regnum = num + mips_regnum (gdbarch)->dspacc - 66;
  else
    return -1;
  return gdbarch_num_regs (gdbarch) + regnum;
}

static int
mips_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  /* Only makes sense to supply raw registers.  */
  gdb_assert (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch));
  /* FIXME: cagney/2002-05-13: Need to look at the pseudo register to
     decide if it is valid.  Should instead define a standard sim/gdb
     register numbering scheme.  */
  if (gdbarch_register_name (gdbarch,
			     gdbarch_num_regs (gdbarch) + regnum)[0] != '\0')
    return regnum;
  else
    return LEGACY_SIM_REGNO_IGNORE;
}


/* Convert an integer into an address.  Extracting the value signed
   guarantees a correctly sign extended address.  */

static CORE_ADDR
mips_integer_to_address (struct gdbarch *gdbarch,
			 struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  return extract_signed_integer (buf, type->length (), byte_order);
}

/* Dummy virtual frame pointer method.  This is no more or less accurate
   than most other architectures; we just need to be explicit about it,
   because the pseudo-register gdbarch_sp_regnum will otherwise lead to
   an assertion failure.  */

static void
mips_virtual_frame_pointer (struct gdbarch *gdbarch, 
			    CORE_ADDR pc, int *reg, LONGEST *offset)
{
  *reg = MIPS_SP_REGNUM;
  *offset = 0;
}

static void
mips_find_abi_section (bfd *abfd, asection *sect, void *obj)
{
  enum mips_abi *abip = (enum mips_abi *) obj;
  const char *name = bfd_section_name (sect);

  if (*abip != MIPS_ABI_UNKNOWN)
    return;

  if (!startswith (name, ".mdebug."))
    return;

  if (strcmp (name, ".mdebug.abi32") == 0)
    *abip = MIPS_ABI_O32;
  else if (strcmp (name, ".mdebug.abiN32") == 0)
    *abip = MIPS_ABI_N32;
  else if (strcmp (name, ".mdebug.abi64") == 0)
    *abip = MIPS_ABI_N64;
  else if (strcmp (name, ".mdebug.abiO64") == 0)
    *abip = MIPS_ABI_O64;
  else if (strcmp (name, ".mdebug.eabi32") == 0)
    *abip = MIPS_ABI_EABI32;
  else if (strcmp (name, ".mdebug.eabi64") == 0)
    *abip = MIPS_ABI_EABI64;
  else
    warning (_("unsupported ABI %s."), name + 8);
}

static void
mips_find_long_section (bfd *abfd, asection *sect, void *obj)
{
  int *lbp = (int *) obj;
  const char *name = bfd_section_name (sect);

  if (startswith (name, ".gcc_compiled_long32"))
    *lbp = 32;
  else if (startswith (name, ".gcc_compiled_long64"))
    *lbp = 64;
  else if (startswith (name, ".gcc_compiled_long"))
    warning (_("unrecognized .gcc_compiled_longXX"));
}

static enum mips_abi
global_mips_abi (void)
{
  int i;

  for (i = 0; mips_abi_strings[i] != NULL; i++)
    if (mips_abi_strings[i] == mips_abi_string)
      return (enum mips_abi) i;

  internal_error (_("unknown ABI string"));
}

/* Return the default compressed instruction set, either of MIPS16
   or microMIPS, selected when none could have been determined from
   the ELF header of the binary being executed (or no binary has been
   selected.  */

static enum mips_isa
global_mips_compression (void)
{
  int i;

  for (i = 0; mips_compression_strings[i] != NULL; i++)
    if (mips_compression_strings[i] == mips_compression_string)
      return (enum mips_isa) i;

  internal_error (_("unknown compressed ISA string"));
}

static void
mips_register_g_packet_guesses (struct gdbarch *gdbarch)
{
  /* If the size matches the set of 32-bit or 64-bit integer registers,
     assume that's what we've got.  */
  register_remote_g_packet_guess (gdbarch, 38 * 4, mips_tdesc_gp32);
  register_remote_g_packet_guess (gdbarch, 38 * 8, mips_tdesc_gp64);

  /* If the size matches the full set of registers GDB traditionally
     knows about, including floating point, for either 32-bit or
     64-bit, assume that's what we've got.  */
  register_remote_g_packet_guess (gdbarch, 90 * 4, mips_tdesc_gp32);
  register_remote_g_packet_guess (gdbarch, 90 * 8, mips_tdesc_gp64);

  /* Otherwise we don't have a useful guess.  */
}

static struct value *
value_of_mips_user_reg (frame_info_ptr frame, const void *baton)
{
  const int *reg_p = (const int *) baton;
  return value_of_register (*reg_p, get_next_frame_sentinel_okay (frame));
}

static struct gdbarch *
mips_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int elf_flags;
  enum mips_abi mips_abi, found_abi, wanted_abi;
  int i, num_regs;
  enum mips_fpu_type fpu_type;
  tdesc_arch_data_up tdesc_data;
  int elf_fpu_type = Val_GNU_MIPS_ABI_FP_ANY;
  const char * const *reg_names;
  struct mips_regnum mips_regnum, *regnum;
  enum mips_isa mips_isa;
  int dspacc;
  int dspctl;

  /* First of all, extract the elf_flags, if available.  */
  if (info.abfd && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;
  else if (arches != NULL)
    {
      mips_gdbarch_tdep *tdep
	= gdbarch_tdep<mips_gdbarch_tdep> (arches->gdbarch);
      elf_flags = tdep->elf_flags;
    }
  else
    elf_flags = 0;
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog,
		"mips_gdbarch_init: elf_flags = 0x%08x\n", elf_flags);

  /* Check ELF_FLAGS to see if it specifies the ABI being used.  */
  switch ((elf_flags & EF_MIPS_ABI))
    {
    case EF_MIPS_ABI_O32:
      found_abi = MIPS_ABI_O32;
      break;
    case EF_MIPS_ABI_O64:
      found_abi = MIPS_ABI_O64;
      break;
    case EF_MIPS_ABI_EABI32:
      found_abi = MIPS_ABI_EABI32;
      break;
    case EF_MIPS_ABI_EABI64:
      found_abi = MIPS_ABI_EABI64;
      break;
    default:
      if ((elf_flags & EF_MIPS_ABI2))
	found_abi = MIPS_ABI_N32;
      else
	found_abi = MIPS_ABI_UNKNOWN;
      break;
    }

  /* GCC creates a pseudo-section whose name describes the ABI.  */
  if (found_abi == MIPS_ABI_UNKNOWN && info.abfd != NULL)
    bfd_map_over_sections (info.abfd, mips_find_abi_section, &found_abi);

  /* If we have no useful BFD information, use the ABI from the last
     MIPS architecture (if there is one).  */
  if (found_abi == MIPS_ABI_UNKNOWN && info.abfd == NULL && arches != NULL)
    {
      mips_gdbarch_tdep *tdep
	= gdbarch_tdep<mips_gdbarch_tdep> (arches->gdbarch);
      found_abi = tdep->found_abi;
    }

  /* Try the architecture for any hint of the correct ABI.  */
  if (found_abi == MIPS_ABI_UNKNOWN
      && info.bfd_arch_info != NULL
      && info.bfd_arch_info->arch == bfd_arch_mips)
    {
      switch (info.bfd_arch_info->mach)
	{
	case bfd_mach_mips3900:
	  found_abi = MIPS_ABI_EABI32;
	  break;
	case bfd_mach_mips4100:
	case bfd_mach_mips5000:
	  found_abi = MIPS_ABI_EABI64;
	  break;
	case bfd_mach_mips8000:
	case bfd_mach_mips10000:
	  /* On Irix, ELF64 executables use the N64 ABI.  The
	     pseudo-sections which describe the ABI aren't present
	     on IRIX.  (Even for executables created by gcc.)  */
	  if (info.abfd != NULL
	      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour
	      && elf_elfheader (info.abfd)->e_ident[EI_CLASS] == ELFCLASS64)
	    found_abi = MIPS_ABI_N64;
	  else
	    found_abi = MIPS_ABI_N32;
	  break;
	}
    }

  /* Default 64-bit objects to N64 instead of O32.  */
  if (found_abi == MIPS_ABI_UNKNOWN
      && info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour
      && elf_elfheader (info.abfd)->e_ident[EI_CLASS] == ELFCLASS64)
    found_abi = MIPS_ABI_N64;

  if (gdbarch_debug)
    gdb_printf (gdb_stdlog, "mips_gdbarch_init: found_abi = %d\n",
		found_abi);

  /* What has the user specified from the command line?  */
  wanted_abi = global_mips_abi ();
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog, "mips_gdbarch_init: wanted_abi = %d\n",
		wanted_abi);

  /* Now that we have found what the ABI for this binary would be,
     check whether the user is overriding it.  */
  if (wanted_abi != MIPS_ABI_UNKNOWN)
    mips_abi = wanted_abi;
  else if (found_abi != MIPS_ABI_UNKNOWN)
    mips_abi = found_abi;
  else
    mips_abi = MIPS_ABI_O32;
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog, "mips_gdbarch_init: mips_abi = %d\n",
		mips_abi);

  /* Make sure we don't use a 32-bit architecture with a 64-bit ABI.  */
  if (mips_abi != MIPS_ABI_EABI32
      && mips_abi != MIPS_ABI_O32
      && info.bfd_arch_info != NULL
      && info.bfd_arch_info->arch == bfd_arch_mips
      && info.bfd_arch_info->bits_per_word < 64)
    info.bfd_arch_info = bfd_lookup_arch (bfd_arch_mips, bfd_mach_mips4000);

  /* Determine the default compressed ISA.  */
  if ((elf_flags & EF_MIPS_ARCH_ASE_MICROMIPS) != 0
      && (elf_flags & EF_MIPS_ARCH_ASE_M16) == 0)
    mips_isa = ISA_MICROMIPS;
  else if ((elf_flags & EF_MIPS_ARCH_ASE_M16) != 0
	   && (elf_flags & EF_MIPS_ARCH_ASE_MICROMIPS) == 0)
    mips_isa = ISA_MIPS16;
  else
    mips_isa = global_mips_compression ();
  mips_compression_string = mips_compression_strings[mips_isa];

  /* Also used when doing an architecture lookup.  */
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog,
		"mips_gdbarch_init: "
		"mips64_transfers_32bit_regs_p = %d\n",
		mips64_transfers_32bit_regs_p);

  /* Determine the MIPS FPU type.  */
#ifdef HAVE_ELF
  if (info.abfd
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_fpu_type = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
					     Tag_GNU_MIPS_ABI_FP);
#endif /* HAVE_ELF */

  if (!mips_fpu_type_auto)
    fpu_type = mips_fpu_type;
  else if (elf_fpu_type != Val_GNU_MIPS_ABI_FP_ANY)
    {
      switch (elf_fpu_type)
	{
	case Val_GNU_MIPS_ABI_FP_DOUBLE:
	  fpu_type = MIPS_FPU_DOUBLE;
	  break;
	case Val_GNU_MIPS_ABI_FP_SINGLE:
	  fpu_type = MIPS_FPU_SINGLE;
	  break;
	case Val_GNU_MIPS_ABI_FP_SOFT:
	default:
	  /* Soft float or unknown.  */
	  fpu_type = MIPS_FPU_NONE;
	  break;
	}
    }
  else if (info.bfd_arch_info != NULL
	   && info.bfd_arch_info->arch == bfd_arch_mips)
    switch (info.bfd_arch_info->mach)
      {
      case bfd_mach_mips3900:
      case bfd_mach_mips4100:
      case bfd_mach_mips4111:
      case bfd_mach_mips4120:
	fpu_type = MIPS_FPU_NONE;
	break;
      case bfd_mach_mips4650:
	fpu_type = MIPS_FPU_SINGLE;
	break;
      default:
	fpu_type = MIPS_FPU_DOUBLE;
	break;
      }
  else if (arches != NULL)
    fpu_type = mips_get_fpu_type (arches->gdbarch);
  else
    fpu_type = MIPS_FPU_DOUBLE;
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog,
		"mips_gdbarch_init: fpu_type = %d\n", fpu_type);

  /* Check for blatant incompatibilities.  */

  /* If we have only 32-bit registers, then we can't debug a 64-bit
     ABI.  */
  if (info.target_desc
      && tdesc_property (info.target_desc, PROPERTY_GP32) != NULL
      && mips_abi != MIPS_ABI_EABI32
      && mips_abi != MIPS_ABI_O32)
    return NULL;

  /* Fill in the OS dependent register numbers and names.  */
  if (info.osabi == GDB_OSABI_LINUX)
    {
      mips_regnum.fp0 = 38;
      mips_regnum.pc = 37;
      mips_regnum.cause = 36;
      mips_regnum.badvaddr = 35;
      mips_regnum.hi = 34;
      mips_regnum.lo = 33;
      mips_regnum.fp_control_status = 70;
      mips_regnum.fp_implementation_revision = 71;
      mips_regnum.dspacc = -1;
      mips_regnum.dspctl = -1;
      dspacc = 72;
      dspctl = 78;
      num_regs = 90;
      reg_names = mips_linux_reg_names;
    }
  else
    {
      mips_regnum.lo = MIPS_EMBED_LO_REGNUM;
      mips_regnum.hi = MIPS_EMBED_HI_REGNUM;
      mips_regnum.badvaddr = MIPS_EMBED_BADVADDR_REGNUM;
      mips_regnum.cause = MIPS_EMBED_CAUSE_REGNUM;
      mips_regnum.pc = MIPS_EMBED_PC_REGNUM;
      mips_regnum.fp0 = MIPS_EMBED_FP0_REGNUM;
      mips_regnum.fp_control_status = 70;
      mips_regnum.fp_implementation_revision = 71;
      mips_regnum.dspacc = dspacc = -1;
      mips_regnum.dspctl = dspctl = -1;
      num_regs = MIPS_LAST_EMBED_REGNUM + 1;
      if (info.bfd_arch_info != NULL
	  && info.bfd_arch_info->mach == bfd_mach_mips3900)
	reg_names = mips_tx39_reg_names;
      else
	reg_names = mips_generic_reg_names;
    }

  /* Check any target description for validity.  */
  if (tdesc_has_registers (info.target_desc))
    {
      static const char *const mips_gprs[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
      };
      static const char *const mips_fprs[] = {
	"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
      };

      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.cpu");
      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (i = MIPS_ZERO_REGNUM; i <= MIPS_RA_REGNUM; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					    mips_gprs[i]);


      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  mips_regnum.lo, "lo");
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  mips_regnum.hi, "hi");
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  mips_regnum.pc, "pc");

      if (!valid_p)
	return NULL;

      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.cp0");
      if (feature == NULL)
	return NULL;

      valid_p = 1;
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  mips_regnum.badvaddr, "badvaddr");
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  MIPS_PS_REGNUM, "status");
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  mips_regnum.cause, "cause");

      if (!valid_p)
	return NULL;

      /* FIXME drow/2007-05-17: The FPU should be optional.  The MIPS
	 backend is not prepared for that, though.  */
      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.fpu");
      if (feature == NULL)
	return NULL;

      valid_p = 1;
      for (i = 0; i < 32; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					    i + mips_regnum.fp0, mips_fprs[i]);

      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  mips_regnum.fp_control_status,
					  "fcsr");
      valid_p
	&= tdesc_numbered_register (feature, tdesc_data.get (),
				    mips_regnum.fp_implementation_revision,
				    "fir");

      if (!valid_p)
	return NULL;

      num_regs = mips_regnum.fp_implementation_revision + 1;

      if (dspacc >= 0)
	{
	  feature = tdesc_find_feature (info.target_desc,
					"org.gnu.gdb.mips.dsp");
	  /* The DSP registers are optional; it's OK if they are absent.  */
	  if (feature != NULL)
	    {
	      i = 0;
	      valid_p = 1;
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspacc + i++, "hi1");
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspacc + i++, "lo1");
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspacc + i++, "hi2");
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspacc + i++, "lo2");
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspacc + i++, "hi3");
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspacc + i++, "lo3");

	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  dspctl, "dspctl");

	      if (!valid_p)
		return NULL;

	      mips_regnum.dspacc = dspacc;
	      mips_regnum.dspctl = dspctl;

	      num_regs = mips_regnum.dspctl + 1;
	    }
	}

      /* It would be nice to detect an attempt to use a 64-bit ABI
	 when only 32-bit registers are provided.  */
      reg_names = NULL;
    }

  /* Try to find a pre-existing architecture.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      mips_gdbarch_tdep *tdep
	= gdbarch_tdep<mips_gdbarch_tdep> (arches->gdbarch);

      /* MIPS needs to be pedantic about which ABI and the compressed
	 ISA variation the object is using.  */
      if (tdep->elf_flags != elf_flags)
	continue;
      if (tdep->mips_abi != mips_abi)
	continue;
      if (tdep->mips_isa != mips_isa)
	continue;
      /* Need to be pedantic about which register virtual size is
	 used.  */
      if (tdep->mips64_transfers_32bit_regs_p
	  != mips64_transfers_32bit_regs_p)
	continue;
      /* Be pedantic about which FPU is selected.  */
      if (mips_get_fpu_type (arches->gdbarch) != fpu_type)
	continue;

      return arches->gdbarch;
    }

  /* Need a new architecture.  Fill in a target specific vector.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new mips_gdbarch_tdep));
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);

  tdep->elf_flags = elf_flags;
  tdep->mips64_transfers_32bit_regs_p = mips64_transfers_32bit_regs_p;
  tdep->found_abi = found_abi;
  tdep->mips_abi = mips_abi;
  tdep->mips_isa = mips_isa;
  tdep->mips_fpu_type = fpu_type;
  tdep->register_size_valid_p = 0;
  tdep->register_size = 0;

  if (info.target_desc)
    {
      /* Some useful properties can be inferred from the target.  */
      if (tdesc_property (info.target_desc, PROPERTY_GP32) != NULL)
	{
	  tdep->register_size_valid_p = 1;
	  tdep->register_size = 4;
	}
      else if (tdesc_property (info.target_desc, PROPERTY_GP64) != NULL)
	{
	  tdep->register_size_valid_p = 1;
	  tdep->register_size = 8;
	}
    }

  /* Initially set everything according to the default ABI/ISA.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_register_reggroup_p (gdbarch, mips_register_reggroup_p);
  set_gdbarch_pseudo_register_read (gdbarch, mips_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						mips_pseudo_register_write);

  set_gdbarch_ax_pseudo_register_collect (gdbarch,
					  mips_ax_pseudo_register_collect);
  set_gdbarch_ax_pseudo_register_push_stack
      (gdbarch, mips_ax_pseudo_register_push_stack);

  set_gdbarch_elf_make_msymbol_special (gdbarch,
					mips_elf_make_msymbol_special);
  set_gdbarch_make_symbol_special (gdbarch, mips_make_symbol_special);
  set_gdbarch_adjust_dwarf2_addr (gdbarch, mips_adjust_dwarf2_addr);
  set_gdbarch_adjust_dwarf2_line (gdbarch, mips_adjust_dwarf2_line);

  regnum = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct mips_regnum);
  *regnum = mips_regnum;
  set_gdbarch_fp0_regnum (gdbarch, regnum->fp0);
  set_gdbarch_num_regs (gdbarch, num_regs);
  set_gdbarch_num_pseudo_regs (gdbarch, num_regs);
  set_gdbarch_register_name (gdbarch, mips_register_name);
  set_gdbarch_virtual_frame_pointer (gdbarch, mips_virtual_frame_pointer);
  tdep->mips_processor_reg_names = reg_names;
  tdep->regnum = regnum;

  switch (mips_abi)
    {
    case MIPS_ABI_O32:
      set_gdbarch_push_dummy_call (gdbarch, mips_o32_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_o32_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 4 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 4 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_O64:
      set_gdbarch_push_dummy_call (gdbarch, mips_o64_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_o64_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 4 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 4 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_EABI32:
      set_gdbarch_push_dummy_call (gdbarch, mips_eabi_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_eabi_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_EABI64:
      set_gdbarch_push_dummy_call (gdbarch, mips_eabi_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_eabi_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_N32:
      set_gdbarch_push_dummy_call (gdbarch, mips_n32n64_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_n32n64_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_long_double_bit (gdbarch, 128);
      set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);
      break;
    case MIPS_ABI_N64:
      set_gdbarch_push_dummy_call (gdbarch, mips_n32n64_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_n32n64_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_long_double_bit (gdbarch, 128);
      set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);
      break;
    default:
      internal_error (_("unknown ABI in switch"));
    }

  /* GCC creates a pseudo-section whose name specifies the size of
     longs, since -mlong32 or -mlong64 may be used independent of
     other options.  How those options affect pointer sizes is ABI and
     architecture dependent, so use them to override the default sizes
     set by the ABI.  This table shows the relationship between ABI,
     -mlongXX, and size of pointers:

     ABI		-mlongXX	ptr bits
     ---		--------	--------
     o32		32		32
     o32		64		32
     n32		32		32
     n32		64		64
     o64		32		32
     o64		64		64
     n64		32		32
     n64		64		64
     eabi32		32		32
     eabi32		64		32
     eabi64		32		32
     eabi64		64		64

    Note that for o32 and eabi32, pointers are always 32 bits
    regardless of any -mlongXX option.  For all others, pointers and
    longs are the same, as set by -mlongXX or set by defaults.  */

  if (info.abfd != NULL)
    {
      int long_bit = 0;

      bfd_map_over_sections (info.abfd, mips_find_long_section, &long_bit);
      if (long_bit)
	{
	  set_gdbarch_long_bit (gdbarch, long_bit);
	  switch (mips_abi)
	    {
	    case MIPS_ABI_O32:
	    case MIPS_ABI_EABI32:
	      break;
	    case MIPS_ABI_N32:
	    case MIPS_ABI_O64:
	    case MIPS_ABI_N64:
	    case MIPS_ABI_EABI64:
	      set_gdbarch_ptr_bit (gdbarch, long_bit);
	      break;
	    default:
	      internal_error (_("unknown ABI in switch"));
	    }
	}
    }

  /* FIXME: jlarmour/2000-04-07: There *is* a flag EF_MIPS_32BIT_MODE
     that could indicate -gp32 BUT gas/config/tc-mips.c contains the
     comment:

     ``We deliberately don't allow "-gp32" to set the MIPS_32BITMODE
     flag in object files because to do so would make it impossible to
     link with libraries compiled without "-gp32".  This is
     unnecessarily restrictive.

     We could solve this problem by adding "-gp32" multilibs to gcc,
     but to set this flag before gcc is built with such multilibs will
     break too many systems.''

     But even more unhelpfully, the default linker output target for
     mips64-elf is elf32-bigmips, and has EF_MIPS_32BIT_MODE set, even
     for 64-bit programs - you need to change the ABI to change this,
     and not all gcc targets support that currently.  Therefore using
     this flag to detect 32-bit mode would do the wrong thing given
     the current gcc - it would make GDB treat these 64-bit programs
     as 32-bit programs by default.  */

  set_gdbarch_read_pc (gdbarch, mips_read_pc);
  set_gdbarch_write_pc (gdbarch, mips_write_pc);

  /* Add/remove bits from an address.  The MIPS needs be careful to
     ensure that all 32 bit addresses are sign extended to 64 bits.  */
  set_gdbarch_addr_bits_remove (gdbarch, mips_addr_bits_remove);

  /* Unwind the frame.  */
  set_gdbarch_unwind_pc (gdbarch, mips_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, mips_unwind_sp);
  set_gdbarch_dummy_id (gdbarch, mips_dummy_id);

  /* Map debug register numbers onto internal register numbers.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, mips_stab_reg_to_regnum);
  set_gdbarch_ecoff_reg_to_regnum (gdbarch,
				   mips_dwarf_dwarf2_ecoff_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch,
				    mips_dwarf_dwarf2_ecoff_reg_to_regnum);
  set_gdbarch_register_sim_regno (gdbarch, mips_register_sim_regno);

  /* MIPS version of CALL_DUMMY.  */

  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_push_dummy_code (gdbarch, mips_push_dummy_code);
  set_gdbarch_frame_align (gdbarch, mips_frame_align);

  set_gdbarch_print_float_info (gdbarch, mips_print_float_info);

  set_gdbarch_convert_register_p (gdbarch, mips_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, mips_register_to_value);
  set_gdbarch_value_to_register (gdbarch, mips_value_to_register);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, mips_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, mips_sw_breakpoint_from_kind);
  set_gdbarch_adjust_breakpoint_address (gdbarch,
					 mips_adjust_breakpoint_address);

  set_gdbarch_skip_prologue (gdbarch, mips_skip_prologue);

  set_gdbarch_stack_frame_destroyed_p (gdbarch, mips_stack_frame_destroyed_p);

  set_gdbarch_pointer_to_address (gdbarch, signed_pointer_to_address);
  set_gdbarch_address_to_pointer (gdbarch, address_to_signed_pointer);
  set_gdbarch_integer_to_address (gdbarch, mips_integer_to_address);

  set_gdbarch_register_type (gdbarch, mips_register_type);

  set_gdbarch_print_registers_info (gdbarch, mips_print_registers_info);

  set_gdbarch_print_insn (gdbarch, gdb_print_insn_mips);
  if (mips_abi == MIPS_ABI_N64)
    set_gdbarch_disassembler_options_implicit
      (gdbarch, (const char *) mips_disassembler_options_n64);
  else if (mips_abi == MIPS_ABI_N32)
    set_gdbarch_disassembler_options_implicit
      (gdbarch, (const char *) mips_disassembler_options_n32);
  else
    set_gdbarch_disassembler_options_implicit
      (gdbarch, (const char *) mips_disassembler_options_o32);
  set_gdbarch_disassembler_options (gdbarch, &mips_disassembler_options);
  set_gdbarch_valid_disassembler_options (gdbarch,
					  disassembler_options_mips ());

  /* FIXME: cagney/2003-08-29: The macros target_have_steppable_watchpoint,
     HAVE_NONSTEPPABLE_WATCHPOINT, and target_have_continuable_watchpoint
     need to all be folded into the target vector.  Since they are
     being used as guards for target_stopped_by_watchpoint, why not have
     target_stopped_by_watchpoint return the type of watchpoint that the code
     is sitting on?  */
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  set_gdbarch_skip_trampoline_code (gdbarch, mips_skip_trampoline_code);

  /* NOTE drow/2012-04-25: We overload the core solib trampoline code
     to support MIPS16.  This is a bad thing.  Make sure not to do it
     if we have an OS ABI that actually supports shared libraries, since
     shared library support is more important.  If we have an OS someday
     that supports both shared libraries and MIPS16, we'll have to find
     a better place for these.
     macro/2012-04-25: But that applies to return trampolines only and
     currently no MIPS OS ABI uses shared libraries that have them.  */
  set_gdbarch_in_solib_return_trampoline (gdbarch, mips_in_return_stub);

  set_gdbarch_single_step_through_delay (gdbarch,
					 mips_single_step_through_delay);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  mips_register_g_packet_guesses (gdbarch);

  /* Hook in OS ABI-specific overrides, if they have been registered.  */
  info.tdesc_data = tdesc_data.get ();
  gdbarch_init_osabi (info, gdbarch);

  /* The hook may have adjusted num_regs, fetch the final value and
     set pc_regnum and sp_regnum now that it has been fixed.  */
  num_regs = gdbarch_num_regs (gdbarch);
  set_gdbarch_pc_regnum (gdbarch, regnum->pc + num_regs);
  set_gdbarch_sp_regnum (gdbarch, MIPS_SP_REGNUM + num_regs);

  /* Unwind the frame.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &mips_stub_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &mips_insn16_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &mips_micro_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &mips_insn32_frame_unwind);
  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_stub_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_insn16_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_micro_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_insn32_frame_base_sniffer);

  if (tdesc_data != nullptr)
    {
      set_tdesc_pseudo_register_type (gdbarch, mips_pseudo_register_type);
      tdesc_use_registers (gdbarch, info.target_desc, std::move (tdesc_data));

      /* Override the normal target description methods to handle our
	 dual real and pseudo registers.  */
      set_gdbarch_register_name (gdbarch, mips_register_name);
      set_gdbarch_register_reggroup_p (gdbarch,
				       mips_tdesc_register_reggroup_p);

      num_regs = gdbarch_num_regs (gdbarch);
      set_gdbarch_num_pseudo_regs (gdbarch, num_regs);
      set_gdbarch_pc_regnum (gdbarch, tdep->regnum->pc + num_regs);
      set_gdbarch_sp_regnum (gdbarch, MIPS_SP_REGNUM + num_regs);
    }

  /* Add ABI-specific aliases for the registers.  */
  if (mips_abi == MIPS_ABI_N32 || mips_abi == MIPS_ABI_N64)
    for (i = 0; i < ARRAY_SIZE (mips_n32_n64_aliases); i++)
      user_reg_add (gdbarch, mips_n32_n64_aliases[i].name,
		    value_of_mips_user_reg, &mips_n32_n64_aliases[i].regnum);
  else
    for (i = 0; i < ARRAY_SIZE (mips_o32_aliases); i++)
      user_reg_add (gdbarch, mips_o32_aliases[i].name,
		    value_of_mips_user_reg, &mips_o32_aliases[i].regnum);

  /* Add some other standard aliases.  */
  for (i = 0; i < ARRAY_SIZE (mips_register_aliases); i++)
    user_reg_add (gdbarch, mips_register_aliases[i].name,
		  value_of_mips_user_reg, &mips_register_aliases[i].regnum);

  for (i = 0; i < ARRAY_SIZE (mips_numeric_register_aliases); i++)
    user_reg_add (gdbarch, mips_numeric_register_aliases[i].name,
		  value_of_mips_user_reg, 
		  &mips_numeric_register_aliases[i].regnum);

  return gdbarch;
}

static void
mips_abi_update (const char *ignore_args,
		 int from_tty, struct cmd_list_element *c)
{
  struct gdbarch_info info;

  /* Force the architecture to update, and (if it's a MIPS architecture)
     mips_gdbarch_init will take care of the rest.  */
  gdbarch_update_p (info);
}

/* Print out which MIPS ABI is in use.  */

static void
show_mips_abi (struct ui_file *file,
	       int from_tty,
	       struct cmd_list_element *ignored_cmd,
	       const char *ignored_value)
{
  if (gdbarch_bfd_arch_info (current_inferior ()->arch  ())->arch
      != bfd_arch_mips)
    gdb_printf
      (file, 
       "The MIPS ABI is unknown because the current architecture "
       "is not MIPS.\n");
  else
    {
      enum mips_abi global_abi = global_mips_abi ();
      enum mips_abi actual_abi = mips_abi (current_inferior ()->arch ());
      const char *actual_abi_str = mips_abi_strings[actual_abi];

      if (global_abi == MIPS_ABI_UNKNOWN)
	gdb_printf
	  (file, 
	   "The MIPS ABI is set automatically (currently \"%s\").\n",
	   actual_abi_str);
      else if (global_abi == actual_abi)
	gdb_printf
	  (file,
	   "The MIPS ABI is assumed to be \"%s\" (due to user setting).\n",
	   actual_abi_str);
      else
	{
	  /* Probably shouldn't happen...  */
	  gdb_printf (file,
		      "The (auto detected) MIPS ABI \"%s\" is in use "
		      "even though the user setting was \"%s\".\n",
		      actual_abi_str, mips_abi_strings[global_abi]);
	}
    }
}

/* Print out which MIPS compressed ISA encoding is used.  */

static void
show_mips_compression (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("The compressed ISA encoding used is %s.\n"),
	      value);
}

/* Return a textual name for MIPS FPU type FPU_TYPE.  */

static const char *
mips_fpu_type_str (enum mips_fpu_type fpu_type)
{
  switch (fpu_type)
    {
    case MIPS_FPU_NONE:
      return "none";
    case MIPS_FPU_SINGLE:
      return "single";
    case MIPS_FPU_DOUBLE:
      return "double";
    default:
      return "???";
    }
}

static void
mips_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  if (tdep != NULL)
    {
      int ef_mips_arch;
      int ef_mips_32bitmode;
      /* Determine the ISA.  */
      switch (tdep->elf_flags & EF_MIPS_ARCH)
	{
	case EF_MIPS_ARCH_1:
	  ef_mips_arch = 1;
	  break;
	case EF_MIPS_ARCH_2:
	  ef_mips_arch = 2;
	  break;
	case EF_MIPS_ARCH_3:
	  ef_mips_arch = 3;
	  break;
	case EF_MIPS_ARCH_4:
	  ef_mips_arch = 4;
	  break;
	default:
	  ef_mips_arch = 0;
	  break;
	}
      /* Determine the size of a pointer.  */
      ef_mips_32bitmode = (tdep->elf_flags & EF_MIPS_32BITMODE);
      gdb_printf (file,
		  "mips_dump_tdep: tdep->elf_flags = 0x%x\n",
		  tdep->elf_flags);
      gdb_printf (file,
		  "mips_dump_tdep: ef_mips_32bitmode = %d\n",
		  ef_mips_32bitmode);
      gdb_printf (file,
		  "mips_dump_tdep: ef_mips_arch = %d\n",
		  ef_mips_arch);
      gdb_printf (file,
		  "mips_dump_tdep: tdep->mips_abi = %d (%s)\n",
		  tdep->mips_abi, mips_abi_strings[tdep->mips_abi]);
      gdb_printf (file,
		  "mips_dump_tdep: "
		  "mips_mask_address_p() %d (default %d)\n",
		  mips_mask_address_p (tdep),
		  tdep->default_mask_address_p);
    }
  gdb_printf (file,
	      "mips_dump_tdep: MIPS_DEFAULT_FPU_TYPE = %d (%s)\n",
	      MIPS_DEFAULT_FPU_TYPE,
	      mips_fpu_type_str (MIPS_DEFAULT_FPU_TYPE));
  gdb_printf (file, "mips_dump_tdep: MIPS_EABI = %d\n",
	      mips_eabi (gdbarch));
  gdb_printf (file,
	      "mips_dump_tdep: MIPS_FPU_TYPE = %d (%s)\n",
	      mips_get_fpu_type (gdbarch),
	      mips_fpu_type_str (mips_get_fpu_type (gdbarch)));
}

void _initialize_mips_tdep ();
void
_initialize_mips_tdep ()
{
  static struct cmd_list_element *mipsfpulist = NULL;

  mips_abi_string = mips_abi_strings[MIPS_ABI_UNKNOWN];
  if (MIPS_ABI_LAST + 1
      != sizeof (mips_abi_strings) / sizeof (mips_abi_strings[0]))
    internal_error (_("mips_abi_strings out of sync"));

  gdbarch_register (bfd_arch_mips, mips_gdbarch_init, mips_dump_tdep);

  /* Create feature sets with the appropriate properties.  The values
     are not important.  */
  mips_tdesc_gp32 = allocate_target_description ().release ();
  set_tdesc_property (mips_tdesc_gp32, PROPERTY_GP32, "");

  mips_tdesc_gp64 = allocate_target_description ().release ();
  set_tdesc_property (mips_tdesc_gp64, PROPERTY_GP64, "");

  /* Add root prefix command for all "set mips"/"show mips" commands.  */
  add_setshow_prefix_cmd ("mips", no_class,
			  _("Various MIPS specific commands."),
			  _("Various MIPS specific commands."),
			  &setmipscmdlist, &showmipscmdlist,
			  &setlist, &showlist);

  /* Allow the user to override the ABI.  */
  add_setshow_enum_cmd ("abi", class_obscure, mips_abi_strings,
			&mips_abi_string, _("\
Set the MIPS ABI used by this program."), _("\
Show the MIPS ABI used by this program."), _("\
This option can be set to one of:\n\
  auto  - the default ABI associated with the current binary\n\
  o32\n\
  o64\n\
  n32\n\
  n64\n\
  eabi32\n\
  eabi64"),
			mips_abi_update,
			show_mips_abi,
			&setmipscmdlist, &showmipscmdlist);

  /* Allow the user to set the ISA to assume for compressed code if ELF
     file flags don't tell or there is no program file selected.  This
     setting is updated whenever unambiguous ELF file flags are interpreted,
     and carried over to subsequent sessions.  */
  add_setshow_enum_cmd ("compression", class_obscure, mips_compression_strings,
			&mips_compression_string, _("\
Set the compressed ISA encoding used by MIPS code."), _("\
Show the compressed ISA encoding used by MIPS code."), _("\
Select the compressed ISA encoding used in functions that have no symbol\n\
information available.  The encoding can be set to either of:\n\
  mips16\n\
  micromips\n\
and is updated automatically from ELF file flags if available."),
			mips_abi_update,
			show_mips_compression,
			&setmipscmdlist, &showmipscmdlist);

  /* Let the user turn off floating point and set the fence post for
     heuristic_proc_start.  */

  add_basic_prefix_cmd ("mipsfpu", class_support,
			_("Set use of MIPS floating-point coprocessor."),
			&mipsfpulist, 0, &setlist);
  add_cmd ("single", class_support, set_mipsfpu_single_command,
	   _("Select single-precision MIPS floating-point coprocessor."),
	   &mipsfpulist);
  cmd_list_element *set_mipsfpu_double_cmd
    = add_cmd ("double", class_support, set_mipsfpu_double_command,
	       _("Select double-precision MIPS floating-point coprocessor."),
	       &mipsfpulist);
  add_alias_cmd ("on", set_mipsfpu_double_cmd, class_support, 1, &mipsfpulist);
  add_alias_cmd ("yes", set_mipsfpu_double_cmd, class_support, 1, &mipsfpulist);
  add_alias_cmd ("1", set_mipsfpu_double_cmd, class_support, 1, &mipsfpulist);

  cmd_list_element *set_mipsfpu_none_cmd
    = add_cmd ("none", class_support, set_mipsfpu_none_command,
	       _("Select no MIPS floating-point coprocessor."), &mipsfpulist);
  add_alias_cmd ("off", set_mipsfpu_none_cmd, class_support, 1, &mipsfpulist);
  add_alias_cmd ("no", set_mipsfpu_none_cmd, class_support, 1, &mipsfpulist);
  add_alias_cmd ("0", set_mipsfpu_none_cmd, class_support, 1, &mipsfpulist);
  add_cmd ("auto", class_support, set_mipsfpu_auto_command,
	   _("Select MIPS floating-point coprocessor automatically."),
	   &mipsfpulist);
  add_cmd ("mipsfpu", class_support, show_mipsfpu_command,
	   _("Show current use of MIPS floating-point coprocessor target."),
	   &showlist);

  /* We really would like to have both "0" and "unlimited" work, but
     command.c doesn't deal with that.  So make it a var_zinteger
     because the user can always use "999999" or some such for unlimited.  */
  add_setshow_zinteger_cmd ("heuristic-fence-post", class_support,
			    &heuristic_fence_post, _("\
Set the distance searched for the start of a function."), _("\
Show the distance searched for the start of a function."), _("\
If you are debugging a stripped executable, GDB needs to search through the\n\
program for the start of a function.  This command sets the distance of the\n\
search.  The only need to set it is when debugging a stripped executable."),
			    reinit_frame_cache_sfunc,
			    NULL, /* FIXME: i18n: The distance searched for
				     the start of a function is %s.  */
			    &setlist, &showlist);

  /* Allow the user to control whether the upper bits of 64-bit
     addresses should be zeroed.  */
  add_setshow_auto_boolean_cmd ("mask-address", no_class,
				&mask_address_var, _("\
Set zeroing of upper 32 bits of 64-bit addresses."), _("\
Show zeroing of upper 32 bits of 64-bit addresses."), _("\
Use \"on\" to enable the masking, \"off\" to disable it and \"auto\" to\n\
allow GDB to determine the correct value."),
				NULL, show_mask_address,
				&setmipscmdlist, &showmipscmdlist);

  /* Allow the user to control the size of 32 bit registers within the
     raw remote packet.  */
  add_setshow_boolean_cmd ("remote-mips64-transfers-32bit-regs", class_obscure,
			   &mips64_transfers_32bit_regs_p, _("\
Set compatibility with 64-bit MIPS target that transfers 32-bit quantities."),
			   _("\
Show compatibility with 64-bit MIPS target that transfers 32-bit quantities."),
			   _("\
Use \"on\" to enable backward compatibility with older MIPS 64 GDB+target\n\
that would transfer 32 bits for some registers (e.g. SR, FSR) and\n\
64 bits for others.  Use \"off\" to disable compatibility mode"),
			   set_mips64_transfers_32bit_regs,
			   NULL, /* FIXME: i18n: Compatibility with 64-bit
				    MIPS target that transfers 32-bit
				    quantities is %s.  */
			   &setlist, &showlist);

  /* Debug this files internals.  */
  add_setshow_zuinteger_cmd ("mips", class_maintenance,
			     &mips_debug, _("\
Set mips debugging."), _("\
Show mips debugging."), _("\
When non-zero, mips specific debugging is enabled."),
			     NULL,
			     NULL, /* FIXME: i18n: Mips debugging is
				      currently %s.  */
			     &setdebuglist, &showdebuglist);
}

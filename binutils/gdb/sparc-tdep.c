/* Target-dependent code for SPARC.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "dis-asm.h"
#include "dwarf2.h"
#include "dwarf2/frame.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "inferior.h"
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "regcache.h"
#include "target.h"
#include "target-descriptions.h"
#include "value.h"

#include "sparc-tdep.h"
#include "sparc-ravenscar-thread.h"
#include <algorithm>

struct regset;

/* This file implements the SPARC 32-bit ABI as defined by the section
   "Low-Level System Information" of the SPARC Compliance Definition
   (SCD) 2.4.1, which is the 32-bit System V psABI for SPARC.  The SCD
   lists changes with respect to the original 32-bit psABI as defined
   in the "System V ABI, SPARC Processor Supplement".

   Note that if we talk about SunOS, we mean SunOS 4.x, which was
   BSD-based, which is sometimes (retroactively?) referred to as
   Solaris 1.x.  If we talk about Solaris we mean Solaris 2.x and
   above (Solaris 7, 8 and 9 are nothing but Solaris 2.7, 2.8 and 2.9
   suffering from severe version number inflation).  Solaris 2.x is
   also known as SunOS 5.x, since that's what uname(1) says.  Solaris
   2.x is SVR4-based.  */

/* Please use the sparc32_-prefix for 32-bit specific code, the
   sparc64_-prefix for 64-bit specific code and the sparc_-prefix for
   code that can handle both.  The 64-bit specific code lives in
   sparc64-tdep.c; don't add any here.  */

/* The stack pointer is offset from the stack frame by a BIAS of 2047
   (0x7ff) for 64-bit code.  BIAS is likely to be defined on SPARC
   hosts, so undefine it first.  */
#undef BIAS
#define BIAS 2047

/* Macros to extract fields from SPARC instructions.  */
#define X_OP(i) (((i) >> 30) & 0x3)
#define X_RD(i) (((i) >> 25) & 0x1f)
#define X_A(i) (((i) >> 29) & 1)
#define X_COND(i) (((i) >> 25) & 0xf)
#define X_OP2(i) (((i) >> 22) & 0x7)
#define X_IMM22(i) ((i) & 0x3fffff)
#define X_OP3(i) (((i) >> 19) & 0x3f)
#define X_RS1(i) (((i) >> 14) & 0x1f)
#define X_RS2(i) ((i) & 0x1f)
#define X_I(i) (((i) >> 13) & 1)
/* Sign extension macros.  */
#define X_DISP22(i) ((X_IMM22 (i) ^ 0x200000) - 0x200000)
#define X_DISP19(i) ((((i) & 0x7ffff) ^ 0x40000) - 0x40000)
#define X_DISP10(i) ((((((i) >> 11) && 0x300) | (((i) >> 5) & 0xff)) ^ 0x200) - 0x200)
#define X_SIMM13(i) ((((i) & 0x1fff) ^ 0x1000) - 0x1000)
/* Macros to identify some instructions.  */
/* RETURN (RETT in V8) */
#define X_RETTURN(i) ((X_OP (i) == 0x2) && (X_OP3 (i) == 0x39))

/* Fetch the instruction at PC.  Instructions are always big-endian
   even if the processor operates in little-endian mode.  */

unsigned long
sparc_fetch_instruction (CORE_ADDR pc)
{
  gdb_byte buf[4];
  unsigned long insn;
  int i;

  /* If we can't read the instruction at PC, return zero.  */
  if (target_read_memory (pc, buf, sizeof (buf)))
    return 0;

  insn = 0;
  for (i = 0; i < sizeof (buf); i++)
    insn = (insn << 8) | buf[i];
  return insn;
}


/* Return non-zero if the instruction corresponding to PC is an "unimp"
   instruction.  */

static int
sparc_is_unimp_insn (CORE_ADDR pc)
{
  const unsigned long insn = sparc_fetch_instruction (pc);
  
  return ((insn & 0xc1c00000) == 0);
}

/* Return non-zero if the instruction corresponding to PC is an
   "annulled" branch, i.e. the annul bit is set.  */

int
sparc_is_annulled_branch_insn (CORE_ADDR pc)
{
  /* The branch instructions featuring an annul bit can be identified
     by the following bit patterns:

     OP=0
      OP2=1: Branch on Integer Condition Codes with Prediction (BPcc).
      OP2=2: Branch on Integer Condition Codes (Bcc).
      OP2=5: Branch on FP Condition Codes with Prediction (FBfcc).
      OP2=6: Branch on FP Condition Codes (FBcc).
      OP2=3 && Bit28=0:
	     Branch on Integer Register with Prediction (BPr).

     This leaves out ILLTRAP (OP2=0), SETHI/NOP (OP2=4) and the V8
     coprocessor branch instructions (Op2=7).  */

  const unsigned long insn = sparc_fetch_instruction (pc);
  const unsigned op2 = X_OP2 (insn);

  if ((X_OP (insn) == 0)
      && ((op2 == 1) || (op2 == 2) || (op2 == 5) || (op2 == 6)
	  || ((op2 == 3) && ((insn & 0x10000000) == 0))))
    return X_A (insn);
  else
    return 0;
}

/* OpenBSD/sparc includes StackGhost, which according to the author's
   website http://stackghost.cerias.purdue.edu "... transparently and
   automatically protects applications' stack frames; more
   specifically, it guards the return pointers.  The protection
   mechanisms require no application source or binary modification and
   imposes only a negligible performance penalty."

   The same website provides the following description of how
   StackGhost works:

   "StackGhost interfaces with the kernel trap handler that would
   normally write out registers to the stack and the handler that
   would read them back in.  By XORing a cookie into the
   return-address saved in the user stack when it is actually written
   to the stack, and then XOR it out when the return-address is pulled
   from the stack, StackGhost can cause attacker corrupted return
   pointers to behave in a manner the attacker cannot predict.
   StackGhost can also use several unused bits in the return pointer
   to detect a smashed return pointer and abort the process."

   For GDB this means that whenever we're reading %i7 from a stack
   frame's window save area, we'll have to XOR the cookie.

   More information on StackGuard can be found on in:

   Mike Frantzen and Mike Shuey.  "StackGhost: Hardware Facilitated
   Stack Protection."  2001.  Published in USENIX Security Symposium
   '01.  */

/* Fetch StackGhost Per-Process XOR cookie.  */

ULONGEST
sparc_fetch_wcookie (struct gdbarch *gdbarch)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct target_ops *ops = current_inferior ()->top_target ();
  gdb_byte buf[8];
  int len;

  len = target_read (ops, TARGET_OBJECT_WCOOKIE, NULL, buf, 0, 8);
  if (len == -1)
    return 0;

  /* We should have either an 32-bit or an 64-bit cookie.  */
  gdb_assert (len == 4 || len == 8);

  return extract_unsigned_integer (buf, len, byte_order);
}


/* The functions on this page are intended to be used to classify
   function arguments.  */

/* Check whether TYPE is "Integral or Pointer".  */

static int
sparc_integral_or_pointer_p (const struct type *type)
{
  int len = type->length ();

  switch (type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_RANGE:
      /* We have byte, half-word, word and extended-word/doubleword
	 integral types.  The doubleword is an extension to the
	 original 32-bit ABI by the SCD 2.4.x.  */
      return (len == 1 || len == 2 || len == 4 || len == 8);
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      /* Allow either 32-bit or 64-bit pointers.  */
      return (len == 4 || len == 8);
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is "Floating".  */

static int
sparc_floating_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_FLT:
      {
	int len = type->length ();
	return (len == 4 || len == 8 || len == 16);
      }
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is "Complex Floating".  */

static int
sparc_complex_floating_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_COMPLEX:
      {
	int len = type->length ();
	return (len == 8 || len == 16 || len == 32);
      }
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is "Structure or Union".

   In terms of Ada subprogram calls, arrays are treated the same as
   struct and union types.  So this function also returns non-zero
   for array types.  */

static int
sparc_structure_or_union_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ARRAY:
      return 1;
    default:
      break;
    }

  return 0;
}

/* Return true if TYPE is returned by memory, false if returned by
   register.  */

static bool
sparc_structure_return_p (const struct type *type)
{
  if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    {
      /* Float vectors are always returned by memory.  */
      if (sparc_floating_p (check_typedef (type->target_type ())))
	return true;
      /* Integer vectors are returned by memory if the vector size
	 is greater than 8 bytes long.  */
      return (type->length () > 8);
    }

  if (sparc_floating_p (type))
    {
      /* Floating point types are passed by register for size 4 and
	 8 bytes, and by memory for size 16 bytes.  */
      return (type->length () == 16);
    }

  /* Other than that, only aggregates of all sizes get returned by
     memory.  */
  return sparc_structure_or_union_p (type);
}

/* Return true if arguments of the given TYPE are passed by
   memory; false if returned by register.  */

static bool
sparc_arg_by_memory_p (const struct type *type)
{
  if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    {
      /* Float vectors are always passed by memory.  */
      if (sparc_floating_p (check_typedef (type->target_type ())))
	return true;
      /* Integer vectors are passed by memory if the vector size
	 is greater than 8 bytes long.  */
      return (type->length () > 8);
    }

  /* Floats are passed by register for size 4 and 8 bytes, and by memory
     for size 16 bytes.  */
  if (sparc_floating_p (type))
    return (type->length () == 16);

  /* Complex floats and aggregates of all sizes are passed by memory.  */
  if (sparc_complex_floating_p (type) || sparc_structure_or_union_p (type))
    return true;

  /* Everything else gets passed by register.  */
  return false;
}

/* Register information.  */
#define SPARC32_FPU_REGISTERS                             \
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",         \
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",   \
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", \
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
#define SPARC32_CP0_REGISTERS \
  "y", "psr", "wim", "tbr", "pc", "npc", "fsr", "csr"

static const char * const sparc_core_register_names[] = {
  SPARC_CORE_REGISTERS
};
static const char * const sparc32_fpu_register_names[] = {
  SPARC32_FPU_REGISTERS
};
static const char * const sparc32_cp0_register_names[] = {
  SPARC32_CP0_REGISTERS
};

static const char * const sparc32_register_names[] =
{
  SPARC_CORE_REGISTERS,
  SPARC32_FPU_REGISTERS,
  SPARC32_CP0_REGISTERS
};

/* Total number of registers.  */
#define SPARC32_NUM_REGS ARRAY_SIZE (sparc32_register_names)

/* We provide the aliases %d0..%d30 for the floating registers as
   "psuedo" registers.  */

static const char * const sparc32_pseudo_register_names[] =
{
  "d0", "d2", "d4", "d6", "d8", "d10", "d12", "d14",
  "d16", "d18", "d20", "d22", "d24", "d26", "d28", "d30"
};

/* Total number of pseudo registers.  */
#define SPARC32_NUM_PSEUDO_REGS ARRAY_SIZE (sparc32_pseudo_register_names)

/* Return the name of pseudo register REGNUM.  */

static const char *
sparc32_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  gdb_assert (regnum < SPARC32_NUM_PSEUDO_REGS);
  return sparc32_pseudo_register_names[regnum];
}

/* Return the name of register REGNUM.  */

static const char *
sparc32_register_name (struct gdbarch *gdbarch, int regnum)
{
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, regnum);

  if (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch))
    return sparc32_register_names[regnum];

  return sparc32_pseudo_register_name (gdbarch, regnum);
}

/* Construct types for ISA-specific registers.  */

static struct type *
sparc_psr_type (struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  if (!tdep->sparc_psr_type)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_sparc_psr", 32);
      append_flags_type_flag (type, 5, "ET");
      append_flags_type_flag (type, 6, "PS");
      append_flags_type_flag (type, 7, "S");
      append_flags_type_flag (type, 12, "EF");
      append_flags_type_flag (type, 13, "EC");

      tdep->sparc_psr_type = type;
    }

  return tdep->sparc_psr_type;
}

static struct type *
sparc_fsr_type (struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  if (!tdep->sparc_fsr_type)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_sparc_fsr", 32);
      append_flags_type_flag (type, 0, "NXA");
      append_flags_type_flag (type, 1, "DZA");
      append_flags_type_flag (type, 2, "UFA");
      append_flags_type_flag (type, 3, "OFA");
      append_flags_type_flag (type, 4, "NVA");
      append_flags_type_flag (type, 5, "NXC");
      append_flags_type_flag (type, 6, "DZC");
      append_flags_type_flag (type, 7, "UFC");
      append_flags_type_flag (type, 8, "OFC");
      append_flags_type_flag (type, 9, "NVC");
      append_flags_type_flag (type, 22, "NS");
      append_flags_type_flag (type, 23, "NXM");
      append_flags_type_flag (type, 24, "DZM");
      append_flags_type_flag (type, 25, "UFM");
      append_flags_type_flag (type, 26, "OFM");
      append_flags_type_flag (type, 27, "NVM");

      tdep->sparc_fsr_type = type;
    }

  return tdep->sparc_fsr_type;
}

/* Return the GDB type object for the "standard" data type of data in
   pseudo register REGNUM.  */

static struct type *
sparc32_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= SPARC32_D0_REGNUM && regnum <= SPARC32_D30_REGNUM)
    return builtin_type (gdbarch)->builtin_double;

  internal_error (_("sparc32_pseudo_register_type: bad register number %d"),
		  regnum);
}

/* Return the GDB type object for the "standard" data type of data in
   register REGNUM.  */

static struct type *
sparc32_register_type (struct gdbarch *gdbarch, int regnum)
{
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_type (gdbarch, regnum);

  if (regnum >= SPARC_F0_REGNUM && regnum <= SPARC_F31_REGNUM)
    return builtin_type (gdbarch)->builtin_float;

  if (regnum == SPARC_SP_REGNUM || regnum == SPARC_FP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  if (regnum == SPARC32_PC_REGNUM || regnum == SPARC32_NPC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;

  if (regnum == SPARC32_PSR_REGNUM)
    return sparc_psr_type (gdbarch);

  if (regnum == SPARC32_FSR_REGNUM)
    return sparc_fsr_type (gdbarch);

  if (regnum >= gdbarch_num_regs (gdbarch))
    return sparc32_pseudo_register_type (gdbarch, regnum);

  return builtin_type (gdbarch)->builtin_int32;
}

static enum register_status
sparc32_pseudo_register_read (struct gdbarch *gdbarch,
			      readable_regcache *regcache,
			      int regnum, gdb_byte *buf)
{
  enum register_status status;

  regnum -= gdbarch_num_regs (gdbarch);
  gdb_assert (regnum >= SPARC32_D0_REGNUM && regnum <= SPARC32_D30_REGNUM);

  regnum = SPARC_F0_REGNUM + 2 * (regnum - SPARC32_D0_REGNUM);
  status = regcache->raw_read (regnum, buf);
  if (status == REG_VALID)
    status = regcache->raw_read (regnum + 1, buf + 4);
  return status;
}

static void
sparc32_pseudo_register_write (struct gdbarch *gdbarch,
			       struct regcache *regcache,
			       int regnum, const gdb_byte *buf)
{
  regnum -= gdbarch_num_regs (gdbarch);
  gdb_assert (regnum >= SPARC32_D0_REGNUM && regnum <= SPARC32_D30_REGNUM);

  regnum = SPARC_F0_REGNUM + 2 * (regnum - SPARC32_D0_REGNUM);
  regcache->raw_write (regnum, buf);
  regcache->raw_write (regnum + 1, buf + 4);
}

/* Implement the stack_frame_destroyed_p gdbarch method.  */

int
sparc_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* This function must return true if we are one instruction after an
     instruction that destroyed the stack frame of the current
     function.  The SPARC instructions used to restore the callers
     stack frame are RESTORE and RETURN/RETT.

     Of these RETURN/RETT is a branch instruction and thus we return
     true if we are in its delay slot.

     RESTORE is almost always found in the delay slot of a branch
     instruction that transfers control to the caller, such as JMPL.
     Thus the next instruction is in the caller frame and we don't
     need to do anything about it.  */

  unsigned int insn = sparc_fetch_instruction (pc - 4);

  return X_RETTURN (insn);
}


static CORE_ADDR
sparc32_frame_align (struct gdbarch *gdbarch, CORE_ADDR address)
{
  /* The ABI requires double-word alignment.  */
  return address & ~0x7;
}

static CORE_ADDR
sparc32_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp,
			 CORE_ADDR funcaddr,
			 struct value **args, int nargs,
			 struct type *value_type,
			 CORE_ADDR *real_pc, CORE_ADDR *bp_addr,
			 struct regcache *regcache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  *bp_addr = sp - 4;
  *real_pc = funcaddr;

  if (using_struct_return (gdbarch, NULL, value_type))
    {
      gdb_byte buf[4];

      /* This is an UNIMP instruction.  */
      store_unsigned_integer (buf, 4, byte_order,
			      value_type->length () & 0x1fff);
      write_memory (sp - 8, buf, 4);
      return sp - 8;
    }

  return sp - 4;
}

static CORE_ADDR
sparc32_store_arguments (struct regcache *regcache, int nargs,
			 struct value **args, CORE_ADDR sp,
			 function_call_return_method return_method,
			 CORE_ADDR struct_addr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* Number of words in the "parameter array".  */
  int num_elements = 0;
  int element = 0;
  int i;

  for (i = 0; i < nargs; i++)
    {
      struct type *type = args[i]->type ();
      int len = type->length ();

      if (sparc_arg_by_memory_p (type))
	{
	  /* Structure, Union and Quad-Precision Arguments.  */
	  sp -= len;

	  /* Use doubleword alignment for these values.  That's always
	     correct, and wasting a few bytes shouldn't be a problem.  */
	  sp &= ~0x7;

	  write_memory (sp, args[i]->contents ().data (), len);
	  args[i] = value_from_pointer (lookup_pointer_type (type), sp);
	  num_elements++;
	}
      else if (sparc_floating_p (type))
	{
	  /* Floating arguments.  */
	  gdb_assert (len == 4 || len == 8);
	  num_elements += (len / 4);
	}
      else
	{
	  /* Arguments passed via the General Purpose Registers.  */
	  num_elements += ((len + 3) / 4);
	}
    }

  /* Always allocate at least six words.  */
  sp -= std::max (6, num_elements) * 4;

  /* The psABI says that "Software convention requires space for the
     struct/union return value pointer, even if the word is unused."  */
  sp -= 4;

  /* The psABI says that "Although software convention and the
     operating system require every stack frame to be doubleword
     aligned."  */
  sp &= ~0x7;

  for (i = 0; i < nargs; i++)
    {
      const bfd_byte *valbuf = args[i]->contents ().data ();
      struct type *type = args[i]->type ();
      int len = type->length ();
      gdb_byte buf[4];

      if (len < 4)
	{
	  memset (buf, 0, 4 - len);
	  memcpy (buf + 4 - len, valbuf, len);
	  valbuf = buf;
	  len = 4;
	}

      gdb_assert (len == 4 || len == 8);

      if (element < 6)
	{
	  int regnum = SPARC_O0_REGNUM + element;

	  regcache->cooked_write (regnum, valbuf);
	  if (len > 4 && element < 5)
	    regcache->cooked_write (regnum + 1, valbuf + 4);
	}

      /* Always store the argument in memory.  */
      write_memory (sp + 4 + element * 4, valbuf, len);
      element += len / 4;
    }

  gdb_assert (element == num_elements);

  if (return_method == return_method_struct)
    {
      gdb_byte buf[4];

      store_unsigned_integer (buf, 4, byte_order, struct_addr);
      write_memory (sp, buf, 4);
    }

  return sp;
}

static CORE_ADDR
sparc32_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			 struct regcache *regcache, CORE_ADDR bp_addr,
			 int nargs, struct value **args, CORE_ADDR sp,
			 function_call_return_method return_method,
			 CORE_ADDR struct_addr)
{
  CORE_ADDR call_pc = (return_method == return_method_struct
		       ? (bp_addr - 12) : (bp_addr - 8));

  /* Set return address.  */
  regcache_cooked_write_unsigned (regcache, SPARC_O7_REGNUM, call_pc);

  /* Set up function arguments.  */
  sp = sparc32_store_arguments (regcache, nargs, args, sp, return_method,
				struct_addr);

  /* Allocate the 16-word window save area.  */
  sp -= 16 * 4;

  /* Stack should be doubleword aligned at this point.  */
  gdb_assert (sp % 8 == 0);

  /* Finally, update the stack pointer.  */
  regcache_cooked_write_unsigned (regcache, SPARC_SP_REGNUM, sp);

  return sp;
}


/* Use the program counter to determine the contents and size of a
   breakpoint instruction.  Return a pointer to a string of bytes that
   encode a breakpoint instruction, store the length of the string in
   *LEN and optionally adjust *PC to point to the correct memory
   location for inserting the breakpoint.  */
constexpr gdb_byte sparc_break_insn[] = { 0x91, 0xd0, 0x20, 0x01 };

typedef BP_MANIPULATION (sparc_break_insn) sparc_breakpoint;


/* Allocate and initialize a frame cache.  */

static struct sparc_frame_cache *
sparc_alloc_frame_cache (void)
{
  struct sparc_frame_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct sparc_frame_cache);

  /* Base address.  */
  cache->base = 0;
  cache->pc = 0;

  /* Frameless until proven otherwise.  */
  cache->frameless_p = 1;
  cache->frame_offset = 0;
  cache->saved_regs_mask = 0;
  cache->copied_regs_mask = 0;
  cache->struct_return_p = 0;

  return cache;
}

/* GCC generates several well-known sequences of instructions at the begining
   of each function prologue when compiling with -fstack-check.  If one of
   such sequences starts at START_PC, then return the address of the
   instruction immediately past this sequence.  Otherwise, return START_PC.  */
   
static CORE_ADDR
sparc_skip_stack_check (const CORE_ADDR start_pc)
{
  CORE_ADDR pc = start_pc;
  unsigned long insn;
  int probing_loop = 0;

  /* With GCC, all stack checking sequences begin with the same two
     instructions, plus an optional one in the case of a probing loop:

	 sethi <some immediate>, %g1
	 sub %sp, %g1, %g1

     or:

	 sethi <some immediate>, %g1
	 sethi <some immediate>, %g4
	 sub %sp, %g1, %g1

     or:

	 sethi <some immediate>, %g1
	 sub %sp, %g1, %g1
	 sethi <some immediate>, %g4

     If the optional instruction is found (setting g4), assume that a
     probing loop will follow.  */

  /* sethi <some immediate>, %g1 */
  insn = sparc_fetch_instruction (pc);
  pc = pc + 4;
  if (!(X_OP (insn) == 0 && X_OP2 (insn) == 0x4 && X_RD (insn) == 1))
    return start_pc;

  /* optional: sethi <some immediate>, %g4 */
  insn = sparc_fetch_instruction (pc);
  pc = pc + 4;
  if (X_OP (insn) == 0 && X_OP2 (insn) == 0x4 && X_RD (insn) == 4)
    {
      probing_loop = 1;
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
    }

  /* sub %sp, %g1, %g1 */
  if (!(X_OP (insn) == 2 && X_OP3 (insn) == 0x4 && !X_I(insn)
	&& X_RD (insn) == 1 && X_RS1 (insn) == 14 && X_RS2 (insn) == 1))
    return start_pc;

  insn = sparc_fetch_instruction (pc);
  pc = pc + 4;

  /* optional: sethi <some immediate>, %g4 */
  if (X_OP (insn) == 0 && X_OP2 (insn) == 0x4 && X_RD (insn) == 4)
    {
      probing_loop = 1;
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
    }

  /* First possible sequence:
	 [first two instructions above]
	 clr [%g1 - some immediate]  */

  /* clr [%g1 - some immediate]  */
  if (X_OP (insn) == 3 && X_OP3(insn) == 0x4 && X_I(insn)
      && X_RS1 (insn) == 1 && X_RD (insn) == 0)
    {
      /* Valid stack-check sequence, return the new PC.  */
      return pc;
    }

  /* Second possible sequence: A small number of probes.
	 [first two instructions above]
	 clr [%g1]
	 add   %g1, -<some immediate>, %g1
	 clr [%g1]
	 [repeat the two instructions above any (small) number of times]
	 clr [%g1 - some immediate]  */

  /* clr [%g1] */
  else if (X_OP (insn) == 3 && X_OP3(insn) == 0x4 && !X_I(insn)
      && X_RS1 (insn) == 1 && X_RD (insn) == 0)
    {
      while (1)
	{
	  /* add %g1, -<some immediate>, %g1 */
	  insn = sparc_fetch_instruction (pc);
	  pc = pc + 4;
	  if (!(X_OP (insn) == 2  && X_OP3(insn) == 0 && X_I(insn)
		&& X_RS1 (insn) == 1 && X_RD (insn) == 1))
	    break;

	  /* clr [%g1] */
	  insn = sparc_fetch_instruction (pc);
	  pc = pc + 4;
	  if (!(X_OP (insn) == 3 && X_OP3(insn) == 0x4 && !X_I(insn)
		&& X_RD (insn) == 0 && X_RS1 (insn) == 1))
	    return start_pc;
	}

      /* clr [%g1 - some immediate] */
      if (!(X_OP (insn) == 3 && X_OP3(insn) == 0x4 && X_I(insn)
	    && X_RS1 (insn) == 1 && X_RD (insn) == 0))
	return start_pc;

      /* We found a valid stack-check sequence, return the new PC.  */
      return pc;
    }
  
  /* Third sequence: A probing loop.
	 [first three instructions above]
	 sub  %g1, %g4, %g4
	 cmp  %g1, %g4
	 be  <disp>
	 add  %g1, -<some immediate>, %g1
	 ba  <disp>
	 clr  [%g1]

     And an optional last probe for the remainder:

	 clr [%g4 - some immediate]  */

  if (probing_loop)
    {
      /* sub  %g1, %g4, %g4 */
      if (!(X_OP (insn) == 2 && X_OP3 (insn) == 0x4 && !X_I(insn)
	    && X_RD (insn) == 4 && X_RS1 (insn) == 1 && X_RS2 (insn) == 4))
	return start_pc;

      /* cmp  %g1, %g4 */
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
      if (!(X_OP (insn) == 2 && X_OP3 (insn) == 0x14 && !X_I(insn)
	    && X_RD (insn) == 0 && X_RS1 (insn) == 1 && X_RS2 (insn) == 4))
	return start_pc;

      /* be  <disp> */
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
      if (!(X_OP (insn) == 0 && X_COND (insn) == 0x1))
	return start_pc;

      /* add  %g1, -<some immediate>, %g1 */
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
      if (!(X_OP (insn) == 2  && X_OP3(insn) == 0 && X_I(insn)
	    && X_RS1 (insn) == 1 && X_RD (insn) == 1))
	return start_pc;

      /* ba  <disp> */
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
      if (!(X_OP (insn) == 0 && X_COND (insn) == 0x8))
	return start_pc;

      /* clr  [%g1] (st %g0, [%g1] or st %g0, [%g1+0]) */
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
      if (!(X_OP (insn) == 3 && X_OP3(insn) == 0x4
	    && X_RD (insn) == 0 && X_RS1 (insn) == 1
	    && (!X_I(insn) || X_SIMM13 (insn) == 0)))
	return start_pc;

      /* We found a valid stack-check sequence, return the new PC.  */

      /* optional: clr [%g4 - some immediate]  */
      insn = sparc_fetch_instruction (pc);
      pc = pc + 4;
      if (!(X_OP (insn) == 3 && X_OP3(insn) == 0x4 && X_I(insn)
	    && X_RS1 (insn) == 4 && X_RD (insn) == 0))
	return pc - 4;
      else
	return pc;
    }

  /* No stack check code in our prologue, return the start_pc.  */
  return start_pc;
}

/* Record the effect of a SAVE instruction on CACHE.  */

void
sparc_record_save_insn (struct sparc_frame_cache *cache)
{
  /* The frame is set up.  */
  cache->frameless_p = 0;

  /* The frame pointer contains the CFA.  */
  cache->frame_offset = 0;

  /* The `local' and `in' registers are all saved.  */
  cache->saved_regs_mask = 0xffff;

  /* The `out' registers are all renamed.  */
  cache->copied_regs_mask = 0xff;
}

/* Do a full analysis of the prologue at PC and update CACHE accordingly.
   Bail out early if CURRENT_PC is reached.  Return the address where
   the analysis stopped.

   We handle both the traditional register window model and the single
   register window (aka flat) model.  */

CORE_ADDR
sparc_analyze_prologue (struct gdbarch *gdbarch, CORE_ADDR pc,
			CORE_ADDR current_pc, struct sparc_frame_cache *cache)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);
  unsigned long insn;
  int offset = 0;
  int dest = -1;

  pc = sparc_skip_stack_check (pc);

  if (current_pc <= pc)
    return current_pc;

  /* We have to handle to "Procedure Linkage Table" (PLT) special.  On
     SPARC the linker usually defines a symbol (typically
     _PROCEDURE_LINKAGE_TABLE_) at the start of the .plt section.
     This symbol makes us end up here with PC pointing at the start of
     the PLT and CURRENT_PC probably pointing at a PLT entry.  If we
     would do our normal prologue analysis, we would probably conclude
     that we've got a frame when in reality we don't, since the
     dynamic linker patches up the first PLT with some code that
     starts with a SAVE instruction.  Patch up PC such that it points
     at the start of our PLT entry.  */
  if (tdep->plt_entry_size > 0 && in_plt_section (current_pc))
    pc = current_pc - ((current_pc - pc) % tdep->plt_entry_size);

  insn = sparc_fetch_instruction (pc);

  /* Recognize store insns and record their sources.  */
  while (X_OP (insn) == 3
	 && (X_OP3 (insn) == 0x4     /* stw */
	     || X_OP3 (insn) == 0x7  /* std */
	     || X_OP3 (insn) == 0xe) /* stx */
	 && X_RS1 (insn) == SPARC_SP_REGNUM)
    {
      int regnum = X_RD (insn);

      /* Recognize stores into the corresponding stack slots.  */
      if (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM
	  && ((X_I (insn)
	       && X_SIMM13 (insn) == (X_OP3 (insn) == 0xe
				      ? (regnum - SPARC_L0_REGNUM) * 8 + BIAS
				      : (regnum - SPARC_L0_REGNUM) * 4))
	      || (!X_I (insn) && regnum == SPARC_L0_REGNUM)))
	{
	  cache->saved_regs_mask |= (1 << (regnum - SPARC_L0_REGNUM));
	  if (X_OP3 (insn) == 0x7)
	    cache->saved_regs_mask |= (1 << (regnum + 1 - SPARC_L0_REGNUM));
	}

      offset += 4;

      insn = sparc_fetch_instruction (pc + offset);
    }

  /* Recognize a SETHI insn and record its destination.  */
  if (X_OP (insn) == 0 && X_OP2 (insn) == 0x04)
    {
      dest = X_RD (insn);
      offset += 4;

      insn = sparc_fetch_instruction (pc + offset);
    }

  /* Allow for an arithmetic operation on DEST or %g1.  */
  if (X_OP (insn) == 2 && X_I (insn)
      && (X_RD (insn) == 1 || X_RD (insn) == dest))
    {
      offset += 4;

      insn = sparc_fetch_instruction (pc + offset);
    }

  /* Check for the SAVE instruction that sets up the frame.  */
  if (X_OP (insn) == 2 && X_OP3 (insn) == 0x3c)
    {
      sparc_record_save_insn (cache);
      offset += 4;
      return pc + offset;
    }

  /* Check for an arithmetic operation on %sp.  */
  if (X_OP (insn) == 2
      && (X_OP3 (insn) == 0 || X_OP3 (insn) == 0x4)
      && X_RS1 (insn) == SPARC_SP_REGNUM
      && X_RD (insn) == SPARC_SP_REGNUM)
    {
      if (X_I (insn))
	{
	  cache->frame_offset = X_SIMM13 (insn);
	  if (X_OP3 (insn) == 0)
	    cache->frame_offset = -cache->frame_offset;
	}
      offset += 4;

      insn = sparc_fetch_instruction (pc + offset);

      /* Check for an arithmetic operation that sets up the frame.  */
      if (X_OP (insn) == 2
	  && (X_OP3 (insn) == 0 || X_OP3 (insn) == 0x4)
	  && X_RS1 (insn) == SPARC_SP_REGNUM
	  && X_RD (insn) == SPARC_FP_REGNUM)
	{
	  cache->frameless_p = 0;
	  cache->frame_offset = 0;
	  /* We could check that the amount subtracted to %sp above is the
	     same as the one added here, but this seems superfluous.  */
	  cache->copied_regs_mask |= 0x40;
	  offset += 4;

	  insn = sparc_fetch_instruction (pc + offset);
	}

      /* Check for a move (or) operation that copies the return register.  */
      if (X_OP (insn) == 2
	  && X_OP3 (insn) == 0x2
	  && !X_I (insn)
	  && X_RS1 (insn) == SPARC_G0_REGNUM
	  && X_RS2 (insn) == SPARC_O7_REGNUM
	  && X_RD (insn) == SPARC_I7_REGNUM)
	{
	   cache->copied_regs_mask |= 0x80;
	   offset += 4;
	}

      return pc + offset;
    }

  return pc;
}

/* Return PC of first real instruction of the function starting at
   START_PC.  */

static CORE_ADDR
sparc32_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  CORE_ADDR func_addr;
  struct sparc_frame_cache cache;

  /* This is the preferred method, find the end of the prologue by
     using the debugging information.  */

  if (find_pc_partial_function (start_pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (start_pc, post_prologue_pc);
    }

  start_pc = sparc_analyze_prologue (gdbarch, start_pc, 0xffffffffUL, &cache);

  /* The psABI says that "Although the first 6 words of arguments
     reside in registers, the standard stack frame reserves space for
     them.".  It also suggests that a function may use that space to
     "write incoming arguments 0 to 5" into that space, and that's
     indeed what GCC seems to be doing.  In that case GCC will
     generate debug information that points to the stack slots instead
     of the registers, so we should consider the instructions that
     write out these incoming arguments onto the stack.  */

  while (1)
    {
      unsigned long insn = sparc_fetch_instruction (start_pc);

      /* Recognize instructions that store incoming arguments into the
	 corresponding stack slots.  */
      if (X_OP (insn) == 3 && (X_OP3 (insn) & 0x3c) == 0x04
	  && X_I (insn) && X_RS1 (insn) == SPARC_FP_REGNUM)
	{
	  int regnum = X_RD (insn);

	  /* Case of arguments still in %o[0..5].  */
	  if (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O5_REGNUM
	      && !(cache.copied_regs_mask & (1 << (regnum - SPARC_O0_REGNUM)))
	      && X_SIMM13 (insn) == 68 + (regnum - SPARC_O0_REGNUM) * 4)
	    {
	      start_pc += 4;
	      continue;
	    }

	  /* Case of arguments copied into %i[0..5].  */
	  if (regnum >= SPARC_I0_REGNUM && regnum <= SPARC_I5_REGNUM
	      && (cache.copied_regs_mask & (1 << (regnum - SPARC_I0_REGNUM)))
	      && X_SIMM13 (insn) == 68 + (regnum - SPARC_I0_REGNUM) * 4)
	    {
	      start_pc += 4;
	      continue;
	    }
	}

      break;
    }

  return start_pc;
}

/* Normal frames.  */

struct sparc_frame_cache *
sparc_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct sparc_frame_cache *cache;

  if (*this_cache)
    return (struct sparc_frame_cache *) *this_cache;

  cache = sparc_alloc_frame_cache ();
  *this_cache = cache;

  cache->pc = get_frame_func (this_frame);
  if (cache->pc != 0)
    sparc_analyze_prologue (get_frame_arch (this_frame), cache->pc,
			    get_frame_pc (this_frame), cache);

  if (cache->frameless_p)
    {
      /* This function is frameless, so %fp (%i6) holds the frame
	 pointer for our calling frame.  Use %sp (%o6) as this frame's
	 base address.  */
      cache->base =
	get_frame_register_unsigned (this_frame, SPARC_SP_REGNUM);
    }
  else
    {
      /* For normal frames, %fp (%i6) holds the frame pointer, the
	 base address for the current stack frame.  */
      cache->base =
	get_frame_register_unsigned (this_frame, SPARC_FP_REGNUM);
    }

  cache->base += cache->frame_offset;

  if (cache->base & 1)
    cache->base += BIAS;

  return cache;
}

static int
sparc32_struct_return_from_sym (struct symbol *sym)
{
  struct type *type = check_typedef (sym->type ());
  enum type_code code = type->code ();

  if (code == TYPE_CODE_FUNC || code == TYPE_CODE_METHOD)
    {
      type = check_typedef (type->target_type ());
      if (sparc_structure_or_union_p (type)
	  || (sparc_floating_p (type) && type->length () == 16))
	return 1;
    }

  return 0;
}

struct sparc_frame_cache *
sparc32_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct sparc_frame_cache *cache;
  struct symbol *sym;

  if (*this_cache)
    return (struct sparc_frame_cache *) *this_cache;

  cache = sparc_frame_cache (this_frame, this_cache);

  sym = find_pc_function (cache->pc);
  if (sym)
    {
      cache->struct_return_p = sparc32_struct_return_from_sym (sym);
    }
  else
    {
      /* There is no debugging information for this function to
	 help us determine whether this function returns a struct
	 or not.  So we rely on another heuristic which is to check
	 the instruction at the return address and see if this is
	 an "unimp" instruction.  If it is, then it is a struct-return
	 function.  */
      CORE_ADDR pc;
      int regnum =
	(cache->copied_regs_mask & 0x80) ? SPARC_I7_REGNUM : SPARC_O7_REGNUM;

      pc = get_frame_register_unsigned (this_frame, regnum) + 8;
      if (sparc_is_unimp_insn (pc))
	cache->struct_return_p = 1;
    }

  return cache;
}

static void
sparc32_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		       struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc32_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc32_frame_prev_register (frame_info_ptr this_frame,
			     void **this_cache, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct sparc_frame_cache *cache =
    sparc32_frame_cache (this_frame, this_cache);

  if (regnum == SPARC32_PC_REGNUM || regnum == SPARC32_NPC_REGNUM)
    {
      CORE_ADDR pc = (regnum == SPARC32_NPC_REGNUM) ? 4 : 0;

      /* If this functions has a Structure, Union or Quad-Precision
	 return value, we have to skip the UNIMP instruction that encodes
	 the size of the structure.  */
      if (cache->struct_return_p)
	pc += 4;

      regnum =
	(cache->copied_regs_mask & 0x80) ? SPARC_I7_REGNUM : SPARC_O7_REGNUM;
      pc += get_frame_register_unsigned (this_frame, regnum) + 8;
      return frame_unwind_got_constant (this_frame, regnum, pc);
    }

  /* Handle StackGhost.  */
  {
    ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);

    if (wcookie != 0 && !cache->frameless_p && regnum == SPARC_I7_REGNUM)
      {
	CORE_ADDR addr = cache->base + (regnum - SPARC_L0_REGNUM) * 4;
	ULONGEST i7;

	/* Read the value in from memory.  */
	i7 = get_frame_memory_unsigned (this_frame, addr, 4);
	return frame_unwind_got_constant (this_frame, regnum, i7 ^ wcookie);
      }
  }

  /* The previous frame's `local' and `in' registers may have been saved
     in the register save area.  */
  if (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM
      && (cache->saved_regs_mask & (1 << (regnum - SPARC_L0_REGNUM))))
    {
      CORE_ADDR addr = cache->base + (regnum - SPARC_L0_REGNUM) * 4;

      return frame_unwind_got_memory (this_frame, regnum, addr);
    }

  /* The previous frame's `out' registers may be accessible as the current
     frame's `in' registers.  */
  if (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O7_REGNUM
      && (cache->copied_regs_mask & (1 << (regnum - SPARC_O0_REGNUM))))
    regnum += (SPARC_I0_REGNUM - SPARC_O0_REGNUM);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind sparc32_frame_unwind =
{
  "sparc32 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  sparc32_frame_this_id,
  sparc32_frame_prev_register,
  NULL,
  default_frame_sniffer
};


static CORE_ADDR
sparc32_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct sparc_frame_cache *cache =
    sparc32_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base sparc32_frame_base =
{
  &sparc32_frame_unwind,
  sparc32_frame_base_address,
  sparc32_frame_base_address,
  sparc32_frame_base_address
};

static struct frame_id
sparc_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR sp;

  sp = get_frame_register_unsigned (this_frame, SPARC_SP_REGNUM);
  if (sp & 1)
    sp += BIAS;
  return frame_id_build (sp, get_frame_pc (this_frame));
}


/* Extract a function return value of TYPE from REGCACHE, and copy
   that into VALBUF.  */

static void
sparc32_extract_return_value (struct type *type, struct regcache *regcache,
			      gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte buf[32];

  gdb_assert (!sparc_structure_return_p (type));

  if (sparc_floating_p (type) || sparc_complex_floating_p (type)
      || type->code () == TYPE_CODE_ARRAY)
    {
      /* Floating return values.  */
      regcache->cooked_read (SPARC_F0_REGNUM, buf);
      if (len > 4)
	regcache->cooked_read (SPARC_F1_REGNUM, buf + 4);
      if (len > 8)
	{
	  regcache->cooked_read (SPARC_F2_REGNUM, buf + 8);
	  regcache->cooked_read (SPARC_F3_REGNUM, buf + 12);
	}
      if (len > 16)
	{
	  regcache->cooked_read (SPARC_F4_REGNUM, buf + 16);
	  regcache->cooked_read (SPARC_F5_REGNUM, buf + 20);
	  regcache->cooked_read (SPARC_F6_REGNUM, buf + 24);
	  regcache->cooked_read (SPARC_F7_REGNUM, buf + 28);
	}
      memcpy (valbuf, buf, len);
    }
  else
    {
      /* Integral and pointer return values.  */
      gdb_assert (sparc_integral_or_pointer_p (type));

      regcache->cooked_read (SPARC_O0_REGNUM, buf);
      if (len > 4)
	{
	  regcache->cooked_read (SPARC_O1_REGNUM, buf + 4);
	  gdb_assert (len == 8);
	  memcpy (valbuf, buf, 8);
	}
      else
	{
	  /* Just stripping off any unused bytes should preserve the
	     signed-ness just fine.  */
	  memcpy (valbuf, buf + 4 - len, len);
	}
    }
}

/* Store the function return value of type TYPE from VALBUF into
   REGCACHE.  */

static void
sparc32_store_return_value (struct type *type, struct regcache *regcache,
			    const gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte buf[32];

  gdb_assert (!sparc_structure_return_p (type));

  if (sparc_floating_p (type) || sparc_complex_floating_p (type))
    {
      /* Floating return values.  */
      memcpy (buf, valbuf, len);
      regcache->cooked_write (SPARC_F0_REGNUM, buf);
      if (len > 4)
	regcache->cooked_write (SPARC_F1_REGNUM, buf + 4);
      if (len > 8)
	{
	  regcache->cooked_write (SPARC_F2_REGNUM, buf + 8);
	  regcache->cooked_write (SPARC_F3_REGNUM, buf + 12);
	}
      if (len > 16)
	{
	  regcache->cooked_write (SPARC_F4_REGNUM, buf + 16);
	  regcache->cooked_write (SPARC_F5_REGNUM, buf + 20);
	  regcache->cooked_write (SPARC_F6_REGNUM, buf + 24);
	  regcache->cooked_write (SPARC_F7_REGNUM, buf + 28);
	}
    }
  else
    {
      /* Integral and pointer return values.  */
      gdb_assert (sparc_integral_or_pointer_p (type));

      if (len > 4)
	{
	  gdb_assert (len == 8);
	  memcpy (buf, valbuf, 8);
	  regcache->cooked_write (SPARC_O1_REGNUM, buf + 4);
	}
      else
	{
	  /* ??? Do we need to do any sign-extension here?  */
	  memcpy (buf + 4 - len, valbuf, len);
	}
      regcache->cooked_write (SPARC_O0_REGNUM, buf);
    }
}

static enum return_value_convention
sparc32_return_value (struct gdbarch *gdbarch, struct value *function,
		      struct type *type, struct regcache *regcache,
		      struct value **read_value, const gdb_byte *writebuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* The psABI says that "...every stack frame reserves the word at
     %fp+64.  If a function returns a structure, union, or
     quad-precision value, this word should hold the address of the
     object into which the return value should be copied."  This
     guarantees that we can always find the return value, not just
     before the function returns.  */

  if (sparc_structure_return_p (type))
    {
      ULONGEST sp;
      CORE_ADDR addr;

      if (read_value != nullptr)
	{
	  regcache_cooked_read_unsigned (regcache, SPARC_SP_REGNUM, &sp);
	  addr = read_memory_unsigned_integer (sp + 64, 4, byte_order);
	  *read_value = value_at_non_lval (type, addr);
	}
      if (writebuf)
	{
	  regcache_cooked_read_unsigned (regcache, SPARC_SP_REGNUM, &sp);
	  addr = read_memory_unsigned_integer (sp + 64, 4, byte_order);
	  write_memory (addr, writebuf, type->length ());
	}

      return RETURN_VALUE_ABI_PRESERVES_ADDRESS;
    }

  if (read_value != nullptr)
    {
      *read_value = value::allocate (type);
      gdb_byte *readbuf = (*read_value)->contents_raw ().data ();
      sparc32_extract_return_value (type, regcache, readbuf);
    }
  if (writebuf)
    sparc32_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

static int
sparc32_stabs_argument_has_addr (struct gdbarch *gdbarch, struct type *type)
{
  return (sparc_structure_or_union_p (type)
	  || (sparc_floating_p (type) && type->length () == 16)
	  || sparc_complex_floating_p (type));
}

static int
sparc32_dwarf2_struct_return_p (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_address_in_block (this_frame);
  struct symbol *sym = find_pc_function (pc);

  if (sym)
    return sparc32_struct_return_from_sym (sym);
  return 0;
}

static void
sparc32_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       frame_info_ptr this_frame)
{
  int off;

  switch (regnum)
    {
    case SPARC_G0_REGNUM:
      /* Since %g0 is always zero, there is no point in saving it, and
	 people will be inclined omit it from the CFI.  Make sure we
	 don't warn about that.  */
      reg->how = DWARF2_FRAME_REG_SAME_VALUE;
      break;
    case SPARC_SP_REGNUM:
      reg->how = DWARF2_FRAME_REG_CFA;
      break;
    case SPARC32_PC_REGNUM:
    case SPARC32_NPC_REGNUM:
      reg->how = DWARF2_FRAME_REG_RA_OFFSET;
      off = 8;
      if (sparc32_dwarf2_struct_return_p (this_frame))
	off += 4;
      if (regnum == SPARC32_NPC_REGNUM)
	off += 4;
      reg->loc.offset = off;
      break;
    }
}

/* Implement the execute_dwarf_cfa_vendor_op method.  */

static bool
sparc_execute_dwarf_cfa_vendor_op (struct gdbarch *gdbarch, gdb_byte op,
				   struct dwarf2_frame_state *fs)
{
  /* Only DW_CFA_GNU_window_save is expected on SPARC.  */
  if (op != DW_CFA_GNU_window_save)
    return false;

  uint64_t reg;
  int size = register_size (gdbarch, 0);

  fs->regs.alloc_regs (32);
  for (reg = 8; reg < 16; reg++)
    {
      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_REG;
      fs->regs.reg[reg].loc.reg = reg + 16;
    }
  for (reg = 16; reg < 32; reg++)
    {
      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
      fs->regs.reg[reg].loc.offset = (reg - 16) * size;
    }

  return true;
}


/* The SPARC Architecture doesn't have hardware single-step support,
   and most operating systems don't implement it either, so we provide
   software single-step mechanism.  */

static CORE_ADDR
sparc_analyze_control_transfer (struct regcache *regcache,
				CORE_ADDR pc, CORE_ADDR *npc)
{
  unsigned long insn = sparc_fetch_instruction (pc);
  int conditional_p = X_COND (insn) & 0x7;
  int branch_p = 0, fused_p = 0;
  long offset = 0;			/* Must be signed for sign-extend.  */

  if (X_OP (insn) == 0 && X_OP2 (insn) == 3)
    {
      if ((insn & 0x10000000) == 0)
	{
	  /* Branch on Integer Register with Prediction (BPr).  */
	  branch_p = 1;
	  conditional_p = 1;
	}
      else
	{
	  /* Compare and Branch  */
	  branch_p = 1;
	  fused_p = 1;
	  offset = 4 * X_DISP10 (insn);
	}
    }
  else if (X_OP (insn) == 0 && X_OP2 (insn) == 6)
    {
      /* Branch on Floating-Point Condition Codes (FBfcc).  */
      branch_p = 1;
      offset = 4 * X_DISP22 (insn);
    }
  else if (X_OP (insn) == 0 && X_OP2 (insn) == 5)
    {
      /* Branch on Floating-Point Condition Codes with Prediction
	 (FBPfcc).  */
      branch_p = 1;
      offset = 4 * X_DISP19 (insn);
    }
  else if (X_OP (insn) == 0 && X_OP2 (insn) == 2)
    {
      /* Branch on Integer Condition Codes (Bicc).  */
      branch_p = 1;
      offset = 4 * X_DISP22 (insn);
    }
  else if (X_OP (insn) == 0 && X_OP2 (insn) == 1)
    {
      /* Branch on Integer Condition Codes with Prediction (BPcc).  */
      branch_p = 1;
      offset = 4 * X_DISP19 (insn);
    }
  else if (X_OP (insn) == 2 && X_OP3 (insn) == 0x3a)
    {
      frame_info_ptr frame = get_current_frame ();

      /* Trap instruction (TRAP).  */
      gdbarch *arch = regcache->arch ();
      sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (arch);
      return tdep->step_trap (frame, insn);
    }

  /* FIXME: Handle DONE and RETRY instructions.  */

  if (branch_p)
    {
      if (fused_p)
	{
	  /* Fused compare-and-branch instructions are non-delayed,
	     and do not have an annulling capability.  So we need to
	     always set a breakpoint on both the NPC and the branch
	     target address.  */
	  gdb_assert (offset != 0);
	  return pc + offset;
	}
      else if (conditional_p)
	{
	  /* For conditional branches, return nPC + 4 iff the annul
	     bit is 1.  */
	  return (X_A (insn) ? *npc + 4 : 0);
	}
      else
	{
	  /* For unconditional branches, return the target if its
	     specified condition is "always" and return nPC + 4 if the
	     condition is "never".  If the annul bit is 1, set *NPC to
	     zero.  */
	  if (X_COND (insn) == 0x0)
	    pc = *npc, offset = 4;
	  if (X_A (insn))
	    *npc = 0;

	  return pc + offset;
	}
    }

  return 0;
}

static CORE_ADDR
sparc_step_trap (frame_info_ptr frame, unsigned long insn)
{
  return 0;
}

static std::vector<CORE_ADDR>
sparc_software_single_step (struct regcache *regcache)
{
  struct gdbarch *arch = regcache->arch ();
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (arch);
  CORE_ADDR npc, nnpc;

  CORE_ADDR pc, orig_npc;
  std::vector<CORE_ADDR> next_pcs;

  pc = regcache_raw_get_unsigned (regcache, tdep->pc_regnum);
  orig_npc = npc = regcache_raw_get_unsigned (regcache, tdep->npc_regnum);

  /* Analyze the instruction at PC.  */
  nnpc = sparc_analyze_control_transfer (regcache, pc, &npc);
  if (npc != 0)
    next_pcs.push_back (npc);

  if (nnpc != 0)
    next_pcs.push_back (nnpc);

  /* Assert that we have set at least one breakpoint, and that
     they're not set at the same spot - unless we're going
     from here straight to NULL, i.e. a call or jump to 0.  */
  gdb_assert (npc != 0 || nnpc != 0 || orig_npc == 0);
  gdb_assert (nnpc != npc || orig_npc == 0);

  return next_pcs;
}

static void
sparc_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  gdbarch *arch = regcache->arch ();
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (arch);

  regcache_cooked_write_unsigned (regcache, tdep->pc_regnum, pc);
  regcache_cooked_write_unsigned (regcache, tdep->npc_regnum, pc + 4);
}


/* Iterate over core file register note sections.  */

static void
sparc_iterate_over_regset_sections (struct gdbarch *gdbarch,
				    iterate_over_regset_sections_cb *cb,
				    void *cb_data,
				    const struct regcache *regcache)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  cb (".reg", tdep->sizeof_gregset, tdep->sizeof_gregset, tdep->gregset, NULL,
      cb_data);
  cb (".reg2", tdep->sizeof_fpregset, tdep->sizeof_fpregset, tdep->fpregset,
      NULL, cb_data);
}


static int
validate_tdesc_registers (const struct target_desc *tdesc,
			  struct tdesc_arch_data *tdesc_data,
			  const char *feature_name,
			  const char * const register_names[],
			  unsigned int registers_num,
			  unsigned int reg_start)
{
  int valid_p = 1;
  const struct tdesc_feature *feature;

  feature = tdesc_find_feature (tdesc, feature_name);
  if (feature == NULL)
    return 0;

  for (unsigned int i = 0; i < registers_num; i++)
    valid_p &= tdesc_numbered_register (feature, tdesc_data,
					reg_start + i,
					register_names[i]);

  return valid_p;
}

static struct gdbarch *
sparc32_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  const struct target_desc *tdesc = info.target_desc;
  int valid_p = 1;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* Allocate space for the new architecture.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new sparc_gdbarch_tdep));
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  tdep->pc_regnum = SPARC32_PC_REGNUM;
  tdep->npc_regnum = SPARC32_NPC_REGNUM;
  tdep->step_trap = sparc_step_trap;
  tdep->fpu_register_names = sparc32_fpu_register_names;
  tdep->fpu_registers_num = ARRAY_SIZE (sparc32_fpu_register_names);
  tdep->cp0_register_names = sparc32_cp0_register_names;
  tdep->cp0_registers_num = ARRAY_SIZE (sparc32_cp0_register_names);

  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);

  set_gdbarch_wchar_bit (gdbarch, 16);
  set_gdbarch_wchar_signed (gdbarch, 1);

  set_gdbarch_num_regs (gdbarch, SPARC32_NUM_REGS);
  set_gdbarch_register_name (gdbarch, sparc32_register_name);
  set_gdbarch_register_type (gdbarch, sparc32_register_type);
  set_gdbarch_num_pseudo_regs (gdbarch, SPARC32_NUM_PSEUDO_REGS);
  set_tdesc_pseudo_register_name (gdbarch, sparc32_pseudo_register_name);
  set_tdesc_pseudo_register_type (gdbarch, sparc32_pseudo_register_type);
  set_gdbarch_pseudo_register_read (gdbarch, sparc32_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						sparc32_pseudo_register_write);

  /* Register numbers of various important registers.  */
  set_gdbarch_sp_regnum (gdbarch, SPARC_SP_REGNUM); /* %sp */
  set_gdbarch_pc_regnum (gdbarch, SPARC32_PC_REGNUM); /* %pc */
  set_gdbarch_fp0_regnum (gdbarch, SPARC_F0_REGNUM); /* %f0 */

  /* Call dummy code.  */
  set_gdbarch_frame_align (gdbarch, sparc32_frame_align);
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_push_dummy_code (gdbarch, sparc32_push_dummy_code);
  set_gdbarch_push_dummy_call (gdbarch, sparc32_push_dummy_call);

  set_gdbarch_return_value_as_value (gdbarch, sparc32_return_value);
  set_gdbarch_stabs_argument_has_addr
    (gdbarch, sparc32_stabs_argument_has_addr);

  set_gdbarch_skip_prologue (gdbarch, sparc32_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       sparc_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       sparc_breakpoint::bp_from_kind);

  set_gdbarch_frame_args_skip (gdbarch, 8);

  set_gdbarch_software_single_step (gdbarch, sparc_software_single_step);
  set_gdbarch_write_pc (gdbarch, sparc_write_pc);

  set_gdbarch_dummy_id (gdbarch, sparc_dummy_id);

  frame_base_set_default (gdbarch, &sparc32_frame_base);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_frame_set_init_reg (gdbarch, sparc32_dwarf2_frame_init_reg);
  /* Register DWARF vendor CFI handler.  */
  set_gdbarch_execute_dwarf_cfa_vendor_op (gdbarch,
					   sparc_execute_dwarf_cfa_vendor_op);
  /* FIXME: kettenis/20050423: Don't enable the unwinder until the
     StackGhost issues have been resolved.  */

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  frame_unwind_append_unwinder (gdbarch, &sparc32_frame_unwind);

  if (tdesc_has_registers (tdesc))
    {
      tdesc_arch_data_up tdesc_data = tdesc_data_alloc ();

      /* Validate that the descriptor provides the mandatory registers
	 and allocate their numbers. */
      valid_p &= validate_tdesc_registers (tdesc, tdesc_data.get (),
					   "org.gnu.gdb.sparc.cpu",
					   sparc_core_register_names,
					   ARRAY_SIZE (sparc_core_register_names),
					   SPARC_G0_REGNUM);
      valid_p &= validate_tdesc_registers (tdesc, tdesc_data.get (),
					   "org.gnu.gdb.sparc.fpu",
					   tdep->fpu_register_names,
					   tdep->fpu_registers_num,
					   SPARC_F0_REGNUM);
      valid_p &= validate_tdesc_registers (tdesc, tdesc_data.get (),
					   "org.gnu.gdb.sparc.cp0",
					   tdep->cp0_register_names,
					   tdep->cp0_registers_num,
					   SPARC_F0_REGNUM
					   + tdep->fpu_registers_num);
      if (!valid_p)
	return NULL;

      /* Target description may have changed. */
      info.tdesc_data = tdesc_data.get ();
      tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));
    }

  /* If we have register sets, enable the generic core file support.  */
  if (tdep->gregset)
    set_gdbarch_iterate_over_regset_sections
      (gdbarch, sparc_iterate_over_regset_sections);

  register_sparc_ravenscar_ops (gdbarch);

  return gdbarch;
}

/* Helper functions for dealing with register windows.  */

void
sparc_supply_rwindow (struct regcache *regcache, CORE_ADDR sp, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int offset = 0;
  gdb_byte buf[8];
  int i;

  /* This function calls functions that depend on the global current thread.  */
  gdb_assert (regcache->ptid () == inferior_ptid);

  if (sp & 1)
    {
      /* Registers are 64-bit.  */
      sp += BIAS;

      for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	{
	  if (regnum == i || regnum == -1)
	    {
	      target_read_memory (sp + ((i - SPARC_L0_REGNUM) * 8), buf, 8);

	      /* Handle StackGhost.  */
	      if (i == SPARC_I7_REGNUM)
		{
		  ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);
		  ULONGEST i7;

		  i7 = extract_unsigned_integer (buf + offset, 8, byte_order);
		  store_unsigned_integer (buf + offset, 8, byte_order,
					  i7 ^ wcookie);
		}

	      regcache->raw_supply (i, buf);
	    }
	}
    }
  else
    {
      /* Registers are 32-bit.  Toss any sign-extension of the stack
	 pointer.  */
      sp &= 0xffffffffUL;

      /* Clear out the top half of the temporary buffer, and put the
	 register value in the bottom half if we're in 64-bit mode.  */
      if (gdbarch_ptr_bit (regcache->arch ()) == 64)
	{
	  memset (buf, 0, 4);
	  offset = 4;
	}

      for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	{
	  if (regnum == i || regnum == -1)
	    {
	      target_read_memory (sp + ((i - SPARC_L0_REGNUM) * 4),
				  buf + offset, 4);

	      /* Handle StackGhost.  */
	      if (i == SPARC_I7_REGNUM)
		{
		  ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);
		  ULONGEST i7;

		  i7 = extract_unsigned_integer (buf + offset, 4, byte_order);
		  store_unsigned_integer (buf + offset, 4, byte_order,
					  i7 ^ wcookie);
		}

	      regcache->raw_supply (i, buf);
	    }
	}
    }
}

void
sparc_collect_rwindow (const struct regcache *regcache,
		       CORE_ADDR sp, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int offset = 0;
  gdb_byte buf[8];
  int i;

  /* This function calls functions that depend on the global current thread.  */
  gdb_assert (regcache->ptid () == inferior_ptid);

  if (sp & 1)
    {
      /* Registers are 64-bit.  */
      sp += BIAS;

      for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	{
	  if (regnum == -1 || regnum == SPARC_SP_REGNUM || regnum == i)
	    {
	      regcache->raw_collect (i, buf);

	      /* Handle StackGhost.  */
	      if (i == SPARC_I7_REGNUM)
		{
		  ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);
		  ULONGEST i7;

		  i7 = extract_unsigned_integer (buf + offset, 8, byte_order);
		  store_unsigned_integer (buf, 8, byte_order, i7 ^ wcookie);
		}

	      target_write_memory (sp + ((i - SPARC_L0_REGNUM) * 8), buf, 8);
	    }
	}
    }
  else
    {
      /* Registers are 32-bit.  Toss any sign-extension of the stack
	 pointer.  */
      sp &= 0xffffffffUL;

      /* Only use the bottom half if we're in 64-bit mode.  */
      if (gdbarch_ptr_bit (regcache->arch ()) == 64)
	offset = 4;

      for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	{
	  if (regnum == -1 || regnum == SPARC_SP_REGNUM || regnum == i)
	    {
	      regcache->raw_collect (i, buf);

	      /* Handle StackGhost.  */
	      if (i == SPARC_I7_REGNUM)
		{
		  ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);
		  ULONGEST i7;

		  i7 = extract_unsigned_integer (buf + offset, 4, byte_order);
		  store_unsigned_integer (buf + offset, 4, byte_order,
					  i7 ^ wcookie);
		}

	      target_write_memory (sp + ((i - SPARC_L0_REGNUM) * 4),
				   buf + offset, 4);
	    }
	}
    }
}

/* Helper functions for dealing with register sets.  */

void
sparc32_supply_gregset (const struct sparc_gregmap *gregmap,
			struct regcache *regcache,
			int regnum, const void *gregs)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  gdb_byte zero[4] = { 0 };
  int i;

  if (regnum == SPARC32_PSR_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC32_PSR_REGNUM, regs + gregmap->r_psr_offset);

  if (regnum == SPARC32_PC_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC32_PC_REGNUM, regs + gregmap->r_pc_offset);

  if (regnum == SPARC32_NPC_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC32_NPC_REGNUM, regs + gregmap->r_npc_offset);

  if (regnum == SPARC32_Y_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC32_Y_REGNUM, regs + gregmap->r_y_offset);

  if (regnum == SPARC_G0_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC_G0_REGNUM, &zero);

  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_O7_REGNUM) || regnum == -1)
    {
      int offset = gregmap->r_g1_offset;

      for (i = SPARC_G1_REGNUM; i <= SPARC_O7_REGNUM; i++)
	{
	  if (regnum == i || regnum == -1)
	    regcache->raw_supply (i, regs + offset);
	  offset += 4;
	}
    }

  if ((regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM) || regnum == -1)
    {
      /* Not all of the register set variants include Locals and
	 Inputs.  For those that don't, we read them off the stack.  */
      if (gregmap->r_l0_offset == -1)
	{
	  ULONGEST sp;

	  regcache_cooked_read_unsigned (regcache, SPARC_SP_REGNUM, &sp);
	  sparc_supply_rwindow (regcache, sp, regnum);
	}
      else
	{
	  int offset = gregmap->r_l0_offset;

	  for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	    {
	      if (regnum == i || regnum == -1)
		regcache->raw_supply (i, regs + offset);
	      offset += 4;
	    }
	}
    }
}

void
sparc32_collect_gregset (const struct sparc_gregmap *gregmap,
			 const struct regcache *regcache,
			 int regnum, void *gregs)
{
  gdb_byte *regs = (gdb_byte *) gregs;
  int i;

  if (regnum == SPARC32_PSR_REGNUM || regnum == -1)
    regcache->raw_collect (SPARC32_PSR_REGNUM, regs + gregmap->r_psr_offset);

  if (regnum == SPARC32_PC_REGNUM || regnum == -1)
    regcache->raw_collect (SPARC32_PC_REGNUM, regs + gregmap->r_pc_offset);

  if (regnum == SPARC32_NPC_REGNUM || regnum == -1)
    regcache->raw_collect (SPARC32_NPC_REGNUM, regs + gregmap->r_npc_offset);

  if (regnum == SPARC32_Y_REGNUM || regnum == -1)
    regcache->raw_collect (SPARC32_Y_REGNUM, regs + gregmap->r_y_offset);

  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_O7_REGNUM) || regnum == -1)
    {
      int offset = gregmap->r_g1_offset;

      /* %g0 is always zero.  */
      for (i = SPARC_G1_REGNUM; i <= SPARC_O7_REGNUM; i++)
	{
	  if (regnum == i || regnum == -1)
	    regcache->raw_collect (i, regs + offset);
	  offset += 4;
	}
    }

  if ((regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM) || regnum == -1)
    {
      /* Not all of the register set variants include Locals and
	 Inputs.  For those that don't, we read them off the stack.  */
      if (gregmap->r_l0_offset != -1)
	{
	  int offset = gregmap->r_l0_offset;

	  for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	    {
	      if (regnum == i || regnum == -1)
		regcache->raw_collect (i, regs + offset);
	      offset += 4;
	    }
	}
    }
}

void
sparc32_supply_fpregset (const struct sparc_fpregmap *fpregmap,
			 struct regcache *regcache,
			 int regnum, const void *fpregs)
{
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i;

  for (i = 0; i < 32; i++)
    {
      if (regnum == (SPARC_F0_REGNUM + i) || regnum == -1)
	regcache->raw_supply (SPARC_F0_REGNUM + i,
			      regs + fpregmap->r_f0_offset + (i * 4));
    }

  if (regnum == SPARC32_FSR_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC32_FSR_REGNUM, regs + fpregmap->r_fsr_offset);
}

void
sparc32_collect_fpregset (const struct sparc_fpregmap *fpregmap,
			  const struct regcache *regcache,
			  int regnum, void *fpregs)
{
  gdb_byte *regs = (gdb_byte *) fpregs;
  int i;

  for (i = 0; i < 32; i++)
    {
      if (regnum == (SPARC_F0_REGNUM + i) || regnum == -1)
	regcache->raw_collect (SPARC_F0_REGNUM + i,
			       regs + fpregmap->r_f0_offset + (i * 4));
    }

  if (regnum == SPARC32_FSR_REGNUM || regnum == -1)
    regcache->raw_collect (SPARC32_FSR_REGNUM,
			   regs + fpregmap->r_fsr_offset);
}


/* SunOS 4.  */

/* From <machine/reg.h>.  */
const struct sparc_gregmap sparc32_sunos4_gregmap =
{
  0 * 4,			/* %psr */
  1 * 4,			/* %pc */
  2 * 4,			/* %npc */
  3 * 4,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  4 * 4,			/* %g1 */
  -1				/* %l0 */
};

const struct sparc_fpregmap sparc32_sunos4_fpregmap =
{
  0 * 4,			/* %f0 */
  33 * 4,			/* %fsr */
};

const struct sparc_fpregmap sparc32_bsd_fpregmap =
{
  0 * 4,			/* %f0 */
  32 * 4,			/* %fsr */
};

void _initialize_sparc_tdep ();
void
_initialize_sparc_tdep ()
{
  gdbarch_register (bfd_arch_sparc, sparc32_gdbarch_init);
}

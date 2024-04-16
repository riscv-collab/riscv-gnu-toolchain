/* Target-dependent code for the ALPHA architecture, for GDB, the GNU Debugger.

   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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
#include "frame-unwind.h"
#include "frame-base.h"
#include "dwarf2/frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "dis-asm.h"
#include "symfile.h"
#include "objfiles.h"
#include "linespec.h"
#include "regcache.h"
#include "reggroups.h"
#include "arch-utils.h"
#include "osabi.h"
#include "infcall.h"
#include "trad-frame.h"

#include "elf-bfd.h"

#include "alpha-tdep.h"
#include <algorithm>

/* Instruction decoding.  The notations for registers, immediates and
   opcodes are the same as the one used in Compaq's Alpha architecture
   handbook.  */

#define INSN_OPCODE(insn) ((insn & 0xfc000000) >> 26)

/* Memory instruction format */
#define MEM_RA(insn) ((insn & 0x03e00000) >> 21)
#define MEM_RB(insn) ((insn & 0x001f0000) >> 16)
#define MEM_DISP(insn) \
  (((insn & 0x8000) == 0) ? (insn & 0xffff) : -((-insn) & 0xffff))

static const int lda_opcode = 0x08;
static const int stq_opcode = 0x2d;

/* Branch instruction format */
#define BR_RA(insn) MEM_RA(insn)

static const int br_opcode = 0x30;
static const int bne_opcode = 0x3d;

/* Operate instruction format */
#define OPR_FUNCTION(insn) ((insn & 0xfe0) >> 5)
#define OPR_HAS_IMMEDIATE(insn) ((insn & 0x1000) == 0x1000)
#define OPR_RA(insn) MEM_RA(insn)
#define OPR_RC(insn) ((insn & 0x1f))
#define OPR_LIT(insn) ((insn & 0x1fe000) >> 13)

static const int subq_opcode = 0x10;
static const int subq_function = 0x29;


/* Return the name of the REGNO register.

   An empty name corresponds to a register number that used to
   be used for a virtual register.  That virtual register has
   been removed, but the index is still reserved to maintain
   compatibility with existing remote alpha targets.  */

static const char *
alpha_register_name (struct gdbarch *gdbarch, int regno)
{
  static const char * const register_names[] =
  {
    "v0",   "t0",   "t1",   "t2",   "t3",   "t4",   "t5",   "t6",
    "t7",   "s0",   "s1",   "s2",   "s3",   "s4",   "s5",   "fp",
    "a0",   "a1",   "a2",   "a3",   "a4",   "a5",   "t8",   "t9",
    "t10",  "t11",  "ra",   "t12",  "at",   "gp",   "sp",   "zero",
    "f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
    "f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
    "f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
    "f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "fpcr",
    "pc",   "",     "unique"
  };

  static_assert (ALPHA_NUM_REGS == ARRAY_SIZE (register_names));
  return register_names[regno];
}

static int
alpha_cannot_fetch_register (struct gdbarch *gdbarch, int regno)
{
  return (strlen (alpha_register_name (gdbarch, regno)) == 0);
}

static int
alpha_cannot_store_register (struct gdbarch *gdbarch, int regno)
{
  return (regno == ALPHA_ZERO_REGNUM
	  || strlen (alpha_register_name (gdbarch, regno)) == 0);
}

static struct type *
alpha_register_type (struct gdbarch *gdbarch, int regno)
{
  if (regno == ALPHA_SP_REGNUM || regno == ALPHA_GP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  if (regno == ALPHA_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;

  /* Don't need to worry about little vs big endian until 
     some jerk tries to port to alpha-unicosmk.  */
  if (regno >= ALPHA_FP0_REGNUM && regno < ALPHA_FP0_REGNUM + 31)
    return builtin_type (gdbarch)->builtin_double;

  return builtin_type (gdbarch)->builtin_int64;
}

/* Is REGNUM a member of REGGROUP?  */

static int
alpha_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			   const struct reggroup *group)
{
  /* Filter out any registers eliminated, but whose regnum is 
     reserved for backward compatibility, e.g. the vfp.  */
  if (*gdbarch_register_name (gdbarch, regnum) == '\0')
    return 0;

  if (group == all_reggroup)
    return 1;

  /* Zero should not be saved or restored.  Technically it is a general
     register (just as $f31 would be a float if we represented it), but
     there's no point displaying it during "info regs", so leave it out
     of all groups except for "all".  */
  if (regnum == ALPHA_ZERO_REGNUM)
    return 0;

  /* All other registers are saved and restored.  */
  if (group == save_reggroup || group == restore_reggroup)
    return 1;

  /* All other groups are non-overlapping.  */

  /* Since this is really a PALcode memory slot...  */
  if (regnum == ALPHA_UNIQUE_REGNUM)
    return group == system_reggroup;

  /* Force the FPCR to be considered part of the floating point state.  */
  if (regnum == ALPHA_FPCR_REGNUM)
    return group == float_reggroup;

  if (regnum >= ALPHA_FP0_REGNUM && regnum < ALPHA_FP0_REGNUM + 31)
    return group == float_reggroup;
  else
    return group == general_reggroup;
}

/* The following represents exactly the conversion performed by
   the LDS instruction.  This applies to both single-precision
   floating point and 32-bit integers.  */

static void
alpha_lds (struct gdbarch *gdbarch, void *out, const void *in)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST mem
    = extract_unsigned_integer ((const gdb_byte *) in, 4, byte_order);
  ULONGEST frac    = (mem >>  0) & 0x7fffff;
  ULONGEST sign    = (mem >> 31) & 1;
  ULONGEST exp_msb = (mem >> 30) & 1;
  ULONGEST exp_low = (mem >> 23) & 0x7f;
  ULONGEST exp, reg;

  exp = (exp_msb << 10) | exp_low;
  if (exp_msb)
    {
      if (exp_low == 0x7f)
	exp = 0x7ff;
    }
  else
    {
      if (exp_low != 0x00)
	exp |= 0x380;
    }

  reg = (sign << 63) | (exp << 52) | (frac << 29);
  store_unsigned_integer ((gdb_byte *) out, 8, byte_order, reg);
}

/* Similarly, this represents exactly the conversion performed by
   the STS instruction.  */

static void
alpha_sts (struct gdbarch *gdbarch, void *out, const void *in)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST reg, mem;

  reg = extract_unsigned_integer ((const gdb_byte *) in, 8, byte_order);
  mem = ((reg >> 32) & 0xc0000000) | ((reg >> 29) & 0x3fffffff);
  store_unsigned_integer ((gdb_byte *) out, 4, byte_order, mem);
}

/* The alpha needs a conversion between register and memory format if the
   register is a floating point register and memory format is float, as the
   register format must be double or memory format is an integer with 4
   bytes, as the representation of integers in floating point
   registers is different.  */

static int
alpha_convert_register_p (struct gdbarch *gdbarch, int regno,
			  struct type *type)
{
  return (regno >= ALPHA_FP0_REGNUM && regno < ALPHA_FP0_REGNUM + 31
	  && type->length () == 4);
}

static int
alpha_register_to_value (frame_info_ptr frame, int regnum,
			 struct type *valtype, gdb_byte *out,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct value *value = get_frame_register_value (frame, regnum);

  gdb_assert (value != NULL);
  *optimizedp = value->optimized_out ();
  *unavailablep = !value->entirely_available ();

  if (*optimizedp || *unavailablep)
    {
      release_value (value);
      return 0;
    }

  /* Convert to VALTYPE.  */

  gdb_assert (valtype->length () == 4);
  alpha_sts (gdbarch, out, value->contents_all ().data ());

  release_value (value);
  return 1;
}

static void
alpha_value_to_register (frame_info_ptr frame, int regnum,
			 struct type *valtype, const gdb_byte *in)
{
  int reg_size = register_size (get_frame_arch (frame), regnum);
  gdb_assert (valtype->length () == 4);
  gdb_assert (reg_size <= ALPHA_REGISTER_SIZE);

  gdb_byte out[ALPHA_REGISTER_SIZE];
  alpha_lds (get_frame_arch (frame), out, in);

  auto out_view = gdb::make_array_view (out, reg_size);
  put_frame_register (get_next_frame_sentinel_okay (frame), regnum, out_view);
}


/* The alpha passes the first six arguments in the registers, the rest on
   the stack.  The register arguments are stored in ARG_REG_BUFFER, and
   then moved into the register file; this simplifies the passing of a
   large struct which extends from the registers to the stack, plus avoids
   three ptrace invocations per word.

   We don't bother tracking which register values should go in integer
   regs or fp regs; we load the same values into both.

   If the called function is returning a structure, the address of the
   structure to be returned is passed as a hidden first argument.  */

static CORE_ADDR
alpha_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		       struct regcache *regcache, CORE_ADDR bp_addr,
		       int nargs, struct value **args, CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int i;
  int accumulate_size = (return_method == return_method_struct) ? 8 : 0;
  struct alpha_arg
    {
      const gdb_byte *contents;
      int len;
      int offset;
    };
  struct alpha_arg *alpha_args = XALLOCAVEC (struct alpha_arg, nargs);
  struct alpha_arg *m_arg;
  gdb_byte arg_reg_buffer[ALPHA_REGISTER_SIZE * ALPHA_NUM_ARG_REGS];
  int required_arg_regs;
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* The ABI places the address of the called function in T12.  */
  regcache_cooked_write_signed (regcache, ALPHA_T12_REGNUM, func_addr);

  /* Set the return address register to point to the entry point
     of the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, ALPHA_RA_REGNUM, bp_addr);

  /* Lay out the arguments in memory.  */
  for (i = 0, m_arg = alpha_args; i < nargs; i++, m_arg++)
    {
      struct value *arg = args[i];
      struct type *arg_type = check_typedef (arg->type ());

      /* Cast argument to long if necessary as the compiler does it too.  */
      switch (arg_type->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	  if (arg_type->length () == 4)
	    {
	      /* 32-bit values must be sign-extended to 64 bits
		 even if the base data type is unsigned.  */
	      arg_type = builtin_type (gdbarch)->builtin_int32;
	      arg = value_cast (arg_type, arg);
	    }
	  if (arg_type->length () < ALPHA_REGISTER_SIZE)
	    {
	      arg_type = builtin_type (gdbarch)->builtin_int64;
	      arg = value_cast (arg_type, arg);
	    }
	  break;

	case TYPE_CODE_FLT:
	  /* "float" arguments loaded in registers must be passed in
	     register format, aka "double".  */
	  if (accumulate_size < sizeof (arg_reg_buffer)
	      && arg_type->length () == 4)
	    {
	      arg_type = builtin_type (gdbarch)->builtin_double;
	      arg = value_cast (arg_type, arg);
	    }
	  /* Tru64 5.1 has a 128-bit long double, and passes this by
	     invisible reference.  No one else uses this data type.  */
	  else if (arg_type->length () == 16)
	    {
	      /* Allocate aligned storage.  */
	      sp = (sp & -16) - 16;

	      /* Write the real data into the stack.  */
	      write_memory (sp, arg->contents ().data (), 16);

	      /* Construct the indirection.  */
	      arg_type = lookup_pointer_type (arg_type);
	      arg = value_from_pointer (arg_type, sp);
	    }
	  break;

	case TYPE_CODE_COMPLEX:
	  /* ??? The ABI says that complex values are passed as two
	     separate scalar values.  This distinction only matters
	     for complex float.  However, GCC does not implement this.  */

	  /* Tru64 5.1 has a 128-bit long double, and passes this by
	     invisible reference.  */
	  if (arg_type->length () == 32)
	    {
	      /* Allocate aligned storage.  */
	      sp = (sp & -16) - 16;

	      /* Write the real data into the stack.  */
	      write_memory (sp, arg->contents ().data (), 32);

	      /* Construct the indirection.  */
	      arg_type = lookup_pointer_type (arg_type);
	      arg = value_from_pointer (arg_type, sp);
	    }
	  break;

	default:
	  break;
	}
      m_arg->len = arg_type->length ();
      m_arg->offset = accumulate_size;
      accumulate_size = (accumulate_size + m_arg->len + 7) & ~7;
      m_arg->contents = arg->contents ().data ();
    }

  /* Determine required argument register loads, loading an argument register
     is expensive as it uses three ptrace calls.  */
  required_arg_regs = accumulate_size / 8;
  if (required_arg_regs > ALPHA_NUM_ARG_REGS)
    required_arg_regs = ALPHA_NUM_ARG_REGS;

  /* Make room for the arguments on the stack.  */
  if (accumulate_size < sizeof(arg_reg_buffer))
    accumulate_size = 0;
  else
    accumulate_size -= sizeof(arg_reg_buffer);
  sp -= accumulate_size;

  /* Keep sp aligned to a multiple of 16 as the ABI requires.  */
  sp &= ~15;

  /* `Push' arguments on the stack.  */
  for (i = nargs; m_arg--, --i >= 0;)
    {
      const gdb_byte *contents = m_arg->contents;
      int offset = m_arg->offset;
      int len = m_arg->len;

      /* Copy the bytes destined for registers into arg_reg_buffer.  */
      if (offset < sizeof(arg_reg_buffer))
	{
	  if (offset + len <= sizeof(arg_reg_buffer))
	    {
	      memcpy (arg_reg_buffer + offset, contents, len);
	      continue;
	    }
	  else
	    {
	      int tlen = sizeof(arg_reg_buffer) - offset;
	      memcpy (arg_reg_buffer + offset, contents, tlen);
	      offset += tlen;
	      contents += tlen;
	      len -= tlen;
	    }
	}

      /* Everything else goes to the stack.  */
      write_memory (sp + offset - sizeof(arg_reg_buffer), contents, len);
    }
  if (return_method == return_method_struct)
    store_unsigned_integer (arg_reg_buffer, ALPHA_REGISTER_SIZE,
			    byte_order, struct_addr);

  /* Load the argument registers.  */
  for (i = 0; i < required_arg_regs; i++)
    {
      regcache->cooked_write (ALPHA_A0_REGNUM + i,
			      arg_reg_buffer + i * ALPHA_REGISTER_SIZE);
      regcache->cooked_write (ALPHA_FPA0_REGNUM + i,
			      arg_reg_buffer + i * ALPHA_REGISTER_SIZE);
    }

  /* Finally, update the stack pointer.  */
  regcache_cooked_write_signed (regcache, ALPHA_SP_REGNUM, sp);

  return sp;
}

/* Extract from REGCACHE the value about to be returned from a function
   and copy it into VALBUF.  */

static void
alpha_extract_return_value (struct type *valtype, struct regcache *regcache,
			    gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte raw_buffer[ALPHA_REGISTER_SIZE];
  ULONGEST l;

  switch (valtype->code ())
    {
    case TYPE_CODE_FLT:
      switch (valtype->length ())
	{
	case 4:
	  regcache->cooked_read (ALPHA_FP0_REGNUM, raw_buffer);
	  alpha_sts (gdbarch, valbuf, raw_buffer);
	  break;

	case 8:
	  regcache->cooked_read (ALPHA_FP0_REGNUM, valbuf);
	  break;

	case 16:
	  regcache_cooked_read_unsigned (regcache, ALPHA_V0_REGNUM, &l);
	  read_memory (l, valbuf, 16);
	  break;

	default:
	  internal_error (_("unknown floating point width"));
	}
      break;

    case TYPE_CODE_COMPLEX:
      switch (valtype->length ())
	{
	case 8:
	  /* ??? This isn't correct wrt the ABI, but it's what GCC does.  */
	  regcache->cooked_read (ALPHA_FP0_REGNUM, valbuf);
	  break;

	case 16:
	  regcache->cooked_read (ALPHA_FP0_REGNUM, valbuf);
	  regcache->cooked_read (ALPHA_FP0_REGNUM + 1, valbuf + 8);
	  break;

	case 32:
	  regcache_cooked_read_unsigned (regcache, ALPHA_V0_REGNUM, &l);
	  read_memory (l, valbuf, 32);
	  break;

	default:
	  internal_error (_("unknown floating point width"));
	}
      break;

    default:
      /* Assume everything else degenerates to an integer.  */
      regcache_cooked_read_unsigned (regcache, ALPHA_V0_REGNUM, &l);
      store_unsigned_integer (valbuf, valtype->length (), byte_order, l);
      break;
    }
}

/* Insert the given value into REGCACHE as if it was being 
   returned by a function.  */

static void
alpha_store_return_value (struct type *valtype, struct regcache *regcache,
			  const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  gdb_byte raw_buffer[ALPHA_REGISTER_SIZE];
  ULONGEST l;

  switch (valtype->code ())
    {
    case TYPE_CODE_FLT:
      switch (valtype->length ())
	{
	case 4:
	  alpha_lds (gdbarch, raw_buffer, valbuf);
	  regcache->cooked_write (ALPHA_FP0_REGNUM, raw_buffer);
	  break;

	case 8:
	  regcache->cooked_write (ALPHA_FP0_REGNUM, valbuf);
	  break;

	case 16:
	  /* FIXME: 128-bit long doubles are returned like structures:
	     by writing into indirect storage provided by the caller
	     as the first argument.  */
	  error (_("Cannot set a 128-bit long double return value."));

	default:
	  internal_error (_("unknown floating point width"));
	}
      break;

    case TYPE_CODE_COMPLEX:
      switch (valtype->length ())
	{
	case 8:
	  /* ??? This isn't correct wrt the ABI, but it's what GCC does.  */
	  regcache->cooked_write (ALPHA_FP0_REGNUM, valbuf);
	  break;

	case 16:
	  regcache->cooked_write (ALPHA_FP0_REGNUM, valbuf);
	  regcache->cooked_write (ALPHA_FP0_REGNUM + 1, valbuf + 8);
	  break;

	case 32:
	  /* FIXME: 128-bit long doubles are returned like structures:
	     by writing into indirect storage provided by the caller
	     as the first argument.  */
	  error (_("Cannot set a 128-bit long double return value."));

	default:
	  internal_error (_("unknown floating point width"));
	}
      break;

    default:
      /* Assume everything else degenerates to an integer.  */
      /* 32-bit values must be sign-extended to 64 bits
	 even if the base data type is unsigned.  */
      if (valtype->length () == 4)
	valtype = builtin_type (gdbarch)->builtin_int32;
      l = unpack_long (valtype, valbuf);
      regcache_cooked_write_unsigned (regcache, ALPHA_V0_REGNUM, l);
      break;
    }
}

static enum return_value_convention
alpha_return_value (struct gdbarch *gdbarch, struct value *function,
		    struct type *type, struct regcache *regcache,
		    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum type_code code = type->code ();
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);

  if ((code == TYPE_CODE_STRUCT
       || code == TYPE_CODE_UNION
       || code == TYPE_CODE_ARRAY)
      && tdep->return_in_memory (type))
    {
      if (readbuf)
	{
	  ULONGEST addr;
	  regcache_raw_read_unsigned (regcache, ALPHA_V0_REGNUM, &addr);
	  read_memory (addr, readbuf, type->length ());
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }

  if (readbuf)
    alpha_extract_return_value (type, regcache, readbuf);
  if (writebuf)
    alpha_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

static int
alpha_return_in_memory_always (struct type *type)
{
  return 1;
}


constexpr gdb_byte alpha_break_insn[] = { 0x80, 0, 0, 0 }; /* call_pal bpt */

typedef BP_MANIPULATION (alpha_break_insn) alpha_breakpoint;


/* This returns the PC of the first insn after the prologue.
   If we can't find the prologue, then return 0.  */

CORE_ADDR
alpha_after_prologue (CORE_ADDR pc)
{
  struct symtab_and_line sal;
  CORE_ADDR func_addr, func_end;

  if (!find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    return 0;

  sal = find_pc_line (func_addr, 0);
  if (sal.end < func_end)
    return sal.end;

  /* The line after the prologue is after the end of the function.  In this
     case, tell the caller to find the prologue the hard way.  */
  return 0;
}

/* Read an instruction from memory at PC, looking through breakpoints.  */

unsigned int
alpha_read_insn (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[ALPHA_INSN_SIZE];
  int res;

  res = target_read_memory (pc, buf, sizeof (buf));
  if (res != 0)
    memory_error (TARGET_XFER_E_IO, pc);
  return extract_unsigned_integer (buf, sizeof (buf), byte_order);
}

/* To skip prologues, I use this predicate.  Returns either PC itself
   if the code at PC does not look like a function prologue; otherwise
   returns an address that (if we're lucky) follows the prologue.  If
   LENIENT, then we must skip everything which is involved in setting
   up the frame (it's OK to skip more, just so long as we don't skip
   anything which might clobber the registers which are being saved.  */

static CORE_ADDR
alpha_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned long inst;
  int offset;
  CORE_ADDR post_prologue_pc;
  gdb_byte buf[ALPHA_INSN_SIZE];

  /* Silently return the unaltered pc upon memory errors.
     This could happen on OSF/1 if decode_line_1 tries to skip the
     prologue for quickstarted shared library functions when the
     shared library is not yet mapped in.
     Reading target memory is slow over serial lines, so we perform
     this check only if the target has shared libraries (which all
     Alpha targets do).  */
  if (target_read_memory (pc, buf, sizeof (buf)))
    return pc;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */

  post_prologue_pc = alpha_after_prologue (pc);
  if (post_prologue_pc != 0)
    return std::max (pc, post_prologue_pc);

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Skip the typical prologue instructions.  These are the stack adjustment
     instruction and the instructions that save registers on the stack
     or in the gcc frame.  */
  for (offset = 0; offset < 100; offset += ALPHA_INSN_SIZE)
    {
      inst = alpha_read_insn (gdbarch, pc + offset);

      if ((inst & 0xffff0000) == 0x27bb0000)	/* ldah $gp,n($t12) */
	continue;
      if ((inst & 0xffff0000) == 0x23bd0000)	/* lda $gp,n($gp) */
	continue;
      if ((inst & 0xffff0000) == 0x23de0000)	/* lda $sp,n($sp) */
	continue;
      if ((inst & 0xffe01fff) == 0x43c0153e)	/* subq $sp,n,$sp */
	continue;

      if (((inst & 0xfc1f0000) == 0xb41e0000		/* stq reg,n($sp) */
	   || (inst & 0xfc1f0000) == 0x9c1e0000)	/* stt reg,n($sp) */
	  && (inst & 0x03e00000) != 0x03e00000)		/* reg != $zero */
	continue;

      if (inst == 0x47de040f)			/* bis sp,sp,fp */
	continue;
      if (inst == 0x47fe040f)			/* bis zero,sp,fp */
	continue;

      break;
    }
  return pc + offset;
}


static const int ldl_l_opcode = 0x2a;
static const int ldq_l_opcode = 0x2b;
static const int stl_c_opcode = 0x2e;
static const int stq_c_opcode = 0x2f;

/* Checks for an atomic sequence of instructions beginning with a LDL_L/LDQ_L
   instruction and ending with a STL_C/STQ_C instruction.  If such a sequence
   is found, attempt to step through it.  A breakpoint is placed at the end of 
   the sequence.  */

static std::vector<CORE_ADDR>
alpha_deal_with_atomic_sequence (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR breaks[2] = {CORE_ADDR_MAX, CORE_ADDR_MAX};
  CORE_ADDR loc = pc;
  CORE_ADDR closing_insn; /* Instruction that closes the atomic sequence.  */
  unsigned int insn = alpha_read_insn (gdbarch, loc);
  int insn_count;
  int index;
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */  
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */
  int bc_insn_count = 0; /* Conditional branch instruction count.  */

  /* Assume all atomic sequences start with a LDL_L/LDQ_L instruction.  */
  if (INSN_OPCODE (insn) != ldl_l_opcode
      && INSN_OPCODE (insn) != ldq_l_opcode)
    return {};

  /* Assume that no atomic sequence is longer than "atomic_sequence_length" 
     instructions.  */
  for (insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      loc += ALPHA_INSN_SIZE;
      insn = alpha_read_insn (gdbarch, loc);

      /* Assume that there is at most one branch in the atomic
	 sequence.  If a branch is found, put a breakpoint in 
	 its destination address.  */
      if (INSN_OPCODE (insn) >= br_opcode)
	{
	  int immediate = (insn & 0x001fffff) << 2;

	  immediate = (immediate ^ 0x400000) - 0x400000;

	  if (bc_insn_count >= 1)
	    return {}; /* More than one branch found, fallback
			  to the standard single-step code.  */

	  breaks[1] = loc + ALPHA_INSN_SIZE + immediate;

	  bc_insn_count++;
	  last_breakpoint++;
	}

      if (INSN_OPCODE (insn) == stl_c_opcode
	  || INSN_OPCODE (insn) == stq_c_opcode)
	break;
    }

  /* Assume that the atomic sequence ends with a STL_C/STQ_C instruction.  */
  if (INSN_OPCODE (insn) != stl_c_opcode
      && INSN_OPCODE (insn) != stq_c_opcode)
    return {};

  closing_insn = loc;
  loc += ALPHA_INSN_SIZE;

  /* Insert a breakpoint right after the end of the atomic sequence.  */
  breaks[0] = loc;

  /* Check for duplicated breakpoints.  Check also for a breakpoint
     placed (branch instruction's destination) anywhere in sequence.  */ 
  if (last_breakpoint
      && (breaks[1] == breaks[0]
	  || (breaks[1] >= pc && breaks[1] <= closing_insn)))
    last_breakpoint = 0;

  std::vector<CORE_ADDR> next_pcs;

  for (index = 0; index <= last_breakpoint; index++)
    next_pcs.push_back (breaks[index]);

  return next_pcs;
}


/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure from
   which we extract the PC (JB_PC) that we will land at.  The PC is copied
   into the "pc".  This routine returns true on success.  */

static int
alpha_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR jb_addr;
  gdb_byte raw_buffer[ALPHA_REGISTER_SIZE];

  jb_addr = get_frame_register_unsigned (frame, ALPHA_A0_REGNUM);

  if (target_read_memory (jb_addr + (tdep->jb_pc * tdep->jb_elt_size),
			  raw_buffer, tdep->jb_elt_size))
    return 0;

  *pc = extract_unsigned_integer (raw_buffer, tdep->jb_elt_size, byte_order);
  return 1;
}


/* Frame unwinder for signal trampolines.  We use alpha tdep bits that
   describe the location and shape of the sigcontext structure.  After
   that, all registers are in memory, so it's easy.  */
/* ??? Shouldn't we be able to do this generically, rather than with
   OSABI data specific to Alpha?  */

struct alpha_sigtramp_unwind_cache
{
  CORE_ADDR sigcontext_addr;
};

static struct alpha_sigtramp_unwind_cache *
alpha_sigtramp_frame_unwind_cache (frame_info_ptr this_frame,
				   void **this_prologue_cache)
{
  struct alpha_sigtramp_unwind_cache *info;

  if (*this_prologue_cache)
    return (struct alpha_sigtramp_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct alpha_sigtramp_unwind_cache);
  *this_prologue_cache = info;

  gdbarch *arch = get_frame_arch (this_frame);
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (arch);
  info->sigcontext_addr = tdep->sigcontext_addr (this_frame);

  return info;
}

/* Return the address of REGNUM in a sigtramp frame.  Since this is
   all arithmetic, it doesn't seem worthwhile to cache it.  */

static CORE_ADDR
alpha_sigtramp_register_address (struct gdbarch *gdbarch,
				 CORE_ADDR sigcontext_addr, int regnum)
{ 
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);

  if (regnum >= 0 && regnum < 32)
    return sigcontext_addr + tdep->sc_regs_offset + regnum * 8;
  else if (regnum >= ALPHA_FP0_REGNUM && regnum < ALPHA_FP0_REGNUM + 32)
    return sigcontext_addr + tdep->sc_fpregs_offset + regnum * 8;
  else if (regnum == ALPHA_PC_REGNUM)
    return sigcontext_addr + tdep->sc_pc_offset; 

  return 0;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
alpha_sigtramp_frame_this_id (frame_info_ptr this_frame,
			      void **this_prologue_cache,
			      struct frame_id *this_id)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);
  struct alpha_sigtramp_unwind_cache *info
    = alpha_sigtramp_frame_unwind_cache (this_frame, this_prologue_cache);
  CORE_ADDR stack_addr, code_addr;

  /* If the OSABI couldn't locate the sigcontext, give up.  */
  if (info->sigcontext_addr == 0)
    return;

  /* If we have dynamic signal trampolines, find their start.
     If we do not, then we must assume there is a symbol record
     that can provide the start address.  */
  if (tdep->dynamic_sigtramp_offset)
    {
      int offset;
      code_addr = get_frame_pc (this_frame);
      offset = tdep->dynamic_sigtramp_offset (gdbarch, code_addr);
      if (offset >= 0)
	code_addr -= offset;
      else
	code_addr = 0;
    }
  else
    code_addr = get_frame_func (this_frame);

  /* The stack address is trivially read from the sigcontext.  */
  stack_addr = alpha_sigtramp_register_address (gdbarch, info->sigcontext_addr,
						ALPHA_SP_REGNUM);
  stack_addr = get_frame_memory_unsigned (this_frame, stack_addr,
					  ALPHA_REGISTER_SIZE);

  *this_id = frame_id_build (stack_addr, code_addr);
}

/* Retrieve the value of REGNUM in FRAME.  Don't give up!  */

static struct value *
alpha_sigtramp_frame_prev_register (frame_info_ptr this_frame,
				    void **this_prologue_cache, int regnum)
{
  struct alpha_sigtramp_unwind_cache *info
    = alpha_sigtramp_frame_unwind_cache (this_frame, this_prologue_cache);
  CORE_ADDR addr;

  if (info->sigcontext_addr != 0)
    {
      /* All integer and fp registers are stored in memory.  */
      addr = alpha_sigtramp_register_address (get_frame_arch (this_frame),
					      info->sigcontext_addr, regnum);
      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);
    }

  /* This extra register may actually be in the sigcontext, but our
     current description of it in alpha_sigtramp_frame_unwind_cache
     doesn't include it.  Too bad.  Fall back on whatever's in the
     outer frame.  */
  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static int
alpha_sigtramp_frame_sniffer (const struct frame_unwind *self,
			      frame_info_ptr this_frame,
			      void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  /* NOTE: cagney/2004-04-30: Do not copy/clone this code.  Instead
     look at tramp-frame.h and other simpler per-architecture
     sigtramp unwinders.  */

  /* We shouldn't even bother to try if the OSABI didn't register a
     sigcontext_addr handler or pc_in_sigtramp handler.  */
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);
  if (tdep->sigcontext_addr == NULL)
    return 0;

  if (tdep->pc_in_sigtramp == NULL)
    return 0;

  /* Otherwise we should be in a signal frame.  */
  find_pc_partial_function (pc, &name, NULL, NULL);
  if (tdep->pc_in_sigtramp (gdbarch, pc, name))
    return 1;

  return 0;
}

static const struct frame_unwind alpha_sigtramp_frame_unwind =
{
  "alpha sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  alpha_sigtramp_frame_this_id,
  alpha_sigtramp_frame_prev_register,
  NULL,
  alpha_sigtramp_frame_sniffer
};



/* Heuristic_proc_start may hunt through the text section for a long
   time across a 2400 baud serial line.  Allows the user to limit this
   search.  */
static int heuristic_fence_post = 0;

/* Attempt to locate the start of the function containing PC.  We assume that
   the previous function ends with an about_to_return insn.  Not foolproof by
   any means, since gcc is happy to put the epilogue in the middle of a
   function.  But we're guessing anyway...  */

static CORE_ADDR
alpha_heuristic_proc_start (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);
  CORE_ADDR last_non_nop = pc;
  CORE_ADDR fence = pc - heuristic_fence_post;
  CORE_ADDR orig_pc = pc;
  CORE_ADDR func;
  struct inferior *inf;

  if (pc == 0)
    return 0;

  /* First see if we can find the start of the function from minimal
     symbol information.  This can succeed with a binary that doesn't
     have debug info, but hasn't been stripped.  */
  func = get_pc_function_start (pc);
  if (func)
    return func;

  if (heuristic_fence_post == -1
      || fence < tdep->vm_min_address)
    fence = tdep->vm_min_address;

  /* Search back for previous return; also stop at a 0, which might be
     seen for instance before the start of a code section.  Don't include
     nops, since this usually indicates padding between functions.  */
  for (pc -= ALPHA_INSN_SIZE; pc >= fence; pc -= ALPHA_INSN_SIZE)
    {
      unsigned int insn = alpha_read_insn (gdbarch, pc);
      switch (insn)
	{
	case 0:			/* invalid insn */
	case 0x6bfa8001:	/* ret $31,($26),1 */
	  return last_non_nop;

	case 0x2ffe0000:	/* unop: ldq_u $31,0($30) */
	case 0x47ff041f:	/* nop: bis $31,$31,$31 */
	  break;

	default:
	  last_non_nop = pc;
	  break;
	}
    }

  inf = current_inferior ();

  /* It's not clear to me why we reach this point when stopping quietly,
     but with this test, at least we don't print out warnings for every
     child forked (eg, on decstation).  22apr93 rich@cygnus.com.  */
  if (inf->control.stop_soon == NO_STOP_QUIETLY)
    {
      static int blurb_printed = 0;

      if (fence == tdep->vm_min_address)
	warning (_("Hit beginning of text section without finding \
enclosing function for address %s"), paddress (gdbarch, orig_pc));
      else
	warning (_("Hit heuristic-fence-post without finding \
enclosing function for address %s"), paddress (gdbarch, orig_pc));

      if (!blurb_printed)
	{
	  gdb_printf (_("\
This warning occurs if you are debugging a function without any symbols\n\
(for example, in a stripped executable).  In that case, you may wish to\n\
increase the size of the search with the `set heuristic-fence-post' command.\n\
\n\
Otherwise, you told GDB there was a function where there isn't one, or\n\
(more likely) you have encountered a bug in GDB.\n"));
	  blurb_printed = 1;
	}
    }

  return 0;
}

/* Fallback alpha frame unwinder.  Uses instruction scanning and knows
   something about the traditional layout of alpha stack frames.  */

struct alpha_heuristic_unwind_cache
{ 
  CORE_ADDR vfp;
  CORE_ADDR start_pc;
  trad_frame_saved_reg *saved_regs;
  int return_reg;
};

/* If a probing loop sequence starts at PC, simulate it and compute
   FRAME_SIZE and PC after its execution.  Otherwise, return with PC and
   FRAME_SIZE unchanged.  */

static void
alpha_heuristic_analyze_probing_loop (struct gdbarch *gdbarch, CORE_ADDR *pc,
				      int *frame_size)
{
  CORE_ADDR cur_pc = *pc;
  int cur_frame_size = *frame_size;
  int nb_of_iterations, reg_index, reg_probe;
  unsigned int insn;

  /* The following pattern is recognized as a probing loop:

	lda     REG_INDEX,NB_OF_ITERATIONS
	lda     REG_PROBE,<immediate>(sp)

     LOOP_START:
	stq     zero,<immediate>(REG_PROBE)
	subq    REG_INDEX,0x1,REG_INDEX
	lda     REG_PROBE,<immediate>(REG_PROBE)
	bne     REG_INDEX, LOOP_START
 
	lda     sp,<immediate>(REG_PROBE)

     If anything different is found, the function returns without
     changing PC and FRAME_SIZE.  Otherwise, PC will point immediately
     after this sequence, and FRAME_SIZE will be updated.  */

  /* lda     REG_INDEX,NB_OF_ITERATIONS */

  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != lda_opcode)
    return;
  reg_index = MEM_RA (insn);
  nb_of_iterations = MEM_DISP (insn);

  /* lda     REG_PROBE,<immediate>(sp) */

  cur_pc += ALPHA_INSN_SIZE;
  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != lda_opcode
      || MEM_RB (insn) != ALPHA_SP_REGNUM)
    return;
  reg_probe = MEM_RA (insn);
  cur_frame_size -= MEM_DISP (insn);

  /* stq     zero,<immediate>(REG_PROBE) */
  
  cur_pc += ALPHA_INSN_SIZE;
  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != stq_opcode
      || MEM_RA (insn) != 0x1f
      || MEM_RB (insn) != reg_probe)
    return;
  
  /* subq    REG_INDEX,0x1,REG_INDEX */

  cur_pc += ALPHA_INSN_SIZE;
  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != subq_opcode
      || !OPR_HAS_IMMEDIATE (insn)
      || OPR_FUNCTION (insn) != subq_function
      || OPR_LIT(insn) != 1
      || OPR_RA (insn) != reg_index
      || OPR_RC (insn) != reg_index)
    return;
  
  /* lda     REG_PROBE,<immediate>(REG_PROBE) */
  
  cur_pc += ALPHA_INSN_SIZE;
  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != lda_opcode
      || MEM_RA (insn) != reg_probe
      || MEM_RB (insn) != reg_probe)
    return;
  cur_frame_size -= MEM_DISP (insn) * nb_of_iterations;

  /* bne     REG_INDEX, LOOP_START */

  cur_pc += ALPHA_INSN_SIZE;
  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != bne_opcode
      || MEM_RA (insn) != reg_index)
    return;

  /* lda     sp,<immediate>(REG_PROBE) */

  cur_pc += ALPHA_INSN_SIZE;
  insn = alpha_read_insn (gdbarch, cur_pc);
  if (INSN_OPCODE (insn) != lda_opcode
      || MEM_RA (insn) != ALPHA_SP_REGNUM
      || MEM_RB (insn) != reg_probe)
    return;
  cur_frame_size -= MEM_DISP (insn);

  *pc = cur_pc;
  *frame_size = cur_frame_size;
}

static struct alpha_heuristic_unwind_cache *
alpha_heuristic_frame_unwind_cache (frame_info_ptr this_frame,
				    void **this_prologue_cache,
				    CORE_ADDR start_pc)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct alpha_heuristic_unwind_cache *info;
  ULONGEST val;
  CORE_ADDR limit_pc, cur_pc;
  int frame_reg, frame_size, return_reg, reg;

  if (*this_prologue_cache)
    return (struct alpha_heuristic_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct alpha_heuristic_unwind_cache);
  *this_prologue_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  limit_pc = get_frame_pc (this_frame);
  if (start_pc == 0)
    start_pc = alpha_heuristic_proc_start (gdbarch, limit_pc);
  info->start_pc = start_pc;

  frame_reg = ALPHA_SP_REGNUM;
  frame_size = 0;
  return_reg = -1;

  /* If we've identified a likely place to start, do code scanning.  */
  if (start_pc != 0)
    {
      /* Limit the forward search to 50 instructions.  */
      if (start_pc + 200 < limit_pc)
	limit_pc = start_pc + 200;

      for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += ALPHA_INSN_SIZE)
	{
	  unsigned int word = alpha_read_insn (gdbarch, cur_pc);

	  if ((word & 0xffff0000) == 0x23de0000)	/* lda $sp,n($sp) */
	    {
	      if (word & 0x8000)
		{
		  /* Consider only the first stack allocation instruction
		     to contain the static size of the frame.  */
		  if (frame_size == 0)
		    frame_size = (-word) & 0xffff;
		}
	      else
		{
		  /* Exit loop if a positive stack adjustment is found, which
		     usually means that the stack cleanup code in the function
		     epilogue is reached.  */
		  break;
		}
	    }
	  else if ((word & 0xfc1f0000) == 0xb41e0000)	/* stq reg,n($sp) */
	    {
	      reg = (word & 0x03e00000) >> 21;

	      /* Ignore this instruction if we have already encountered
		 an instruction saving the same register earlier in the
		 function code.  The current instruction does not tell
		 us where the original value upon function entry is saved.
		 All it says is that the function we are scanning reused
		 that register for some computation of its own, and is now
		 saving its result.  */
	      if (info->saved_regs[reg].is_addr ())
		continue;

	      if (reg == 31)
		continue;

	      /* Do not compute the address where the register was saved yet,
		 because we don't know yet if the offset will need to be
		 relative to $sp or $fp (we can not compute the address
		 relative to $sp if $sp is updated during the execution of
		 the current subroutine, for instance when doing some alloca).
		 So just store the offset for the moment, and compute the
		 address later when we know whether this frame has a frame
		 pointer or not.  */
	      /* Hack: temporarily add one, so that the offset is non-zero
		 and we can tell which registers have save offsets below.  */
	      info->saved_regs[reg].set_addr ((word & 0xffff) + 1);

	      /* Starting with OSF/1-3.2C, the system libraries are shipped
		 without local symbols, but they still contain procedure
		 descriptors without a symbol reference. GDB is currently
		 unable to find these procedure descriptors and uses
		 heuristic_proc_desc instead.
		 As some low level compiler support routines (__div*, __add*)
		 use a non-standard return address register, we have to
		 add some heuristics to determine the return address register,
		 or stepping over these routines will fail.
		 Usually the return address register is the first register
		 saved on the stack, but assembler optimization might
		 rearrange the register saves.
		 So we recognize only a few registers (t7, t9, ra) within
		 the procedure prologue as valid return address registers.
		 If we encounter a return instruction, we extract the
		 return address register from it.

		 FIXME: Rewriting GDB to access the procedure descriptors,
		 e.g. via the minimal symbol table, might obviate this
		 hack.  */
	      if (return_reg == -1
		  && cur_pc < (start_pc + 80)
		  && (reg == ALPHA_T7_REGNUM
		      || reg == ALPHA_T9_REGNUM
		      || reg == ALPHA_RA_REGNUM))
		return_reg = reg;
	    }
	  else if ((word & 0xffe0ffff) == 0x6be08001)	/* ret zero,reg,1 */
	    return_reg = (word >> 16) & 0x1f;
	  else if (word == 0x47de040f)			/* bis sp,sp,fp */
	    frame_reg = ALPHA_GCC_FP_REGNUM;
	  else if (word == 0x47fe040f)			/* bis zero,sp,fp */
	    frame_reg = ALPHA_GCC_FP_REGNUM;

	  alpha_heuristic_analyze_probing_loop (gdbarch, &cur_pc, &frame_size);
	}

      /* If we haven't found a valid return address register yet, keep
	 searching in the procedure prologue.  */
      if (return_reg == -1)
	{
	  while (cur_pc < (limit_pc + 80) && cur_pc < (start_pc + 80))
	    {
	      unsigned int word = alpha_read_insn (gdbarch, cur_pc);

	      if ((word & 0xfc1f0000) == 0xb41e0000)	/* stq reg,n($sp) */
		{
		  reg = (word & 0x03e00000) >> 21;
		  if (reg == ALPHA_T7_REGNUM
		      || reg == ALPHA_T9_REGNUM
		      || reg == ALPHA_RA_REGNUM)
		    {
		      return_reg = reg;
		      break;
		    }
		}
	      else if ((word & 0xffe0ffff) == 0x6be08001) /* ret zero,reg,1 */
		{
		  return_reg = (word >> 16) & 0x1f;
		  break;
		}

	      cur_pc += ALPHA_INSN_SIZE;
	    }
	}
    }

  /* Failing that, do default to the customary RA.  */
  if (return_reg == -1)
    return_reg = ALPHA_RA_REGNUM;
  info->return_reg = return_reg;

  val = get_frame_register_unsigned (this_frame, frame_reg);
  info->vfp = val + frame_size;

  /* Convert offsets to absolute addresses.  See above about adding
     one to the offsets to make all detected offsets non-zero.  */
  for (reg = 0; reg < ALPHA_NUM_REGS; ++reg)
    if (info->saved_regs[reg].is_addr ())
      info->saved_regs[reg].set_addr (info->saved_regs[reg].addr ()
				      + val - 1);

  /* The stack pointer of the previous frame is computed by popping
     the current stack frame.  */
  if (!info->saved_regs[ALPHA_SP_REGNUM].is_addr ())
   info->saved_regs[ALPHA_SP_REGNUM].set_value (info->vfp);

  return info;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
alpha_heuristic_frame_this_id (frame_info_ptr this_frame,
			       void **this_prologue_cache,
			       struct frame_id *this_id)
{
  struct alpha_heuristic_unwind_cache *info
    = alpha_heuristic_frame_unwind_cache (this_frame, this_prologue_cache, 0);

  *this_id = frame_id_build (info->vfp, info->start_pc);
}

/* Retrieve the value of REGNUM in FRAME.  Don't give up!  */

static struct value *
alpha_heuristic_frame_prev_register (frame_info_ptr this_frame,
				     void **this_prologue_cache, int regnum)
{
  struct alpha_heuristic_unwind_cache *info
    = alpha_heuristic_frame_unwind_cache (this_frame, this_prologue_cache, 0);

  /* The PC of the previous frame is stored in the link register of
     the current frame.  Frob regnum so that we pull the value from
     the correct place.  */
  if (regnum == ALPHA_PC_REGNUM)
    regnum = info->return_reg;
  
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static const struct frame_unwind alpha_heuristic_frame_unwind =
{
  "alpha prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  alpha_heuristic_frame_this_id,
  alpha_heuristic_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
alpha_heuristic_frame_base_address (frame_info_ptr this_frame,
				    void **this_prologue_cache)
{
  struct alpha_heuristic_unwind_cache *info
    = alpha_heuristic_frame_unwind_cache (this_frame, this_prologue_cache, 0);

  return info->vfp;
}

static const struct frame_base alpha_heuristic_frame_base = {
  &alpha_heuristic_frame_unwind,
  alpha_heuristic_frame_base_address,
  alpha_heuristic_frame_base_address,
  alpha_heuristic_frame_base_address
};

/* Just like reinit_frame_cache, but with the right arguments to be
   callable as an sfunc.  Used by the "set heuristic-fence-post" command.  */

static void
reinit_frame_cache_sfunc (const char *args,
			  int from_tty, struct cmd_list_element *c)
{
  reinit_frame_cache ();
}

/* Helper routines for alpha*-nat.c files to move register sets to and
   from core files.  The UNIQUE pointer is allowed to be NULL, as most
   targets don't supply this value in their core files.  */

void
alpha_supply_int_regs (struct regcache *regcache, int regno,
		       const void *r0_r30, const void *pc, const void *unique)
{
  const gdb_byte *regs = (const gdb_byte *) r0_r30;
  int i;

  for (i = 0; i < 31; ++i)
    if (regno == i || regno == -1)
      regcache->raw_supply (i, regs + i * 8);

  if (regno == ALPHA_ZERO_REGNUM || regno == -1)
    {
      const gdb_byte zero[8] = { 0 };

      regcache->raw_supply (ALPHA_ZERO_REGNUM, zero);
    }

  if (regno == ALPHA_PC_REGNUM || regno == -1)
    regcache->raw_supply (ALPHA_PC_REGNUM, pc);

  if (regno == ALPHA_UNIQUE_REGNUM || regno == -1)
    regcache->raw_supply (ALPHA_UNIQUE_REGNUM, unique);
}

void
alpha_fill_int_regs (const struct regcache *regcache,
		     int regno, void *r0_r30, void *pc, void *unique)
{
  gdb_byte *regs = (gdb_byte *) r0_r30;
  int i;

  for (i = 0; i < 31; ++i)
    if (regno == i || regno == -1)
      regcache->raw_collect (i, regs + i * 8);

  if (regno == ALPHA_PC_REGNUM || regno == -1)
    regcache->raw_collect (ALPHA_PC_REGNUM, pc);

  if (unique && (regno == ALPHA_UNIQUE_REGNUM || regno == -1))
    regcache->raw_collect (ALPHA_UNIQUE_REGNUM, unique);
}

void
alpha_supply_fp_regs (struct regcache *regcache, int regno,
		      const void *f0_f30, const void *fpcr)
{
  const gdb_byte *regs = (const gdb_byte *) f0_f30;
  int i;

  for (i = ALPHA_FP0_REGNUM; i < ALPHA_FP0_REGNUM + 31; ++i)
    if (regno == i || regno == -1)
      regcache->raw_supply (i, regs + (i - ALPHA_FP0_REGNUM) * 8);

  if (regno == ALPHA_FPCR_REGNUM || regno == -1)
    regcache->raw_supply (ALPHA_FPCR_REGNUM, fpcr);
}

void
alpha_fill_fp_regs (const struct regcache *regcache,
		    int regno, void *f0_f30, void *fpcr)
{
  gdb_byte *regs = (gdb_byte *) f0_f30;
  int i;

  for (i = ALPHA_FP0_REGNUM; i < ALPHA_FP0_REGNUM + 31; ++i)
    if (regno == i || regno == -1)
      regcache->raw_collect (i, regs + (i - ALPHA_FP0_REGNUM) * 8);

  if (regno == ALPHA_FPCR_REGNUM || regno == -1)
    regcache->raw_collect (ALPHA_FPCR_REGNUM, fpcr);
}



/* Return nonzero if the G_floating register value in REG is equal to
   zero for FP control instructions.  */
   
static int
fp_register_zero_p (LONGEST reg)
{
  /* Check that all bits except the sign bit are zero.  */
  const LONGEST zero_mask = ((LONGEST) 1 << 63) ^ -1;

  return ((reg & zero_mask) == 0);
}

/* Return the value of the sign bit for the G_floating register
   value held in REG.  */

static int
fp_register_sign_bit (LONGEST reg)
{
  const LONGEST sign_mask = (LONGEST) 1 << 63;

  return ((reg & sign_mask) != 0);
}

/* alpha_software_single_step() is called just before we want to resume
   the inferior, if we want to single-step it but there is no hardware
   or kernel single-step support (NetBSD on Alpha, for example).  We find
   the target of the coming instruction and breakpoint it.  */

static CORE_ADDR
alpha_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  unsigned int insn;
  unsigned int op;
  int regno;
  int offset;
  LONGEST rav;

  insn = alpha_read_insn (gdbarch, pc);

  /* Opcode is top 6 bits.  */
  op = (insn >> 26) & 0x3f;

  if (op == 0x1a)
    {
      /* Jump format: target PC is:
	 RB & ~3  */
      return (regcache_raw_get_unsigned (regcache, (insn >> 16) & 0x1f) & ~3);
    }

  if ((op & 0x30) == 0x30)
    {
      /* Branch format: target PC is:
	 (new PC) + (4 * sext(displacement))  */
      if (op == 0x30		/* BR */
	  || op == 0x34)	/* BSR */
	{
 branch_taken:
	  offset = (insn & 0x001fffff);
	  if (offset & 0x00100000)
	    offset  |= 0xffe00000;
	  offset *= ALPHA_INSN_SIZE;
	  return (pc + ALPHA_INSN_SIZE + offset);
	}

      /* Need to determine if branch is taken; read RA.  */
      regno = (insn >> 21) & 0x1f;
      switch (op)
	{
	  case 0x31:              /* FBEQ */
	  case 0x36:              /* FBGE */
	  case 0x37:              /* FBGT */
	  case 0x33:              /* FBLE */
	  case 0x32:              /* FBLT */
	  case 0x35:              /* FBNE */
	    regno += gdbarch_fp0_regnum (gdbarch);
	}
      
      rav = regcache_raw_get_signed (regcache, regno);

      switch (op)
	{
	case 0x38:		/* BLBC */
	  if ((rav & 1) == 0)
	    goto branch_taken;
	  break;
	case 0x3c:		/* BLBS */
	  if (rav & 1)
	    goto branch_taken;
	  break;
	case 0x39:		/* BEQ */
	  if (rav == 0)
	    goto branch_taken;
	  break;
	case 0x3d:		/* BNE */
	  if (rav != 0)
	    goto branch_taken;
	  break;
	case 0x3a:		/* BLT */
	  if (rav < 0)
	    goto branch_taken;
	  break;
	case 0x3b:		/* BLE */
	  if (rav <= 0)
	    goto branch_taken;
	  break;
	case 0x3f:		/* BGT */
	  if (rav > 0)
	    goto branch_taken;
	  break;
	case 0x3e:		/* BGE */
	  if (rav >= 0)
	    goto branch_taken;
	  break;

	/* Floating point branches.  */
	
	case 0x31:              /* FBEQ */
	  if (fp_register_zero_p (rav))
	    goto branch_taken;
	  break;
	case 0x36:              /* FBGE */
	  if (fp_register_sign_bit (rav) == 0 || fp_register_zero_p (rav))
	    goto branch_taken;
	  break;
	case 0x37:              /* FBGT */
	  if (fp_register_sign_bit (rav) == 0 && ! fp_register_zero_p (rav))
	    goto branch_taken;
	  break;
	case 0x33:              /* FBLE */
	  if (fp_register_sign_bit (rav) == 1 || fp_register_zero_p (rav))
	    goto branch_taken;
	  break;
	case 0x32:              /* FBLT */
	  if (fp_register_sign_bit (rav) == 1 && ! fp_register_zero_p (rav))
	    goto branch_taken;
	  break;
	case 0x35:              /* FBNE */
	  if (! fp_register_zero_p (rav))
	    goto branch_taken;
	  break;
	}
    }

  /* Not a branch or branch not taken; target PC is:
     pc + 4  */
  return (pc + ALPHA_INSN_SIZE);
}

std::vector<CORE_ADDR>
alpha_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();

  CORE_ADDR pc = regcache_read_pc (regcache);

  std::vector<CORE_ADDR> next_pcs
    = alpha_deal_with_atomic_sequence (gdbarch, pc);
  if (!next_pcs.empty ())
    return next_pcs;

  CORE_ADDR next_pc = alpha_next_pc (regcache, pc);
  return {next_pc};
}


/* Initialize the current architecture based on INFO.  If possible, re-use an
   architecture from ARCHES, which is a list of architectures already created
   during this debugging session.

   Called e.g. at program startup, when reading a core file, and when reading
   a binary file.  */

static struct gdbarch *
alpha_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* Find a candidate among extant architectures.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new alpha_gdbarch_tdep));
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);

  /* Lowest text address.  This is used by heuristic_proc_start()
     to decide when to stop looking.  */
  tdep->vm_min_address = (CORE_ADDR) 0x120000000LL;

  tdep->dynamic_sigtramp_offset = NULL;
  tdep->sigcontext_addr = NULL;
  tdep->sc_pc_offset = 2 * 8;
  tdep->sc_regs_offset = 4 * 8;
  tdep->sc_fpregs_offset = tdep->sc_regs_offset + 32 * 8 + 8;

  tdep->jb_pc = -1;	/* longjmp support not enabled by default.  */

  tdep->return_in_memory = alpha_return_in_memory_always;

  /* Type sizes */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 64);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_wchar_bit (gdbarch, 64);
  set_gdbarch_wchar_signed (gdbarch, 0);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 64);

  /* Register info */
  set_gdbarch_num_regs (gdbarch, ALPHA_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, ALPHA_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, ALPHA_PC_REGNUM);
  set_gdbarch_fp0_regnum (gdbarch, ALPHA_FP0_REGNUM);

  set_gdbarch_register_name (gdbarch, alpha_register_name);
  set_gdbarch_register_type (gdbarch, alpha_register_type);

  set_gdbarch_cannot_fetch_register (gdbarch, alpha_cannot_fetch_register);
  set_gdbarch_cannot_store_register (gdbarch, alpha_cannot_store_register);

  set_gdbarch_convert_register_p (gdbarch, alpha_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, alpha_register_to_value);
  set_gdbarch_value_to_register (gdbarch, alpha_value_to_register);

  set_gdbarch_register_reggroup_p (gdbarch, alpha_register_reggroup_p);

  /* Prologue heuristics.  */
  set_gdbarch_skip_prologue (gdbarch, alpha_skip_prologue);

  /* Call info.  */

  set_gdbarch_return_value (gdbarch, alpha_return_value);

  /* Settings for calling functions in the inferior.  */
  set_gdbarch_push_dummy_call (gdbarch, alpha_push_dummy_call);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       alpha_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       alpha_breakpoint::bp_from_kind);
  set_gdbarch_decr_pc_after_break (gdbarch, ALPHA_INSN_SIZE);
  set_gdbarch_cannot_step_breakpoint (gdbarch, 1);

  /* Handles single stepping of atomic sequences.  */
  set_gdbarch_software_single_step (gdbarch, alpha_software_single_step);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Now that we have tuned the configuration, set a few final things
     based on what the OS ABI has told us.  */

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, alpha_get_longjmp_target);

  frame_unwind_append_unwinder (gdbarch, &alpha_sigtramp_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &alpha_heuristic_frame_unwind);

  frame_base_set_default (gdbarch, &alpha_heuristic_frame_base);

  return gdbarch;
}

void
alpha_dwarf2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  dwarf2_append_unwinders (gdbarch);
  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
}

void _initialize_alpha_tdep ();
void
_initialize_alpha_tdep ()
{

  gdbarch_register (bfd_arch_alpha, alpha_gdbarch_init, NULL);

  /* Let the user set the fence post for heuristic_proc_start.  */

  /* We really would like to have both "0" and "unlimited" work, but
     command.c doesn't deal with that.  So make it a var_zinteger
     because the user can always use "999999" or some such for unlimited.  */
  /* We need to throw away the frame cache when we set this, since it
     might change our ability to get backtraces.  */
  add_setshow_zinteger_cmd ("heuristic-fence-post", class_support,
			    &heuristic_fence_post, _("\
Set the distance searched for the start of a function."), _("\
Show the distance searched for the start of a function."), _("\
If you are debugging a stripped executable, GDB needs to search through the\n\
program for the start of a function.  This command sets the distance of the\n\
search.  The only need to set it is when debugging a stripped executable."),
			    reinit_frame_cache_sfunc,
			    NULL, /* FIXME: i18n: The distance searched for
				     the start of a function is \"%d\".  */
			    &setlist, &showlist);
}

/* Target-dependent code for the Motorola 68000 series.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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
#include "dwarf2/frame.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbtypes.h"
#include "symtab.h"
#include "gdbcore.h"
#include "value.h"
#include "inferior.h"
#include "regcache.h"
#include "arch-utils.h"
#include "osabi.h"
#include "dis-asm.h"
#include "target-descriptions.h"
#include "floatformat.h"
#include "target-float.h"
#include "elf-bfd.h"
#include "elf/m68k.h"

#include "m68k-tdep.h"


#define P_LINKL_FP	0x480e
#define P_LINKW_FP	0x4e56
#define P_PEA_FP	0x4856
#define P_MOVEAL_SP_FP	0x2c4f
#define P_ADDAW_SP	0xdefc
#define P_ADDAL_SP	0xdffc
#define P_SUBQW_SP	0x514f
#define P_SUBQL_SP	0x518f
#define P_LEA_SP_SP	0x4fef
#define P_LEA_PC_A5	0x4bfb0170
#define P_FMOVEMX_SP	0xf227
#define P_MOVEL_SP	0x2f00
#define P_MOVEML_SP	0x48e7

/* Offset from SP to first arg on stack at first instruction of a function.  */
#define SP_ARG0 (1 * 4)

#if !defined (BPT_VECTOR)
#define BPT_VECTOR 0xf
#endif

constexpr gdb_byte m68k_break_insn[] = {0x4e, (0x40 | BPT_VECTOR)};

typedef BP_MANIPULATION (m68k_break_insn) m68k_breakpoint;


/* Construct types for ISA-specific registers.  */
static struct type *
m68k_ps_type (struct gdbarch *gdbarch)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (!tdep->m68k_ps_type)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_m68k_ps", 32);
      append_flags_type_flag (type, 0, "C");
      append_flags_type_flag (type, 1, "V");
      append_flags_type_flag (type, 2, "Z");
      append_flags_type_flag (type, 3, "N");
      append_flags_type_flag (type, 4, "X");
      append_flags_type_flag (type, 8, "I0");
      append_flags_type_flag (type, 9, "I1");
      append_flags_type_flag (type, 10, "I2");
      append_flags_type_flag (type, 12, "M");
      append_flags_type_flag (type, 13, "S");
      append_flags_type_flag (type, 14, "T0");
      append_flags_type_flag (type, 15, "T1");

      tdep->m68k_ps_type = type;
    }

  return tdep->m68k_ps_type;
}

static struct type *
m68881_ext_type (struct gdbarch *gdbarch)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (!tdep->m68881_ext_type)
    {
      type_allocator alloc (gdbarch);
      tdep->m68881_ext_type
	= init_float_type (alloc, -1, "builtin_type_m68881_ext",
			   floatformats_m68881_ext);
    }

  return tdep->m68881_ext_type;
}

/* Return the GDB type object for the "standard" data type of data in
   register N.  This should be int for D0-D7, SR, FPCONTROL and
   FPSTATUS, long double for FP0-FP7, and void pointer for all others
   (A0-A7, PC, FPIADDR).  Note, for registers which contain
   addresses return pointer to void, not pointer to char, because we
   don't want to attempt to print the string after printing the
   address.  */

static struct type *
m68k_register_type (struct gdbarch *gdbarch, int regnum)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (tdep->fpregs_present)
    {
      if (regnum >= gdbarch_fp0_regnum (gdbarch)
	  && regnum <= gdbarch_fp0_regnum (gdbarch) + 7)
	{
	  if (tdep->flavour == m68k_coldfire_flavour)
	    return builtin_type (gdbarch)->builtin_double;
	  else
	    return m68881_ext_type (gdbarch);
	}

      if (regnum == M68K_FPI_REGNUM)
	return builtin_type (gdbarch)->builtin_func_ptr;

      if (regnum == M68K_FPC_REGNUM || regnum == M68K_FPS_REGNUM)
	return builtin_type (gdbarch)->builtin_int32;
    }
  else
    {
      if (regnum >= M68K_FP0_REGNUM && regnum <= M68K_FPI_REGNUM)
	return builtin_type (gdbarch)->builtin_int0;
    }

  if (regnum == gdbarch_pc_regnum (gdbarch))
    return builtin_type (gdbarch)->builtin_func_ptr;

  if (regnum >= M68K_A0_REGNUM && regnum <= M68K_A0_REGNUM + 7)
    return builtin_type (gdbarch)->builtin_data_ptr;

  if (regnum == M68K_PS_REGNUM)
    return m68k_ps_type (gdbarch);

  return builtin_type (gdbarch)->builtin_int32;
}

static const char * const m68k_register_names[] = {
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "a0", "a1", "a2", "a3", "a4", "a5", "fp", "sp",
    "ps", "pc",
    "fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "fp7",
    "fpcontrol", "fpstatus", "fpiaddr"
  };

/* Function: m68k_register_name
   Returns the name of the standard m68k register regnum.  */

static const char *
m68k_register_name (struct gdbarch *gdbarch, int regnum)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  static_assert (ARRAY_SIZE (m68k_register_names) == M68K_NUM_REGS);
  if (regnum >= M68K_FP0_REGNUM && regnum <= M68K_FPI_REGNUM
      && tdep->fpregs_present == 0)
    return "";
  else
    return m68k_register_names[regnum];
}

/* Return nonzero if a value of type TYPE stored in register REGNUM
   needs any special handling.  */

static int
m68k_convert_register_p (struct gdbarch *gdbarch,
			 int regnum, struct type *type)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (!tdep->fpregs_present)
    return 0;
  return (regnum >= M68K_FP0_REGNUM && regnum <= M68K_FP0_REGNUM + 7
	  /* We only support floating-point values.  */
	  && type->code () == TYPE_CODE_FLT
	  && type != register_type (gdbarch, M68K_FP0_REGNUM));
}

/* Read a value of type TYPE from register REGNUM in frame FRAME, and
   return its contents in TO.  */

static int
m68k_register_to_value (frame_info_ptr frame, int regnum,
			struct type *type, gdb_byte *to,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte from[M68K_MAX_REGISTER_SIZE];
  struct type *fpreg_type = register_type (gdbarch, M68K_FP0_REGNUM);

  gdb_assert (type->code () == TYPE_CODE_FLT);

  /* Convert to TYPE.  */
  auto from_view
    = gdb::make_array_view (from, register_size (gdbarch, regnum));
  frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);
  if (!get_frame_register_bytes (next_frame, regnum, 0, from_view, optimizedp,
				 unavailablep))
    return 0;

  target_float_convert (from, fpreg_type, to, type);
  *optimizedp = *unavailablep = 0;
  return 1;
}

/* Write the contents FROM of a value of type TYPE into register
   REGNUM in frame FRAME.  */

static void
m68k_value_to_register (frame_info_ptr frame, int regnum,
			struct type *type, const gdb_byte *from)
{
  gdb_byte to[M68K_MAX_REGISTER_SIZE];
  gdbarch *arch = get_frame_arch (frame);
  struct type *fpreg_type = register_type (arch, M68K_FP0_REGNUM);

  /* We only support floating-point values.  */
  if (type->code () != TYPE_CODE_FLT)
    {
      warning (_("Cannot convert non-floating-point type "
	       "to floating-point register value."));
      return;
    }

  /* Convert from TYPE.  */
  target_float_convert (from, type, to, fpreg_type);
  auto to_view = gdb::make_array_view (to, fpreg_type->length ());
  put_frame_register (get_next_frame_sentinel_okay (frame), regnum, to_view);
}


/* There is a fair number of calling conventions that are in somewhat
   wide use.  The 68000/08/10 don't support an FPU, not even as a
   coprocessor.  All function return values are stored in %d0/%d1.
   Structures are returned in a static buffer, a pointer to which is
   returned in %d0.  This means that functions returning a structure
   are not re-entrant.  To avoid this problem some systems use a
   convention where the caller passes a pointer to a buffer in %a1
   where the return values is to be stored.  This convention is the
   default, and is implemented in the function m68k_return_value.

   The 68020/030/040/060 do support an FPU, either as a coprocessor
   (68881/2) or built-in (68040/68060).  That's why System V release 4
   (SVR4) introduces a new calling convention specified by the SVR4
   psABI.  Integer values are returned in %d0/%d1, pointer return
   values in %a0 and floating values in %fp0.  When calling functions
   returning a structure the caller should pass a pointer to a buffer
   for the return value in %a0.  This convention is implemented in the
   function m68k_svr4_return_value, and by appropriately setting the
   struct_value_regnum member of `struct gdbarch_tdep'.

   GNU/Linux returns values in the same way as SVR4 does, but uses %a1
   for passing the structure return value buffer.

   GCC can also generate code where small structures are returned in
   %d0/%d1 instead of in memory by using -freg-struct-return.  This is
   the default on NetBSD a.out, OpenBSD and GNU/Linux and several
   embedded systems.  This convention is implemented by setting the
   struct_return member of `struct gdbarch_tdep' to reg_struct_return.

   GCC also has an "embedded" ABI.  This works like the SVR4 ABI,
   except that pointers are returned in %D0.  This is implemented by
   setting the pointer_result_regnum member of `struct gdbarch_tdep'
   as appropriate.  */

/* Read a function return value of TYPE from REGCACHE, and copy that
   into VALBUF.  */

static void
m68k_extract_return_value (struct type *type, struct regcache *regcache,
			   gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte buf[M68K_MAX_REGISTER_SIZE];

  if (type->code () == TYPE_CODE_PTR && len == 4)
    {
      struct gdbarch *gdbarch = regcache->arch ();
      m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);
      regcache->raw_read (tdep->pointer_result_regnum, valbuf);
    }
  else if (len <= 4)
    {
      regcache->raw_read (M68K_D0_REGNUM, buf);
      memcpy (valbuf, buf + (4 - len), len);
    }
  else if (len <= 8)
    {
      regcache->raw_read (M68K_D0_REGNUM, buf);
      memcpy (valbuf, buf + (8 - len), len - 4);
      regcache->raw_read (M68K_D1_REGNUM, valbuf + (len - 4));
    }
  else
    internal_error (_("Cannot extract return value of %d bytes long."), len);
}

static void
m68k_svr4_extract_return_value (struct type *type, struct regcache *regcache,
				gdb_byte *valbuf)
{
  gdb_byte buf[M68K_MAX_REGISTER_SIZE];
  struct gdbarch *gdbarch = regcache->arch ();
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (tdep->float_return && type->code () == TYPE_CODE_FLT)
    {
      struct type *fpreg_type = register_type (gdbarch, M68K_FP0_REGNUM);
      regcache->raw_read (M68K_FP0_REGNUM, buf);
      target_float_convert (buf, fpreg_type, valbuf, type);
    }
  else
    m68k_extract_return_value (type, regcache, valbuf);
}

/* Write a function return value of TYPE from VALBUF into REGCACHE.  */

static void
m68k_store_return_value (struct type *type, struct regcache *regcache,
			 const gdb_byte *valbuf)
{
  int len = type->length ();

  if (type->code () == TYPE_CODE_PTR && len == 4)
    {
      struct gdbarch *gdbarch = regcache->arch ();
      m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);
      regcache->raw_write (tdep->pointer_result_regnum, valbuf);
      /* gdb historically also set D0 in the SVR4 case.  */
      if (tdep->pointer_result_regnum != M68K_D0_REGNUM)
	regcache->raw_write (M68K_D0_REGNUM, valbuf);
    }
  else if (len <= 4)
    regcache->raw_write_part (M68K_D0_REGNUM, 4 - len, len, valbuf);
  else if (len <= 8)
    {
      regcache->raw_write_part (M68K_D0_REGNUM, 8 - len, len - 4, valbuf);
      regcache->raw_write (M68K_D1_REGNUM, valbuf + (len - 4));
    }
  else
    internal_error (_("Cannot store return value of %d bytes long."), len);
}

static void
m68k_svr4_store_return_value (struct type *type, struct regcache *regcache,
			      const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (tdep->float_return && type->code () == TYPE_CODE_FLT)
    {
      struct type *fpreg_type = register_type (gdbarch, M68K_FP0_REGNUM);
      gdb_byte buf[M68K_MAX_REGISTER_SIZE];
      target_float_convert (valbuf, type, buf, fpreg_type);
      regcache->raw_write (M68K_FP0_REGNUM, buf);
    }
  else
    m68k_store_return_value (type, regcache, valbuf);
}

/* Return non-zero if TYPE, which is assumed to be a structure, union or
   complex type, should be returned in registers for architecture
   GDBARCH.  */

static int
m68k_reg_struct_return_p (struct gdbarch *gdbarch, struct type *type)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);
  enum type_code code = type->code ();
  int len = type->length ();

  gdb_assert (code == TYPE_CODE_STRUCT || code == TYPE_CODE_UNION
	      || code == TYPE_CODE_COMPLEX || code == TYPE_CODE_ARRAY);

  if (tdep->struct_return == pcc_struct_return)
    return 0;

  const bool is_vector = code == TYPE_CODE_ARRAY && type->is_vector ();

  if (is_vector
      && check_typedef (type->target_type ())->code () == TYPE_CODE_FLT)
    return 0;

  /* According to m68k_return_in_memory in the m68k GCC back-end,
     strange things happen for small aggregate types.  Aggregate types
     with only one component are always returned like the type of the
     component.  Aggregate types whose size is 2, 4, or 8 are returned
     in registers if their natural alignment is at least 16 bits.

     We reject vectors here, as experimentally this gives the correct
     answer.  */
  if (!is_vector && (len == 2 || len == 4 || len == 8))
    return type_align (type) >= 2;

  return (len == 1 || len == 2 || len == 4 || len == 8);
}

/* Determine, for architecture GDBARCH, how a return value of TYPE
   should be returned.  If it is supposed to be returned in registers,
   and READBUF is non-zero, read the appropriate value from REGCACHE,
   and copy it into READBUF.  If WRITEBUF is non-zero, write the value
   from WRITEBUF into REGCACHE.  */

static enum return_value_convention
m68k_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *type, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum type_code code = type->code ();

  /* GCC returns a `long double' in memory too.  */
  if (((code == TYPE_CODE_STRUCT || code == TYPE_CODE_UNION
	|| code == TYPE_CODE_COMPLEX || code == TYPE_CODE_ARRAY)
       && !m68k_reg_struct_return_p (gdbarch, type))
      || (code == TYPE_CODE_FLT && type->length () == 12))
    {
      /* The default on m68k is to return structures in static memory.
	 Consequently a function must return the address where we can
	 find the return value.  */

      if (readbuf)
	{
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, M68K_D0_REGNUM, &addr);
	  read_memory (addr, readbuf, type->length ());
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }

  if (readbuf)
    m68k_extract_return_value (type, regcache, readbuf);
  if (writebuf)
    m68k_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}

static enum return_value_convention
m68k_svr4_return_value (struct gdbarch *gdbarch, struct value *function,
			struct type *type, struct regcache *regcache,
			gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum type_code code = type->code ();
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  /* Aggregates with a single member are always returned like their
     sole element.  */
  if ((code == TYPE_CODE_STRUCT || code == TYPE_CODE_UNION)
      && type->num_fields () == 1)
    {
      type = check_typedef (type->field (0).type ());
      return m68k_svr4_return_value (gdbarch, function, type, regcache,
				     readbuf, writebuf);
    }

  if (((code == TYPE_CODE_STRUCT || code == TYPE_CODE_UNION
	|| code == TYPE_CODE_COMPLEX || code == TYPE_CODE_ARRAY)
       && !m68k_reg_struct_return_p (gdbarch, type))
      /* GCC may return a `long double' in memory too.  */
      || (!tdep->float_return
	  && code == TYPE_CODE_FLT
	  && type->length () == 12))
    {
      /* The System V ABI says that:

	 "A function returning a structure or union also sets %a0 to
	 the value it finds in %a0.  Thus when the caller receives
	 control again, the address of the returned object resides in
	 register %a0."

	 So the ABI guarantees that we can always find the return
	 value just after the function has returned.

	 However, GCC also implements the "embedded" ABI.  That ABI
	 does not preserve %a0 across calls, but does write the value
	 back to %d0.  */

      if (readbuf)
	{
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, tdep->pointer_result_regnum,
				      &addr);
	  read_memory (addr, readbuf, type->length ());
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }

  if (readbuf)
    m68k_svr4_extract_return_value (type, regcache, readbuf);
  if (writebuf)
    m68k_svr4_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* Always align the frame to a 4-byte boundary.  This is required on
   coldfire and harmless on the rest.  */

static CORE_ADDR
m68k_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align the stack to four bytes.  */
  return sp & ~3;
}

static CORE_ADDR
m68k_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		      struct value **args, CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  int i;

  /* Push arguments in reverse order.  */
  for (i = nargs - 1; i >= 0; i--)
    {
      struct type *value_type = args[i]->enclosing_type ();
      int len = value_type->length ();
      int container_len = (len + 3) & ~3;
      int offset;

      /* Non-scalars bigger than 4 bytes are left aligned, others are
	 right aligned.  */
      if ((value_type->code () == TYPE_CODE_STRUCT
	   || value_type->code () == TYPE_CODE_UNION
	   || value_type->code () == TYPE_CODE_ARRAY)
	  && len > 4)
	offset = 0;
      else
	offset = container_len - len;
      sp -= container_len;
      write_memory (sp + offset, args[i]->contents_all ().data (), len);
    }

  /* Store struct value address.  */
  if (return_method == return_method_struct)
    {
      store_unsigned_integer (buf, 4, byte_order, struct_addr);
      regcache->cooked_write (tdep->struct_value_regnum, buf);
    }

  /* Store return address.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, bp_addr);
  write_memory (sp, buf, 4);

  /* Finally, update the stack pointer...  */
  store_unsigned_integer (buf, 4, byte_order, sp);
  regcache->cooked_write (M68K_SP_REGNUM, buf);

  /* ...and fake a frame pointer.  */
  regcache->cooked_write (M68K_FP_REGNUM, buf);

  /* DWARF2/GCC uses the stack address *before* the function call as a
     frame's CFA.  */
  return sp + 8;
}

/* Convert a dwarf or dwarf2 regnumber to a GDB regnum.  */

static int
m68k_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (num < 8)
    /* d0..7 */
    return (num - 0) + M68K_D0_REGNUM;
  else if (num < 16)
    /* a0..7 */
    return (num - 8) + M68K_A0_REGNUM;
  else if (num < 24 && tdep->fpregs_present)
    /* fp0..7 */
    return (num - 16) + M68K_FP0_REGNUM;
  else if (num == 25)
    /* pc */
    return M68K_PC_REGNUM;
  else
    return -1;
}


struct m68k_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR sp_offset;
  CORE_ADDR pc;

  /* Saved registers.  */
  CORE_ADDR saved_regs[M68K_NUM_REGS];
  CORE_ADDR saved_sp;

  /* Stack space reserved for local variables.  */
  long locals;
};

/* Allocate and initialize a frame cache.  */

static struct m68k_frame_cache *
m68k_alloc_frame_cache (void)
{
  struct m68k_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct m68k_frame_cache);

  /* Base address.  */
  cache->base = 0;
  cache->sp_offset = -4;
  cache->pc = 0;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset (that's where %fp is supposed to be stored).  */
  for (i = 0; i < M68K_NUM_REGS; i++)
    cache->saved_regs[i] = -1;

  /* Frameless until proven otherwise.  */
  cache->locals = -1;

  return cache;
}

/* Check whether PC points at a code that sets up a new stack frame.
   If so, it updates CACHE and returns the address of the first
   instruction after the sequence that sets removes the "hidden"
   argument from the stack or CURRENT_PC, whichever is smaller.
   Otherwise, return PC.  */

static CORE_ADDR
m68k_analyze_frame_setup (struct gdbarch *gdbarch,
			  CORE_ADDR pc, CORE_ADDR current_pc,
			  struct m68k_frame_cache *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int op;

  if (pc >= current_pc)
    return current_pc;

  op = read_memory_unsigned_integer (pc, 2, byte_order);

  if (op == P_LINKW_FP || op == P_LINKL_FP || op == P_PEA_FP)
    {
      cache->saved_regs[M68K_FP_REGNUM] = 0;
      cache->sp_offset += 4;
      if (op == P_LINKW_FP)
	{
	  /* link.w %fp, #-N */
	  /* link.w %fp, #0; adda.l #-N, %sp */
	  cache->locals = -read_memory_integer (pc + 2, 2, byte_order);

	  if (pc + 4 < current_pc && cache->locals == 0)
	    {
	      op = read_memory_unsigned_integer (pc + 4, 2, byte_order);
	      if (op == P_ADDAL_SP)
		{
		  cache->locals = read_memory_integer (pc + 6, 4, byte_order);
		  return pc + 10;
		}
	    }

	  return pc + 4;
	}
      else if (op == P_LINKL_FP)
	{
	  /* link.l %fp, #-N */
	  cache->locals = -read_memory_integer (pc + 2, 4, byte_order);
	  return pc + 6;
	}
      else
	{
	  /* pea (%fp); movea.l %sp, %fp */
	  cache->locals = 0;

	  if (pc + 2 < current_pc)
	    {
	      op = read_memory_unsigned_integer (pc + 2, 2, byte_order);

	      if (op == P_MOVEAL_SP_FP)
		{
		  /* move.l %sp, %fp */
		  return pc + 4;
		}
	    }

	  return pc + 2;
	}
    }
  else if ((op & 0170777) == P_SUBQW_SP || (op & 0170777) == P_SUBQL_SP)
    {
      /* subq.[wl] #N,%sp */
      /* subq.[wl] #8,%sp; subq.[wl] #N,%sp */
      cache->locals = (op & 07000) == 0 ? 8 : (op & 07000) >> 9;
      if (pc + 2 < current_pc)
	{
	  op = read_memory_unsigned_integer (pc + 2, 2, byte_order);
	  if ((op & 0170777) == P_SUBQW_SP || (op & 0170777) == P_SUBQL_SP)
	    {
	      cache->locals += (op & 07000) == 0 ? 8 : (op & 07000) >> 9;
	      return pc + 4;
	    }
	}
      return pc + 2;
    }
  else if (op == P_ADDAW_SP || op == P_LEA_SP_SP)
    {
      /* adda.w #-N,%sp */
      /* lea (-N,%sp),%sp */
      cache->locals = -read_memory_integer (pc + 2, 2, byte_order);
      return pc + 4;
    }
  else if (op == P_ADDAL_SP)
    {
      /* adda.l #-N,%sp */
      cache->locals = -read_memory_integer (pc + 2, 4, byte_order);
      return pc + 6;
    }

  return pc;
}

/* Check whether PC points at code that saves registers on the stack.
   If so, it updates CACHE and returns the address of the first
   instruction after the register saves or CURRENT_PC, whichever is
   smaller.  Otherwise, return PC.  */

static CORE_ADDR
m68k_analyze_register_saves (struct gdbarch *gdbarch, CORE_ADDR pc,
			     CORE_ADDR current_pc,
			     struct m68k_frame_cache *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (cache->locals >= 0)
    {
      CORE_ADDR offset;
      int op;
      int i, mask, regno;

      offset = -4 - cache->locals;
      while (pc < current_pc)
	{
	  op = read_memory_unsigned_integer (pc, 2, byte_order);
	  if (op == P_FMOVEMX_SP
	      && tdep->fpregs_present)
	    {
	      /* fmovem.x REGS,-(%sp) */
	      op = read_memory_unsigned_integer (pc + 2, 2, byte_order);
	      if ((op & 0xff00) == 0xe000)
		{
		  mask = op & 0xff;
		  for (i = 0; i < 16; i++, mask >>= 1)
		    {
		      if (mask & 1)
			{
			  cache->saved_regs[i + M68K_FP0_REGNUM] = offset;
			  offset -= 12;
			}
		    }
		  pc += 4;
		}
	      else
		break;
	    }
	  else if ((op & 0177760) == P_MOVEL_SP)
	    {
	      /* move.l %R,-(%sp) */
	      regno = op & 017;
	      cache->saved_regs[regno] = offset;
	      offset -= 4;
	      pc += 2;
	    }
	  else if (op == P_MOVEML_SP)
	    {
	      /* movem.l REGS,-(%sp) */
	      mask = read_memory_unsigned_integer (pc + 2, 2, byte_order);
	      for (i = 0; i < 16; i++, mask >>= 1)
		{
		  if (mask & 1)
		    {
		      cache->saved_regs[15 - i] = offset;
		      offset -= 4;
		    }
		}
	      pc += 4;
	    }
	  else
	    break;
	}
    }

  return pc;
}


/* Do a full analysis of the prologue at PC and update CACHE
   accordingly.  Bail out early if CURRENT_PC is reached.  Return the
   address where the analysis stopped.

   We handle all cases that can be generated by gcc.

   For allocating a stack frame:

   link.w %a6,#-N
   link.l %a6,#-N
   pea (%fp); move.l %sp,%fp
   link.w %a6,#0; add.l #-N,%sp
   subq.l #N,%sp
   subq.w #N,%sp
   subq.w #8,%sp; subq.w #N-8,%sp
   add.w #-N,%sp
   lea (-N,%sp),%sp
   add.l #-N,%sp

   For saving registers:

   fmovem.x REGS,-(%sp)
   move.l R1,-(%sp)
   move.l R1,-(%sp); move.l R2,-(%sp)
   movem.l REGS,-(%sp)

   For setting up the PIC register:

   lea (%pc,N),%a5

   */

static CORE_ADDR
m68k_analyze_prologue (struct gdbarch *gdbarch, CORE_ADDR pc,
		       CORE_ADDR current_pc, struct m68k_frame_cache *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int op;

  pc = m68k_analyze_frame_setup (gdbarch, pc, current_pc, cache);
  pc = m68k_analyze_register_saves (gdbarch, pc, current_pc, cache);
  if (pc >= current_pc)
    return current_pc;

  /* Check for GOT setup.  */
  op = read_memory_unsigned_integer (pc, 4, byte_order);
  if (op == P_LEA_PC_A5)
    {
      /* lea (%pc,N),%a5 */
      return pc + 8;
    }

  return pc;
}

/* Return PC of first real instruction.  */

static CORE_ADDR
m68k_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  struct m68k_frame_cache cache;
  CORE_ADDR pc;

  cache.locals = -1;
  pc = m68k_analyze_prologue (gdbarch, start_pc, (CORE_ADDR) -1, &cache);
  if (cache.locals < 0)
    return start_pc;
  return pc;
}

static CORE_ADDR
m68k_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  gdb_byte buf[8];

  frame_unwind_register (next_frame, gdbarch_pc_regnum (gdbarch), buf);
  return extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);
}

/* Normal frames.  */

static struct m68k_frame_cache *
m68k_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct m68k_frame_cache *cache;
  gdb_byte buf[4];
  int i;

  if (*this_cache)
    return (struct m68k_frame_cache *) *this_cache;

  cache = m68k_alloc_frame_cache ();
  *this_cache = cache;

  /* In principle, for normal frames, %fp holds the frame pointer,
     which holds the base address for the current stack frame.
     However, for functions that don't need it, the frame pointer is
     optional.  For these "frameless" functions the frame pointer is
     actually the frame pointer of the calling frame.  Signal
     trampolines are just a special case of a "frameless" function.
     They (usually) share their frame pointer with the frame that was
     in progress when the signal occurred.  */

  get_frame_register (this_frame, M68K_FP_REGNUM, buf);
  cache->base = extract_unsigned_integer (buf, 4, byte_order);
  if (cache->base == 0)
    return cache;

  /* For normal frames, %pc is stored at 4(%fp).  */
  cache->saved_regs[M68K_PC_REGNUM] = 4;

  cache->pc = get_frame_func (this_frame);
  if (cache->pc != 0)
    m68k_analyze_prologue (get_frame_arch (this_frame), cache->pc,
			   get_frame_pc (this_frame), cache);

  if (cache->locals < 0)
    {
      /* We didn't find a valid frame, which means that CACHE->base
	 currently holds the frame pointer for our calling frame.  If
	 we're at the start of a function, or somewhere half-way its
	 prologue, the function's frame probably hasn't been fully
	 setup yet.  Try to reconstruct the base address for the stack
	 frame by looking at the stack pointer.  For truly "frameless"
	 functions this might work too.  */

      get_frame_register (this_frame, M68K_SP_REGNUM, buf);
      cache->base = extract_unsigned_integer (buf, 4, byte_order)
		    + cache->sp_offset;
    }

  /* Now that we have the base address for the stack frame we can
     calculate the value of %sp in the calling frame.  */
  cache->saved_sp = cache->base + 8;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < M68K_NUM_REGS; i++)
    if (cache->saved_regs[i] != -1)
      cache->saved_regs[i] += cache->base;

  return cache;
}

static void
m68k_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct m68k_frame_cache *cache = m68k_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  /* See the end of m68k_push_dummy_call.  */
  *this_id = frame_id_build (cache->base + 8, cache->pc);
}

static struct value *
m68k_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			  int regnum)
{
  struct m68k_frame_cache *cache = m68k_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  if (regnum == M68K_SP_REGNUM && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < M68K_NUM_REGS && cache->saved_regs[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind m68k_frame_unwind =
{
  "m68k prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  m68k_frame_this_id,
  m68k_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
m68k_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct m68k_frame_cache *cache = m68k_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base m68k_frame_base =
{
  &m68k_frame_unwind,
  m68k_frame_base_address,
  m68k_frame_base_address,
  m68k_frame_base_address
};

static struct frame_id
m68k_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR fp;

  fp = get_frame_register_unsigned (this_frame, M68K_FP_REGNUM);

  /* See the end of m68k_push_dummy_call.  */
  return frame_id_build (fp + 8, get_frame_pc (this_frame));
}


/* Figure out where the longjmp will land.  Slurp the args out of the stack.
   We expect the first arg to be a pointer to the jmp_buf structure from which
   we extract the pc (JB_PC) that we will land at.  The pc is copied into PC.
   This routine returns true on success.  */

static int
m68k_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  gdb_byte *buf;
  CORE_ADDR sp, jb_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (tdep->jb_pc < 0)
    {
      internal_error (_("m68k_get_longjmp_target: not implemented"));
      return 0;
    }

  buf = (gdb_byte *) alloca (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
  sp = get_frame_register_unsigned (frame, gdbarch_sp_regnum (gdbarch));

  if (target_read_memory (sp + SP_ARG0,	/* Offset of first arg on stack.  */
			  buf, gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT))
    return 0;

  jb_addr = extract_unsigned_integer (buf, gdbarch_ptr_bit (gdbarch)
					     / TARGET_CHAR_BIT, byte_order);

  if (target_read_memory (jb_addr + tdep->jb_pc * tdep->jb_elt_size, buf,
			  gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT),
			  byte_order)
    return 0;

  *pc = extract_unsigned_integer (buf, gdbarch_ptr_bit (gdbarch)
					 / TARGET_CHAR_BIT, byte_order);
  return 1;
}


/* This is the implementation of gdbarch method
   return_in_first_hidden_param_p.  */

static int
m68k_return_in_first_hidden_param_p (struct gdbarch *gdbarch,
				     struct type *type)
{
  return 0;
}

/* System V Release 4 (SVR4).  */

void
m68k_svr4_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  /* SVR4 uses a different calling convention.  */
  set_gdbarch_return_value (gdbarch, m68k_svr4_return_value);

  /* SVR4 uses %a0 instead of %a1.  */
  tdep->struct_value_regnum = M68K_A0_REGNUM;

  /* SVR4 returns pointers in %a0.  */
  tdep->pointer_result_regnum = M68K_A0_REGNUM;
}

/* GCC's m68k "embedded" ABI.  This is like the SVR4 ABI, but pointer
   values are returned in %d0, not %a0.  */

static void
m68k_embedded_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  m68k_svr4_init_abi (info, gdbarch);
  tdep->pointer_result_regnum = M68K_D0_REGNUM;
}



/* Function: m68k_gdbarch_init
   Initializer function for the m68k gdbarch vector.
   Called by gdbarch.  Sets up the gdbarch vector(s) for this target.  */

static struct gdbarch *
m68k_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_list *best_arch;
  tdesc_arch_data_up tdesc_data;
  int i;
  enum m68k_flavour flavour = m68k_no_flavour;
  int has_fp = 1;
  const struct floatformat **long_double_format = floatformats_m68881_ext;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (info.target_desc))
    {
      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.m68k.core");

      if (feature == NULL)
	{
	  feature = tdesc_find_feature (info.target_desc,
					"org.gnu.gdb.coldfire.core");
	  if (feature != NULL)
	    flavour = m68k_coldfire_flavour;
	}

      if (feature == NULL)
	{
	  feature = tdesc_find_feature (info.target_desc,
					"org.gnu.gdb.fido.core");
	  if (feature != NULL)
	    flavour = m68k_fido_flavour;
	}

      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (i = 0; i <= M68K_PC_REGNUM; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					    m68k_register_names[i]);

      if (!valid_p)
	return NULL;

      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.coldfire.fp");
      if (feature != NULL)
	{
	  valid_p = 1;
	  for (i = M68K_FP0_REGNUM; i <= M68K_FPI_REGNUM; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
						m68k_register_names[i]);
	  if (!valid_p)
	    return NULL;
	}
      else
	has_fp = 0;
    }

  /* The mechanism for returning floating values from function
     and the type of long double depend on whether we're
     on ColdFire or standard m68k.  */

  if (info.bfd_arch_info && info.bfd_arch_info->mach != 0)
    {
      const bfd_arch_info_type *coldfire_arch = 
	bfd_lookup_arch (bfd_arch_m68k, bfd_mach_mcf_isa_a_nodiv);

      if (coldfire_arch
	  && ((*info.bfd_arch_info->compatible) 
	      (info.bfd_arch_info, coldfire_arch)))
	flavour = m68k_coldfire_flavour;
    }
  
  /* Try to figure out if the arch uses floating registers to return
     floating point values from functions.  On ColdFire, floating
     point values are returned in D0.  */
  int float_return = 0;
  if (has_fp && flavour != m68k_coldfire_flavour)
    float_return = 1;
#ifdef HAVE_ELF
  if (info.abfd && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    {
      int fp_abi = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
					     Tag_GNU_M68K_ABI_FP);
      if (fp_abi == 1)
	float_return = 1;
      else if (fp_abi == 2)
	float_return = 0;
    }
#endif /* HAVE_ELF */

  /* If there is already a candidate, use it.  */
  for (best_arch = gdbarch_list_lookup_by_info (arches, &info);
       best_arch != NULL;
       best_arch = gdbarch_list_lookup_by_info (best_arch->next, &info))
    {
      m68k_gdbarch_tdep *tdep
	= gdbarch_tdep<m68k_gdbarch_tdep> (best_arch->gdbarch);

      if (flavour != tdep->flavour)
	continue;

      if (has_fp != tdep->fpregs_present)
	continue;

      if (float_return != tdep->float_return)
	continue;

      break;
    }

  if (best_arch != NULL)
    return best_arch->gdbarch;

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new m68k_gdbarch_tdep));
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  tdep->fpregs_present = has_fp;
  tdep->float_return = float_return;
  tdep->flavour = flavour;

  if (flavour == m68k_coldfire_flavour || flavour == m68k_fido_flavour)
    long_double_format = floatformats_ieee_double;
  set_gdbarch_long_double_format (gdbarch, long_double_format);
  set_gdbarch_long_double_bit (gdbarch, long_double_format[0]->totalsize);

  set_gdbarch_skip_prologue (gdbarch, m68k_skip_prologue);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, m68k_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, m68k_breakpoint::bp_from_kind);

  /* Stack grows down.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_frame_align (gdbarch, m68k_frame_align);

  set_gdbarch_believe_pcc_promotion (gdbarch, 1);
  if (flavour == m68k_coldfire_flavour || flavour == m68k_fido_flavour)
    set_gdbarch_decr_pc_after_break (gdbarch, 2);

  set_gdbarch_frame_args_skip (gdbarch, 8);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, m68k_dwarf_reg_to_regnum);

  set_gdbarch_register_type (gdbarch, m68k_register_type);
  set_gdbarch_register_name (gdbarch, m68k_register_name);
  set_gdbarch_num_regs (gdbarch, M68K_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, M68K_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, M68K_PC_REGNUM);
  set_gdbarch_ps_regnum (gdbarch, M68K_PS_REGNUM);
  set_gdbarch_convert_register_p (gdbarch, m68k_convert_register_p);
  set_gdbarch_register_to_value (gdbarch,  m68k_register_to_value);
  set_gdbarch_value_to_register (gdbarch, m68k_value_to_register);

  if (has_fp)
    set_gdbarch_fp0_regnum (gdbarch, M68K_FP0_REGNUM);

  /* Function call & return.  */
  set_gdbarch_push_dummy_call (gdbarch, m68k_push_dummy_call);
  set_gdbarch_return_value (gdbarch, m68k_return_value);
  set_gdbarch_return_in_first_hidden_param_p (gdbarch,
					      m68k_return_in_first_hidden_param_p);

#if defined JB_PC && defined JB_ELEMENT_SIZE
  tdep->jb_pc = JB_PC;
  tdep->jb_elt_size = JB_ELEMENT_SIZE;
#else
  tdep->jb_pc = -1;
#endif
  tdep->pointer_result_regnum = M68K_D0_REGNUM;
  tdep->struct_value_regnum = M68K_A1_REGNUM;
  tdep->struct_return = reg_struct_return;

  /* Frame unwinder.  */
  set_gdbarch_dummy_id (gdbarch, m68k_dummy_id);
  set_gdbarch_unwind_pc (gdbarch, m68k_unwind_pc);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_append_unwinders (gdbarch);

  frame_base_set_default (gdbarch, &m68k_frame_base);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Now we have tuned the configuration, set a few final things,
     based on what the OS ABI has told us.  */

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, m68k_get_longjmp_target);

  frame_unwind_append_unwinder (gdbarch, &m68k_frame_unwind);

  if (tdesc_data != nullptr)
    tdesc_use_registers (gdbarch, info.target_desc, std::move (tdesc_data));

  return gdbarch;
}


static void
m68k_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  if (tdep == NULL)
    return;
}

/* OSABI sniffer for m68k.  */

static enum gdb_osabi
m68k_osabi_sniffer (bfd *abfd)
{
  unsigned int elfosabi = elf_elfheader (abfd)->e_ident[EI_OSABI];

  if (elfosabi == ELFOSABI_NONE)
    return GDB_OSABI_SVR4;

  return GDB_OSABI_UNKNOWN;
}

void _initialize_m68k_tdep ();
void
_initialize_m68k_tdep ()
{
  gdbarch_register (bfd_arch_m68k, m68k_gdbarch_init, m68k_dump_tdep);

  gdbarch_register_osabi_sniffer (bfd_arch_m68k, bfd_target_elf_flavour,
				  m68k_osabi_sniffer);
  gdbarch_register_osabi (bfd_arch_m68k, 0, GDB_OSABI_SVR4,
			  m68k_embedded_init_abi);
}

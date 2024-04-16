/* Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "regcache.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "infcall.h"
#include "ppc-tdep.h"
#include "target-float.h"
#include "value.h"
#include "xcoffread.h"

/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
rs6000_lynx178_push_dummy_call (struct gdbarch *gdbarch,
				struct value *function,
				struct regcache *regcache, CORE_ADDR bp_addr,
				int nargs, struct value **args, CORE_ADDR sp,
				function_call_return_method return_method,
				CORE_ADDR struct_addr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int ii;
  int len = 0;
  int argno;			/* current argument number */
  int argbytes;			/* current argument byte */
  gdb_byte tmp_buffer[50];
  int f_argno = 0;		/* current floating point argno */
  int wordsize = tdep->wordsize;

  struct value *arg = 0;
  struct type *type;

  ULONGEST saved_sp;

  /* The calling convention this function implements assumes the
     processor has floating-point registers.  We shouldn't be using it
     on PPC variants that lack them.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  /* The first eight words of ther arguments are passed in registers.
     Copy them appropriately.  */
  ii = 0;

  /* If the function is returning a `struct', then the first word
     (which will be passed in r3) is used for struct return address.
     In that case we should advance one word and start from r4
     register to copy parameters.  */
  if (return_method == return_method_struct)
    {
      regcache_raw_write_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
				   struct_addr);
      ii++;
    }

  /* Effectively indirect call... gcc does...

     return_val example( float, int);

     eabi:
     float in fp0, int in r3
     offset of stack on overflow 8/16
     for varargs, must go by type.
     power open:
     float in r3&r4, int in r5
     offset of stack on overflow different
     both:
     return in r3 or f0.  If no float, must study how gcc emulates floats;
     pay attention to arg promotion.
     User may have to cast\args to handle promotion correctly
     since gdb won't know if prototype supplied or not.  */

  for (argno = 0, argbytes = 0; argno < nargs && ii < 8; ++ii)
    {
      int reg_size = register_size (gdbarch, ii + 3);

      arg = args[argno];
      type = check_typedef (arg->type ());
      len = type->length ();

      if (type->code () == TYPE_CODE_FLT)
	{

	  /* Floating point arguments are passed in fpr's, as well as gpr's.
	     There are 13 fpr's reserved for passing parameters.  At this point
	     there is no way we would run out of them.

	     Always store the floating point value using the register's
	     floating-point format.  */
	  const int fp_regnum = tdep->ppc_fp0_regnum + 1 + f_argno;
	  gdb_byte reg_val[PPC_MAX_REGISTER_SIZE];
	  struct type *reg_type = register_type (gdbarch, fp_regnum);

	  gdb_assert (len <= 8);

	  target_float_convert (arg->contents ().data (), type, reg_val,
				reg_type);
	  regcache->cooked_write (fp_regnum, reg_val);
	  ++f_argno;
	}

      if (len > reg_size)
	{

	  /* Argument takes more than one register.  */
	  while (argbytes < len)
	    {
	      gdb_byte word[PPC_MAX_REGISTER_SIZE];
	      memset (word, 0, reg_size);
	      memcpy (word,
		      ((char *) arg->contents ().data ()) + argbytes,
		      (len - argbytes) > reg_size
			? reg_size : len - argbytes);
	      regcache->cooked_write (tdep->ppc_gp0_regnum + 3 + ii, word);
	      ++ii, argbytes += reg_size;

	      if (ii >= 8)
		goto ran_out_of_registers_for_arguments;
	    }
	  argbytes = 0;
	  --ii;
	}
      else
	{
	  /* Argument can fit in one register.  No problem.  */
	  gdb_byte word[PPC_MAX_REGISTER_SIZE];

	  memset (word, 0, reg_size);
	  memcpy (word, arg->contents ().data (), len);
	  regcache->cooked_write (tdep->ppc_gp0_regnum + 3 +ii, word);
	}
      ++argno;
    }

ran_out_of_registers_for_arguments:

  regcache_cooked_read_unsigned (regcache,
				 gdbarch_sp_regnum (gdbarch),
				 &saved_sp);

  /* Location for 8 parameters are always reserved.  */
  sp -= wordsize * 8;

  /* Another six words for back chain, TOC register, link register, etc.  */
  sp -= wordsize * 6;

  /* Stack pointer must be quadword aligned.  */
  sp = align_down (sp, 16);

  /* If there are more arguments, allocate space for them in
     the stack, then push them starting from the ninth one.  */

  if ((argno < nargs) || argbytes)
    {
      int space = 0, jj;

      if (argbytes)
	{
	  space += align_up (len - argbytes, 4);
	  jj = argno + 1;
	}
      else
	jj = argno;

      for (; jj < nargs; ++jj)
	{
	  struct value *val = args[jj];

	  space += align_up (val->type ()->length (), 4);
	}

      /* Add location required for the rest of the parameters.  */
      space = align_up (space, 16);
      sp -= space;

      /* This is another instance we need to be concerned about
	 securing our stack space.  If we write anything underneath %sp
	 (r1), we might conflict with the kernel who thinks he is free
	 to use this area.  So, update %sp first before doing anything
	 else.  */

      regcache_raw_write_signed (regcache,
				 gdbarch_sp_regnum (gdbarch), sp);

      /* If the last argument copied into the registers didn't fit there
	 completely, push the rest of it into stack.  */

      if (argbytes)
	{
	  write_memory (sp + 24 + (ii * 4),
			arg->contents ().data () + argbytes,
			len - argbytes);
	  ++argno;
	  ii += align_up (len - argbytes, 4) / 4;
	}

      /* Push the rest of the arguments into stack.  */
      for (; argno < nargs; ++argno)
	{

	  arg = args[argno];
	  type = check_typedef (arg->type ());
	  len = type->length ();


	  /* Float types should be passed in fpr's, as well as in the
	     stack.  */
	  if (type->code () == TYPE_CODE_FLT && f_argno < 13)
	    {

	      gdb_assert (len <= 8);

	      regcache->cooked_write (tdep->ppc_fp0_regnum + 1 + f_argno,
				      arg->contents ().data ());
	      ++f_argno;
	    }

	  write_memory (sp + 24 + (ii * 4), arg->contents ().data (), len);
	  ii += align_up (len, 4) / 4;
	}
    }

  /* Set the stack pointer.  According to the ABI, the SP is meant to
     be set _before_ the corresponding stack space is used.  On AIX,
     this even applies when the target has been completely stopped!
     Not doing this can lead to conflicts with the kernel which thinks
     that it still has control over this not-yet-allocated stack
     region.  */
  regcache_raw_write_signed (regcache, gdbarch_sp_regnum (gdbarch), sp);

  /* Set back chain properly.  */
  store_unsigned_integer (tmp_buffer, wordsize, byte_order, saved_sp);
  write_memory (sp, tmp_buffer, wordsize);

  /* Point the inferior function call's return address at the dummy's
     breakpoint.  */
  regcache_raw_write_signed (regcache, tdep->ppc_lr_regnum, bp_addr);

  target_store_registers (regcache, -1);
  return sp;
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
rs6000_lynx178_return_value (struct gdbarch *gdbarch, struct value *function,
			     struct type *valtype, struct regcache *regcache,
			     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* The calling convention this function implements assumes the
     processor has floating-point registers.  We shouldn't be using it
     on PowerPC variants that lack them.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  /* AltiVec extension: Functions that declare a vector data type as a
     return value place that return value in VR2.  */
  if (valtype->code () == TYPE_CODE_ARRAY && valtype->is_vector ()
      && valtype->length () == 16)
    {
      if (readbuf)
	regcache->cooked_read (tdep->ppc_vr0_regnum + 2, readbuf);
      if (writebuf)
	regcache->cooked_write (tdep->ppc_vr0_regnum + 2, writebuf);

      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* If the called subprogram returns an aggregate, there exists an
     implicit first argument, whose value is the address of a caller-
     allocated buffer into which the callee is assumed to store its
     return value.  All explicit parameters are appropriately
     relabeled.  */
  if (valtype->code () == TYPE_CODE_STRUCT
      || valtype->code () == TYPE_CODE_UNION
      || valtype->code () == TYPE_CODE_ARRAY)
    return RETURN_VALUE_STRUCT_CONVENTION;

  /* Scalar floating-point values are returned in FPR1 for float or
     double, and in FPR1:FPR2 for quadword precision.  Fortran
     complex*8 and complex*16 are returned in FPR1:FPR2, and
     complex*32 is returned in FPR1:FPR4.  */
  if (valtype->code () == TYPE_CODE_FLT
      && (valtype->length () == 4 || valtype->length () == 8))
    {
      struct type *regtype = register_type (gdbarch, tdep->ppc_fp0_regnum);
      gdb_byte regval[8];

      /* FIXME: kettenis/2007-01-01: Add support for quadword
	 precision and complex.  */

      if (readbuf)
	{
	  regcache->cooked_read (tdep->ppc_fp0_regnum + 1, regval);
	  target_float_convert (regval, regtype, readbuf, valtype);
	}
      if (writebuf)
	{
	  target_float_convert (writebuf, valtype, regval, regtype);
	  regcache->cooked_write (tdep->ppc_fp0_regnum + 1, regval);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
  }

  /* Values of the types int, long, short, pointer, and char (length
     is less than or equal to four bytes), as well as bit values of
     lengths less than or equal to 32 bits, must be returned right
     justified in GPR3 with signed values sign extended and unsigned
     values zero extended, as necessary.  */
  if (valtype->length () <= tdep->wordsize)
    {
      if (readbuf)
	{
	  ULONGEST regval;

	  /* For reading we don't have to worry about sign extension.  */
	  regcache_cooked_read_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
					 &regval);
	  store_unsigned_integer (readbuf, valtype->length (), byte_order,
				  regval);
	}
      if (writebuf)
	{
	  /* For writing, use unpack_long since that should handle any
	     required sign extension.  */
	  regcache_cooked_write_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
					  unpack_long (valtype, writebuf));
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* Eight-byte non-floating-point scalar values must be returned in
     GPR3:GPR4.  */

  if (valtype->length () == 8)
    {
      gdb_assert (valtype->code () != TYPE_CODE_FLT);
      gdb_assert (tdep->wordsize == 4);

      if (readbuf)
	{
	  gdb_byte regval[8];

	  regcache->cooked_read (tdep->ppc_gp0_regnum + 3, regval);
	  regcache->cooked_read (tdep->ppc_gp0_regnum + 4, regval + 4);
	  memcpy (readbuf, regval, 8);
	}
      if (writebuf)
	{
	  regcache->cooked_write (tdep->ppc_gp0_regnum + 3, writebuf);
	  regcache->cooked_write (tdep->ppc_gp0_regnum + 4, writebuf + 4);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  return RETURN_VALUE_STRUCT_CONVENTION;
}

/* PowerPC Lynx178 OSABI sniffer.  */

static enum gdb_osabi
rs6000_lynx178_osabi_sniffer (bfd *abfd)
{
  if (bfd_get_flavour (abfd) != bfd_target_xcoff_flavour)
    return GDB_OSABI_UNKNOWN;

  /* The only noticeable difference between Lynx178 XCOFF files and
     AIX XCOFF files comes from the fact that there are no shared
     libraries on Lynx178.  So if the number of import files is
     different from zero, it cannot be a Lynx178 binary.  */
  if (xcoff_get_n_import_files (abfd) != 0)
    return GDB_OSABI_UNKNOWN;

  return GDB_OSABI_LYNXOS178;
}

/* Callback for powerpc-lynx178 initialization.  */

static void
rs6000_lynx178_init_osabi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_push_dummy_call (gdbarch, rs6000_lynx178_push_dummy_call);
  set_gdbarch_return_value (gdbarch, rs6000_lynx178_return_value);
  set_gdbarch_long_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
}

void _initialize_rs6000_lynx178_tdep ();
void
_initialize_rs6000_lynx178_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_rs6000,
				  bfd_target_xcoff_flavour,
				  rs6000_lynx178_osabi_sniffer);
  gdbarch_register_osabi (bfd_arch_rs6000, 0, GDB_OSABI_LYNXOS178,
			  rs6000_lynx178_init_osabi);
}


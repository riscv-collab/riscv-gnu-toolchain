/* Darwin support for GDB, the GNU debugger.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.

   Contributed by Apple Computer, Inc.

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
#include "gdbcore.h"
#include "target.h"
#include "symtab.h"
#include "regcache.h"
#include "objfiles.h"

#include "i387-tdep.h"
#include "i386-tdep.h"
#include "osabi.h"
#include "ui-out.h"
#include "i386-darwin-tdep.h"
#include "solib.h"
#include "solib-darwin.h"
#include "dwarf2/frame.h"
#include <algorithm>

/* Offsets into the struct i386_thread_state where we'll find the saved regs.
   From <mach/i386/thread_status.h> and i386-tdep.h.  */
int i386_darwin_thread_state_reg_offset[] =
{
   0 * 4,   /* EAX */
   2 * 4,   /* ECX */
   3 * 4,   /* EDX */
   1 * 4,   /* EBX */
   7 * 4,   /* ESP */
   6 * 4,   /* EBP */
   5 * 4,   /* ESI */
   4 * 4,   /* EDI */
  10 * 4,   /* EIP */
   9 * 4,   /* EFLAGS */
  11 * 4,   /* CS */
   8 * 4,   /* SS */
  12 * 4,   /* DS */
  13 * 4,   /* ES */
  14 * 4,   /* FS */
  15 * 4    /* GS */
};

const int i386_darwin_thread_state_num_regs = 
  ARRAY_SIZE (i386_darwin_thread_state_reg_offset);

/* Assuming THIS_FRAME is a Darwin sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
i386_darwin_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR bp;
  CORE_ADDR si;
  gdb_byte buf[4];

  get_frame_register (this_frame, I386_EBP_REGNUM, buf);
  bp = extract_unsigned_integer (buf, 4, byte_order);

  /* A pointer to the ucontext is passed as the fourth argument
     to the signal handler.  */
  read_memory (bp + 24, buf, 4);
  si = extract_unsigned_integer (buf, 4, byte_order);

  /* The pointer to mcontext is at offset 28.  */
  read_memory (si + 28, buf, 4);

  /* First register (eax) is at offset 12.  */
  return extract_unsigned_integer (buf, 4, byte_order) + 12;
}

/* Return true if the PC of THIS_FRAME is in a signal trampoline which
   may have DWARF-2 CFI.

   On Darwin, signal trampolines have DWARF-2 CFI but it has only one FDE
   that covers only the indirect call to the user handler.
   Without this function, the frame is recognized as a normal frame which is
   not expected.  */

int
darwin_dwarf_signal_frame_p (struct gdbarch *gdbarch,
			     frame_info_ptr this_frame)
{
  return i386_sigtramp_p (this_frame);
}

/* Check whether TYPE is a 128-bit vector (__m128, __m128d or __m128i).  */

static int
i386_m128_p (struct type *type)
{
  return (type->code () == TYPE_CODE_ARRAY && type->is_vector ()
	  && type->length () == 16);
}

/* Return the alignment for TYPE when passed as an argument.  */

static int
i386_darwin_arg_type_alignment (struct type *type)
{
  type = check_typedef (type);
  /* According to Mac OS X ABI document (passing arguments):
     6.  The caller places 64-bit vectors (__m64) on the parameter area,
	 aligned to 8-byte boundaries.
     7.  [...]  The caller aligns 128-bit vectors in the parameter area to
	 16-byte boundaries.  */
  if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    return type->length ();
  /* 4.  The caller places all the fields of structures (or unions) with no
	 vector elements in the parameter area.  These structures are 4-byte
	 aligned.
     5.  The caller places structures with vector elements on the stack,
	 16-byte aligned.  */
  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION)
    {
      int i;
      int res = 4;
      for (i = 0; i < type->num_fields (); i++)
	{
	  int align
	    = i386_darwin_arg_type_alignment (type->field (i).type ());

	  res = std::max (res, align);
	}
      return res;
    }
  /* 2.  The caller aligns nonvector arguments to 4-byte boundaries.  */
  return 4;
}

static CORE_ADDR
i386_darwin_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			     struct regcache *regcache, CORE_ADDR bp_addr,
			     int nargs, struct value **args, CORE_ADDR sp,
			     function_call_return_method return_method,
			     CORE_ADDR struct_addr)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  int i;
  int write_pass;

  /* Determine the total space required for arguments and struct
     return address in a first pass, then push arguments in a second pass.  */

  for (write_pass = 0; write_pass < 2; write_pass++)
    {
      int args_space = 0;
      int num_m128 = 0;

      if (return_method == return_method_struct)
	{
	  if (write_pass)
	    {
	      /* Push value address.  */
	      store_unsigned_integer (buf, 4, byte_order, struct_addr);
	      write_memory (sp, buf, 4);
	    }
	  args_space += 4;
	}

      for (i = 0; i < nargs; i++)
	{
	  struct type *arg_type = args[i]->enclosing_type ();

	  if (i386_m128_p (arg_type) && num_m128 < 4)
	    {
	      if (write_pass)
		{
		  const gdb_byte *val = args[i]->contents_all ().data ();
		  regcache->raw_write (I387_MM0_REGNUM(tdep) + num_m128, val);
		}
	      num_m128++;
	    }
	  else
	    {
	      args_space = align_up (args_space,
				     i386_darwin_arg_type_alignment (arg_type));
	      if (write_pass)
		write_memory (sp + args_space,
			      args[i]->contents_all ().data (),
			      arg_type->length ());

	      /* The System V ABI says that:
		 
		 "An argument's size is increased, if necessary, to make it a
		 multiple of [32-bit] words.  This may require tail padding,
		 depending on the size of the argument."
		 
		 This makes sure the stack stays word-aligned.  */
	      args_space += align_up (arg_type->length (), 4);
	    }
	}

      /* Darwin i386 ABI:
	 1.  The caller ensures that the stack is 16-byte aligned at the point
	     of the function call.  */
      if (!write_pass)
	sp = align_down (sp - args_space, 16);
    }

  /* Store return address.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, bp_addr);
  write_memory (sp, buf, 4);

  /* Finally, update the stack pointer...  */
  store_unsigned_integer (buf, 4, byte_order, sp);
  regcache->cooked_write (I386_ESP_REGNUM, buf);

  /* ...and fake a frame pointer.  */
  regcache->cooked_write (I386_EBP_REGNUM, buf);

  /* MarkK wrote: This "+ 8" is all over the place:
     (i386_frame_this_id, i386_sigtramp_frame_this_id,
     i386_dummy_id).  It's there, since all frame unwinders for
     a given target have to agree (within a certain margin) on the
     definition of the stack address of a frame.  Otherwise frame id
     comparison might not work correctly.  Since DWARF2/GCC uses the
     stack address *before* the function call as a frame's CFA.  On
     the i386, when %ebp is used as a frame pointer, the offset
     between the contents %ebp and the CFA as defined by GCC.  */
  return sp + 8;
}

static void
i386_darwin_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* We support the SSE registers.  */
  tdep->num_xmm_regs = I386_NUM_XREGS - 1;
  set_gdbarch_num_regs (gdbarch, I386_SSE_NUM_REGS);

  dwarf2_frame_set_signal_frame_p (gdbarch, darwin_dwarf_signal_frame_p);
  set_gdbarch_push_dummy_call (gdbarch, i386_darwin_push_dummy_call);

  tdep->struct_return = reg_struct_return;

  tdep->sigtramp_p = i386_sigtramp_p;
  tdep->sigcontext_addr = i386_darwin_sigcontext_addr;
  tdep->sc_reg_offset = i386_darwin_thread_state_reg_offset;
  tdep->sc_num_regs = i386_darwin_thread_state_num_regs;

  tdep->jb_pc_offset = 48;

  /* Although the i387 extended floating-point has only 80 significant
     bits, a `long double' actually takes up 128, probably to enforce
     alignment.  */
  set_gdbarch_long_double_bit (gdbarch, 128);

  set_gdbarch_so_ops (gdbarch, &darwin_so_ops);
}

static enum gdb_osabi
i386_mach_o_osabi_sniffer (bfd *abfd)
{
  if (!bfd_check_format (abfd, bfd_object))
    return GDB_OSABI_UNKNOWN;
  
  if (bfd_get_arch (abfd) == bfd_arch_i386)
    return GDB_OSABI_DARWIN;

  return GDB_OSABI_UNKNOWN;
}

void _initialize_i386_darwin_tdep ();
void
_initialize_i386_darwin_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_unknown, bfd_target_mach_o_flavour,
				  i386_mach_o_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_i386_i386,
			  GDB_OSABI_DARWIN, i386_darwin_init_abi);
}

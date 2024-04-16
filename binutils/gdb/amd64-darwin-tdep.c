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
#include "gdbsupport/x86-xstate.h"
#include "amd64-tdep.h"
#include "osabi.h"
#include "ui-out.h"
#include "amd64-darwin-tdep.h"
#include "i386-darwin-tdep.h"
#include "solib.h"
#include "solib-darwin.h"
#include "dwarf2/frame.h"

/* Offsets into the struct x86_thread_state64 where we'll find the saved regs.
   From <mach/i386/thread_status.h> and amd64-tdep.h.  */
int amd64_darwin_thread_state_reg_offset[] =
{
  0 * 8,			/* %rax */
  1 * 8,			/* %rbx */
  2 * 8,			/* %rcx */
  3 * 8,			/* %rdx */
  5 * 8,			/* %rsi */
  4 * 8,			/* %rdi */
  6 * 8,			/* %rbp */
  7 * 8,			/* %rsp */
  8 * 8,			/* %r8 ...  */
  9 * 8,
  10 * 8,
  11 * 8,
  12 * 8,
  13 * 8,
  14 * 8,
  15 * 8,			/* ... %r15 */
  16 * 8,			/* %rip */
  17 * 8,			/* %rflags */
  18 * 8,			/* %cs */
  -1,				/* %ss */
  -1,				/* %ds */
  -1,				/* %es */
  19 * 8,			/* %fs */
  20 * 8			/* %gs */
};

const int amd64_darwin_thread_state_num_regs = 
  ARRAY_SIZE (amd64_darwin_thread_state_reg_offset);

/* Assuming THIS_FRAME is a Darwin sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
amd64_darwin_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR rbx;
  gdb_byte buf[8];

  /* A pointer to the ucontext is passed as the fourth argument
     to the signal handler, which is saved in rbx.  */
  get_frame_register (this_frame, AMD64_RBX_REGNUM, buf);
  rbx = extract_unsigned_integer (buf, 8, byte_order);

  /* The pointer to mcontext is at offset 48.  */
  read_memory (rbx + 48, buf, 8);

  /* First register (rax) is at offset 16.  */
  return extract_unsigned_integer (buf, 8, byte_order) + 16;
}

static void
x86_darwin_init_abi_64 (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  amd64_init_abi (info, gdbarch,
		  amd64_target_description (X86_XSTATE_SSE_MASK, true));

  tdep->struct_return = reg_struct_return;

  dwarf2_frame_set_signal_frame_p (gdbarch, darwin_dwarf_signal_frame_p);

  tdep->sigtramp_p = i386_sigtramp_p;
  tdep->sigcontext_addr = amd64_darwin_sigcontext_addr;
  tdep->sc_reg_offset = amd64_darwin_thread_state_reg_offset;
  tdep->sc_num_regs = amd64_darwin_thread_state_num_regs;

  tdep->jb_pc_offset = 56;

  set_gdbarch_so_ops (gdbarch, &darwin_so_ops);
}

void _initialize_amd64_darwin_tdep ();
void
_initialize_amd64_darwin_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_DARWIN, x86_darwin_init_abi_64);
}

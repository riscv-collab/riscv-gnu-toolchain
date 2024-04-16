/* GNU/Linux on  TI C6x target support.
   Copyright (C) 2011-2024 Free Software Foundation, Inc.
   Contributed by Yao Qi <yao@codesourcery.com>

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
#include "solib.h"
#include "osabi.h"
#include "linux-tdep.h"
#include "tic6x-tdep.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "elf-bfd.h"
#include "elf/tic6x.h"
#include "gdbarch.h"
#include "solib-dsbt.h"

/* The offset from rt_sigframe pointer to SP register.  */
#define TIC6X_SP_RT_SIGFRAME 8
/* Size of struct siginfo info.  */
#define TIC6X_SIGINFO_SIZE 128
/* Size of type stack_t, which contains three fields of type void*, int, and
   size_t respectively.  */
#define TIC6X_STACK_T_SIZE (3 * 4)

static const gdb_byte tic6x_bkpt_bnop_be[] = { 0x00, 0x00, 0xa1, 0x22 };
static const gdb_byte tic6x_bkpt_bnop_le[] = { 0x22, 0xa1, 0x00, 0x00 };

/* Return the offset of register REGNUM in struct sigcontext.  Return 0 if no
   such register in sigcontext.  */

static unsigned int
tic6x_register_sigcontext_offset (unsigned int regnum, struct gdbarch *gdbarch)
{
  tic6x_gdbarch_tdep *tdep = gdbarch_tdep<tic6x_gdbarch_tdep> (gdbarch);

  if (regnum == TIC6X_A4_REGNUM || regnum == TIC6X_A4_REGNUM + 2
      || regnum == TIC6X_A4_REGNUM + 4)
    return 4 * (regnum - TIC6X_A4_REGNUM + 2);	/* A4, A6, A8 */
  else if (regnum == TIC6X_A5_REGNUM || regnum == TIC6X_A5_REGNUM + 2
	   || regnum == TIC6X_A5_REGNUM + 4)
    return 4 * (regnum - TIC6X_A5_REGNUM + 12);	/* A5, A7, A9 */
  else if (regnum == TIC6X_B4_REGNUM || regnum == TIC6X_B4_REGNUM + 2
	   || regnum == TIC6X_B4_REGNUM + 4)
    return 4 * (regnum - TIC6X_B4_REGNUM + 3);	/* B4, B6, B8 */
  else if (regnum == TIC6X_B5_REGNUM || regnum == TIC6X_B5_REGNUM + 2
	   || regnum == TIC6X_B5_REGNUM + 4)
    return 4 * (regnum - TIC6X_B5_REGNUM + 19);	/* B5, B7, B9 */
  else if (regnum < TIC6X_A4_REGNUM)
    return 4 * (regnum - 0 + 8);	/* A0 - A3 */
  else if (regnum >= TIC6X_B0_REGNUM && regnum < TIC6X_B4_REGNUM)
    return 4 * (regnum - TIC6X_B0_REGNUM + 15);	/* B0 - B3 */
  else if (regnum >= 34 && regnum < 34 + 32)
    return 4 * (regnum - 34 + 23);	/* A16 - A31, B16 - B31 */
  else if (regnum == TIC6X_PC_REGNUM)
    return 4 * (tdep->has_gp ? 55 : 23);
  else if (regnum == TIC6X_SP_REGNUM)
    return 4;

  return 0;
}

/* Support unwinding frame in signal trampoline.  We don't check sigreturn,
   since it is not used in kernel.  */

static void
tic6x_linux_rt_sigreturn_init (const struct tramp_frame *self,
			       frame_info_ptr this_frame,
			       struct trad_frame_cache *this_cache,
			       CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, TIC6X_SP_REGNUM);
  /* The base of struct sigcontext is computed by examining the definition of
     struct rt_sigframe in linux kernel source arch/c6x/kernel/signal.c.  */
  CORE_ADDR base = (sp + TIC6X_SP_RT_SIGFRAME
		    /* Pointer type *pinfo and *puc in struct rt_sigframe.  */
		    + 4 + 4
		    + TIC6X_SIGINFO_SIZE
		    + 4 + 4 /* uc_flags and *uc_link in struct ucontext.  */
		    + TIC6X_STACK_T_SIZE);
  tic6x_gdbarch_tdep *tdep = gdbarch_tdep<tic6x_gdbarch_tdep> (gdbarch);
  unsigned int reg_offset;
  unsigned int i;

  for (i = 0; i < 10; i++)	/* A0 - A9 */
    {
      reg_offset = tic6x_register_sigcontext_offset (i, gdbarch);
      gdb_assert (reg_offset != 0);

      trad_frame_set_reg_addr (this_cache, i, base + reg_offset);
    }

  for (i = TIC6X_B0_REGNUM; i < TIC6X_B0_REGNUM + 10; i++)	/* B0 - B9 */
    {
      reg_offset = tic6x_register_sigcontext_offset (i, gdbarch);
      gdb_assert (reg_offset != 0);

      trad_frame_set_reg_addr (this_cache, i, base + reg_offset);
    }

  if (tdep->has_gp)
    for (i = 34; i < 34 + 32; i++)	/* A16 - A31, B16 - B31 */
      {
	reg_offset = tic6x_register_sigcontext_offset (i, gdbarch);
	gdb_assert (reg_offset != 0);

	trad_frame_set_reg_addr (this_cache, i, base + reg_offset);
      }

  trad_frame_set_reg_addr (this_cache, TIC6X_PC_REGNUM,
			   base + tic6x_register_sigcontext_offset (TIC6X_PC_REGNUM,
								    gdbarch));
  trad_frame_set_reg_addr (this_cache, TIC6X_SP_REGNUM,
			   base + tic6x_register_sigcontext_offset (TIC6X_SP_REGNUM,
								    gdbarch));

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static struct tramp_frame tic6x_linux_rt_sigreturn_tramp_frame =
{
  SIGTRAMP_FRAME,
  4,
  {
    {0x000045aa, 0x0fffffff},	/* mvk .S2 139,b0 */
    {0x10000000, ULONGEST_MAX},		/* swe */
    {TRAMP_SENTINEL_INSN}
  },
  tic6x_linux_rt_sigreturn_init
};

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
tic6x_linux_syscall_next_pc (frame_info_ptr frame)
{
  ULONGEST syscall_number = get_frame_register_unsigned (frame,
							 TIC6X_B0_REGNUM);
  CORE_ADDR pc = get_frame_pc (frame);

  if (syscall_number == 139 /* rt_sigreturn */)
    return frame_unwind_caller_pc (frame);

  return pc + 4;
}


static void
tic6x_uclinux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  tic6x_gdbarch_tdep *tdep = gdbarch_tdep<tic6x_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  /* Shared library handling.  */
  set_gdbarch_so_ops (gdbarch, &dsbt_so_ops);

  tdep->syscall_next_pc = tic6x_linux_syscall_next_pc;

#ifdef HAVE_ELF
  /* In tic6x Linux kernel, breakpoint instructions varies on different archs.
     On C64x+ and C67x+, breakpoint instruction is 0x56454314, which is an
     illegal opcode.  On other arch, breakpoint instruction is 0x0000a122
     (BNOP .S2 0,5).  */
  if (info.abfd)
    switch (bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC, Tag_ISA))
      {
      case C6XABI_Tag_ISA_C64XP:
      case C6XABI_Tag_ISA_C67XP:
	if (info.byte_order == BFD_ENDIAN_BIG)
	  tdep->breakpoint = tic6x_bkpt_illegal_opcode_be;
	else
	  tdep->breakpoint = tic6x_bkpt_illegal_opcode_le;
	break;
      default:
	{
	  if (info.byte_order == BFD_ENDIAN_BIG)
	    tdep->breakpoint = tic6x_bkpt_bnop_be;
	  else
	    tdep->breakpoint = tic6x_bkpt_bnop_le;
	}
      }
#endif

  /* Signal trampoline support.  */
  tramp_frame_prepend_unwinder (gdbarch,
				&tic6x_linux_rt_sigreturn_tramp_frame);
}

void _initialize_tic6x_linux_tdep ();
void
_initialize_tic6x_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_tic6x, 0, GDB_OSABI_LINUX,
			  tic6x_uclinux_init_abi);
}

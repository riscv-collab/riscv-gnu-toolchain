/* Target-dependent code for OpenBSD/alpha.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "osabi.h"

#include "obsd-tdep.h"
#include "alpha-tdep.h"
#include "alpha-bsd-tdep.h"
#include "solib-svr4.h"

/* Signal trampolines.  */

/* The OpenBSD kernel maps the signal trampoline at some random
   location in user space, which means that the traditional BSD way of
   detecting it won't work.

   The signal trampoline will be mapped at an address that is page
   aligned.  We recognize the signal trampoline by looking for the
   sigreturn system call.  */

static const int alphaobsd_page_size = 8192;

static LONGEST
alphaobsd_sigtramp_offset (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  return (pc & (alphaobsd_page_size - 1));
}

static int
alphaobsd_pc_in_sigtramp (struct gdbarch *gdbarch,
			  CORE_ADDR pc, const char *name)
{
  CORE_ADDR start_pc = (pc & ~(alphaobsd_page_size - 1));
  unsigned insn;

  if (name)
    return 0;

  /* Check for "".  */
  insn = alpha_read_insn (gdbarch, start_pc + 5 * ALPHA_INSN_SIZE);
  if (insn != 0x201f0067)
    return 0;

  /* Check for "".  */
  insn = alpha_read_insn (gdbarch, start_pc + 6 * ALPHA_INSN_SIZE);
  if (insn != 0x00000083)
    return 0;

  return 1;
}

static CORE_ADDR
alphaobsd_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);

  if (alphaobsd_sigtramp_offset (gdbarch, pc) < 3 * ALPHA_INSN_SIZE)
    {
      /* On entry, a pointer the `struct sigcontext' is passed in %a2.  */
      return get_frame_register_unsigned (this_frame, ALPHA_A0_REGNUM + 2);
    }
  else if (alphaobsd_sigtramp_offset (gdbarch, pc) < 4 * ALPHA_INSN_SIZE)
    {
      /* It is stored on the stack Before calling the signal handler.  */
      CORE_ADDR sp;
      sp = get_frame_register_unsigned (this_frame, ALPHA_SP_REGNUM);
      return get_frame_memory_unsigned (this_frame, sp, 8);
    }
  else
    {
      /* It is reloaded into %a0 for the sigreturn(2) call.  */
      return get_frame_register_unsigned (this_frame, ALPHA_A0_REGNUM);
    }
}


static void
alphaobsd_init_abi(struct gdbarch_info info, struct gdbarch *gdbarch)
{
  alpha_gdbarch_tdep *tdep = gdbarch_tdep<alpha_gdbarch_tdep> (gdbarch);

  /* Hook into the DWARF CFI frame unwinder.  */
  alpha_dwarf2_init_abi (info, gdbarch);

  /* Hook into the MDEBUG frame unwinder.  */
  alpha_mdebug_init_abi (info, gdbarch);

  /* OpenBSD/alpha 3.0 and earlier does not provide single step
     support via ptrace(2); use software single-stepping for now.  */
  set_gdbarch_software_single_step (gdbarch, alpha_software_single_step);

  /* OpenBSD/alpha has SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);
  set_gdbarch_skip_solib_resolver (gdbarch, obsd_skip_solib_resolver);

  tdep->dynamic_sigtramp_offset = alphaobsd_sigtramp_offset;
  tdep->pc_in_sigtramp = alphaobsd_pc_in_sigtramp;
  tdep->sigcontext_addr = alphaobsd_sigcontext_addr;

  tdep->jb_pc = 2;
  tdep->jb_elt_size = 8;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, alphanbsd_iterate_over_regset_sections);
}


void _initialize_alphaobsd_tdep ();
void
_initialize_alphaobsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_alpha, 0, GDB_OSABI_OPENBSD,
			  alphaobsd_init_abi);
}

/* Target-dependent code for GNU/Linux on OpenRISC processors.
   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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
#include "or1k-tdep.h"
#include "osabi.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "solib-svr4.h"
#include "regset.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "gdbarch.h"

#include "features/or1k-linux.c"

/* Define the general register mapping.  The kernel and GDB put registers
   r1 to r31 in the same place.  The NPC register is stored at index 32 in
   linux and 33 in GDB, in GDB 32 is for PPC which is not populated from linux.
   Register r0 is always 0 and can be ignored.  */

static const struct regcache_map_entry or1k_linux_gregmap[] =
{
  { 32, OR1K_ZERO_REGNUM, 4 }, /* r0 to r31 */
  { 1,  OR1K_NPC_REGNUM, 4 },
  { 0 }
};

/* Define the general register regset.  */

static const struct regset or1k_linux_gregset =
{
  or1k_linux_gregmap, regcache_supply_regset, regcache_collect_regset
};

/* Define hook for core file support.  */

static void
or1k_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  cb (".reg", (33 * 4), (33 * 4), &or1k_linux_gregset, NULL, cb_data);
}

/* Signal trampoline support.  */

static void or1k_linux_sigframe_init (const struct tramp_frame *self,
				       frame_info_ptr this_frame,
				       struct trad_frame_cache *this_cache,
				       CORE_ADDR func);

#define OR1K_RT_SIGRETURN		139

#define OR1K_INST_L_ORI_R11_R0_IMM	0xa9600000
#define OR1K_INST_L_SYS_1		0x20000001
#define OR1K_INST_L_NOP			0x15000000

static const struct tramp_frame or1k_linux_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { OR1K_INST_L_ORI_R11_R0_IMM | OR1K_RT_SIGRETURN, ULONGEST_MAX },
    { OR1K_INST_L_SYS_1, ULONGEST_MAX },
    { OR1K_INST_L_NOP, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  or1k_linux_sigframe_init,
  NULL
};

/* Runtime signal frames look like this:
  struct rt_sigframe {
    struct siginfo info;
    struct ucontext uc;
    unsigned char retcode[16];
  };

  struct ucontext {
    unsigned long     uc_flags;     - 4
    struct ucontext  *uc_link;      - 4
    stack_t           uc_stack;     - 4 * 3
    struct sigcontext uc_mcontext;
    sigset_t          uc_sigmask;
  };

  struct sigcontext {
    struct user_regs_struct regs;
    unsigned long oldmask;
  };

  struct user_regs_struct {
    unsigned long gpr[32];
    unsigned long pc;
    unsigned long sr;
  };  */

#define SIGFRAME_SIGINFO_SIZE		128
#define UCONTEXT_MCONTEXT_OFFSET	20

static void
or1k_linux_sigframe_init (const struct tramp_frame *self,
			   frame_info_ptr this_frame,
			   struct trad_frame_cache *this_cache,
			   CORE_ADDR func)
{
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR mcontext_base;
  CORE_ADDR regs_base;

  mcontext_base = frame_sp + SIGFRAME_SIGINFO_SIZE + UCONTEXT_MCONTEXT_OFFSET;

  /* Handle the general registers 0-31 followed by the PC.  */
  regs_base = mcontext_base;
  for (int i = 0; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, OR1K_ZERO_REGNUM + i,
			     regs_base + (i * 4));
  trad_frame_set_reg_addr (this_cache, OR1K_NPC_REGNUM, regs_base + (32 * 4));
  trad_frame_set_reg_addr (this_cache, OR1K_SR_REGNUM, regs_base + (33 * 4));

  /* Choice of the bottom of the sigframe is somewhat arbitrary.  */
  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* Initialize OpenRISC Linux ABI info.  */

static void
or1k_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch, 0);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  set_gdbarch_software_single_step (gdbarch, or1k_software_single_step);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, or1k_linux_iterate_over_regset_sections);

  tramp_frame_prepend_unwinder (gdbarch, &or1k_linux_sigframe);
}

/* Initialize OpenRISC Linux target support.  */

void _initialize_or1k_linux_tdep ();
void
_initialize_or1k_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_or1k, 0, GDB_OSABI_LINUX,
			  or1k_linux_init_abi);

  /* Initialize the standard target descriptions.  */
  initialize_tdesc_or1k_linux ();
}

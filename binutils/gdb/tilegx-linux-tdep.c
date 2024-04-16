/* Target-dependent code for GNU/Linux on Tilera TILE-Gx processors.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "linux-tdep.h"
#include "glibc-tdep.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "regcache.h"
#include "regset.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "tilegx-tdep.h"
#include "gdbarch.h"

/* Signal trampoline support.  */

static void
tilegx_linux_sigframe_init (const struct tramp_frame *self,
			    frame_info_ptr this_frame,
			    struct trad_frame_cache *this_cache,
			    CORE_ADDR func)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, 54);

  /* Base address of register save area.  */
  CORE_ADDR base = sp
		   + 16    /* Skip ABI_SAVE_AREA.  */
		   + 128   /* Skip SIGINFO.  */
		   + 40;   /* Skip UCONTEXT.  */

  /* Address of saved LR register (R56) which holds previous PC.  */
  CORE_ADDR prev_pc = base + 56 * 8;

  int i;

  for (i = 0; i < 56; i++)
    trad_frame_set_reg_addr (this_cache, i, base + i * 8);

  trad_frame_set_reg_value (this_cache, 64,
			    get_frame_memory_unsigned (this_frame, prev_pc, 8));

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

static const struct tramp_frame tilegx_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  8,
  {
    { 0x00045fe551483000ULL, ULONGEST_MAX }, /* { moveli r10, 139 } */
    { 0x286b180051485000ULL, ULONGEST_MAX }, /* { swint1 } */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  tilegx_linux_sigframe_init
};

/* Register map; must match struct pt_regs in "ptrace.h".  */

static const struct regcache_map_entry tilegx_linux_regmap[] =
  {
    { TILEGX_NUM_EASY_REGS, TILEGX_FIRST_EASY_REGNUM, 8 },
    { 1, TILEGX_PC_REGNUM, 8 },
    { 1, TILEGX_FAULTNUM_REGNUM, 8 },
    { 0 }
  };

#define TILEGX_LINUX_SIZEOF_GREGSET (64 * 8)

/* TILE-Gx Linux kernel register set.  */

static const struct regset tilegx_linux_regset =
{
  tilegx_linux_regmap,
  regcache_supply_regset, regcache_collect_regset
};


static void
tilegx_iterate_over_regset_sections (struct gdbarch *gdbarch,
				     iterate_over_regset_sections_cb *cb,
				     void *cb_data,
				     const struct regcache *regcache)
{
  cb (".reg", TILEGX_LINUX_SIZEOF_GREGSET, TILEGX_LINUX_SIZEOF_GREGSET,
      &tilegx_linux_regset, NULL, cb_data);
}

/* OS specific initialization of gdbarch.  */

static void
tilegx_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  int arch_size = gdbarch_addr_bit (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  tramp_frame_prepend_unwinder (gdbarch, &tilegx_linux_rt_sigframe);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, tilegx_iterate_over_regset_sections);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  if (arch_size == 32)
    set_solib_svr4_fetch_link_map_offsets (gdbarch,
					   linux_ilp32_fetch_link_map_offsets);
  else
    set_solib_svr4_fetch_link_map_offsets (gdbarch,
					   linux_lp64_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);
}

void _initialize_tilegx_linux_tdep ();
void
_initialize_tilegx_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_tilegx, bfd_mach_tilegx, GDB_OSABI_LINUX,
			  tilegx_linux_init_abi);
}

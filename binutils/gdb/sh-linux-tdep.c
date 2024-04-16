/* Target-dependent code for GNU/Linux Super-H.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

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

#include "solib-svr4.h"
#include "symtab.h"

#include "trad-frame.h"
#include "tramp-frame.h"

#include "glibc-tdep.h"
#include "sh-tdep.h"
#include "linux-tdep.h"
#include "gdbarch.h"

#define REGSx16(base) \
  {(base),      0}, \
  {(base) +  1, 4}, \
  {(base) +  2, 8}, \
  {(base) +  3, 12}, \
  {(base) +  4, 16}, \
  {(base) +  5, 20}, \
  {(base) +  6, 24}, \
  {(base) +  7, 28}, \
  {(base) +  8, 32}, \
  {(base) +  9, 36}, \
  {(base) + 10, 40}, \
  {(base) + 11, 44}, \
  {(base) + 12, 48}, \
  {(base) + 13, 52}, \
  {(base) + 14, 56}, \
  {(base) + 15, 60}

/* Describe the contents of the .reg section of the core file.  */

static const struct sh_corefile_regmap gregs_table[] =
{
  REGSx16 (R0_REGNUM),
  {PC_REGNUM,   64},
  {PR_REGNUM,   68},
  {SR_REGNUM,   72},
  {GBR_REGNUM,  76},
  {MACH_REGNUM, 80},
  {MACL_REGNUM, 84},
  {-1 /* Terminator.  */, 0}
};

/* Describe the contents of the .reg2 section of the core file.  */

static const struct sh_corefile_regmap fpregs_table[] =
{
  REGSx16 (FR0_REGNUM),
  /* REGSx16 xfp_regs omitted.  */
  {FPSCR_REGNUM, 128},
  {FPUL_REGNUM,  132},
  {-1 /* Terminator.  */, 0}
};

/* SH signal handler frame support.  */

static void
sh_linux_sigtramp_cache (frame_info_ptr this_frame,
			 struct trad_frame_cache *this_cache,
			 CORE_ADDR func, int regs_offset)
{
  int i;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR base = get_frame_register_unsigned (this_frame,
						gdbarch_sp_regnum (gdbarch));
  CORE_ADDR regs = base + regs_offset;

  for (i = 0; i < 18; i++)
    trad_frame_set_reg_addr (this_cache, i, regs + i * 4);

  trad_frame_set_reg_addr (this_cache, SR_REGNUM, regs + 18 * 4);
  trad_frame_set_reg_addr (this_cache, GBR_REGNUM, regs + 19 * 4);
  trad_frame_set_reg_addr (this_cache, MACH_REGNUM, regs + 20 * 4);
  trad_frame_set_reg_addr (this_cache, MACL_REGNUM, regs + 21 * 4);

  /* Restore FP state if we have an FPU.  */
  if (gdbarch_fp0_regnum (gdbarch) != -1)
    {
      CORE_ADDR fpregs = regs + 22 * 4;
      for (i = FR0_REGNUM; i <= FP_LAST_REGNUM; i++)
	trad_frame_set_reg_addr (this_cache, i, fpregs + i * 4);
      trad_frame_set_reg_addr (this_cache, FPSCR_REGNUM, fpregs + 32 * 4);
      trad_frame_set_reg_addr (this_cache, FPUL_REGNUM, fpregs + 33 * 4);
    }

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

/* Implement struct tramp_frame "init" callbacks for signal
   trampolines on 32-bit SH.  */

static void
sh_linux_sigreturn_init (const struct tramp_frame *self,
			 frame_info_ptr this_frame,
			 struct trad_frame_cache *this_cache,
			 CORE_ADDR func)
{
  /* SH 32-bit sigframe: sigcontext at start of sigframe,
     registers start after a single 'oldmask' word.  */
  sh_linux_sigtramp_cache (this_frame, this_cache, func, 4);
}

static void
sh_linux_rt_sigreturn_init (const struct tramp_frame *self,
			    frame_info_ptr this_frame,
			    struct trad_frame_cache *this_cache,
			    CORE_ADDR func)
{
  /* SH 32-bit rt_sigframe: starts with a siginfo (128 bytes), then
     we can find sigcontext embedded within a ucontext (offset 20 bytes).
     Then registers start after a single 'oldmask' word.  */
  sh_linux_sigtramp_cache (this_frame, this_cache, func,
			   128 /* sizeof (struct siginfo)  */
			   + 20 /* offsetof (struct ucontext, uc_mcontext) */
			   + 4 /* oldmask word at start of sigcontext */);
}

/* Instruction patterns.  */
#define SH_MOVW     0x9305
#define SH_TRAP     0xc300
#define SH_OR_R0_R0 0x200b       

/* SH sigreturn syscall numbers.  */
#define SH_NR_SIGRETURN 0x0077
#define SH_NR_RT_SIGRETURN 0x00ad

static struct tramp_frame sh_linux_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  2,
  {
    { SH_MOVW, 0xffff },
    { SH_TRAP, 0xff00 }, /* #imm argument part filtered out.  */
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_NR_SIGRETURN, 0xffff },
    { TRAMP_SENTINEL_INSN }
  },
  sh_linux_sigreturn_init
};

static struct tramp_frame sh_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  2,
  {
    { SH_MOVW, 0xffff },
    { SH_TRAP, 0xff00 }, /* #imm argument part filtered out.  */
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_OR_R0_R0, 0xffff },
    { SH_NR_RT_SIGRETURN, 0xffff },
    { TRAMP_SENTINEL_INSN }
  },
  sh_linux_rt_sigreturn_init
};

static void
sh_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch, 0);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  sh_gdbarch_tdep *tdep = gdbarch_tdep<sh_gdbarch_tdep> (gdbarch);

  /* Remember regset characteristics.  The sizes should match
     elf_gregset_t and elf_fpregset_t from Linux.  */
  tdep->core_gregmap = (struct sh_corefile_regmap *) gregs_table;
  tdep->sizeof_gregset = 92;
  tdep->core_fpregmap = (struct sh_corefile_regmap *) fpregs_table;
  tdep->sizeof_fpregset = 136;

  tramp_frame_prepend_unwinder (gdbarch, &sh_linux_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch, &sh_linux_rt_sigreturn_tramp_frame);
}

void _initialize_sh_linux_tdep ();
void
_initialize_sh_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sh, 0, GDB_OSABI_LINUX, sh_linux_init_abi);
}

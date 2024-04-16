/* Target-dependent code for FreeBSD/amd64.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "regset.h"
#include "target.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "i386-fbsd-tdep.h"
#include "gdbsupport/x86-xstate.h"

#include "amd64-tdep.h"
#include "amd64-fbsd-tdep.h"
#include "fbsd-tdep.h"
#include "solib-svr4.h"
#include "inferior.h"

/* The general-purpose regset consists of 22 64-bit slots, most of
   which contain individual registers, but a few contain multiple
   16-bit segment registers.  */
#define AMD64_FBSD_SIZEOF_GREGSET	(22 * 8)

/* The segment base register set consists of 2 64-bit registers.  */
#define AMD64_FBSD_SIZEOF_SEGBASES_REGSET	(2 * 8)

/* Register maps.  */

static const struct regcache_map_entry amd64_fbsd_gregmap[] =
{
  { 1, AMD64_R15_REGNUM, 0 },
  { 1, AMD64_R14_REGNUM, 0 },
  { 1, AMD64_R13_REGNUM, 0 },
  { 1, AMD64_R12_REGNUM, 0 },
  { 1, AMD64_R11_REGNUM, 0 },
  { 1, AMD64_R10_REGNUM, 0 },
  { 1, AMD64_R9_REGNUM, 0 },
  { 1, AMD64_R8_REGNUM, 0 },
  { 1, AMD64_RDI_REGNUM, 0 },
  { 1, AMD64_RSI_REGNUM, 0 },
  { 1, AMD64_RBP_REGNUM, 0 },
  { 1, AMD64_RBX_REGNUM, 0 },
  { 1, AMD64_RDX_REGNUM, 0 },
  { 1, AMD64_RCX_REGNUM, 0 },
  { 1, AMD64_RAX_REGNUM, 0 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* trapno */
  { 1, AMD64_FS_REGNUM, 2 },
  { 1, AMD64_GS_REGNUM, 2 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* err */
  { 1, AMD64_ES_REGNUM, 2 },
  { 1, AMD64_DS_REGNUM, 2 },
  { 1, AMD64_RIP_REGNUM, 0 },
  { 1, AMD64_CS_REGNUM, 8 },
  { 1, AMD64_EFLAGS_REGNUM, 8 },
  { 1, AMD64_RSP_REGNUM, 0 },
  { 1, AMD64_SS_REGNUM, 8 },
  { 0 }
};

static const struct regcache_map_entry amd64_fbsd_segbases_regmap[] =
{
  { 1, AMD64_FSBASE_REGNUM, 0 },
  { 1, AMD64_GSBASE_REGNUM, 0 },
  { 0 }
};

/* This layout including fsbase and gsbase was adopted in FreeBSD
   8.0.  */

static const struct regcache_map_entry amd64_fbsd_mcregmap[] =
{
  { 1, REGCACHE_MAP_SKIP, 8 },	/* mc_onstack */
  { 1, AMD64_RDI_REGNUM, 0 },
  { 1, AMD64_RSI_REGNUM, 0 },
  { 1, AMD64_RDX_REGNUM, 0 },
  { 1, AMD64_RCX_REGNUM, 0 },
  { 1, AMD64_R8_REGNUM, 0 },
  { 1, AMD64_R9_REGNUM, 0 },
  { 1, AMD64_RAX_REGNUM, 0 },
  { 1, AMD64_RBX_REGNUM, 0 },
  { 1, AMD64_RBP_REGNUM, 0 },
  { 1, AMD64_R10_REGNUM, 0 },
  { 1, AMD64_R11_REGNUM, 0 },
  { 1, AMD64_R12_REGNUM, 0 },
  { 1, AMD64_R13_REGNUM, 0 },
  { 1, AMD64_R14_REGNUM, 0 },
  { 1, AMD64_R15_REGNUM, 0 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_trapno */
  { 1, AMD64_FS_REGNUM, 2 },
  { 1, AMD64_GS_REGNUM, 2 },
  { 1, REGCACHE_MAP_SKIP, 8 },	/* mc_addr */
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_flags */
  { 1, AMD64_ES_REGNUM, 2 },
  { 1, AMD64_DS_REGNUM, 2 },
  { 1, REGCACHE_MAP_SKIP, 8 },	/* mc_err */
  { 1, AMD64_RIP_REGNUM, 0 },
  { 1, AMD64_CS_REGNUM, 8 },
  { 1, AMD64_EFLAGS_REGNUM, 8 },
  { 1, AMD64_RSP_REGNUM, 0 },
  { 1, AMD64_SS_REGNUM, 8 },
  { 1, REGCACHE_MAP_SKIP, 8 },	/* mc_len */
  { 1, REGCACHE_MAP_SKIP, 8 },	/* mc_fpformat */
  { 1, REGCACHE_MAP_SKIP, 8 },	/* mc_ownedfp */
  { 64, REGCACHE_MAP_SKIP, 8 },	/* mc_fpstate */
  { 1, AMD64_FSBASE_REGNUM, 0 },
  { 1, AMD64_GSBASE_REGNUM, 0 },
  { 0 }
};

/* Register set definitions.  */

const struct regset amd64_fbsd_gregset =
{
  amd64_fbsd_gregmap, regcache_supply_regset, regcache_collect_regset
};

const struct regset amd64_fbsd_segbases_regset =
{
  amd64_fbsd_segbases_regmap, regcache_supply_regset, regcache_collect_regset
};

/* Support for signal handlers.  */

/* In a signal frame, rsp points to a 'struct sigframe' which is
   defined as:

   struct sigframe {
	union {
		__siginfohandler_t	*sf_action;
		__sighandler_t		*sf_handler;
	} sf_ahu;
	ucontext_t	sf_uc;
	...
   }

   ucontext_t is defined as:

   struct __ucontext {
	   sigset_t	uc_sigmask;
	   mcontext_t	uc_mcontext;
	   ...
   };

   The mcontext_t contains the general purpose register set as well
   as the floating point or XSAVE state.  */

/* NB: There is an 8 byte padding hole between sf_ahu and sf_uc. */
#define AMD64_SIGFRAME_UCONTEXT_OFFSET 		16
#define AMD64_UCONTEXT_MCONTEXT_OFFSET		16
#define AMD64_SIZEOF_MCONTEXT_T			800

/* Implement the "init" method of struct tramp_frame.  */

static void
amd64_fbsd_sigframe_init (const struct tramp_frame *self,
			  frame_info_ptr this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, AMD64_RSP_REGNUM);
  CORE_ADDR mcontext_addr
    = (sp
       + AMD64_SIGFRAME_UCONTEXT_OFFSET
       + AMD64_UCONTEXT_MCONTEXT_OFFSET);

  trad_frame_set_reg_regmap (this_cache, amd64_fbsd_mcregmap, mcontext_addr,
			     AMD64_SIZEOF_MCONTEXT_T);

  /* Don't bother with floating point or XSAVE state for now.  The
     current helper routines for parsing FXSAVE and XSAVE state only
     work with regcaches.  This could perhaps create a temporary
     regcache, collect the register values from mc_fpstate and
     mc_xfpustate, and then set register values in the trad_frame.  */

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static const struct tramp_frame amd64_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  1,
  {
    {0x48, ULONGEST_MAX},		/* lea	   SIGF_UC(%rsp),%rdi */
    {0x8d, ULONGEST_MAX},
    {0x7c, ULONGEST_MAX},
    {0x24, ULONGEST_MAX},
    {0x10, ULONGEST_MAX},
    {0x6a, ULONGEST_MAX},		/* pushq   $0 */
    {0x00, ULONGEST_MAX},
    {0x48, ULONGEST_MAX},		/* movq	   $SYS_sigreturn,%rax */
    {0xc7, ULONGEST_MAX},
    {0xc0, ULONGEST_MAX},
    {0xa1, ULONGEST_MAX},
    {0x01, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x0f, ULONGEST_MAX},		/* syscall */
    {0x05, ULONGEST_MAX},
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  amd64_fbsd_sigframe_init
};

/* Implement the core_read_description gdbarch method.  */

static const struct target_desc *
amd64fbsd_core_read_description (struct gdbarch *gdbarch,
				 struct target_ops *target,
				 bfd *abfd)
{
  x86_xsave_layout layout;
  uint64_t xcr0 = i386_fbsd_core_read_xsave_info (abfd, layout);
  if (xcr0 == 0)
    xcr0 = X86_XSTATE_SSE_MASK;

  return amd64_target_description (xcr0, true);
}

/* Similar to amd64_supply_fpregset, but use XSAVE extended state.  */

static void
amd64fbsd_supply_xstateregset (const struct regset *regset,
			       struct regcache *regcache, int regnum,
			       const void *xstateregs, size_t len)
{
  amd64_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to amd64_collect_fpregset, but use XSAVE extended state.  */

static void
amd64fbsd_collect_xstateregset (const struct regset *regset,
				const struct regcache *regcache,
				int regnum, void *xstateregs, size_t len)
{
  amd64_collect_xsave (regcache, regnum, xstateregs, 1);
}

static const struct regset amd64fbsd_xstateregset =
  {
    NULL,
    amd64fbsd_supply_xstateregset,
    amd64fbsd_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
amd64fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  cb (".reg", AMD64_FBSD_SIZEOF_GREGSET, AMD64_FBSD_SIZEOF_GREGSET,
      &amd64_fbsd_gregset, NULL, cb_data);
  cb (".reg2", tdep->sizeof_fpregset, tdep->sizeof_fpregset, &amd64_fpregset,
      NULL, cb_data);
  cb (".reg-x86-segbases", AMD64_FBSD_SIZEOF_SEGBASES_REGSET,
      AMD64_FBSD_SIZEOF_SEGBASES_REGSET, &amd64_fbsd_segbases_regset,
      "segment bases", cb_data);
  if (tdep->xsave_layout.sizeof_xsave != 0)
    cb (".reg-xstate", tdep->xsave_layout.sizeof_xsave,
	tdep->xsave_layout.sizeof_xsave, &amd64fbsd_xstateregset,
	"XSAVE extended state", cb_data);
}

/* Implement the get_thread_local_address gdbarch method.  */

static CORE_ADDR
amd64fbsd_get_thread_local_address (struct gdbarch *gdbarch, ptid_t ptid,
				    CORE_ADDR lm_addr, CORE_ADDR offset)
{
  regcache *regcache
    = get_thread_arch_regcache (current_inferior (), ptid, gdbarch);

  target_fetch_registers (regcache, AMD64_FSBASE_REGNUM);

  ULONGEST fsbase;
  if (regcache->cooked_read (AMD64_FSBASE_REGNUM, &fsbase) != REG_VALID)
    error (_("Unable to fetch %%fsbase"));

  CORE_ADDR dtv_addr = fsbase + gdbarch_ptr_bit (gdbarch) / 8;
  return fbsd_get_thread_local_address (gdbarch, dtv_addr, lm_addr, offset);
}

static void
amd64fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* Generic FreeBSD support. */
  fbsd_init_abi (info, gdbarch);

  /* Obviously FreeBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  amd64_init_abi (info, gdbarch,
		  amd64_target_description (X86_XSTATE_SSE_MASK, true));

  tramp_frame_prepend_unwinder (gdbarch, &amd64_fbsd_sigframe);

  tdep->xsave_xcr0_offset = I386_FBSD_XSAVE_XCR0_OFFSET;
  set_gdbarch_core_read_x86_xsave_layout
    (gdbarch, i386_fbsd_core_read_x86_xsave_layout);

  /* Iterate over core file register note sections.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, amd64fbsd_iterate_over_regset_sections);

  set_gdbarch_core_read_description (gdbarch,
				     amd64fbsd_core_read_description);

  /* FreeBSD uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);

  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
  set_gdbarch_get_thread_local_address (gdbarch,
					amd64fbsd_get_thread_local_address);
}

void _initialize_amd64fbsd_tdep ();
void
_initialize_amd64fbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64,
			  GDB_OSABI_FREEBSD, amd64fbsd_init_abi);
}

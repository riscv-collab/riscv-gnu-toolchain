/* Target-dependent code for FreeBSD/i386.

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
#include "gdbcore.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "i386-fbsd-tdep.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "fbsd-tdep.h"
#include "solib-svr4.h"
#include "inferior.h"

/* The general-purpose regset consists of 19 32-bit slots.  */
#define I386_FBSD_SIZEOF_GREGSET	(19 * 4)

/* The segment base register set consists of 2 32-bit registers.  */
#define I386_FBSD_SIZEOF_SEGBASES_REGSET	(2 * 4)

/* Register maps.  */

static const struct regcache_map_entry i386_fbsd_gregmap[] =
{
  { 1, I386_FS_REGNUM, 4 },
  { 1, I386_ES_REGNUM, 4 },
  { 1, I386_DS_REGNUM, 4 },
  { 1, I386_EDI_REGNUM, 0 },
  { 1, I386_ESI_REGNUM, 0 },
  { 1, I386_EBP_REGNUM, 0 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* isp */
  { 1, I386_EBX_REGNUM, 0 },
  { 1, I386_EDX_REGNUM, 0 },
  { 1, I386_ECX_REGNUM, 0 },
  { 1, I386_EAX_REGNUM, 0 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* trapno */
  { 1, REGCACHE_MAP_SKIP, 4 },	/* err */
  { 1, I386_EIP_REGNUM, 0 },
  { 1, I386_CS_REGNUM, 4 },
  { 1, I386_EFLAGS_REGNUM, 0 },
  { 1, I386_ESP_REGNUM, 0 },
  { 1, I386_SS_REGNUM, 4 },
  { 1, I386_GS_REGNUM, 4 },
  { 0 }
};

static const struct regcache_map_entry i386_fbsd_segbases_regmap[] =
{
  { 1, I386_FSBASE_REGNUM, 0 },
  { 1, I386_GSBASE_REGNUM, 0 },
  { 0 }
};

/* This layout including fsbase and gsbase was adopted in FreeBSD
   8.0.  */

static const struct regcache_map_entry i386_fbsd_mcregmap[] =
{
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_onstack */
  { 1, I386_GS_REGNUM, 4 },
  { 1, I386_FS_REGNUM, 4 },
  { 1, I386_ES_REGNUM, 4 },
  { 1, I386_DS_REGNUM, 4 },
  { 1, I386_EDI_REGNUM, 0 },
  { 1, I386_ESI_REGNUM, 0 },
  { 1, I386_EBP_REGNUM, 0 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* isp */
  { 1, I386_EBX_REGNUM, 0 },
  { 1, I386_EDX_REGNUM, 0 },
  { 1, I386_ECX_REGNUM, 0 },
  { 1, I386_EAX_REGNUM, 0 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_trapno */
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_err */
  { 1, I386_EIP_REGNUM, 0 },
  { 1, I386_CS_REGNUM, 4 },
  { 1, I386_EFLAGS_REGNUM, 0 },
  { 1, I386_ESP_REGNUM, 0 },
  { 1, I386_SS_REGNUM, 4 },
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_len */
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_fpformat */
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_ownedfp */
  { 1, REGCACHE_MAP_SKIP, 4 },	/* mc_flags */
  { 128, REGCACHE_MAP_SKIP, 4 },/* mc_fpstate */
  { 1, I386_FSBASE_REGNUM, 0 },
  { 1, I386_GSBASE_REGNUM, 0 },
  { 0 }
};

/* Register set definitions.  */

const struct regset i386_fbsd_gregset =
{
  i386_fbsd_gregmap, regcache_supply_regset, regcache_collect_regset
};

const struct regset i386_fbsd_segbases_regset =
{
  i386_fbsd_segbases_regmap, regcache_supply_regset, regcache_collect_regset
};

/* Support for signal handlers.  */

/* In a signal frame, esp points to a 'struct sigframe' which is
   defined as:

   struct sigframe {
	register_t	sf_signum;
	register_t	sf_siginfo;
	register_t	sf_ucontext;
	register_t	sf_addr;
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

/* NB: There is a 12 byte padding hole between sf_ahu and sf_uc. */
#define I386_SIGFRAME_UCONTEXT_OFFSET 		32
#define I386_UCONTEXT_MCONTEXT_OFFSET		16
#define I386_SIZEOF_MCONTEXT_T			640

/* Implement the "init" method of struct tramp_frame.  */

static void
i386_fbsd_sigframe_init (const struct tramp_frame *self,
			 frame_info_ptr this_frame,
			 struct trad_frame_cache *this_cache,
			 CORE_ADDR func)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, I386_ESP_REGNUM);
  CORE_ADDR mcontext_addr
    = (sp
       + I386_SIGFRAME_UCONTEXT_OFFSET
       + I386_UCONTEXT_MCONTEXT_OFFSET);

  trad_frame_set_reg_regmap (this_cache, i386_fbsd_mcregmap, mcontext_addr,
			     I386_SIZEOF_MCONTEXT_T);

  /* Don't bother with floating point or XSAVE state for now.  The
     current helper routines for parsing FXSAVE and XSAVE state only
     work with regcaches.  This could perhaps create a temporary
     regcache, collect the register values from mc_fpstate and
     mc_xfpustate, and then set register values in the trad_frame.  */

  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static const struct tramp_frame i386_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  1,
  {
    {0x8d, ULONGEST_MAX},		/* lea     SIGF_UC(%esp),%eax */
    {0x44, ULONGEST_MAX},
    {0x24, ULONGEST_MAX},
    {0x20, ULONGEST_MAX},
    {0x50, ULONGEST_MAX},		/* pushl   %eax */
    {0xf7, ULONGEST_MAX},		/* testl   $PSL_VM,UC_EFLAGS(%eax) */
    {0x40, ULONGEST_MAX},
    {0x54, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x02, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x75, ULONGEST_MAX},		/* jne	   +3 */
    {0x03, ULONGEST_MAX},
    {0x8e, ULONGEST_MAX},		/* mov	   UC_GS(%eax),%gs */
    {0x68, ULONGEST_MAX},
    {0x14, ULONGEST_MAX},
    {0xb8, ULONGEST_MAX},		/* movl   $SYS_sigreturn,%eax */
    {0xa1, ULONGEST_MAX},
    {0x01, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x50, ULONGEST_MAX},		/* pushl   %eax */
    {0xcd, ULONGEST_MAX},		/* int	   $0x80 */
    {0x80, ULONGEST_MAX},
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  i386_fbsd_sigframe_init
};

/* FreeBSD/i386 binaries running under an amd64 kernel use a different
   trampoline.  This trampoline differs from the i386 kernel trampoline
   in that it omits a middle section that conditionally restores
   %gs.  */

static const struct tramp_frame i386_fbsd64_sigframe =
{
  SIGTRAMP_FRAME,
  1,
  {
    {0x8d, ULONGEST_MAX},		/* lea     SIGF_UC(%esp),%eax */
    {0x44, ULONGEST_MAX},
    {0x24, ULONGEST_MAX},
    {0x20, ULONGEST_MAX},
    {0x50, ULONGEST_MAX},		/* pushl   %eax */
    {0xb8, ULONGEST_MAX},		/* movl   $SYS_sigreturn,%eax */
    {0xa1, ULONGEST_MAX},
    {0x01, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x00, ULONGEST_MAX},
    {0x50, ULONGEST_MAX},		/* pushl   %eax */
    {0xcd, ULONGEST_MAX},		/* int	   $0x80 */
    {0x80, ULONGEST_MAX},
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  i386_fbsd_sigframe_init
};

/* See i386-fbsd-tdep.h.  */

uint64_t
i386_fbsd_core_read_xsave_info (bfd *abfd, x86_xsave_layout &layout)
{
  asection *xstate = bfd_get_section_by_name (abfd, ".reg-xstate");
  if (xstate == nullptr)
    return 0;

  /* Check extended state size.  */
  size_t size = bfd_section_size (xstate);
  if (size < X86_XSTATE_AVX_SIZE)
    return 0;

  char contents[8];
  if (! bfd_get_section_contents (abfd, xstate, contents,
				  I386_FBSD_XSAVE_XCR0_OFFSET, 8))
    {
      warning (_("Couldn't read `xcr0' bytes from "
		 "`.reg-xstate' section in core file."));
      return 0;
    }

  uint64_t xcr0 = bfd_get_64 (abfd, contents);

  if (!i387_guess_xsave_layout (xcr0, size, layout))
    return 0;

  return xcr0;
}

/* See i386-fbsd-tdep.h.  */

bool
i386_fbsd_core_read_x86_xsave_layout (struct gdbarch *gdbarch,
				      x86_xsave_layout &layout)
{
  return i386_fbsd_core_read_xsave_info (core_bfd, layout) != 0;
}

/* Implement the core_read_description gdbarch method.  */

static const struct target_desc *
i386fbsd_core_read_description (struct gdbarch *gdbarch,
				struct target_ops *target,
				bfd *abfd)
{
  x86_xsave_layout layout;
  uint64_t xcr0 = i386_fbsd_core_read_xsave_info (abfd, layout);
  if (xcr0 == 0)
    xcr0 = X86_XSTATE_X87_MASK;
  return i386_target_description (xcr0, true);
}

/* Similar to i386_supply_fpregset, but use XSAVE extended state.  */

static void
i386fbsd_supply_xstateregset (const struct regset *regset,
			      struct regcache *regcache, int regnum,
			      const void *xstateregs, size_t len)
{
  i387_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to i386_collect_fpregset, but use XSAVE extended state.  */

static void
i386fbsd_collect_xstateregset (const struct regset *regset,
			       const struct regcache *regcache,
			       int regnum, void *xstateregs, size_t len)
{
  i387_collect_xsave (regcache, regnum, xstateregs, 1);
}

/* Register set definitions.  */

static const struct regset i386fbsd_xstateregset =
  {
    NULL,
    i386fbsd_supply_xstateregset,
    i386fbsd_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
i386fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  cb (".reg", I386_FBSD_SIZEOF_GREGSET, I386_FBSD_SIZEOF_GREGSET,
      &i386_fbsd_gregset, NULL, cb_data);
  cb (".reg2", tdep->sizeof_fpregset, tdep->sizeof_fpregset, &i386_fpregset,
      NULL, cb_data);
  cb (".reg-x86-segbases", I386_FBSD_SIZEOF_SEGBASES_REGSET,
      I386_FBSD_SIZEOF_SEGBASES_REGSET, &i386_fbsd_segbases_regset,
      "segment bases", cb_data);

  if (tdep->xsave_layout.sizeof_xsave != 0)
    cb (".reg-xstate", tdep->xsave_layout.sizeof_xsave,
	tdep->xsave_layout.sizeof_xsave, &i386fbsd_xstateregset,
	"XSAVE extended state", cb_data);
}

/* Implement the get_thread_local_address gdbarch method.  */

static CORE_ADDR
i386fbsd_get_thread_local_address (struct gdbarch *gdbarch, ptid_t ptid,
				   CORE_ADDR lm_addr, CORE_ADDR offset)
{
  regcache *regcache
    = get_thread_arch_regcache (current_inferior (), ptid, gdbarch);

  target_fetch_registers (regcache, I386_GSBASE_REGNUM);

  ULONGEST gsbase;
  if (regcache->cooked_read (I386_GSBASE_REGNUM, &gsbase) != REG_VALID)
    error (_("Unable to fetch %%gsbase"));

  CORE_ADDR dtv_addr = gsbase + gdbarch_ptr_bit (gdbarch) / 8;
  return fbsd_get_thread_local_address (gdbarch, dtv_addr, lm_addr, offset);
}

static void
i386fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* Generic FreeBSD support. */
  fbsd_init_abi (info, gdbarch);

  /* Obviously FreeBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);

  /* FreeBSD reserves some space for its FPU emulator in
     `struct fpreg'.  */
  tdep->sizeof_fpregset = 176;

  /* FreeBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  tramp_frame_prepend_unwinder (gdbarch, &i386_fbsd_sigframe);
  tramp_frame_prepend_unwinder (gdbarch, &i386_fbsd64_sigframe);

  i386_elf_init_abi (info, gdbarch);

  tdep->xsave_xcr0_offset = I386_FBSD_XSAVE_XCR0_OFFSET;
  set_gdbarch_core_read_x86_xsave_layout
    (gdbarch, i386_fbsd_core_read_x86_xsave_layout);

  /* Iterate over core file register note sections.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, i386fbsd_iterate_over_regset_sections);

  set_gdbarch_core_read_description (gdbarch,
				     i386fbsd_core_read_description);

  /* FreeBSD uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
  set_gdbarch_get_thread_local_address (gdbarch,
					i386fbsd_get_thread_local_address);
}

void _initialize_i386fbsd_tdep ();
void
_initialize_i386fbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_FREEBSD,
			  i386fbsd_init_abi);
}

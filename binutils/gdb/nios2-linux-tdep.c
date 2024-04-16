/* Target-dependent code for GNU/Linux on Nios II.
   Copyright (C) 2012-2024 Free Software Foundation, Inc.
   Contributed by Mentor Graphics, Inc.

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
#include "osabi.h"
#include "solib-svr4.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "symtab.h"
#include "regset.h"
#include "regcache.h"
#include "linux-tdep.h"
#include "glibc-tdep.h"
#include "nios2-tdep.h"
#include "gdbarch.h"

/* Core file and register set support.  */

/* Map from the normal register enumeration order to the order that
   registers appear in core files, which corresponds to the order
   of the register slots in the kernel's struct pt_regs.  */

static const int reg_offsets[NIOS2_NUM_REGS] =
{
  -1,  8,  9, 10, 11, 12, 13, 14,	/* r0 - r7 */
  0,  1,  2,  3,  4,  5,  6,  7,	/* r8 - r15 */
  23, 24, 25, 26, 27, 28, 29, 30,	/* r16 - r23 */
  -1, -1, 19, 18, 17, 21, -1, 16,	/* et bt gp sp fp ea sstatus ra */
  21,					/* pc */
  -1, 20, -1, -1, -1, -1, -1, -1,	/* status estatus ...  */
  -1, -1, -1, -1, -1, -1, -1, -1
};

/* General register set size.  Should match sizeof (struct pt_regs) +
   sizeof (struct switch_stack) from the NIOS2 Linux kernel patch.  */

#define NIOS2_GREGS_SIZE (4 * 34)

/* Implement the supply_regset hook for core files.  */

static void
nios2_supply_gregset (const struct regset *regset,
		      struct regcache *regcache,
		      int regnum, const void *gregs_buf, size_t len)
{
  const gdb_byte *gregs = (const gdb_byte *) gregs_buf;
  int regno;
  static const gdb_byte zero_buf[4] = {0, 0, 0, 0};

  for (regno = NIOS2_Z_REGNUM; regno <= NIOS2_MPUACC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      {
	if (reg_offsets[regno] != -1)
	  regcache->raw_supply (regno, gregs + 4 * reg_offsets[regno]);
	else
	  regcache->raw_supply (regno, zero_buf);
      }
}

/* Implement the collect_regset hook for core files.  */

static void
nios2_collect_gregset (const struct regset *regset,
		       const struct regcache *regcache,
		       int regnum, void *gregs_buf, size_t len)
{
  gdb_byte *gregs = (gdb_byte *) gregs_buf;
  int regno;

  for (regno = NIOS2_Z_REGNUM; regno <= NIOS2_MPUACC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      {
	if (reg_offsets[regno] != -1)
	  regcache->raw_collect (regno, gregs + 4 * reg_offsets[regno]);
      }
}

static const struct regset nios2_core_regset =
{
  NULL,
  nios2_supply_gregset,
  nios2_collect_gregset
};

/* Iterate over core file register note sections.  */

static void
nios2_iterate_over_regset_sections (struct gdbarch *gdbarch,
				    iterate_over_regset_sections_cb *cb,
				    void *cb_data,
				    const struct regcache *regcache)
{
  cb (".reg", NIOS2_GREGS_SIZE, NIOS2_GREGS_SIZE, &nios2_core_regset, NULL,
      cb_data);
}

/* Initialize a trad-frame cache corresponding to the tramp-frame.
   FUNC is the address of the instruction TRAMP[0] in memory.

   This ABI is not documented.  It corresponds to rt_setup_ucontext in
   the kernel arch/nios2/kernel/signal.c file.

   The key points are:
   - The kernel creates a trampoline at the hard-wired address 0x1044.
   - The stack pointer points to an object of type struct rt_sigframe.
     The definition of this structure is not exported from the kernel.
     The register save area is located at offset 152 bytes (as determined
     by inspection of the stack contents in the debugger), and the
     registers are saved as r1-r23, ra, fp, gp, ea, sp.

   This interface was implemented with kernel version 3.19 (the first
   official mainline kernel).  Older unofficial kernel versions used
   incompatible conventions; we do not support those here.  */

#define NIOS2_SIGRETURN_TRAMP_ADDR 0x1044
#define NIOS2_SIGRETURN_REGSAVE_OFFSET 152

static void
nios2_linux_rt_sigreturn_init (const struct tramp_frame *self,
			       frame_info_ptr next_frame,
			       struct trad_frame_cache *this_cache,
			       CORE_ADDR func)
{
  CORE_ADDR sp = get_frame_register_unsigned (next_frame, NIOS2_SP_REGNUM);
  CORE_ADDR base = sp + NIOS2_SIGRETURN_REGSAVE_OFFSET;
  int i;

  for (i = 0; i < 23; i++)
    trad_frame_set_reg_addr (this_cache, i + 1, base + i * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_RA_REGNUM, base + 23 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_FP_REGNUM, base + 24 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_GP_REGNUM, base + 25 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_PC_REGNUM, base + 27 * 4);
  trad_frame_set_reg_addr (this_cache, NIOS2_SP_REGNUM, base + 28 * 4);

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

/* Trampoline for sigreturn.  This has the form
     movi r2, __NR_rt_sigreturn
     trap 0
   appropriately encoded for R1 or R2.  */

static struct tramp_frame nios2_r1_linux_rt_sigreturn_tramp_frame =
{
  SIGTRAMP_FRAME,
  4,
  {
    { MATCH_R1_MOVI | SET_IW_I_B (2) | SET_IW_I_IMM16 (139), ULONGEST_MAX },
    { MATCH_R1_TRAP | SET_IW_R_IMM5 (0), ULONGEST_MAX},
    { TRAMP_SENTINEL_INSN }
  },
  nios2_linux_rt_sigreturn_init
};

static struct tramp_frame nios2_r2_linux_rt_sigreturn_tramp_frame =
{
  SIGTRAMP_FRAME,
  4,
  {
    { MATCH_R2_MOVI | SET_IW_F2I16_B (2) | SET_IW_F2I16_IMM16 (139), ULONGEST_MAX },
    { MATCH_R2_TRAP | SET_IW_X2L5_IMM5 (0), ULONGEST_MAX},
    { TRAMP_SENTINEL_INSN }
  },
  nios2_linux_rt_sigreturn_init
};

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
nios2_linux_syscall_next_pc (frame_info_ptr frame,
			     const struct nios2_opcode *op)
{
  CORE_ADDR pc = get_frame_pc (frame);
  ULONGEST syscall_nr = get_frame_register_unsigned (frame, NIOS2_R2_REGNUM);

  /* If we are about to make a sigreturn syscall, use the unwinder to
     decode the signal frame.  */
  if (syscall_nr == 139 /* rt_sigreturn */)
    return frame_unwind_caller_pc (frame);

  return pc + op->size;
}

/* Return true if PC is a kernel helper, a function mapped by the kernel
   into user space on an unwritable page.  Currently the only such function
   is __kuser_cmpxchg at 0x1004.  See arch/nios2/kernel/entry.S in the Linux
   kernel sources and sysdeps/unix/sysv/linux/nios2/atomic-machine.h in
   GLIBC.  */
static bool
nios2_linux_is_kernel_helper (CORE_ADDR pc)
{
  return pc == 0x1004;
}

/* Hook function for gdbarch_register_osabi.  */

static void
nios2_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  nios2_gdbarch_tdep *tdep = gdbarch_tdep<nios2_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);
  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, nios2_iterate_over_regset_sections);
  /* Linux signal frame unwinders.  */
  if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_nios2r2)
    tramp_frame_prepend_unwinder (gdbarch,
				  &nios2_r2_linux_rt_sigreturn_tramp_frame);
  else
    tramp_frame_prepend_unwinder (gdbarch,
				  &nios2_r1_linux_rt_sigreturn_tramp_frame);

  tdep->syscall_next_pc = nios2_linux_syscall_next_pc;
  tdep->is_kernel_helper = nios2_linux_is_kernel_helper;

  /* Index of target address word in glibc jmp_buf.  */
  tdep->jb_pc = 10;
}

void _initialize_nios2_linux_tdep ();
void
_initialize_nios2_linux_tdep ()
{

  const struct bfd_arch_info *arch_info;

  for (arch_info = bfd_lookup_arch (bfd_arch_nios2, 0);
       arch_info != NULL;
       arch_info = arch_info->next)
    gdbarch_register_osabi (bfd_arch_nios2, arch_info->mach,
			    GDB_OSABI_LINUX, nios2_linux_init_abi);
}

/* Target-dependent code for GNU/Linux on RISC-V processors.
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
#include "riscv-tdep.h"
#include "osabi.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "solib-svr4.h"
#include "regset.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "gdbarch.h"

/* The following value is derived from __NR_rt_sigreturn in
   <include/uapi/asm-generic/unistd.h> from the Linux source tree.  */

#define RISCV_NR_rt_sigreturn 139

/* Define the general register mapping.  The kernel puts the PC at offset 0,
   gdb puts it at offset 32.  Register x0 is always 0 and can be ignored.
   Registers x1 to x31 are in the same place.  */

static const struct regcache_map_entry riscv_linux_gregmap[] =
{
  { 1,  RISCV_PC_REGNUM, 0 },
  { 31, RISCV_RA_REGNUM, 0 }, /* x1 to x31 */
  { 0 }
};

/* Define the FP register mapping.  The kernel puts the 32 FP regs first, and
   then FCSR.  */

static const struct regcache_map_entry riscv_linux_fregmap[] =
{
  { 32, RISCV_FIRST_FP_REGNUM, 0 },
  { 1, RISCV_CSR_FCSR_REGNUM, 0 },
  { 0 }
};

/* Define the general register regset.  */

static const struct regset riscv_linux_gregset =
{
  riscv_linux_gregmap, riscv_supply_regset, regcache_collect_regset
};

/* Define the FP register regset.  */

static const struct regset riscv_linux_fregset =
{
  riscv_linux_fregmap, riscv_supply_regset, regcache_collect_regset
};

/* Define hook for core file support.  */

static void
riscv_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					  iterate_over_regset_sections_cb *cb,
					  void *cb_data,
					  const struct regcache *regcache)
{
  cb (".reg", (32 * riscv_isa_xlen (gdbarch)), (32 * riscv_isa_xlen (gdbarch)),
      &riscv_linux_gregset, NULL, cb_data);
  /* The kernel is adding 8 bytes for FCSR.  */
  cb (".reg2", (32 * riscv_isa_flen (gdbarch)) + 8,
      (32 * riscv_isa_flen (gdbarch)) + 8,
      &riscv_linux_fregset, NULL, cb_data);
}

/* Signal trampoline support.  */

static void riscv_linux_sigframe_init (const struct tramp_frame *self,
				       frame_info_ptr this_frame,
				       struct trad_frame_cache *this_cache,
				       CORE_ADDR func);

#define RISCV_INST_LI_A7_SIGRETURN	0x08b00893
#define RISCV_INST_ECALL		0x00000073

static const struct tramp_frame riscv_linux_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { RISCV_INST_LI_A7_SIGRETURN, ULONGEST_MAX },
    { RISCV_INST_ECALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  riscv_linux_sigframe_init,
  NULL
};

/* Runtime signal frames look like this:
   struct rt_sigframe {
     struct siginfo info;
     struct ucontext uc;
   };

   struct ucontext {
     unsigned long __uc_flags;
     struct ucontext *uclink;
     stack_t uc_stack;
     sigset_t uc_sigmask;
     char __glibc_reserved[1024 / 8 - sizeof (sigset_t)];
     mcontext_t uc_mcontext;
   }; */

#define SIGFRAME_SIGINFO_SIZE		128
#define UCONTEXT_MCONTEXT_OFFSET	176

static void
riscv_linux_sigframe_init (const struct tramp_frame *self,
			   frame_info_ptr this_frame,
			   struct trad_frame_cache *this_cache,
			   CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int xlen = riscv_isa_xlen (gdbarch);
  int flen = riscv_isa_flen (gdbarch);
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR mcontext_base;
  CORE_ADDR regs_base;

  mcontext_base = frame_sp + SIGFRAME_SIGINFO_SIZE + UCONTEXT_MCONTEXT_OFFSET;

  /* Handle the integer registers.  The first one is PC, followed by x1
     through x31.  */
  regs_base = mcontext_base;
  trad_frame_set_reg_addr (this_cache, RISCV_PC_REGNUM, regs_base);
  for (int i = 1; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, RISCV_ZERO_REGNUM + i,
			     regs_base + (i * xlen));

  /* Handle the FP registers.  First comes the 32 FP registers, followed by
     fcsr.  */
  regs_base += 32 * xlen;
  for (int i = 0; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, RISCV_FIRST_FP_REGNUM + i,
			     regs_base + (i * flen));
  regs_base += 32 * flen;
  trad_frame_set_reg_addr (this_cache, RISCV_CSR_FCSR_REGNUM, regs_base);

  /* Choice of the bottom of the sigframe is somewhat arbitrary.  */
  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* When FRAME is at a syscall instruction (ECALL), return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
riscv_linux_syscall_next_pc (frame_info_ptr frame)
{
  const CORE_ADDR pc = get_frame_pc (frame);
  const ULONGEST a7 = get_frame_register_unsigned (frame, RISCV_A7_REGNUM);

  if (a7 == RISCV_NR_rt_sigreturn)
    return frame_unwind_caller_pc (frame);

  return pc + 4 /* Length of the ECALL insn.  */;
}

/* Initialize RISC-V Linux ABI info.  */

static void
riscv_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  set_gdbarch_software_single_step (gdbarch, riscv_software_single_step);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 (riscv_isa_xlen (gdbarch) == 4
					  ? linux_ilp32_fetch_link_map_offsets
					  : linux_lp64_fetch_link_map_offsets));

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, riscv_linux_iterate_over_regset_sections);

  tramp_frame_prepend_unwinder (gdbarch, &riscv_linux_sigframe);

  tdep->syscall_next_pc = riscv_linux_syscall_next_pc;
}

/* Initialize RISC-V Linux target support.  */

void _initialize_riscv_linux_tdep ();
void
_initialize_riscv_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_riscv, 0, GDB_OSABI_LINUX,
			  riscv_linux_init_abi);
}

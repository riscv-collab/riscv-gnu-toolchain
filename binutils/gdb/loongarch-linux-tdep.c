/* Target-dependent code for GNU/Linux on LoongArch processors.

   Copyright (C) 2022-2024 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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
#include "glibc-tdep.h"
#include "inferior.h"
#include "linux-tdep.h"
#include "loongarch-tdep.h"
#include "solib-svr4.h"
#include "target-descriptions.h"
#include "trad-frame.h"
#include "tramp-frame.h"

/* Unpack an elf_gregset_t into GDB's register cache.  */

static void
loongarch_supply_gregset (const struct regset *regset,
			  struct regcache *regcache, int regnum,
			  const void *gprs, size_t len)
{
  int regsize = register_size (regcache->arch (), 0);
  const gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      regcache->raw_supply_zeroed (0);

      for (int i = 1; i < 32; i++)
	{
	  buf = (const gdb_byte*) gprs + regsize * i;
	  regcache->raw_supply (i, (const void *) buf);
	}

      buf = (const gdb_byte*) gprs + regsize * LOONGARCH_ORIG_A0_REGNUM;
      regcache->raw_supply (LOONGARCH_ORIG_A0_REGNUM, (const void *) buf);

      buf = (const gdb_byte*) gprs + regsize * LOONGARCH_PC_REGNUM;
      regcache->raw_supply (LOONGARCH_PC_REGNUM, (const void *) buf);

      buf = (const gdb_byte*) gprs + regsize * LOONGARCH_BADV_REGNUM;
      regcache->raw_supply (LOONGARCH_BADV_REGNUM, (const void *) buf);
    }
  else if (regnum == 0)
    regcache->raw_supply_zeroed (0);
  else if ((regnum > 0 && regnum < 32)
	   || regnum == LOONGARCH_ORIG_A0_REGNUM
	   || regnum == LOONGARCH_PC_REGNUM
	   || regnum == LOONGARCH_BADV_REGNUM)
    {
      buf = (const gdb_byte*) gprs + regsize * regnum;
      regcache->raw_supply (regnum, (const void *) buf);
    }
}

/* Pack the GDB's register cache value into an elf_gregset_t.  */

static void
loongarch_fill_gregset (const struct regset *regset,
			const struct regcache *regcache, int regnum,
			void *gprs, size_t len)
{
  int regsize = register_size (regcache->arch (), 0);
  gdb_byte *buf = nullptr;

  if (regnum == -1)
    {
      for (int i = 0; i < 32; i++)
	{
	  buf = (gdb_byte *) gprs + regsize * i;
	  regcache->raw_collect (i, (void *) buf);
	}

      buf = (gdb_byte *) gprs + regsize * LOONGARCH_ORIG_A0_REGNUM;
      regcache->raw_collect (LOONGARCH_ORIG_A0_REGNUM, (void *) buf);

      buf = (gdb_byte *) gprs + regsize * LOONGARCH_PC_REGNUM;
      regcache->raw_collect (LOONGARCH_PC_REGNUM, (void *) buf);

      buf = (gdb_byte *) gprs + regsize * LOONGARCH_BADV_REGNUM;
      regcache->raw_collect (LOONGARCH_BADV_REGNUM, (void *) buf);
    }
  else if ((regnum >= 0 && regnum < 32)
	   || regnum == LOONGARCH_ORIG_A0_REGNUM
	   || regnum == LOONGARCH_PC_REGNUM
	   || regnum == LOONGARCH_BADV_REGNUM)
    {
      buf = (gdb_byte *) gprs + regsize * regnum;
      regcache->raw_collect (regnum, (void *) buf);
    }
}

/* Define the general register regset.  */

const struct regset loongarch_gregset =
{
  nullptr,
  loongarch_supply_gregset,
  loongarch_fill_gregset,
};

/* Unpack an elf_fpregset_t into GDB's register cache.  */
static void
loongarch_supply_fpregset (const struct regset *r,
			   struct regcache *regcache, int regnum,
			   const void *fprs, size_t len)
{
  const gdb_byte *buf = nullptr;
  int fprsize = register_size (regcache->arch (), LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (regcache->arch (), LOONGARCH_FIRST_FCC_REGNUM);

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
	{
	  buf = (const gdb_byte *)fprs + fprsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_FP_REGNUM + i, (const void *)buf);
	}
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
	{
	  buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	    fccsize * i;
	  regcache->raw_supply (LOONGARCH_FIRST_FCC_REGNUM + i, (const void *)buf);
	}
      buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_supply (LOONGARCH_FCSR_REGNUM, (const void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FP_REGNUM && regnum < LOONGARCH_FIRST_FCC_REGNUM)
    {
      buf = (const gdb_byte *)fprs + fprsize * (regnum - LOONGARCH_FIRST_FP_REGNUM);
      regcache->raw_supply (regnum, (const void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FCC_REGNUM && regnum < LOONGARCH_FCSR_REGNUM)
    {
      buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * (regnum - LOONGARCH_FIRST_FCC_REGNUM);
      regcache->raw_supply (regnum, (const void *)buf);
    }
  else if (regnum == LOONGARCH_FCSR_REGNUM)
    {
      buf = (const gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_supply (regnum, (const void *)buf);
    }
}

/* Pack the GDB's register cache value into an elf_fpregset_t.  */
static void
loongarch_fill_fpregset (const struct regset *r,
			 const struct regcache *regcache, int regnum,
			 void *fprs, size_t len)
{
  gdb_byte *buf = nullptr;
  int fprsize = register_size (regcache->arch (), LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (regcache->arch (), LOONGARCH_FIRST_FCC_REGNUM);

  if (regnum == -1)
    {
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
	{
	  buf = (gdb_byte *)fprs + fprsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_FP_REGNUM + i, (void *)buf);
	}
      for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
	{
	  buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	    fccsize * i;
	  regcache->raw_collect (LOONGARCH_FIRST_FCC_REGNUM + i, (void *)buf);
	}
      buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_collect (LOONGARCH_FCSR_REGNUM, (void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FP_REGNUM && regnum < LOONGARCH_FIRST_FCC_REGNUM)
    {
      buf = (gdb_byte *)fprs + fprsize * (regnum - LOONGARCH_FIRST_FP_REGNUM);
      regcache->raw_collect (regnum, (void *)buf);
    }
  else if (regnum >= LOONGARCH_FIRST_FCC_REGNUM && regnum < LOONGARCH_FCSR_REGNUM)
    {
      buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * (regnum - LOONGARCH_FIRST_FCC_REGNUM);
      regcache->raw_collect (regnum, (void *)buf);
    }
  else if (regnum == LOONGARCH_FCSR_REGNUM)
    {
      buf = (gdb_byte *)fprs + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * LOONGARCH_LINUX_NUM_FCC;
      regcache->raw_collect (regnum, (void *)buf);
    }
}

/* Define the FP register regset.  */
const struct regset loongarch_fpregset =
{
  nullptr,
  loongarch_supply_fpregset,
  loongarch_fill_fpregset,
};

/* Implement the "init" method of struct tramp_frame.  */

#define LOONGARCH_RT_SIGFRAME_UCONTEXT_OFFSET	128
#define LOONGARCH_UCONTEXT_SIGCONTEXT_OFFSET	176

static void
loongarch_linux_rt_sigframe_init (const struct tramp_frame *self,
				  frame_info_ptr this_frame,
				  struct trad_frame_cache *this_cache,
				  CORE_ADDR func)
{
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR sigcontext_base = (frame_sp + LOONGARCH_RT_SIGFRAME_UCONTEXT_OFFSET
			       + LOONGARCH_UCONTEXT_SIGCONTEXT_OFFSET);

  trad_frame_set_reg_addr (this_cache, LOONGARCH_PC_REGNUM, sigcontext_base);
  for (int i = 0; i < 32; i++)
    trad_frame_set_reg_addr (this_cache, i, sigcontext_base + 8 + i * 8);

  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* li.w    a7, __NR_rt_sigreturn  */
#define LOONGARCH_INST_LIW_A7_RT_SIGRETURN	0x03822c0b
/* syscall 0  */
#define LOONGARCH_INST_SYSCALL			0x002b0000

static const struct tramp_frame loongarch_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { LOONGARCH_INST_LIW_A7_RT_SIGRETURN, ULONGEST_MAX },
    { LOONGARCH_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  loongarch_linux_rt_sigframe_init,
  nullptr
};

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
loongarch_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  int gprsize = register_size (gdbarch, 0);
  int fprsize = register_size (gdbarch, LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (gdbarch, LOONGARCH_FIRST_FCC_REGNUM);
  int fcsrsize = register_size (gdbarch, LOONGARCH_FCSR_REGNUM);
  int fpsize = fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
    fccsize * LOONGARCH_LINUX_NUM_FCC + fcsrsize;

  cb (".reg", LOONGARCH_LINUX_NUM_GREGSET * gprsize,
      LOONGARCH_LINUX_NUM_GREGSET * gprsize, &loongarch_gregset, nullptr, cb_data);
  cb (".reg2", fpsize, fpsize, &loongarch_fpregset, nullptr, cb_data);
}

/* The following value is derived from __NR_rt_sigreturn in
   <include/uapi/asm-generic/unistd.h> from the Linux source tree.  */

#define LOONGARCH_NR_rt_sigreturn	139

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
loongarch_linux_syscall_next_pc (frame_info_ptr frame)
{
  const CORE_ADDR pc = get_frame_pc (frame);
  ULONGEST a7 = get_frame_register_unsigned (frame, LOONGARCH_A7_REGNUM);

  /* If we are about to make a sigreturn syscall, use the unwinder to
     decode the signal frame.  */
  if (a7 == LOONGARCH_NR_rt_sigreturn)
    return frame_unwind_caller_pc (frame);

  return pc + 4;
}

/* Initialize LoongArch Linux ABI info.  */

static void
loongarch_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  loongarch_gdbarch_tdep *tdep = gdbarch_tdep<loongarch_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 info.bfd_arch_info->bits_per_address == 32
					 ? linux_ilp32_fetch_link_map_offsets
					 : linux_lp64_fetch_link_map_offsets);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch, svr4_fetch_objfile_link_map);

  /* Prepend tramp frame unwinder for signal.  */
  tramp_frame_prepend_unwinder (gdbarch, &loongarch_linux_rt_sigframe);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections (gdbarch, loongarch_iterate_over_regset_sections);

  tdep->syscall_next_pc = loongarch_linux_syscall_next_pc;
}

/* Initialize LoongArch Linux target support.  */

void _initialize_loongarch_linux_tdep ();
void
_initialize_loongarch_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_loongarch, bfd_mach_loongarch32,
			  GDB_OSABI_LINUX, loongarch_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_loongarch, bfd_mach_loongarch64,
			  GDB_OSABI_LINUX, loongarch_linux_init_abi);
}

/* Target-dependent code for FreeBSD/mips.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "trad-frame.h"
#include "tramp-frame.h"

#include "fbsd-tdep.h"
#include "mips-tdep.h"
#include "mips-fbsd-tdep.h"

#include "solib-svr4.h"

/* Core file support. */

/* Number of registers in `struct reg' from <machine/reg.h>.  The
   first 38 follow the standard MIPS layout.  The 39th holds
   IC_INT_REG on RM7K and RM9K processors.  The 40th is a dummy for
   padding.  */
#define MIPS_FBSD_NUM_GREGS	40

/* Number of registers in `struct fpreg' from <machine/reg.h>.  The
   first 32 hold floating point registers.  33 holds the FSR.  The
   34th holds FIR on FreeBSD 12.0 and newer kernels.  On older kernels
   it was a zero-filled dummy for padding.  */
#define MIPS_FBSD_NUM_FPREGS	34

/* Supply a single register.  The register size might not match, so use
   regcache->raw_supply_integer ().  */

static void
mips_fbsd_supply_reg (struct regcache *regcache, int regnum, const void *addr,
		      size_t len)
{
  regcache->raw_supply_integer (regnum, (const gdb_byte *) addr, len, true);
}

/* Collect a single register.  The register size might not match, so use
   regcache->raw_collect_integer ().  */

static void
mips_fbsd_collect_reg (const struct regcache *regcache, int regnum, void *addr,
		       size_t len)
{
  regcache->raw_collect_integer (regnum, (gdb_byte *) addr, len, true);
}

/* Supply the floating-point registers stored in FPREGS to REGCACHE.
   Each floating-point register in FPREGS is REGSIZE bytes in
   length.  */

void
mips_fbsd_supply_fpregs (struct regcache *regcache, int regnum,
			 const void *fpregs, size_t regsize)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i, fp0num;

  fp0num = mips_regnum (gdbarch)->fp0;
  for (i = 0; i <= 32; i++)
    if (regnum == fp0num + i || regnum == -1)
      mips_fbsd_supply_reg (regcache, fp0num + i,
			    regs + i * regsize, regsize);
  if (regnum == mips_regnum (gdbarch)->fp_control_status || regnum == -1)
    mips_fbsd_supply_reg (regcache, mips_regnum (gdbarch)->fp_control_status,
			  regs + 32 * regsize, regsize);
  if ((regnum == mips_regnum (gdbarch)->fp_implementation_revision
       || regnum == -1)
      && extract_unsigned_integer (regs + 33 * regsize, regsize,
				   gdbarch_byte_order (gdbarch)) != 0)
    mips_fbsd_supply_reg (regcache,
			  mips_regnum (gdbarch)->fp_implementation_revision,
			  regs + 33 * regsize, regsize);
}

/* Supply the general-purpose registers stored in GREGS to REGCACHE.
   Each general-purpose register in GREGS is REGSIZE bytes in
   length.  */

void
mips_fbsd_supply_gregs (struct regcache *regcache, int regnum,
			const void *gregs, size_t regsize)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  for (i = 0; i <= mips_regnum (gdbarch)->pc; i++)
    if (regnum == i || regnum == -1)
      mips_fbsd_supply_reg (regcache, i, regs + i * regsize, regsize);
}

/* Collect the floating-point registers from REGCACHE and store them
   in FPREGS.  Each floating-point register in FPREGS is REGSIZE bytes
   in length.  */

void
mips_fbsd_collect_fpregs (const struct regcache *regcache, int regnum,
			  void *fpregs, size_t regsize)
{
  struct gdbarch *gdbarch = regcache->arch ();
  gdb_byte *regs = (gdb_byte *) fpregs;
  int i, fp0num;

  fp0num = mips_regnum (gdbarch)->fp0;
  for (i = 0; i < 32; i++)
    if (regnum == fp0num + i || regnum == -1)
      mips_fbsd_collect_reg (regcache, fp0num + i,
			     regs + i * regsize, regsize);
  if (regnum == mips_regnum (gdbarch)->fp_control_status || regnum == -1)
    mips_fbsd_collect_reg (regcache, mips_regnum (gdbarch)->fp_control_status,
			   regs + 32 * regsize, regsize);
  if (regnum == mips_regnum (gdbarch)->fp_implementation_revision
      || regnum == -1)
    mips_fbsd_collect_reg (regcache,
			   mips_regnum (gdbarch)->fp_implementation_revision,
			   regs + 33 * regsize, regsize);
}

/* Collect the general-purpose registers from REGCACHE and store them
   in GREGS.  Each general-purpose register in GREGS is REGSIZE bytes
   in length.  */

void
mips_fbsd_collect_gregs (const struct regcache *regcache, int regnum,
			 void *gregs, size_t regsize)
{
  struct gdbarch *gdbarch = regcache->arch ();
  gdb_byte *regs = (gdb_byte *) gregs;
  int i;

  for (i = 0; i <= mips_regnum (gdbarch)->pc; i++)
    if (regnum == i || regnum == -1)
      mips_fbsd_collect_reg (regcache, i, regs + i * regsize, regsize);
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
mips_fbsd_supply_fpregset (const struct regset *regset,
			   struct regcache *regcache,
			   int regnum, const void *fpregs, size_t len)
{
  size_t regsize = mips_abi_regsize (regcache->arch ());

  gdb_assert (len >= MIPS_FBSD_NUM_FPREGS * regsize);

  mips_fbsd_supply_fpregs (regcache, regnum, fpregs, regsize);
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by FPREGS and LEN in the floating-point
   register set REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
mips_fbsd_collect_fpregset (const struct regset *regset,
			    const struct regcache *regcache,
			    int regnum, void *fpregs, size_t len)
{
  size_t regsize = mips_abi_regsize (regcache->arch ());

  gdb_assert (len >= MIPS_FBSD_NUM_FPREGS * regsize);

  mips_fbsd_collect_fpregs (regcache, regnum, fpregs, regsize);
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
mips_fbsd_supply_gregset (const struct regset *regset,
			  struct regcache *regcache, int regnum,
			  const void *gregs, size_t len)
{
  size_t regsize = mips_abi_regsize (regcache->arch ());

  gdb_assert (len >= MIPS_FBSD_NUM_GREGS * regsize);

  mips_fbsd_supply_gregs (regcache, regnum, gregs, regsize);
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by GREGS and LEN in the general-purpose
   register set REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
mips_fbsd_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs, size_t len)
{
  size_t regsize = mips_abi_regsize (regcache->arch ());

  gdb_assert (len >= MIPS_FBSD_NUM_GREGS * regsize);

  mips_fbsd_collect_gregs (regcache, regnum, gregs, regsize);
}

/* FreeBSD/mips register sets.  */

static const struct regset mips_fbsd_gregset =
{
  NULL,
  mips_fbsd_supply_gregset,
  mips_fbsd_collect_gregset,
};

static const struct regset mips_fbsd_fpregset =
{
  NULL,
  mips_fbsd_supply_fpregset,
  mips_fbsd_collect_fpregset,
};

/* Iterate over core file register note sections.  */

static void
mips_fbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  size_t regsize = mips_abi_regsize (gdbarch);

  cb (".reg", MIPS_FBSD_NUM_GREGS * regsize, MIPS_FBSD_NUM_GREGS * regsize,
      &mips_fbsd_gregset, NULL, cb_data);
  cb (".reg2", MIPS_FBSD_NUM_FPREGS * regsize, MIPS_FBSD_NUM_FPREGS * regsize,
      &mips_fbsd_fpregset, NULL, cb_data);
}

/* Signal trampoline support.  */

#define FBSD_SYS_sigreturn	417

#define MIPS_INST_LI_V0_SIGRETURN 0x24020000 + FBSD_SYS_sigreturn
#define MIPS_INST_SYSCALL	0x0000000c
#define MIPS_INST_BREAK		0x0000000d

#define O32_SIGFRAME_UCONTEXT_OFFSET	(16)
#define O32_SIGSET_T_SIZE	(16)

#define O32_UCONTEXT_ONSTACK	(O32_SIGSET_T_SIZE)
#define O32_UCONTEXT_PC		(O32_UCONTEXT_ONSTACK + 4)
#define O32_UCONTEXT_REGS	(O32_UCONTEXT_PC + 4)
#define O32_UCONTEXT_SR		(O32_UCONTEXT_REGS + 4 * 32)
#define O32_UCONTEXT_LO		(O32_UCONTEXT_SR + 4)
#define O32_UCONTEXT_HI		(O32_UCONTEXT_LO + 4)
#define O32_UCONTEXT_FPUSED	(O32_UCONTEXT_HI + 4)
#define O32_UCONTEXT_FPREGS	(O32_UCONTEXT_FPUSED + 4)

#define O32_UCONTEXT_REG_SIZE	4

static void
mips_fbsd_sigframe_init (const struct tramp_frame *self,
			 frame_info_ptr this_frame,
			 struct trad_frame_cache *cache,
			 CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp, ucontext_addr, addr;
  int regnum;
  gdb_byte buf[4];

  /* We find the appropriate instance of `ucontext_t' at a
     fixed offset in the signal frame.  */
  sp = get_frame_register_signed (this_frame,
				  MIPS_SP_REGNUM + gdbarch_num_regs (gdbarch));
  ucontext_addr = sp + O32_SIGFRAME_UCONTEXT_OFFSET;

  /* PC.  */
  regnum = mips_regnum (gdbarch)->pc;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + O32_UCONTEXT_PC);

  /* GPRs.  */
  for (regnum = MIPS_ZERO_REGNUM, addr = ucontext_addr + O32_UCONTEXT_REGS;
       regnum <= MIPS_RA_REGNUM; regnum++, addr += O32_UCONTEXT_REG_SIZE)
    trad_frame_set_reg_addr (cache,
			     regnum + gdbarch_num_regs (gdbarch),
			     addr);

  regnum = MIPS_PS_REGNUM;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + O32_UCONTEXT_SR);

  /* HI and LO.  */
  regnum = mips_regnum (gdbarch)->lo;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + O32_UCONTEXT_LO);
  regnum = mips_regnum (gdbarch)->hi;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + O32_UCONTEXT_HI);

  if (target_read_memory (ucontext_addr + O32_UCONTEXT_FPUSED, buf, 4) == 0
      && extract_unsigned_integer (buf, 4, byte_order) != 0)
    {
      for (regnum = 0, addr = ucontext_addr + O32_UCONTEXT_FPREGS;
	   regnum < 32; regnum++, addr += O32_UCONTEXT_REG_SIZE)
	trad_frame_set_reg_addr (cache,
				 regnum + gdbarch_fp0_regnum (gdbarch),
				 addr);
      trad_frame_set_reg_addr (cache, mips_regnum (gdbarch)->fp_control_status,
			       addr);
    }

  trad_frame_set_id (cache, frame_id_build (sp, func));
}

#define MIPS_INST_ADDIU_A0_SP_O32 (0x27a40000 \
				   + O32_SIGFRAME_UCONTEXT_OFFSET)

static const struct tramp_frame mips_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  MIPS_INSN32_SIZE,
  {
    { MIPS_INST_ADDIU_A0_SP_O32, ULONGEST_MAX },	/* addiu   a0, sp, SIGF_UC */
    { MIPS_INST_LI_V0_SIGRETURN, ULONGEST_MAX },	/* li      v0, SYS_sigreturn */
    { MIPS_INST_SYSCALL, ULONGEST_MAX },		/* syscall */
    { MIPS_INST_BREAK, ULONGEST_MAX },		/* break */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_fbsd_sigframe_init
};

#define N64_SIGFRAME_UCONTEXT_OFFSET	(32)
#define N64_SIGSET_T_SIZE	(16)

#define N64_UCONTEXT_ONSTACK	(N64_SIGSET_T_SIZE)
#define N64_UCONTEXT_PC		(N64_UCONTEXT_ONSTACK + 8)
#define N64_UCONTEXT_REGS	(N64_UCONTEXT_PC + 8)
#define N64_UCONTEXT_SR		(N64_UCONTEXT_REGS + 8 * 32)
#define N64_UCONTEXT_LO		(N64_UCONTEXT_SR + 8)
#define N64_UCONTEXT_HI		(N64_UCONTEXT_LO + 8)
#define N64_UCONTEXT_FPUSED	(N64_UCONTEXT_HI + 8)
#define N64_UCONTEXT_FPREGS	(N64_UCONTEXT_FPUSED + 8)

#define N64_UCONTEXT_REG_SIZE	8

static void
mips64_fbsd_sigframe_init (const struct tramp_frame *self,
			   frame_info_ptr this_frame,
			   struct trad_frame_cache *cache,
			   CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp, ucontext_addr, addr;
  int regnum;
  gdb_byte buf[4];

  /* We find the appropriate instance of `ucontext_t' at a
     fixed offset in the signal frame.  */
  sp = get_frame_register_signed (this_frame,
				  MIPS_SP_REGNUM + gdbarch_num_regs (gdbarch));
  ucontext_addr = sp + N64_SIGFRAME_UCONTEXT_OFFSET;

  /* PC.  */
  regnum = mips_regnum (gdbarch)->pc;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + N64_UCONTEXT_PC);

  /* GPRs.  */
  for (regnum = MIPS_ZERO_REGNUM, addr = ucontext_addr + N64_UCONTEXT_REGS;
       regnum <= MIPS_RA_REGNUM; regnum++, addr += N64_UCONTEXT_REG_SIZE)
    trad_frame_set_reg_addr (cache,
			     regnum + gdbarch_num_regs (gdbarch),
			     addr);

  regnum = MIPS_PS_REGNUM;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + N64_UCONTEXT_SR);

  /* HI and LO.  */
  regnum = mips_regnum (gdbarch)->lo;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + N64_UCONTEXT_LO);
  regnum = mips_regnum (gdbarch)->hi;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   ucontext_addr + N64_UCONTEXT_HI);

  if (target_read_memory (ucontext_addr + N64_UCONTEXT_FPUSED, buf, 4) == 0
      && extract_unsigned_integer (buf, 4, byte_order) != 0)
    {
      for (regnum = 0, addr = ucontext_addr + N64_UCONTEXT_FPREGS;
	   regnum < 32; regnum++, addr += N64_UCONTEXT_REG_SIZE)
	trad_frame_set_reg_addr (cache,
				 regnum + gdbarch_fp0_regnum (gdbarch),
				 addr);
      trad_frame_set_reg_addr (cache, mips_regnum (gdbarch)->fp_control_status,
			       addr);
    }

  trad_frame_set_id (cache, frame_id_build (sp, func));
}

#define MIPS_INST_ADDIU_A0_SP_N32 (0x27a40000 \
				   + N64_SIGFRAME_UCONTEXT_OFFSET)

static const struct tramp_frame mipsn32_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  MIPS_INSN32_SIZE,
  {
    { MIPS_INST_ADDIU_A0_SP_N32, ULONGEST_MAX },	/* addiu   a0, sp, SIGF_UC */
    { MIPS_INST_LI_V0_SIGRETURN, ULONGEST_MAX },	/* li      v0, SYS_sigreturn */
    { MIPS_INST_SYSCALL, ULONGEST_MAX },		/* syscall */
    { MIPS_INST_BREAK, ULONGEST_MAX },		/* break */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips64_fbsd_sigframe_init
};

#define MIPS_INST_DADDIU_A0_SP_N64 (0x67a40000 \
				    + N64_SIGFRAME_UCONTEXT_OFFSET)

static const struct tramp_frame mips64_fbsd_sigframe =
{
  SIGTRAMP_FRAME,
  MIPS_INSN32_SIZE,
  {
    { MIPS_INST_DADDIU_A0_SP_N64, ULONGEST_MAX },	/* daddiu  a0, sp, SIGF_UC */
    { MIPS_INST_LI_V0_SIGRETURN, ULONGEST_MAX },	/* li      v0, SYS_sigreturn */
    { MIPS_INST_SYSCALL, ULONGEST_MAX },		/* syscall */
    { MIPS_INST_BREAK, ULONGEST_MAX },		/* break */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips64_fbsd_sigframe_init
};

/* Shared library support.  */

/* FreeBSD/mips can use an alternate routine in the runtime linker to
   resolve functions.  */

static CORE_ADDR
mips_fbsd_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol msym
    = lookup_bound_minimal_symbol ("_mips_rtld_bind");
  if (msym.minsym != nullptr && msym.value_address () == pc)
    return frame_unwind_caller_pc (get_current_frame ());

  return fbsd_skip_solib_resolver (gdbarch, pc);
}

/* FreeBSD/mips uses a slightly different `struct link_map' than the
   other FreeBSD platforms as it includes an additional `l_off'
   member.  */

static struct link_map_offsets *
mips_fbsd_ilp32_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 4;
      lmo.r_brk_offset = 8;
      lmo.r_ldsomap_offset = -1;
      lmo.r_next_offset = -1;

      lmo.link_map_size = 24;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 8;
      lmo.l_ld_offset = 12;
      lmo.l_next_offset = 16;
      lmo.l_prev_offset = 20;
    }

  return lmp;
}

static struct link_map_offsets *
mips_fbsd_lp64_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 8;
      lmo.r_brk_offset = 16;
      lmo.r_ldsomap_offset = -1;
      lmo.r_next_offset = -1;

      lmo.link_map_size = 48;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 16;
      lmo.l_ld_offset = 24;
      lmo.l_next_offset = 32;
      lmo.l_prev_offset = 40;
    }

  return lmp;
}

static void
mips_fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  enum mips_abi abi = mips_abi (gdbarch);

  /* Generic FreeBSD support.  */
  fbsd_init_abi (info, gdbarch);

  set_gdbarch_software_single_step (gdbarch, mips_software_single_step);

  switch (abi)
    {
      case MIPS_ABI_O32:
	tramp_frame_prepend_unwinder (gdbarch, &mips_fbsd_sigframe);
	break;
      case MIPS_ABI_N32:
	tramp_frame_prepend_unwinder (gdbarch, &mipsn32_fbsd_sigframe);
	break;
      case MIPS_ABI_N64:
	tramp_frame_prepend_unwinder (gdbarch, &mips64_fbsd_sigframe);
	break;
    }

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, mips_fbsd_iterate_over_regset_sections);

  set_gdbarch_skip_solib_resolver (gdbarch, mips_fbsd_skip_solib_resolver);

  /* FreeBSD/mips has SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, (gdbarch_ptr_bit (gdbarch) == 32 ?
	       mips_fbsd_ilp32_fetch_link_map_offsets :
	       mips_fbsd_lp64_fetch_link_map_offsets));
}

void _initialize_mips_fbsd_tdep ();
void
_initialize_mips_fbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_mips, 0, GDB_OSABI_FREEBSD,
			  mips_fbsd_init_abi);
}

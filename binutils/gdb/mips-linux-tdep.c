/* Target-dependent code for GNU/Linux on MIPS processors.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "solib-svr4.h"
#include "osabi.h"
#include "mips-tdep.h"
#include "frame.h"
#include "regcache.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "solib.h"
#include "solist.h"
#include "symtab.h"
#include "target-descriptions.h"
#include "regset.h"
#include "mips-linux-tdep.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "xml-syscall.h"
#include "gdbsupport/gdb_signals.h"
#include "inferior.h"

#include "features/mips-linux.c"
#include "features/mips-dsp-linux.c"
#include "features/mips64-linux.c"
#include "features/mips64-dsp-linux.c"

static struct target_so_ops mips_svr4_so_ops;

/* This enum represents the signals' numbers on the MIPS
   architecture.  It just contains the signal definitions which are
   different from the generic implementation.

   It is derived from the file <arch/mips/include/uapi/asm/signal.h>,
   from the Linux kernel tree.  */

enum
  {
    MIPS_LINUX_SIGEMT = 7,
    MIPS_LINUX_SIGBUS = 10,
    MIPS_LINUX_SIGSYS = 12,
    MIPS_LINUX_SIGUSR1 = 16,
    MIPS_LINUX_SIGUSR2 = 17,
    MIPS_LINUX_SIGCHLD = 18,
    MIPS_LINUX_SIGCLD = MIPS_LINUX_SIGCHLD,
    MIPS_LINUX_SIGPWR = 19,
    MIPS_LINUX_SIGWINCH = 20,
    MIPS_LINUX_SIGURG = 21,
    MIPS_LINUX_SIGIO = 22,
    MIPS_LINUX_SIGPOLL = MIPS_LINUX_SIGIO,
    MIPS_LINUX_SIGSTOP = 23,
    MIPS_LINUX_SIGTSTP = 24,
    MIPS_LINUX_SIGCONT = 25,
    MIPS_LINUX_SIGTTIN = 26,
    MIPS_LINUX_SIGTTOU = 27,
    MIPS_LINUX_SIGVTALRM = 28,
    MIPS_LINUX_SIGPROF = 29,
    MIPS_LINUX_SIGXCPU = 30,
    MIPS_LINUX_SIGXFSZ = 31,

    MIPS_LINUX_SIGRTMIN = 32,
    MIPS_LINUX_SIGRT64 = 64,
    MIPS_LINUX_SIGRTMAX = 127,
  };

/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure
   from which we extract the pc (MIPS_LINUX_JB_PC) that we will land
   at.  The pc is copied into PC.  This routine returns 1 on
   success.  */

#define MIPS_LINUX_JB_ELEMENT_SIZE 4
#define MIPS_LINUX_JB_PC 0

static int
mips_linux_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT];

  jb_addr = get_frame_register_unsigned (frame, MIPS_A0_REGNUM);

  if (target_read_memory ((jb_addr
			   + MIPS_LINUX_JB_PC * MIPS_LINUX_JB_ELEMENT_SIZE),
			  buf, gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT))
    return 0;

  *pc = extract_unsigned_integer (buf,
				  gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT,
				  byte_order);

  return 1;
}

/* Transform the bits comprising a 32-bit register to the right size
   for regcache_raw_supply().  This is needed when mips_isa_regsize()
   is 8.  */

static void
supply_32bit_reg (struct regcache *regcache, int regnum, const void *addr)
{
  regcache->raw_supply_integer (regnum, (const gdb_byte *) addr, 4, true);
}

/* Unpack an elf_gregset_t into GDB's register cache.  */

void
mips_supply_gregset (struct regcache *regcache,
		     const mips_elf_gregset_t *gregsetp)
{
  int regi;
  const mips_elf_greg_t *regp = *gregsetp;
  struct gdbarch *gdbarch = regcache->arch ();

  for (regi = EF_REG0 + 1; regi <= EF_REG31; regi++)
    supply_32bit_reg (regcache, regi - EF_REG0, regp + regi);

  if (mips_linux_restart_reg_p (gdbarch))
    supply_32bit_reg (regcache, MIPS_RESTART_REGNUM, regp + EF_REG0);

  supply_32bit_reg (regcache, mips_regnum (gdbarch)->lo, regp + EF_LO);
  supply_32bit_reg (regcache, mips_regnum (gdbarch)->hi, regp + EF_HI);

  supply_32bit_reg (regcache, mips_regnum (gdbarch)->pc,
		    regp + EF_CP0_EPC);
  supply_32bit_reg (regcache, mips_regnum (gdbarch)->badvaddr,
		    regp + EF_CP0_BADVADDR);
  supply_32bit_reg (regcache, MIPS_PS_REGNUM, regp + EF_CP0_STATUS);
  supply_32bit_reg (regcache, mips_regnum (gdbarch)->cause,
		    regp + EF_CP0_CAUSE);

  /* Fill the inaccessible zero register with zero.  */
  regcache->raw_supply_zeroed (MIPS_ZERO_REGNUM);
}

static void
mips_supply_gregset_wrapper (const struct regset *regset,
			     struct regcache *regcache,
			     int regnum, const void *gregs, size_t len)
{
  gdb_assert (len >= sizeof (mips_elf_gregset_t));

  mips_supply_gregset (regcache, (const mips_elf_gregset_t *)gregs);
}

/* Pack our registers (or one register) into an elf_gregset_t.  */

void
mips_fill_gregset (const struct regcache *regcache,
		   mips_elf_gregset_t *gregsetp, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int regaddr, regi;
  mips_elf_greg_t *regp = *gregsetp;
  void *dst;

  if (regno == -1)
    {
      memset (regp, 0, sizeof (mips_elf_gregset_t));
      for (regi = 1; regi < 32; regi++)
	mips_fill_gregset (regcache, gregsetp, regi);
      mips_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->lo);
      mips_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->hi);
      mips_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->pc);
      mips_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->badvaddr);
      mips_fill_gregset (regcache, gregsetp, MIPS_PS_REGNUM);
      mips_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->cause);
      mips_fill_gregset (regcache, gregsetp, MIPS_RESTART_REGNUM);
      return;
   }

  if (regno > 0 && regno < 32)
    {
      dst = regp + regno + EF_REG0;
      regcache->raw_collect (regno, dst);
      return;
    }

  if (regno == mips_regnum (gdbarch)->lo)
     regaddr = EF_LO;
  else if (regno == mips_regnum (gdbarch)->hi)
    regaddr = EF_HI;
  else if (regno == mips_regnum (gdbarch)->pc)
    regaddr = EF_CP0_EPC;
  else if (regno == mips_regnum (gdbarch)->badvaddr)
    regaddr = EF_CP0_BADVADDR;
  else if (regno == MIPS_PS_REGNUM)
    regaddr = EF_CP0_STATUS;
  else if (regno == mips_regnum (gdbarch)->cause)
    regaddr = EF_CP0_CAUSE;
  else if (mips_linux_restart_reg_p (gdbarch)
	   && regno == MIPS_RESTART_REGNUM)
    regaddr = EF_REG0;
  else
    regaddr = -1;

  if (regaddr != -1)
    {
      dst = regp + regaddr;
      regcache->raw_collect (regno, dst);
    }
}

static void
mips_fill_gregset_wrapper (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs, size_t len)
{
  gdb_assert (len >= sizeof (mips_elf_gregset_t));

  mips_fill_gregset (regcache, (mips_elf_gregset_t *)gregs, regnum);
}

/* Support for 64-bit ABIs.  */

/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure
   from which we extract the pc (MIPS_LINUX_JB_PC) that we will land
   at.  The pc is copied into PC.  This routine returns 1 on
   success.  */

/* Details about jmp_buf.  */

#define MIPS64_LINUX_JB_PC 0

static int
mips64_linux_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte *buf
    = (gdb_byte *) alloca (gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
  int element_size = gdbarch_ptr_bit (gdbarch) == 32 ? 4 : 8;

  jb_addr = get_frame_register_unsigned (frame, MIPS_A0_REGNUM);

  if (target_read_memory (jb_addr + MIPS64_LINUX_JB_PC * element_size,
			  buf,
			  gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT))
    return 0;

  *pc = extract_unsigned_integer (buf,
				  gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT,
				  byte_order);

  return 1;
}

/* Register set support functions.  These operate on standard 64-bit
   regsets, but work whether the target is 32-bit or 64-bit.  A 32-bit
   target will still use the 64-bit format for PTRACE_GETREGS.  */

/* Supply a 64-bit register.  */

static void
supply_64bit_reg (struct regcache *regcache, int regnum,
		  const gdb_byte *buf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
      && register_size (gdbarch, regnum) == 4)
    regcache->raw_supply (regnum, buf + 4);
  else
    regcache->raw_supply (regnum, buf);
}

/* Unpack a 64-bit elf_gregset_t into GDB's register cache.  */

void
mips64_supply_gregset (struct regcache *regcache,
		       const mips64_elf_gregset_t *gregsetp)
{
  int regi;
  const mips64_elf_greg_t *regp = *gregsetp;
  struct gdbarch *gdbarch = regcache->arch ();

  for (regi = MIPS64_EF_REG0 + 1; regi <= MIPS64_EF_REG31; regi++)
    supply_64bit_reg (regcache, regi - MIPS64_EF_REG0,
		      (const gdb_byte *) (regp + regi));

  if (mips_linux_restart_reg_p (gdbarch))
    supply_64bit_reg (regcache, MIPS_RESTART_REGNUM,
		      (const gdb_byte *) (regp + MIPS64_EF_REG0));

  supply_64bit_reg (regcache, mips_regnum (gdbarch)->lo,
		    (const gdb_byte *) (regp + MIPS64_EF_LO));
  supply_64bit_reg (regcache, mips_regnum (gdbarch)->hi,
		    (const gdb_byte *) (regp + MIPS64_EF_HI));

  supply_64bit_reg (regcache, mips_regnum (gdbarch)->pc,
		    (const gdb_byte *) (regp + MIPS64_EF_CP0_EPC));
  supply_64bit_reg (regcache, mips_regnum (gdbarch)->badvaddr,
		    (const gdb_byte *) (regp + MIPS64_EF_CP0_BADVADDR));
  supply_64bit_reg (regcache, MIPS_PS_REGNUM,
		    (const gdb_byte *) (regp + MIPS64_EF_CP0_STATUS));
  supply_64bit_reg (regcache, mips_regnum (gdbarch)->cause,
		    (const gdb_byte *) (regp + MIPS64_EF_CP0_CAUSE));

  /* Fill the inaccessible zero register with zero.  */
  regcache->raw_supply_zeroed (MIPS_ZERO_REGNUM);
}

static void
mips64_supply_gregset_wrapper (const struct regset *regset,
			       struct regcache *regcache,
			       int regnum, const void *gregs, size_t len)
{
  gdb_assert (len >= sizeof (mips64_elf_gregset_t));

  mips64_supply_gregset (regcache, (const mips64_elf_gregset_t *)gregs);
}

/* Pack our registers (or one register) into a 64-bit elf_gregset_t.  */

void
mips64_fill_gregset (const struct regcache *regcache,
		     mips64_elf_gregset_t *gregsetp, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int regaddr, regi;
  mips64_elf_greg_t *regp = *gregsetp;
  void *dst;

  if (regno == -1)
    {
      memset (regp, 0, sizeof (mips64_elf_gregset_t));
      for (regi = 1; regi < 32; regi++)
	mips64_fill_gregset (regcache, gregsetp, regi);
      mips64_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->lo);
      mips64_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->hi);
      mips64_fill_gregset (regcache, gregsetp, mips_regnum (gdbarch)->pc);
      mips64_fill_gregset (regcache, gregsetp,
			   mips_regnum (gdbarch)->badvaddr);
      mips64_fill_gregset (regcache, gregsetp, MIPS_PS_REGNUM);
      mips64_fill_gregset (regcache, gregsetp,  mips_regnum (gdbarch)->cause);
      mips64_fill_gregset (regcache, gregsetp, MIPS_RESTART_REGNUM);
      return;
   }

  if (regno > 0 && regno < 32)
    regaddr = regno + MIPS64_EF_REG0;
  else if (regno == mips_regnum (gdbarch)->lo)
    regaddr = MIPS64_EF_LO;
  else if (regno == mips_regnum (gdbarch)->hi)
    regaddr = MIPS64_EF_HI;
  else if (regno == mips_regnum (gdbarch)->pc)
    regaddr = MIPS64_EF_CP0_EPC;
  else if (regno == mips_regnum (gdbarch)->badvaddr)
    regaddr = MIPS64_EF_CP0_BADVADDR;
  else if (regno == MIPS_PS_REGNUM)
    regaddr = MIPS64_EF_CP0_STATUS;
  else if (regno == mips_regnum (gdbarch)->cause)
    regaddr = MIPS64_EF_CP0_CAUSE;
  else if (mips_linux_restart_reg_p (gdbarch)
	   && regno == MIPS_RESTART_REGNUM)
    regaddr = MIPS64_EF_REG0;
  else
    regaddr = -1;

  if (regaddr != -1)
    {
      dst = regp + regaddr;
      regcache->raw_collect_integer (regno, (gdb_byte *) dst, 8, true);
    }
}

static void
mips64_fill_gregset_wrapper (const struct regset *regset,
			     const struct regcache *regcache,
			     int regnum, void *gregs, size_t len)
{
  gdb_assert (len >= sizeof (mips64_elf_gregset_t));

  mips64_fill_gregset (regcache, (mips64_elf_gregset_t *)gregs, regnum);
}

/* Likewise, unpack an elf_fpregset_t.  Linux only uses even-numbered
   FPR slots in the Status.FR=0 mode, storing even-odd FPR pairs as the
   SDC1 instruction would.  When run on MIPS I architecture processors
   all FPR slots used to be used, unusually, holding the respective FPRs
   in the first 4 bytes, but that was corrected for consistency, with
   `linux-mips.org' (LMO) commit 42533948caac ("Major pile of FP emulator
   changes."), the fix corrected with LMO commit 849fa7a50dff ("R3k FPU
   ptrace() handling fixes."), and then broken and fixed over and over
   again, until last time fixed with commit 80cbfad79096 ("MIPS: Correct
   MIPS I FP context layout").  */

void
mips64_supply_fpregset (struct regcache *regcache,
			const mips64_elf_fpregset_t *fpregsetp)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int regi;

  if (register_size (gdbarch, gdbarch_fp0_regnum (gdbarch)) == 4)
    for (regi = 0; regi < 32; regi++)
      {
	const gdb_byte *reg_ptr
	  = (const gdb_byte *) (*fpregsetp + (regi & ~1));
	if ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG) != (regi & 1))
	  reg_ptr += 4;
	regcache->raw_supply (gdbarch_fp0_regnum (gdbarch) + regi, reg_ptr);
      }
  else
    for (regi = 0; regi < 32; regi++)
      regcache->raw_supply (gdbarch_fp0_regnum (gdbarch) + regi,
			    (const char *) (*fpregsetp + regi));

  supply_32bit_reg (regcache, mips_regnum (gdbarch)->fp_control_status,
		    (const gdb_byte *) (*fpregsetp + 32));

  /* The ABI doesn't tell us how to supply FCRIR, and core dumps don't
     include it - but the result of PTRACE_GETFPREGS does.  The best we
     can do is to assume that its value is present.  */
  supply_32bit_reg (regcache,
		    mips_regnum (gdbarch)->fp_implementation_revision,
		    (const gdb_byte *) (*fpregsetp + 32) + 4);
}

static void
mips64_supply_fpregset_wrapper (const struct regset *regset,
				struct regcache *regcache,
				int regnum, const void *gregs, size_t len)
{
  gdb_assert (len >= sizeof (mips64_elf_fpregset_t));

  mips64_supply_fpregset (regcache, (const mips64_elf_fpregset_t *)gregs);
}

/* Likewise, pack one or all floating point registers into an
   elf_fpregset_t.  See `mips_supply_fpregset' for an explanation
   of the layout.  */

void
mips64_fill_fpregset (const struct regcache *regcache,
		      mips64_elf_fpregset_t *fpregsetp, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  gdb_byte *to;

  if ((regno >= gdbarch_fp0_regnum (gdbarch))
      && (regno < gdbarch_fp0_regnum (gdbarch) + 32))
    {
      if (register_size (gdbarch, regno) == 4)
	{
	  int regi = regno - gdbarch_fp0_regnum (gdbarch);

	  to = (gdb_byte *) (*fpregsetp + (regi & ~1));
	  if ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG) != (regi & 1))
	    to += 4;
	  regcache->raw_collect (regno, to);
	}
      else
	{
	  to = (gdb_byte *) (*fpregsetp + regno
			     - gdbarch_fp0_regnum (gdbarch));
	  regcache->raw_collect (regno, to);
	}
    }
  else if (regno == mips_regnum (gdbarch)->fp_control_status)
    {
      to = (gdb_byte *) (*fpregsetp + 32);
      regcache->raw_collect_integer (regno, to, 4, true);
    }
  else if (regno == mips_regnum (gdbarch)->fp_implementation_revision)
    {
      to = (gdb_byte *) (*fpregsetp + 32) + 4;
      regcache->raw_collect_integer (regno, to, 4, true);
    }
  else if (regno == -1)
    {
      int regi;

      for (regi = 0; regi < 32; regi++)
	mips64_fill_fpregset (regcache, fpregsetp,
			      gdbarch_fp0_regnum (gdbarch) + regi);
      mips64_fill_fpregset (regcache, fpregsetp,
			    mips_regnum (gdbarch)->fp_control_status);
      mips64_fill_fpregset (regcache, fpregsetp,
			    mips_regnum (gdbarch)->fp_implementation_revision);
    }
}

static void
mips64_fill_fpregset_wrapper (const struct regset *regset,
			      const struct regcache *regcache,
			      int regnum, void *gregs, size_t len)
{
  gdb_assert (len >= sizeof (mips64_elf_fpregset_t));

  mips64_fill_fpregset (regcache, (mips64_elf_fpregset_t *)gregs, regnum);
}

static const struct regset mips_linux_gregset =
  {
    NULL, mips_supply_gregset_wrapper, mips_fill_gregset_wrapper
  };

static const struct regset mips64_linux_gregset =
  {
    NULL, mips64_supply_gregset_wrapper, mips64_fill_gregset_wrapper
  };

static const struct regset mips64_linux_fpregset =
  {
    NULL, mips64_supply_fpregset_wrapper, mips64_fill_fpregset_wrapper
  };

static void
mips_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  if (register_size (gdbarch, MIPS_ZERO_REGNUM) == 4)
    {
      cb (".reg", sizeof (mips_elf_gregset_t), sizeof (mips_elf_gregset_t),
	  &mips_linux_gregset, NULL, cb_data);
      cb (".reg2", sizeof (mips64_elf_fpregset_t),
	  sizeof (mips64_elf_fpregset_t), &mips64_linux_fpregset,
	  NULL, cb_data);
    }
  else
    {
      cb (".reg", sizeof (mips64_elf_gregset_t), sizeof (mips64_elf_gregset_t),
	  &mips64_linux_gregset, NULL, cb_data);
      cb (".reg2", sizeof (mips64_elf_fpregset_t),
	  sizeof (mips64_elf_fpregset_t), &mips64_linux_fpregset,
	  NULL, cb_data);
    }
}

static const struct target_desc *
mips_linux_core_read_description (struct gdbarch *gdbarch,
				  struct target_ops *target,
				  bfd *abfd)
{
  asection *section = bfd_get_section_by_name (abfd, ".reg");
  if (! section)
    return NULL;

  switch (bfd_section_size (section))
    {
    case sizeof (mips_elf_gregset_t):
      return mips_tdesc_gp32;

    case sizeof (mips64_elf_gregset_t):
      return mips_tdesc_gp64;

    default:
      return NULL;
    }
}


/* Check the code at PC for a dynamic linker lazy resolution stub.
   GNU ld for MIPS has put lazy resolution stubs into a ".MIPS.stubs"
   section uniformly since version 2.15.  If the pc is in that section,
   then we are in such a stub.  Before that ".stub" was used in 32-bit
   ELF binaries, however we do not bother checking for that since we
   have never had and that case should be extremely rare these days.
   Instead we pattern-match on the code generated by GNU ld.  They look
   like this:

   lw t9,0x8010(gp)
   addu t7,ra
   jalr t9,ra
   addiu t8,zero,INDEX

   (with the appropriate doubleword instructions for N64).  As any lazy
   resolution stubs in microMIPS binaries will always be in a
   ".MIPS.stubs" section we only ever verify standard MIPS patterns. */

static int
mips_linux_in_dynsym_stub (CORE_ADDR pc)
{
  gdb_byte buf[28], *p;
  ULONGEST insn, insn1;
  int n64 = (mips_abi (current_inferior ()->arch ()) == MIPS_ABI_N64);
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());

  if (in_mips_stubs_section (pc))
    return 1;

  read_memory (pc - 12, buf, 28);

  if (n64)
    {
      /* ld t9,0x8010(gp) */
      insn1 = 0xdf998010;
    }
  else
    {
      /* lw t9,0x8010(gp) */
      insn1 = 0x8f998010;
    }

  p = buf + 12;
  while (p >= buf)
    {
      insn = extract_unsigned_integer (p, 4, byte_order);
      if (insn == insn1)
	break;
      p -= 4;
    }
  if (p < buf)
    return 0;

  insn = extract_unsigned_integer (p + 4, 4, byte_order);
  if (n64)
    {
      /* 'daddu t7,ra' or 'or t7, ra, zero'*/
      if (insn != 0x03e0782d && insn != 0x03e07825)
	return 0;
    }
  else
    {
      /* 'addu t7,ra'  or 'or t7, ra, zero'*/
      if (insn != 0x03e07821 && insn != 0x03e07825)
	return 0;
    }

  insn = extract_unsigned_integer (p + 8, 4, byte_order);
  /* jalr t9,ra */
  if (insn != 0x0320f809)
    return 0;

  insn = extract_unsigned_integer (p + 12, 4, byte_order);
  if (n64)
    {
      /* daddiu t8,zero,0 */
      if ((insn & 0xffff0000) != 0x64180000)
	return 0;
    }
  else
    {
      /* addiu t8,zero,0 */
      if ((insn & 0xffff0000) != 0x24180000)
	return 0;
    }

  return 1;
}

/* Return non-zero iff PC belongs to the dynamic linker resolution
   code, a PLT entry, or a lazy binding stub.  */

static int
mips_linux_in_dynsym_resolve_code (CORE_ADDR pc)
{
  /* Check whether PC is in the dynamic linker.  This also checks
     whether it is in the .plt section, used by non-PIC executables.  */
  if (svr4_in_dynsym_resolve_code (pc))
    return 1;

  /* Likewise for the stubs.  They live in the .MIPS.stubs section these
     days, so we check if the PC is within, than fall back to a pattern
     match.  */
  if (mips_linux_in_dynsym_stub (pc))
    return 1;

  return 0;
}

/* See the comments for SKIP_SOLIB_RESOLVER at the top of infrun.c,
   and glibc_skip_solib_resolver in glibc-tdep.c.  The normal glibc
   implementation of this triggers at "fixup" from the same objfile as
   "_dl_runtime_resolve"; MIPS GNU/Linux can trigger at
   "__dl_runtime_resolve" directly.  An unresolved lazy binding
   stub will point to _dl_runtime_resolve, which will first call
   __dl_runtime_resolve, and then pass control to the resolved
   function.  */

static CORE_ADDR
mips_linux_skip_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol resolver;

  resolver = lookup_minimal_symbol ("__dl_runtime_resolve", NULL, NULL);

  if (resolver.minsym && resolver.value_address () == pc)
    return frame_unwind_caller_pc (get_current_frame ());

  return glibc_skip_solib_resolver (gdbarch, pc);
}

/* Signal trampoline support.  There are four supported layouts for a
   signal frame: o32 sigframe, o32 rt_sigframe, n32 rt_sigframe, and
   n64 rt_sigframe.  We handle them all independently; not the most
   efficient way, but simplest.  First, declare all the unwinders.  */

static void mips_linux_o32_sigframe_init (const struct tramp_frame *self,
					  frame_info_ptr this_frame,
					  struct trad_frame_cache *this_cache,
					  CORE_ADDR func);

static void mips_linux_n32n64_sigframe_init (const struct tramp_frame *self,
					     frame_info_ptr this_frame,
					     struct trad_frame_cache *this_cache,
					     CORE_ADDR func);

static int mips_linux_sigframe_validate (const struct tramp_frame *self,
					 frame_info_ptr this_frame,
					 CORE_ADDR *pc);

static int micromips_linux_sigframe_validate (const struct tramp_frame *self,
					      frame_info_ptr this_frame,
					      CORE_ADDR *pc);

#define MIPS_NR_LINUX 4000
#define MIPS_NR_N64_LINUX 5000
#define MIPS_NR_N32_LINUX 6000

#define MIPS_NR_sigreturn MIPS_NR_LINUX + 119
#define MIPS_NR_rt_sigreturn MIPS_NR_LINUX + 193
#define MIPS_NR_N64_rt_sigreturn MIPS_NR_N64_LINUX + 211
#define MIPS_NR_N32_rt_sigreturn MIPS_NR_N32_LINUX + 211

#define MIPS_INST_LI_V0_SIGRETURN 0x24020000 + MIPS_NR_sigreturn
#define MIPS_INST_LI_V0_RT_SIGRETURN 0x24020000 + MIPS_NR_rt_sigreturn
#define MIPS_INST_LI_V0_N64_RT_SIGRETURN 0x24020000 + MIPS_NR_N64_rt_sigreturn
#define MIPS_INST_LI_V0_N32_RT_SIGRETURN 0x24020000 + MIPS_NR_N32_rt_sigreturn
#define MIPS_INST_SYSCALL 0x0000000c

#define MICROMIPS_INST_LI_V0 0x3040
#define MICROMIPS_INST_POOL32A 0x0000
#define MICROMIPS_INST_SYSCALL 0x8b7c

static const struct tramp_frame mips_linux_o32_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { MIPS_INST_LI_V0_SIGRETURN, ULONGEST_MAX },
    { MIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_o32_sigframe_init,
  mips_linux_sigframe_validate
};

static const struct tramp_frame mips_linux_o32_rt_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { MIPS_INST_LI_V0_RT_SIGRETURN, ULONGEST_MAX },
    { MIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX } },
  mips_linux_o32_sigframe_init,
  mips_linux_sigframe_validate
};

static const struct tramp_frame mips_linux_n32_rt_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { MIPS_INST_LI_V0_N32_RT_SIGRETURN, ULONGEST_MAX },
    { MIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_n32n64_sigframe_init,
  mips_linux_sigframe_validate
};

static const struct tramp_frame mips_linux_n64_rt_sigframe = {
  SIGTRAMP_FRAME,
  4,
  {
    { MIPS_INST_LI_V0_N64_RT_SIGRETURN, ULONGEST_MAX },
    { MIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_n32n64_sigframe_init,
  mips_linux_sigframe_validate
};

static const struct tramp_frame micromips_linux_o32_sigframe = {
  SIGTRAMP_FRAME,
  2,
  {
    { MICROMIPS_INST_LI_V0, ULONGEST_MAX },
    { MIPS_NR_sigreturn, ULONGEST_MAX },
    { MICROMIPS_INST_POOL32A, ULONGEST_MAX },
    { MICROMIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_o32_sigframe_init,
  micromips_linux_sigframe_validate
};

static const struct tramp_frame micromips_linux_o32_rt_sigframe = {
  SIGTRAMP_FRAME,
  2,
  {
    { MICROMIPS_INST_LI_V0, ULONGEST_MAX },
    { MIPS_NR_rt_sigreturn, ULONGEST_MAX },
    { MICROMIPS_INST_POOL32A, ULONGEST_MAX },
    { MICROMIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_o32_sigframe_init,
  micromips_linux_sigframe_validate
};

static const struct tramp_frame micromips_linux_n32_rt_sigframe = {
  SIGTRAMP_FRAME,
  2,
  {
    { MICROMIPS_INST_LI_V0, ULONGEST_MAX },
    { MIPS_NR_N32_rt_sigreturn, ULONGEST_MAX },
    { MICROMIPS_INST_POOL32A, ULONGEST_MAX },
    { MICROMIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_n32n64_sigframe_init,
  micromips_linux_sigframe_validate
};

static const struct tramp_frame micromips_linux_n64_rt_sigframe = {
  SIGTRAMP_FRAME,
  2,
  {
    { MICROMIPS_INST_LI_V0, ULONGEST_MAX },
    { MIPS_NR_N64_rt_sigreturn, ULONGEST_MAX },
    { MICROMIPS_INST_POOL32A, ULONGEST_MAX },
    { MICROMIPS_INST_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips_linux_n32n64_sigframe_init,
  micromips_linux_sigframe_validate
};

/* The unwinder for o32 signal frames.  The legacy structures look
   like this:

   struct sigframe {
     u32 sf_ass[4];            [argument save space for o32]
     u32 sf_code[2];           [signal trampoline or fill]
     struct sigcontext sf_sc;
     sigset_t sf_mask;
   };

   Pre-2.6.12 sigcontext:

   struct sigcontext {
	unsigned int       sc_regmask;          [Unused]
	unsigned int       sc_status;
	unsigned long long sc_pc;
	unsigned long long sc_regs[32];
	unsigned long long sc_fpregs[32];
	unsigned int       sc_ownedfp;
	unsigned int       sc_fpc_csr;
	unsigned int       sc_fpc_eir;          [Unused]
	unsigned int       sc_used_math;
	unsigned int       sc_ssflags;          [Unused]
	[Alignment hole of four bytes]
	unsigned long long sc_mdhi;
	unsigned long long sc_mdlo;

	unsigned int       sc_cause;            [Unused]
	unsigned int       sc_badvaddr;         [Unused]

	unsigned long      sc_sigset[4];        [kernel's sigset_t]
   };

   Post-2.6.12 sigcontext (SmartMIPS/DSP support added):

   struct sigcontext {
	unsigned int       sc_regmask;          [Unused]
	unsigned int       sc_status;           [Unused]
	unsigned long long sc_pc;
	unsigned long long sc_regs[32];
	unsigned long long sc_fpregs[32];
	unsigned int       sc_acx;
	unsigned int       sc_fpc_csr;
	unsigned int       sc_fpc_eir;          [Unused]
	unsigned int       sc_used_math;
	unsigned int       sc_dsp;
	[Alignment hole of four bytes]
	unsigned long long sc_mdhi;
	unsigned long long sc_mdlo;
	unsigned long      sc_hi1;
	unsigned long      sc_lo1;
	unsigned long      sc_hi2;
	unsigned long      sc_lo2;
	unsigned long      sc_hi3;
	unsigned long      sc_lo3;
   };

   The RT signal frames look like this:

   struct rt_sigframe {
     u32 rs_ass[4];            [argument save space for o32]
     u32 rs_code[2]            [signal trampoline or fill]
     struct siginfo rs_info;
     struct ucontext rs_uc;
   };

   struct ucontext {
     unsigned long     uc_flags;
     struct ucontext  *uc_link;
     stack_t           uc_stack;
     [Alignment hole of four bytes]
     struct sigcontext uc_mcontext;
     sigset_t          uc_sigmask;
   };  */

#define SIGFRAME_SIGCONTEXT_OFFSET   (6 * 4)

#define RTSIGFRAME_SIGINFO_SIZE      128
#define STACK_T_SIZE                 (3 * 4)
#define UCONTEXT_SIGCONTEXT_OFFSET   (2 * 4 + STACK_T_SIZE + 4)
#define RTSIGFRAME_SIGCONTEXT_OFFSET (SIGFRAME_SIGCONTEXT_OFFSET \
				      + RTSIGFRAME_SIGINFO_SIZE \
				      + UCONTEXT_SIGCONTEXT_OFFSET)

#define SIGCONTEXT_PC       (1 * 8)
#define SIGCONTEXT_REGS     (2 * 8)
#define SIGCONTEXT_FPREGS   (34 * 8)
#define SIGCONTEXT_FPCSR    (66 * 8 + 4)
#define SIGCONTEXT_DSPCTL   (68 * 8 + 0)
#define SIGCONTEXT_HI       (69 * 8)
#define SIGCONTEXT_LO       (70 * 8)
#define SIGCONTEXT_CAUSE    (71 * 8 + 0)
#define SIGCONTEXT_BADVADDR (71 * 8 + 4)
#define SIGCONTEXT_HI1      (71 * 8 + 0)
#define SIGCONTEXT_LO1      (71 * 8 + 4)
#define SIGCONTEXT_HI2      (72 * 8 + 0)
#define SIGCONTEXT_LO2      (72 * 8 + 4)
#define SIGCONTEXT_HI3      (73 * 8 + 0)
#define SIGCONTEXT_LO3      (73 * 8 + 4)

#define SIGCONTEXT_REG_SIZE 8

static void
mips_linux_o32_sigframe_init (const struct tramp_frame *self,
			      frame_info_ptr this_frame,
			      struct trad_frame_cache *this_cache,
			      CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int ireg;
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR sigcontext_base;
  const struct mips_regnum *regs = mips_regnum (gdbarch);
  CORE_ADDR regs_base;

  if (self == &mips_linux_o32_sigframe
      || self == &micromips_linux_o32_sigframe)
    sigcontext_base = frame_sp + SIGFRAME_SIGCONTEXT_OFFSET;
  else
    sigcontext_base = frame_sp + RTSIGFRAME_SIGCONTEXT_OFFSET;

  /* I'm not proud of this hack.  Eventually we will have the
     infrastructure to indicate the size of saved registers on a
     per-frame basis, but right now we don't; the kernel saves eight
     bytes but we only want four.  Use regs_base to access any
     64-bit fields.  */
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    regs_base = sigcontext_base + 4;
  else
    regs_base = sigcontext_base;

  if (mips_linux_restart_reg_p (gdbarch))
    trad_frame_set_reg_addr (this_cache,
			     (MIPS_RESTART_REGNUM
			      + gdbarch_num_regs (gdbarch)),
			     regs_base + SIGCONTEXT_REGS);

  for (ireg = 1; ireg < 32; ireg++)
    trad_frame_set_reg_addr (this_cache,
			     (ireg + MIPS_ZERO_REGNUM
			      + gdbarch_num_regs (gdbarch)),
			     (regs_base + SIGCONTEXT_REGS
			      + ireg * SIGCONTEXT_REG_SIZE));

  for (ireg = 0; ireg < 32; ireg++)
    if ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG) != (ireg & 1))
      trad_frame_set_reg_addr (this_cache,
			       ireg + regs->fp0 + gdbarch_num_regs (gdbarch),
			       (sigcontext_base + SIGCONTEXT_FPREGS + 4
				+ (ireg & ~1) * SIGCONTEXT_REG_SIZE));
    else
      trad_frame_set_reg_addr (this_cache,
			       ireg + regs->fp0 + gdbarch_num_regs (gdbarch),
			       (sigcontext_base + SIGCONTEXT_FPREGS
				+ (ireg & ~1) * SIGCONTEXT_REG_SIZE));

  trad_frame_set_reg_addr (this_cache,
			   regs->pc + gdbarch_num_regs (gdbarch),
			   regs_base + SIGCONTEXT_PC);

  trad_frame_set_reg_addr (this_cache,
			   (regs->fp_control_status
			    + gdbarch_num_regs (gdbarch)),
			   sigcontext_base + SIGCONTEXT_FPCSR);

  if (regs->dspctl != -1)
    trad_frame_set_reg_addr (this_cache,
			     regs->dspctl + gdbarch_num_regs (gdbarch),
			     sigcontext_base + SIGCONTEXT_DSPCTL);

  trad_frame_set_reg_addr (this_cache,
			   regs->hi + gdbarch_num_regs (gdbarch),
			   regs_base + SIGCONTEXT_HI);
  trad_frame_set_reg_addr (this_cache,
			   regs->lo + gdbarch_num_regs (gdbarch),
			   regs_base + SIGCONTEXT_LO);

  if (regs->dspacc != -1)
    {
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 0 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_HI1);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 1 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_LO1);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 2 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_HI2);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 3 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_LO2);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 4 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_HI3);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 5 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_LO3);
    }
  else
    {
      trad_frame_set_reg_addr (this_cache,
			       regs->cause + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_CAUSE);
      trad_frame_set_reg_addr (this_cache,
			       regs->badvaddr + gdbarch_num_regs (gdbarch),
			       sigcontext_base + SIGCONTEXT_BADVADDR);
    }

  /* Choice of the bottom of the sigframe is somewhat arbitrary.  */
  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* For N32/N64 things look different.  There is no non-rt signal frame.

  struct rt_sigframe_n32 {
    u32 rs_ass[4];                  [ argument save space for o32 ]
    u32 rs_code[2];                 [ signal trampoline or fill ]
    struct siginfo rs_info;
    struct ucontextn32 rs_uc;
  };

  struct ucontextn32 {
    u32                 uc_flags;
    s32                 uc_link;
    stack32_t           uc_stack;
    struct sigcontext   uc_mcontext;
    sigset_t            uc_sigmask;   [ mask last for extensibility ]
  };

  struct rt_sigframe {
    u32 rs_ass[4];                  [ argument save space for o32 ]
    u32 rs_code[2];                 [ signal trampoline ]
    struct siginfo rs_info;
    struct ucontext rs_uc;
  };

  struct ucontext {
    unsigned long     uc_flags;
    struct ucontext  *uc_link;
    stack_t           uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t          uc_sigmask;   [ mask last for extensibility ]
  };

  And the sigcontext is different (this is for both n32 and n64):

  struct sigcontext {
    unsigned long long sc_regs[32];
    unsigned long long sc_fpregs[32];
    unsigned long long sc_mdhi;
    unsigned long long sc_hi1;
    unsigned long long sc_hi2;
    unsigned long long sc_hi3;
    unsigned long long sc_mdlo;
    unsigned long long sc_lo1;
    unsigned long long sc_lo2;
    unsigned long long sc_lo3;
    unsigned long long sc_pc;
    unsigned int       sc_fpc_csr;
    unsigned int       sc_used_math;
    unsigned int       sc_dsp;
    unsigned int       sc_reserved;
  };

  That is the post-2.6.12 definition of the 64-bit sigcontext; before
  then, there were no hi1-hi3 or lo1-lo3.  Cause and badvaddr were
  included too.  */

#define N32_STACK_T_SIZE		STACK_T_SIZE
#define N64_STACK_T_SIZE		(2 * 8 + 4)
#define N32_UCONTEXT_SIGCONTEXT_OFFSET  (2 * 4 + N32_STACK_T_SIZE + 4)
#define N64_UCONTEXT_SIGCONTEXT_OFFSET  (2 * 8 + N64_STACK_T_SIZE + 4)
#define N32_SIGFRAME_SIGCONTEXT_OFFSET	(SIGFRAME_SIGCONTEXT_OFFSET \
					 + RTSIGFRAME_SIGINFO_SIZE \
					 + N32_UCONTEXT_SIGCONTEXT_OFFSET)
#define N64_SIGFRAME_SIGCONTEXT_OFFSET	(SIGFRAME_SIGCONTEXT_OFFSET \
					 + RTSIGFRAME_SIGINFO_SIZE \
					 + N64_UCONTEXT_SIGCONTEXT_OFFSET)

#define N64_SIGCONTEXT_REGS     (0 * 8)
#define N64_SIGCONTEXT_FPREGS   (32 * 8)
#define N64_SIGCONTEXT_HI       (64 * 8)
#define N64_SIGCONTEXT_HI1      (65 * 8)
#define N64_SIGCONTEXT_HI2      (66 * 8)
#define N64_SIGCONTEXT_HI3      (67 * 8)
#define N64_SIGCONTEXT_LO       (68 * 8)
#define N64_SIGCONTEXT_LO1      (69 * 8)
#define N64_SIGCONTEXT_LO2      (70 * 8)
#define N64_SIGCONTEXT_LO3      (71 * 8)
#define N64_SIGCONTEXT_PC       (72 * 8)
#define N64_SIGCONTEXT_FPCSR    (73 * 8 + 0)
#define N64_SIGCONTEXT_DSPCTL   (74 * 8 + 0)

#define N64_SIGCONTEXT_REG_SIZE 8

static void
mips_linux_n32n64_sigframe_init (const struct tramp_frame *self,
				 frame_info_ptr this_frame,
				 struct trad_frame_cache *this_cache,
				 CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int ireg;
  CORE_ADDR frame_sp = get_frame_sp (this_frame);
  CORE_ADDR sigcontext_base;
  const struct mips_regnum *regs = mips_regnum (gdbarch);

  if (self == &mips_linux_n32_rt_sigframe
      || self == &micromips_linux_n32_rt_sigframe)
    sigcontext_base = frame_sp + N32_SIGFRAME_SIGCONTEXT_OFFSET;
  else
    sigcontext_base = frame_sp + N64_SIGFRAME_SIGCONTEXT_OFFSET;

  if (mips_linux_restart_reg_p (gdbarch))
    trad_frame_set_reg_addr (this_cache,
			     (MIPS_RESTART_REGNUM
			      + gdbarch_num_regs (gdbarch)),
			     sigcontext_base + N64_SIGCONTEXT_REGS);

  for (ireg = 1; ireg < 32; ireg++)
    trad_frame_set_reg_addr (this_cache,
			     (ireg + MIPS_ZERO_REGNUM
			      + gdbarch_num_regs (gdbarch)),
			     (sigcontext_base + N64_SIGCONTEXT_REGS
			      + ireg * N64_SIGCONTEXT_REG_SIZE));

  for (ireg = 0; ireg < 32; ireg++)
    trad_frame_set_reg_addr (this_cache,
			     ireg + regs->fp0 + gdbarch_num_regs (gdbarch),
			     (sigcontext_base + N64_SIGCONTEXT_FPREGS
			      + ireg * N64_SIGCONTEXT_REG_SIZE));

  trad_frame_set_reg_addr (this_cache,
			   regs->pc + gdbarch_num_regs (gdbarch),
			   sigcontext_base + N64_SIGCONTEXT_PC);

  trad_frame_set_reg_addr (this_cache,
			   (regs->fp_control_status
			    + gdbarch_num_regs (gdbarch)),
			   sigcontext_base + N64_SIGCONTEXT_FPCSR);

  trad_frame_set_reg_addr (this_cache,
			   regs->hi + gdbarch_num_regs (gdbarch),
			   sigcontext_base + N64_SIGCONTEXT_HI);
  trad_frame_set_reg_addr (this_cache,
			   regs->lo + gdbarch_num_regs (gdbarch),
			   sigcontext_base + N64_SIGCONTEXT_LO);

  if (regs->dspacc != -1)
    {
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 0 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + N64_SIGCONTEXT_HI1);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 1 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + N64_SIGCONTEXT_LO1);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 2 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + N64_SIGCONTEXT_HI2);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 3 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + N64_SIGCONTEXT_LO2);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 4 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + N64_SIGCONTEXT_HI3);
      trad_frame_set_reg_addr (this_cache,
			       regs->dspacc + 5 + gdbarch_num_regs (gdbarch),
			       sigcontext_base + N64_SIGCONTEXT_LO3);
    }
  if (regs->dspctl != -1)
    trad_frame_set_reg_addr (this_cache,
			     regs->dspctl + gdbarch_num_regs (gdbarch),
			     sigcontext_base + N64_SIGCONTEXT_DSPCTL);

  /* Choice of the bottom of the sigframe is somewhat arbitrary.  */
  trad_frame_set_id (this_cache, frame_id_build (frame_sp, func));
}

/* Implement struct tramp_frame's "validate" method for standard MIPS code.  */

static int
mips_linux_sigframe_validate (const struct tramp_frame *self,
			      frame_info_ptr this_frame,
			      CORE_ADDR *pc)
{
  return mips_pc_is_mips (*pc);
}

/* Implement struct tramp_frame's "validate" method for microMIPS code.  */

static int
micromips_linux_sigframe_validate (const struct tramp_frame *self,
				   frame_info_ptr this_frame,
				   CORE_ADDR *pc)
{
  if (mips_pc_is_micromips (get_frame_arch (this_frame), *pc))
    {
      *pc = mips_unmake_compact_addr (*pc);
      return 1;
    }
  else
    return 0;
}

/* Implement the "write_pc" gdbarch method.  */

static void
mips_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();

  mips_write_pc (regcache, pc);

  /* Clear the syscall restart flag.  */
  if (mips_linux_restart_reg_p (gdbarch))
    regcache_cooked_write_unsigned (regcache, MIPS_RESTART_REGNUM, 0);
}

/* Return 1 if MIPS_RESTART_REGNUM is usable.  */

int
mips_linux_restart_reg_p (struct gdbarch *gdbarch)
{
  /* If we do not have a target description with registers, then
     MIPS_RESTART_REGNUM will not be included in the register set.  */
  if (!tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return 0;

  /* If we do, then MIPS_RESTART_REGNUM is safe to check; it will
     either be GPR-sized or missing.  */
  return register_size (gdbarch, MIPS_RESTART_REGNUM) > 0;
}

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
mips_linux_syscall_next_pc (frame_info_ptr frame)
{
  CORE_ADDR pc = get_frame_pc (frame);
  ULONGEST v0 = get_frame_register_unsigned (frame, MIPS_V0_REGNUM);

  /* If we are about to make a sigreturn syscall, use the unwinder to
     decode the signal frame.  */
  if (v0 == MIPS_NR_sigreturn
      || v0 == MIPS_NR_rt_sigreturn
      || v0 == MIPS_NR_N64_rt_sigreturn
      || v0 == MIPS_NR_N32_rt_sigreturn)
    return frame_unwind_caller_pc (get_current_frame ());

  return pc + 4;
}

/* Return the current system call's number present in the
   v0 register.  When the function fails, it returns -1.  */

static LONGEST
mips_linux_get_syscall_number (struct gdbarch *gdbarch,
			       thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int regsize = register_size (gdbarch, MIPS_V0_REGNUM);
  /* The content of a register */
  gdb_byte buf[8];
  /* The result */
  LONGEST ret;

  /* Make sure we're in a known ABI */
  gdb_assert (tdep->mips_abi == MIPS_ABI_O32
	      || tdep->mips_abi == MIPS_ABI_N32
	      || tdep->mips_abi == MIPS_ABI_N64);

  gdb_assert (regsize <= sizeof (buf));

  /* Getting the system call number from the register.
     syscall number is in v0 or $2.  */
  regcache->cooked_read (MIPS_V0_REGNUM, buf);

  ret = extract_signed_integer (buf, regsize, byte_order);

  return ret;
}

/* Implementation of `gdbarch_gdb_signal_to_target', as defined in
   gdbarch.h.  */

static int
mips_gdb_signal_to_target (struct gdbarch *gdbarch,
			   enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_EMT:
      return MIPS_LINUX_SIGEMT;

    case GDB_SIGNAL_BUS:
      return MIPS_LINUX_SIGBUS;

    case GDB_SIGNAL_SYS:
      return MIPS_LINUX_SIGSYS;

    case GDB_SIGNAL_USR1:
      return MIPS_LINUX_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return MIPS_LINUX_SIGUSR2;

    case GDB_SIGNAL_CHLD:
      return MIPS_LINUX_SIGCHLD;

    case GDB_SIGNAL_PWR:
      return MIPS_LINUX_SIGPWR;

    case GDB_SIGNAL_WINCH:
      return MIPS_LINUX_SIGWINCH;

    case GDB_SIGNAL_URG:
      return MIPS_LINUX_SIGURG;

    case GDB_SIGNAL_IO:
      return MIPS_LINUX_SIGIO;

    case GDB_SIGNAL_POLL:
      return MIPS_LINUX_SIGPOLL;

    case GDB_SIGNAL_STOP:
      return MIPS_LINUX_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return MIPS_LINUX_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return MIPS_LINUX_SIGCONT;

    case GDB_SIGNAL_TTIN:
      return MIPS_LINUX_SIGTTIN;

    case GDB_SIGNAL_TTOU:
      return MIPS_LINUX_SIGTTOU;

    case GDB_SIGNAL_VTALRM:
      return MIPS_LINUX_SIGVTALRM;

    case GDB_SIGNAL_PROF:
      return MIPS_LINUX_SIGPROF;

    case GDB_SIGNAL_XCPU:
      return MIPS_LINUX_SIGXCPU;

    case GDB_SIGNAL_XFSZ:
      return MIPS_LINUX_SIGXFSZ;

    /* GDB_SIGNAL_REALTIME_32 is not continuous in <gdb/signals.def>,
       therefore we have to handle it here.  */
    case GDB_SIGNAL_REALTIME_32:
      return MIPS_LINUX_SIGRTMIN;
    }

  if (signal >= GDB_SIGNAL_REALTIME_33
      && signal <= GDB_SIGNAL_REALTIME_63)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_33;

      return MIPS_LINUX_SIGRTMIN + 1 + offset;
    }
  else if (signal >= GDB_SIGNAL_REALTIME_64
	   && signal <= GDB_SIGNAL_REALTIME_127)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_64;

      return MIPS_LINUX_SIGRT64 + offset;
    }

  return linux_gdb_signal_to_target (gdbarch, signal);
}

/* Translate signals based on MIPS signal values.
   Adapted from gdb/gdbsupport/signals.c.  */

static enum gdb_signal
mips_gdb_signal_from_target (struct gdbarch *gdbarch, int signal)
{
  switch (signal)
    {
    case MIPS_LINUX_SIGEMT:
      return GDB_SIGNAL_EMT;

    case MIPS_LINUX_SIGBUS:
      return GDB_SIGNAL_BUS;

    case MIPS_LINUX_SIGSYS:
      return GDB_SIGNAL_SYS;

    case MIPS_LINUX_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case MIPS_LINUX_SIGUSR2:
      return GDB_SIGNAL_USR2;

    case MIPS_LINUX_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    case MIPS_LINUX_SIGPWR:
      return GDB_SIGNAL_PWR;

    case MIPS_LINUX_SIGWINCH:
      return GDB_SIGNAL_WINCH;

    case MIPS_LINUX_SIGURG:
      return GDB_SIGNAL_URG;

    /* No way to differentiate between SIGIO and SIGPOLL.
       Therefore, we just handle the first one.  */
    case MIPS_LINUX_SIGIO:
      return GDB_SIGNAL_IO;

    case MIPS_LINUX_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case MIPS_LINUX_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case MIPS_LINUX_SIGCONT:
      return GDB_SIGNAL_CONT;

    case MIPS_LINUX_SIGTTIN:
      return GDB_SIGNAL_TTIN;

    case MIPS_LINUX_SIGTTOU:
      return GDB_SIGNAL_TTOU;

    case MIPS_LINUX_SIGVTALRM:
      return GDB_SIGNAL_VTALRM;

    case MIPS_LINUX_SIGPROF:
      return GDB_SIGNAL_PROF;

    case MIPS_LINUX_SIGXCPU:
      return GDB_SIGNAL_XCPU;

    case MIPS_LINUX_SIGXFSZ:
      return GDB_SIGNAL_XFSZ;
    }

  if (signal >= MIPS_LINUX_SIGRTMIN && signal <= MIPS_LINUX_SIGRTMAX)
    {
      /* GDB_SIGNAL_REALTIME values are not contiguous, map parts of
	 the MIPS block to the respective GDB_SIGNAL_REALTIME blocks.  */
      int offset = signal - MIPS_LINUX_SIGRTMIN;

      if (offset == 0)
	return GDB_SIGNAL_REALTIME_32;
      else if (offset < 32)
	return (enum gdb_signal) (offset - 1
				  + (int) GDB_SIGNAL_REALTIME_33);
      else
	return (enum gdb_signal) (offset - 32
				  + (int) GDB_SIGNAL_REALTIME_64);
    }

  return linux_gdb_signal_from_target (gdbarch, signal);
}

/* Initialize one of the GNU/Linux OS ABIs.  */

static void
mips_linux_init_abi (struct gdbarch_info info,
		     struct gdbarch *gdbarch)
{
  mips_gdbarch_tdep *tdep = gdbarch_tdep<mips_gdbarch_tdep> (gdbarch);
  enum mips_abi abi = mips_abi (gdbarch);
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;

  linux_init_abi (info, gdbarch, 0);

  /* Get the syscall number from the arch's register.  */
  set_gdbarch_get_syscall_number (gdbarch, mips_linux_get_syscall_number);

  switch (abi)
    {
      case MIPS_ABI_O32:
	set_gdbarch_get_longjmp_target (gdbarch,
					mips_linux_get_longjmp_target);
	set_solib_svr4_fetch_link_map_offsets
	  (gdbarch, linux_ilp32_fetch_link_map_offsets);
	tramp_frame_prepend_unwinder (gdbarch, &micromips_linux_o32_sigframe);
	tramp_frame_prepend_unwinder (gdbarch,
				      &micromips_linux_o32_rt_sigframe);
	tramp_frame_prepend_unwinder (gdbarch, &mips_linux_o32_sigframe);
	tramp_frame_prepend_unwinder (gdbarch, &mips_linux_o32_rt_sigframe);
	set_xml_syscall_file_name (gdbarch, "syscalls/mips-o32-linux.xml");
	break;
      case MIPS_ABI_N32:
	set_gdbarch_get_longjmp_target (gdbarch,
					mips_linux_get_longjmp_target);
	set_solib_svr4_fetch_link_map_offsets
	  (gdbarch, linux_ilp32_fetch_link_map_offsets);
	set_gdbarch_long_double_bit (gdbarch, 128);
	set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);
	tramp_frame_prepend_unwinder (gdbarch,
				      &micromips_linux_n32_rt_sigframe);
	tramp_frame_prepend_unwinder (gdbarch, &mips_linux_n32_rt_sigframe);
	set_xml_syscall_file_name (gdbarch, "syscalls/mips-n32-linux.xml");
	break;
      case MIPS_ABI_N64:
	set_gdbarch_get_longjmp_target (gdbarch,
					mips64_linux_get_longjmp_target);
	set_solib_svr4_fetch_link_map_offsets
	  (gdbarch, linux_lp64_fetch_link_map_offsets);
	set_gdbarch_long_double_bit (gdbarch, 128);
	set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);
	tramp_frame_prepend_unwinder (gdbarch,
				      &micromips_linux_n64_rt_sigframe);
	tramp_frame_prepend_unwinder (gdbarch, &mips_linux_n64_rt_sigframe);
	set_xml_syscall_file_name (gdbarch, "syscalls/mips-n64-linux.xml");
	break;
      default:
	break;
    }

  set_gdbarch_skip_solib_resolver (gdbarch, mips_linux_skip_resolver);

  set_gdbarch_software_single_step (gdbarch, mips_software_single_step);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Initialize this lazily, to avoid an initialization order
     dependency on solib-svr4.c's _initialize routine.  */
  if (mips_svr4_so_ops.in_dynsym_resolve_code == NULL)
    {
      mips_svr4_so_ops = svr4_so_ops;
      mips_svr4_so_ops.in_dynsym_resolve_code
	= mips_linux_in_dynsym_resolve_code;
    }
  set_gdbarch_so_ops (gdbarch, &mips_svr4_so_ops);

  set_gdbarch_write_pc (gdbarch, mips_linux_write_pc);

  set_gdbarch_core_read_description (gdbarch,
				     mips_linux_core_read_description);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, mips_linux_iterate_over_regset_sections);

  set_gdbarch_gdb_signal_from_target (gdbarch,
				      mips_gdb_signal_from_target);

  set_gdbarch_gdb_signal_to_target (gdbarch,
				    mips_gdb_signal_to_target);

  tdep->syscall_next_pc = mips_linux_syscall_next_pc;

  if (tdesc_data)
    {
      const struct tdesc_feature *feature;

      /* If we have target-described registers, then we can safely
	 reserve a number for MIPS_RESTART_REGNUM (whether it is
	 described or not).  */
      gdb_assert (gdbarch_num_regs (gdbarch) <= MIPS_RESTART_REGNUM);
      set_gdbarch_num_regs (gdbarch, MIPS_RESTART_REGNUM + 1);
      set_gdbarch_num_pseudo_regs (gdbarch, MIPS_RESTART_REGNUM + 1);

      /* If it's present, then assign it to the reserved number.  */
      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.linux");
      if (feature != NULL)
	tdesc_numbered_register (feature, tdesc_data, MIPS_RESTART_REGNUM,
				 "restart");
    }
}

void _initialize_mips_linux_tdep ();
void
_initialize_mips_linux_tdep ()
{
  const struct bfd_arch_info *arch_info;

  for (arch_info = bfd_lookup_arch (bfd_arch_mips, 0);
       arch_info != NULL;
       arch_info = arch_info->next)
    {
      gdbarch_register_osabi (bfd_arch_mips, arch_info->mach,
			      GDB_OSABI_LINUX,
			      mips_linux_init_abi);
    }

  /* Initialize the standard target descriptions.  */
  initialize_tdesc_mips_linux ();
  initialize_tdesc_mips_dsp_linux ();
  initialize_tdesc_mips64_linux ();
  initialize_tdesc_mips64_dsp_linux ();
}

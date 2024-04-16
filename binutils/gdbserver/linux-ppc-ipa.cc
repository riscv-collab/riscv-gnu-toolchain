/* GNU/Linux/PowerPC specific low level interface, for the in-process
   agent library for GDB.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include <sys/mman.h>
#include "tracepoint.h"
#include "arch/ppc-linux-tdesc.h"
#include "linux-ppc-tdesc-init.h"
#include <elf.h>
#ifdef HAVE_GETAUXVAL
#include <sys/auxv.h>
#endif

/* These macros define the position of registers in the buffer collected
   by the fast tracepoint jump pad.  */
#define FT_CR_R0	0
#define FT_CR_CR	32
#define FT_CR_XER	33
#define FT_CR_LR	34
#define FT_CR_CTR	35
#define FT_CR_PC	36
#define FT_CR_GPR(n)	(FT_CR_R0 + (n))

static const int ppc_ft_collect_regmap[] = {
  /* GPRs */
  FT_CR_GPR (0), FT_CR_GPR (1), FT_CR_GPR (2),
  FT_CR_GPR (3), FT_CR_GPR (4), FT_CR_GPR (5),
  FT_CR_GPR (6), FT_CR_GPR (7), FT_CR_GPR (8),
  FT_CR_GPR (9), FT_CR_GPR (10), FT_CR_GPR (11),
  FT_CR_GPR (12), FT_CR_GPR (13), FT_CR_GPR (14),
  FT_CR_GPR (15), FT_CR_GPR (16), FT_CR_GPR (17),
  FT_CR_GPR (18), FT_CR_GPR (19), FT_CR_GPR (20),
  FT_CR_GPR (21), FT_CR_GPR (22), FT_CR_GPR (23),
  FT_CR_GPR (24), FT_CR_GPR (25), FT_CR_GPR (26),
  FT_CR_GPR (27), FT_CR_GPR (28), FT_CR_GPR (29),
  FT_CR_GPR (30), FT_CR_GPR (31),
  /* FPRs - not collected.  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  FT_CR_PC, /* PC */
  -1, /* MSR */
  FT_CR_CR, /* CR */
  FT_CR_LR, /* LR */
  FT_CR_CTR, /* CTR */
  FT_CR_XER, /* XER */
  -1, /* FPSCR */
};

#define PPC_NUM_FT_COLLECT_GREGS \
  (sizeof (ppc_ft_collect_regmap) / sizeof(ppc_ft_collect_regmap[0]))

/* Supply registers collected by the fast tracepoint jump pad.
   BUF is the second argument we pass to gdb_collect in jump pad.  */

void
supply_fast_tracepoint_registers (struct regcache *regcache,
				  const unsigned char *buf)
{
  int i;

  for (i = 0; i < PPC_NUM_FT_COLLECT_GREGS; i++)
    {
      if (ppc_ft_collect_regmap[i] == -1)
	continue;
      supply_register (regcache, i,
		       ((char *) buf)
			+ ppc_ft_collect_regmap[i] * sizeof (long));
    }
}

/* Return the value of register REGNUM.  RAW_REGS is collected buffer
   by jump pad.  This function is called by emit_reg.  */

ULONGEST
get_raw_reg (const unsigned char *raw_regs, int regnum)
{
  if (regnum >= PPC_NUM_FT_COLLECT_GREGS)
    return 0;
  if (ppc_ft_collect_regmap[regnum] == -1)
    return 0;

  return *(unsigned long *) (raw_regs
			     + ppc_ft_collect_regmap[regnum] * sizeof (long));
}

/* Allocate buffer for the jump pads.  The branch instruction has a reach
   of +/- 32MiB, and the executable is loaded at 0x10000000 (256MiB).

   64-bit: To maximize the area of executable that can use tracepoints,
   try allocating at 0x10000000 - size initially, decreasing until we hit
   a free area.

   32-bit: ld.so loads dynamic libraries right below the executable, so
   we cannot depend on that area (dynamic libraries can be quite large).
   Instead, aim right after the executable - at sbrk(0).  This will
   cause future brk to fail, and malloc will fallback to mmap.  */

void *
alloc_jump_pad_buffer (size_t size)
{
#ifdef __powerpc64__
  uintptr_t addr;
  uintptr_t exec_base = getauxval (AT_PHDR);
  int pagesize;
  void *res;

  if (exec_base == 0)
    exec_base = 0x10000000;

  pagesize = sysconf (_SC_PAGE_SIZE);
  if (pagesize == -1)
    perror_with_name ("sysconf");

  addr = exec_base - size;

  /* size should already be page-aligned, but this can't hurt.  */
  addr &= ~(pagesize - 1);

  /* Search for a free area.  If we hit 0, we're out of luck.  */
  for (; addr; addr -= pagesize)
    {
      /* No MAP_FIXED - we don't want to zap someone's mapping.  */
      res = mmap ((void *) addr, size,
		  PROT_READ | PROT_WRITE | PROT_EXEC,
		  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

      /* If we got what we wanted, return.  */
      if ((uintptr_t) res == addr)
	return res;

      /* If we got a mapping, but at a wrong address, undo it.  */
      if (res != MAP_FAILED)
	munmap (res, size);
    }

  return NULL;
#else
  void *target = sbrk (0);
  void *res = mmap (target, size, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (res == target)
    return res;

  if (res != MAP_FAILED)
    munmap (res, size);

  return NULL;
#endif
}

/* Return target_desc to use for IPA, given the tdesc index passed by
   gdbserver.  */

const struct target_desc *
get_ipa_tdesc (int idx)
{
  switch (idx)
    {
#ifdef __powerpc64__
    case PPC_TDESC_BASE:
      return tdesc_powerpc_64l;
    case PPC_TDESC_ALTIVEC:
      return tdesc_powerpc_altivec64l;
    case PPC_TDESC_VSX:
      return tdesc_powerpc_vsx64l;
    case PPC_TDESC_ISA205:
      return tdesc_powerpc_isa205_64l;
    case PPC_TDESC_ISA205_ALTIVEC:
      return tdesc_powerpc_isa205_altivec64l;
    case PPC_TDESC_ISA205_VSX:
      return tdesc_powerpc_isa205_vsx64l;
    case PPC_TDESC_ISA205_PPR_DSCR_VSX:
      return tdesc_powerpc_isa205_ppr_dscr_vsx64l;
    case PPC_TDESC_ISA207_VSX:
      return tdesc_powerpc_isa207_vsx64l;
    case PPC_TDESC_ISA207_HTM_VSX:
      return tdesc_powerpc_isa207_htm_vsx64l;
#else
    case PPC_TDESC_BASE:
      return tdesc_powerpc_32l;
    case PPC_TDESC_ALTIVEC:
      return tdesc_powerpc_altivec32l;
    case PPC_TDESC_VSX:
      return tdesc_powerpc_vsx32l;
    case PPC_TDESC_ISA205:
      return tdesc_powerpc_isa205_32l;
    case PPC_TDESC_ISA205_ALTIVEC:
      return tdesc_powerpc_isa205_altivec32l;
    case PPC_TDESC_ISA205_VSX:
      return tdesc_powerpc_isa205_vsx32l;
    case PPC_TDESC_ISA205_PPR_DSCR_VSX:
      return tdesc_powerpc_isa205_ppr_dscr_vsx32l;
    case PPC_TDESC_ISA207_VSX:
      return tdesc_powerpc_isa207_vsx32l;
    case PPC_TDESC_ISA207_HTM_VSX:
      return tdesc_powerpc_isa207_htm_vsx32l;
    case PPC_TDESC_E500:
      return tdesc_powerpc_e500l;
#endif
    default:
      internal_error ("unknown ipa tdesc index: %d", idx);
#ifdef __powerpc64__
      return tdesc_powerpc_64l;
#else
      return tdesc_powerpc_32l;
#endif
    }
}


/* Initialize ipa_tdesc and others.  */

void
initialize_low_tracepoint (void)
{
#ifdef __powerpc64__
  init_registers_powerpc_64l ();
  init_registers_powerpc_altivec64l ();
  init_registers_powerpc_vsx64l ();
  init_registers_powerpc_isa205_64l ();
  init_registers_powerpc_isa205_altivec64l ();
  init_registers_powerpc_isa205_vsx64l ();
  init_registers_powerpc_isa205_ppr_dscr_vsx64l ();
  init_registers_powerpc_isa207_vsx64l ();
  init_registers_powerpc_isa207_htm_vsx64l ();
#else
  init_registers_powerpc_32l ();
  init_registers_powerpc_altivec32l ();
  init_registers_powerpc_vsx32l ();
  init_registers_powerpc_isa205_32l ();
  init_registers_powerpc_isa205_altivec32l ();
  init_registers_powerpc_isa205_vsx32l ();
  init_registers_powerpc_isa205_ppr_dscr_vsx32l ();
  init_registers_powerpc_isa207_vsx32l ();
  init_registers_powerpc_isa207_htm_vsx32l ();
  init_registers_powerpc_e500l ();
#endif
}

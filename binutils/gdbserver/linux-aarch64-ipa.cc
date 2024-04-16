/* GNU/Linux/AArch64 specific low level interface, for the in-process
   agent library for GDB.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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
#include <elf.h>
#ifdef HAVE_GETAUXVAL
#include <sys/auxv.h>
#endif
#include "linux-aarch64-tdesc.h"

/* Each register saved by the jump pad is in a 16 byte cell.  */
#define FT_CR_SIZE 16

#define FT_CR_FPCR	0
#define FT_CR_FPSR	1
#define FT_CR_CPSR	2
#define FT_CR_PC	3
#define FT_CR_SP	4
#define FT_CR_X0	5
#define FT_CR_GPR(n)	(FT_CR_X0 + (n))
#define FT_CR_FPR(n)	(FT_CR_GPR (31) + (n))

/* Mapping between registers collected by the jump pad and GDB's register
   array layout used by regcache.

   See linux-aarch64-low.c (aarch64_install_fast_tracepoint_jump_pad) for
   more details.  */

static const int aarch64_ft_collect_regmap[] = {
  FT_CR_GPR (0),
  FT_CR_GPR (1),
  FT_CR_GPR (2),
  FT_CR_GPR (3),
  FT_CR_GPR (4),
  FT_CR_GPR (5),
  FT_CR_GPR (6),
  FT_CR_GPR (7),
  FT_CR_GPR (8),
  FT_CR_GPR (9),
  FT_CR_GPR (10),
  FT_CR_GPR (11),
  FT_CR_GPR (12),
  FT_CR_GPR (13),
  FT_CR_GPR (14),
  FT_CR_GPR (15),
  FT_CR_GPR (16),
  FT_CR_GPR (17),
  FT_CR_GPR (18),
  FT_CR_GPR (19),
  FT_CR_GPR (20),
  FT_CR_GPR (21),
  FT_CR_GPR (22),
  FT_CR_GPR (23),
  FT_CR_GPR (24),
  FT_CR_GPR (25),
  FT_CR_GPR (26),
  FT_CR_GPR (27),
  FT_CR_GPR (28),
  /* FP */
  FT_CR_GPR (29),
  /* LR */
  FT_CR_GPR (30),
  FT_CR_SP,
  FT_CR_PC,
  FT_CR_CPSR,
  FT_CR_FPR (0),
  FT_CR_FPR (1),
  FT_CR_FPR (2),
  FT_CR_FPR (3),
  FT_CR_FPR (4),
  FT_CR_FPR (5),
  FT_CR_FPR (6),
  FT_CR_FPR (7),
  FT_CR_FPR (8),
  FT_CR_FPR (9),
  FT_CR_FPR (10),
  FT_CR_FPR (11),
  FT_CR_FPR (12),
  FT_CR_FPR (13),
  FT_CR_FPR (14),
  FT_CR_FPR (15),
  FT_CR_FPR (16),
  FT_CR_FPR (17),
  FT_CR_FPR (18),
  FT_CR_FPR (19),
  FT_CR_FPR (20),
  FT_CR_FPR (21),
  FT_CR_FPR (22),
  FT_CR_FPR (23),
  FT_CR_FPR (24),
  FT_CR_FPR (25),
  FT_CR_FPR (26),
  FT_CR_FPR (27),
  FT_CR_FPR (28),
  FT_CR_FPR (29),
  FT_CR_FPR (30),
  FT_CR_FPR (31),
  FT_CR_FPSR,
  FT_CR_FPCR
};

#define AARCH64_NUM_FT_COLLECT_GREGS \
  (sizeof (aarch64_ft_collect_regmap) / sizeof(aarch64_ft_collect_regmap[0]))

/* Fill in REGCACHE with registers saved by the jump pad in BUF.  */

void
supply_fast_tracepoint_registers (struct regcache *regcache,
				  const unsigned char *buf)
{
  int i;

  for (i = 0; i < AARCH64_NUM_FT_COLLECT_GREGS; i++)
    supply_register (regcache, i,
		     ((char *) buf)
		     + (aarch64_ft_collect_regmap[i] * FT_CR_SIZE));
}

ULONGEST
get_raw_reg (const unsigned char *raw_regs, int regnum)
{
  if (regnum >= AARCH64_NUM_FT_COLLECT_GREGS)
    return 0;

  return *(ULONGEST *) (raw_regs
			+ aarch64_ft_collect_regmap[regnum] * FT_CR_SIZE);
}

/* Return target_desc to use for IPA, given the tdesc index passed by
   gdbserver.  Index is ignored, since we have only one tdesc
   at the moment.  SVE, pauth, MTE and TLS not yet supported.  */

const struct target_desc *
get_ipa_tdesc (int idx)
{
  return aarch64_linux_read_description ({});
}

/* Allocate buffer for the jump pads.  The branch instruction has a reach
   of +/- 128MiB, and the executable is loaded at 0x400000 (4MiB).
   To maximize the area of executable that can use tracepoints, try
   allocating at 0x400000 - size initially, decreasing until we hit
   a free area.  */

void *
alloc_jump_pad_buffer (size_t size)
{
  uintptr_t addr;
  uintptr_t exec_base = getauxval (AT_PHDR);
  int pagesize;
  void *res;

  if (exec_base == 0)
    exec_base = 0x400000;

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
}

void
initialize_low_tracepoint (void)
{
  /* SVE, pauth, MTE and TLS not yet supported.  */
  aarch64_linux_read_description ({});
}

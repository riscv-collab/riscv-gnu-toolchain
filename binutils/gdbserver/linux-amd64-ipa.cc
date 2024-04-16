/* GNU/Linux/x86-64 specific low level interface, for the in-process
   agent library for GDB.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "linux-x86-tdesc.h"
#include "gdbsupport/x86-xstate.h"

/* Defined in auto-generated file amd64-linux.c.  */
void init_registers_amd64_linux (void);
extern const struct target_desc *tdesc_amd64_linux;

/* fast tracepoints collect registers.  */

#define FT_CR_RIP 0
#define FT_CR_EFLAGS 1
#define FT_CR_R8 2
#define FT_CR_R9 3
#define FT_CR_R10 4
#define FT_CR_R11 5
#define FT_CR_R12 6
#define FT_CR_R13 7
#define FT_CR_R14 8
#define FT_CR_R15 9
#define FT_CR_RAX 10
#define FT_CR_RBX 11
#define FT_CR_RCX 12
#define FT_CR_RDX 13
#define FT_CR_RSI 14
#define FT_CR_RDI 15
#define FT_CR_RBP 16
#define FT_CR_RSP 17

static const int x86_64_ft_collect_regmap[] = {
  FT_CR_RAX * 8, FT_CR_RBX * 8, FT_CR_RCX * 8, FT_CR_RDX * 8,
  FT_CR_RSI * 8, FT_CR_RDI * 8, FT_CR_RBP * 8, FT_CR_RSP * 8,
  FT_CR_R8 * 8,  FT_CR_R9 * 8,  FT_CR_R10 * 8, FT_CR_R11 * 8,
  FT_CR_R12 * 8, FT_CR_R13 * 8, FT_CR_R14 * 8, FT_CR_R15 * 8,
  FT_CR_RIP * 8, FT_CR_EFLAGS * 8
};

#define X86_64_NUM_FT_COLLECT_GREGS \
  (sizeof (x86_64_ft_collect_regmap) / sizeof(x86_64_ft_collect_regmap[0]))

void
supply_fast_tracepoint_registers (struct regcache *regcache,
				  const unsigned char *buf)
{
  int i;

  for (i = 0; i < X86_64_NUM_FT_COLLECT_GREGS; i++)
    supply_register (regcache, i,
		     ((char *) buf) + x86_64_ft_collect_regmap[i]);
}

ULONGEST
get_raw_reg (const unsigned char *raw_regs, int regnum)
{
  if (regnum >= X86_64_NUM_FT_COLLECT_GREGS)
    return 0;

  return *(ULONGEST *) (raw_regs + x86_64_ft_collect_regmap[regnum]);
}

#ifdef HAVE_UST

#include <ust/processor.h>

/* "struct registers" is the UST object type holding the registers at
   the time of the static tracepoint marker call.  This doesn't
   contain RIP, but we know what it must have been (the marker
   address).  */

#define ST_REGENTRY(REG)			\
  {						\
    offsetof (struct registers, REG),		\
    sizeof (((struct registers *) NULL)->REG)	\
  }

static struct
{
  int offset;
  int size;
} x86_64_st_collect_regmap[] =
  {
    ST_REGENTRY(rax),
    ST_REGENTRY(rbx),
    ST_REGENTRY(rcx),
    ST_REGENTRY(rdx),
    ST_REGENTRY(rsi),
    ST_REGENTRY(rdi),
    ST_REGENTRY(rbp),
    ST_REGENTRY(rsp),
    ST_REGENTRY(r8),
    ST_REGENTRY(r9),
    ST_REGENTRY(r10),
    ST_REGENTRY(r11),
    ST_REGENTRY(r12),
    ST_REGENTRY(r13),
    ST_REGENTRY(r14),
    ST_REGENTRY(r15),
    { -1, 0 },
    ST_REGENTRY(rflags),
    ST_REGENTRY(cs),
    ST_REGENTRY(ss),
  };

#define X86_64_NUM_ST_COLLECT_GREGS \
  (sizeof (x86_64_st_collect_regmap) / sizeof (x86_64_st_collect_regmap[0]))

/* GDB's RIP register number.  */
#define AMD64_RIP_REGNUM 16

void
supply_static_tracepoint_registers (struct regcache *regcache,
				    const unsigned char *buf,
				    CORE_ADDR pc)
{
  int i;
  unsigned long newpc = pc;

  supply_register (regcache, AMD64_RIP_REGNUM, &newpc);

  for (i = 0; i < X86_64_NUM_ST_COLLECT_GREGS; i++)
    if (x86_64_st_collect_regmap[i].offset != -1)
      {
	switch (x86_64_st_collect_regmap[i].size)
	  {
	  case 8:
	    supply_register (regcache, i,
			     ((char *) buf)
			     + x86_64_st_collect_regmap[i].offset);
	    break;
	  case 2:
	    {
	      unsigned long reg
		= * (short *) (((char *) buf)
			       + x86_64_st_collect_regmap[i].offset);
	      reg &= 0xffff;
	      supply_register (regcache, i, &reg);
	    }
	    break;
	  default:
	    internal_error ("unhandled register size: %d",
			    x86_64_st_collect_regmap[i].size);
	    break;
	  }
      }
}

#endif /* HAVE_UST */

#if !defined __ILP32__
/* Map the tdesc index to xcr0 mask.  */
static uint64_t idx2mask[X86_TDESC_LAST] = {
  X86_XSTATE_X87_MASK,
  X86_XSTATE_SSE_MASK,
  X86_XSTATE_AVX_MASK,
  X86_XSTATE_MPX_MASK,
  X86_XSTATE_AVX_MPX_MASK,
  X86_XSTATE_AVX_AVX512_MASK,
  X86_XSTATE_AVX_MPX_AVX512_PKU_MASK,
};
#endif

/* Return target_desc to use for IPA, given the tdesc index passed by
   gdbserver.  */

const struct target_desc *
get_ipa_tdesc (int idx)
{
  if (idx >= X86_TDESC_LAST)
    {
      internal_error ("unknown ipa tdesc index: %d", idx);
    }

#if defined __ILP32__
  switch (idx)
    {
    case X86_TDESC_SSE:
      return amd64_linux_read_description (X86_XSTATE_SSE_MASK, true);
    case X86_TDESC_AVX:
      return amd64_linux_read_description (X86_XSTATE_AVX_MASK, true);
    case X86_TDESC_AVX_AVX512:
      return amd64_linux_read_description (X86_XSTATE_AVX_AVX512_MASK, true);
    default:
      break;
    }
#else
  return amd64_linux_read_description (idx2mask[idx], false);
#endif

  internal_error ("unknown ipa tdesc index: %d", idx);
}

/* Allocate buffer for the jump pads.  The branch instruction has a
   reach of +/- 31-bit, and the executable is loaded at low addresses.

   64-bit: Use MAP_32BIT to allocate in the first 2GB.  Shared
   libraries, being allocated at the top, are unfortunately out of
   luck.

   x32: Since MAP_32BIT is 64-bit only, do the placement manually.
   Try allocating at '0x80000000 - SIZE' initially, decreasing until
   we hit a free area.  This ensures the executable is fully covered,
   and is as close as possible to the shared libraries, which are
   usually mapped at the top of the first 4GB of the address space.
*/

void *
alloc_jump_pad_buffer (size_t size)
{
#if __ILP32__
  uintptr_t addr;
  int pagesize;

  pagesize = sysconf (_SC_PAGE_SIZE);
  if (pagesize == -1)
    perror_with_name ("sysconf");

  addr = 0x80000000 - size;

  /* size should already be page-aligned, but this can't hurt.  */
  addr &= ~(pagesize - 1);

  /* Search for a free area.  If we hit 0, we're out of luck.  */
  for (; addr; addr -= pagesize)
    {
      void *res;

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
  void *res = mmap (NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
		    MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);

  if (res == MAP_FAILED)
    return NULL;

  return res;
#endif
}

void
initialize_low_tracepoint (void)
{
#if defined __ILP32__
  amd64_linux_read_description (X86_XSTATE_SSE_MASK, true);
  amd64_linux_read_description (X86_XSTATE_AVX_MASK, true);
  amd64_linux_read_description (X86_XSTATE_AVX_AVX512_MASK, true);
#else
  for (auto i = 0; i < X86_TDESC_LAST; i++)
    amd64_linux_read_description (idx2mask[i], false);
#endif
}

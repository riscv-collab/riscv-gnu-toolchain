/* Copyright (C) 1995-2024 Free Software Foundation, Inc.

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
#include "arch/arm.h"
#include "arch/arm-linux.h"
#include "linux-low.h"
#include "linux-aarch32-low.h"

#include <sys/ptrace.h>
/* Don't include elf.h if linux/elf.h got included by gdb_proc_service.h.
   On Bionic elf.h and linux/elf.h have conflicting definitions.  */
#ifndef ELFMAG0
#include <elf.h>
#endif

/* Correct in either endianness.  */
#define arm_abi_breakpoint 0xef9f0001UL

/* For new EABI binaries.  We recognize it regardless of which ABI
   is used for gdbserver, so single threaded debugging should work
   OK, but for multi-threaded debugging we only insert the current
   ABI's breakpoint instruction.  For now at least.  */
#define arm_eabi_breakpoint 0xe7f001f0UL

#if (defined __ARM_EABI__ || defined __aarch64__)
static const unsigned long arm_breakpoint = arm_eabi_breakpoint;
#else
static const unsigned long arm_breakpoint = arm_abi_breakpoint;
#endif

#define arm_breakpoint_len 4
static const unsigned short thumb_breakpoint = 0xde01;
#define thumb_breakpoint_len 2
static const unsigned short thumb2_breakpoint[] = { 0xf7f0, 0xa000 };
#define thumb2_breakpoint_len 4

/* Some older versions of GNU/Linux and Android do not define
   the following macros.  */
#ifndef NT_ARM_VFP
#define NT_ARM_VFP 0x400
#endif

/* Collect GP registers from REGCACHE to buffer BUF.  */

void
arm_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;
  uint32_t *regs = (uint32_t *) buf;
  uint32_t cpsr = regs[ARM_CPSR_GREGNUM];

  for (i = ARM_A1_REGNUM; i <= ARM_PC_REGNUM; i++)
    collect_register (regcache, i, &regs[i]);

  collect_register (regcache, ARM_PS_REGNUM, &regs[ARM_CPSR_GREGNUM]);
  /* Keep reserved bits bit 20 to bit 23.  */
  regs[ARM_CPSR_GREGNUM] = ((regs[ARM_CPSR_GREGNUM] & 0xff0fffff)
			    | (cpsr & 0x00f00000));
}

/* Supply GP registers contents, stored in BUF, to REGCACHE.  */

void
arm_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;
  char zerobuf[8];
  const uint32_t *regs = (const uint32_t *) buf;
  uint32_t cpsr = regs[ARM_CPSR_GREGNUM];

  memset (zerobuf, 0, 8);
  for (i = ARM_A1_REGNUM; i <= ARM_PC_REGNUM; i++)
    supply_register (regcache, i, &regs[i]);

  for (; i < ARM_PS_REGNUM; i++)
    supply_register (regcache, i, zerobuf);

  /* Clear reserved bits bit 20 to bit 23.  */
  cpsr &= 0xff0fffff;
  supply_register (regcache, ARM_PS_REGNUM, &cpsr);
}

/* Collect NUM number of VFP registers from REGCACHE to buffer BUF.  */

void
arm_fill_vfpregset_num (struct regcache *regcache, void *buf, int num)
{
  int i, base;

  gdb_assert (num == 16 || num == 32);

  base = find_regno (regcache->tdesc, "d0");
  for (i = 0; i < num; i++)
    collect_register (regcache, base + i, (char *) buf + i * 8);

  collect_register_by_name (regcache, "fpscr", (char *) buf + 32 * 8);
}

/* Supply NUM number of VFP registers contents, stored in BUF, to
   REGCACHE.  */

void
arm_store_vfpregset_num (struct regcache *regcache, const void *buf, int num)
{
  int i, base;

  gdb_assert (num == 16 || num == 32);

  base = find_regno (regcache->tdesc, "d0");
  for (i = 0; i < num; i++)
    supply_register (regcache, base + i, (char *) buf + i * 8);

  supply_register_by_name (regcache, "fpscr", (char *) buf + 32 * 8);
}

static void
arm_fill_vfpregset (struct regcache *regcache, void *buf)
{
  arm_fill_vfpregset_num (regcache, buf, 32);
}

static void
arm_store_vfpregset (struct regcache *regcache, const void *buf)
{
  arm_store_vfpregset_num (regcache, buf, 32);
}

/* Register sets with using PTRACE_GETREGSET.  */

static struct regset_info aarch32_regsets[] = {
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS,
    ARM_CORE_REGS_SIZE + ARM_INT_REGISTER_SIZE, GENERAL_REGS,
    arm_fill_gregset, arm_store_gregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_VFP, ARM_VFP3_REGS_SIZE,
    EXTENDED_REGS,
    arm_fill_vfpregset, arm_store_vfpregset },
  NULL_REGSET
};

static struct regsets_info aarch32_regsets_info =
  {
    aarch32_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

struct regs_info regs_info_aarch32 =
  {
    NULL, /* regset_bitmap */
    NULL, /* usrregs */
    &aarch32_regsets_info
  };

/* Returns 1 if the current instruction set is thumb, 0 otherwise.  */

int
arm_is_thumb_mode (void)
{
  struct regcache *regcache = get_thread_regcache (current_thread, 1);
  unsigned long cpsr;

  collect_register_by_name (regcache, "cpsr", &cpsr);

  if (cpsr & 0x20)
    return 1;
  else
    return 0;
}

/* Returns 1 if there is a software breakpoint at location.  */

int
arm_breakpoint_at (CORE_ADDR where)
{
  if (arm_is_thumb_mode ())
    {
      /* Thumb mode.  */
      unsigned short insn;

      the_target->read_memory (where, (unsigned char *) &insn, 2);
      if (insn == thumb_breakpoint)
	return 1;

      if (insn == thumb2_breakpoint[0])
	{
	  the_target->read_memory (where + 2, (unsigned char *) &insn, 2);
	  if (insn == thumb2_breakpoint[1])
	    return 1;
	}
    }
  else
    {
      /* ARM mode.  */
      unsigned long insn;

      the_target->read_memory (where, (unsigned char *) &insn, 4);
      if (insn == arm_abi_breakpoint)
	return 1;

      if (insn == arm_eabi_breakpoint)
	return 1;
    }

  return 0;
}

/* Implementation of linux_target_ops method "breakpoint_kind_from_pc".

   Determine the type and size of breakpoint to insert at PCPTR.  Uses the
   program counter value to determine whether a 16-bit or 32-bit breakpoint
   should be used.  It returns the breakpoint's kind, and adjusts the program
   counter (if necessary) to point to the actual memory location where the
   breakpoint should be inserted.  */

int
arm_breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  if (IS_THUMB_ADDR (*pcptr))
    {
      gdb_byte buf[2];

      *pcptr = UNMAKE_THUMB_ADDR (*pcptr);

      /* Check whether we are replacing a thumb2 32-bit instruction.  */
      if (target_read_memory (*pcptr, buf, 2) == 0)
	{
	  unsigned short inst1 = 0;

	  target_read_memory (*pcptr, (gdb_byte *) &inst1, 2);
	  if (thumb_insn_size (inst1) == 4)
	    return ARM_BP_KIND_THUMB2;
	}
      return ARM_BP_KIND_THUMB;
    }
  else
    return ARM_BP_KIND_ARM;
}

/*  Implementation of the linux_target_ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
arm_sw_breakpoint_from_kind (int kind , int *size)
{
  *size = arm_breakpoint_len;
  /* Define an ARM-mode breakpoint; we only set breakpoints in the C
     library, which is most likely to be ARM.  If the kernel supports
     clone events, we will never insert a breakpoint, so even a Thumb
     C library will work; so will mixing EABI/non-EABI gdbserver and
     application.  */
  switch (kind)
    {
      case ARM_BP_KIND_THUMB:
	*size = thumb_breakpoint_len;
	return (gdb_byte *) &thumb_breakpoint;
      case ARM_BP_KIND_THUMB2:
	*size = thumb2_breakpoint_len;
	return (gdb_byte *) &thumb2_breakpoint;
      case ARM_BP_KIND_ARM:
	*size = arm_breakpoint_len;
	return (const gdb_byte *) &arm_breakpoint;
      default:
       return NULL;
    }
  return NULL;
}

/* Implementation of the linux_target_ops method
   "breakpoint_kind_from_current_state".  */

int
arm_breakpoint_kind_from_current_state (CORE_ADDR *pcptr)
{
  if (arm_is_thumb_mode ())
    {
      *pcptr = MAKE_THUMB_ADDR (*pcptr);
      return arm_breakpoint_kind_from_pc (pcptr);
    }
  else
    {
      return arm_breakpoint_kind_from_pc (pcptr);
    }
}

void
initialize_low_arch_aarch32 (void)
{
  initialize_regsets_info (&aarch32_regsets_info);
}

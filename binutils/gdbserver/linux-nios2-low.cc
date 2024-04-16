/* GNU/Linux/Nios II specific low level interface, for the remote server for
   GDB.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "linux-low.h"
#include "elf/common.h"
#include "nat/gdb_ptrace.h"
#include <endian.h>
#include "gdb_proc_service.h"
#include <asm/ptrace.h>

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

/* Linux target op definitions for the NIOS II architecture.  */

class nios2_target : public linux_process_target
{
public:

  const regs_info *get_regs_info () override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

protected:

  void low_arch_setup () override;

  bool low_cannot_fetch_register (int regno) override;

  bool low_cannot_store_register (int regno) override;

  bool low_supports_breakpoints () override;

  CORE_ADDR low_get_pc (regcache *regcache) override;

  void low_set_pc (regcache *regcache, CORE_ADDR newpc) override;

  bool low_breakpoint_at (CORE_ADDR pc) override;
};

/* The singleton target ops object.  */

static nios2_target the_nios2_target;

bool
nios2_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
nios2_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_32bit (regcache);
}

void
nios2_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  linux_set_pc_32bit (regcache, pc);
}

/* The following definition must agree with the number of registers
   defined in "struct user_regs" in GLIBC
   (sysdeps/unix/sysv/linux/nios2/sys/user.h), and also with
   NIOS2_NUM_REGS in GDB proper.  */

#define nios2_num_regs 49

/* Defined in auto-generated file nios2-linux.c.  */

void init_registers_nios2_linux (void);
extern const struct target_desc *tdesc_nios2_linux;

/* This union is used to convert between int and byte buffer
   representations of register contents.  */

union nios2_register
{
  unsigned char buf[4];
  int reg32;
};

/* Return the ptrace ``address'' of register REGNO. */

static int nios2_regmap[] = {
  -1,  1,  2,  3,  4,  5,  6,  7,
  8,  9,  10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47,
  48,
  0
};

/* Implement the low_arch_setup linux target ops method.  */

void
nios2_target::low_arch_setup ()
{
  current_process ()->tdesc = tdesc_nios2_linux;
}

/* Implement the low_cannot_fetch_register linux target ops method.  */

bool
nios2_target::low_cannot_fetch_register (int regno)
{
  return (nios2_regmap[regno] == -1);
}

/* Implement the low_cannot_store_register linux target ops method.  */

bool
nios2_target::low_cannot_store_register (int regno)
{
  return (nios2_regmap[regno] == -1);
}

/* Breakpoint support.  Also see comments on nios2_breakpoint_from_pc
   in nios2-tdep.c.  */

#if defined(__nios2_arch__) && __nios2_arch__ == 2
#define NIOS2_BREAKPOINT 0xb7fd0020
#define CDX_BREAKPOINT 0xd7c9
#else
#define NIOS2_BREAKPOINT 0x003b6ffa
#endif

/* We only register the 4-byte breakpoint, even on R2 targets which also
   support 2-byte breakpoints.  Since there is no supports_z_point_type
   function provided, gdbserver never inserts software breakpoints itself
   and instead relies on GDB to insert the breakpoint of the correct length
   via a memory write.  */
static const unsigned int nios2_breakpoint = NIOS2_BREAKPOINT;
#define nios2_breakpoint_len 4

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
nios2_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = nios2_breakpoint_len;
  return (const gdb_byte *) &nios2_breakpoint;
}

/* Implement the low_breakpoint_at linux target ops method.  */

bool
nios2_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  /* For R2, first check for the 2-byte CDX trap.n breakpoint encoding.  */
#if defined(__nios2_arch__) && __nios2_arch__ == 2
  read_memory (where, (unsigned char *) &insn, 2);
  if (insn == CDX_BREAKPOINT)
    return true;
#endif

  read_memory (where, (unsigned char *) &insn, 4);
  if (insn == nios2_breakpoint)
    return true;
  return false;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) *base - idx);

  return PS_OK;
}

/* Helper functions to collect/supply a single register REGNO.  */

static void
nios2_collect_register (struct regcache *regcache, int regno,
			union nios2_register *reg)
{
  union nios2_register tmp_reg;

  collect_register (regcache, regno, &tmp_reg.reg32);
  reg->reg32 = tmp_reg.reg32;
}

static void
nios2_supply_register (struct regcache *regcache, int regno,
		       const union nios2_register *reg)
{
  supply_register (regcache, regno, reg->buf);
}

/* We have only a single register set on Nios II.  */

static void
nios2_fill_gregset (struct regcache *regcache, void *buf)
{
  union nios2_register *regset = (union nios2_register *) buf;
  int i;

  for (i = 1; i < nios2_num_regs; i++)
    nios2_collect_register (regcache, i, regset + i);
}

static void
nios2_store_gregset (struct regcache *regcache, const void *buf)
{
  const union nios2_register *regset = (union nios2_register *) buf;
  int i;

  for (i = 0; i < nios2_num_regs; i++)
    nios2_supply_register (regcache, i, regset + i);
}

static struct regset_info nios2_regsets[] =
{
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS,
    nios2_num_regs * 4, GENERAL_REGS,
    nios2_fill_gregset, nios2_store_gregset },
  NULL_REGSET
};

static struct regsets_info nios2_regsets_info =
  {
    nios2_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info nios2_usrregs_info =
  {
    nios2_num_regs,
    nios2_regmap,
  };

static struct regs_info myregs_info =
  {
    NULL, /* regset_bitmap */
    &nios2_usrregs_info,
    &nios2_regsets_info
  };

const regs_info *
nios2_target::get_regs_info ()
{
  return &myregs_info;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_nios2_target;

void
initialize_low_arch (void)
{
  init_registers_nios2_linux ();

  initialize_regsets_info (&nios2_regsets_info);
}

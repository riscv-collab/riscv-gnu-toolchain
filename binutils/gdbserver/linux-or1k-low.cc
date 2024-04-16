/* GNU/Linux/OR1K specific low level interface for the GDB server.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

/* Linux target op definitions for the OpenRISC architecture.  */

class or1k_target : public linux_process_target
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

static or1k_target the_or1k_target;

bool
or1k_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
or1k_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_32bit (regcache);
}

void
or1k_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  linux_set_pc_32bit (regcache, pc);
}

/* The following definition must agree with the number of registers
   defined in "struct user_regs" in GLIBC
   (sysdeps/unix/sysv/linux/or1k/sys/ucontext.h), and also with
   OR1K_NUM_REGS in GDB proper.  */

#define or1k_num_regs 35

/* Defined in auto-generated file or1k-linux.c.  */

void init_registers_or1k_linux (void);
extern const struct target_desc *tdesc_or1k_linux;

/* This union is used to convert between int and byte buffer
   representations of register contents.  */

union or1k_register
{
  unsigned char buf[4];
  int reg32;
};

/* Return the ptrace ``address'' of register REGNO. */

static int or1k_regmap[] = {
  -1,  1,  2,  3,  4,  5,  6,  7,
  8,  9,  10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31,
  -1, /* PC */
  -1, /* ORIGINAL R11 */
  -1  /* SYSCALL NO */
};

/* Implement the low_arch_setup linux target ops method.  */

void
or1k_target::low_arch_setup ()
{
  current_process ()->tdesc = tdesc_or1k_linux;
}

/* Implement the low_cannot_fetch_register linux target ops method.  */

bool
or1k_target::low_cannot_fetch_register (int regno)
{
  return (or1k_regmap[regno] == -1);
}

/* Implement the low_cannot_store_register linux target ops method.  */

bool
or1k_target::low_cannot_store_register (int regno)
{
  return (or1k_regmap[regno] == -1);
}

/* Breakpoint support.  */

static const unsigned int or1k_breakpoint = 0x21000001;
#define or1k_breakpoint_len 4

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
or1k_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = or1k_breakpoint_len;
  return (const gdb_byte *) &or1k_breakpoint;
}

/* Implement the low_breakpoint_at linux target ops method.  */

bool
or1k_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  read_memory (where, (unsigned char *) &insn, or1k_breakpoint_len);
  if (insn == or1k_breakpoint)
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
or1k_collect_register (struct regcache *regcache, int regno,
			union or1k_register *reg)
{
  union or1k_register tmp_reg;

  collect_register (regcache, regno, &tmp_reg.reg32);
  reg->reg32 = tmp_reg.reg32;
}

static void
or1k_supply_register (struct regcache *regcache, int regno,
		       const union or1k_register *reg)
{
  supply_register (regcache, regno, reg->buf);
}

/* We have only a single register set on OpenRISC.  */

static void
or1k_fill_gregset (struct regcache *regcache, void *buf)
{
  union or1k_register *regset = (union or1k_register *) buf;
  int i;

  for (i = 1; i < or1k_num_regs; i++)
    or1k_collect_register (regcache, i, regset + i);
}

static void
or1k_store_gregset (struct regcache *regcache, const void *buf)
{
  const union or1k_register *regset = (union or1k_register *) buf;
  int i;

  for (i = 0; i < or1k_num_regs; i++)
    or1k_supply_register (regcache, i, regset + i);
}

static struct regset_info or1k_regsets[] =
{
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS,
    or1k_num_regs * 4, GENERAL_REGS,
    or1k_fill_gregset, or1k_store_gregset },
  NULL_REGSET
};

static struct regsets_info or1k_regsets_info =
  {
    or1k_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info or1k_usrregs_info =
  {
    or1k_num_regs,
    or1k_regmap,
  };

static struct regs_info or1k_regs =
  {
    NULL, /* regset_bitmap */
    &or1k_usrregs_info,
    &or1k_regsets_info
  };

const regs_info *
or1k_target::get_regs_info ()
{
  return &or1k_regs;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_or1k_target;

void
initialize_low_arch (void)
{
  init_registers_or1k_linux ();

  initialize_regsets_info (&or1k_regsets_info);
}

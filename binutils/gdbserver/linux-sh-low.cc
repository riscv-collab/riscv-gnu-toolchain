/* GNU/Linux/SH specific low level interface, for the remote server for GDB.
   Copyright (C) 1995-2024 Free Software Foundation, Inc.

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

/* Linux target op definitions for the SH architecture.  */

class sh_target : public linux_process_target
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

static sh_target the_sh_target;

bool
sh_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
sh_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_32bit (regcache);
}

void
sh_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  linux_set_pc_32bit (regcache, pc);
}

/* Defined in auto-generated file reg-sh.c.  */
void init_registers_sh (void);
extern const struct target_desc *tdesc_sh;

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#include <asm/ptrace.h>

#define sh_num_regs 41

/* Currently, don't check/send MQ.  */
static int sh_regmap[] = {
 0,	4,	8,	12,	16,	20,	24,	28,
 32,	36,	40,	44,	48,	52,	56,	60,

 REG_PC*4,   REG_PR*4,   REG_GBR*4,  -1,
 REG_MACH*4, REG_MACL*4, REG_SR*4,
 REG_FPUL*4, REG_FPSCR*4,

 REG_FPREG0*4+0,   REG_FPREG0*4+4,   REG_FPREG0*4+8,   REG_FPREG0*4+12,
 REG_FPREG0*4+16,  REG_FPREG0*4+20,  REG_FPREG0*4+24,  REG_FPREG0*4+28,
 REG_FPREG0*4+32,  REG_FPREG0*4+36,  REG_FPREG0*4+40,  REG_FPREG0*4+44,
 REG_FPREG0*4+48,  REG_FPREG0*4+52,  REG_FPREG0*4+56,  REG_FPREG0*4+60,
};

bool
sh_target::low_cannot_store_register (int regno)
{
  return false;
}

bool
sh_target::low_cannot_fetch_register (int regno)
{
  return false;
}

/* Correct in either endianness, obviously.  */
static const unsigned short sh_breakpoint = 0xc3c3;
#define sh_breakpoint_len 2

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
sh_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = sh_breakpoint_len;
  return (const gdb_byte *) &sh_breakpoint;
}

bool
sh_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned short insn;

  read_memory (where, (unsigned char *) &insn, 2);
  if (insn == sh_breakpoint)
    return true;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
  return false;
}

/* Provide only a fill function for the general register set.  ps_lgetregs
   will use this for NPTL support.  */

static void sh_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < 23; i++)
    if (sh_regmap[i] != -1)
      collect_register (regcache, i, (char *) buf + sh_regmap[i]);
}

static struct regset_info sh_regsets[] = {
  { 0, 0, 0, 0, GENERAL_REGS, sh_fill_gregset, NULL },
  NULL_REGSET
};

static struct regsets_info sh_regsets_info =
  {
    sh_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info sh_usrregs_info =
  {
    sh_num_regs,
    sh_regmap,
  };

static struct regs_info myregs_info =
  {
    NULL, /* regset_bitmap */
    &sh_usrregs_info,
    &sh_regsets_info
  };

const regs_info *
sh_target::get_regs_info ()
{
  return &myregs_info;
}

void
sh_target::low_arch_setup ()
{
  current_process ()->tdesc = tdesc_sh;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_sh_target;

void
initialize_low_arch (void)
{
  init_registers_sh ();

  initialize_regsets_info (&sh_regsets_info);
}

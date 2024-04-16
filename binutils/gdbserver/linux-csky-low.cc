/* GNU/Linux/MIPS specific low level interface, for the remote server for GDB.
   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "tdesc.h"
#include "linux-low.h"
#include <sys/ptrace.h>
#include "gdb_proc_service.h"
#include <asm/ptrace.h>
#include <elf.h>
#include "arch/csky.h"

/* Linux target op definitions for the CSKY architecture.  */

class csky_target : public linux_process_target
{
public:

  const regs_info *get_regs_info () override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

  bool supports_z_point_type (char z_type) override;

  bool supports_hardware_single_step () override;

protected:

  void low_arch_setup () override;

  bool low_cannot_fetch_register (int regno) override;

  bool low_cannot_store_register (int regno) override;

  CORE_ADDR low_get_pc (regcache *regcache) override;

  void low_set_pc (regcache *regcache, CORE_ADDR newpc) override;

  bool low_breakpoint_at (CORE_ADDR pc) override;
};

/* The singleton target ops object.  */

static csky_target the_csky_target;

/* Return the ptrace "address" of register REGNO.  */
static int csky_regmap[] = {
   0*4,  1*4,  2*4,  3*4,  4*4,  5*4,  6*4,  7*4,
   8*4,  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,
  16*4, 17*4, 18*4, 19*4, 20*4, 21*4, 22*4, 23*4,
  24*4, 25*4, 26*4, 27*4, 28*4, 29*4, 30*4, 31*4,
    -1,   -1,   -1,   -1, 34*4, 35*4,   -1,   -1,
  40*4, 42*4, 44*4, 46*4, 48*4, 50*4, 52*4, 54*4, /* fr0 ~ fr15, 64bit  */
  56*4, 58*4, 60*4, 62*4, 64*4, 66*4, 68*4, 70*4,
  72*4, 76*4, 80*4, 84*4, 88*4, 92*4, 96*4,100*4, /* vr0 ~ vr15, 128bit  */
 104*4,108*4,112*4,116*4,120*4,124*4,128*4,132*4,
  33*4,  /* pc  */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  32*4,   -1,   -1,   -1,   -1,   -1,   -1,   -1, /* psr  */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  73*4, 72*4, 74*4,   -1,   -1,   -1, 14*4,  /* fcr, fid, fesr, usp  */
};

/* CSKY software breakpoint instruction code.  */

/* When the kernel code version is behind v4.x,
   illegal insn 0x1464 will be a software bkpt trigger.
   When an illegal insn exception happens, the case
   that insn at EPC is 0x1464 will be recognized as SIGTRAP.  */
static unsigned short csky_breakpoint_illegal_2_v2 = 0x1464;
static unsigned int csky_breakpoint_illegal_4_v2 = 0x14641464;

bool
csky_target::low_cannot_fetch_register (int regno)
{
  if (csky_regmap[regno] == -1)
    return true;

  return false;
}

bool
csky_target::low_cannot_store_register (int regno)
{
  if (csky_regmap[regno] == -1)
    return true;

  return false;
}

/* Get the value of pc register.  */

CORE_ADDR
csky_target::low_get_pc (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "pc", &pc);
  return pc;
}

/* Set pc register.  */

void
csky_target::low_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long new_pc = pc;
  supply_register_by_name (regcache, "pc", &new_pc);
}


void
csky_target::low_arch_setup ()
{
  static const char *expedite_regs[] = { "r14", "pc", NULL };
  target_desc_up tdesc = csky_create_target_description ();

  if (tdesc->expedite_regs.empty ())
    {
      init_target_desc (tdesc.get (), expedite_regs);
      gdb_assert (!tdesc->expedite_regs.empty ());
    }

  current_process ()->tdesc = tdesc.release ();

  return;
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  struct pt_regs regset;
  if (ptrace (PTRACE_GETREGSET, lwpid,
	      (PTRACE_TYPE_ARG3) (long) NT_PRSTATUS, &regset) != 0)
    return PS_ERR;

  *base = (void *) regset.tls;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *)*base - idx);

  return PS_OK;
}

/* Gdbserver uses PTRACE_GET/SET_RGESET.  */

static void
csky_fill_pt_gregset (struct regcache *regcache, void *buf)
{
  int i, base;
  struct pt_regs *regset = (struct pt_regs *)buf;

  collect_register_by_name (regcache, "r15",  &regset->lr);
  collect_register_by_name (regcache, "pc", &regset->pc);
  collect_register_by_name (regcache, "psr", &regset->sr);
  collect_register_by_name (regcache, "r14", &regset->usp);

  collect_register_by_name (regcache, "r0", &regset->a0);
  collect_register_by_name (regcache, "r1", &regset->a1);
  collect_register_by_name (regcache, "r2", &regset->a2);
  collect_register_by_name (regcache, "r3", &regset->a3);

  base = find_regno (regcache->tdesc, "r4");

  for (i = 0; i < 10; i++)
    collect_register (regcache, base + i,   &regset->regs[i]);

  base = find_regno (regcache->tdesc, "r16");
  for (i = 0; i < 16; i++)
    collect_register (regcache, base + i,   &regset->exregs[i]);

  collect_register_by_name (regcache, "hi", &regset->rhi);
  collect_register_by_name (regcache, "lo", &regset->rlo);
}

static void
csky_store_pt_gregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const struct pt_regs *regset = (const struct pt_regs *) buf;

  supply_register_by_name (regcache, "r15",  &regset->lr);
  supply_register_by_name (regcache, "pc", &regset->pc);
  supply_register_by_name (regcache, "psr", &regset->sr);
  supply_register_by_name (regcache, "r14", &regset->usp);

  supply_register_by_name (regcache, "r0", &regset->a0);
  supply_register_by_name (regcache, "r1", &regset->a1);
  supply_register_by_name (regcache, "r2", &regset->a2);
  supply_register_by_name (regcache, "r3", &regset->a3);

  base = find_regno (regcache->tdesc, "r4");

  for (i = 0; i < 10; i++)
    supply_register (regcache, base + i,   &regset->regs[i]);

  base = find_regno (regcache->tdesc, "r16");
  for (i = 0; i < 16; i++)
    supply_register (regcache, base + i,   &regset->exregs[i]);

  supply_register_by_name (regcache, "hi", &regset->rhi);
  supply_register_by_name (regcache, "lo", &regset->rlo);
}

static void
csky_fill_pt_vrregset (struct regcache *regcache, void *buf)
{
  int i, base;
  struct user_fp *regset = (struct user_fp *)buf;

  base = find_regno (regcache->tdesc, "vr0");

  for (i = 0; i < 16; i++)
    collect_register (regcache, base + i, &regset->vr[i * 4]);
  collect_register_by_name (regcache, "fcr", &regset->fcr);
  collect_register_by_name (regcache, "fesr", &regset->fesr);
  collect_register_by_name (regcache, "fid", &regset->fid);
}


static void
csky_store_pt_vrregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const struct user_fp *regset = (const struct user_fp *)buf;

  base = find_regno (regcache->tdesc, "vr0");

  for (i = 0; i < 16; i++)
    supply_register (regcache, base + i, &regset->vr[i * 4]);

  base = find_regno (regcache->tdesc, "fr0");

  for (i = 0; i < 16; i++)
    supply_register (regcache, base + i, &regset->vr[i * 4]);
  supply_register_by_name (regcache, "fcr", &regset->fcr);
  supply_register_by_name (regcache, "fesr", &regset->fesr);
  supply_register_by_name (regcache, "fid", &regset->fid);
}

struct regset_info csky_regsets[] = {
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS, sizeof(struct pt_regs),
    GENERAL_REGS, csky_fill_pt_gregset, csky_store_pt_gregset},

  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_FPREGSET, sizeof(struct user_fp),
    FP_REGS, csky_fill_pt_vrregset, csky_store_pt_vrregset},
  NULL_REGSET
};


static struct regsets_info csky_regsets_info =
{
  csky_regsets, /* Regsets  */
  0,            /* Num_regsets  */
  NULL,         /* Disabled_regsets  */
};


static struct regs_info csky_regs_info =
{
  NULL, /* FIXME: what's this  */
  NULL, /* PEEKUSER/POKEUSR isn't supported by kernel > 4.x */
  &csky_regsets_info
};


const regs_info *
csky_target::get_regs_info ()
{
  return &csky_regs_info;
}

/* Implementation of linux_target_ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
csky_target::sw_breakpoint_from_kind (int kind, int *size)
{
  if (kind == 4)
    {
      *size = 4;
      return (const gdb_byte *) &csky_breakpoint_illegal_4_v2;
    }
  else
    {
      *size = 2;
      return (const gdb_byte *) &csky_breakpoint_illegal_2_v2;
    }
}

bool
csky_target::supports_z_point_type (char z_type)
{
  /* FIXME: hardware breakpoint support ??  */
  if (z_type == Z_PACKET_SW_BP)
    return true;

  return false;
}

bool
csky_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  /* Here just read 2 bytes, as csky_breakpoint_illegal_4_v2
     is double of csky_breakpoint_illegal_2_v2, csky_breakpoint_bkpt_4
     is double of csky_breakpoint_bkpt_2. Others are 2 bytes bkpt.  */
  read_memory (where, (unsigned char *) &insn, 2);

  if (insn == csky_breakpoint_illegal_2_v2)
    return true;

  return false;
}

/* Support for hardware single step.  */

bool
csky_target::supports_hardware_single_step ()
{
  return true;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_csky_target;

void
initialize_low_arch (void)
{
  initialize_regsets_info (&csky_regsets_info);
}

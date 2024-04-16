/* GNU/Linux/Xtensa specific low level interface, for the remote server for GDB.
   Copyright (C) 2007-2024 Free Software Foundation, Inc.

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

/* Linux target op definitions for the Xtensa architecture.  */

class xtensa_target : public linux_process_target
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

static xtensa_target the_xtensa_target;

bool
xtensa_target::low_cannot_fetch_register (int regno)
{
  gdb_assert_not_reached ("linux target op low_cannot_fetch_register "
			  "is not implemented by the target");
}

bool
xtensa_target::low_cannot_store_register (int regno)
{
  gdb_assert_not_reached ("linux target op low_cannot_store_register "
			  "is not implemented by the target");
}

bool
xtensa_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
xtensa_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_32bit (regcache);
}

void
xtensa_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  linux_set_pc_32bit (regcache, pc);
}

/* Defined in auto-generated file reg-xtensa.c.  */
void init_registers_xtensa (void);
extern const struct target_desc *tdesc_xtensa;

#include <asm/ptrace.h>
#include <xtensa-config.h>
#include "arch/xtensa.h"
#include "gdb_proc_service.h"

#include "xtensa-xtregs.c"

enum regnum {
	R_PC=0,	R_PS,
	R_LBEG,	R_LEND,	R_LCOUNT,
	R_SAR,
	R_WS, R_WB,
	R_THREADPTR,
	R_A0 = 64
};

static void
xtensa_fill_gregset (struct regcache *regcache, void *buf)
{
  elf_greg_t* rset = (elf_greg_t*)buf;
  const struct target_desc *tdesc = regcache->tdesc;
  int ar0_regnum;
  char *ptr;
  int i;

  /* Take care of AR registers.  */

  ar0_regnum = find_regno (tdesc, "ar0");
  ptr = (char*)&rset[R_A0];

  for (i = ar0_regnum; i < ar0_regnum + XCHAL_NUM_AREGS; i++)
    {
      collect_register (regcache, i, ptr);
      ptr += register_size (tdesc, i);
    }

  if (XSHAL_ABI == XTHAL_ABI_CALL0)
    {
      int a0_regnum = find_regno (tdesc, "a0");
      ptr = (char *) &rset[R_A0 + 4 * rset[R_WB]];

      for (i = a0_regnum; i < a0_regnum + C0_NREGS; i++)
	{
	  if ((4 * rset[R_WB] + i - a0_regnum) == XCHAL_NUM_AREGS)
	    ptr = (char *) &rset[R_A0];
	  collect_register (regcache, i, ptr);
	  ptr += register_size (tdesc, i);
	}
    }

  /* Loop registers, if hardware has it.  */

#if XCHAL_HAVE_LOOPS
  collect_register_by_name (regcache, "lbeg", (char*)&rset[R_LBEG]);
  collect_register_by_name (regcache, "lend", (char*)&rset[R_LEND]);
  collect_register_by_name (regcache, "lcount", (char*)&rset[R_LCOUNT]);
#endif

  collect_register_by_name (regcache, "sar", (char*)&rset[R_SAR]);
  collect_register_by_name (regcache, "pc", (char*)&rset[R_PC]);
  collect_register_by_name (regcache, "ps", (char*)&rset[R_PS]);
  collect_register_by_name (regcache, "windowbase", (char*)&rset[R_WB]);
  collect_register_by_name (regcache, "windowstart", (char*)&rset[R_WS]);

#if XCHAL_HAVE_THREADPTR
  collect_register_by_name (regcache, "threadptr",
			    (char *) &rset[R_THREADPTR]);
#endif
}

static void
xtensa_store_gregset (struct regcache *regcache, const void *buf)
{
  const elf_greg_t* rset = (const elf_greg_t*)buf;
  const struct target_desc *tdesc = regcache->tdesc;
  int ar0_regnum;
  char *ptr;
  int i;

  /* Take care of AR registers.  */

  ar0_regnum = find_regno (tdesc, "ar0");
  ptr = (char *)&rset[R_A0];

  for (i = ar0_regnum; i < ar0_regnum + XCHAL_NUM_AREGS; i++)
    {
      supply_register (regcache, i, ptr);
      ptr += register_size (tdesc, i);
    }

  if (XSHAL_ABI == XTHAL_ABI_CALL0)
    {
      int a0_regnum = find_regno (tdesc, "a0");
      ptr = (char *) &rset[R_A0 + (4 * rset[R_WB]) % XCHAL_NUM_AREGS];

      for (i = a0_regnum; i < a0_regnum + C0_NREGS; i++)
	{
	  if ((4 * rset[R_WB] + i - a0_regnum) == XCHAL_NUM_AREGS)
	    ptr = (char *) &rset[R_A0];
	  supply_register (regcache, i, ptr);
	  ptr += register_size (tdesc, i);
	}
    }

  /* Loop registers, if hardware has it.  */

#if XCHAL_HAVE_LOOPS
  supply_register_by_name (regcache, "lbeg", (char*)&rset[R_LBEG]);
  supply_register_by_name (regcache, "lend", (char*)&rset[R_LEND]);
  supply_register_by_name (regcache, "lcount", (char*)&rset[R_LCOUNT]);
#endif

  supply_register_by_name (regcache, "sar", (char*)&rset[R_SAR]);
  supply_register_by_name (regcache, "pc", (char*)&rset[R_PC]);
  supply_register_by_name (regcache, "ps", (char*)&rset[R_PS]);
  supply_register_by_name (regcache, "windowbase", (char*)&rset[R_WB]);
  supply_register_by_name (regcache, "windowstart", (char*)&rset[R_WS]);

#if XCHAL_HAVE_THREADPTR
  supply_register_by_name (regcache, "threadptr",
			   (char *) &rset[R_THREADPTR]);
#endif
}

/* Xtensa GNU/Linux PTRACE interface includes extended register set.  */

static void
xtensa_fill_xtregset (struct regcache *regcache, void *buf)
{
  const xtensa_regtable_t *ptr;

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    {
      collect_register_by_name (regcache, ptr->name,
				(char*)buf + ptr->ptrace_offset);
    }
}

static void
xtensa_store_xtregset (struct regcache *regcache, const void *buf)
{
  const xtensa_regtable_t *ptr;

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    {
      supply_register_by_name (regcache, ptr->name,
				(char*)buf + ptr->ptrace_offset);
    }
}

static struct regset_info xtensa_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, sizeof (elf_gregset_t),
    GENERAL_REGS,
    xtensa_fill_gregset, xtensa_store_gregset },
  { PTRACE_GETXTREGS, PTRACE_SETXTREGS, 0, XTENSA_ELF_XTREG_SIZE,
    EXTENDED_REGS,
    xtensa_fill_xtregset, xtensa_store_xtregset },
  NULL_REGSET
};

#if XCHAL_HAVE_BE
#define XTENSA_BREAKPOINT {0xd2,0x0f}
#else
#define XTENSA_BREAKPOINT {0x2d,0xf0}
#endif

static const gdb_byte xtensa_breakpoint[] = XTENSA_BREAKPOINT;
#define xtensa_breakpoint_len 2

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
xtensa_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = xtensa_breakpoint_len;
  return xtensa_breakpoint;
}

bool
xtensa_target::low_breakpoint_at (CORE_ADDR where)
{
    unsigned long insn;

    read_memory (where, (unsigned char *) &insn, xtensa_breakpoint_len);
    return memcmp((char *) &insn,
		  xtensa_breakpoint, xtensa_breakpoint_len) == 0;
}

/* Called by libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  xtensa_elf_gregset_t regs;

  if (ptrace (PTRACE_GETREGS, lwpid, NULL, &regs) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) regs.threadptr - idx);

  return PS_OK;
}

static struct regsets_info xtensa_regsets_info =
  {
    xtensa_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct regs_info myregs_info =
  {
    NULL, /* regset_bitmap */
    NULL, /* usrregs */
    &xtensa_regsets_info
  };

void
xtensa_target::low_arch_setup ()
{
  current_process ()->tdesc = tdesc_xtensa;
}

const regs_info *
xtensa_target::get_regs_info ()
{
  return &myregs_info;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_xtensa_target;

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
  init_registers_xtensa ();

  initialize_regsets_info (&xtensa_regsets_info);
}

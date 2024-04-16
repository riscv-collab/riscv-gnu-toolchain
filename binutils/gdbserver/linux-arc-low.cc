/* Target dependent code for the remote server for GNU/Linux ARC.

   Copyright 2020-2024 Free Software Foundation, Inc.

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
#include "regdef.h"
#include "linux-low.h"
#include "tdesc.h"
#include "arch/arc.h"

#include <linux/elf.h>
#include <arpa/inet.h>

/* Linux starting with 4.12 supports NT_ARC_V2 note type, which adds R30,
   R58 and R59 registers.  */
#ifdef NT_ARC_V2
#define ARC_HAS_V2_REGSET
#endif

/* The encoding of the instruction "TRAP_S 1" (endianness agnostic).  */
#define TRAP_S_1_OPCODE	0x783e
#define TRAP_S_1_SIZE	2

/* Using a mere "uint16_t arc_linux_traps_s = TRAP_S_1_OPCODE" would
   work as well, because the endianness will end up correctly when
   the code is compiled for the same endianness as the target (see
   the notes for "low_breakpoint_at" in this file).  However, this
   illustrates how the __BIG_ENDIAN__ macro can be used to make
   easy-to-understand codes.  */
#if defined(__BIG_ENDIAN__)
/* 0x78, 0x3e.  */
static gdb_byte arc_linux_trap_s[TRAP_S_1_SIZE]
	= {TRAP_S_1_OPCODE >> 8, TRAP_S_1_OPCODE & 0xFF};
#else
/* 0x3e, 0x78.  */
static gdb_byte arc_linux_trap_s[TRAP_S_1_SIZE]
	= {TRAP_S_1_OPCODE && 0xFF, TRAP_S_1_OPCODE >> 8};
#endif

/* Linux target op definitions for the ARC architecture.
   Note for future: in case of adding the protected method low_get_next_pcs(),
   the public method supports_software_single_step() should be added to return
   "true".  */

class arc_target : public linux_process_target
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

  bool low_breakpoint_at (CORE_ADDR where) override;
};

/* The singleton target ops object.  */

static arc_target the_arc_target;

bool
arc_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
arc_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_32bit (regcache);
}

void
arc_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  linux_set_pc_32bit (regcache, pc);
}

static const struct target_desc *
arc_linux_read_description (void)
{
#ifdef __ARC700__
  arc_arch_features features (4, ARC_ISA_ARCV1);
#else
  arc_arch_features features (4, ARC_ISA_ARCV2);
#endif
  target_desc_up tdesc = arc_create_target_description (features);

  static const char *expedite_regs[] = { "sp", "status32", nullptr };
  init_target_desc (tdesc.get (), expedite_regs);

  return tdesc.release ();
}

void
arc_target::low_arch_setup ()
{
  current_process ()->tdesc = arc_linux_read_description ();
}

bool
arc_target::low_cannot_fetch_register (int regno)
{
  return (regno >= current_process ()->tdesc->reg_defs.size ());
}

bool
arc_target::low_cannot_store_register (int regno)
{
  return (regno >= current_process ()->tdesc->reg_defs.size ());
}

/* This works for both endianness.  Below you see an illustration of how
   the "trap_s 1" instruction encoded for both endianness in the memory
   will end up as the TRAP_S_1_OPCODE constant:

   BE: 0x78 0x3e --> at INSN addr: 0x78 0x3e --> INSN = 0x783e
   LE: 0x3e 0x78 --> at INSN addr: 0x3e 0x78 --> INSN = 0x783e

   One can employ "memcmp()" for comparing the arrays too.  */

bool
arc_target::low_breakpoint_at (CORE_ADDR where)
{
  uint16_t insn;

  /* "the_target" global variable is the current object at hand.  */
  this->read_memory (where, (gdb_byte *) &insn, TRAP_S_1_SIZE);
  return (insn == TRAP_S_1_OPCODE);
}

/* PTRACE_GETREGSET/NT_PRSTATUS and PTRACE_SETREGSET/NT_PRSTATUS work with
   regsets in a struct, "user_regs_struct", defined in the
   linux/arch/arc/include/uapi/asm/ptrace.h header.  This code supports
   ARC Linux ABI v3 and v4.  */

/* Populate a ptrace NT_PRSTATUS regset from a regcache.

   This appears to be a unique approach to populating the buffer, but
   being name, rather than offset based, it is robust to future API
   changes, as there is no need to create a regmap of registers in the
   user_regs_struct.  */

static void
arc_fill_gregset (struct regcache *regcache, void *buf)
{
  struct user_regs_struct *regbuf = (struct user_regs_struct *) buf;

  /* Core registers.  */
  collect_register_by_name (regcache, "r0", &(regbuf->scratch.r0));
  collect_register_by_name (regcache, "r1", &(regbuf->scratch.r1));
  collect_register_by_name (regcache, "r2", &(regbuf->scratch.r2));
  collect_register_by_name (regcache, "r3", &(regbuf->scratch.r3));
  collect_register_by_name (regcache, "r4", &(regbuf->scratch.r4));
  collect_register_by_name (regcache, "r5", &(regbuf->scratch.r5));
  collect_register_by_name (regcache, "r6", &(regbuf->scratch.r6));
  collect_register_by_name (regcache, "r7", &(regbuf->scratch.r7));
  collect_register_by_name (regcache, "r8", &(regbuf->scratch.r8));
  collect_register_by_name (regcache, "r9", &(regbuf->scratch.r9));
  collect_register_by_name (regcache, "r10", &(regbuf->scratch.r10));
  collect_register_by_name (regcache, "r11", &(regbuf->scratch.r11));
  collect_register_by_name (regcache, "r12", &(regbuf->scratch.r12));
  collect_register_by_name (regcache, "r13", &(regbuf->callee.r13));
  collect_register_by_name (regcache, "r14", &(regbuf->callee.r14));
  collect_register_by_name (regcache, "r15", &(regbuf->callee.r15));
  collect_register_by_name (regcache, "r16", &(regbuf->callee.r16));
  collect_register_by_name (regcache, "r17", &(regbuf->callee.r17));
  collect_register_by_name (regcache, "r18", &(regbuf->callee.r18));
  collect_register_by_name (regcache, "r19", &(regbuf->callee.r19));
  collect_register_by_name (regcache, "r20", &(regbuf->callee.r20));
  collect_register_by_name (regcache, "r21", &(regbuf->callee.r21));
  collect_register_by_name (regcache, "r22", &(regbuf->callee.r22));
  collect_register_by_name (regcache, "r23", &(regbuf->callee.r23));
  collect_register_by_name (regcache, "r24", &(regbuf->callee.r24));
  collect_register_by_name (regcache, "r25", &(regbuf->callee.r25));
  collect_register_by_name (regcache, "gp", &(regbuf->scratch.gp));
  collect_register_by_name (regcache, "fp", &(regbuf->scratch.fp));
  collect_register_by_name (regcache, "sp", &(regbuf->scratch.sp));
  collect_register_by_name (regcache, "blink", &(regbuf->scratch.blink));

  /* Loop registers.  */
  collect_register_by_name (regcache, "lp_count", &(regbuf->scratch.lp_count));
  collect_register_by_name (regcache, "lp_start", &(regbuf->scratch.lp_start));
  collect_register_by_name (regcache, "lp_end", &(regbuf->scratch.lp_end));

  /* The current "pc" value must be written to "eret" (exception return
     address) register, because that is the address that the kernel code
     will jump back to after a breakpoint exception has been raised.
     The "pc_stop" value is ignored by the genregs_set() in
     linux/arch/arc/kernel/ptrace.c.  */
  collect_register_by_name (regcache, "pc", &(regbuf->scratch.ret));

  /* Currently ARC Linux ptrace doesn't allow writes to status32 because
     some of its bits are kernel mode-only and shoudn't be writable from
     user-space.  Writing status32 from debugger could be useful, though,
     so ability to write non-privileged bits will be added to kernel
     sooner or later.  */

  /* BTA.  */
  collect_register_by_name (regcache, "bta", &(regbuf->scratch.bta));
}

/* Populate a regcache from a ptrace NT_PRSTATUS regset.  */

static void
arc_store_gregset (struct regcache *regcache, const void *buf)
{
  const struct user_regs_struct *regbuf = (const struct user_regs_struct *) buf;

  /* Core registers.  */
  supply_register_by_name (regcache, "r0", &(regbuf->scratch.r0));
  supply_register_by_name (regcache, "r1", &(regbuf->scratch.r1));
  supply_register_by_name (regcache, "r2", &(regbuf->scratch.r2));
  supply_register_by_name (regcache, "r3", &(regbuf->scratch.r3));
  supply_register_by_name (regcache, "r4", &(regbuf->scratch.r4));
  supply_register_by_name (regcache, "r5", &(regbuf->scratch.r5));
  supply_register_by_name (regcache, "r6", &(regbuf->scratch.r6));
  supply_register_by_name (regcache, "r7", &(regbuf->scratch.r7));
  supply_register_by_name (regcache, "r8", &(regbuf->scratch.r8));
  supply_register_by_name (regcache, "r9", &(regbuf->scratch.r9));
  supply_register_by_name (regcache, "r10", &(regbuf->scratch.r10));
  supply_register_by_name (regcache, "r11", &(regbuf->scratch.r11));
  supply_register_by_name (regcache, "r12", &(regbuf->scratch.r12));
  supply_register_by_name (regcache, "r13", &(regbuf->callee.r13));
  supply_register_by_name (regcache, "r14", &(regbuf->callee.r14));
  supply_register_by_name (regcache, "r15", &(regbuf->callee.r15));
  supply_register_by_name (regcache, "r16", &(regbuf->callee.r16));
  supply_register_by_name (regcache, "r17", &(regbuf->callee.r17));
  supply_register_by_name (regcache, "r18", &(regbuf->callee.r18));
  supply_register_by_name (regcache, "r19", &(regbuf->callee.r19));
  supply_register_by_name (regcache, "r20", &(regbuf->callee.r20));
  supply_register_by_name (regcache, "r21", &(regbuf->callee.r21));
  supply_register_by_name (regcache, "r22", &(regbuf->callee.r22));
  supply_register_by_name (regcache, "r23", &(regbuf->callee.r23));
  supply_register_by_name (regcache, "r24", &(regbuf->callee.r24));
  supply_register_by_name (regcache, "r25", &(regbuf->callee.r25));
  supply_register_by_name (regcache, "gp", &(regbuf->scratch.gp));
  supply_register_by_name (regcache, "fp", &(regbuf->scratch.fp));
  supply_register_by_name (regcache, "sp", &(regbuf->scratch.sp));
  supply_register_by_name (regcache, "blink", &(regbuf->scratch.blink));

  /* Loop registers.  */
  supply_register_by_name (regcache, "lp_count", &(regbuf->scratch.lp_count));
  supply_register_by_name (regcache, "lp_start", &(regbuf->scratch.lp_start));
  supply_register_by_name (regcache, "lp_end", &(regbuf->scratch.lp_end));

  /* The genregs_get() in linux/arch/arc/kernel/ptrace.c populates the
     pseudo register "stop_pc" with the "efa" (exception fault address)
     register.  This was deemed necessary, because the breakpoint
     instruction, "trap_s 1", is a committing one; i.e. the "eret"
     (exception return address) register will be pointing to the next
     instruction, while "efa" points to the address that raised the
     breakpoint.  */
  supply_register_by_name (regcache, "pc", &(regbuf->stop_pc));
  unsigned long pcl = regbuf->stop_pc & ~3L;
  supply_register_by_name (regcache, "pcl", &pcl);

  /* Other auxilliary registers.  */
  supply_register_by_name (regcache, "status32", &(regbuf->scratch.status32));

  /* BTA.  */
  supply_register_by_name (regcache, "bta", &(regbuf->scratch.bta));
}

#ifdef ARC_HAS_V2_REGSET

/* Look through a regcache's TDESC for a register named NAME.
   If found, return true; false, otherwise.  */

static bool
is_reg_name_available_p (const struct target_desc *tdesc,
			 const char *name)
{
  for (const gdb::reg &reg : tdesc->reg_defs)
    if (strcmp (name, reg.name) == 0)
      return true;
  return false;
}

/* Copy registers from regcache to user_regs_arcv2.  */

static void
arc_fill_v2_regset (struct regcache *regcache, void *buf)
{
  struct user_regs_arcv2 *regbuf = (struct user_regs_arcv2 *) buf;

  if (is_reg_name_available_p (regcache->tdesc, "r30"))
    collect_register_by_name (regcache, "r30", &(regbuf->r30));

  if (is_reg_name_available_p (regcache->tdesc, "r58"))
    collect_register_by_name (regcache, "r58", &(regbuf->r58));

  if (is_reg_name_available_p (regcache->tdesc, "r59"))
    collect_register_by_name (regcache, "r59", &(regbuf->r59));
}

/* Copy registers from user_regs_arcv2 to regcache.  */

static void
arc_store_v2_regset (struct regcache *regcache, const void *buf)
{
  struct user_regs_arcv2 *regbuf = (struct user_regs_arcv2 *) buf;

  if (is_reg_name_available_p (regcache->tdesc, "r30"))
    supply_register_by_name (regcache, "r30", &(regbuf->r30));

  if (is_reg_name_available_p (regcache->tdesc, "r58"))
    supply_register_by_name (regcache, "r58", &(regbuf->r58));

  if (is_reg_name_available_p (regcache->tdesc, "r59"))
    supply_register_by_name (regcache, "r59", &(regbuf->r59));
}

#endif

/* Fetch the thread-local storage pointer for libthread_db.  Note that
   this function is not called from GDB, but is called from libthread_db.

   This is the same function as for other architectures, for example in
   linux-arm-low.c.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph, lwpid_t lwpid,
		    int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, nullptr, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *) *base - idx);

  return PS_OK;
}

static struct regset_info arc_regsets[] =
{
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS,
    sizeof (struct user_regs_struct), GENERAL_REGS,
    arc_fill_gregset, arc_store_gregset
  },
#ifdef ARC_HAS_V2_REGSET
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARC_V2,
    sizeof (struct user_regs_arcv2), GENERAL_REGS,
    arc_fill_v2_regset, arc_store_v2_regset
  },
#endif
  NULL_REGSET
};

static struct regsets_info arc_regsets_info =
{
  arc_regsets,	/* regsets */
  0,		/* num_regsets */
  nullptr,	/* disabled regsets */
};

static struct regs_info arc_regs_info =
{
  nullptr,	/* regset_bitmap */
  nullptr,	/* usrregs */
  &arc_regsets_info
};

const regs_info *
arc_target::get_regs_info ()
{
  return &arc_regs_info;
}

/* One of the methods necessary for Z0 packet support.  */

const gdb_byte *
arc_target::sw_breakpoint_from_kind (int kind, int *size)
{
  gdb_assert (kind == TRAP_S_1_SIZE);
  *size = kind;
  return arc_linux_trap_s;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_arc_target;

void
initialize_low_arch (void)
{
  initialize_regsets_info (&arc_regsets_info);
}

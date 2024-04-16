/* Low level interface to ptrace, for the remote server for GDB.
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

#include "nat/gdb_ptrace.h"

#include "gdb_proc_service.h"

/* The stack pointer is offset from the stack frame by a BIAS of 2047
   (0x7ff) for 64-bit code.  BIAS is likely to be defined on SPARC
   hosts, so undefine it first.  */
#undef BIAS
#define BIAS 2047

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define INSN_SIZE 4

#define SPARC_R_REGS_NUM 32
#define SPARC_F_REGS_NUM 48
#define SPARC_CONTROL_REGS_NUM 6

#define sparc_num_regs \
  (SPARC_R_REGS_NUM + SPARC_F_REGS_NUM + SPARC_CONTROL_REGS_NUM)

/* Linux target op definitions for the SPARC architecture.  */

class sparc_target : public linux_process_target
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

  /* No low_set_pc is needed.  */

  bool low_breakpoint_at (CORE_ADDR pc) override;
};

/* The singleton target ops object.  */

static sparc_target the_sparc_target;

bool
sparc_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
sparc_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_64bit (regcache);
}

/* Each offset is multiplied by 8, because of the register size.
   These offsets apply to the buffer sent/filled by ptrace.
   Additionally, the array elements order corresponds to the .dat file, and the
   gdb's registers enumeration order.  */

static int sparc_regmap[] = {
  /* These offsets correspond to GET/SETREGSET.  */
	-1,  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,	 /* g0 .. g7 */
	7*8,  8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8,	 /* o0 .. o5, sp, o7 */
	-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,	 /* l0 .. l7 */
	-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,	 /* i0 .. i5, fp, i7 */

  /* Floating point registers offsets correspond to GET/SETFPREGSET.  */
    0*4,  1*4,  2*4,  3*4,  4*4,  5*4,  6*4,  7*4,	   /*  f0 ..  f7 */
    8*4,  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,	   /*  f8 .. f15 */
   16*4, 17*4, 18*4, 19*4, 20*4, 21*4, 22*4, 23*4,	   /* f16 .. f23 */
   24*4, 25*4, 26*4, 27*4, 28*4, 29*4, 30*4, 31*4,	   /* f24 .. f31 */

  /* F32 offset starts next to f31: 31*4+4 = 16 * 8.  */
   16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,	   /* f32 .. f46 */
   24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8,	   /* f48 .. f62 */

   17 *8, /*    pc */
   18 *8, /*   npc */
   16 *8, /* state */
  /* FSR offset also corresponds to GET/SETFPREGSET, ans is placed
     next to f62.  */
   32 *8, /*   fsr */
      -1, /*  fprs */
  /* Y register is 32-bits length, but gdb takes care of that.  */
   19 *8, /*     y */

};


struct regs_range_t
{
  int regno_start;
  int regno_end;
};

static const struct regs_range_t gregs_ranges[] = {
 {  0, 31 }, /*   g0 .. i7  */
 { 80, 82 }, /*   pc .. state */
 { 84, 85 }  /* fprs .. y */
};

#define N_GREGS_RANGES (sizeof (gregs_ranges) / sizeof (struct regs_range_t))

static const struct regs_range_t fpregs_ranges[] = {
 { 32, 79 }, /* f0 .. f62  */
 { 83, 83 }  /* fsr */
};

#define N_FPREGS_RANGES (sizeof (fpregs_ranges) / sizeof (struct regs_range_t))

/* Defined in auto-generated file reg-sparc64.c.  */
void init_registers_sparc64 (void);
extern const struct target_desc *tdesc_sparc64;

bool
sparc_target::low_cannot_store_register (int regno)
{
  return (regno >= sparc_num_regs || sparc_regmap[regno] == -1);
}

bool
sparc_target::low_cannot_fetch_register (int regno)
{
  return (regno >= sparc_num_regs || sparc_regmap[regno] == -1);
}

static void
sparc_fill_gregset_to_stack (struct regcache *regcache, const void *buf)
{
  int i;
  CORE_ADDR addr = 0;
  unsigned char tmp_reg_buf[8];
  const int l0_regno = find_regno (regcache->tdesc, "l0");
  const int i7_regno = l0_regno + 15;

  /* These registers have to be stored in the stack.  */
  memcpy (&addr,
	  ((char *) buf) + sparc_regmap[find_regno (regcache->tdesc, "sp")],
	  sizeof (addr));

  addr += BIAS;

  for (i = l0_regno; i <= i7_regno; i++)
    {
      collect_register (regcache, i, tmp_reg_buf);
      the_target->write_memory (addr, tmp_reg_buf, sizeof (tmp_reg_buf));
      addr += sizeof (tmp_reg_buf);
    }
}

static void
sparc_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;
  int range;

  for (range = 0; range < N_GREGS_RANGES; range++)
    for (i = gregs_ranges[range].regno_start;
	 i <= gregs_ranges[range].regno_end; i++)
      if (sparc_regmap[i] != -1)
	collect_register (regcache, i, ((char *) buf) + sparc_regmap[i]);

  sparc_fill_gregset_to_stack (regcache, buf);
}

static void
sparc_fill_fpregset (struct regcache *regcache, void *buf)
{
  int i;
  int range;

  for (range = 0; range < N_FPREGS_RANGES; range++)
    for (i = fpregs_ranges[range].regno_start;
	 i <= fpregs_ranges[range].regno_end; i++)
      collect_register (regcache, i, ((char *) buf) + sparc_regmap[i]);

}

static void
sparc_store_gregset_from_stack (struct regcache *regcache, const void *buf)
{
  int i;
  CORE_ADDR addr = 0;
  unsigned char tmp_reg_buf[8];
  const int l0_regno = find_regno (regcache->tdesc, "l0");
  const int i7_regno = l0_regno + 15;

  /* These registers have to be obtained from the stack.  */
  memcpy (&addr,
	  ((char *) buf) + sparc_regmap[find_regno (regcache->tdesc, "sp")],
	  sizeof (addr));

  addr += BIAS;

  for (i = l0_regno; i <= i7_regno; i++)
    {
      the_target->read_memory (addr, tmp_reg_buf, sizeof (tmp_reg_buf));
      supply_register (regcache, i, tmp_reg_buf);
      addr += sizeof (tmp_reg_buf);
    }
}

static void
sparc_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;
  char zerobuf[8];
  int range;

  memset (zerobuf, 0, sizeof (zerobuf));

  for (range = 0; range < N_GREGS_RANGES; range++)
    for (i = gregs_ranges[range].regno_start;
	 i <= gregs_ranges[range].regno_end; i++)
      if (sparc_regmap[i] != -1)
	supply_register (regcache, i, ((char *) buf) + sparc_regmap[i]);
      else
	supply_register (regcache, i, zerobuf);

  sparc_store_gregset_from_stack (regcache, buf);
}

static void
sparc_store_fpregset (struct regcache *regcache, const void *buf)
{
  int i;
  int range;

  for (range = 0; range < N_FPREGS_RANGES; range++)
    for (i = fpregs_ranges[range].regno_start;
	 i <= fpregs_ranges[range].regno_end;
	 i++)
      supply_register (regcache, i, ((char *) buf) + sparc_regmap[i]);
}

static const gdb_byte sparc_breakpoint[INSN_SIZE] = {
  0x91, 0xd0, 0x20, 0x01
};
#define sparc_breakpoint_len INSN_SIZE

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
sparc_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = sparc_breakpoint_len;
  return sparc_breakpoint;
}

bool
sparc_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned char insn[INSN_SIZE];

  read_memory (where, (unsigned char *) insn, sizeof (insn));

  if (memcmp (sparc_breakpoint, insn, sizeof (insn)) == 0)
    return true;

  /* If necessary, recognize more trap instructions here.  GDB only
     uses TRAP Always.  */

  return false;
}

void
sparc_target::low_arch_setup ()
{
  current_process ()->tdesc = tdesc_sparc64;
}

static struct regset_info sparc_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, sizeof (elf_gregset_t),
    GENERAL_REGS,
    sparc_fill_gregset, sparc_store_gregset },
  { PTRACE_GETFPREGS, PTRACE_SETFPREGS, 0, sizeof (fpregset_t),
    FP_REGS,
    sparc_fill_fpregset, sparc_store_fpregset },
  NULL_REGSET
};

static struct regsets_info sparc_regsets_info =
  {
    sparc_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info sparc_usrregs_info =
  {
    sparc_num_regs,
    /* No regmap needs to be provided since this impl. doesn't use
       USRREGS.  */
    NULL
  };

static struct regs_info myregs_info =
  {
    NULL, /* regset_bitmap */
    &sparc_usrregs_info,
    &sparc_regsets_info
  };

const regs_info *
sparc_target::get_regs_info ()
{
  return &myregs_info;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_sparc_target;

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */
  init_registers_sparc64 ();

  initialize_regsets_info (&sparc_regsets_info);
}

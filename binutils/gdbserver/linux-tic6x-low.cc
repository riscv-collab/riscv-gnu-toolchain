/* Target dependent code for GDB on TI C6x systems.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Andrew Jenner <andrew@codesourcery.com>
   Contributed by Yao Qi <yao@codesourcery.com>

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
#include "arch/tic6x.h"
#include "tdesc.h"

#include "nat/gdb_ptrace.h"
#include <endian.h>

#include "gdb_proc_service.h"

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

/* There are at most 69 registers accessible in ptrace.  */
#define TIC6X_NUM_REGS 69

#include <asm/ptrace.h>

/* Linux target op definitions for the TI C6x architecture.  */

class tic6x_target : public linux_process_target
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

static tic6x_target the_tic6x_target;

/* Defined in auto-generated file tic6x-c64xp-linux.c.  */
void init_registers_tic6x_c64xp_linux (void);
extern const struct target_desc *tdesc_tic6x_c64xp_linux;

/* Defined in auto-generated file tic6x-c64x-linux.c.  */
void init_registers_tic6x_c64x_linux (void);
extern const struct target_desc *tdesc_tic6x_c64x_linux;

/* Defined in auto-generated file tic62x-c6xp-linux.c.  */
void init_registers_tic6x_c62x_linux (void);
extern const struct target_desc *tdesc_tic6x_c62x_linux;

union tic6x_register
{
  unsigned char buf[4];

  int reg32;
};

/* Return the ptrace ``address'' of register REGNO.  */

#if __BYTE_ORDER == __BIG_ENDIAN
static int tic6x_regmap_c64xp[] = {
  /* A0 - A15 */
  53, 52, 55, 54, 57, 56, 59, 58,
  61, 60, 63, 62, 65, 64, 67, 66,
  /* B0 - B15 */
  23, 22, 25, 24, 27, 26, 29, 28,
  31, 30, 33, 32, 35, 34, 69, 68,
  /* CSR PC */
  5, 4,
  /* A16 - A31 */
  37, 36, 39, 38, 41, 40, 43, 42,
  45, 44, 47, 46, 49, 48, 51, 50,
  /* B16 - B31 */
  7,  6,  9,  8,  11, 10, 13, 12,
  15, 14, 17, 16, 19, 18, 21, 20,
  /* TSR, ILC, RILC */
  1,  2, 3
};

static int tic6x_regmap_c64x[] = {
  /* A0 - A15 */
  51, 50, 53, 52, 55, 54, 57, 56,
  59, 58, 61, 60, 63, 62, 65, 64,
  /* B0 - B15 */
  21, 20, 23, 22, 25, 24, 27, 26,
  29, 28, 31, 30, 33, 32, 67, 66,
  /* CSR PC */
  3,  2,
  /* A16 - A31 */
  35, 34, 37, 36, 39, 38, 41, 40,
  43, 42, 45, 44, 47, 46, 49, 48,
  /* B16 - B31 */
  5,  4,  7,  6,  9,  8,  11, 10,
  13, 12, 15, 14, 17, 16, 19, 18,
  -1, -1, -1
};

static int tic6x_regmap_c62x[] = {
  /* A0 - A15 */
  19, 18, 21, 20, 23, 22, 25, 24,
  27, 26, 29, 28, 31, 30, 33, 32,
  /* B0 - B15 */
   5,  4,  7,  6,  9,  8, 11, 10,
  13, 12, 15, 14, 17, 16, 35, 34,
  /* CSR, PC */
  3, 2,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1
};

#else
static int tic6x_regmap_c64xp[] = {
  /* A0 - A15 */
  52, 53, 54, 55, 56, 57, 58, 59,
  60, 61, 62, 63, 64, 65, 66, 67,
  /* B0 - B15 */
  22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 68, 69,
  /* CSR PC */
   4,  5,
  /* A16 - A31 */
  36, 37, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 49, 50, 51,
  /* B16 -B31 */
   6,  7,  8,  9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 31,
  /* TSR, ILC, RILC */
  0,  3, 2
};

static int tic6x_regmap_c64x[] = {
  /* A0 - A15 */
  50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65,
  /* B0 - B15 */
  20, 21, 22, 23, 24, 25, 26, 27,
  28, 29, 30, 31, 32, 33, 66, 67,
  /* CSR PC */
  2,  3,
  /* A16 - A31 */
  34, 35, 36, 37, 38, 39, 40, 41,
  42, 43, 44, 45, 46, 47, 48, 49,
  /* B16 - B31 */
  4,  5,  6,  7,  8,  9,  10, 11,
  12, 13, 14, 15, 16, 17, 18, 19,
  -1, -1, -1
};

static int tic6x_regmap_c62x[] = {
  /* A0 - A15 */
  18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, 28, 29, 30, 31, 32, 33,
  /* B0 - B15 */
  4,  5,  6,  7,  8,  9, 10, 11,
  12, 13, 14, 15, 16, 17, 34, 35,
  /* CSR PC */
  2,  3,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1
};

#endif

static int *tic6x_regmap;
static unsigned int tic6x_breakpoint;
#define tic6x_breakpoint_len 4

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
tic6x_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = tic6x_breakpoint_len;
  return (const gdb_byte *) &tic6x_breakpoint;
}

static struct usrregs_info tic6x_usrregs_info =
  {
    TIC6X_NUM_REGS,
    NULL, /* Set in tic6x_read_description.  */
  };

static const struct target_desc *
tic6x_read_description (enum c6x_feature feature)
{
  static target_desc *tdescs[C6X_LAST] = { };
  struct target_desc **tdesc = &tdescs[feature];

  if (*tdesc == NULL)
    {
      *tdesc = tic6x_create_target_description (feature);
      static const char *expedite_regs[] = { "A15", "PC", NULL };
      init_target_desc (*tdesc, expedite_regs);
    }

  return *tdesc;
}

bool
tic6x_target::low_cannot_fetch_register (int regno)
{
  return (tic6x_regmap[regno] == -1);
}

bool
tic6x_target::low_cannot_store_register (int regno)
{
  return (tic6x_regmap[regno] == -1);
}

bool
tic6x_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
tic6x_target::low_get_pc (regcache *regcache)
{
  union tic6x_register pc;

  collect_register_by_name (regcache, "PC", pc.buf);
  return pc.reg32;
}

void
tic6x_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  union tic6x_register newpc;

  newpc.reg32 = pc;
  supply_register_by_name (regcache, "PC", newpc.buf);
}

bool
tic6x_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  read_memory (where, (unsigned char *) &insn, 4);
  if (insn == tic6x_breakpoint)
    return true;

  /* If necessary, recognize more trap instructions here.  GDB only uses the
     one.  */
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

static void
tic6x_collect_register (struct regcache *regcache, int regno,
			union tic6x_register *reg)
{
  union tic6x_register tmp_reg;

  collect_register (regcache, regno, &tmp_reg.reg32);
  reg->reg32 = tmp_reg.reg32;
}

static void
tic6x_supply_register (struct regcache *regcache, int regno,
		       const union tic6x_register *reg)
{
  int offset = 0;

  supply_register (regcache, regno, reg->buf + offset);
}

static void
tic6x_fill_gregset (struct regcache *regcache, void *buf)
{
  auto regset = static_cast<union tic6x_register *> (buf);
  int i;

  for (i = 0; i < TIC6X_NUM_REGS; i++)
    if (tic6x_regmap[i] != -1)
      tic6x_collect_register (regcache, i, regset + tic6x_regmap[i]);
}

static void
tic6x_store_gregset (struct regcache *regcache, const void *buf)
{
  const auto regset = static_cast<const union tic6x_register *> (buf);
  int i;

  for (i = 0; i < TIC6X_NUM_REGS; i++)
    if (tic6x_regmap[i] != -1)
      tic6x_supply_register (regcache, i, regset + tic6x_regmap[i]);
}

static struct regset_info tic6x_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, TIC6X_NUM_REGS * 4, GENERAL_REGS,
    tic6x_fill_gregset, tic6x_store_gregset },
  NULL_REGSET
};

void
tic6x_target::low_arch_setup ()
{
  register unsigned int csr asm ("B2");
  unsigned int cpuid;
  enum c6x_feature feature = C6X_CORE;

  /* Determine the CPU we're running on to find the register order.  */
  __asm__ ("MVC .S2 CSR,%0" : "=r" (csr) :);
  cpuid = csr >> 24;
  switch (cpuid)
    {
    case 0x00: /* C62x */
    case 0x02: /* C67x */
      tic6x_regmap = tic6x_regmap_c62x;
      tic6x_breakpoint = 0x0000a122;  /* BNOP .S2 0,5 */
      feature = C6X_CORE;
      break;
    case 0x03: /* C67x+ */
      tic6x_regmap = tic6x_regmap_c64x;
      tic6x_breakpoint = 0x0000a122;  /* BNOP .S2 0,5 */
      feature = C6X_GP;
      break;
    case 0x0c: /* C64x */
      tic6x_regmap = tic6x_regmap_c64x;
      tic6x_breakpoint = 0x0000a122;  /* BNOP .S2 0,5 */
      feature = C6X_GP;
      break;
    case 0x10: /* C64x+ */
    case 0x14: /* C674x */
    case 0x15: /* C66x */
      tic6x_regmap = tic6x_regmap_c64xp;
      tic6x_breakpoint = 0x56454314;  /* illegal opcode */
      feature = C6X_C6XP;
      break;
    default:
      error ("Unknown CPU ID 0x%02x", cpuid);
    }
  tic6x_usrregs_info.regmap = tic6x_regmap;

  current_process ()->tdesc = tic6x_read_description (feature);
}

static struct regsets_info tic6x_regsets_info =
  {
    tic6x_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct regs_info myregs_info =
  {
    NULL, /* regset_bitmap */
    &tic6x_usrregs_info,
    &tic6x_regsets_info
  };

const regs_info *
tic6x_target::get_regs_info ()
{
  return &myregs_info;
}

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"

namespace selftests {
namespace tdesc {
static void
tic6x_tdesc_test ()
{
  SELF_CHECK (*tdesc_tic6x_c62x_linux == *tic6x_read_description (C6X_CORE));
  SELF_CHECK (*tdesc_tic6x_c64x_linux == *tic6x_read_description (C6X_GP));
  SELF_CHECK (*tdesc_tic6x_c64xp_linux == *tic6x_read_description (C6X_C6XP));
}
}
}
#endif

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_tic6x_target;

void
initialize_low_arch (void)
{
#if GDB_SELF_TEST
  /* Initialize the Linux target descriptions.  */
  init_registers_tic6x_c64xp_linux ();
  init_registers_tic6x_c64x_linux ();
  init_registers_tic6x_c62x_linux ();

  selftests::register_test ("tic6x-tdesc", selftests::tdesc::tic6x_tdesc_test);
#endif

  initialize_regsets_info (&tic6x_regsets_info);
}

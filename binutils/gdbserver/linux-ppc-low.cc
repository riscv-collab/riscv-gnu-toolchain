/* GNU/Linux/PowerPC specific low level interface, for the remote server for
   GDB.
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

#include "elf/common.h"
#include <sys/uio.h>
#include <elf.h>
#include <asm/ptrace.h>

#include "arch/ppc-linux-common.h"
#include "arch/ppc-linux-tdesc.h"
#include "nat/ppc-linux.h"
#include "nat/linux-ptrace.h"
#include "linux-ppc-tdesc-init.h"
#include "ax.h"
#include "tracepoint.h"

#define PPC_FIELD(value, from, len) \
	(((value) >> (32 - (from) - (len))) & ((1 << (len)) - 1))
#define PPC_SEXT(v, bs) \
	((((CORE_ADDR) (v) & (((CORE_ADDR) 1 << (bs)) - 1)) \
	  ^ ((CORE_ADDR) 1 << ((bs) - 1))) \
	 - ((CORE_ADDR) 1 << ((bs) - 1)))
#define PPC_OP6(insn)	PPC_FIELD (insn, 0, 6)
#define PPC_BO(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_LI(insn)	(PPC_SEXT (PPC_FIELD (insn, 6, 24), 24) << 2)
#define PPC_BD(insn)	(PPC_SEXT (PPC_FIELD (insn, 16, 14), 14) << 2)

/* Linux target op definitions for the PowerPC architecture.  */

class ppc_target : public linux_process_target
{
public:

  const regs_info *get_regs_info () override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

  bool supports_z_point_type (char z_type) override;


  void low_collect_ptrace_register (regcache *regcache, int regno,
				    char *buf) override;

  void low_supply_ptrace_register (regcache *regcache, int regno,
				   const char *buf) override;

  bool supports_tracepoints () override;

  bool supports_fast_tracepoints () override;

  int install_fast_tracepoint_jump_pad
    (CORE_ADDR tpoint, CORE_ADDR tpaddr, CORE_ADDR collector,
     CORE_ADDR lockaddr, ULONGEST orig_size, CORE_ADDR *jump_entry,
     CORE_ADDR *trampoline, ULONGEST *trampoline_size,
     unsigned char *jjump_pad_insn, ULONGEST *jjump_pad_insn_size,
     CORE_ADDR *adjusted_insn_addr, CORE_ADDR *adjusted_insn_addr_end,
     char *err) override;

  int get_min_fast_tracepoint_insn_len () override;

  struct emit_ops *emit_ops () override;

  int get_ipa_tdesc_idx () override;

protected:

  void low_arch_setup () override;

  bool low_cannot_fetch_register (int regno) override;

  bool low_cannot_store_register (int regno) override;

  bool low_supports_breakpoints () override;

  CORE_ADDR low_get_pc (regcache *regcache) override;

  void low_set_pc (regcache *regcache, CORE_ADDR newpc) override;

  bool low_breakpoint_at (CORE_ADDR pc) override;

  int low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  int low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  int low_get_thread_area (int lwpid, CORE_ADDR *addrp) override;
};

/* The singleton target ops object.  */

static ppc_target the_ppc_target;

/* Holds the AT_HWCAP auxv entry.  */

static unsigned long ppc_hwcap;

/* Holds the AT_HWCAP2 auxv entry.  */

static unsigned long ppc_hwcap2;


#define ppc_num_regs 73

#ifdef __powerpc64__
/* We use a constant for FPSCR instead of PT_FPSCR, because
   many shipped PPC64 kernels had the wrong value in ptrace.h.  */
static int ppc_regmap[] =
 {PT_R0 * 8,     PT_R1 * 8,     PT_R2 * 8,     PT_R3 * 8,
  PT_R4 * 8,     PT_R5 * 8,     PT_R6 * 8,     PT_R7 * 8,
  PT_R8 * 8,     PT_R9 * 8,     PT_R10 * 8,    PT_R11 * 8,
  PT_R12 * 8,    PT_R13 * 8,    PT_R14 * 8,    PT_R15 * 8,
  PT_R16 * 8,    PT_R17 * 8,    PT_R18 * 8,    PT_R19 * 8,
  PT_R20 * 8,    PT_R21 * 8,    PT_R22 * 8,    PT_R23 * 8,
  PT_R24 * 8,    PT_R25 * 8,    PT_R26 * 8,    PT_R27 * 8,
  PT_R28 * 8,    PT_R29 * 8,    PT_R30 * 8,    PT_R31 * 8,
  PT_FPR0*8,     PT_FPR0*8 + 8, PT_FPR0*8+16,  PT_FPR0*8+24,
  PT_FPR0*8+32,  PT_FPR0*8+40,  PT_FPR0*8+48,  PT_FPR0*8+56,
  PT_FPR0*8+64,  PT_FPR0*8+72,  PT_FPR0*8+80,  PT_FPR0*8+88,
  PT_FPR0*8+96,  PT_FPR0*8+104,  PT_FPR0*8+112,  PT_FPR0*8+120,
  PT_FPR0*8+128, PT_FPR0*8+136,  PT_FPR0*8+144,  PT_FPR0*8+152,
  PT_FPR0*8+160,  PT_FPR0*8+168,  PT_FPR0*8+176,  PT_FPR0*8+184,
  PT_FPR0*8+192,  PT_FPR0*8+200,  PT_FPR0*8+208,  PT_FPR0*8+216,
  PT_FPR0*8+224,  PT_FPR0*8+232,  PT_FPR0*8+240,  PT_FPR0*8+248,
  PT_NIP * 8,    PT_MSR * 8,    PT_CCR * 8,    PT_LNK * 8,
  PT_CTR * 8,    PT_XER * 8,    PT_FPR0*8 + 256,
  PT_ORIG_R3 * 8, PT_TRAP * 8 };
#else
/* Currently, don't check/send MQ.  */
static int ppc_regmap[] =
 {PT_R0 * 4,     PT_R1 * 4,     PT_R2 * 4,     PT_R3 * 4,
  PT_R4 * 4,     PT_R5 * 4,     PT_R6 * 4,     PT_R7 * 4,
  PT_R8 * 4,     PT_R9 * 4,     PT_R10 * 4,    PT_R11 * 4,
  PT_R12 * 4,    PT_R13 * 4,    PT_R14 * 4,    PT_R15 * 4,
  PT_R16 * 4,    PT_R17 * 4,    PT_R18 * 4,    PT_R19 * 4,
  PT_R20 * 4,    PT_R21 * 4,    PT_R22 * 4,    PT_R23 * 4,
  PT_R24 * 4,    PT_R25 * 4,    PT_R26 * 4,    PT_R27 * 4,
  PT_R28 * 4,    PT_R29 * 4,    PT_R30 * 4,    PT_R31 * 4,
  PT_FPR0*4,     PT_FPR0*4 + 8, PT_FPR0*4+16,  PT_FPR0*4+24,
  PT_FPR0*4+32,  PT_FPR0*4+40,  PT_FPR0*4+48,  PT_FPR0*4+56,
  PT_FPR0*4+64,  PT_FPR0*4+72,  PT_FPR0*4+80,  PT_FPR0*4+88,
  PT_FPR0*4+96,  PT_FPR0*4+104,  PT_FPR0*4+112,  PT_FPR0*4+120,
  PT_FPR0*4+128, PT_FPR0*4+136,  PT_FPR0*4+144,  PT_FPR0*4+152,
  PT_FPR0*4+160,  PT_FPR0*4+168,  PT_FPR0*4+176,  PT_FPR0*4+184,
  PT_FPR0*4+192,  PT_FPR0*4+200,  PT_FPR0*4+208,  PT_FPR0*4+216,
  PT_FPR0*4+224,  PT_FPR0*4+232,  PT_FPR0*4+240,  PT_FPR0*4+248,
  PT_NIP * 4,    PT_MSR * 4,    PT_CCR * 4,    PT_LNK * 4,
  PT_CTR * 4,    PT_XER * 4,    PT_FPSCR * 4,
  PT_ORIG_R3 * 4, PT_TRAP * 4
 };

static int ppc_regmap_e500[] =
 {PT_R0 * 4,     PT_R1 * 4,     PT_R2 * 4,     PT_R3 * 4,
  PT_R4 * 4,     PT_R5 * 4,     PT_R6 * 4,     PT_R7 * 4,
  PT_R8 * 4,     PT_R9 * 4,     PT_R10 * 4,    PT_R11 * 4,
  PT_R12 * 4,    PT_R13 * 4,    PT_R14 * 4,    PT_R15 * 4,
  PT_R16 * 4,    PT_R17 * 4,    PT_R18 * 4,    PT_R19 * 4,
  PT_R20 * 4,    PT_R21 * 4,    PT_R22 * 4,    PT_R23 * 4,
  PT_R24 * 4,    PT_R25 * 4,    PT_R26 * 4,    PT_R27 * 4,
  PT_R28 * 4,    PT_R29 * 4,    PT_R30 * 4,    PT_R31 * 4,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  -1,            -1,            -1,            -1,
  PT_NIP * 4,    PT_MSR * 4,    PT_CCR * 4,    PT_LNK * 4,
  PT_CTR * 4,    PT_XER * 4,    -1,
  PT_ORIG_R3 * 4, PT_TRAP * 4
 };
#endif

/* Check whether the kernel provides a register set with number
   REGSET_ID of size REGSETSIZE for process/thread TID.  */

static int
ppc_check_regset (int tid, int regset_id, int regsetsize)
{
  void *buf = alloca (regsetsize);
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = regsetsize;

  if (ptrace (PTRACE_GETREGSET, tid, regset_id, &iov) >= 0
      || errno == ENODATA)
    return 1;
  return 0;
}

bool
ppc_target::low_cannot_store_register (int regno)
{
  const struct target_desc *tdesc = current_process ()->tdesc;

#ifndef __powerpc64__
  /* Some kernels do not allow us to store fpscr.  */
  if (!(ppc_hwcap & PPC_FEATURE_HAS_SPE)
      && regno == find_regno (tdesc, "fpscr"))
    return true;
#endif

  /* Some kernels do not allow us to store orig_r3 or trap.  */
  if (regno == find_regno (tdesc, "orig_r3")
      || regno == find_regno (tdesc, "trap"))
    return true;

  return false;
}

bool
ppc_target::low_cannot_fetch_register (int regno)
{
  return false;
}

void
ppc_target::low_collect_ptrace_register (regcache *regcache, int regno,
					 char *buf)
{
  memset (buf, 0, sizeof (long));

  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
      /* Little-endian values always sit at the left end of the buffer.  */
      collect_register (regcache, regno, buf);
    }
  else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
      /* Big-endian values sit at the right end of the buffer.  In case of
	 registers whose sizes are smaller than sizeof (long), we must use a
	 padding to access them correctly.  */
      int size = register_size (regcache->tdesc, regno);

      if (size < sizeof (long))
	collect_register (regcache, regno, buf + sizeof (long) - size);
      else
	collect_register (regcache, regno, buf);
    }
  else
    perror_with_name ("Unexpected byte order");
}

void
ppc_target::low_supply_ptrace_register (regcache *regcache, int regno,
					const char *buf)
{
  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
      /* Little-endian values always sit at the left end of the buffer.  */
      supply_register (regcache, regno, buf);
    }
  else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
      /* Big-endian values sit at the right end of the buffer.  In case of
	 registers whose sizes are smaller than sizeof (long), we must use a
	 padding to access them correctly.  */
      int size = register_size (regcache->tdesc, regno);

      if (size < sizeof (long))
	supply_register (regcache, regno, buf + sizeof (long) - size);
      else
	supply_register (regcache, regno, buf);
    }
  else
    perror_with_name ("Unexpected byte order");
}

bool
ppc_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
ppc_target::low_get_pc (regcache *regcache)
{
  if (register_size (regcache->tdesc, 0) == 4)
    {
      unsigned int pc;
      collect_register_by_name (regcache, "pc", &pc);
      return (CORE_ADDR) pc;
    }
  else
    {
      unsigned long pc;
      collect_register_by_name (regcache, "pc", &pc);
      return (CORE_ADDR) pc;
    }
}

void
ppc_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  if (register_size (regcache->tdesc, 0) == 4)
    {
      unsigned int newpc = pc;
      supply_register_by_name (regcache, "pc", &newpc);
    }
  else
    {
      unsigned long newpc = pc;
      supply_register_by_name (regcache, "pc", &newpc);
    }
}

#ifndef __powerpc64__
static int ppc_regmap_adjusted;
#endif


/* Correct in either endianness.
   This instruction is "twge r2, r2", which GDB uses as a software
   breakpoint.  */
static const unsigned int ppc_breakpoint = 0x7d821008;
#define ppc_breakpoint_len 4

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
ppc_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = ppc_breakpoint_len;
  return (const gdb_byte *) &ppc_breakpoint;
}

bool
ppc_target::low_breakpoint_at (CORE_ADDR where)
{
  unsigned int insn;

  read_memory (where, (unsigned char *) &insn, 4);
  if (insn == ppc_breakpoint)
    return true;
  /* If necessary, recognize more trap instructions here.  GDB only uses
     the one.  */

  return false;
}

/* Implement supports_z_point_type target-ops.
   Returns true if type Z_TYPE breakpoint is supported.

   Handling software breakpoint at server side, so tracepoints
   and breakpoints can be inserted at the same location.  */

bool
ppc_target::supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
      return true;
    case Z_PACKET_HW_BP:
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_ACCESS_WP:
    default:
      return false;
    }
}

/* Implement the low_insert_point linux target op.
   Returns 0 on success, -1 on failure and 1 on unsupported.  */

int
ppc_target::low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
			      int size, raw_breakpoint *bp)
{
  switch (type)
    {
    case raw_bkpt_type_sw:
      return insert_memory_breakpoint (bp);

    case raw_bkpt_type_hw:
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_access_wp:
    default:
      /* Unsupported.  */
      return 1;
    }
}

/* Implement the low_remove_point linux target op.
   Returns 0 on success, -1 on failure and 1 on unsupported.  */

int
ppc_target::low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
			      int size, raw_breakpoint *bp)
{
  switch (type)
    {
    case raw_bkpt_type_sw:
      return remove_memory_breakpoint (bp);

    case raw_bkpt_type_hw:
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_access_wp:
    default:
      /* Unsupported.  */
      return 1;
    }
}

/* Provide only a fill function for the general register set.  ps_lgetregs
   will use this for NPTL support.  */

static void ppc_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  ppc_target *my_ppc_target = (ppc_target *) the_linux_target;

  for (i = 0; i < 32; i++)
    my_ppc_target->low_collect_ptrace_register (regcache, i,
						(char *) buf + ppc_regmap[i]);

  for (i = 64; i < 70; i++)
    my_ppc_target->low_collect_ptrace_register (regcache, i,
						(char *) buf + ppc_regmap[i]);

  for (i = 71; i < 73; i++)
    my_ppc_target->low_collect_ptrace_register (regcache, i,
						(char *) buf + ppc_regmap[i]);
}

/* Program Priority Register regset fill function.  */

static void
ppc_fill_pprregset (struct regcache *regcache, void *buf)
{
  char *ppr = (char *) buf;

  collect_register_by_name (regcache, "ppr", ppr);
}

/* Program Priority Register regset store function.  */

static void
ppc_store_pprregset (struct regcache *regcache, const void *buf)
{
  const char *ppr = (const char *) buf;

  supply_register_by_name (regcache, "ppr", ppr);
}

/* Data Stream Control Register regset fill function.  */

static void
ppc_fill_dscrregset (struct regcache *regcache, void *buf)
{
  char *dscr = (char *) buf;

  collect_register_by_name (regcache, "dscr", dscr);
}

/* Data Stream Control Register regset store function.  */

static void
ppc_store_dscrregset (struct regcache *regcache, const void *buf)
{
  const char *dscr = (const char *) buf;

  supply_register_by_name (regcache, "dscr", dscr);
}

/* Target Address Register regset fill function.  */

static void
ppc_fill_tarregset (struct regcache *regcache, void *buf)
{
  char *tar = (char *) buf;

  collect_register_by_name (regcache, "tar", tar);
}

/* Target Address Register regset store function.  */

static void
ppc_store_tarregset (struct regcache *regcache, const void *buf)
{
  const char *tar = (const char *) buf;

  supply_register_by_name (regcache, "tar", tar);
}

/* Event-Based Branching regset store function.  Unless the inferior
   has a perf event open, ptrace can return in error when reading and
   writing to the regset, with ENODATA.  For reading, the registers
   will correctly show as unavailable.  For writing, gdbserver
   currently only caches any register writes from P and G packets and
   the stub always tries to write all the regsets when resuming the
   inferior, which would result in frequent warnings.  For this
   reason, we don't define a fill function.  This also means that the
   client-side regcache will be dirty if the user tries to write to
   the EBB registers.  G packets that the client sends to write to
   unrelated registers will also include data for EBB registers, even
   if they are unavailable.  */

static void
ppc_store_ebbregset (struct regcache *regcache, const void *buf)
{
  const char *regset = (const char *) buf;

  /* The order in the kernel regset is: EBBRR, EBBHR, BESCR.  In the
     .dat file is BESCR, EBBHR, EBBRR.  */
  supply_register_by_name (regcache, "ebbrr", &regset[0]);
  supply_register_by_name (regcache, "ebbhr", &regset[8]);
  supply_register_by_name (regcache, "bescr", &regset[16]);
}

/* Performance Monitoring Unit regset fill function.  */

static void
ppc_fill_pmuregset (struct regcache *regcache, void *buf)
{
  char *regset = (char *) buf;

  /* The order in the kernel regset is SIAR, SDAR, SIER, MMCR2, MMCR0.
     In the .dat file is MMCR0, MMCR2, SIAR, SDAR, SIER.  */
  collect_register_by_name (regcache, "siar", &regset[0]);
  collect_register_by_name (regcache, "sdar", &regset[8]);
  collect_register_by_name (regcache, "sier", &regset[16]);
  collect_register_by_name (regcache, "mmcr2", &regset[24]);
  collect_register_by_name (regcache, "mmcr0", &regset[32]);
}

/* Performance Monitoring Unit regset store function.  */

static void
ppc_store_pmuregset (struct regcache *regcache, const void *buf)
{
  const char *regset = (const char *) buf;

  supply_register_by_name (regcache, "siar", &regset[0]);
  supply_register_by_name (regcache, "sdar", &regset[8]);
  supply_register_by_name (regcache, "sier", &regset[16]);
  supply_register_by_name (regcache, "mmcr2", &regset[24]);
  supply_register_by_name (regcache, "mmcr0", &regset[32]);
}

/* Hardware Transactional Memory special-purpose register regset fill
   function.  */

static void
ppc_fill_tm_sprregset (struct regcache *regcache, void *buf)
{
  int i, base;
  char *regset = (char *) buf;

  base = find_regno (regcache->tdesc, "tfhar");
  for (i = 0; i < 3; i++)
    collect_register (regcache, base + i, &regset[i * 8]);
}

/* Hardware Transactional Memory special-purpose register regset store
   function.  */

static void
ppc_store_tm_sprregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = (const char *) buf;

  base = find_regno (regcache->tdesc, "tfhar");
  for (i = 0; i < 3; i++)
    supply_register (regcache, base + i, &regset[i * 8]);
}

/* For the same reasons as the EBB regset, none of the HTM
   checkpointed regsets have a fill function.  These registers are
   only available if the inferior is in a transaction.  */

/* Hardware Transactional Memory checkpointed general-purpose regset
   store function.  */

static void
ppc_store_tm_cgprregset (struct regcache *regcache, const void *buf)
{
  int i, base, size, endian_offset;
  const char *regset = (const char *) buf;

  base = find_regno (regcache->tdesc, "cr0");
  size = register_size (regcache->tdesc, base);

  gdb_assert (size == 4 || size == 8);

  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * size]);

  endian_offset = 0;

  if ((size == 8) && (__BYTE_ORDER == __BIG_ENDIAN))
    endian_offset = 4;

  supply_register_by_name (regcache, "ccr",
			   &regset[PT_CCR * size + endian_offset]);

  supply_register_by_name (regcache, "cxer",
			   &regset[PT_XER * size + endian_offset]);

  supply_register_by_name (regcache, "clr", &regset[PT_LNK * size]);
  supply_register_by_name (regcache, "cctr", &regset[PT_CTR * size]);
}

/* Hardware Transactional Memory checkpointed floating-point regset
   store function.  */

static void
ppc_store_tm_cfprregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = (const char *) buf;

  base = find_regno (regcache->tdesc, "cf0");

  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 8]);

  supply_register_by_name (regcache, "cfpscr", &regset[32 * 8]);
}

/* Hardware Transactional Memory checkpointed vector regset store
   function.  */

static void
ppc_store_tm_cvrregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = (const char *) buf;
  int vscr_offset = 0;

  base = find_regno (regcache->tdesc, "cvr0");

  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 16]);

  if (__BYTE_ORDER == __BIG_ENDIAN)
    vscr_offset = 12;

  supply_register_by_name (regcache, "cvscr",
			   &regset[32 * 16 + vscr_offset]);

  supply_register_by_name (regcache, "cvrsave", &regset[33 * 16]);
}

/* Hardware Transactional Memory checkpointed vector-scalar regset
   store function.  */

static void
ppc_store_tm_cvsxregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = (const char *) buf;

  base = find_regno (regcache->tdesc, "cvs0h");
  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 8]);
}

/* Hardware Transactional Memory checkpointed Program Priority
   Register regset store function.  */

static void
ppc_store_tm_cpprregset (struct regcache *regcache, const void *buf)
{
  const char *cppr = (const char *) buf;

  supply_register_by_name (regcache, "cppr", cppr);
}

/* Hardware Transactional Memory checkpointed Data Stream Control
   Register regset store function.  */

static void
ppc_store_tm_cdscrregset (struct regcache *regcache, const void *buf)
{
  const char *cdscr = (const char *) buf;

  supply_register_by_name (regcache, "cdscr", cdscr);
}

/* Hardware Transactional Memory checkpointed Target Address Register
   regset store function.  */

static void
ppc_store_tm_ctarregset (struct regcache *regcache, const void *buf)
{
  const char *ctar = (const char *) buf;

  supply_register_by_name (regcache, "ctar", ctar);
}

static void
ppc_fill_vsxregset (struct regcache *regcache, void *buf)
{
  int i, base;
  char *regset = (char *) buf;

  base = find_regno (regcache->tdesc, "vs0h");
  for (i = 0; i < 32; i++)
    collect_register (regcache, base + i, &regset[i * 8]);
}

static void
ppc_store_vsxregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = (const char *) buf;

  base = find_regno (regcache->tdesc, "vs0h");
  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 8]);
}

static void
ppc_fill_vrregset (struct regcache *regcache, void *buf)
{
  int i, base;
  char *regset = (char *) buf;
  int vscr_offset = 0;

  base = find_regno (regcache->tdesc, "vr0");
  for (i = 0; i < 32; i++)
    collect_register (regcache, base + i, &regset[i * 16]);

  if (__BYTE_ORDER == __BIG_ENDIAN)
    vscr_offset = 12;

  collect_register_by_name (regcache, "vscr",
			    &regset[32 * 16 + vscr_offset]);

  collect_register_by_name (regcache, "vrsave", &regset[33 * 16]);
}

static void
ppc_store_vrregset (struct regcache *regcache, const void *buf)
{
  int i, base;
  const char *regset = (const char *) buf;
  int vscr_offset = 0;

  base = find_regno (regcache->tdesc, "vr0");
  for (i = 0; i < 32; i++)
    supply_register (regcache, base + i, &regset[i * 16]);

  if (__BYTE_ORDER == __BIG_ENDIAN)
    vscr_offset = 12;

  supply_register_by_name (regcache, "vscr",
			   &regset[32 * 16 + vscr_offset]);
  supply_register_by_name (regcache, "vrsave", &regset[33 * 16]);
}

struct gdb_evrregset_t
{
  unsigned long evr[32];
  unsigned long long acc;
  unsigned long spefscr;
};

static void
ppc_fill_evrregset (struct regcache *regcache, void *buf)
{
  int i, ev0;
  struct gdb_evrregset_t *regset = (struct gdb_evrregset_t *) buf;

  ev0 = find_regno (regcache->tdesc, "ev0h");
  for (i = 0; i < 32; i++)
    collect_register (regcache, ev0 + i, &regset->evr[i]);

  collect_register_by_name (regcache, "acc", &regset->acc);
  collect_register_by_name (regcache, "spefscr", &regset->spefscr);
}

static void
ppc_store_evrregset (struct regcache *regcache, const void *buf)
{
  int i, ev0;
  const struct gdb_evrregset_t *regset = (const struct gdb_evrregset_t *) buf;

  ev0 = find_regno (regcache->tdesc, "ev0h");
  for (i = 0; i < 32; i++)
    supply_register (regcache, ev0 + i, &regset->evr[i]);

  supply_register_by_name (regcache, "acc", &regset->acc);
  supply_register_by_name (regcache, "spefscr", &regset->spefscr);
}

static struct regset_info ppc_regsets[] = {
  /* List the extra register sets before GENERAL_REGS.  That way we will
     fetch them every time, but still fall back to PTRACE_PEEKUSER for the
     general registers.  Some kernels support these, but not the newer
     PPC_PTRACE_GETREGS.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CTAR, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_ctarregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CDSCR, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_cdscrregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CPPR, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_cpprregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CVSX, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_cvsxregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CVMX, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_cvrregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CFPR, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_cfprregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_CGPR, 0, EXTENDED_REGS,
    NULL, ppc_store_tm_cgprregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TM_SPR, 0, EXTENDED_REGS,
    ppc_fill_tm_sprregset, ppc_store_tm_sprregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_EBB, 0, EXTENDED_REGS,
    NULL, ppc_store_ebbregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_PMU, 0, EXTENDED_REGS,
    ppc_fill_pmuregset, ppc_store_pmuregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_TAR, 0, EXTENDED_REGS,
    ppc_fill_tarregset, ppc_store_tarregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_PPR, 0, EXTENDED_REGS,
    ppc_fill_pprregset, ppc_store_pprregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PPC_DSCR, 0, EXTENDED_REGS,
    ppc_fill_dscrregset, ppc_store_dscrregset },
  { PTRACE_GETVSXREGS, PTRACE_SETVSXREGS, 0, 0, EXTENDED_REGS,
  ppc_fill_vsxregset, ppc_store_vsxregset },
  { PTRACE_GETVRREGS, PTRACE_SETVRREGS, 0, 0, EXTENDED_REGS,
    ppc_fill_vrregset, ppc_store_vrregset },
  { PTRACE_GETEVRREGS, PTRACE_SETEVRREGS, 0, 0, EXTENDED_REGS,
    ppc_fill_evrregset, ppc_store_evrregset },
  { 0, 0, 0, 0, GENERAL_REGS, ppc_fill_gregset, NULL },
  NULL_REGSET
};

static struct usrregs_info ppc_usrregs_info =
  {
    ppc_num_regs,
    ppc_regmap,
  };

static struct regsets_info ppc_regsets_info =
  {
    ppc_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct regs_info myregs_info =
  {
    NULL, /* regset_bitmap */
    &ppc_usrregs_info,
    &ppc_regsets_info
  };

const regs_info *
ppc_target::get_regs_info ()
{
  return &myregs_info;
}

void
ppc_target::low_arch_setup ()
{
  const struct target_desc *tdesc;
  struct regset_info *regset;
  struct ppc_linux_features features = ppc_linux_no_features;

  int tid = lwpid_of (current_thread);

  features.wordsize = ppc_linux_target_wordsize (tid);

  if (features.wordsize == 4)
      tdesc = tdesc_powerpc_32l;
  else
      tdesc = tdesc_powerpc_64l;

  current_process ()->tdesc = tdesc;

  /* The value of current_process ()->tdesc needs to be set for this
     call.  */
  ppc_hwcap = linux_get_hwcap (current_thread->id.pid (), features.wordsize);
  ppc_hwcap2 = linux_get_hwcap2 (current_thread->id.pid (), features.wordsize);

  features.isa205 = ppc_linux_has_isa205 (ppc_hwcap);

  if (ppc_hwcap & PPC_FEATURE_HAS_VSX)
    features.vsx = true;

  if (ppc_hwcap & PPC_FEATURE_HAS_ALTIVEC)
    features.altivec = true;

  if ((ppc_hwcap2 & PPC_FEATURE2_DSCR)
      && ppc_check_regset (tid, NT_PPC_DSCR, PPC_LINUX_SIZEOF_DSCRREGSET)
      && ppc_check_regset (tid, NT_PPC_PPR, PPC_LINUX_SIZEOF_PPRREGSET))
    {
      features.ppr_dscr = true;
      if ((ppc_hwcap2 & PPC_FEATURE2_ARCH_2_07)
	  && (ppc_hwcap2 & PPC_FEATURE2_TAR)
	  && (ppc_hwcap2 & PPC_FEATURE2_EBB)
	  && ppc_check_regset (tid, NT_PPC_TAR,
			       PPC_LINUX_SIZEOF_TARREGSET)
	  && ppc_check_regset (tid, NT_PPC_EBB,
			       PPC_LINUX_SIZEOF_EBBREGSET)
	  && ppc_check_regset (tid, NT_PPC_PMU,
			       PPC_LINUX_SIZEOF_PMUREGSET))
	{
	  features.isa207 = true;
	  if ((ppc_hwcap2 & PPC_FEATURE2_HTM)
	      && ppc_check_regset (tid, NT_PPC_TM_SPR,
				   PPC_LINUX_SIZEOF_TM_SPRREGSET))
	    features.htm = true;
	}
    }

  tdesc = ppc_linux_match_description (features);

  /* On 32-bit machines, check for SPE registers.
     Set the low target's regmap field as appropriately.  */
#ifndef __powerpc64__
  if (ppc_hwcap & PPC_FEATURE_HAS_SPE)
    tdesc = tdesc_powerpc_e500l;

  if (!ppc_regmap_adjusted)
    {
      if (ppc_hwcap & PPC_FEATURE_HAS_SPE)
	ppc_usrregs_info.regmap = ppc_regmap_e500;

      /* If the FPSCR is 64-bit wide, we need to fetch the whole
	 64-bit slot and not just its second word.  The PT_FPSCR
	 supplied in a 32-bit GDB compilation doesn't reflect
	 this.  */
      if (register_size (tdesc, 70) == 8)
	ppc_regmap[70] = (48 + 2*32) * sizeof (long);

      ppc_regmap_adjusted = 1;
   }
#endif

  current_process ()->tdesc = tdesc;

  for (regset = ppc_regsets; regset->size >= 0; regset++)
    switch (regset->get_request)
      {
      case PTRACE_GETVRREGS:
	regset->size = features.altivec ? PPC_LINUX_SIZEOF_VRREGSET : 0;
	break;
      case PTRACE_GETVSXREGS:
	regset->size = features.vsx ? PPC_LINUX_SIZEOF_VSXREGSET : 0;
	break;
      case PTRACE_GETEVRREGS:
	if (ppc_hwcap & PPC_FEATURE_HAS_SPE)
	  regset->size = 32 * 4 + 8 + 4;
	else
	  regset->size = 0;
	break;
      case PTRACE_GETREGSET:
	switch (regset->nt_type)
	  {
	  case NT_PPC_PPR:
	    regset->size = (features.ppr_dscr ?
			    PPC_LINUX_SIZEOF_PPRREGSET : 0);
	    break;
	  case NT_PPC_DSCR:
	    regset->size = (features.ppr_dscr ?
			    PPC_LINUX_SIZEOF_DSCRREGSET : 0);
	    break;
	  case NT_PPC_TAR:
	    regset->size = (features.isa207 ?
			    PPC_LINUX_SIZEOF_TARREGSET : 0);
	    break;
	  case NT_PPC_EBB:
	    regset->size = (features.isa207 ?
			    PPC_LINUX_SIZEOF_EBBREGSET : 0);
	    break;
	  case NT_PPC_PMU:
	    regset->size = (features.isa207 ?
			    PPC_LINUX_SIZEOF_PMUREGSET : 0);
	    break;
	  case NT_PPC_TM_SPR:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_TM_SPRREGSET : 0);
	    break;
	  case NT_PPC_TM_CGPR:
	    if (features.wordsize == 4)
	      regset->size = (features.htm ?
			      PPC32_LINUX_SIZEOF_CGPRREGSET : 0);
	    else
	      regset->size = (features.htm ?
			      PPC64_LINUX_SIZEOF_CGPRREGSET : 0);
	    break;
	  case NT_PPC_TM_CFPR:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_CFPRREGSET : 0);
	    break;
	  case NT_PPC_TM_CVMX:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_CVMXREGSET : 0);
	    break;
	  case NT_PPC_TM_CVSX:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_CVSXREGSET : 0);
	    break;
	  case NT_PPC_TM_CPPR:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_CPPRREGSET : 0);
	    break;
	  case NT_PPC_TM_CDSCR:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_CDSCRREGSET : 0);
	    break;
	  case NT_PPC_TM_CTAR:
	    regset->size = (features.htm ?
			    PPC_LINUX_SIZEOF_CTARREGSET : 0);
	    break;
	  default:
	    break;
	  }
	break;
      default:
	break;
      }
}

/* Implementation of target ops method "supports_tracepoints".  */

bool
ppc_target::supports_tracepoints ()
{
  return true;
}

/* Get the thread area address.  This is used to recognize which
   thread is which when tracing with the in-process agent library.  We
   don't read anything from the address, and treat it as opaque; it's
   the address itself that we assume is unique per-thread.  */

int
ppc_target::low_get_thread_area (int lwpid, CORE_ADDR *addr)
{
  struct lwp_info *lwp = find_lwp_pid (ptid_t (lwpid));
  struct thread_info *thr = get_lwp_thread (lwp);
  struct regcache *regcache = get_thread_regcache (thr, 1);
  ULONGEST tp = 0;

#ifdef __powerpc64__
  if (register_size (regcache->tdesc, 0) == 8)
    collect_register_by_name (regcache, "r13", &tp);
  else
#endif
    collect_register_by_name (regcache, "r2", &tp);

  *addr = tp;

  return 0;
}

#ifdef __powerpc64__

/* Older glibc doesn't provide this.  */

#ifndef EF_PPC64_ABI
#define EF_PPC64_ABI 3
#endif

/* Returns 1 if inferior is using ELFv2 ABI.  Undefined for 32-bit
   inferiors.  */

static int
is_elfv2_inferior (void)
{
  /* To be used as fallback if we're unable to determine the right result -
     assume inferior uses the same ABI as gdbserver.  */
#if _CALL_ELF == 2
  const int def_res = 1;
#else
  const int def_res = 0;
#endif
  CORE_ADDR phdr;
  Elf64_Ehdr ehdr;

  const struct target_desc *tdesc = current_process ()->tdesc;
  int wordsize = register_size (tdesc, 0);

  if (!linux_get_auxv (current_thread->id.pid (), wordsize, AT_PHDR, &phdr))
    return def_res;

  /* Assume ELF header is at the beginning of the page where program headers
     are located.  If it doesn't look like one, bail.  */

  read_inferior_memory (phdr & ~0xfff, (unsigned char *) &ehdr, sizeof ehdr);
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG))
    return def_res;

  return (ehdr.e_flags & EF_PPC64_ABI) == 2;
}

#endif

/* Generate a ds-form instruction in BUF and return the number of bytes written

   0      6     11   16          30 32
   | OPCD | RST | RA |     DS    |XO|  */

__attribute__((unused)) /* Maybe unused due to conditional compilation.  */
static int
gen_ds_form (uint32_t *buf, int opcd, int rst, int ra, int ds, int xo)
{
  uint32_t insn;

  gdb_assert ((opcd & ~0x3f) == 0);
  gdb_assert ((rst & ~0x1f) == 0);
  gdb_assert ((ra & ~0x1f) == 0);
  gdb_assert ((xo & ~0x3) == 0);

  insn = (rst << 21) | (ra << 16) | (ds & 0xfffc) | (xo & 0x3);
  *buf = (opcd << 26) | insn;
  return 1;
}

/* Followings are frequently used ds-form instructions.  */

#define GEN_STD(buf, rs, ra, offset)	gen_ds_form (buf, 62, rs, ra, offset, 0)
#define GEN_STDU(buf, rs, ra, offset)	gen_ds_form (buf, 62, rs, ra, offset, 1)
#define GEN_LD(buf, rt, ra, offset)	gen_ds_form (buf, 58, rt, ra, offset, 0)
#define GEN_LDU(buf, rt, ra, offset)	gen_ds_form (buf, 58, rt, ra, offset, 1)

/* Generate a d-form instruction in BUF.

   0      6     11   16             32
   | OPCD | RST | RA |       D      |  */

static int
gen_d_form (uint32_t *buf, int opcd, int rst, int ra, int si)
{
  uint32_t insn;

  gdb_assert ((opcd & ~0x3f) == 0);
  gdb_assert ((rst & ~0x1f) == 0);
  gdb_assert ((ra & ~0x1f) == 0);

  insn = (rst << 21) | (ra << 16) | (si & 0xffff);
  *buf = (opcd << 26) | insn;
  return 1;
}

/* Followings are frequently used d-form instructions.  */

#define GEN_ADDI(buf, rt, ra, si)	gen_d_form (buf, 14, rt, ra, si)
#define GEN_ADDIS(buf, rt, ra, si)	gen_d_form (buf, 15, rt, ra, si)
#define GEN_LI(buf, rt, si)		GEN_ADDI (buf, rt, 0, si)
#define GEN_LIS(buf, rt, si)		GEN_ADDIS (buf, rt, 0, si)
#define GEN_ORI(buf, rt, ra, si)	gen_d_form (buf, 24, rt, ra, si)
#define GEN_ORIS(buf, rt, ra, si)	gen_d_form (buf, 25, rt, ra, si)
#define GEN_LWZ(buf, rt, ra, si)	gen_d_form (buf, 32, rt, ra, si)
#define GEN_STW(buf, rt, ra, si)	gen_d_form (buf, 36, rt, ra, si)
#define GEN_STWU(buf, rt, ra, si)	gen_d_form (buf, 37, rt, ra, si)

/* Generate a xfx-form instruction in BUF and return the number of bytes
   written.

   0      6     11         21        31 32
   | OPCD | RST |    RI    |    XO   |/|  */

static int
gen_xfx_form (uint32_t *buf, int opcd, int rst, int ri, int xo)
{
  uint32_t insn;
  unsigned int n = ((ri & 0x1f) << 5) | ((ri >> 5) & 0x1f);

  gdb_assert ((opcd & ~0x3f) == 0);
  gdb_assert ((rst & ~0x1f) == 0);
  gdb_assert ((xo & ~0x3ff) == 0);

  insn = (rst << 21) | (n << 11) | (xo << 1);
  *buf = (opcd << 26) | insn;
  return 1;
}

/* Followings are frequently used xfx-form instructions.  */

#define GEN_MFSPR(buf, rt, spr)		gen_xfx_form (buf, 31, rt, spr, 339)
#define GEN_MTSPR(buf, rt, spr)		gen_xfx_form (buf, 31, rt, spr, 467)
#define GEN_MFCR(buf, rt)		gen_xfx_form (buf, 31, rt, 0, 19)
#define GEN_MTCR(buf, rt)		gen_xfx_form (buf, 31, rt, 0x3cf, 144)
#define GEN_SYNC(buf, L, E)             gen_xfx_form (buf, 31, L & 0x3, \
						      E & 0xf, 598)
#define GEN_LWSYNC(buf)			GEN_SYNC (buf, 1, 0)


/* Generate a x-form instruction in BUF and return the number of bytes written.

   0      6     11   16   21       31 32
   | OPCD | RST | RA | RB |   XO   |RC|  */

static int
gen_x_form (uint32_t *buf, int opcd, int rst, int ra, int rb, int xo, int rc)
{
  uint32_t insn;

  gdb_assert ((opcd & ~0x3f) == 0);
  gdb_assert ((rst & ~0x1f) == 0);
  gdb_assert ((ra & ~0x1f) == 0);
  gdb_assert ((rb & ~0x1f) == 0);
  gdb_assert ((xo & ~0x3ff) == 0);
  gdb_assert ((rc & ~1) == 0);

  insn = (rst << 21) | (ra << 16) | (rb << 11) | (xo << 1) | rc;
  *buf = (opcd << 26) | insn;
  return 1;
}

/* Followings are frequently used x-form instructions.  */

#define GEN_OR(buf, ra, rs, rb)		gen_x_form (buf, 31, rs, ra, rb, 444, 0)
#define GEN_MR(buf, ra, rs)		GEN_OR (buf, ra, rs, rs)
#define GEN_LWARX(buf, rt, ra, rb)	gen_x_form (buf, 31, rt, ra, rb, 20, 0)
#define GEN_STWCX(buf, rs, ra, rb)	gen_x_form (buf, 31, rs, ra, rb, 150, 1)
/* Assume bf = cr7.  */
#define GEN_CMPW(buf, ra, rb)		gen_x_form (buf, 31, 28, ra, rb, 0, 0)


/* Generate a md-form instruction in BUF and return the number of bytes written.

   0      6    11   16   21   27   30 31 32
   | OPCD | RS | RA | sh | mb | XO |sh|Rc|  */

static int
gen_md_form (uint32_t *buf, int opcd, int rs, int ra, int sh, int mb,
	     int xo, int rc)
{
  uint32_t insn;
  unsigned int n = ((mb & 0x1f) << 1) | ((mb >> 5) & 0x1);
  unsigned int sh0_4 = sh & 0x1f;
  unsigned int sh5 = (sh >> 5) & 1;

  gdb_assert ((opcd & ~0x3f) == 0);
  gdb_assert ((rs & ~0x1f) == 0);
  gdb_assert ((ra & ~0x1f) == 0);
  gdb_assert ((sh & ~0x3f) == 0);
  gdb_assert ((mb & ~0x3f) == 0);
  gdb_assert ((xo & ~0x7) == 0);
  gdb_assert ((rc & ~0x1) == 0);

  insn = (rs << 21) | (ra << 16) | (sh0_4 << 11) | (n << 5)
	 | (sh5 << 1) | (xo << 2) | (rc & 1);
  *buf = (opcd << 26) | insn;
  return 1;
}

/* The following are frequently used md-form instructions.  */

#define GEN_RLDICL(buf, ra, rs ,sh, mb) \
				gen_md_form (buf, 30, rs, ra, sh, mb, 0, 0)
#define GEN_RLDICR(buf, ra, rs ,sh, mb) \
				gen_md_form (buf, 30, rs, ra, sh, mb, 1, 0)

/* Generate a i-form instruction in BUF and return the number of bytes written.

   0      6                          30 31 32
   | OPCD |            LI            |AA|LK|  */

static int
gen_i_form (uint32_t *buf, int opcd, int li, int aa, int lk)
{
  uint32_t insn;

  gdb_assert ((opcd & ~0x3f) == 0);

  insn = (li & 0x3fffffc) | (aa & 1) | (lk & 1);
  *buf = (opcd << 26) | insn;
  return 1;
}

/* The following are frequently used i-form instructions.  */

#define GEN_B(buf, li)		gen_i_form (buf, 18, li, 0, 0)
#define GEN_BL(buf, li)		gen_i_form (buf, 18, li, 0, 1)

/* Generate a b-form instruction in BUF and return the number of bytes written.

   0      6    11   16               30 31 32
   | OPCD | BO | BI |      BD        |AA|LK|  */

static int
gen_b_form (uint32_t *buf, int opcd, int bo, int bi, int bd,
	    int aa, int lk)
{
  uint32_t insn;

  gdb_assert ((opcd & ~0x3f) == 0);
  gdb_assert ((bo & ~0x1f) == 0);
  gdb_assert ((bi & ~0x1f) == 0);

  insn = (bo << 21) | (bi << 16) | (bd & 0xfffc) | (aa & 1) | (lk & 1);
  *buf = (opcd << 26) | insn;
  return 1;
}

/* The following are frequently used b-form instructions.  */
/* Assume bi = cr7.  */
#define GEN_BNE(buf, bd)  gen_b_form (buf, 16, 0x4, (7 << 2) | 2, bd, 0 ,0)

/* GEN_LOAD and GEN_STORE generate 64- or 32-bit load/store for ppc64 or ppc32
   respectively.  They are primary used for save/restore GPRs in jump-pad,
   not used for bytecode compiling.  */

#ifdef __powerpc64__
#define GEN_LOAD(buf, rt, ra, si, is_64)	(is_64 ? \
						 GEN_LD (buf, rt, ra, si) : \
						 GEN_LWZ (buf, rt, ra, si))
#define GEN_STORE(buf, rt, ra, si, is_64)	(is_64 ? \
						 GEN_STD (buf, rt, ra, si) : \
						 GEN_STW (buf, rt, ra, si))
#else
#define GEN_LOAD(buf, rt, ra, si, is_64)	GEN_LWZ (buf, rt, ra, si)
#define GEN_STORE(buf, rt, ra, si, is_64)	GEN_STW (buf, rt, ra, si)
#endif

/* Generate a sequence of instructions to load IMM in the register REG.
   Write the instructions in BUF and return the number of bytes written.  */

static int
gen_limm (uint32_t *buf, int reg, uint64_t imm, int is_64)
{
  uint32_t *p = buf;

  if ((imm + 32768) < 65536)
    {
      /* li	reg, imm[15:0] */
      p += GEN_LI (p, reg, imm);
    }
  else if ((imm >> 32) == 0)
    {
      /* lis	reg, imm[31:16]
	 ori	reg, reg, imm[15:0]
	 rldicl reg, reg, 0, 32 */
      p += GEN_LIS (p, reg, (imm >> 16) & 0xffff);
      if ((imm & 0xffff) != 0)
	p += GEN_ORI (p, reg, reg, imm & 0xffff);
      /* Clear upper 32-bit if sign-bit is set.  */
      if (imm & (1u << 31) && is_64)
	p += GEN_RLDICL (p, reg, reg, 0, 32);
    }
  else
    {
      gdb_assert (is_64);
      /* lis    reg, <imm[63:48]>
	 ori    reg, reg, <imm[48:32]>
	 rldicr reg, reg, 32, 31
	 oris   reg, reg, <imm[31:16]>
	 ori    reg, reg, <imm[15:0]> */
      p += GEN_LIS (p, reg, ((imm >> 48) & 0xffff));
      if (((imm >> 32) & 0xffff) != 0)
	p += GEN_ORI (p, reg, reg, ((imm >> 32) & 0xffff));
      p += GEN_RLDICR (p, reg, reg, 32, 31);
      if (((imm >> 16) & 0xffff) != 0)
	p += GEN_ORIS (p, reg, reg, ((imm >> 16) & 0xffff));
      if ((imm & 0xffff) != 0)
	p += GEN_ORI (p, reg, reg, (imm & 0xffff));
    }

  return p - buf;
}

/* Generate a sequence for atomically exchange at location LOCK.
   This code sequence clobbers r6, r7, r8.  LOCK is the location for
   the atomic-xchg, OLD_VALUE is expected old value stored in the
   location, and R_NEW is a register for the new value.  */

static int
gen_atomic_xchg (uint32_t *buf, CORE_ADDR lock, int old_value, int r_new,
		 int is_64)
{
  const int r_lock = 6;
  const int r_old = 7;
  const int r_tmp = 8;
  uint32_t *p = buf;

  /*
  1: lwarx   TMP, 0, LOCK
     cmpwi   TMP, OLD
     bne     1b
     stwcx.  NEW, 0, LOCK
     bne     1b */

  p += gen_limm (p, r_lock, lock, is_64);
  p += gen_limm (p, r_old, old_value, is_64);

  p += GEN_LWARX (p, r_tmp, 0, r_lock);
  p += GEN_CMPW (p, r_tmp, r_old);
  p += GEN_BNE (p, -8);
  p += GEN_STWCX (p, r_new, 0, r_lock);
  p += GEN_BNE (p, -16);

  return p - buf;
}

/* Generate a sequence of instructions for calling a function
   at address of FN.  Return the number of bytes are written in BUF.  */

static int
gen_call (uint32_t *buf, CORE_ADDR fn, int is_64, int is_opd)
{
  uint32_t *p = buf;

  /* Must be called by r12 for caller to calculate TOC address. */
  p += gen_limm (p, 12, fn, is_64);
  if (is_opd)
    {
      p += GEN_LOAD (p, 11, 12, 16, is_64);
      p += GEN_LOAD (p, 2, 12, 8, is_64);
      p += GEN_LOAD (p, 12, 12, 0, is_64);
    }
  p += GEN_MTSPR (p, 12, 9);		/* mtctr  r12 */
  *p++ = 0x4e800421;			/* bctrl */

  return p - buf;
}

/* Copy the instruction from OLDLOC to *TO, and update *TO to *TO + size
   of instruction.  This function is used to adjust pc-relative instructions
   when copying.  */

static void
ppc_relocate_instruction (CORE_ADDR *to, CORE_ADDR oldloc)
{
  uint32_t insn, op6;
  long rel, newrel;

  read_inferior_memory (oldloc, (unsigned char *) &insn, 4);
  op6 = PPC_OP6 (insn);

  if (op6 == 18 && (insn & 2) == 0)
    {
      /* branch && AA = 0 */
      rel = PPC_LI (insn);
      newrel = (oldloc - *to) + rel;

      /* Out of range. Cannot relocate instruction.  */
      if (newrel >= (1 << 25) || newrel < -(1 << 25))
	return;

      insn = (insn & ~0x3fffffc) | (newrel & 0x3fffffc);
    }
  else if (op6 == 16 && (insn & 2) == 0)
    {
      /* conditional branch && AA = 0 */

      /* If the new relocation is too big for even a 26-bit unconditional
	 branch, there is nothing we can do.  Just abort.

	 Otherwise, if it can be fit in 16-bit conditional branch, just
	 copy the instruction and relocate the address.

	 If the it's  big for conditional-branch (16-bit), try to invert the
	 condition and jump with 26-bit branch.  For example,

	 beq  .Lgoto
	 INSN1

	 =>

	 bne  1f (+8)
	 b    .Lgoto
       1:INSN1

	 After this transform, we are actually jump from *TO+4 instead of *TO,
	 so check the relocation again because it will be 1-insn farther then
	 before if *TO is after OLDLOC.


	 For BDNZT (or so) is transformed from

	 bdnzt  eq, .Lgoto
	 INSN1

	 =>

	 bdz    1f (+12)
	 bf     eq, 1f (+8)
	 b      .Lgoto
       1:INSN1

	 See also "BO field encodings".  */

      rel = PPC_BD (insn);
      newrel = (oldloc - *to) + rel;

      if (newrel < (1 << 15) && newrel >= -(1 << 15))
	insn = (insn & ~0xfffc) | (newrel & 0xfffc);
      else if ((PPC_BO (insn) & 0x14) == 0x4 || (PPC_BO (insn) & 0x14) == 0x10)
	{
	  newrel -= 4;

	  /* Out of range. Cannot relocate instruction.  */
	  if (newrel >= (1 << 25) || newrel < -(1 << 25))
	    return;

	  if ((PPC_BO (insn) & 0x14) == 0x4)
	    insn ^= (1 << 24);
	  else if ((PPC_BO (insn) & 0x14) == 0x10)
	    insn ^= (1 << 22);

	  /* Jump over the unconditional branch.  */
	  insn = (insn & ~0xfffc) | 0x8;
	  target_write_memory (*to, (unsigned char *) &insn, 4);
	  *to += 4;

	  /* Build a unconditional branch and copy LK bit.  */
	  insn = (18 << 26) | (0x3fffffc & newrel) | (insn & 0x3);
	  target_write_memory (*to, (unsigned char *) &insn, 4);
	  *to += 4;

	  return;
	}
      else if ((PPC_BO (insn) & 0x14) == 0)
	{
	  uint32_t bdnz_insn = (16 << 26) | (0x10 << 21) | 12;
	  uint32_t bf_insn = (16 << 26) | (0x4 << 21) | 8;

	  newrel -= 8;

	  /* Out of range. Cannot relocate instruction.  */
	  if (newrel >= (1 << 25) || newrel < -(1 << 25))
	    return;

	  /* Copy BI field.  */
	  bf_insn |= (insn & 0x1f0000);

	  /* Invert condition.  */
	  bdnz_insn |= (insn ^ (1 << 22)) & (1 << 22);
	  bf_insn |= (insn ^ (1 << 24)) & (1 << 24);

	  target_write_memory (*to, (unsigned char *) &bdnz_insn, 4);
	  *to += 4;
	  target_write_memory (*to, (unsigned char *) &bf_insn, 4);
	  *to += 4;

	  /* Build a unconditional branch and copy LK bit.  */
	  insn = (18 << 26) | (0x3fffffc & newrel) | (insn & 0x3);
	  target_write_memory (*to, (unsigned char *) &insn, 4);
	  *to += 4;

	  return;
	}
      else /* (BO & 0x14) == 0x14, branch always.  */
	{
	  /* Out of range. Cannot relocate instruction.  */
	  if (newrel >= (1 << 25) || newrel < -(1 << 25))
	    return;

	  /* Build a unconditional branch and copy LK bit.  */
	  insn = (18 << 26) | (0x3fffffc & newrel) | (insn & 0x3);
	  target_write_memory (*to, (unsigned char *) &insn, 4);
	  *to += 4;

	  return;
	}
    }

  target_write_memory (*to, (unsigned char *) &insn, 4);
  *to += 4;
}

bool
ppc_target::supports_fast_tracepoints ()
{
  return true;
}

/* Implement install_fast_tracepoint_jump_pad of target_ops.
   See target.h for details.  */

int
ppc_target::install_fast_tracepoint_jump_pad (CORE_ADDR tpoint,
					      CORE_ADDR tpaddr,
					      CORE_ADDR collector,
					      CORE_ADDR lockaddr,
					      ULONGEST orig_size,
					      CORE_ADDR *jump_entry,
					      CORE_ADDR *trampoline,
					      ULONGEST *trampoline_size,
					      unsigned char *jjump_pad_insn,
					      ULONGEST *jjump_pad_insn_size,
					      CORE_ADDR *adjusted_insn_addr,
					      CORE_ADDR *adjusted_insn_addr_end,
					      char *err)
{
  uint32_t buf[256];
  uint32_t *p = buf;
  int j, offset;
  CORE_ADDR buildaddr = *jump_entry;
  const CORE_ADDR entryaddr = *jump_entry;
  int rsz, min_frame, frame_size, tp_reg;
#ifdef __powerpc64__
  struct regcache *regcache = get_thread_regcache (current_thread, 0);
  int is_64 = register_size (regcache->tdesc, 0) == 8;
  int is_opd = is_64 && !is_elfv2_inferior ();
#else
  int is_64 = 0, is_opd = 0;
#endif

#ifdef __powerpc64__
  if (is_64)
    {
      /* Minimum frame size is 32 bytes for ELFv2, and 112 bytes for ELFv1.  */
      rsz = 8;
      min_frame = 112;
      frame_size = (40 * rsz) + min_frame;
      tp_reg = 13;
    }
  else
    {
#endif
      rsz = 4;
      min_frame = 16;
      frame_size = (40 * rsz) + min_frame;
      tp_reg = 2;
#ifdef __powerpc64__
    }
#endif

  /* Stack frame layout for this jump pad,

     High	thread_area (r13/r2)    |
		tpoint			- collecting_t obj
		PC/<tpaddr>		| +36
		CTR			| +35
		LR			| +34
		XER			| +33
		CR			| +32
		R31			|
		R29			|
		...			|
		R1			| +1
		R0			- collected registers
		...			|
		...			|
     Low	Back-chain		-


     The code flow of this jump pad,

     1. Adjust SP
     2. Save GPR and SPR
     3. Prepare argument
     4. Call gdb_collector
     5. Restore GPR and SPR
     6. Restore SP
     7. Build a jump for back to the program
     8. Copy/relocate original instruction
     9. Build a jump for replacing original instruction.  */

  /* Adjust stack pointer.  */
  if (is_64)
    p += GEN_STDU (p, 1, 1, -frame_size);		/* stdu   r1,-frame_size(r1) */
  else
    p += GEN_STWU (p, 1, 1, -frame_size);		/* stwu   r1,-frame_size(r1) */

  /* Store GPRs.  Save R1 later, because it had just been modified, but
     we want the original value.  */
  for (j = 2; j < 32; j++)
    p += GEN_STORE (p, j, 1, min_frame + j * rsz, is_64);
  p += GEN_STORE (p, 0, 1, min_frame + 0 * rsz, is_64);
  /* Set r0 to the original value of r1 before adjusting stack frame,
     and then save it.  */
  p += GEN_ADDI (p, 0, 1, frame_size);
  p += GEN_STORE (p, 0, 1, min_frame + 1 * rsz, is_64);

  /* Save CR, XER, LR, and CTR.  */
  p += GEN_MFCR (p, 3);					/* mfcr   r3 */
  p += GEN_MFSPR (p, 4, 1);				/* mfxer  r4 */
  p += GEN_MFSPR (p, 5, 8);				/* mflr   r5 */
  p += GEN_MFSPR (p, 6, 9);				/* mfctr  r6 */
  p += GEN_STORE (p, 3, 1, min_frame + 32 * rsz, is_64);/* std    r3, 32(r1) */
  p += GEN_STORE (p, 4, 1, min_frame + 33 * rsz, is_64);/* std    r4, 33(r1) */
  p += GEN_STORE (p, 5, 1, min_frame + 34 * rsz, is_64);/* std    r5, 34(r1) */
  p += GEN_STORE (p, 6, 1, min_frame + 35 * rsz, is_64);/* std    r6, 35(r1) */

  /* Save PC<tpaddr>  */
  p += gen_limm (p, 3, tpaddr, is_64);
  p += GEN_STORE (p, 3, 1, min_frame + 36 * rsz, is_64);


  /* Setup arguments to collector.  */
  /* Set r4 to collected registers.  */
  p += GEN_ADDI (p, 4, 1, min_frame);
  /* Set r3 to TPOINT.  */
  p += gen_limm (p, 3, tpoint, is_64);

  /* Prepare collecting_t object for lock.  */
  p += GEN_STORE (p, 3, 1, min_frame + 37 * rsz, is_64);
  p += GEN_STORE (p, tp_reg, 1, min_frame + 38 * rsz, is_64);
  /* Set R5 to collecting object.  */
  p += GEN_ADDI (p, 5, 1, 37 * rsz);

  p += GEN_LWSYNC (p);
  p += gen_atomic_xchg (p, lockaddr, 0, 5, is_64);
  p += GEN_LWSYNC (p);

  /* Call to collector.  */
  p += gen_call (p, collector, is_64, is_opd);

  /* Simply write 0 to release the lock.  */
  p += gen_limm (p, 3, lockaddr, is_64);
  p += gen_limm (p, 4, 0, is_64);
  p += GEN_LWSYNC (p);
  p += GEN_STORE (p, 4, 3, 0, is_64);

  /* Restore stack and registers.  */
  p += GEN_LOAD (p, 3, 1, min_frame + 32 * rsz, is_64);	/* ld	r3, 32(r1) */
  p += GEN_LOAD (p, 4, 1, min_frame + 33 * rsz, is_64);	/* ld	r4, 33(r1) */
  p += GEN_LOAD (p, 5, 1, min_frame + 34 * rsz, is_64);	/* ld	r5, 34(r1) */
  p += GEN_LOAD (p, 6, 1, min_frame + 35 * rsz, is_64);	/* ld	r6, 35(r1) */
  p += GEN_MTCR (p, 3);					/* mtcr	  r3 */
  p += GEN_MTSPR (p, 4, 1);				/* mtxer  r4 */
  p += GEN_MTSPR (p, 5, 8);				/* mtlr   r5 */
  p += GEN_MTSPR (p, 6, 9);				/* mtctr  r6 */

  /* Restore GPRs.  */
  for (j = 2; j < 32; j++)
    p += GEN_LOAD (p, j, 1, min_frame + j * rsz, is_64);
  p += GEN_LOAD (p, 0, 1, min_frame + 0 * rsz, is_64);
  /* Restore SP.  */
  p += GEN_ADDI (p, 1, 1, frame_size);

  /* Flush instructions to inferior memory.  */
  target_write_memory (buildaddr, (unsigned char *) buf, (p - buf) * 4);

  /* Now, insert the original instruction to execute in the jump pad.  */
  *adjusted_insn_addr = buildaddr + (p - buf) * 4;
  *adjusted_insn_addr_end = *adjusted_insn_addr;
  ppc_relocate_instruction (adjusted_insn_addr_end, tpaddr);

  /* Verify the relocation size.  If should be 4 for normal copy,
     8 or 12 for some conditional branch.  */
  if ((*adjusted_insn_addr_end - *adjusted_insn_addr == 0)
      || (*adjusted_insn_addr_end - *adjusted_insn_addr > 12))
    {
      sprintf (err, "E.Unexpected instruction length = %d"
		    "when relocate instruction.",
		    (int) (*adjusted_insn_addr_end - *adjusted_insn_addr));
      return 1;
    }

  buildaddr = *adjusted_insn_addr_end;
  p = buf;
  /* Finally, write a jump back to the program.  */
  offset = (tpaddr + 4) - buildaddr;
  if (offset >= (1 << 25) || offset < -(1 << 25))
    {
      sprintf (err, "E.Jump back from jump pad too far from tracepoint "
		    "(offset 0x%x > 26-bit).", offset);
      return 1;
    }
  /* b <tpaddr+4> */
  p += GEN_B (p, offset);
  target_write_memory (buildaddr, (unsigned char *) buf, (p - buf) * 4);
  *jump_entry = buildaddr + (p - buf) * 4;

  /* The jump pad is now built.  Wire in a jump to our jump pad.  This
     is always done last (by our caller actually), so that we can
     install fast tracepoints with threads running.  This relies on
     the agent's atomic write support.  */
  offset = entryaddr - tpaddr;
  if (offset >= (1 << 25) || offset < -(1 << 25))
    {
      sprintf (err, "E.Jump back from jump pad too far from tracepoint "
		    "(offset 0x%x > 26-bit).", offset);
      return 1;
    }
  /* b <jentry> */
  GEN_B ((uint32_t *) jjump_pad_insn, offset);
  *jjump_pad_insn_size = 4;

  return 0;
}

/* Returns the minimum instruction length for installing a tracepoint.  */

int
ppc_target::get_min_fast_tracepoint_insn_len ()
{
  return 4;
}

/* Emits a given buffer into the target at current_insn_ptr.  Length
   is in units of 32-bit words.  */

static void
emit_insns (uint32_t *buf, int n)
{
  n = n * sizeof (uint32_t);
  target_write_memory (current_insn_ptr, (unsigned char *) buf, n);
  current_insn_ptr += n;
}

#define __EMIT_ASM(NAME, INSNS)					\
  do								\
    {								\
      extern uint32_t start_bcax_ ## NAME [];			\
      extern uint32_t end_bcax_ ## NAME [];			\
      emit_insns (start_bcax_ ## NAME,				\
		  end_bcax_ ## NAME - start_bcax_ ## NAME);	\
      __asm__ (".section .text.__ppcbcax\n\t"			\
	       "start_bcax_" #NAME ":\n\t"			\
	       INSNS "\n\t"					\
	       "end_bcax_" #NAME ":\n\t"			\
	       ".previous\n\t");				\
    } while (0)

#define _EMIT_ASM(NAME, INSNS)		__EMIT_ASM (NAME, INSNS)
#define EMIT_ASM(INSNS)			_EMIT_ASM (__LINE__, INSNS)

/*

  Bytecode execution stack frame - 32-bit

	|  LR save area           (SP + 4)
 SP' -> +- Back chain             (SP + 0)
	|  Save r31   for access saved arguments
	|  Save r30   for bytecode stack pointer
	|  Save r4    for incoming argument *value
	|  Save r3    for incoming argument regs
 r30 -> +- Bytecode execution stack
	|
	|  64-byte (8 doublewords) at initial.
	|  Expand stack as needed.
	|
	+-
	|  Some padding for minimum stack frame and 16-byte alignment.
	|  16 bytes.
 SP     +- Back-chain (SP')

  initial frame size
  = 16 + (4 * 4) + 64
  = 96

   r30 is the stack-pointer for bytecode machine.
       It should point to next-empty, so we can use LDU for pop.
   r3  is used for cache of the high part of TOP value.
       It was the first argument, pointer to regs.
   r4  is used for cache of the low part of TOP value.
       It was the second argument, pointer to the result.
       We should set *result = TOP after leaving this function.

 Note:
 * To restore stack at epilogue
   => sp = r31
 * To check stack is big enough for bytecode execution.
   => r30 - 8 > SP + 8
 * To return execution result.
   => 0(r4) = TOP

 */

/* Regardless of endian, register 3 is always high part, 4 is low part.
   These defines are used when the register pair is stored/loaded.
   Likewise, to simplify code, have a similar define for 5:6. */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TOP_FIRST	"4"
#define TOP_SECOND	"3"
#define TMP_FIRST	"6"
#define TMP_SECOND	"5"
#else
#define TOP_FIRST	"3"
#define TOP_SECOND	"4"
#define TMP_FIRST	"5"
#define TMP_SECOND	"6"
#endif

/* Emit prologue in inferior memory.  See above comments.  */

static void
ppc_emit_prologue (void)
{
  EMIT_ASM (/* Save return address.  */
	    "mflr  0		\n"
	    "stw   0, 4(1)	\n"
	    /* Adjust SP.  96 is the initial frame size.  */
	    "stwu  1, -96(1)	\n"
	    /* Save r30 and incoming arguments.  */
	    "stw   31, 96-4(1)	\n"
	    "stw   30, 96-8(1)	\n"
	    "stw   4, 96-12(1)	\n"
	    "stw   3, 96-16(1)	\n"
	    /* Point r31 to original r1 for access arguments.  */
	    "addi  31, 1, 96	\n"
	    /* Set r30 to pointing stack-top.  */
	    "addi  30, 1, 64	\n"
	    /* Initial r3/TOP to 0.  */
	    "li    3, 0		\n"
	    "li    4, 0		\n");
}

/* Emit epilogue in inferior memory.  See above comments.  */

static void
ppc_emit_epilogue (void)
{
  EMIT_ASM (/* *result = TOP */
	    "lwz   5, -12(31)	\n"
	    "stw   " TOP_FIRST ", 0(5)	\n"
	    "stw   " TOP_SECOND ", 4(5)	\n"
	    /* Restore registers.  */
	    "lwz   31, -4(31)	\n"
	    "lwz   30, -8(31)	\n"
	    /* Restore SP.  */
	    "lwz   1, 0(1)      \n"
	    /* Restore LR.  */
	    "lwz   0, 4(1)	\n"
	    /* Return 0 for no-error.  */
	    "li    3, 0		\n"
	    "mtlr  0		\n"
	    "blr		\n");
}

/* TOP = stack[--sp] + TOP  */

static void
ppc_emit_add (void)
{
  EMIT_ASM ("lwzu  " TMP_FIRST ", 8(30)	\n"
	    "lwz   " TMP_SECOND ", 4(30)\n"
	    "addc  4, 6, 4	\n"
	    "adde  3, 5, 3	\n");
}

/* TOP = stack[--sp] - TOP  */

static void
ppc_emit_sub (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "subfc  4, 4, 6	\n"
	    "subfe  3, 3, 5	\n");
}

/* TOP = stack[--sp] * TOP  */

static void
ppc_emit_mul (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "mulhwu 7, 6, 4	\n"
	    "mullw  3, 6, 3	\n"
	    "mullw  5, 4, 5	\n"
	    "mullw  4, 6, 4	\n"
	    "add    3, 5, 3	\n"
	    "add    3, 7, 3	\n");
}

/* TOP = stack[--sp] << TOP  */

static void
ppc_emit_lsh (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "subfic 3, 4, 32\n"		/* r3 = 32 - TOP */
	    "addi   7, 4, -32\n"	/* r7 = TOP - 32 */
	    "slw    5, 5, 4\n"		/* Shift high part left */
	    "slw    4, 6, 4\n"		/* Shift low part left */
	    "srw    3, 6, 3\n"		/* Shift low to high if shift < 32 */
	    "slw    7, 6, 7\n"		/* Shift low to high if shift >= 32 */
	    "or     3, 5, 3\n"
	    "or     3, 7, 3\n");	/* Assemble high part */
}

/* Top = stack[--sp] >> TOP
   (Arithmetic shift right)  */

static void
ppc_emit_rsh_signed (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "addi   7, 4, -32\n"	/* r7 = TOP - 32 */
	    "sraw   3, 5, 4\n"		/* Shift high part right */
	    "cmpwi  7, 1\n"
	    "blt    0, 1f\n"		/* If shift <= 32, goto 1: */
	    "sraw   4, 5, 7\n"		/* Shift high to low */
	    "b      2f\n"
	    "1:\n"
	    "subfic 7, 4, 32\n"		/* r7 = 32 - TOP */
	    "srw    4, 6, 4\n"		/* Shift low part right */
	    "slw    5, 5, 7\n"		/* Shift high to low */
	    "or     4, 4, 5\n"		/* Assemble low part */
	    "2:\n");
}

/* Top = stack[--sp] >> TOP
   (Logical shift right)  */

static void
ppc_emit_rsh_unsigned (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "subfic 3, 4, 32\n"		/* r3 = 32 - TOP */
	    "addi   7, 4, -32\n"	/* r7 = TOP - 32 */
	    "srw    6, 6, 4\n"		/* Shift low part right */
	    "slw    3, 5, 3\n"		/* Shift high to low if shift < 32 */
	    "srw    7, 5, 7\n"		/* Shift high to low if shift >= 32 */
	    "or     6, 6, 3\n"
	    "srw    3, 5, 4\n"		/* Shift high part right */
	    "or     4, 6, 7\n");	/* Assemble low part */
}

/* Emit code for signed-extension specified by ARG.  */

static void
ppc_emit_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM ("extsb  4, 4\n"
		"srawi 3, 4, 31");
      break;
    case 16:
      EMIT_ASM ("extsh  4, 4\n"
		"srawi 3, 4, 31");
      break;
    case 32:
      EMIT_ASM ("srawi 3, 4, 31");
      break;
    default:
      emit_error = 1;
    }
}

/* Emit code for zero-extension specified by ARG.  */

static void
ppc_emit_zero_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM ("clrlwi 4,4,24\n"
		"li 3, 0\n");
      break;
    case 16:
      EMIT_ASM ("clrlwi 4,4,16\n"
		"li 3, 0\n");
      break;
    case 32:
      EMIT_ASM ("li 3, 0");
      break;
    default:
      emit_error = 1;
    }
}

/* TOP = !TOP
   i.e., TOP = (TOP == 0) ? 1 : 0;  */

static void
ppc_emit_log_not (void)
{
  EMIT_ASM ("or      4, 3, 4	\n"
	    "cntlzw  4, 4	\n"
	    "srwi    4, 4, 5	\n"
	    "li      3, 0	\n");
}

/* TOP = stack[--sp] & TOP  */

static void
ppc_emit_bit_and (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "and  4, 6, 4	\n"
	    "and  3, 5, 3	\n");
}

/* TOP = stack[--sp] | TOP  */

static void
ppc_emit_bit_or (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "or  4, 6, 4	\n"
	    "or  3, 5, 3	\n");
}

/* TOP = stack[--sp] ^ TOP  */

static void
ppc_emit_bit_xor (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "xor  4, 6, 4	\n"
	    "xor  3, 5, 3	\n");
}

/* TOP = ~TOP
   i.e., TOP = ~(TOP | TOP)  */

static void
ppc_emit_bit_not (void)
{
  EMIT_ASM ("nor  3, 3, 3	\n"
	    "nor  4, 4, 4	\n");
}

/* TOP = stack[--sp] == TOP  */

static void
ppc_emit_equal (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "xor     4, 6, 4	\n"
	    "xor     3, 5, 3	\n"
	    "or      4, 3, 4	\n"
	    "cntlzw  4, 4	\n"
	    "srwi    4, 4, 5	\n"
	    "li      3, 0	\n");
}

/* TOP = stack[--sp] < TOP
   (Signed comparison)  */

static void
ppc_emit_less_signed (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "cmplw   6, 6, 4		\n"
	    "cmpw    7, 5, 3		\n"
	    /* CR6 bit 0 = low less and high equal */
	    "crand   6*4+0, 6*4+0, 7*4+2\n"
	    /* CR7 bit 0 = (low less and high equal) or high less */
	    "cror    7*4+0, 7*4+0, 6*4+0\n"
	    "mfcr    4			\n"
	    "rlwinm  4, 4, 29, 31, 31	\n"
	    "li      3, 0		\n");
}

/* TOP = stack[--sp] < TOP
   (Unsigned comparison)  */

static void
ppc_emit_less_unsigned (void)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "cmplw   6, 6, 4		\n"
	    "cmplw   7, 5, 3		\n"
	    /* CR6 bit 0 = low less and high equal */
	    "crand   6*4+0, 6*4+0, 7*4+2\n"
	    /* CR7 bit 0 = (low less and high equal) or high less */
	    "cror    7*4+0, 7*4+0, 6*4+0\n"
	    "mfcr    4			\n"
	    "rlwinm  4, 4, 29, 31, 31	\n"
	    "li      3, 0		\n");
}

/* Access the memory address in TOP in size of SIZE.
   Zero-extend the read value.  */

static void
ppc_emit_ref (int size)
{
  switch (size)
    {
    case 1:
      EMIT_ASM ("lbz   4, 0(4)\n"
		"li    3, 0");
      break;
    case 2:
      EMIT_ASM ("lhz   4, 0(4)\n"
		"li    3, 0");
      break;
    case 4:
      EMIT_ASM ("lwz   4, 0(4)\n"
		"li    3, 0");
      break;
    case 8:
      if (__BYTE_ORDER == __LITTLE_ENDIAN)
	EMIT_ASM ("lwz   3, 4(4)\n"
		  "lwz   4, 0(4)");
      else
	EMIT_ASM ("lwz   3, 0(4)\n"
		  "lwz   4, 4(4)");
      break;
    }
}

/* TOP = NUM  */

static void
ppc_emit_const (LONGEST num)
{
  uint32_t buf[10];
  uint32_t *p = buf;

  p += gen_limm (p, 3, num >> 32 & 0xffffffff, 0);
  p += gen_limm (p, 4, num & 0xffffffff, 0);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Set TOP to the value of register REG by calling get_raw_reg function
   with two argument, collected buffer and register number.  */

static void
ppc_emit_reg (int reg)
{
  uint32_t buf[13];
  uint32_t *p = buf;

  /* fctx->regs is passed in r3 and then saved in -16(31).  */
  p += GEN_LWZ (p, 3, 31, -16);
  p += GEN_LI (p, 4, reg);	/* li	r4, reg */
  p += gen_call (p, get_raw_reg_func_addr (), 0, 0);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));

  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
      EMIT_ASM ("mr 5, 4\n"
		"mr 4, 3\n"
		"mr 3, 5\n");
    }
}

/* TOP = stack[--sp] */

static void
ppc_emit_pop (void)
{
  EMIT_ASM ("lwzu " TOP_FIRST ", 8(30)	\n"
	    "lwz " TOP_SECOND ", 4(30)	\n");
}

/* stack[sp++] = TOP

   Because we may use up bytecode stack, expand 8 doublewords more
   if needed.  */

static void
ppc_emit_stack_flush (void)
{
  /* Make sure bytecode stack is big enough before push.
     Otherwise, expand 64-byte more.  */

  EMIT_ASM ("  stw   " TOP_FIRST ", 0(30)	\n"
	    "  stw   " TOP_SECOND ", 4(30)\n"
	    "  addi  5, 30, -(8 + 8)	\n"
	    "  cmpw  7, 5, 1		\n"
	    "  bgt   7, 1f		\n"
	    "  stwu  31, -64(1)		\n"
	    "1:addi  30, 30, -8		\n");
}

/* Swap TOP and stack[sp-1]  */

static void
ppc_emit_swap (void)
{
  EMIT_ASM ("lwz  " TMP_FIRST ", 8(30)	\n"
	    "lwz  " TMP_SECOND ", 12(30)	\n"
	    "stw  " TOP_FIRST ", 8(30)	\n"
	    "stw  " TOP_SECOND ", 12(30)	\n"
	    "mr   3, 5		\n"
	    "mr   4, 6		\n");
}

/* Discard N elements in the stack.  Also used for ppc64.  */

static void
ppc_emit_stack_adjust (int n)
{
  uint32_t buf[6];
  uint32_t *p = buf;

  n = n << 3;
  if ((n >> 15) != 0)
    {
      emit_error = 1;
      return;
    }

  p += GEN_ADDI (p, 30, 30, n);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Call function FN.  */

static void
ppc_emit_call (CORE_ADDR fn)
{
  uint32_t buf[11];
  uint32_t *p = buf;

  p += gen_call (p, fn, 0, 0);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* FN's prototype is `LONGEST(*fn)(int)'.
   TOP = fn (arg1)
  */

static void
ppc_emit_int_call_1 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[15];
  uint32_t *p = buf;

  /* Setup argument.  arg1 is a 16-bit value.  */
  p += gen_limm (p, 3, (uint32_t) arg1, 0);
  p += gen_call (p, fn, 0, 0);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));

  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
      EMIT_ASM ("mr 5, 4\n"
		"mr 4, 3\n"
		"mr 3, 5\n");
    }
}

/* FN's prototype is `void(*fn)(int,LONGEST)'.
   fn (arg1, TOP)

   TOP should be preserved/restored before/after the call.  */

static void
ppc_emit_void_call_2 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[21];
  uint32_t *p = buf;

  /* Save TOP.  0(30) is next-empty.  */
  p += GEN_STW (p, 3, 30, 0);
  p += GEN_STW (p, 4, 30, 4);

  /* Setup argument.  arg1 is a 16-bit value.  */
  if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
       p += GEN_MR (p, 5, 4);
       p += GEN_MR (p, 6, 3);
    }
  else
    {
       p += GEN_MR (p, 5, 3);
       p += GEN_MR (p, 6, 4);
    }
  p += gen_limm (p, 3, (uint32_t) arg1, 0);
  p += gen_call (p, fn, 0, 0);

  /* Restore TOP */
  p += GEN_LWZ (p, 3, 30, 0);
  p += GEN_LWZ (p, 4, 30, 4);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Note in the following goto ops:

   When emitting goto, the target address is later relocated by
   write_goto_address.  OFFSET_P is the offset of the branch instruction
   in the code sequence, and SIZE_P is how to relocate the instruction,
   recognized by ppc_write_goto_address.  In current implementation,
   SIZE can be either 24 or 14 for branch of conditional-branch instruction.
 */

/* If TOP is true, goto somewhere.  Otherwise, just fall-through.  */

static void
ppc_emit_if_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("or.    3, 3, 4	\n"
	    "lwzu " TOP_FIRST ", 8(30)	\n"
	    "lwz " TOP_SECOND ", 4(30)	\n"
	    "1:bne  0, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Unconditional goto.  Also used for ppc64.  */

static void
ppc_emit_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("1:b	1b");

  if (offset_p)
    *offset_p = 0;
  if (size_p)
    *size_p = 24;
}

/* Goto if stack[--sp] == TOP  */

static void
ppc_emit_eq_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("lwzu  " TMP_FIRST ", 8(30)	\n"
	    "lwz   " TMP_SECOND ", 4(30)	\n"
	    "xor   4, 6, 4	\n"
	    "xor   3, 5, 3	\n"
	    "or.   3, 3, 4	\n"
	    "lwzu  " TOP_FIRST ", 8(30)	\n"
	    "lwz   " TOP_SECOND ", 4(30)	\n"
	    "1:beq 0, 1b	\n");

  if (offset_p)
    *offset_p = 28;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] != TOP  */

static void
ppc_emit_ne_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("lwzu  " TMP_FIRST ", 8(30)	\n"
	    "lwz   " TMP_SECOND ", 4(30)	\n"
	    "xor   4, 6, 4	\n"
	    "xor   3, 5, 3	\n"
	    "or.   3, 3, 4	\n"
	    "lwzu  " TOP_FIRST ", 8(30)	\n"
	    "lwz   " TOP_SECOND ", 4(30)	\n"
	    "1:bne 0, 1b	\n");

  if (offset_p)
    *offset_p = 28;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] < TOP  */

static void
ppc_emit_lt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "cmplw   6, 6, 4		\n"
	    "cmpw    7, 5, 3		\n"
	    /* CR6 bit 0 = low less and high equal */
	    "crand   6*4+0, 6*4+0, 7*4+2\n"
	    /* CR7 bit 0 = (low less and high equal) or high less */
	    "cror    7*4+0, 7*4+0, 6*4+0\n"
	    "lwzu    " TOP_FIRST ", 8(30)	\n"
	    "lwz     " TOP_SECOND ", 4(30)\n"
	    "1:blt   7, 1b	\n");

  if (offset_p)
    *offset_p = 32;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] <= TOP  */

static void
ppc_emit_le_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "cmplw   6, 6, 4		\n"
	    "cmpw    7, 5, 3		\n"
	    /* CR6 bit 0 = low less/equal and high equal */
	    "crandc   6*4+0, 7*4+2, 6*4+1\n"
	    /* CR7 bit 0 = (low less/eq and high equal) or high less */
	    "cror    7*4+0, 7*4+0, 6*4+0\n"
	    "lwzu    " TOP_FIRST ", 8(30)	\n"
	    "lwz     " TOP_SECOND ", 4(30)\n"
	    "1:blt   7, 1b	\n");

  if (offset_p)
    *offset_p = 32;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] > TOP  */

static void
ppc_emit_gt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "cmplw   6, 6, 4		\n"
	    "cmpw    7, 5, 3		\n"
	    /* CR6 bit 0 = low greater and high equal */
	    "crand   6*4+0, 6*4+1, 7*4+2\n"
	    /* CR7 bit 0 = (low greater and high equal) or high greater */
	    "cror    7*4+0, 7*4+1, 6*4+0\n"
	    "lwzu    " TOP_FIRST ", 8(30)	\n"
	    "lwz     " TOP_SECOND ", 4(30)\n"
	    "1:blt   7, 1b	\n");

  if (offset_p)
    *offset_p = 32;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] >= TOP  */

static void
ppc_emit_ge_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("lwzu " TMP_FIRST ", 8(30)	\n"
	    "lwz " TMP_SECOND ", 4(30)	\n"
	    "cmplw   6, 6, 4		\n"
	    "cmpw    7, 5, 3		\n"
	    /* CR6 bit 0 = low ge and high equal */
	    "crandc  6*4+0, 7*4+2, 6*4+0\n"
	    /* CR7 bit 0 = (low ge and high equal) or high greater */
	    "cror    7*4+0, 7*4+1, 6*4+0\n"
	    "lwzu    " TOP_FIRST ", 8(30)\n"
	    "lwz     " TOP_SECOND ", 4(30)\n"
	    "1:blt   7, 1b	\n");

  if (offset_p)
    *offset_p = 32;
  if (size_p)
    *size_p = 14;
}

/* Relocate previous emitted branch instruction.  FROM is the address
   of the branch instruction, TO is the goto target address, and SIZE
   if the value we set by *SIZE_P before.  Currently, it is either
   24 or 14 of branch and conditional-branch instruction.
   Also used for ppc64.  */

static void
ppc_write_goto_address (CORE_ADDR from, CORE_ADDR to, int size)
{
  long rel = to - from;
  uint32_t insn;
  int opcd;

  read_inferior_memory (from, (unsigned char *) &insn, 4);
  opcd = (insn >> 26) & 0x3f;

  switch (size)
    {
    case 14:
      if (opcd != 16
	  || (rel >= (1 << 15) || rel < -(1 << 15)))
	emit_error = 1;
      insn = (insn & ~0xfffc) | (rel & 0xfffc);
      break;
    case 24:
      if (opcd != 18
	  || (rel >= (1 << 25) || rel < -(1 << 25)))
	emit_error = 1;
      insn = (insn & ~0x3fffffc) | (rel & 0x3fffffc);
      break;
    default:
      emit_error = 1;
    }

  if (!emit_error)
    target_write_memory (from, (unsigned char *) &insn, 4);
}

/* Table of emit ops for 32-bit.  */

static struct emit_ops ppc_emit_ops_impl =
{
  ppc_emit_prologue,
  ppc_emit_epilogue,
  ppc_emit_add,
  ppc_emit_sub,
  ppc_emit_mul,
  ppc_emit_lsh,
  ppc_emit_rsh_signed,
  ppc_emit_rsh_unsigned,
  ppc_emit_ext,
  ppc_emit_log_not,
  ppc_emit_bit_and,
  ppc_emit_bit_or,
  ppc_emit_bit_xor,
  ppc_emit_bit_not,
  ppc_emit_equal,
  ppc_emit_less_signed,
  ppc_emit_less_unsigned,
  ppc_emit_ref,
  ppc_emit_if_goto,
  ppc_emit_goto,
  ppc_write_goto_address,
  ppc_emit_const,
  ppc_emit_call,
  ppc_emit_reg,
  ppc_emit_pop,
  ppc_emit_stack_flush,
  ppc_emit_zero_ext,
  ppc_emit_swap,
  ppc_emit_stack_adjust,
  ppc_emit_int_call_1,
  ppc_emit_void_call_2,
  ppc_emit_eq_goto,
  ppc_emit_ne_goto,
  ppc_emit_lt_goto,
  ppc_emit_le_goto,
  ppc_emit_gt_goto,
  ppc_emit_ge_goto
};

#ifdef __powerpc64__

/*

  Bytecode execution stack frame - 64-bit

	|  LR save area           (SP + 16)
	|  CR save area           (SP + 8)
 SP' -> +- Back chain             (SP + 0)
	|  Save r31   for access saved arguments
	|  Save r30   for bytecode stack pointer
	|  Save r4    for incoming argument *value
	|  Save r3    for incoming argument regs
 r30 -> +- Bytecode execution stack
	|
	|  64-byte (8 doublewords) at initial.
	|  Expand stack as needed.
	|
	+-
	|  Some padding for minimum stack frame.
	|  112 for ELFv1.
 SP     +- Back-chain (SP')

  initial frame size
  = 112 + (4 * 8) + 64
  = 208

   r30 is the stack-pointer for bytecode machine.
       It should point to next-empty, so we can use LDU for pop.
   r3  is used for cache of TOP value.
       It was the first argument, pointer to regs.
   r4  is the second argument, pointer to the result.
       We should set *result = TOP after leaving this function.

 Note:
 * To restore stack at epilogue
   => sp = r31
 * To check stack is big enough for bytecode execution.
   => r30 - 8 > SP + 112
 * To return execution result.
   => 0(r4) = TOP

 */

/* Emit prologue in inferior memory.  See above comments.  */

static void
ppc64v1_emit_prologue (void)
{
  /* On ELFv1, function pointers really point to function descriptor,
     so emit one here.  We don't care about contents of words 1 and 2,
     so let them just overlap out code.  */
  uint64_t opd = current_insn_ptr + 8;
  uint32_t buf[2];

  /* Mind the strict aliasing rules.  */
  memcpy (buf, &opd, sizeof buf);
  emit_insns(buf, 2);
  EMIT_ASM (/* Save return address.  */
	    "mflr  0		\n"
	    "std   0, 16(1)	\n"
	    /* Save r30 and incoming arguments.  */
	    "std   31, -8(1)	\n"
	    "std   30, -16(1)	\n"
	    "std   4, -24(1)	\n"
	    "std   3, -32(1)	\n"
	    /* Point r31 to current r1 for access arguments.  */
	    "mr    31, 1	\n"
	    /* Adjust SP.  208 is the initial frame size.  */
	    "stdu  1, -208(1)	\n"
	    /* Set r30 to pointing stack-top.  */
	    "addi  30, 1, 168	\n"
	    /* Initial r3/TOP to 0.  */
	    "li	   3, 0		\n");
}

/* Emit prologue in inferior memory.  See above comments.  */

static void
ppc64v2_emit_prologue (void)
{
  EMIT_ASM (/* Save return address.  */
	    "mflr  0		\n"
	    "std   0, 16(1)	\n"
	    /* Save r30 and incoming arguments.  */
	    "std   31, -8(1)	\n"
	    "std   30, -16(1)	\n"
	    "std   4, -24(1)	\n"
	    "std   3, -32(1)	\n"
	    /* Point r31 to current r1 for access arguments.  */
	    "mr    31, 1	\n"
	    /* Adjust SP.  208 is the initial frame size.  */
	    "stdu  1, -208(1)	\n"
	    /* Set r30 to pointing stack-top.  */
	    "addi  30, 1, 168	\n"
	    /* Initial r3/TOP to 0.  */
	    "li	   3, 0		\n");
}

/* Emit epilogue in inferior memory.  See above comments.  */

static void
ppc64_emit_epilogue (void)
{
  EMIT_ASM (/* Restore SP.  */
	    "ld    1, 0(1)      \n"
	    /* *result = TOP */
	    "ld    4, -24(1)	\n"
	    "std   3, 0(4)	\n"
	    /* Restore registers.  */
	    "ld    31, -8(1)	\n"
	    "ld    30, -16(1)	\n"
	    /* Restore LR.  */
	    "ld    0, 16(1)	\n"
	    /* Return 0 for no-error.  */
	    "li    3, 0		\n"
	    "mtlr  0		\n"
	    "blr		\n");
}

/* TOP = stack[--sp] + TOP  */

static void
ppc64_emit_add (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "add  3, 4, 3	\n");
}

/* TOP = stack[--sp] - TOP  */

static void
ppc64_emit_sub (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "sub  3, 4, 3	\n");
}

/* TOP = stack[--sp] * TOP  */

static void
ppc64_emit_mul (void)
{
  EMIT_ASM ("ldu    4, 8(30)	\n"
	    "mulld  3, 4, 3	\n");
}

/* TOP = stack[--sp] << TOP  */

static void
ppc64_emit_lsh (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "sld  3, 4, 3	\n");
}

/* Top = stack[--sp] >> TOP
   (Arithmetic shift right)  */

static void
ppc64_emit_rsh_signed (void)
{
  EMIT_ASM ("ldu   4, 8(30)	\n"
	    "srad  3, 4, 3	\n");
}

/* Top = stack[--sp] >> TOP
   (Logical shift right)  */

static void
ppc64_emit_rsh_unsigned (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "srd  3, 4, 3	\n");
}

/* Emit code for signed-extension specified by ARG.  */

static void
ppc64_emit_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM ("extsb  3, 3");
      break;
    case 16:
      EMIT_ASM ("extsh  3, 3");
      break;
    case 32:
      EMIT_ASM ("extsw  3, 3");
      break;
    default:
      emit_error = 1;
    }
}

/* Emit code for zero-extension specified by ARG.  */

static void
ppc64_emit_zero_ext (int arg)
{
  switch (arg)
    {
    case 8:
      EMIT_ASM ("rldicl 3,3,0,56");
      break;
    case 16:
      EMIT_ASM ("rldicl 3,3,0,48");
      break;
    case 32:
      EMIT_ASM ("rldicl 3,3,0,32");
      break;
    default:
      emit_error = 1;
    }
}

/* TOP = !TOP
   i.e., TOP = (TOP == 0) ? 1 : 0;  */

static void
ppc64_emit_log_not (void)
{
  EMIT_ASM ("cntlzd  3, 3	\n"
	    "srdi    3, 3, 6	\n");
}

/* TOP = stack[--sp] & TOP  */

static void
ppc64_emit_bit_and (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "and  3, 4, 3	\n");
}

/* TOP = stack[--sp] | TOP  */

static void
ppc64_emit_bit_or (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "or   3, 4, 3	\n");
}

/* TOP = stack[--sp] ^ TOP  */

static void
ppc64_emit_bit_xor (void)
{
  EMIT_ASM ("ldu  4, 8(30)	\n"
	    "xor  3, 4, 3	\n");
}

/* TOP = ~TOP
   i.e., TOP = ~(TOP | TOP)  */

static void
ppc64_emit_bit_not (void)
{
  EMIT_ASM ("nor  3, 3, 3	\n");
}

/* TOP = stack[--sp] == TOP  */

static void
ppc64_emit_equal (void)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "xor     3, 3, 4	\n"
	    "cntlzd  3, 3	\n"
	    "srdi    3, 3, 6	\n");
}

/* TOP = stack[--sp] < TOP
   (Signed comparison)  */

static void
ppc64_emit_less_signed (void)
{
  EMIT_ASM ("ldu     4, 8(30)		\n"
	    "cmpd    7, 4, 3		\n"
	    "mfcr    3			\n"
	    "rlwinm  3, 3, 29, 31, 31	\n");
}

/* TOP = stack[--sp] < TOP
   (Unsigned comparison)  */

static void
ppc64_emit_less_unsigned (void)
{
  EMIT_ASM ("ldu     4, 8(30)		\n"
	    "cmpld   7, 4, 3		\n"
	    "mfcr    3			\n"
	    "rlwinm  3, 3, 29, 31, 31	\n");
}

/* Access the memory address in TOP in size of SIZE.
   Zero-extend the read value.  */

static void
ppc64_emit_ref (int size)
{
  switch (size)
    {
    case 1:
      EMIT_ASM ("lbz   3, 0(3)");
      break;
    case 2:
      EMIT_ASM ("lhz   3, 0(3)");
      break;
    case 4:
      EMIT_ASM ("lwz   3, 0(3)");
      break;
    case 8:
      EMIT_ASM ("ld    3, 0(3)");
      break;
    }
}

/* TOP = NUM  */

static void
ppc64_emit_const (LONGEST num)
{
  uint32_t buf[5];
  uint32_t *p = buf;

  p += gen_limm (p, 3, num, 1);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Set TOP to the value of register REG by calling get_raw_reg function
   with two argument, collected buffer and register number.  */

static void
ppc64v1_emit_reg (int reg)
{
  uint32_t buf[15];
  uint32_t *p = buf;

  /* fctx->regs is passed in r3 and then saved in 176(1).  */
  p += GEN_LD (p, 3, 31, -32);
  p += GEN_LI (p, 4, reg);
  p += GEN_STD (p, 2, 1, 40);	/* Save TOC.  */
  p += gen_call (p, get_raw_reg_func_addr (), 1, 1);
  p += GEN_LD (p, 2, 1, 40);	/* Restore TOC.  */

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Likewise, for ELFv2.  */

static void
ppc64v2_emit_reg (int reg)
{
  uint32_t buf[12];
  uint32_t *p = buf;

  /* fctx->regs is passed in r3 and then saved in 176(1).  */
  p += GEN_LD (p, 3, 31, -32);
  p += GEN_LI (p, 4, reg);
  p += GEN_STD (p, 2, 1, 24);	/* Save TOC.  */
  p += gen_call (p, get_raw_reg_func_addr (), 1, 0);
  p += GEN_LD (p, 2, 1, 24);	/* Restore TOC.  */

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* TOP = stack[--sp] */

static void
ppc64_emit_pop (void)
{
  EMIT_ASM ("ldu  3, 8(30)");
}

/* stack[sp++] = TOP

   Because we may use up bytecode stack, expand 8 doublewords more
   if needed.  */

static void
ppc64_emit_stack_flush (void)
{
  /* Make sure bytecode stack is big enough before push.
     Otherwise, expand 64-byte more.  */

  EMIT_ASM ("  std   3, 0(30)		\n"
	    "  addi  4, 30, -(112 + 8)	\n"
	    "  cmpd  7, 4, 1		\n"
	    "  bgt   7, 1f		\n"
	    "  stdu  31, -64(1)		\n"
	    "1:addi  30, 30, -8		\n");
}

/* Swap TOP and stack[sp-1]  */

static void
ppc64_emit_swap (void)
{
  EMIT_ASM ("ld   4, 8(30)	\n"
	    "std  3, 8(30)	\n"
	    "mr   3, 4		\n");
}

/* Call function FN - ELFv1.  */

static void
ppc64v1_emit_call (CORE_ADDR fn)
{
  uint32_t buf[13];
  uint32_t *p = buf;

  p += GEN_STD (p, 2, 1, 40);	/* Save TOC.  */
  p += gen_call (p, fn, 1, 1);
  p += GEN_LD (p, 2, 1, 40);	/* Restore TOC.  */

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Call function FN - ELFv2.  */

static void
ppc64v2_emit_call (CORE_ADDR fn)
{
  uint32_t buf[10];
  uint32_t *p = buf;

  p += GEN_STD (p, 2, 1, 24);	/* Save TOC.  */
  p += gen_call (p, fn, 1, 0);
  p += GEN_LD (p, 2, 1, 24);	/* Restore TOC.  */

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* FN's prototype is `LONGEST(*fn)(int)'.
   TOP = fn (arg1)
  */

static void
ppc64v1_emit_int_call_1 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[13];
  uint32_t *p = buf;

  /* Setup argument.  arg1 is a 16-bit value.  */
  p += gen_limm (p, 3, arg1, 1);
  p += GEN_STD (p, 2, 1, 40);	/* Save TOC.  */
  p += gen_call (p, fn, 1, 1);
  p += GEN_LD (p, 2, 1, 40);	/* Restore TOC.  */

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Likewise for ELFv2.  */

static void
ppc64v2_emit_int_call_1 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[10];
  uint32_t *p = buf;

  /* Setup argument.  arg1 is a 16-bit value.  */
  p += gen_limm (p, 3, arg1, 1);
  p += GEN_STD (p, 2, 1, 24);	/* Save TOC.  */
  p += gen_call (p, fn, 1, 0);
  p += GEN_LD (p, 2, 1, 24);	/* Restore TOC.  */

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* FN's prototype is `void(*fn)(int,LONGEST)'.
   fn (arg1, TOP)

   TOP should be preserved/restored before/after the call.  */

static void
ppc64v1_emit_void_call_2 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[17];
  uint32_t *p = buf;

  /* Save TOP.  0(30) is next-empty.  */
  p += GEN_STD (p, 3, 30, 0);

  /* Setup argument.  arg1 is a 16-bit value.  */
  p += GEN_MR (p, 4, 3);		/* mr	r4, r3 */
  p += gen_limm (p, 3, arg1, 1);
  p += GEN_STD (p, 2, 1, 40);	/* Save TOC.  */
  p += gen_call (p, fn, 1, 1);
  p += GEN_LD (p, 2, 1, 40);	/* Restore TOC.  */

  /* Restore TOP */
  p += GEN_LD (p, 3, 30, 0);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* Likewise for ELFv2.  */

static void
ppc64v2_emit_void_call_2 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[14];
  uint32_t *p = buf;

  /* Save TOP.  0(30) is next-empty.  */
  p += GEN_STD (p, 3, 30, 0);

  /* Setup argument.  arg1 is a 16-bit value.  */
  p += GEN_MR (p, 4, 3);		/* mr	r4, r3 */
  p += gen_limm (p, 3, arg1, 1);
  p += GEN_STD (p, 2, 1, 24);	/* Save TOC.  */
  p += gen_call (p, fn, 1, 0);
  p += GEN_LD (p, 2, 1, 24);	/* Restore TOC.  */

  /* Restore TOP */
  p += GEN_LD (p, 3, 30, 0);

  emit_insns (buf, p - buf);
  gdb_assert ((p - buf) <= (sizeof (buf) / sizeof (*buf)));
}

/* If TOP is true, goto somewhere.  Otherwise, just fall-through.  */

static void
ppc64_emit_if_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("cmpdi  7, 3, 0	\n"
	    "ldu    3, 8(30)	\n"
	    "1:bne  7, 1b	\n");

  if (offset_p)
    *offset_p = 8;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] == TOP  */

static void
ppc64_emit_eq_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "cmpd    7, 4, 3	\n"
	    "ldu     3, 8(30)	\n"
	    "1:beq   7, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] != TOP  */

static void
ppc64_emit_ne_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "cmpd    7, 4, 3	\n"
	    "ldu     3, 8(30)	\n"
	    "1:bne   7, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] < TOP  */

static void
ppc64_emit_lt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "cmpd    7, 4, 3	\n"
	    "ldu     3, 8(30)	\n"
	    "1:blt   7, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] <= TOP  */

static void
ppc64_emit_le_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "cmpd    7, 4, 3	\n"
	    "ldu     3, 8(30)	\n"
	    "1:ble   7, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] > TOP  */

static void
ppc64_emit_gt_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "cmpd    7, 4, 3	\n"
	    "ldu     3, 8(30)	\n"
	    "1:bgt   7, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Goto if stack[--sp] >= TOP  */

static void
ppc64_emit_ge_goto (int *offset_p, int *size_p)
{
  EMIT_ASM ("ldu     4, 8(30)	\n"
	    "cmpd    7, 4, 3	\n"
	    "ldu     3, 8(30)	\n"
	    "1:bge   7, 1b	\n");

  if (offset_p)
    *offset_p = 12;
  if (size_p)
    *size_p = 14;
}

/* Table of emit ops for 64-bit ELFv1.  */

static struct emit_ops ppc64v1_emit_ops_impl =
{
  ppc64v1_emit_prologue,
  ppc64_emit_epilogue,
  ppc64_emit_add,
  ppc64_emit_sub,
  ppc64_emit_mul,
  ppc64_emit_lsh,
  ppc64_emit_rsh_signed,
  ppc64_emit_rsh_unsigned,
  ppc64_emit_ext,
  ppc64_emit_log_not,
  ppc64_emit_bit_and,
  ppc64_emit_bit_or,
  ppc64_emit_bit_xor,
  ppc64_emit_bit_not,
  ppc64_emit_equal,
  ppc64_emit_less_signed,
  ppc64_emit_less_unsigned,
  ppc64_emit_ref,
  ppc64_emit_if_goto,
  ppc_emit_goto,
  ppc_write_goto_address,
  ppc64_emit_const,
  ppc64v1_emit_call,
  ppc64v1_emit_reg,
  ppc64_emit_pop,
  ppc64_emit_stack_flush,
  ppc64_emit_zero_ext,
  ppc64_emit_swap,
  ppc_emit_stack_adjust,
  ppc64v1_emit_int_call_1,
  ppc64v1_emit_void_call_2,
  ppc64_emit_eq_goto,
  ppc64_emit_ne_goto,
  ppc64_emit_lt_goto,
  ppc64_emit_le_goto,
  ppc64_emit_gt_goto,
  ppc64_emit_ge_goto
};

/* Table of emit ops for 64-bit ELFv2.  */

static struct emit_ops ppc64v2_emit_ops_impl =
{
  ppc64v2_emit_prologue,
  ppc64_emit_epilogue,
  ppc64_emit_add,
  ppc64_emit_sub,
  ppc64_emit_mul,
  ppc64_emit_lsh,
  ppc64_emit_rsh_signed,
  ppc64_emit_rsh_unsigned,
  ppc64_emit_ext,
  ppc64_emit_log_not,
  ppc64_emit_bit_and,
  ppc64_emit_bit_or,
  ppc64_emit_bit_xor,
  ppc64_emit_bit_not,
  ppc64_emit_equal,
  ppc64_emit_less_signed,
  ppc64_emit_less_unsigned,
  ppc64_emit_ref,
  ppc64_emit_if_goto,
  ppc_emit_goto,
  ppc_write_goto_address,
  ppc64_emit_const,
  ppc64v2_emit_call,
  ppc64v2_emit_reg,
  ppc64_emit_pop,
  ppc64_emit_stack_flush,
  ppc64_emit_zero_ext,
  ppc64_emit_swap,
  ppc_emit_stack_adjust,
  ppc64v2_emit_int_call_1,
  ppc64v2_emit_void_call_2,
  ppc64_emit_eq_goto,
  ppc64_emit_ne_goto,
  ppc64_emit_lt_goto,
  ppc64_emit_le_goto,
  ppc64_emit_gt_goto,
  ppc64_emit_ge_goto
};

#endif

/* Implementation of target ops method "emit_ops".  */

emit_ops *
ppc_target::emit_ops ()
{
#ifdef __powerpc64__
  struct regcache *regcache = get_thread_regcache (current_thread, 0);

  if (register_size (regcache->tdesc, 0) == 8)
    {
      if (is_elfv2_inferior ())
	return &ppc64v2_emit_ops_impl;
      else
	return &ppc64v1_emit_ops_impl;
    }
#endif
  return &ppc_emit_ops_impl;
}

/* Implementation of target ops method "get_ipa_tdesc_idx".  */

int
ppc_target::get_ipa_tdesc_idx ()
{
  struct regcache *regcache = get_thread_regcache (current_thread, 0);
  const struct target_desc *tdesc = regcache->tdesc;

#ifdef __powerpc64__
  if (tdesc == tdesc_powerpc_64l)
    return PPC_TDESC_BASE;
  if (tdesc == tdesc_powerpc_altivec64l)
    return PPC_TDESC_ALTIVEC;
  if (tdesc == tdesc_powerpc_vsx64l)
    return PPC_TDESC_VSX;
  if (tdesc == tdesc_powerpc_isa205_64l)
    return PPC_TDESC_ISA205;
  if (tdesc == tdesc_powerpc_isa205_altivec64l)
    return PPC_TDESC_ISA205_ALTIVEC;
  if (tdesc == tdesc_powerpc_isa205_vsx64l)
    return PPC_TDESC_ISA205_VSX;
  if (tdesc == tdesc_powerpc_isa205_ppr_dscr_vsx64l)
    return PPC_TDESC_ISA205_PPR_DSCR_VSX;
  if (tdesc == tdesc_powerpc_isa207_vsx64l)
    return PPC_TDESC_ISA207_VSX;
  if (tdesc == tdesc_powerpc_isa207_htm_vsx64l)
    return PPC_TDESC_ISA207_HTM_VSX;
#endif

  if (tdesc == tdesc_powerpc_32l)
    return PPC_TDESC_BASE;
  if (tdesc == tdesc_powerpc_altivec32l)
    return PPC_TDESC_ALTIVEC;
  if (tdesc == tdesc_powerpc_vsx32l)
    return PPC_TDESC_VSX;
  if (tdesc == tdesc_powerpc_isa205_32l)
    return PPC_TDESC_ISA205;
  if (tdesc == tdesc_powerpc_isa205_altivec32l)
    return PPC_TDESC_ISA205_ALTIVEC;
  if (tdesc == tdesc_powerpc_isa205_vsx32l)
    return PPC_TDESC_ISA205_VSX;
  if (tdesc == tdesc_powerpc_isa205_ppr_dscr_vsx32l)
    return PPC_TDESC_ISA205_PPR_DSCR_VSX;
  if (tdesc == tdesc_powerpc_isa207_vsx32l)
    return PPC_TDESC_ISA207_VSX;
  if (tdesc == tdesc_powerpc_isa207_htm_vsx32l)
    return PPC_TDESC_ISA207_HTM_VSX;
  if (tdesc == tdesc_powerpc_e500l)
    return PPC_TDESC_E500;

  return 0;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_ppc_target;

void
initialize_low_arch (void)
{
  /* Initialize the Linux target descriptions.  */

  init_registers_powerpc_32l ();
  init_registers_powerpc_altivec32l ();
  init_registers_powerpc_vsx32l ();
  init_registers_powerpc_isa205_32l ();
  init_registers_powerpc_isa205_altivec32l ();
  init_registers_powerpc_isa205_vsx32l ();
  init_registers_powerpc_isa205_ppr_dscr_vsx32l ();
  init_registers_powerpc_isa207_vsx32l ();
  init_registers_powerpc_isa207_htm_vsx32l ();
  init_registers_powerpc_e500l ();
#if __powerpc64__
  init_registers_powerpc_64l ();
  init_registers_powerpc_altivec64l ();
  init_registers_powerpc_vsx64l ();
  init_registers_powerpc_isa205_64l ();
  init_registers_powerpc_isa205_altivec64l ();
  init_registers_powerpc_isa205_vsx64l ();
  init_registers_powerpc_isa205_ppr_dscr_vsx64l ();
  init_registers_powerpc_isa207_vsx64l ();
  init_registers_powerpc_isa207_htm_vsx64l ();
#endif

  initialize_regsets_info (&ppc_regsets_info);
}

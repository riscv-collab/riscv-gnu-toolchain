/* Target-dependent code for the IA-64 for GDB, the GNU debugger.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "ia64-tdep.h"
#include "arch-utils.h"
#include "gdbcore.h"
#include "regcache.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "linux-tdep.h"
#include "regset.h"

#include <ctype.h>

/* The sigtramp code is in a non-readable (executable-only) region
   of memory called the ``gate page''.  The addresses in question
   were determined by examining the system headers.  They are
   overly generous to allow for different pages sizes.  */

#define GATE_AREA_START 0xa000000000000100LL
#define GATE_AREA_END   0xa000000000020000LL

/* Offset to sigcontext structure from frame of handler.  */
#define IA64_LINUX_SIGCONTEXT_OFFSET 192

static int
ia64_linux_pc_in_sigtramp (CORE_ADDR pc)
{
  return (pc >= (CORE_ADDR) GATE_AREA_START && pc < (CORE_ADDR) GATE_AREA_END);
}

/* IA-64 GNU/Linux specific function which, given a frame address and
   a register number, returns the address at which that register may be
   found.  0 is returned for registers which aren't stored in the
   sigcontext structure.  */

static CORE_ADDR
ia64_linux_sigcontext_register_address (struct gdbarch *gdbarch,
					CORE_ADDR sp, int regno)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];
  CORE_ADDR sigcontext_addr = 0;

  /* The address of the sigcontext area is found at offset 16 in the
     sigframe.  */
  read_memory (sp + 16, buf, 8);
  sigcontext_addr = extract_unsigned_integer (buf, 8, byte_order);

  if (IA64_GR0_REGNUM <= regno && regno <= IA64_GR31_REGNUM)
    return sigcontext_addr + 200 + 8 * (regno - IA64_GR0_REGNUM);
  else if (IA64_BR0_REGNUM <= regno && regno <= IA64_BR7_REGNUM)
    return sigcontext_addr + 136 + 8 * (regno - IA64_BR0_REGNUM);
  else if (IA64_FR0_REGNUM <= regno && regno <= IA64_FR127_REGNUM)
    return sigcontext_addr + 464 + 16 * (regno - IA64_FR0_REGNUM);
  else
    switch (regno)
      {
      case IA64_IP_REGNUM :
	return sigcontext_addr + 40;
      case IA64_CFM_REGNUM :
	return sigcontext_addr + 48;
      case IA64_PSR_REGNUM :
	return sigcontext_addr + 56;		/* user mask only */
      /* sc_ar_rsc is provided, from which we could compute bspstore, but
	 I don't think it's worth it.  Anyway, if we want it, it's at offset
	 64.  */
      case IA64_BSP_REGNUM :
	return sigcontext_addr + 72;
      case IA64_RNAT_REGNUM :
	return sigcontext_addr + 80;
      case IA64_CCV_REGNUM :
	return sigcontext_addr + 88;
      case IA64_UNAT_REGNUM :
	return sigcontext_addr + 96;
      case IA64_FPSR_REGNUM :
	return sigcontext_addr + 104;
      case IA64_PFS_REGNUM :
	return sigcontext_addr + 112;
      case IA64_LC_REGNUM :
	return sigcontext_addr + 120;
      case IA64_PR_REGNUM :
	return sigcontext_addr + 128;
      default :
	return 0;
      }
}

static void
ia64_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  ia64_write_pc (regcache, pc);

  /* We must be careful with modifying the instruction-pointer: if we
     just interrupt a system call, the kernel would ordinarily try to
     restart it when we resume the inferior, which typically results
     in SIGSEGV or SIGILL.  We prevent this by clearing r10, which
     will tell the kernel that r8 does NOT contain a valid error code
     and hence it will skip system-call restart.

     The clearing of r10 is safe as long as ia64_write_pc() is only
     called as part of setting up an inferior call.  */
  regcache_cooked_write_unsigned (regcache, IA64_GR10_REGNUM, 0);
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
ia64_linux_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return ((isdigit (*s) && s[1] == '[' && s[2] == 'r') /* Displacement.  */
	  || *s == 'r' /* Register value.  */
	  || isdigit (*s));  /* Literal number.  */
}

/* Core file support. */

static const struct regcache_map_entry ia64_linux_gregmap[] =
  {
    { 32, IA64_GR0_REGNUM, 8 },	/* r0 ... r31 */
    { 1, REGCACHE_MAP_SKIP, 8 }, /* FIXME: NAT collection bits? */
    { 1, IA64_PR_REGNUM, 8 },
    { 8, IA64_BR0_REGNUM, 8 },	/* b0 ... b7 */
    { 1, IA64_IP_REGNUM, 8 },
    { 1, IA64_CFM_REGNUM, 8 },
    { 1, IA64_PSR_REGNUM, 8 },
    { 1, IA64_RSC_REGNUM, 8 },
    { 1, IA64_BSP_REGNUM, 8 },
    { 1, IA64_BSPSTORE_REGNUM, 8 },
    { 1, IA64_RNAT_REGNUM, 8 },
    { 1, IA64_CCV_REGNUM, 8 },
    { 1, IA64_UNAT_REGNUM, 8 },
    { 1, IA64_FPSR_REGNUM, 8 },
    { 1, IA64_PFS_REGNUM, 8 },
    { 1, IA64_LC_REGNUM, 8 },
    { 1, IA64_EC_REGNUM, 8 },
    { 0 }
  };

/* Size of 'gregset_t', as defined by the Linux kernel.  Note that
   this is more than actually mapped in the regmap above.  */

#define IA64_LINUX_GREGS_SIZE (128 * 8)

static const struct regcache_map_entry ia64_linux_fpregmap[] =
  {
    { 128, IA64_FR0_REGNUM, 16 }, /* f0 ... f127 */
    { 0 }
  };

#define IA64_LINUX_FPREGS_SIZE (128 * 16)

static void
ia64_linux_supply_fpregset (const struct regset *regset,
			    struct regcache *regcache,
			    int regnum, const void *regs, size_t len)
{
  const gdb_byte f_zero[16] = { 0 };
  const gdb_byte f_one[16] =
    { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xff, 0, 0, 0, 0, 0, 0 };

  regcache_supply_regset (regset, regcache, regnum, regs, len);

  /* Kernel generated cores have fr1==0 instead of 1.0.  Older GDBs
     did the same.  So ignore whatever might be recorded in fpregset_t
     for fr0/fr1 and always supply their expected values.  */
  if (regnum == -1 || regnum == IA64_FR0_REGNUM)
    regcache->raw_supply (IA64_FR0_REGNUM, f_zero);
  if (regnum == -1 || regnum == IA64_FR1_REGNUM)
    regcache->raw_supply (IA64_FR1_REGNUM, f_one);
}

static const struct regset ia64_linux_gregset =
  {
    ia64_linux_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

static const struct regset ia64_linux_fpregset =
  {
    ia64_linux_fpregmap,
    ia64_linux_supply_fpregset, regcache_collect_regset
  };

static void
ia64_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  cb (".reg", IA64_LINUX_GREGS_SIZE, IA64_LINUX_GREGS_SIZE, &ia64_linux_gregset,
      NULL, cb_data);
  cb (".reg2", IA64_LINUX_FPREGS_SIZE, IA64_LINUX_FPREGS_SIZE,
      &ia64_linux_fpregset, NULL, cb_data);
}

static void
ia64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);
  static const char *const stap_register_prefixes[] = { "r", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "[",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { "]",
								    NULL };

  linux_init_abi (info, gdbarch, 0);

  /* Set the method of obtaining the sigcontext addresses at which
     registers are saved.  */
  tdep->sigcontext_register_address = ia64_linux_sigcontext_register_address;

  /* Set the pc_in_sigtramp method.  */
  tdep->pc_in_sigtramp = ia64_linux_pc_in_sigtramp;

  set_gdbarch_write_pc (gdbarch, ia64_linux_write_pc);

  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_lp64_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Core file support. */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, ia64_linux_iterate_over_regset_sections);

  /* SystemTap related.  */
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					    stap_register_indirection_suffixes);
  set_gdbarch_stap_gdb_register_prefix (gdbarch, "r");
  set_gdbarch_stap_is_single_operand (gdbarch,
				      ia64_linux_stap_is_single_operand);
}

void _initialize_ia64_linux_tdep ();
void
_initialize_ia64_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_ia64, 0, GDB_OSABI_LINUX,
			  ia64_linux_init_abi);
}

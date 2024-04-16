/* Target-dependent code for QNX Neutrino x86.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Contributed by QNX Software Systems Ltd.

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
#include "frame.h"
#include "osabi.h"
#include "regcache.h"
#include "target.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "nto-tdep.h"
#include "solib.h"
#include "solib-svr4.h"

#ifndef X86_CPU_FXSR
#define X86_CPU_FXSR (1L << 12)
#endif

/* Why 13?  Look in our /usr/include/x86/context.h header at the
   x86_cpu_registers structure and you'll see an 'exx' junk register
   that is just filler.  Don't ask me, ask the kernel guys.  */
#define NUM_GPREGS 13

/* Mapping between the general-purpose registers in `struct xxx'
   format and GDB's register cache layout.  */

/* From <x86/context.h>.  */
static int i386nto_gregset_reg_offset[] =
{
  7 * 4,			/* %eax */
  6 * 4,			/* %ecx */
  5 * 4,			/* %edx */
  4 * 4,			/* %ebx */
  11 * 4,			/* %esp */
  2 * 4,			/* %epb */
  1 * 4,			/* %esi */
  0 * 4,			/* %edi */
  8 * 4,			/* %eip */
  10 * 4,			/* %eflags */
  9 * 4,			/* %cs */
  12 * 4,			/* %ss */
  -1				/* filler */
};

/* Given a GDB register number REGNUM, return the offset into
   Neutrino's register structure or -1 if the register is unknown.  */

static int
nto_reg_offset (int regnum)
{
  if (regnum >= 0 && regnum < ARRAY_SIZE (i386nto_gregset_reg_offset))
    return i386nto_gregset_reg_offset[regnum];

  return -1;
}

static void
i386nto_supply_gregset (struct regcache *regcache, char *gpregs)
{
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->gregset_reg_offset == i386nto_gregset_reg_offset);
  i386_gregset.supply_regset (&i386_gregset, regcache, -1,
			      gpregs, NUM_GPREGS * 4);
}

static void
i386nto_supply_fpregset (struct regcache *regcache, char *fpregs)
{
  if (nto_cpuinfo_valid && nto_cpuinfo_flags | X86_CPU_FXSR)
    i387_supply_fxsave (regcache, -1, fpregs);
  else
    i387_supply_fsave (regcache, -1, fpregs);
}

static void
i386nto_supply_regset (struct regcache *regcache, int regset, char *data)
{
  switch (regset)
    {
    case NTO_REG_GENERAL:
      i386nto_supply_gregset (regcache, data);
      break;
    case NTO_REG_FLOAT:
      i386nto_supply_fpregset (regcache, data);
      break;
    }
}

static int
i386nto_regset_id (int regno)
{
  if (regno == -1)
    return NTO_REG_END;
  else if (regno < I386_NUM_GREGS)
    return NTO_REG_GENERAL;
  else if (regno < I386_NUM_GREGS + I387_NUM_REGS)
    return NTO_REG_FLOAT;
  else if (regno < I386_SSE_NUM_REGS)
    return NTO_REG_FLOAT; /* We store xmm registers in fxsave_area.  */

  return -1;			/* Error.  */
}

static int
i386nto_register_area (struct gdbarch *gdbarch,
		       int regno, int regset, unsigned *off)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  *off = 0;
  if (regset == NTO_REG_GENERAL)
    {
      if (regno == -1)
	return NUM_GPREGS * 4;

      *off = nto_reg_offset (regno);
      if (*off == -1)
	return 0;
      return 4;
    }
  else if (regset == NTO_REG_FLOAT)
    {
      unsigned off_adjust, regsize, regset_size, regno_base;
      /* The following are flags indicating number in our fxsave_area.  */
      int first_four = (regno >= I387_FCTRL_REGNUM (tdep)
			&& regno <= I387_FISEG_REGNUM (tdep));
      int second_four = (regno > I387_FISEG_REGNUM (tdep)
			 && regno <= I387_FOP_REGNUM (tdep));
      int st_reg = (regno >= I387_ST0_REGNUM (tdep)
		    && regno < I387_ST0_REGNUM (tdep) + 8);
      int xmm_reg = (regno >= I387_XMM0_REGNUM (tdep)
		     && regno < I387_MXCSR_REGNUM (tdep));

      if (nto_cpuinfo_valid && nto_cpuinfo_flags | X86_CPU_FXSR)
	{
	  off_adjust = 32;
	  regsize = 16;
	  regset_size = 512;
	  /* fxsave_area structure.  */
	  if (first_four)
	    {
	      /* fpu_control_word, fpu_status_word, fpu_tag_word, fpu_operand
		 registers.  */
	      regsize = 2; /* Two bytes each.  */
	      off_adjust = 0;
	      regno_base = I387_FCTRL_REGNUM (tdep);
	    }
	  else if (second_four)
	    {
	      /* fpu_ip, fpu_cs, fpu_op, fpu_ds registers.  */
	      regsize = 4;
	      off_adjust = 8;
	      regno_base = I387_FISEG_REGNUM (tdep) + 1;
	    }
	  else if (st_reg)
	    {
	      /* ST registers.  */
	      regsize = 16;
	      off_adjust = 32;
	      regno_base = I387_ST0_REGNUM (tdep);
	    }
	  else if (xmm_reg)
	    {
	      /* XMM registers.  */
	      regsize = 16;
	      off_adjust = 160;
	      regno_base = I387_XMM0_REGNUM (tdep);
	    }
	  else if (regno == I387_MXCSR_REGNUM (tdep))
	    {
	      regsize = 4;
	      off_adjust = 24;
	      regno_base = I387_MXCSR_REGNUM (tdep);
	    }
	  else
	    {
	      /* Whole regset.  */
	      gdb_assert (regno == -1);
	      off_adjust = 0;
	      regno_base = 0;
	      regsize = regset_size;
	    }
	}
      else
	{
	  regset_size = 108;
	  /* fsave_area structure.  */
	  if (first_four || second_four)
	    {
	      /* fpu_control_word, ... , fpu_ds registers.  */
	      regsize = 4;
	      off_adjust = 0;
	      regno_base = I387_FCTRL_REGNUM (tdep);
	    }
	  else if (st_reg)
	    {
	      /* One of ST registers.  */
	      regsize = 10;
	      off_adjust = 7 * 4;
	      regno_base = I387_ST0_REGNUM (tdep);
	    }
	  else
	    {
	      /* Whole regset.  */
	      gdb_assert (regno == -1);
	      off_adjust = 0;
	      regno_base = 0;
	      regsize = regset_size;
	    }
	}

      if (regno != -1)
	*off = off_adjust + (regno - regno_base) * regsize;
      else
	*off = 0;
      return regsize;
    }
  return -1;
}

static int
i386nto_regset_fill (const struct regcache *regcache, int regset, char *data)
{
  if (regset == NTO_REG_GENERAL)
    {
      int regno;

      for (regno = 0; regno < NUM_GPREGS; regno++)
	{
	  int offset = nto_reg_offset (regno);
	  if (offset != -1)
	    regcache->raw_collect (regno, data + offset);
	}
    }
  else if (regset == NTO_REG_FLOAT)
    {
      if (nto_cpuinfo_valid && nto_cpuinfo_flags | X86_CPU_FXSR)
	i387_collect_fxsave (regcache, -1, data);
      else
	i387_collect_fsave (regcache, -1, data);
    }
  else
    return -1;

  return 0;
}

/* Return whether THIS_FRAME corresponds to a QNX Neutrino sigtramp
   routine.  */

static int
i386nto_sigtramp_p (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return name && strcmp ("__signalstub", name) == 0;
}

/* Assuming THIS_FRAME is a QNX Neutrino sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
i386nto_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  CORE_ADDR ptrctx;

  /* We store __ucontext_t addr in EDI register.  */
  get_frame_register (this_frame, I386_EDI_REGNUM, buf);
  ptrctx = extract_unsigned_integer (buf, 4, byte_order);
  ptrctx += 24 /* Context pointer is at this offset.  */;

  return ptrctx;
}

static void
init_i386nto_ops (void)
{
  nto_regset_id = i386nto_regset_id;
  nto_supply_gregset = i386nto_supply_gregset;
  nto_supply_fpregset = i386nto_supply_fpregset;
  nto_supply_altregset = nto_dummy_supply_regset;
  nto_supply_regset = i386nto_supply_regset;
  nto_register_area = i386nto_register_area;
  nto_regset_fill = i386nto_regset_fill;
  nto_fetch_link_map_offsets =
    svr4_ilp32_fetch_link_map_offsets;
}

static void
i386nto_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  static struct target_so_ops nto_svr4_so_ops;

  /* Deal with our strange signals.  */
  nto_initialize_signals ();

  /* NTO uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* Neutrino rewinds to look more normal.  Need to override the i386
     default which is [unfortunately] to decrement the PC.  */
  set_gdbarch_decr_pc_after_break (gdbarch, 0);

  tdep->gregset_reg_offset = i386nto_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386nto_gregset_reg_offset);
  tdep->sizeof_gregset = NUM_GPREGS * 4;

  tdep->sigtramp_p = i386nto_sigtramp_p;
  tdep->sigcontext_addr = i386nto_sigcontext_addr;
  tdep->sc_reg_offset = i386nto_gregset_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386nto_gregset_reg_offset);

  /* Setjmp()'s return PC saved in EDX (5).  */
  tdep->jb_pc_offset = 20;	/* 5x32 bit ints in.  */

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* Initialize this lazily, to avoid an initialization order
     dependency on solib-svr4.c's _initialize routine.  */
  if (nto_svr4_so_ops.in_dynsym_resolve_code == NULL)
    {
      nto_svr4_so_ops = svr4_so_ops;

      /* Our loader handles solib relocations differently than svr4.  */
      nto_svr4_so_ops.relocate_section_addresses
	= nto_relocate_section_addresses;

      /* Supply a nice function to find our solibs.  */
      nto_svr4_so_ops.find_and_open_solib
	= nto_find_and_open_solib;

      /* Our linker code is in libc.  */
      nto_svr4_so_ops.in_dynsym_resolve_code
	= nto_in_dynsym_resolve_code;
    }
  set_gdbarch_so_ops (gdbarch, &nto_svr4_so_ops);

  set_gdbarch_wchar_bit (gdbarch, 32);
  set_gdbarch_wchar_signed (gdbarch, 0);
}

void _initialize_i386nto_tdep ();
void
_initialize_i386nto_tdep ()
{
  init_i386nto_ops ();
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_QNXNTO,
			  i386nto_init_abi);
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_elf_flavour,
				  nto_elf_osabi_sniffer);
}

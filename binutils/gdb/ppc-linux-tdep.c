/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "objfiles.h"
#include "regcache.h"
#include "value.h"
#include "osabi.h"
#include "regset.h"
#include "solib-svr4.h"
#include "solib.h"
#include "solist.h"
#include "ppc-tdep.h"
#include "ppc64-tdep.h"
#include "ppc-linux-tdep.h"
#include "arch/ppc-linux-common.h"
#include "arch/ppc-linux-tdesc.h"
#include "glibc-tdep.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "tramp-frame.h"
#include "observable.h"
#include "auxv.h"
#include "elf/common.h"
#include "elf/ppc64.h"
#include "arch-utils.h"
#include "xml-syscall.h"
#include "linux-tdep.h"
#include "linux-record.h"
#include "record-full.h"
#include "infrun.h"
#include "expop.h"

#include "stap-probe.h"
#include "ax.h"
#include "ax-gdb.h"
#include "cli/cli-utils.h"
#include "parser-defs.h"
#include "user-regs.h"
#include <ctype.h>
#include "elf-bfd.h"
#include "producer.h"
#include "target-float.h"

#include "features/rs6000/powerpc-32l.c"
#include "features/rs6000/powerpc-altivec32l.c"
#include "features/rs6000/powerpc-vsx32l.c"
#include "features/rs6000/powerpc-isa205-32l.c"
#include "features/rs6000/powerpc-isa205-altivec32l.c"
#include "features/rs6000/powerpc-isa205-vsx32l.c"
#include "features/rs6000/powerpc-isa205-ppr-dscr-vsx32l.c"
#include "features/rs6000/powerpc-isa207-vsx32l.c"
#include "features/rs6000/powerpc-isa207-htm-vsx32l.c"
#include "features/rs6000/powerpc-64l.c"
#include "features/rs6000/powerpc-altivec64l.c"
#include "features/rs6000/powerpc-vsx64l.c"
#include "features/rs6000/powerpc-isa205-64l.c"
#include "features/rs6000/powerpc-isa205-altivec64l.c"
#include "features/rs6000/powerpc-isa205-vsx64l.c"
#include "features/rs6000/powerpc-isa205-ppr-dscr-vsx64l.c"
#include "features/rs6000/powerpc-isa207-vsx64l.c"
#include "features/rs6000/powerpc-isa207-htm-vsx64l.c"
#include "features/rs6000/powerpc-e500l.c"
#include "dwarf2/frame.h"

/* Shared library operations for PowerPC-Linux.  */
static struct target_so_ops powerpc_so_ops;

/* The syscall's XML filename for PPC and PPC64.  */
#define XML_SYSCALL_FILENAME_PPC "syscalls/ppc-linux.xml"
#define XML_SYSCALL_FILENAME_PPC64 "syscalls/ppc64-linux.xml"

/* ppc_linux_memory_remove_breakpoints attempts to remove a breakpoint
   in much the same fashion as memory_remove_breakpoint in mem-break.c,
   but is careful not to write back the previous contents if the code
   in question has changed in between inserting the breakpoint and
   removing it.

   Here is the problem that we're trying to solve...

   Once upon a time, before introducing this function to remove
   breakpoints from the inferior, setting a breakpoint on a shared
   library function prior to running the program would not work
   properly.  In order to understand the problem, it is first
   necessary to understand a little bit about dynamic linking on
   this platform.

   A call to a shared library function is accomplished via a bl
   (branch-and-link) instruction whose branch target is an entry
   in the procedure linkage table (PLT).  The PLT in the object
   file is uninitialized.  To gdb, prior to running the program, the
   entries in the PLT are all zeros.

   Once the program starts running, the shared libraries are loaded
   and the procedure linkage table is initialized, but the entries in
   the table are not (necessarily) resolved.  Once a function is
   actually called, the code in the PLT is hit and the function is
   resolved.  In order to better illustrate this, an example is in
   order; the following example is from the gdb testsuite.
	    
	We start the program shmain.

	    [kev@arroyo testsuite]$ ../gdb gdb.base/shmain
	    [...]

	We place two breakpoints, one on shr1 and the other on main.

	    (gdb) b shr1
	    Breakpoint 1 at 0x100409d4
	    (gdb) b main
	    Breakpoint 2 at 0x100006a0: file gdb.base/shmain.c, line 44.

	Examine the instruction (and the immediatly following instruction)
	upon which the breakpoint was placed.  Note that the PLT entry
	for shr1 contains zeros.

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      .long 0x0
	    0x100409d8 <shr1+4>:    .long 0x0

	Now run 'til main.

	    (gdb) r
	    Starting program: gdb.base/shmain 
	    Breakpoint 1 at 0xffaf790: file gdb.base/shr1.c, line 19.

	    Breakpoint 2, main ()
		at gdb.base/shmain.c:44
	    44        g = 1;

	Examine the PLT again.  Note that the loading of the shared
	library has initialized the PLT to code which loads a constant
	(which I think is an index into the GOT) into r11 and then
	branches a short distance to the code which actually does the
	resolving.

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      li      r11,4
	    0x100409d8 <shr1+4>:    b       0x10040984 <sg+4>
	    (gdb) c
	    Continuing.

	    Breakpoint 1, shr1 (x=1)
		at gdb.base/shr1.c:19
	    19        l = 1;

	Now we've hit the breakpoint at shr1.  (The breakpoint was
	reset from the PLT entry to the actual shr1 function after the
	shared library was loaded.) Note that the PLT entry has been
	resolved to contain a branch that takes us directly to shr1.
	(The real one, not the PLT entry.)

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      b       0xffaf76c <shr1>
	    0x100409d8 <shr1+4>:    b       0x10040984 <sg+4>

   The thing to note here is that the PLT entry for shr1 has been
   changed twice.

   Now the problem should be obvious.  GDB places a breakpoint (a
   trap instruction) on the zero value of the PLT entry for shr1.
   Later on, after the shared library had been loaded and the PLT
   initialized, GDB gets a signal indicating this fact and attempts
   (as it always does when it stops) to remove all the breakpoints.

   The breakpoint removal was causing the former contents (a zero
   word) to be written back to the now initialized PLT entry thus
   destroying a portion of the initialization that had occurred only a
   short time ago.  When execution continued, the zero word would be
   executed as an instruction an illegal instruction trap was
   generated instead.  (0 is not a legal instruction.)

   The fix for this problem was fairly straightforward.  The function
   memory_remove_breakpoint from mem-break.c was copied to this file,
   modified slightly, and renamed to ppc_linux_memory_remove_breakpoint.
   In tm-linux.h, MEMORY_REMOVE_BREAKPOINT is defined to call this new
   function.

   The differences between ppc_linux_memory_remove_breakpoint () and
   memory_remove_breakpoint () are minor.  All that the former does
   that the latter does not is check to make sure that the breakpoint
   location actually contains a breakpoint (trap instruction) prior
   to attempting to write back the old contents.  If it does contain
   a trap instruction, we allow the old contents to be written back.
   Otherwise, we silently do nothing.

   The big question is whether memory_remove_breakpoint () should be
   changed to have the same functionality.  The downside is that more
   traffic is generated for remote targets since we'll have an extra
   fetch of a memory word each time a breakpoint is removed.

   For the time being, we'll leave this self-modifying-code-friendly
   version in ppc-linux-tdep.c, but it ought to be migrated somewhere
   else in the event that some other platform has similar needs with
   regard to removing breakpoints in some potentially self modifying
   code.  */
static int
ppc_linux_memory_remove_breakpoint (struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->reqstd_address;
  const unsigned char *bp;
  int val;
  int bplen;
  gdb_byte old_contents[BREAKPOINT_MAX];

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &bplen);

  /* Make sure we see the memory breakpoints.  */
  scoped_restore restore_memory
    = make_scoped_restore_show_memory_breakpoints (1);
  val = target_read_memory (addr, old_contents, bplen);

  /* If our breakpoint is no longer at the address, this means that the
     program modified the code on us, so it is wrong to put back the
     old value.  */
  if (val == 0 && memcmp (bp, old_contents, bplen) == 0)
    val = target_write_raw_memory (addr, bp_tgt->shadow_contents, bplen);

  return val;
}

/* For historic reasons, PPC 32 GNU/Linux follows PowerOpen rather
   than the 32 bit SYSV R4 ABI structure return convention - all
   structures, no matter their size, are put in memory.  Vectors,
   which were added later, do get returned in a register though.  */

static enum return_value_convention
ppc_linux_return_value (struct gdbarch *gdbarch, struct value *function,
			struct type *valtype, struct regcache *regcache,
			struct value **read_value, const gdb_byte *writebuf)
{  
  gdb_byte *readbuf = nullptr;
  if (read_value != nullptr)
    {
      *read_value = value::allocate (valtype);
      readbuf = (*read_value)->contents_raw ().data ();
    }

  if ((valtype->code () == TYPE_CODE_STRUCT
       || valtype->code () == TYPE_CODE_UNION)
      && !((valtype->length () == 16 || valtype->length () == 8)
	   && valtype->is_vector ()))
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    return ppc_sysv_abi_return_value (gdbarch, function, valtype, regcache,
				      readbuf, writebuf);
}

/* PLT stub in an executable.  */
static const struct ppc_insn_pattern powerpc32_plt_stub[] =
  {
    { 0xffff0000, 0x3d600000, 0 },	/* lis   r11, xxxx	 */
    { 0xffff0000, 0x816b0000, 0 },	/* lwz   r11, xxxx(r11)  */
    { 0xffffffff, 0x7d6903a6, 0 },	/* mtctr r11		 */
    { 0xffffffff, 0x4e800420, 0 },	/* bctr			 */
    {          0,          0, 0 }
  };

/* PLT stubs in a shared library or PIE.
   The first variant is used when the PLT entry is within +/-32k of
   the GOT pointer (r30).  */
static const struct ppc_insn_pattern powerpc32_plt_stub_so_1[] =
  {
    { 0xffff0000, 0x817e0000, 0 },	/* lwz   r11, xxxx(r30)  */
    { 0xffffffff, 0x7d6903a6, 0 },	/* mtctr r11		 */
    { 0xffffffff, 0x4e800420, 0 },	/* bctr			 */
    {          0,          0, 0 }
  };

/* The second variant is used when the PLT entry is more than +/-32k
   from the GOT pointer (r30).  */
static const struct ppc_insn_pattern powerpc32_plt_stub_so_2[] =
  {
    { 0xffff0000, 0x3d7e0000, 0 },	/* addis r11, r30, xxxx  */
    { 0xffff0000, 0x816b0000, 0 },	/* lwz   r11, xxxx(r11)  */
    { 0xffffffff, 0x7d6903a6, 0 },	/* mtctr r11		 */
    { 0xffffffff, 0x4e800420, 0 },	/* bctr			 */
    {          0,          0, 0 }
  };

/* The max number of insns we check using ppc_insns_match_pattern.  */
#define POWERPC32_PLT_CHECK_LEN (ARRAY_SIZE (powerpc32_plt_stub) - 1)

/* Check if PC is in PLT stub.  For non-secure PLT, stub is in .plt
   section.  For secure PLT, stub is in .text and we need to check
   instruction patterns.  */

static int
powerpc_linux_in_dynsym_resolve_code (CORE_ADDR pc)
{
  struct bound_minimal_symbol sym;

  /* Check whether PC is in the dynamic linker.  This also checks
     whether it is in the .plt section, used by non-PIC executables.  */
  if (svr4_in_dynsym_resolve_code (pc))
    return 1;

  /* Check if we are in the resolver.  */
  sym = lookup_minimal_symbol_by_pc (pc);
  if (sym.minsym != NULL
      && (strcmp (sym.minsym->linkage_name (), "__glink") == 0
	  || strcmp (sym.minsym->linkage_name (), "__glink_PLTresolve") == 0))
    return 1;

  return 0;
}

/* Follow PLT stub to actual routine.

   When the execution direction is EXEC_REVERSE, scan backward to
   check whether we are in the middle of a PLT stub.  Currently,
   we only look-behind at most 4 instructions (the max length of a PLT
   stub sequence.  */

static CORE_ADDR
ppc_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  unsigned int insnbuf[POWERPC32_PLT_CHECK_LEN];
  struct gdbarch *gdbarch = get_frame_arch (frame);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR target = 0;
  int scan_limit, i;

  scan_limit = 1;
  /* When reverse-debugging, scan backward to check whether we are
     in the middle of trampoline code.  */
  if (execution_direction == EXEC_REVERSE)
    scan_limit = 4;	/* At most 4 instructions.  */

  for (i = 0; i < scan_limit; i++)
    {
      if (ppc_insns_match_pattern (frame, pc, powerpc32_plt_stub, insnbuf))
	{
	  /* Calculate PLT entry address from
	     lis   r11, xxxx
	     lwz   r11, xxxx(r11).  */
	  target = ((ppc_insn_d_field (insnbuf[0]) << 16)
		    + ppc_insn_d_field (insnbuf[1]));
	}
      else if (i < ARRAY_SIZE (powerpc32_plt_stub_so_1) - 1
	       && ppc_insns_match_pattern (frame, pc, powerpc32_plt_stub_so_1,
					   insnbuf))
	{
	  /* Calculate PLT entry address from
	     lwz   r11, xxxx(r30).  */
	  target = (ppc_insn_d_field (insnbuf[0])
		    + get_frame_register_unsigned (frame,
						   tdep->ppc_gp0_regnum + 30));
	}
      else if (ppc_insns_match_pattern (frame, pc, powerpc32_plt_stub_so_2,
					insnbuf))
	{
	  /* Calculate PLT entry address from
	     addis r11, r30, xxxx
	     lwz   r11, xxxx(r11).  */
	  target = ((ppc_insn_d_field (insnbuf[0]) << 16)
		    + ppc_insn_d_field (insnbuf[1])
		    + get_frame_register_unsigned (frame,
						   tdep->ppc_gp0_regnum + 30));
	}
      else
	{
	  /* Scan backward one more instruction if it doesn't match.  */
	  pc -= 4;
	  continue;
	}

      target = read_memory_unsigned_integer (target, 4, byte_order);
      return target;
    }

  return 0;
}

/* Wrappers to handle Linux-only registers.  */

static void
ppc_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t len)
{
  const struct ppc_reg_offsets *offsets
    = (const struct ppc_reg_offsets *) regset->regmap;

  ppc_supply_gregset (regset, regcache, regnum, gregs, len);

  if (ppc_linux_trap_reg_p (regcache->arch ()))
    {
      /* "orig_r3" is stored 2 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_ORIG_R3_REGNUM)
	ppc_supply_reg (regcache, PPC_ORIG_R3_REGNUM, (const gdb_byte *) gregs,
			offsets->pc_offset + 2 * offsets->gpr_size,
			offsets->gpr_size);

      /* "trap" is stored 8 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_TRAP_REGNUM)
	ppc_supply_reg (regcache, PPC_TRAP_REGNUM, (const gdb_byte *) gregs,
			offsets->pc_offset + 8 * offsets->gpr_size,
			offsets->gpr_size);
    }
}

static void
ppc_linux_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs, size_t len)
{
  const struct ppc_reg_offsets *offsets
    = (const struct ppc_reg_offsets *) regset->regmap;

  /* Clear areas in the linux gregset not written elsewhere.  */
  if (regnum == -1)
    memset (gregs, 0, len);

  ppc_collect_gregset (regset, regcache, regnum, gregs, len);

  if (ppc_linux_trap_reg_p (regcache->arch ()))
    {
      /* "orig_r3" is stored 2 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_ORIG_R3_REGNUM)
	ppc_collect_reg (regcache, PPC_ORIG_R3_REGNUM, (gdb_byte *) gregs,
			 offsets->pc_offset + 2 * offsets->gpr_size,
			 offsets->gpr_size);

      /* "trap" is stored 8 slots after "pc".  */
      if (regnum == -1 || regnum == PPC_TRAP_REGNUM)
	ppc_collect_reg (regcache, PPC_TRAP_REGNUM, (gdb_byte *) gregs,
			 offsets->pc_offset + 8 * offsets->gpr_size,
			 offsets->gpr_size);
    }
}

/* Regset descriptions.  */
static const struct ppc_reg_offsets ppc32_linux_reg_offsets =
  {
    /* General-purpose registers.  */
    /* .r0_offset = */ 0,
    /* .gpr_size = */ 4,
    /* .xr_size = */ 4,
    /* .pc_offset = */ 128,
    /* .ps_offset = */ 132,
    /* .cr_offset = */ 152,
    /* .lr_offset = */ 144,
    /* .ctr_offset = */ 140,
    /* .xer_offset = */ 148,
    /* .mq_offset = */ 156,

    /* Floating-point registers.  */
    /* .f0_offset = */ 0,
    /* .fpscr_offset = */ 256,
    /* .fpscr_size = */ 8
  };

static const struct ppc_reg_offsets ppc64_linux_reg_offsets =
  {
    /* General-purpose registers.  */
    /* .r0_offset = */ 0,
    /* .gpr_size = */ 8,
    /* .xr_size = */ 8,
    /* .pc_offset = */ 256,
    /* .ps_offset = */ 264,
    /* .cr_offset = */ 304,
    /* .lr_offset = */ 288,
    /* .ctr_offset = */ 280,
    /* .xer_offset = */ 296,
    /* .mq_offset = */ 312,

    /* Floating-point registers.  */
    /* .f0_offset = */ 0,
    /* .fpscr_offset = */ 256,
    /* .fpscr_size = */ 8
  };

static const struct regset ppc32_linux_gregset = {
  &ppc32_linux_reg_offsets,
  ppc_linux_supply_gregset,
  ppc_linux_collect_gregset
};

static const struct regset ppc64_linux_gregset = {
  &ppc64_linux_reg_offsets,
  ppc_linux_supply_gregset,
  ppc_linux_collect_gregset
};

static const struct regset ppc32_linux_fpregset = {
  &ppc32_linux_reg_offsets,
  ppc_supply_fpregset,
  ppc_collect_fpregset
};

static const struct regcache_map_entry ppc32_le_linux_vrregmap[] =
  {
      { 32, PPC_VR0_REGNUM, 16 },
      { 1, PPC_VSCR_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 12 },
      { 1, PPC_VRSAVE_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 12 },
      { 0 }
  };

static const struct regcache_map_entry ppc32_be_linux_vrregmap[] =
  {
      { 32, PPC_VR0_REGNUM, 16 },
      { 1, REGCACHE_MAP_SKIP, 12},
      { 1, PPC_VSCR_REGNUM, 4 },
      { 1, PPC_VRSAVE_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 12 },
      { 0 }
  };

static const struct regset ppc32_le_linux_vrregset = {
  ppc32_le_linux_vrregmap,
  regcache_supply_regset,
  regcache_collect_regset
};

static const struct regset ppc32_be_linux_vrregset = {
  ppc32_be_linux_vrregmap,
  regcache_supply_regset,
  regcache_collect_regset
};

static const struct regcache_map_entry ppc32_linux_vsxregmap[] =
  {
      { 32, PPC_VSR0_UPPER_REGNUM, 8 },
      { 0 }
  };

static const struct regset ppc32_linux_vsxregset = {
  ppc32_linux_vsxregmap,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Program Priorty Register regmap.  */

static const struct regcache_map_entry ppc32_regmap_ppr[] =
  {
      { 1, PPC_PPR_REGNUM, 8 },
      { 0 }
  };

/* Program Priorty Register regset.  */

const struct regset ppc32_linux_pprregset = {
  ppc32_regmap_ppr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Data Stream Control Register regmap.  */

static const struct regcache_map_entry ppc32_regmap_dscr[] =
  {
      { 1, PPC_DSCR_REGNUM, 8 },
      { 0 }
  };

/* Data Stream Control Register regset.  */

const struct regset ppc32_linux_dscrregset = {
  ppc32_regmap_dscr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Target Address Register regmap.  */

static const struct regcache_map_entry ppc32_regmap_tar[] =
  {
      { 1, PPC_TAR_REGNUM, 8 },
      { 0 }
  };

/* Target Address Register regset.  */

const struct regset ppc32_linux_tarregset = {
  ppc32_regmap_tar,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Event-Based Branching regmap.  */

static const struct regcache_map_entry ppc32_regmap_ebb[] =
  {
      { 1, PPC_EBBRR_REGNUM, 8 },
      { 1, PPC_EBBHR_REGNUM, 8 },
      { 1, PPC_BESCR_REGNUM, 8 },
      { 0 }
  };

/* Event-Based Branching regset.  */

const struct regset ppc32_linux_ebbregset = {
  ppc32_regmap_ebb,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Performance Monitoring Unit regmap.  */

static const struct regcache_map_entry ppc32_regmap_pmu[] =
  {
      { 1, PPC_SIAR_REGNUM, 8 },
      { 1, PPC_SDAR_REGNUM, 8 },
      { 1, PPC_SIER_REGNUM, 8 },
      { 1, PPC_MMCR2_REGNUM, 8 },
      { 1, PPC_MMCR0_REGNUM, 8 },
      { 0 }
  };

/* Performance Monitoring Unit regset.  */

const struct regset ppc32_linux_pmuregset = {
  ppc32_regmap_pmu,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Hardware Transactional Memory special-purpose register regmap.  */

static const struct regcache_map_entry ppc32_regmap_tm_spr[] =
  {
      { 1, PPC_TFHAR_REGNUM, 8 },
      { 1, PPC_TEXASR_REGNUM, 8 },
      { 1, PPC_TFIAR_REGNUM, 8 },
      { 0 }
  };

/* Hardware Transactional Memory special-purpose register regset.  */

const struct regset ppc32_linux_tm_sprregset = {
  ppc32_regmap_tm_spr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Regmaps for the Hardware Transactional Memory checkpointed
   general-purpose regsets for 32-bit, 64-bit big-endian, and 64-bit
   little endian targets.  The ptrace and core file buffers for 64-bit
   targets use 8-byte fields for the 4-byte registers, and the
   position of the register in the fields depends on the endianness.
   The 32-bit regmap is the same for both endian types because the
   fields are all 4-byte long.

   The layout of checkpointed GPR regset is the same as a regular
   struct pt_regs, but we skip all registers that are not actually
   checkpointed by the processor (e.g. msr, nip), except when
   generating a core file.  The 64-bit regset is 48 * 8 bytes long.
   In some 64-bit kernels, the regset for a 32-bit inferior has the
   same length, but all the registers are squeezed in the first half
   (48 * 4 bytes).  The pt_regs struct calls the regular cr ccr, but
   we use ccr for "checkpointed condition register".  Note that CR
   (condition register) field 0 is not checkpointed, but the kernel
   returns all 4 bytes.  The skipped registers should not be touched
   when writing the regset to the inferior (with
   PTRACE_SETREGSET).  */

static const struct regcache_map_entry ppc32_regmap_cgpr[] =
  {
      { 32, PPC_CR0_REGNUM, 4 },
      { 3, REGCACHE_MAP_SKIP, 4 }, /* nip, msr, orig_gpr3.  */
      { 1, PPC_CCTR_REGNUM, 4 },
      { 1, PPC_CLR_REGNUM, 4 },
      { 1, PPC_CXER_REGNUM, 4 },
      { 1, PPC_CCR_REGNUM, 4 },
      { 9, REGCACHE_MAP_SKIP, 4 }, /* All the rest.  */
      { 0 }
  };

static const struct regcache_map_entry ppc64_le_regmap_cgpr[] =
  {
      { 32, PPC_CR0_REGNUM, 8 },
      { 3, REGCACHE_MAP_SKIP, 8 },
      { 1, PPC_CCTR_REGNUM, 8 },
      { 1, PPC_CLR_REGNUM, 8 },
      { 1, PPC_CXER_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 4 }, /* CXER padding.  */
      { 1, PPC_CCR_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 4}, /* CCR padding.  */
      { 9, REGCACHE_MAP_SKIP, 8},
      { 0 }
  };

static const struct regcache_map_entry ppc64_be_regmap_cgpr[] =
  {
      { 32, PPC_CR0_REGNUM, 8 },
      { 3, REGCACHE_MAP_SKIP, 8 },
      { 1, PPC_CCTR_REGNUM, 8 },
      { 1, PPC_CLR_REGNUM, 8 },
      { 1, REGCACHE_MAP_SKIP, 4}, /* CXER padding.  */
      { 1, PPC_CXER_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 4}, /* CCR padding.  */
      { 1, PPC_CCR_REGNUM, 4 },
      { 9, REGCACHE_MAP_SKIP, 8},
      { 0 }
  };

/* Regsets for the Hardware Transactional Memory checkpointed
   general-purpose registers for 32-bit, 64-bit big-endian, and 64-bit
   little endian targets.

   Some 64-bit kernels generate a checkpointed gpr note section with
   48*8 bytes for a 32-bit thread, of which only 48*4 are actually
   used, so we set the variable size flag in the corresponding regset
   to accept this case.  */

static const struct regset ppc32_linux_cgprregset = {
  ppc32_regmap_cgpr,
  regcache_supply_regset,
  regcache_collect_regset,
  REGSET_VARIABLE_SIZE
};

static const struct regset ppc64_be_linux_cgprregset = {
  ppc64_be_regmap_cgpr,
  regcache_supply_regset,
  regcache_collect_regset
};

static const struct regset ppc64_le_linux_cgprregset = {
  ppc64_le_regmap_cgpr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Hardware Transactional Memory checkpointed floating-point regmap.  */

static const struct regcache_map_entry ppc32_regmap_cfpr[] =
  {
      { 32, PPC_CF0_REGNUM, 8 },
      { 1, PPC_CFPSCR_REGNUM, 8 },
      { 0 }
  };

/* Hardware Transactional Memory checkpointed floating-point regset.  */

const struct regset ppc32_linux_cfprregset = {
  ppc32_regmap_cfpr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Regmaps for the Hardware Transactional Memory checkpointed vector
   regsets, for big and little endian targets.  The position of the
   4-byte VSCR in its 16-byte field depends on the endianness.  */

static const struct regcache_map_entry ppc32_le_regmap_cvmx[] =
  {
      { 32, PPC_CVR0_REGNUM, 16 },
      { 1, PPC_CVSCR_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 12 },
      { 1, PPC_CVRSAVE_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 12 },
      { 0 }
  };

static const struct regcache_map_entry ppc32_be_regmap_cvmx[] =
  {
      { 32, PPC_CVR0_REGNUM, 16 },
      { 1, REGCACHE_MAP_SKIP, 12 },
      { 1, PPC_CVSCR_REGNUM, 4 },
      { 1, PPC_CVRSAVE_REGNUM, 4 },
      { 1, REGCACHE_MAP_SKIP, 12},
      { 0 }
  };

/* Hardware Transactional Memory checkpointed vector regsets, for little
   and big endian targets.  */

static const struct regset ppc32_le_linux_cvmxregset = {
  ppc32_le_regmap_cvmx,
  regcache_supply_regset,
  regcache_collect_regset
};

static const struct regset ppc32_be_linux_cvmxregset = {
  ppc32_be_regmap_cvmx,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Hardware Transactional Memory checkpointed vector-scalar regmap.  */

static const struct regcache_map_entry ppc32_regmap_cvsx[] =
  {
      { 32, PPC_CVSR0_UPPER_REGNUM, 8 },
      { 0 }
  };

/* Hardware Transactional Memory checkpointed vector-scalar regset.  */

const struct regset ppc32_linux_cvsxregset = {
  ppc32_regmap_cvsx,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Hardware Transactional Memory checkpointed Program Priority Register
   regmap.  */

static const struct regcache_map_entry ppc32_regmap_cppr[] =
  {
      { 1, PPC_CPPR_REGNUM, 8 },
      { 0 }
  };

/* Hardware Transactional Memory checkpointed Program Priority Register
   regset.  */

const struct regset ppc32_linux_cpprregset = {
  ppc32_regmap_cppr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Hardware Transactional Memory checkpointed Data Stream Control
   Register regmap.  */

static const struct regcache_map_entry ppc32_regmap_cdscr[] =
  {
      { 1, PPC_CDSCR_REGNUM, 8 },
      { 0 }
  };

/* Hardware Transactional Memory checkpointed Data Stream Control
   Register regset.  */

const struct regset ppc32_linux_cdscrregset = {
  ppc32_regmap_cdscr,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Hardware Transactional Memory checkpointed Target Address Register
   regmap.  */

static const struct regcache_map_entry ppc32_regmap_ctar[] =
  {
      { 1, PPC_CTAR_REGNUM, 8 },
      { 0 }
  };

/* Hardware Transactional Memory checkpointed Target Address Register
   regset.  */

const struct regset ppc32_linux_ctarregset = {
  ppc32_regmap_ctar,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset *
ppc_linux_gregset (int wordsize)
{
  return wordsize == 8 ? &ppc64_linux_gregset : &ppc32_linux_gregset;
}

const struct regset *
ppc_linux_fpregset (void)
{
  return &ppc32_linux_fpregset;
}

const struct regset *
ppc_linux_vrregset (struct gdbarch *gdbarch)
{
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    return &ppc32_be_linux_vrregset;
  else
    return &ppc32_le_linux_vrregset;
}

const struct regset *
ppc_linux_vsxregset (void)
{
  return &ppc32_linux_vsxregset;
}

const struct regset *
ppc_linux_cgprregset (struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (tdep->wordsize == 4)
    {
      return &ppc32_linux_cgprregset;
    }
  else
    {
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	return &ppc64_be_linux_cgprregset;
      else
	return &ppc64_le_linux_cgprregset;
    }
}

const struct regset *
ppc_linux_cvmxregset (struct gdbarch *gdbarch)
{
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    return &ppc32_be_linux_cvmxregset;
  else
    return &ppc32_le_linux_cvmxregset;
}

/* Collect function used to generate the core note for the
   checkpointed GPR regset.  Here, we don't want to skip the
   "checkpointed" NIP and MSR, so that the note section we generate is
   similar to the one generated by the kernel.  To avoid having to
   define additional registers in GDB which are not actually
   checkpointed in the architecture, we copy TFHAR to the checkpointed
   NIP slot, which is what the kernel does, and copy the regular MSR
   to the checkpointed MSR slot, which will have a similar value in
   most cases.  */

static void
ppc_linux_collect_core_cpgrregset (const struct regset *regset,
				   const struct regcache *regcache,
				   int regnum, void *buf, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  const struct regset *cgprregset = ppc_linux_cgprregset (gdbarch);

  /* We collect the checkpointed GPRs already defined in the regular
     regmap, then overlay TFHAR/MSR on the checkpointed NIP/MSR
     slots.  */
  cgprregset->collect_regset (cgprregset, regcache, regnum, buf, len);

  /* Check that we are collecting all the registers, which should be
     the case when generating a core file.  */
  if (regnum != -1)
    return;

  /* PT_NIP and PT_MSR are 32 and 33 for powerpc.  Don't redefine
     these symbols since this file can run on clients in other
     architectures where they can already be defined to other
     values.  */
  int pt_offset = 32;

  /* Check that our buffer is long enough to hold two slots at
     pt_offset * wordsize, one for NIP and one for MSR.  */
  gdb_assert ((pt_offset + 2) * tdep->wordsize <= len);

  /* TFHAR is 8 bytes wide, but the NIP slot for a 32-bit thread is
     4-bytes long.  We use raw_collect_integer which handles
     differences in the sizes for the source and destination buffers
     for both endian modes.  */
  (regcache->raw_collect_integer
   (PPC_TFHAR_REGNUM, ((gdb_byte *) buf) + pt_offset * tdep->wordsize,
    tdep->wordsize, false));

  pt_offset = 33;

  (regcache->raw_collect_integer
   (PPC_MSR_REGNUM, ((gdb_byte *) buf) + pt_offset * tdep->wordsize,
    tdep->wordsize, false));
}

/* Iterate over supported core file register note sections. */

static void
ppc_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int have_altivec = tdep->ppc_vr0_regnum != -1;
  int have_vsx = tdep->ppc_vsr0_upper_regnum != -1;
  int have_ppr = tdep->ppc_ppr_regnum != -1;
  int have_dscr = tdep->ppc_dscr_regnum != -1;
  int have_tar = tdep->ppc_tar_regnum != -1;

  if (tdep->wordsize == 4)
    cb (".reg", 48 * 4, 48 * 4, &ppc32_linux_gregset, NULL, cb_data);
  else
    cb (".reg", 48 * 8, 48 * 8, &ppc64_linux_gregset, NULL, cb_data);

  cb (".reg2", 264, 264, &ppc32_linux_fpregset, NULL, cb_data);

  if (have_altivec)
    {
      const struct regset *vrregset = ppc_linux_vrregset (gdbarch);
      cb (".reg-ppc-vmx", PPC_LINUX_SIZEOF_VRREGSET, PPC_LINUX_SIZEOF_VRREGSET,
	  vrregset, "ppc Altivec", cb_data);
    }

  if (have_vsx)
    cb (".reg-ppc-vsx", PPC_LINUX_SIZEOF_VSXREGSET, PPC_LINUX_SIZEOF_VSXREGSET,
	&ppc32_linux_vsxregset, "POWER7 VSX", cb_data);

  if (have_ppr)
    cb (".reg-ppc-ppr", PPC_LINUX_SIZEOF_PPRREGSET,
	PPC_LINUX_SIZEOF_PPRREGSET,
	&ppc32_linux_pprregset, "Priority Program Register", cb_data);

  if (have_dscr)
    cb (".reg-ppc-dscr", PPC_LINUX_SIZEOF_DSCRREGSET,
	PPC_LINUX_SIZEOF_DSCRREGSET,
	&ppc32_linux_dscrregset, "Data Stream Control Register",
	cb_data);

  if (have_tar)
    cb (".reg-ppc-tar", PPC_LINUX_SIZEOF_TARREGSET,
	PPC_LINUX_SIZEOF_TARREGSET,
	&ppc32_linux_tarregset, "Target Address Register", cb_data);

  /* EBB registers are unavailable when ptrace returns ENODATA.  Check
     availability when generating a core file (regcache != NULL).  */
  if (tdep->have_ebb)
    if (regcache == NULL
	|| REG_VALID == regcache->get_register_status (PPC_BESCR_REGNUM))
      cb (".reg-ppc-ebb", PPC_LINUX_SIZEOF_EBBREGSET,
	  PPC_LINUX_SIZEOF_EBBREGSET,
	  &ppc32_linux_ebbregset, "Event-based Branching Registers",
	  cb_data);

  if (tdep->ppc_mmcr0_regnum != -1)
    cb (".reg-ppc-pmu", PPC_LINUX_SIZEOF_PMUREGSET,
	PPC_LINUX_SIZEOF_PMUREGSET,
	&ppc32_linux_pmuregset, "Performance Monitor Registers",
	cb_data);

  if (tdep->have_htm_spr)
    cb (".reg-ppc-tm-spr", PPC_LINUX_SIZEOF_TM_SPRREGSET,
	PPC_LINUX_SIZEOF_TM_SPRREGSET,
	&ppc32_linux_tm_sprregset,
	"Hardware Transactional Memory Special Purpose Registers",
	cb_data);

  /* Checkpointed registers can be unavailable, don't call back if
     we are generating a core file.  */

  if (tdep->have_htm_core)
    {
      /* Only generate the checkpointed GPR core note if we also have
	 access to the HTM SPRs, because we need TFHAR to fill the
	 "checkpointed" NIP slot.  We can read a core file without it
	 since GDB is not aware of this NIP as a visible register.  */
      if (regcache == NULL ||
	  (REG_VALID == regcache->get_register_status (PPC_CR0_REGNUM)
	   && tdep->have_htm_spr))
	{
	  int cgpr_size = (tdep->wordsize == 4?
			   PPC32_LINUX_SIZEOF_CGPRREGSET
			   : PPC64_LINUX_SIZEOF_CGPRREGSET);

	  const struct regset *cgprregset =
	    ppc_linux_cgprregset (gdbarch);

	  if (regcache != NULL)
	    {
	      struct regset core_cgprregset = *cgprregset;

	      core_cgprregset.collect_regset
		= ppc_linux_collect_core_cpgrregset;

	      cb (".reg-ppc-tm-cgpr",
		  cgpr_size, cgpr_size,
		  &core_cgprregset,
		  "Checkpointed General Purpose Registers", cb_data);
	    }
	  else
	    {
	      cb (".reg-ppc-tm-cgpr",
		  cgpr_size, cgpr_size,
		  cgprregset,
		  "Checkpointed General Purpose Registers", cb_data);
	    }
	}
    }

  if (tdep->have_htm_fpu)
    {
      if (regcache == NULL ||
	  REG_VALID == regcache->get_register_status (PPC_CF0_REGNUM))
	cb (".reg-ppc-tm-cfpr", PPC_LINUX_SIZEOF_CFPRREGSET,
	    PPC_LINUX_SIZEOF_CFPRREGSET,
	    &ppc32_linux_cfprregset,
	    "Checkpointed Floating Point Registers", cb_data);
    }

  if (tdep->have_htm_altivec)
    {
      if (regcache == NULL ||
	  REG_VALID == regcache->get_register_status (PPC_CVR0_REGNUM))
	{
	  const struct regset *cvmxregset =
	    ppc_linux_cvmxregset (gdbarch);

	  cb (".reg-ppc-tm-cvmx", PPC_LINUX_SIZEOF_CVMXREGSET,
	      PPC_LINUX_SIZEOF_CVMXREGSET,
	      cvmxregset,
	      "Checkpointed Altivec (VMX) Registers", cb_data);
	}
    }

  if (tdep->have_htm_vsx)
    {
      if (regcache == NULL ||
	  (REG_VALID
	   == regcache->get_register_status (PPC_CVSR0_UPPER_REGNUM)))
	cb (".reg-ppc-tm-cvsx", PPC_LINUX_SIZEOF_CVSXREGSET,
	    PPC_LINUX_SIZEOF_CVSXREGSET,
	    &ppc32_linux_cvsxregset,
	    "Checkpointed VSX Registers", cb_data);
    }

  if (tdep->ppc_cppr_regnum != -1)
    {
      if (regcache == NULL ||
	  REG_VALID == regcache->get_register_status (PPC_CPPR_REGNUM))
	cb (".reg-ppc-tm-cppr", PPC_LINUX_SIZEOF_CPPRREGSET,
	    PPC_LINUX_SIZEOF_CPPRREGSET,
	    &ppc32_linux_cpprregset,
	    "Checkpointed Priority Program Register", cb_data);
    }

  if (tdep->ppc_cdscr_regnum != -1)
    {
      if (regcache == NULL ||
	  REG_VALID == regcache->get_register_status (PPC_CDSCR_REGNUM))
	cb (".reg-ppc-tm-cdscr", PPC_LINUX_SIZEOF_CDSCRREGSET,
	    PPC_LINUX_SIZEOF_CDSCRREGSET,
	    &ppc32_linux_cdscrregset,
	    "Checkpointed Data Stream Control Register", cb_data);
    }

  if (tdep->ppc_ctar_regnum)
    {
      if ( regcache == NULL ||
	   REG_VALID == regcache->get_register_status (PPC_CTAR_REGNUM))
	cb (".reg-ppc-tm-ctar", PPC_LINUX_SIZEOF_CTARREGSET,
	    PPC_LINUX_SIZEOF_CTARREGSET,
	    &ppc32_linux_ctarregset,
	    "Checkpointed Target Address Register", cb_data);
    }
}

static void
ppc_linux_sigtramp_cache (frame_info_ptr this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func, LONGEST offset,
			  int bias)
{
  CORE_ADDR base;
  CORE_ADDR regs;
  CORE_ADDR gpregs;
  CORE_ADDR fpregs;
  int i;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  base = get_frame_register_unsigned (this_frame,
				      gdbarch_sp_regnum (gdbarch));
  if (bias > 0 && get_frame_pc (this_frame) != func)
    /* See below, some signal trampolines increment the stack as their
       first instruction, need to compensate for that.  */
    base -= bias;

  /* Find the address of the register buffer pointer.  */
  regs = base + offset;
  /* Use that to find the address of the corresponding register
     buffers.  */
  gpregs = read_memory_unsigned_integer (regs, tdep->wordsize, byte_order);
  fpregs = gpregs + 48 * tdep->wordsize;

  /* General purpose.  */
  for (i = 0; i < 32; i++)
    {
      int regnum = i + tdep->ppc_gp0_regnum;
      trad_frame_set_reg_addr (this_cache,
			       regnum, gpregs + i * tdep->wordsize);
    }
  trad_frame_set_reg_addr (this_cache,
			   gdbarch_pc_regnum (gdbarch),
			   gpregs + 32 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_ctr_regnum,
			   gpregs + 35 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_lr_regnum,
			   gpregs + 36 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_xer_regnum,
			   gpregs + 37 * tdep->wordsize);
  trad_frame_set_reg_addr (this_cache, tdep->ppc_cr_regnum,
			   gpregs + 38 * tdep->wordsize);

  if (ppc_linux_trap_reg_p (gdbarch))
    {
      trad_frame_set_reg_addr (this_cache, PPC_ORIG_R3_REGNUM,
			       gpregs + 34 * tdep->wordsize);
      trad_frame_set_reg_addr (this_cache, PPC_TRAP_REGNUM,
			       gpregs + 40 * tdep->wordsize);
    }

  if (ppc_floating_point_unit_p (gdbarch))
    {
      /* Floating point registers.  */
      for (i = 0; i < 32; i++)
	{
	  int regnum = i + gdbarch_fp0_regnum (gdbarch);
	  trad_frame_set_reg_addr (this_cache, regnum,
				   fpregs + i * tdep->wordsize);
	}
      trad_frame_set_reg_addr (this_cache, tdep->ppc_fpscr_regnum,
			 fpregs + 32 * tdep->wordsize);
    }
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

static void
ppc32_linux_sigaction_cache_init (const struct tramp_frame *self,
				  frame_info_ptr this_frame,
				  struct trad_frame_cache *this_cache,
				  CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0xd0 /* Offset to ucontext_t.  */
			    + 0x30 /* Offset to .reg.  */,
			    0);
}

static void
ppc64_linux_sigaction_cache_init (const struct tramp_frame *self,
				  frame_info_ptr this_frame,
				  struct trad_frame_cache *this_cache,
				  CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0x80 /* Offset to ucontext_t.  */
			    + 0xe0 /* Offset to .reg.  */,
			    128);
}

static void
ppc32_linux_sighandler_cache_init (const struct tramp_frame *self,
				   frame_info_ptr this_frame,
				   struct trad_frame_cache *this_cache,
				   CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0x40 /* Offset to ucontext_t.  */
			    + 0x1c /* Offset to .reg.  */,
			    0);
}

static void
ppc64_linux_sighandler_cache_init (const struct tramp_frame *self,
				   frame_info_ptr this_frame,
				   struct trad_frame_cache *this_cache,
				   CORE_ADDR func)
{
  ppc_linux_sigtramp_cache (this_frame, this_cache, func,
			    0x80 /* Offset to struct sigcontext.  */
			    + 0x38 /* Offset to .reg.  */,
			    128);
}

static struct tramp_frame ppc32_linux_sigaction_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  { 
    { 0x380000ac, ULONGEST_MAX }, /* li r0, 172 */
    { 0x44000002, ULONGEST_MAX }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc32_linux_sigaction_cache_init
};
static struct tramp_frame ppc64_linux_sigaction_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { 0x38210080, ULONGEST_MAX }, /* addi r1,r1,128 */
    { 0x380000ac, ULONGEST_MAX }, /* li r0, 172 */
    { 0x44000002, ULONGEST_MAX }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc64_linux_sigaction_cache_init
};
static struct tramp_frame ppc32_linux_sighandler_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  { 
    { 0x38000077, ULONGEST_MAX }, /* li r0,119 */
    { 0x44000002, ULONGEST_MAX }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc32_linux_sighandler_cache_init
};
static struct tramp_frame ppc64_linux_sighandler_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  { 
    { 0x38210080, ULONGEST_MAX }, /* addi r1,r1,128 */
    { 0x38000077, ULONGEST_MAX }, /* li r0,119 */
    { 0x44000002, ULONGEST_MAX }, /* sc */
    { TRAMP_SENTINEL_INSN },
  },
  ppc64_linux_sighandler_cache_init
};

/* Return 1 if PPC_ORIG_R3_REGNUM and PPC_TRAP_REGNUM are usable.  */
int
ppc_linux_trap_reg_p (struct gdbarch *gdbarch)
{
  /* If we do not have a target description with registers, then
     the special registers will not be included in the register set.  */
  if (!tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return 0;

  /* If we do, then it is safe to check the size.  */
  return register_size (gdbarch, PPC_ORIG_R3_REGNUM) > 0
	 && register_size (gdbarch, PPC_TRAP_REGNUM) > 0;
}

/* Return the current system call's number present in the
   r0 register.  When the function fails, it returns -1.  */
static LONGEST
ppc_linux_get_syscall_number (struct gdbarch *gdbarch,
			      thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Make sure we're in a 32- or 64-bit machine */
  gdb_assert (tdep->wordsize == 4 || tdep->wordsize == 8);

  /* The content of a register */
  gdb::byte_vector buf (tdep->wordsize);

  /* Getting the system call number from the register.
     When dealing with PowerPC architecture, this information
     is stored at 0th register.  */
  regcache->cooked_read (tdep->ppc_gp0_regnum, buf.data ());

  return extract_signed_integer (buf.data (), tdep->wordsize, byte_order);
}

/* PPC process record-replay */

static struct linux_record_tdep ppc_linux_record_tdep;
static struct linux_record_tdep ppc64_linux_record_tdep;

/* ppc_canonicalize_syscall maps from the native PowerPC Linux set of
   syscall ids into a canonical set of syscall ids used by process
   record.  (See arch/powerpc/include/uapi/asm/unistd.h in kernel tree.)
   Return -1 if this system call is not supported by process record.
   Otherwise, return the syscall number for process record of given
   SYSCALL.  */

static enum gdb_syscall
ppc_canonicalize_syscall (int syscall, int wordsize)
{
  int result = -1;

  if (syscall <= 165)
    result = syscall;
  else if (syscall >= 167 && syscall <= 190)	/* Skip query_module 166 */
    result = syscall + 1;
  else if (syscall >= 192 && syscall <= 197)	/* mmap2 */
    result = syscall;
  else if (syscall == 208)			/* tkill */
    result = gdb_sys_tkill;
  else if (syscall >= 207 && syscall <= 220)	/* gettid */
    result = syscall + 224 - 207;
  else if (syscall >= 234 && syscall <= 239)	/* exit_group */
    result = syscall + 252 - 234;
  else if (syscall >= 240 && syscall <= 248)	/* timer_create */
    result = syscall += 259 - 240;
  else if (syscall >= 250 && syscall <= 251)	/* tgkill */
    result = syscall + 270 - 250;
  else if (syscall == 286)
    result = gdb_sys_openat;
  else if (syscall == 291)
    {
      if (wordsize == 64)
	result = gdb_sys_newfstatat;
      else
	result = gdb_sys_fstatat64;
    }
  else if (syscall == 317)
    result = gdb_sys_pipe2;
  else if (syscall == 336)
    result = gdb_sys_recv;
  else if (syscall == 337)
    result = gdb_sys_recvfrom;
  else if (syscall == 342)
    result = gdb_sys_recvmsg;
  else if (syscall == 359)
    result = gdb_sys_getrandom;

  return (enum gdb_syscall) result;
}

/* Record registers which might be clobbered during system call.
   Return 0 if successful.  */

static int
ppc_linux_syscall_record (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  ULONGEST scnum;
  enum gdb_syscall syscall_gdb;
  int ret;

  regcache_raw_read_unsigned (regcache, tdep->ppc_gp0_regnum, &scnum);
  syscall_gdb = ppc_canonicalize_syscall (scnum, tdep->wordsize);

  if (syscall_gdb < 0)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %d\n"), (int) scnum);
      return 0;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     int i, j;
     int regsets[] = { tdep->ppc_gp0_regnum,
		       tdep->ppc_fp0_regnum,
		       tdep->ppc_vr0_regnum,
		       tdep->ppc_vsr0_upper_regnum };

     for (j = 0; j < 4; j++)
       {
	 if (regsets[j] == -1)
	   continue;
	 for (i = 0; i < 32; i++)
	   {
	     if (record_full_arch_list_add_reg (regcache, regsets[j] + i))
	       return -1;
	   }
       }

     if (record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum))
       return -1;
     if (record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum))
       return -1;
     if (record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum))
       return -1;
     if (record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum))
       return -1;

     return 0;
   }

  if (tdep->wordsize == 8)
    ret = record_linux_system_call (syscall_gdb, regcache,
				    &ppc64_linux_record_tdep);
  else
    ret = record_linux_system_call (syscall_gdb, regcache,
				    &ppc_linux_record_tdep);

  if (ret != 0)
    return ret;

  /* Record registers clobbered during syscall.  */
  for (int i = 3; i <= 12; i++)
    {
      if (record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + i))
	return -1;
    }
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + 0))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum))
    return -1;

  return 0;
}

/* Record registers which might be clobbered during signal handling.
   Return 0 if successful.  */

static int
ppc_linux_record_signal (struct gdbarch *gdbarch, struct regcache *regcache,
			 enum gdb_signal signal)
{
  /* See handle_rt_signal64 in arch/powerpc/kernel/signal_64.c
	 handle_rt_signal32 in arch/powerpc/kernel/signal_32.c
	 arch/powerpc/include/asm/ptrace.h
     for details.  */
  const int SIGNAL_FRAMESIZE = 128;
  const int sizeof_rt_sigframe = 1440 * 2 + 8 * 2 + 4 * 6 + 8 + 8 + 128 + 512;
  ULONGEST sp;
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int i;

  for (i = 3; i <= 12; i++)
    {
      if (record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + i))
	return -1;
    }

  if (record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum))
    return -1;
  if (record_full_arch_list_add_reg (regcache, gdbarch_pc_regnum (gdbarch)))
    return -1;
  if (record_full_arch_list_add_reg (regcache, gdbarch_sp_regnum (gdbarch)))
    return -1;

  /* Record the change in the stack.
     frame-size = sizeof (struct rt_sigframe) + SIGNAL_FRAMESIZE  */
  regcache_raw_read_unsigned (regcache, gdbarch_sp_regnum (gdbarch), &sp);
  sp -= SIGNAL_FRAMESIZE;
  sp -= sizeof_rt_sigframe;

  if (record_full_arch_list_add_mem (sp, SIGNAL_FRAMESIZE + sizeof_rt_sigframe))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;

  return 0;
}

static void
ppc_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();

  regcache_cooked_write_unsigned (regcache, gdbarch_pc_regnum (gdbarch), pc);

  /* Set special TRAP register to -1 to prevent the kernel from
     messing with the PC we just installed, if we happen to be
     within an interrupted system call that the kernel wants to
     restart.

     Note that after we return from the dummy call, the TRAP and
     ORIG_R3 registers will be automatically restored, and the
     kernel continues to restart the system call at this point.  */
  if (ppc_linux_trap_reg_p (gdbarch))
    regcache_cooked_write_unsigned (regcache, PPC_TRAP_REGNUM, -1);
}

static const struct target_desc *
ppc_linux_core_read_description (struct gdbarch *gdbarch,
				 struct target_ops *target,
				 bfd *abfd)
{
  struct ppc_linux_features features = ppc_linux_no_features;
  asection *altivec = bfd_get_section_by_name (abfd, ".reg-ppc-vmx");
  asection *vsx = bfd_get_section_by_name (abfd, ".reg-ppc-vsx");
  asection *section = bfd_get_section_by_name (abfd, ".reg");
  asection *ppr = bfd_get_section_by_name (abfd, ".reg-ppc-ppr");
  asection *dscr = bfd_get_section_by_name (abfd, ".reg-ppc-dscr");
  asection *tar = bfd_get_section_by_name (abfd, ".reg-ppc-tar");
  asection *pmu = bfd_get_section_by_name (abfd, ".reg-ppc-pmu");
  asection *htmspr = bfd_get_section_by_name (abfd, ".reg-ppc-tm-spr");

  if (! section)
    return NULL;

  switch (bfd_section_size (section))
    {
    case 48 * 4:
      features.wordsize = 4;
      break;
    case 48 * 8:
      features.wordsize = 8;
      break;
    default:
      return NULL;
    }

  if (altivec)
    features.altivec = true;

  if (vsx)
    features.vsx = true;

  std::optional<gdb::byte_vector> auxv = target_read_auxv_raw (target);
  CORE_ADDR hwcap = linux_get_hwcap (auxv, target, gdbarch);

  features.isa205 = ppc_linux_has_isa205 (hwcap);

  if (ppr && dscr)
    {
      features.ppr_dscr = true;

      /* We don't require the EBB note section to be present in the
	 core file to select isa207 because these registers could have
	 been unavailable when the core file was created.  They will
	 be in the tdep but will show as unavailable.  */
      if (tar && pmu)
	{
	  features.isa207 = true;
	  if (htmspr)
	    features.htm = true;
	}
    }

  return ppc_linux_match_description (features);
}


/* Implementation of `gdbarch_elf_make_msymbol_special', as defined in
   gdbarch.h.  This implementation is used for the ELFv2 ABI only.  */

static void
ppc_elfv2_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  if ((sym->flags & BSF_SYNTHETIC) != 0)
    /* ELFv2 synthetic symbols (the PLT stubs and the __glink_PLTresolve
       trampoline) do not have a local entry point.  */
    return;

  elf_symbol_type *elf_sym = (elf_symbol_type *)sym;

  /* If the symbol is marked as having a local entry point, set a target
     flag in the msymbol.  We currently only support local entry point
     offsets of 8 bytes, which is the only entry point offset ever used
     by current compilers.  If/when other offsets are ever used, we will
     have to use additional target flag bits to store them.  */
  switch (PPC64_LOCAL_ENTRY_OFFSET (elf_sym->internal_elf_sym.st_other))
    {
    default:
      break;
    case 8:
      msym->set_target_flag_1 (true);
      break;
    }
}

/* Implementation of `gdbarch_skip_entrypoint', as defined in
   gdbarch.h.  This implementation is used for the ELFv2 ABI only.  */

static CORE_ADDR
ppc_elfv2_skip_entrypoint (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol fun;
  int local_entry_offset = 0;

  fun = lookup_minimal_symbol_by_pc (pc);
  if (fun.minsym == NULL)
    return pc;

  /* See ppc_elfv2_elf_make_msymbol_special for how local entry point
     offset values are encoded.  */
  if (fun.minsym->target_flag_1 ())
    local_entry_offset = 8;

  if (fun.value_address () <= pc
      && pc < fun.value_address () + local_entry_offset)
    return fun.value_address () + local_entry_offset;

  return pc;
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
ppc_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == 'i' /* Literal number.  */
	  || (isdigit (*s) && s[1] == '('
	      && isdigit (s[2])) /* Displacement.  */
	  || (*s == '(' && isdigit (s[1])) /* Register indirection.  */
	  || isdigit (*s)); /* Register value.  */
}

/* Implementation of `gdbarch_stap_parse_special_token', as defined in
   gdbarch.h.  */

static expr::operation_up
ppc_stap_parse_special_token (struct gdbarch *gdbarch,
			      struct stap_parse_info *p)
{
  if (isdigit (*p->arg))
    {
      /* This temporary pointer is needed because we have to do a lookahead.
	  We could be dealing with a register displacement, and in such case
	  we would not need to do anything.  */
      const char *s = p->arg;
      char *regname;
      int len;

      while (isdigit (*s))
	++s;

      if (*s == '(')
	{
	  /* It is a register displacement indeed.  Returning 0 means we are
	     deferring the treatment of this case to the generic parser.  */
	  return {};
	}

      len = s - p->arg;
      regname = (char *) alloca (len + 2);
      regname[0] = 'r';

      strncpy (regname + 1, p->arg, len);
      ++len;
      regname[len] = '\0';

      if (user_reg_map_name_to_regnum (gdbarch, regname, len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname, p->saved_arg);

      p->arg = s;

      return expr::make_operation<expr::register_operation> (regname);
    }

  /* All the other tokens should be handled correctly by the generic
     parser.  */
  return {};
}

/* Initialize linux_record_tdep if not initialized yet.
   WORDSIZE is 4 or 8 for 32- or 64-bit PowerPC Linux respectively.
   Sizes of data structures are initialized accordingly.  */

static void
ppc_init_linux_record_tdep (struct linux_record_tdep *record_tdep,
			    int wordsize)
{
  /* The values for TCGETS, TCSETS, TCSETSW, TCSETSF are based on the
     size of struct termios in the kernel source.
     include/uapi/asm-generic/termbits.h  */
#define SIZE_OF_STRUCT_TERMIOS  0x2c

  /* Simply return if it had been initialized.  */
  if (record_tdep->size_pointer != 0)
    return;

  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */

  if (wordsize == 8)
    {
      record_tdep->size_pointer = 8;
      record_tdep->size__old_kernel_stat = 32;
      record_tdep->size_tms = 32;
      record_tdep->size_loff_t = 8;
      record_tdep->size_flock = 32;
      record_tdep->size_oldold_utsname = 45;
      record_tdep->size_ustat = 32;
      record_tdep->size_old_sigaction = 32;
      record_tdep->size_old_sigset_t = 8;
      record_tdep->size_rlimit = 16;
      record_tdep->size_rusage = 144;
      record_tdep->size_timeval = 16;
      record_tdep->size_timezone = 8;
      record_tdep->size_old_gid_t = 4;
      record_tdep->size_old_uid_t = 4;
      record_tdep->size_fd_set = 128;
      record_tdep->size_old_dirent = 280;
      record_tdep->size_statfs = 120;
      record_tdep->size_statfs64 = 120;
      record_tdep->size_sockaddr = 16;
      record_tdep->size_int = 4;
      record_tdep->size_long = 8;
      record_tdep->size_ulong = 8;
      record_tdep->size_msghdr = 56;
      record_tdep->size_itimerval = 32;
      record_tdep->size_stat = 144;
      record_tdep->size_old_utsname = 325;
      record_tdep->size_sysinfo = 112;
      record_tdep->size_msqid_ds = 120;
      record_tdep->size_shmid_ds = 112;
      record_tdep->size_new_utsname = 390;
      record_tdep->size_timex = 208;
      record_tdep->size_mem_dqinfo = 24;
      record_tdep->size_if_dqblk = 72;
      record_tdep->size_fs_quota_stat = 80;
      record_tdep->size_timespec = 16;
      record_tdep->size_pollfd = 8;
      record_tdep->size_NFS_FHSIZE = 32;
      record_tdep->size_knfsd_fh = 132;
      record_tdep->size_TASK_COMM_LEN = 16;
      record_tdep->size_sigaction = 32;
      record_tdep->size_sigset_t = 8;
      record_tdep->size_siginfo_t = 128;
      record_tdep->size_cap_user_data_t = 8;
      record_tdep->size_stack_t = 24;
      record_tdep->size_off_t = 8;
      record_tdep->size_stat64 = 104;
      record_tdep->size_gid_t = 4;
      record_tdep->size_uid_t = 4;
      record_tdep->size_PAGE_SIZE = 0x10000;	/* 64KB */
      record_tdep->size_flock64 = 32;
      record_tdep->size_io_event = 32;
      record_tdep->size_iocb = 64;
      record_tdep->size_epoll_event = 16;
      record_tdep->size_itimerspec = 32;
      record_tdep->size_mq_attr = 64;
      record_tdep->size_termios = 44;
      record_tdep->size_pid_t = 4;
      record_tdep->size_winsize = 8;
      record_tdep->size_serial_struct = 72;
      record_tdep->size_serial_icounter_struct = 80;
      record_tdep->size_size_t = 8;
      record_tdep->size_iovec = 16;
      record_tdep->size_time_t = 8;
    }
  else if (wordsize == 4)
    {
      record_tdep->size_pointer = 4;
      record_tdep->size__old_kernel_stat = 32;
      record_tdep->size_tms = 16;
      record_tdep->size_loff_t = 8;
      record_tdep->size_flock = 16;
      record_tdep->size_oldold_utsname = 45;
      record_tdep->size_ustat = 20;
      record_tdep->size_old_sigaction = 16;
      record_tdep->size_old_sigset_t = 4;
      record_tdep->size_rlimit = 8;
      record_tdep->size_rusage = 72;
      record_tdep->size_timeval = 8;
      record_tdep->size_timezone = 8;
      record_tdep->size_old_gid_t = 4;
      record_tdep->size_old_uid_t = 4;
      record_tdep->size_fd_set = 128;
      record_tdep->size_old_dirent = 268;
      record_tdep->size_statfs = 64;
      record_tdep->size_statfs64 = 88;
      record_tdep->size_sockaddr = 16;
      record_tdep->size_int = 4;
      record_tdep->size_long = 4;
      record_tdep->size_ulong = 4;
      record_tdep->size_msghdr = 28;
      record_tdep->size_itimerval = 16;
      record_tdep->size_stat = 88;
      record_tdep->size_old_utsname = 325;
      record_tdep->size_sysinfo = 64;
      record_tdep->size_msqid_ds = 68;
      record_tdep->size_shmid_ds = 60;
      record_tdep->size_new_utsname = 390;
      record_tdep->size_timex = 128;
      record_tdep->size_mem_dqinfo = 24;
      record_tdep->size_if_dqblk = 72;
      record_tdep->size_fs_quota_stat = 80;
      record_tdep->size_timespec = 8;
      record_tdep->size_pollfd = 8;
      record_tdep->size_NFS_FHSIZE = 32;
      record_tdep->size_knfsd_fh = 132;
      record_tdep->size_TASK_COMM_LEN = 16;
      record_tdep->size_sigaction = 20;
      record_tdep->size_sigset_t = 8;
      record_tdep->size_siginfo_t = 128;
      record_tdep->size_cap_user_data_t = 4;
      record_tdep->size_stack_t = 12;
      record_tdep->size_off_t = 4;
      record_tdep->size_stat64 = 104;
      record_tdep->size_gid_t = 4;
      record_tdep->size_uid_t = 4;
      record_tdep->size_PAGE_SIZE = 0x10000;	/* 64KB */
      record_tdep->size_flock64 = 32;
      record_tdep->size_io_event = 32;
      record_tdep->size_iocb = 64;
      record_tdep->size_epoll_event = 16;
      record_tdep->size_itimerspec = 16;
      record_tdep->size_mq_attr = 32;
      record_tdep->size_termios = 44;
      record_tdep->size_pid_t = 4;
      record_tdep->size_winsize = 8;
      record_tdep->size_serial_struct = 60;
      record_tdep->size_serial_icounter_struct = 80;
      record_tdep->size_size_t = 4;
      record_tdep->size_iovec = 8;
      record_tdep->size_time_t = 4;
    }
  else
    internal_error (_("unexpected wordsize"));

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  record_tdep->fcntl_F_GETLK = 5;
  record_tdep->fcntl_F_GETLK64 = 12;
  record_tdep->fcntl_F_SETLK64 = 13;
  record_tdep->fcntl_F_SETLKW64 = 14;

  record_tdep->arg1 = PPC_R0_REGNUM + 3;
  record_tdep->arg2 = PPC_R0_REGNUM + 4;
  record_tdep->arg3 = PPC_R0_REGNUM + 5;
  record_tdep->arg4 = PPC_R0_REGNUM + 6;
  record_tdep->arg5 = PPC_R0_REGNUM + 7;
  record_tdep->arg6 = PPC_R0_REGNUM + 8;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.
     See arch/powerpc/include/uapi/asm/ioctls.h.  */
  record_tdep->ioctl_TCGETA = 0x40147417;
  record_tdep->ioctl_TCSETA = 0x80147418;
  record_tdep->ioctl_TCSETAW = 0x80147419;
  record_tdep->ioctl_TCSETAF = 0x8014741c;
  record_tdep->ioctl_TCGETS = 0x40007413 | (SIZE_OF_STRUCT_TERMIOS << 16);
  record_tdep->ioctl_TCSETS = 0x80007414 | (SIZE_OF_STRUCT_TERMIOS << 16);
  record_tdep->ioctl_TCSETSW = 0x80007415 | (SIZE_OF_STRUCT_TERMIOS << 16);
  record_tdep->ioctl_TCSETSF = 0x80007416 | (SIZE_OF_STRUCT_TERMIOS << 16);

  record_tdep->ioctl_TCSBRK = 0x2000741d;
  record_tdep->ioctl_TCXONC = 0x2000741e;
  record_tdep->ioctl_TCFLSH = 0x2000741f;
  record_tdep->ioctl_TIOCEXCL = 0x540c;
  record_tdep->ioctl_TIOCNXCL = 0x540d;
  record_tdep->ioctl_TIOCSCTTY = 0x540e;
  record_tdep->ioctl_TIOCGPGRP = 0x40047477;
  record_tdep->ioctl_TIOCSPGRP = 0x80047476;
  record_tdep->ioctl_TIOCOUTQ = 0x40047473;
  record_tdep->ioctl_TIOCSTI = 0x5412;
  record_tdep->ioctl_TIOCGWINSZ = 0x40087468;
  record_tdep->ioctl_TIOCSWINSZ = 0x80087467;
  record_tdep->ioctl_TIOCMGET = 0x5415;
  record_tdep->ioctl_TIOCMBIS = 0x5416;
  record_tdep->ioctl_TIOCMBIC = 0x5417;
  record_tdep->ioctl_TIOCMSET = 0x5418;
  record_tdep->ioctl_TIOCGSOFTCAR = 0x5419;
  record_tdep->ioctl_TIOCSSOFTCAR = 0x541a;
  record_tdep->ioctl_FIONREAD = 0x4004667f;
  record_tdep->ioctl_TIOCINQ = 0x4004667f;
  record_tdep->ioctl_TIOCLINUX = 0x541c;
  record_tdep->ioctl_TIOCCONS = 0x541d;
  record_tdep->ioctl_TIOCGSERIAL = 0x541e;
  record_tdep->ioctl_TIOCSSERIAL = 0x541f;
  record_tdep->ioctl_TIOCPKT = 0x5420;
  record_tdep->ioctl_FIONBIO = 0x8004667e;
  record_tdep->ioctl_TIOCNOTTY = 0x5422;
  record_tdep->ioctl_TIOCSETD = 0x5423;
  record_tdep->ioctl_TIOCGETD = 0x5424;
  record_tdep->ioctl_TCSBRKP = 0x5425;
  record_tdep->ioctl_TIOCSBRK = 0x5427;
  record_tdep->ioctl_TIOCCBRK = 0x5428;
  record_tdep->ioctl_TIOCGSID = 0x5429;
  record_tdep->ioctl_TIOCGPTN = 0x40045430;
  record_tdep->ioctl_TIOCSPTLCK = 0x80045431;
  record_tdep->ioctl_FIONCLEX = 0x20006602;
  record_tdep->ioctl_FIOCLEX = 0x20006601;
  record_tdep->ioctl_FIOASYNC = 0x8004667d;
  record_tdep->ioctl_TIOCSERCONFIG = 0x5453;
  record_tdep->ioctl_TIOCSERGWILD = 0x5454;
  record_tdep->ioctl_TIOCSERSWILD = 0x5455;
  record_tdep->ioctl_TIOCGLCKTRMIOS = 0x5456;
  record_tdep->ioctl_TIOCSLCKTRMIOS = 0x5457;
  record_tdep->ioctl_TIOCSERGSTRUCT = 0x5458;
  record_tdep->ioctl_TIOCSERGETLSR = 0x5459;
  record_tdep->ioctl_TIOCSERGETMULTI = 0x545a;
  record_tdep->ioctl_TIOCSERSETMULTI = 0x545b;
  record_tdep->ioctl_TIOCMIWAIT = 0x545c;
  record_tdep->ioctl_TIOCGICOUNT = 0x545d;
  record_tdep->ioctl_FIOQSIZE = 0x40086680;
}

/* Return a floating-point format for a floating-point variable of
   length LEN in bits.  If non-NULL, NAME is the name of its type.
   If no suitable type is found, return NULL.  */

static const struct floatformat **
ppc_floatformat_for_type (struct gdbarch *gdbarch,
			  const char *name, int len)
{
  if (len == 128 && name)
    {
      if (strcmp (name, "__float128") == 0
	  || strcmp (name, "_Float128") == 0
	  || strcmp (name, "_Float64x") == 0
	  || strcmp (name, "complex _Float128") == 0
	  || strcmp (name, "complex _Float64x") == 0)
	return floatformats_ieee_quad;

      if (strcmp (name, "__ibm128") == 0)
	return floatformats_ibm_long_double;
    }

  return default_floatformat_for_type (gdbarch, name, len);
}

static bool
linux_dwarf2_omit_typedef_p (struct type *target_type,
			     const char *producer, const char *name)
{
  int gcc_major, gcc_minor;

  if (producer_is_gcc (producer, &gcc_major, &gcc_minor))
    {
      if ((target_type->code () == TYPE_CODE_FLT
	   || target_type->code () == TYPE_CODE_COMPLEX)
	  && (strcmp (name, "long double") == 0
	      || strcmp (name, "complex long double") == 0))
	{
	  /* IEEE 128-bit floating point and IBM long double are two
	     encodings for 128-bit values.  The DWARF debug data can't
	     distinguish between them.  See bugzilla:
	     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104194

	     A GCC hack was introduced to still allow the debugger to identify
	     the case where "long double" uses the IEEE 128-bit floating point
	     format: GCC will emit a bogus DWARF type record pretending that
	     "long double" is a typedef alias for the _Float128 type.

	     This hack should not be visible to the GDB user, so we replace
	     this bogus typedef by a normal floating-point type, copying the
	     format information from the target type of the bogus typedef.  */
	  return true;
	}
    }
  return false;
}

/* Specify the powerpc64le target triplet.
   This can be variations of
	ppc64le-{distro}-linux-gcc
   and
	powerpc64le-{distro}-linux-gcc.  */

static const char *
ppc64le_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  return "p(ower)?pc64le";
}

/* Specify the powerpc64 target triplet.
   This can be variations of
	ppc64-{distro}-linux-gcc
   and
	powerpc64-{distro}-linux-gcc.  */

static const char *
ppc64_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  return "p(ower)?pc64";
}

/* Implement the linux_gcc_target_options method.  */

static std::string
ppc64_linux_gcc_target_options (struct gdbarch *gdbarch)
{
  return "";
}

static displaced_step_prepare_status
ppc_linux_displaced_step_prepare  (gdbarch *arch, thread_info *thread,
				   CORE_ADDR &displaced_pc)
{
  ppc_inferior_data *per_inferior = get_ppc_per_inferior (thread->inf);
  if (!per_inferior->disp_step_buf.has_value ())
    {
      /* Figure out where the displaced step buffer is.  */
      CORE_ADDR disp_step_buf_addr
	= linux_displaced_step_location (thread->inf->arch ());

      per_inferior->disp_step_buf.emplace (disp_step_buf_addr);
    }

  return per_inferior->disp_step_buf->prepare (thread, displaced_pc);
}

/* Convert a Dwarf 2 register number to a GDB register number for Linux.  */

static int
rs6000_linux_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep>(gdbarch);

  if (0 <= num && num <= 31)
    return tdep->ppc_gp0_regnum + num;
  else if (32 <= num && num <= 63)
    /* Map dwarf register numbers for floating point, double, IBM double and
       IEEE 128-bit floating point to the fpr range.  Will have to fix the
       mapping for the IEEE 128-bit register numbers later.  */
    return tdep->ppc_fp0_regnum + (num - 32);
  else if (77 <= num && num < 77 + 32)
    return tdep->ppc_vr0_regnum + (num - 77);
  else
    switch (num)
      {
      case 65:
	return tdep->ppc_lr_regnum;
      case 66:
	return tdep->ppc_ctr_regnum;
      case 76:
	return tdep->ppc_xer_regnum;
      case 109:
	return tdep->ppc_vrsave_regnum;
      case 110:
	return tdep->ppc_vrsave_regnum - 1; /* vscr */
      }

  /* Unknown DWARF register number.  */
  return -1;
}

/* Translate a .eh_frame register to DWARF register, or adjust a
   .debug_frame register.  */

static int
rs6000_linux_adjust_frame_regnum (struct gdbarch *gdbarch, int num,
				  int eh_frame_p)
{
  /* Linux uses the same numbering for .debug_frame numbering as .eh_frame.  */
  return num;
}

static void
ppc_linux_init_abi (struct gdbarch_info info,
		    struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;
  static const char *const stap_integer_prefixes[] = { "i", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "(",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { ")",
								    NULL };

  linux_init_abi (info, gdbarch, 0);

  /* PPC GNU/Linux uses either 64-bit or 128-bit long doubles; where
     128-bit, they can be either IBM long double or IEEE quad long double.
     The 64-bit long double case will be detected automatically using
     the size specified in debug info.  We use a .gnu.attribute flag
     to distinguish between the IBM long double and IEEE quad cases.  */
  set_gdbarch_long_double_bit (gdbarch, 16 * TARGET_CHAR_BIT);
  if (tdep->long_double_abi == POWERPC_LONG_DOUBLE_IEEE128)
    set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);
  else
    set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);

  /* Support for floating-point data type variants.  */
  set_gdbarch_floatformat_for_type (gdbarch, ppc_floatformat_for_type);

  /* Support for replacing typedef record.  */
  set_gdbarch_dwarf2_omit_typedef_p (gdbarch, linux_dwarf2_omit_typedef_p);

  /* Handle inferior calls during interrupted system calls.  */
  set_gdbarch_write_pc (gdbarch, ppc_linux_write_pc);

  /* Get the syscall number from the arch's register.  */
  set_gdbarch_get_syscall_number (gdbarch, ppc_linux_get_syscall_number);

  /* SystemTap functions.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					  stap_register_indirection_suffixes);
  set_gdbarch_stap_gdb_register_prefix (gdbarch, "r");
  set_gdbarch_stap_is_single_operand (gdbarch, ppc_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					ppc_stap_parse_special_token);
  /* Linux DWARF register mapping is different from the other OSes.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch,
				    rs6000_linux_dwarf2_reg_to_regnum);
  /* Note on Linux the mapping for the DWARF registers and the stab registers
     use the same numbers.  Install rs6000_linux_dwarf2_reg_to_regnum for the
     stab register mappings as well.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch,
				    rs6000_linux_dwarf2_reg_to_regnum);
  dwarf2_frame_set_adjust_regnum (gdbarch, rs6000_linux_adjust_frame_regnum);

  if (tdep->wordsize == 4)
    {
      /* Until November 2001, gcc did not comply with the 32 bit SysV
	 R4 ABI requirement that structures less than or equal to 8
	 bytes should be returned in registers.  Instead GCC was using
	 the AIX/PowerOpen ABI - everything returned in memory
	 (well ignoring vectors that is).  When this was corrected, it
	 wasn't fixed for GNU/Linux native platform.  Use the
	 PowerOpen struct convention.  */
      set_gdbarch_return_value_as_value (gdbarch, ppc_linux_return_value);
      set_gdbarch_return_value (gdbarch, nullptr);

      set_gdbarch_memory_remove_breakpoint (gdbarch,
					    ppc_linux_memory_remove_breakpoint);

      /* Shared library handling.  */
      set_gdbarch_skip_trampoline_code (gdbarch, ppc_skip_trampoline_code);
      set_solib_svr4_fetch_link_map_offsets
	(gdbarch, linux_ilp32_fetch_link_map_offsets);

      /* Setting the correct XML syscall filename.  */
      set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_PPC);

      /* Trampolines.  */
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc32_linux_sigaction_tramp_frame);
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc32_linux_sighandler_tramp_frame);

      /* BFD target for core files.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	set_gdbarch_gcore_bfd_target (gdbarch, "elf32-powerpcle");
      else
	set_gdbarch_gcore_bfd_target (gdbarch, "elf32-powerpc");

      if (powerpc_so_ops.in_dynsym_resolve_code == NULL)
	{
	  powerpc_so_ops = svr4_so_ops;
	  /* Override dynamic resolve function.  */
	  powerpc_so_ops.in_dynsym_resolve_code =
	    powerpc_linux_in_dynsym_resolve_code;
	}
      set_gdbarch_so_ops (gdbarch, &powerpc_so_ops);

      set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);
    }
  
  if (tdep->wordsize == 8)
    {
      if (tdep->elf_abi == POWERPC_ELF_V1)
	{
	  /* Handle PPC GNU/Linux 64-bit function pointers (which are really
	     function descriptors).  */
	  set_gdbarch_convert_from_func_ptr_addr
	    (gdbarch, ppc64_convert_from_func_ptr_addr);

	  set_gdbarch_elf_make_msymbol_special
	    (gdbarch, ppc64_elf_make_msymbol_special);
	}
      else
	{
	  set_gdbarch_elf_make_msymbol_special
	    (gdbarch, ppc_elfv2_elf_make_msymbol_special);

	  set_gdbarch_skip_entrypoint (gdbarch, ppc_elfv2_skip_entrypoint);
	}

      /* Shared library handling.  */
      set_gdbarch_skip_trampoline_code (gdbarch, ppc64_skip_trampoline_code);
      set_solib_svr4_fetch_link_map_offsets
	(gdbarch, linux_lp64_fetch_link_map_offsets);

      /* Setting the correct XML syscall filename.  */
      set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_PPC64);

      /* Trampolines.  */
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc64_linux_sigaction_tramp_frame);
      tramp_frame_prepend_unwinder (gdbarch,
				    &ppc64_linux_sighandler_tramp_frame);

      /* BFD target for core files.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	set_gdbarch_gcore_bfd_target (gdbarch, "elf64-powerpcle");
      else
	set_gdbarch_gcore_bfd_target (gdbarch, "elf64-powerpc");
     /* Set compiler triplet.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE)
	set_gdbarch_gnu_triplet_regexp (gdbarch, ppc64le_gnu_triplet_regexp);
      else
	set_gdbarch_gnu_triplet_regexp (gdbarch, ppc64_gnu_triplet_regexp);
      /* Set GCC target options.  */
      set_gdbarch_gcc_target_options (gdbarch, ppc64_linux_gcc_target_options);
    }

  set_gdbarch_core_read_description (gdbarch, ppc_linux_core_read_description);
  set_gdbarch_iterate_over_regset_sections (gdbarch,
					    ppc_linux_iterate_over_regset_sections);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  if (tdesc_data)
    {
      const struct tdesc_feature *feature;

      /* If we have target-described registers, then we can safely
	 reserve a number for PPC_ORIG_R3_REGNUM and PPC_TRAP_REGNUM
	 (whether they are described or not).  */
      gdb_assert (gdbarch_num_regs (gdbarch) <= PPC_ORIG_R3_REGNUM);
      set_gdbarch_num_regs (gdbarch, PPC_TRAP_REGNUM + 1);

      /* If they are present, then assign them to the reserved number.  */
      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.power.linux");
      if (feature != NULL)
	{
	  tdesc_numbered_register (feature, tdesc_data,
				   PPC_ORIG_R3_REGNUM, "orig_r3");
	  tdesc_numbered_register (feature, tdesc_data,
				   PPC_TRAP_REGNUM, "trap");
	}
    }

  /* Support reverse debugging.  */
  set_gdbarch_process_record (gdbarch, ppc_process_record);
  set_gdbarch_process_record_signal (gdbarch, ppc_linux_record_signal);
  tdep->ppc_syscall_record = ppc_linux_syscall_record;

  ppc_init_linux_record_tdep (&ppc_linux_record_tdep, 4);
  ppc_init_linux_record_tdep (&ppc64_linux_record_tdep, 8);

  /* Setup displaced stepping.  */
  set_gdbarch_displaced_step_prepare (gdbarch,
				      ppc_linux_displaced_step_prepare);

}

void _initialize_ppc_linux_tdep ();
void
_initialize_ppc_linux_tdep ()
{
  /* Register for all sub-families of the POWER/PowerPC: 32-bit and
     64-bit PowerPC, and the older rs6k.  */
  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc, GDB_OSABI_LINUX,
			 ppc_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_powerpc, bfd_mach_ppc64, GDB_OSABI_LINUX,
			 ppc_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_rs6000, bfd_mach_rs6k, GDB_OSABI_LINUX,
			 ppc_linux_init_abi);

  /* Initialize the Linux target descriptions.  */
  initialize_tdesc_powerpc_32l ();
  initialize_tdesc_powerpc_altivec32l ();
  initialize_tdesc_powerpc_vsx32l ();
  initialize_tdesc_powerpc_isa205_32l ();
  initialize_tdesc_powerpc_isa205_altivec32l ();
  initialize_tdesc_powerpc_isa205_vsx32l ();
  initialize_tdesc_powerpc_isa205_ppr_dscr_vsx32l ();
  initialize_tdesc_powerpc_isa207_vsx32l ();
  initialize_tdesc_powerpc_isa207_htm_vsx32l ();
  initialize_tdesc_powerpc_64l ();
  initialize_tdesc_powerpc_altivec64l ();
  initialize_tdesc_powerpc_vsx64l ();
  initialize_tdesc_powerpc_isa205_64l ();
  initialize_tdesc_powerpc_isa205_altivec64l ();
  initialize_tdesc_powerpc_isa205_vsx64l ();
  initialize_tdesc_powerpc_isa205_ppr_dscr_vsx64l ();
  initialize_tdesc_powerpc_isa207_vsx64l ();
  initialize_tdesc_powerpc_isa207_htm_vsx64l ();
  initialize_tdesc_powerpc_e500l ();
}

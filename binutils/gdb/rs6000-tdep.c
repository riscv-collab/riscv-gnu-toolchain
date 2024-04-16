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
#include "infrun.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "arch-utils.h"
#include "regcache.h"
#include "regset.h"
#include "target-float.h"
#include "value.h"
#include "parser-defs.h"
#include "osabi.h"
#include "infcall.h"
#include "sim-regno.h"
#include "sim/sim-ppc.h"
#include "reggroups.h"
#include "dwarf2/frame.h"
#include "target-descriptions.h"
#include "user-regs.h"
#include "record-full.h"
#include "auxv.h"

#include "coff/internal.h"
#include "libcoff.h"
#include "coff/xcoff.h"
#include "libxcoff.h"

#include "elf-bfd.h"
#include "elf/ppc.h"
#include "elf/ppc64.h"

#include "solib-svr4.h"
#include "ppc-tdep.h"
#include "ppc-ravenscar-thread.h"

#include "dis-asm.h"

#include "trad-frame.h"
#include "frame-unwind.h"
#include "frame-base.h"

#include "ax.h"
#include "ax-gdb.h"
#include <algorithm>

#include "features/rs6000/powerpc-32.c"
#include "features/rs6000/powerpc-altivec32.c"
#include "features/rs6000/powerpc-vsx32.c"
#include "features/rs6000/powerpc-403.c"
#include "features/rs6000/powerpc-403gc.c"
#include "features/rs6000/powerpc-405.c"
#include "features/rs6000/powerpc-505.c"
#include "features/rs6000/powerpc-601.c"
#include "features/rs6000/powerpc-602.c"
#include "features/rs6000/powerpc-603.c"
#include "features/rs6000/powerpc-604.c"
#include "features/rs6000/powerpc-64.c"
#include "features/rs6000/powerpc-altivec64.c"
#include "features/rs6000/powerpc-vsx64.c"
#include "features/rs6000/powerpc-7400.c"
#include "features/rs6000/powerpc-750.c"
#include "features/rs6000/powerpc-860.c"
#include "features/rs6000/powerpc-e500.c"
#include "features/rs6000/rs6000.c"

/* Determine if regnum is an SPE pseudo-register.  */
#define IS_SPE_PSEUDOREG(tdep, regnum) ((tdep)->ppc_ev0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_ev0_regnum \
    && (regnum) < (tdep)->ppc_ev0_regnum + 32)

/* Determine if regnum is a decimal float pseudo-register.  */
#define IS_DFP_PSEUDOREG(tdep, regnum) ((tdep)->ppc_dl0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_dl0_regnum \
    && (regnum) < (tdep)->ppc_dl0_regnum + 16)

/* Determine if regnum is a "vX" alias for the raw "vrX" vector
   registers.  */
#define IS_V_ALIAS_PSEUDOREG(tdep, regnum) (\
    (tdep)->ppc_v0_alias_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_v0_alias_regnum \
    && (regnum) < (tdep)->ppc_v0_alias_regnum + ppc_num_vrs)

/* Determine if regnum is a POWER7 VSX register.  */
#define IS_VSX_PSEUDOREG(tdep, regnum) ((tdep)->ppc_vsr0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_vsr0_regnum \
    && (regnum) < (tdep)->ppc_vsr0_regnum + ppc_num_vsrs)

/* Determine if regnum is a POWER7 Extended FP register.  */
#define IS_EFP_PSEUDOREG(tdep, regnum) ((tdep)->ppc_efpr0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_efpr0_regnum \
    && (regnum) < (tdep)->ppc_efpr0_regnum + ppc_num_efprs)

/* Determine if regnum is a checkpointed decimal float
   pseudo-register.  */
#define IS_CDFP_PSEUDOREG(tdep, regnum) ((tdep)->ppc_cdl0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_cdl0_regnum \
    && (regnum) < (tdep)->ppc_cdl0_regnum + 16)

/* Determine if regnum is a Checkpointed POWER7 VSX register.  */
#define IS_CVSX_PSEUDOREG(tdep, regnum) ((tdep)->ppc_cvsr0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_cvsr0_regnum \
    && (regnum) < (tdep)->ppc_cvsr0_regnum + ppc_num_vsrs)

/* Determine if regnum is a Checkpointed POWER7 Extended FP register.  */
#define IS_CEFP_PSEUDOREG(tdep, regnum) ((tdep)->ppc_cefpr0_regnum >= 0 \
    && (regnum) >= (tdep)->ppc_cefpr0_regnum \
    && (regnum) < (tdep)->ppc_cefpr0_regnum + ppc_num_efprs)

/* Holds the current set of options to be passed to the disassembler.  */
static char *powerpc_disassembler_options;

/* The list of available "set powerpc ..." and "show powerpc ..."
   commands.  */
static struct cmd_list_element *setpowerpccmdlist = NULL;
static struct cmd_list_element *showpowerpccmdlist = NULL;

static enum auto_boolean powerpc_soft_float_global = AUTO_BOOLEAN_AUTO;

/* The vector ABI to use.  Keep this in sync with powerpc_vector_abi.  */
static const char *const powerpc_vector_strings[] =
{
  "auto",
  "generic",
  "altivec",
  "spe",
  NULL
};

/* A variable that can be configured by the user.  */
static enum powerpc_vector_abi powerpc_vector_abi_global = POWERPC_VEC_AUTO;
static const char *powerpc_vector_abi_string = "auto";

/* PowerPC-related per-inferior data.  */

static const registry<inferior>::key<ppc_inferior_data> ppc_inferior_data_key;

/* Get the per-inferior PowerPC data for INF.  */

ppc_inferior_data *
get_ppc_per_inferior (inferior *inf)
{
  ppc_inferior_data *per_inf = ppc_inferior_data_key.get (inf);

  if (per_inf == nullptr)
    per_inf = ppc_inferior_data_key.emplace (inf);

  return per_inf;
}

/* To be used by skip_prologue.  */

struct rs6000_framedata
  {
    int offset;			/* total size of frame --- the distance
				   by which we decrement sp to allocate
				   the frame */
    int saved_gpr;		/* smallest # of saved gpr */
    unsigned int gpr_mask;	/* Each bit is an individual saved GPR.  */
    int saved_fpr;		/* smallest # of saved fpr */
    int saved_vr;               /* smallest # of saved vr */
    int saved_ev;               /* smallest # of saved ev */
    int alloca_reg;		/* alloca register number (frame ptr) */
    char frameless;		/* true if frameless functions.  */
    char nosavedpc;		/* true if pc not saved.  */
    char used_bl;		/* true if link register clobbered */
    int gpr_offset;		/* offset of saved gprs from prev sp */
    int fpr_offset;		/* offset of saved fprs from prev sp */
    int vr_offset;              /* offset of saved vrs from prev sp */
    int ev_offset;              /* offset of saved evs from prev sp */
    int lr_offset;		/* offset of saved lr */
    int lr_register;		/* register of saved lr, if trustworthy */
    int cr_offset;		/* offset of saved cr */
    int vrsave_offset;          /* offset of saved vrsave register */
  };


/* Is REGNO a VSX register? Return 1 if so, 0 otherwise.  */
int
vsx_register_p (struct gdbarch *gdbarch, int regno)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  if (tdep->ppc_vsr0_regnum < 0)
    return 0;
  else
    return (regno >= tdep->ppc_vsr0_upper_regnum && regno
	    <= tdep->ppc_vsr0_upper_regnum + 31);
}

/* Is REGNO an AltiVec register?  Return 1 if so, 0 otherwise.  */
int
altivec_register_p (struct gdbarch *gdbarch, int regno)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  if (tdep->ppc_vr0_regnum < 0 || tdep->ppc_vrsave_regnum < 0)
    return 0;
  else
    return (regno >= tdep->ppc_vr0_regnum && regno <= tdep->ppc_vrsave_regnum);
}


/* Return true if REGNO is an SPE register, false otherwise.  */
int
spe_register_p (struct gdbarch *gdbarch, int regno)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  
  /* Is it a reference to EV0 -- EV31, and do we have those?  */
  if (IS_SPE_PSEUDOREG (tdep, regno))
    return 1;

  /* Is it a reference to one of the raw upper GPR halves?  */
  if (tdep->ppc_ev0_upper_regnum >= 0
      && tdep->ppc_ev0_upper_regnum <= regno
      && regno < tdep->ppc_ev0_upper_regnum + ppc_num_gprs)
    return 1;

  /* Is it a reference to the 64-bit accumulator, and do we have that?  */
  if (tdep->ppc_acc_regnum >= 0
      && tdep->ppc_acc_regnum == regno)
    return 1;

  /* Is it a reference to the SPE floating-point status and control register,
     and do we have that?  */
  if (tdep->ppc_spefscr_regnum >= 0
      && tdep->ppc_spefscr_regnum == regno)
    return 1;

  return 0;
}


/* Return non-zero if the architecture described by GDBARCH has
   floating-point registers (f0 --- f31 and fpscr).  */
int
ppc_floating_point_unit_p (struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  return (tdep->ppc_fp0_regnum >= 0
	  && tdep->ppc_fpscr_regnum >= 0);
}

/* Return non-zero if the architecture described by GDBARCH has
   Altivec registers (vr0 --- vr31, vrsave and vscr).  */
int
ppc_altivec_support_p (struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  return (tdep->ppc_vr0_regnum >= 0
	  && tdep->ppc_vrsave_regnum >= 0);
}

/* Check that TABLE[GDB_REGNO] is not already initialized, and then
   set it to SIM_REGNO.

   This is a helper function for init_sim_regno_table, constructing
   the table mapping GDB register numbers to sim register numbers; we
   initialize every element in that table to -1 before we start
   filling it in.  */
static void
set_sim_regno (int *table, int gdb_regno, int sim_regno)
{
  /* Make sure we don't try to assign any given GDB register a sim
     register number more than once.  */
  gdb_assert (table[gdb_regno] == -1);
  table[gdb_regno] = sim_regno;
}


/* Initialize ARCH->tdep->sim_regno, the table mapping GDB register
   numbers to simulator register numbers, based on the values placed
   in the ARCH->tdep->ppc_foo_regnum members.  */
static void
init_sim_regno_table (struct gdbarch *arch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (arch);
  int total_regs = gdbarch_num_regs (arch);
  int *sim_regno = GDBARCH_OBSTACK_CALLOC (arch, total_regs, int);
  int i;
  static const char *const segment_regs[] = {
    "sr0", "sr1", "sr2", "sr3", "sr4", "sr5", "sr6", "sr7",
    "sr8", "sr9", "sr10", "sr11", "sr12", "sr13", "sr14", "sr15"
  };

  /* Presume that all registers not explicitly mentioned below are
     unavailable from the sim.  */
  for (i = 0; i < total_regs; i++)
    sim_regno[i] = -1;

  /* General-purpose registers.  */
  for (i = 0; i < ppc_num_gprs; i++)
    set_sim_regno (sim_regno, tdep->ppc_gp0_regnum + i, sim_ppc_r0_regnum + i);
  
  /* Floating-point registers.  */
  if (tdep->ppc_fp0_regnum >= 0)
    for (i = 0; i < ppc_num_fprs; i++)
      set_sim_regno (sim_regno,
		     tdep->ppc_fp0_regnum + i,
		     sim_ppc_f0_regnum + i);
  if (tdep->ppc_fpscr_regnum >= 0)
    set_sim_regno (sim_regno, tdep->ppc_fpscr_regnum, sim_ppc_fpscr_regnum);

  set_sim_regno (sim_regno, gdbarch_pc_regnum (arch), sim_ppc_pc_regnum);
  set_sim_regno (sim_regno, tdep->ppc_ps_regnum, sim_ppc_ps_regnum);
  set_sim_regno (sim_regno, tdep->ppc_cr_regnum, sim_ppc_cr_regnum);

  /* Segment registers.  */
  for (i = 0; i < ppc_num_srs; i++)
    {
      int gdb_regno;

      gdb_regno = user_reg_map_name_to_regnum (arch, segment_regs[i], -1);
      if (gdb_regno >= 0)
	set_sim_regno (sim_regno, gdb_regno, sim_ppc_sr0_regnum + i);
    }

  /* Altivec registers.  */
  if (tdep->ppc_vr0_regnum >= 0)
    {
      for (i = 0; i < ppc_num_vrs; i++)
	set_sim_regno (sim_regno,
		       tdep->ppc_vr0_regnum + i,
		       sim_ppc_vr0_regnum + i);

      /* FIXME: jimb/2004-07-15: when we have tdep->ppc_vscr_regnum,
	 we can treat this more like the other cases.  */
      set_sim_regno (sim_regno,
		     tdep->ppc_vr0_regnum + ppc_num_vrs,
		     sim_ppc_vscr_regnum);
    }
  /* vsave is a special-purpose register, so the code below handles it.  */

  /* SPE APU (E500) registers.  */
  if (tdep->ppc_ev0_upper_regnum >= 0)
    for (i = 0; i < ppc_num_gprs; i++)
      set_sim_regno (sim_regno,
		     tdep->ppc_ev0_upper_regnum + i,
		     sim_ppc_rh0_regnum + i);
  if (tdep->ppc_acc_regnum >= 0)
    set_sim_regno (sim_regno, tdep->ppc_acc_regnum, sim_ppc_acc_regnum);
  /* spefscr is a special-purpose register, so the code below handles it.  */

#ifdef WITH_PPC_SIM
  /* Now handle all special-purpose registers.  Verify that they
     haven't mistakenly been assigned numbers by any of the above
     code.  */
  for (i = 0; i < sim_ppc_num_sprs; i++)
    {
      const char *spr_name = sim_spr_register_name (i);
      int gdb_regno = -1;

      if (spr_name != NULL)
	gdb_regno = user_reg_map_name_to_regnum (arch, spr_name, -1);

      if (gdb_regno != -1)
	set_sim_regno (sim_regno, gdb_regno, sim_ppc_spr0_regnum + i);
    }
#endif

  /* Drop the initialized array into place.  */
  tdep->sim_regno = sim_regno;
}


/* Given a GDB register number REG, return the corresponding SIM
   register number.  */
static int
rs6000_register_sim_regno (struct gdbarch *gdbarch, int reg)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int sim_regno;

  if (tdep->sim_regno == NULL)
    init_sim_regno_table (gdbarch);

  gdb_assert (0 <= reg && reg <= gdbarch_num_cooked_regs (gdbarch));
  sim_regno = tdep->sim_regno[reg];

  if (sim_regno >= 0)
    return sim_regno;
  else
    return LEGACY_SIM_REGNO_IGNORE;
}



/* Register set support functions.  */

/* REGS + OFFSET contains register REGNUM in a field REGSIZE wide.
   Write the register to REGCACHE.  */

void
ppc_supply_reg (struct regcache *regcache, int regnum, 
		const gdb_byte *regs, size_t offset, int regsize)
{
  if (regnum != -1 && offset != -1)
    {
      if (regsize > 4)
	{
	  struct gdbarch *gdbarch = regcache->arch ();
	  int gdb_regsize = register_size (gdbarch, regnum);
	  if (gdb_regsize < regsize
	      && gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	    offset += regsize - gdb_regsize;
	}
      regcache->raw_supply (regnum, regs + offset);
    }
}

/* Read register REGNUM from REGCACHE and store to REGS + OFFSET
   in a field REGSIZE wide.  Zero pad as necessary.  */

void
ppc_collect_reg (const struct regcache *regcache, int regnum,
		 gdb_byte *regs, size_t offset, int regsize)
{
  if (regnum != -1 && offset != -1)
    {
      if (regsize > 4)
	{
	  struct gdbarch *gdbarch = regcache->arch ();
	  int gdb_regsize = register_size (gdbarch, regnum);
	  if (gdb_regsize < regsize)
	    {
	      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		{
		  memset (regs + offset, 0, regsize - gdb_regsize);
		  offset += regsize - gdb_regsize;
		}
	      else
		memset (regs + offset + regsize - gdb_regsize, 0,
			regsize - gdb_regsize);
	    }
	}
      regcache->raw_collect (regnum, regs + offset);
    }
}
    
static int
ppc_greg_offset (struct gdbarch *gdbarch,
		 ppc_gdbarch_tdep *tdep,
		 const struct ppc_reg_offsets *offsets,
		 int regnum,
		 int *regsize)
{
  *regsize = offsets->gpr_size;
  if (regnum >= tdep->ppc_gp0_regnum
      && regnum < tdep->ppc_gp0_regnum + ppc_num_gprs)
    return (offsets->r0_offset
	    + (regnum - tdep->ppc_gp0_regnum) * offsets->gpr_size);

  if (regnum == gdbarch_pc_regnum (gdbarch))
    return offsets->pc_offset;

  if (regnum == tdep->ppc_ps_regnum)
    return offsets->ps_offset;

  if (regnum == tdep->ppc_lr_regnum)
    return offsets->lr_offset;

  if (regnum == tdep->ppc_ctr_regnum)
    return offsets->ctr_offset;

  *regsize = offsets->xr_size;
  if (regnum == tdep->ppc_cr_regnum)
    return offsets->cr_offset;

  if (regnum == tdep->ppc_xer_regnum)
    return offsets->xer_offset;

  if (regnum == tdep->ppc_mq_regnum)
    return offsets->mq_offset;

  return -1;
}

static int
ppc_fpreg_offset (ppc_gdbarch_tdep *tdep,
		  const struct ppc_reg_offsets *offsets,
		  int regnum)
{
  if (regnum >= tdep->ppc_fp0_regnum
      && regnum < tdep->ppc_fp0_regnum + ppc_num_fprs)
    return offsets->f0_offset + (regnum - tdep->ppc_fp0_regnum) * 8;

  if (regnum == tdep->ppc_fpscr_regnum)
    return offsets->fpscr_offset;

  return -1;
}

/* Supply register REGNUM in the general-purpose register set REGSET
   from the buffer specified by GREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

void
ppc_supply_gregset (const struct regset *regset, struct regcache *regcache,
		    int regnum, const void *gregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  const struct ppc_reg_offsets *offsets
    = (const struct ppc_reg_offsets *) regset->regmap;
  size_t offset;
  int regsize;

  if (regnum == -1)
    {
      int i;
      int gpr_size = offsets->gpr_size;

      for (i = tdep->ppc_gp0_regnum, offset = offsets->r0_offset;
	   i < tdep->ppc_gp0_regnum + ppc_num_gprs;
	   i++, offset += gpr_size)
	ppc_supply_reg (regcache, i, (const gdb_byte *) gregs, offset,
			gpr_size);

      ppc_supply_reg (regcache, gdbarch_pc_regnum (gdbarch),
		      (const gdb_byte *) gregs, offsets->pc_offset, gpr_size);
      ppc_supply_reg (regcache, tdep->ppc_ps_regnum,
		      (const gdb_byte *) gregs, offsets->ps_offset, gpr_size);
      ppc_supply_reg (regcache, tdep->ppc_lr_regnum,
		      (const gdb_byte *) gregs, offsets->lr_offset, gpr_size);
      ppc_supply_reg (regcache, tdep->ppc_ctr_regnum,
		      (const gdb_byte *) gregs, offsets->ctr_offset, gpr_size);
      ppc_supply_reg (regcache, tdep->ppc_cr_regnum,
		      (const gdb_byte *) gregs, offsets->cr_offset,
		      offsets->xr_size);
      ppc_supply_reg (regcache, tdep->ppc_xer_regnum,
		      (const gdb_byte *) gregs, offsets->xer_offset,
		      offsets->xr_size);
      ppc_supply_reg (regcache, tdep->ppc_mq_regnum,
		      (const gdb_byte *) gregs, offsets->mq_offset,
		      offsets->xr_size);
      return;
    }

  offset = ppc_greg_offset (gdbarch, tdep, offsets, regnum, &regsize);
  ppc_supply_reg (regcache, regnum, (const gdb_byte *) gregs, offset, regsize);
}

/* Supply register REGNUM in the floating-point register set REGSET
   from the buffer specified by FPREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

void
ppc_supply_fpregset (const struct regset *regset, struct regcache *regcache,
		     int regnum, const void *fpregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const struct ppc_reg_offsets *offsets;
  size_t offset;

  if (!ppc_floating_point_unit_p (gdbarch))
    return;

  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  offsets = (const struct ppc_reg_offsets *) regset->regmap;
  if (regnum == -1)
    {
      int i;

      for (i = tdep->ppc_fp0_regnum, offset = offsets->f0_offset;
	   i < tdep->ppc_fp0_regnum + ppc_num_fprs;
	   i++, offset += 8)
	ppc_supply_reg (regcache, i, (const gdb_byte *) fpregs, offset, 8);

      ppc_supply_reg (regcache, tdep->ppc_fpscr_regnum,
		      (const gdb_byte *) fpregs, offsets->fpscr_offset,
		      offsets->fpscr_size);
      return;
    }

  offset = ppc_fpreg_offset (tdep, offsets, regnum);
  ppc_supply_reg (regcache, regnum, (const gdb_byte *) fpregs, offset,
		  regnum == tdep->ppc_fpscr_regnum ? offsets->fpscr_size : 8);
}

/* Collect register REGNUM in the general-purpose register set
   REGSET from register cache REGCACHE into the buffer specified by
   GREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

void
ppc_collect_gregset (const struct regset *regset,
		     const struct regcache *regcache,
		     int regnum, void *gregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  const struct ppc_reg_offsets *offsets
    = (const struct ppc_reg_offsets *) regset->regmap;
  size_t offset;
  int regsize;

  if (regnum == -1)
    {
      int i;
      int gpr_size = offsets->gpr_size;

      for (i = tdep->ppc_gp0_regnum, offset = offsets->r0_offset;
	   i < tdep->ppc_gp0_regnum + ppc_num_gprs;
	   i++, offset += gpr_size)
	ppc_collect_reg (regcache, i, (gdb_byte *) gregs, offset, gpr_size);

      ppc_collect_reg (regcache, gdbarch_pc_regnum (gdbarch),
		       (gdb_byte *) gregs, offsets->pc_offset, gpr_size);
      ppc_collect_reg (regcache, tdep->ppc_ps_regnum,
		       (gdb_byte *) gregs, offsets->ps_offset, gpr_size);
      ppc_collect_reg (regcache, tdep->ppc_lr_regnum,
		       (gdb_byte *) gregs, offsets->lr_offset, gpr_size);
      ppc_collect_reg (regcache, tdep->ppc_ctr_regnum,
		       (gdb_byte *) gregs, offsets->ctr_offset, gpr_size);
      ppc_collect_reg (regcache, tdep->ppc_cr_regnum,
		       (gdb_byte *) gregs, offsets->cr_offset,
		       offsets->xr_size);
      ppc_collect_reg (regcache, tdep->ppc_xer_regnum,
		       (gdb_byte *) gregs, offsets->xer_offset,
		       offsets->xr_size);
      ppc_collect_reg (regcache, tdep->ppc_mq_regnum,
		       (gdb_byte *) gregs, offsets->mq_offset,
		       offsets->xr_size);
      return;
    }

  offset = ppc_greg_offset (gdbarch, tdep, offsets, regnum, &regsize);
  ppc_collect_reg (regcache, regnum, (gdb_byte *) gregs, offset, regsize);
}

/* Collect register REGNUM in the floating-point register set
   REGSET from register cache REGCACHE into the buffer specified by
   FPREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

void
ppc_collect_fpregset (const struct regset *regset,
		      const struct regcache *regcache,
		      int regnum, void *fpregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const struct ppc_reg_offsets *offsets;
  size_t offset;

  if (!ppc_floating_point_unit_p (gdbarch))
    return;

  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  offsets = (const struct ppc_reg_offsets *) regset->regmap;
  if (regnum == -1)
    {
      int i;

      for (i = tdep->ppc_fp0_regnum, offset = offsets->f0_offset;
	   i < tdep->ppc_fp0_regnum + ppc_num_fprs;
	   i++, offset += 8)
	ppc_collect_reg (regcache, i, (gdb_byte *) fpregs, offset, 8);

      ppc_collect_reg (regcache, tdep->ppc_fpscr_regnum,
		       (gdb_byte *) fpregs, offsets->fpscr_offset,
		       offsets->fpscr_size);
      return;
    }

  offset = ppc_fpreg_offset (tdep, offsets, regnum);
  ppc_collect_reg (regcache, regnum, (gdb_byte *) fpregs, offset,
		   regnum == tdep->ppc_fpscr_regnum ? offsets->fpscr_size : 8);
}

static int
insn_changes_sp_or_jumps (unsigned long insn)
{
  int opcode = (insn >> 26) & 0x03f;
  int sd = (insn >> 21) & 0x01f;
  int a = (insn >> 16) & 0x01f;
  int subcode = (insn >> 1) & 0x3ff;

  /* Changes the stack pointer.  */

  /* NOTE: There are many ways to change the value of a given register.
	   The ways below are those used when the register is R1, the SP,
	   in a funtion's epilogue.  */

  if (opcode == 31 && subcode == 444 && a == 1)
    return 1;  /* mr R1,Rn */
  if (opcode == 14 && sd == 1)
    return 1;  /* addi R1,Rn,simm */
  if (opcode == 58 && sd == 1)
    return 1;  /* ld R1,ds(Rn) */

  /* Transfers control.  */

  if (opcode == 18)
    return 1;  /* b */
  if (opcode == 16)
    return 1;  /* bc */
  if (opcode == 19 && subcode == 16)
    return 1;  /* bclr */
  if (opcode == 19 && subcode == 528)
    return 1;  /* bcctr */

  return 0;
}

/* Return true if we are in the function's epilogue, i.e. after the
   instruction that destroyed the function's stack frame.

   1) scan forward from the point of execution:
       a) If you find an instruction that modifies the stack pointer
	  or transfers control (except a return), execution is not in
	  an epilogue, return.
       b) Stop scanning if you find a return instruction or reach the
	  end of the function or reach the hard limit for the size of
	  an epilogue.
   2) scan backward from the point of execution:
	a) If you find an instruction that modifies the stack pointer,
	    execution *is* in an epilogue, return.
	b) Stop scanning if you reach an instruction that transfers
	   control or the beginning of the function or reach the hard
	   limit for the size of an epilogue.  */

static int
rs6000_in_function_epilogue_frame_p (frame_info_ptr curfrm,
				     struct gdbarch *gdbarch, CORE_ADDR pc)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  bfd_byte insn_buf[PPC_INSN_SIZE];
  CORE_ADDR scan_pc, func_start, func_end, epilogue_start, epilogue_end;
  unsigned long insn;

  /* Find the search limits based on function boundaries and hard limit.  */

  if (!find_pc_partial_function (pc, NULL, &func_start, &func_end))
    return 0;

  epilogue_start = pc - PPC_MAX_EPILOGUE_INSTRUCTIONS * PPC_INSN_SIZE;
  if (epilogue_start < func_start) epilogue_start = func_start;

  epilogue_end = pc + PPC_MAX_EPILOGUE_INSTRUCTIONS * PPC_INSN_SIZE;
  if (epilogue_end > func_end) epilogue_end = func_end;

  /* Scan forward until next 'blr'.  */

  for (scan_pc = pc; scan_pc < epilogue_end; scan_pc += PPC_INSN_SIZE)
    {
      if (!safe_frame_unwind_memory (curfrm, scan_pc,
				     {insn_buf, PPC_INSN_SIZE}))
	return 0;
      insn = extract_unsigned_integer (insn_buf, PPC_INSN_SIZE, byte_order);
      if (insn == 0x4e800020)
	break;
      /* Assume a bctr is a tail call unless it points strictly within
	 this function.  */
      if (insn == 0x4e800420)
	{
	  CORE_ADDR ctr = get_frame_register_unsigned (curfrm,
						       tdep->ppc_ctr_regnum);
	  if (ctr > func_start && ctr < func_end)
	    return 0;
	  else
	    break;
	}
      if (insn_changes_sp_or_jumps (insn))
	return 0;
    }

  /* Scan backward until adjustment to stack pointer (R1).  */

  for (scan_pc = pc - PPC_INSN_SIZE;
       scan_pc >= epilogue_start;
       scan_pc -= PPC_INSN_SIZE)
    {
      if (!safe_frame_unwind_memory (curfrm, scan_pc,
				     {insn_buf, PPC_INSN_SIZE}))
	return 0;
      insn = extract_unsigned_integer (insn_buf, PPC_INSN_SIZE, byte_order);
      if (insn_changes_sp_or_jumps (insn))
	return 1;
    }

  return 0;
}

/* Implement the stack_frame_destroyed_p gdbarch method.  */

static int
rs6000_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  return rs6000_in_function_epilogue_frame_p (get_current_frame (),
					      gdbarch, pc);
}

/* Get the ith function argument for the current function.  */
static CORE_ADDR
rs6000_fetch_pointer_argument (frame_info_ptr frame, int argi, 
			       struct type *type)
{
  return get_frame_register_unsigned (frame, 3 + argi);
}

/* Sequence of bytes for breakpoint instruction.  */

constexpr gdb_byte big_breakpoint[] = { 0x7f, 0xe0, 0x00, 0x08 };
constexpr gdb_byte little_breakpoint[] = { 0x08, 0x00, 0xe0, 0x7f };

typedef BP_MANIPULATION_ENDIAN (little_breakpoint, big_breakpoint)
  rs6000_breakpoint;

/* Instruction masks for displaced stepping.  */
#define OP_MASK 0xfc000000
#define BP_MASK 0xFC0007FE
#define B_INSN 0x48000000
#define BC_INSN 0x40000000
#define BXL_INSN 0x4c000000
#define BP_INSN 0x7C000008

/* Instruction masks used during single-stepping of atomic
   sequences.  */
#define LOAD_AND_RESERVE_MASK 0xfc0007fe
#define LWARX_INSTRUCTION 0x7c000028
#define LDARX_INSTRUCTION 0x7c0000A8
#define LBARX_INSTRUCTION 0x7c000068
#define LHARX_INSTRUCTION 0x7c0000e8
#define LQARX_INSTRUCTION 0x7c000228
#define STORE_CONDITIONAL_MASK 0xfc0007ff
#define STWCX_INSTRUCTION 0x7c00012d
#define STDCX_INSTRUCTION 0x7c0001ad
#define STBCX_INSTRUCTION 0x7c00056d
#define STHCX_INSTRUCTION 0x7c0005ad
#define STQCX_INSTRUCTION 0x7c00016d

/* Instruction masks for single-stepping of addpcis/lnia.  */
#define ADDPCIS_INSN            0x4c000004
#define ADDPCIS_INSN_MASK       0xfc00003e
#define ADDPCIS_TARGET_REGISTER 0x03F00000
#define ADDPCIS_INSN_REGSHIFT   21

#define PNOP_MASK 0xfff3ffff
#define PNOP_INSN 0x07000000
#define R_MASK 0x00100000
#define R_ZERO 0x00000000

/* Check if insn is one of the Load And Reserve instructions used for atomic
   sequences.  */
#define IS_LOAD_AND_RESERVE_INSN(insn)	((insn & LOAD_AND_RESERVE_MASK) == LWARX_INSTRUCTION \
					 || (insn & LOAD_AND_RESERVE_MASK) == LDARX_INSTRUCTION \
					 || (insn & LOAD_AND_RESERVE_MASK) == LBARX_INSTRUCTION \
					 || (insn & LOAD_AND_RESERVE_MASK) == LHARX_INSTRUCTION \
					 || (insn & LOAD_AND_RESERVE_MASK) == LQARX_INSTRUCTION)
/* Check if insn is one of the Store Conditional instructions used for atomic
   sequences.  */
#define IS_STORE_CONDITIONAL_INSN(insn)	((insn & STORE_CONDITIONAL_MASK) == STWCX_INSTRUCTION \
					 || (insn & STORE_CONDITIONAL_MASK) == STDCX_INSTRUCTION \
					 || (insn & STORE_CONDITIONAL_MASK) == STBCX_INSTRUCTION \
					 || (insn & STORE_CONDITIONAL_MASK) == STHCX_INSTRUCTION \
					 || (insn & STORE_CONDITIONAL_MASK) == STQCX_INSTRUCTION)

typedef buf_displaced_step_copy_insn_closure
  ppc_displaced_step_copy_insn_closure;

/* We can't displaced step atomic sequences.  */

static displaced_step_copy_insn_closure_up
ppc_displaced_step_copy_insn (struct gdbarch *gdbarch,
			      CORE_ADDR from, CORE_ADDR to,
			      struct regcache *regs)
{
  size_t len = gdbarch_displaced_step_buffer_length (gdbarch);
  gdb_assert (len > PPC_INSN_SIZE);
  std::unique_ptr<ppc_displaced_step_copy_insn_closure> closure
    (new ppc_displaced_step_copy_insn_closure (len));
  gdb_byte *buf = closure->buf.data ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int insn;

  len = target_read (current_inferior()->top_target(), TARGET_OBJECT_MEMORY, NULL,
		     buf, from, len);
  if ((ssize_t) len < PPC_INSN_SIZE)
    memory_error (TARGET_XFER_E_IO, from);

  insn = extract_signed_integer (buf, PPC_INSN_SIZE, byte_order);

  /* Check for PNOP and for prefixed instructions with R=0.  Those
     instructions are safe to displace.  Prefixed instructions with R=1
     will read/write data to/from locations relative to the current PC.
     We would not be able to fixup after an instruction has written data
    into a displaced location, so decline to displace those instructions.  */
  if ((insn & OP_MASK) == 1 << 26)
    {
      if (((insn & PNOP_MASK) != PNOP_INSN)
	  && ((insn & R_MASK) != R_ZERO))
	{
	  displaced_debug_printf ("Not displacing prefixed instruction %08x at %s",
				  insn, paddress (gdbarch, from));
	  return NULL;
	}
    }
  else
    /* Non-prefixed instructions..  */
    {
      /* Set the instruction length to 4 to match the actual instruction
	 length.  */
      len = 4;
    }

  /* Assume all atomic sequences start with a Load and Reserve instruction.  */
  if (IS_LOAD_AND_RESERVE_INSN (insn))
    {
      displaced_debug_printf ("can't displaced step atomic sequence at %s",
			      paddress (gdbarch, from));

      return NULL;
    }

  write_memory (to, buf, len);

  displaced_debug_printf ("copy %s->%s: %s",
			  paddress (gdbarch, from), paddress (gdbarch, to),
			  bytes_to_string (buf, len).c_str ());

  /* This is a work around for a problem with g++ 4.8.  */
  return displaced_step_copy_insn_closure_up (closure.release ());
}

/* Fix up the state of registers and memory after having single-stepped
   a displaced instruction.  */
static void
ppc_displaced_step_fixup (struct gdbarch *gdbarch,
			  struct displaced_step_copy_insn_closure *closure_,
			  CORE_ADDR from, CORE_ADDR to,
			  struct regcache *regs, bool completed_p)
{
  /* If the displaced instruction didn't complete successfully then all we
     need to do is restore the program counter.  */
  if (!completed_p)
    {
      CORE_ADDR pc = regcache_read_pc (regs);
      pc = from + (pc - to);
      regcache_write_pc (regs, pc);
      return;
    }

  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* Our closure is a copy of the instruction.  */
  ppc_displaced_step_copy_insn_closure *closure
    = (ppc_displaced_step_copy_insn_closure *) closure_;
  ULONGEST insn  = extract_unsigned_integer (closure->buf.data (),
					     PPC_INSN_SIZE, byte_order);
  ULONGEST opcode;
  /* Offset for non PC-relative instructions.  */
  LONGEST offset;

  opcode = insn & OP_MASK;

  /* Set offset to 8 if this is an 8-byte (prefixed) instruction.  */
  if ((opcode) == 1 << 26)
    offset = 2 * PPC_INSN_SIZE;
  else
    offset = PPC_INSN_SIZE;

  displaced_debug_printf ("(ppc) fixup (%s, %s)",
			  paddress (gdbarch, from), paddress (gdbarch, to));

  /* Handle the addpcis/lnia instruction.  */
  if ((insn & ADDPCIS_INSN_MASK) == ADDPCIS_INSN)
    {
      LONGEST displaced_offset;
      ULONGEST current_val;
      /* Measure the displacement.  */
      displaced_offset = from - to;
      /* Identify the target register that was updated by the instruction.  */
      int regnum = (insn & ADDPCIS_TARGET_REGISTER) >> ADDPCIS_INSN_REGSHIFT;
      /* Read and update the target value.  */
      regcache_cooked_read_unsigned (regs, regnum , &current_val);
      displaced_debug_printf ("addpcis target regnum %d was %s now %s",
			      regnum, paddress (gdbarch, current_val),
			      paddress (gdbarch, current_val
					+ displaced_offset));
      regcache_cooked_write_unsigned (regs, regnum,
					current_val + displaced_offset);
      /* point the PC back at the non-displaced instruction.  */
      regcache_cooked_write_unsigned (regs, gdbarch_pc_regnum (gdbarch),
				    from + offset);
    }
  /* Handle PC-relative branch instructions.  */
  else if (opcode == B_INSN || opcode == BC_INSN || opcode == BXL_INSN)
    {
      ULONGEST current_pc;

      /* Read the current PC value after the instruction has been executed
	 in a displaced location.  Calculate the offset to be applied to the
	 original PC value before the displaced stepping.  */
      regcache_cooked_read_unsigned (regs, gdbarch_pc_regnum (gdbarch),
				      &current_pc);
      offset = current_pc - to;

      if (opcode != BXL_INSN)
	{
	  /* Check for AA bit indicating whether this is an absolute
	     addressing or PC-relative (1: absolute, 0: relative).  */
	  if (!(insn & 0x2))
	    {
	      /* PC-relative addressing is being used in the branch.  */
	      displaced_debug_printf ("(ppc) branch instruction: %s",
				      paddress (gdbarch, insn));
	      displaced_debug_printf ("(ppc) adjusted PC from %s to %s",
				      paddress (gdbarch, current_pc),
				      paddress (gdbarch, from + offset));

	      regcache_cooked_write_unsigned (regs,
					      gdbarch_pc_regnum (gdbarch),
					      from + offset);
	    }
	}
      else
	{
	  /* If we're here, it means we have a branch to LR or CTR.  If the
	     branch was taken, the offset is probably greater than 4 (the next
	     instruction), so it's safe to assume that an offset of 4 means we
	     did not take the branch.  */
	  if (offset == PPC_INSN_SIZE)
	    regcache_cooked_write_unsigned (regs, gdbarch_pc_regnum (gdbarch),
					    from + PPC_INSN_SIZE);
	}

      /* Check for LK bit indicating whether we should set the link
	 register to point to the next instruction
	 (1: Set, 0: Don't set).  */
      if (insn & 0x1)
	{
	  /* Link register needs to be set to the next instruction's PC.  */
	  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
	  regcache_cooked_write_unsigned (regs,
					  tdep->ppc_lr_regnum,
					  from + PPC_INSN_SIZE);
	  displaced_debug_printf ("(ppc) adjusted LR to %s",
				  paddress (gdbarch, from + PPC_INSN_SIZE));

	}
    }
  /* Check for breakpoints in the inferior.  If we've found one, place the PC
     right at the breakpoint instruction.  */
  else if ((insn & BP_MASK) == BP_INSN)
    regcache_cooked_write_unsigned (regs, gdbarch_pc_regnum (gdbarch), from);
  else
    {
      /* Handle any other instructions that do not fit in the categories
	 above.  */
      regcache_cooked_write_unsigned (regs, gdbarch_pc_regnum (gdbarch),
				      from + offset);
    }
}

/* Implementation of gdbarch_displaced_step_prepare.  */

static displaced_step_prepare_status
ppc_displaced_step_prepare  (gdbarch *arch, thread_info *thread,
			     CORE_ADDR &displaced_pc)
{
  ppc_inferior_data *per_inferior = get_ppc_per_inferior (thread->inf);

  if (!per_inferior->disp_step_buf.has_value ())
    {
      /* Figure out where the displaced step buffer is.  */
      CORE_ADDR disp_step_buf_addr
	= displaced_step_at_entry_point (thread->inf->arch ());

      per_inferior->disp_step_buf.emplace (disp_step_buf_addr);
    }

  return per_inferior->disp_step_buf->prepare (thread, displaced_pc);
}

/* Implementation of gdbarch_displaced_step_finish.  */

static displaced_step_finish_status
ppc_displaced_step_finish (gdbarch *arch, thread_info *thread,
			   const target_waitstatus &status)
{
  ppc_inferior_data *per_inferior = get_ppc_per_inferior (thread->inf);

  gdb_assert (per_inferior->disp_step_buf.has_value ());

  return per_inferior->disp_step_buf->finish (arch, thread, status);
}

/* Implementation of gdbarch_displaced_step_restore_all_in_ptid.  */

static void
ppc_displaced_step_restore_all_in_ptid (inferior *parent_inf, ptid_t ptid)
{
  ppc_inferior_data *per_inferior = ppc_inferior_data_key.get (parent_inf);

  if (per_inferior == nullptr
      || !per_inferior->disp_step_buf.has_value ())
    return;

  per_inferior->disp_step_buf->restore_in_ptid (ptid);
}

/* Always use hardware single-stepping to execute the
   displaced instruction.  */
static bool
ppc_displaced_step_hw_singlestep (struct gdbarch *gdbarch)
{
  return true;
}

/* Checks for an atomic sequence of instructions beginning with a
   Load And Reserve instruction and ending with a Store Conditional
   instruction.  If such a sequence is found, attempt to step through it.
   A breakpoint is placed at the end of the sequence.  */
std::vector<CORE_ADDR>
ppc_deal_with_atomic_sequence (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc = regcache_read_pc (regcache);
  CORE_ADDR breaks[2] = {CORE_ADDR_MAX, CORE_ADDR_MAX};
  CORE_ADDR loc = pc;
  CORE_ADDR closing_insn; /* Instruction that closes the atomic sequence.  */
  int insn = read_memory_integer (loc, PPC_INSN_SIZE, byte_order);
  int insn_count;
  int index;
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */  
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */
  int bc_insn_count = 0; /* Conditional branch instruction count.  */

  /* Assume all atomic sequences start with a Load And Reserve instruction.  */
  if (!IS_LOAD_AND_RESERVE_INSN (insn))
    return {};

  /* Assume that no atomic sequence is longer than "atomic_sequence_length" 
     instructions.  */
  for (insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      if ((insn & OP_MASK) == 1 << 26)
	loc += 2 * PPC_INSN_SIZE;
      else
	loc += PPC_INSN_SIZE;
      insn = read_memory_integer (loc, PPC_INSN_SIZE, byte_order);

      /* Assume that there is at most one conditional branch in the atomic
	 sequence.  If a conditional branch is found, put a breakpoint in 
	 its destination address.  */
      if ((insn & OP_MASK) == BC_INSN)
	{
	  int immediate = ((insn & 0xfffc) ^ 0x8000) - 0x8000;
	  int absolute = insn & 2;

	  if (bc_insn_count >= 1)
	    return {}; /* More than one conditional branch found, fallback
			  to the standard single-step code.  */
 
	  if (absolute)
	    breaks[1] = immediate;
	  else
	    breaks[1] = loc + immediate;

	  bc_insn_count++;
	  last_breakpoint++;
	}

      if (IS_STORE_CONDITIONAL_INSN (insn))
	break;
    }

  /* Assume that the atomic sequence ends with a Store Conditional
     instruction.  */
  if (!IS_STORE_CONDITIONAL_INSN (insn))
    return {};

  closing_insn = loc;
  loc += PPC_INSN_SIZE;

  /* Insert a breakpoint right after the end of the atomic sequence.  */
  breaks[0] = loc;

  /* Check for duplicated breakpoints.  Check also for a breakpoint
     placed (branch instruction's destination) anywhere in sequence.  */
  if (last_breakpoint
      && (breaks[1] == breaks[0]
	  || (breaks[1] >= pc && breaks[1] <= closing_insn)))
    last_breakpoint = 0;

  std::vector<CORE_ADDR> next_pcs;

  for (index = 0; index <= last_breakpoint; index++)
    next_pcs.push_back (breaks[index]);

  return next_pcs;
}


#define SIGNED_SHORT(x) 						\
  ((sizeof (short) == 2)						\
   ? ((int)(short)(x))							\
   : ((int)((((x) & 0xffff) ^ 0x8000) - 0x8000)))

#define GET_SRC_REG(x) (((x) >> 21) & 0x1f)

/* Limit the number of skipped non-prologue instructions, as the examining
   of the prologue is expensive.  */
static int max_skip_non_prologue_insns = 10;

/* Return nonzero if the given instruction OP can be part of the prologue
   of a function and saves a parameter on the stack.  FRAMEP should be
   set if one of the previous instructions in the function has set the
   Frame Pointer.  */

static int
store_param_on_stack_p (unsigned long op, int framep, int *r0_contains_arg)
{
  /* Move parameters from argument registers to temporary register.  */
  if ((op & 0xfc0007fe) == 0x7c000378)         /* mr(.)  Rx,Ry */
    {
      /* Rx must be scratch register r0.  */
      const int rx_regno = (op >> 16) & 31;
      /* Ry: Only r3 - r10 are used for parameter passing.  */
      const int ry_regno = GET_SRC_REG (op);

      if (rx_regno == 0 && ry_regno >= 3 && ry_regno <= 10)
	{
	  *r0_contains_arg = 1;
	  return 1;
	}
      else
	return 0;
    }

  /* Save a General Purpose Register on stack.  */

  if ((op & 0xfc1f0003) == 0xf8010000 ||       /* std  Rx,NUM(r1) */
      (op & 0xfc1f0000) == 0xd8010000)         /* stfd Rx,NUM(r1) */
    {
      /* Rx: Only r3 - r10 are used for parameter passing.  */
      const int rx_regno = GET_SRC_REG (op);

      return (rx_regno >= 3 && rx_regno <= 10);
    }
	   
  /* Save a General Purpose Register on stack via the Frame Pointer.  */

  if (framep &&
      ((op & 0xfc1f0000) == 0x901f0000 ||     /* st rx,NUM(r31) */
       (op & 0xfc1f0000) == 0x981f0000 ||     /* stb Rx,NUM(r31) */
       (op & 0xfc1f0000) == 0xd81f0000))      /* stfd Rx,NUM(r31) */
    {
      /* Rx: Usually, only r3 - r10 are used for parameter passing.
	 However, the compiler sometimes uses r0 to hold an argument.  */
      const int rx_regno = GET_SRC_REG (op);

      return ((rx_regno >= 3 && rx_regno <= 10)
	      || (rx_regno == 0 && *r0_contains_arg));
    }

  if ((op & 0xfc1f0000) == 0xfc010000)         /* frsp, fp?,NUM(r1) */
    {
      /* Only f2 - f8 are used for parameter passing.  */
      const int src_regno = GET_SRC_REG (op);

      return (src_regno >= 2 && src_regno <= 8);
    }

  if (framep && ((op & 0xfc1f0000) == 0xfc1f0000))  /* frsp, fp?,NUM(r31) */
    {
      /* Only f2 - f8 are used for parameter passing.  */
      const int src_regno = GET_SRC_REG (op);

      return (src_regno >= 2 && src_regno <= 8);
    }

  /* Not an insn that saves a parameter on stack.  */
  return 0;
}

/* Assuming that INSN is a "bl" instruction located at PC, return
   nonzero if the destination of the branch is a "blrl" instruction.
   
   This sequence is sometimes found in certain function prologues.
   It allows the function to load the LR register with a value that
   they can use to access PIC data using PC-relative offsets.  */

static int
bl_to_blrl_insn_p (CORE_ADDR pc, int insn, enum bfd_endian byte_order)
{
  CORE_ADDR dest;
  int immediate;
  int absolute;
  int dest_insn;

  absolute = (int) ((insn >> 1) & 1);
  immediate = ((insn & ~3) << 6) >> 6;
  if (absolute)
    dest = immediate;
  else
    dest = pc + immediate;

  dest_insn = read_memory_integer (dest, 4, byte_order);
  if ((dest_insn & 0xfc00ffff) == 0x4c000021) /* blrl */
    return 1;

  return 0;
}

/* Return true if OP is a stw or std instruction with
   register operands RS and RA and any immediate offset.

   If WITH_UPDATE is true, also return true if OP is
   a stwu or stdu instruction with the same operands.

   Return false otherwise.
   */
static bool
store_insn_p (unsigned long op, unsigned long rs,
	      unsigned long ra, bool with_update)
{
  rs = rs << 21;
  ra = ra << 16;

  if (/* std RS, SIMM(RA) */
      ((op & 0xffff0003) == (rs | ra | 0xf8000000)) ||
      /* stw RS, SIMM(RA) */
      ((op & 0xffff0000) == (rs | ra | 0x90000000)))
    return true;

  if (with_update)
    {
      if (/* stdu RS, SIMM(RA) */
	  ((op & 0xffff0003) == (rs | ra | 0xf8000001)) ||
	  /* stwu RS, SIMM(RA) */
	  ((op & 0xffff0000) == (rs | ra | 0x94000000)))
	return true;
    }

  return false;
}

/* Masks for decoding a branch-and-link (bl) instruction.

   BL_MASK and BL_INSTRUCTION are used in combination with each other.
   The former is anded with the opcode in question; if the result of
   this masking operation is equal to BL_INSTRUCTION, then the opcode in
   question is a ``bl'' instruction.
   
   BL_DISPLACEMENT_MASK is anded with the opcode in order to extract
   the branch displacement.  */

#define BL_MASK 0xfc000001
#define BL_INSTRUCTION 0x48000001
#define BL_DISPLACEMENT_MASK 0x03fffffc

static unsigned long
rs6000_fetch_instruction (struct gdbarch *gdbarch, const CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  unsigned long op;

  /* Fetch the instruction and convert it to an integer.  */
  if (target_read_memory (pc, buf, 4))
    return 0;
  op = extract_unsigned_integer (buf, 4, byte_order);

  return op;
}

/* GCC generates several well-known sequences of instructions at the begining
   of each function prologue when compiling with -fstack-check.  If one of
   such sequences starts at START_PC, then return the address of the
   instruction immediately past this sequence.  Otherwise, return START_PC.  */
   
static CORE_ADDR
rs6000_skip_stack_check (struct gdbarch *gdbarch, const CORE_ADDR start_pc)
{
  CORE_ADDR pc = start_pc;
  unsigned long op = rs6000_fetch_instruction (gdbarch, pc);

  /* First possible sequence: A small number of probes.
	 stw 0, -<some immediate>(1)
	 [repeat this instruction any (small) number of times].  */
  
  if ((op & 0xffff0000) == 0x90010000)
    {
      while ((op & 0xffff0000) == 0x90010000)
	{
	  pc = pc + 4;
	  op = rs6000_fetch_instruction (gdbarch, pc);
	}
      return pc;
    }

  /* Second sequence: A probing loop.
	 addi 12,1,-<some immediate>
	 lis 0,-<some immediate>
	 [possibly ori 0,0,<some immediate>]
	 add 0,12,0
	 cmpw 0,12,0
	 beq 0,<disp>
	 addi 12,12,-<some immediate>
	 stw 0,0(12)
	 b <disp>
	 [possibly one last probe: stw 0,<some immediate>(12)].  */

  while (1)
    {
      /* addi 12,1,-<some immediate> */
      if ((op & 0xffff0000) != 0x39810000)
	break;

      /* lis 0,-<some immediate> */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xffff0000) != 0x3c000000)
	break;

      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      /* [possibly ori 0,0,<some immediate>] */
      if ((op & 0xffff0000) == 0x60000000)
	{
	  pc = pc + 4;
	  op = rs6000_fetch_instruction (gdbarch, pc);
	}
      /* add 0,12,0 */
      if (op != 0x7c0c0214)
	break;

      /* cmpw 0,12,0 */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if (op != 0x7c0c0000)
	break;

      /* beq 0,<disp> */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xff9f0001) != 0x41820000)
	break;

      /* addi 12,12,-<some immediate> */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xffff0000) != 0x398c0000)
	break;

      /* stw 0,0(12) */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if (op != 0x900c0000)
	break;

      /* b <disp> */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xfc000001) != 0x48000000)
	break;

      /* [possibly one last probe: stw 0,<some immediate>(12)].  */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xffff0000) == 0x900c0000)
	{
	  pc = pc + 4;
	  op = rs6000_fetch_instruction (gdbarch, pc);
	}

      /* We found a valid stack-check sequence, return the new PC.  */
      return pc;
    }

  /* Third sequence: No probe; instead, a comparison between the stack size
     limit (saved in a run-time global variable) and the current stack
     pointer:

	addi 0,1,-<some immediate>
	lis 12,__gnat_stack_limit@ha
	lwz 12,__gnat_stack_limit@l(12)
	twllt 0,12

     or, with a small variant in the case of a bigger stack frame:
	addis 0,1,<some immediate>
	addic 0,0,-<some immediate>
	lis 12,__gnat_stack_limit@ha
	lwz 12,__gnat_stack_limit@l(12)
	twllt 0,12
  */
  while (1)
    {
      /* addi 0,1,-<some immediate> */
      if ((op & 0xffff0000) != 0x38010000)
	{
	  /* small stack frame variant not recognized; try the
	     big stack frame variant: */

	  /* addis 0,1,<some immediate> */
	  if ((op & 0xffff0000) != 0x3c010000)
	    break;

	  /* addic 0,0,-<some immediate> */
	  pc = pc + 4;
	  op = rs6000_fetch_instruction (gdbarch, pc);
	  if ((op & 0xffff0000) != 0x30000000)
	    break;
	}

      /* lis 12,<some immediate> */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xffff0000) != 0x3d800000)
	break;
      
      /* lwz 12,<some immediate>(12) */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xffff0000) != 0x818c0000)
	break;

      /* twllt 0,12 */
      pc = pc + 4;
      op = rs6000_fetch_instruction (gdbarch, pc);
      if ((op & 0xfffffffe) != 0x7c406008)
	break;

      /* We found a valid stack-check sequence, return the new PC.  */
      return pc;
    }

  /* No stack check code in our prologue, return the start_pc.  */
  return start_pc;
}

/* return pc value after skipping a function prologue and also return
   information about a function frame.

   in struct rs6000_framedata fdata:
   - frameless is TRUE, if function does not have a frame.
   - nosavedpc is TRUE, if function does not save %pc value in its frame.
   - offset is the initial size of this stack frame --- the amount by
   which we decrement the sp to allocate the frame.
   - saved_gpr is the number of the first saved gpr.
   - saved_fpr is the number of the first saved fpr.
   - saved_vr is the number of the first saved vr.
   - saved_ev is the number of the first saved ev.
   - alloca_reg is the number of the register used for alloca() handling.
   Otherwise -1.
   - gpr_offset is the offset of the first saved gpr from the previous frame.
   - fpr_offset is the offset of the first saved fpr from the previous frame.
   - vr_offset is the offset of the first saved vr from the previous frame.
   - ev_offset is the offset of the first saved ev from the previous frame.
   - lr_offset is the offset of the saved lr
   - cr_offset is the offset of the saved cr
   - vrsave_offset is the offset of the saved vrsave register.  */

static CORE_ADDR
skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc, CORE_ADDR lim_pc,
	       struct rs6000_framedata *fdata)
{
  CORE_ADDR orig_pc = pc;
  CORE_ADDR last_prologue_pc = pc;
  CORE_ADDR li_found_pc = 0;
  gdb_byte buf[4];
  unsigned long op;
  long offset = 0;
  long alloca_reg_offset = 0;
  long vr_saved_offset = 0;
  int lr_reg = -1;
  int cr_reg = -1;
  int vr_reg = -1;
  int ev_reg = -1;
  long ev_offset = 0;
  int vrsave_reg = -1;
  int reg;
  int framep = 0;
  int minimal_toc_loaded = 0;
  int prev_insn_was_prologue_insn = 1;
  int num_skip_non_prologue_insns = 0;
  int r0_contains_arg = 0;
  const struct bfd_arch_info *arch_info = gdbarch_bfd_arch_info (gdbarch);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  memset (fdata, 0, sizeof (struct rs6000_framedata));
  fdata->saved_gpr = -1;
  fdata->saved_fpr = -1;
  fdata->saved_vr = -1;
  fdata->saved_ev = -1;
  fdata->alloca_reg = -1;
  fdata->frameless = 1;
  fdata->nosavedpc = 1;
  fdata->lr_register = -1;

  pc = rs6000_skip_stack_check (gdbarch, pc);
  if (pc >= lim_pc)
    pc = lim_pc;

  for (;; pc += 4)
    {
      /* Sometimes it isn't clear if an instruction is a prologue
	 instruction or not.  When we encounter one of these ambiguous
	 cases, we'll set prev_insn_was_prologue_insn to 0 (false).
	 Otherwise, we'll assume that it really is a prologue instruction.  */
      if (prev_insn_was_prologue_insn)
	last_prologue_pc = pc;

      /* Stop scanning if we've hit the limit.  */
      if (pc >= lim_pc)
	break;

      prev_insn_was_prologue_insn = 1;

      /* Fetch the instruction and convert it to an integer.  */
      if (target_read_memory (pc, buf, 4))
	break;
      op = extract_unsigned_integer (buf, 4, byte_order);

      if ((op & 0xfc1fffff) == 0x7c0802a6)
	{			/* mflr Rx */
	  /* Since shared library / PIC code, which needs to get its
	     address at runtime, can appear to save more than one link
	     register vis:

	     stwu r1,-304(r1)
	     mflr r3
	     bl 0xff570d0 (blrl)
	     stw r30,296(r1)
	     mflr r30
	     stw r31,300(r1)
	     stw r3,308(r1);
	     ...

	     remember just the first one, but skip over additional
	     ones.  */
	  if (lr_reg == -1)
	    lr_reg = (op & 0x03e00000) >> 21;
	  if (lr_reg == 0)
	    r0_contains_arg = 0;
	  continue;
	}
      else if ((op & 0xfc1fffff) == 0x7c000026)
	{			/* mfcr Rx */
	  cr_reg = (op & 0x03e00000) >> 21;
	  if (cr_reg == 0)
	    r0_contains_arg = 0;
	  continue;

	}
      else if ((op & 0xfc1f0000) == 0xd8010000)
	{			/* stfd Rx,NUM(r1) */
	  reg = GET_SRC_REG (op);
	  if (fdata->saved_fpr == -1 || fdata->saved_fpr > reg)
	    {
	      fdata->saved_fpr = reg;
	      fdata->fpr_offset = SIGNED_SHORT (op) + offset;
	    }
	  continue;

	}
      else if (((op & 0xfc1f0000) == 0xbc010000) ||	/* stm Rx, NUM(r1) */
	       (((op & 0xfc1f0000) == 0x90010000 ||	/* st rx,NUM(r1) */
		 (op & 0xfc1f0003) == 0xf8010000) &&	/* std rx,NUM(r1) */
		(op & 0x03e00000) >= 0x01a00000))	/* rx >= r13 */
	{

	  reg = GET_SRC_REG (op);
	  if ((op & 0xfc1f0000) == 0xbc010000)
	    fdata->gpr_mask |= ~((1U << reg) - 1);
	  else
	    fdata->gpr_mask |= 1U << reg;
	  if (fdata->saved_gpr == -1 || fdata->saved_gpr > reg)
	    {
	      fdata->saved_gpr = reg;
	      if ((op & 0xfc1f0003) == 0xf8010000)
		op &= ~3UL;
	      fdata->gpr_offset = SIGNED_SHORT (op) + offset;
	    }
	  continue;

	}
      else if ((op & 0xffff0000) == 0x3c4c0000
	       || (op & 0xffff0000) == 0x3c400000
	       || (op & 0xffff0000) == 0x38420000)
	{
	  /* .	0:	addis 2,12,.TOC.-0b@ha
	     .		addi 2,2,.TOC.-0b@l
	     or
	     .		lis 2,.TOC.@ha
	     .		addi 2,2,.TOC.@l
	     used by ELFv2 global entry points to set up r2.  */
	  continue;
	}
      else if (op == 0x60000000)
	{
	  /* nop */
	  /* Allow nops in the prologue, but do not consider them to
	     be part of the prologue unless followed by other prologue
	     instructions.  */
	  prev_insn_was_prologue_insn = 0;
	  continue;

	}
      else if ((op & 0xffff0000) == 0x3c000000)
	{			/* addis 0,0,NUM, used for >= 32k frames */
	  fdata->offset = (op & 0x0000ffff) << 16;
	  fdata->frameless = 0;
	  r0_contains_arg = 0;
	  continue;

	}
      else if ((op & 0xffff0000) == 0x60000000)
	{			/* ori 0,0,NUM, 2nd half of >= 32k frames */
	  fdata->offset |= (op & 0x0000ffff);
	  fdata->frameless = 0;
	  r0_contains_arg = 0;
	  continue;

	}
      else if (lr_reg >= 0 &&
	       ((store_insn_p (op, lr_reg, 1, true)) ||
		(framep &&
		 (store_insn_p (op, lr_reg,
				fdata->alloca_reg - tdep->ppc_gp0_regnum,
				false)))))
	{
	  if (store_insn_p (op, lr_reg, 1, true))
	    fdata->lr_offset = offset;
	  else /* LR save through frame pointer. */
	    fdata->lr_offset = alloca_reg_offset;

	  fdata->nosavedpc = 0;
	  /* Invalidate lr_reg, but don't set it to -1.
	     That would mean that it had never been set.  */
	  lr_reg = -2;
	  if ((op & 0xfc000003) == 0xf8000000 ||	/* std */
	      (op & 0xfc000000) == 0x90000000)		/* stw */
	    {
	      /* Does not update r1, so add displacement to lr_offset.  */
	      fdata->lr_offset += SIGNED_SHORT (op);
	    }
	  continue;

	}
      else if (cr_reg >= 0 &&
	       (store_insn_p (op, cr_reg, 1, true)))
	{
	  fdata->cr_offset = offset;
	  /* Invalidate cr_reg, but don't set it to -1.
	     That would mean that it had never been set.  */
	  cr_reg = -2;
	  if ((op & 0xfc000003) == 0xf8000000 ||
	      (op & 0xfc000000) == 0x90000000)
	    {
	      /* Does not update r1, so add displacement to cr_offset.  */
	      fdata->cr_offset += SIGNED_SHORT (op);
	    }
	  continue;

	}
      else if ((op & 0xfe80ffff) == 0x42800005 && lr_reg != -1)
	{
	  /* bcl 20,xx,.+4 is used to get the current PC, with or without
	     prediction bits.  If the LR has already been saved, we can
	     skip it.  */
	  continue;
	}
      else if (op == 0x48000005)
	{			/* bl .+4 used in 
				   -mrelocatable */
	  fdata->used_bl = 1;
	  continue;

	}
      else if (op == 0x48000004)
	{			/* b .+4 (xlc) */
	  break;

	}
      else if ((op & 0xffff0000) == 0x3fc00000 ||  /* addis 30,0,foo@ha, used
						      in V.4 -mminimal-toc */
	       (op & 0xffff0000) == 0x3bde0000)
	{			/* addi 30,30,foo@l */
	  continue;

	}
      else if ((op & 0xfc000001) == 0x48000001)
	{			/* bl foo, 
				   to save fprs???  */

	  fdata->frameless = 0;

	  /* If the return address has already been saved, we can skip
	     calls to blrl (for PIC).  */
	  if (lr_reg != -1 && bl_to_blrl_insn_p (pc, op, byte_order))
	    {
	      fdata->used_bl = 1;
	      continue;
	    }

	  /* Don't skip over the subroutine call if it is not within
	     the first three instructions of the prologue and either
	     we have no line table information or the line info tells
	     us that the subroutine call is not part of the line
	     associated with the prologue.  */
	  if ((pc - orig_pc) > 8)
	    {
	      struct symtab_and_line prologue_sal = find_pc_line (orig_pc, 0);
	      struct symtab_and_line this_sal = find_pc_line (pc, 0);

	      if ((prologue_sal.line == 0)
		  || (prologue_sal.line != this_sal.line))
		break;
	    }

	  op = read_memory_integer (pc + 4, 4, byte_order);

	  /* At this point, make sure this is not a trampoline
	     function (a function that simply calls another functions,
	     and nothing else).  If the next is not a nop, this branch
	     was part of the function prologue.  */

	  if (op == 0x4def7b82 || op == 0)	/* crorc 15, 15, 15 */
	    break;		/* Don't skip over 
				   this branch.  */

	  fdata->used_bl = 1;
	  continue;
	}
      /* update stack pointer */
      else if ((op & 0xfc1f0000) == 0x94010000)
	{		/* stu rX,NUM(r1) ||  stwu rX,NUM(r1) */
	  fdata->frameless = 0;
	  fdata->offset = SIGNED_SHORT (op);
	  offset = fdata->offset;
	  continue;
	}
      else if ((op & 0xfc1f07fa) == 0x7c01016a)
	{		/* stwux rX,r1,rY  || stdux rX,r1,rY */
	  /* No way to figure out what r1 is going to be.  */
	  fdata->frameless = 0;
	  offset = fdata->offset;
	  continue;
	}
      else if ((op & 0xfc1f0003) == 0xf8010001)
	{			/* stdu rX,NUM(r1) */
	  fdata->frameless = 0;
	  fdata->offset = SIGNED_SHORT (op & ~3UL);
	  offset = fdata->offset;
	  continue;
	}
      else if ((op & 0xffff0000) == 0x38210000)
	{			/* addi r1,r1,SIMM */
	  fdata->frameless = 0;
	  fdata->offset += SIGNED_SHORT (op);
	  offset = fdata->offset;
	  continue;
	}
      /* Load up minimal toc pointer.  Do not treat an epilogue restore
	 of r31 as a minimal TOC load.  */
      else if (((op >> 22) == 0x20f	||	/* l r31,... or l r30,...  */
	       (op >> 22) == 0x3af)		/* ld r31,... or ld r30,...  */
	       && !framep
	       && !minimal_toc_loaded)
	{
	  minimal_toc_loaded = 1;
	  continue;

	  /* move parameters from argument registers to local variable
	     registers */
	}
      else if ((op & 0xfc0007fe) == 0x7c000378 &&	/* mr(.)  Rx,Ry */
	       (((op >> 21) & 31) >= 3) &&              /* R3 >= Ry >= R10 */
	       (((op >> 21) & 31) <= 10) &&
	       ((long) ((op >> 16) & 31)
		>= fdata->saved_gpr)) /* Rx: local var reg */
	{
	  continue;

	  /* store parameters in stack */
	}
      /* Move parameters from argument registers to temporary register.  */
      else if (store_param_on_stack_p (op, framep, &r0_contains_arg))
	{
	  continue;

	  /* Set up frame pointer */
	}
      else if (op == 0x603d0000)       /* oril r29, r1, 0x0 */
	{
	  fdata->frameless = 0;
	  framep = 1;
	  fdata->alloca_reg = (tdep->ppc_gp0_regnum + 29);
	  alloca_reg_offset = offset;
	  continue;

	  /* Another way to set up the frame pointer.  */
	}
      else if (op == 0x603f0000	/* oril r31, r1, 0x0 */
	       || op == 0x7c3f0b78)
	{			/* mr r31, r1 */
	  fdata->frameless = 0;
	  framep = 1;
	  fdata->alloca_reg = (tdep->ppc_gp0_regnum + 31);
	  alloca_reg_offset = offset;
	  continue;

	  /* Another way to set up the frame pointer.  */
	}
      else if ((op & 0xfc1fffff) == 0x38010000)
	{			/* addi rX, r1, 0x0 */
	  fdata->frameless = 0;
	  framep = 1;
	  fdata->alloca_reg = (tdep->ppc_gp0_regnum
			       + ((op & ~0x38010000) >> 21));
	  alloca_reg_offset = offset;
	  continue;
	}
      /* AltiVec related instructions.  */
      /* Store the vrsave register (spr 256) in another register for
	 later manipulation, or load a register into the vrsave
	 register.  2 instructions are used: mfvrsave and
	 mtvrsave.  They are shorthand notation for mfspr Rn, SPR256
	 and mtspr SPR256, Rn.  */
      /* mfspr Rn SPR256 == 011111 nnnnn 0000001000 01010100110
	 mtspr SPR256 Rn == 011111 nnnnn 0000001000 01110100110  */
      else if ((op & 0xfc1fffff) == 0x7c0042a6)    /* mfvrsave Rn */
	{
	  vrsave_reg = GET_SRC_REG (op);
	  continue;
	}
      else if ((op & 0xfc1fffff) == 0x7c0043a6)     /* mtvrsave Rn */
	{
	  continue;
	}
      /* Store the register where vrsave was saved to onto the stack:
	 rS is the register where vrsave was stored in a previous
	 instruction.  */
      /* 100100 sssss 00001 dddddddd dddddddd */
      else if ((op & 0xfc1f0000) == 0x90010000)     /* stw rS, d(r1) */
	{
	  if (vrsave_reg == GET_SRC_REG (op))
	    {
	      fdata->vrsave_offset = SIGNED_SHORT (op) + offset;
	      vrsave_reg = -1;
	    }
	  continue;
	}
      /* Compute the new value of vrsave, by modifying the register
	 where vrsave was saved to.  */
      else if (((op & 0xfc000000) == 0x64000000)    /* oris Ra, Rs, UIMM */
	       || ((op & 0xfc000000) == 0x60000000))/* ori Ra, Rs, UIMM */
	{
	  continue;
	}
      /* li r0, SIMM (short for addi r0, 0, SIMM).  This is the first
	 in a pair of insns to save the vector registers on the
	 stack.  */
      /* 001110 00000 00000 iiii iiii iiii iiii  */
      /* 001110 01110 00000 iiii iiii iiii iiii  */
      else if ((op & 0xffff0000) == 0x38000000         /* li r0, SIMM */
	       || (op & 0xffff0000) == 0x39c00000)     /* li r14, SIMM */
	{
	  if ((op & 0xffff0000) == 0x38000000)
	    r0_contains_arg = 0;
	  li_found_pc = pc;
	  vr_saved_offset = SIGNED_SHORT (op);

	  /* This insn by itself is not part of the prologue, unless
	     if part of the pair of insns mentioned above.  So do not
	     record this insn as part of the prologue yet.  */
	  prev_insn_was_prologue_insn = 0;
	}
      /* Store vector register S at (r31+r0) aligned to 16 bytes.  */      
      /* 011111 sssss 11111 00000 00111001110 */
      else if ((op & 0xfc1fffff) == 0x7c1f01ce)   /* stvx Vs, R31, R0 */
	{
	  if (pc == (li_found_pc + 4))
	    {
	      vr_reg = GET_SRC_REG (op);
	      /* If this is the first vector reg to be saved, or if
		 it has a lower number than others previously seen,
		 reupdate the frame info.  */
	      if (fdata->saved_vr == -1 || fdata->saved_vr > vr_reg)
		{
		  fdata->saved_vr = vr_reg;
		  fdata->vr_offset = vr_saved_offset + offset;
		}
	      vr_saved_offset = -1;
	      vr_reg = -1;
	      li_found_pc = 0;
	    }
	}
      /* End AltiVec related instructions.  */

      /* Start BookE related instructions.  */
      /* Store gen register S at (r31+uimm).
	 Any register less than r13 is volatile, so we don't care.  */
      /* 000100 sssss 11111 iiiii 01100100001 */
      else if (arch_info->mach == bfd_mach_ppc_e500
	       && (op & 0xfc1f07ff) == 0x101f0321)    /* evstdd Rs,uimm(R31) */
	{
	  if ((op & 0x03e00000) >= 0x01a00000)	/* Rs >= r13 */
	    {
	      unsigned int imm;
	      ev_reg = GET_SRC_REG (op);
	      imm = (op >> 11) & 0x1f;
	      ev_offset = imm * 8;
	      /* If this is the first vector reg to be saved, or if
		 it has a lower number than others previously seen,
		 reupdate the frame info.  */
	      if (fdata->saved_ev == -1 || fdata->saved_ev > ev_reg)
		{
		  fdata->saved_ev = ev_reg;
		  fdata->ev_offset = ev_offset + offset;
		}
	    }
	  continue;
	}
      /* Store gen register rS at (r1+rB).  */
      /* 000100 sssss 00001 bbbbb 01100100000 */
      else if (arch_info->mach == bfd_mach_ppc_e500
	       && (op & 0xffe007ff) == 0x13e00320)     /* evstddx RS,R1,Rb */
	{
	  if (pc == (li_found_pc + 4))
	    {
	      ev_reg = GET_SRC_REG (op);
	      /* If this is the first vector reg to be saved, or if
		 it has a lower number than others previously seen,
		 reupdate the frame info.  */
	      /* We know the contents of rB from the previous instruction.  */
	      if (fdata->saved_ev == -1 || fdata->saved_ev > ev_reg)
		{
		  fdata->saved_ev = ev_reg;
		  fdata->ev_offset = vr_saved_offset + offset;
		}
	      vr_saved_offset = -1;
	      ev_reg = -1;
	      li_found_pc = 0;
	    }
	  continue;
	}
      /* Store gen register r31 at (rA+uimm).  */
      /* 000100 11111 aaaaa iiiii 01100100001 */
      else if (arch_info->mach == bfd_mach_ppc_e500
	       && (op & 0xffe007ff) == 0x13e00321)   /* evstdd R31,Ra,UIMM */
	{
	  /* Wwe know that the source register is 31 already, but
	     it can't hurt to compute it.  */
	  ev_reg = GET_SRC_REG (op);
	  ev_offset = ((op >> 11) & 0x1f) * 8;
	  /* If this is the first vector reg to be saved, or if
	     it has a lower number than others previously seen,
	     reupdate the frame info.  */
	  if (fdata->saved_ev == -1 || fdata->saved_ev > ev_reg)
	    {
	      fdata->saved_ev = ev_reg;
	      fdata->ev_offset = ev_offset + offset;
	    }

	  continue;
	}
      /* Store gen register S at (r31+r0).
	 Store param on stack when offset from SP bigger than 4 bytes.  */
      /* 000100 sssss 11111 00000 01100100000 */
      else if (arch_info->mach == bfd_mach_ppc_e500
	       && (op & 0xfc1fffff) == 0x101f0320)     /* evstddx Rs,R31,R0 */
	{
	  if (pc == (li_found_pc + 4))
	    {
	      if ((op & 0x03e00000) >= 0x01a00000)
		{
		  ev_reg = GET_SRC_REG (op);
		  /* If this is the first vector reg to be saved, or if
		     it has a lower number than others previously seen,
		     reupdate the frame info.  */
		  /* We know the contents of r0 from the previous
		     instruction.  */
		  if (fdata->saved_ev == -1 || fdata->saved_ev > ev_reg)
		    {
		      fdata->saved_ev = ev_reg;
		      fdata->ev_offset = vr_saved_offset + offset;
		    }
		  ev_reg = -1;
		}
	      vr_saved_offset = -1;
	      li_found_pc = 0;
	      continue;
	    }
	}
      /* End BookE related instructions.  */

      else
	{
	  /* Not a recognized prologue instruction.
	     Handle optimizer code motions into the prologue by continuing
	     the search if we have no valid frame yet or if the return
	     address is not yet saved in the frame.  Also skip instructions
	     if some of the GPRs expected to be saved are not yet saved.  */
	  if (fdata->frameless == 0 && fdata->nosavedpc == 0
	      && fdata->saved_gpr != -1)
	    {
	      unsigned int all_mask = ~((1U << fdata->saved_gpr) - 1);

	      if ((fdata->gpr_mask & all_mask) == all_mask)
		break;
	    }

	  if (op == 0x4e800020		/* blr */
	      || op == 0x4e800420)	/* bctr */
	    /* Do not scan past epilogue in frameless functions or
	       trampolines.  */
	    break;
	  if ((op & 0xf4000000) == 0x40000000) /* bxx */
	    /* Never skip branches.  */
	    break;

	  /* Test based on opcode and mask values of
	     powerpc_opcodes[svc..svcla] in opcodes/ppc-opc.c.  */
	  if ((op & 0xffff0000) == 0x44000000)
	    /* Never skip system calls.  */
	    break;

	  if (num_skip_non_prologue_insns++ > max_skip_non_prologue_insns)
	    /* Do not scan too many insns, scanning insns is expensive with
	       remote targets.  */
	    break;

	  /* Continue scanning.  */
	  prev_insn_was_prologue_insn = 0;
	  continue;
	}
    }

#if 0
/* I have problems with skipping over __main() that I need to address
 * sometime.  Previously, I used to use misc_function_vector which
 * didn't work as well as I wanted to be.  -MGO */

  /* If the first thing after skipping a prolog is a branch to a function,
     this might be a call to an initializer in main(), introduced by gcc2.
     We'd like to skip over it as well.  Fortunately, xlc does some extra
     work before calling a function right after a prologue, thus we can
     single out such gcc2 behaviour.  */


  if ((op & 0xfc000001) == 0x48000001)
    {				/* bl foo, an initializer function?  */
      op = read_memory_integer (pc + 4, 4, byte_order);

      if (op == 0x4def7b82)
	{			/* cror 0xf, 0xf, 0xf (nop) */

	  /* Check and see if we are in main.  If so, skip over this
	     initializer function as well.  */

	  tmp = find_pc_misc_function (pc);
	  if (tmp >= 0
	      && strcmp (misc_function_vector[tmp].name, main_name ()) == 0)
	    return pc + 8;
	}
    }
#endif /* 0 */

  if (pc == lim_pc && lr_reg >= 0)
    fdata->lr_register = lr_reg;

  fdata->offset = -fdata->offset;
  return last_prologue_pc;
}

static CORE_ADDR
rs6000_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct rs6000_framedata frame;
  CORE_ADDR limit_pc, func_addr, func_end_addr = 0;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end_addr))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 100;          /* Magic.  */

  /* Do not allow limit_pc to be past the function end, if we know
     where that end is...  */
  if (func_end_addr && limit_pc > func_end_addr)
    limit_pc = func_end_addr;

  pc = skip_prologue (gdbarch, pc, limit_pc, &frame);
  return pc;
}

/* When compiling for EABI, some versions of GCC emit a call to __eabi
   in the prologue of main().

   The function below examines the code pointed at by PC and checks to
   see if it corresponds to a call to __eabi.  If so, it returns the
   address of the instruction following that call.  Otherwise, it simply
   returns PC.  */

static CORE_ADDR
rs6000_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  unsigned long op;

  if (target_read_memory (pc, buf, 4))
    return pc;
  op = extract_unsigned_integer (buf, 4, byte_order);

  if ((op & BL_MASK) == BL_INSTRUCTION)
    {
      CORE_ADDR displ = op & BL_DISPLACEMENT_MASK;
      CORE_ADDR call_dest = pc + 4 + displ;
      struct bound_minimal_symbol s = lookup_minimal_symbol_by_pc (call_dest);

      /* We check for ___eabi (three leading underscores) in addition
	 to __eabi in case the GCC option "-fleading-underscore" was
	 used to compile the program.  */
      if (s.minsym != NULL
	  && s.minsym->linkage_name () != NULL
	  && (strcmp (s.minsym->linkage_name (), "__eabi") == 0
	      || strcmp (s.minsym->linkage_name (), "___eabi") == 0))
	pc += 4;
    }
  return pc;
}

/* All the ABI's require 16 byte alignment.  */
static CORE_ADDR
rs6000_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return (addr & -16);
}

/* Return whether handle_inferior_event() should proceed through code
   starting at PC in function NAME when stepping.

   The AIX -bbigtoc linker option generates functions @FIX0, @FIX1, etc. to
   handle memory references that are too distant to fit in instructions
   generated by the compiler.  For example, if 'foo' in the following
   instruction:

     lwz r9,foo(r2)

   is greater than 32767, the linker might replace the lwz with a branch to
   somewhere in @FIX1 that does the load in 2 instructions and then branches
   back to where execution should continue.

   GDB should silently step over @FIX code, just like AIX dbx does.
   Unfortunately, the linker uses the "b" instruction for the
   branches, meaning that the link register doesn't get set.
   Therefore, GDB's usual step_over_function () mechanism won't work.

   Instead, use the gdbarch_skip_trampoline_code and
   gdbarch_skip_trampoline_code hooks in handle_inferior_event() to skip past
   @FIX code.  */

static int
rs6000_in_solib_return_trampoline (struct gdbarch *gdbarch,
				   CORE_ADDR pc, const char *name)
{
  return name && startswith (name, "@FIX");
}

/* Skip code that the user doesn't want to see when stepping:

   1. Indirect function calls use a piece of trampoline code to do context
   switching, i.e. to set the new TOC table.  Skip such code if we are on
   its first instruction (as when we have single-stepped to here).

   2. Skip shared library trampoline code (which is different from
   indirect function call trampolines).

   3. Skip bigtoc fixup code.

   Result is desired PC to step until, or NULL if we are not in
   code that should be skipped.  */

static CORE_ADDR
rs6000_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int ii, op;
  int rel;
  CORE_ADDR solib_target_pc;
  struct bound_minimal_symbol msymbol;

  static unsigned trampoline_code[] =
  {
    0x800b0000,			/*     l   r0,0x0(r11)  */
    0x90410014,			/*    st   r2,0x14(r1)  */
    0x7c0903a6,			/* mtctr   r0           */
    0x804b0004,			/*     l   r2,0x4(r11)  */
    0x816b0008,			/*     l  r11,0x8(r11)  */
    0x4e800420,			/*  bctr                */
    0x4e800020,			/*    br                */
    0
  };

  /* Check for bigtoc fixup code.  */
  msymbol = lookup_minimal_symbol_by_pc (pc);
  if (msymbol.minsym
      && rs6000_in_solib_return_trampoline (gdbarch, pc,
					    msymbol.minsym->linkage_name ()))
    {
      /* Double-check that the third instruction from PC is relative "b".  */
      op = read_memory_integer (pc + 8, 4, byte_order);
      if ((op & 0xfc000003) == 0x48000000)
	{
	  /* Extract bits 6-29 as a signed 24-bit relative word address and
	     add it to the containing PC.  */
	  rel = ((int)(op << 6) >> 6);
	  return pc + 8 + rel;
	}
    }

  /* If pc is in a shared library trampoline, return its target.  */
  solib_target_pc = find_solib_trampoline_target (frame, pc);
  if (solib_target_pc)
    return solib_target_pc;

  for (ii = 0; trampoline_code[ii]; ++ii)
    {
      op = read_memory_integer (pc + (ii * 4), 4, byte_order);
      if (op != trampoline_code[ii])
	return 0;
    }
  ii = get_frame_register_unsigned (frame, 11);	/* r11 holds destination
						   addr.  */
  pc = read_memory_unsigned_integer (ii, tdep->wordsize, byte_order);
  return pc;
}

/* ISA-specific vector types.  */

static struct type *
rs6000_builtin_type_vec64 (struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (!tdep->ppc_builtin_type_vec64)
    {
      const struct builtin_type *bt = builtin_type (gdbarch);

      /* The type we're building is this: */
#if 0
      union __gdb_builtin_type_vec64
	{
	  int64_t uint64;
	  float v2_float[2];
	  int32_t v2_int32[2];
	  int16_t v4_int16[4];
	  int8_t v8_int8[8];
	};
#endif

      struct type *t;

      t = arch_composite_type (gdbarch,
			       "__ppc_builtin_type_vec64", TYPE_CODE_UNION);
      append_composite_type_field (t, "uint64", bt->builtin_int64);
      append_composite_type_field (t, "v2_float",
				   init_vector_type (bt->builtin_float, 2));
      append_composite_type_field (t, "v2_int32",
				   init_vector_type (bt->builtin_int32, 2));
      append_composite_type_field (t, "v4_int16",
				   init_vector_type (bt->builtin_int16, 4));
      append_composite_type_field (t, "v8_int8",
				   init_vector_type (bt->builtin_int8, 8));

      t->set_is_vector (true);
      t->set_name ("ppc_builtin_type_vec64");
      tdep->ppc_builtin_type_vec64 = t;
    }

  return tdep->ppc_builtin_type_vec64;
}

/* Vector 128 type.  */

static struct type *
rs6000_builtin_type_vec128 (struct gdbarch *gdbarch)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (!tdep->ppc_builtin_type_vec128)
    {
      const struct builtin_type *bt = builtin_type (gdbarch);

      /* The type we're building is this

	 type = union __ppc_builtin_type_vec128 {
	     float128_t float128;
	     uint128_t uint128;
	     double v2_double[2];
	     float v4_float[4];
	     int32_t v4_int32[4];
	     int16_t v8_int16[8];
	     int8_t v16_int8[16];
	 }
      */

      /* PPC specific type for IEEE 128-bit float field */
      type_allocator alloc (gdbarch);
      struct type *t_float128
	= init_float_type (alloc, 128, "float128_t", floatformats_ieee_quad);

      struct type *t;

      t = arch_composite_type (gdbarch,
			       "__ppc_builtin_type_vec128", TYPE_CODE_UNION);
      append_composite_type_field (t, "float128", t_float128);
      append_composite_type_field (t, "uint128", bt->builtin_uint128);
      append_composite_type_field (t, "v2_double",
				   init_vector_type (bt->builtin_double, 2));
      append_composite_type_field (t, "v4_float",
				   init_vector_type (bt->builtin_float, 4));
      append_composite_type_field (t, "v4_int32",
				   init_vector_type (bt->builtin_int32, 4));
      append_composite_type_field (t, "v8_int16",
				   init_vector_type (bt->builtin_int16, 8));
      append_composite_type_field (t, "v16_int8",
				   init_vector_type (bt->builtin_int8, 16));

      t->set_is_vector (true);
      t->set_name ("ppc_builtin_type_vec128");
      tdep->ppc_builtin_type_vec128 = t;
    }

  return tdep->ppc_builtin_type_vec128;
}

/* Return the name of register number REGNO, or the empty string if it
   is an anonymous register.  */

static const char *
rs6000_register_name (struct gdbarch *gdbarch, int regno)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  /* The upper half "registers" have names in the XML description,
     but we present only the low GPRs and the full 64-bit registers
     to the user.  */
  if (tdep->ppc_ev0_upper_regnum >= 0
      && tdep->ppc_ev0_upper_regnum <= regno
      && regno < tdep->ppc_ev0_upper_regnum + ppc_num_gprs)
    return "";

  /* Hide the upper halves of the vs0~vs31 registers.  */
  if (tdep->ppc_vsr0_regnum >= 0
      && tdep->ppc_vsr0_upper_regnum <= regno
      && regno < tdep->ppc_vsr0_upper_regnum + ppc_num_gprs)
    return "";

  /* Hide the upper halves of the cvs0~cvs31 registers.  */
  if (PPC_CVSR0_UPPER_REGNUM <= regno
      && regno < (to_underlying (PPC_CVSR0_UPPER_REGNUM)
		  + to_underlying (ppc_num_gprs)))
    return "";

  /* Check if the SPE pseudo registers are available.  */
  if (IS_SPE_PSEUDOREG (tdep, regno))
    {
      static const char *const spe_regnames[] = {
	"ev0", "ev1", "ev2", "ev3", "ev4", "ev5", "ev6", "ev7",
	"ev8", "ev9", "ev10", "ev11", "ev12", "ev13", "ev14", "ev15",
	"ev16", "ev17", "ev18", "ev19", "ev20", "ev21", "ev22", "ev23",
	"ev24", "ev25", "ev26", "ev27", "ev28", "ev29", "ev30", "ev31",
      };
      return spe_regnames[regno - tdep->ppc_ev0_regnum];
    }

  /* Check if the decimal128 pseudo-registers are available.  */
  if (IS_DFP_PSEUDOREG (tdep, regno))
    {
      static const char *const dfp128_regnames[] = {
	"dl0", "dl1", "dl2", "dl3",
	"dl4", "dl5", "dl6", "dl7",
	"dl8", "dl9", "dl10", "dl11",
	"dl12", "dl13", "dl14", "dl15"
      };
      return dfp128_regnames[regno - tdep->ppc_dl0_regnum];
    }

  /* Check if this is a vX alias for a raw vrX vector register.  */
  if (IS_V_ALIAS_PSEUDOREG (tdep, regno))
    {
      static const char *const vector_alias_regnames[] = {
	"v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
	"v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
	"v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
	"v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"
      };
      return vector_alias_regnames[regno - tdep->ppc_v0_alias_regnum];
    }

  /* Check if this is a VSX pseudo-register.  */
  if (IS_VSX_PSEUDOREG (tdep, regno))
    {
      static const char *const vsx_regnames[] = {
	"vs0", "vs1", "vs2", "vs3", "vs4", "vs5", "vs6", "vs7",
	"vs8", "vs9", "vs10", "vs11", "vs12", "vs13", "vs14",
	"vs15", "vs16", "vs17", "vs18", "vs19", "vs20", "vs21",
	"vs22", "vs23", "vs24", "vs25", "vs26", "vs27", "vs28",
	"vs29", "vs30", "vs31", "vs32", "vs33", "vs34", "vs35",
	"vs36", "vs37", "vs38", "vs39", "vs40", "vs41", "vs42",
	"vs43", "vs44", "vs45", "vs46", "vs47", "vs48", "vs49",
	"vs50", "vs51", "vs52", "vs53", "vs54", "vs55", "vs56",
	"vs57", "vs58", "vs59", "vs60", "vs61", "vs62", "vs63"
      };
      return vsx_regnames[regno - tdep->ppc_vsr0_regnum];
    }

  /* Check if the this is a Extended FP pseudo-register.  */
  if (IS_EFP_PSEUDOREG (tdep, regno))
    {
      static const char *const efpr_regnames[] = {
	"f32", "f33", "f34", "f35", "f36", "f37", "f38",
	"f39", "f40", "f41", "f42", "f43", "f44", "f45",
	"f46", "f47", "f48", "f49", "f50", "f51",
	"f52", "f53", "f54", "f55", "f56", "f57",
	"f58", "f59", "f60", "f61", "f62", "f63"
      };
      return efpr_regnames[regno - tdep->ppc_efpr0_regnum];
    }

  /* Check if this is a Checkpointed DFP pseudo-register.  */
  if (IS_CDFP_PSEUDOREG (tdep, regno))
    {
      static const char *const cdfp128_regnames[] = {
	"cdl0", "cdl1", "cdl2", "cdl3",
	"cdl4", "cdl5", "cdl6", "cdl7",
	"cdl8", "cdl9", "cdl10", "cdl11",
	"cdl12", "cdl13", "cdl14", "cdl15"
      };
      return cdfp128_regnames[regno - tdep->ppc_cdl0_regnum];
    }

  /* Check if this is a Checkpointed VSX pseudo-register.  */
  if (IS_CVSX_PSEUDOREG (tdep, regno))
    {
      static const char *const cvsx_regnames[] = {
	"cvs0", "cvs1", "cvs2", "cvs3", "cvs4", "cvs5", "cvs6", "cvs7",
	"cvs8", "cvs9", "cvs10", "cvs11", "cvs12", "cvs13", "cvs14",
	"cvs15", "cvs16", "cvs17", "cvs18", "cvs19", "cvs20", "cvs21",
	"cvs22", "cvs23", "cvs24", "cvs25", "cvs26", "cvs27", "cvs28",
	"cvs29", "cvs30", "cvs31", "cvs32", "cvs33", "cvs34", "cvs35",
	"cvs36", "cvs37", "cvs38", "cvs39", "cvs40", "cvs41", "cvs42",
	"cvs43", "cvs44", "cvs45", "cvs46", "cvs47", "cvs48", "cvs49",
	"cvs50", "cvs51", "cvs52", "cvs53", "cvs54", "cvs55", "cvs56",
	"cvs57", "cvs58", "cvs59", "cvs60", "cvs61", "cvs62", "cvs63"
      };
      return cvsx_regnames[regno - tdep->ppc_cvsr0_regnum];
    }

  /* Check if the this is a Checkpointed Extended FP pseudo-register.  */
  if (IS_CEFP_PSEUDOREG (tdep, regno))
    {
      static const char *const cefpr_regnames[] = {
	"cf32", "cf33", "cf34", "cf35", "cf36", "cf37", "cf38",
	"cf39", "cf40", "cf41", "cf42", "cf43", "cf44", "cf45",
	"cf46", "cf47", "cf48", "cf49", "cf50", "cf51",
	"cf52", "cf53", "cf54", "cf55", "cf56", "cf57",
	"cf58", "cf59", "cf60", "cf61", "cf62", "cf63"
      };
      return cefpr_regnames[regno - tdep->ppc_cefpr0_regnum];
    }

  return tdesc_register_name (gdbarch, regno);
}

/* Return the GDB type object for the "standard" data type of data in
   register N.  */

static struct type *
rs6000_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  /* These are the e500 pseudo-registers.  */
  if (IS_SPE_PSEUDOREG (tdep, regnum))
    return rs6000_builtin_type_vec64 (gdbarch);
  else if (IS_DFP_PSEUDOREG (tdep, regnum)
	   || IS_CDFP_PSEUDOREG (tdep, regnum))
    /* PPC decimal128 pseudo-registers.  */
    return builtin_type (gdbarch)->builtin_declong;
  else if (IS_V_ALIAS_PSEUDOREG (tdep, regnum))
    return gdbarch_register_type (gdbarch,
				  tdep->ppc_vr0_regnum
				  + (regnum
				     - tdep->ppc_v0_alias_regnum));
  else if (IS_VSX_PSEUDOREG (tdep, regnum)
	   || IS_CVSX_PSEUDOREG (tdep, regnum))
    /* POWER7 VSX pseudo-registers.  */
    return rs6000_builtin_type_vec128 (gdbarch);
  else if (IS_EFP_PSEUDOREG (tdep, regnum)
	   || IS_CEFP_PSEUDOREG (tdep, regnum))
    /* POWER7 Extended FP pseudo-registers.  */
    return builtin_type (gdbarch)->builtin_double;
  else
    internal_error (_("rs6000_pseudo_register_type: "
		      "called on unexpected register '%s' (%d)"),
		    gdbarch_register_name (gdbarch, regnum), regnum);
}

/* Check if REGNUM is a member of REGGROUP.  We only need to handle
   the vX aliases for the vector registers by always returning false
   to avoid duplicated information in "info register vector/all",
   since the raw vrX registers will already show in these cases.  For
   other pseudo-registers we use the default membership function.  */

static int
rs6000_pseudo_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				   const struct reggroup *group)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (IS_V_ALIAS_PSEUDOREG (tdep, regnum))
    return 0;
  else
    return default_register_reggroup_p (gdbarch, regnum, group);
}

/* The register format for RS/6000 floating point registers is always
   double, we need a conversion if the memory format is float.  */

static int
rs6000_convert_register_p (struct gdbarch *gdbarch, int regnum,
			   struct type *type)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  return (tdep->ppc_fp0_regnum >= 0
	  && regnum >= tdep->ppc_fp0_regnum
	  && regnum < tdep->ppc_fp0_regnum + ppc_num_fprs
	  && type->code () == TYPE_CODE_FLT
	  && (type->length ()
	      == builtin_type (gdbarch)->builtin_float->length ()));
}

static int
ieee_128_float_regnum_adjust (struct gdbarch *gdbarch, struct type *type,
			      int regnum)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  /* If we have the an IEEE 128-bit floating point value, need to map the
   register number to the corresponding VSR.  */
  if (tdep->ppc_vsr0_regnum != -1
      && regnum >= tdep->ppc_fp0_regnum
      && regnum < (tdep->ppc_fp0_regnum + ppc_num_fprs)
      && (gdbarch_long_double_format (gdbarch) == floatformats_ieee_quad)
      && (type->length() == 16))
    regnum = regnum - tdep->ppc_fp0_regnum + tdep->ppc_vsr0_regnum;

  return regnum;
}

static int
rs6000_register_to_value (frame_info_ptr frame,
			  int regnum,
			  struct type *type,
			  gdb_byte *to,
			  int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte from[PPC_MAX_REGISTER_SIZE];
  
  gdb_assert (type->code () == TYPE_CODE_FLT);

  /* We have an IEEE 128-bit float -- need to change regnum mapping from
     fpr to vsr.  */
  regnum = ieee_128_float_regnum_adjust (gdbarch, type, regnum);

  auto from_view
    = gdb::make_array_view (from, register_size (gdbarch, regnum));
  frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);
  if (!get_frame_register_bytes (next_frame, regnum, 0, from_view, optimizedp,
				 unavailablep))
    return 0;

  target_float_convert (from, builtin_type (gdbarch)->builtin_double,
			to, type);
  *optimizedp = *unavailablep = 0;
  return 1;
}

static void
rs6000_value_to_register (frame_info_ptr frame,
			  int regnum,
			  struct type *type,
			  const gdb_byte *from)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte to[PPC_MAX_REGISTER_SIZE];

  gdb_assert (type->code () == TYPE_CODE_FLT);

  /* We have an IEEE 128-bit float -- need to change regnum mapping from
     fpr to vsr.  */
  regnum = ieee_128_float_regnum_adjust (gdbarch, type, regnum);

  struct type *to_type = builtin_type (gdbarch)->builtin_double;
  target_float_convert (from, type, to, to_type);
  auto to_view = gdb::make_array_view (to, to_type->length ());
  put_frame_register (get_next_frame_sentinel_okay (frame), regnum, to_view);
}

static value *
rs6000_value_from_register (gdbarch *gdbarch, type *type, int regnum,
			    const frame_info_ptr &this_frame)
{
  /* We have an IEEE 128-bit float -- need to change regnum mapping from
     fpr to vsr.  */
  regnum = ieee_128_float_regnum_adjust (gdbarch, type, regnum);

  value *value
    = value::allocate_register (get_next_frame_sentinel_okay (this_frame),
				regnum, type);

  /* Any structure stored in more than one register will always be
     an integral number of registers.  Otherwise, you need to do
     some fiddling with the last register copied here for little
     endian machines.  */
  if (type_byte_order (type) == BFD_ENDIAN_BIG
      && type->length () < register_size (gdbarch, regnum))
    /* Big-endian, and we want less than full size.  */
    value->set_offset (register_size (gdbarch, regnum) - type->length ());
  else
    value->set_offset (0);

  return value;
}

 /* The type of a function that moves the value of REG between CACHE
    or BUF --- in either direction.  */
typedef enum register_status (*move_ev_register_func) (struct regcache *,
						       int, void *);

/* Move SPE vector register values between a 64-bit buffer and the two
   32-bit raw register halves in a regcache.  This function handles
   both splitting a 64-bit value into two 32-bit halves, and joining
   two halves into a whole 64-bit value, depending on the function
   passed as the MOVE argument.

   EV_REG must be the number of an SPE evN vector register --- a
   pseudoregister.  REGCACHE must be a regcache, and BUFFER must be a
   64-bit buffer.

   Call MOVE once for each 32-bit half of that register, passing
   REGCACHE, the number of the raw register corresponding to that
   half, and the address of the appropriate half of BUFFER.

   For example, passing 'regcache_raw_read' as the MOVE function will
   fill BUFFER with the full 64-bit contents of EV_REG.  Or, passing
   'regcache_raw_supply' will supply the contents of BUFFER to the
   appropriate pair of raw registers in REGCACHE.

   You may need to cast away some 'const' qualifiers when passing
   MOVE, since this function can't tell at compile-time which of
   REGCACHE or BUFFER is acting as the source of the data.  If C had
   co-variant type qualifiers, ...  */

static enum register_status
e500_move_ev_register (move_ev_register_func move,
		       struct regcache *regcache, int ev_reg, void *buffer)
{
  struct gdbarch *arch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (arch);
  int reg_index;
  gdb_byte *byte_buffer = (gdb_byte *) buffer;
  enum register_status status;

  gdb_assert (IS_SPE_PSEUDOREG (tdep, ev_reg));

  reg_index = ev_reg - tdep->ppc_ev0_regnum;

  if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG)
    {
      status = move (regcache, tdep->ppc_ev0_upper_regnum + reg_index,
		     byte_buffer);
      if (status == REG_VALID)
	status = move (regcache, tdep->ppc_gp0_regnum + reg_index,
		       byte_buffer + 4);
    }
  else
    {
      status = move (regcache, tdep->ppc_gp0_regnum + reg_index, byte_buffer);
      if (status == REG_VALID)
	status = move (regcache, tdep->ppc_ev0_upper_regnum + reg_index,
		       byte_buffer + 4);
    }

  return status;
}

static enum register_status
do_regcache_raw_write (struct regcache *regcache, int regnum, void *buffer)
{
  regcache->raw_write (regnum, (const gdb_byte *) buffer);

  return REG_VALID;
}

static enum register_status
e500_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int ev_reg, gdb_byte *buffer)
{
  struct gdbarch *arch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index;
  enum register_status status;

  gdb_assert (IS_SPE_PSEUDOREG (tdep, ev_reg));

  reg_index = ev_reg - tdep->ppc_ev0_regnum;

  if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG)
    {
      status = regcache->raw_read (tdep->ppc_ev0_upper_regnum + reg_index,
				   buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (tdep->ppc_gp0_regnum + reg_index,
				     buffer + 4);
    }
  else
    {
      status = regcache->raw_read (tdep->ppc_gp0_regnum + reg_index, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (tdep->ppc_ev0_upper_regnum + reg_index,
				     buffer + 4);
    }

  return status;

}

static void
e500_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int reg_nr, const gdb_byte *buffer)
{
  e500_move_ev_register (do_regcache_raw_write, regcache,
			 reg_nr, (void *) buffer);
}

/* Read method for DFP pseudo-registers.  */
static enum register_status
dfp_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int reg_nr, gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, fp0;
  enum register_status status;

  if (IS_DFP_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_dl0_regnum;
      fp0 = PPC_F0_REGNUM;
    }
  else
    {
      gdb_assert (IS_CDFP_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cdl0_regnum;
      fp0 = PPC_CF0_REGNUM;
    }

  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    {
      /* Read two FP registers to form a whole dl register.  */
      status = regcache->raw_read (fp0 + 2 * reg_index, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (fp0 + 2 * reg_index + 1,
				     buffer + 8);
    }
  else
    {
      status = regcache->raw_read (fp0 + 2 * reg_index + 1, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (fp0 + 2 * reg_index, buffer + 8);
    }

  return status;
}

/* Write method for DFP pseudo-registers.  */
static void
dfp_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int reg_nr, const gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, fp0;

  if (IS_DFP_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_dl0_regnum;
      fp0 = PPC_F0_REGNUM;
    }
  else
    {
      gdb_assert (IS_CDFP_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cdl0_regnum;
      fp0 = PPC_CF0_REGNUM;
    }

  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    {
      /* Write each half of the dl register into a separate
	 FP register.  */
      regcache->raw_write (fp0 + 2 * reg_index, buffer);
      regcache->raw_write (fp0 + 2 * reg_index + 1, buffer + 8);
    }
  else
    {
      regcache->raw_write (fp0 + 2 * reg_index + 1, buffer);
      regcache->raw_write (fp0 + 2 * reg_index, buffer + 8);
    }
}

/* Read method for the vX aliases for the raw vrX registers.  */

static enum register_status
v_alias_pseudo_register_read (struct gdbarch *gdbarch,
			      readable_regcache *regcache, int reg_nr,
			      gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  gdb_assert (IS_V_ALIAS_PSEUDOREG (tdep, reg_nr));

  return regcache->raw_read (tdep->ppc_vr0_regnum
			     + (reg_nr - tdep->ppc_v0_alias_regnum),
			     buffer);
}

/* Write method for the vX aliases for the raw vrX registers.  */

static void
v_alias_pseudo_register_write (struct gdbarch *gdbarch,
			       struct regcache *regcache,
			       int reg_nr, const gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  gdb_assert (IS_V_ALIAS_PSEUDOREG (tdep, reg_nr));

  regcache->raw_write (tdep->ppc_vr0_regnum
		       + (reg_nr - tdep->ppc_v0_alias_regnum), buffer);
}

/* Read method for POWER7 VSX pseudo-registers.  */
static enum register_status
vsx_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int reg_nr, gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, vr0, fp0, vsr0_upper;
  enum register_status status;

  if (IS_VSX_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_vsr0_regnum;
      vr0 = PPC_VR0_REGNUM;
      fp0 = PPC_F0_REGNUM;
      vsr0_upper = PPC_VSR0_UPPER_REGNUM;
    }
  else
    {
      gdb_assert (IS_CVSX_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cvsr0_regnum;
      vr0 = PPC_CVR0_REGNUM;
      fp0 = PPC_CF0_REGNUM;
      vsr0_upper = PPC_CVSR0_UPPER_REGNUM;
    }

  /* Read the portion that overlaps the VMX registers.  */
  if (reg_index > 31)
    status = regcache->raw_read (vr0 + reg_index - 32, buffer);
  else
    /* Read the portion that overlaps the FPR registers.  */
    if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
      {
	status = regcache->raw_read (fp0 + reg_index, buffer);
	if (status == REG_VALID)
	  status = regcache->raw_read (vsr0_upper + reg_index,
				       buffer + 8);
      }
    else
      {
	status = regcache->raw_read (fp0 + reg_index, buffer + 8);
	if (status == REG_VALID)
	  status = regcache->raw_read (vsr0_upper + reg_index, buffer);
      }

  return status;
}

/* Write method for POWER7 VSX pseudo-registers.  */
static void
vsx_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int reg_nr, const gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, vr0, fp0, vsr0_upper;

  if (IS_VSX_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_vsr0_regnum;
      vr0 = PPC_VR0_REGNUM;
      fp0 = PPC_F0_REGNUM;
      vsr0_upper = PPC_VSR0_UPPER_REGNUM;
    }
  else
    {
      gdb_assert (IS_CVSX_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cvsr0_regnum;
      vr0 = PPC_CVR0_REGNUM;
      fp0 = PPC_CF0_REGNUM;
      vsr0_upper = PPC_CVSR0_UPPER_REGNUM;
    }

  /* Write the portion that overlaps the VMX registers.  */
  if (reg_index > 31)
    regcache->raw_write (vr0 + reg_index - 32, buffer);
  else
    /* Write the portion that overlaps the FPR registers.  */
    if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
      {
	regcache->raw_write (fp0 + reg_index, buffer);
	regcache->raw_write (vsr0_upper + reg_index, buffer + 8);
      }
    else
      {
	regcache->raw_write (fp0 + reg_index, buffer + 8);
	regcache->raw_write (vsr0_upper + reg_index, buffer);
      }
}

/* Read method for POWER7 Extended FP pseudo-registers.  */
static enum register_status
efp_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int reg_nr, gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, vr0;

  if (IS_EFP_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_efpr0_regnum;
      vr0 = PPC_VR0_REGNUM;
    }
  else
    {
      gdb_assert (IS_CEFP_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cefpr0_regnum;
      vr0 = PPC_CVR0_REGNUM;
    }

  int offset = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 0 : 8;

  /* Read the portion that overlaps the VMX register.  */
  return regcache->raw_read_part (vr0 + reg_index, offset,
				  register_size (gdbarch, reg_nr),
				  buffer);
}

/* Write method for POWER7 Extended FP pseudo-registers.  */
static void
efp_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int reg_nr, const gdb_byte *buffer)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, vr0;
  int offset = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 0 : 8;

  if (IS_EFP_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_efpr0_regnum;
      vr0 = PPC_VR0_REGNUM;
    }
  else
    {
      gdb_assert (IS_CEFP_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cefpr0_regnum;
      vr0 = PPC_CVR0_REGNUM;

      /* The call to raw_write_part fails silently if the initial read
	 of the read-update-write sequence returns an invalid status,
	 so we check this manually and throw an error if needed.  */
      regcache->raw_update (vr0 + reg_index);
      if (regcache->get_register_status (vr0 + reg_index) != REG_VALID)
	error (_("Cannot write to the checkpointed EFP register, "
		 "the corresponding vector register is unavailable."));
    }

  /* Write the portion that overlaps the VMX register.  */
  regcache->raw_write_part (vr0 + reg_index, offset,
			    register_size (gdbarch, reg_nr), buffer);
}

static enum register_status
rs6000_pseudo_register_read (struct gdbarch *gdbarch,
			     readable_regcache *regcache,
			     int reg_nr, gdb_byte *buffer)
{
  struct gdbarch *regcache_arch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  gdb_assert (regcache_arch == gdbarch);

  if (IS_SPE_PSEUDOREG (tdep, reg_nr))
    return e500_pseudo_register_read (gdbarch, regcache, reg_nr, buffer);
  else if (IS_DFP_PSEUDOREG (tdep, reg_nr)
	   || IS_CDFP_PSEUDOREG (tdep, reg_nr))
    return dfp_pseudo_register_read (gdbarch, regcache, reg_nr, buffer);
  else if (IS_V_ALIAS_PSEUDOREG (tdep, reg_nr))
    return v_alias_pseudo_register_read (gdbarch, regcache, reg_nr,
					 buffer);
  else if (IS_VSX_PSEUDOREG (tdep, reg_nr)
	   || IS_CVSX_PSEUDOREG (tdep, reg_nr))
    return vsx_pseudo_register_read (gdbarch, regcache, reg_nr, buffer);
  else if (IS_EFP_PSEUDOREG (tdep, reg_nr)
	   || IS_CEFP_PSEUDOREG (tdep, reg_nr))
    return efp_pseudo_register_read (gdbarch, regcache, reg_nr, buffer);
  else
    internal_error (_("rs6000_pseudo_register_read: "
		    "called on unexpected register '%s' (%d)"),
		    gdbarch_register_name (gdbarch, reg_nr), reg_nr);
}

static void
rs6000_pseudo_register_write (struct gdbarch *gdbarch,
			      struct regcache *regcache,
			      int reg_nr, const gdb_byte *buffer)
{
  struct gdbarch *regcache_arch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  gdb_assert (regcache_arch == gdbarch);

  if (IS_SPE_PSEUDOREG (tdep, reg_nr))
    e500_pseudo_register_write (gdbarch, regcache, reg_nr, buffer);
  else if (IS_DFP_PSEUDOREG (tdep, reg_nr)
	   || IS_CDFP_PSEUDOREG (tdep, reg_nr))
    dfp_pseudo_register_write (gdbarch, regcache, reg_nr, buffer);
  else if (IS_V_ALIAS_PSEUDOREG (tdep, reg_nr))
    v_alias_pseudo_register_write (gdbarch, regcache, reg_nr, buffer);
  else if (IS_VSX_PSEUDOREG (tdep, reg_nr)
	   || IS_CVSX_PSEUDOREG (tdep, reg_nr))
    vsx_pseudo_register_write (gdbarch, regcache, reg_nr, buffer);
  else if (IS_EFP_PSEUDOREG (tdep, reg_nr)
	   || IS_CEFP_PSEUDOREG (tdep, reg_nr))
    efp_pseudo_register_write (gdbarch, regcache, reg_nr, buffer);
  else
    internal_error (_("rs6000_pseudo_register_write: "
		    "called on unexpected register '%s' (%d)"),
		    gdbarch_register_name (gdbarch, reg_nr), reg_nr);
}

/* Set the register mask in AX with the registers that form the DFP or
   checkpointed DFP pseudo-register REG_NR.  */

static void
dfp_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				struct agent_expr *ax, int reg_nr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, fp0;

  if (IS_DFP_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_dl0_regnum;
      fp0 = PPC_F0_REGNUM;
    }
  else
    {
      gdb_assert (IS_CDFP_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cdl0_regnum;
      fp0 = PPC_CF0_REGNUM;
    }

  ax_reg_mask (ax, fp0 + 2 * reg_index);
  ax_reg_mask (ax, fp0 + 2 * reg_index + 1);
}

/* Set the register mask in AX with the raw vector register that
   corresponds to its REG_NR alias.  */

static void
v_alias_pseudo_register_collect (struct gdbarch *gdbarch,
				 struct agent_expr *ax, int reg_nr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  gdb_assert (IS_V_ALIAS_PSEUDOREG (tdep, reg_nr));

  ax_reg_mask (ax, tdep->ppc_vr0_regnum
	       + (reg_nr - tdep->ppc_v0_alias_regnum));
}

/* Set the register mask in AX with the registers that form the VSX or
   checkpointed VSX pseudo-register REG_NR.  */

static void
vsx_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				struct agent_expr *ax, int reg_nr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, vr0, fp0, vsr0_upper;

  if (IS_VSX_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_vsr0_regnum;
      vr0 = PPC_VR0_REGNUM;
      fp0 = PPC_F0_REGNUM;
      vsr0_upper = PPC_VSR0_UPPER_REGNUM;
    }
  else
    {
      gdb_assert (IS_CVSX_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cvsr0_regnum;
      vr0 = PPC_CVR0_REGNUM;
      fp0 = PPC_CF0_REGNUM;
      vsr0_upper = PPC_CVSR0_UPPER_REGNUM;
    }

  if (reg_index > 31)
    {
      ax_reg_mask (ax, vr0 + reg_index - 32);
    }
  else
    {
      ax_reg_mask (ax, fp0 + reg_index);
      ax_reg_mask (ax, vsr0_upper + reg_index);
    }
}

/* Set the register mask in AX with the register that corresponds to
   the EFP or checkpointed EFP pseudo-register REG_NR.  */

static void
efp_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				struct agent_expr *ax, int reg_nr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int reg_index, vr0;

  if (IS_EFP_PSEUDOREG (tdep, reg_nr))
    {
      reg_index = reg_nr - tdep->ppc_efpr0_regnum;
      vr0 = PPC_VR0_REGNUM;
    }
  else
    {
      gdb_assert (IS_CEFP_PSEUDOREG (tdep, reg_nr));

      reg_index = reg_nr - tdep->ppc_cefpr0_regnum;
      vr0 = PPC_CVR0_REGNUM;
    }

  ax_reg_mask (ax, vr0 + reg_index);
}

static int
rs6000_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				   struct agent_expr *ax, int reg_nr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  if (IS_SPE_PSEUDOREG (tdep, reg_nr))
    {
      int reg_index = reg_nr - tdep->ppc_ev0_regnum;
      ax_reg_mask (ax, tdep->ppc_gp0_regnum + reg_index);
      ax_reg_mask (ax, tdep->ppc_ev0_upper_regnum + reg_index);
    }
  else if (IS_DFP_PSEUDOREG (tdep, reg_nr)
	   || IS_CDFP_PSEUDOREG (tdep, reg_nr))
    {
      dfp_ax_pseudo_register_collect (gdbarch, ax, reg_nr);
    }
  else if (IS_V_ALIAS_PSEUDOREG (tdep, reg_nr))
    {
      v_alias_pseudo_register_collect (gdbarch, ax, reg_nr);
    }
  else if (IS_VSX_PSEUDOREG (tdep, reg_nr)
	   || IS_CVSX_PSEUDOREG (tdep, reg_nr))
    {
      vsx_ax_pseudo_register_collect (gdbarch, ax, reg_nr);
    }
  else if (IS_EFP_PSEUDOREG (tdep, reg_nr)
	   || IS_CEFP_PSEUDOREG (tdep, reg_nr))
    {
      efp_ax_pseudo_register_collect (gdbarch, ax, reg_nr);
    }
  else
    internal_error (_("rs6000_pseudo_register_collect: "
		    "called on unexpected register '%s' (%d)"),
		    gdbarch_register_name (gdbarch, reg_nr), reg_nr);
  return 0;
}


static void
rs6000_gen_return_address (struct gdbarch *gdbarch,
			   struct agent_expr *ax, struct axs_value *value,
			   CORE_ADDR scope)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  value->type = register_type (gdbarch, tdep->ppc_lr_regnum);
  value->kind = axs_lvalue_register;
  value->u.reg = tdep->ppc_lr_regnum;
}


/* Convert a DBX STABS register number to a GDB register number.  */
static int
rs6000_stab_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (0 <= num && num <= 31)
    return tdep->ppc_gp0_regnum + num;
  else if (32 <= num && num <= 63)
    /* FIXME: jimb/2004-05-05: What should we do when the debug info
       specifies registers the architecture doesn't have?  Our
       callers don't check the value we return.  */
    return tdep->ppc_fp0_regnum + (num - 32);
  else if (77 <= num && num <= 108)
    return tdep->ppc_vr0_regnum + (num - 77);
  else if (1200 <= num && num < 1200 + 32)
    return tdep->ppc_ev0_upper_regnum + (num - 1200);
  else
    switch (num)
      {
      case 64: 
	return tdep->ppc_mq_regnum;
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
      case 111:
	return tdep->ppc_acc_regnum;
      case 112:
	return tdep->ppc_spefscr_regnum;
      default: 
	return num;
      }
}


/* Convert a Dwarf 2 register number to a GDB register number.  */
static int
rs6000_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (0 <= num && num <= 31)
    return tdep->ppc_gp0_regnum + num;
  else if (32 <= num && num <= 63)
    /* FIXME: jimb/2004-05-05: What should we do when the debug info
       specifies registers the architecture doesn't have?  Our
       callers don't check the value we return.  */
    return tdep->ppc_fp0_regnum + (num - 32);
  else if (1124 <= num && num < 1124 + 32)
    return tdep->ppc_vr0_regnum + (num - 1124);
  else if (1200 <= num && num < 1200 + 32)
    return tdep->ppc_ev0_upper_regnum + (num - 1200);
  else
    switch (num)
      {
      case 64:
	return tdep->ppc_cr_regnum;
      case 67:
	return tdep->ppc_vrsave_regnum - 1; /* vscr */
      case 99:
	return tdep->ppc_acc_regnum;
      case 100:
	return tdep->ppc_mq_regnum;
      case 101:
	return tdep->ppc_xer_regnum;
      case 108:
	return tdep->ppc_lr_regnum;
      case 109:
	return tdep->ppc_ctr_regnum;
      case 356:
	return tdep->ppc_vrsave_regnum;
      case 612:
	return tdep->ppc_spefscr_regnum;
      }

  /* Unknown DWARF register number.  */
  return -1;
}

/* Translate a .eh_frame register to DWARF register, or adjust a
   .debug_frame register.  */

static int
rs6000_adjust_frame_regnum (struct gdbarch *gdbarch, int num, int eh_frame_p)
{
  /* GCC releases before 3.4 use GCC internal register numbering in
     .debug_frame (and .debug_info, et cetera).  The numbering is
     different from the standard SysV numbering for everything except
     for GPRs and FPRs.  We can not detect this problem in most cases
     - to get accurate debug info for variables living in lr, ctr, v0,
     et cetera, use a newer version of GCC.  But we must detect
     one important case - lr is in column 65 in .debug_frame output,
     instead of 108.

     GCC 3.4, and the "hammer" branch, have a related problem.  They
     record lr register saves in .debug_frame as 108, but still record
     the return column as 65.  We fix that up too.

     We can do this because 65 is assigned to fpsr, and GCC never
     generates debug info referring to it.  To add support for
     handwritten debug info that restores fpsr, we would need to add a
     producer version check to this.  */
  if (!eh_frame_p)
    {
      if (num == 65)
	return 108;
      else
	return num;
    }

  /* .eh_frame is GCC specific.  For binary compatibility, it uses GCC
     internal register numbering; translate that to the standard DWARF2
     register numbering.  */
  if (0 <= num && num <= 63)	/* r0-r31,fp0-fp31 */
    return num;
  else if (68 <= num && num <= 75) /* cr0-cr8 */
    return num - 68 + 86;
  else if (77 <= num && num <= 108) /* vr0-vr31 */
    return num - 77 + 1124;
  else
    switch (num)
      {
      case 64: /* mq */
	return 100;
      case 65: /* lr */
	return 108;
      case 66: /* ctr */
	return 109;
      case 76: /* xer */
	return 101;
      case 109: /* vrsave */
	return 356;
      case 110: /* vscr */
	return 67;
      case 111: /* spe_acc */
	return 99;
      case 112: /* spefscr */
	return 612;
      default:
	return num;
      }
}


/* Handling the various POWER/PowerPC variants.  */

/* Information about a particular processor variant.  */

struct ppc_variant
  {
    /* Name of this variant.  */
    const char *name;

    /* English description of the variant.  */
    const char *description;

    /* bfd_arch_info.arch corresponding to variant.  */
    enum bfd_architecture arch;

    /* bfd_arch_info.mach corresponding to variant.  */
    unsigned long mach;

    /* Target description for this variant.  */
    const struct target_desc **tdesc;
  };

static struct ppc_variant variants[] =
{
  {"powerpc", "PowerPC user-level", bfd_arch_powerpc,
   bfd_mach_ppc, &tdesc_powerpc_altivec32},
  {"power", "POWER user-level", bfd_arch_rs6000,
   bfd_mach_rs6k, &tdesc_rs6000},
  {"403", "IBM PowerPC 403", bfd_arch_powerpc,
   bfd_mach_ppc_403, &tdesc_powerpc_403},
  {"405", "IBM PowerPC 405", bfd_arch_powerpc,
   bfd_mach_ppc_405, &tdesc_powerpc_405},
  {"601", "Motorola PowerPC 601", bfd_arch_powerpc,
   bfd_mach_ppc_601, &tdesc_powerpc_601},
  {"602", "Motorola PowerPC 602", bfd_arch_powerpc,
   bfd_mach_ppc_602, &tdesc_powerpc_602},
  {"603", "Motorola/IBM PowerPC 603 or 603e", bfd_arch_powerpc,
   bfd_mach_ppc_603, &tdesc_powerpc_603},
  {"604", "Motorola PowerPC 604 or 604e", bfd_arch_powerpc,
   604, &tdesc_powerpc_604},
  {"403GC", "IBM PowerPC 403GC", bfd_arch_powerpc,
   bfd_mach_ppc_403gc, &tdesc_powerpc_403gc},
  {"505", "Motorola PowerPC 505", bfd_arch_powerpc,
   bfd_mach_ppc_505, &tdesc_powerpc_505},
  {"860", "Motorola PowerPC 860 or 850", bfd_arch_powerpc,
   bfd_mach_ppc_860, &tdesc_powerpc_860},
  {"750", "Motorola/IBM PowerPC 750 or 740", bfd_arch_powerpc,
   bfd_mach_ppc_750, &tdesc_powerpc_750},
  {"7400", "Motorola/IBM PowerPC 7400 (G4)", bfd_arch_powerpc,
   bfd_mach_ppc_7400, &tdesc_powerpc_7400},
  {"e500", "Motorola PowerPC e500", bfd_arch_powerpc,
   bfd_mach_ppc_e500, &tdesc_powerpc_e500},

  /* 64-bit */
  {"powerpc64", "PowerPC 64-bit user-level", bfd_arch_powerpc,
   bfd_mach_ppc64, &tdesc_powerpc_altivec64},
  {"620", "Motorola PowerPC 620", bfd_arch_powerpc,
   bfd_mach_ppc_620, &tdesc_powerpc_64},
  {"630", "Motorola PowerPC 630", bfd_arch_powerpc,
   bfd_mach_ppc_630, &tdesc_powerpc_64},
  {"a35", "PowerPC A35", bfd_arch_powerpc,
   bfd_mach_ppc_a35, &tdesc_powerpc_64},
  {"rs64ii", "PowerPC rs64ii", bfd_arch_powerpc,
   bfd_mach_ppc_rs64ii, &tdesc_powerpc_64},
  {"rs64iii", "PowerPC rs64iii", bfd_arch_powerpc,
   bfd_mach_ppc_rs64iii, &tdesc_powerpc_64},

  /* FIXME: I haven't checked the register sets of the following.  */
  {"rs1", "IBM POWER RS1", bfd_arch_rs6000,
   bfd_mach_rs6k_rs1, &tdesc_rs6000},
  {"rsc", "IBM POWER RSC", bfd_arch_rs6000,
   bfd_mach_rs6k_rsc, &tdesc_rs6000},
  {"rs2", "IBM POWER RS2", bfd_arch_rs6000,
   bfd_mach_rs6k_rs2, &tdesc_rs6000},

  {0, 0, (enum bfd_architecture) 0, 0, 0}
};

/* Return the variant corresponding to architecture ARCH and machine number
   MACH.  If no such variant exists, return null.  */

static const struct ppc_variant *
find_variant_by_arch (enum bfd_architecture arch, unsigned long mach)
{
  const struct ppc_variant *v;

  for (v = variants; v->name; v++)
    if (arch == v->arch && mach == v->mach)
      return v;

  return NULL;
}



struct rs6000_frame_cache
{
  CORE_ADDR base;
  CORE_ADDR initial_sp;
  trad_frame_saved_reg *saved_regs;

  /* Set BASE_P to true if this frame cache is properly initialized.
     Otherwise set to false because some registers or memory cannot
     collected.  */
  int base_p;
  /* Cache PC for building unavailable frame.  */
  CORE_ADDR pc;
};

static struct rs6000_frame_cache *
rs6000_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct rs6000_frame_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct rs6000_framedata fdata;
  int wordsize = tdep->wordsize;
  CORE_ADDR func = 0, pc = 0;

  if ((*this_cache) != NULL)
    return (struct rs6000_frame_cache *) (*this_cache);
  cache = FRAME_OBSTACK_ZALLOC (struct rs6000_frame_cache);
  (*this_cache) = cache;
  cache->pc = 0;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  try
    {
      func = get_frame_func (this_frame);
      cache->pc = func;
      pc = get_frame_pc (this_frame);
      skip_prologue (gdbarch, func, pc, &fdata);

      /* Figure out the parent's stack pointer.  */

      /* NOTE: cagney/2002-04-14: The ->frame points to the inner-most
	 address of the current frame.  Things might be easier if the
	 ->frame pointed to the outer-most address of the frame.  In
	 the mean time, the address of the prev frame is used as the
	 base address of this frame.  */
      cache->base = get_frame_register_unsigned
	(this_frame, gdbarch_sp_regnum (gdbarch));
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
      return (struct rs6000_frame_cache *) (*this_cache);
    }

  /* If the function appears to be frameless, check a couple of likely
     indicators that we have simply failed to find the frame setup.
     Two common cases of this are missing symbols (i.e.
     get_frame_func returns the wrong address or 0), and assembly
     stubs which have a fast exit path but set up a frame on the slow
     path.

     If the LR appears to return to this function, then presume that
     we have an ABI compliant frame that we failed to find.  */
  if (fdata.frameless && fdata.lr_offset == 0)
    {
      CORE_ADDR saved_lr;
      int make_frame = 0;

      saved_lr = get_frame_register_unsigned (this_frame, tdep->ppc_lr_regnum);
      if (func == 0 && saved_lr == pc)
	make_frame = 1;
      else if (func != 0)
	{
	  CORE_ADDR saved_func = get_pc_function_start (saved_lr);
	  if (func == saved_func)
	    make_frame = 1;
	}

      if (make_frame)
	{
	  fdata.frameless = 0;
	  fdata.lr_offset = tdep->lr_frame_offset;
	}
    }

  if (!fdata.frameless)
    {
      /* Frameless really means stackless.  */
      ULONGEST backchain;

      if (safe_read_memory_unsigned_integer (cache->base, wordsize,
					     byte_order, &backchain))
	cache->base = (CORE_ADDR) backchain;
    }

  cache->saved_regs[gdbarch_sp_regnum (gdbarch)].set_value (cache->base);

  /* if != -1, fdata.saved_fpr is the smallest number of saved_fpr.
     All fpr's from saved_fpr to fp31 are saved.  */

  if (fdata.saved_fpr >= 0)
    {
      int i;
      CORE_ADDR fpr_addr = cache->base + fdata.fpr_offset;

      /* If skip_prologue says floating-point registers were saved,
	 but the current architecture has no floating-point registers,
	 then that's strange.  But we have no indices to even record
	 the addresses under, so we just ignore it.  */
      if (ppc_floating_point_unit_p (gdbarch))
	for (i = fdata.saved_fpr; i < ppc_num_fprs; i++)
	  {
	    cache->saved_regs[tdep->ppc_fp0_regnum + i].set_addr (fpr_addr);
	    fpr_addr += 8;
	  }
    }

  /* if != -1, fdata.saved_gpr is the smallest number of saved_gpr.
     All gpr's from saved_gpr to gpr31 are saved (except during the
     prologue).  */

  if (fdata.saved_gpr >= 0)
    {
      int i;
      CORE_ADDR gpr_addr = cache->base + fdata.gpr_offset;
      for (i = fdata.saved_gpr; i < ppc_num_gprs; i++)
	{
	  if (fdata.gpr_mask & (1U << i))
	    cache->saved_regs[tdep->ppc_gp0_regnum + i].set_addr (gpr_addr);
	  gpr_addr += wordsize;
	}
    }

  /* if != -1, fdata.saved_vr is the smallest number of saved_vr.
     All vr's from saved_vr to vr31 are saved.  */
  if (tdep->ppc_vr0_regnum != -1 && tdep->ppc_vrsave_regnum != -1)
    {
      if (fdata.saved_vr >= 0)
	{
	  int i;
	  CORE_ADDR vr_addr = cache->base + fdata.vr_offset;
	  for (i = fdata.saved_vr; i < 32; i++)
	    {
	      cache->saved_regs[tdep->ppc_vr0_regnum + i].set_addr (vr_addr);
	      vr_addr += register_size (gdbarch, tdep->ppc_vr0_regnum);
	    }
	}
    }

  /* if != -1, fdata.saved_ev is the smallest number of saved_ev.
     All vr's from saved_ev to ev31 are saved. ?????  */
  if (tdep->ppc_ev0_regnum != -1)
    {
      if (fdata.saved_ev >= 0)
	{
	  int i;
	  CORE_ADDR ev_addr = cache->base + fdata.ev_offset;
	  CORE_ADDR off = (byte_order == BFD_ENDIAN_BIG ? 4 : 0);

	  for (i = fdata.saved_ev; i < ppc_num_gprs; i++)
	    {
	      cache->saved_regs[tdep->ppc_ev0_regnum + i].set_addr (ev_addr);
	      cache->saved_regs[tdep->ppc_gp0_regnum + i].set_addr (ev_addr
								    + off);
	      ev_addr += register_size (gdbarch, tdep->ppc_ev0_regnum);
	    }
	}
    }

  /* If != 0, fdata.cr_offset is the offset from the frame that
     holds the CR.  */
  if (fdata.cr_offset != 0)
    cache->saved_regs[tdep->ppc_cr_regnum].set_addr (cache->base
						     + fdata.cr_offset);

  /* If != 0, fdata.lr_offset is the offset from the frame that
     holds the LR.  */
  if (fdata.lr_offset != 0)
    cache->saved_regs[tdep->ppc_lr_regnum].set_addr (cache->base
						     + fdata.lr_offset);
  else if (fdata.lr_register != -1)
    cache->saved_regs[tdep->ppc_lr_regnum].set_realreg (fdata.lr_register);
  /* The PC is found in the link register.  */
  cache->saved_regs[gdbarch_pc_regnum (gdbarch)] =
    cache->saved_regs[tdep->ppc_lr_regnum];

  /* If != 0, fdata.vrsave_offset is the offset from the frame that
     holds the VRSAVE.  */
  if (fdata.vrsave_offset != 0)
    cache->saved_regs[tdep->ppc_vrsave_regnum].set_addr (cache->base
							 + fdata.vrsave_offset);

  if (fdata.alloca_reg < 0)
    /* If no alloca register used, then fi->frame is the value of the
       %sp for this frame, and it is good enough.  */
    cache->initial_sp
      = get_frame_register_unsigned (this_frame, gdbarch_sp_regnum (gdbarch));
  else
    cache->initial_sp
      = get_frame_register_unsigned (this_frame, fdata.alloca_reg);

  cache->base_p = 1;
  return cache;
}

static void
rs6000_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		      struct frame_id *this_id)
{
  struct rs6000_frame_cache *info = rs6000_frame_cache (this_frame,
							this_cache);

  if (!info->base_p)
    {
      (*this_id) = frame_id_build_unavailable_stack (info->pc);
      return;
    }

  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;

  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
rs6000_frame_prev_register (frame_info_ptr this_frame,
			    void **this_cache, int regnum)
{
  struct rs6000_frame_cache *info = rs6000_frame_cache (this_frame,
							this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static const struct frame_unwind rs6000_frame_unwind =
{
  "rs6000 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  rs6000_frame_this_id,
  rs6000_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Allocate and initialize a frame cache for an epilogue frame.
   SP is restored and prev-PC is stored in LR.  */

static struct rs6000_frame_cache *
rs6000_epilogue_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct rs6000_frame_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (*this_cache)
    return (struct rs6000_frame_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct rs6000_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  try
    {
      /* At this point the stack looks as if we just entered the
	 function, and the return address is stored in LR.  */
      CORE_ADDR sp, lr;

      sp = get_frame_register_unsigned (this_frame, gdbarch_sp_regnum (gdbarch));
      lr = get_frame_register_unsigned (this_frame, tdep->ppc_lr_regnum);

      cache->base = sp;
      cache->initial_sp = sp;

      cache->saved_regs[gdbarch_pc_regnum (gdbarch)].set_value (lr);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  return cache;
}

/* Implementation of frame_unwind.this_id, as defined in frame_unwind.h.
   Return the frame ID of an epilogue frame.  */

static void
rs6000_epilogue_frame_this_id (frame_info_ptr this_frame,
			       void **this_cache, struct frame_id *this_id)
{
  CORE_ADDR pc;
  struct rs6000_frame_cache *info =
    rs6000_epilogue_frame_cache (this_frame, this_cache);

  pc = get_frame_func (this_frame);
  if (info->base == 0)
    (*this_id) = frame_id_build_unavailable_stack (pc);
  else
    (*this_id) = frame_id_build (info->base, pc);
}

/* Implementation of frame_unwind.prev_register, as defined in frame_unwind.h.
   Return the register value of REGNUM in previous frame.  */

static struct value *
rs6000_epilogue_frame_prev_register (frame_info_ptr this_frame,
				     void **this_cache, int regnum)
{
  struct rs6000_frame_cache *info =
    rs6000_epilogue_frame_cache (this_frame, this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

/* Implementation of frame_unwind.sniffer, as defined in frame_unwind.h.
   Check whether this an epilogue frame.  */

static int
rs6000_epilogue_frame_sniffer (const struct frame_unwind *self,
			       frame_info_ptr this_frame,
			       void **this_prologue_cache)
{
  if (frame_relative_level (this_frame) == 0)
    return rs6000_in_function_epilogue_frame_p (this_frame,
						get_frame_arch (this_frame),
						get_frame_pc (this_frame));
  else
    return 0;
}

/* Frame unwinder for epilogue frame.  This is required for reverse step-over
   a function without debug information.  */

static const struct frame_unwind rs6000_epilogue_frame_unwind =
{
  "rs6000 epilogue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  rs6000_epilogue_frame_this_id, rs6000_epilogue_frame_prev_register,
  NULL,
  rs6000_epilogue_frame_sniffer
};


static CORE_ADDR
rs6000_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct rs6000_frame_cache *info = rs6000_frame_cache (this_frame,
							this_cache);
  return info->initial_sp;
}

static const struct frame_base rs6000_frame_base = {
  &rs6000_frame_unwind,
  rs6000_frame_base_address,
  rs6000_frame_base_address,
  rs6000_frame_base_address
};

static const struct frame_base *
rs6000_frame_base_sniffer (frame_info_ptr this_frame)
{
  return &rs6000_frame_base;
}

/* DWARF-2 frame support.  Used to handle the detection of
  clobbered registers during function calls.  */

static void
ppc_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			    struct dwarf2_frame_state_reg *reg,
			    frame_info_ptr this_frame)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  /* PPC32 and PPC64 ABI's are the same regarding volatile and
     non-volatile registers.  We will use the same code for both.  */

  /* Call-saved GP registers.  */
  if ((regnum >= tdep->ppc_gp0_regnum + 14
      && regnum <= tdep->ppc_gp0_regnum + 31)
      || (regnum == tdep->ppc_gp0_regnum + 1))
    reg->how = DWARF2_FRAME_REG_SAME_VALUE;

  /* Call-clobbered GP registers.  */
  if ((regnum >= tdep->ppc_gp0_regnum + 3
      && regnum <= tdep->ppc_gp0_regnum + 12)
      || (regnum == tdep->ppc_gp0_regnum))
    reg->how = DWARF2_FRAME_REG_UNDEFINED;

  /* Deal with FP registers, if supported.  */
  if (tdep->ppc_fp0_regnum >= 0)
    {
      /* Call-saved FP registers.  */
      if ((regnum >= tdep->ppc_fp0_regnum + 14
	  && regnum <= tdep->ppc_fp0_regnum + 31))
	reg->how = DWARF2_FRAME_REG_SAME_VALUE;

      /* Call-clobbered FP registers.  */
      if ((regnum >= tdep->ppc_fp0_regnum
	  && regnum <= tdep->ppc_fp0_regnum + 13))
	reg->how = DWARF2_FRAME_REG_UNDEFINED;
    }

  /* Deal with ALTIVEC registers, if supported.  */
  if (tdep->ppc_vr0_regnum > 0 && tdep->ppc_vrsave_regnum > 0)
    {
      /* Call-saved Altivec registers.  */
      if ((regnum >= tdep->ppc_vr0_regnum + 20
	  && regnum <= tdep->ppc_vr0_regnum + 31)
	  || regnum == tdep->ppc_vrsave_regnum)
	reg->how = DWARF2_FRAME_REG_SAME_VALUE;

      /* Call-clobbered Altivec registers.  */
      if ((regnum >= tdep->ppc_vr0_regnum
	  && regnum <= tdep->ppc_vr0_regnum + 19))
	reg->how = DWARF2_FRAME_REG_UNDEFINED;
    }

  /* Handle PC register and Stack Pointer correctly.  */
  if (regnum == gdbarch_pc_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_CFA;
}


/* Return true if a .gnu_attributes section exists in BFD and it
   indicates we are using SPE extensions OR if a .PPC.EMB.apuinfo
   section exists in BFD and it indicates that SPE extensions are in
   use.  Check the .gnu.attributes section first, as the binary might be
   compiled for SPE, but not actually using SPE instructions.  */

static int
bfd_uses_spe_extensions (bfd *abfd)
{
  asection *sect;
  gdb_byte *contents = NULL;
  bfd_size_type size;
  gdb_byte *ptr;
  int success = 0;

  if (!abfd)
    return 0;

#ifdef HAVE_ELF
  /* Using Tag_GNU_Power_ABI_Vector here is a bit of a hack, as the user
     could be using the SPE vector abi without actually using any spe
     bits whatsoever.  But it's close enough for now.  */
  int vector_abi = bfd_elf_get_obj_attr_int (abfd, OBJ_ATTR_GNU,
					     Tag_GNU_Power_ABI_Vector);
  if (vector_abi == 3)
    return 1;
#endif

  sect = bfd_get_section_by_name (abfd, ".PPC.EMB.apuinfo");
  if (!sect)
    return 0;

  size = bfd_section_size (sect);
  contents = (gdb_byte *) xmalloc (size);
  if (!bfd_get_section_contents (abfd, sect, contents, 0, size))
    {
      xfree (contents);
      return 0;
    }

  /* Parse the .PPC.EMB.apuinfo section.  The layout is as follows:

     struct {
       uint32 name_len;
       uint32 data_len;
       uint32 type;
       char name[name_len rounded up to 4-byte alignment];
       char data[data_len];
     };

     Technically, there's only supposed to be one such structure in a
     given apuinfo section, but the linker is not always vigilant about
     merging apuinfo sections from input files.  Just go ahead and parse
     them all, exiting early when we discover the binary uses SPE
     insns.

     It's not specified in what endianness the information in this
     section is stored.  Assume that it's the endianness of the BFD.  */
  ptr = contents;
  while (1)
    {
      unsigned int name_len;
      unsigned int data_len;
      unsigned int type;

      /* If we can't read the first three fields, we're done.  */
      if (size < 12)
	break;

      name_len = bfd_get_32 (abfd, ptr);
      name_len = (name_len + 3) & ~3U; /* Round to 4 bytes.  */
      data_len = bfd_get_32 (abfd, ptr + 4);
      type = bfd_get_32 (abfd, ptr + 8);
      ptr += 12;

      /* The name must be "APUinfo\0".  */
      if (name_len != 8
	  && strcmp ((const char *) ptr, "APUinfo") != 0)
	break;
      ptr += name_len;

      /* The type must be 2.  */
      if (type != 2)
	break;

      /* The data is stored as a series of uint32.  The upper half of
	 each uint32 indicates the particular APU used and the lower
	 half indicates the revision of that APU.  We just care about
	 the upper half.  */

      /* Not 4-byte quantities.  */
      if (data_len & 3U)
	break;

      while (data_len)
	{
	  unsigned int apuinfo = bfd_get_32 (abfd, ptr);
	  unsigned int apu = apuinfo >> 16;
	  ptr += 4;
	  data_len -= 4;

	  /* The SPE APU is 0x100; the SPEFP APU is 0x101.  Accept
	     either.  */
	  if (apu == 0x100 || apu == 0x101)
	    {
	      success = 1;
	      data_len = 0;
	    }
	}

      if (success)
	break;
    }

  xfree (contents);
  return success;
}

/* These are macros for parsing instruction fields (I.1.6.28)  */

#define PPC_FIELD(value, from, len) \
	(((value) >> (32 - (from) - (len))) & ((1 << (len)) - 1))
#define PPC_SEXT(v, bs) \
	((((CORE_ADDR) (v) & (((CORE_ADDR) 1 << (bs)) - 1)) \
	  ^ ((CORE_ADDR) 1 << ((bs) - 1))) \
	 - ((CORE_ADDR) 1 << ((bs) - 1)))
#define PPC_OP6(insn)	PPC_FIELD (insn, 0, 6)
#define PPC_EXTOP(insn)	PPC_FIELD (insn, 21, 10)
#define PPC_RT(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_RS(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_RA(insn)	PPC_FIELD (insn, 11, 5)
#define PPC_RB(insn)	PPC_FIELD (insn, 16, 5)
#define PPC_NB(insn)	PPC_FIELD (insn, 16, 5)
#define PPC_VRT(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_FRT(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_SPR(insn)	(PPC_FIELD (insn, 11, 5) \
			| (PPC_FIELD (insn, 16, 5) << 5))
#define PPC_BO(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_T(insn)	PPC_FIELD (insn, 6, 5)
#define PPC_D(insn)	PPC_SEXT (PPC_FIELD (insn, 16, 16), 16)
#define PPC_DS(insn)	PPC_SEXT (PPC_FIELD (insn, 16, 14), 14)
#define PPC_DQ(insn)	PPC_SEXT (PPC_FIELD (insn, 16, 12), 12)
#define PPC_BIT(insn,n)	((insn & (1 << (31 - (n)))) ? 1 : 0)
#define PPC_OE(insn)	PPC_BIT (insn, 21)
#define PPC_RC(insn)	PPC_BIT (insn, 31)
#define PPC_Rc(insn)	PPC_BIT (insn, 21)
#define PPC_LK(insn)	PPC_BIT (insn, 31)
#define PPC_TX(insn)	PPC_BIT (insn, 31)
#define PPC_LEV(insn)	PPC_FIELD (insn, 20, 7)

#define PPC_XT(insn)	((PPC_TX (insn) << 5) | PPC_T (insn))
#define PPC_XTp(insn)	((PPC_BIT (insn, 10) << 5)	\
			 | PPC_FIELD (insn, 6, 4) << 1)
#define PPC_XSp(insn)	((PPC_BIT (insn, 10) << 5)	\
			 | PPC_FIELD (insn, 6, 4) << 1)
#define PPC_XER_NB(xer)	(xer & 0x7f)

/* The following macros are for the prefixed instructions.  */
#define P_PPC_D(insn_prefix, insn_suffix) \
  PPC_SEXT (PPC_FIELD (insn_prefix, 14, 18) << 16 \
	    | PPC_FIELD (insn_suffix, 16, 16), 34)
#define P_PPC_TX5(insn_sufix)	PPC_BIT (insn_suffix, 5)
#define P_PPC_TX15(insn_suffix) PPC_BIT (insn_suffix, 15)
#define P_PPC_XT(insn_suffix)	((PPC_TX (insn_suffix) << 5) \
				 | PPC_T (insn_suffix))
#define P_PPC_XT5(insn_suffix) ((P_PPC_TX5 (insn_suffix) << 5) \
				| PPC_T (insn_suffix))
#define P_PPC_XT15(insn_suffix) \
  ((P_PPC_TX15 (insn_suffix) << 5) | PPC_T (insn_suffix))

/* Record Vector-Scalar Registers.
   For VSR less than 32, it's represented by an FPR and an VSR-upper register.
   Otherwise, it's just a VR register.  Record them accordingly.  */

static int
ppc_record_vsr (struct regcache *regcache, ppc_gdbarch_tdep *tdep, int vsr)
{
  if (vsr < 0 || vsr >= 64)
    return -1;

  if (vsr >= 32)
    {
      if (tdep->ppc_vr0_regnum >= 0)
	record_full_arch_list_add_reg (regcache, tdep->ppc_vr0_regnum + vsr - 32);
    }
  else
    {
      if (tdep->ppc_fp0_regnum >= 0)
	record_full_arch_list_add_reg (regcache, tdep->ppc_fp0_regnum + vsr);
      if (tdep->ppc_vsr0_upper_regnum >= 0)
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_vsr0_upper_regnum + vsr);
    }

  return 0;
}

/* The ppc_record_ACC_fpscr() records the changes to the VSR registers
   modified by a floating point instruction.  The ENTRY argument selects which
   of the eight AT entries needs to be recorded.  The boolean SAVE_FPSCR
   argument is set to TRUE to indicate the FPSCR also needs to be recorded.
   The function returns 0 on success.  */

static int
ppc_record_ACC_fpscr (struct regcache *regcache, ppc_gdbarch_tdep *tdep,
		      int entry, bool save_fpscr)
{
  int i;
  if (entry < 0 || entry >= 8)
    return -1;

  /* The ACC register file consists of 8 register entries, each register
     entry consist of four 128-bit rows.

     The ACC rows map to specific VSR registers.
	 ACC[0][0] -> VSR[0]
	 ACC[0][1] -> VSR[1]
	 ACC[0][2] -> VSR[2]
	 ACC[0][3] -> VSR[3]
	      ...
	 ACC[7][0] -> VSR[28]
	 ACC[7][1] -> VSR[29]
	 ACC[7][2] -> VSR[30]
	 ACC[7][3] -> VSR[31]

     NOTE:
     In ISA 3.1 the ACC is mapped on top of VSR[0] thru VSR[31].

     In the future, the ACC may be implemented as an independent register file
     rather than mapping on top of the VSRs.  This will then require the ACC to
     be assigned its own register number and the ptrace interface to be able
     access the ACC.  Note the ptrace interface for the ACC will also need to
     be implemented.  */

  /* ACC maps over the same VSR space as the fp registers.  */
  for (i = 0; i < 4; i++)
    {
      record_full_arch_list_add_reg (regcache, tdep->ppc_fp0_regnum
				     + entry * 4 + i);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vsr0_upper_regnum
				     + entry * 4 + i);
    }

  if (save_fpscr)
    record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);

  return 0;
}

/* Parse and record instructions primary opcode-4 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op4 (struct gdbarch *gdbarch, struct regcache *regcache,
			CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int ext = PPC_FIELD (insn, 21, 11);
  int vra = PPC_FIELD (insn, 11, 5);

  switch (ext & 0x3f)
    {
    case 32:		/* Vector Multiply-High-Add Signed Halfword Saturate */
    case 33:		/* Vector Multiply-High-Round-Add Signed Halfword Saturate */
    case 39:		/* Vector Multiply-Sum Unsigned Halfword Saturate */
    case 41:		/* Vector Multiply-Sum Signed Halfword Saturate */
      record_full_arch_list_add_reg (regcache, PPC_VSCR_REGNUM);
      [[fallthrough]];
    case 20:		/* Move To VSR Byte Mask Immediate opcode, b2 = 0,
			   ignore bit 31 */
    case 21:		/* Move To VSR Byte Mask Immediate opcode, b2 = 1,
			   ignore bit 31 */
    case 23:		/* Vector Multiply-Sum & write Carry-out Unsigned
			   Doubleword */
    case 24:		/* Vector Extract Double Unsigned Byte to VSR
			   using GPR-specified Left-Index */
    case 25:		/* Vector Extract Double Unsigned Byte to VSR
			   using GPR-specified Right-Index */
    case 26:		/* Vector Extract Double Unsigned Halfword to VSR
			   using GPR-specified Left-Index */
    case 27:		/* Vector Extract Double Unsigned Halfword to VSR
			   using GPR-specified Right-Index */
    case 28:		/* Vector Extract Double Unsigned Word to VSR
			   using GPR-specified Left-Index */
    case 29:		/* Vector Extract Double Unsigned Word to VSR
			   using GPR-specified Right-Index */
    case 30:		/* Vector Extract Double Unsigned Doubleword to VSR
			   using GPR-specified Left-Index */
    case 31:		/* Vector Extract Double Unsigned Doubleword to VSR
			   using GPR-specified Right-Index */
    case 42:		/* Vector Select */
    case 43:		/* Vector Permute */
    case 59:		/* Vector Permute Right-indexed */
    case 22: 		/* Vector Shift
			      Left  Double by Bit Immediate if insn[21] = 0
			      Right Double by Bit Immediate if insn[21] = 1 */
    case 44:		/* Vector Shift Left Double by Octet Immediate */
    case 45:		/* Vector Permute and Exclusive-OR */
    case 60:		/* Vector Add Extended Unsigned Quadword Modulo */
    case 61:		/* Vector Add Extended & write Carry Unsigned Quadword */
    case 62:		/* Vector Subtract Extended Unsigned Quadword Modulo */
    case 63:		/* Vector Subtract Extended & write Carry Unsigned Quadword */
    case 34:		/* Vector Multiply-Low-Add Unsigned Halfword Modulo */
    case 35:		/* Vector Multiply-Sum Unsigned Doubleword Modulo */
    case 36:		/* Vector Multiply-Sum Unsigned Byte Modulo */
    case 37:		/* Vector Multiply-Sum Mixed Byte Modulo */
    case 38:		/* Vector Multiply-Sum Unsigned Halfword Modulo */
    case 40:		/* Vector Multiply-Sum Signed Halfword Modulo */
    case 46:		/* Vector Multiply-Add Single-Precision */
    case 47:		/* Vector Negative Multiply-Subtract Single-Precision */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      return 0;

    case 48:		/* Multiply-Add High Doubleword */
    case 49:		/* Multiply-Add High Doubleword Unsigned */
    case 51:		/* Multiply-Add Low Doubleword */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;
    }

  switch ((ext & 0x1ff))
    {
    case 385:
      if (vra != 0	/* Decimal Convert To Signed Quadword */
	  && vra != 2	/* Decimal Convert From Signed Quadword */
	  && vra != 4	/* Decimal Convert To Zoned */
	  && vra != 5	/* Decimal Convert To National */
	  && vra != 6	/* Decimal Convert From Zoned */
	  && vra != 7	/* Decimal Convert From National */
	  && vra != 31)	/* Decimal Set Sign */
	break;
      [[fallthrough]];
			/* 5.16 Decimal Integer Arithmetic Instructions */
    case 1:		/* Decimal Add Modulo */
    case 65:		/* Decimal Subtract Modulo */

    case 193:		/* Decimal Shift */
    case 129:		/* Decimal Unsigned Shift */
    case 449:		/* Decimal Shift and Round */

    case 257:		/* Decimal Truncate */
    case 321:		/* Decimal Unsigned Truncate */

      /* Bit-21 should be set.  */
      if (!PPC_BIT (insn, 21))
	break;

      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;
    }

  /* Bit-21 is used for RC */
  switch (ext & 0x3ff)
    {
    case 5:		/* Vector Rotate Left Quadword */
    case 69:		/* Vector Rotate Left Quadword then Mask Insert */
    case 325:		/* Vector Rotate Left Quadword then AND with Mask */
    case 6:		/* Vector Compare Equal To Unsigned Byte */
    case 70:		/* Vector Compare Equal To Unsigned Halfword */
    case 134:		/* Vector Compare Equal To Unsigned Word */
    case 199:		/* Vector Compare Equal To Unsigned Doubleword */
    case 774:		/* Vector Compare Greater Than Signed Byte */
    case 838:		/* Vector Compare Greater Than Signed Halfword */
    case 902:		/* Vector Compare Greater Than Signed Word */
    case 967:		/* Vector Compare Greater Than Signed Doubleword */
    case 903:		/* Vector Compare Greater Than Signed Quadword */
    case 518:		/* Vector Compare Greater Than Unsigned Byte */
    case 646:		/* Vector Compare Greater Than Unsigned Word */
    case 582:		/* Vector Compare Greater Than Unsigned Halfword */
    case 711:		/* Vector Compare Greater Than Unsigned Doubleword */
    case 647:		/* Vector Compare Greater Than Unsigned Quadword */
    case 966:		/* Vector Compare Bounds Single-Precision */
    case 198:		/* Vector Compare Equal To Single-Precision */
    case 454:		/* Vector Compare Greater Than or Equal To Single-Precision */
    case 455:		/* Vector Compare Equal Quadword */
    case 710:		/* Vector Compare Greater Than Single-Precision */
    case 7:		/* Vector Compare Not Equal Byte */
    case 71:		/* Vector Compare Not Equal Halfword */
    case 135:		/* Vector Compare Not Equal Word */
    case 263:		/* Vector Compare Not Equal or Zero Byte */
    case 327:		/* Vector Compare Not Equal or Zero Halfword */
    case 391:		/* Vector Compare Not Equal or Zero Word */
      if (PPC_Rc (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      return 0;

    case 13:
      switch (vra)    /* Bit-21 is used for RC */
	{
	case 0:       /* Vector String Isolate Byte Left-justified */
	case 1:       /* Vector String Isolate Byte Right-justified */
	case 2:       /* Vector String Isolate Halfword Left-justified */
	case 3:       /* Vector String Isolate Halfword Right-justified */
	  if (PPC_Rc (insn))
	    record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_vr0_regnum
					 + PPC_VRT (insn));
      return 0;
	}
    }

  if (ext  == 1538)
    {
      switch (vra)
	{
	case 0:		/* Vector Count Leading Zero Least-Significant Bits
			   Byte */
	case 1:		/* Vector Count Trailing Zero Least-Significant Bits
			   Byte */
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_gp0_regnum + PPC_RT (insn));
	  return 0;

	case 6:		/* Vector Negate Word */
	case 7:		/* Vector Negate Doubleword */
	case 8:		/* Vector Parity Byte Word */
	case 9:		/* Vector Parity Byte Doubleword */
	case 10:	/* Vector Parity Byte Quadword */
	case 16:	/* Vector Extend Sign Byte To Word */
	case 17:	/* Vector Extend Sign Halfword To Word */
	case 24:	/* Vector Extend Sign Byte To Doubleword */
	case 25:	/* Vector Extend Sign Halfword To Doubleword */
	case 26:	/* Vector Extend Sign Word To Doubleword */
	case 27:	/* Vector Extend Sign Doubleword To Quadword */
	case 28:	/* Vector Count Trailing Zeros Byte */
	case 29:	/* Vector Count Trailing Zeros Halfword */
	case 30:	/* Vector Count Trailing Zeros Word */
	case 31:	/* Vector Count Trailing Zeros Doubleword */
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_vr0_regnum + PPC_VRT (insn));
	  return 0;
	}
    }

  if (ext == 1602)
    {
      switch (vra)
	{
	case 0: 	/* Vector Expand Byte Mask */
	case 1: 	/* Vector Expand Halfword Mask */
	case 2: 	/* Vector Expand Word Mask */
	case 3: 	/* Vector Expand Doubleword Mask */
	case 4: 	/* Vector Expand Quadword Mask */
	case 16:	/* Move to VSR Byte Mask */
	case 17:	/* Move to VSR Halfword Mask */
	case 18:	/* Move to VSR Word Mask */
	case 19:	/* Move to VSR Doubleword Mask */
	case 20:	/* Move to VSR Quadword Mask */
	  ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
	  return 0;

	case 8: 	/* Vector Extract Byte Mask */
	case 9: 	/* Vector Extract Halfword Mask */
	case 10: 	/* Vector Extract Word Mask */
	case 11: 	/* Vector Extract Doubleword Mask */
	case 12: 	/* Vector Extract Quadword Mask */

	/* Ignore the MP bit in the LSB position of the vra value. */
	case 24:	/* Vector Count Mask Bits Byte, MP = 0 */
	case 25:	/* Vector Count Mask Bits Byte, MP = 1 */
	case 26:	/* Vector Count Mask Bits Halfword, MP = 0 */
	case 27:	/* Vector Count Mask Bits Halfword, MP = 1 */
	case 28:	/* Vector Count Mask Bits Word, MP = 0 */
	case 29:	/* Vector Count Mask Bits Word, MP = 1 */
	case 30:	/* Vector Count Mask Bits Doubleword, MP = 0 */
	case 31:	/* Vector Count Mask Bits Doubleword, MP = 1 */
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_gp0_regnum + PPC_RT (insn));
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_gp0_regnum + PPC_RT (insn));
	  return 0;
	}
    }

  switch (ext)
    {

    case 257:		/* Vector Compare Unsigned Quadword */
    case 321:		/* Vector Compare Signed Quadword */
      /* Comparison tests that always set CR field BF */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      return 0;

    case 142:		/* Vector Pack Unsigned Halfword Unsigned Saturate */
    case 206:		/* Vector Pack Unsigned Word Unsigned Saturate */
    case 270:		/* Vector Pack Signed Halfword Unsigned Saturate */
    case 334:		/* Vector Pack Signed Word Unsigned Saturate */
    case 398:		/* Vector Pack Signed Halfword Signed Saturate */
    case 462:		/* Vector Pack Signed Word Signed Saturate */
    case 1230:		/* Vector Pack Unsigned Doubleword Unsigned Saturate */
    case 1358:		/* Vector Pack Signed Doubleword Unsigned Saturate */
    case 1486:		/* Vector Pack Signed Doubleword Signed Saturate */
    case 512:		/* Vector Add Unsigned Byte Saturate */
    case 576:		/* Vector Add Unsigned Halfword Saturate */
    case 640:		/* Vector Add Unsigned Word Saturate */
    case 768:		/* Vector Add Signed Byte Saturate */
    case 832:		/* Vector Add Signed Halfword Saturate */
    case 896:		/* Vector Add Signed Word Saturate */
    case 1536:		/* Vector Subtract Unsigned Byte Saturate */
    case 1600:		/* Vector Subtract Unsigned Halfword Saturate */
    case 1664:		/* Vector Subtract Unsigned Word Saturate */
    case 1792:		/* Vector Subtract Signed Byte Saturate */
    case 1856:		/* Vector Subtract Signed Halfword Saturate */
    case 1920:		/* Vector Subtract Signed Word Saturate */

    case 1544:		/* Vector Sum across Quarter Unsigned Byte Saturate */
    case 1800:		/* Vector Sum across Quarter Signed Byte Saturate */
    case 1608:		/* Vector Sum across Quarter Signed Halfword Saturate */
    case 1672:		/* Vector Sum across Half Signed Word Saturate */
    case 1928:		/* Vector Sum across Signed Word Saturate */
    case 970:		/* Vector Convert To Signed Fixed-Point Word Saturate */
    case 906:		/* Vector Convert To Unsigned Fixed-Point Word Saturate */
      record_full_arch_list_add_reg (regcache, PPC_VSCR_REGNUM);
      [[fallthrough]];
    case 12:		/* Vector Merge High Byte */
    case 14:		/* Vector Pack Unsigned Halfword Unsigned Modulo */
    case 76:		/* Vector Merge High Halfword */
    case 78:		/* Vector Pack Unsigned Word Unsigned Modulo */
    case 140:		/* Vector Merge High Word */
    case 268:		/* Vector Merge Low Byte */
    case 332:		/* Vector Merge Low Halfword */
    case 396:		/* Vector Merge Low Word */
    case 397:		/* Vector Clear Leftmost Bytes */
    case 461:		/* Vector Clear Rightmost Bytes */
    case 526:		/* Vector Unpack High Signed Byte */
    case 590:		/* Vector Unpack High Signed Halfword */
    case 654:		/* Vector Unpack Low Signed Byte */
    case 718:		/* Vector Unpack Low Signed Halfword */
    case 782:		/* Vector Pack Pixel */
    case 846:		/* Vector Unpack High Pixel */
    case 974:		/* Vector Unpack Low Pixel */
    case 1102:		/* Vector Pack Unsigned Doubleword Unsigned Modulo */
    case 1614:		/* Vector Unpack High Signed Word */
    case 1676:		/* Vector Merge Odd Word */
    case 1742:		/* Vector Unpack Low Signed Word */
    case 1932:		/* Vector Merge Even Word */
    case 524:		/* Vector Splat Byte */
    case 588:		/* Vector Splat Halfword */
    case 652:		/* Vector Splat Word */
    case 780:		/* Vector Splat Immediate Signed Byte */
    case 844:		/* Vector Splat Immediate Signed Halfword */
    case 908:		/* Vector Splat Immediate Signed Word */
    case 261:		/* Vector Shift Left Quadword */
    case 452:		/* Vector Shift Left */
    case 517:		/* Vector Shift Right Quadword */
    case 708:		/* Vector Shift Right */
    case 773:		/* Vector Shift Right Algebraic Quadword */
    case 1036:		/* Vector Shift Left by Octet */
    case 1100:		/* Vector Shift Right by Octet */
    case 0:		/* Vector Add Unsigned Byte Modulo */
    case 64:		/* Vector Add Unsigned Halfword Modulo */
    case 128:		/* Vector Add Unsigned Word Modulo */
    case 192:		/* Vector Add Unsigned Doubleword Modulo */
    case 256:		/* Vector Add Unsigned Quadword Modulo */
    case 320:		/* Vector Add & write Carry Unsigned Quadword */
    case 384:		/* Vector Add and Write Carry-Out Unsigned Word */
    case 8:		/* Vector Multiply Odd Unsigned Byte */
    case 72:		/* Vector Multiply Odd Unsigned Halfword */
    case 136:		/* Vector Multiply Odd Unsigned Word */
    case 200:		/* Vector Multiply Odd Unsigned Doubleword */
    case 264:		/* Vector Multiply Odd Signed Byte */
    case 328:		/* Vector Multiply Odd Signed Halfword */
    case 392:		/* Vector Multiply Odd Signed Word */
    case 456:		/* Vector Multiply Odd Signed Doubleword */
    case 520:		/* Vector Multiply Even Unsigned Byte */
    case 584:		/* Vector Multiply Even Unsigned Halfword */
    case 648:		/* Vector Multiply Even Unsigned Word */
    case 712:		/* Vector Multiply Even Unsigned Doubleword */
    case 776:		/* Vector Multiply Even Signed Byte */
    case 840:		/* Vector Multiply Even Signed Halfword */
    case 904:		/* Vector Multiply Even Signed Word */
    case 968:		/* Vector Multiply Even Signed Doubleword */
    case 457:		/* Vector Multiply Low Doubleword */
    case 649:		/* Vector Multiply High Unsigned Word */
    case 713:		/* Vector Multiply High Unsigned Doubleword */
    case 905:		/* Vector Multiply High Signed Word */
    case 969:		/* Vector Multiply High Signed Doubleword */
    case 11:		/* Vector Divide Unsigned Quadword */
    case 203:		/* Vector Divide Unsigned Doubleword */
    case 139:		/* Vector Divide Unsigned Word */
    case 267:		/* Vector Divide Signed Quadword */
    case 459:		/* Vector Divide Signed Doubleword */
    case 395:		/* Vector Divide Signed Word */
    case 523:		/* Vector Divide Extended Unsigned Quadword */
    case 715:		/* Vector Divide Extended Unsigned Doubleword */
    case 651:		/* Vector Divide Extended Unsigned Word */
    case 779:		/* Vector Divide Extended Signed Quadword */
    case 971:		/* Vector Divide Extended Signed Doubleword */
    case 907:		/* Vector Divide Extended Unsigned Word */
    case 1547:		/* Vector Modulo Unsigned Quadword */
    case 1675:		/* Vector Modulo Unsigned Word */
    case 1739:		/* Vector Modulo Unsigned Doubleword */
    case 1803:		/* Vector Modulo Signed Quadword */
    case 1931:		/* Vector Modulo Signed Word */
    case 1995:		/* Vector Modulo Signed Doubleword */

    case 137:		/* Vector Multiply Unsigned Word Modulo */
    case 1024:		/* Vector Subtract Unsigned Byte Modulo */
    case 1088:		/* Vector Subtract Unsigned Halfword Modulo */
    case 1152:		/* Vector Subtract Unsigned Word Modulo */
    case 1216:		/* Vector Subtract Unsigned Doubleword Modulo */
    case 1280:		/* Vector Subtract Unsigned Quadword Modulo */
    case 1344:		/* Vector Subtract & write Carry Unsigned Quadword */
    case 1408:		/* Vector Subtract and Write Carry-Out Unsigned Word */
    case 1282:		/* Vector Average Signed Byte */
    case 1346:		/* Vector Average Signed Halfword */
    case 1410:		/* Vector Average Signed Word */
    case 1026:		/* Vector Average Unsigned Byte */
    case 1090:		/* Vector Average Unsigned Halfword */
    case 1154:		/* Vector Average Unsigned Word */
    case 258:		/* Vector Maximum Signed Byte */
    case 322:		/* Vector Maximum Signed Halfword */
    case 386:		/* Vector Maximum Signed Word */
    case 450:		/* Vector Maximum Signed Doubleword */
    case 2:		/* Vector Maximum Unsigned Byte */
    case 66:		/* Vector Maximum Unsigned Halfword */
    case 130:		/* Vector Maximum Unsigned Word */
    case 194:		/* Vector Maximum Unsigned Doubleword */
    case 770:		/* Vector Minimum Signed Byte */
    case 834:		/* Vector Minimum Signed Halfword */
    case 898:		/* Vector Minimum Signed Word */
    case 962:		/* Vector Minimum Signed Doubleword */
    case 514:		/* Vector Minimum Unsigned Byte */
    case 578:		/* Vector Minimum Unsigned Halfword */
    case 642:		/* Vector Minimum Unsigned Word */
    case 706:		/* Vector Minimum Unsigned Doubleword */
    case 1028:		/* Vector Logical AND */
    case 1668:		/* Vector Logical Equivalent */
    case 1092:		/* Vector Logical AND with Complement */
    case 1412:		/* Vector Logical NAND */
    case 1348:		/* Vector Logical OR with Complement */
    case 1156:		/* Vector Logical OR */
    case 1284:		/* Vector Logical NOR */
    case 1220:		/* Vector Logical XOR */
    case 4:		/* Vector Rotate Left Byte */
    case 132:		/* Vector Rotate Left Word VX-form */
    case 68:		/* Vector Rotate Left Halfword */
    case 196:		/* Vector Rotate Left Doubleword */
    case 260:		/* Vector Shift Left Byte */
    case 388:		/* Vector Shift Left Word */
    case 324:		/* Vector Shift Left Halfword */
    case 1476:		/* Vector Shift Left Doubleword */
    case 516:		/* Vector Shift Right Byte */
    case 644:		/* Vector Shift Right Word */
    case 580:		/* Vector Shift Right Halfword */
    case 1732:		/* Vector Shift Right Doubleword */
    case 772:		/* Vector Shift Right Algebraic Byte */
    case 900:		/* Vector Shift Right Algebraic Word */
    case 836:		/* Vector Shift Right Algebraic Halfword */
    case 964:		/* Vector Shift Right Algebraic Doubleword */
    case 10:		/* Vector Add Single-Precision */
    case 74:		/* Vector Subtract Single-Precision */
    case 1034:		/* Vector Maximum Single-Precision */
    case 1098:		/* Vector Minimum Single-Precision */
    case 842:		/* Vector Convert From Signed Fixed-Point Word */
    case 778:		/* Vector Convert From Unsigned Fixed-Point Word */
    case 714:		/* Vector Round to Single-Precision Integer toward -Infinity */
    case 522:		/* Vector Round to Single-Precision Integer Nearest */
    case 650:		/* Vector Round to Single-Precision Integer toward +Infinity */
    case 586:		/* Vector Round to Single-Precision Integer toward Zero */
    case 394:		/* Vector 2 Raised to the Exponent Estimate Floating-Point */
    case 458:		/* Vector Log Base 2 Estimate Floating-Point */
    case 266:		/* Vector Reciprocal Estimate Single-Precision */
    case 330:		/* Vector Reciprocal Square Root Estimate Single-Precision */
    case 1288:		/* Vector AES Cipher */
    case 1289:		/* Vector AES Cipher Last */
    case 1352:		/* Vector AES Inverse Cipher */
    case 1353:		/* Vector AES Inverse Cipher Last */
    case 1480:		/* Vector AES SubBytes */
    case 1730:		/* Vector SHA-512 Sigma Doubleword */
    case 1666:		/* Vector SHA-256 Sigma Word */
    case 1032:		/* Vector Polynomial Multiply-Sum Byte */
    case 1160:		/* Vector Polynomial Multiply-Sum Word */
    case 1096:		/* Vector Polynomial Multiply-Sum Halfword */
    case 1224:		/* Vector Polynomial Multiply-Sum Doubleword */
    case 1292:		/* Vector Gather Bits by Bytes by Doubleword */
    case 1794:		/* Vector Count Leading Zeros Byte */
    case 1858:		/* Vector Count Leading Zeros Halfword */
    case 1922:		/* Vector Count Leading Zeros Word */
    case 1924:		/* Vector Count Leading Zeros Doubleword under
			   bit Mask*/
    case 1986:		/* Vector Count Leading Zeros Doubleword */
    case 1988:		/* Vector Count Trailing Zeros Doubleword under bit
			   Mask */
    case 1795:		/* Vector Population Count Byte */
    case 1859:		/* Vector Population Count Halfword */
    case 1923:		/* Vector Population Count Word */
    case 1987:		/* Vector Population Count Doubleword */
    case 1356:		/* Vector Bit Permute Quadword */
    case 1484:		/* Vector Bit Permute Doubleword */
    case 513:		/* Vector Multiply-by-10 Unsigned Quadword */
    case 1:		/* Vector Multiply-by-10 & write Carry Unsigned
			   Quadword */
    case 577:		/* Vector Multiply-by-10 Extended Unsigned Quadword */
    case 65:		/* Vector Multiply-by-10 Extended & write Carry
			   Unsigned Quadword */
    case 1027:		/* Vector Absolute Difference Unsigned Byte */
    case 1091:		/* Vector Absolute Difference Unsigned Halfword */
    case 1155:		/* Vector Absolute Difference Unsigned Word */
    case 1796:		/* Vector Shift Right Variable */
    case 1860:		/* Vector Shift Left Variable */
    case 133:		/* Vector Rotate Left Word then Mask Insert */
    case 197:		/* Vector Rotate Left Doubleword then Mask Insert */
    case 389:		/* Vector Rotate Left Word then AND with Mask */
    case 453:		/* Vector Rotate Left Doubleword then AND with Mask */
    case 525:		/* Vector Extract Unsigned Byte */
    case 589:		/* Vector Extract Unsigned Halfword */
    case 653:		/* Vector Extract Unsigned Word */
    case 717:		/* Vector Extract Doubleword */
    case 15:		/* Vector Insert Byte from VSR using GPR-specified
			   Left-Index */
    case 79:		/* Vector Insert Halfword from VSR using GPR-specified
			   Left-Index */
    case 143:		/* Vector Insert Word from VSR using GPR-specified
			   Left-Index */
    case 207:		/* Vector Insert Word from GPR using
			   immediate-specified index */
    case 463:		/* Vector Insert Doubleword from GPR using
			   immediate-specified index */
    case 271:		/* Vector Insert Byte from VSR using GPR-specified
			   Right-Index */
    case 335:		/* Vector Insert Halfword from VSR using GPR-specified
			   Right-Index */
    case 399:		/* Vector Insert Word from VSR using GPR-specified
			   Right-Index */
    case 527:		/* Vector Insert Byte from GPR using GPR-specified
			   Left-Index */
    case 591:		/* Vector Insert Halfword from GPR using GPR-specified
			   Left-Index */
    case 655:		/* Vector Insert Word from GPR using GPR-specified
			   Left-Index */
    case 719:		/* Vector Insert Doubleword from GPR using
			    GPR-specified Left-Index */
    case 783:		/* Vector Insert Byte from GPR using GPR-specified
			   Right-Index */
    case 847:		/* Vector Insert Halfword from GPR using GPR-specified
			   Left-Index */
    case 911:		/* Vector Insert Word from GPR using GPR-specified
			   Left-Index */
    case 975:		/* Vector Insert Doubleword from GPR using
			    GPR-specified Right-Index */
    case 781:		/* Vector Insert Byte */
    case 845:		/* Vector Insert Halfword */
    case 909:		/* Vector Insert Word */
    case 973:		/* Vector Insert Doubleword */
    case 1357:		/* Vector Centrifuge Doubleword */
    case 1421:		/* Vector Parallel Bits Extract Doubleword */
    case 1485:		/* Vector Parallel Bits Deposit Doubleword */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      return 0;

    case 1228:		/* Vector Gather every Nth Bit */
    case 1549:		/* Vector Extract Unsigned Byte Left-Indexed */
    case 1613:		/* Vector Extract Unsigned Halfword Left-Indexed */
    case 1677:		/* Vector Extract Unsigned Word Left-Indexed */
    case 1805:		/* Vector Extract Unsigned Byte Right-Indexed */
    case 1869:		/* Vector Extract Unsigned Halfword Right-Indexed */
    case 1933:		/* Vector Extract Unsigned Word Right-Indexed */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;

    case 1604:		/* Move To Vector Status and Control Register */
      record_full_arch_list_add_reg (regcache, PPC_VSCR_REGNUM);
      return 0;
    case 1540:		/* Move From Vector Status and Control Register */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      return 0;
    case 833:		/* Decimal Copy Sign */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;
    }

  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s, 4-%d.\n", insn, paddress (gdbarch, addr), ext);
  return -1;
}

/* Parse and record instructions of primary opcode 6 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op6 (struct gdbarch *gdbarch, struct regcache *regcache,
			CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int subtype = PPC_FIELD (insn, 28, 4);
  CORE_ADDR ea = 0;

  switch (subtype)
    {
    case 0:    /* Load VSX Vector Paired */
      ppc_record_vsr (regcache, tdep, PPC_XTp (insn));
      ppc_record_vsr (regcache, tdep, PPC_XTp (insn) + 1);
      return 0;
    case 1:    /* Store VSX Vector Paired */
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ea);
      ea += PPC_DQ (insn) << 4;
      record_full_arch_list_add_mem (ea, 32);
      return 0;
    }
  return -1;
}

/* Parse and record instructions of primary opcode-19 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op19 (struct gdbarch *gdbarch, struct regcache *regcache,
			   CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int ext = PPC_EXTOP (insn);

  switch (ext & 0x01f)
    {
    case 2:		/* Add PC Immediate Shifted */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;
    }

  switch (ext)
    {
    case 0:		/* Move Condition Register Field */
    case 33:		/* Condition Register NOR */
    case 129:		/* Condition Register AND with Complement */
    case 193:		/* Condition Register XOR */
    case 225:		/* Condition Register NAND */
    case 257:		/* Condition Register AND */
    case 289:		/* Condition Register Equivalent */
    case 417:		/* Condition Register OR with Complement */
    case 449:		/* Condition Register OR */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 16:		/* Branch Conditional */
    case 560:		/* Branch Conditional to Branch Target Address Register */
      if ((PPC_BO (insn) & 0x4) == 0)
	record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum);
      [[fallthrough]];
    case 528:		/* Branch Conditional to Count Register */
      if (PPC_LK (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum);
      return 0;

    case 150:		/* Instruction Synchronize */
      /* Do nothing.  */
      return 0;
    }

  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s, 19-%d.\n", insn, paddress (gdbarch, addr), ext);
  return -1;
}

/* Parse and record instructions of primary opcode-31 with the extended opcode
   177.  The argument is the word instruction (insn).  Return 0 if successful.
*/

static int
ppc_process_record_op31_177 (struct gdbarch *gdbarch,
			     struct regcache *regcache,
			     uint32_t insn)
{
  int RA_opcode = PPC_RA(insn);
  int as = PPC_FIELD (insn, 6, 3);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  switch (RA_opcode)
    {
    case 0: 		/* VSX Move From Accumulator, xxmfacc */
    case 1: 		/* VSX Move To Accumulator, xxmtacc */
    case 3: 		/* VSX Set Accumulator to Zero, xxsetaccz */
      ppc_record_ACC_fpscr (regcache, tdep, as, false);
      return 0;
    }
  return -1;
}

/* Parse and record instructions of primary opcode-31 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op31 (struct gdbarch *gdbarch, struct regcache *regcache,
			   CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int ext = PPC_EXTOP (insn);
  int tmp, nr, nb = 0, i;
  CORE_ADDR at_dcsz, ea = 0;
  ULONGEST rb, ra, xer;
  int size = 0;

  /* These instructions have OE bit.  */
  switch (ext & 0x1ff)
    {
    /* These write RT and XER.  Update CR if RC is set.  */
    case 8:		/* Subtract from carrying */
    case 10:		/* Add carrying */
    case 136:		/* Subtract from extended */
    case 138:		/* Add extended */
    case 200:		/* Subtract from zero extended */
    case 202:		/* Add to zero extended */
    case 232:		/* Subtract from minus one extended */
    case 234:		/* Add to minus one extended */
      /* CA is always altered, but SO/OV are only altered when OE=1.
	 In any case, XER is always altered.  */
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;

    /* These write RT.  Update CR if RC is set and update XER if OE is set.  */
    case 40:		/* Subtract from */
    case 104:		/* Negate */
    case 233:		/* Multiply low doubleword */
    case 235:		/* Multiply low word */
    case 266:		/* Add */
    case 393:		/* Divide Doubleword Extended Unsigned */
    case 395:		/* Divide Word Extended Unsigned */
    case 425:		/* Divide Doubleword Extended */
    case 427:		/* Divide Word Extended */
    case 457:		/* Divide Doubleword Unsigned */
    case 459:		/* Divide Word Unsigned */
    case 489:		/* Divide Doubleword */
    case 491:		/* Divide Word */
      if (PPC_OE (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      [[fallthrough]];
    case 9:		/* Multiply High Doubleword Unsigned */
    case 11:		/* Multiply High Word Unsigned */
    case 73:		/* Multiply High Doubleword */
    case 75:		/* Multiply High Word */
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;
    }

  if ((ext & 0x1f) == 15)
    {
      /* Integer Select. bit[16:20] is used for BC.  */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;
    }

  if ((ext & 0xff) == 170)
    {
      /* Add Extended using alternate carry bits */
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;
    }

  switch (ext)
    {
    case 78:		/* Determine Leftmost Zero Byte */
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;

    /* These only write RT.  */
    case 19:		/* Move from condition register */
			/* Move From One Condition Register Field */
    case 74:		/* Add and Generate Sixes */
    case 74 | 0x200:	/* Add and Generate Sixes (bit-21 dont-care) */
    case 302:		/* Move From Branch History Rolling Buffer */
    case 339:		/* Move From Special Purpose Register */
    case 371:		/* Move From Time Base [Phased-Out]  */
    case 309:		/* Load Doubleword Monitored Indexed  */
    case 128:		/* Set Boolean */
    case 384:		/* Set Boolean Condition */
    case 416:		/* Set Boolean Condition Reverse */
    case 448:		/* Set Negative Boolean Condition */
    case 480:		/* Set Negative Boolean Condition Reverse */
    case 755:		/* Deliver A Random Number */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;

    /* These only write to RA.  */
    case 51:		/* Move From VSR Doubleword */
    case 59:		/* Count Leading Zeros Doubleword under bit Mask */
    case 115:		/* Move From VSR Word and Zero */
    case 122:		/* Population count bytes */
    case 155:		/* Byte-Reverse Word */
    case 156:		/* Parallel Bits Deposit Doubleword */
    case 187:		/* Byte-Reverse Doubleword */
    case 188:		/* Parallel Bits Extract Doubleword */
    case 219:		/* Byte-Reverse Halfword */
    case 220:		/* Centrifuge Doubleword */
    case 378:		/* Population count words */
    case 506:		/* Population count doublewords */
    case 154:		/* Parity Word */
    case 186:		/* Parity Doubleword */
    case 252:		/* Bit Permute Doubleword */
    case 282:		/* Convert Declets To Binary Coded Decimal */
    case 314:		/* Convert Binary Coded Decimal To Declets */
    case 508:		/* Compare bytes */
    case 307:		/* Move From VSR Lower Doubleword */
    case 571:		/* Count Trailing Zeros Doubleword under bit Mask */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      return 0;

    /* These write CR and optional RA.  */
    case 792:		/* Shift Right Algebraic Word */
    case 794:		/* Shift Right Algebraic Doubleword */
    case 824:		/* Shift Right Algebraic Word Immediate */
    case 826:		/* Shift Right Algebraic Doubleword Immediate (413) */
    case 826 | 1:	/* Shift Right Algebraic Doubleword Immediate (413) */
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 0:		/* Compare */
    case 32:		/* Compare logical */
    case 144:		/* Move To Condition Register Fields */
			/* Move To One Condition Register Field */
    case 192:		/* Compare Ranged Byte */
    case 224:		/* Compare Equal Byte */
    case 576:		/* Move XER to CR Extended */
    case 902:		/* Paste (should always fail due to single-stepping and
			   the memory location might not be accessible, so
			   record only CR) */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    /* These write to RT.  Update RA if 'update indexed.'  */
    case 53:		/* Load Doubleword with Update Indexed */
    case 119:		/* Load Byte and Zero with Update Indexed */
    case 311:		/* Load Halfword and Zero with Update Indexed */
    case 55:		/* Load Word and Zero with Update Indexed */
    case 375:		/* Load Halfword Algebraic with Update Indexed */
    case 373:		/* Load Word Algebraic with Update Indexed */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 21:		/* Load Doubleword Indexed */
    case 52:		/* Load Byte And Reserve Indexed */
    case 116:		/* Load Halfword And Reserve Indexed */
    case 20:		/* Load Word And Reserve Indexed */
    case 84:		/* Load Doubleword And Reserve Indexed */
    case 87:		/* Load Byte and Zero Indexed */
    case 279:		/* Load Halfword and Zero Indexed */
    case 23:		/* Load Word and Zero Indexed */
    case 343:		/* Load Halfword Algebraic Indexed */
    case 341:		/* Load Word Algebraic Indexed */
    case 790:		/* Load Halfword Byte-Reverse Indexed */
    case 534:		/* Load Word Byte-Reverse Indexed */
    case 532:		/* Load Doubleword Byte-Reverse Indexed */
    case 582:		/* Load Word Atomic */
    case 614:		/* Load Doubleword Atomic */
    case 265:		/* Modulo Unsigned Doubleword */
    case 777:		/* Modulo Signed Doubleword */
    case 267:		/* Modulo Unsigned Word */
    case 779:		/* Modulo Signed Word */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      return 0;

    case 597:		/* Load String Word Immediate */
    case 533:		/* Load String Word Indexed */
      if (ext == 597)
	{
	nr = PPC_NB (insn);
	if (nr == 0)
	  nr = 32;
	}
      else
	{
	  regcache_raw_read_unsigned (regcache, tdep->ppc_xer_regnum, &xer);
	  nr = PPC_XER_NB (xer);
	}

      nr = (nr + 3) >> 2;

      /* If n=0, the contents of register RT are undefined.  */
      if (nr == 0)
	nr = 1;

      for (i = 0; i < nr; i++)
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_gp0_regnum
				       + ((PPC_RT (insn) + i) & 0x1f));
      return 0;

    case 276:		/* Load Quadword And Reserve Indexed */
      tmp = tdep->ppc_gp0_regnum + (PPC_RT (insn) & ~1);
      record_full_arch_list_add_reg (regcache, tmp);
      record_full_arch_list_add_reg (regcache, tmp + 1);
      return 0;

    /* These write VRT.  */
    case 6:		/* Load Vector for Shift Left Indexed */
    case 38:		/* Load Vector for Shift Right Indexed */
    case 7:		/* Load Vector Element Byte Indexed */
    case 39:		/* Load Vector Element Halfword Indexed */
    case 71:		/* Load Vector Element Word Indexed */
    case 103:		/* Load Vector Indexed */
    case 359:		/* Load Vector Indexed LRU */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_vr0_regnum + PPC_VRT (insn));
      return 0;

    /* These write FRT.  Update RA if 'update indexed.'  */
    case 567:		/* Load Floating-Point Single with Update Indexed */
    case 631:		/* Load Floating-Point Double with Update Indexed */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 535:		/* Load Floating-Point Single Indexed */
    case 599:		/* Load Floating-Point Double Indexed */
    case 855:		/* Load Floating-Point as Integer Word Algebraic Indexed */
    case 887:		/* Load Floating-Point as Integer Word and Zero Indexed */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      return 0;

    case 791:		/* Load Floating-Point Double Pair Indexed */
      tmp = tdep->ppc_fp0_regnum + (PPC_FRT (insn) & ~1);
      record_full_arch_list_add_reg (regcache, tmp);
      record_full_arch_list_add_reg (regcache, tmp + 1);
      return 0;

    /* These write to destination register PPC_XT. */
    case 179:		/* Move To VSR Doubleword */
    case 211:		/* Move To VSR Word Algebraic */
    case 243:		/* Move To VSR Word and Zero */
    case 588:		/* Load VSX Scalar Doubleword Indexed */
    case 524:		/* Load VSX Scalar Single-Precision Indexed */
    case 76:		/* Load VSX Scalar as Integer Word Algebraic Indexed */
    case 12:		/* Load VSX Scalar as Integer Word and Zero Indexed */
    case 13:		/* Load VSX Vector Rightmost Byte Indexed */
    case 45:		/* Load VSX Vector Rightmost Halfword Indexed */
    case 77:		/* Load VSX Vector Rightmost Word Indexed */
    case 109:		/* Load VSX Vector Rightmost Doubleword Indexed */
    case 844:		/* Load VSX Vector Doubleword*2 Indexed */
    case 332:		/* Load VSX Vector Doubleword & Splat Indexed */
    case 780:		/* Load VSX Vector Word*4 Indexed */
    case 268:		/* Load VSX Vector Indexed */
    case 364:		/* Load VSX Vector Word & Splat Indexed */
    case 812:		/* Load VSX Vector Halfword*8 Indexed */
    case 876:		/* Load VSX Vector Byte*16 Indexed */
    case 269:		/* Load VSX Vector with Length */
    case 301:		/* Load VSX Vector Left-justified with Length */
    case 781:		/* Load VSX Scalar as Integer Byte & Zero Indexed */
    case 813:		/* Load VSX Scalar as Integer Halfword & Zero Indexed */
    case 403:		/* Move To VSR Word & Splat */
    case 435:		/* Move To VSR Double Doubleword */
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;

    case 333:		/* Load VSX Vector Paired Indexed */
      ppc_record_vsr (regcache, tdep, PPC_XTp (insn));
      ppc_record_vsr (regcache, tdep, PPC_XTp (insn) + 1);
      return 0;

    /* These write RA.  Update CR if RC is set.  */
    case 24:		/* Shift Left Word */
    case 26:		/* Count Leading Zeros Word */
    case 27:		/* Shift Left Doubleword */
    case 28:		/* AND */
    case 58:		/* Count Leading Zeros Doubleword */
    case 60:		/* AND with Complement */
    case 124:		/* NOR */
    case 284:		/* Equivalent */
    case 316:		/* XOR */
    case 476:		/* NAND */
    case 412:		/* OR with Complement */
    case 444:		/* OR */
    case 536:		/* Shift Right Word */
    case 539:		/* Shift Right Doubleword */
    case 922:		/* Extend Sign Halfword */
    case 954:		/* Extend Sign Byte */
    case 986:		/* Extend Sign Word */
    case 538:		/* Count Trailing Zeros Word */
    case 570:		/* Count Trailing Zeros Doubleword */
    case 890:		/* Extend-Sign Word and Shift Left Immediate (445) */
    case 890 | 1:	/* Extend-Sign Word and Shift Left Immediate (445) */

      if (ext == 444 && tdep->ppc_ppr_regnum >= 0
	  && (PPC_RS (insn) == PPC_RA (insn))
	  && (PPC_RA (insn) == PPC_RB (insn))
	  && !PPC_RC (insn))
	{
	  /* or Rx,Rx,Rx alters PRI in PPR.  */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_ppr_regnum);
	  return 0;
	}

      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      return 0;

    /* Store memory.  */
    case 181:		/* Store Doubleword with Update Indexed */
    case 183:		/* Store Word with Update Indexed */
    case 247:		/* Store Byte with Update Indexed */
    case 439:		/* Store Half Word with Update Indexed */
    case 695:		/* Store Floating-Point Single with Update Indexed */
    case 759:		/* Store Floating-Point Double with Update Indexed */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 135:		/* Store Vector Element Byte Indexed */
    case 167:		/* Store Vector Element Halfword Indexed */
    case 199:		/* Store Vector Element Word Indexed */
    case 231:		/* Store Vector Indexed */
    case 487:		/* Store Vector Indexed LRU */
    case 716:		/* Store VSX Scalar Doubleword Indexed */
    case 140:		/* Store VSX Scalar as Integer Word Indexed */
    case 652:		/* Store VSX Scalar Single-Precision Indexed */
    case 972:		/* Store VSX Vector Doubleword*2 Indexed */
    case 908:		/* Store VSX Vector Word*4 Indexed */
    case 149:		/* Store Doubleword Indexed */
    case 151:		/* Store Word Indexed */
    case 215:		/* Store Byte Indexed */
    case 407:		/* Store Half Word Indexed */
    case 694:		/* Store Byte Conditional Indexed */
    case 726:		/* Store Halfword Conditional Indexed */
    case 150:		/* Store Word Conditional Indexed */
    case 214:		/* Store Doubleword Conditional Indexed */
    case 182:		/* Store Quadword Conditional Indexed */
    case 662:		/* Store Word Byte-Reverse Indexed */
    case 918:		/* Store Halfword Byte-Reverse Indexed */
    case 660:		/* Store Doubleword Byte-Reverse Indexed */
    case 663:		/* Store Floating-Point Single Indexed */
    case 727:		/* Store Floating-Point Double Indexed */
    case 919:		/* Store Floating-Point Double Pair Indexed */
    case 983:		/* Store Floating-Point as Integer Word Indexed */
    case 396:		/* Store VSX Vector Indexed */
    case 940:		/* Store VSX Vector Halfword*8 Indexed */
    case 1004:		/* Store VSX Vector Byte*16 Indexed */
    case 909:		/* Store VSX Scalar as Integer Byte Indexed */
    case 941:		/* Store VSX Scalar as Integer Halfword Indexed */
      if (ext == 694 || ext == 726 || ext == 150 || ext == 214 || ext == 182)
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);

      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      regcache_raw_read_unsigned (regcache,
				  tdep->ppc_gp0_regnum + PPC_RB (insn), &rb);
      ea = ra + rb;

      switch (ext)
	{
	case 183:	/* Store Word with Update Indexed */
	case 199:	/* Store Vector Element Word Indexed */
	case 140:	/* Store VSX Scalar as Integer Word Indexed */
	case 652:	/* Store VSX Scalar Single-Precision Indexed */
	case 151:	/* Store Word Indexed */
	case 150:	/* Store Word Conditional Indexed */
	case 662:	/* Store Word Byte-Reverse Indexed */
	case 663:	/* Store Floating-Point Single Indexed */
	case 695:	/* Store Floating-Point Single with Update Indexed */
	case 983:	/* Store Floating-Point as Integer Word Indexed */
	  size = 4;
	  break;
	case 247:	/* Store Byte with Update Indexed */
	case 135:	/* Store Vector Element Byte Indexed */
	case 215:	/* Store Byte Indexed */
	case 694:	/* Store Byte Conditional Indexed */
	case 909:	/* Store VSX Scalar as Integer Byte Indexed */
	  size = 1;
	  break;
	case 439:	/* Store Halfword with Update Indexed */
	case 167:	/* Store Vector Element Halfword Indexed */
	case 407:	/* Store Halfword Indexed */
	case 726:	/* Store Halfword Conditional Indexed */
	case 918:	/* Store Halfword Byte-Reverse Indexed */
	case 941:	/* Store VSX Scalar as Integer Halfword Indexed */
	  size = 2;
	  break;
	case 181:	/* Store Doubleword with Update Indexed */
	case 716:	/* Store VSX Scalar Doubleword Indexed */
	case 149:	/* Store Doubleword Indexed */
	case 214:	/* Store Doubleword Conditional Indexed */
	case 660:	/* Store Doubleword Byte-Reverse Indexed */
	case 727:	/* Store Floating-Point Double Indexed */
	case 759:	/* Store Floating-Point Double with Update Indexed */
	  size = 8;
	  break;
	case 972:	/* Store VSX Vector Doubleword*2 Indexed */
	case 908:	/* Store VSX Vector Word*4 Indexed */
	case 182:	/* Store Quadword Conditional Indexed */
	case 231:	/* Store Vector Indexed */
	case 487:	/* Store Vector Indexed LRU */
	case 919:	/* Store Floating-Point Double Pair Indexed */
	case 396:	/* Store VSX Vector Indexed */
	case 940:	/* Store VSX Vector Halfword*8 Indexed */
	case 1004:	/* Store VSX Vector Byte*16 Indexed */
	  size = 16;
	  break;
	default:
	  gdb_assert (0);
	}

      /* Align address for Store Vector instructions.  */
      switch (ext)
	{
	case 167:	/* Store Vector Element Halfword Indexed */
	  ea = ea & ~0x1ULL;
	  break;

	case 199:	/* Store Vector Element Word Indexed */
	  ea = ea & ~0x3ULL;
	  break;

	case 231:	/* Store Vector Indexed */
	case 487:	/* Store Vector Indexed LRU */
	  ea = ea & ~0xfULL;
	  break;
	}

      record_full_arch_list_add_mem (ea, size);
      return 0;

    case 141:		/* Store VSX Vector Rightmost Byte Indexed */
    case 173:		/* Store VSX Vector Rightmost Halfword Indexed */
    case 205:		/* Store VSX Vector Rightmost Word Indexed */
    case 237:		/* Store VSX Vector Rightmost Doubleword Indexed */
      switch(ext)
	{
	  case 141: nb = 1;
	  break;
	  case 173: nb = 2;
	  break;
	  case 205: nb = 4;
	  break;
	  case 237: nb = 8;
	  break;
	}
      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RB (insn), &rb);
      ea = ra + rb;
      record_full_arch_list_add_mem (ea, nb);
      return 0;

    case 397:		/* Store VSX Vector with Length */
    case 429:		/* Store VSX Vector Left-justified with Length */
      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      ea = ra;
      regcache_raw_read_unsigned (regcache,
				  tdep->ppc_gp0_regnum + PPC_RB (insn), &rb);
      /* Store up to 16 bytes.  */
      nb = (rb & 0xff) > 16 ? 16 : (rb & 0xff);
      if (nb > 0)
	record_full_arch_list_add_mem (ea, nb);
      return 0;

    case 461:		/* Store VSX Vector Paired Indexed */
      {
	if (PPC_RA (insn) != 0)
	  regcache_raw_read_unsigned (regcache,
				      tdep->ppc_gp0_regnum
				      + PPC_RA (insn), &ea);
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RB (insn), &rb);
	ea += rb;
	record_full_arch_list_add_mem (ea, 32);
	return 0;
      }

    case 710:		/* Store Word Atomic */
    case 742:		/* Store Doubleword Atomic */
      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      ea = ra;
      switch (ext)
	{
	case 710:	/* Store Word Atomic */
	  size = 8;
	  break;
	case 742:	/* Store Doubleword Atomic */
	  size = 16;
	  break;
	default:
	  gdb_assert (0);
	}
      record_full_arch_list_add_mem (ea, size);
      return 0;

    case 725:		/* Store String Word Immediate */
      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      ea += ra;

      nb = PPC_NB (insn);
      if (nb == 0)
	nb = 32;

      record_full_arch_list_add_mem (ea, nb);

      return 0;

    case 661:		/* Store String Word Indexed */
      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      ea += ra;

      regcache_raw_read_unsigned (regcache, tdep->ppc_xer_regnum, &xer);
      nb = PPC_XER_NB (xer);

      if (nb != 0)
	{
	  regcache_raw_read_unsigned (regcache,
				      tdep->ppc_gp0_regnum + PPC_RB (insn),
				      &rb);
	  ea += rb;
	  record_full_arch_list_add_mem (ea, nb);
	}

      return 0;

    case 467:		/* Move To Special Purpose Register */
      switch (PPC_SPR (insn))
	{
	case 1:			/* XER */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
	  return 0;
	case 3:			/* DSCR */
	  if (tdep->ppc_dscr_regnum >= 0)
	    record_full_arch_list_add_reg (regcache, tdep->ppc_dscr_regnum);
	  return 0;
	case 8:			/* LR */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum);
	  return 0;
	case 9:			/* CTR */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum);
	  return 0;
	case 256:		/* VRSAVE */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_vrsave_regnum);
	  return 0;
	case 815:		/* TAR */
	  if (tdep->ppc_tar_regnum >= 0)
	    record_full_arch_list_add_reg (regcache, tdep->ppc_tar_regnum);
	  return 0;
	case 896:
	case 898:		/* PPR */
	  if (tdep->ppc_ppr_regnum >= 0)
	    record_full_arch_list_add_reg (regcache, tdep->ppc_ppr_regnum);
	  return 0;
	}

      goto UNKNOWN_OP;

    case 147:		/* Move To Split Little Endian */
      record_full_arch_list_add_reg (regcache, tdep->ppc_ps_regnum);
      return 0;

    case 512:		/* Move to Condition Register from XER */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      return 0;

    case 4:		/* Trap Word */
    case 68:		/* Trap Doubleword */
    case 430:		/* Clear BHRB */
    case 598:		/* Synchronize */
    case 62:		/* Wait for Interrupt */
    case 30:		/* Wait */
    case 22:		/* Instruction Cache Block Touch */
    case 854:		/* Enforce In-order Execution of I/O */
    case 246:		/* Data Cache Block Touch for Store */
    case 54:		/* Data Cache Block Store */
    case 86:		/* Data Cache Block Flush */
    case 278:		/* Data Cache Block Touch */
    case 758:		/* Data Cache Block Allocate */
    case 982:		/* Instruction Cache Block Invalidate */
    case 774:		/* Copy */
    case 838:		/* CP_Abort */
      return 0;

    case 654:		/* Transaction Begin */
    case 686:		/* Transaction End */
    case 750:		/* Transaction Suspend or Resume */
    case 782:		/* Transaction Abort Word Conditional */
    case 814:		/* Transaction Abort Doubleword Conditional */
    case 846:		/* Transaction Abort Word Conditional Immediate */
    case 878:		/* Transaction Abort Doubleword Conditional Immediate */
    case 910:		/* Transaction Abort */
      record_full_arch_list_add_reg (regcache, tdep->ppc_ps_regnum);
      [[fallthrough]];
    case 718:		/* Transaction Check */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 1014:		/* Data Cache Block set to Zero */
      if (target_auxv_search (AT_DCACHEBSIZE, &at_dcsz) <= 0
	  || at_dcsz == 0)
	at_dcsz = 128; /* Assume 128-byte cache line size (POWER8)  */

      ra = 0;
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn), &ra);
      regcache_raw_read_unsigned (regcache,
				  tdep->ppc_gp0_regnum + PPC_RB (insn), &rb);
      ea = (ra + rb) & ~((ULONGEST) (at_dcsz - 1));
      record_full_arch_list_add_mem (ea, at_dcsz);
      return 0;

    case 177:
      if (ppc_process_record_op31_177 (gdbarch, regcache, insn) == 0)
	return 0;
    }

UNKNOWN_OP:
  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s, 31-%d.\n", insn, paddress (gdbarch, addr), ext);
  return -1;
}

/* Parse and record instructions of primary opcode-59 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op59 (struct gdbarch *gdbarch, struct regcache *regcache,
			 CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int ext = PPC_EXTOP (insn);
  int at = PPC_FIELD (insn, 6, 3);

  /* Note the mnemonics for the pmxvf64ger* instructions were officially
     changed to pmdmxvf64ger*.  The old mnemonics are still supported as
     extended mnemonics.  */

  switch (ext & 0x1f)
    {
    case 18:		/* Floating Divide */
    case 20:		/* Floating Subtract */
    case 21:		/* Floating Add */
    case 22:		/* Floating Square Root */
    case 24:		/* Floating Reciprocal Estimate */
    case 25:		/* Floating Multiply */
    case 26:		/* Floating Reciprocal Square Root Estimate */
    case 28:		/* Floating Multiply-Subtract */
    case 29:		/* Floating Multiply-Add */
    case 30:		/* Floating Negative Multiply-Subtract */
    case 31:		/* Floating Negative Multiply-Add */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);

      return 0;
    }

  /* MMA instructions, keep looking.  */
  switch (ext >> 2)    /* Additional opcode field is upper 8-bits of ext */
    {
    case 3:	/* VSX Vector 8-bit Signed/Unsigned Integer GER, xvi8ger4 */
    case 2:	/* VSX Vector 8-bit Signed/Unsigned Integer GER Positive
		   multiply, Positive accumulate, xvi8ger4pp */

    case 99:	/* VSX Vector 8-bit Signed/Unsigned Integer GER with
		   Saturate Positive multiply, Positive accumulate,
		   xvi8ger4spp */

    case 35:	/* VSX Vector 4-bit Signed Integer GER, xvi4ger8 */
    case 34:	/* VSX Vector 4-bit Signed Integer GER Positive multiply,
		   Positive accumulate, xvi4ger8pp */

    case 75:	/* VSX Vector 16-bit Signed Integer GER, xvi16ger2 */
    case 107:	/* VSX Vector 16-bit Signed Integer GER  Positive multiply,
		   Positive accumulate, xvi16ger2pp */

    case 43:	/* VSX Vector 16-bit Signed Integer GER with Saturation,
		   xvi16ger2s */
    case 42:	/* VSX Vector 16-bit Signed Integer GER with Saturation
		   Positive multiply, Positive accumulate, xvi16ger2spp */
      ppc_record_ACC_fpscr (regcache, tdep, at, false);
      return 0;

    case 19:	/* VSX Vector 16-bit Floating-Point GER, xvf16ger2 */
    case 18:	/* VSX Vector 16-bit Floating-Point GER Positive multiply,
		   Positive accumulate, xvf16ger2pp */
    case 146:	/* VSX Vector 16-bit Floating-Point GER Positive multiply,
		   Negative accumulate, xvf16ger2pn */
    case 82:	/* VSX Vector 16-bit Floating-Point GER Negative multiply,
		   Positive accumulate, xvf16ger2np */
    case 210:	/* VSX Vector 16-bit Floating-Point GER Negative multiply,
		   Negative accumulate, xvf16ger2nn */

    case 27:	/* VSX Vector 32-bit Floating-Point GER, xvf32ger */
    case 26:	/* VSX Vector 32-bit Floating-Point GER Positive multiply,
		   Positive accumulate, xvf32gerpp */
    case 154:	/* VSX Vector 32-bit Floating-Point GER Positive multiply,
		   Negative accumulate, xvf32gerpn */
    case 90:	/* VSX Vector 32-bit Floating-Point GER Negative multiply,
		   Positive accumulate, xvf32gernp */
    case 218:	/* VSX Vector 32-bit Floating-Point GER Negative multiply,
		   Negative accumulate, xvf32gernn */

    case 59:	/* VSX Vector 64-bit Floating-Point GER, pmdmxvf64ger
		   (pmxvf64ger)  */
    case 58:	/* VSX Vector 64-bit Floating-Point GER Positive multiply,
		   Positive accumulate, xvf64gerpp */
    case 186:	/* VSX Vector 64-bit Floating-Point GER Positive multiply,
		   Negative accumulate, xvf64gerpn */
    case 122:	/* VSX Vector 64-bit Floating-Point GER Negative multiply,
		   Positive accumulate, xvf64gernp */
    case 250:	/* VSX Vector 64-bit Floating-Point GER Negative multiply,
		   Negative accumulate, pmdmxvf64gernn (pmxvf64gernn)  */

    case 51:	/* VSX Vector bfloat16 GER, xvbf16ger2 */
    case 50:	/* VSX Vector bfloat16 GER Positive multiply,
		   Positive accumulate, xvbf16ger2pp */
    case 178:	/* VSX Vector bfloat16 GER Positive multiply,
		   Negative accumulate, xvbf16ger2pn */
    case 114:	/* VSX Vector bfloat16 GER Negative multiply,
		   Positive accumulate, xvbf16ger2np */
    case 242:	/* VSX Vector bfloat16 GER Negative multiply,
		   Negative accumulate, xvbf16ger2nn */
      ppc_record_ACC_fpscr (regcache, tdep, at, true);
      return 0;
    }

  switch (ext)
    {
    case 2:		/* DFP Add */
    case 3:		/* DFP Quantize */
    case 34:		/* DFP Multiply */
    case 35:		/* DFP Reround */
    case 67:		/* DFP Quantize Immediate */
    case 99:		/* DFP Round To FP Integer With Inexact */
    case 227:		/* DFP Round To FP Integer Without Inexact */
    case 258:		/* DFP Convert To DFP Long! */
    case 290:		/* DFP Convert To Fixed */
    case 514:		/* DFP Subtract */
    case 546:		/* DFP Divide */
    case 770:		/* DFP Round To DFP Short! */
    case 802:		/* DFP Convert From Fixed */
    case 834:		/* DFP Encode BCD To DPD */
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 130:		/* DFP Compare Ordered */
    case 162:		/* DFP Test Exponent */
    case 194:		/* DFP Test Data Class */
    case 226:		/* DFP Test Data Group */
    case 642:		/* DFP Compare Unordered */
    case 674:		/* DFP Test Significance */
    case 675:		/* DFP Test Significance Immediate */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 66:		/* DFP Shift Significand Left Immediate */
    case 98:		/* DFP Shift Significand Right Immediate */
    case 322:		/* DFP Decode DPD To BCD */
    case 354:		/* DFP Extract Biased Exponent */
    case 866:		/* DFP Insert Biased Exponent */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 846:		/* Floating Convert From Integer Doubleword Single */
    case 974:		/* Floating Convert From Integer Doubleword Unsigned
			   Single */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);

      return 0;
    }

  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s, 59-%d.\n", insn, paddress (gdbarch, addr), ext);
  return -1;
}

/* Parse and record an XX2-Form instruction with opcode 60 at ADDR.  The
   word instruction is an argument insn.  Return 0 if successful.  */

static int
ppc_process_record_op60_XX2 (struct gdbarch *gdbarch,
			     struct regcache *regcache,
			     CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int RA_opcode = PPC_RA(insn);

  switch (RA_opcode)
    {
    case 2:	/* VSX Vector Test Least-Significant Bit by Byte */
    case 25:	/* VSX Vector round and Convert Single-Precision format
		   to Half-Precision format.  Only changes the CR
		   field.  */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;
    case 17:	/* VSX Vector Convert with round Single-Precision
		   to bfloat16 format */
    case 24:	/* VSX Vector Convert Half-Precision format to
		   Single-Precision format */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 0:	/* VSX Vector Extract Exponent Double-Precision */
    case 1:	/* VSX Vector Extract Significand Double-Precision */
    case 7:	/* VSX Vector Byte-Reverse Halfword */
    case 8:	/* VSX Vector Extract Exponent Single-Precision */
    case 9:	/* VSX Vector Extract Significand Single-Precision */
    case 15:	/* VSX Vector Byte-Reverse Word */
    case 16:	/* VSX Vector Convert bfloat16 to Single-Precision
		   format Non-signaling */
    case 23:	/* VSX Vector Byte-Reverse Doubleword */
    case 31:	/* VSX Vector Byte-Reverse Quadword */
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;
    }

  return -1;
}

/* Parse and record instructions of primary opcode-60 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op60 (struct gdbarch *gdbarch, struct regcache *regcache,
			   CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int ext = PPC_EXTOP (insn);

  switch (ext >> 2)
    {
    case 0:		/* VSX Scalar Add Single-Precision */
    case 32:		/* VSX Scalar Add Double-Precision */
    case 24:		/* VSX Scalar Divide Single-Precision */
    case 56:		/* VSX Scalar Divide Double-Precision */
    case 176:		/* VSX Scalar Copy Sign Double-Precision */
    case 33:		/* VSX Scalar Multiply-Add Double-Precision */
    case 41:		/* ditto */
    case 1:		/* VSX Scalar Multiply-Add Single-Precision */
    case 9:		/* ditto */
    case 160:		/* VSX Scalar Maximum Double-Precision */
    case 168:		/* VSX Scalar Minimum Double-Precision */
    case 49:		/* VSX Scalar Multiply-Subtract Double-Precision */
    case 57:		/* ditto */
    case 17:		/* VSX Scalar Multiply-Subtract Single-Precision */
    case 25:		/* ditto */
    case 48:		/* VSX Scalar Multiply Double-Precision */
    case 16:		/* VSX Scalar Multiply Single-Precision */
    case 161:		/* VSX Scalar Negative Multiply-Add Double-Precision */
    case 169:		/* ditto */
    case 129:		/* VSX Scalar Negative Multiply-Add Single-Precision */
    case 137:		/* ditto */
    case 177:		/* VSX Scalar Negative Multiply-Subtract Double-Precision */
    case 185:		/* ditto */
    case 145:		/* VSX Scalar Negative Multiply-Subtract Single-Precision */
    case 153:		/* ditto */
    case 40:		/* VSX Scalar Subtract Double-Precision */
    case 8:		/* VSX Scalar Subtract Single-Precision */
    case 96:		/* VSX Vector Add Double-Precision */
    case 64:		/* VSX Vector Add Single-Precision */
    case 120:		/* VSX Vector Divide Double-Precision */
    case 88:		/* VSX Vector Divide Single-Precision */
    case 97:		/* VSX Vector Multiply-Add Double-Precision */
    case 105:		/* ditto */
    case 65:		/* VSX Vector Multiply-Add Single-Precision */
    case 73:		/* ditto */
    case 224:		/* VSX Vector Maximum Double-Precision */
    case 192:		/* VSX Vector Maximum Single-Precision */
    case 232:		/* VSX Vector Minimum Double-Precision */
    case 200:		/* VSX Vector Minimum Single-Precision */
    case 113:		/* VSX Vector Multiply-Subtract Double-Precision */
    case 121:		/* ditto */
    case 81:		/* VSX Vector Multiply-Subtract Single-Precision */
    case 89:		/* ditto */
    case 112:		/* VSX Vector Multiply Double-Precision */
    case 80:		/* VSX Vector Multiply Single-Precision */
    case 225:		/* VSX Vector Negative Multiply-Add Double-Precision */
    case 233:		/* ditto */
    case 193:		/* VSX Vector Negative Multiply-Add Single-Precision */
    case 201:		/* ditto */
    case 241:		/* VSX Vector Negative Multiply-Subtract Double-Precision */
    case 249:		/* ditto */
    case 209:		/* VSX Vector Negative Multiply-Subtract Single-Precision */
    case 217:		/* ditto */
    case 104:		/* VSX Vector Subtract Double-Precision */
    case 72:		/* VSX Vector Subtract Single-Precision */
    case 128:		/* VSX Scalar Maximum Type-C Double-Precision */
    case 136:		/* VSX Scalar Minimum Type-C Double-Precision */
    case 144:		/* VSX Scalar Maximum Type-J Double-Precision */
    case 152:		/* VSX Scalar Minimum Type-J Double-Precision */
    case 3:		/* VSX Scalar Compare Equal Double-Precision */
    case 11:		/* VSX Scalar Compare Greater Than Double-Precision */
    case 19:		/* VSX Scalar Compare Greater Than or Equal
			   Double-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 240:		/* VSX Vector Copy Sign Double-Precision */
    case 208:		/* VSX Vector Copy Sign Single-Precision */
    case 130:		/* VSX Logical AND */
    case 138:		/* VSX Logical AND with Complement */
    case 186:		/* VSX Logical Equivalence */
    case 178:		/* VSX Logical NAND */
    case 170:		/* VSX Logical OR with Complement */
    case 162:		/* VSX Logical NOR */
    case 146:		/* VSX Logical OR */
    case 154:		/* VSX Logical XOR */
    case 18:		/* VSX Merge High Word */
    case 50:		/* VSX Merge Low Word */
    case 10:		/* VSX Permute Doubleword Immediate (DM=0) */
    case 10 | 0x20:	/* VSX Permute Doubleword Immediate (DM=1) */
    case 10 | 0x40:	/* VSX Permute Doubleword Immediate (DM=2) */
    case 10 | 0x60:	/* VSX Permute Doubleword Immediate (DM=3) */
    case 2:		/* VSX Shift Left Double by Word Immediate (SHW=0) */
    case 2 | 0x20:	/* VSX Shift Left Double by Word Immediate (SHW=1) */
    case 2 | 0x40:	/* VSX Shift Left Double by Word Immediate (SHW=2) */
    case 2 | 0x60:	/* VSX Shift Left Double by Word Immediate (SHW=3) */
    case 216:		/* VSX Vector Insert Exponent Single-Precision */
    case 248:		/* VSX Vector Insert Exponent Double-Precision */
    case 26:		/* VSX Vector Permute */
    case 58:		/* VSX Vector Permute Right-indexed */
    case 213:		/* VSX Vector Test Data Class Single-Precision (DC=0) */
    case 213 | 0x8:	/* VSX Vector Test Data Class Single-Precision (DC=1) */
    case 245:		/* VSX Vector Test Data Class Double-Precision (DC=0) */
    case 245 | 0x8:	/* VSX Vector Test Data Class Double-Precision (DC=1) */
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;

    case 61:		/* VSX Scalar Test for software Divide Double-Precision */
    case 125:		/* VSX Vector Test for software Divide Double-Precision */
    case 93:		/* VSX Vector Test for software Divide Single-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 35:		/* VSX Scalar Compare Unordered Double-Precision */
    case 43:		/* VSX Scalar Compare Ordered Double-Precision */
    case 59:		/* VSX Scalar Compare Exponents Double-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;
    }

  switch ((ext >> 2) & 0x7f) /* Mask out Rc-bit.  */
    {
    case 99:		/* VSX Vector Compare Equal To Double-Precision */
    case 67:		/* VSX Vector Compare Equal To Single-Precision */
    case 115:		/* VSX Vector Compare Greater Than or
			   Equal To Double-Precision */
    case 83:		/* VSX Vector Compare Greater Than or
			   Equal To Single-Precision */
    case 107:		/* VSX Vector Compare Greater Than Double-Precision */
    case 75:		/* VSX Vector Compare Greater Than Single-Precision */
      if (PPC_Rc (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;
    }

  switch (ext >> 1)
    {
    case 265:		/* VSX Scalar round Double-Precision to
			   Single-Precision and Convert to
			   Single-Precision format */
    case 344:		/* VSX Scalar truncate Double-Precision to
			   Integer and Convert to Signed Integer
			   Doubleword format with Saturate */
    case 88:		/* VSX Scalar truncate Double-Precision to
			   Integer and Convert to Signed Integer Word
			   Format with Saturate */
    case 328:		/* VSX Scalar truncate Double-Precision integer
			   and Convert to Unsigned Integer Doubleword
			   Format with Saturate */
    case 72:		/* VSX Scalar truncate Double-Precision to
			   Integer and Convert to Unsigned Integer Word
			   Format with Saturate */
    case 329:		/* VSX Scalar Convert Single-Precision to
			   Double-Precision format */
    case 376:		/* VSX Scalar Convert Signed Integer
			   Doubleword to floating-point format and
			   Round to Double-Precision format */
    case 312:		/* VSX Scalar Convert Signed Integer
			   Doubleword to floating-point format and
			   round to Single-Precision */
    case 360:		/* VSX Scalar Convert Unsigned Integer
			   Doubleword to floating-point format and
			   Round to Double-Precision format */
    case 296:		/* VSX Scalar Convert Unsigned Integer
			   Doubleword to floating-point format and
			   Round to Single-Precision */
    case 73:		/* VSX Scalar Round to Double-Precision Integer
			   Using Round to Nearest Away */
    case 107:		/* VSX Scalar Round to Double-Precision Integer
			   Exact using Current rounding mode */
    case 121:		/* VSX Scalar Round to Double-Precision Integer
			   Using Round toward -Infinity */
    case 105:		/* VSX Scalar Round to Double-Precision Integer
			   Using Round toward +Infinity */
    case 89:		/* VSX Scalar Round to Double-Precision Integer
			   Using Round toward Zero */
    case 90:		/* VSX Scalar Reciprocal Estimate Double-Precision */
    case 26:		/* VSX Scalar Reciprocal Estimate Single-Precision */
    case 281:		/* VSX Scalar Round to Single-Precision */
    case 74:		/* VSX Scalar Reciprocal Square Root Estimate
			   Double-Precision */
    case 10:		/* VSX Scalar Reciprocal Square Root Estimate
			   Single-Precision */
    case 75:		/* VSX Scalar Square Root Double-Precision */
    case 11:		/* VSX Scalar Square Root Single-Precision */
    case 393:		/* VSX Vector round Double-Precision to
			   Single-Precision and Convert to
			   Single-Precision format */
    case 472:		/* VSX Vector truncate Double-Precision to
			   Integer and Convert to Signed Integer
			   Doubleword format with Saturate */
    case 216:		/* VSX Vector truncate Double-Precision to
			   Integer and Convert to Signed Integer Word
			   Format with Saturate */
    case 456:		/* VSX Vector truncate Double-Precision to
			   Integer and Convert to Unsigned Integer
			   Doubleword format with Saturate */
    case 200:		/* VSX Vector truncate Double-Precision to
			   Integer and Convert to Unsigned Integer Word
			   Format with Saturate */
    case 457:		/* VSX Vector Convert Single-Precision to
			   Double-Precision format */
    case 408:		/* VSX Vector truncate Single-Precision to
			   Integer and Convert to Signed Integer
			   Doubleword format with Saturate */
    case 152:		/* VSX Vector truncate Single-Precision to
			   Integer and Convert to Signed Integer Word
			   Format with Saturate */
    case 392:		/* VSX Vector truncate Single-Precision to
			   Integer and Convert to Unsigned Integer
			   Doubleword format with Saturate */
    case 136:		/* VSX Vector truncate Single-Precision to
			   Integer and Convert to Unsigned Integer Word
			   Format with Saturate */
    case 504:		/* VSX Vector Convert and round Signed Integer
			   Doubleword to Double-Precision format */
    case 440:		/* VSX Vector Convert and round Signed Integer
			   Doubleword to Single-Precision format */
    case 248:		/* VSX Vector Convert Signed Integer Word to
			   Double-Precision format */
    case 184:		/* VSX Vector Convert and round Signed Integer
			   Word to Single-Precision format */
    case 488:		/* VSX Vector Convert and round Unsigned
			   Integer Doubleword to Double-Precision format */
    case 424:		/* VSX Vector Convert and round Unsigned
			   Integer Doubleword to Single-Precision format */
    case 232:		/* VSX Vector Convert and round Unsigned
			   Integer Word to Double-Precision format */
    case 168:		/* VSX Vector Convert and round Unsigned
			   Integer Word to Single-Precision format */
    case 201:		/* VSX Vector Round to Double-Precision
			   Integer using round to Nearest Away */
    case 235:		/* VSX Vector Round to Double-Precision
			   Integer Exact using Current rounding mode */
    case 249:		/* VSX Vector Round to Double-Precision
			   Integer using round toward -Infinity */
    case 233:		/* VSX Vector Round to Double-Precision
			   Integer using round toward +Infinity */
    case 217:		/* VSX Vector Round to Double-Precision
			   Integer using round toward Zero */
    case 218:		/* VSX Vector Reciprocal Estimate Double-Precision */
    case 154:		/* VSX Vector Reciprocal Estimate Single-Precision */
    case 137:		/* VSX Vector Round to Single-Precision Integer
			   Using Round to Nearest Away */
    case 171:		/* VSX Vector Round to Single-Precision Integer
			   Exact Using Current rounding mode */
    case 185:		/* VSX Vector Round to Single-Precision Integer
			   Using Round toward -Infinity */
    case 169:		/* VSX Vector Round to Single-Precision Integer
			   Using Round toward +Infinity */
    case 153:		/* VSX Vector Round to Single-Precision Integer
			   Using round toward Zero */
    case 202:		/* VSX Vector Reciprocal Square Root Estimate
			   Double-Precision */
    case 138:		/* VSX Vector Reciprocal Square Root Estimate
			   Single-Precision */
    case 203:		/* VSX Vector Square Root Double-Precision */
    case 139:		/* VSX Vector Square Root Single-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 345:		/* VSX Scalar Absolute Value Double-Precision */
    case 267:		/* VSX Scalar Convert Scalar Single-Precision to
			   Vector Single-Precision format Non-signalling */
    case 331:		/* VSX Scalar Convert Single-Precision to
			   Double-Precision format Non-signalling */
    case 361:		/* VSX Scalar Negative Absolute Value Double-Precision */
    case 377:		/* VSX Scalar Negate Double-Precision */
    case 473:		/* VSX Vector Absolute Value Double-Precision */
    case 409:		/* VSX Vector Absolute Value Single-Precision */
    case 489:		/* VSX Vector Negative Absolute Value Double-Precision */
    case 425:		/* VSX Vector Negative Absolute Value Single-Precision */
    case 505:		/* VSX Vector Negate Double-Precision */
    case 441:		/* VSX Vector Negate Single-Precision */
    case 164:		/* VSX Splat Word */
    case 165:		/* VSX Vector Extract Unsigned Word */
    case 181:		/* VSX Vector Insert Word */
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;

    case 298:		/* VSX Scalar Test Data Class Single-Precision */
    case 362:		/* VSX Scalar Test Data Class Double-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 106:		/* VSX Scalar Test for software Square Root
			   Double-Precision */
    case 234:		/* VSX Vector Test for software Square Root
			   Double-Precision */
    case 170:		/* VSX Vector Test for software Square Root
			   Single-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 347:
      switch (PPC_FIELD (insn, 11, 5))
	{
	case 0:		/* VSX Scalar Extract Exponent Double-Precision */
	case 1:		/* VSX Scalar Extract Significand Double-Precision */
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_gp0_regnum + PPC_RT (insn));
	  return 0;
	case 16:	/* VSX Scalar Convert Half-Precision format to
			   Double-Precision format */
	case 17:	/* VSX Scalar round & Convert Double-Precision format
			   to Half-Precision format */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
	  ppc_record_vsr (regcache, tdep, PPC_XT (insn));
	  return 0;
	}
      break;

    case 475:
      if (ppc_process_record_op60_XX2 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      return 0;
    }

  switch (ext)
    {
    case 360:
      if (PPC_FIELD (insn, 11, 2) == 0)  /* VSX Vector Splat Immediate Byte */
	{
	  ppc_record_vsr (regcache, tdep, PPC_XT (insn));
	  return 0;
	}
      if (PPC_FIELD (insn, 11, 5) == 31)  /* Load VSX Vector Special Value
					     Quadword */
	{
	  ppc_record_vsr (regcache, tdep, PPC_XT (insn));
	  return 0;
	}
      break;
    case 916:		/* VSX Vector Generate PCV from Byte Mask */
    case 917:		/* VSX Vector Generate PCV from Halfword Mask */
    case 948:		/* VSX Vector Generate PCV from Word Mask */
    case 949:		/* VSX Vector Generate PCV from Doubleword Mask */
    case 918:		/* VSX Scalar Insert Exponent Double-Precision */
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;
    }

  if (((ext >> 3) & 0x3) == 3)	/* VSX Select */
    {
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;
    }

  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s, 60-%d.\n", insn, paddress (gdbarch, addr), ext);
  return -1;
}

/* Parse and record instructions of primary opcode-61 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op61 (struct gdbarch *gdbarch, struct regcache *regcache,
			   CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  ULONGEST ea = 0;
  int size;

  switch (insn & 0x3)
    {
    case 0:		/* Store Floating-Point Double Pair */
    case 2:		/* Store VSX Scalar Doubleword */
    case 3:		/* Store VSX Scalar Single */
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn),
				    &ea);
      ea += PPC_DS (insn) << 2;
      switch (insn & 0x3)
	{
	case 0:		/* Store Floating-Point Double Pair */
	  size = 16;
	  break;
	case 2:		/* Store VSX Scalar Doubleword */
	  size = 8;
	  break;
	case 3:		/* Store VSX Scalar Single */
	  size = 4;
	  break;
	default:
	  gdb_assert (0);
	}
      record_full_arch_list_add_mem (ea, size);
      return 0;
    }

  switch (insn & 0x7)
    {
    case 1:		/* Load VSX Vector */
      ppc_record_vsr (regcache, tdep, PPC_XT (insn));
      return 0;
    case 5:		/* Store VSX Vector */
      if (PPC_RA (insn) != 0)
	regcache_raw_read_unsigned (regcache,
				    tdep->ppc_gp0_regnum + PPC_RA (insn),
				    &ea);
      ea += PPC_DQ (insn) << 4;
      record_full_arch_list_add_mem (ea, 16);
      return 0;
    }

  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s.\n", insn, paddress (gdbarch, addr));
  return -1;
}

/* Parse and record instructions of primary opcode-63 at ADDR.
   Return 0 if successful.  */

static int
ppc_process_record_op63 (struct gdbarch *gdbarch, struct regcache *regcache,
			   CORE_ADDR addr, uint32_t insn)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int ext = PPC_EXTOP (insn);
  int tmp;

  switch (ext & 0x1f)
    {
    case 18:		/* Floating Divide */
    case 20:		/* Floating Subtract */
    case 21:		/* Floating Add */
    case 22:		/* Floating Square Root */
    case 24:		/* Floating Reciprocal Estimate */
    case 25:		/* Floating Multiply */
    case 26:		/* Floating Reciprocal Square Root Estimate */
    case 28:		/* Floating Multiply-Subtract */
    case 29:		/* Floating Multiply-Add */
    case 30:		/* Floating Negative Multiply-Subtract */
    case 31:		/* Floating Negative Multiply-Add */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 23:		/* Floating Select */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;
    }

  switch (ext & 0xff)
    {
    case 5:		/* VSX Scalar Round to Quad-Precision Integer */
    case 37:		/* VSX Scalar Round Quad-Precision to Double-Extended
			   Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
      return 0;
    }

  switch (ext)
    {
    case 2:		/* DFP Add Quad */
    case 3:		/* DFP Quantize Quad */
    case 34:		/* DFP Multiply Quad */
    case 35:		/* DFP Reround Quad */
    case 67:		/* DFP Quantize Immediate Quad */
    case 99:		/* DFP Round To FP Integer With Inexact Quad */
    case 227:		/* DFP Round To FP Integer Without Inexact Quad */
    case 258:		/* DFP Convert To DFP Extended Quad */
    case 514:		/* DFP Subtract Quad */
    case 546:		/* DFP Divide Quad */
    case 770:		/* DFP Round To DFP Long Quad */
    case 802:		/* DFP Convert From Fixed Quad */
    case 834:		/* DFP Encode BCD To DPD Quad */
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      tmp = tdep->ppc_fp0_regnum + (PPC_FRT (insn) & ~1);
      record_full_arch_list_add_reg (regcache, tmp);
      record_full_arch_list_add_reg (regcache, tmp + 1);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 130:		/* DFP Compare Ordered Quad */
    case 162:		/* DFP Test Exponent Quad */
    case 194:		/* DFP Test Data Class Quad */
    case 226:		/* DFP Test Data Group Quad */
    case 642:		/* DFP Compare Unordered Quad */
    case 674:		/* DFP Test Significance Quad */
    case 675:		/* DFP Test Significance Immediate Quad */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 66:		/* DFP Shift Significand Left Immediate Quad */
    case 98:		/* DFP Shift Significand Right Immediate Quad */
    case 322:		/* DFP Decode DPD To BCD Quad */
    case 866:		/* DFP Insert Biased Exponent Quad */
      tmp = tdep->ppc_fp0_regnum + (PPC_FRT (insn) & ~1);
      record_full_arch_list_add_reg (regcache, tmp);
      record_full_arch_list_add_reg (regcache, tmp + 1);
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 290:		/* DFP Convert To Fixed Quad */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 354:		/* DFP Extract Biased Exponent Quad */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 12:		/* Floating Round to Single-Precision */
    case 14:		/* Floating Convert To Integer Word */
    case 15:		/* Floating Convert To Integer Word
			   with round toward Zero */
    case 142:		/* Floating Convert To Integer Word Unsigned */
    case 143:		/* Floating Convert To Integer Word Unsigned
			   with round toward Zero */
    case 392:		/* Floating Round to Integer Nearest */
    case 424:		/* Floating Round to Integer Toward Zero */
    case 456:		/* Floating Round to Integer Plus */
    case 488:		/* Floating Round to Integer Minus */
    case 814:		/* Floating Convert To Integer Doubleword */
    case 815:		/* Floating Convert To Integer Doubleword
			   with round toward Zero */
    case 846:		/* Floating Convert From Integer Doubleword */
    case 942:		/* Floating Convert To Integer Doubleword Unsigned */
    case 943:		/* Floating Convert To Integer Doubleword Unsigned
			   with round toward Zero */
    case 974:		/* Floating Convert From Integer Doubleword Unsigned */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 583:
      switch (PPC_FIELD (insn, 11, 5))
	{
	  case 1:	/* Move From FPSCR & Clear Enables */
	  case 20:	/* Move From FPSCR Control & set DRN */
	  case 21:	/* Move From FPSCR Control & set DRN Immediate */
	  case 22:	/* Move From FPSCR Control & set RN */
	  case 23:	/* Move From FPSCR Control & set RN Immediate */
	    record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
	    [[fallthrough]];
	  case 0:	/* Move From FPSCR */
	  case 24:	/* Move From FPSCR Lightweight */
	    if (PPC_FIELD (insn, 11, 5) == 0 && PPC_RC (insn))
	      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
	    record_full_arch_list_add_reg (regcache,
					   tdep->ppc_fp0_regnum
					   + PPC_FRT (insn));
	    return 0;
	}
      break;

    case 8:		/* Floating Copy Sign */
    case 40:		/* Floating Negate */
    case 72:		/* Floating Move Register */
    case 136:		/* Floating Negative Absolute Value */
    case 264:		/* Floating Absolute Value */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 838:		/* Floating Merge Odd Word */
    case 966:		/* Floating Merge Even Word */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      return 0;

    case 38:		/* Move To FPSCR Bit 1 */
    case 70:		/* Move To FPSCR Bit 0 */
    case 134:		/* Move To FPSCR Field Immediate */
    case 711:		/* Move To FPSCR Fields */
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      return 0;

    case 0:		/* Floating Compare Unordered */
    case 32:		/* Floating Compare Ordered */
    case 64:		/* Move to Condition Register from FPSCR */
    case 132:		/* VSX Scalar Compare Ordered Quad-Precision */
    case 164:		/* VSX Scalar Compare Exponents Quad-Precision */
    case 644:		/* VSX Scalar Compare Unordered Quad-Precision */
    case 708:		/* VSX Scalar Test Data Class Quad-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 128:		/* Floating Test for software Divide */
    case 160:		/* Floating Test for software Square Root */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      return 0;

    case 4:		/* VSX Scalar Add Quad-Precision */
    case 36:		/* VSX Scalar Multiply Quad-Precision */
    case 388:		/* VSX Scalar Multiply-Add Quad-Precision */
    case 420:		/* VSX Scalar Multiply-Subtract Quad-Precision */
    case 452:		/* VSX Scalar Negative Multiply-Add Quad-Precision */
    case 484:		/* VSX Scalar Negative Multiply-Subtract
			   Quad-Precision */
    case 516:		/* VSX Scalar Subtract Quad-Precision */
    case 548:		/* VSX Scalar Divide Quad-Precision */
    case 994:
      {
      switch (PPC_FIELD (insn, 11, 5))
	{
	case 0:	/* DFP Convert From Fixed Quadword Quad */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);

	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_fp0_regnum
					 + PPC_FRT (insn));
	  record_full_arch_list_add_reg (regcache,
					 tdep->ppc_fp0_regnum
					 + PPC_FRT (insn) + 1);
	  return 0;
	case 1:	/* DFP Convert To Fixed Quadword Quad */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
	  ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
	  return 0;
	}
      }

      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 68:		/* VSX Scalar Compare Equal Quad-Precision */
    case 196:		/* VSX Scalar Compare Greater Than or Equal
			   Quad-Precision */
    case 228:		/* VSX Scalar Compare Greater Than Quad-Precision */
    case 676:		/* VSX Scalar Maximum Type-C Quad-Precision */
    case 740:		/* VSX Scalar Minimum Type-C Quad-Precision */
      record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
      [[fallthrough]];
    case 100:		/* VSX Scalar Copy Sign Quad-Precision */
    case 868:		/* VSX Scalar Insert Exponent Quad-Precision */
      ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
      return 0;

    case 804:
      switch (PPC_FIELD (insn, 11, 5))
	{
	case 27:	/* VSX Scalar Square Root Quad-Precision */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
	  [[fallthrough]];
	case 0:		/* VSX Scalar Absolute Quad-Precision */
	case 2:		/* VSX Scalar Extract Exponent Quad-Precision */
	case 8:		/* VSX Scalar Negative Absolute Quad-Precision */
	case 16:	/* VSX Scalar Negate Quad-Precision */
	case 18:	/* VSX Scalar Extract Significand Quad-Precision */
	  ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
	  return 0;
	}
      break;

    case 836:
      switch (PPC_FIELD (insn, 11, 5))
	{
	case 0:		/* VSX Scalar Convert with round to zero
			   Quad-Precision to Unsigned Quadword  */
	case 1:		/* VSX Scalar truncate & Convert Quad-Precision format
			   to Unsigned Word format */
	case 2:		/* VSX Scalar Convert Unsigned Doubleword format to
			   Quad-Precision format */
	case 3:		/* VSX Scalar Convert with round
			   Unsigned Quadword to Quad-Precision  */
	case 8:		/* VSX Scalar Convert with round to zero
			   Quad-Precision to Signed Quadword  */
	case 9:		/* VSX Scalar truncate & Convert Quad-Precision format
			   to Signed Word format */
	case 10:	/* VSX Scalar Convert Signed Doubleword format to
			   Quad-Precision format */
	case 11:	/* VSX Scalar Convert with round
			   Signed Quadword to Quad-Precision */
	case 17:	/* VSX Scalar truncate & Convert Quad-Precision format
			   to Unsigned Doubleword format */
	case 20:	/* VSX Scalar round & Convert Quad-Precision format to
			   Double-Precision format */
	case 22:	/* VSX Scalar Convert Double-Precision format to
			   Quad-Precision format */
	case 25:	/* VSX Scalar truncate & Convert Quad-Precision format
			   to Signed Doubleword format */
	  record_full_arch_list_add_reg (regcache, tdep->ppc_fpscr_regnum);
	  ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
	  return 0;
	}
    }

  gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
	      "at %s, 63-%d.\n", insn, paddress (gdbarch, addr), ext);
  return -1;
}

/* Record the prefixed instructions with primary opcode 32.  The arguments are
   the first 32-bits of the instruction (insn_prefix), and the second 32-bits
   of the instruction (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_op42 (struct gdbarch *gdbarch,
				struct regcache *regcache,
				uint32_t insn_prefix, uint32_t insn_suffix)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST1 = PPC_FIELD (insn_prefix, 8, 1);

  if (ST1 != 0)
    return -1;

  switch (type)
    {
    case 0:  /* Prefixed Load VSX Scalar Doubleword, plxsd */
      ppc_record_vsr (regcache, tdep, PPC_VRT (insn_suffix) + 32);
      break;
    case 2:  /* Prefixed Load Halfword Algebraic, plha */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum
				     + PPC_RT (insn_suffix));
      break;
    default:
      return -1;
    }
  return 0;
}

/* Record the prefixed XX3-Form instructions with primary opcode 59.  The
   arguments are the first 32-bits of the instruction (insn_prefix), and the
   second 32-bits of the instruction (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_op59_XX3 (struct gdbarch *gdbarch,
				    struct regcache *regcache,
				    uint32_t insn_prefix, uint32_t insn_suffix)
{
  int opcode = PPC_FIELD (insn_suffix, 21, 8);
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST4 = PPC_FIELD (insn_prefix, 8, 4);
  int at = PPC_FIELD (insn_suffix, 6, 3);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  /* Note, the mnemonics for the pmxvf16ger*, pmxvf32ger*,pmxvf64ger*,
     pmxvi4ger8*, pmxvi8ger4* pmxvi16ger2* instructions were officially
     changed to pmdmxbf16ger*, pmdmxvf32ger*, pmdmxvf64ger*, pmdmxvi4ger8*,
     pmdmxvi8ger4*, pmdmxvi16ger* respectively.  The old mnemonics are still
     supported by the assembler as extended mnemonics.  The disassembler
     generates the new mnemonics.  */
  if (type == 3)
    {
      if (ST4 == 9)
	switch (opcode)
	  {
	  case 35:	/* Prefixed Masked VSX Vector 4-bit Signed Integer GER
			   MMIRR, pmdmxvi4ger8 (pmxvi4ger8) */
	  case 34:	/* Prefixed Masked VSX Vector 4-bit Signed Integer GER
			   MMIRR, pmdmxvi4ger8pp (pmxvi4ger8pp) */

	  case 99:	/* Prefixed Masked VSX Vector 8-bit Signed/Unsigned
			   Integer GER with Saturate Positive multiply,
			   Positive accumulate, xvi8ger4spp */

	  case 3:	/* Prefixed Masked VSX Vector 8-bit Signed/Unsigned
			   Integer GER MMIRR, pmdmxvi8ger4 (pmxvi8ger4)  */
	  case 2:	/* Prefixed Masked VSX Vector 8-bit Signed/Unsigned
			   Integer GER Positive multiply, Positive accumulate
			   MMIRR, pmdmxvi8ger4pp (pmxvi8ger4pp)  */

	  case 75:	/* Prefixed Masked VSX Vector 16-bit Signed Integer
			   GER MMIRR, pmdmxvi16ger2 (pmxvi16ger2)  */
	  case 107:	/* Prefixed Masked VSX Vector 16-bit Signed Integer
			   GER  Positive multiply, Positive accumulate,
			   pmdmxvi16ger2pp (pmxvi16ger2pp)  */

	  case 43:	/* Prefixed Masked VSX Vector 16-bit Signed Integer
			   GER with Saturation MMIRR, pmdmxvi16ger2s
			   (pmxvi16ger2s)  */
	  case 42:	/* Prefixed Masked VSX Vector 16-bit Signed Integer
			   GER with Saturation Positive multiply, Positive
			   accumulate MMIRR, pmdmxvi16ger2spp (pmxvi16ger2spp)
			*/
	    ppc_record_ACC_fpscr (regcache, tdep, at, false);
	    return 0;

	  case 19:	/* Prefixed Masked VSX Vector 16-bit Floating-Point
			   GER MMIRR, pmdmxvf16ger2 (pmxvf16ger2)  */
	  case 18:	/* Prefixed Masked VSX Vector 16-bit Floating-Point
			   GER Positive multiply, Positive accumulate MMIRR,
			   pmdmxvf16ger2pp (pmxvf16ger2pp)  */
	  case 146:	/* Prefixed Masked VSX Vector 16-bit Floating-Point
			   GER Positive multiply, Negative accumulate MMIRR,
			   pmdmxvf16ger2pn (pmxvf16ger2pn)  */
	  case 82:	/* Prefixed Masked VSX Vector 16-bit Floating-Point
			   GER Negative multiply, Positive accumulate MMIRR,
			   pmdmxvf16ger2np (pmxvf16ger2np)  */
	  case 210:	/* Prefixed Masked VSX Vector 16-bit Floating-Point
			   GER Negative multiply, Negative accumulate MMIRR,
			   pmdmxvf16ger2nn (pmxvf16ger2nn)  */

	  case 27:	/* Prefixed Masked VSX Vector 32-bit Floating-Point
			   GER MMIRR, pmdmxvf32ger (pmxvf32ger)  */
	  case 26:	/* Prefixed Masked VSX Vector 32-bit Floating-Point
			   GER Positive multiply, Positive accumulate MMIRR,
			   pmdmxvf32gerpp (pmxvf32gerpp)  */
	  case 154:	/* Prefixed Masked VSX Vector 32-bit Floating-Point
			   GER Positive multiply, Negative accumulate MMIRR,
			   pmdmxvf32gerpn (pmxvf32gerpn)  */
	  case 90:	/* Prefixed Masked VSX Vector 32-bit Floating-Point
			   GER Negative multiply, Positive accumulate MMIRR,
			   pmdmxvf32gernp (pmxvf32gernp )*/
	  case 218:	/* Prefixed Masked VSX Vector 32-bit Floating-Point
			   GER Negative multiply, Negative accumulate MMIRR,
			   pmdmxvf32gernn (pmxvf32gernn)  */

	  case 59:	/* Prefixed Masked VSX Vector 64-bit Floating-Point
			   GER MMIRR, pmdmxvf64ger (pmxvf64ger)  */
	  case 58:	/* Floating-Point GER Positive multiply, Positive
			   accumulate MMIRR, pmdmxvf64gerpp (pmxvf64gerpp)  */
	  case 186:	/* Prefixed Masked VSX Vector 64-bit Floating-Point
			   GER Positive multiply, Negative accumulate MMIRR,
			   pmdmxvf64gerpn (pmxvf64gerpn)  */
	  case 122:	/* Prefixed Masked VSX Vector 64-bit Floating-Point
			   GER Negative multiply, Positive accumulate MMIRR,
			   pmdmxvf64gernp (pmxvf64gernp)  */
	  case 250:	/* Prefixed Masked VSX Vector 64-bit Floating-Point
			   GER Negative multiply, Negative accumulate MMIRR,
			   pmdmxvf64gernn (pmxvf64gernn)  */

	  case 51:	/* Prefixed Masked VSX Vector bfloat16 GER MMIRR,
			   pmdmxvbf16ger2 (pmxvbf16ger2)  */
	  case 50:	/* Prefixed Masked VSX Vector bfloat16 GER Positive
			   multiply, Positive accumulate MMIRR,
			   pmdmxvbf16ger2pp (pmxvbf16ger2pp)  */
	  case 178:	/* Prefixed Masked VSX Vector bfloat16 GER Positive
			   multiply, Negative accumulate MMIRR,
			   pmdmxvbf16ger2pn (pmxvbf16ger2pn)  */
	  case 114:	/* Prefixed Masked VSX Vector bfloat16 GER Negative
			   multiply, Positive accumulate MMIRR,
			   pmdmxvbf16ger2np (pmxvbf16ger2np)  */
	  case 242:	/* Prefixed Masked VSX Vector bfloat16 GER Negative
			   multiply, Negative accumulate MMIRR,
			   pmdmxvbf16ger2nn (pmxvbf16ger2nn)  */
	    ppc_record_ACC_fpscr (regcache, tdep, at, true);
	    return 0;
	  }
    }
  else
    return -1;

  return 0;
}

/* Record the prefixed store instructions.  The arguments are the instruction
   address, the first 32-bits of the instruction(insn_prefix) and the following
   32-bits of the instruction (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_store (struct gdbarch *gdbarch,
				 struct regcache *regcache,
				 CORE_ADDR addr, uint32_t insn_prefix,
				 uint32_t insn_suffix)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  ULONGEST iaddr = 0;
  int size;
  int R = PPC_BIT (insn_prefix, 11);
  int op6 = PPC_OP6 (insn_suffix);

  if (R == 0)
    {
      if (PPC_RA (insn_suffix) != 0)
	regcache_raw_read_unsigned (regcache, tdep->ppc_gp0_regnum
				    + PPC_RA (insn_suffix), &iaddr);
    }
  else
    {
      iaddr = addr;     /* PC relative */
    }

  switch (op6)
    {
    case 38:
      size =  1;    /* store byte, pstb */
      break;
    case 44:
      size =  2;    /* store halfword, psth */
      break;
    case 36:
    case 52:
      size =  4;    /* store word, pstw, pstfs */
      break;
    case 54:
    case 61:
      size =  8;    /* store double word, pstd, pstfd */
      break;
    case 60:
      size = 16;    /* store quadword, pstq */
      break;
    default: return -1;
    }

  iaddr += P_PPC_D (insn_prefix, insn_suffix);
  record_full_arch_list_add_mem (iaddr, size);
  return 0;
}

/* Record the prefixed instructions with primary op code 32.  The arguments
   are the first 32-bits of the instruction (insn_prefix) and the following
   32-bits of the instruction (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_op32 (struct gdbarch *gdbarch,
				struct regcache *regcache,
				uint32_t insn_prefix, uint32_t insn_suffix)
{
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST1 = PPC_FIELD (insn_prefix, 8, 1);
  int ST4 = PPC_FIELD (insn_prefix, 8, 4);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (type == 1)
    {
      if (ST4 == 0)
	{
	  switch (PPC_FIELD (insn_suffix, 11, 3))
	    {
	    case 0:  	/* VSX Vector Splat Immediate Word 8RR, xxsplti32dx */
	      ppc_record_vsr (regcache, tdep, P_PPC_XT15 (insn_suffix));
	      return 0;
	    }

	  switch (PPC_FIELD (insn_suffix, 11, 4))
	    {
	    case 2:  	/* VSX Vector Splat Immediate Double-Precision
			   8RR, xxspltidp */
	    case 3:  	/* VSX Vector Splat Immediate Word 8RR, xxspltiw */
	      ppc_record_vsr (regcache, tdep, P_PPC_XT15 (insn_suffix));
	      return 0;
	    default:
	      return -1;
	    }
	}
      else
	return -1;

    }
  else if (type == 2)
    {
      if (ST1 == 0)  		/* Prefixed Load Word and Zero, plwz */
	record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum
				       + PPC_RT (insn_suffix));
      else
	return -1;

    }
  else
    return -1;

  return 0;
}

/* Record the prefixed instructions with primary op code 33.  The arguments
   are the first 32-bits of the instruction(insn_prefix) and the following
   32-bits of the instruction (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_op33 (struct gdbarch *gdbarch,
				struct regcache *regcache,
				uint32_t insn_prefix, uint32_t insn_suffix)
{
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST4 = PPC_FIELD (insn_prefix, 8, 4);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (type == 1)
    {
      if (ST4 == 0)
	switch (PPC_FIELD (insn_suffix, 26, 2))
	  {
	  case 0:  	/* VSX Vector Blend Variable Byte 8RR, xxblendvb */
	  case 1:  	/* VSX Vector Blend Variable Halfword, xxblendvh */
	  case 2:  	/* VSX Vector Blend Variable Word, xxblendvw */
	  case 3:  	/* VSX Vector Blend Variable Doubleword, xxblendvd */
	    ppc_record_vsr (regcache, tdep, PPC_XT (insn_suffix));
	  break;
	  default:
	    return -1;
	  }
      else
	return -1;

    }
  else
    return -1;

  return 0;
}

/* Record the prefixed instructions with primary op code 34.  The arguments
   are the first 32-bits of the instruction(insn_prefix) and the following
   32-bits of the instruction (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_op34 (struct gdbarch *gdbarch,
				struct regcache *regcache,
				uint32_t insn_prefix, uint32_t insn_suffix)
{
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST1 = PPC_FIELD (insn_prefix, 8, 1);
  int ST4 = PPC_FIELD (insn_prefix, 8, 4);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (type == 1)
    {
      if (ST4 == 0)
	switch (PPC_FIELD (insn_suffix, 26, 2))
	  {
	  case 0:  	/* VSX Vector Permute Extended 8RR, xxpermx */
	  case 1:  	/* VSX Vector Evaluate 8RR, xxeval */
	    ppc_record_vsr (regcache, tdep, P_PPC_XT (insn_suffix));
	    break;
	  default:
	    return -1;
	  }
      else
	return -1;

    }
  else if (type == 2)
    {
      if (ST1 == 0)  		/* Prefixed Load Word and Zero, plbz */
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_gp0_regnum
				       + PPC_RT (insn_suffix));
      else
	return -1;

    }
  else
    return -1;

  return 0;
}

/* Record the prefixed VSX store, form DS, instructions.  The arguments are the
   instruction address (addr), the first 32-bits of the instruction
   (insn_prefix) followed by the 32-bit instruction suffix (insn_suffix).
   Return 0 on success.  */

static int
ppc_process_record_prefix_store_vsx_ds_form (struct gdbarch *gdbarch,
					     struct regcache *regcache,
					     CORE_ADDR addr,
					     uint32_t insn_prefix,
					     uint32_t insn_suffix)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  ULONGEST ea = 0;
  int size;
  int R = PPC_BIT (insn_prefix, 11);
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST1 = PPC_FIELD (insn_prefix, 8, 1);

  if ((type == 0) && (ST1 == 0))
    {
      if (R == 0)
	{
	  if (PPC_RA (insn_suffix) != 0)
	    regcache_raw_read_unsigned (regcache,
					tdep->ppc_gp0_regnum
					+ PPC_RA (insn_suffix),
					&ea);
	}
      else
	{
	  ea = addr;     /* PC relative */
	}

      ea += P_PPC_D (insn_prefix, insn_suffix);
      switch (PPC_FIELD (insn_suffix, 0, 6))
	{
	case 46:    /* Prefixed Store VSX Scalar Doubleword, pstxsd */
	  size = 8;
	  break;
	case 47:    /* Prefixed,Store VSX Scalar Single-Precision, pstxssp */
	  size = 4;
	  break;
	default:
	  return -1;
	}
      record_full_arch_list_add_mem (ea, size);
      return 0;
  }
  else
    return -1;
}

/* Record the prefixed VSX, form D, instructions.  The arguments are the
   instruction address for PC-relative addresss (addr), the first 32-bits of
   the instruction (insn_prefix) and the following 32-bits of the instruction
   (insn_suffix).  Return 0 on success.  */

static int
ppc_process_record_prefix_vsx_d_form (struct gdbarch *gdbarch,
				      struct regcache *regcache,
				      CORE_ADDR addr,
				      uint32_t insn_prefix,
				      uint32_t insn_suffix)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  ULONGEST ea = 0;
  int size;
  int R = PPC_BIT (insn_prefix, 11);
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST1 = PPC_FIELD (insn_prefix, 8, 1);

  if ((type == 0) && (ST1 == 0))
    {
      switch (PPC_FIELD (insn_suffix, 0, 5))
	{
	case 25:	/* Prefixed Load VSX Vector, plxv */
	  ppc_record_vsr (regcache, tdep, P_PPC_XT5 (insn_prefix));
	  return 0;
	case 27:	/* Prefixed Store VSX Vector 8LS, pstxv */
	  {
	    size = 16;
	    if (R == 0)
	      {
		if (PPC_RA (insn_suffix) != 0)
		  regcache_raw_read_unsigned (regcache,
					      tdep->ppc_gp0_regnum
					      + PPC_RA (insn_suffix),
					      &ea);
	      }
	    else
	      {
		ea = addr;     /* PC relative */
	      }

	    ea += P_PPC_D (insn_prefix, insn_suffix);
	    record_full_arch_list_add_mem (ea, size);
	    return 0;
	  }
	}
      return -1;
    }
  else
    return -1;
}

/* Parse the current instruction and record the values of the registers and
   memory that will be changed in current instruction to "record_arch_list".
   Return -1 if something wrong.  */

/* This handles the recording of the various prefix instructions.  It takes
   the instruction address, the first 32-bits of the instruction (insn_prefix)
   and the following 32-bits of the instruction (insn_suffix).  Return 0 on
   success.  */

static int
ppc_process_prefix_instruction (int insn_prefix, int insn_suffix,
				CORE_ADDR addr,	struct gdbarch *gdbarch,
				struct regcache *regcache)
{
  int type = PPC_FIELD (insn_prefix, 6, 2);
  int ST1 = PPC_FIELD (insn_prefix, 8, 1);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int op6;

  /* D-form has uses a 5-bit opcode in the instruction suffix */
  if (ppc_process_record_prefix_vsx_d_form ( gdbarch, regcache, addr,
					     insn_prefix, insn_suffix) == 0)
    goto SUCCESS;

  op6 = PPC_OP6 (insn_suffix);  /* 6-bit opcode in the instruction suffix */

  switch (op6)
    {
    case 14:		/* Prefixed Add Immediate, paddi */
      if ((type == 2) && (ST1 == 0))
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_gp0_regnum
				       + PPC_RT (insn_suffix));
      else
	goto UNKNOWN_PREFIX_OP;
      break;

    case 32:
      if (ppc_process_record_prefix_op32 (gdbarch, regcache,
					  insn_prefix, insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;

    case 33:
      if (ppc_process_record_prefix_op33 (gdbarch, regcache,
					  insn_prefix, insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;

    case 34:		/* Prefixed Load Byte and Zero, plbz */
      if (ppc_process_record_prefix_op34 (gdbarch, regcache,
					  insn_prefix, insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;
    case 40:		/* Prefixed Load Halfword and Zero, plhz */
      if ((type == 2) && (ST1 == 0))
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_gp0_regnum
				       + PPC_RT (insn_suffix));
      else
	goto UNKNOWN_PREFIX_OP;
      break;

      break;

    case 36:		/* Prefixed Store Word, pstw */
    case 38:		/* Prefixed Store Byte, pstb */
    case 44:		/* Prefixed Store Halfword, psth */
    case 52:		/* Prefixed Store Floating-Point Single, pstfs */
    case 54:		/* Prefixed Store Floating-Point Double, pstfd */
    case 60:		/* Prefixed Store Quadword, pstq */
    case 61:		/* Prefixed Store Doubleword, pstd */
      if (ppc_process_record_prefix_store (gdbarch, regcache, addr,
					   insn_prefix, insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;

    case 42:
      if (ppc_process_record_prefix_op42 (gdbarch, regcache,
					  insn_prefix, insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;

    case 43:          /* Prefixed Load VSX Scalar Single-Precision, plxssp */
      if ((type == 0) && (ST1 == 0))
	  ppc_record_vsr (regcache, tdep, PPC_VRT (insn_suffix) + 32);
      else
	  goto UNKNOWN_PREFIX_OP;
      break;

    case 46:
    case 47:
      if (ppc_process_record_prefix_store_vsx_ds_form (gdbarch, regcache, addr,
					       insn_prefix, insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;

    case 56:		/* Prefixed Load Quadword, plq */
      {
	if ((type == 0) && (ST1 == 0))
	  {
	    int tmp;
	    tmp = tdep->ppc_gp0_regnum + (PPC_RT (insn_suffix) & ~1);
	    record_full_arch_list_add_reg (regcache, tmp);
	    record_full_arch_list_add_reg (regcache, tmp + 1);
	  }
	else
	  goto UNKNOWN_PREFIX_OP;
	break;
      }

    case 41:		/* Prefixed Load Word Algebraic, plwa */
    case 57:  		/* Prefixed Load Doubleword, pld */
      if ((type == 0) && (ST1 == 0))
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_gp0_regnum
				       + PPC_RT (insn_suffix));
      else
	goto UNKNOWN_PREFIX_OP;
      break;

    case 48:		/* Prefixed Load Floating-Point Single, plfs */
    case 50:		/* Prefixed Load Floating-Point Double, plfd */
      if ((type == 2) && (ST1 == 0))
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_fp0_regnum
				       + PPC_FRT (insn_suffix));
      else
	goto UNKNOWN_PREFIX_OP;
      break;

    case 58:		/* Prefixed Load VSX Vector Paired, plxvp */
      if ((type == 0) && (ST1 == 0))
	{
	  ppc_record_vsr (regcache, tdep, PPC_XTp (insn_suffix));
	  ppc_record_vsr (regcache, tdep, PPC_XTp (insn_suffix) + 1);
	}
      else
	goto UNKNOWN_PREFIX_OP;
      break;

    case 59:
      if (ppc_process_record_prefix_op59_XX3 (gdbarch, regcache, insn_prefix,
					      insn_suffix) != 0)
	goto UNKNOWN_PREFIX_OP;
      break;

    case 62:	    /* Prefixed Store VSX Vector Paired 8LS, pstxvp */
      if ((type == 0) && (ST1 == 0))
	{
	  int R = PPC_BIT (insn_prefix, 11);
	  CORE_ADDR ea = 0;

	  if (R == 0)
	    {
	      if (PPC_RA (insn_suffix) != 0)
		regcache_raw_read_unsigned (regcache,
					    tdep->ppc_gp0_regnum
					    + PPC_RA (insn_suffix), &ea);
	    }
	  else
	    {
	      ea = addr;     /* PC relative */
	    }

	  ea += P_PPC_D (insn_prefix, insn_suffix) << 4;
	  record_full_arch_list_add_mem (ea, 32);
	}
      else
	goto UNKNOWN_PREFIX_OP;
      break;

    default:
UNKNOWN_PREFIX_OP:
      gdb_printf (gdb_stdlog,
		  "Warning: Don't know how to record prefix instruction "
		  "%08x %08x at %s, %d.\n",
		  insn_prefix, insn_suffix, paddress (gdbarch, addr),
		  op6);
      return -1;
    }

 SUCCESS:
  if (record_full_arch_list_add_reg (regcache, PPC_PC_REGNUM))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;
  return 0;
}

int
ppc_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		      CORE_ADDR addr)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  uint32_t insn, insn_suffix;
  int op6, tmp, i;

  insn = read_memory_unsigned_integer (addr, 4, byte_order);
  op6 = PPC_OP6 (insn);

  switch (op6)
    {
    case 1:		/* prefixed instruction */
      {
	/* Get the lower 32-bits of the prefixed instruction. */
	insn_suffix = read_memory_unsigned_integer (addr+4, 4, byte_order);
	return ppc_process_prefix_instruction (insn, insn_suffix, addr,
					       gdbarch, regcache);
      }
    case 2:		/* Trap Doubleword Immediate */
    case 3:		/* Trap Word Immediate */
      /* Do nothing.  */
      break;

    case 4:             /* Vector Integer, Compare, Logical, Shift, etc.  */
      if (ppc_process_record_op4 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 6:             /* Vector Load and Store */
      if (ppc_process_record_op6 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 17:		/* System call */
      if (PPC_LEV (insn) != 0)
	goto UNKNOWN_OP;

      if (tdep->ppc_syscall_record != NULL)
	{
	  if (tdep->ppc_syscall_record (regcache) != 0)
	    return -1;
	}
      else
	{
	  gdb_printf (gdb_stderr, _("no syscall record support\n"));
	  return -1;
	}
      break;

    case 7:		/* Multiply Low Immediate */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      break;

    case 8:		/* Subtract From Immediate Carrying */
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      break;

    case 10:		/* Compare Logical Immediate  */
    case 11:		/* Compare Immediate */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      break;

    case 13:		/* Add Immediate Carrying and Record */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      [[fallthrough]];
    case 12:		/* Add Immediate Carrying */
      record_full_arch_list_add_reg (regcache, tdep->ppc_xer_regnum);
      [[fallthrough]];
    case 14:		/* Add Immediate */
    case 15:		/* Add Immediate Shifted */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      break;

    case 16:		/* Branch Conditional */
      if ((PPC_BO (insn) & 0x4) == 0)
	record_full_arch_list_add_reg (regcache, tdep->ppc_ctr_regnum);
      [[fallthrough]];
    case 18:		/* Branch */
      if (PPC_LK (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_lr_regnum);
      break;

    case 19:
      if (ppc_process_record_op19 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 20:		/* Rotate Left Word Immediate then Mask Insert */
    case 21:		/* Rotate Left Word Immediate then AND with Mask */
    case 23:		/* Rotate Left Word then AND with Mask */
    case 30:		/* Rotate Left Doubleword Immediate then Clear Left */
			/* Rotate Left Doubleword Immediate then Clear Right */
			/* Rotate Left Doubleword Immediate then Clear */
			/* Rotate Left Doubleword then Clear Left */
			/* Rotate Left Doubleword then Clear Right */
			/* Rotate Left Doubleword Immediate then Mask Insert */
      if (PPC_RC (insn))
	record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      break;

    case 28:		/* AND Immediate */
    case 29:		/* AND Immediate Shifted */
      record_full_arch_list_add_reg (regcache, tdep->ppc_cr_regnum);
      [[fallthrough]];
    case 24:		/* OR Immediate */
    case 25:		/* OR Immediate Shifted */
    case 26:		/* XOR Immediate */
    case 27:		/* XOR Immediate Shifted */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      break;

    case 31:
      if (ppc_process_record_op31 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 33:		/* Load Word and Zero with Update */
    case 35:		/* Load Byte and Zero with Update */
    case 41:		/* Load Halfword and Zero with Update */
    case 43:		/* Load Halfword Algebraic with Update */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 32:		/* Load Word and Zero */
    case 34:		/* Load Byte and Zero */
    case 40:		/* Load Halfword and Zero */
    case 42:		/* Load Halfword Algebraic */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      break;

    case 46:		/* Load Multiple Word */
      for (i = PPC_RT (insn); i < 32; i++)
	record_full_arch_list_add_reg (regcache, tdep->ppc_gp0_regnum + i);
      break;

    case 56:		/* Load Quadword */
      tmp = tdep->ppc_gp0_regnum + (PPC_RT (insn) & ~1);
      record_full_arch_list_add_reg (regcache, tmp);
      record_full_arch_list_add_reg (regcache, tmp + 1);
      break;

    case 49:		/* Load Floating-Point Single with Update */
    case 51:		/* Load Floating-Point Double with Update */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 48:		/* Load Floating-Point Single */
    case 50:		/* Load Floating-Point Double */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_fp0_regnum + PPC_FRT (insn));
      break;

    case 47:		/* Store Multiple Word */
	{
	  ULONGEST iaddr = 0;

	  if (PPC_RA (insn) != 0)
	    regcache_raw_read_unsigned (regcache,
					tdep->ppc_gp0_regnum + PPC_RA (insn),
					&iaddr);

	  iaddr += PPC_D (insn);
	  record_full_arch_list_add_mem (iaddr, 4 * (32 - PPC_RS (insn)));
	}
      break;

    case 37:		/* Store Word with Update */
    case 39:		/* Store Byte with Update */
    case 45:		/* Store Halfword with Update */
    case 53:		/* Store Floating-Point Single with Update */
    case 55:		/* Store Floating-Point Double with Update */
      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RA (insn));
      [[fallthrough]];
    case 36:		/* Store Word */
    case 38:		/* Store Byte */
    case 44:		/* Store Halfword */
    case 52:		/* Store Floating-Point Single */
    case 54:		/* Store Floating-Point Double */
	{
	  ULONGEST iaddr = 0;
	  int size = -1;

	  if (PPC_RA (insn) != 0)
	    regcache_raw_read_unsigned (regcache,
					tdep->ppc_gp0_regnum + PPC_RA (insn),
					&iaddr);
	  iaddr += PPC_D (insn);

	  if (op6 == 36 || op6 == 37 || op6 == 52 || op6 == 53)
	    size = 4;
	  else if (op6 == 54 || op6 == 55)
	    size = 8;
	  else if (op6 == 44 || op6 == 45)
	    size = 2;
	  else if (op6 == 38 || op6 == 39)
	    size = 1;
	  else
	    gdb_assert (0);

	  record_full_arch_list_add_mem (iaddr, size);
	}
      break;

    case 57:
      switch (insn & 0x3)
	{
	case 0:		/* Load Floating-Point Double Pair */
	  tmp = tdep->ppc_fp0_regnum + (PPC_RT (insn) & ~1);
	  record_full_arch_list_add_reg (regcache, tmp);
	  record_full_arch_list_add_reg (regcache, tmp + 1);
	  break;
	case 2:		/* Load VSX Scalar Doubleword */
	case 3:		/* Load VSX Scalar Single */
	  ppc_record_vsr (regcache, tdep, PPC_VRT (insn) + 32);
	  break;
	default:
	  goto UNKNOWN_OP;
	}
      break;

    case 58:		/* Load Doubleword */
			/* Load Doubleword with Update */
			/* Load Word Algebraic */
      if (PPC_FIELD (insn, 30, 2) > 2)
	goto UNKNOWN_OP;

      record_full_arch_list_add_reg (regcache,
				     tdep->ppc_gp0_regnum + PPC_RT (insn));
      if (PPC_BIT (insn, 31))
	record_full_arch_list_add_reg (regcache,
				       tdep->ppc_gp0_regnum + PPC_RA (insn));
      break;

    case 59:
      if (ppc_process_record_op59 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 60:
      if (ppc_process_record_op60 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 61:
      if (ppc_process_record_op61 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    case 62:		/* Store Doubleword */
			/* Store Doubleword with Update */
			/* Store Quadword with Update */
	{
	  ULONGEST iaddr = 0;
	  int size;
	  int sub2 = PPC_FIELD (insn, 30, 2);

	  if (sub2 > 2)
	    goto UNKNOWN_OP;

	  if (PPC_RA (insn) != 0)
	    regcache_raw_read_unsigned (regcache,
					tdep->ppc_gp0_regnum + PPC_RA (insn),
					&iaddr);

	  size = (sub2 == 2) ? 16 : 8;

	  iaddr += PPC_DS (insn) << 2;
	  record_full_arch_list_add_mem (iaddr, size);

	  if (op6 == 62 && sub2 == 1)
	    record_full_arch_list_add_reg (regcache,
					   tdep->ppc_gp0_regnum +
					   PPC_RA (insn));

	  break;
	}

    case 63:
      if (ppc_process_record_op63 (gdbarch, regcache, addr, insn) != 0)
	return -1;
      break;

    default:
UNKNOWN_OP:
      gdb_printf (gdb_stdlog, "Warning: Don't know how to record %08x "
		  "at %s, %d.\n", insn, paddress (gdbarch, addr), op6);
      return -1;
    }

  if (record_full_arch_list_add_reg (regcache, PPC_PC_REGNUM))
    return -1;
  if (record_full_arch_list_add_end ())
    return -1;
  return 0;
}

/* Used for matching tw, twi, td and tdi instructions for POWER.  */

static constexpr uint32_t TX_INSN_MASK = 0xFC0007FF;
static constexpr uint32_t TW_INSN = 0x7C000008;
static constexpr uint32_t TD_INSN = 0x7C000088;

static constexpr uint32_t TXI_INSN_MASK = 0xFC000000;
static constexpr uint32_t TWI_INSN = 0x0C000000;
static constexpr uint32_t TDI_INSN = 0x08000000;

static inline bool
is_tw_insn (uint32_t insn)
{
  return (insn & TX_INSN_MASK) == TW_INSN;
}

static inline bool
is_twi_insn (uint32_t insn)
{
  return (insn & TXI_INSN_MASK) == TWI_INSN;
}

static inline bool
is_td_insn (uint32_t insn)
{
  return (insn & TX_INSN_MASK) == TD_INSN;
}

static inline bool
is_tdi_insn (uint32_t insn)
{
  return (insn & TXI_INSN_MASK) == TDI_INSN;
}

/* Implementation of gdbarch_program_breakpoint_here_p for POWER.  */

static bool
rs6000_program_breakpoint_here_p (gdbarch *gdbarch, CORE_ADDR address)
{
  gdb_byte target_mem[PPC_INSN_SIZE];

  /* Enable the automatic memory restoration from breakpoints while
     we read the memory.  Otherwise we may find temporary breakpoints, ones
     inserted by GDB, and flag them as permanent breakpoints.  */
  scoped_restore restore_memory
    = make_scoped_restore_show_memory_breakpoints (0);

  if (target_read_memory (address, target_mem, PPC_INSN_SIZE) == 0)
    {
      uint32_t insn = (uint32_t) extract_unsigned_integer
	(target_mem, PPC_INSN_SIZE, gdbarch_byte_order_for_code (gdbarch));

      /* Check if INSN is a TW, TWI, TD or TDI instruction.  There
	 are multiple choices of such instructions with different registers
	 and / or immediate values but they all cause a break. */
      if (is_tw_insn (insn) || is_twi_insn (insn) || is_td_insn (insn)
	  || is_tdi_insn (insn))
	return true;
    }

  return false;
}

/* Implement the update_call_site_pc arch hook.  */

static CORE_ADDR
ppc64_update_call_site_pc (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  /* Some versions of GCC emit:

     .  bl function
     .  nop
     .  ...

     but emit DWARF where the DW_AT_call_return_pc points to
     instruction after the 'nop'.  Note that while the compiler emits
     a 'nop', the linker might put some other instruction there -- so
     we just unconditionally check the next instruction.  */
  return pc + 4;
}

/* Initialize the current architecture based on INFO.  If possible, re-use an
   architecture from ARCHES, which is a list of architectures already created
   during this debugging session.

   Called e.g. at program startup, when reading a core file, and when reading
   a binary file.  */

static struct gdbarch *
rs6000_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int wordsize, from_xcoff_exec, from_elf_exec;
  enum bfd_architecture arch;
  unsigned long mach;
  bfd abfd;
  enum auto_boolean soft_float_flag = powerpc_soft_float_global;
  int soft_float;
  enum powerpc_long_double_abi long_double_abi = POWERPC_LONG_DOUBLE_AUTO;
  enum powerpc_vector_abi vector_abi = powerpc_vector_abi_global;
  enum powerpc_elf_abi elf_abi = POWERPC_ELF_AUTO;
  int have_fpu = 0, have_spe = 0, have_mq = 0, have_altivec = 0;
  int have_dfp = 0, have_vsx = 0, have_ppr = 0, have_dscr = 0;
  int have_tar = 0, have_ebb = 0, have_pmu = 0, have_htm_spr = 0;
  int have_htm_core = 0, have_htm_fpu = 0, have_htm_altivec = 0;
  int have_htm_vsx = 0, have_htm_ppr = 0, have_htm_dscr = 0;
  int have_htm_tar = 0;
  int tdesc_wordsize = -1;
  const struct target_desc *tdesc = info.target_desc;
  tdesc_arch_data_up tdesc_data;
  int num_pseudoregs = 0;
  int cur_reg;

  from_xcoff_exec = info.abfd && info.abfd->format == bfd_object &&
    bfd_get_flavour (info.abfd) == bfd_target_xcoff_flavour;

  from_elf_exec = info.abfd && info.abfd->format == bfd_object &&
    bfd_get_flavour (info.abfd) == bfd_target_elf_flavour;

  /* Check word size.  If INFO is from a binary file, infer it from
     that, else choose a likely default.  */
  if (from_xcoff_exec)
    {
      if (bfd_xcoff_is_xcoff64 (info.abfd))
	wordsize = 8;
      else
	wordsize = 4;
    }
  else if (from_elf_exec)
    {
      if (elf_elfheader (info.abfd)->e_ident[EI_CLASS] == ELFCLASS64)
	wordsize = 8;
      else
	wordsize = 4;
    }
  else if (tdesc_has_registers (tdesc))
    wordsize = -1;
  else
    {
      if (info.bfd_arch_info != NULL && info.bfd_arch_info->bits_per_word != 0)
	wordsize = (info.bfd_arch_info->bits_per_word
		    / info.bfd_arch_info->bits_per_byte);
      else
	wordsize = 4;
    }

  /* Get the architecture and machine from the BFD.  */
  arch = info.bfd_arch_info->arch;
  mach = info.bfd_arch_info->mach;

  /* For e500 executables, the apuinfo section is of help here.  Such
     section contains the identifier and revision number of each
     Application-specific Processing Unit that is present on the
     chip.  The content of the section is determined by the assembler
     which looks at each instruction and determines which unit (and
     which version of it) can execute it.  Grovel through the section
     looking for relevant e500 APUs.  */

  if (bfd_uses_spe_extensions (info.abfd))
    {
      arch = info.bfd_arch_info->arch;
      mach = bfd_mach_ppc_e500;
      bfd_default_set_arch_mach (&abfd, arch, mach);
      info.bfd_arch_info = bfd_get_arch_info (&abfd);
    }

  /* Find a default target description which describes our register
     layout, if we do not already have one.  */
  if (! tdesc_has_registers (tdesc))
    {
      const struct ppc_variant *v;

      /* Choose variant.  */
      v = find_variant_by_arch (arch, mach);
      if (!v)
	return NULL;

      tdesc = *v->tdesc;
    }

  gdb_assert (tdesc_has_registers (tdesc));

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      static const char *const gprs[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
      };
      const struct tdesc_feature *feature;
      int i, valid_p;
      static const char *const msr_names[] = { "msr", "ps" };
      static const char *const cr_names[] = { "cr", "cnd" };
      static const char *const ctr_names[] = { "ctr", "cnt" };

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.core");
      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (i = 0; i < ppc_num_gprs; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					    i, gprs[i]);
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  PPC_PC_REGNUM, "pc");
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  PPC_LR_REGNUM, "lr");
      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					  PPC_XER_REGNUM, "xer");

      /* Allow alternate names for these registers, to accommodate GDB's
	 historic naming.  */
      valid_p &= tdesc_numbered_register_choices (feature, tdesc_data.get (),
						  PPC_MSR_REGNUM, msr_names);
      valid_p &= tdesc_numbered_register_choices (feature, tdesc_data.get (),
						  PPC_CR_REGNUM, cr_names);
      valid_p &= tdesc_numbered_register_choices (feature, tdesc_data.get (),
						  PPC_CTR_REGNUM, ctr_names);

      if (!valid_p)
	return NULL;

      have_mq = tdesc_numbered_register (feature, tdesc_data.get (),
					 PPC_MQ_REGNUM, "mq");

      tdesc_wordsize = tdesc_register_bitsize (feature, "pc") / 8;
      if (wordsize == -1)
	wordsize = tdesc_wordsize;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.fpu");
      if (feature != NULL)
	{
	  static const char *const fprs[] = {
	    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
	    "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
	    "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	    "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
	  };
	  valid_p = 1;
	  for (i = 0; i < ppc_num_fprs; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_F0_REGNUM + i, fprs[i]);
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_FPSCR_REGNUM, "fpscr");

	  if (!valid_p)
	    return NULL;
	  have_fpu = 1;

	  /* The fpscr register was expanded in isa 2.05 to 64 bits
	     along with the addition of the decimal floating point
	     facility.  */
	  if (tdesc_register_bitsize (feature, "fpscr") > 32)
	    have_dfp = 1;
	}
      else
	have_fpu = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.altivec");
      if (feature != NULL)
	{
	  static const char *const vector_regs[] = {
	    "vr0", "vr1", "vr2", "vr3", "vr4", "vr5", "vr6", "vr7",
	    "vr8", "vr9", "vr10", "vr11", "vr12", "vr13", "vr14", "vr15",
	    "vr16", "vr17", "vr18", "vr19", "vr20", "vr21", "vr22", "vr23",
	    "vr24", "vr25", "vr26", "vr27", "vr28", "vr29", "vr30", "vr31"
	  };

	  valid_p = 1;
	  for (i = 0; i < ppc_num_gprs; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_VR0_REGNUM + i,
						vector_regs[i]);
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_VSCR_REGNUM, "vscr");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_VRSAVE_REGNUM, "vrsave");

	  if (have_spe || !valid_p)
	    return NULL;
	  have_altivec = 1;
	}
      else
	have_altivec = 0;

      /* Check for POWER7 VSX registers support.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.vsx");

      if (feature != NULL)
	{
	  static const char *const vsx_regs[] = {
	    "vs0h", "vs1h", "vs2h", "vs3h", "vs4h", "vs5h",
	    "vs6h", "vs7h", "vs8h", "vs9h", "vs10h", "vs11h",
	    "vs12h", "vs13h", "vs14h", "vs15h", "vs16h", "vs17h",
	    "vs18h", "vs19h", "vs20h", "vs21h", "vs22h", "vs23h",
	    "vs24h", "vs25h", "vs26h", "vs27h", "vs28h", "vs29h",
	    "vs30h", "vs31h"
	  };

	  valid_p = 1;

	  for (i = 0; i < ppc_num_vshrs; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_VSR0_UPPER_REGNUM + i,
						vsx_regs[i]);

	  if (!valid_p || !have_fpu || !have_altivec)
	    return NULL;

	  have_vsx = 1;
	}
      else
	have_vsx = 0;

      /* On machines supporting the SPE APU, the general-purpose registers
	 are 64 bits long.  There are SIMD vector instructions to treat them
	 as pairs of floats, but the rest of the instruction set treats them
	 as 32-bit registers, and only operates on their lower halves.

	 In the GDB regcache, we treat their high and low halves as separate
	 registers.  The low halves we present as the general-purpose
	 registers, and then we have pseudo-registers that stitch together
	 the upper and lower halves and present them as pseudo-registers.

	 Thus, the target description is expected to supply the upper
	 halves separately.  */

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.spe");
      if (feature != NULL)
	{
	  static const char *const upper_spe[] = {
	    "ev0h", "ev1h", "ev2h", "ev3h",
	    "ev4h", "ev5h", "ev6h", "ev7h",
	    "ev8h", "ev9h", "ev10h", "ev11h",
	    "ev12h", "ev13h", "ev14h", "ev15h",
	    "ev16h", "ev17h", "ev18h", "ev19h",
	    "ev20h", "ev21h", "ev22h", "ev23h",
	    "ev24h", "ev25h", "ev26h", "ev27h",
	    "ev28h", "ev29h", "ev30h", "ev31h"
	  };

	  valid_p = 1;
	  for (i = 0; i < ppc_num_gprs; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_SPE_UPPER_GP0_REGNUM + i,
						upper_spe[i]);
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_SPE_ACC_REGNUM, "acc");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_SPE_FSCR_REGNUM, "spefscr");

	  if (have_mq || have_fpu || !valid_p)
	    return NULL;
	  have_spe = 1;
	}
      else
	have_spe = 0;

      /* Program Priority Register.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.ppr");
      if (feature != NULL)
	{
	  valid_p = 1;
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_PPR_REGNUM, "ppr");

	  if (!valid_p)
	    return NULL;
	  have_ppr = 1;
	}
      else
	have_ppr = 0;

      /* Data Stream Control Register.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.dscr");
      if (feature != NULL)
	{
	  valid_p = 1;
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_DSCR_REGNUM, "dscr");

	  if (!valid_p)
	    return NULL;
	  have_dscr = 1;
	}
      else
	have_dscr = 0;

      /* Target Address Register.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.tar");
      if (feature != NULL)
	{
	  valid_p = 1;
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_TAR_REGNUM, "tar");

	  if (!valid_p)
	    return NULL;
	  have_tar = 1;
	}
      else
	have_tar = 0;

      /* Event-based Branching Registers.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.ebb");
      if (feature != NULL)
	{
	  static const char *const ebb_regs[] = {
	    "bescr", "ebbhr", "ebbrr"
	  };

	  valid_p = 1;
	  for (i = 0; i < ARRAY_SIZE (ebb_regs); i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_BESCR_REGNUM + i,
						ebb_regs[i]);
	  if (!valid_p)
	    return NULL;
	  have_ebb = 1;
	}
      else
	have_ebb = 0;

      /* Subset of the ISA 2.07 Performance Monitor Registers provided
	 by Linux.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.linux.pmu");
      if (feature != NULL)
	{
	  valid_p = 1;

	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_MMCR0_REGNUM,
					      "mmcr0");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_MMCR2_REGNUM,
					      "mmcr2");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_SIAR_REGNUM,
					      "siar");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_SDAR_REGNUM,
					      "sdar");
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      PPC_SIER_REGNUM,
					      "sier");

	  if (!valid_p)
	    return NULL;
	  have_pmu = 1;
	}
      else
	have_pmu = 0;

      /* Hardware Transactional Memory Registers.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.spr");
      if (feature != NULL)
	{
	  static const char *const tm_spr_regs[] = {
	    "tfhar", "texasr", "tfiar"
	  };

	  valid_p = 1;
	  for (i = 0; i < ARRAY_SIZE (tm_spr_regs); i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_TFHAR_REGNUM + i,
						tm_spr_regs[i]);
	  if (!valid_p)
	    return NULL;

	  have_htm_spr = 1;
	}
      else
	have_htm_spr = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.core");
      if (feature != NULL)
	{
	  static const char *const cgprs[] = {
	    "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",
	    "cr8", "cr9", "cr10", "cr11", "cr12", "cr13", "cr14",
	    "cr15", "cr16", "cr17", "cr18", "cr19", "cr20", "cr21",
	    "cr22", "cr23", "cr24", "cr25", "cr26", "cr27", "cr28",
	    "cr29", "cr30", "cr31", "ccr", "cxer", "clr", "cctr"
	  };

	  valid_p = 1;

	  for (i = 0; i < ARRAY_SIZE (cgprs); i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_CR0_REGNUM + i,
						cgprs[i]);
	  if (!valid_p)
	    return NULL;

	  have_htm_core = 1;
	}
      else
	have_htm_core = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.fpu");
      if (feature != NULL)
	{
	  valid_p = 1;

	  static const char *const cfprs[] = {
	    "cf0", "cf1", "cf2", "cf3", "cf4", "cf5", "cf6", "cf7",
	    "cf8", "cf9", "cf10", "cf11", "cf12", "cf13", "cf14", "cf15",
	    "cf16", "cf17", "cf18", "cf19", "cf20", "cf21", "cf22",
	    "cf23", "cf24", "cf25", "cf26", "cf27", "cf28", "cf29",
	    "cf30", "cf31", "cfpscr"
	  };

	  for (i = 0; i < ARRAY_SIZE (cfprs); i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_CF0_REGNUM + i,
						cfprs[i]);

	  if (!valid_p)
	    return NULL;
	  have_htm_fpu = 1;
	}
      else
	have_htm_fpu = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.altivec");
      if (feature != NULL)
	{
	  valid_p = 1;

	  static const char *const cvmx[] = {
	    "cvr0", "cvr1", "cvr2", "cvr3", "cvr4", "cvr5", "cvr6",
	    "cvr7", "cvr8", "cvr9", "cvr10", "cvr11", "cvr12", "cvr13",
	    "cvr14", "cvr15","cvr16", "cvr17", "cvr18", "cvr19", "cvr20",
	    "cvr21", "cvr22", "cvr23", "cvr24", "cvr25", "cvr26",
	    "cvr27", "cvr28", "cvr29", "cvr30", "cvr31", "cvscr",
	    "cvrsave"
	  };

	  for (i = 0; i < ARRAY_SIZE (cvmx); i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						PPC_CVR0_REGNUM + i,
						cvmx[i]);

	  if (!valid_p)
	    return NULL;
	  have_htm_altivec = 1;
	}
      else
	have_htm_altivec = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.vsx");
      if (feature != NULL)
	{
	  valid_p = 1;

	  static const char *const cvsx[] = {
	    "cvs0h", "cvs1h", "cvs2h", "cvs3h", "cvs4h", "cvs5h",
	    "cvs6h", "cvs7h", "cvs8h", "cvs9h", "cvs10h", "cvs11h",
	    "cvs12h", "cvs13h", "cvs14h", "cvs15h", "cvs16h", "cvs17h",
	    "cvs18h", "cvs19h", "cvs20h", "cvs21h", "cvs22h", "cvs23h",
	    "cvs24h", "cvs25h", "cvs26h", "cvs27h", "cvs28h", "cvs29h",
	    "cvs30h", "cvs31h"
	  };

	  for (i = 0; i < ARRAY_SIZE (cvsx); i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						(PPC_CVSR0_UPPER_REGNUM
						 + i),
						cvsx[i]);

	  if (!valid_p || !have_htm_fpu || !have_htm_altivec)
	    return NULL;
	  have_htm_vsx = 1;
	}
      else
	have_htm_vsx = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.ppr");
      if (feature != NULL)
	{
	  valid_p = tdesc_numbered_register (feature, tdesc_data.get (),
					     PPC_CPPR_REGNUM, "cppr");

	  if (!valid_p)
	    return NULL;
	  have_htm_ppr = 1;
	}
      else
	have_htm_ppr = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.dscr");
      if (feature != NULL)
	{
	  valid_p = tdesc_numbered_register (feature, tdesc_data.get (),
					     PPC_CDSCR_REGNUM, "cdscr");

	  if (!valid_p)
	    return NULL;
	  have_htm_dscr = 1;
	}
      else
	have_htm_dscr = 0;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.power.htm.tar");
      if (feature != NULL)
	{
	  valid_p = tdesc_numbered_register (feature, tdesc_data.get (),
					     PPC_CTAR_REGNUM, "ctar");

	  if (!valid_p)
	    return NULL;
	  have_htm_tar = 1;
	}
      else
	have_htm_tar = 0;
    }

  /* If we have a 64-bit binary on a 32-bit target, complain.  Also
     complain for a 32-bit binary on a 64-bit target; we do not yet
     support that.  For instance, the 32-bit ABI routines expect
     32-bit GPRs.

     As long as there isn't an explicit target description, we'll
     choose one based on the BFD architecture and get a word size
     matching the binary (probably powerpc:common or
     powerpc:common64).  So there is only trouble if a 64-bit target
     supplies a 64-bit description while debugging a 32-bit
     binary.  */
  if (tdesc_wordsize != -1 && tdesc_wordsize != wordsize)
    return NULL;

#ifdef HAVE_ELF
  if (from_elf_exec)
    {
      switch (elf_elfheader (info.abfd)->e_flags & EF_PPC64_ABI)
	{
	case 1:
	  elf_abi = POWERPC_ELF_V1;
	  break;
	case 2:
	  elf_abi = POWERPC_ELF_V2;
	  break;
	default:
	  break;
	}
    }

  if (soft_float_flag == AUTO_BOOLEAN_AUTO && from_elf_exec)
    {
      switch (bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
					Tag_GNU_Power_ABI_FP) & 3)
	{
	case 1:
	  soft_float_flag = AUTO_BOOLEAN_FALSE;
	  break;
	case 2:
	  soft_float_flag = AUTO_BOOLEAN_TRUE;
	  break;
	default:
	  break;
	}
    }

  if (long_double_abi == POWERPC_LONG_DOUBLE_AUTO && from_elf_exec)
    {
      switch (bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
					Tag_GNU_Power_ABI_FP) >> 2)
	{
	case 1:
	  long_double_abi = POWERPC_LONG_DOUBLE_IBM128;
	  break;
	case 3:
	  long_double_abi = POWERPC_LONG_DOUBLE_IEEE128;
	  break;
	default:
	  break;
	}
    }

  if (vector_abi == POWERPC_VEC_AUTO && from_elf_exec)
    {
      switch (bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
					Tag_GNU_Power_ABI_Vector))
	{
	case 1:
	  vector_abi = POWERPC_VEC_GENERIC;
	  break;
	case 2:
	  vector_abi = POWERPC_VEC_ALTIVEC;
	  break;
	case 3:
	  vector_abi = POWERPC_VEC_SPE;
	  break;
	default:
	  break;
	}
    }
#endif

  /* At this point, the only supported ELF-based 64-bit little-endian
     operating system is GNU/Linux, and this uses the ELFv2 ABI by
     default.  All other supported ELF-based operating systems use the
     ELFv1 ABI by default.  Therefore, if the ABI marker is missing,
     e.g. because we run a legacy binary, or have attached to a process
     and have not found any associated binary file, set the default
     according to this heuristic.  */
  if (elf_abi == POWERPC_ELF_AUTO)
    {
      if (wordsize == 8 && info.byte_order == BFD_ENDIAN_LITTLE)
	elf_abi = POWERPC_ELF_V2;
      else
	elf_abi = POWERPC_ELF_V1;
    }

  if (soft_float_flag == AUTO_BOOLEAN_TRUE)
    soft_float = 1;
  else if (soft_float_flag == AUTO_BOOLEAN_FALSE)
    soft_float = 0;
  else
    soft_float = !have_fpu;

  /* If we have a hard float binary or setting but no floating point
     registers, downgrade to soft float anyway.  We're still somewhat
     useful in this scenario.  */
  if (!soft_float && !have_fpu)
    soft_float = 1;

  /* Similarly for vector registers.  */
  if (vector_abi == POWERPC_VEC_ALTIVEC && !have_altivec)
    vector_abi = POWERPC_VEC_GENERIC;

  if (vector_abi == POWERPC_VEC_SPE && !have_spe)
    vector_abi = POWERPC_VEC_GENERIC;

  if (vector_abi == POWERPC_VEC_AUTO)
    {
      if (have_altivec)
	vector_abi = POWERPC_VEC_ALTIVEC;
      else if (have_spe)
	vector_abi = POWERPC_VEC_SPE;
      else
	vector_abi = POWERPC_VEC_GENERIC;
    }

  /* Do not limit the vector ABI based on available hardware, since we
     do not yet know what hardware we'll decide we have.  Yuck!  FIXME!  */

  /* Find a candidate among extant architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* Word size in the various PowerPC bfd_arch_info structs isn't
	 meaningful, because 64-bit CPUs can run in 32-bit mode.  So, perform
	 separate word size check.  */
      ppc_gdbarch_tdep *tdep
	= gdbarch_tdep<ppc_gdbarch_tdep> (arches->gdbarch);
      if (tdep && tdep->elf_abi != elf_abi)
	continue;
      if (tdep && tdep->soft_float != soft_float)
	continue;
      if (tdep && tdep->long_double_abi != long_double_abi)
	continue;
      if (tdep && tdep->vector_abi != vector_abi)
	continue;
      if (tdep && tdep->wordsize == wordsize)
	return arches->gdbarch;
    }

  /* None found, create a new architecture from INFO, whose bfd_arch_info
     validity depends on the source:
       - executable		useless
       - rs6000_host_arch()	good
       - core file		good
       - "set arch"		trust blindly
       - GDB startup		useless but harmless */

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new ppc_gdbarch_tdep));
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  tdep->wordsize = wordsize;
  tdep->elf_abi = elf_abi;
  tdep->soft_float = soft_float;
  tdep->long_double_abi = long_double_abi;
  tdep->vector_abi = vector_abi;

  tdep->ppc_gp0_regnum = PPC_R0_REGNUM;
  tdep->ppc_toc_regnum = PPC_R0_REGNUM + 2;
  tdep->ppc_ps_regnum = PPC_MSR_REGNUM;
  tdep->ppc_cr_regnum = PPC_CR_REGNUM;
  tdep->ppc_lr_regnum = PPC_LR_REGNUM;
  tdep->ppc_ctr_regnum = PPC_CTR_REGNUM;
  tdep->ppc_xer_regnum = PPC_XER_REGNUM;
  tdep->ppc_mq_regnum = have_mq ? PPC_MQ_REGNUM : -1;

  tdep->ppc_fp0_regnum = have_fpu ? PPC_F0_REGNUM : -1;
  tdep->ppc_fpscr_regnum = have_fpu ? PPC_FPSCR_REGNUM : -1;
  tdep->ppc_vsr0_upper_regnum = have_vsx ? PPC_VSR0_UPPER_REGNUM : -1;
  tdep->ppc_vr0_regnum = have_altivec ? PPC_VR0_REGNUM : -1;
  tdep->ppc_vrsave_regnum = have_altivec ? PPC_VRSAVE_REGNUM : -1;
  tdep->ppc_ev0_upper_regnum = have_spe ? PPC_SPE_UPPER_GP0_REGNUM : -1;
  tdep->ppc_acc_regnum = have_spe ? PPC_SPE_ACC_REGNUM : -1;
  tdep->ppc_spefscr_regnum = have_spe ? PPC_SPE_FSCR_REGNUM : -1;
  tdep->ppc_ppr_regnum = have_ppr ? PPC_PPR_REGNUM : -1;
  tdep->ppc_dscr_regnum = have_dscr ? PPC_DSCR_REGNUM : -1;
  tdep->ppc_tar_regnum = have_tar ? PPC_TAR_REGNUM : -1;
  tdep->have_ebb = have_ebb;

  /* If additional pmu registers are added, care must be taken when
     setting new fields in the tdep below, to maintain compatibility
     with features that only provide some of the registers.  Currently
     gdb access to the pmu registers is only supported in linux, and
     linux only provides a subset of the pmu registers defined in the
     architecture.  */

  tdep->ppc_mmcr0_regnum = have_pmu ? PPC_MMCR0_REGNUM : -1;
  tdep->ppc_mmcr2_regnum = have_pmu ? PPC_MMCR2_REGNUM : -1;
  tdep->ppc_siar_regnum = have_pmu ? PPC_SIAR_REGNUM : -1;
  tdep->ppc_sdar_regnum = have_pmu ? PPC_SDAR_REGNUM : -1;
  tdep->ppc_sier_regnum = have_pmu ? PPC_SIER_REGNUM : -1;

  tdep->have_htm_spr = have_htm_spr;
  tdep->have_htm_core = have_htm_core;
  tdep->have_htm_fpu = have_htm_fpu;
  tdep->have_htm_altivec = have_htm_altivec;
  tdep->have_htm_vsx = have_htm_vsx;
  tdep->ppc_cppr_regnum = have_htm_ppr ? PPC_CPPR_REGNUM : -1;
  tdep->ppc_cdscr_regnum = have_htm_dscr ? PPC_CDSCR_REGNUM : -1;
  tdep->ppc_ctar_regnum = have_htm_tar ? PPC_CTAR_REGNUM : -1;

  set_gdbarch_pc_regnum (gdbarch, PPC_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, PPC_R0_REGNUM + 1);
  set_gdbarch_fp0_regnum (gdbarch, tdep->ppc_fp0_regnum);
  set_gdbarch_register_sim_regno (gdbarch, rs6000_register_sim_regno);

  /* The XML specification for PowerPC sensibly calls the MSR "msr".
     GDB traditionally called it "ps", though, so let GDB add an
     alias.  */
  set_gdbarch_ps_regnum (gdbarch, tdep->ppc_ps_regnum);

  if (wordsize == 8)
    {
      set_gdbarch_return_value (gdbarch, ppc64_sysv_abi_return_value);
      set_gdbarch_update_call_site_pc (gdbarch, ppc64_update_call_site_pc);
    }
  else
    set_gdbarch_return_value (gdbarch, ppc_sysv_abi_return_value);
  set_gdbarch_get_return_buf_addr (gdbarch, ppc_sysv_get_return_buf_addr);

  /* Set lr_frame_offset.  */
  if (wordsize == 8)
    tdep->lr_frame_offset = 16;
  else
    tdep->lr_frame_offset = 4;

  if (have_spe || have_dfp || have_altivec
      || have_vsx || have_htm_fpu || have_htm_vsx)
    {
      set_gdbarch_pseudo_register_read (gdbarch, rs6000_pseudo_register_read);
      set_gdbarch_deprecated_pseudo_register_write
	(gdbarch, rs6000_pseudo_register_write);
      set_gdbarch_ax_pseudo_register_collect (gdbarch,
	      rs6000_ax_pseudo_register_collect);
    }

  set_gdbarch_gen_return_address (gdbarch, rs6000_gen_return_address);

  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  set_gdbarch_num_regs (gdbarch, PPC_NUM_REGS);

  if (have_spe)
    num_pseudoregs += 32;
  if (have_dfp)
    num_pseudoregs += 16;
  if (have_altivec)
    num_pseudoregs += 32;
  if (have_vsx)
    /* Include both VSX and Extended FP registers.  */
    num_pseudoregs += 96;
  if (have_htm_fpu)
    num_pseudoregs += 16;
  /* Include both checkpointed VSX and EFP registers.  */
  if (have_htm_vsx)
    num_pseudoregs += 64 + 32;

  set_gdbarch_num_pseudo_regs (gdbarch, num_pseudoregs);

  set_gdbarch_ptr_bit (gdbarch, wordsize * TARGET_CHAR_BIT);
  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, wordsize * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 16 * TARGET_CHAR_BIT);
  set_gdbarch_char_signed (gdbarch, 0);

  set_gdbarch_frame_align (gdbarch, rs6000_frame_align);
  if (wordsize == 8)
    /* PPC64 SYSV.  */
    set_gdbarch_frame_red_zone_size (gdbarch, 288);

  set_gdbarch_convert_register_p (gdbarch, rs6000_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, rs6000_register_to_value);
  set_gdbarch_value_to_register (gdbarch, rs6000_value_to_register);
  set_gdbarch_value_from_register (gdbarch, rs6000_value_from_register);

  set_gdbarch_stab_reg_to_regnum (gdbarch, rs6000_stab_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, rs6000_dwarf2_reg_to_regnum);

  if (wordsize == 4)
    set_gdbarch_push_dummy_call (gdbarch, ppc_sysv_abi_push_dummy_call);
  else if (wordsize == 8)
    set_gdbarch_push_dummy_call (gdbarch, ppc64_sysv_abi_push_dummy_call);

  set_gdbarch_skip_prologue (gdbarch, rs6000_skip_prologue);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, rs6000_stack_frame_destroyed_p);
  set_gdbarch_skip_main_prologue (gdbarch, rs6000_skip_main_prologue);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       rs6000_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       rs6000_breakpoint::bp_from_kind);
  set_gdbarch_program_breakpoint_here_p (gdbarch,
					 rs6000_program_breakpoint_here_p);

  /* The value of symbols of type N_SO and N_FUN maybe null when
     it shouldn't be.  */
  set_gdbarch_sofun_address_maybe_missing (gdbarch, 1);

  /* Handles single stepping of atomic sequences.  */
  set_gdbarch_software_single_step (gdbarch, ppc_deal_with_atomic_sequence);
  
  /* Not sure on this.  FIXMEmgo */
  set_gdbarch_frame_args_skip (gdbarch, 8);

  /* Helpers for function argument information.  */
  set_gdbarch_fetch_pointer_argument (gdbarch, rs6000_fetch_pointer_argument);

  /* Trampoline.  */
  set_gdbarch_in_solib_return_trampoline
    (gdbarch, rs6000_in_solib_return_trampoline);
  set_gdbarch_skip_trampoline_code (gdbarch, rs6000_skip_trampoline_code);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_append_unwinders (gdbarch);
  dwarf2_frame_set_adjust_regnum (gdbarch, rs6000_adjust_frame_regnum);

  /* Frame handling.  */
  dwarf2_frame_set_init_reg (gdbarch, ppc_dwarf2_frame_init_reg);

  /* Setup displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					ppc_displaced_step_copy_insn);
  set_gdbarch_displaced_step_hw_singlestep (gdbarch,
					    ppc_displaced_step_hw_singlestep);
  set_gdbarch_displaced_step_fixup (gdbarch, ppc_displaced_step_fixup);
  set_gdbarch_displaced_step_prepare (gdbarch, ppc_displaced_step_prepare);
  set_gdbarch_displaced_step_finish (gdbarch, ppc_displaced_step_finish);
  set_gdbarch_displaced_step_restore_all_in_ptid
    (gdbarch, ppc_displaced_step_restore_all_in_ptid);
  set_gdbarch_displaced_step_buffer_length (gdbarch, 2 * PPC_INSN_SIZE);

  set_gdbarch_max_insn_length (gdbarch, PPC_INSN_SIZE);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  info.target_desc = tdesc;
  info.tdesc_data = tdesc_data.get ();
  gdbarch_init_osabi (info, gdbarch);

  switch (info.osabi)
    {
    case GDB_OSABI_LINUX:
    case GDB_OSABI_NETBSD:
    case GDB_OSABI_UNKNOWN:
      frame_unwind_append_unwinder (gdbarch, &rs6000_epilogue_frame_unwind);
      frame_unwind_append_unwinder (gdbarch, &rs6000_frame_unwind);
      frame_base_append_sniffer (gdbarch, rs6000_frame_base_sniffer);
      break;
    default:
      set_gdbarch_believe_pcc_promotion (gdbarch, 1);

      frame_unwind_append_unwinder (gdbarch, &rs6000_epilogue_frame_unwind);
      frame_unwind_append_unwinder (gdbarch, &rs6000_frame_unwind);
      frame_base_append_sniffer (gdbarch, rs6000_frame_base_sniffer);
    }

  set_tdesc_pseudo_register_type (gdbarch, rs6000_pseudo_register_type);
  set_tdesc_pseudo_register_reggroup_p (gdbarch,
					rs6000_pseudo_register_reggroup_p);
  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  /* Override the normal target description method to make the SPE upper
     halves anonymous.  */
  set_gdbarch_register_name (gdbarch, rs6000_register_name);

  /* Choose register numbers for all supported pseudo-registers.  */
  tdep->ppc_ev0_regnum = -1;
  tdep->ppc_dl0_regnum = -1;
  tdep->ppc_v0_alias_regnum = -1;
  tdep->ppc_vsr0_regnum = -1;
  tdep->ppc_efpr0_regnum = -1;
  tdep->ppc_cdl0_regnum = -1;
  tdep->ppc_cvsr0_regnum = -1;
  tdep->ppc_cefpr0_regnum = -1;

  cur_reg = gdbarch_num_regs (gdbarch);

  if (have_spe)
    {
      tdep->ppc_ev0_regnum = cur_reg;
      cur_reg += 32;
    }
  if (have_dfp)
    {
      tdep->ppc_dl0_regnum = cur_reg;
      cur_reg += 16;
    }
  if (have_altivec)
    {
      tdep->ppc_v0_alias_regnum = cur_reg;
      cur_reg += 32;
    }
  if (have_vsx)
    {
      tdep->ppc_vsr0_regnum = cur_reg;
      cur_reg += 64;
      tdep->ppc_efpr0_regnum = cur_reg;
      cur_reg += 32;
    }
  if (have_htm_fpu)
    {
      tdep->ppc_cdl0_regnum = cur_reg;
      cur_reg += 16;
    }
  if (have_htm_vsx)
    {
      tdep->ppc_cvsr0_regnum = cur_reg;
      cur_reg += 64;
      tdep->ppc_cefpr0_regnum = cur_reg;
      cur_reg += 32;
    }

  gdb_assert (gdbarch_num_cooked_regs (gdbarch) == cur_reg);

  /* Register the ravenscar_arch_ops.  */
  if (mach == bfd_mach_ppc_e500)
    register_e500_ravenscar_ops (gdbarch);
  else
    register_ppc_ravenscar_ops (gdbarch);

  set_gdbarch_disassembler_options (gdbarch, &powerpc_disassembler_options);
  set_gdbarch_valid_disassembler_options (gdbarch,
					  disassembler_options_powerpc ());

  return gdbarch;
}

static void
rs6000_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  if (tdep == NULL)
    return;

  /* FIXME: Dump gdbarch_tdep.  */
}

static void
powerpc_set_soft_float (const char *args, int from_tty,
			struct cmd_list_element *c)
{
  struct gdbarch_info info;

  /* Update the architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (_("could not update architecture"));
}

static void
powerpc_set_vector_abi (const char *args, int from_tty,
			struct cmd_list_element *c)
{
  int vector_abi;

  for (vector_abi = POWERPC_VEC_AUTO;
       vector_abi != POWERPC_VEC_LAST;
       vector_abi++)
    if (strcmp (powerpc_vector_abi_string,
		powerpc_vector_strings[vector_abi]) == 0)
      {
	powerpc_vector_abi_global = (enum powerpc_vector_abi) vector_abi;
	break;
      }

  if (vector_abi == POWERPC_VEC_LAST)
    internal_error (_("Invalid vector ABI accepted: %s."),
		    powerpc_vector_abi_string);

  /* Update the architecture.  */
  gdbarch_info info;
  if (!gdbarch_update_p (info))
    internal_error (_("could not update architecture"));
}

/* Show the current setting of the exact watchpoints flag.  */

static void
show_powerpc_exact_watchpoints (struct ui_file *file, int from_tty,
				struct cmd_list_element *c,
				const char *value)
{
  gdb_printf (file, _("Use of exact watchpoints is %s.\n"), value);
}

/* Read a PPC instruction from memory.  */

static unsigned int
read_insn (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  return read_memory_unsigned_integer (pc, 4, byte_order);
}

/* Return non-zero if the instructions at PC match the series
   described in PATTERN, or zero otherwise.  PATTERN is an array of
   'struct ppc_insn_pattern' objects, terminated by an entry whose
   mask is zero.

   When the match is successful, fill INSNS[i] with what PATTERN[i]
   matched.  If PATTERN[i] is optional, and the instruction wasn't
   present, set INSNS[i] to 0 (which is not a valid PPC instruction).
   INSNS should have as many elements as PATTERN, minus the terminator.
   Note that, if PATTERN contains optional instructions which aren't
   present in memory, then INSNS will have holes, so INSNS[i] isn't
   necessarily the i'th instruction in memory.  */

int
ppc_insns_match_pattern (frame_info_ptr frame, CORE_ADDR pc,
			 const struct ppc_insn_pattern *pattern,
			 unsigned int *insns)
{
  int i;
  unsigned int insn;

  for (i = 0, insn = 0; pattern[i].mask; i++)
    {
      if (insn == 0)
	insn = read_insn (frame, pc);
      insns[i] = 0;
      if ((insn & pattern[i].mask) == pattern[i].data)
	{
	  insns[i] = insn;
	  pc += 4;
	  insn = 0;
	}
      else if (!pattern[i].optional)
	return 0;
    }

  return 1;
}

/* Return the 'd' field of the d-form instruction INSN, properly
   sign-extended.  */

CORE_ADDR
ppc_insn_d_field (unsigned int insn)
{
  return ((((CORE_ADDR) insn & 0xffff) ^ 0x8000) - 0x8000);
}

/* Return the 'ds' field of the ds-form instruction INSN, with the two
   zero bits concatenated at the right, and properly
   sign-extended.  */

CORE_ADDR
ppc_insn_ds_field (unsigned int insn)
{
  return ((((CORE_ADDR) insn & 0xfffc) ^ 0x8000) - 0x8000);
}

CORE_ADDR
ppc_insn_prefix_dform (unsigned int insn1, unsigned int insn2)
{
  /* result is 34-bits  */
  return (CORE_ADDR) ((((insn1 & 0x3ffff) ^ 0x20000) - 0x20000) << 16)
    | (CORE_ADDR)(insn2 & 0xffff);
}

/* Initialization code.  */

void _initialize_rs6000_tdep ();
void
_initialize_rs6000_tdep ()
{
  gdbarch_register (bfd_arch_rs6000, rs6000_gdbarch_init, rs6000_dump_tdep);
  gdbarch_register (bfd_arch_powerpc, rs6000_gdbarch_init, rs6000_dump_tdep);

  /* Initialize the standard target descriptions.  */
  initialize_tdesc_powerpc_32 ();
  initialize_tdesc_powerpc_altivec32 ();
  initialize_tdesc_powerpc_vsx32 ();
  initialize_tdesc_powerpc_403 ();
  initialize_tdesc_powerpc_403gc ();
  initialize_tdesc_powerpc_405 ();
  initialize_tdesc_powerpc_505 ();
  initialize_tdesc_powerpc_601 ();
  initialize_tdesc_powerpc_602 ();
  initialize_tdesc_powerpc_603 ();
  initialize_tdesc_powerpc_604 ();
  initialize_tdesc_powerpc_64 ();
  initialize_tdesc_powerpc_altivec64 ();
  initialize_tdesc_powerpc_vsx64 ();
  initialize_tdesc_powerpc_7400 ();
  initialize_tdesc_powerpc_750 ();
  initialize_tdesc_powerpc_860 ();
  initialize_tdesc_powerpc_e500 ();
  initialize_tdesc_rs6000 ();

  /* Add root prefix command for all "set powerpc"/"show powerpc"
     commands.  */
  add_setshow_prefix_cmd ("powerpc", no_class,
			  _("Various PowerPC-specific commands."),
			  _("Various PowerPC-specific commands."),
			  &setpowerpccmdlist, &showpowerpccmdlist,
			  &setlist, &showlist);

  /* Add a command to allow the user to force the ABI.  */
  add_setshow_auto_boolean_cmd ("soft-float", class_support,
				&powerpc_soft_float_global,
				_("Set whether to use a soft-float ABI."),
				_("Show whether to use a soft-float ABI."),
				NULL,
				powerpc_set_soft_float, NULL,
				&setpowerpccmdlist, &showpowerpccmdlist);

  add_setshow_enum_cmd ("vector-abi", class_support, powerpc_vector_strings,
			&powerpc_vector_abi_string,
			_("Set the vector ABI."),
			_("Show the vector ABI."),
			NULL, powerpc_set_vector_abi, NULL,
			&setpowerpccmdlist, &showpowerpccmdlist);

  add_setshow_boolean_cmd ("exact-watchpoints", class_support,
			   &target_exact_watchpoints,
			   _("\
Set whether to use just one debug register for watchpoints on scalars."),
			   _("\
Show whether to use just one debug register for watchpoints on scalars."),
			   _("\
If true, GDB will use only one debug register when watching a variable of\n\
scalar type, thus assuming that the variable is accessed through the address\n\
of its first byte."),
			   NULL, show_powerpc_exact_watchpoints,
			   &setpowerpccmdlist, &showpowerpccmdlist);
}

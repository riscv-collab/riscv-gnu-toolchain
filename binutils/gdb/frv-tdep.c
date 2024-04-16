/* Target-dependent code for the Fujitsu FR-V, for GDB, the GNU Debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "gdbcore.h"
#include "arch-utils.h"
#include "regcache.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "dis-asm.h"
#include "sim-regno.h"
#include "sim/sim-frv.h"
#include "symtab.h"
#include "elf-bfd.h"
#include "elf/frv.h"
#include "osabi.h"
#include "infcall.h"
#include "solib.h"
#include "frv-tdep.h"
#include "objfiles.h"
#include "gdbarch.h"

/* Make cgen names unique to prevent ODR conflicts with other targets.  */
#define GDB_CGEN_REMAP_PREFIX frv
#include "cgen-remap.h"
#include "opcodes/frv-desc.h"

struct frv_unwind_cache		/* was struct frame_extra_info */
  {
    /* The previous frame's inner-most stack address.  Used as this
       frame ID's stack_addr.  */
    CORE_ADDR prev_sp;

    /* The frame's base, optionally used by the high-level debug info.  */
    CORE_ADDR base;

    /* Table indicating the location of each and every register.  */
    trad_frame_saved_reg *saved_regs;
  };

/* A structure describing a particular variant of the FRV.
   We allocate and initialize one of these structures when we create
   the gdbarch object for a variant.

   At the moment, all the FR variants we support differ only in which
   registers are present; the portable code of GDB knows that
   registers whose names are the empty string don't exist, so the
   `register_names' array captures all the per-variant information we
   need.

   in the future, if we need to have per-variant maps for raw size,
   virtual type, etc., we should replace register_names with an array
   of structures, each of which gives all the necessary info for one
   register.  Don't stick parallel arrays in here --- that's so
   Fortran.  */
struct frv_gdbarch_tdep : gdbarch_tdep_base
{
  /* Which ABI is in use?  */
  enum frv_abi frv_abi {};

  /* How many general-purpose registers does this variant have?  */
  int num_gprs = 0;

  /* How many floating-point registers does this variant have?  */
  int num_fprs = 0;

  /* How many hardware watchpoints can it support?  */
  int num_hw_watchpoints = 0;

  /* How many hardware breakpoints can it support?  */
  int num_hw_breakpoints = 0;

  /* Register names.  */
  const char **register_names = nullptr;
};

using frv_gdbarch_tdep_up = std::unique_ptr<frv_gdbarch_tdep>;

/* Return the FR-V ABI associated with GDBARCH.  */
enum frv_abi
frv_abi (struct gdbarch *gdbarch)
{
  frv_gdbarch_tdep *tdep = gdbarch_tdep<frv_gdbarch_tdep> (gdbarch);
  return tdep->frv_abi;
}

/* Fetch the interpreter and executable loadmap addresses (for shared
   library support) for the FDPIC ABI.  Return 0 if successful, -1 if
   not.  (E.g, -1 will be returned if the ABI isn't the FDPIC ABI.)  */
int
frv_fdpic_loadmap_addresses (struct gdbarch *gdbarch, CORE_ADDR *interp_addr,
			     CORE_ADDR *exec_addr)
{
  if (frv_abi (gdbarch) != FRV_ABI_FDPIC)
    return -1;
  else
    {
      regcache *regcache = get_thread_regcache (inferior_thread ());

      if (interp_addr != NULL)
	{
	  ULONGEST val;
	  regcache_cooked_read_unsigned (regcache,
					 fdpic_loadmap_interp_regnum, &val);
	  *interp_addr = val;
	}
      if (exec_addr != NULL)
	{
	  ULONGEST val;
	  regcache_cooked_read_unsigned (regcache,
					 fdpic_loadmap_exec_regnum, &val);
	  *exec_addr = val;
	}
      return 0;
    }
}

/* Allocate a new variant structure, and set up default values for all
   the fields.  */
static frv_gdbarch_tdep_up
new_variant ()
{
  int r;

  frv_gdbarch_tdep_up var (new frv_gdbarch_tdep);

  var->frv_abi = FRV_ABI_EABI;
  var->num_gprs = 64;
  var->num_fprs = 64;
  var->num_hw_watchpoints = 0;
  var->num_hw_breakpoints = 0;

  /* By default, don't supply any general-purpose or floating-point
     register names.  */
  var->register_names 
    = (const char **) xmalloc ((frv_num_regs + frv_num_pseudo_regs)
			       * sizeof (const char *));
  for (r = 0; r < frv_num_regs + frv_num_pseudo_regs; r++)
    var->register_names[r] = "";

  /* Do, however, supply default names for the known special-purpose
     registers.  */

  var->register_names[pc_regnum] = "pc";
  var->register_names[lr_regnum] = "lr";
  var->register_names[lcr_regnum] = "lcr";
     
  var->register_names[psr_regnum] = "psr";
  var->register_names[ccr_regnum] = "ccr";
  var->register_names[cccr_regnum] = "cccr";
  var->register_names[tbr_regnum] = "tbr";

  /* Debug registers.  */
  var->register_names[brr_regnum] = "brr";
  var->register_names[dbar0_regnum] = "dbar0";
  var->register_names[dbar1_regnum] = "dbar1";
  var->register_names[dbar2_regnum] = "dbar2";
  var->register_names[dbar3_regnum] = "dbar3";

  /* iacc0 (Only found on MB93405.)  */
  var->register_names[iacc0h_regnum] = "iacc0h";
  var->register_names[iacc0l_regnum] = "iacc0l";
  var->register_names[iacc0_regnum] = "iacc0";

  /* fsr0 (Found on FR555 and FR501.)  */
  var->register_names[fsr0_regnum] = "fsr0";

  /* acc0 - acc7.  The architecture provides for the possibility of many
     more (up to 64 total), but we don't want to make that big of a hole
     in the G packet.  If we need more in the future, we'll add them
     elsewhere.  */
  for (r = acc0_regnum; r <= acc7_regnum; r++)
    var->register_names[r]
      = xstrprintf ("acc%d", r - acc0_regnum).release ();

  /* accg0 - accg7: These are one byte registers.  The remote protocol
     provides the raw values packed four into a slot.  accg0123 and
     accg4567 correspond to accg0 - accg3 and accg4-accg7 respectively.
     We don't provide names for accg0123 and accg4567 since the user will
     likely not want to see these raw values.  */

  for (r = accg0_regnum; r <= accg7_regnum; r++)
    var->register_names[r]
      = xstrprintf ("accg%d", r - accg0_regnum).release ();

  /* msr0 and msr1.  */

  var->register_names[msr0_regnum] = "msr0";
  var->register_names[msr1_regnum] = "msr1";

  /* gner and fner registers.  */
  var->register_names[gner0_regnum] = "gner0";
  var->register_names[gner1_regnum] = "gner1";
  var->register_names[fner0_regnum] = "fner0";
  var->register_names[fner1_regnum] = "fner1";

  return var;
}


/* Indicate that the variant VAR has NUM_GPRS general-purpose
   registers, and fill in the names array appropriately.  */
static void
set_variant_num_gprs (frv_gdbarch_tdep *var, int num_gprs)
{
  int r;

  var->num_gprs = num_gprs;

  for (r = 0; r < num_gprs; ++r)
    {
      char buf[20];

      xsnprintf (buf, sizeof (buf), "gr%d", r);
      var->register_names[first_gpr_regnum + r] = xstrdup (buf);
    }
}


/* Indicate that the variant VAR has NUM_FPRS floating-point
   registers, and fill in the names array appropriately.  */
static void
set_variant_num_fprs (frv_gdbarch_tdep *var, int num_fprs)
{
  int r;

  var->num_fprs = num_fprs;

  for (r = 0; r < num_fprs; ++r)
    {
      char buf[20];

      xsnprintf (buf, sizeof (buf), "fr%d", r);
      var->register_names[first_fpr_regnum + r] = xstrdup (buf);
    }
}

static void
set_variant_abi_fdpic (frv_gdbarch_tdep *var)
{
  var->frv_abi = FRV_ABI_FDPIC;
  var->register_names[fdpic_loadmap_exec_regnum] = xstrdup ("loadmap_exec");
  var->register_names[fdpic_loadmap_interp_regnum]
    = xstrdup ("loadmap_interp");
}

static void
set_variant_scratch_registers (frv_gdbarch_tdep *var)
{
  var->register_names[scr0_regnum] = xstrdup ("scr0");
  var->register_names[scr1_regnum] = xstrdup ("scr1");
  var->register_names[scr2_regnum] = xstrdup ("scr2");
  var->register_names[scr3_regnum] = xstrdup ("scr3");
}

static const char *
frv_register_name (struct gdbarch *gdbarch, int reg)
{
  frv_gdbarch_tdep *tdep = gdbarch_tdep<frv_gdbarch_tdep> (gdbarch);
  return tdep->register_names[reg];
}


static struct type *
frv_register_type (struct gdbarch *gdbarch, int reg)
{
  if (reg >= first_fpr_regnum && reg <= last_fpr_regnum)
    return builtin_type (gdbarch)->builtin_float;
  else if (reg == iacc0_regnum)
    return builtin_type (gdbarch)->builtin_int64;
  else
    return builtin_type (gdbarch)->builtin_int32;
}

static enum register_status
frv_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			  int reg, gdb_byte *buffer)
{
  enum register_status status;

  if (reg == iacc0_regnum)
    {
      status = regcache->raw_read (iacc0h_regnum, buffer);
      if (status == REG_VALID)
	status = regcache->raw_read (iacc0l_regnum, (bfd_byte *) buffer + 4);
    }
  else if (accg0_regnum <= reg && reg <= accg7_regnum)
    {
      /* The accg raw registers have four values in each slot with the
	 lowest register number occupying the first byte.  */

      int raw_regnum = accg0123_regnum + (reg - accg0_regnum) / 4;
      int byte_num = (reg - accg0_regnum) % 4;
      gdb_byte buf[4];

      status = regcache->raw_read (raw_regnum, buf);
      if (status == REG_VALID)
	{
	  memset (buffer, 0, 4);
	  /* FR-V is big endian, so put the requested byte in the
	     first byte of the buffer allocated to hold the
	     pseudo-register.  */
	  buffer[0] = buf[byte_num];
	}
    }
  else
    gdb_assert_not_reached ("invalid pseudo register number");

  return status;
}

static void
frv_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			  int reg, const gdb_byte *buffer)
{
  if (reg == iacc0_regnum)
    {
      regcache->raw_write (iacc0h_regnum, buffer);
      regcache->raw_write (iacc0l_regnum, (bfd_byte *) buffer + 4);
    }
  else if (accg0_regnum <= reg && reg <= accg7_regnum)
    {
      /* The accg raw registers have four values in each slot with the
	 lowest register number occupying the first byte.  */

      int raw_regnum = accg0123_regnum + (reg - accg0_regnum) / 4;
      int byte_num = (reg - accg0_regnum) % 4;
      gdb_byte buf[4];

      regcache->raw_read (raw_regnum, buf);
      buf[byte_num] = ((bfd_byte *) buffer)[0];
      regcache->raw_write (raw_regnum, buf);
    }
}

static int
frv_register_sim_regno (struct gdbarch *gdbarch, int reg)
{
  static const int spr_map[] =
    {
      H_SPR_PSR,		/* psr_regnum */
      H_SPR_CCR,		/* ccr_regnum */
      H_SPR_CCCR,		/* cccr_regnum */
      -1,			/* fdpic_loadmap_exec_regnum */
      -1,			/* fdpic_loadmap_interp_regnum */
      -1,			/* 134 */
      H_SPR_TBR,		/* tbr_regnum */
      H_SPR_BRR,		/* brr_regnum */
      H_SPR_DBAR0,		/* dbar0_regnum */
      H_SPR_DBAR1,		/* dbar1_regnum */
      H_SPR_DBAR2,		/* dbar2_regnum */
      H_SPR_DBAR3,		/* dbar3_regnum */
      H_SPR_SCR0,		/* scr0_regnum */
      H_SPR_SCR1,		/* scr1_regnum */
      H_SPR_SCR2,		/* scr2_regnum */
      H_SPR_SCR3,		/* scr3_regnum */
      H_SPR_LR,			/* lr_regnum */
      H_SPR_LCR,		/* lcr_regnum */
      H_SPR_IACC0H,		/* iacc0h_regnum */
      H_SPR_IACC0L,		/* iacc0l_regnum */
      H_SPR_FSR0,		/* fsr0_regnum */
      /* FIXME: Add infrastructure for fetching/setting ACC and ACCG regs.  */
      -1,			/* acc0_regnum */
      -1,			/* acc1_regnum */
      -1,			/* acc2_regnum */
      -1,			/* acc3_regnum */
      -1,			/* acc4_regnum */
      -1,			/* acc5_regnum */
      -1,			/* acc6_regnum */
      -1,			/* acc7_regnum */
      -1,			/* acc0123_regnum */
      -1,			/* acc4567_regnum */
      H_SPR_MSR0,		/* msr0_regnum */
      H_SPR_MSR1,		/* msr1_regnum */
      H_SPR_GNER0,		/* gner0_regnum */
      H_SPR_GNER1,		/* gner1_regnum */
      H_SPR_FNER0,		/* fner0_regnum */
      H_SPR_FNER1,		/* fner1_regnum */
    };

  gdb_assert (reg >= 0 && reg < gdbarch_num_regs (gdbarch));

  if (first_gpr_regnum <= reg && reg <= last_gpr_regnum)
    return reg - first_gpr_regnum + SIM_FRV_GR0_REGNUM;
  else if (first_fpr_regnum <= reg && reg <= last_fpr_regnum)
    return reg - first_fpr_regnum + SIM_FRV_FR0_REGNUM;
  else if (pc_regnum == reg)
    return SIM_FRV_PC_REGNUM;
  else if (reg >= first_spr_regnum
	   && reg < first_spr_regnum + sizeof (spr_map) / sizeof (spr_map[0]))
    {
      int spr_reg_offset = spr_map[reg - first_spr_regnum];

      if (spr_reg_offset < 0)
	return SIM_REGNO_DOES_NOT_EXIST;
      else
	return SIM_FRV_SPR0_REGNUM + spr_reg_offset;
    }

  internal_error (_("Bad register number %d"), reg);
}

constexpr gdb_byte frv_break_insn[] = {0xc0, 0x70, 0x00, 0x01};

typedef BP_MANIPULATION (frv_break_insn) frv_breakpoint;

/* Define the maximum number of instructions which may be packed into a
   bundle (VLIW instruction).  */
static const int max_instrs_per_bundle = 8;

/* Define the size (in bytes) of an FR-V instruction.  */
static const int frv_instr_size = 4;

/* Adjust a breakpoint's address to account for the FR-V architecture's
   constraint that a break instruction must not appear as any but the
   first instruction in the bundle.  */
static CORE_ADDR
frv_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr)
{
  int count = max_instrs_per_bundle;
  CORE_ADDR addr = bpaddr - frv_instr_size;
  CORE_ADDR func_start = get_pc_function_start (bpaddr);

  /* Find the end of the previous packing sequence.  This will be indicated
     by either attempting to access some inaccessible memory or by finding
     an instruction word whose packing bit is set to one.  */
  while (count-- > 0 && addr >= func_start)
    {
      gdb_byte instr[frv_instr_size];
      int status;

      status = target_read_memory (addr, instr, sizeof instr);

      if (status != 0)
	break;

      /* This is a big endian architecture, so byte zero will have most
	 significant byte.  The most significant bit of this byte is the
	 packing bit.  */
      if (instr[0] & 0x80)
	break;

      addr -= frv_instr_size;
    }

  if (count > 0)
    bpaddr = addr + frv_instr_size;

  return bpaddr;
}


/* Return true if REG is a caller-saves ("scratch") register,
   false otherwise.  */
static int
is_caller_saves_reg (int reg)
{
  return ((4 <= reg && reg <= 7)
	  || (14 <= reg && reg <= 15)
	  || (32 <= reg && reg <= 47));
}


/* Return true if REG is a callee-saves register, false otherwise.  */
static int
is_callee_saves_reg (int reg)
{
  return ((16 <= reg && reg <= 31)
	  || (48 <= reg && reg <= 63));
}


/* Return true if REG is an argument register, false otherwise.  */
static int
is_argument_reg (int reg)
{
  return (8 <= reg && reg <= 13);
}

/* Scan an FR-V prologue, starting at PC, until frame->PC.
   If FRAME is non-zero, fill in its saved_regs with appropriate addresses.
   We assume FRAME's saved_regs array has already been allocated and cleared.
   Return the first PC value after the prologue.

   Note that, for unoptimized code, we almost don't need this function
   at all; all arguments and locals live on the stack, so we just need
   the FP to find everything.  The catch: structures passed by value
   have their addresses living in registers; they're never spilled to
   the stack.  So if you ever want to be able to get to these
   arguments in any frame but the top, you'll need to do this serious
   prologue analysis.  */
static CORE_ADDR
frv_analyze_prologue (struct gdbarch *gdbarch, CORE_ADDR pc,
		      frame_info_ptr this_frame,
		      struct frv_unwind_cache *info)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* When writing out instruction bitpatterns, we use the following
     letters to label instruction fields:
     P - The parallel bit.  We don't use this.
     J - The register number of GRj in the instruction description.
     K - The register number of GRk in the instruction description.
     I - The register number of GRi.
     S - a signed immediate offset.
     U - an unsigned immediate offset.

     The dots below the numbers indicate where hex digit boundaries
     fall, to make it easier to check the numbers.  */

  /* Non-zero iff we've seen the instruction that initializes the
     frame pointer for this function's frame.  */
  int fp_set = 0;

  /* If fp_set is non_zero, then this is the distance from
     the stack pointer to frame pointer: fp = sp + fp_offset.  */
  int fp_offset = 0;

  /* Total size of frame prior to any alloca operations.  */
  int framesize = 0;

  /* Flag indicating if lr has been saved on the stack.  */
  int lr_saved_on_stack = 0;

  /* The number of the general-purpose register we saved the return
     address ("link register") in, or -1 if we haven't moved it yet.  */
  int lr_save_reg = -1;

  /* Offset (from sp) at which lr has been saved on the stack.  */

  int lr_sp_offset = 0;

  /* If gr_saved[i] is non-zero, then we've noticed that general
     register i has been saved at gr_sp_offset[i] from the stack
     pointer.  */
  char gr_saved[64];
  int gr_sp_offset[64];

  /* The address of the most recently scanned prologue instruction.  */
  CORE_ADDR last_prologue_pc;

  /* The address of the next instruction.  */
  CORE_ADDR next_pc;

  /* The upper bound to of the pc values to scan.  */
  CORE_ADDR lim_pc;

  memset (gr_saved, 0, sizeof (gr_saved));

  last_prologue_pc = pc;

  /* Try to compute an upper limit (on how far to scan) based on the
     line number info.  */
  lim_pc = skip_prologue_using_sal (gdbarch, pc);
  /* If there's no line number info, lim_pc will be 0.  In that case,
     set the limit to be 100 instructions away from pc.  Hopefully, this
     will be far enough away to account for the entire prologue.  Don't
     worry about overshooting the end of the function.  The scan loop
     below contains some checks to avoid scanning unreasonably far.  */
  if (lim_pc == 0)
    lim_pc = pc + 400;

  /* If we have a frame, we don't want to scan past the frame's pc.  This
     will catch those cases where the pc is in the prologue.  */
  if (this_frame)
    {
      CORE_ADDR frame_pc = get_frame_pc (this_frame);
      if (frame_pc < lim_pc)
	lim_pc = frame_pc;
    }

  /* Scan the prologue.  */
  while (pc < lim_pc)
    {
      gdb_byte buf[frv_instr_size];
      LONGEST op;

      if (target_read_memory (pc, buf, sizeof buf) != 0)
	break;
      op = extract_signed_integer (buf, byte_order);

      next_pc = pc + 4;

      /* The tests in this chain of ifs should be in order of
	 decreasing selectivity, so that more particular patterns get
	 to fire before less particular patterns.  */

      /* Some sort of control transfer instruction: stop scanning prologue.
	 Integer Conditional Branch:
	  X XXXX XX 0000110 XX XXXXXXXXXXXXXXXX
	 Floating-point / media Conditional Branch:
	  X XXXX XX 0000111 XX XXXXXXXXXXXXXXXX
	 LCR Conditional Branch to LR
	  X XXXX XX 0001110 XX XX 001 X XXXXXXXXXX
	 Integer conditional Branches to LR
	  X XXXX XX 0001110 XX XX 010 X XXXXXXXXXX
	  X XXXX XX 0001110 XX XX 011 X XXXXXXXXXX
	 Floating-point/Media Branches to LR
	  X XXXX XX 0001110 XX XX 110 X XXXXXXXXXX
	  X XXXX XX 0001110 XX XX 111 X XXXXXXXXXX
	 Jump and Link
	  X XXXXX X 0001100 XXXXXX XXXXXX XXXXXX
	  X XXXXX X 0001101 XXXXXX XXXXXX XXXXXX
	 Call
	  X XXXXXX 0001111 XXXXXXXXXXXXXXXXXX
	 Return from Trap
	  X XXXXX X 0000101 XXXXXX XXXXXX XXXXXX
	 Integer Conditional Trap
	  X XXXX XX 0000100 XXXXXX XXXX 00 XXXXXX
	  X XXXX XX 0011100 XXXXXX XXXXXXXXXXXX
	 Floating-point /media Conditional Trap
	  X XXXX XX 0000100 XXXXXX XXXX 01 XXXXXX
	  X XXXX XX 0011101 XXXXXX XXXXXXXXXXXX
	 Break
	  X XXXX XX 0000100 XXXXXX XXXX 11 XXXXXX
	 Media Trap
	  X XXXX XX 0000100 XXXXXX XXXX 10 XXXXXX */
      if ((op & 0x01d80000) == 0x00180000 /* Conditional branches and Call */
	  || (op & 0x01f80000) == 0x00300000  /* Jump and Link */
	  || (op & 0x01f80000) == 0x00100000  /* Return from Trap, Trap */
	  || (op & 0x01f80000) == 0x00700000) /* Trap immediate */
	{
	  /* Stop scanning; not in prologue any longer.  */
	  break;
	}

      /* Loading something from memory into fp probably means that
	 we're in the epilogue.  Stop scanning the prologue.
	 ld @(GRi, GRk), fp
	 X 000010 0000010 XXXXXX 000100 XXXXXX
	 ldi @(GRi, d12), fp
	 X 000010 0110010 XXXXXX XXXXXXXXXXXX */
      else if ((op & 0x7ffc0fc0) == 0x04080100
	       || (op & 0x7ffc0000) == 0x04c80000)
	{
	  break;
	}

      /* Setting the FP from the SP:
	 ori sp, 0, fp
	 P 000010 0100010 000001 000000000000 = 0x04881000
	 0 111111 1111111 111111 111111111111 = 0x7fffffff
	     .    .   .    .   .    .   .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7fffffff) == 0x04881000)
	{
	  fp_set = 1;
	  fp_offset = 0;
	  last_prologue_pc = next_pc;
	}

      /* Move the link register to the scratch register grJ, before saving:
	 movsg lr, grJ
	 P 000100 0000011 010000 000111 JJJJJJ = 0x080d01c0
	 0 111111 1111111 111111 111111 000000 = 0x7fffffc0
	     .    .   .    .   .    .    .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7fffffc0) == 0x080d01c0)
	{
	  int gr_j = op & 0x3f;

	  /* If we're moving it to a scratch register, that's fine.  */
	  if (is_caller_saves_reg (gr_j))
	    {
	      lr_save_reg = gr_j;
	      last_prologue_pc = next_pc;
	    }
	}

      /* To save multiple callee-saves registers on the stack, at
	 offset zero:

	 std grK,@(sp,gr0)
	 P KKKKKK 0000011 000001 000011 000000 = 0x000c10c0
	 0 000000 1111111 111111 111111 111111 = 0x01ffffff

	 stq grK,@(sp,gr0)
	 P KKKKKK 0000011 000001 000100 000000 = 0x000c1100
	 0 000000 1111111 111111 111111 111111 = 0x01ffffff
	     .    .   .    .   .    .    .   .
	 We treat this as part of the prologue, and record the register's
	 saved address in the frame structure.  */
      else if ((op & 0x01ffffff) == 0x000c10c0
	    || (op & 0x01ffffff) == 0x000c1100)
	{
	  int gr_k = ((op >> 25) & 0x3f);
	  int ope  = ((op >> 6)  & 0x3f);
	  int count;
	  int i;

	  /* Is it an std or an stq?  */
	  if (ope == 0x03)
	    count = 2;
	  else
	    count = 4;

	  /* Is it really a callee-saves register?  */
	  if (is_callee_saves_reg (gr_k))
	    {
	      for (i = 0; i < count; i++)
		{
		  gr_saved[gr_k + i] = 1;
		  gr_sp_offset[gr_k + i] = 4 * i;
		}
	      last_prologue_pc = next_pc;
	    }
	}

      /* Adjusting the stack pointer.  (The stack pointer is GR1.)
	 addi sp, S, sp
	 P 000001 0010000 000001 SSSSSSSSSSSS = 0x02401000
	 0 111111 1111111 111111 000000000000 = 0x7ffff000
	     .    .   .    .   .    .   .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7ffff000) == 0x02401000)
	{
	  if (framesize == 0)
	    {
	      /* Sign-extend the twelve-bit field.
		 (Isn't there a better way to do this?)  */
	      int s = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;

	      framesize -= s;
	      last_prologue_pc = pc;
	    }
	  else
	    {
	      /* If the prologue is being adjusted again, we've
		 likely gone too far; i.e. we're probably in the
		 epilogue.  */
	      break;
	    }
	}

      /* Setting the FP to a constant distance from the SP:
	 addi sp, S, fp
	 P 000010 0010000 000001 SSSSSSSSSSSS = 0x04401000
	 0 111111 1111111 111111 000000000000 = 0x7ffff000
	     .    .   .    .   .    .   .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7ffff000) == 0x04401000)
	{
	  /* Sign-extend the twelve-bit field.
	     (Isn't there a better way to do this?)  */
	  int s = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;
	  fp_set = 1;
	  fp_offset = s;
	  last_prologue_pc = pc;
	}

      /* To spill an argument register to a scratch register:
	    ori GRi, 0, GRk
	 P KKKKKK 0100010 IIIIII 000000000000 = 0x00880000
	 0 000000 1111111 000000 111111111111 = 0x01fc0fff
	     .    .   .    .   .    .   .   .
	 For the time being, we treat this as a prologue instruction,
	 assuming that GRi is an argument register.  This one's kind
	 of suspicious, because it seems like it could be part of a
	 legitimate body instruction.  But we only come here when the
	 source info wasn't helpful, so we have to do the best we can.
	 Hopefully once GCC and GDB agree on how to emit line number
	 info for prologues, then this code will never come into play.  */
      else if ((op & 0x01fc0fff) == 0x00880000)
	{
	  int gr_i = ((op >> 12) & 0x3f);

	  /* Make sure that the source is an arg register; if it is, we'll
	     treat it as a prologue instruction.  */
	  if (is_argument_reg (gr_i))
	    last_prologue_pc = next_pc;
	}

      /* To spill 16-bit values to the stack:
	     sthi GRk, @(fp, s)
	 P KKKKKK 1010001 000010 SSSSSSSSSSSS = 0x01442000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
	     .    .   .    .   .    .   .   . 
	 And for 8-bit values, we use STB instructions.
	     stbi GRk, @(fp, s)
	 P KKKKKK 1010000 000010 SSSSSSSSSSSS = 0x01402000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
	     .    .   .    .   .    .   .   .
	 We check that GRk is really an argument register, and treat
	 all such as part of the prologue.  */
      else if (   (op & 0x01fff000) == 0x01442000
	       || (op & 0x01fff000) == 0x01402000)
	{
	  int gr_k = ((op >> 25) & 0x3f);

	  /* Make sure that GRk is really an argument register; treat
	     it as a prologue instruction if so.  */
	  if (is_argument_reg (gr_k))
	    last_prologue_pc = next_pc;
	}

      /* To save multiple callee-saves register on the stack, at a
	 non-zero offset:

	 stdi GRk, @(sp, s)
	 P KKKKKK 1010011 000001 SSSSSSSSSSSS = 0x014c1000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
	     .    .   .    .   .    .   .   .
	 stqi GRk, @(sp, s)
	 P KKKKKK 1010100 000001 SSSSSSSSSSSS = 0x01501000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
	     .    .   .    .   .    .   .   .
	 We treat this as part of the prologue, and record the register's
	 saved address in the frame structure.  */
      else if ((op & 0x01fff000) == 0x014c1000
	    || (op & 0x01fff000) == 0x01501000)
	{
	  int gr_k = ((op >> 25) & 0x3f);
	  int count;
	  int i;

	  /* Is it a stdi or a stqi?  */
	  if ((op & 0x01fff000) == 0x014c1000)
	    count = 2;
	  else
	    count = 4;

	  /* Is it really a callee-saves register?  */
	  if (is_callee_saves_reg (gr_k))
	    {
	      /* Sign-extend the twelve-bit field.
		 (Isn't there a better way to do this?)  */
	      int s = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;

	      for (i = 0; i < count; i++)
		{
		  gr_saved[gr_k + i] = 1;
		  gr_sp_offset[gr_k + i] = s + (4 * i);
		}
	      last_prologue_pc = next_pc;
	    }
	}

      /* Storing any kind of integer register at any constant offset
	 from any other register.

	 st GRk, @(GRi, gr0)
	 P KKKKKK 0000011 IIIIII 000010 000000 = 0x000c0080
	 0 000000 1111111 000000 111111 111111 = 0x01fc0fff
	     .    .   .    .   .    .    .   .
	 sti GRk, @(GRi, d12)
	 P KKKKKK 1010010 IIIIII SSSSSSSSSSSS = 0x01480000
	 0 000000 1111111 000000 000000000000 = 0x01fc0000
	     .    .   .    .   .    .   .   .
	 These could be almost anything, but a lot of prologue
	 instructions fall into this pattern, so let's decode the
	 instruction once, and then work at a higher level.  */
      else if (((op & 0x01fc0fff) == 0x000c0080)
	    || ((op & 0x01fc0000) == 0x01480000))
	{
	  int gr_k = ((op >> 25) & 0x3f);
	  int gr_i = ((op >> 12) & 0x3f);
	  int offset;

	  /* Are we storing with gr0 as an offset, or using an
	     immediate value?  */
	  if ((op & 0x01fc0fff) == 0x000c0080)
	    offset = 0;
	  else
	    offset = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;

	  /* If the address isn't relative to the SP or FP, it's not a
	     prologue instruction.  */
	  if (gr_i != sp_regnum && gr_i != fp_regnum)
	    {
	      /* Do nothing; not a prologue instruction.  */
	    }

	  /* Saving the old FP in the new frame (relative to the SP).  */
	  else if (gr_k == fp_regnum && gr_i == sp_regnum)
	    {
	      gr_saved[fp_regnum] = 1;
	      gr_sp_offset[fp_regnum] = offset;
	      last_prologue_pc = next_pc;
	    }

	  /* Saving callee-saves register(s) on the stack, relative to
	     the SP.  */
	  else if (gr_i == sp_regnum
		   && is_callee_saves_reg (gr_k))
	    {
	      gr_saved[gr_k] = 1;
	      if (gr_i == sp_regnum)
		gr_sp_offset[gr_k] = offset;
	      else
		gr_sp_offset[gr_k] = offset + fp_offset;
	      last_prologue_pc = next_pc;
	    }

	  /* Saving the scratch register holding the return address.  */
	  else if (lr_save_reg != -1
		   && gr_k == lr_save_reg)
	    {
	      lr_saved_on_stack = 1;
	      if (gr_i == sp_regnum)
		lr_sp_offset = offset;
	      else
		lr_sp_offset = offset + fp_offset;
	      last_prologue_pc = next_pc;
	    }

	  /* Spilling int-sized arguments to the stack.  */
	  else if (is_argument_reg (gr_k))
	    last_prologue_pc = next_pc;
	}
      pc = next_pc;
    }

  if (this_frame && info)
    {
      int i;
      ULONGEST this_base;

      /* If we know the relationship between the stack and frame
	 pointers, record the addresses of the registers we noticed.
	 Note that we have to do this as a separate step at the end,
	 because instructions may save relative to the SP, but we need
	 their addresses relative to the FP.  */
      if (fp_set)
	this_base = get_frame_register_unsigned (this_frame, fp_regnum);
      else
	this_base = get_frame_register_unsigned (this_frame, sp_regnum);

      for (i = 0; i < 64; i++)
	if (gr_saved[i])
	  info->saved_regs[i].set_addr (this_base - fp_offset
					+ gr_sp_offset[i]);

      info->prev_sp = this_base - fp_offset + framesize;
      info->base = this_base;

      /* If LR was saved on the stack, record its location.  */
      if (lr_saved_on_stack)
	info->saved_regs[lr_regnum].set_addr (this_base - fp_offset
					      + lr_sp_offset);

      /* The call instruction moves the caller's PC in the callee's LR.
	 Since this is an unwind, do the reverse.  Copy the location of LR
	 into PC (the address / regnum) so that a request for PC will be
	 converted into a request for the LR.  */
      info->saved_regs[pc_regnum] = info->saved_regs[lr_regnum];

      /* Save the previous frame's computed SP value.  */
      info->saved_regs[sp_regnum].set_value (info->prev_sp);
    }

  return last_prologue_pc;
}


static CORE_ADDR
frv_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end, new_pc;

  new_pc = pc;

  /* If the line table has entry for a line *within* the function
     (i.e., not in the prologue, and not past the end), then that's
     our location.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      struct symtab_and_line sal;

      sal = find_pc_line (func_addr, 0);

      if (sal.line != 0 && sal.end < func_end)
	{
	  new_pc = sal.end;
	}
    }

  /* The FR-V prologue is at least five instructions long (twenty bytes).
     If we didn't find a real source location past that, then
     do a full analysis of the prologue.  */
  if (new_pc < pc + 20)
    new_pc = frv_analyze_prologue (gdbarch, pc, 0, 0);

  return new_pc;
}


/* Examine the instruction pointed to by PC.  If it corresponds to
   a call to __main, return the address of the next instruction.
   Otherwise, return PC.  */

static CORE_ADDR
frv_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  unsigned long op;
  CORE_ADDR orig_pc = pc;

  if (target_read_memory (pc, buf, 4))
    return pc;
  op = extract_unsigned_integer (buf, 4, byte_order);

  /* In PIC code, GR15 may be loaded from some offset off of FP prior
     to the call instruction.
     
     Skip over this instruction if present.  It won't be present in
     non-PIC code, and even in PIC code, it might not be present.
     (This is due to the fact that GR15, the FDPIC register, already
     contains the correct value.)

     The general form of the LDI is given first, followed by the
     specific instruction with the GRi and GRk filled in as FP and
     GR15.

     ldi @(GRi, d12), GRk
     P KKKKKK 0110010 IIIIII SSSSSSSSSSSS = 0x00c80000
     0 000000 1111111 000000 000000000000 = 0x01fc0000
	 .    .   .    .   .    .   .   .
     ldi @(FP, d12), GR15
     P KKKKKK 0110010 IIIIII SSSSSSSSSSSS = 0x1ec82000
     0 001111 1111111 000010 000000000000 = 0x7ffff000
	 .    .   .    .   .    .   .   .               */

  if ((op & 0x7ffff000) == 0x1ec82000)
    {
      pc += 4;
      if (target_read_memory (pc, buf, 4))
	return orig_pc;
      op = extract_unsigned_integer (buf, 4, byte_order);
    }

  /* The format of an FRV CALL instruction is as follows:

     call label24
     P HHHHHH 0001111 LLLLLLLLLLLLLLLLLL = 0x003c0000
     0 000000 1111111 000000000000000000 = 0x01fc0000
	 .    .   .    .   .   .   .   .

     where label24 is constructed by concatenating the H bits with the
     L bits.  The call target is PC + (4 * sign_ext(label24)).  */

  if ((op & 0x01fc0000) == 0x003c0000)
    {
      LONGEST displ;
      CORE_ADDR call_dest;
      struct bound_minimal_symbol s;

      displ = ((op & 0xfe000000) >> 7) | (op & 0x0003ffff);
      if ((displ & 0x00800000) != 0)
	displ |= ~((LONGEST) 0x00ffffff);

      call_dest = pc + 4 * displ;
      s = lookup_minimal_symbol_by_pc (call_dest);

      if (s.minsym != NULL
	  && s.minsym->linkage_name () != NULL
	  && strcmp (s.minsym->linkage_name (), "__main") == 0)
	{
	  pc += 4;
	  return pc;
	}
    }
  return orig_pc;
}


static struct frv_unwind_cache *
frv_frame_unwind_cache (frame_info_ptr this_frame,
			 void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct frv_unwind_cache *info;

  if ((*this_prologue_cache))
    return (struct frv_unwind_cache *) (*this_prologue_cache);

  info = FRAME_OBSTACK_ZALLOC (struct frv_unwind_cache);
  (*this_prologue_cache) = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Prologue analysis does the rest...  */
  frv_analyze_prologue (gdbarch,
			get_frame_func (this_frame), this_frame, info);

  return info;
}

static void
frv_extract_return_value (struct type *type, struct regcache *regcache,
			  gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();

  if (len <= 4)
    {
      ULONGEST gpr8_val;
      regcache_cooked_read_unsigned (regcache, 8, &gpr8_val);
      store_unsigned_integer (valbuf, len, byte_order, gpr8_val);
    }
  else if (len == 8)
    {
      ULONGEST regval;

      regcache_cooked_read_unsigned (regcache, 8, &regval);
      store_unsigned_integer (valbuf, 4, byte_order, regval);
      regcache_cooked_read_unsigned (regcache, 9, &regval);
      store_unsigned_integer ((bfd_byte *) valbuf + 4, 4, byte_order, regval);
    }
  else
    internal_error (_("Illegal return value length: %d"), len);
}

static CORE_ADDR
frv_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Require dword alignment.  */
  return align_down (sp, 8);
}

static CORE_ADDR
find_func_descr (struct gdbarch *gdbarch, CORE_ADDR entry_point)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR descr;
  gdb_byte valbuf[4];
  CORE_ADDR start_addr;

  /* If we can't find the function in the symbol table, then we assume
     that the function address is already in descriptor form.  */
  if (!find_pc_partial_function (entry_point, NULL, &start_addr, NULL)
      || entry_point != start_addr)
    return entry_point;

  descr = frv_fdpic_find_canonical_descriptor (entry_point);

  if (descr != 0)
    return descr;

  /* Construct a non-canonical descriptor from space allocated on
     the stack.  */

  descr = value_as_long (value_allocate_space_in_inferior (8));
  store_unsigned_integer (valbuf, 4, byte_order, entry_point);
  write_memory (descr, valbuf, 4);
  store_unsigned_integer (valbuf, 4, byte_order,
			  frv_fdpic_find_global_pointer (entry_point));
  write_memory (descr + 4, valbuf, 4);
  return descr;
}

static CORE_ADDR
frv_convert_from_func_ptr_addr (struct gdbarch *gdbarch, CORE_ADDR addr,
				struct target_ops *targ)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR entry_point;
  CORE_ADDR got_address;

  entry_point = get_target_memory_unsigned (targ, addr, 4, byte_order);
  got_address = get_target_memory_unsigned (targ, addr + 4, 4, byte_order);

  if (got_address == frv_fdpic_find_global_pointer (entry_point))
    return entry_point;
  else
    return addr;
}

static CORE_ADDR
frv_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr,
		     int nargs, struct value **args, CORE_ADDR sp,
		     function_call_return_method return_method,
		     CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int argreg;
  int argnum;
  const gdb_byte *val;
  gdb_byte valbuf[4];
  struct value *arg;
  struct type *arg_type;
  int len;
  enum type_code typecode;
  CORE_ADDR regval;
  int stack_space;
  int stack_offset;
  enum frv_abi abi = frv_abi (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

#if 0
  printf("Push %d args at sp = %x, struct_return=%d (%x)\n",
	 nargs, (int) sp, struct_return, struct_addr);
#endif

  stack_space = 0;
  for (argnum = 0; argnum < nargs; ++argnum)
    stack_space += align_up (args[argnum]->type ()->length (), 4);

  stack_space -= (6 * 4);
  if (stack_space > 0)
    sp -= stack_space;

  /* Make sure stack is dword aligned.  */
  sp = align_down (sp, 8);

  stack_offset = 0;

  argreg = 8;

  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, struct_return_regnum,
				    struct_addr);

  for (argnum = 0; argnum < nargs; ++argnum)
    {
      arg = args[argnum];
      arg_type = check_typedef (arg->type ());
      len = arg_type->length ();
      typecode = arg_type->code ();

      if (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
	{
	  store_unsigned_integer (valbuf, 4, byte_order,
				  arg->address ());
	  typecode = TYPE_CODE_PTR;
	  len = 4;
	  val = valbuf;
	}
      else if (abi == FRV_ABI_FDPIC
	       && len == 4
	       && typecode == TYPE_CODE_PTR
	       && arg_type->target_type ()->code () == TYPE_CODE_FUNC)
	{
	  /* The FDPIC ABI requires function descriptors to be passed instead
	     of entry points.  */
	  CORE_ADDR addr = extract_unsigned_integer
	    (arg->contents ().data (), 4, byte_order);
	  addr = find_func_descr (gdbarch, addr);
	  store_unsigned_integer (valbuf, 4, byte_order, addr);
	  typecode = TYPE_CODE_PTR;
	  len = 4;
	  val = valbuf;
	}
      else
	{
	  val = arg->contents ().data ();
	}

      while (len > 0)
	{
	  int partial_len = (len < 4 ? len : 4);

	  if (argreg < 14)
	    {
	      regval = extract_unsigned_integer (val, partial_len, byte_order);
#if 0
	      printf("  Argnum %d data %x -> reg %d\n",
		     argnum, (int) regval, argreg);
#endif
	      regcache_cooked_write_unsigned (regcache, argreg, regval);
	      ++argreg;
	    }
	  else
	    {
#if 0
	      printf("  Argnum %d data %x -> offset %d (%x)\n",
		     argnum, *((int *)val), stack_offset,
		     (int) (sp + stack_offset));
#endif
	      write_memory (sp + stack_offset, val, partial_len);
	      stack_offset += align_up (partial_len, 4);
	    }
	  len -= partial_len;
	  val += partial_len;
	}
    }

  /* Set the return address.  For the frv, the return breakpoint is
     always at BP_ADDR.  */
  regcache_cooked_write_unsigned (regcache, lr_regnum, bp_addr);

  if (abi == FRV_ABI_FDPIC)
    {
      /* Set the GOT register for the FDPIC ABI.  */
      regcache_cooked_write_unsigned
	(regcache, first_gpr_regnum + 15,
	 frv_fdpic_find_global_pointer (func_addr));
    }

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, sp_regnum, sp);

  return sp;
}

static void
frv_store_return_value (struct type *type, struct regcache *regcache,
			const gdb_byte *valbuf)
{
  int len = type->length ();

  if (len <= 4)
    {
      bfd_byte val[4];
      memset (val, 0, sizeof (val));
      memcpy (val + (4 - len), valbuf, len);
      regcache->cooked_write (8, val);
    }
  else if (len == 8)
    {
      regcache->cooked_write (8, valbuf);
      regcache->cooked_write (9, (bfd_byte *) valbuf + 4);
    }
  else
    internal_error (_("Don't know how to return a %d-byte value."), len);
}

static enum return_value_convention
frv_return_value (struct gdbarch *gdbarch, struct value *function,
		  struct type *valtype, struct regcache *regcache,
		  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int struct_return = valtype->code () == TYPE_CODE_STRUCT
		      || valtype->code () == TYPE_CODE_UNION
		      || valtype->code () == TYPE_CODE_ARRAY;

  if (writebuf != NULL)
    {
      gdb_assert (!struct_return);
      frv_store_return_value (valtype, regcache, writebuf);
    }

  if (readbuf != NULL)
    {
      gdb_assert (!struct_return);
      frv_extract_return_value (valtype, regcache, readbuf);
    }

  if (struct_return)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
frv_frame_this_id (frame_info_ptr this_frame,
		    void **this_prologue_cache, struct frame_id *this_id)
{
  struct frv_unwind_cache *info
    = frv_frame_unwind_cache (this_frame, this_prologue_cache);
  CORE_ADDR base;
  CORE_ADDR func;
  struct bound_minimal_symbol msym_stack;
  struct frame_id id;

  /* The FUNC is easy.  */
  func = get_frame_func (this_frame);

  /* Check if the stack is empty.  */
  msym_stack = lookup_minimal_symbol ("_stack", NULL, NULL);
  if (msym_stack.minsym && info->base == msym_stack.value_address ())
    return;

  /* Hopefully the prologue analysis either correctly determined the
     frame's base (which is the SP from the previous frame), or set
     that base to "NULL".  */
  base = info->prev_sp;
  if (base == 0)
    return;

  id = frame_id_build (base, func);
  (*this_id) = id;
}

static struct value *
frv_frame_prev_register (frame_info_ptr this_frame,
			 void **this_prologue_cache, int regnum)
{
  struct frv_unwind_cache *info
    = frv_frame_unwind_cache (this_frame, this_prologue_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static const struct frame_unwind frv_frame_unwind = {
  "frv prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  frv_frame_this_id,
  frv_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
frv_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct frv_unwind_cache *info
    = frv_frame_unwind_cache (this_frame, this_cache);
  return info->base;
}

static const struct frame_base frv_frame_base = {
  &frv_frame_unwind,
  frv_frame_base_address,
  frv_frame_base_address,
  frv_frame_base_address
};

static struct gdbarch *
frv_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  int elf_flags = 0;

  /* Check to see if we've already built an appropriate architecture
     object for this executable.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches)
    return arches->gdbarch;

  /* Select the right tdep structure for this variant.  */
  gdbarch *gdbarch = gdbarch_alloc (&info, new_variant ());
  frv_gdbarch_tdep *var = gdbarch_tdep<frv_gdbarch_tdep> (gdbarch);

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_frv:
    case bfd_mach_frvsimple:
    case bfd_mach_fr300:
    case bfd_mach_fr500:
    case bfd_mach_frvtomcat:
    case bfd_mach_fr550:
      set_variant_num_gprs (var, 64);
      set_variant_num_fprs (var, 64);
      break;

    case bfd_mach_fr400:
    case bfd_mach_fr450:
      set_variant_num_gprs (var, 32);
      set_variant_num_fprs (var, 32);
      break;

    default:
      /* Never heard of this variant.  */
      return 0;
    }

  /* Extract the ELF flags, if available.  */
  if (info.abfd && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;

  if (elf_flags & EF_FRV_FDPIC)
    set_variant_abi_fdpic (var);

  if (elf_flags & EF_FRV_CPU_FR450)
    set_variant_scratch_registers (var);

  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 32);

  set_gdbarch_num_regs (gdbarch, frv_num_regs);
  set_gdbarch_num_pseudo_regs (gdbarch, frv_num_pseudo_regs);

  set_gdbarch_sp_regnum (gdbarch, sp_regnum);
  set_gdbarch_deprecated_fp_regnum (gdbarch, fp_regnum);
  set_gdbarch_pc_regnum (gdbarch, pc_regnum);

  set_gdbarch_register_name (gdbarch, frv_register_name);
  set_gdbarch_register_type (gdbarch, frv_register_type);
  set_gdbarch_register_sim_regno (gdbarch, frv_register_sim_regno);

  set_gdbarch_pseudo_register_read (gdbarch, frv_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						frv_pseudo_register_write);

  set_gdbarch_skip_prologue (gdbarch, frv_skip_prologue);
  set_gdbarch_skip_main_prologue (gdbarch, frv_skip_main_prologue);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, frv_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, frv_breakpoint::bp_from_kind);
  set_gdbarch_adjust_breakpoint_address
    (gdbarch, frv_adjust_breakpoint_address);

  set_gdbarch_return_value (gdbarch, frv_return_value);

  /* Frame stuff.  */
  set_gdbarch_frame_align (gdbarch, frv_frame_align);
  frame_base_set_default (gdbarch, &frv_frame_base);
  /* We set the sniffer lower down after the OSABI hooks have been
     established.  */

  /* Settings for calling functions in the inferior.  */
  set_gdbarch_push_dummy_call (gdbarch, frv_push_dummy_call);

  /* Settings that should be unnecessary.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Hardware watchpoint / breakpoint support.  */
  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_frv:
    case bfd_mach_frvsimple:
    case bfd_mach_fr300:
    case bfd_mach_fr500:
    case bfd_mach_frvtomcat:
      /* fr500-style hardware debugging support.  */
      var->num_hw_watchpoints = 4;
      var->num_hw_breakpoints = 4;
      break;

    case bfd_mach_fr400:
    case bfd_mach_fr450:
      /* fr400-style hardware debugging support.  */
      var->num_hw_watchpoints = 2;
      var->num_hw_breakpoints = 4;
      break;

    default:
      /* Otherwise, assume we don't have hardware debugging support.  */
      var->num_hw_watchpoints = 0;
      var->num_hw_breakpoints = 0;
      break;
    }

  if (frv_abi (gdbarch) == FRV_ABI_FDPIC)
    set_gdbarch_convert_from_func_ptr_addr (gdbarch,
					    frv_convert_from_func_ptr_addr);

  set_gdbarch_so_ops (gdbarch, &frv_so_ops);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Set the fallback (prologue based) frame sniffer.  */
  frame_unwind_append_unwinder (gdbarch, &frv_frame_unwind);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     frv_fetch_objfile_link_map);

  return gdbarch;
}

void _initialize_frv_tdep ();
void
_initialize_frv_tdep ()
{
  gdbarch_register (bfd_arch_frv, frv_gdbarch_init);
}

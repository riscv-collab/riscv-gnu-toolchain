/* Common target dependent code for GDB on ARM systems.
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

#ifndef ARM_TDEP_H
#define ARM_TDEP_H

/* Forward declarations.  */
struct regset;
struct address_space;
struct get_next_pcs;
struct arm_get_next_pcs;
struct gdb_get_next_pcs;

/* Set to true if the 32-bit mode is in use.  */

extern bool arm_apcs_32;

#include "gdbarch.h"
#include "arch/arm.h"
#include "infrun.h"

#include <vector>

/* Number of machine registers.  The only define actually required 
   is gdbarch_num_regs.  The other definitions are used for documentation
   purposes and code readability.  */
/* For 26 bit ARM code, a fake copy of the PC is placed in register 25 (PS)
   (and called PS for processor status) so the status bits can be cleared
   from the PC (register 15).  For 32 bit ARM code, a copy of CPSR is placed
   in PS.  */
#define NUM_FREGS	8	/* Number of floating point registers.  */
#define NUM_SREGS	2	/* Number of status registers.  */
#define NUM_GREGS	16	/* Number of general purpose registers.  */



/* Type of floating-point code in use by inferior.  There are really 3 models
   that are traditionally supported (plus the endianness issue), but gcc can
   only generate 2 of those.  The third is APCS_FLOAT, where arguments to
   functions are passed in floating-point registers.  

   In addition to the traditional models, VFP adds two more. 

   If you update this enum, don't forget to update fp_model_strings in 
   arm-tdep.c.  */

enum arm_float_model
{
  ARM_FLOAT_AUTO,	/* Automatic detection.  Do not set in tdep.  */
  ARM_FLOAT_SOFT_FPA,	/* Traditional soft-float (mixed-endian on LE ARM).  */
  ARM_FLOAT_FPA,	/* FPA co-processor.  GCC calling convention.  */
  ARM_FLOAT_SOFT_VFP,	/* Soft-float with pure-endian doubles.  */
  ARM_FLOAT_VFP,	/* Full VFP calling convention.  */
  ARM_FLOAT_LAST	/* Keep at end.  */
};

/* ABI used by the inferior.  */
enum arm_abi_kind
{
  ARM_ABI_AUTO,
  ARM_ABI_APCS,
  ARM_ABI_AAPCS,
  ARM_ABI_LAST
};

/* Convention for returning structures.  */

enum struct_return
{
  pcc_struct_return,		/* Return "short" structures in memory.  */
  reg_struct_return		/* Return "short" structures in registers.  */
};

/* Target-dependent structure in gdbarch.  */
struct arm_gdbarch_tdep : gdbarch_tdep_base
{
  /* The ABI for this architecture.  It should never be set to
     ARM_ABI_AUTO.  */
  enum arm_abi_kind arm_abi {};

  enum arm_float_model fp_model {}; /* Floating point calling conventions.  */

  bool have_fpa_registers = false;	/* Does the target report the FPA registers?  */
  bool have_wmmx_registers = false;	/* Does the target report the WMMX registers?  */
  /* The number of VFP registers reported by the target.  It is zero
     if VFP registers are not supported.  */
  int vfp_register_count = 0;
  bool have_s_pseudos = false;	/* Are we synthesizing the single precision
				   VFP registers?  */
  int s_pseudo_base = 0;	/* Register number for the first S pseudo
				   register.  */
  int s_pseudo_count = 0;	/* Number of S pseudo registers.  */
  bool have_q_pseudos = false;	/* Are we synthesizing the quad precision
				   Q (NEON or MVE) registers?  Requires
				   have_s_pseudos.  */
  int q_pseudo_base = 0;	/* Register number for the first quad
				   precision pseudo register.  */
  int q_pseudo_count = 0;	/* Number of quad precision pseudo
				   registers.  */
  bool have_neon = false;	/* Do we have a NEON unit?  */

  bool have_mve = false;	/* Do we have a MVE extension?  */
  int mve_vpr_regnum = 0;	/* MVE VPR register number.  */
  int mve_pseudo_base = 0;	/* Number of the first MVE pseudo register.  */
  int mve_pseudo_count = 0;	/* Total number of MVE pseudo registers.  */

  bool have_pacbti = false;	/* True if we have the ARMv8.1-m PACBTI
				   extensions.  */
  int pacbti_pseudo_base = 0;	/* Number of the first PACBTI pseudo
				   register.  */
  int pacbti_pseudo_count = 0;	/* Total number of PACBTI pseudo registers.  */

  int m_profile_msp_regnum = ARM_SP_REGNUM;	/* M-profile MSP register number.  */
  int m_profile_psp_regnum = ARM_SP_REGNUM;	/* M-profile PSP register number.  */

  /* Secure and Non-secure stack pointers with security extension.  */
  int m_profile_msp_ns_regnum = ARM_SP_REGNUM;	/* M-profile MSP_NS register number.  */
  int m_profile_psp_ns_regnum = ARM_SP_REGNUM;	/* M-profile PSP_NS register number.  */
  int m_profile_msp_s_regnum = ARM_SP_REGNUM;	/* M-profile MSP_S register number.  */
  int m_profile_psp_s_regnum = ARM_SP_REGNUM;	/* M-profile PSP_S register number.  */

  int tls_regnum = 0;		/* Number of the tpidruro register.  */

  bool is_m = false;		/* Does the target follow the "M" profile.  */
  bool have_sec_ext = false;	/* Do we have security extensions?  */
  CORE_ADDR lowest_pc = 0;	/* Lowest address at which instructions
				   will appear.  */

  const gdb_byte *arm_breakpoint = nullptr;	/* Breakpoint pattern for an ARM insn.  */
  int arm_breakpoint_size = 0;	/* And its size.  */
  const gdb_byte *thumb_breakpoint = nullptr;	/* Breakpoint pattern for a Thumb insn.  */
  int thumb_breakpoint_size = 0;	/* And its size.  */

  /* If the Thumb breakpoint is an undefined instruction (which is
     affected by IT blocks) rather than a BKPT instruction (which is
     not), then we need a 32-bit Thumb breakpoint to preserve the
     instruction count in IT blocks.  */
  const gdb_byte *thumb2_breakpoint = nullptr;
  int thumb2_breakpoint_size = 0;

  int jb_pc = 0;			/* Offset to PC value in jump buffer.
				   If this is negative, longjmp support
				   will be disabled.  */
  size_t jb_elt_size = 0;		/* And the size of each entry in the buf.  */

  /* Convention for returning structures.  */
  enum struct_return struct_return {};

  /* ISA-specific data types.  */
  struct type *arm_ext_type = nullptr;
  struct type *neon_double_type = nullptr;
  struct type *neon_quad_type = nullptr;

   /* syscall record.  */
  int (*arm_syscall_record) (struct regcache *regcache,
			     unsigned long svc_number) = nullptr;
};

/* Structures used for displaced stepping.  */

/* The maximum number of temporaries available for displaced instructions.  */
#define DISPLACED_TEMPS			16
/* The maximum number of modified instructions generated for one single-stepped
   instruction, including the breakpoint (usually at the end of the instruction
   sequence) and any scratch words, etc.  */
#define ARM_DISPLACED_MODIFIED_INSNS	8

struct arm_displaced_step_copy_insn_closure
  : public displaced_step_copy_insn_closure
{
  ULONGEST tmp[DISPLACED_TEMPS];
  int rd;
  int wrote_to_pc;
  union
  {
    struct
    {
      int xfersize;
      int rn;			   /* Writeback register.  */
      unsigned int immed : 1;      /* Offset is immediate.  */
      unsigned int writeback : 1;  /* Perform base-register writeback.  */
      unsigned int restore_r4 : 1; /* Used r4 as scratch.  */
    } ldst;

    struct
    {
      unsigned long dest;
      unsigned int link : 1;
      unsigned int exchange : 1;
      unsigned int cond : 4;
    } branch;

    struct
    {
      unsigned int regmask;
      int rn;
      CORE_ADDR xfer_addr;
      unsigned int load : 1;
      unsigned int user : 1;
      unsigned int increment : 1;
      unsigned int before : 1;
      unsigned int writeback : 1;
      unsigned int cond : 4;
    } block;

    struct
    {
      unsigned int immed : 1;
    } preload;

    struct
    {
      /* If non-NULL, override generic SVC handling (e.g. for a particular
	 OS).  */
      int (*copy_svc_os) (struct gdbarch *gdbarch, struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc);
    } svc;
  } u;

  /* The size of original instruction, 2 or 4.  */
  unsigned int insn_size;
  /* True if the original insn (and thus all replacement insns) are Thumb
     instead of ARM.   */
  unsigned int is_thumb;

  /* The slots in the array is used in this way below,
     - ARM instruction occupies one slot,
     - Thumb 16 bit instruction occupies one slot,
     - Thumb 32-bit instruction occupies *two* slots, one part for each.  */
  unsigned long modinsn[ARM_DISPLACED_MODIFIED_INSNS];
  int numinsns;
  CORE_ADDR insn_addr;
  CORE_ADDR scratch_base;
  void (*cleanup) (struct gdbarch *, struct regcache *,
		   arm_displaced_step_copy_insn_closure *);
};

/* Values for the WRITE_PC argument to displaced_write_reg.  If the register
   write may write to the PC, specifies the way the CPSR T bit, etc. is
   modified by the instruction.  */

enum pc_write_style
{
  BRANCH_WRITE_PC,
  BX_WRITE_PC,
  LOAD_WRITE_PC,
  ALU_WRITE_PC,
  CANNOT_WRITE_PC
};

extern void
  arm_process_displaced_insn (struct gdbarch *gdbarch, CORE_ADDR from,
			      CORE_ADDR to, struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc);
extern void
  arm_displaced_init_closure (struct gdbarch *gdbarch, CORE_ADDR from,
			      CORE_ADDR to,
			      arm_displaced_step_copy_insn_closure *dsc);
extern ULONGEST
  displaced_read_reg (regcache *regs, arm_displaced_step_copy_insn_closure *dsc,
		      int regno);
extern void
  displaced_write_reg (struct regcache *regs,
		       arm_displaced_step_copy_insn_closure *dsc, int regno,
		       ULONGEST val, enum pc_write_style write_pc);

CORE_ADDR arm_skip_stub (frame_info_ptr, CORE_ADDR);

ULONGEST arm_get_next_pcs_read_memory_unsigned_integer (CORE_ADDR memaddr,
							int len,
							int byte_order);

CORE_ADDR arm_get_next_pcs_addr_bits_remove (struct arm_get_next_pcs *self,
					     CORE_ADDR val);

int arm_get_next_pcs_is_thumb (struct arm_get_next_pcs *self);

std::vector<CORE_ADDR> arm_software_single_step (struct regcache *);
int arm_is_thumb (struct regcache *regcache);
int arm_frame_is_thumb (frame_info_ptr frame);

extern void arm_displaced_step_fixup (struct gdbarch *,
				      displaced_step_copy_insn_closure *,
				      CORE_ADDR, CORE_ADDR,
				      struct regcache *, bool);

/* Return the bit mask in ARM_PS_REGNUM that indicates Thumb mode.  */
extern int arm_psr_thumb_bit (struct gdbarch *);

/* Is the instruction at the given memory address a Thumb or ARM
   instruction?  */
extern int arm_pc_is_thumb (struct gdbarch *, CORE_ADDR);

extern int arm_process_record (struct gdbarch *gdbarch, 
			       struct regcache *regcache, CORE_ADDR addr);
/* Functions exported from arm-bsd-tdep.h.  */

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

extern void
  armbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache);

/* Get the correct Arm target description with given FP hardware type.  */
const target_desc *arm_read_description (arm_fp_type fp_type, bool tls);

/* Get the correct Arm M-Profile target description with given hardware
   type.  */
const target_desc *arm_read_mprofile_description (arm_m_profile_type m_type);

#endif /* arm-tdep.h */

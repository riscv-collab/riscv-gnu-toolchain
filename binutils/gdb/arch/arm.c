/* Common target dependent code for GDB on ARM systems.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/common-regcache.h"
#include "arm.h"

#include "../features/arm/arm-core.c"
#include "../features/arm/arm-tls.c"
#include "../features/arm/arm-vfpv2.c"
#include "../features/arm/arm-vfpv3.c"
#include "../features/arm/xscale-iwmmxt.c"
#include "../features/arm/arm-m-profile.c"
#include "../features/arm/arm-m-profile-with-fpa.c"
#include "../features/arm/arm-m-profile-mve.c"
#include "../features/arm/arm-m-system.c"

/* See arm.h.  */

int
thumb_insn_size (unsigned short inst1)
{
  if ((inst1 & 0xe000) == 0xe000 && (inst1 & 0x1800) != 0)
    return 4;
  else
    return 2;
}

/* See arm.h.  */

int
condition_true (unsigned long cond, unsigned long status_reg)
{
  if (cond == INST_AL || cond == INST_NV)
    return 1;

  switch (cond)
    {
    case INST_EQ:
      return ((status_reg & FLAG_Z) != 0);
    case INST_NE:
      return ((status_reg & FLAG_Z) == 0);
    case INST_CS:
      return ((status_reg & FLAG_C) != 0);
    case INST_CC:
      return ((status_reg & FLAG_C) == 0);
    case INST_MI:
      return ((status_reg & FLAG_N) != 0);
    case INST_PL:
      return ((status_reg & FLAG_N) == 0);
    case INST_VS:
      return ((status_reg & FLAG_V) != 0);
    case INST_VC:
      return ((status_reg & FLAG_V) == 0);
    case INST_HI:
      return ((status_reg & (FLAG_C | FLAG_Z)) == FLAG_C);
    case INST_LS:
      return ((status_reg & (FLAG_C | FLAG_Z)) != FLAG_C);
    case INST_GE:
      return (((status_reg & FLAG_N) == 0) == ((status_reg & FLAG_V) == 0));
    case INST_LT:
      return (((status_reg & FLAG_N) == 0) != ((status_reg & FLAG_V) == 0));
    case INST_GT:
      return (((status_reg & FLAG_Z) == 0)
	      && (((status_reg & FLAG_N) == 0)
		  == ((status_reg & FLAG_V) == 0)));
    case INST_LE:
      return (((status_reg & FLAG_Z) != 0)
	      || (((status_reg & FLAG_N) == 0)
		  != ((status_reg & FLAG_V) == 0)));
    }
  return 1;
}


/* See arm.h.  */

int
thumb_advance_itstate (unsigned int itstate)
{
  /* Preserve IT[7:5], the first three bits of the condition.  Shift
     the upcoming condition flags left by one bit.  */
  itstate = (itstate & 0xe0) | ((itstate << 1) & 0x1f);

  /* If we have finished the IT block, clear the state.  */
  if ((itstate & 0x0f) == 0)
    itstate = 0;

  return itstate;
}

/* See arm.h.  */

int
arm_instruction_changes_pc (uint32_t this_instr)
{
  if (bits (this_instr, 28, 31) == INST_NV)
    /* Unconditional instructions.  */
    switch (bits (this_instr, 24, 27))
      {
      case 0xa:
      case 0xb:
	/* Branch with Link and change to Thumb.  */
	return 1;
      case 0xc:
      case 0xd:
      case 0xe:
	/* Coprocessor register transfer.  */
	if (bits (this_instr, 12, 15) == 15)
	  error (_("Invalid update to pc in instruction"));
	return 0;
      default:
	return 0;
      }
  else
    switch (bits (this_instr, 25, 27))
      {
      case 0x0:
	if (bits (this_instr, 23, 24) == 2 && bit (this_instr, 20) == 0)
	  {
	    /* Multiplies and extra load/stores.  */
	    if (bit (this_instr, 4) == 1 && bit (this_instr, 7) == 1)
	      /* Neither multiplies nor extension load/stores are allowed
		 to modify PC.  */
	      return 0;

	    /* Otherwise, miscellaneous instructions.  */

	    /* BX <reg>, BXJ <reg>, BLX <reg> */
	    if (bits (this_instr, 4, 27) == 0x12fff1
		|| bits (this_instr, 4, 27) == 0x12fff2
		|| bits (this_instr, 4, 27) == 0x12fff3)
	      return 1;

	    /* Other miscellaneous instructions are unpredictable if they
	       modify PC.  */
	    return 0;
	  }
	/* Data processing instruction.  */
	[[fallthrough]];

      case 0x1:
	if (bits (this_instr, 12, 15) == 15)
	  return 1;
	else
	  return 0;

      case 0x2:
      case 0x3:
	/* Media instructions and architecturally undefined instructions.  */
	if (bits (this_instr, 25, 27) == 3 && bit (this_instr, 4) == 1)
	  return 0;

	/* Stores.  */
	if (bit (this_instr, 20) == 0)
	  return 0;

	/* Loads.  */
	if (bits (this_instr, 12, 15) == ARM_PC_REGNUM)
	  return 1;
	else
	  return 0;

      case 0x4:
	/* Load/store multiple.  */
	if (bit (this_instr, 20) == 1 && bit (this_instr, 15) == 1)
	  return 1;
	else
	  return 0;

      case 0x5:
	/* Branch and branch with link.  */
	return 1;

      case 0x6:
      case 0x7:
	/* Coprocessor transfers or SWIs can not affect PC.  */
	return 0;

      default:
	internal_error (_("bad value in switch"));
      }
}

/* See arm.h.  */

int
thumb_instruction_changes_pc (unsigned short inst)
{
  if ((inst & 0xff00) == 0xbd00)	/* pop {rlist, pc} */
    return 1;

  if ((inst & 0xf000) == 0xd000)	/* conditional branch */
    return 1;

  if ((inst & 0xf800) == 0xe000)	/* unconditional branch */
    return 1;

  if ((inst & 0xff00) == 0x4700)	/* bx REG, blx REG */
    return 1;

  if ((inst & 0xff87) == 0x4687)	/* mov pc, REG */
    return 1;

  if ((inst & 0xf500) == 0xb100)	/* CBNZ or CBZ.  */
    return 1;

  return 0;
}


/* See arm.h.  */

int
thumb2_instruction_changes_pc (unsigned short inst1, unsigned short inst2)
{
  if ((inst1 & 0xf800) == 0xf000 && (inst2 & 0x8000) == 0x8000)
    {
      /* Branches and miscellaneous control instructions.  */

      if ((inst2 & 0x1000) != 0 || (inst2 & 0xd001) == 0xc000)
	{
	  /* B, BL, BLX.  */
	  return 1;
	}
      else if (inst1 == 0xf3de && (inst2 & 0xff00) == 0x3f00)
	{
	  /* SUBS PC, LR, #imm8.  */
	  return 1;
	}
      else if ((inst2 & 0xd000) == 0x8000 && (inst1 & 0x0380) != 0x0380)
	{
	  /* Conditional branch.  */
	  return 1;
	}

      return 0;
    }

  if ((inst1 & 0xfe50) == 0xe810)
    {
      /* Load multiple or RFE.  */

      if (bit (inst1, 7) && !bit (inst1, 8))
	{
	  /* LDMIA or POP */
	  if (bit (inst2, 15))
	    return 1;
	}
      else if (!bit (inst1, 7) && bit (inst1, 8))
	{
	  /* LDMDB */
	  if (bit (inst2, 15))
	    return 1;
	}
      else if (bit (inst1, 7) && bit (inst1, 8))
	{
	  /* RFEIA */
	  return 1;
	}
      else if (!bit (inst1, 7) && !bit (inst1, 8))
	{
	  /* RFEDB */
	  return 1;
	}

      return 0;
    }

  if ((inst1 & 0xffef) == 0xea4f && (inst2 & 0xfff0) == 0x0f00)
    {
      /* MOV PC or MOVS PC.  */
      return 1;
    }

  if ((inst1 & 0xff70) == 0xf850 && (inst2 & 0xf000) == 0xf000)
    {
      /* LDR PC.  */
      if (bits (inst1, 0, 3) == 15)
	return 1;
      if (bit (inst1, 7))
	return 1;
      if (bit (inst2, 11))
	return 1;
      if ((inst2 & 0x0fc0) == 0x0000)
	return 1;

      return 0;
    }

  if ((inst1 & 0xfff0) == 0xe8d0 && (inst2 & 0xfff0) == 0xf000)
    {
      /* TBB.  */
      return 1;
    }

  if ((inst1 & 0xfff0) == 0xe8d0 && (inst2 & 0xfff0) == 0xf010)
    {
      /* TBH.  */
      return 1;
    }

  return 0;
}

/* See arm.h.  */

unsigned long
shifted_reg_val (reg_buffer_common *regcache, unsigned long inst,
		 int carry, unsigned long pc_val, unsigned long status_reg)
{
  unsigned long res, shift;
  int rm = bits (inst, 0, 3);
  unsigned long shifttype = bits (inst, 5, 6);

  if (bit (inst, 4))
    {
      int rs = bits (inst, 8, 11);
      shift = (rs == 15
	       ? pc_val + 8
	       : regcache_raw_get_unsigned (regcache, rs)) & 0xFF;
    }
  else
    shift = bits (inst, 7, 11);

  res = (rm == ARM_PC_REGNUM
	 ? (pc_val + (bit (inst, 4) ? 12 : 8))
	 : regcache_raw_get_unsigned (regcache, rm));

  switch (shifttype)
    {
    case 0:			/* LSL */
      res = shift >= 32 ? 0 : res << shift;
      break;

    case 1:			/* LSR */
      res = shift >= 32 ? 0 : res >> shift;
      break;

    case 2:			/* ASR */
      if (shift >= 32)
	shift = 31;
      res = ((res & 0x80000000L)
	     ? ~((~res) >> shift) : res >> shift);
      break;

    case 3:			/* ROR/RRX */
      shift &= 31;
      if (shift == 0)
	res = (res >> 1) | (carry ? 0x80000000L : 0);
      else
	res = (res >> shift) | (res << (32 - shift));
      break;
    }

  return res & 0xffffffff;
}

/* See arch/arm.h.  */

target_desc *
arm_create_target_description (arm_fp_type fp_type, bool tls)
{
  target_desc_up tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  if (fp_type == ARM_FP_TYPE_IWMMXT)
    set_tdesc_architecture (tdesc.get (), "iwmmxt");
  else
    set_tdesc_architecture (tdesc.get (), "arm");
#endif

  long regnum = 0;

  regnum = create_feature_arm_arm_core (tdesc.get (), regnum);

  switch (fp_type)
    {
    case ARM_FP_TYPE_NONE:
      break;

    case ARM_FP_TYPE_VFPV2:
      regnum = create_feature_arm_arm_vfpv2 (tdesc.get (), regnum);
      break;

    case ARM_FP_TYPE_VFPV3:
      regnum = create_feature_arm_arm_vfpv3 (tdesc.get (), regnum);
      break;

    case ARM_FP_TYPE_IWMMXT:
      regnum = create_feature_arm_xscale_iwmmxt (tdesc.get (), regnum);
      break;

    default:
      error (_("Invalid Arm FP type: %d"), fp_type);
    }

  if (tls)
    regnum = create_feature_arm_arm_tls (tdesc.get (), regnum);

  return tdesc.release ();
}

/* See arch/arm.h.  */

target_desc *
arm_create_mprofile_target_description (arm_m_profile_type m_type)
{
  target_desc *tdesc = allocate_target_description ().release ();

#ifndef IN_PROCESS_AGENT
  set_tdesc_architecture (tdesc, "arm");
#endif

  long regnum = 0;

  switch (m_type)
    {
    case ARM_M_TYPE_M_PROFILE:
      regnum = create_feature_arm_arm_m_profile (tdesc, regnum);
      break;

    case ARM_M_TYPE_VFP_D16:
      regnum = create_feature_arm_arm_m_profile (tdesc, regnum);
      regnum = create_feature_arm_arm_vfpv2 (tdesc, regnum);
      break;

    case ARM_M_TYPE_WITH_FPA:
      regnum = create_feature_arm_arm_m_profile_with_fpa (tdesc, regnum);
      break;

    case ARM_M_TYPE_MVE:
      regnum = create_feature_arm_arm_m_profile (tdesc, regnum);
      regnum = create_feature_arm_arm_vfpv2 (tdesc, regnum);
      regnum = create_feature_arm_arm_m_profile_mve (tdesc, regnum);
      break;

    case ARM_M_TYPE_SYSTEM:
      regnum = create_feature_arm_arm_m_profile (tdesc, regnum);
      regnum = create_feature_arm_arm_m_system (tdesc, regnum);
      break;

    default:
      error (_("Invalid Arm M type: %d"), m_type);
    }

  return tdesc;
}

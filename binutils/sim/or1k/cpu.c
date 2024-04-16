/* Misc. support for CPU family or1k32bf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2024 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#define WANT_CPU or1k32bf
#define WANT_CPU_OR1K32BF

#include "sim-main.h"
#include "cgen-ops.h"

/* Get the value of h-pc.  */

USI
or1k32bf_h_pc_get (SIM_CPU *current_cpu)
{
  return GET_H_PC ();
}

/* Set a value for h-pc.  */

void
or1k32bf_h_pc_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_PC (newval);
}

/* Get the value of h-spr.  */

USI
or1k32bf_h_spr_get (SIM_CPU *current_cpu, UINT regno)
{
  return GET_H_SPR (regno);
}

/* Set a value for h-spr.  */

void
or1k32bf_h_spr_set (SIM_CPU *current_cpu, UINT regno, USI newval)
{
  SET_H_SPR (regno, newval);
}

/* Get the value of h-gpr.  */

USI
or1k32bf_h_gpr_get (SIM_CPU *current_cpu, UINT regno)
{
  return GET_H_GPR (regno);
}

/* Set a value for h-gpr.  */

void
or1k32bf_h_gpr_set (SIM_CPU *current_cpu, UINT regno, USI newval)
{
  SET_H_GPR (regno, newval);
}

/* Get the value of h-fsr.  */

SF
or1k32bf_h_fsr_get (SIM_CPU *current_cpu, UINT regno)
{
  return GET_H_FSR (regno);
}

/* Set a value for h-fsr.  */

void
or1k32bf_h_fsr_set (SIM_CPU *current_cpu, UINT regno, SF newval)
{
  SET_H_FSR (regno, newval);
}

/* Get the value of h-fd32r.  */

DF
or1k32bf_h_fd32r_get (SIM_CPU *current_cpu, UINT regno)
{
  return GET_H_FD32R (regno);
}

/* Set a value for h-fd32r.  */

void
or1k32bf_h_fd32r_set (SIM_CPU *current_cpu, UINT regno, DF newval)
{
  SET_H_FD32R (regno, newval);
}

/* Get the value of h-i64r.  */

DI
or1k32bf_h_i64r_get (SIM_CPU *current_cpu, UINT regno)
{
  return GET_H_I64R (regno);
}

/* Set a value for h-i64r.  */

void
or1k32bf_h_i64r_set (SIM_CPU *current_cpu, UINT regno, DI newval)
{
  SET_H_I64R (regno, newval);
}

/* Get the value of h-sys-vr.  */

USI
or1k32bf_h_sys_vr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_VR ();
}

/* Set a value for h-sys-vr.  */

void
or1k32bf_h_sys_vr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_VR (newval);
}

/* Get the value of h-sys-upr.  */

USI
or1k32bf_h_sys_upr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR ();
}

/* Set a value for h-sys-upr.  */

void
or1k32bf_h_sys_upr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR (newval);
}

/* Get the value of h-sys-cpucfgr.  */

USI
or1k32bf_h_sys_cpucfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR ();
}

/* Set a value for h-sys-cpucfgr.  */

void
or1k32bf_h_sys_cpucfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR (newval);
}

/* Get the value of h-sys-dmmucfgr.  */

USI
or1k32bf_h_sys_dmmucfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_DMMUCFGR ();
}

/* Set a value for h-sys-dmmucfgr.  */

void
or1k32bf_h_sys_dmmucfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_DMMUCFGR (newval);
}

/* Get the value of h-sys-immucfgr.  */

USI
or1k32bf_h_sys_immucfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_IMMUCFGR ();
}

/* Set a value for h-sys-immucfgr.  */

void
or1k32bf_h_sys_immucfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_IMMUCFGR (newval);
}

/* Get the value of h-sys-dccfgr.  */

USI
or1k32bf_h_sys_dccfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_DCCFGR ();
}

/* Set a value for h-sys-dccfgr.  */

void
or1k32bf_h_sys_dccfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_DCCFGR (newval);
}

/* Get the value of h-sys-iccfgr.  */

USI
or1k32bf_h_sys_iccfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ICCFGR ();
}

/* Set a value for h-sys-iccfgr.  */

void
or1k32bf_h_sys_iccfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ICCFGR (newval);
}

/* Get the value of h-sys-dcfgr.  */

USI
or1k32bf_h_sys_dcfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_DCFGR ();
}

/* Set a value for h-sys-dcfgr.  */

void
or1k32bf_h_sys_dcfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_DCFGR (newval);
}

/* Get the value of h-sys-pccfgr.  */

USI
or1k32bf_h_sys_pccfgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_PCCFGR ();
}

/* Set a value for h-sys-pccfgr.  */

void
or1k32bf_h_sys_pccfgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_PCCFGR (newval);
}

/* Get the value of h-sys-npc.  */

USI
or1k32bf_h_sys_npc_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_NPC ();
}

/* Set a value for h-sys-npc.  */

void
or1k32bf_h_sys_npc_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_NPC (newval);
}

/* Get the value of h-sys-sr.  */

USI
or1k32bf_h_sys_sr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR ();
}

/* Set a value for h-sys-sr.  */

void
or1k32bf_h_sys_sr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR (newval);
}

/* Get the value of h-sys-ppc.  */

USI
or1k32bf_h_sys_ppc_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_PPC ();
}

/* Set a value for h-sys-ppc.  */

void
or1k32bf_h_sys_ppc_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_PPC (newval);
}

/* Get the value of h-sys-fpcsr.  */

USI
or1k32bf_h_sys_fpcsr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR ();
}

/* Set a value for h-sys-fpcsr.  */

void
or1k32bf_h_sys_fpcsr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR (newval);
}

/* Get the value of h-sys-epcr0.  */

USI
or1k32bf_h_sys_epcr0_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR0 ();
}

/* Set a value for h-sys-epcr0.  */

void
or1k32bf_h_sys_epcr0_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR0 (newval);
}

/* Get the value of h-sys-epcr1.  */

USI
or1k32bf_h_sys_epcr1_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR1 ();
}

/* Set a value for h-sys-epcr1.  */

void
or1k32bf_h_sys_epcr1_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR1 (newval);
}

/* Get the value of h-sys-epcr2.  */

USI
or1k32bf_h_sys_epcr2_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR2 ();
}

/* Set a value for h-sys-epcr2.  */

void
or1k32bf_h_sys_epcr2_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR2 (newval);
}

/* Get the value of h-sys-epcr3.  */

USI
or1k32bf_h_sys_epcr3_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR3 ();
}

/* Set a value for h-sys-epcr3.  */

void
or1k32bf_h_sys_epcr3_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR3 (newval);
}

/* Get the value of h-sys-epcr4.  */

USI
or1k32bf_h_sys_epcr4_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR4 ();
}

/* Set a value for h-sys-epcr4.  */

void
or1k32bf_h_sys_epcr4_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR4 (newval);
}

/* Get the value of h-sys-epcr5.  */

USI
or1k32bf_h_sys_epcr5_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR5 ();
}

/* Set a value for h-sys-epcr5.  */

void
or1k32bf_h_sys_epcr5_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR5 (newval);
}

/* Get the value of h-sys-epcr6.  */

USI
or1k32bf_h_sys_epcr6_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR6 ();
}

/* Set a value for h-sys-epcr6.  */

void
or1k32bf_h_sys_epcr6_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR6 (newval);
}

/* Get the value of h-sys-epcr7.  */

USI
or1k32bf_h_sys_epcr7_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR7 ();
}

/* Set a value for h-sys-epcr7.  */

void
or1k32bf_h_sys_epcr7_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR7 (newval);
}

/* Get the value of h-sys-epcr8.  */

USI
or1k32bf_h_sys_epcr8_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR8 ();
}

/* Set a value for h-sys-epcr8.  */

void
or1k32bf_h_sys_epcr8_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR8 (newval);
}

/* Get the value of h-sys-epcr9.  */

USI
or1k32bf_h_sys_epcr9_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR9 ();
}

/* Set a value for h-sys-epcr9.  */

void
or1k32bf_h_sys_epcr9_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR9 (newval);
}

/* Get the value of h-sys-epcr10.  */

USI
or1k32bf_h_sys_epcr10_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR10 ();
}

/* Set a value for h-sys-epcr10.  */

void
or1k32bf_h_sys_epcr10_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR10 (newval);
}

/* Get the value of h-sys-epcr11.  */

USI
or1k32bf_h_sys_epcr11_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR11 ();
}

/* Set a value for h-sys-epcr11.  */

void
or1k32bf_h_sys_epcr11_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR11 (newval);
}

/* Get the value of h-sys-epcr12.  */

USI
or1k32bf_h_sys_epcr12_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR12 ();
}

/* Set a value for h-sys-epcr12.  */

void
or1k32bf_h_sys_epcr12_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR12 (newval);
}

/* Get the value of h-sys-epcr13.  */

USI
or1k32bf_h_sys_epcr13_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR13 ();
}

/* Set a value for h-sys-epcr13.  */

void
or1k32bf_h_sys_epcr13_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR13 (newval);
}

/* Get the value of h-sys-epcr14.  */

USI
or1k32bf_h_sys_epcr14_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR14 ();
}

/* Set a value for h-sys-epcr14.  */

void
or1k32bf_h_sys_epcr14_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR14 (newval);
}

/* Get the value of h-sys-epcr15.  */

USI
or1k32bf_h_sys_epcr15_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EPCR15 ();
}

/* Set a value for h-sys-epcr15.  */

void
or1k32bf_h_sys_epcr15_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EPCR15 (newval);
}

/* Get the value of h-sys-eear0.  */

USI
or1k32bf_h_sys_eear0_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR0 ();
}

/* Set a value for h-sys-eear0.  */

void
or1k32bf_h_sys_eear0_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR0 (newval);
}

/* Get the value of h-sys-eear1.  */

USI
or1k32bf_h_sys_eear1_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR1 ();
}

/* Set a value for h-sys-eear1.  */

void
or1k32bf_h_sys_eear1_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR1 (newval);
}

/* Get the value of h-sys-eear2.  */

USI
or1k32bf_h_sys_eear2_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR2 ();
}

/* Set a value for h-sys-eear2.  */

void
or1k32bf_h_sys_eear2_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR2 (newval);
}

/* Get the value of h-sys-eear3.  */

USI
or1k32bf_h_sys_eear3_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR3 ();
}

/* Set a value for h-sys-eear3.  */

void
or1k32bf_h_sys_eear3_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR3 (newval);
}

/* Get the value of h-sys-eear4.  */

USI
or1k32bf_h_sys_eear4_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR4 ();
}

/* Set a value for h-sys-eear4.  */

void
or1k32bf_h_sys_eear4_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR4 (newval);
}

/* Get the value of h-sys-eear5.  */

USI
or1k32bf_h_sys_eear5_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR5 ();
}

/* Set a value for h-sys-eear5.  */

void
or1k32bf_h_sys_eear5_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR5 (newval);
}

/* Get the value of h-sys-eear6.  */

USI
or1k32bf_h_sys_eear6_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR6 ();
}

/* Set a value for h-sys-eear6.  */

void
or1k32bf_h_sys_eear6_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR6 (newval);
}

/* Get the value of h-sys-eear7.  */

USI
or1k32bf_h_sys_eear7_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR7 ();
}

/* Set a value for h-sys-eear7.  */

void
or1k32bf_h_sys_eear7_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR7 (newval);
}

/* Get the value of h-sys-eear8.  */

USI
or1k32bf_h_sys_eear8_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR8 ();
}

/* Set a value for h-sys-eear8.  */

void
or1k32bf_h_sys_eear8_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR8 (newval);
}

/* Get the value of h-sys-eear9.  */

USI
or1k32bf_h_sys_eear9_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR9 ();
}

/* Set a value for h-sys-eear9.  */

void
or1k32bf_h_sys_eear9_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR9 (newval);
}

/* Get the value of h-sys-eear10.  */

USI
or1k32bf_h_sys_eear10_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR10 ();
}

/* Set a value for h-sys-eear10.  */

void
or1k32bf_h_sys_eear10_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR10 (newval);
}

/* Get the value of h-sys-eear11.  */

USI
or1k32bf_h_sys_eear11_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR11 ();
}

/* Set a value for h-sys-eear11.  */

void
or1k32bf_h_sys_eear11_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR11 (newval);
}

/* Get the value of h-sys-eear12.  */

USI
or1k32bf_h_sys_eear12_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR12 ();
}

/* Set a value for h-sys-eear12.  */

void
or1k32bf_h_sys_eear12_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR12 (newval);
}

/* Get the value of h-sys-eear13.  */

USI
or1k32bf_h_sys_eear13_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR13 ();
}

/* Set a value for h-sys-eear13.  */

void
or1k32bf_h_sys_eear13_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR13 (newval);
}

/* Get the value of h-sys-eear14.  */

USI
or1k32bf_h_sys_eear14_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR14 ();
}

/* Set a value for h-sys-eear14.  */

void
or1k32bf_h_sys_eear14_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR14 (newval);
}

/* Get the value of h-sys-eear15.  */

USI
or1k32bf_h_sys_eear15_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_EEAR15 ();
}

/* Set a value for h-sys-eear15.  */

void
or1k32bf_h_sys_eear15_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_EEAR15 (newval);
}

/* Get the value of h-sys-esr0.  */

USI
or1k32bf_h_sys_esr0_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR0 ();
}

/* Set a value for h-sys-esr0.  */

void
or1k32bf_h_sys_esr0_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR0 (newval);
}

/* Get the value of h-sys-esr1.  */

USI
or1k32bf_h_sys_esr1_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR1 ();
}

/* Set a value for h-sys-esr1.  */

void
or1k32bf_h_sys_esr1_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR1 (newval);
}

/* Get the value of h-sys-esr2.  */

USI
or1k32bf_h_sys_esr2_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR2 ();
}

/* Set a value for h-sys-esr2.  */

void
or1k32bf_h_sys_esr2_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR2 (newval);
}

/* Get the value of h-sys-esr3.  */

USI
or1k32bf_h_sys_esr3_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR3 ();
}

/* Set a value for h-sys-esr3.  */

void
or1k32bf_h_sys_esr3_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR3 (newval);
}

/* Get the value of h-sys-esr4.  */

USI
or1k32bf_h_sys_esr4_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR4 ();
}

/* Set a value for h-sys-esr4.  */

void
or1k32bf_h_sys_esr4_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR4 (newval);
}

/* Get the value of h-sys-esr5.  */

USI
or1k32bf_h_sys_esr5_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR5 ();
}

/* Set a value for h-sys-esr5.  */

void
or1k32bf_h_sys_esr5_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR5 (newval);
}

/* Get the value of h-sys-esr6.  */

USI
or1k32bf_h_sys_esr6_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR6 ();
}

/* Set a value for h-sys-esr6.  */

void
or1k32bf_h_sys_esr6_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR6 (newval);
}

/* Get the value of h-sys-esr7.  */

USI
or1k32bf_h_sys_esr7_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR7 ();
}

/* Set a value for h-sys-esr7.  */

void
or1k32bf_h_sys_esr7_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR7 (newval);
}

/* Get the value of h-sys-esr8.  */

USI
or1k32bf_h_sys_esr8_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR8 ();
}

/* Set a value for h-sys-esr8.  */

void
or1k32bf_h_sys_esr8_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR8 (newval);
}

/* Get the value of h-sys-esr9.  */

USI
or1k32bf_h_sys_esr9_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR9 ();
}

/* Set a value for h-sys-esr9.  */

void
or1k32bf_h_sys_esr9_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR9 (newval);
}

/* Get the value of h-sys-esr10.  */

USI
or1k32bf_h_sys_esr10_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR10 ();
}

/* Set a value for h-sys-esr10.  */

void
or1k32bf_h_sys_esr10_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR10 (newval);
}

/* Get the value of h-sys-esr11.  */

USI
or1k32bf_h_sys_esr11_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR11 ();
}

/* Set a value for h-sys-esr11.  */

void
or1k32bf_h_sys_esr11_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR11 (newval);
}

/* Get the value of h-sys-esr12.  */

USI
or1k32bf_h_sys_esr12_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR12 ();
}

/* Set a value for h-sys-esr12.  */

void
or1k32bf_h_sys_esr12_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR12 (newval);
}

/* Get the value of h-sys-esr13.  */

USI
or1k32bf_h_sys_esr13_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR13 ();
}

/* Set a value for h-sys-esr13.  */

void
or1k32bf_h_sys_esr13_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR13 (newval);
}

/* Get the value of h-sys-esr14.  */

USI
or1k32bf_h_sys_esr14_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR14 ();
}

/* Set a value for h-sys-esr14.  */

void
or1k32bf_h_sys_esr14_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR14 (newval);
}

/* Get the value of h-sys-esr15.  */

USI
or1k32bf_h_sys_esr15_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_ESR15 ();
}

/* Set a value for h-sys-esr15.  */

void
or1k32bf_h_sys_esr15_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_ESR15 (newval);
}

/* Get the value of h-sys-gpr0.  */

USI
or1k32bf_h_sys_gpr0_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR0 ();
}

/* Set a value for h-sys-gpr0.  */

void
or1k32bf_h_sys_gpr0_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR0 (newval);
}

/* Get the value of h-sys-gpr1.  */

USI
or1k32bf_h_sys_gpr1_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR1 ();
}

/* Set a value for h-sys-gpr1.  */

void
or1k32bf_h_sys_gpr1_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR1 (newval);
}

/* Get the value of h-sys-gpr2.  */

USI
or1k32bf_h_sys_gpr2_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR2 ();
}

/* Set a value for h-sys-gpr2.  */

void
or1k32bf_h_sys_gpr2_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR2 (newval);
}

/* Get the value of h-sys-gpr3.  */

USI
or1k32bf_h_sys_gpr3_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR3 ();
}

/* Set a value for h-sys-gpr3.  */

void
or1k32bf_h_sys_gpr3_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR3 (newval);
}

/* Get the value of h-sys-gpr4.  */

USI
or1k32bf_h_sys_gpr4_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR4 ();
}

/* Set a value for h-sys-gpr4.  */

void
or1k32bf_h_sys_gpr4_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR4 (newval);
}

/* Get the value of h-sys-gpr5.  */

USI
or1k32bf_h_sys_gpr5_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR5 ();
}

/* Set a value for h-sys-gpr5.  */

void
or1k32bf_h_sys_gpr5_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR5 (newval);
}

/* Get the value of h-sys-gpr6.  */

USI
or1k32bf_h_sys_gpr6_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR6 ();
}

/* Set a value for h-sys-gpr6.  */

void
or1k32bf_h_sys_gpr6_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR6 (newval);
}

/* Get the value of h-sys-gpr7.  */

USI
or1k32bf_h_sys_gpr7_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR7 ();
}

/* Set a value for h-sys-gpr7.  */

void
or1k32bf_h_sys_gpr7_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR7 (newval);
}

/* Get the value of h-sys-gpr8.  */

USI
or1k32bf_h_sys_gpr8_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR8 ();
}

/* Set a value for h-sys-gpr8.  */

void
or1k32bf_h_sys_gpr8_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR8 (newval);
}

/* Get the value of h-sys-gpr9.  */

USI
or1k32bf_h_sys_gpr9_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR9 ();
}

/* Set a value for h-sys-gpr9.  */

void
or1k32bf_h_sys_gpr9_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR9 (newval);
}

/* Get the value of h-sys-gpr10.  */

USI
or1k32bf_h_sys_gpr10_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR10 ();
}

/* Set a value for h-sys-gpr10.  */

void
or1k32bf_h_sys_gpr10_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR10 (newval);
}

/* Get the value of h-sys-gpr11.  */

USI
or1k32bf_h_sys_gpr11_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR11 ();
}

/* Set a value for h-sys-gpr11.  */

void
or1k32bf_h_sys_gpr11_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR11 (newval);
}

/* Get the value of h-sys-gpr12.  */

USI
or1k32bf_h_sys_gpr12_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR12 ();
}

/* Set a value for h-sys-gpr12.  */

void
or1k32bf_h_sys_gpr12_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR12 (newval);
}

/* Get the value of h-sys-gpr13.  */

USI
or1k32bf_h_sys_gpr13_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR13 ();
}

/* Set a value for h-sys-gpr13.  */

void
or1k32bf_h_sys_gpr13_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR13 (newval);
}

/* Get the value of h-sys-gpr14.  */

USI
or1k32bf_h_sys_gpr14_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR14 ();
}

/* Set a value for h-sys-gpr14.  */

void
or1k32bf_h_sys_gpr14_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR14 (newval);
}

/* Get the value of h-sys-gpr15.  */

USI
or1k32bf_h_sys_gpr15_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR15 ();
}

/* Set a value for h-sys-gpr15.  */

void
or1k32bf_h_sys_gpr15_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR15 (newval);
}

/* Get the value of h-sys-gpr16.  */

USI
or1k32bf_h_sys_gpr16_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR16 ();
}

/* Set a value for h-sys-gpr16.  */

void
or1k32bf_h_sys_gpr16_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR16 (newval);
}

/* Get the value of h-sys-gpr17.  */

USI
or1k32bf_h_sys_gpr17_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR17 ();
}

/* Set a value for h-sys-gpr17.  */

void
or1k32bf_h_sys_gpr17_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR17 (newval);
}

/* Get the value of h-sys-gpr18.  */

USI
or1k32bf_h_sys_gpr18_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR18 ();
}

/* Set a value for h-sys-gpr18.  */

void
or1k32bf_h_sys_gpr18_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR18 (newval);
}

/* Get the value of h-sys-gpr19.  */

USI
or1k32bf_h_sys_gpr19_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR19 ();
}

/* Set a value for h-sys-gpr19.  */

void
or1k32bf_h_sys_gpr19_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR19 (newval);
}

/* Get the value of h-sys-gpr20.  */

USI
or1k32bf_h_sys_gpr20_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR20 ();
}

/* Set a value for h-sys-gpr20.  */

void
or1k32bf_h_sys_gpr20_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR20 (newval);
}

/* Get the value of h-sys-gpr21.  */

USI
or1k32bf_h_sys_gpr21_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR21 ();
}

/* Set a value for h-sys-gpr21.  */

void
or1k32bf_h_sys_gpr21_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR21 (newval);
}

/* Get the value of h-sys-gpr22.  */

USI
or1k32bf_h_sys_gpr22_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR22 ();
}

/* Set a value for h-sys-gpr22.  */

void
or1k32bf_h_sys_gpr22_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR22 (newval);
}

/* Get the value of h-sys-gpr23.  */

USI
or1k32bf_h_sys_gpr23_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR23 ();
}

/* Set a value for h-sys-gpr23.  */

void
or1k32bf_h_sys_gpr23_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR23 (newval);
}

/* Get the value of h-sys-gpr24.  */

USI
or1k32bf_h_sys_gpr24_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR24 ();
}

/* Set a value for h-sys-gpr24.  */

void
or1k32bf_h_sys_gpr24_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR24 (newval);
}

/* Get the value of h-sys-gpr25.  */

USI
or1k32bf_h_sys_gpr25_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR25 ();
}

/* Set a value for h-sys-gpr25.  */

void
or1k32bf_h_sys_gpr25_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR25 (newval);
}

/* Get the value of h-sys-gpr26.  */

USI
or1k32bf_h_sys_gpr26_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR26 ();
}

/* Set a value for h-sys-gpr26.  */

void
or1k32bf_h_sys_gpr26_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR26 (newval);
}

/* Get the value of h-sys-gpr27.  */

USI
or1k32bf_h_sys_gpr27_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR27 ();
}

/* Set a value for h-sys-gpr27.  */

void
or1k32bf_h_sys_gpr27_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR27 (newval);
}

/* Get the value of h-sys-gpr28.  */

USI
or1k32bf_h_sys_gpr28_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR28 ();
}

/* Set a value for h-sys-gpr28.  */

void
or1k32bf_h_sys_gpr28_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR28 (newval);
}

/* Get the value of h-sys-gpr29.  */

USI
or1k32bf_h_sys_gpr29_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR29 ();
}

/* Set a value for h-sys-gpr29.  */

void
or1k32bf_h_sys_gpr29_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR29 (newval);
}

/* Get the value of h-sys-gpr30.  */

USI
or1k32bf_h_sys_gpr30_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR30 ();
}

/* Set a value for h-sys-gpr30.  */

void
or1k32bf_h_sys_gpr30_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR30 (newval);
}

/* Get the value of h-sys-gpr31.  */

USI
or1k32bf_h_sys_gpr31_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR31 ();
}

/* Set a value for h-sys-gpr31.  */

void
or1k32bf_h_sys_gpr31_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR31 (newval);
}

/* Get the value of h-sys-gpr32.  */

USI
or1k32bf_h_sys_gpr32_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR32 ();
}

/* Set a value for h-sys-gpr32.  */

void
or1k32bf_h_sys_gpr32_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR32 (newval);
}

/* Get the value of h-sys-gpr33.  */

USI
or1k32bf_h_sys_gpr33_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR33 ();
}

/* Set a value for h-sys-gpr33.  */

void
or1k32bf_h_sys_gpr33_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR33 (newval);
}

/* Get the value of h-sys-gpr34.  */

USI
or1k32bf_h_sys_gpr34_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR34 ();
}

/* Set a value for h-sys-gpr34.  */

void
or1k32bf_h_sys_gpr34_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR34 (newval);
}

/* Get the value of h-sys-gpr35.  */

USI
or1k32bf_h_sys_gpr35_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR35 ();
}

/* Set a value for h-sys-gpr35.  */

void
or1k32bf_h_sys_gpr35_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR35 (newval);
}

/* Get the value of h-sys-gpr36.  */

USI
or1k32bf_h_sys_gpr36_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR36 ();
}

/* Set a value for h-sys-gpr36.  */

void
or1k32bf_h_sys_gpr36_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR36 (newval);
}

/* Get the value of h-sys-gpr37.  */

USI
or1k32bf_h_sys_gpr37_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR37 ();
}

/* Set a value for h-sys-gpr37.  */

void
or1k32bf_h_sys_gpr37_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR37 (newval);
}

/* Get the value of h-sys-gpr38.  */

USI
or1k32bf_h_sys_gpr38_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR38 ();
}

/* Set a value for h-sys-gpr38.  */

void
or1k32bf_h_sys_gpr38_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR38 (newval);
}

/* Get the value of h-sys-gpr39.  */

USI
or1k32bf_h_sys_gpr39_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR39 ();
}

/* Set a value for h-sys-gpr39.  */

void
or1k32bf_h_sys_gpr39_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR39 (newval);
}

/* Get the value of h-sys-gpr40.  */

USI
or1k32bf_h_sys_gpr40_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR40 ();
}

/* Set a value for h-sys-gpr40.  */

void
or1k32bf_h_sys_gpr40_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR40 (newval);
}

/* Get the value of h-sys-gpr41.  */

USI
or1k32bf_h_sys_gpr41_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR41 ();
}

/* Set a value for h-sys-gpr41.  */

void
or1k32bf_h_sys_gpr41_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR41 (newval);
}

/* Get the value of h-sys-gpr42.  */

USI
or1k32bf_h_sys_gpr42_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR42 ();
}

/* Set a value for h-sys-gpr42.  */

void
or1k32bf_h_sys_gpr42_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR42 (newval);
}

/* Get the value of h-sys-gpr43.  */

USI
or1k32bf_h_sys_gpr43_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR43 ();
}

/* Set a value for h-sys-gpr43.  */

void
or1k32bf_h_sys_gpr43_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR43 (newval);
}

/* Get the value of h-sys-gpr44.  */

USI
or1k32bf_h_sys_gpr44_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR44 ();
}

/* Set a value for h-sys-gpr44.  */

void
or1k32bf_h_sys_gpr44_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR44 (newval);
}

/* Get the value of h-sys-gpr45.  */

USI
or1k32bf_h_sys_gpr45_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR45 ();
}

/* Set a value for h-sys-gpr45.  */

void
or1k32bf_h_sys_gpr45_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR45 (newval);
}

/* Get the value of h-sys-gpr46.  */

USI
or1k32bf_h_sys_gpr46_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR46 ();
}

/* Set a value for h-sys-gpr46.  */

void
or1k32bf_h_sys_gpr46_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR46 (newval);
}

/* Get the value of h-sys-gpr47.  */

USI
or1k32bf_h_sys_gpr47_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR47 ();
}

/* Set a value for h-sys-gpr47.  */

void
or1k32bf_h_sys_gpr47_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR47 (newval);
}

/* Get the value of h-sys-gpr48.  */

USI
or1k32bf_h_sys_gpr48_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR48 ();
}

/* Set a value for h-sys-gpr48.  */

void
or1k32bf_h_sys_gpr48_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR48 (newval);
}

/* Get the value of h-sys-gpr49.  */

USI
or1k32bf_h_sys_gpr49_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR49 ();
}

/* Set a value for h-sys-gpr49.  */

void
or1k32bf_h_sys_gpr49_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR49 (newval);
}

/* Get the value of h-sys-gpr50.  */

USI
or1k32bf_h_sys_gpr50_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR50 ();
}

/* Set a value for h-sys-gpr50.  */

void
or1k32bf_h_sys_gpr50_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR50 (newval);
}

/* Get the value of h-sys-gpr51.  */

USI
or1k32bf_h_sys_gpr51_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR51 ();
}

/* Set a value for h-sys-gpr51.  */

void
or1k32bf_h_sys_gpr51_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR51 (newval);
}

/* Get the value of h-sys-gpr52.  */

USI
or1k32bf_h_sys_gpr52_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR52 ();
}

/* Set a value for h-sys-gpr52.  */

void
or1k32bf_h_sys_gpr52_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR52 (newval);
}

/* Get the value of h-sys-gpr53.  */

USI
or1k32bf_h_sys_gpr53_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR53 ();
}

/* Set a value for h-sys-gpr53.  */

void
or1k32bf_h_sys_gpr53_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR53 (newval);
}

/* Get the value of h-sys-gpr54.  */

USI
or1k32bf_h_sys_gpr54_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR54 ();
}

/* Set a value for h-sys-gpr54.  */

void
or1k32bf_h_sys_gpr54_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR54 (newval);
}

/* Get the value of h-sys-gpr55.  */

USI
or1k32bf_h_sys_gpr55_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR55 ();
}

/* Set a value for h-sys-gpr55.  */

void
or1k32bf_h_sys_gpr55_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR55 (newval);
}

/* Get the value of h-sys-gpr56.  */

USI
or1k32bf_h_sys_gpr56_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR56 ();
}

/* Set a value for h-sys-gpr56.  */

void
or1k32bf_h_sys_gpr56_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR56 (newval);
}

/* Get the value of h-sys-gpr57.  */

USI
or1k32bf_h_sys_gpr57_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR57 ();
}

/* Set a value for h-sys-gpr57.  */

void
or1k32bf_h_sys_gpr57_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR57 (newval);
}

/* Get the value of h-sys-gpr58.  */

USI
or1k32bf_h_sys_gpr58_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR58 ();
}

/* Set a value for h-sys-gpr58.  */

void
or1k32bf_h_sys_gpr58_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR58 (newval);
}

/* Get the value of h-sys-gpr59.  */

USI
or1k32bf_h_sys_gpr59_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR59 ();
}

/* Set a value for h-sys-gpr59.  */

void
or1k32bf_h_sys_gpr59_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR59 (newval);
}

/* Get the value of h-sys-gpr60.  */

USI
or1k32bf_h_sys_gpr60_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR60 ();
}

/* Set a value for h-sys-gpr60.  */

void
or1k32bf_h_sys_gpr60_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR60 (newval);
}

/* Get the value of h-sys-gpr61.  */

USI
or1k32bf_h_sys_gpr61_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR61 ();
}

/* Set a value for h-sys-gpr61.  */

void
or1k32bf_h_sys_gpr61_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR61 (newval);
}

/* Get the value of h-sys-gpr62.  */

USI
or1k32bf_h_sys_gpr62_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR62 ();
}

/* Set a value for h-sys-gpr62.  */

void
or1k32bf_h_sys_gpr62_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR62 (newval);
}

/* Get the value of h-sys-gpr63.  */

USI
or1k32bf_h_sys_gpr63_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR63 ();
}

/* Set a value for h-sys-gpr63.  */

void
or1k32bf_h_sys_gpr63_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR63 (newval);
}

/* Get the value of h-sys-gpr64.  */

USI
or1k32bf_h_sys_gpr64_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR64 ();
}

/* Set a value for h-sys-gpr64.  */

void
or1k32bf_h_sys_gpr64_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR64 (newval);
}

/* Get the value of h-sys-gpr65.  */

USI
or1k32bf_h_sys_gpr65_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR65 ();
}

/* Set a value for h-sys-gpr65.  */

void
or1k32bf_h_sys_gpr65_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR65 (newval);
}

/* Get the value of h-sys-gpr66.  */

USI
or1k32bf_h_sys_gpr66_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR66 ();
}

/* Set a value for h-sys-gpr66.  */

void
or1k32bf_h_sys_gpr66_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR66 (newval);
}

/* Get the value of h-sys-gpr67.  */

USI
or1k32bf_h_sys_gpr67_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR67 ();
}

/* Set a value for h-sys-gpr67.  */

void
or1k32bf_h_sys_gpr67_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR67 (newval);
}

/* Get the value of h-sys-gpr68.  */

USI
or1k32bf_h_sys_gpr68_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR68 ();
}

/* Set a value for h-sys-gpr68.  */

void
or1k32bf_h_sys_gpr68_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR68 (newval);
}

/* Get the value of h-sys-gpr69.  */

USI
or1k32bf_h_sys_gpr69_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR69 ();
}

/* Set a value for h-sys-gpr69.  */

void
or1k32bf_h_sys_gpr69_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR69 (newval);
}

/* Get the value of h-sys-gpr70.  */

USI
or1k32bf_h_sys_gpr70_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR70 ();
}

/* Set a value for h-sys-gpr70.  */

void
or1k32bf_h_sys_gpr70_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR70 (newval);
}

/* Get the value of h-sys-gpr71.  */

USI
or1k32bf_h_sys_gpr71_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR71 ();
}

/* Set a value for h-sys-gpr71.  */

void
or1k32bf_h_sys_gpr71_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR71 (newval);
}

/* Get the value of h-sys-gpr72.  */

USI
or1k32bf_h_sys_gpr72_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR72 ();
}

/* Set a value for h-sys-gpr72.  */

void
or1k32bf_h_sys_gpr72_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR72 (newval);
}

/* Get the value of h-sys-gpr73.  */

USI
or1k32bf_h_sys_gpr73_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR73 ();
}

/* Set a value for h-sys-gpr73.  */

void
or1k32bf_h_sys_gpr73_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR73 (newval);
}

/* Get the value of h-sys-gpr74.  */

USI
or1k32bf_h_sys_gpr74_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR74 ();
}

/* Set a value for h-sys-gpr74.  */

void
or1k32bf_h_sys_gpr74_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR74 (newval);
}

/* Get the value of h-sys-gpr75.  */

USI
or1k32bf_h_sys_gpr75_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR75 ();
}

/* Set a value for h-sys-gpr75.  */

void
or1k32bf_h_sys_gpr75_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR75 (newval);
}

/* Get the value of h-sys-gpr76.  */

USI
or1k32bf_h_sys_gpr76_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR76 ();
}

/* Set a value for h-sys-gpr76.  */

void
or1k32bf_h_sys_gpr76_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR76 (newval);
}

/* Get the value of h-sys-gpr77.  */

USI
or1k32bf_h_sys_gpr77_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR77 ();
}

/* Set a value for h-sys-gpr77.  */

void
or1k32bf_h_sys_gpr77_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR77 (newval);
}

/* Get the value of h-sys-gpr78.  */

USI
or1k32bf_h_sys_gpr78_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR78 ();
}

/* Set a value for h-sys-gpr78.  */

void
or1k32bf_h_sys_gpr78_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR78 (newval);
}

/* Get the value of h-sys-gpr79.  */

USI
or1k32bf_h_sys_gpr79_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR79 ();
}

/* Set a value for h-sys-gpr79.  */

void
or1k32bf_h_sys_gpr79_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR79 (newval);
}

/* Get the value of h-sys-gpr80.  */

USI
or1k32bf_h_sys_gpr80_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR80 ();
}

/* Set a value for h-sys-gpr80.  */

void
or1k32bf_h_sys_gpr80_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR80 (newval);
}

/* Get the value of h-sys-gpr81.  */

USI
or1k32bf_h_sys_gpr81_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR81 ();
}

/* Set a value for h-sys-gpr81.  */

void
or1k32bf_h_sys_gpr81_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR81 (newval);
}

/* Get the value of h-sys-gpr82.  */

USI
or1k32bf_h_sys_gpr82_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR82 ();
}

/* Set a value for h-sys-gpr82.  */

void
or1k32bf_h_sys_gpr82_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR82 (newval);
}

/* Get the value of h-sys-gpr83.  */

USI
or1k32bf_h_sys_gpr83_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR83 ();
}

/* Set a value for h-sys-gpr83.  */

void
or1k32bf_h_sys_gpr83_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR83 (newval);
}

/* Get the value of h-sys-gpr84.  */

USI
or1k32bf_h_sys_gpr84_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR84 ();
}

/* Set a value for h-sys-gpr84.  */

void
or1k32bf_h_sys_gpr84_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR84 (newval);
}

/* Get the value of h-sys-gpr85.  */

USI
or1k32bf_h_sys_gpr85_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR85 ();
}

/* Set a value for h-sys-gpr85.  */

void
or1k32bf_h_sys_gpr85_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR85 (newval);
}

/* Get the value of h-sys-gpr86.  */

USI
or1k32bf_h_sys_gpr86_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR86 ();
}

/* Set a value for h-sys-gpr86.  */

void
or1k32bf_h_sys_gpr86_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR86 (newval);
}

/* Get the value of h-sys-gpr87.  */

USI
or1k32bf_h_sys_gpr87_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR87 ();
}

/* Set a value for h-sys-gpr87.  */

void
or1k32bf_h_sys_gpr87_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR87 (newval);
}

/* Get the value of h-sys-gpr88.  */

USI
or1k32bf_h_sys_gpr88_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR88 ();
}

/* Set a value for h-sys-gpr88.  */

void
or1k32bf_h_sys_gpr88_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR88 (newval);
}

/* Get the value of h-sys-gpr89.  */

USI
or1k32bf_h_sys_gpr89_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR89 ();
}

/* Set a value for h-sys-gpr89.  */

void
or1k32bf_h_sys_gpr89_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR89 (newval);
}

/* Get the value of h-sys-gpr90.  */

USI
or1k32bf_h_sys_gpr90_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR90 ();
}

/* Set a value for h-sys-gpr90.  */

void
or1k32bf_h_sys_gpr90_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR90 (newval);
}

/* Get the value of h-sys-gpr91.  */

USI
or1k32bf_h_sys_gpr91_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR91 ();
}

/* Set a value for h-sys-gpr91.  */

void
or1k32bf_h_sys_gpr91_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR91 (newval);
}

/* Get the value of h-sys-gpr92.  */

USI
or1k32bf_h_sys_gpr92_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR92 ();
}

/* Set a value for h-sys-gpr92.  */

void
or1k32bf_h_sys_gpr92_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR92 (newval);
}

/* Get the value of h-sys-gpr93.  */

USI
or1k32bf_h_sys_gpr93_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR93 ();
}

/* Set a value for h-sys-gpr93.  */

void
or1k32bf_h_sys_gpr93_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR93 (newval);
}

/* Get the value of h-sys-gpr94.  */

USI
or1k32bf_h_sys_gpr94_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR94 ();
}

/* Set a value for h-sys-gpr94.  */

void
or1k32bf_h_sys_gpr94_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR94 (newval);
}

/* Get the value of h-sys-gpr95.  */

USI
or1k32bf_h_sys_gpr95_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR95 ();
}

/* Set a value for h-sys-gpr95.  */

void
or1k32bf_h_sys_gpr95_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR95 (newval);
}

/* Get the value of h-sys-gpr96.  */

USI
or1k32bf_h_sys_gpr96_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR96 ();
}

/* Set a value for h-sys-gpr96.  */

void
or1k32bf_h_sys_gpr96_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR96 (newval);
}

/* Get the value of h-sys-gpr97.  */

USI
or1k32bf_h_sys_gpr97_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR97 ();
}

/* Set a value for h-sys-gpr97.  */

void
or1k32bf_h_sys_gpr97_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR97 (newval);
}

/* Get the value of h-sys-gpr98.  */

USI
or1k32bf_h_sys_gpr98_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR98 ();
}

/* Set a value for h-sys-gpr98.  */

void
or1k32bf_h_sys_gpr98_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR98 (newval);
}

/* Get the value of h-sys-gpr99.  */

USI
or1k32bf_h_sys_gpr99_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR99 ();
}

/* Set a value for h-sys-gpr99.  */

void
or1k32bf_h_sys_gpr99_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR99 (newval);
}

/* Get the value of h-sys-gpr100.  */

USI
or1k32bf_h_sys_gpr100_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR100 ();
}

/* Set a value for h-sys-gpr100.  */

void
or1k32bf_h_sys_gpr100_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR100 (newval);
}

/* Get the value of h-sys-gpr101.  */

USI
or1k32bf_h_sys_gpr101_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR101 ();
}

/* Set a value for h-sys-gpr101.  */

void
or1k32bf_h_sys_gpr101_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR101 (newval);
}

/* Get the value of h-sys-gpr102.  */

USI
or1k32bf_h_sys_gpr102_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR102 ();
}

/* Set a value for h-sys-gpr102.  */

void
or1k32bf_h_sys_gpr102_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR102 (newval);
}

/* Get the value of h-sys-gpr103.  */

USI
or1k32bf_h_sys_gpr103_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR103 ();
}

/* Set a value for h-sys-gpr103.  */

void
or1k32bf_h_sys_gpr103_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR103 (newval);
}

/* Get the value of h-sys-gpr104.  */

USI
or1k32bf_h_sys_gpr104_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR104 ();
}

/* Set a value for h-sys-gpr104.  */

void
or1k32bf_h_sys_gpr104_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR104 (newval);
}

/* Get the value of h-sys-gpr105.  */

USI
or1k32bf_h_sys_gpr105_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR105 ();
}

/* Set a value for h-sys-gpr105.  */

void
or1k32bf_h_sys_gpr105_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR105 (newval);
}

/* Get the value of h-sys-gpr106.  */

USI
or1k32bf_h_sys_gpr106_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR106 ();
}

/* Set a value for h-sys-gpr106.  */

void
or1k32bf_h_sys_gpr106_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR106 (newval);
}

/* Get the value of h-sys-gpr107.  */

USI
or1k32bf_h_sys_gpr107_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR107 ();
}

/* Set a value for h-sys-gpr107.  */

void
or1k32bf_h_sys_gpr107_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR107 (newval);
}

/* Get the value of h-sys-gpr108.  */

USI
or1k32bf_h_sys_gpr108_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR108 ();
}

/* Set a value for h-sys-gpr108.  */

void
or1k32bf_h_sys_gpr108_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR108 (newval);
}

/* Get the value of h-sys-gpr109.  */

USI
or1k32bf_h_sys_gpr109_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR109 ();
}

/* Set a value for h-sys-gpr109.  */

void
or1k32bf_h_sys_gpr109_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR109 (newval);
}

/* Get the value of h-sys-gpr110.  */

USI
or1k32bf_h_sys_gpr110_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR110 ();
}

/* Set a value for h-sys-gpr110.  */

void
or1k32bf_h_sys_gpr110_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR110 (newval);
}

/* Get the value of h-sys-gpr111.  */

USI
or1k32bf_h_sys_gpr111_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR111 ();
}

/* Set a value for h-sys-gpr111.  */

void
or1k32bf_h_sys_gpr111_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR111 (newval);
}

/* Get the value of h-sys-gpr112.  */

USI
or1k32bf_h_sys_gpr112_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR112 ();
}

/* Set a value for h-sys-gpr112.  */

void
or1k32bf_h_sys_gpr112_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR112 (newval);
}

/* Get the value of h-sys-gpr113.  */

USI
or1k32bf_h_sys_gpr113_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR113 ();
}

/* Set a value for h-sys-gpr113.  */

void
or1k32bf_h_sys_gpr113_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR113 (newval);
}

/* Get the value of h-sys-gpr114.  */

USI
or1k32bf_h_sys_gpr114_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR114 ();
}

/* Set a value for h-sys-gpr114.  */

void
or1k32bf_h_sys_gpr114_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR114 (newval);
}

/* Get the value of h-sys-gpr115.  */

USI
or1k32bf_h_sys_gpr115_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR115 ();
}

/* Set a value for h-sys-gpr115.  */

void
or1k32bf_h_sys_gpr115_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR115 (newval);
}

/* Get the value of h-sys-gpr116.  */

USI
or1k32bf_h_sys_gpr116_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR116 ();
}

/* Set a value for h-sys-gpr116.  */

void
or1k32bf_h_sys_gpr116_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR116 (newval);
}

/* Get the value of h-sys-gpr117.  */

USI
or1k32bf_h_sys_gpr117_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR117 ();
}

/* Set a value for h-sys-gpr117.  */

void
or1k32bf_h_sys_gpr117_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR117 (newval);
}

/* Get the value of h-sys-gpr118.  */

USI
or1k32bf_h_sys_gpr118_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR118 ();
}

/* Set a value for h-sys-gpr118.  */

void
or1k32bf_h_sys_gpr118_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR118 (newval);
}

/* Get the value of h-sys-gpr119.  */

USI
or1k32bf_h_sys_gpr119_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR119 ();
}

/* Set a value for h-sys-gpr119.  */

void
or1k32bf_h_sys_gpr119_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR119 (newval);
}

/* Get the value of h-sys-gpr120.  */

USI
or1k32bf_h_sys_gpr120_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR120 ();
}

/* Set a value for h-sys-gpr120.  */

void
or1k32bf_h_sys_gpr120_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR120 (newval);
}

/* Get the value of h-sys-gpr121.  */

USI
or1k32bf_h_sys_gpr121_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR121 ();
}

/* Set a value for h-sys-gpr121.  */

void
or1k32bf_h_sys_gpr121_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR121 (newval);
}

/* Get the value of h-sys-gpr122.  */

USI
or1k32bf_h_sys_gpr122_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR122 ();
}

/* Set a value for h-sys-gpr122.  */

void
or1k32bf_h_sys_gpr122_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR122 (newval);
}

/* Get the value of h-sys-gpr123.  */

USI
or1k32bf_h_sys_gpr123_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR123 ();
}

/* Set a value for h-sys-gpr123.  */

void
or1k32bf_h_sys_gpr123_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR123 (newval);
}

/* Get the value of h-sys-gpr124.  */

USI
or1k32bf_h_sys_gpr124_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR124 ();
}

/* Set a value for h-sys-gpr124.  */

void
or1k32bf_h_sys_gpr124_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR124 (newval);
}

/* Get the value of h-sys-gpr125.  */

USI
or1k32bf_h_sys_gpr125_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR125 ();
}

/* Set a value for h-sys-gpr125.  */

void
or1k32bf_h_sys_gpr125_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR125 (newval);
}

/* Get the value of h-sys-gpr126.  */

USI
or1k32bf_h_sys_gpr126_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR126 ();
}

/* Set a value for h-sys-gpr126.  */

void
or1k32bf_h_sys_gpr126_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR126 (newval);
}

/* Get the value of h-sys-gpr127.  */

USI
or1k32bf_h_sys_gpr127_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR127 ();
}

/* Set a value for h-sys-gpr127.  */

void
or1k32bf_h_sys_gpr127_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR127 (newval);
}

/* Get the value of h-sys-gpr128.  */

USI
or1k32bf_h_sys_gpr128_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR128 ();
}

/* Set a value for h-sys-gpr128.  */

void
or1k32bf_h_sys_gpr128_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR128 (newval);
}

/* Get the value of h-sys-gpr129.  */

USI
or1k32bf_h_sys_gpr129_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR129 ();
}

/* Set a value for h-sys-gpr129.  */

void
or1k32bf_h_sys_gpr129_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR129 (newval);
}

/* Get the value of h-sys-gpr130.  */

USI
or1k32bf_h_sys_gpr130_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR130 ();
}

/* Set a value for h-sys-gpr130.  */

void
or1k32bf_h_sys_gpr130_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR130 (newval);
}

/* Get the value of h-sys-gpr131.  */

USI
or1k32bf_h_sys_gpr131_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR131 ();
}

/* Set a value for h-sys-gpr131.  */

void
or1k32bf_h_sys_gpr131_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR131 (newval);
}

/* Get the value of h-sys-gpr132.  */

USI
or1k32bf_h_sys_gpr132_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR132 ();
}

/* Set a value for h-sys-gpr132.  */

void
or1k32bf_h_sys_gpr132_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR132 (newval);
}

/* Get the value of h-sys-gpr133.  */

USI
or1k32bf_h_sys_gpr133_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR133 ();
}

/* Set a value for h-sys-gpr133.  */

void
or1k32bf_h_sys_gpr133_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR133 (newval);
}

/* Get the value of h-sys-gpr134.  */

USI
or1k32bf_h_sys_gpr134_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR134 ();
}

/* Set a value for h-sys-gpr134.  */

void
or1k32bf_h_sys_gpr134_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR134 (newval);
}

/* Get the value of h-sys-gpr135.  */

USI
or1k32bf_h_sys_gpr135_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR135 ();
}

/* Set a value for h-sys-gpr135.  */

void
or1k32bf_h_sys_gpr135_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR135 (newval);
}

/* Get the value of h-sys-gpr136.  */

USI
or1k32bf_h_sys_gpr136_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR136 ();
}

/* Set a value for h-sys-gpr136.  */

void
or1k32bf_h_sys_gpr136_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR136 (newval);
}

/* Get the value of h-sys-gpr137.  */

USI
or1k32bf_h_sys_gpr137_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR137 ();
}

/* Set a value for h-sys-gpr137.  */

void
or1k32bf_h_sys_gpr137_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR137 (newval);
}

/* Get the value of h-sys-gpr138.  */

USI
or1k32bf_h_sys_gpr138_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR138 ();
}

/* Set a value for h-sys-gpr138.  */

void
or1k32bf_h_sys_gpr138_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR138 (newval);
}

/* Get the value of h-sys-gpr139.  */

USI
or1k32bf_h_sys_gpr139_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR139 ();
}

/* Set a value for h-sys-gpr139.  */

void
or1k32bf_h_sys_gpr139_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR139 (newval);
}

/* Get the value of h-sys-gpr140.  */

USI
or1k32bf_h_sys_gpr140_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR140 ();
}

/* Set a value for h-sys-gpr140.  */

void
or1k32bf_h_sys_gpr140_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR140 (newval);
}

/* Get the value of h-sys-gpr141.  */

USI
or1k32bf_h_sys_gpr141_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR141 ();
}

/* Set a value for h-sys-gpr141.  */

void
or1k32bf_h_sys_gpr141_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR141 (newval);
}

/* Get the value of h-sys-gpr142.  */

USI
or1k32bf_h_sys_gpr142_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR142 ();
}

/* Set a value for h-sys-gpr142.  */

void
or1k32bf_h_sys_gpr142_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR142 (newval);
}

/* Get the value of h-sys-gpr143.  */

USI
or1k32bf_h_sys_gpr143_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR143 ();
}

/* Set a value for h-sys-gpr143.  */

void
or1k32bf_h_sys_gpr143_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR143 (newval);
}

/* Get the value of h-sys-gpr144.  */

USI
or1k32bf_h_sys_gpr144_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR144 ();
}

/* Set a value for h-sys-gpr144.  */

void
or1k32bf_h_sys_gpr144_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR144 (newval);
}

/* Get the value of h-sys-gpr145.  */

USI
or1k32bf_h_sys_gpr145_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR145 ();
}

/* Set a value for h-sys-gpr145.  */

void
or1k32bf_h_sys_gpr145_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR145 (newval);
}

/* Get the value of h-sys-gpr146.  */

USI
or1k32bf_h_sys_gpr146_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR146 ();
}

/* Set a value for h-sys-gpr146.  */

void
or1k32bf_h_sys_gpr146_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR146 (newval);
}

/* Get the value of h-sys-gpr147.  */

USI
or1k32bf_h_sys_gpr147_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR147 ();
}

/* Set a value for h-sys-gpr147.  */

void
or1k32bf_h_sys_gpr147_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR147 (newval);
}

/* Get the value of h-sys-gpr148.  */

USI
or1k32bf_h_sys_gpr148_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR148 ();
}

/* Set a value for h-sys-gpr148.  */

void
or1k32bf_h_sys_gpr148_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR148 (newval);
}

/* Get the value of h-sys-gpr149.  */

USI
or1k32bf_h_sys_gpr149_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR149 ();
}

/* Set a value for h-sys-gpr149.  */

void
or1k32bf_h_sys_gpr149_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR149 (newval);
}

/* Get the value of h-sys-gpr150.  */

USI
or1k32bf_h_sys_gpr150_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR150 ();
}

/* Set a value for h-sys-gpr150.  */

void
or1k32bf_h_sys_gpr150_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR150 (newval);
}

/* Get the value of h-sys-gpr151.  */

USI
or1k32bf_h_sys_gpr151_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR151 ();
}

/* Set a value for h-sys-gpr151.  */

void
or1k32bf_h_sys_gpr151_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR151 (newval);
}

/* Get the value of h-sys-gpr152.  */

USI
or1k32bf_h_sys_gpr152_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR152 ();
}

/* Set a value for h-sys-gpr152.  */

void
or1k32bf_h_sys_gpr152_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR152 (newval);
}

/* Get the value of h-sys-gpr153.  */

USI
or1k32bf_h_sys_gpr153_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR153 ();
}

/* Set a value for h-sys-gpr153.  */

void
or1k32bf_h_sys_gpr153_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR153 (newval);
}

/* Get the value of h-sys-gpr154.  */

USI
or1k32bf_h_sys_gpr154_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR154 ();
}

/* Set a value for h-sys-gpr154.  */

void
or1k32bf_h_sys_gpr154_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR154 (newval);
}

/* Get the value of h-sys-gpr155.  */

USI
or1k32bf_h_sys_gpr155_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR155 ();
}

/* Set a value for h-sys-gpr155.  */

void
or1k32bf_h_sys_gpr155_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR155 (newval);
}

/* Get the value of h-sys-gpr156.  */

USI
or1k32bf_h_sys_gpr156_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR156 ();
}

/* Set a value for h-sys-gpr156.  */

void
or1k32bf_h_sys_gpr156_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR156 (newval);
}

/* Get the value of h-sys-gpr157.  */

USI
or1k32bf_h_sys_gpr157_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR157 ();
}

/* Set a value for h-sys-gpr157.  */

void
or1k32bf_h_sys_gpr157_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR157 (newval);
}

/* Get the value of h-sys-gpr158.  */

USI
or1k32bf_h_sys_gpr158_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR158 ();
}

/* Set a value for h-sys-gpr158.  */

void
or1k32bf_h_sys_gpr158_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR158 (newval);
}

/* Get the value of h-sys-gpr159.  */

USI
or1k32bf_h_sys_gpr159_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR159 ();
}

/* Set a value for h-sys-gpr159.  */

void
or1k32bf_h_sys_gpr159_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR159 (newval);
}

/* Get the value of h-sys-gpr160.  */

USI
or1k32bf_h_sys_gpr160_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR160 ();
}

/* Set a value for h-sys-gpr160.  */

void
or1k32bf_h_sys_gpr160_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR160 (newval);
}

/* Get the value of h-sys-gpr161.  */

USI
or1k32bf_h_sys_gpr161_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR161 ();
}

/* Set a value for h-sys-gpr161.  */

void
or1k32bf_h_sys_gpr161_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR161 (newval);
}

/* Get the value of h-sys-gpr162.  */

USI
or1k32bf_h_sys_gpr162_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR162 ();
}

/* Set a value for h-sys-gpr162.  */

void
or1k32bf_h_sys_gpr162_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR162 (newval);
}

/* Get the value of h-sys-gpr163.  */

USI
or1k32bf_h_sys_gpr163_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR163 ();
}

/* Set a value for h-sys-gpr163.  */

void
or1k32bf_h_sys_gpr163_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR163 (newval);
}

/* Get the value of h-sys-gpr164.  */

USI
or1k32bf_h_sys_gpr164_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR164 ();
}

/* Set a value for h-sys-gpr164.  */

void
or1k32bf_h_sys_gpr164_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR164 (newval);
}

/* Get the value of h-sys-gpr165.  */

USI
or1k32bf_h_sys_gpr165_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR165 ();
}

/* Set a value for h-sys-gpr165.  */

void
or1k32bf_h_sys_gpr165_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR165 (newval);
}

/* Get the value of h-sys-gpr166.  */

USI
or1k32bf_h_sys_gpr166_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR166 ();
}

/* Set a value for h-sys-gpr166.  */

void
or1k32bf_h_sys_gpr166_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR166 (newval);
}

/* Get the value of h-sys-gpr167.  */

USI
or1k32bf_h_sys_gpr167_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR167 ();
}

/* Set a value for h-sys-gpr167.  */

void
or1k32bf_h_sys_gpr167_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR167 (newval);
}

/* Get the value of h-sys-gpr168.  */

USI
or1k32bf_h_sys_gpr168_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR168 ();
}

/* Set a value for h-sys-gpr168.  */

void
or1k32bf_h_sys_gpr168_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR168 (newval);
}

/* Get the value of h-sys-gpr169.  */

USI
or1k32bf_h_sys_gpr169_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR169 ();
}

/* Set a value for h-sys-gpr169.  */

void
or1k32bf_h_sys_gpr169_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR169 (newval);
}

/* Get the value of h-sys-gpr170.  */

USI
or1k32bf_h_sys_gpr170_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR170 ();
}

/* Set a value for h-sys-gpr170.  */

void
or1k32bf_h_sys_gpr170_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR170 (newval);
}

/* Get the value of h-sys-gpr171.  */

USI
or1k32bf_h_sys_gpr171_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR171 ();
}

/* Set a value for h-sys-gpr171.  */

void
or1k32bf_h_sys_gpr171_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR171 (newval);
}

/* Get the value of h-sys-gpr172.  */

USI
or1k32bf_h_sys_gpr172_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR172 ();
}

/* Set a value for h-sys-gpr172.  */

void
or1k32bf_h_sys_gpr172_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR172 (newval);
}

/* Get the value of h-sys-gpr173.  */

USI
or1k32bf_h_sys_gpr173_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR173 ();
}

/* Set a value for h-sys-gpr173.  */

void
or1k32bf_h_sys_gpr173_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR173 (newval);
}

/* Get the value of h-sys-gpr174.  */

USI
or1k32bf_h_sys_gpr174_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR174 ();
}

/* Set a value for h-sys-gpr174.  */

void
or1k32bf_h_sys_gpr174_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR174 (newval);
}

/* Get the value of h-sys-gpr175.  */

USI
or1k32bf_h_sys_gpr175_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR175 ();
}

/* Set a value for h-sys-gpr175.  */

void
or1k32bf_h_sys_gpr175_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR175 (newval);
}

/* Get the value of h-sys-gpr176.  */

USI
or1k32bf_h_sys_gpr176_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR176 ();
}

/* Set a value for h-sys-gpr176.  */

void
or1k32bf_h_sys_gpr176_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR176 (newval);
}

/* Get the value of h-sys-gpr177.  */

USI
or1k32bf_h_sys_gpr177_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR177 ();
}

/* Set a value for h-sys-gpr177.  */

void
or1k32bf_h_sys_gpr177_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR177 (newval);
}

/* Get the value of h-sys-gpr178.  */

USI
or1k32bf_h_sys_gpr178_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR178 ();
}

/* Set a value for h-sys-gpr178.  */

void
or1k32bf_h_sys_gpr178_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR178 (newval);
}

/* Get the value of h-sys-gpr179.  */

USI
or1k32bf_h_sys_gpr179_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR179 ();
}

/* Set a value for h-sys-gpr179.  */

void
or1k32bf_h_sys_gpr179_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR179 (newval);
}

/* Get the value of h-sys-gpr180.  */

USI
or1k32bf_h_sys_gpr180_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR180 ();
}

/* Set a value for h-sys-gpr180.  */

void
or1k32bf_h_sys_gpr180_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR180 (newval);
}

/* Get the value of h-sys-gpr181.  */

USI
or1k32bf_h_sys_gpr181_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR181 ();
}

/* Set a value for h-sys-gpr181.  */

void
or1k32bf_h_sys_gpr181_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR181 (newval);
}

/* Get the value of h-sys-gpr182.  */

USI
or1k32bf_h_sys_gpr182_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR182 ();
}

/* Set a value for h-sys-gpr182.  */

void
or1k32bf_h_sys_gpr182_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR182 (newval);
}

/* Get the value of h-sys-gpr183.  */

USI
or1k32bf_h_sys_gpr183_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR183 ();
}

/* Set a value for h-sys-gpr183.  */

void
or1k32bf_h_sys_gpr183_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR183 (newval);
}

/* Get the value of h-sys-gpr184.  */

USI
or1k32bf_h_sys_gpr184_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR184 ();
}

/* Set a value for h-sys-gpr184.  */

void
or1k32bf_h_sys_gpr184_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR184 (newval);
}

/* Get the value of h-sys-gpr185.  */

USI
or1k32bf_h_sys_gpr185_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR185 ();
}

/* Set a value for h-sys-gpr185.  */

void
or1k32bf_h_sys_gpr185_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR185 (newval);
}

/* Get the value of h-sys-gpr186.  */

USI
or1k32bf_h_sys_gpr186_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR186 ();
}

/* Set a value for h-sys-gpr186.  */

void
or1k32bf_h_sys_gpr186_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR186 (newval);
}

/* Get the value of h-sys-gpr187.  */

USI
or1k32bf_h_sys_gpr187_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR187 ();
}

/* Set a value for h-sys-gpr187.  */

void
or1k32bf_h_sys_gpr187_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR187 (newval);
}

/* Get the value of h-sys-gpr188.  */

USI
or1k32bf_h_sys_gpr188_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR188 ();
}

/* Set a value for h-sys-gpr188.  */

void
or1k32bf_h_sys_gpr188_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR188 (newval);
}

/* Get the value of h-sys-gpr189.  */

USI
or1k32bf_h_sys_gpr189_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR189 ();
}

/* Set a value for h-sys-gpr189.  */

void
or1k32bf_h_sys_gpr189_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR189 (newval);
}

/* Get the value of h-sys-gpr190.  */

USI
or1k32bf_h_sys_gpr190_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR190 ();
}

/* Set a value for h-sys-gpr190.  */

void
or1k32bf_h_sys_gpr190_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR190 (newval);
}

/* Get the value of h-sys-gpr191.  */

USI
or1k32bf_h_sys_gpr191_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR191 ();
}

/* Set a value for h-sys-gpr191.  */

void
or1k32bf_h_sys_gpr191_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR191 (newval);
}

/* Get the value of h-sys-gpr192.  */

USI
or1k32bf_h_sys_gpr192_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR192 ();
}

/* Set a value for h-sys-gpr192.  */

void
or1k32bf_h_sys_gpr192_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR192 (newval);
}

/* Get the value of h-sys-gpr193.  */

USI
or1k32bf_h_sys_gpr193_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR193 ();
}

/* Set a value for h-sys-gpr193.  */

void
or1k32bf_h_sys_gpr193_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR193 (newval);
}

/* Get the value of h-sys-gpr194.  */

USI
or1k32bf_h_sys_gpr194_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR194 ();
}

/* Set a value for h-sys-gpr194.  */

void
or1k32bf_h_sys_gpr194_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR194 (newval);
}

/* Get the value of h-sys-gpr195.  */

USI
or1k32bf_h_sys_gpr195_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR195 ();
}

/* Set a value for h-sys-gpr195.  */

void
or1k32bf_h_sys_gpr195_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR195 (newval);
}

/* Get the value of h-sys-gpr196.  */

USI
or1k32bf_h_sys_gpr196_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR196 ();
}

/* Set a value for h-sys-gpr196.  */

void
or1k32bf_h_sys_gpr196_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR196 (newval);
}

/* Get the value of h-sys-gpr197.  */

USI
or1k32bf_h_sys_gpr197_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR197 ();
}

/* Set a value for h-sys-gpr197.  */

void
or1k32bf_h_sys_gpr197_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR197 (newval);
}

/* Get the value of h-sys-gpr198.  */

USI
or1k32bf_h_sys_gpr198_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR198 ();
}

/* Set a value for h-sys-gpr198.  */

void
or1k32bf_h_sys_gpr198_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR198 (newval);
}

/* Get the value of h-sys-gpr199.  */

USI
or1k32bf_h_sys_gpr199_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR199 ();
}

/* Set a value for h-sys-gpr199.  */

void
or1k32bf_h_sys_gpr199_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR199 (newval);
}

/* Get the value of h-sys-gpr200.  */

USI
or1k32bf_h_sys_gpr200_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR200 ();
}

/* Set a value for h-sys-gpr200.  */

void
or1k32bf_h_sys_gpr200_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR200 (newval);
}

/* Get the value of h-sys-gpr201.  */

USI
or1k32bf_h_sys_gpr201_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR201 ();
}

/* Set a value for h-sys-gpr201.  */

void
or1k32bf_h_sys_gpr201_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR201 (newval);
}

/* Get the value of h-sys-gpr202.  */

USI
or1k32bf_h_sys_gpr202_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR202 ();
}

/* Set a value for h-sys-gpr202.  */

void
or1k32bf_h_sys_gpr202_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR202 (newval);
}

/* Get the value of h-sys-gpr203.  */

USI
or1k32bf_h_sys_gpr203_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR203 ();
}

/* Set a value for h-sys-gpr203.  */

void
or1k32bf_h_sys_gpr203_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR203 (newval);
}

/* Get the value of h-sys-gpr204.  */

USI
or1k32bf_h_sys_gpr204_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR204 ();
}

/* Set a value for h-sys-gpr204.  */

void
or1k32bf_h_sys_gpr204_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR204 (newval);
}

/* Get the value of h-sys-gpr205.  */

USI
or1k32bf_h_sys_gpr205_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR205 ();
}

/* Set a value for h-sys-gpr205.  */

void
or1k32bf_h_sys_gpr205_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR205 (newval);
}

/* Get the value of h-sys-gpr206.  */

USI
or1k32bf_h_sys_gpr206_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR206 ();
}

/* Set a value for h-sys-gpr206.  */

void
or1k32bf_h_sys_gpr206_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR206 (newval);
}

/* Get the value of h-sys-gpr207.  */

USI
or1k32bf_h_sys_gpr207_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR207 ();
}

/* Set a value for h-sys-gpr207.  */

void
or1k32bf_h_sys_gpr207_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR207 (newval);
}

/* Get the value of h-sys-gpr208.  */

USI
or1k32bf_h_sys_gpr208_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR208 ();
}

/* Set a value for h-sys-gpr208.  */

void
or1k32bf_h_sys_gpr208_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR208 (newval);
}

/* Get the value of h-sys-gpr209.  */

USI
or1k32bf_h_sys_gpr209_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR209 ();
}

/* Set a value for h-sys-gpr209.  */

void
or1k32bf_h_sys_gpr209_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR209 (newval);
}

/* Get the value of h-sys-gpr210.  */

USI
or1k32bf_h_sys_gpr210_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR210 ();
}

/* Set a value for h-sys-gpr210.  */

void
or1k32bf_h_sys_gpr210_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR210 (newval);
}

/* Get the value of h-sys-gpr211.  */

USI
or1k32bf_h_sys_gpr211_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR211 ();
}

/* Set a value for h-sys-gpr211.  */

void
or1k32bf_h_sys_gpr211_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR211 (newval);
}

/* Get the value of h-sys-gpr212.  */

USI
or1k32bf_h_sys_gpr212_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR212 ();
}

/* Set a value for h-sys-gpr212.  */

void
or1k32bf_h_sys_gpr212_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR212 (newval);
}

/* Get the value of h-sys-gpr213.  */

USI
or1k32bf_h_sys_gpr213_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR213 ();
}

/* Set a value for h-sys-gpr213.  */

void
or1k32bf_h_sys_gpr213_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR213 (newval);
}

/* Get the value of h-sys-gpr214.  */

USI
or1k32bf_h_sys_gpr214_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR214 ();
}

/* Set a value for h-sys-gpr214.  */

void
or1k32bf_h_sys_gpr214_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR214 (newval);
}

/* Get the value of h-sys-gpr215.  */

USI
or1k32bf_h_sys_gpr215_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR215 ();
}

/* Set a value for h-sys-gpr215.  */

void
or1k32bf_h_sys_gpr215_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR215 (newval);
}

/* Get the value of h-sys-gpr216.  */

USI
or1k32bf_h_sys_gpr216_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR216 ();
}

/* Set a value for h-sys-gpr216.  */

void
or1k32bf_h_sys_gpr216_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR216 (newval);
}

/* Get the value of h-sys-gpr217.  */

USI
or1k32bf_h_sys_gpr217_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR217 ();
}

/* Set a value for h-sys-gpr217.  */

void
or1k32bf_h_sys_gpr217_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR217 (newval);
}

/* Get the value of h-sys-gpr218.  */

USI
or1k32bf_h_sys_gpr218_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR218 ();
}

/* Set a value for h-sys-gpr218.  */

void
or1k32bf_h_sys_gpr218_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR218 (newval);
}

/* Get the value of h-sys-gpr219.  */

USI
or1k32bf_h_sys_gpr219_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR219 ();
}

/* Set a value for h-sys-gpr219.  */

void
or1k32bf_h_sys_gpr219_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR219 (newval);
}

/* Get the value of h-sys-gpr220.  */

USI
or1k32bf_h_sys_gpr220_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR220 ();
}

/* Set a value for h-sys-gpr220.  */

void
or1k32bf_h_sys_gpr220_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR220 (newval);
}

/* Get the value of h-sys-gpr221.  */

USI
or1k32bf_h_sys_gpr221_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR221 ();
}

/* Set a value for h-sys-gpr221.  */

void
or1k32bf_h_sys_gpr221_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR221 (newval);
}

/* Get the value of h-sys-gpr222.  */

USI
or1k32bf_h_sys_gpr222_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR222 ();
}

/* Set a value for h-sys-gpr222.  */

void
or1k32bf_h_sys_gpr222_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR222 (newval);
}

/* Get the value of h-sys-gpr223.  */

USI
or1k32bf_h_sys_gpr223_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR223 ();
}

/* Set a value for h-sys-gpr223.  */

void
or1k32bf_h_sys_gpr223_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR223 (newval);
}

/* Get the value of h-sys-gpr224.  */

USI
or1k32bf_h_sys_gpr224_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR224 ();
}

/* Set a value for h-sys-gpr224.  */

void
or1k32bf_h_sys_gpr224_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR224 (newval);
}

/* Get the value of h-sys-gpr225.  */

USI
or1k32bf_h_sys_gpr225_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR225 ();
}

/* Set a value for h-sys-gpr225.  */

void
or1k32bf_h_sys_gpr225_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR225 (newval);
}

/* Get the value of h-sys-gpr226.  */

USI
or1k32bf_h_sys_gpr226_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR226 ();
}

/* Set a value for h-sys-gpr226.  */

void
or1k32bf_h_sys_gpr226_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR226 (newval);
}

/* Get the value of h-sys-gpr227.  */

USI
or1k32bf_h_sys_gpr227_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR227 ();
}

/* Set a value for h-sys-gpr227.  */

void
or1k32bf_h_sys_gpr227_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR227 (newval);
}

/* Get the value of h-sys-gpr228.  */

USI
or1k32bf_h_sys_gpr228_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR228 ();
}

/* Set a value for h-sys-gpr228.  */

void
or1k32bf_h_sys_gpr228_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR228 (newval);
}

/* Get the value of h-sys-gpr229.  */

USI
or1k32bf_h_sys_gpr229_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR229 ();
}

/* Set a value for h-sys-gpr229.  */

void
or1k32bf_h_sys_gpr229_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR229 (newval);
}

/* Get the value of h-sys-gpr230.  */

USI
or1k32bf_h_sys_gpr230_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR230 ();
}

/* Set a value for h-sys-gpr230.  */

void
or1k32bf_h_sys_gpr230_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR230 (newval);
}

/* Get the value of h-sys-gpr231.  */

USI
or1k32bf_h_sys_gpr231_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR231 ();
}

/* Set a value for h-sys-gpr231.  */

void
or1k32bf_h_sys_gpr231_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR231 (newval);
}

/* Get the value of h-sys-gpr232.  */

USI
or1k32bf_h_sys_gpr232_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR232 ();
}

/* Set a value for h-sys-gpr232.  */

void
or1k32bf_h_sys_gpr232_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR232 (newval);
}

/* Get the value of h-sys-gpr233.  */

USI
or1k32bf_h_sys_gpr233_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR233 ();
}

/* Set a value for h-sys-gpr233.  */

void
or1k32bf_h_sys_gpr233_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR233 (newval);
}

/* Get the value of h-sys-gpr234.  */

USI
or1k32bf_h_sys_gpr234_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR234 ();
}

/* Set a value for h-sys-gpr234.  */

void
or1k32bf_h_sys_gpr234_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR234 (newval);
}

/* Get the value of h-sys-gpr235.  */

USI
or1k32bf_h_sys_gpr235_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR235 ();
}

/* Set a value for h-sys-gpr235.  */

void
or1k32bf_h_sys_gpr235_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR235 (newval);
}

/* Get the value of h-sys-gpr236.  */

USI
or1k32bf_h_sys_gpr236_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR236 ();
}

/* Set a value for h-sys-gpr236.  */

void
or1k32bf_h_sys_gpr236_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR236 (newval);
}

/* Get the value of h-sys-gpr237.  */

USI
or1k32bf_h_sys_gpr237_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR237 ();
}

/* Set a value for h-sys-gpr237.  */

void
or1k32bf_h_sys_gpr237_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR237 (newval);
}

/* Get the value of h-sys-gpr238.  */

USI
or1k32bf_h_sys_gpr238_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR238 ();
}

/* Set a value for h-sys-gpr238.  */

void
or1k32bf_h_sys_gpr238_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR238 (newval);
}

/* Get the value of h-sys-gpr239.  */

USI
or1k32bf_h_sys_gpr239_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR239 ();
}

/* Set a value for h-sys-gpr239.  */

void
or1k32bf_h_sys_gpr239_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR239 (newval);
}

/* Get the value of h-sys-gpr240.  */

USI
or1k32bf_h_sys_gpr240_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR240 ();
}

/* Set a value for h-sys-gpr240.  */

void
or1k32bf_h_sys_gpr240_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR240 (newval);
}

/* Get the value of h-sys-gpr241.  */

USI
or1k32bf_h_sys_gpr241_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR241 ();
}

/* Set a value for h-sys-gpr241.  */

void
or1k32bf_h_sys_gpr241_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR241 (newval);
}

/* Get the value of h-sys-gpr242.  */

USI
or1k32bf_h_sys_gpr242_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR242 ();
}

/* Set a value for h-sys-gpr242.  */

void
or1k32bf_h_sys_gpr242_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR242 (newval);
}

/* Get the value of h-sys-gpr243.  */

USI
or1k32bf_h_sys_gpr243_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR243 ();
}

/* Set a value for h-sys-gpr243.  */

void
or1k32bf_h_sys_gpr243_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR243 (newval);
}

/* Get the value of h-sys-gpr244.  */

USI
or1k32bf_h_sys_gpr244_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR244 ();
}

/* Set a value for h-sys-gpr244.  */

void
or1k32bf_h_sys_gpr244_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR244 (newval);
}

/* Get the value of h-sys-gpr245.  */

USI
or1k32bf_h_sys_gpr245_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR245 ();
}

/* Set a value for h-sys-gpr245.  */

void
or1k32bf_h_sys_gpr245_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR245 (newval);
}

/* Get the value of h-sys-gpr246.  */

USI
or1k32bf_h_sys_gpr246_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR246 ();
}

/* Set a value for h-sys-gpr246.  */

void
or1k32bf_h_sys_gpr246_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR246 (newval);
}

/* Get the value of h-sys-gpr247.  */

USI
or1k32bf_h_sys_gpr247_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR247 ();
}

/* Set a value for h-sys-gpr247.  */

void
or1k32bf_h_sys_gpr247_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR247 (newval);
}

/* Get the value of h-sys-gpr248.  */

USI
or1k32bf_h_sys_gpr248_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR248 ();
}

/* Set a value for h-sys-gpr248.  */

void
or1k32bf_h_sys_gpr248_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR248 (newval);
}

/* Get the value of h-sys-gpr249.  */

USI
or1k32bf_h_sys_gpr249_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR249 ();
}

/* Set a value for h-sys-gpr249.  */

void
or1k32bf_h_sys_gpr249_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR249 (newval);
}

/* Get the value of h-sys-gpr250.  */

USI
or1k32bf_h_sys_gpr250_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR250 ();
}

/* Set a value for h-sys-gpr250.  */

void
or1k32bf_h_sys_gpr250_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR250 (newval);
}

/* Get the value of h-sys-gpr251.  */

USI
or1k32bf_h_sys_gpr251_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR251 ();
}

/* Set a value for h-sys-gpr251.  */

void
or1k32bf_h_sys_gpr251_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR251 (newval);
}

/* Get the value of h-sys-gpr252.  */

USI
or1k32bf_h_sys_gpr252_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR252 ();
}

/* Set a value for h-sys-gpr252.  */

void
or1k32bf_h_sys_gpr252_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR252 (newval);
}

/* Get the value of h-sys-gpr253.  */

USI
or1k32bf_h_sys_gpr253_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR253 ();
}

/* Set a value for h-sys-gpr253.  */

void
or1k32bf_h_sys_gpr253_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR253 (newval);
}

/* Get the value of h-sys-gpr254.  */

USI
or1k32bf_h_sys_gpr254_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR254 ();
}

/* Set a value for h-sys-gpr254.  */

void
or1k32bf_h_sys_gpr254_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR254 (newval);
}

/* Get the value of h-sys-gpr255.  */

USI
or1k32bf_h_sys_gpr255_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR255 ();
}

/* Set a value for h-sys-gpr255.  */

void
or1k32bf_h_sys_gpr255_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR255 (newval);
}

/* Get the value of h-sys-gpr256.  */

USI
or1k32bf_h_sys_gpr256_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR256 ();
}

/* Set a value for h-sys-gpr256.  */

void
or1k32bf_h_sys_gpr256_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR256 (newval);
}

/* Get the value of h-sys-gpr257.  */

USI
or1k32bf_h_sys_gpr257_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR257 ();
}

/* Set a value for h-sys-gpr257.  */

void
or1k32bf_h_sys_gpr257_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR257 (newval);
}

/* Get the value of h-sys-gpr258.  */

USI
or1k32bf_h_sys_gpr258_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR258 ();
}

/* Set a value for h-sys-gpr258.  */

void
or1k32bf_h_sys_gpr258_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR258 (newval);
}

/* Get the value of h-sys-gpr259.  */

USI
or1k32bf_h_sys_gpr259_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR259 ();
}

/* Set a value for h-sys-gpr259.  */

void
or1k32bf_h_sys_gpr259_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR259 (newval);
}

/* Get the value of h-sys-gpr260.  */

USI
or1k32bf_h_sys_gpr260_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR260 ();
}

/* Set a value for h-sys-gpr260.  */

void
or1k32bf_h_sys_gpr260_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR260 (newval);
}

/* Get the value of h-sys-gpr261.  */

USI
or1k32bf_h_sys_gpr261_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR261 ();
}

/* Set a value for h-sys-gpr261.  */

void
or1k32bf_h_sys_gpr261_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR261 (newval);
}

/* Get the value of h-sys-gpr262.  */

USI
or1k32bf_h_sys_gpr262_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR262 ();
}

/* Set a value for h-sys-gpr262.  */

void
or1k32bf_h_sys_gpr262_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR262 (newval);
}

/* Get the value of h-sys-gpr263.  */

USI
or1k32bf_h_sys_gpr263_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR263 ();
}

/* Set a value for h-sys-gpr263.  */

void
or1k32bf_h_sys_gpr263_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR263 (newval);
}

/* Get the value of h-sys-gpr264.  */

USI
or1k32bf_h_sys_gpr264_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR264 ();
}

/* Set a value for h-sys-gpr264.  */

void
or1k32bf_h_sys_gpr264_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR264 (newval);
}

/* Get the value of h-sys-gpr265.  */

USI
or1k32bf_h_sys_gpr265_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR265 ();
}

/* Set a value for h-sys-gpr265.  */

void
or1k32bf_h_sys_gpr265_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR265 (newval);
}

/* Get the value of h-sys-gpr266.  */

USI
or1k32bf_h_sys_gpr266_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR266 ();
}

/* Set a value for h-sys-gpr266.  */

void
or1k32bf_h_sys_gpr266_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR266 (newval);
}

/* Get the value of h-sys-gpr267.  */

USI
or1k32bf_h_sys_gpr267_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR267 ();
}

/* Set a value for h-sys-gpr267.  */

void
or1k32bf_h_sys_gpr267_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR267 (newval);
}

/* Get the value of h-sys-gpr268.  */

USI
or1k32bf_h_sys_gpr268_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR268 ();
}

/* Set a value for h-sys-gpr268.  */

void
or1k32bf_h_sys_gpr268_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR268 (newval);
}

/* Get the value of h-sys-gpr269.  */

USI
or1k32bf_h_sys_gpr269_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR269 ();
}

/* Set a value for h-sys-gpr269.  */

void
or1k32bf_h_sys_gpr269_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR269 (newval);
}

/* Get the value of h-sys-gpr270.  */

USI
or1k32bf_h_sys_gpr270_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR270 ();
}

/* Set a value for h-sys-gpr270.  */

void
or1k32bf_h_sys_gpr270_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR270 (newval);
}

/* Get the value of h-sys-gpr271.  */

USI
or1k32bf_h_sys_gpr271_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR271 ();
}

/* Set a value for h-sys-gpr271.  */

void
or1k32bf_h_sys_gpr271_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR271 (newval);
}

/* Get the value of h-sys-gpr272.  */

USI
or1k32bf_h_sys_gpr272_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR272 ();
}

/* Set a value for h-sys-gpr272.  */

void
or1k32bf_h_sys_gpr272_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR272 (newval);
}

/* Get the value of h-sys-gpr273.  */

USI
or1k32bf_h_sys_gpr273_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR273 ();
}

/* Set a value for h-sys-gpr273.  */

void
or1k32bf_h_sys_gpr273_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR273 (newval);
}

/* Get the value of h-sys-gpr274.  */

USI
or1k32bf_h_sys_gpr274_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR274 ();
}

/* Set a value for h-sys-gpr274.  */

void
or1k32bf_h_sys_gpr274_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR274 (newval);
}

/* Get the value of h-sys-gpr275.  */

USI
or1k32bf_h_sys_gpr275_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR275 ();
}

/* Set a value for h-sys-gpr275.  */

void
or1k32bf_h_sys_gpr275_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR275 (newval);
}

/* Get the value of h-sys-gpr276.  */

USI
or1k32bf_h_sys_gpr276_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR276 ();
}

/* Set a value for h-sys-gpr276.  */

void
or1k32bf_h_sys_gpr276_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR276 (newval);
}

/* Get the value of h-sys-gpr277.  */

USI
or1k32bf_h_sys_gpr277_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR277 ();
}

/* Set a value for h-sys-gpr277.  */

void
or1k32bf_h_sys_gpr277_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR277 (newval);
}

/* Get the value of h-sys-gpr278.  */

USI
or1k32bf_h_sys_gpr278_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR278 ();
}

/* Set a value for h-sys-gpr278.  */

void
or1k32bf_h_sys_gpr278_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR278 (newval);
}

/* Get the value of h-sys-gpr279.  */

USI
or1k32bf_h_sys_gpr279_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR279 ();
}

/* Set a value for h-sys-gpr279.  */

void
or1k32bf_h_sys_gpr279_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR279 (newval);
}

/* Get the value of h-sys-gpr280.  */

USI
or1k32bf_h_sys_gpr280_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR280 ();
}

/* Set a value for h-sys-gpr280.  */

void
or1k32bf_h_sys_gpr280_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR280 (newval);
}

/* Get the value of h-sys-gpr281.  */

USI
or1k32bf_h_sys_gpr281_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR281 ();
}

/* Set a value for h-sys-gpr281.  */

void
or1k32bf_h_sys_gpr281_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR281 (newval);
}

/* Get the value of h-sys-gpr282.  */

USI
or1k32bf_h_sys_gpr282_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR282 ();
}

/* Set a value for h-sys-gpr282.  */

void
or1k32bf_h_sys_gpr282_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR282 (newval);
}

/* Get the value of h-sys-gpr283.  */

USI
or1k32bf_h_sys_gpr283_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR283 ();
}

/* Set a value for h-sys-gpr283.  */

void
or1k32bf_h_sys_gpr283_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR283 (newval);
}

/* Get the value of h-sys-gpr284.  */

USI
or1k32bf_h_sys_gpr284_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR284 ();
}

/* Set a value for h-sys-gpr284.  */

void
or1k32bf_h_sys_gpr284_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR284 (newval);
}

/* Get the value of h-sys-gpr285.  */

USI
or1k32bf_h_sys_gpr285_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR285 ();
}

/* Set a value for h-sys-gpr285.  */

void
or1k32bf_h_sys_gpr285_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR285 (newval);
}

/* Get the value of h-sys-gpr286.  */

USI
or1k32bf_h_sys_gpr286_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR286 ();
}

/* Set a value for h-sys-gpr286.  */

void
or1k32bf_h_sys_gpr286_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR286 (newval);
}

/* Get the value of h-sys-gpr287.  */

USI
or1k32bf_h_sys_gpr287_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR287 ();
}

/* Set a value for h-sys-gpr287.  */

void
or1k32bf_h_sys_gpr287_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR287 (newval);
}

/* Get the value of h-sys-gpr288.  */

USI
or1k32bf_h_sys_gpr288_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR288 ();
}

/* Set a value for h-sys-gpr288.  */

void
or1k32bf_h_sys_gpr288_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR288 (newval);
}

/* Get the value of h-sys-gpr289.  */

USI
or1k32bf_h_sys_gpr289_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR289 ();
}

/* Set a value for h-sys-gpr289.  */

void
or1k32bf_h_sys_gpr289_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR289 (newval);
}

/* Get the value of h-sys-gpr290.  */

USI
or1k32bf_h_sys_gpr290_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR290 ();
}

/* Set a value for h-sys-gpr290.  */

void
or1k32bf_h_sys_gpr290_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR290 (newval);
}

/* Get the value of h-sys-gpr291.  */

USI
or1k32bf_h_sys_gpr291_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR291 ();
}

/* Set a value for h-sys-gpr291.  */

void
or1k32bf_h_sys_gpr291_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR291 (newval);
}

/* Get the value of h-sys-gpr292.  */

USI
or1k32bf_h_sys_gpr292_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR292 ();
}

/* Set a value for h-sys-gpr292.  */

void
or1k32bf_h_sys_gpr292_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR292 (newval);
}

/* Get the value of h-sys-gpr293.  */

USI
or1k32bf_h_sys_gpr293_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR293 ();
}

/* Set a value for h-sys-gpr293.  */

void
or1k32bf_h_sys_gpr293_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR293 (newval);
}

/* Get the value of h-sys-gpr294.  */

USI
or1k32bf_h_sys_gpr294_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR294 ();
}

/* Set a value for h-sys-gpr294.  */

void
or1k32bf_h_sys_gpr294_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR294 (newval);
}

/* Get the value of h-sys-gpr295.  */

USI
or1k32bf_h_sys_gpr295_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR295 ();
}

/* Set a value for h-sys-gpr295.  */

void
or1k32bf_h_sys_gpr295_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR295 (newval);
}

/* Get the value of h-sys-gpr296.  */

USI
or1k32bf_h_sys_gpr296_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR296 ();
}

/* Set a value for h-sys-gpr296.  */

void
or1k32bf_h_sys_gpr296_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR296 (newval);
}

/* Get the value of h-sys-gpr297.  */

USI
or1k32bf_h_sys_gpr297_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR297 ();
}

/* Set a value for h-sys-gpr297.  */

void
or1k32bf_h_sys_gpr297_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR297 (newval);
}

/* Get the value of h-sys-gpr298.  */

USI
or1k32bf_h_sys_gpr298_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR298 ();
}

/* Set a value for h-sys-gpr298.  */

void
or1k32bf_h_sys_gpr298_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR298 (newval);
}

/* Get the value of h-sys-gpr299.  */

USI
or1k32bf_h_sys_gpr299_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR299 ();
}

/* Set a value for h-sys-gpr299.  */

void
or1k32bf_h_sys_gpr299_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR299 (newval);
}

/* Get the value of h-sys-gpr300.  */

USI
or1k32bf_h_sys_gpr300_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR300 ();
}

/* Set a value for h-sys-gpr300.  */

void
or1k32bf_h_sys_gpr300_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR300 (newval);
}

/* Get the value of h-sys-gpr301.  */

USI
or1k32bf_h_sys_gpr301_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR301 ();
}

/* Set a value for h-sys-gpr301.  */

void
or1k32bf_h_sys_gpr301_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR301 (newval);
}

/* Get the value of h-sys-gpr302.  */

USI
or1k32bf_h_sys_gpr302_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR302 ();
}

/* Set a value for h-sys-gpr302.  */

void
or1k32bf_h_sys_gpr302_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR302 (newval);
}

/* Get the value of h-sys-gpr303.  */

USI
or1k32bf_h_sys_gpr303_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR303 ();
}

/* Set a value for h-sys-gpr303.  */

void
or1k32bf_h_sys_gpr303_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR303 (newval);
}

/* Get the value of h-sys-gpr304.  */

USI
or1k32bf_h_sys_gpr304_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR304 ();
}

/* Set a value for h-sys-gpr304.  */

void
or1k32bf_h_sys_gpr304_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR304 (newval);
}

/* Get the value of h-sys-gpr305.  */

USI
or1k32bf_h_sys_gpr305_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR305 ();
}

/* Set a value for h-sys-gpr305.  */

void
or1k32bf_h_sys_gpr305_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR305 (newval);
}

/* Get the value of h-sys-gpr306.  */

USI
or1k32bf_h_sys_gpr306_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR306 ();
}

/* Set a value for h-sys-gpr306.  */

void
or1k32bf_h_sys_gpr306_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR306 (newval);
}

/* Get the value of h-sys-gpr307.  */

USI
or1k32bf_h_sys_gpr307_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR307 ();
}

/* Set a value for h-sys-gpr307.  */

void
or1k32bf_h_sys_gpr307_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR307 (newval);
}

/* Get the value of h-sys-gpr308.  */

USI
or1k32bf_h_sys_gpr308_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR308 ();
}

/* Set a value for h-sys-gpr308.  */

void
or1k32bf_h_sys_gpr308_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR308 (newval);
}

/* Get the value of h-sys-gpr309.  */

USI
or1k32bf_h_sys_gpr309_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR309 ();
}

/* Set a value for h-sys-gpr309.  */

void
or1k32bf_h_sys_gpr309_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR309 (newval);
}

/* Get the value of h-sys-gpr310.  */

USI
or1k32bf_h_sys_gpr310_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR310 ();
}

/* Set a value for h-sys-gpr310.  */

void
or1k32bf_h_sys_gpr310_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR310 (newval);
}

/* Get the value of h-sys-gpr311.  */

USI
or1k32bf_h_sys_gpr311_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR311 ();
}

/* Set a value for h-sys-gpr311.  */

void
or1k32bf_h_sys_gpr311_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR311 (newval);
}

/* Get the value of h-sys-gpr312.  */

USI
or1k32bf_h_sys_gpr312_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR312 ();
}

/* Set a value for h-sys-gpr312.  */

void
or1k32bf_h_sys_gpr312_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR312 (newval);
}

/* Get the value of h-sys-gpr313.  */

USI
or1k32bf_h_sys_gpr313_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR313 ();
}

/* Set a value for h-sys-gpr313.  */

void
or1k32bf_h_sys_gpr313_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR313 (newval);
}

/* Get the value of h-sys-gpr314.  */

USI
or1k32bf_h_sys_gpr314_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR314 ();
}

/* Set a value for h-sys-gpr314.  */

void
or1k32bf_h_sys_gpr314_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR314 (newval);
}

/* Get the value of h-sys-gpr315.  */

USI
or1k32bf_h_sys_gpr315_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR315 ();
}

/* Set a value for h-sys-gpr315.  */

void
or1k32bf_h_sys_gpr315_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR315 (newval);
}

/* Get the value of h-sys-gpr316.  */

USI
or1k32bf_h_sys_gpr316_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR316 ();
}

/* Set a value for h-sys-gpr316.  */

void
or1k32bf_h_sys_gpr316_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR316 (newval);
}

/* Get the value of h-sys-gpr317.  */

USI
or1k32bf_h_sys_gpr317_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR317 ();
}

/* Set a value for h-sys-gpr317.  */

void
or1k32bf_h_sys_gpr317_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR317 (newval);
}

/* Get the value of h-sys-gpr318.  */

USI
or1k32bf_h_sys_gpr318_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR318 ();
}

/* Set a value for h-sys-gpr318.  */

void
or1k32bf_h_sys_gpr318_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR318 (newval);
}

/* Get the value of h-sys-gpr319.  */

USI
or1k32bf_h_sys_gpr319_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR319 ();
}

/* Set a value for h-sys-gpr319.  */

void
or1k32bf_h_sys_gpr319_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR319 (newval);
}

/* Get the value of h-sys-gpr320.  */

USI
or1k32bf_h_sys_gpr320_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR320 ();
}

/* Set a value for h-sys-gpr320.  */

void
or1k32bf_h_sys_gpr320_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR320 (newval);
}

/* Get the value of h-sys-gpr321.  */

USI
or1k32bf_h_sys_gpr321_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR321 ();
}

/* Set a value for h-sys-gpr321.  */

void
or1k32bf_h_sys_gpr321_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR321 (newval);
}

/* Get the value of h-sys-gpr322.  */

USI
or1k32bf_h_sys_gpr322_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR322 ();
}

/* Set a value for h-sys-gpr322.  */

void
or1k32bf_h_sys_gpr322_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR322 (newval);
}

/* Get the value of h-sys-gpr323.  */

USI
or1k32bf_h_sys_gpr323_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR323 ();
}

/* Set a value for h-sys-gpr323.  */

void
or1k32bf_h_sys_gpr323_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR323 (newval);
}

/* Get the value of h-sys-gpr324.  */

USI
or1k32bf_h_sys_gpr324_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR324 ();
}

/* Set a value for h-sys-gpr324.  */

void
or1k32bf_h_sys_gpr324_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR324 (newval);
}

/* Get the value of h-sys-gpr325.  */

USI
or1k32bf_h_sys_gpr325_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR325 ();
}

/* Set a value for h-sys-gpr325.  */

void
or1k32bf_h_sys_gpr325_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR325 (newval);
}

/* Get the value of h-sys-gpr326.  */

USI
or1k32bf_h_sys_gpr326_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR326 ();
}

/* Set a value for h-sys-gpr326.  */

void
or1k32bf_h_sys_gpr326_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR326 (newval);
}

/* Get the value of h-sys-gpr327.  */

USI
or1k32bf_h_sys_gpr327_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR327 ();
}

/* Set a value for h-sys-gpr327.  */

void
or1k32bf_h_sys_gpr327_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR327 (newval);
}

/* Get the value of h-sys-gpr328.  */

USI
or1k32bf_h_sys_gpr328_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR328 ();
}

/* Set a value for h-sys-gpr328.  */

void
or1k32bf_h_sys_gpr328_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR328 (newval);
}

/* Get the value of h-sys-gpr329.  */

USI
or1k32bf_h_sys_gpr329_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR329 ();
}

/* Set a value for h-sys-gpr329.  */

void
or1k32bf_h_sys_gpr329_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR329 (newval);
}

/* Get the value of h-sys-gpr330.  */

USI
or1k32bf_h_sys_gpr330_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR330 ();
}

/* Set a value for h-sys-gpr330.  */

void
or1k32bf_h_sys_gpr330_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR330 (newval);
}

/* Get the value of h-sys-gpr331.  */

USI
or1k32bf_h_sys_gpr331_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR331 ();
}

/* Set a value for h-sys-gpr331.  */

void
or1k32bf_h_sys_gpr331_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR331 (newval);
}

/* Get the value of h-sys-gpr332.  */

USI
or1k32bf_h_sys_gpr332_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR332 ();
}

/* Set a value for h-sys-gpr332.  */

void
or1k32bf_h_sys_gpr332_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR332 (newval);
}

/* Get the value of h-sys-gpr333.  */

USI
or1k32bf_h_sys_gpr333_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR333 ();
}

/* Set a value for h-sys-gpr333.  */

void
or1k32bf_h_sys_gpr333_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR333 (newval);
}

/* Get the value of h-sys-gpr334.  */

USI
or1k32bf_h_sys_gpr334_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR334 ();
}

/* Set a value for h-sys-gpr334.  */

void
or1k32bf_h_sys_gpr334_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR334 (newval);
}

/* Get the value of h-sys-gpr335.  */

USI
or1k32bf_h_sys_gpr335_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR335 ();
}

/* Set a value for h-sys-gpr335.  */

void
or1k32bf_h_sys_gpr335_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR335 (newval);
}

/* Get the value of h-sys-gpr336.  */

USI
or1k32bf_h_sys_gpr336_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR336 ();
}

/* Set a value for h-sys-gpr336.  */

void
or1k32bf_h_sys_gpr336_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR336 (newval);
}

/* Get the value of h-sys-gpr337.  */

USI
or1k32bf_h_sys_gpr337_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR337 ();
}

/* Set a value for h-sys-gpr337.  */

void
or1k32bf_h_sys_gpr337_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR337 (newval);
}

/* Get the value of h-sys-gpr338.  */

USI
or1k32bf_h_sys_gpr338_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR338 ();
}

/* Set a value for h-sys-gpr338.  */

void
or1k32bf_h_sys_gpr338_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR338 (newval);
}

/* Get the value of h-sys-gpr339.  */

USI
or1k32bf_h_sys_gpr339_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR339 ();
}

/* Set a value for h-sys-gpr339.  */

void
or1k32bf_h_sys_gpr339_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR339 (newval);
}

/* Get the value of h-sys-gpr340.  */

USI
or1k32bf_h_sys_gpr340_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR340 ();
}

/* Set a value for h-sys-gpr340.  */

void
or1k32bf_h_sys_gpr340_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR340 (newval);
}

/* Get the value of h-sys-gpr341.  */

USI
or1k32bf_h_sys_gpr341_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR341 ();
}

/* Set a value for h-sys-gpr341.  */

void
or1k32bf_h_sys_gpr341_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR341 (newval);
}

/* Get the value of h-sys-gpr342.  */

USI
or1k32bf_h_sys_gpr342_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR342 ();
}

/* Set a value for h-sys-gpr342.  */

void
or1k32bf_h_sys_gpr342_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR342 (newval);
}

/* Get the value of h-sys-gpr343.  */

USI
or1k32bf_h_sys_gpr343_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR343 ();
}

/* Set a value for h-sys-gpr343.  */

void
or1k32bf_h_sys_gpr343_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR343 (newval);
}

/* Get the value of h-sys-gpr344.  */

USI
or1k32bf_h_sys_gpr344_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR344 ();
}

/* Set a value for h-sys-gpr344.  */

void
or1k32bf_h_sys_gpr344_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR344 (newval);
}

/* Get the value of h-sys-gpr345.  */

USI
or1k32bf_h_sys_gpr345_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR345 ();
}

/* Set a value for h-sys-gpr345.  */

void
or1k32bf_h_sys_gpr345_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR345 (newval);
}

/* Get the value of h-sys-gpr346.  */

USI
or1k32bf_h_sys_gpr346_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR346 ();
}

/* Set a value for h-sys-gpr346.  */

void
or1k32bf_h_sys_gpr346_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR346 (newval);
}

/* Get the value of h-sys-gpr347.  */

USI
or1k32bf_h_sys_gpr347_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR347 ();
}

/* Set a value for h-sys-gpr347.  */

void
or1k32bf_h_sys_gpr347_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR347 (newval);
}

/* Get the value of h-sys-gpr348.  */

USI
or1k32bf_h_sys_gpr348_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR348 ();
}

/* Set a value for h-sys-gpr348.  */

void
or1k32bf_h_sys_gpr348_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR348 (newval);
}

/* Get the value of h-sys-gpr349.  */

USI
or1k32bf_h_sys_gpr349_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR349 ();
}

/* Set a value for h-sys-gpr349.  */

void
or1k32bf_h_sys_gpr349_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR349 (newval);
}

/* Get the value of h-sys-gpr350.  */

USI
or1k32bf_h_sys_gpr350_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR350 ();
}

/* Set a value for h-sys-gpr350.  */

void
or1k32bf_h_sys_gpr350_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR350 (newval);
}

/* Get the value of h-sys-gpr351.  */

USI
or1k32bf_h_sys_gpr351_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR351 ();
}

/* Set a value for h-sys-gpr351.  */

void
or1k32bf_h_sys_gpr351_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR351 (newval);
}

/* Get the value of h-sys-gpr352.  */

USI
or1k32bf_h_sys_gpr352_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR352 ();
}

/* Set a value for h-sys-gpr352.  */

void
or1k32bf_h_sys_gpr352_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR352 (newval);
}

/* Get the value of h-sys-gpr353.  */

USI
or1k32bf_h_sys_gpr353_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR353 ();
}

/* Set a value for h-sys-gpr353.  */

void
or1k32bf_h_sys_gpr353_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR353 (newval);
}

/* Get the value of h-sys-gpr354.  */

USI
or1k32bf_h_sys_gpr354_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR354 ();
}

/* Set a value for h-sys-gpr354.  */

void
or1k32bf_h_sys_gpr354_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR354 (newval);
}

/* Get the value of h-sys-gpr355.  */

USI
or1k32bf_h_sys_gpr355_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR355 ();
}

/* Set a value for h-sys-gpr355.  */

void
or1k32bf_h_sys_gpr355_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR355 (newval);
}

/* Get the value of h-sys-gpr356.  */

USI
or1k32bf_h_sys_gpr356_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR356 ();
}

/* Set a value for h-sys-gpr356.  */

void
or1k32bf_h_sys_gpr356_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR356 (newval);
}

/* Get the value of h-sys-gpr357.  */

USI
or1k32bf_h_sys_gpr357_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR357 ();
}

/* Set a value for h-sys-gpr357.  */

void
or1k32bf_h_sys_gpr357_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR357 (newval);
}

/* Get the value of h-sys-gpr358.  */

USI
or1k32bf_h_sys_gpr358_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR358 ();
}

/* Set a value for h-sys-gpr358.  */

void
or1k32bf_h_sys_gpr358_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR358 (newval);
}

/* Get the value of h-sys-gpr359.  */

USI
or1k32bf_h_sys_gpr359_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR359 ();
}

/* Set a value for h-sys-gpr359.  */

void
or1k32bf_h_sys_gpr359_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR359 (newval);
}

/* Get the value of h-sys-gpr360.  */

USI
or1k32bf_h_sys_gpr360_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR360 ();
}

/* Set a value for h-sys-gpr360.  */

void
or1k32bf_h_sys_gpr360_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR360 (newval);
}

/* Get the value of h-sys-gpr361.  */

USI
or1k32bf_h_sys_gpr361_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR361 ();
}

/* Set a value for h-sys-gpr361.  */

void
or1k32bf_h_sys_gpr361_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR361 (newval);
}

/* Get the value of h-sys-gpr362.  */

USI
or1k32bf_h_sys_gpr362_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR362 ();
}

/* Set a value for h-sys-gpr362.  */

void
or1k32bf_h_sys_gpr362_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR362 (newval);
}

/* Get the value of h-sys-gpr363.  */

USI
or1k32bf_h_sys_gpr363_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR363 ();
}

/* Set a value for h-sys-gpr363.  */

void
or1k32bf_h_sys_gpr363_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR363 (newval);
}

/* Get the value of h-sys-gpr364.  */

USI
or1k32bf_h_sys_gpr364_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR364 ();
}

/* Set a value for h-sys-gpr364.  */

void
or1k32bf_h_sys_gpr364_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR364 (newval);
}

/* Get the value of h-sys-gpr365.  */

USI
or1k32bf_h_sys_gpr365_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR365 ();
}

/* Set a value for h-sys-gpr365.  */

void
or1k32bf_h_sys_gpr365_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR365 (newval);
}

/* Get the value of h-sys-gpr366.  */

USI
or1k32bf_h_sys_gpr366_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR366 ();
}

/* Set a value for h-sys-gpr366.  */

void
or1k32bf_h_sys_gpr366_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR366 (newval);
}

/* Get the value of h-sys-gpr367.  */

USI
or1k32bf_h_sys_gpr367_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR367 ();
}

/* Set a value for h-sys-gpr367.  */

void
or1k32bf_h_sys_gpr367_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR367 (newval);
}

/* Get the value of h-sys-gpr368.  */

USI
or1k32bf_h_sys_gpr368_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR368 ();
}

/* Set a value for h-sys-gpr368.  */

void
or1k32bf_h_sys_gpr368_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR368 (newval);
}

/* Get the value of h-sys-gpr369.  */

USI
or1k32bf_h_sys_gpr369_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR369 ();
}

/* Set a value for h-sys-gpr369.  */

void
or1k32bf_h_sys_gpr369_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR369 (newval);
}

/* Get the value of h-sys-gpr370.  */

USI
or1k32bf_h_sys_gpr370_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR370 ();
}

/* Set a value for h-sys-gpr370.  */

void
or1k32bf_h_sys_gpr370_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR370 (newval);
}

/* Get the value of h-sys-gpr371.  */

USI
or1k32bf_h_sys_gpr371_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR371 ();
}

/* Set a value for h-sys-gpr371.  */

void
or1k32bf_h_sys_gpr371_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR371 (newval);
}

/* Get the value of h-sys-gpr372.  */

USI
or1k32bf_h_sys_gpr372_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR372 ();
}

/* Set a value for h-sys-gpr372.  */

void
or1k32bf_h_sys_gpr372_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR372 (newval);
}

/* Get the value of h-sys-gpr373.  */

USI
or1k32bf_h_sys_gpr373_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR373 ();
}

/* Set a value for h-sys-gpr373.  */

void
or1k32bf_h_sys_gpr373_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR373 (newval);
}

/* Get the value of h-sys-gpr374.  */

USI
or1k32bf_h_sys_gpr374_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR374 ();
}

/* Set a value for h-sys-gpr374.  */

void
or1k32bf_h_sys_gpr374_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR374 (newval);
}

/* Get the value of h-sys-gpr375.  */

USI
or1k32bf_h_sys_gpr375_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR375 ();
}

/* Set a value for h-sys-gpr375.  */

void
or1k32bf_h_sys_gpr375_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR375 (newval);
}

/* Get the value of h-sys-gpr376.  */

USI
or1k32bf_h_sys_gpr376_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR376 ();
}

/* Set a value for h-sys-gpr376.  */

void
or1k32bf_h_sys_gpr376_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR376 (newval);
}

/* Get the value of h-sys-gpr377.  */

USI
or1k32bf_h_sys_gpr377_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR377 ();
}

/* Set a value for h-sys-gpr377.  */

void
or1k32bf_h_sys_gpr377_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR377 (newval);
}

/* Get the value of h-sys-gpr378.  */

USI
or1k32bf_h_sys_gpr378_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR378 ();
}

/* Set a value for h-sys-gpr378.  */

void
or1k32bf_h_sys_gpr378_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR378 (newval);
}

/* Get the value of h-sys-gpr379.  */

USI
or1k32bf_h_sys_gpr379_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR379 ();
}

/* Set a value for h-sys-gpr379.  */

void
or1k32bf_h_sys_gpr379_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR379 (newval);
}

/* Get the value of h-sys-gpr380.  */

USI
or1k32bf_h_sys_gpr380_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR380 ();
}

/* Set a value for h-sys-gpr380.  */

void
or1k32bf_h_sys_gpr380_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR380 (newval);
}

/* Get the value of h-sys-gpr381.  */

USI
or1k32bf_h_sys_gpr381_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR381 ();
}

/* Set a value for h-sys-gpr381.  */

void
or1k32bf_h_sys_gpr381_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR381 (newval);
}

/* Get the value of h-sys-gpr382.  */

USI
or1k32bf_h_sys_gpr382_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR382 ();
}

/* Set a value for h-sys-gpr382.  */

void
or1k32bf_h_sys_gpr382_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR382 (newval);
}

/* Get the value of h-sys-gpr383.  */

USI
or1k32bf_h_sys_gpr383_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR383 ();
}

/* Set a value for h-sys-gpr383.  */

void
or1k32bf_h_sys_gpr383_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR383 (newval);
}

/* Get the value of h-sys-gpr384.  */

USI
or1k32bf_h_sys_gpr384_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR384 ();
}

/* Set a value for h-sys-gpr384.  */

void
or1k32bf_h_sys_gpr384_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR384 (newval);
}

/* Get the value of h-sys-gpr385.  */

USI
or1k32bf_h_sys_gpr385_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR385 ();
}

/* Set a value for h-sys-gpr385.  */

void
or1k32bf_h_sys_gpr385_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR385 (newval);
}

/* Get the value of h-sys-gpr386.  */

USI
or1k32bf_h_sys_gpr386_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR386 ();
}

/* Set a value for h-sys-gpr386.  */

void
or1k32bf_h_sys_gpr386_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR386 (newval);
}

/* Get the value of h-sys-gpr387.  */

USI
or1k32bf_h_sys_gpr387_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR387 ();
}

/* Set a value for h-sys-gpr387.  */

void
or1k32bf_h_sys_gpr387_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR387 (newval);
}

/* Get the value of h-sys-gpr388.  */

USI
or1k32bf_h_sys_gpr388_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR388 ();
}

/* Set a value for h-sys-gpr388.  */

void
or1k32bf_h_sys_gpr388_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR388 (newval);
}

/* Get the value of h-sys-gpr389.  */

USI
or1k32bf_h_sys_gpr389_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR389 ();
}

/* Set a value for h-sys-gpr389.  */

void
or1k32bf_h_sys_gpr389_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR389 (newval);
}

/* Get the value of h-sys-gpr390.  */

USI
or1k32bf_h_sys_gpr390_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR390 ();
}

/* Set a value for h-sys-gpr390.  */

void
or1k32bf_h_sys_gpr390_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR390 (newval);
}

/* Get the value of h-sys-gpr391.  */

USI
or1k32bf_h_sys_gpr391_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR391 ();
}

/* Set a value for h-sys-gpr391.  */

void
or1k32bf_h_sys_gpr391_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR391 (newval);
}

/* Get the value of h-sys-gpr392.  */

USI
or1k32bf_h_sys_gpr392_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR392 ();
}

/* Set a value for h-sys-gpr392.  */

void
or1k32bf_h_sys_gpr392_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR392 (newval);
}

/* Get the value of h-sys-gpr393.  */

USI
or1k32bf_h_sys_gpr393_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR393 ();
}

/* Set a value for h-sys-gpr393.  */

void
or1k32bf_h_sys_gpr393_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR393 (newval);
}

/* Get the value of h-sys-gpr394.  */

USI
or1k32bf_h_sys_gpr394_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR394 ();
}

/* Set a value for h-sys-gpr394.  */

void
or1k32bf_h_sys_gpr394_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR394 (newval);
}

/* Get the value of h-sys-gpr395.  */

USI
or1k32bf_h_sys_gpr395_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR395 ();
}

/* Set a value for h-sys-gpr395.  */

void
or1k32bf_h_sys_gpr395_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR395 (newval);
}

/* Get the value of h-sys-gpr396.  */

USI
or1k32bf_h_sys_gpr396_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR396 ();
}

/* Set a value for h-sys-gpr396.  */

void
or1k32bf_h_sys_gpr396_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR396 (newval);
}

/* Get the value of h-sys-gpr397.  */

USI
or1k32bf_h_sys_gpr397_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR397 ();
}

/* Set a value for h-sys-gpr397.  */

void
or1k32bf_h_sys_gpr397_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR397 (newval);
}

/* Get the value of h-sys-gpr398.  */

USI
or1k32bf_h_sys_gpr398_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR398 ();
}

/* Set a value for h-sys-gpr398.  */

void
or1k32bf_h_sys_gpr398_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR398 (newval);
}

/* Get the value of h-sys-gpr399.  */

USI
or1k32bf_h_sys_gpr399_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR399 ();
}

/* Set a value for h-sys-gpr399.  */

void
or1k32bf_h_sys_gpr399_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR399 (newval);
}

/* Get the value of h-sys-gpr400.  */

USI
or1k32bf_h_sys_gpr400_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR400 ();
}

/* Set a value for h-sys-gpr400.  */

void
or1k32bf_h_sys_gpr400_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR400 (newval);
}

/* Get the value of h-sys-gpr401.  */

USI
or1k32bf_h_sys_gpr401_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR401 ();
}

/* Set a value for h-sys-gpr401.  */

void
or1k32bf_h_sys_gpr401_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR401 (newval);
}

/* Get the value of h-sys-gpr402.  */

USI
or1k32bf_h_sys_gpr402_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR402 ();
}

/* Set a value for h-sys-gpr402.  */

void
or1k32bf_h_sys_gpr402_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR402 (newval);
}

/* Get the value of h-sys-gpr403.  */

USI
or1k32bf_h_sys_gpr403_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR403 ();
}

/* Set a value for h-sys-gpr403.  */

void
or1k32bf_h_sys_gpr403_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR403 (newval);
}

/* Get the value of h-sys-gpr404.  */

USI
or1k32bf_h_sys_gpr404_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR404 ();
}

/* Set a value for h-sys-gpr404.  */

void
or1k32bf_h_sys_gpr404_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR404 (newval);
}

/* Get the value of h-sys-gpr405.  */

USI
or1k32bf_h_sys_gpr405_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR405 ();
}

/* Set a value for h-sys-gpr405.  */

void
or1k32bf_h_sys_gpr405_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR405 (newval);
}

/* Get the value of h-sys-gpr406.  */

USI
or1k32bf_h_sys_gpr406_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR406 ();
}

/* Set a value for h-sys-gpr406.  */

void
or1k32bf_h_sys_gpr406_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR406 (newval);
}

/* Get the value of h-sys-gpr407.  */

USI
or1k32bf_h_sys_gpr407_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR407 ();
}

/* Set a value for h-sys-gpr407.  */

void
or1k32bf_h_sys_gpr407_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR407 (newval);
}

/* Get the value of h-sys-gpr408.  */

USI
or1k32bf_h_sys_gpr408_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR408 ();
}

/* Set a value for h-sys-gpr408.  */

void
or1k32bf_h_sys_gpr408_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR408 (newval);
}

/* Get the value of h-sys-gpr409.  */

USI
or1k32bf_h_sys_gpr409_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR409 ();
}

/* Set a value for h-sys-gpr409.  */

void
or1k32bf_h_sys_gpr409_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR409 (newval);
}

/* Get the value of h-sys-gpr410.  */

USI
or1k32bf_h_sys_gpr410_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR410 ();
}

/* Set a value for h-sys-gpr410.  */

void
or1k32bf_h_sys_gpr410_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR410 (newval);
}

/* Get the value of h-sys-gpr411.  */

USI
or1k32bf_h_sys_gpr411_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR411 ();
}

/* Set a value for h-sys-gpr411.  */

void
or1k32bf_h_sys_gpr411_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR411 (newval);
}

/* Get the value of h-sys-gpr412.  */

USI
or1k32bf_h_sys_gpr412_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR412 ();
}

/* Set a value for h-sys-gpr412.  */

void
or1k32bf_h_sys_gpr412_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR412 (newval);
}

/* Get the value of h-sys-gpr413.  */

USI
or1k32bf_h_sys_gpr413_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR413 ();
}

/* Set a value for h-sys-gpr413.  */

void
or1k32bf_h_sys_gpr413_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR413 (newval);
}

/* Get the value of h-sys-gpr414.  */

USI
or1k32bf_h_sys_gpr414_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR414 ();
}

/* Set a value for h-sys-gpr414.  */

void
or1k32bf_h_sys_gpr414_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR414 (newval);
}

/* Get the value of h-sys-gpr415.  */

USI
or1k32bf_h_sys_gpr415_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR415 ();
}

/* Set a value for h-sys-gpr415.  */

void
or1k32bf_h_sys_gpr415_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR415 (newval);
}

/* Get the value of h-sys-gpr416.  */

USI
or1k32bf_h_sys_gpr416_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR416 ();
}

/* Set a value for h-sys-gpr416.  */

void
or1k32bf_h_sys_gpr416_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR416 (newval);
}

/* Get the value of h-sys-gpr417.  */

USI
or1k32bf_h_sys_gpr417_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR417 ();
}

/* Set a value for h-sys-gpr417.  */

void
or1k32bf_h_sys_gpr417_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR417 (newval);
}

/* Get the value of h-sys-gpr418.  */

USI
or1k32bf_h_sys_gpr418_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR418 ();
}

/* Set a value for h-sys-gpr418.  */

void
or1k32bf_h_sys_gpr418_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR418 (newval);
}

/* Get the value of h-sys-gpr419.  */

USI
or1k32bf_h_sys_gpr419_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR419 ();
}

/* Set a value for h-sys-gpr419.  */

void
or1k32bf_h_sys_gpr419_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR419 (newval);
}

/* Get the value of h-sys-gpr420.  */

USI
or1k32bf_h_sys_gpr420_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR420 ();
}

/* Set a value for h-sys-gpr420.  */

void
or1k32bf_h_sys_gpr420_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR420 (newval);
}

/* Get the value of h-sys-gpr421.  */

USI
or1k32bf_h_sys_gpr421_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR421 ();
}

/* Set a value for h-sys-gpr421.  */

void
or1k32bf_h_sys_gpr421_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR421 (newval);
}

/* Get the value of h-sys-gpr422.  */

USI
or1k32bf_h_sys_gpr422_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR422 ();
}

/* Set a value for h-sys-gpr422.  */

void
or1k32bf_h_sys_gpr422_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR422 (newval);
}

/* Get the value of h-sys-gpr423.  */

USI
or1k32bf_h_sys_gpr423_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR423 ();
}

/* Set a value for h-sys-gpr423.  */

void
or1k32bf_h_sys_gpr423_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR423 (newval);
}

/* Get the value of h-sys-gpr424.  */

USI
or1k32bf_h_sys_gpr424_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR424 ();
}

/* Set a value for h-sys-gpr424.  */

void
or1k32bf_h_sys_gpr424_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR424 (newval);
}

/* Get the value of h-sys-gpr425.  */

USI
or1k32bf_h_sys_gpr425_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR425 ();
}

/* Set a value for h-sys-gpr425.  */

void
or1k32bf_h_sys_gpr425_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR425 (newval);
}

/* Get the value of h-sys-gpr426.  */

USI
or1k32bf_h_sys_gpr426_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR426 ();
}

/* Set a value for h-sys-gpr426.  */

void
or1k32bf_h_sys_gpr426_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR426 (newval);
}

/* Get the value of h-sys-gpr427.  */

USI
or1k32bf_h_sys_gpr427_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR427 ();
}

/* Set a value for h-sys-gpr427.  */

void
or1k32bf_h_sys_gpr427_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR427 (newval);
}

/* Get the value of h-sys-gpr428.  */

USI
or1k32bf_h_sys_gpr428_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR428 ();
}

/* Set a value for h-sys-gpr428.  */

void
or1k32bf_h_sys_gpr428_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR428 (newval);
}

/* Get the value of h-sys-gpr429.  */

USI
or1k32bf_h_sys_gpr429_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR429 ();
}

/* Set a value for h-sys-gpr429.  */

void
or1k32bf_h_sys_gpr429_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR429 (newval);
}

/* Get the value of h-sys-gpr430.  */

USI
or1k32bf_h_sys_gpr430_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR430 ();
}

/* Set a value for h-sys-gpr430.  */

void
or1k32bf_h_sys_gpr430_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR430 (newval);
}

/* Get the value of h-sys-gpr431.  */

USI
or1k32bf_h_sys_gpr431_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR431 ();
}

/* Set a value for h-sys-gpr431.  */

void
or1k32bf_h_sys_gpr431_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR431 (newval);
}

/* Get the value of h-sys-gpr432.  */

USI
or1k32bf_h_sys_gpr432_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR432 ();
}

/* Set a value for h-sys-gpr432.  */

void
or1k32bf_h_sys_gpr432_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR432 (newval);
}

/* Get the value of h-sys-gpr433.  */

USI
or1k32bf_h_sys_gpr433_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR433 ();
}

/* Set a value for h-sys-gpr433.  */

void
or1k32bf_h_sys_gpr433_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR433 (newval);
}

/* Get the value of h-sys-gpr434.  */

USI
or1k32bf_h_sys_gpr434_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR434 ();
}

/* Set a value for h-sys-gpr434.  */

void
or1k32bf_h_sys_gpr434_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR434 (newval);
}

/* Get the value of h-sys-gpr435.  */

USI
or1k32bf_h_sys_gpr435_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR435 ();
}

/* Set a value for h-sys-gpr435.  */

void
or1k32bf_h_sys_gpr435_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR435 (newval);
}

/* Get the value of h-sys-gpr436.  */

USI
or1k32bf_h_sys_gpr436_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR436 ();
}

/* Set a value for h-sys-gpr436.  */

void
or1k32bf_h_sys_gpr436_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR436 (newval);
}

/* Get the value of h-sys-gpr437.  */

USI
or1k32bf_h_sys_gpr437_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR437 ();
}

/* Set a value for h-sys-gpr437.  */

void
or1k32bf_h_sys_gpr437_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR437 (newval);
}

/* Get the value of h-sys-gpr438.  */

USI
or1k32bf_h_sys_gpr438_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR438 ();
}

/* Set a value for h-sys-gpr438.  */

void
or1k32bf_h_sys_gpr438_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR438 (newval);
}

/* Get the value of h-sys-gpr439.  */

USI
or1k32bf_h_sys_gpr439_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR439 ();
}

/* Set a value for h-sys-gpr439.  */

void
or1k32bf_h_sys_gpr439_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR439 (newval);
}

/* Get the value of h-sys-gpr440.  */

USI
or1k32bf_h_sys_gpr440_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR440 ();
}

/* Set a value for h-sys-gpr440.  */

void
or1k32bf_h_sys_gpr440_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR440 (newval);
}

/* Get the value of h-sys-gpr441.  */

USI
or1k32bf_h_sys_gpr441_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR441 ();
}

/* Set a value for h-sys-gpr441.  */

void
or1k32bf_h_sys_gpr441_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR441 (newval);
}

/* Get the value of h-sys-gpr442.  */

USI
or1k32bf_h_sys_gpr442_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR442 ();
}

/* Set a value for h-sys-gpr442.  */

void
or1k32bf_h_sys_gpr442_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR442 (newval);
}

/* Get the value of h-sys-gpr443.  */

USI
or1k32bf_h_sys_gpr443_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR443 ();
}

/* Set a value for h-sys-gpr443.  */

void
or1k32bf_h_sys_gpr443_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR443 (newval);
}

/* Get the value of h-sys-gpr444.  */

USI
or1k32bf_h_sys_gpr444_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR444 ();
}

/* Set a value for h-sys-gpr444.  */

void
or1k32bf_h_sys_gpr444_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR444 (newval);
}

/* Get the value of h-sys-gpr445.  */

USI
or1k32bf_h_sys_gpr445_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR445 ();
}

/* Set a value for h-sys-gpr445.  */

void
or1k32bf_h_sys_gpr445_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR445 (newval);
}

/* Get the value of h-sys-gpr446.  */

USI
or1k32bf_h_sys_gpr446_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR446 ();
}

/* Set a value for h-sys-gpr446.  */

void
or1k32bf_h_sys_gpr446_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR446 (newval);
}

/* Get the value of h-sys-gpr447.  */

USI
or1k32bf_h_sys_gpr447_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR447 ();
}

/* Set a value for h-sys-gpr447.  */

void
or1k32bf_h_sys_gpr447_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR447 (newval);
}

/* Get the value of h-sys-gpr448.  */

USI
or1k32bf_h_sys_gpr448_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR448 ();
}

/* Set a value for h-sys-gpr448.  */

void
or1k32bf_h_sys_gpr448_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR448 (newval);
}

/* Get the value of h-sys-gpr449.  */

USI
or1k32bf_h_sys_gpr449_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR449 ();
}

/* Set a value for h-sys-gpr449.  */

void
or1k32bf_h_sys_gpr449_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR449 (newval);
}

/* Get the value of h-sys-gpr450.  */

USI
or1k32bf_h_sys_gpr450_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR450 ();
}

/* Set a value for h-sys-gpr450.  */

void
or1k32bf_h_sys_gpr450_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR450 (newval);
}

/* Get the value of h-sys-gpr451.  */

USI
or1k32bf_h_sys_gpr451_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR451 ();
}

/* Set a value for h-sys-gpr451.  */

void
or1k32bf_h_sys_gpr451_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR451 (newval);
}

/* Get the value of h-sys-gpr452.  */

USI
or1k32bf_h_sys_gpr452_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR452 ();
}

/* Set a value for h-sys-gpr452.  */

void
or1k32bf_h_sys_gpr452_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR452 (newval);
}

/* Get the value of h-sys-gpr453.  */

USI
or1k32bf_h_sys_gpr453_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR453 ();
}

/* Set a value for h-sys-gpr453.  */

void
or1k32bf_h_sys_gpr453_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR453 (newval);
}

/* Get the value of h-sys-gpr454.  */

USI
or1k32bf_h_sys_gpr454_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR454 ();
}

/* Set a value for h-sys-gpr454.  */

void
or1k32bf_h_sys_gpr454_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR454 (newval);
}

/* Get the value of h-sys-gpr455.  */

USI
or1k32bf_h_sys_gpr455_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR455 ();
}

/* Set a value for h-sys-gpr455.  */

void
or1k32bf_h_sys_gpr455_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR455 (newval);
}

/* Get the value of h-sys-gpr456.  */

USI
or1k32bf_h_sys_gpr456_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR456 ();
}

/* Set a value for h-sys-gpr456.  */

void
or1k32bf_h_sys_gpr456_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR456 (newval);
}

/* Get the value of h-sys-gpr457.  */

USI
or1k32bf_h_sys_gpr457_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR457 ();
}

/* Set a value for h-sys-gpr457.  */

void
or1k32bf_h_sys_gpr457_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR457 (newval);
}

/* Get the value of h-sys-gpr458.  */

USI
or1k32bf_h_sys_gpr458_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR458 ();
}

/* Set a value for h-sys-gpr458.  */

void
or1k32bf_h_sys_gpr458_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR458 (newval);
}

/* Get the value of h-sys-gpr459.  */

USI
or1k32bf_h_sys_gpr459_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR459 ();
}

/* Set a value for h-sys-gpr459.  */

void
or1k32bf_h_sys_gpr459_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR459 (newval);
}

/* Get the value of h-sys-gpr460.  */

USI
or1k32bf_h_sys_gpr460_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR460 ();
}

/* Set a value for h-sys-gpr460.  */

void
or1k32bf_h_sys_gpr460_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR460 (newval);
}

/* Get the value of h-sys-gpr461.  */

USI
or1k32bf_h_sys_gpr461_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR461 ();
}

/* Set a value for h-sys-gpr461.  */

void
or1k32bf_h_sys_gpr461_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR461 (newval);
}

/* Get the value of h-sys-gpr462.  */

USI
or1k32bf_h_sys_gpr462_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR462 ();
}

/* Set a value for h-sys-gpr462.  */

void
or1k32bf_h_sys_gpr462_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR462 (newval);
}

/* Get the value of h-sys-gpr463.  */

USI
or1k32bf_h_sys_gpr463_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR463 ();
}

/* Set a value for h-sys-gpr463.  */

void
or1k32bf_h_sys_gpr463_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR463 (newval);
}

/* Get the value of h-sys-gpr464.  */

USI
or1k32bf_h_sys_gpr464_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR464 ();
}

/* Set a value for h-sys-gpr464.  */

void
or1k32bf_h_sys_gpr464_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR464 (newval);
}

/* Get the value of h-sys-gpr465.  */

USI
or1k32bf_h_sys_gpr465_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR465 ();
}

/* Set a value for h-sys-gpr465.  */

void
or1k32bf_h_sys_gpr465_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR465 (newval);
}

/* Get the value of h-sys-gpr466.  */

USI
or1k32bf_h_sys_gpr466_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR466 ();
}

/* Set a value for h-sys-gpr466.  */

void
or1k32bf_h_sys_gpr466_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR466 (newval);
}

/* Get the value of h-sys-gpr467.  */

USI
or1k32bf_h_sys_gpr467_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR467 ();
}

/* Set a value for h-sys-gpr467.  */

void
or1k32bf_h_sys_gpr467_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR467 (newval);
}

/* Get the value of h-sys-gpr468.  */

USI
or1k32bf_h_sys_gpr468_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR468 ();
}

/* Set a value for h-sys-gpr468.  */

void
or1k32bf_h_sys_gpr468_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR468 (newval);
}

/* Get the value of h-sys-gpr469.  */

USI
or1k32bf_h_sys_gpr469_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR469 ();
}

/* Set a value for h-sys-gpr469.  */

void
or1k32bf_h_sys_gpr469_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR469 (newval);
}

/* Get the value of h-sys-gpr470.  */

USI
or1k32bf_h_sys_gpr470_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR470 ();
}

/* Set a value for h-sys-gpr470.  */

void
or1k32bf_h_sys_gpr470_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR470 (newval);
}

/* Get the value of h-sys-gpr471.  */

USI
or1k32bf_h_sys_gpr471_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR471 ();
}

/* Set a value for h-sys-gpr471.  */

void
or1k32bf_h_sys_gpr471_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR471 (newval);
}

/* Get the value of h-sys-gpr472.  */

USI
or1k32bf_h_sys_gpr472_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR472 ();
}

/* Set a value for h-sys-gpr472.  */

void
or1k32bf_h_sys_gpr472_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR472 (newval);
}

/* Get the value of h-sys-gpr473.  */

USI
or1k32bf_h_sys_gpr473_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR473 ();
}

/* Set a value for h-sys-gpr473.  */

void
or1k32bf_h_sys_gpr473_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR473 (newval);
}

/* Get the value of h-sys-gpr474.  */

USI
or1k32bf_h_sys_gpr474_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR474 ();
}

/* Set a value for h-sys-gpr474.  */

void
or1k32bf_h_sys_gpr474_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR474 (newval);
}

/* Get the value of h-sys-gpr475.  */

USI
or1k32bf_h_sys_gpr475_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR475 ();
}

/* Set a value for h-sys-gpr475.  */

void
or1k32bf_h_sys_gpr475_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR475 (newval);
}

/* Get the value of h-sys-gpr476.  */

USI
or1k32bf_h_sys_gpr476_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR476 ();
}

/* Set a value for h-sys-gpr476.  */

void
or1k32bf_h_sys_gpr476_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR476 (newval);
}

/* Get the value of h-sys-gpr477.  */

USI
or1k32bf_h_sys_gpr477_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR477 ();
}

/* Set a value for h-sys-gpr477.  */

void
or1k32bf_h_sys_gpr477_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR477 (newval);
}

/* Get the value of h-sys-gpr478.  */

USI
or1k32bf_h_sys_gpr478_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR478 ();
}

/* Set a value for h-sys-gpr478.  */

void
or1k32bf_h_sys_gpr478_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR478 (newval);
}

/* Get the value of h-sys-gpr479.  */

USI
or1k32bf_h_sys_gpr479_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR479 ();
}

/* Set a value for h-sys-gpr479.  */

void
or1k32bf_h_sys_gpr479_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR479 (newval);
}

/* Get the value of h-sys-gpr480.  */

USI
or1k32bf_h_sys_gpr480_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR480 ();
}

/* Set a value for h-sys-gpr480.  */

void
or1k32bf_h_sys_gpr480_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR480 (newval);
}

/* Get the value of h-sys-gpr481.  */

USI
or1k32bf_h_sys_gpr481_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR481 ();
}

/* Set a value for h-sys-gpr481.  */

void
or1k32bf_h_sys_gpr481_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR481 (newval);
}

/* Get the value of h-sys-gpr482.  */

USI
or1k32bf_h_sys_gpr482_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR482 ();
}

/* Set a value for h-sys-gpr482.  */

void
or1k32bf_h_sys_gpr482_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR482 (newval);
}

/* Get the value of h-sys-gpr483.  */

USI
or1k32bf_h_sys_gpr483_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR483 ();
}

/* Set a value for h-sys-gpr483.  */

void
or1k32bf_h_sys_gpr483_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR483 (newval);
}

/* Get the value of h-sys-gpr484.  */

USI
or1k32bf_h_sys_gpr484_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR484 ();
}

/* Set a value for h-sys-gpr484.  */

void
or1k32bf_h_sys_gpr484_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR484 (newval);
}

/* Get the value of h-sys-gpr485.  */

USI
or1k32bf_h_sys_gpr485_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR485 ();
}

/* Set a value for h-sys-gpr485.  */

void
or1k32bf_h_sys_gpr485_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR485 (newval);
}

/* Get the value of h-sys-gpr486.  */

USI
or1k32bf_h_sys_gpr486_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR486 ();
}

/* Set a value for h-sys-gpr486.  */

void
or1k32bf_h_sys_gpr486_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR486 (newval);
}

/* Get the value of h-sys-gpr487.  */

USI
or1k32bf_h_sys_gpr487_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR487 ();
}

/* Set a value for h-sys-gpr487.  */

void
or1k32bf_h_sys_gpr487_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR487 (newval);
}

/* Get the value of h-sys-gpr488.  */

USI
or1k32bf_h_sys_gpr488_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR488 ();
}

/* Set a value for h-sys-gpr488.  */

void
or1k32bf_h_sys_gpr488_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR488 (newval);
}

/* Get the value of h-sys-gpr489.  */

USI
or1k32bf_h_sys_gpr489_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR489 ();
}

/* Set a value for h-sys-gpr489.  */

void
or1k32bf_h_sys_gpr489_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR489 (newval);
}

/* Get the value of h-sys-gpr490.  */

USI
or1k32bf_h_sys_gpr490_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR490 ();
}

/* Set a value for h-sys-gpr490.  */

void
or1k32bf_h_sys_gpr490_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR490 (newval);
}

/* Get the value of h-sys-gpr491.  */

USI
or1k32bf_h_sys_gpr491_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR491 ();
}

/* Set a value for h-sys-gpr491.  */

void
or1k32bf_h_sys_gpr491_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR491 (newval);
}

/* Get the value of h-sys-gpr492.  */

USI
or1k32bf_h_sys_gpr492_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR492 ();
}

/* Set a value for h-sys-gpr492.  */

void
or1k32bf_h_sys_gpr492_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR492 (newval);
}

/* Get the value of h-sys-gpr493.  */

USI
or1k32bf_h_sys_gpr493_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR493 ();
}

/* Set a value for h-sys-gpr493.  */

void
or1k32bf_h_sys_gpr493_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR493 (newval);
}

/* Get the value of h-sys-gpr494.  */

USI
or1k32bf_h_sys_gpr494_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR494 ();
}

/* Set a value for h-sys-gpr494.  */

void
or1k32bf_h_sys_gpr494_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR494 (newval);
}

/* Get the value of h-sys-gpr495.  */

USI
or1k32bf_h_sys_gpr495_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR495 ();
}

/* Set a value for h-sys-gpr495.  */

void
or1k32bf_h_sys_gpr495_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR495 (newval);
}

/* Get the value of h-sys-gpr496.  */

USI
or1k32bf_h_sys_gpr496_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR496 ();
}

/* Set a value for h-sys-gpr496.  */

void
or1k32bf_h_sys_gpr496_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR496 (newval);
}

/* Get the value of h-sys-gpr497.  */

USI
or1k32bf_h_sys_gpr497_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR497 ();
}

/* Set a value for h-sys-gpr497.  */

void
or1k32bf_h_sys_gpr497_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR497 (newval);
}

/* Get the value of h-sys-gpr498.  */

USI
or1k32bf_h_sys_gpr498_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR498 ();
}

/* Set a value for h-sys-gpr498.  */

void
or1k32bf_h_sys_gpr498_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR498 (newval);
}

/* Get the value of h-sys-gpr499.  */

USI
or1k32bf_h_sys_gpr499_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR499 ();
}

/* Set a value for h-sys-gpr499.  */

void
or1k32bf_h_sys_gpr499_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR499 (newval);
}

/* Get the value of h-sys-gpr500.  */

USI
or1k32bf_h_sys_gpr500_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR500 ();
}

/* Set a value for h-sys-gpr500.  */

void
or1k32bf_h_sys_gpr500_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR500 (newval);
}

/* Get the value of h-sys-gpr501.  */

USI
or1k32bf_h_sys_gpr501_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR501 ();
}

/* Set a value for h-sys-gpr501.  */

void
or1k32bf_h_sys_gpr501_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR501 (newval);
}

/* Get the value of h-sys-gpr502.  */

USI
or1k32bf_h_sys_gpr502_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR502 ();
}

/* Set a value for h-sys-gpr502.  */

void
or1k32bf_h_sys_gpr502_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR502 (newval);
}

/* Get the value of h-sys-gpr503.  */

USI
or1k32bf_h_sys_gpr503_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR503 ();
}

/* Set a value for h-sys-gpr503.  */

void
or1k32bf_h_sys_gpr503_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR503 (newval);
}

/* Get the value of h-sys-gpr504.  */

USI
or1k32bf_h_sys_gpr504_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR504 ();
}

/* Set a value for h-sys-gpr504.  */

void
or1k32bf_h_sys_gpr504_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR504 (newval);
}

/* Get the value of h-sys-gpr505.  */

USI
or1k32bf_h_sys_gpr505_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR505 ();
}

/* Set a value for h-sys-gpr505.  */

void
or1k32bf_h_sys_gpr505_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR505 (newval);
}

/* Get the value of h-sys-gpr506.  */

USI
or1k32bf_h_sys_gpr506_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR506 ();
}

/* Set a value for h-sys-gpr506.  */

void
or1k32bf_h_sys_gpr506_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR506 (newval);
}

/* Get the value of h-sys-gpr507.  */

USI
or1k32bf_h_sys_gpr507_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR507 ();
}

/* Set a value for h-sys-gpr507.  */

void
or1k32bf_h_sys_gpr507_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR507 (newval);
}

/* Get the value of h-sys-gpr508.  */

USI
or1k32bf_h_sys_gpr508_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR508 ();
}

/* Set a value for h-sys-gpr508.  */

void
or1k32bf_h_sys_gpr508_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR508 (newval);
}

/* Get the value of h-sys-gpr509.  */

USI
or1k32bf_h_sys_gpr509_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR509 ();
}

/* Set a value for h-sys-gpr509.  */

void
or1k32bf_h_sys_gpr509_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR509 (newval);
}

/* Get the value of h-sys-gpr510.  */

USI
or1k32bf_h_sys_gpr510_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR510 ();
}

/* Set a value for h-sys-gpr510.  */

void
or1k32bf_h_sys_gpr510_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR510 (newval);
}

/* Get the value of h-sys-gpr511.  */

USI
or1k32bf_h_sys_gpr511_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_GPR511 ();
}

/* Set a value for h-sys-gpr511.  */

void
or1k32bf_h_sys_gpr511_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_GPR511 (newval);
}

/* Get the value of h-mac-maclo.  */

USI
or1k32bf_h_mac_maclo_get (SIM_CPU *current_cpu)
{
  return GET_H_MAC_MACLO ();
}

/* Set a value for h-mac-maclo.  */

void
or1k32bf_h_mac_maclo_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_MAC_MACLO (newval);
}

/* Get the value of h-mac-machi.  */

USI
or1k32bf_h_mac_machi_get (SIM_CPU *current_cpu)
{
  return GET_H_MAC_MACHI ();
}

/* Set a value for h-mac-machi.  */

void
or1k32bf_h_mac_machi_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_MAC_MACHI (newval);
}

/* Get the value of h-tick-ttmr.  */

USI
or1k32bf_h_tick_ttmr_get (SIM_CPU *current_cpu)
{
  return GET_H_TICK_TTMR ();
}

/* Set a value for h-tick-ttmr.  */

void
or1k32bf_h_tick_ttmr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_TICK_TTMR (newval);
}

/* Get the value of h-sys-vr-rev.  */

USI
or1k32bf_h_sys_vr_rev_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_VR_REV ();
}

/* Set a value for h-sys-vr-rev.  */

void
or1k32bf_h_sys_vr_rev_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_VR_REV (newval);
}

/* Get the value of h-sys-vr-cfg.  */

USI
or1k32bf_h_sys_vr_cfg_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_VR_CFG ();
}

/* Set a value for h-sys-vr-cfg.  */

void
or1k32bf_h_sys_vr_cfg_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_VR_CFG (newval);
}

/* Get the value of h-sys-vr-ver.  */

USI
or1k32bf_h_sys_vr_ver_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_VR_VER ();
}

/* Set a value for h-sys-vr-ver.  */

void
or1k32bf_h_sys_vr_ver_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_VR_VER (newval);
}

/* Get the value of h-sys-upr-up.  */

USI
or1k32bf_h_sys_upr_up_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_UP ();
}

/* Set a value for h-sys-upr-up.  */

void
or1k32bf_h_sys_upr_up_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_UP (newval);
}

/* Get the value of h-sys-upr-dcp.  */

USI
or1k32bf_h_sys_upr_dcp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_DCP ();
}

/* Set a value for h-sys-upr-dcp.  */

void
or1k32bf_h_sys_upr_dcp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_DCP (newval);
}

/* Get the value of h-sys-upr-icp.  */

USI
or1k32bf_h_sys_upr_icp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_ICP ();
}

/* Set a value for h-sys-upr-icp.  */

void
or1k32bf_h_sys_upr_icp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_ICP (newval);
}

/* Get the value of h-sys-upr-dmp.  */

USI
or1k32bf_h_sys_upr_dmp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_DMP ();
}

/* Set a value for h-sys-upr-dmp.  */

void
or1k32bf_h_sys_upr_dmp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_DMP (newval);
}

/* Get the value of h-sys-upr-mp.  */

USI
or1k32bf_h_sys_upr_mp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_MP ();
}

/* Set a value for h-sys-upr-mp.  */

void
or1k32bf_h_sys_upr_mp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_MP (newval);
}

/* Get the value of h-sys-upr-imp.  */

USI
or1k32bf_h_sys_upr_imp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_IMP ();
}

/* Set a value for h-sys-upr-imp.  */

void
or1k32bf_h_sys_upr_imp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_IMP (newval);
}

/* Get the value of h-sys-upr-dup.  */

USI
or1k32bf_h_sys_upr_dup_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_DUP ();
}

/* Set a value for h-sys-upr-dup.  */

void
or1k32bf_h_sys_upr_dup_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_DUP (newval);
}

/* Get the value of h-sys-upr-pcup.  */

USI
or1k32bf_h_sys_upr_pcup_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_PCUP ();
}

/* Set a value for h-sys-upr-pcup.  */

void
or1k32bf_h_sys_upr_pcup_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_PCUP (newval);
}

/* Get the value of h-sys-upr-picp.  */

USI
or1k32bf_h_sys_upr_picp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_PICP ();
}

/* Set a value for h-sys-upr-picp.  */

void
or1k32bf_h_sys_upr_picp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_PICP (newval);
}

/* Get the value of h-sys-upr-pmp.  */

USI
or1k32bf_h_sys_upr_pmp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_PMP ();
}

/* Set a value for h-sys-upr-pmp.  */

void
or1k32bf_h_sys_upr_pmp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_PMP (newval);
}

/* Get the value of h-sys-upr-ttp.  */

USI
or1k32bf_h_sys_upr_ttp_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_TTP ();
}

/* Set a value for h-sys-upr-ttp.  */

void
or1k32bf_h_sys_upr_ttp_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_TTP (newval);
}

/* Get the value of h-sys-upr-cup.  */

USI
or1k32bf_h_sys_upr_cup_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_UPR_CUP ();
}

/* Set a value for h-sys-upr-cup.  */

void
or1k32bf_h_sys_upr_cup_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_UPR_CUP (newval);
}

/* Get the value of h-sys-cpucfgr-nsgr.  */

USI
or1k32bf_h_sys_cpucfgr_nsgr_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_NSGR ();
}

/* Set a value for h-sys-cpucfgr-nsgr.  */

void
or1k32bf_h_sys_cpucfgr_nsgr_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_NSGR (newval);
}

/* Get the value of h-sys-cpucfgr-cgf.  */

USI
or1k32bf_h_sys_cpucfgr_cgf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_CGF ();
}

/* Set a value for h-sys-cpucfgr-cgf.  */

void
or1k32bf_h_sys_cpucfgr_cgf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_CGF (newval);
}

/* Get the value of h-sys-cpucfgr-ob32s.  */

USI
or1k32bf_h_sys_cpucfgr_ob32s_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_OB32S ();
}

/* Set a value for h-sys-cpucfgr-ob32s.  */

void
or1k32bf_h_sys_cpucfgr_ob32s_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_OB32S (newval);
}

/* Get the value of h-sys-cpucfgr-ob64s.  */

USI
or1k32bf_h_sys_cpucfgr_ob64s_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_OB64S ();
}

/* Set a value for h-sys-cpucfgr-ob64s.  */

void
or1k32bf_h_sys_cpucfgr_ob64s_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_OB64S (newval);
}

/* Get the value of h-sys-cpucfgr-of32s.  */

USI
or1k32bf_h_sys_cpucfgr_of32s_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_OF32S ();
}

/* Set a value for h-sys-cpucfgr-of32s.  */

void
or1k32bf_h_sys_cpucfgr_of32s_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_OF32S (newval);
}

/* Get the value of h-sys-cpucfgr-of64s.  */

USI
or1k32bf_h_sys_cpucfgr_of64s_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_OF64S ();
}

/* Set a value for h-sys-cpucfgr-of64s.  */

void
or1k32bf_h_sys_cpucfgr_of64s_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_OF64S (newval);
}

/* Get the value of h-sys-cpucfgr-ov64s.  */

USI
or1k32bf_h_sys_cpucfgr_ov64s_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_OV64S ();
}

/* Set a value for h-sys-cpucfgr-ov64s.  */

void
or1k32bf_h_sys_cpucfgr_ov64s_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_OV64S (newval);
}

/* Get the value of h-sys-cpucfgr-nd.  */

USI
or1k32bf_h_sys_cpucfgr_nd_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_CPUCFGR_ND ();
}

/* Set a value for h-sys-cpucfgr-nd.  */

void
or1k32bf_h_sys_cpucfgr_nd_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_CPUCFGR_ND (newval);
}

/* Get the value of h-sys-sr-sm.  */

USI
or1k32bf_h_sys_sr_sm_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_SM ();
}

/* Set a value for h-sys-sr-sm.  */

void
or1k32bf_h_sys_sr_sm_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_SM (newval);
}

/* Get the value of h-sys-sr-tee.  */

USI
or1k32bf_h_sys_sr_tee_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_TEE ();
}

/* Set a value for h-sys-sr-tee.  */

void
or1k32bf_h_sys_sr_tee_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_TEE (newval);
}

/* Get the value of h-sys-sr-iee.  */

USI
or1k32bf_h_sys_sr_iee_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_IEE ();
}

/* Set a value for h-sys-sr-iee.  */

void
or1k32bf_h_sys_sr_iee_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_IEE (newval);
}

/* Get the value of h-sys-sr-dce.  */

USI
or1k32bf_h_sys_sr_dce_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_DCE ();
}

/* Set a value for h-sys-sr-dce.  */

void
or1k32bf_h_sys_sr_dce_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_DCE (newval);
}

/* Get the value of h-sys-sr-ice.  */

USI
or1k32bf_h_sys_sr_ice_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_ICE ();
}

/* Set a value for h-sys-sr-ice.  */

void
or1k32bf_h_sys_sr_ice_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_ICE (newval);
}

/* Get the value of h-sys-sr-dme.  */

USI
or1k32bf_h_sys_sr_dme_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_DME ();
}

/* Set a value for h-sys-sr-dme.  */

void
or1k32bf_h_sys_sr_dme_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_DME (newval);
}

/* Get the value of h-sys-sr-ime.  */

USI
or1k32bf_h_sys_sr_ime_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_IME ();
}

/* Set a value for h-sys-sr-ime.  */

void
or1k32bf_h_sys_sr_ime_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_IME (newval);
}

/* Get the value of h-sys-sr-lee.  */

USI
or1k32bf_h_sys_sr_lee_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_LEE ();
}

/* Set a value for h-sys-sr-lee.  */

void
or1k32bf_h_sys_sr_lee_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_LEE (newval);
}

/* Get the value of h-sys-sr-ce.  */

USI
or1k32bf_h_sys_sr_ce_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_CE ();
}

/* Set a value for h-sys-sr-ce.  */

void
or1k32bf_h_sys_sr_ce_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_CE (newval);
}

/* Get the value of h-sys-sr-f.  */

USI
or1k32bf_h_sys_sr_f_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_F ();
}

/* Set a value for h-sys-sr-f.  */

void
or1k32bf_h_sys_sr_f_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_F (newval);
}

/* Get the value of h-sys-sr-cy.  */

USI
or1k32bf_h_sys_sr_cy_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_CY ();
}

/* Set a value for h-sys-sr-cy.  */

void
or1k32bf_h_sys_sr_cy_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_CY (newval);
}

/* Get the value of h-sys-sr-ov.  */

USI
or1k32bf_h_sys_sr_ov_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_OV ();
}

/* Set a value for h-sys-sr-ov.  */

void
or1k32bf_h_sys_sr_ov_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_OV (newval);
}

/* Get the value of h-sys-sr-ove.  */

USI
or1k32bf_h_sys_sr_ove_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_OVE ();
}

/* Set a value for h-sys-sr-ove.  */

void
or1k32bf_h_sys_sr_ove_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_OVE (newval);
}

/* Get the value of h-sys-sr-dsx.  */

USI
or1k32bf_h_sys_sr_dsx_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_DSX ();
}

/* Set a value for h-sys-sr-dsx.  */

void
or1k32bf_h_sys_sr_dsx_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_DSX (newval);
}

/* Get the value of h-sys-sr-eph.  */

USI
or1k32bf_h_sys_sr_eph_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_EPH ();
}

/* Set a value for h-sys-sr-eph.  */

void
or1k32bf_h_sys_sr_eph_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_EPH (newval);
}

/* Get the value of h-sys-sr-fo.  */

USI
or1k32bf_h_sys_sr_fo_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_FO ();
}

/* Set a value for h-sys-sr-fo.  */

void
or1k32bf_h_sys_sr_fo_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_FO (newval);
}

/* Get the value of h-sys-sr-sumra.  */

USI
or1k32bf_h_sys_sr_sumra_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_SUMRA ();
}

/* Set a value for h-sys-sr-sumra.  */

void
or1k32bf_h_sys_sr_sumra_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_SUMRA (newval);
}

/* Get the value of h-sys-sr-cid.  */

USI
or1k32bf_h_sys_sr_cid_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_SR_CID ();
}

/* Set a value for h-sys-sr-cid.  */

void
or1k32bf_h_sys_sr_cid_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_SR_CID (newval);
}

/* Get the value of h-sys-fpcsr-fpee.  */

USI
or1k32bf_h_sys_fpcsr_fpee_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_FPEE ();
}

/* Set a value for h-sys-fpcsr-fpee.  */

void
or1k32bf_h_sys_fpcsr_fpee_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_FPEE (newval);
}

/* Get the value of h-sys-fpcsr-rm.  */

USI
or1k32bf_h_sys_fpcsr_rm_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_RM ();
}

/* Set a value for h-sys-fpcsr-rm.  */

void
or1k32bf_h_sys_fpcsr_rm_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_RM (newval);
}

/* Get the value of h-sys-fpcsr-ovf.  */

USI
or1k32bf_h_sys_fpcsr_ovf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_OVF ();
}

/* Set a value for h-sys-fpcsr-ovf.  */

void
or1k32bf_h_sys_fpcsr_ovf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_OVF (newval);
}

/* Get the value of h-sys-fpcsr-unf.  */

USI
or1k32bf_h_sys_fpcsr_unf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_UNF ();
}

/* Set a value for h-sys-fpcsr-unf.  */

void
or1k32bf_h_sys_fpcsr_unf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_UNF (newval);
}

/* Get the value of h-sys-fpcsr-snf.  */

USI
or1k32bf_h_sys_fpcsr_snf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_SNF ();
}

/* Set a value for h-sys-fpcsr-snf.  */

void
or1k32bf_h_sys_fpcsr_snf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_SNF (newval);
}

/* Get the value of h-sys-fpcsr-qnf.  */

USI
or1k32bf_h_sys_fpcsr_qnf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_QNF ();
}

/* Set a value for h-sys-fpcsr-qnf.  */

void
or1k32bf_h_sys_fpcsr_qnf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_QNF (newval);
}

/* Get the value of h-sys-fpcsr-zf.  */

USI
or1k32bf_h_sys_fpcsr_zf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_ZF ();
}

/* Set a value for h-sys-fpcsr-zf.  */

void
or1k32bf_h_sys_fpcsr_zf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_ZF (newval);
}

/* Get the value of h-sys-fpcsr-ixf.  */

USI
or1k32bf_h_sys_fpcsr_ixf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_IXF ();
}

/* Set a value for h-sys-fpcsr-ixf.  */

void
or1k32bf_h_sys_fpcsr_ixf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_IXF (newval);
}

/* Get the value of h-sys-fpcsr-ivf.  */

USI
or1k32bf_h_sys_fpcsr_ivf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_IVF ();
}

/* Set a value for h-sys-fpcsr-ivf.  */

void
or1k32bf_h_sys_fpcsr_ivf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_IVF (newval);
}

/* Get the value of h-sys-fpcsr-inf.  */

USI
or1k32bf_h_sys_fpcsr_inf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_INF ();
}

/* Set a value for h-sys-fpcsr-inf.  */

void
or1k32bf_h_sys_fpcsr_inf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_INF (newval);
}

/* Get the value of h-sys-fpcsr-dzf.  */

USI
or1k32bf_h_sys_fpcsr_dzf_get (SIM_CPU *current_cpu)
{
  return GET_H_SYS_FPCSR_DZF ();
}

/* Set a value for h-sys-fpcsr-dzf.  */

void
or1k32bf_h_sys_fpcsr_dzf_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_SYS_FPCSR_DZF (newval);
}

/* Get the value of h-atomic-reserve.  */

BI
or1k32bf_h_atomic_reserve_get (SIM_CPU *current_cpu)
{
  return CPU (h_atomic_reserve);
}

/* Set a value for h-atomic-reserve.  */

void
or1k32bf_h_atomic_reserve_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_atomic_reserve) = newval;
}

/* Get the value of h-atomic-address.  */

SI
or1k32bf_h_atomic_address_get (SIM_CPU *current_cpu)
{
  return CPU (h_atomic_address);
}

/* Set a value for h-atomic-address.  */

void
or1k32bf_h_atomic_address_set (SIM_CPU *current_cpu, SI newval)
{
  CPU (h_atomic_address) = newval;
}

/* Get the value of h-roff1.  */

BI
or1k32bf_h_roff1_get (SIM_CPU *current_cpu)
{
  return CPU (h_roff1);
}

/* Set a value for h-roff1.  */

void
or1k32bf_h_roff1_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_roff1) = newval;
}

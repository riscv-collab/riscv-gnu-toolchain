/* OpenRISC simulator support code header
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

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

#ifndef OR1K_SIM_H
#define OR1K_SIM_H

#include "symcat.h"

/* GDB register numbers.  */
#define PPC_REGNUM	32
#define PC_REGNUM	33
#define SR_REGNUM	34

/* Misc. profile data.  */
typedef struct
{
} OR1K_MISC_PROFILE;

/* Nop codes used in nop simulation.  */
#define NOP_NOP         0x0
#define NOP_EXIT        0x1
#define NOP_REPORT      0x2
#define NOP_PUTC        0x4
#define NOP_CNT_RESET   0x5
#define NOP_GET_TICKS   0x6
#define NOP_GET_PS      0x7
#define NOP_TRACE_ON    0x8
#define NOP_TRACE_OFF   0x9
#define NOP_RANDOM      0xa
#define NOP_OR1KSIM     0xb
#define NOP_EXIT_SILENT 0xc

#define NUM_SPR 0x20000
#define SPR_GROUP_SHIFT 11
#define SPR_GROUP_FIRST(group) (((UWI) SPR_GROUP_##group) << SPR_GROUP_SHIFT)
#define SPR_ADDR(group,index) \
  (SPR_GROUP_FIRST(group) | ((UWI) SPR_INDEX_##group##_##index))

/* Define word getters and setter helpers based on those from
   sim/common/cgen-mem.h.  */
#define GETTWI GETTSI
#define SETTWI SETTSI

void or1k_cpu_init (SIM_DESC sd, sim_cpu *current_cpu, const USI or1k_vr,
		    const USI or1k_upr, const USI or1k_cpucfgr);

void or1k32bf_insn_before (sim_cpu* current_cpu, SEM_PC vpc, const IDESC *idesc);
void or1k32bf_insn_after (sim_cpu* current_cpu, SEM_PC vpc, const IDESC *idesc);
void or1k32bf_fpu_error (CGEN_FPU* fpu, int status);
void or1k32bf_exception (sim_cpu *current_cpu, USI pc, USI exnum);
void or1k32bf_rfe (sim_cpu *current_cpu);
void or1k32bf_nop (sim_cpu *current_cpu, USI uimm16);
USI or1k32bf_mfspr (sim_cpu *current_cpu, USI addr);
void or1k32bf_mtspr (sim_cpu *current_cpu, USI addr, USI val);

int or1k32bf_fetch_register (sim_cpu *current_cpu, int rn, void *buf, int len);
int or1k32bf_store_register (sim_cpu *current_cpu, int rn, const void *buf,
			     int len);
int or1k32bf_model_or1200_u_exec (sim_cpu *current_cpu, const IDESC *idesc,
				  int unit_num, int referenced);
int or1k32bf_model_or1200nd_u_exec (sim_cpu *current_cpu, const IDESC *idesc,
				    int unit_num, int referenced);
void or1k32bf_model_insn_before (sim_cpu *current_cpu, int first_p);
void or1k32bf_model_insn_after (sim_cpu *current_cpu, int last_p, int cycles);
USI or1k32bf_h_spr_get_raw (sim_cpu *current_cpu, USI addr);
void or1k32bf_h_spr_set_raw (sim_cpu *current_cpu, USI addr, USI val);
USI or1k32bf_h_spr_field_get_raw (sim_cpu *current_cpu, USI addr, int msb,
				  int lsb);
void or1k32bf_h_spr_field_set_raw (sim_cpu *current_cpu, USI addr, int msb,
				   int lsb, USI val);
USI or1k32bf_make_load_store_addr (sim_cpu *current_cpu, USI base, SI offset,
				   int size);

USI or1k32bf_ff1 (sim_cpu *current_cpu, USI val);
USI or1k32bf_fl1 (sim_cpu *current_cpu, USI val);

#define OR1K_DEFAULT_MEM_SIZE 0x800000	/* 8M */

struct or1k_sim_cpu
{
  OR1K_MISC_PROFILE or1k_misc_profile;
#define CPU_OR1K_MISC_PROFILE(cpu) (& OR1K_SIM_CPU (cpu)->or1k_misc_profile)

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
  UWI spr[NUM_SPR];

  /* Next instruction will be in delay slot.  */
  BI next_delay_slot;
  /* Currently in delay slot.  */
  BI delay_slot;

#ifdef WANT_CPU_OR1K32BF
  OR1K32BF_CPU_DATA cpu_data;
#endif
};
#define OR1K_SIM_CPU(cpu) ((struct or1k_sim_cpu *) CPU_ARCH_DATA (cpu))

#endif /* OR1K_SIM_H */

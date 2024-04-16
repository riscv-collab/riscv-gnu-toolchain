/* Lattice Mico32 simulator support code
   Contributed by Jon Beniston <jon@beniston.com>

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

/* Main header for the LM32 simulator.  */

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#define WITH_SCACHE_PBB 1

#include "sim-basics.h"
#include "opcodes/lm32-desc.h"
#include "opcodes/lm32-opc.h"
#include "arch.h"
#include "sim-base.h"
#include "cgen-sim.h"
#include "lm32-sim.h"

struct lm32_sim_cpu
{
  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
#if defined (WANT_CPU_LM32BF)
  LM32BF_CPU_DATA cpu_data;
#endif
};
#define LM32_SIM_CPU(cpu) ((struct lm32_sim_cpu *) CPU_ARCH_DATA (cpu))

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN lm32_core_signal ATTRIBUTE_NORETURN;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
lm32_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

/* From traps.c.  */
extern USI lm32bf_b_insn (SIM_CPU * current_cpu, USI r0, USI f_r0);
extern USI lm32bf_divu_insn (SIM_CPU * current_cpu, IADDR pc, USI r0, USI r1, USI r2);
extern USI lm32bf_modu_insn (SIM_CPU * current_cpu, IADDR pc, USI r0, USI r1, USI r2);
extern void lm32bf_wcsr_insn (SIM_CPU * current_cpu, USI f_csr, USI r1);
extern USI lm32bf_break_insn (SIM_CPU * current_cpu, IADDR pc);
extern USI lm32bf_scall_insn (SIM_CPU * current_cpu, IADDR pc);

/* From user.c.  */
extern UINT lm32bf_user_insn (SIM_CPU * current_cpu, INT r0, INT r1, UINT imm);

#endif /* SIM_MAIN_H */

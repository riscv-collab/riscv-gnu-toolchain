/* RISC-V simulator.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of the GNU simulators.

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

#ifndef RISCV_MAIN_H
#define RISCV_MAIN_H

struct riscv_sim_cpu {
  union {
    unsigned_word regs[32];
    struct {
      /* These are the ABI names.  */
      unsigned_word zero, ra, sp, gp, tp;
      unsigned_word t0, t1, t2;
      unsigned_word s0, s1;
      unsigned_word a0, a1, a2, a3, a4, a5, a6, a7;
      unsigned_word s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
      unsigned_word t3, t4, t5, t6;
    };
  };
  union {
    unsigned_word fpregs[32];
    struct {
      /* These are the ABI names.  */
      unsigned_word ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7;
      unsigned_word fs0, fs1;
      unsigned_word fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7;
      unsigned_word fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11;
      unsigned_word ft8, ft9, ft10, ft11;
    };
  };
  sim_cia pc;

  struct {
#define DECLARE_CSR(name, ...) unsigned_word name;
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
  } csr;
};
#define RISCV_SIM_CPU(cpu) ((struct riscv_sim_cpu *) CPU_ARCH_DATA (cpu))

struct atomic_mem_reserved_list;
struct atomic_mem_reserved_list {
  struct atomic_mem_reserved_list *next;
  address_word addr;
};

struct riscv_sim_state {
  struct atomic_mem_reserved_list *amo_reserved_list;
};
#define RISCV_SIM_STATE(sd) ((struct riscv_sim_state *) STATE_ARCH_DATA (sd))

extern void step_once (SIM_CPU *);
extern void initialize_cpu (SIM_DESC, SIM_CPU *, int);
extern void initialize_env (SIM_DESC, const char * const *argv,
			    const char * const *env);

#define DEFAULT_MEM_SIZE (64 * 1024 * 1024)

#define RISCV_XLEN(cpu) MACH_WORD_BITSIZE (CPU_MACH (cpu))

#endif

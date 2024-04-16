/* collection of junk waiting time to sort out
   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

#ifndef M32R_SIM_H
#define M32R_SIM_H

#include "symcat.h"

/* GDB register numbers.  */
#define PSW_REGNUM	16
#define CBR_REGNUM	17
#define SPI_REGNUM	18
#define SPU_REGNUM	19
#define BPC_REGNUM	20
#define PC_REGNUM	21
#define ACCL_REGNUM	22
#define ACCH_REGNUM	23
#define ACC1L_REGNUM	24
#define ACC1H_REGNUM	25
#define BBPSW_REGNUM	26
#define BBPC_REGNUM	27
#define EVB_REGNUM	28

extern int m32r_decode_gdb_ctrl_regnum (int);

/* The other cpu cores reuse m32rbf funcs to avoid duplication, but they don't
   provide externs to access, and we can't e.g. include decode.h in decodex.h
   because of all the redefinitions of cgen macros.  */

extern void m32rbf_model_insn_before (SIM_CPU *, int);
extern void m32rbf_model_insn_after (SIM_CPU *, int, int);
extern CPUREG_FETCH_FN m32rbf_fetch_register;
extern CPUREG_STORE_FN m32rbf_store_register;
extern UQI  m32rbf_h_psw_get (SIM_CPU *);
extern void m32rbf_h_psw_set (SIM_CPU *, UQI);
extern UQI  m32r2f_h_psw_get (SIM_CPU *);
extern void m32r2f_h_psw_set (SIM_CPU *, UQI);
extern UQI  m32rxf_h_psw_get (SIM_CPU *);
extern void m32rxf_h_psw_set (SIM_CPU *, UQI);
extern void m32rbf_h_bpsw_set (SIM_CPU *, UQI);
extern void m32r2f_h_bpsw_set (SIM_CPU *, UQI);
extern void m32rxf_h_bpsw_set (SIM_CPU *, UQI);
extern SI   m32rbf_h_gr_get (SIM_CPU *, UINT);
extern void m32rbf_h_gr_set (SIM_CPU *, UINT, SI);
extern USI  m32rbf_h_cr_get (SIM_CPU *, UINT);
extern void m32rbf_h_cr_set (SIM_CPU *, UINT, USI);

/* Cover macros for hardware accesses.
   FIXME: Eventually move to cgen.  */
#define GET_H_SM() ((CPU (h_psw) & 0x80) != 0)

extern USI  m32rbf_h_cr_get_handler (SIM_CPU *, UINT);
extern void m32rbf_h_cr_set_handler (SIM_CPU *, UINT, USI);
extern USI  m32r2f_h_cr_get_handler (SIM_CPU *, UINT);
extern void m32r2f_h_cr_set_handler (SIM_CPU *, UINT, USI);
extern USI  m32rxf_h_cr_get_handler (SIM_CPU *, UINT);
extern void m32rxf_h_cr_set_handler (SIM_CPU *, UINT, USI);

#ifndef GET_H_CR
#define GET_H_CR(regno) \
  XCONCAT2 (WANT_CPU,_h_cr_get_handler) (current_cpu, (regno))
#define SET_H_CR(regno, val) \
  XCONCAT2 (WANT_CPU,_h_cr_set_handler) (current_cpu, (regno), (val))
#endif

extern UQI  m32rbf_h_psw_get_handler (SIM_CPU *);
extern void m32rbf_h_psw_set_handler (SIM_CPU *, UQI);
extern UQI  m32r2f_h_psw_get_handler (SIM_CPU *);
extern void m32r2f_h_psw_set_handler (SIM_CPU *, UQI);
extern UQI  m32rxf_h_psw_get_handler (SIM_CPU *);
extern void m32rxf_h_psw_set_handler (SIM_CPU *, UQI);

#ifndef  GET_H_PSW
#define GET_H_PSW() \
  XCONCAT2 (WANT_CPU,_h_psw_get_handler) (current_cpu)
#define SET_H_PSW(val) \
  XCONCAT2 (WANT_CPU,_h_psw_set_handler) (current_cpu, (val))
#endif

/* FIXME: These prototypes are necessary because the cgen generated
   cpu.h, cpux.h and cpu2.h headers do not provide them, and functions
   which take or return parameters that are larger than an int must be
   prototyed in order for them to work correctly.

   The correct solution is to fix the code in cgen/sim.scm to generate
   prototypes for each of the functions it generates.  */
extern DI   m32rbf_h_accum_get_handler (SIM_CPU *);
extern void m32rbf_h_accum_set_handler (SIM_CPU *, DI);
extern DI   m32r2f_h_accum_get_handler (SIM_CPU *);
extern void m32r2f_h_accum_set_handler (SIM_CPU *, DI);
extern DI   m32rxf_h_accum_get_handler (SIM_CPU *);
extern void m32rxf_h_accum_set_handler (SIM_CPU *, DI);

extern DI   m32r2f_h_accums_get_handler (SIM_CPU *, UINT);
extern void m32r2f_h_accums_set_handler (SIM_CPU *, UINT, DI);
extern DI   m32rxf_h_accums_get_handler (SIM_CPU *, UINT);
extern void m32rxf_h_accums_set_handler (SIM_CPU *, UINT, DI);

#ifndef  GET_H_ACCUM
#define GET_H_ACCUM() \
  XCONCAT2 (WANT_CPU,_h_accum_get_handler) (current_cpu)
#define SET_H_ACCUM(val) \
  XCONCAT2 (WANT_CPU,_h_accum_set_handler) (current_cpu, (val))
#endif

/* Misc. profile data.  */

typedef struct {
  /* nop insn slot filler count */
  unsigned int fillnop_count;
  /* number of parallel insns */
  unsigned int parallel_count;

  /* FIXME: generalize this to handle all insn lengths, move to common.  */
  /* number of short insns, not including parallel ones */
  unsigned int short_count;
  /* number of long insns */
  unsigned int long_count;

  /* Working area for computing cycle counts.  */
  unsigned long insn_cycles; /* FIXME: delete */
  unsigned long cti_stall;
  unsigned long load_stall;
  unsigned long biggest_cycles;

  /* Bitmask of registers loaded by previous insn.  */
  unsigned int load_regs;
  /* Bitmask of registers loaded by current insn.  */
  unsigned int load_regs_pending;
} M32R_MISC_PROFILE;

/* Initialize the working area.  */
void m32r_init_insn_cycles (SIM_CPU *, int);
/* Update the totals for the insn.  */
void m32r_record_insn_cycles (SIM_CPU *, int);

/* This is invoked by the nop pattern in the .cpu file.  */
#define PROFILE_COUNT_FILLNOPS(cpu, addr) \
do { \
  if (PROFILE_INSN_P (cpu) \
      && (addr & 3) != 0) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->fillnop_count; \
} while (0)

/* This is invoked by the execute section of mloop{,x}.in.  */
#define PROFILE_COUNT_PARINSNS(cpu) \
do { \
  if (PROFILE_INSN_P (cpu)) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->parallel_count; \
} while (0)

/* This is invoked by the execute section of mloop{,x}.in.  */
#define PROFILE_COUNT_SHORTINSNS(cpu) \
do { \
  if (PROFILE_INSN_P (cpu)) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->short_count; \
} while (0)

/* This is invoked by the execute section of mloop{,x}.in.  */
#define PROFILE_COUNT_LONGINSNS(cpu) \
do { \
  if (PROFILE_INSN_P (cpu)) \
    ++ CPU_M32R_MISC_PROFILE (cpu)->long_count; \
} while (0)

#define GETTWI GETTSI
#define SETTWI SETTSI

/* Additional execution support.  */


/* Hardware/device support.
   ??? Will eventually want to move device stuff to config files.  */

/* Exception, Interrupt, and Trap addresses */
#define EIT_SYSBREAK_ADDR	0x10
#define EIT_RSVD_INSN_ADDR	0x20
#define EIT_ADDR_EXCP_ADDR	0x30
#define EIT_TRAP_BASE_ADDR	0x40
#define EIT_EXTERN_ADDR		0x80
#define EIT_RESET_ADDR		0x7ffffff0
#define EIT_WAKEUP_ADDR		0x7ffffff0

/* Special purpose traps.  */
#define TRAP_SYSCALL	0
#define TRAP_BREAKPOINT	1

/* Handle the trap insn.  */
USI m32r_trap (SIM_CPU *, PCADDR, int);

struct m32r_sim_cpu {
  M32R_MISC_PROFILE m32r_misc_profile;
#define CPU_M32R_MISC_PROFILE(cpu) (& M32R_SIM_CPU (cpu)->m32r_misc_profile)

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
#if defined (WANT_CPU_M32RBF)
  M32RBF_CPU_DATA cpu_data;
#endif
#if defined (WANT_CPU_M32RXF)
  M32RXF_CPU_DATA cpu_data;
#elif defined (WANT_CPU_M32R2F)
  M32R2F_CPU_DATA cpu_data;
#endif
};
#define M32R_SIM_CPU(cpu) ((struct m32r_sim_cpu *) CPU_ARCH_DATA (cpu))

#endif /* M32R_SIM_H */

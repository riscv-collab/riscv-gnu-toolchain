/* Simulation code for the CR16 processor.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */


#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include "ansidecl.h"
#include "sim/callback.h"
#include "opcode/cr16.h"
#include "bfd.h"
#include "sim-main.h"

#define DEBUG_TRACE		0x00000001
#define DEBUG_VALUES		0x00000002
#define DEBUG_LINE_NUMBER	0x00000004
#define DEBUG_MEMSIZE		0x00000008
#define DEBUG_INSTRUCTION	0x00000010
#define DEBUG_TRAP		0x00000020
#define DEBUG_MEMORY		0x00000040

#ifndef	DEBUG
#define	DEBUG (DEBUG_TRACE | DEBUG_VALUES | DEBUG_LINE_NUMBER)
#endif

extern int cr16_debug;

#include "sim/sim.h"
#include "sim-config.h"
#include "sim-types.h"

/* FIXME: CR16 defines */
typedef uint16_t reg_t;
typedef uint32_t creg_t;

struct simops 
{
  char mnemonic[12];
  uint32_t size;
  uint32_t mask;
  uint32_t opcode;
  int format;
  char fname[12];
  void (*func)(SIM_DESC, SIM_CPU *);
  int numops;
  operand_desc operands[4];
};

enum _ins_type
{
   INS_UNKNOWN,			/* unknown instruction */
   INS_NO_TYPE_INS,
   INS_ARITH_INS,
   INS_LD_STOR_INS,
   INS_BRANCH_INS,
   INS_ARITH_BYTE_INS,
   INS_SHIFT_INS,
   INS_BRANCH_NEQ_INS,
   INS_STOR_IMM_INS, 
   INS_CSTBIT_INS, 
   INS_MAX
};

extern unsigned long ins_type_counters[ (int)INS_MAX ];

enum {
  SP_IDX = 15,
};

/* Write-back slots */
union slot_data {
  unsigned_1 _1;
  unsigned_2 _2;
  unsigned_4 _4;
};
struct slot {
  void *dest;
  int size;
  union slot_data data;
  union slot_data mask;
};
enum {
 NR_SLOTS = 16
};
#define SLOT (State.slot)
#define SLOT_NR (State.slot_nr)
#define SLOT_PEND_MASK(DEST, MSK, VAL) \
  do \
    { \
      SLOT[SLOT_NR].dest = &(DEST); \
      SLOT[SLOT_NR].size = sizeof (DEST); \
      switch (sizeof (DEST)) \
        { \
        case 1: \
          SLOT[SLOT_NR].data._1 = (unsigned_1) (VAL); \
          SLOT[SLOT_NR].mask._1 = (unsigned_1) (MSK); \
          break; \
        case 2: \
          SLOT[SLOT_NR].data._2 = (unsigned_2) (VAL); \
          SLOT[SLOT_NR].mask._2 = (unsigned_2) (MSK); \
          break; \
        case 4: \
          SLOT[SLOT_NR].data._4 = (unsigned_4) (VAL); \
          SLOT[SLOT_NR].mask._4 = (unsigned_4) (MSK); \
          break; \
        } \
      SLOT_NR = (SLOT_NR + 1); \
    } \
  while (0)
#define SLOT_PEND(DEST, VAL) SLOT_PEND_MASK(DEST, 0, VAL)
#define SLOT_DISCARD() (SLOT_NR = 0)
#define SLOT_FLUSH() \
  do \
    { \
      int i; \
      for (i = 0; i < SLOT_NR; i++) \
	{ \
	  switch (SLOT[i].size) \
	    { \
	    case 1: \
	      *(unsigned_1*) SLOT[i].dest &= SLOT[i].mask._1; \
	      *(unsigned_1*) SLOT[i].dest |= SLOT[i].data._1; \
	      break; \
	    case 2: \
	      *(unsigned_2*) SLOT[i].dest &= SLOT[i].mask._2; \
	      *(unsigned_2*) SLOT[i].dest |= SLOT[i].data._2; \
	      break; \
	    case 4: \
	      *(unsigned_4*) SLOT[i].dest &= SLOT[i].mask._4; \
	      *(unsigned_4*) SLOT[i].dest |= SLOT[i].data._4; \
	      break; \
	    } \
        } \
      SLOT_NR = 0; \
    } \
  while (0)
#define SLOT_DUMP() \
  do \
    { \
      int i; \
      for (i = 0; i < SLOT_NR; i++) \
	{ \
	  switch (SLOT[i].size) \
	    { \
	    case 1: \
              printf ("SLOT %d *0x%08lx & 0x%02x | 0x%02x\n", i, \
		      (long) SLOT[i].dest, \
                      (unsigned) SLOT[i].mask._1, \
                      (unsigned) SLOT[i].data._1); \
	      break; \
	    case 2: \
              printf ("SLOT %d *0x%08lx & 0x%04x | 0x%04x\n", i, \
		      (long) SLOT[i].dest, \
                      (unsigned) SLOT[i].mask._2, \
                      (unsigned) SLOT[i].data._2); \
	      break; \
	    case 4: \
              printf ("SLOT %d *0x%08lx & 0x%08x | 0x%08x\n", i, \
		      (long) SLOT[i].dest, \
                      (unsigned) SLOT[i].mask._4, \
                      (unsigned) SLOT[i].data._4); \
	      break; \
	    case 8: \
              printf ("SLOT %d *0x%08lx & 0x%08x%08x | 0x%08x%08x\n", i, \
		      (long) SLOT[i].dest, \
                      (unsigned) (SLOT[i].mask._8 >> 32),  \
                      (unsigned) SLOT[i].mask._8, \
                      (unsigned) (SLOT[i].data._8 >> 32),  \
                      (unsigned) SLOT[i].data._8); \
	      break; \
	    } \
        } \
    } \
  while (0)

struct _state
{
  creg_t regs[16];		/* general-purpose registers */
#define GPR(N) (State.regs[(N)] + 0)
#define SET_GPR(N,VAL) (State.regs[(N)] = (VAL))

#define GPR32(N) \
     (N < 12) ? \
     ((((uint16_t) State.regs[(N) + 1]) << 16) | (uint16_t) State.regs[(N)]) \
     : GPR (N) 

#define SET_GPR32(N,VAL) do { \
     if (N < 11)  \
       { SET_GPR (N + 1, (VAL) >> 16); SET_GPR (N, ((VAL) & 0xffff));} \
     else { if ( N == 11) \
             { SET_GPR (N + 1, ((GPR32 (12)) & 0xffff0000)|((VAL) >> 16)); \
	       SET_GPR (N, ((VAL) & 0xffff));} \
            else SET_GPR (N, (VAL));} \
    } while (0)

  creg_t cregs[16];		/* control registers */
#define CREG(N) (State.cregs[(N)] + 0)
#define SET_CREG(N,VAL) move_to_cr (sd, cpu, (N), 0, (VAL), 0)
#define SET_HW_CREG(N,VAL) move_to_cr (sd, cpu, (N), 0, (VAL), 1)

  reg_t sp[2];                  /* holding area for SPI(0)/SPU(1) */
#define HELD_SP(N) (State.sp[(N)] + 0)
#define SET_HELD_SP(N,VAL) SLOT_PEND (State.sp[(N)], (VAL))

  /* writeback info */
  struct slot slot[NR_SLOTS];
  int slot_nr;

  /* trace data */
  struct {
    uint16_t psw;
  } trace;

  int	pc_changed;

  /* NOTE: everything below this line is not reset by
     sim_create_inferior() */

  enum _ins_type ins_type;

};

extern struct _state State;


extern uint32_t OP[4];
extern uint32_t sign_flag;
extern struct simops Simops[];

enum
{
  PC_CR   = 0,
  BDS_CR  = 1,
  BSR_CR  = 2,
  DCR_CR  = 3,
  CAR0_CR = 5,
  CAR1_CR = 7,
  CFG_CR  = 9,
  PSR_CR  = 10,
  INTBASE_CR = 11,
  ISP_CR = 13,
  USP_CR = 15
};

enum
{
  PSR_I_BIT = 0x0800,
  PSR_P_BIT = 0x0400,
  PSR_E_BIT = 0x0200,
  PSR_N_BIT = 0x0080,
  PSR_Z_BIT = 0x0040,
  PSR_F_BIT = 0x0020,
  PSR_U_BIT = 0x0008,
  PSR_L_BIT = 0x0004,
  PSR_T_BIT = 0x0002,
  PSR_C_BIT = 0x0001
};

#define PSR CREG (PSR_CR)
#define SET_PSR(VAL) SET_CREG (PSR_CR, (VAL))
#define SET_HW_PSR(VAL) SET_HW_CREG (PSR_CR, (VAL))
#define SET_PSR_BIT(MASK,VAL) move_to_cr (sd, cpu, PSR_CR, ~((creg_t) MASK), (VAL) ? (MASK) : 0, 1)

#define PSR_SM ((PSR & PSR_SM_BIT) != 0)
#define SET_PSR_SM(VAL) SET_PSR_BIT (PSR_SM_BIT, (VAL))

#define PSR_I ((PSR & PSR_I_BIT) != 0)
#define SET_PSR_I(VAL) SET_PSR_BIT (PSR_I_BIT, (VAL))

#define PSR_DB ((PSR & PSR_DB_BIT) != 0)
#define SET_PSR_DB(VAL) SET_PSR_BIT (PSR_DB_BIT, (VAL))

#define PSR_P ((PSR & PSR_P_BIT) != 0)
#define SET_PSR_P(VAL) SET_PSR_BIT (PSR_P_BIT, (VAL))

#define PSR_E ((PSR & PSR_E_BIT) != 0)
#define SET_PSR_E(VAL) SET_PSR_BIT (PSR_E_BIT, (VAL))

#define PSR_N ((PSR & PSR_N_BIT) != 0)
#define SET_PSR_N(VAL) SET_PSR_BIT (PSR_N_BIT, (VAL))

#define PSR_Z ((PSR & PSR_Z_BIT) != 0)
#define SET_PSR_Z(VAL) SET_PSR_BIT (PSR_Z_BIT, (VAL))

#define PSR_F ((PSR & PSR_F_BIT) != 0)
#define SET_PSR_F(VAL) SET_PSR_BIT (PSR_F_BIT, (VAL))

#define PSR_U ((PSR & PSR_U_BIT) != 0)
#define SET_PSR_U(VAL) SET_PSR_BIT (PSR_U_BIT, (VAL))

#define PSR_L ((PSR & PSR_L_BIT) != 0)
#define SET_PSR_L(VAL) SET_PSR_BIT (PSR_L_BIT, (VAL))

#define PSR_T ((PSR & PSR_T_BIT) != 0)
#define SET_PSR_T(VAL) SET_PSR_BIT (PSR_T_BIT, (VAL))

#define PSR_C ((PSR & PSR_C_BIT) != 0)
#define SET_PSR_C(VAL) SET_PSR_BIT (PSR_C_BIT, (VAL))

/* See simopsc.:move_to_cr() for registers that can not be read-from
   or assigned-to directly */

#define PC	CREG (PC_CR)
#define SET_PC(VAL) SET_CREG (PC_CR, (VAL))
//#define SET_PC(VAL) (State.cregs[PC_CR] = (VAL))

#define BPSR	CREG (BPSR_CR)
#define SET_BPSR(VAL) SET_CREG (BPSR_CR, (VAL))

#define BPC	CREG (BPC_CR)
#define SET_BPC(VAL) SET_CREG (BPC_CR, (VAL))

#define DPSR	CREG (DPSR_CR)
#define SET_DPSR(VAL) SET_CREG (DPSR_CR, (VAL))

#define DPC	CREG (DPC_CR)
#define SET_DPC(VAL) SET_CREG (DPC_CR, (VAL))

#define RPT_C	CREG (RPT_C_CR)
#define SET_RPT_C(VAL) SET_CREG (RPT_C_CR, (VAL))

#define RPT_S	CREG (RPT_S_CR)
#define SET_RPT_S(VAL) SET_CREG (RPT_S_CR, (VAL))

#define RPT_E	CREG (RPT_E_CR)
#define SET_RPT_E(VAL) SET_CREG (RPT_E_CR, (VAL))

#define MOD_S	CREG (MOD_S_CR)
#define SET_MOD_S(VAL) SET_CREG (MOD_S_CR, (VAL))

#define MOD_E	CREG (MOD_E_CR)
#define SET_MOD_E(VAL) SET_CREG (MOD_E_CR, (VAL))

#define IBA	CREG (IBA_CR)
#define SET_IBA(VAL) SET_CREG (IBA_CR, (VAL))


#define SIG_CR16_STOP	-1
#define SIG_CR16_EXIT	-2
#define SIG_CR16_BUS    -3
#define SIG_CR16_IAD    -4

/* TODO: Resolve conflicts with common headers.  */
#undef SEXT8
#undef SEXT16
#undef SEXT32

#define SEXT3(x)	((((x)&0x7)^(~3))+4)	

/* sign-extend a 4-bit number */
#define SEXT4(x)	((((x)&0xf)^(~7))+8)	

/* sign-extend an 8-bit number */
#define SEXT8(x)	((((x)&0xff)^(~0x7f))+0x80)

/* sign-extend a 16-bit number */
#define SEXT16(x)	((((x)&0xffff)^(~0x7fff))+0x8000)

/* sign-extend a 24-bit number */
#define SEXT24(x)	((((x)&0xffffff)^(~0x7fffff))+0x800000)

/* sign-extend a 32-bit number */
#define SEXT32(x)	((((x)&0xffffffff)^(~0x7fffffff))+0x80000000)

#define SB(addr, data)		sim_core_write_1 (cpu, PC, read_map, addr, data)
#define RB(addr)		sim_core_read_1 (cpu, PC, read_map, addr)
#define SW(addr, data)		sim_core_write_unaligned_2 (cpu, PC, read_map, addr, data)
#define RW(addr)		sim_core_read_unaligned_2 (cpu, PC, read_map, addr)
#define SLW(addr, data)		sim_core_write_unaligned_4 (cpu, PC, read_map, addr, data)

/* Yes, this is as whacked as it looks.  The sim currently reads little endian
   for 16 bits, but then merge them like big endian to get 32 bits.  */
static inline uint32_t get_longword (SIM_CPU *cpu, address_word addr)
{
  return (RW (addr) << 16) | RW (addr + 2);
}
#define RLW(addr)		get_longword (cpu, addr)

#define JMP(x)			do { SET_PC (x); State.pc_changed = 1; } while (0)

#define RIE_VECTOR_START  0xffc2
#define AE_VECTOR_START   0xffc3
#define TRAP_VECTOR_START 0xffc4	/* vector for trap 0 */
#define DBT_VECTOR_START  0xffd4
#define SDBT_VECTOR_START 0xffd5

#define INT_VECTOR_START   0xFFFE00 /*maskable interrupt - mapped to ICU */
#define NMI_VECTOR_START   0xFFFF00 /*non-maskable interrupt;for observability*/
#define ISE_VECTOR_START   0xFFFC00 /*in-system emulation trap */
#define ADBG_VECTOR_START  0xFFFC02 /*alternate debug trap */
#define ATRC_VECTOR_START  0xFFFC0C /*alternate trace trap */
#define ABPT_VECTOR_START  0xFFFC0E /*alternate break point trap */


/* Scedule a store of VAL into cr[CR].  MASK indicates the bits in
   cr[CR] that should not be modified (i.e. cr[CR] = (cr[CR] & MASK) |
   (VAL & ~MASK)).  In addition, unless PSR_HW_P, a VAL intended for
   PSR is masked for zero bits. */

extern creg_t move_to_cr (SIM_DESC, SIM_CPU *, int cr, creg_t mask, creg_t val, int psw_hw_p);

#ifndef SIGTRAP
#define SIGTRAP 5
#endif
/* Special purpose trap  */
#define TRAP_BREAKPOINT 8

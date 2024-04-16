#ifndef MICROBLAZE_H
#define MICROBLAZE_H

/* Copyright 2009-2024 Free Software Foundation, Inc.

   This file is part of the Xilinx MicroBlaze simulator.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include "opcodes/microblaze-opcm.h"

#define GET_RD	((inst & RD_MASK) >> RD_LOW)
#define GET_RA	((inst & RA_MASK) >> RA_LOW)
#define GET_RB	((inst & RB_MASK) >> RB_LOW)

#define CPU     (*MICROBLAZE_SIM_CPU (cpu))

#define RD      CPU.regs[rd]
#define RA      CPU.regs[ra]
#define RB      CPU.regs[rb]
/* #define IMM     immword */

#define SA	CPU.spregs[IMM & 0x1]

#define IMM_H	CPU.imm_high
#define IMM_L	((inst & IMM_MASK) >> IMM_LOW)

#define IMM_ENABLE CPU.imm_enable

#define IMM             (IMM_ENABLE ?					\
                         (((unsigned_2)IMM_H << 16) | (unsigned_2)IMM_L) :	\
                         (imm_unsigned ?				\
			  (0xFFFF & IMM_L) :				\
                          (IMM_L & 0x8000 ?                             \
			   (0xFFFF0000 | IMM_L) :                       \
			   (0x0000FFFF & IMM_L))))

#define PC	CPU.spregs[0]
#define	MSR	CPU.spregs[1]
#define SP      CPU.regs[29]
#define RETREG  CPU.regs[3]


#define MEM_RD_BYTE(X)	sim_core_read_1 (cpu, 0, read_map, X)
#define MEM_RD_HALF(X)	sim_core_read_2 (cpu, 0, read_map, X)
#define MEM_RD_WORD(X)	sim_core_read_4 (cpu, 0, read_map, X)
#define MEM_RD_UBYTE(X) (unsigned_1) MEM_RD_BYTE(X)
#define MEM_RD_UHALF(X) (unsigned_2) MEM_RD_HALF(X)
#define MEM_RD_UWORD(X) (unsigned_4) MEM_RD_WORD(X)

#define MEM_WR_BYTE(X, D) sim_core_write_1 (cpu, 0, write_map, X, D)
#define MEM_WR_HALF(X, D) sim_core_write_2 (cpu, 0, write_map, X, D)
#define MEM_WR_WORD(X, D) sim_core_write_4 (cpu, 0, write_map, X, D)


#define MICROBLAZE_SEXT8(X)	((char) X)
#define MICROBLAZE_SEXT16(X)	((short) X)


#define CARRY		carry
#define C_rd		((MSR & 0x4) >> 2)
#define C_wr(D)		MSR = (D ? MSR | 0x80000004 : MSR & 0x7FFFFFFB)

#define C_calc(X, Y, C)	((((unsigned_4)Y == MAX_WORD) && (C == 1)) ?		 \
			 1 :						 \
			 ((MAX_WORD - (unsigned_4)X) < ((unsigned_4)Y + C)))

#define BIP_MASK	0x00000008
#define CARRY_MASK	0x00000004
#define INTR_EN_MASK	0x00000002
#define BUSLOCK_MASK	0x00000001

#define DELAY_SLOT      delay_slot_enable = 1
#define BRANCH          branch_taken = 1

#define NUM_REGS 	32
#define NUM_SPECIAL 	2
#define INST_SIZE 	4

#define MAX_WORD	0xFFFFFFFF
#define MICROBLAZE_HALT_INST  0xb8000000

#endif /* MICROBLAZE_H */


/* Blackfin Memory Management Unit (MMU) model.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

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

#ifndef DV_BFIN_MMU_H
#define DV_BFIN_MMU_H

#undef PAGE_SIZE	/* Cleanup system headers.  */

void mmu_check_addr (SIM_CPU *, bu32 addr, bool write, bool inst, int size);
void mmu_check_cache_addr (SIM_CPU *, bu32 addr, bool write, bool inst);
void mmu_process_fault (SIM_CPU *, bu32 addr, bool write, bool inst, bool unaligned, bool miss);
void mmu_log_ifault (SIM_CPU *);

/* MEM_CONTROL */
#define ENM    (1 << 0)
#define ENCPLB (1 << 1)
#define MC     (1 << 2)

#define ENDM         ENM
#define ENDCPLB      ENCPLB
#define DMC_AB_SRAM      0x0
#define DMC_AB_CACHE     0xc
#define DMC_ACACHE_BSRAM 0x8

/* CPLB_DATA */
#define CPLB_VALID   (1 << 0)
#define CPLB_USER_RD (1 << 2)
#define CPLB_USER_WR (1 << 3)
#define CPLB_USER_RW (CPLB_USER_RD | CPLB_USER_WR)
#define CPLB_SUPV_WR (1 << 4)
#define CPLB_L1SRAM  (1 << 5)
#define CPLB_DA0ACC  (1 << 6)
#define CPLB_DIRTY   (1 << 7)
#define CPLB_L1_CHBL (1 << 12)
#define CPLB_WT      (1 << 14)
#define PAGE_SIZE    (3 << 16)
#define PAGE_SIZE_1K (0 << 16)
#define PAGE_SIZE_4K (1 << 16)
#define PAGE_SIZE_1M (2 << 16)
#define PAGE_SIZE_4M (3 << 16)

/* CPLB_STATUS */
#define FAULT_CPLB0   (1 << 0)
#define FAULT_CPLB1   (1 << 1)
#define FAULT_CPLB2   (1 << 2)
#define FAULT_CPLB3   (1 << 3)
#define FAULT_CPLB4   (1 << 4)
#define FAULT_CPLB5   (1 << 5)
#define FAULT_CPLB6   (1 << 6)
#define FAULT_CPLB7   (1 << 7)
#define FAULT_CPLB8   (1 << 8)
#define FAULT_CPLB9   (1 << 9)
#define FAULT_CPLB10  (1 << 10)
#define FAULT_CPLB11  (1 << 11)
#define FAULT_CPLB12  (1 << 12)
#define FAULT_CPLB13  (1 << 13)
#define FAULT_CPLB14  (1 << 14)
#define FAULT_CPLB15  (1 << 15)
#define FAULT_READ    (0 << 16)
#define FAULT_WRITE   (1 << 16)
#define FAULT_USER    (0 << 17)
#define FAULT_SUPV    (1 << 17)
#define FAULT_DAG0    (0 << 18)
#define FAULT_DAG1    (1 << 18)
#define FAULT_ILLADDR (1 << 19)

/* DTEST_COMMAND */
#define TEST_READ       (0 << 1)
#define TEST_WRITE      (1 << 1)
#define TEST_TAG_ARRAY  (0 << 2)
#define TEST_DATA_ARRAY (1 << 2)
#define TEST_DBANK      (1 << 23)
#define TEST_DATA_SRAM  (0 << 24)
#define TEST_INST_SRAM  (1 << 24)

#endif

/* Special Purpose Registers definitions

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef SPR_DEFS_H
#define SPR_DEFS_H

#define MAX_GRPS 32
#define MAX_SPRS_PER_GRP_BITS 11

/* Base addresses for the groups */
#define SPRGROUP_SYS   (0<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_DMMU  (1<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_IMMU  (2<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_DC    (3<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_IC    (4<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_MAC   (5<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_D     (6<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_PC    (7<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_PM    (8<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_PIC   (9<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_TT    (10<< MAX_SPRS_PER_GRP_BITS)
#define SPRGROUP_FP    (11<< MAX_SPRS_PER_GRP_BITS)

/* System control and status group */
#define SPR_VR          (SPRGROUP_SYS + 0)
#define SPR_UPR         (SPRGROUP_SYS + 1)
#define SPR_CPUCFGR     (SPRGROUP_SYS + 2)
#define SPR_DMMUCFGR    (SPRGROUP_SYS + 3)
#define SPR_IMMUCFGR    (SPRGROUP_SYS + 4)
#define SPR_DCCFGR      (SPRGROUP_SYS + 5)
#define SPR_ICCFGR      (SPRGROUP_SYS + 6)
#define SPR_DCFGR       (SPRGROUP_SYS + 7)
#define SPR_PCCFGR      (SPRGROUP_SYS + 8)
#define SPR_NPC         (SPRGROUP_SYS + 16)
#define SPR_SR          (SPRGROUP_SYS + 17)
#define SPR_PPC         (SPRGROUP_SYS + 18)
#define SPR_FPCSR       (SPRGROUP_SYS + 20)
#define SPR_EPCR_BASE   (SPRGROUP_SYS + 32)
#define SPR_EPCR_LAST   (SPRGROUP_SYS + 47)
#define SPR_EEAR_BASE   (SPRGROUP_SYS + 48)
#define SPR_EEAR_LAST   (SPRGROUP_SYS + 63)
#define SPR_ESR_BASE    (SPRGROUP_SYS + 64)
#define SPR_ESR_LAST    (SPRGROUP_SYS + 79)
#define SPR_GPR_BASE    (SPRGROUP_SYS + 1024)

/* Data MMU group */
#define SPR_DMMUCR  (SPRGROUP_DMMU + 0)
#define SPR_DMMUPR  (SPRGROUP_DMMU + 1)
#define SPR_DTLBEIR     (SPRGROUP_DMMU + 2)
#define SPR_DTLBMR_BASE(WAY)    (SPRGROUP_DMMU + 0x200 + (WAY) * 0x100)
#define SPR_DTLBMR_LAST(WAY)    (SPRGROUP_DMMU + 0x27f + (WAY) * 0x100)
#define SPR_DTLBTR_BASE(WAY)    (SPRGROUP_DMMU + 0x280 + (WAY) * 0x100)
#define SPR_DTLBTR_LAST(WAY)    (SPRGROUP_DMMU + 0x2ff + (WAY) * 0x100)

/* Instruction MMU group */
#define SPR_IMMUCR  (SPRGROUP_IMMU + 0)
#define SPR_ITLBEIR     (SPRGROUP_IMMU + 2)
#define SPR_ITLBMR_BASE(WAY)    (SPRGROUP_IMMU + 0x200 + (WAY) * 0x100)
#define SPR_ITLBMR_LAST(WAY)    (SPRGROUP_IMMU + 0x27f + (WAY) * 0x100)
#define SPR_ITLBTR_BASE(WAY)    (SPRGROUP_IMMU + 0x280 + (WAY) * 0x100)
#define SPR_ITLBTR_LAST(WAY)    (SPRGROUP_IMMU + 0x2ff + (WAY) * 0x100)

/* Data cache group */
#define SPR_DCCR    (SPRGROUP_DC + 0)
#define SPR_DCBPR   (SPRGROUP_DC + 1)
#define SPR_DCBFR   (SPRGROUP_DC + 2)
#define SPR_DCBIR   (SPRGROUP_DC + 3)
#define SPR_DCBWR   (SPRGROUP_DC + 4)
#define SPR_DCBLR   (SPRGROUP_DC + 5)
#define SPR_DCR_BASE(WAY)   (SPRGROUP_DC + 0x200 + (WAY) * 0x200)
#define SPR_DCR_LAST(WAY)   (SPRGROUP_DC + 0x3ff + (WAY) * 0x200)

/* Instruction cache group */
#define SPR_ICCR    (SPRGROUP_IC + 0)
#define SPR_ICBPR   (SPRGROUP_IC + 1)
#define SPR_ICBIR   (SPRGROUP_IC + 2)
#define SPR_ICBLR   (SPRGROUP_IC + 3)
#define SPR_ICR_BASE(WAY)   (SPRGROUP_IC + 0x200 + (WAY) * 0x200)
#define SPR_ICR_LAST(WAY)   (SPRGROUP_IC + 0x3ff + (WAY) * 0x200)

/* MAC group */
#define SPR_MACLO   (SPRGROUP_MAC + 1)
#define SPR_MACHI   (SPRGROUP_MAC + 2)

/* Bit definitions for the Supervision Register.  */
#define SPR_SR_SM          0x00000001 /* Supervisor Mode */
#define SPR_SR_TEE         0x00000002 /* Tick timer Exception Enable */
#define SPR_SR_IEE         0x00000004 /* Interrupt Exception Enable */
#define SPR_SR_DCE         0x00000008 /* Data Cache Enable */
#define SPR_SR_ICE         0x00000010 /* Instruction Cache Enable */
#define SPR_SR_DME         0x00000020 /* Data MMU Enable */
#define SPR_SR_IME         0x00000040 /* Instruction MMU Enable */
#define SPR_SR_LEE         0x00000080 /* Little Endian Enable */
#define SPR_SR_CE          0x00000100 /* CID Enable */
#define SPR_SR_F           0x00000200 /* Condition Flag */
#define SPR_SR_CY          0x00000400 /* Carry flag */
#define SPR_SR_OV          0x00000800 /* Overflow flag */
#define SPR_SR_OVE         0x00001000 /* Overflow flag Exception */
#define SPR_SR_DSX         0x00002000 /* Delay Slot Exception */
#define SPR_SR_EPH         0x00004000 /* Exception Prefix High */
#define SPR_SR_FO          0x00008000 /* Fixed one */
#define SPR_SR_SUMRA       0x00010000 /* Supervisor SPR read access */
#define SPR_SR_RES         0x0ffe0000 /* Reserved */
#define SPR_SR_CID         0xf0000000 /* Context ID */

#endif /* SPR_DEFS_H */

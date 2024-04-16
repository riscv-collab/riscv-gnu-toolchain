/* Blackfin Core Event Controller (CEC) model.

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

#ifndef DV_BFIN_CEC_H
#define DV_BFIN_CEC_H

#include "sim-main.h"

/* 0xFFE02100 ... 0xFFE02110 */
#define BFIN_COREMMR_EVT_OVERRIDE	(BFIN_COREMMR_CEC_BASE + (4 * 0))
#define BFIN_COREMMR_IMASK		(BFIN_COREMMR_CEC_BASE + (4 * 1))
#define BFIN_COREMMR_IPEND		(BFIN_COREMMR_CEC_BASE + (4 * 2))
#define BFIN_COREMMR_ILAT		(BFIN_COREMMR_CEC_BASE + (4 * 3))
#define BFIN_COREMMR_IPRIO		(BFIN_COREMMR_CEC_BASE + (4 * 4))

#define IVG_EMU		0
#define IVG_RST		1
#define IVG_NMI		2
#define IVG_EVX		3
#define IVG_IRPTEN	4	/* Global is Reserved */
#define IVG_IVHW	5
#define IVG_IVTMR	6
#define IVG7		7
#define IVG8		8
#define IVG9		9
#define IVG10		10
#define IVG11		11
#define IVG12		12
#define IVG13		13
#define IVG14		14
#define IVG15		15
#define IVG_USER	16	/* Not real; for internal use */

#define IVG_EMU_B	(1 << IVG_EMU)
#define IVG_RST_B	(1 << IVG_RST)
#define IVG_NMI_B	(1 << IVG_NMI)
#define IVG_EVX_B	(1 << IVG_EVX)
#define IVG_IRPTEN_B	(1 << IVG_IRPTEN)
#define IVG_IVHW_B	(1 << IVG_IVHW)
#define IVG_IVTMR_B	(1 << IVG_IVTMR)
#define IVG7_B		(1 << IVG7)
#define IVG8_B		(1 << IVG8)
#define IVG9_B		(1 << IVG9)
#define IVG10_B		(1 << IVG10)
#define IVG11_B		(1 << IVG11)
#define IVG12_B		(1 << IVG12)
#define IVG13_B		(1 << IVG13)
#define IVG14_B		(1 << IVG14)
#define IVG15_B		(1 << IVG15)
#define IVG_UNMASKABLE_B \
	(IVG_EMU_B | IVG_RST_B | IVG_NMI_B | IVG_EVX_B | IVG_IRPTEN_B)
#define IVG_MASKABLE_B \
	(IVG_IVHW_B | IVG_IVTMR_B | IVG7_B | IVG8_B | IVG9_B | \
	 IVG10_B | IVG11_B | IVG12_B | IVG13_B | IVG14_B | IVG15_B)

#define VEC_SYS		0x0
#define VEC_EXCPT01	0x1
#define VEC_EXCPT02	0x2
#define VEC_EXCPT03	0x3
#define VEC_EXCPT04	0x4
#define VEC_EXCPT05	0x5
#define VEC_EXCPT06	0x6
#define VEC_EXCPT07	0x7
#define VEC_EXCPT08	0x8
#define VEC_EXCPT09	0x9
#define VEC_EXCPT10	0xa
#define VEC_EXCPT11	0xb
#define VEC_EXCPT12	0xc
#define VEC_EXCPT13	0xd
#define VEC_EXCPT14	0xe
#define VEC_EXCPT15	0xf
#define VEC_STEP	0x10	/* single step */
#define VEC_OVFLOW	0x11	/* trace buffer overflow */
#define VEC_UNDEF_I	0x21	/* undefined instruction */
#define VEC_ILGAL_I	0x22	/* illegal instruction combo (multi-issue) */
#define VEC_CPLB_VL	0x23	/* DCPLB protection violation */
#define VEC_MISALI_D	0x24	/* unaligned data access */
#define VEC_UNCOV	0x25	/* unrecoverable event (double fault) */
#define VEC_CPLB_M	0x26	/* DCPLB miss */
#define VEC_CPLB_MHIT	0x27	/* multiple DCPLB hit */
#define VEC_WATCH	0x28	/* watchpoint match */
#define VEC_ISTRU_VL	0x29	/* ADSP-BF535 only */
#define VEC_MISALI_I	0x2a	/* unaligned instruction access */
#define VEC_CPLB_I_VL	0x2b	/* ICPLB protection violation */
#define VEC_CPLB_I_M	0x2c	/* ICPLB miss */
#define VEC_CPLB_I_MHIT	0x2d	/* multiple ICPLB hit */
#define VEC_ILL_RES	0x2e	/* illegal supervisor resource */
/*
 * The hardware reserves 63+ for future use - we use it to tell our
 * normal exception handling code we have a hardware error
 */
#define VEC_HWERR	63
#define VEC_SIM_BASE	64
#define VEC_SIM_HLT	(VEC_SIM_BASE + 1)
#define VEC_SIM_ABORT	(VEC_SIM_BASE + 2)
#define VEC_SIM_TRAP	(VEC_SIM_BASE + 3)
#define VEC_SIM_DBGA	(VEC_SIM_BASE + 4)
extern void cec_exception (SIM_CPU *, int vec_excp);

#define HWERR_SYSTEM_MMR	0x02
#define HWERR_EXTERN_ADDR	0x03
#define HWERR_PERF_FLOW		0x12
#define HWERR_RAISE_5		0x18
extern void cec_hwerr (SIM_CPU *, int hwerr);
extern void cec_latch (SIM_CPU *, int ivg);
extern void cec_return (SIM_CPU *, int ivg);

extern int cec_get_ivg (SIM_CPU *);
extern bool cec_is_supervisor_mode (SIM_CPU *);
extern bool cec_is_user_mode (SIM_CPU *);
extern void cec_require_supervisor (SIM_CPU *);

extern bu32 cec_cli (SIM_CPU *);
extern void cec_sti (SIM_CPU *, bu32 ints);

extern void cec_push_reti (SIM_CPU *);
extern void cec_pop_reti (SIM_CPU *);

#endif

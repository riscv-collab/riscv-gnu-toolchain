/* Register offsets for HPPA running GNU/Linux.

   Copyright (C) 2007-2024 Free Software Foundation, Inc.

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

#ifndef HPPA_LINUX_OFFSETS_H
#define HPPA_LINUX_OFFSETS_H

#define PT_PSW offsetof(struct pt_regs, gr[ 0])
#define PT_GR1 offsetof(struct pt_regs, gr[ 1])
#define PT_GR2 offsetof(struct pt_regs, gr[ 2])
#define PT_GR3 offsetof(struct pt_regs, gr[ 3])
#define PT_GR4 offsetof(struct pt_regs, gr[ 4])
#define PT_GR5 offsetof(struct pt_regs, gr[ 5])
#define PT_GR6 offsetof(struct pt_regs, gr[ 6])
#define PT_GR7 offsetof(struct pt_regs, gr[ 7])
#define PT_GR8 offsetof(struct pt_regs, gr[ 8])
#define PT_GR9 offsetof(struct pt_regs, gr[ 9])
#define PT_GR10 offsetof(struct pt_regs, gr[10])
#define PT_GR11 offsetof(struct pt_regs, gr[11])
#define PT_GR12 offsetof(struct pt_regs, gr[12])
#define PT_GR13 offsetof(struct pt_regs, gr[13])
#define PT_GR14 offsetof(struct pt_regs, gr[14])
#define PT_GR15 offsetof(struct pt_regs, gr[15])
#define PT_GR16 offsetof(struct pt_regs, gr[16])
#define PT_GR17 offsetof(struct pt_regs, gr[17])
#define PT_GR18 offsetof(struct pt_regs, gr[18])
#define PT_GR19 offsetof(struct pt_regs, gr[19])
#define PT_GR20 offsetof(struct pt_regs, gr[20])
#define PT_GR21 offsetof(struct pt_regs, gr[21])
#define PT_GR22 offsetof(struct pt_regs, gr[22])
#define PT_GR23 offsetof(struct pt_regs, gr[23])
#define PT_GR24 offsetof(struct pt_regs, gr[24])
#define PT_GR25 offsetof(struct pt_regs, gr[25])
#define PT_GR26 offsetof(struct pt_regs, gr[26])
#define PT_GR27 offsetof(struct pt_regs, gr[27])
#define PT_GR28 offsetof(struct pt_regs, gr[28])
#define PT_GR29 offsetof(struct pt_regs, gr[29])
#define PT_GR30 offsetof(struct pt_regs, gr[30])
#define PT_GR31 offsetof(struct pt_regs, gr[31])
#define PT_FR0 offsetof(struct pt_regs, fr[ 0])
#define PT_FR1 offsetof(struct pt_regs, fr[ 1])
#define PT_FR2 offsetof(struct pt_regs, fr[ 2])
#define PT_FR3 offsetof(struct pt_regs, fr[ 3])
#define PT_FR4 offsetof(struct pt_regs, fr[ 4])
#define PT_FR5 offsetof(struct pt_regs, fr[ 5])
#define PT_FR6 offsetof(struct pt_regs, fr[ 6])
#define PT_FR7 offsetof(struct pt_regs, fr[ 7])
#define PT_FR8 offsetof(struct pt_regs, fr[ 8])
#define PT_FR9 offsetof(struct pt_regs, fr[ 9])
#define PT_FR10 offsetof(struct pt_regs, fr[10])
#define PT_FR11 offsetof(struct pt_regs, fr[11])
#define PT_FR12 offsetof(struct pt_regs, fr[12])
#define PT_FR13 offsetof(struct pt_regs, fr[13])
#define PT_FR14 offsetof(struct pt_regs, fr[14])
#define PT_FR15 offsetof(struct pt_regs, fr[15])
#define PT_FR16 offsetof(struct pt_regs, fr[16])
#define PT_FR17 offsetof(struct pt_regs, fr[17])
#define PT_FR18 offsetof(struct pt_regs, fr[18])
#define PT_FR19 offsetof(struct pt_regs, fr[19])
#define PT_FR20 offsetof(struct pt_regs, fr[20])
#define PT_FR21 offsetof(struct pt_regs, fr[21])
#define PT_FR22 offsetof(struct pt_regs, fr[22])
#define PT_FR23 offsetof(struct pt_regs, fr[23])
#define PT_FR24 offsetof(struct pt_regs, fr[24])
#define PT_FR25 offsetof(struct pt_regs, fr[25])
#define PT_FR26 offsetof(struct pt_regs, fr[26])
#define PT_FR27 offsetof(struct pt_regs, fr[27])
#define PT_FR28 offsetof(struct pt_regs, fr[28])
#define PT_FR29 offsetof(struct pt_regs, fr[29])
#define PT_FR30 offsetof(struct pt_regs, fr[30])
#define PT_FR31 offsetof(struct pt_regs, fr[31])
#define PT_SR0 offsetof(struct pt_regs, sr[ 0])
#define PT_SR1 offsetof(struct pt_regs, sr[ 1])
#define PT_SR2 offsetof(struct pt_regs, sr[ 2])
#define PT_SR3 offsetof(struct pt_regs, sr[ 3])
#define PT_SR4 offsetof(struct pt_regs, sr[ 4])
#define PT_SR5 offsetof(struct pt_regs, sr[ 5])
#define PT_SR6 offsetof(struct pt_regs, sr[ 6])
#define PT_SR7 offsetof(struct pt_regs, sr[ 7])
#define PT_IASQ0 offsetof(struct pt_regs, iasq[0])
#define PT_IASQ1 offsetof(struct pt_regs, iasq[1])
#define PT_IAOQ0 offsetof(struct pt_regs, iaoq[0])
#define PT_IAOQ1 offsetof(struct pt_regs, iaoq[1])
#define PT_CR27 offsetof(struct pt_regs, cr27)
#define PT_ORIG_R28 offsetof(struct pt_regs, orig_r28)
#define PT_KSP offsetof(struct pt_regs, ksp)
#define PT_KPC offsetof(struct pt_regs, kpc)
#define PT_SAR offsetof(struct pt_regs, sar)
#define PT_IIR offsetof(struct pt_regs, iir)
#define PT_ISR offsetof(struct pt_regs, isr)
#define PT_IOR offsetof(struct pt_regs, ior)

#endif /* HPPA_LINUX_OFFSETS_H */

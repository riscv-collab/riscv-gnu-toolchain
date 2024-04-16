/* Blackfin Trace (TBUF) model.

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

#ifndef DV_BFIN_TRACE_H
#define DV_BFIN_TRACE_H

/* TBUFCTL Masks */
#define TBUFPWR			0x0001
#define TBUFEN			0x0002
#define TBUFOVF			0x0004
#define TBUFCMPLP_SINGLE	0x0008
#define TBUFCMPLP_DOUBLE	0x0010
#define TBUFCMPLP		(TBUFCMPLP_SINGLE | TBUFCMPLP_DOUBLE)

void bfin_trace_queue (SIM_CPU *, bu32 src_pc, bu32 dst_pc, int hwloop);

#endif

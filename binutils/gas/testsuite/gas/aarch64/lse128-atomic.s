/* lse128-atomic.s Test file For AArch64 LSE128 atomic instructions
   encoding.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of GAS.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

	.macro format_0 op
	.irp suffix, , a, al, l
		\op\suffix x0, x1, [x2]
		\op\suffix x2, x3, [sp]
	.endr
	.endm

func:
	format_0 ldclrp
	format_0 ldsetp
	format_0 swpp

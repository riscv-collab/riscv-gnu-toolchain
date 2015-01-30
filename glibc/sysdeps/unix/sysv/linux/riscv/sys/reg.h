/* Copyright (C) 2001-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _SYS_REG_H
#define _SYS_REG_H	1

/* Index into an array of 8 byte longs returned from ptrace for
   location of the users' stored general purpose registers.  Note that
   these are in logical order, not physical order! */

#define REG_PC 0

#define REG_RA 1
#define REG_SP 2
#define REG_GP 3
#define REG_TP 4

#define REG_T0 5
#define REG_T1 6
#define REG_T2 7
#define REG_T3 28
#define REG_T4 29
#define REG_T5 30
#define REG_T6 31

#define REG_S0 8
#define REG_S1 9
#define REG_S2 18
#define REG_S3 19
#define REG_S4 20
#define REG_S5 21
#define REG_S6 22
#define REG_S7 23
#define REG_S8 24
#define REG_S9 25
#define REG_S10 26
#define REG_S11 27

#define REG_A0 10
#define REG_A1 11
#define REG_A2 12
#define REG_A3 13
#define REG_A4 14
#define REG_A5 15
#define REG_A6 16
#define REG_A7 17

#define REG_STATUS 32
#define REG_BADVADDR 33
#define REG_CAUSE 34
#define REG_SYSCALLNO 35

#endif

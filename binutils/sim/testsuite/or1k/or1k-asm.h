/* Testsuite assembly helpers for OpenRISC.

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

#ifndef OR1K_ASM_H
#define OR1K_ASM_H

#define OR1K_INST(...) __VA_ARGS__

#if defined(__OR1K_NODELAY__)
#define OR1K_DELAYED(a, b) a; b
#define OR1K_DELAYED_NOP(a) a
.nodelay
#elif defined(__OR1K_DELAY__)
#define OR1K_DELAYED(a, b) b; a
#define OR1K_DELAYED_NOP(a) a; l.nop
#elif defined(__OR1K_DELAY_COMPAT__)
#define OR1K_DELAYED(a, b) a; b; l.nop
#define OR1K_DELAYED_NOP(a) a; l.nop
#else
#error One of __OR1K_NODELAY__, __OR1K_DELAY__, or __OR1K_DELAY_COMPAT__ must be defined
#endif

#endif /* OR1K_ASM_H */

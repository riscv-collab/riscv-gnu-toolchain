/* UART model.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions and Mike Frysinger.

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

#ifndef DV_M32R_UART_H
#define DV_M32R_UART_H

/* Should move these settings to a flag to the uart device, and the adresses to
   the sim-model framework.  */

/* Serial device addresses.  */
#ifdef M32R_EVA /* orig eva board, no longer supported */
#define UART_BASE_ADDR		0xff102000
#define UART_INCHAR_ADDR	0xff102013
#define UART_OUTCHAR_ADDR	0xff10200f
#define UART_STATUS_ADDR	0xff102006
/* Indicate ready bit is inverted.  */
#define UART_INPUT_READY0
#else
/* These are the values for the MSA2000 board.
   ??? Will eventually need to move this to a config file.  */
#define UART_BASE_ADDR		0xff004000
#define UART_INCHAR_ADDR	0xff004009
#define UART_OUTCHAR_ADDR	0xff004007
#define UART_STATUS_ADDR	0xff004002
#endif

#define UART_INPUT_READY	0x4
#define UART_OUTPUT_READY	0x1

#endif

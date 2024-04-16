/* Simulator for TI MSP430 and MSP430x processors.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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

#ifndef _MSP430_SIM_H_
#define _MSP430_SIM_H_

typedef enum { UNSIGN_32, SIGN_32, UNSIGN_MAC_32, SIGN_MAC_32 } hwmult_type;
typedef enum { UNSIGN_64, SIGN_64 } hw32mult_type;

struct msp430_cpu_state
{
  int regs[16];
  int cio_breakpoint;
  int cio_buffer;

  hwmult_type  hwmult_type;
  uint16_t   hwmult_op1;
  uint16_t   hwmult_op2;
  uint32_t   hwmult_result;
  int32_t     hwmult_signed_result;
  uint32_t   hwmult_accumulator;
  int32_t     hwmult_signed_accumulator;

  hw32mult_type  hw32mult_type;
  uint32_t     hw32mult_op1;
  uint32_t     hw32mult_op2;
  uint64_t     hw32mult_result;
};

#define HWMULT(sd, field) MSP430_SIM_CPU (STATE_CPU (sd, 0))->field

#define MSP430_SIM_CPU(cpu) ((struct msp430_cpu_state *) CPU_ARCH_DATA (cpu))

#endif

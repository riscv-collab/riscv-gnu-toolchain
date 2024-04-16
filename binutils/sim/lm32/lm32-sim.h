/* Contributed by Jon Beniston <jon@beniston.com>

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef LM32_SIM_H
#define LM32_SIM_H

#include "sim/sim-lm32.h"

/* CSRs.  */
#define LM32_CSR_IE             0
#define LM32_CSR_IM             1
#define LM32_CSR_IP             2
#define LM32_CSR_ICC            3
#define LM32_CSR_DCC            4
#define LM32_CSR_CC             5
#define LM32_CSR_CFG            6
#define LM32_CSR_EBA            7
#define LM32_CSR_DC             8
#define LM32_CSR_DEBA           9
#define LM32_CSR_JTX            0xe
#define LM32_CSR_JRX            0xf
#define LM32_CSR_BP0            0x10
#define LM32_CSR_BP1            0x11
#define LM32_CSR_BP2            0x12
#define LM32_CSR_BP3            0x13
#define LM32_CSR_WP0            0x18
#define LM32_CSR_WP1            0x19
#define LM32_CSR_WP2            0x1a
#define LM32_CSR_WP3            0x1b

/* Exception IDs.  */
#define LM32_EID_RESET                  0
#define LM32_EID_BREAKPOINT             1
#define LM32_EID_INSTRUCTION_BUS_ERROR  2
#define LM32_EID_WATCHPOINT             3
#define LM32_EID_DATA_BUS_ERROR         4
#define LM32_EID_DIVIDE_BY_ZERO         5
#define LM32_EID_INTERRUPT              6
#define LM32_EID_SYSTEM_CALL            7

#endif /* LM32_SIM_H */

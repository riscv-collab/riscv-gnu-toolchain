/* Simulator header for cris.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2024 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef CRIS_ARCH_H
#define CRIS_ARCH_H

#define TARGET_BIG_ENDIAN 1

#define WI  SI
#define UWI USI
#define AI  USI

#define IAI USI

/* Enum declaration for model types.  */
typedef enum model_type {
  MODEL_CRISV10, MODEL_CRISV32, MODEL_MAX
} MODEL_TYPE;

#define MAX_MODELS ((int) MODEL_MAX)

/* Enum declaration for unit types.  */
typedef enum unit_type {
  UNIT_NONE, UNIT_CRISV10_U_MOVEM, UNIT_CRISV10_U_MULTIPLY, UNIT_CRISV10_U_SKIP4
 , UNIT_CRISV10_U_STALL, UNIT_CRISV10_U_CONST32, UNIT_CRISV10_U_CONST16, UNIT_CRISV10_U_MEM
 , UNIT_CRISV10_U_EXEC, UNIT_CRISV32_U_EXEC_TO_SR, UNIT_CRISV32_U_EXEC_MOVEM, UNIT_CRISV32_U_EXEC
 , UNIT_CRISV32_U_SKIP4, UNIT_CRISV32_U_CONST32, UNIT_CRISV32_U_CONST16, UNIT_CRISV32_U_JUMP
 , UNIT_CRISV32_U_JUMP_SR, UNIT_CRISV32_U_JUMP_R, UNIT_CRISV32_U_BRANCH, UNIT_CRISV32_U_MULTIPLY
 , UNIT_CRISV32_U_MOVEM_MTOR, UNIT_CRISV32_U_MOVEM_RTOM, UNIT_CRISV32_U_MEM_W, UNIT_CRISV32_U_MEM_R
 , UNIT_CRISV32_U_MEM, UNIT_MAX
} UNIT_TYPE;

#define MAX_UNITS (4)

#endif /* CRIS_ARCH_H */

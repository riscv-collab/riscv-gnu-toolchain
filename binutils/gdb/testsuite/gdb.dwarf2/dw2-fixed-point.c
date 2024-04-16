/* Copyright 2016-2024 Free Software Foundation, Inc.

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

#include <stdint.h>

/* Simulate an Ada variable declared inside package Pck as follow:
      type FP1_Type is delta 0.1 range -1.0 .. +1.0;
      FP1_Var : FP1_Type := 0.25;  */
int8_t pck__fp1_var = 4;

/* Simulate an Ada variable declared inside package Pck as follow:
      type FP1_Type is delta 0.1 range -1.0 .. +1.0;
      FP1_Var2 : FP1_Type := 0.50;
   Basically, the same as FP1_Var, but with a different value.  */
int8_t pck__fp1_var2 = 8;

/* Simulate an Ada variable declared inside package Pck as follow:
      type FP2_Type is delta 0.01 digits 14;
      FP2_Var : FP2_Type := -0.01;  */
int32_t pck__fp2_var = -1;

/* Simulate an Ada variable declared inside package Pck as follow:
      type FP3_Type is delta 0.1 range 0.0 .. 1.0 with Small => 0.1/3.0;
      FP3_Var : FP3_Type := 0.1;  */
int8_t pck__fp3_var = 3;

/* Simulate an Ada variable declared inside package Pck as follow:
      type FP1_Type is delta 0.1 range -1.0 .. +1.0;
      FP1_Var : FP1_Type := 1.0;  */
int8_t pck__fp1_range_var = 16;

int
main (void)
{
  pck__fp1_var++;
  pck__fp1_var2++;
  pck__fp2_var++;
  pck__fp3_var++;
  pck__fp1_range_var++;

  return 0;
}

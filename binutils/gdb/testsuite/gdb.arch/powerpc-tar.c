/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

int main (void)
{
  void * target1 = &&target1_l;
  void * target2 = &&target2_l;
  asm volatile ("mtspr 815,%0" : : "r" (target1) : );

  /* Branch always to TAR.  */
  asm volatile ("bctar 20,0,0"); // marker

 target2_l:
  asm volatile ("nop"); // marker 2
 target1_l:
  asm volatile ("nop");

  return 0;
}

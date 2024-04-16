/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

/* Write bytes with values ranging from 0 to 31 to each byte of each
   corresponding vector register.  */
int main (void)
{
  asm volatile ("vspltisb 0, 0" : : : "v0");
  asm volatile ("vspltisb 1, 1" : : : "v1");

  asm volatile ("vaddubm 2, 1, 1" : : : "v2");
  asm volatile ("vaddubm 3, 2, 1" : : : "v3");
  asm volatile ("vaddubm 4, 3, 1" : : : "v4");
  asm volatile ("vaddubm 5, 4, 1" : : : "v5");
  asm volatile ("vaddubm 6, 5, 1" : : : "v6");
  asm volatile ("vaddubm 7, 6, 1" : : : "v7");
  asm volatile ("vaddubm 8, 7, 1" : : : "v8");
  asm volatile ("vaddubm 9, 8, 1" : : : "v9");
  asm volatile ("vaddubm 10, 9, 1" : : : "v10");
  asm volatile ("vaddubm 11, 10, 1" : : : "v11");
  asm volatile ("vaddubm 12, 11, 1" : : : "v12");
  asm volatile ("vaddubm 13, 12, 1" : : : "v13");
  asm volatile ("vaddubm 14, 13, 1" : : : "v14");
  asm volatile ("vaddubm 15, 14, 1" : : : "v15");
  asm volatile ("vaddubm 16, 15, 1" : : : "v16");
  asm volatile ("vaddubm 17, 16, 1" : : : "v17");
  asm volatile ("vaddubm 18, 17, 1" : : : "v18");
  asm volatile ("vaddubm 19, 18, 1" : : : "v19");
  asm volatile ("vaddubm 20, 19, 1" : : : "v20");
  asm volatile ("vaddubm 21, 20, 1" : : : "v21");
  asm volatile ("vaddubm 22, 21, 1" : : : "v22");
  asm volatile ("vaddubm 23, 22, 1" : : : "v23");
  asm volatile ("vaddubm 24, 23, 1" : : : "v24");
  asm volatile ("vaddubm 25, 24, 1" : : : "v25");
  asm volatile ("vaddubm 26, 25, 1" : : : "v26");
  asm volatile ("vaddubm 27, 26, 1" : : : "v27");
  asm volatile ("vaddubm 28, 27, 1" : : : "v28");
  asm volatile ("vaddubm 29, 28, 1" : : : "v29");
  asm volatile ("vaddubm 30, 29, 1" : : : "v30");
  asm volatile ("vaddubm 31, 30, 1" : : : "v31");

  asm volatile ("nop"); // marker

  return 0;
}

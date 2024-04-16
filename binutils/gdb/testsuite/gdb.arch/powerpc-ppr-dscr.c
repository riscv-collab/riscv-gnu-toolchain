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
  /* Set Load Stream Disable bit in DSCR.  */
  unsigned long dscr = 0x20;

  /* This is the non-privileged SPR number to access DSCR,
     available since isa 207.  */
  asm volatile ("mtspr 3,%0" : : "r" (dscr));

  /* Set PPR to low priority (010 in bits 11:13, or
     0x0008000000000000).  */
  asm volatile ("or 1,1,1");
  asm volatile ("nop"); // marker
  asm volatile ("nop");

  return 0;
}

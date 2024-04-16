/* This file is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

int
main (void)
{
  /* Dummy instruction just so GDB doesn't stop at the first breakpoint
     instruction.  */
  __asm __volatile ("nop\n\t");

  /* Multiple BRK instruction patterns.  */
  __asm __volatile ("brk %0\n\t" ::"n"(0x0));
  __asm __volatile ("brk %0\n\t" ::"n"(0x900 + 0xf));
  __asm __volatile ("brk %0\n\t" ::"n"(0xf000));

  return 0;
}

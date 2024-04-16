/* This file is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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
  char buf0[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  char buf1[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
                 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f};
  long val;
  void *addr;
    
  addr = &buf0[0];
  __asm __volatile ("ldr %x0, [%1]\n\t"
		    "ldr q0, [%x0]"
		    : "=r" (val)
		    : "r" (&addr)
		    : "q0" );

  addr = &buf1[0];
  __asm __volatile ("ldr %x0, [%1]\n\t"
		    "ldr q1, [%x0]"
		    : "=r" (val)
		    : "r" (&addr)
		    : "q1" );
  
  return 1;
}


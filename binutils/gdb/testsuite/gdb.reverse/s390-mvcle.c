/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <stddef.h>

char dst[0x1000000];
char src[8] = "abcdefgh";

void
marker1 (void)
{
}

void
marker2 (void)
{
}

static void
mvcle (void)
{
  register void *pdst asm("r2") = dst;
  register size_t ndst asm("r3") = sizeof dst;
  register void *psrc asm("r4") = src;
  register size_t nsrc asm("r5") = sizeof src;
  asm volatile ("0: mvcle 2, 4, 0x69\n"
		"jo 0b\n"
                : "=r" (pdst), "=r" (ndst), "=r" (psrc), "=r" (nsrc)
                : "0" (pdst), "1" (ndst), "2" (psrc), "3" (nsrc)
                : "cc", "memory");
}

int
main (void)
{
  marker1 ();
  mvcle ();
  marker2 ();
  return 0;
}

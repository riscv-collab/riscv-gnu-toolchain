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

int main(void)
{
  unsigned long tmp, cond;
  unsigned long dword = 0;

  /* Test that we can step over ldxr/stxr. This sequence should step from
     ldxr to the following __asm __volatile.  */
  __asm __volatile ("1:     ldxr    %0,%2\n"                             \
                    "       cmp     %0,#1\n"                             \
                    "       b.eq    out\n"                               \
                    "       add     %0,%0,1\n"                           \
                    "       stxr    %w1,%0,%2\n"                         \
                    "       cbnz    %w1,1b"                              \
                    : "=&r" (tmp), "=&r" (cond), "+Q" (dword)            \
                    : : "memory");

  /* This sequence should take the conditional branch and step from ldxr
     to the return dword line.  */
  __asm __volatile ("1:     ldxr    %0,%2\n"                             \
                    "       cmp     %0,#1\n"                             \
                    "       b.eq    out\n"                               \
                    "       add     %0,%0,1\n"                           \
                    "       stxr    %w1,%0,%2\n"                         \
                    "       cbnz    %w1,1b\n"                            \
                    : "=&r" (tmp), "=&r" (cond), "+Q" (dword)            \
                    : : "memory");

  dword = -1;
__asm __volatile ("out:\n");
  return dword;
}

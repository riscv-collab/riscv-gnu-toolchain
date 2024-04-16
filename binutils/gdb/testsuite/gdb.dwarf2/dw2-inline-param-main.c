/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

asm (".globl cu_text_start");
asm ("cu_text_start:");

volatile int v;

int
main (void)
{
  asm (".globl block_start");
  asm ("block_start:");

  v = 1;

  asm ("break_at:");

  v = 2;

  asm (".globl block_end");
  asm ("block_end:");

  return 0;
}

asm (".globl cu_text_end");
asm ("cu_text_end:");

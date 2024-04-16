/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

static volatile int v;

asm ("func_start: .globl func_start\n");
static int
func (void)
{
  v++;
asm ("func0: .globl func0\n");
  v++;
asm ("func1: .globl func1\n");
  v++;
asm ("func2: .globl func2\n");
  v++;
asm ("func3: .globl func3\n");
  return v;
}
asm ("func_end: .globl func_end\n");

/* Equivalent copy but renamed s/func/fund/.  */

asm ("fund_start: .globl fund_start\n");
static int
fund (void)
{
  v++;
asm ("fund0: .globl fund0\n");
  v++;
asm ("fund1: .globl fund1\n");
  v++;
asm ("fund2: .globl fund2\n");
  v++;
asm ("fund3: .globl fund3\n");
  return v;
}
asm ("fund_end: .globl fund_end\n");

int
main (void)
{
  return func () + fund ();
}

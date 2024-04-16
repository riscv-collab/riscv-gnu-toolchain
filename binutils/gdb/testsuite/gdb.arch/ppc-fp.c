/* Copyright 2008-2024 Free Software Foundation, Inc.

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

int
main (int argc, char *argv[])
{
  double result;

  asm ("fdiv %0, %1, %1\n"	/* Invalid operation.  */
       : "=f" (result)
       : "f" (0.0));

  asm ("mtfsf 0xff, %0\n"  /* Reset FPSCR.  */
       :
       : "f" (0.0));

  asm ("fdiv %0, %1, %2\n"	/* Division by zero.  */
       : "=f" (result)
       : "f" (1.25), "f" (0.0));

  return 0;
}

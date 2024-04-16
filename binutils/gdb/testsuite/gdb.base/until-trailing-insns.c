/* Copyright 2022-2024 Free Software Foundation, Inc.

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
main ()
{							/* TAG: main prologue */
    asm ("main_label: .globl main_label");
    asm ("loop_start: .globl loop_start");
    int a, i;
    i = 0;						/* TAG: loop assignment */
    while (1)						/* TAG: loop line */
      {
	asm ("loop_condition: .globl loop_condition");
	if (i >= 10) break;				/* TAG: loop condition */
	asm ("loop_code: .globl loop_code");
	a = i;						/* TAG: loop code */
	asm ("loop_increment: .globl loop_increment");
	i++;						/* TAG: loop increment */
	asm ("loop_jump: .globl loop_jump");
      }
    asm ("main_return: .globl main_return");
    return 0;						/* TAG: main return */
}

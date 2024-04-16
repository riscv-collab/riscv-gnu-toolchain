/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

/* An instruction with the same length as decr_pc_after_break.  This
   is 1-byte on x86.  */
#define INSN asm ("nop")

void
test_user_bp (void)
{
  INSN; /* break for user-bp test here */
  INSN; /* insn1 */
  INSN; /* insn2 */
  INSN; /* insn3 */
}

void
foo (void)
{
}

void
test_step_resume (void)
{
  foo (); /* break for step-resume test here */
  INSN; /* insn1 */
  INSN; /* insn2 */
}

int
main (void)
{
  test_user_bp ();
  test_step_resume ();
  return 0;
}

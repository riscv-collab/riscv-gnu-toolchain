/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* Test to confirm that gdb properly handles lnia instructions
   that load the current PC into a target register when executed
   from a displaced location.  */

.global main
.type main,function
main:
	lnia 3  # /* set r3 */
	lnia 4  # /* set r4 */
	lnia 5  # /* set r5 */
	lnia 6  # /* set r6 */
	lnia 7  # /* set r7 */
	lnia 8  # /* set r8 */
	lnia 9  # /* set r9 */
	blr

	.section	.note.GNU-stack,"",@progbits

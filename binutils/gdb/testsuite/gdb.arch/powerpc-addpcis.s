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


/*  Test to confirm that gdb is properly single stepping over the
    displaced addpcis instruction.  */

.global main
.type main,function
# addpcis: the sum of NIA + ( D || 0x0000) is placed in RT.
main:
	subpcis 3,+0x100  	# /* set r3 */
	subpcis 4,+0x10  	# /* set r4 */
	subpcis 5,+0x1  	# /* set r5 */
	lnia    6  		# /* set r6 */
	addpcis 7,+0x1  	# /* set r7 */
	addpcis 8,+0x10  	# /* set r8 */
	addpcis 9,+0x100  	# /* set r9 */
	blr


	.section	.note.GNU-stack,"",@progbits

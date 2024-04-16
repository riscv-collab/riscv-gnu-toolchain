/* Copyright 2009-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file is part of the gdb testsuite.

   Test displaced stepping over VEX-encoded RIP-relative AVX
   instructions.  */

	.text
	.global main
main:
        nop

/***********************************************/

/* Test a VEX2-encoded RIP-relative instruction.  */

	.global test_rip_vex2
test_rip_vex2:
	vmovsd ro_var(%rip),%xmm0
	.global test_rip_vex2
test_rip_vex2_end:
	nop

/* Test a VEX3-encoded RIP-relative instruction.  */

	.global test_rip_vex3
test_rip_vex3:
	vextractf128 $0x0,%ymm0,var128(%rip)
	.global test_rip_vex3
test_rip_vex3_end:
	nop

	/* skip over test data */
	jmp done

/* RIP-relative ro-data for VEX2 test above.  */

ro_var:
	.8byte 0x1122334455667788
	.8byte 0x8877665544332211

/***********************************************/

/* All done.  */

done:
	mov $0,%rdi
	call exit
	hlt

/* RIP-relative data for VEX3 test above.  */

.data
var128:
	.8byte 0xaa55aa55aa55aa55
	.8byte 0x55aa55aa55aa55aa
	.section	.note.GNU-stack,"",@progbits

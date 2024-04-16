/* Tests instructions l.mfspr and l.mtspr.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

# mach: or1k
# output: report(0x00000000);\n
# output: report(0x00002801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002801);\n
# output: report(0x00000000);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002801);\n
# output: report(0x00002801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00000801);\n
# output: report(0x00002000);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002000);\n
# output: report(0x00000801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002801);\n
# output: report(0x00000001);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00000800);\n
# output: report(0x00002801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00000000);\n
# output: report(0x00002801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002801);\n
# output: report(0x00000000);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002801);\n
# output: report(0x00002801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00000801);\n
# output: report(0x00002000);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002000);\n
# output: report(0x00000801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00002801);\n
# output: report(0x00000001);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: report(0x00000800);\n
# output: report(0x00002801);\n
# output: report(0xdeadbeef);\n
# output: \n
# output: exit(0)\n

#include "or1k-asm-test-env.h"

#define MACLO_VAL 0xdeadbeef

	/* A macro to carry out a test of l.mfspr.

	   MACLO (0x2801) is used as the SPR, since it can be read and
	   cleared using l.macrc and can be set using l.maci.  spr_number
	   and immediate_val_to_or should be chosen to address this
	   register.

	   The value placed in the register is entirely arbitrary - we use
	   0xdeadbeef.  */

	.macro TEST_MFSPR  spr_number, immediate_val_to_or
	REPORT_IMMEDIATE_TO_CONSOLE \spr_number
	REPORT_IMMEDIATE_TO_CONSOLE \immediate_val_to_or
	/* Write MACLO_VAL to MACLO.  */
	l.macrc r2
	LOAD_IMMEDIATE r2, MACLO_VAL
	l.maci  r2, 1
	LOAD_IMMEDIATE r5, \spr_number
	l.mfspr r4, r5, \immediate_val_to_or
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	/* A macro to carry out a test of l.mtspr

	   MACLO (0x2801) is used as the SPR, since it can be read and
	   cleared using l.macrc and can be set using l.maci.  The
	   arguments spr_number and immediate_val_to_or should be chosen
	   to address this register.

	   The value placed in the register is entirely arbitrary - we use
	   0xdeadbeef.  */

	.macro TEST_MTSPR  spr_number, immediate_val_to_or
	REPORT_IMMEDIATE_TO_CONSOLE \spr_number
	REPORT_IMMEDIATE_TO_CONSOLE \immediate_val_to_or
	/* Clear MACLO  */
	l.macrc r2
	LOAD_IMMEDIATE r4, MACLO_VAL
	LOAD_IMMEDIATE r5, \spr_number
	l.mtspr r5, r4, \immediate_val_to_or
	/* Retrieve MACLO.  */
	l.macrc r4
	REPORT_REG_TO_CONSOLE r4
	PRINT_NEWLINE_TO_CONSOLE
	.endm

	STANDARD_TEST_ENVIRONMENT

	.section .text
start_tests:
	PUSH LINK_REGISTER_R9

	/* Test the l.mfspr instruction with a range of operands.  */

	/* Move a test value using zero in the register.  */
	TEST_MFSPR SPR_VR, SPR_MACLO /* 0x0000, 0x2801  */

	/* Move a test value using zero as the constant.  */
	TEST_MFSPR SPR_MACLO, SPR_VR /* 0x2801, 0x0000  */

	/* Move a test value using non-zero in both register and constant.  */

	/* Some of these values will not give the correct result if OR
	   rather than ADD is used to determine the SPR address.  */
	TEST_MFSPR SPR_MACLO,  SPR_MACLO /* 0x2801, 0x2801  */
	TEST_MFSPR SPR_DMMUPR, SPR_ICCR /* 0x0801, 0x2000  */
	TEST_MFSPR SPR_ICCR,   SPR_DMMUPR /* 0x2000, 0x0801  */
	TEST_MFSPR SPR_MACLO,  SPR_UPR /* 0x2801, 0x0001  */
	TEST_MFSPR SPR_DMMUCR, SPR_MACLO /* 0x0800, 0x2801  */

	/* Test the l.mtspr instruction with a range of operands.  */

	/* Move a test value using zero in the register.  */
	TEST_MTSPR SPR_VR, SPR_MACLO /* 0x0000, 0x2801  */

	/* Move a test value using zero as the constant.  */
	TEST_MTSPR SPR_MACLO, SPR_VR /* 0x2801, 0x0000  */

	/* Move a test value using non-zero in both register and constant.  */

	/* Some of these values will not give the correct result if or
	   rather than add is used to determine the SPR address.  */
	TEST_MTSPR SPR_MACLO,  SPR_MACLO /* 0x2801, 0x2801  */
	TEST_MTSPR SPR_DMMUPR, SPR_ICCR /* 0x0801, 0x2000  */
	TEST_MTSPR SPR_ICCR,   SPR_DMMUPR /* 0x2000, 0x0801  */
	TEST_MTSPR SPR_MACLO,  SPR_UPR /* 0x2801, 0x0001  */
	TEST_MTSPR SPR_DMMUCR, SPR_MACLO /* 0x0800, 0x2801  */

	POP LINK_REGISTER_R9
	RETURN_TO_LINK_REGISTER_R9

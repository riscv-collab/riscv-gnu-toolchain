/* This testcase is part of GDB, the GNU debugger.

   Copyright 2004-2024 Free Software Foundation, Inc.

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

void li_stw (void);

void stack_check_probe_1 (void);
void stack_check_probe_2 (void);
void stack_check_probe_loop_1 (void);
void stack_check_probe_loop_2 (void);

int
main (void)
{
  li_stw ();
  stack_check_probe_1 ();
  stack_check_probe_2 ();
  stack_check_probe_loop_1 ();
  stack_check_probe_loop_2 ();
  return 0;
}

/* Asm for procedure li_stw().

   The purpose of this function is to verify that GDB does not
   include the li insn as part of the function prologue (only part
   of the prologue if part of a pair of insns saving vector registers).
   Similarly, GDB should not include the stw insn following the li insn,
   because the source register is not used for parameter passing.  */


asm ("        .csect .text[PR]\n"
     "        .align 2\n"
     "        .lglobl .li_stw\n"
     "        .csect li_stw[DS]\n"
     "li_stw:\n"
     "        .long .li_stw, TOC[tc0], 0\n"
     "        .csect .text[PR]\n"
     ".li_stw:\n"
     "        stw 31,-4(1)\n"
     "        stwu 1,-48(1)\n"
     "        mr 31,1\n"
     "        stw 11,24(31)\n"
     "        li 0,8765\n"
     "        stw 0,28(31)\n"
     "        lwz 1,0(1)\n"
     "        lwz 31,-4(1)\n"
     "        blr\n");

/* Asm for procedure stack_check_probe_1().

   The purpose of this function is to verify that GDB can skip the stack
   checking probing at the beginning of the prologue.  */

asm ("        .csect .text[PR]\n"
     "        .align 2\n"
     "        .globl stack_check_probe_1\n"
     "        .globl .stack_check_probe_1\n"
     "        .csect stack_check_probe_1[DS]\n"
     "stack_check_probe_1:\n"
     "        .long .stack_check_probe_1, TOC[tc0], 0\n"
     "        .csect .text[PR]\n"
     ".stack_check_probe_1:\n"
     "        stw 0,-12336(1)\n"
     "        stw 31,-4(1)\n"
     "        stwu 1,-48(1)\n"
     "        mr 31,1\n"
     "        lwz 1,0(1)\n"
     "        lwz 31,-4(1)\n"
     "        blr\n");

/* Asm for procedure stack_check_probe_2 ().

   Similar to stack_check_probe_1, but with a different probing sequence
   (several probes).  */

asm ("        .csect .text[PR]\n"
     "        .align 2\n"
     "        .globl stack_check_probe_2\n"
     "        .globl .stack_check_probe_2\n"
     "        .csect stack_check_probe_2[DS]\n"
     "stack_check_probe_2:\n"
     "        .long .stack_check_probe_2, TOC[tc0], 0\n"
     "        .csect .text[PR]\n"
     ".stack_check_probe_2:\n"
     "        stw 0,-16384(1)\n"
     "        stw 0,-20480(1)\n"
     "        stw 0,-24576(1)\n"
     "        stw 0,-28672(1)\n"
     "        stw 0,-28752(1)\n"
     "        mflr 0\n"
     "        stw 31,-4(1)\n"
     "        stw 0,8(1)\n"
     "        stwu 1,-16464(1)\n"
     "        mr 31,1\n"
     "        lwz 1,0(1)\n"
     "        lwz 0,8(1)\n"
     "        mtlr 0\n"
     "        lwz 31,-4(1)\n"
     "        blr\n");

/* Asm for procedure stack_check_probe_loop_1() and stack_check_probe_loop_2().

   Similar to stack_check_probe_1, but with a different probing sequence
   (probing loop).  */

asm ("        .csect .text[PR]\n"
     "        .align 2\n"
     "        .globl stack_check_probe_loop_1\n"
     "        .globl .stack_check_probe_loop_1\n"
     "        .csect stack_check_probe_loop_1[DS]\n"
     "stack_check_probe_loop_1:\n"
     "        .long .stack_check_probe_loop_1, TOC[tc0], 0\n"
     "        .csect .text[PR]\n"
     ".stack_check_probe_loop_1:\n"
     "        addi 12,1,-12288\n"
     "        lis 0,-8\n"
     "        ori 0,0,4096\n"
     "        add 0,12,0\n"
     "LPSRL1..0:\n"
     "        cmpw 0,12,0\n"
     "        beq 0,LPSRE1..0\n"
     "        addi 12,12,-4096\n"
     "        stw 0,0(12)\n"
     "        b LPSRL1..0\n"
     "LPSRE1..0:\n"
     "        stw 0,-4080(12)\n"
     "        mflr 0\n"
     "        stw 31,-4(1)\n"
     "        stw 0,8(1)\n"
     "        lis 0,0xfff8\n"
     "        ori 0,0,16\n"
     "        stwux 1,1,0\n"
     "        mr 31,1\n"
     "        lwz 1,0(1)\n"
     "        lwz 0,8(1)\n"
     "        mtlr 0\n"
     "        lwz 31,-4(1)\n"
     "        blr\n");

asm ("        .csect .text[PR]\n"
     "        .align 2\n"
     "        .globl stack_check_probe_loop_2\n"
     "        .globl .stack_check_probe_loop_2\n"
     "        .csect stack_check_probe_loop_2[DS]\n"
     "stack_check_probe_loop_2:\n"
     "        .long .stack_check_probe_loop_2, TOC[tc0], 0\n"
     "        .csect .text[PR]\n"
     ".stack_check_probe_loop_2:\n"
     "        addi 12,1,-12288\n"
     "        lis 0,-8\n"
     "        add 0,12,0\n"
     "LPSRL2..0:\n"
     "        cmpw 0,12,0\n"
     "        beq 0,LPSRE2..0\n"
     "        addi 12,12,-4096\n"
     "        stw 0,0(12)\n"
     "        b LPSRL2..0\n"
     "LPSRE2..0:\n"
     "        mflr 0\n"
     "        stw 31,-4(1)\n"
     "        stw 0,8(1)\n"
     "        lis 0,0xfff8\n"
     "        ori 0,0,16\n"
     "        stwux 1,1,0\n"
     "        mr 31,1\n"
     "        lwz 1,0(1)\n"
     "        lwz 0,8(1)\n"
     "        mtlr 0\n"
     "        lwz 31,-4(1)\n"
     "        blr\n");

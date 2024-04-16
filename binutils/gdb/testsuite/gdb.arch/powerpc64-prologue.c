/* Unwinder test program.

   Copyright 2006-2024 Free Software Foundation, Inc.

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

#ifdef SYMBOL_PREFIX
#define SYMBOL(str)	SYMBOL_PREFIX #str
#else
#define SYMBOL(str)	#str
#endif

void gdb2029 (void);
void optimized_1 (void);

int
main (void)
{
  gdb2029 ();
  optimized_1 ();
  return 0;
}

void
optimized_1_marker (void)
{
}

void
gdb2029_marker (void)
{
}

/* A typical PIC prologue from GCC.  */
/* This is a ppc64(le) variation of the code as seen
   in powerpc-prologue.c.  */

asm(".text\n"
    "	.p2align 3\n"
    SYMBOL (gdb2029) ":\n"
    "	mflr	%r0\n"
    "	std	%r0,16(%r1)\n"
    "	std	%r31,-8(%r1)\n"
    "	stdu	%r1,-128(%r1)\n"
    "	mr	%r31,%r1\n"
    "	mr	%r9,%r3\n"
    "	stw	%r9,176(%r31)\n"
    "	lwz	%r9,176(%r31)\n"
    "	bl	gdb2029_marker\n"
    "	nop\n"
    "	mr	%r9,%r3\n"
    "	mr	%r3,%r9\n"
    "	addi	%r1,%r31,128\n"
    "	ld	%r0,16(%r1)\n"
    "	mtlr	%r0\n"
    "	ld	%r31,-8(%r1)\n"
    "	blr");

/* A heavily scheduled prologue.  */
asm(".text\n"
    "	.p2align 3\n"
    SYMBOL (optimized_1) ":\n"
    "	stdu	%r1,-32(%r1)\n"
    "	lis	%r9,-16342\n"
    "	lis	%r11,-16342\n"
    "	mflr	%r0\n"
    "	addi	%r11,%r11,3776\n"
    "	std	%r30,12(%r1)\n"
    "	addi	%r31,%r9,3152\n"
    "	cmplw	%cr7,%r31,%r11\n"
    "	std	%r0,36(%r1)\n"
    "	mr	%r30,%r3\n"
    "	bl	optimized_1_marker\n"
    "	nop	\n"
    "	ld	%r0,36(%r1)\n"
    "	mtlr	%r0\n"
    "	ld	%r30,12(%r1)\n"
    "	addi	%r1,%r1,32\n"
    "	blr");

/* Copyright 2012-2024 Free Software Foundation, Inc.

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

/* Testcase compiled by gcc -m32 -S -fasynchronous-unwind-tables -O2:

static int __attribute__ ((regparm (0), stdcall, noinline, noclone))
f (int x)
{
  return x;
}
static int __attribute__ ((noinline, noclone))
g (void)
{
  return f (1) + 1;
}
int
main (void)
{
  return g () + 1;
}

 */

	.text
	.type	f, @function
f:
.LFB0:
	.cfi_startproc
	movl	4(%esp), %eax
	ret	$4
	.cfi_endproc
.LFE0:
	.size	f, .-f
	.type	g, @function
g:
.LFB1:
	.cfi_startproc
	subl	$4, %esp
	.cfi_def_cfa_offset 8
	movl	$1, (%esp)
	call	f
	.cfi_def_cfa_offset 4
	subl	$4, %esp
	.cfi_def_cfa_offset 8
	addl	$1, %eax
	addl	$4, %esp
	.cfi_def_cfa_offset 4
	ret
	.cfi_endproc
.LFE1:
	.size	g, .-g
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	call	g
	addl	$1, %eax
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits

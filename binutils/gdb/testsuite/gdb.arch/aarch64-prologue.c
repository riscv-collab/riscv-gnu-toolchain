/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <stdio.h>

extern void excessiveprologue ();

void
innerfunc ()
{
  printf ("inner\n");
}

/* excessiveprologue saves to the stack in multiple ways.  */

asm ("\t.section .gnu.sgstubs,\"ax\",%progbits\n"
     "\t.global excessiveprologue\n"
     "\t.type excessiveprologue, %function\n"
     "excessiveprologue:\n"
     "\tstp	x29, x30, [sp, #-208]!\n"
     "\tmov	x29, sp\n"
     "\tstp	w0,w1,[sp,16]\n"
     "\tstp	x2,x3,[sp,24]\n"
     "\tstr	w4,[sp,40]\n"
     "\tstr	x5,[sp,48]\n"
     "\tstur	w6,[sp,52]\n"
     "\tstur	x7,[sp,56]\n"
     "\tstp	s0,s1,[sp,64]\n"
     "\tstp	d2,d3,[sp,72]\n"
     "\tstp	q4,q5,[sp,96]\n"
     "\tstr	b6,[sp,128]\n"
     "\tstr	h7,[sp,132]\n"
     "\tstr	s8,[sp,136]\n"
     "\tstr	d9,[sp,140]\n"
     "\tstr	q10,[sp,148]\n"
     "\tstur	b11,[sp,164]\n"
     "\tstur	h12,[sp,160]\n"
     "\tstur	s13,[sp,172]\n"
     "\tstur	d14,[sp,176]\n"
     "\tstur	q15,[sp,184]\n"
     "\tbl innerfunc\n"
     "\tldp	w0,w1,[sp,16]\n"
     "\tldp	x2,x3,[sp,24]\n"
     "\tldr	w4,[sp,40]\n"
     "\tldr	x5,[sp,48]\n"
     "\tldur	w6,[sp,52]\n"
     "\tldur	x7,[sp,56]\n"
     "\tldp	s0,s1,[sp,64]\n"
     "\tldp	d2,d3,[sp,72]\n"
     "\tldp	q4,q5,[sp,96]\n"
     "\tldr	b6,[sp,128]\n"
     "\tldr	h7,[sp,132]\n"
     "\tldr	s8,[sp,136]\n"
     "\tldr	d9,[sp,140]\n"
     "\tldr	q10,[sp,148]\n"
     "\tldur	b11,[sp,164]\n"
     "\tldur	h12,[sp,160]\n"
     "\tldur	s13,[sp,172]\n"
     "\tldur	d14,[sp,176]\n"
     "\tldur	q15,[sp,184]\n"
     "\tldp	x29, x30, [sp], #208\n"
     "ret\n");

int
main (void)
{
  excessiveprologue ();
  return 0;
}

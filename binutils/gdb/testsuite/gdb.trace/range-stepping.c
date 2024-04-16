/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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
#define SYMBOL(str)     SYMBOL_PREFIX #str
#else
#define SYMBOL(str)     #str
#endif

/* `set_point' further below is the label where we'll set tracepoints
   at.  The insn at the label must the large enough to fit a fast
   tracepoint jump.  */
#if (defined __x86_64__ || defined __i386__)
#  define NOP "   .byte 0xe9,0x00,0x00,0x00,0x00\n" /* jmp $+5 (5-byte nop) */
#elif (defined __aarch64__)
#  define NOP "    nop\n"
#else
#  define NOP "" /* port me */
#endif

int
main(void)
{
  /* Note: 'volatile' is used to make sure the compiler doesn't
     optimize out these variables.  We want to be sure instructions
     are generated for accesses.  */
  volatile int i = 0;

  /* Generate a single line with a label in the middle where we can
     place either a trap tracepoint or a fast tracepoint.  */
#define LINE_WITH_FAST_TRACEPOINT					\
  do {									\
    i = 1;								\
    asm ("    .global " SYMBOL (set_point) "\n"			\
	 SYMBOL (set_point) ":\n"					\
	 NOP								\
    );									\
    i = 2;								\
 } while (0)

  LINE_WITH_FAST_TRACEPOINT; /* location 1 */

  return 0;
}

/* Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

void standard (void);

int
main (void)
{
  standard ();
  return 0;
}

/* A normal prologue.  */

#ifdef __x86_64__
asm(".text\n"
    "    .align 8\n"
    SYMBOL (standard) ":\n"
    "    push %rbp\n"
    "    mov  %rsp, %rbp\n"
    "    push %rdi\n"
    "    int3\n"
    "    leaveq\n"
    "    retq\n");

#else

asm(".text\n"
    "    .align 8\n"
    "    .global " SYMBOL(standard) "\n"
    SYMBOL (standard) ":\n"
    "    pushl %ebp\n"
    "    movl  %esp, %ebp\n"
    "    pushl %edi\n"
    "    int3\n"
    "    leave\n"
    "    ret\n");

#endif

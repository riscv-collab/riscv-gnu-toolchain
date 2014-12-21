/* Startup code compliant to the ELF Mips ABI.
   Copyright (C) 1995, 1997, 2000, 2001, 2002, 2003, 2004
	Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   In addition to the permissions in the GNU Lesser General Public
   License, the Free Software Foundation gives you unlimited
   permission to link the compiled version of this file with other
   programs, and to distribute those programs without any restriction
   coming from the use of this file. (The GNU Lesser General Public
   License restrictions do apply in other respects; for example, they
   cover modification of the file, and distribution when not linked
   into another program.)

   Note that people who make modified versions of this file are not
   obligated to grant this special exception for their modified
   versions; it is their choice whether to do so. The GNU Lesser
   General Public License gives permission to release a modified
   version without this exception; this exception also makes it
   possible to release a modified version which carries forward this
   exception.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#define __ASSEMBLY__ 1
#include <entry.h>
#include <sysdep.h>
#include <sys/asm.h>

/* The entry point's job is to call __libc_start_main.  Per the ABI,
   a0 contains the address of a function to be passed to atexit.
   __libc_start_main wants this in a5.  */

ENTRY(ENTRY_POINT)
	lla   gp, _gp
	move  a5, a0  /* rtld_fini */
	lla   a0, main
	REG_L a1, 0(sp)      /* argc */
	addi  a2, sp, SZREG  /* argv */
	andi  sp, sp, ALMASK /* Align stack. */
	lla   a3, __libc_csu_init
	lla   a4, __libc_csu_fini
	move  a6, sp  /* stack_end */
	
	tail  __libc_start_main@
END(ENTRY_POINT)

/* Define a symbol for the first piece of initialized data.  */
	.data
	.globl __data_start
__data_start:
	.weak data_start
	data_start = __data_start

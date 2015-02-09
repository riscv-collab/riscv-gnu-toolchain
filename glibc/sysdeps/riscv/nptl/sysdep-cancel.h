/* Copyright (C) 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <sysdep.h>
#include <sysdeps/generic/sysdep.h>
#include <tls.h>
#ifndef __ASSEMBLER__
# include <nptl/pthreadP.h>
#endif
#include <sys/asm.h>

/* Gas will put the initial save of $gp into the CIE, because it appears to
   happen before any instructions.  So we use cfi_same_value instead of
   cfi_restore.  */

#if IS_IN (libc) || IS_IN (libpthread) || IS_IN (librt)

# undef PSEUDO
# define PSEUDO(name, syscall_name, args)				\
      .align 2;								\
  L(pseudo_start):							\
  99: j __syscall_error;						\
  ENTRY (name)								\
    SINGLE_THREAD_P(t0);						\
    bnez t0, L(pseudo_cancel);  					\
  .type __##syscall_name##_nocancel, @function;				\
  .globl __##syscall_name##_nocancel;					\
  __##syscall_name##_nocancel:						\
    li a7, SYS_ify(syscall_name);					\
    scall;								\
    bltz a0, 99b;							\
    ret;								\
  .size __##syscall_name##_nocancel,.-__##syscall_name##_nocancel;	\
  L(pseudo_cancel):							\
    addi sp, sp, -STKSPACE;						\
    REG_S ra, STKOFF_RA(sp);						\
    cfi_rel_offset (ra, STKOFF_RA);					\
    PUSHARGS_##args;			/* save syscall args */		\
    CENABLE;								\
    REG_S a0, STKOFF_SVMSK(sp);		/* save mask */			\
    POPARGS_##args;			/* restore syscall args */	\
    li a7, SYS_ify (syscall_name);					\
    scall;								\
    REG_S a0, STKOFF_A0(sp);		/* save syscall result */	\
    REG_L a0, STKOFF_SVMSK(sp);		/* pass mask as arg1 */		\
    CDISABLE;								\
    REG_L ra, STKOFF_RA(sp);		/* restore return address */	\
    REG_L a0, STKOFF_A0(sp);		/* restore syscall result */	\
    addi sp, sp, STKSPACE;						\
    bltz a0, 99b;							\
  L(pseudo_end):


# define PUSHARGS_0	/* nothing to do */
# define PUSHARGS_1	PUSHARGS_0 REG_S a0, STKOFF_A0(sp); cfi_rel_offset (a0, STKOFF_A0);
# define PUSHARGS_2	PUSHARGS_1 REG_S a1, STKOFF_A1(sp); cfi_rel_offset (a1, STKOFF_A1);
# define PUSHARGS_3	PUSHARGS_2 REG_S a2, STKOFF_A2(sp); cfi_rel_offset (a2, STKOFF_A2);
# define PUSHARGS_4	PUSHARGS_3 REG_S a3, STKOFF_A3(sp); cfi_rel_offset (a3, STKOFF_A3);
# define PUSHARGS_5	PUSHARGS_4 REG_S a4, STKOFF_A4(sp); cfi_rel_offset (a4, STKOFF_A4);
# define PUSHARGS_6	PUSHARGS_5 REG_S a5, STKOFF_A5(sp); cfi_rel_offset (a5, STKOFF_A5);

# define POPARGS_0	/* nothing to do */
# define POPARGS_1	POPARGS_0 REG_L a0, STKOFF_A0(sp);
# define POPARGS_2	POPARGS_1 REG_L a1, STKOFF_A1(sp);
# define POPARGS_3	POPARGS_2 REG_L a2, STKOFF_A2(sp);
# define POPARGS_4	POPARGS_3 REG_L a3, STKOFF_A3(sp);
# define POPARGS_5	POPARGS_4 REG_L a4, STKOFF_A4(sp);
# define POPARGS_6	POPARGS_5 REG_L a5, STKOFF_A5(sp);

/* Avoid D$ misses by keeping less-used arguments further down stack.  */
# define STKOFF_A5	0
# define STKOFF_A4	(STKOFF_A5 + SZREG)
# define STKOFF_A3	(STKOFF_A4 + SZREG)
# define STKOFF_A2	(STKOFF_A3 + SZREG)
# define STKOFF_A1	(STKOFF_A2 + SZREG)
# define STKOFF_A0	(STKOFF_A1 + SZREG)
# define STKOFF_SVMSK	(STKOFF_A0 + SZREG)
# define STKOFF_RA	(STKOFF_SVMSK + SZREG)
# define STKSPACE	(STKOFF_RA + SZREG)

# if IS_IN (libpthread)
#  define CENABLE  call __pthread_enable_asynccancel
#  define CDISABLE call __pthread_disable_asynccancel
# elif IS_IN (librt)
#  define CENABLE  call __librt_enable_asynccancel
#  define CDISABLE call __librt_disable_asynccancel
# else
#  define CENABLE  call __libc_enable_asynccancel
#  define CDISABLE call __libc_disable_asynccancel
# endif

# ifndef __ASSEMBLER__
#  define SINGLE_THREAD_P						\
	__builtin_expect (THREAD_GETMEM (THREAD_SELF,			\
					 header.multiple_threads)	\
			  == 0, 1)
# else
#  include "tcb-offsets.h"
#  define SINGLE_THREAD_P(reg)						\
	lw reg, MULTIPLE_THREADS_OFFSET(tp)
#endif

#elif !defined __ASSEMBLER__

# define SINGLE_THREAD_P 1
# define NO_CANCELLATION 1

#endif

#ifndef __ASSEMBLER__
# define RTLD_SINGLE_THREAD_P \
  __builtin_expect (THREAD_GETMEM (THREAD_SELF, \
				   header.multiple_threads) == 0, 1)
#endif

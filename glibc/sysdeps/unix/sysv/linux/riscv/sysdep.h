/* Copyright (C) 2000, 2002, 2003, 2004, 2005, 2006, 2009
   Free Software Foundation, Inc.
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

#ifndef _LINUX_MIPS_SYSDEP_H
#define _LINUX_MIPS_SYSDEP_H 1

/* There is some commonality.  */
#include <sysdeps/unix/riscv/sysdep.h>

#include <tls.h>

/* In order to get __set_errno() definition in INLINE_SYSCALL.  */
#ifndef __ASSEMBLER__
#include <errno.h>
#endif

/* For Linux we can use the system call table in the header file
	/usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#ifdef __STDC__
# define SYS_ify(syscall_name)	__NR_##syscall_name
#else
# define SYS_ify(syscall_name)	__NR_/**/syscall_name
#endif

#ifdef __ASSEMBLER__

/* We don't want the label for the error handler to be visible in the symbol
   table when we define it here.  */
#ifdef __PIC__
# define SYSCALL_ERROR_LABEL 99b
#endif

#else   /* ! __ASSEMBLER__ */

/* Define a macro which expands into the inline wrapper code for a system
   call.  */
#undef INLINE_SYSCALL
#define INLINE_SYSCALL(name, nr, args...)				\
  ({ INTERNAL_SYSCALL_DECL(err);					\
     long result_var = INTERNAL_SYSCALL (name, err, nr, args);		\
     if ( INTERNAL_SYSCALL_ERROR_P (result_var, err) )			\
       {								\
	 __set_errno (INTERNAL_SYSCALL_ERRNO (result_var, err));	\
	 result_var = -1L;						\
       }								\
     result_var; })

#undef INTERNAL_SYSCALL_DECL
#define INTERNAL_SYSCALL_DECL(err) do { } while (0)

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val, err)   ((long) (val) < 0)

#undef INTERNAL_SYSCALL_ERRNO
#define INTERNAL_SYSCALL_ERRNO(val, err)     (-val)

#undef INTERNAL_SYSCALL
#define INTERNAL_SYSCALL(name, err, nr, args...) \
	internal_syscall##nr (SYS_ify (name), err, args)

#undef INTERNAL_SYSCALL_NCS
#define INTERNAL_SYSCALL_NCS(number, err, nr, args...) \
	internal_syscall##nr (number, err, args)

#define internal_syscall0(number, err, dummy...)			\
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0)							\
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall1(number, err, arg0)				\
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0)						\
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall2(number, err, arg0, arg1)	    		\
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	register long __a1 asm("a1") = (long) (arg1); 			\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0), "r"(__a1)				\
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall3(number, err, arg0, arg1, arg2)      		\
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	register long __a1 asm("a1") = (long) (arg1); 			\
	register long __a2 asm("a2") = (long) (arg2); 			\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0), "r"(__a1), "r"(__a2)			\
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall4(number, err, arg0, arg1, arg2, arg3)	  \
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	register long __a1 asm("a1") = (long) (arg1); 			\
	register long __a2 asm("a2") = (long) (arg2); 			\
	register long __a3 asm("a3") = (long) (arg3);   		\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0), "r"(__a1), "r"(__a2), "r"(__a3)	\
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall5(number, err, arg0, arg1, arg2, arg3, arg4)    \
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	register long __a1 asm("a1") = (long) (arg1); 			\
	register long __a2 asm("a2") = (long) (arg2); 			\
	register long __a3 asm("a3") = (long) (arg3);   		\
	register long __a4 asm("a4") = (long) (arg4);   		\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0), "r"(__a1), "r"(__a2), "r"(__a3), "r"(__a4)     \
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall6(number, err, arg0, arg1, arg2, arg3, arg4, arg5) \
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	register long __a1 asm("a1") = (long) (arg1); 			\
	register long __a2 asm("a2") = (long) (arg2); 			\
	register long __a3 asm("a3") = (long) (arg3);   		\
	register long __a4 asm("a4") = (long) (arg4);   		\
	register long __a5 asm("a5") = (long) (arg5);   		\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0), "r"(__a1), "r"(__a2), "r"(__a3), "r"(__a4), "r"(__a5)     \
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define internal_syscall7(number, err, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
({ 									\
	long _sys_result;						\
									\
	{								\
	register long __v0 asm("v0") = number;				\
	register long __a0 asm("a0") = (long) (arg0); 			\
	register long __a1 asm("a1") = (long) (arg1); 			\
	register long __a2 asm("a2") = (long) (arg2); 			\
	register long __a3 asm("a3") = (long) (arg3);   		\
	register long __a4 asm("a4") = (long) (arg4);   		\
	register long __a5 asm("a5") = (long) (arg5);   		\
	register long __a6 asm("a6") = (long) (arg6);   		\
	__asm__ volatile ( 						\
	"scall\n\t" 							\
	: "+r" (__v0)							\
	: "r" (__v0), "r"(__a0), "r"(__a1), "r"(__a2), "r"(__a3), "r"(__a4), "r"(__a5), "r"(__a6)     \
	: __SYSCALL_CLOBBERS); 						\
	_sys_result = __v0;						\
	}								\
	_sys_result;							\
})

#define __SYSCALL_CLOBBERS "v1", "memory"
#endif /* __ASSEMBLER__ */

/* Pointer mangling is not supported.  */
#define PTR_MANGLE(var) (void) (var)
#define PTR_DEMANGLE(var) (void) (var)

#endif /* linux/mips/sysdep.h */

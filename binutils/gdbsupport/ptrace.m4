dnl Copyright (C) 2012-2024 Free Software Foundation, Inc.
dnl
dnl This file is part of GDB.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Check the return and argument types of ptrace.

AC_DEFUN([GDB_AC_PTRACE],
[

AC_CHECK_HEADERS([sys/ptrace.h ptrace.h])

gdb_ptrace_headers='
#include <sys/types.h>
#if HAVE_SYS_PTRACE_H
# include <sys/ptrace.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
'

# Check return type.  Varargs (used on GNU/Linux) conflict with the
# empty argument list, so check for that explicitly.
AC_CACHE_CHECK(
  [return type of ptrace],
  [gdb_cv_func_ptrace_ret],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[$gdb_ptrace_headers],
	[extern long ptrace (enum __ptrace_request, ...);]
      )],
     [gdb_cv_func_ptrace_ret='long'],
     [AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
	   [$gdb_ptrace_headers],
	   [extern int ptrace ();]
	 )],
	[gdb_cv_func_ptrace_ret='int'],
	[gdb_cv_func_ptrace_ret='long']
      )]
   )]
)

AC_DEFINE_UNQUOTED(
  [PTRACE_TYPE_RET],
  [$gdb_cv_func_ptrace_ret],
  [Define as the return type of ptrace.]
)

# Check argument types.
AC_CACHE_CHECK(
  [types of arguments for ptrace],
  [gdb_cv_func_ptrace_args],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
	[$gdb_ptrace_headers],
	[extern long ptrace (enum __ptrace_request, ...);]
      )],
     [gdb_cv_func_ptrace_args='enum __ptrace_request,int,long,long'],
     [for gdb_arg1 in 'int' 'long'; do
	for gdb_arg2 in 'pid_t' 'int' 'long'; do
	  for gdb_arg3 in 'int *' 'caddr_t' 'int' 'long' 'void *'; do
	    for gdb_arg4 in 'int' 'long' 'void *'; do
	      AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM(
		   [$gdb_ptrace_headers],
		   [extern $gdb_cv_func_ptrace_ret ptrace ($gdb_arg1, $gdb_arg2, $gdb_arg3, $gdb_arg4);]
		 )],
		[gdb_cv_func_ptrace_args="$gdb_arg1,$gdb_arg2,$gdb_arg3,$gdb_arg4";
		 break 4;]
	      )

	      for gdb_arg5 in 'int *' 'int' 'long'; do
		AC_COMPILE_IFELSE(
		  [AC_LANG_PROGRAM(
		     [$gdb_ptrace_headers],
		     [extern $gdb_cv_func_ptrace_ret ptrace ($gdb_arg1, $gdb_arg2, $gdb_arg3, $gdb_arg4, $gdb_arg5);]
		   )],
		  [gdb_cv_func_ptrace_args="$gdb_arg1,$gdb_arg2,$gdb_arg3,$gdb_arg4,$gdb_arg5";
		   break 5;]
		)
	      done
	    done
	  done
	done
      done
      # Provide a safe default value.
      : ${gdb_cv_func_ptrace_args='int,int,long,long'}]
   )]
)

ac_save_IFS=$IFS; IFS=','
set dummy `echo "$gdb_cv_func_ptrace_args" | sed 's/\*/\*/g'`
IFS=$ac_save_IFS
shift
AC_DEFINE_UNQUOTED(PTRACE_TYPE_ARG1, $[1],
  [Define to the type of arg 1 for ptrace.])
AC_DEFINE_UNQUOTED(PTRACE_TYPE_ARG3, $[3],
  [Define to the type of arg 3 for ptrace.])
AC_DEFINE_UNQUOTED(PTRACE_TYPE_ARG4, $[4],
  [Define to the type of arg 4 for ptrace.])
if test -n "$[5]"; then
  AC_DEFINE_UNQUOTED(PTRACE_TYPE_ARG5, $[5],
    [Define to the type of arg 5 for ptrace.])
fi
])

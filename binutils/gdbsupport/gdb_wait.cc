/* Support code for standard wait macros in gdb_wait.h.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"

#include "gdb_wait.h"

#ifdef __MINGW32__

/* The underlying idea is that when a Windows program is terminated by
   a fatal exception, its exit code is the value of that exception, as
   defined by the various EXCEPTION_* symbols in the Windows API
   headers.  We thus emulate WTERMSIG etc. by translating the fatal
   exception codes to more-or-less equivalent Posix signals.

   The translation below is not perfect, because a program could
   legitimately exit normally with a status whose value happens to
   have the high bits set, but that's extremely rare, to say the
   least, and it is deemed such a negligibly small probability of
   false positives is justified by the utility of reporting the
   terminating signal in the "normal" cases.  */

# include <signal.h>

# define WIN32_LEAN_AND_MEAN
# include <windows.h>		/* for EXCEPTION_* constants */

struct xlate_status
{
  /* The exit status (actually, fatal exception code).  */
  DWORD status;

  /* The corresponding signal value.  */
  int sig;
};

int
windows_status_to_termsig (unsigned long status)
{
  static const xlate_status status_xlate_tbl[] =
    {
     {EXCEPTION_ACCESS_VIOLATION,	  SIGSEGV},
     {EXCEPTION_IN_PAGE_ERROR,		  SIGSEGV},
     {EXCEPTION_INVALID_HANDLE,		  SIGSEGV},
     {EXCEPTION_ILLEGAL_INSTRUCTION,	  SIGILL},
     {EXCEPTION_NONCONTINUABLE_EXCEPTION, SIGILL},
     {EXCEPTION_ARRAY_BOUNDS_EXCEEDED,	  SIGSEGV},
     {EXCEPTION_FLT_DENORMAL_OPERAND,	  SIGFPE},
     {EXCEPTION_FLT_DIVIDE_BY_ZERO,	  SIGFPE},
     {EXCEPTION_FLT_INEXACT_RESULT,	  SIGFPE},
     {EXCEPTION_FLT_INVALID_OPERATION,	  SIGFPE},
     {EXCEPTION_FLT_OVERFLOW,		  SIGFPE},
     {EXCEPTION_FLT_STACK_CHECK,	  SIGFPE},
     {EXCEPTION_FLT_UNDERFLOW,		  SIGFPE},
     {EXCEPTION_INT_DIVIDE_BY_ZERO,	  SIGFPE},
     {EXCEPTION_INT_OVERFLOW,		  SIGFPE},
     {EXCEPTION_PRIV_INSTRUCTION,	  SIGILL},
     {EXCEPTION_STACK_OVERFLOW,		  SIGSEGV},
     {CONTROL_C_EXIT,			  SIGTERM}
    };

  for (const xlate_status &x : status_xlate_tbl)
    if (x.status == status)
      return x.sig;

  return -1;
}

#endif	/* __MINGW32__ */

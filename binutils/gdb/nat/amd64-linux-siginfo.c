/* Low-level siginfo manipulation for amd64.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include <signal.h>
#include "amd64-linux-siginfo.h"

#define GDB_SI_SIZE 128

/* The types below define the most complete kernel siginfo types known
   for the architecture, independent of the system/libc headers.  They
   are named from a 64-bit kernel's perspective:

   | layout | type                 |
   |--------+----------------------|
   | 64-bit | nat_siginfo_t        |
   | 32-bit | compat_siginfo_t     |
   | x32    | compat_x32_siginfo_t |
*/

#ifndef __ILP32__

typedef int nat_int_t;
typedef unsigned long nat_uptr_t;

typedef int nat_time_t;
typedef int nat_timer_t;

/* For native 64-bit, clock_t in _sigchld is 64-bit.  */
typedef long nat_clock_t;

union nat_sigval_t
{
  nat_int_t sival_int;
  nat_uptr_t sival_ptr;
};

struct nat_siginfo_t
{
  int si_signo;
  int si_errno;
  int si_code;

  union
  {
    int _pad[((128 / sizeof (int)) - 4)];
    /* kill() */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
    } _kill;

    /* POSIX.1b timers */
    struct
    {
      nat_timer_t _tid;
      int _overrun;
      nat_sigval_t _sigval;
    } _timer;

    /* POSIX.1b signals */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      nat_sigval_t _sigval;
    } _rt;

    /* SIGCHLD */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      int _status;
      nat_clock_t _utime;
      nat_clock_t _stime;
    } _sigchld;

    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
    struct
    {
      nat_uptr_t _addr;
      short int _addr_lsb;
      struct
      {
	nat_uptr_t _lower;
	nat_uptr_t _upper;
      } si_addr_bnd;
    } _sigfault;

    /* SIGPOLL */
    struct
    {
      int _band;
      int _fd;
    } _sigpoll;
  } _sifields;
};

#endif /* __ILP32__ */

/* These types below (compat_*) define a siginfo type that is layout
   compatible with the siginfo type exported by the 32-bit userspace
   support.  */

typedef int compat_int_t;
typedef unsigned int compat_uptr_t;

typedef int compat_time_t;
typedef int compat_timer_t;
typedef int compat_clock_t;

struct compat_timeval
{
  compat_time_t tv_sec;
  int tv_usec;
};

union compat_sigval_t
{
  compat_int_t sival_int;
  compat_uptr_t sival_ptr;
};

struct compat_siginfo_t
{
  int si_signo;
  int si_errno;
  int si_code;

  union
  {
    int _pad[((128 / sizeof (int)) - 3)];

    /* kill() */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
    } _kill;

    /* POSIX.1b timers */
    struct
    {
      compat_timer_t _tid;
      int _overrun;
      compat_sigval_t _sigval;
    } _timer;

    /* POSIX.1b signals */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      compat_sigval_t _sigval;
    } _rt;

    /* SIGCHLD */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      int _status;
      compat_clock_t _utime;
      compat_clock_t _stime;
    } _sigchld;

    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
    struct
    {
      unsigned int _addr;
      short int _addr_lsb;
      struct
      {
	unsigned int _lower;
	unsigned int _upper;
      } si_addr_bnd;
    } _sigfault;

    /* SIGPOLL */
    struct
    {
      int _band;
      int _fd;
    } _sigpoll;
  } _sifields;
};

/* For x32, clock_t in _sigchld is 64bit aligned at 4 bytes.  */
typedef long __attribute__ ((__aligned__ (4))) compat_x32_clock_t;

struct __attribute__ ((__aligned__ (8))) compat_x32_siginfo_t
{
  int si_signo;
  int si_errno;
  int si_code;

  union
  {
    int _pad[((128 / sizeof (int)) - 3)];

    /* kill() */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
    } _kill;

    /* POSIX.1b timers */
    struct
    {
      compat_timer_t _tid;
      int _overrun;
      compat_sigval_t _sigval;
    } _timer;

    /* POSIX.1b signals */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      compat_sigval_t _sigval;
    } _rt;

    /* SIGCHLD */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      int _status;
      compat_x32_clock_t _utime;
      compat_x32_clock_t _stime;
    } _sigchld;

    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
    struct
    {
      unsigned int _addr;
      unsigned int _addr_lsb;
    } _sigfault;

    /* SIGPOLL */
    struct
    {
      int _band;
      int _fd;
    } _sigpoll;
  } _sifields;
};

/* To simplify usage of siginfo fields.  */

#define cpt_si_pid _sifields._kill._pid
#define cpt_si_uid _sifields._kill._uid
#define cpt_si_timerid _sifields._timer._tid
#define cpt_si_overrun _sifields._timer._overrun
#define cpt_si_status _sifields._sigchld._status
#define cpt_si_utime _sifields._sigchld._utime
#define cpt_si_stime _sifields._sigchld._stime
#define cpt_si_ptr _sifields._rt._sigval.sival_ptr
#define cpt_si_addr _sifields._sigfault._addr
#define cpt_si_addr_lsb _sifields._sigfault._addr_lsb
#define cpt_si_lower _sifields._sigfault.si_addr_bnd._lower
#define cpt_si_upper _sifields._sigfault.si_addr_bnd._upper
#define cpt_si_band _sifields._sigpoll._band
#define cpt_si_fd _sifields._sigpoll._fd

/* glibc at least up to 2.3.2 doesn't have si_timerid, si_overrun.
   In their place is si_timer1,si_timer2.  */

#ifndef si_timerid
#define si_timerid si_timer1
#endif
#ifndef si_overrun
#define si_overrun si_timer2
#endif

#ifndef SEGV_BNDERR
#define SEGV_BNDERR	3
#endif

/* The type of the siginfo object the kernel returns in
   PTRACE_GETSIGINFO.  If gdb is built as a x32 program, we get a x32
   siginfo.  */
#ifdef __ILP32__
typedef compat_x32_siginfo_t ptrace_siginfo_t;
#else
typedef nat_siginfo_t ptrace_siginfo_t;
#endif

/*  Convert the system provided siginfo into compatible siginfo.  */

static void
compat_siginfo_from_siginfo (compat_siginfo_t *to, const siginfo_t *from)
{
  ptrace_siginfo_t from_ptrace;

  memcpy (&from_ptrace, from, sizeof (from_ptrace));
  memset (to, 0, sizeof (*to));

  to->si_signo = from_ptrace.si_signo;
  to->si_errno = from_ptrace.si_errno;
  to->si_code = from_ptrace.si_code;

  if (to->si_code == SI_TIMER)
    {
      to->cpt_si_timerid = from_ptrace.cpt_si_timerid;
      to->cpt_si_overrun = from_ptrace.cpt_si_overrun;
      to->cpt_si_ptr = from_ptrace.cpt_si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->cpt_si_pid = from_ptrace.cpt_si_pid;
      to->cpt_si_uid = from_ptrace.cpt_si_uid;
    }
#ifndef __ILP32__
  /* The struct compat_x32_siginfo_t doesn't contain
     cpt_si_lower/cpt_si_upper.  */
  else if (to->si_code == SEGV_BNDERR
	   && to->si_signo == SIGSEGV)
    {
      to->cpt_si_addr = from_ptrace.cpt_si_addr;
      to->cpt_si_lower = from_ptrace.cpt_si_lower;
      to->cpt_si_upper = from_ptrace.cpt_si_upper;
    }
#endif
  else if (to->si_code < 0)
    {
      to->cpt_si_pid = from_ptrace.cpt_si_pid;
      to->cpt_si_uid = from_ptrace.cpt_si_uid;
      to->cpt_si_ptr = from_ptrace.cpt_si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->cpt_si_pid = from_ptrace.cpt_si_pid;
	  to->cpt_si_uid = from_ptrace.cpt_si_uid;
	  to->cpt_si_status = from_ptrace.cpt_si_status;
	  to->cpt_si_utime = from_ptrace.cpt_si_utime;
	  to->cpt_si_stime = from_ptrace.cpt_si_stime;
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->cpt_si_addr = from_ptrace.cpt_si_addr;
	  break;
	case SIGPOLL:
	  to->cpt_si_band = from_ptrace.cpt_si_band;
	  to->cpt_si_fd = from_ptrace.cpt_si_fd;
	  break;
	default:
	  to->cpt_si_pid = from_ptrace.cpt_si_pid;
	  to->cpt_si_uid = from_ptrace.cpt_si_uid;
	  to->cpt_si_ptr = from_ptrace.cpt_si_ptr;
	  break;
	}
    }
}

/* Convert the compatible siginfo into system siginfo.  */

static void
siginfo_from_compat_siginfo (siginfo_t *to, const compat_siginfo_t *from)
{
  ptrace_siginfo_t to_ptrace;

  memset (&to_ptrace, 0, sizeof (to_ptrace));

  to_ptrace.si_signo = from->si_signo;
  to_ptrace.si_errno = from->si_errno;
  to_ptrace.si_code = from->si_code;

  if (to_ptrace.si_code == SI_TIMER)
    {
      to_ptrace.cpt_si_timerid = from->cpt_si_timerid;
      to_ptrace.cpt_si_overrun = from->cpt_si_overrun;
      to_ptrace.cpt_si_ptr = from->cpt_si_ptr;
    }
  else if (to_ptrace.si_code == SI_USER)
    {
      to_ptrace.cpt_si_pid = from->cpt_si_pid;
      to_ptrace.cpt_si_uid = from->cpt_si_uid;
    }
  if (to_ptrace.si_code < 0)
    {
      to_ptrace.cpt_si_pid = from->cpt_si_pid;
      to_ptrace.cpt_si_uid = from->cpt_si_uid;
      to_ptrace.cpt_si_ptr = from->cpt_si_ptr;
    }
  else
    {
      switch (to_ptrace.si_signo)
	{
	case SIGCHLD:
	  to_ptrace.cpt_si_pid = from->cpt_si_pid;
	  to_ptrace.cpt_si_uid = from->cpt_si_uid;
	  to_ptrace.cpt_si_status = from->cpt_si_status;
	  to_ptrace.cpt_si_utime = from->cpt_si_utime;
	  to_ptrace.cpt_si_stime = from->cpt_si_stime;
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to_ptrace.cpt_si_addr = from->cpt_si_addr;
	  to_ptrace.cpt_si_addr_lsb = from->cpt_si_addr_lsb;
	  break;
	case SIGPOLL:
	  to_ptrace.cpt_si_band = from->cpt_si_band;
	  to_ptrace.cpt_si_fd = from->cpt_si_fd;
	  break;
	default:
	  to_ptrace.cpt_si_pid = from->cpt_si_pid;
	  to_ptrace.cpt_si_uid = from->cpt_si_uid;
	  to_ptrace.cpt_si_ptr = from->cpt_si_ptr;
	  break;
	}
    }
  memcpy (to, &to_ptrace, sizeof (to_ptrace));
}

/*  Convert the system provided siginfo into compatible x32 siginfo.  */

static void
compat_x32_siginfo_from_siginfo (compat_x32_siginfo_t *to,
				 const siginfo_t *from)
{
  ptrace_siginfo_t from_ptrace;

  memcpy (&from_ptrace, from, sizeof (from_ptrace));
  memset (to, 0, sizeof (*to));

  to->si_signo = from_ptrace.si_signo;
  to->si_errno = from_ptrace.si_errno;
  to->si_code = from_ptrace.si_code;

  if (to->si_code == SI_TIMER)
    {
      to->cpt_si_timerid = from_ptrace.cpt_si_timerid;
      to->cpt_si_overrun = from_ptrace.cpt_si_overrun;
      to->cpt_si_ptr = from_ptrace.cpt_si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->cpt_si_pid = from_ptrace.cpt_si_pid;
      to->cpt_si_uid = from_ptrace.cpt_si_uid;
    }
  else if (to->si_code < 0)
    {
      to->cpt_si_pid = from_ptrace.cpt_si_pid;
      to->cpt_si_uid = from_ptrace.cpt_si_uid;
      to->cpt_si_ptr = from_ptrace.cpt_si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->cpt_si_pid = from_ptrace.cpt_si_pid;
	  to->cpt_si_uid = from_ptrace.cpt_si_uid;
	  to->cpt_si_status = from_ptrace.cpt_si_status;
	  memcpy (&to->cpt_si_utime, &from_ptrace.cpt_si_utime,
		  sizeof (to->cpt_si_utime));
	  memcpy (&to->cpt_si_stime, &from_ptrace.cpt_si_stime,
		  sizeof (to->cpt_si_stime));
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->cpt_si_addr = from_ptrace.cpt_si_addr;
	  break;
	case SIGPOLL:
	  to->cpt_si_band = from_ptrace.cpt_si_band;
	  to->cpt_si_fd = from_ptrace.cpt_si_fd;
	  break;
	default:
	  to->cpt_si_pid = from_ptrace.cpt_si_pid;
	  to->cpt_si_uid = from_ptrace.cpt_si_uid;
	  to->cpt_si_ptr = from_ptrace.cpt_si_ptr;
	  break;
	}
    }
}




/* Convert the compatible x32 siginfo into system siginfo.  */
static void
siginfo_from_compat_x32_siginfo (siginfo_t *to,
				 const compat_x32_siginfo_t *from)
{
  ptrace_siginfo_t to_ptrace;

  memset (&to_ptrace, 0, sizeof (to_ptrace));
  to_ptrace.si_signo = from->si_signo;
  to_ptrace.si_errno = from->si_errno;
  to_ptrace.si_code = from->si_code;

  if (to_ptrace.si_code == SI_TIMER)
    {
      to_ptrace.cpt_si_timerid = from->cpt_si_timerid;
      to_ptrace.cpt_si_overrun = from->cpt_si_overrun;
      to_ptrace.cpt_si_ptr = from->cpt_si_ptr;
    }
  else if (to_ptrace.si_code == SI_USER)
    {
      to_ptrace.cpt_si_pid = from->cpt_si_pid;
      to_ptrace.cpt_si_uid = from->cpt_si_uid;
    }
  if (to_ptrace.si_code < 0)
    {
      to_ptrace.cpt_si_pid = from->cpt_si_pid;
      to_ptrace.cpt_si_uid = from->cpt_si_uid;
      to_ptrace.cpt_si_ptr = from->cpt_si_ptr;
    }
  else
    {
      switch (to_ptrace.si_signo)
	{
	case SIGCHLD:
	  to_ptrace.cpt_si_pid = from->cpt_si_pid;
	  to_ptrace.cpt_si_uid = from->cpt_si_uid;
	  to_ptrace.cpt_si_status = from->cpt_si_status;
	  memcpy (&to_ptrace.cpt_si_utime, &from->cpt_si_utime,
		  sizeof (to_ptrace.cpt_si_utime));
	  memcpy (&to_ptrace.cpt_si_stime, &from->cpt_si_stime,
		  sizeof (to_ptrace.cpt_si_stime));
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to_ptrace.cpt_si_addr = from->cpt_si_addr;
	  break;
	case SIGPOLL:
	  to_ptrace.cpt_si_band = from->cpt_si_band;
	  to_ptrace.cpt_si_fd = from->cpt_si_fd;
	  break;
	default:
	  to_ptrace.cpt_si_pid = from->cpt_si_pid;
	  to_ptrace.cpt_si_uid = from->cpt_si_uid;
	  to_ptrace.cpt_si_ptr = from->cpt_si_ptr;
	  break;
	}
    }
  memcpy (to, &to_ptrace, sizeof (to_ptrace));
}

/* Convert a ptrace siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to PTRACE.  If DIRECTION is 0, then copy from NATIVE to
   INF.  */

int
amd64_linux_siginfo_fixup_common (siginfo_t *ptrace, gdb_byte *inf,
				  int direction,
				  enum amd64_siginfo_fixup_mode mode)
{
  if (mode == FIXUP_32)
    {
      if (direction == 0)
	compat_siginfo_from_siginfo ((compat_siginfo_t *) inf, ptrace);
      else
	siginfo_from_compat_siginfo (ptrace, (compat_siginfo_t *) inf);

      return 1;
    }
  else if (mode == FIXUP_X32)
    {
      if (direction == 0)
	compat_x32_siginfo_from_siginfo ((compat_x32_siginfo_t *) inf,
					 ptrace);
      else
	siginfo_from_compat_x32_siginfo (ptrace,
					 (compat_x32_siginfo_t *) inf);

      return 1;
    }
  return 0;
}

/* Sanity check for the siginfo structure sizes.  */

static_assert (sizeof (siginfo_t) == GDB_SI_SIZE);
#ifndef __ILP32__
static_assert (sizeof (nat_siginfo_t) == GDB_SI_SIZE);
#endif
static_assert (sizeof (compat_x32_siginfo_t) == GDB_SI_SIZE);
static_assert (sizeof (compat_siginfo_t) == GDB_SI_SIZE);
static_assert (sizeof (ptrace_siginfo_t) == GDB_SI_SIZE);

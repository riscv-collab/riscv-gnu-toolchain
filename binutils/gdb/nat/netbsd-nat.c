/* Internal interfaces for the NetBSD code.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "nat/netbsd-nat.h"
#include "gdbsupport/common-debug.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>

#include <cstring>

#include "gdbsupport/function-view.h"

namespace netbsd_nat
{

/* See netbsd-nat.h.  */

const char *
pid_to_exec_file (pid_t pid)
{
  static char buf[PATH_MAX];
  int mib[4] = {CTL_KERN, KERN_PROC_ARGS, pid, KERN_PROC_PATHNAME};
  size_t buflen = sizeof (buf);
  if (::sysctl (mib, ARRAY_SIZE (mib), buf, &buflen, NULL, 0) != 0)
    return NULL;
  return buf;
}

/* Generic thread (LWP) lister within a specified PID.  The CALLBACK
   parameters is a C++ function that is called for each detected thread.
   When the CALLBACK function returns true, the iteration is interrupted.

   This function assumes internally that the queried process is stopped
   and the number of threads does not change between two sysctl () calls.  */

static bool
netbsd_thread_lister (const pid_t pid,
		      gdb::function_view<bool (const struct kinfo_lwp *)>
		      callback)
{
  int mib[5] = {CTL_KERN, KERN_LWP, pid, sizeof (struct kinfo_lwp), 0};
  size_t size;

  if (sysctl (mib, ARRAY_SIZE (mib), NULL, &size, NULL, 0) == -1 || size == 0)
    perror_with_name (("sysctl"));

  mib[4] = size / sizeof (size_t);

  gdb::unique_xmalloc_ptr<struct kinfo_lwp[]> kl
    ((struct kinfo_lwp *) xcalloc (size, 1));

  if (sysctl (mib, ARRAY_SIZE (mib), kl.get (), &size, NULL, 0) == -1
      || size == 0)
    perror_with_name (("sysctl"));

  for (size_t i = 0; i < size / sizeof (struct kinfo_lwp); i++)
    {
      struct kinfo_lwp *l = &kl[i];

      /* Return true if the specified thread is alive.  */
      auto lwp_alive
	= [] (struct kinfo_lwp *lwp)
	  {
	    switch (lwp->l_stat)
	      {
	      case LSSLEEP:
	      case LSRUN:
	      case LSONPROC:
	      case LSSTOP:
	      case LSSUSPENDED:
		return true;
	      default:
		return false;
	      }
	  };

      /* Ignore embryonic or demised threads.  */
      if (!lwp_alive (l))
	continue;

      if (callback (l))
	return true;
    }

  return false;
}

/* See netbsd-nat.h.  */

bool
thread_alive (ptid_t ptid)
{
  pid_t pid = ptid.pid ();
  lwpid_t lwp = ptid.lwp ();

  auto fn
    = [=] (const struct kinfo_lwp *kl)
      {
	return kl->l_lid == lwp;
      };

  return netbsd_thread_lister (pid, fn);
}

/* See netbsd-nat.h.  */

const char *
thread_name (ptid_t ptid)
{
  pid_t pid = ptid.pid ();
  lwpid_t lwp = ptid.lwp ();

  static char buf[KI_LNAMELEN] = {};

  auto fn
    = [=] (const struct kinfo_lwp *kl)
      {
	if (kl->l_lid == lwp)
	  {
	    xsnprintf (buf, sizeof buf, "%s", kl->l_name);
	    return true;
	  }
	return false;
      };

  if (netbsd_thread_lister (pid, fn))
    return buf;
  else
    return NULL;
}

/* See netbsd-nat.h.  */

void
for_each_thread (pid_t pid, gdb::function_view<void (ptid_t)> callback)
{
  auto fn
    = [=, &callback] (const struct kinfo_lwp *kl)
      {
	ptid_t ptid = ptid_t (pid, kl->l_lid, 0);
	callback (ptid);
	return false;
      };

  netbsd_thread_lister (pid, fn);
}

/* See netbsd-nat.h.  */

void
enable_proc_events (pid_t pid)
{
  int events;

  if (ptrace (PT_GET_EVENT_MASK, pid, &events, sizeof (events)) == -1)
    perror_with_name (("ptrace"));

  events |= PTRACE_LWP_CREATE;
  events |= PTRACE_LWP_EXIT;

  if (ptrace (PT_SET_EVENT_MASK, pid, &events, sizeof (events)) == -1)
    perror_with_name (("ptrace"));
}

/* See netbsd-nat.h.  */

int
qxfer_siginfo (pid_t pid, const char *annex, unsigned char *readbuf,
	       unsigned const char *writebuf, CORE_ADDR offset, int len)
{
  ptrace_siginfo_t psi;

  if (offset > sizeof (siginfo_t))
    return -1;

  if (ptrace (PT_GET_SIGINFO, pid, &psi, sizeof (psi)) == -1)
    return -1;

  if (offset + len > sizeof (siginfo_t))
    len = sizeof (siginfo_t) - offset;

  if (readbuf != NULL)
    memcpy (readbuf, ((gdb_byte *) &psi.psi_siginfo) + offset, len);
  else
    {
      memcpy (((gdb_byte *) &psi.psi_siginfo) + offset, writebuf, len);

      if (ptrace (PT_SET_SIGINFO, pid, &psi, sizeof (psi)) == -1)
	return -1;
    }
  return len;
}

/* See netbsd-nat.h.  */

int
write_memory (pid_t pid, unsigned const char *writebuf, CORE_ADDR offset,
	      size_t len, size_t *xfered_len)
{
  struct ptrace_io_desc io;
  io.piod_op = PIOD_WRITE_D;
  io.piod_len = len;

  size_t bytes_written = 0;

  /* Zero length write always succeeds.  */
  if (len > 0)
    {
      do
	{
	  io.piod_addr = (void *)(writebuf + bytes_written);
	  io.piod_offs = (void *)(offset + bytes_written);

	  errno = 0;
	  int rv = ptrace (PT_IO, pid, &io, 0);
	  if (rv == -1)
	    {
	      gdb_assert (errno != 0);
	      return errno;
	    }
	  if (io.piod_len == 0)
	    break;

	  bytes_written += io.piod_len;
	  io.piod_len = len - bytes_written;
	}
      while (bytes_written < len);
    }

  if (xfered_len != nullptr)
    *xfered_len = bytes_written;

  return 0;
}

/* See netbsd-nat.h.  */

int
read_memory (pid_t pid, unsigned char *readbuf, CORE_ADDR offset,
	      size_t len, size_t *xfered_len)
{
  struct ptrace_io_desc io;
  io.piod_op = PIOD_READ_D;
  io.piod_len = len;

  size_t bytes_read = 0;

  /* Zero length read always succeeds.  */
  if (len > 0)
    {
      do
	{
	  io.piod_offs = (void *)(offset + bytes_read);
	  io.piod_addr = readbuf + bytes_read;

	  int rv = ptrace (PT_IO, pid, &io, 0);
	  if (rv == -1)
	    return errno;
	  if (io.piod_len == 0)
	    break;

	  bytes_read += io.piod_len;
	  io.piod_len = len - bytes_read;
	}
      while (bytes_read < len);
    }

  if (xfered_len != nullptr)
    *xfered_len = bytes_read;

  return 0;
}

}

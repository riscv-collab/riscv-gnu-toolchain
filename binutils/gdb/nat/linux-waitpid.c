/* Wrapper implementation for waitpid for GNU/Linux (LWP layer).

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#include "linux-nat.h"
#include "linux-waitpid.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbsupport/eintr.h"

/* See linux-waitpid.h.  */

std::string
status_to_str (int status)
{
  if (WIFSTOPPED (status))
    {
      if (WSTOPSIG (status) == SYSCALL_SIGTRAP)
	return string_printf ("%s - %s (stopped at syscall)",
			      strsigno (SIGTRAP), strsignal (SIGTRAP));
      else
	return string_printf ("%s - %s (stopped)",
			      strsigno (WSTOPSIG (status)),
			      strsignal (WSTOPSIG (status)));
    }
  else if (WIFSIGNALED (status))
    return string_printf ("%s - %s (terminated)",
			  strsigno (WTERMSIG (status)),
			  strsignal (WTERMSIG (status)));
  else
    return string_printf ("%d (exited)", WEXITSTATUS (status));
}

/* See linux-waitpid.h.  */

int
my_waitpid (int pid, int *status, int flags)
{
  return gdb::handle_eintr (-1, ::waitpid, pid, status, flags);
}

/* User/system CPU time clocks that follow the std::chrono interface.
   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#include "run-time-clock.h"
#if defined HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

using namespace std::chrono;

run_time_clock::time_point
run_time_clock::now () noexcept
{
  return time_point (microseconds (get_run_time ()));
}

#ifdef HAVE_GETRUSAGE
static std::chrono::microseconds
timeval_to_microseconds (struct timeval *tv)
{
  return (seconds (tv->tv_sec) + microseconds (tv->tv_usec));
}
#endif

void
run_time_clock::now (user_cpu_time_clock::time_point &user,
		     system_cpu_time_clock::time_point &system) noexcept
{
#ifdef HAVE_GETRUSAGE
  struct rusage rusage;

  getrusage (RUSAGE_SELF, &rusage);

  microseconds utime = timeval_to_microseconds (&rusage.ru_utime);
  microseconds stime = timeval_to_microseconds (&rusage.ru_stime);
  user = user_cpu_time_clock::time_point (utime);
  system = system_cpu_time_clock::time_point (stime);
#else
  user = user_cpu_time_clock::time_point (microseconds (get_run_time ()));
  system = system_cpu_time_clock::time_point (microseconds::zero ());
#endif
}

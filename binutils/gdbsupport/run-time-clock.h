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

#ifndef COMMON_RUN_TIME_CLOCK_H
#define COMMON_RUN_TIME_CLOCK_H

#include <chrono>

/* Count the total amount of time spent executing in user mode.  */

struct user_cpu_time_clock
{
  using duration = std::chrono::microseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<user_cpu_time_clock>;

  static constexpr bool is_steady = true;

  /* Use run_time_clock::now instead.  */
  static time_point now () noexcept = delete;
};

/* Count the total amount of time spent executing in kernel mode.  */

struct system_cpu_time_clock
{
  using duration = std::chrono::microseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<system_cpu_time_clock>;

  static constexpr bool is_steady = true;

  /* Use run_time_clock::now instead.  */
  static time_point now () noexcept = delete;
};

/* Count the total amount of time spent executing in userspace+kernel
   mode.  */

struct run_time_clock
{
  using duration = std::chrono::microseconds;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<run_time_clock>;

  static constexpr bool is_steady = true;

  static time_point now () noexcept;

  /* Return the user/system time as separate time points, if
     supported.  If not supported, then the combined user+kernel time
     is returned in USER and SYSTEM is set to zero.  */
  static void now (user_cpu_time_clock::time_point &user,
		   system_cpu_time_clock::time_point &system) noexcept;
};

#endif /* COMMON_RUN_TIME_CLOCK_H */

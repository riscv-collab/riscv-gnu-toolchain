/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_GDB_SYS_TIME_H
#define COMMON_GDB_SYS_TIME_H

#include <sys/time.h>

/* On MinGW-w64, gnulib's sys/time.h replaces 'struct timeval' and
   gettimeofday with versions that support 64-bit time_t, for POSIX
   compliance.  However, the gettimeofday replacement does not ever
   return time_t values larger than 31-bit, as it simply returns the
   system's gettimeofday's (signed) 32-bit result as (signed) 64-bit.
   Because we don't really need the POSIX compliance, and it ends up
   causing conflicts with other libraries we use that don't use gnulib
   and thus work with the native struct timeval, such as Winsock2's
   native 'select' and libiberty, simply undefine away gnulib's
   replacements.  */
#if GNULIB_defined_struct_timeval
# undef timeval
# undef gettimeofday
#endif

#endif /* COMMON_GDB_SYS_TIME_H */

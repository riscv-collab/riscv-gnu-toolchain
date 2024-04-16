/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#define _GNU_SOURCE
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef USE_SYSCALL
# define my_time(TLOC) syscall (SYS_time, TLOC)
#else
# define my_time(TLOC) time (TLOC)
#endif

void
marker1 (void)
{
}

void
marker2 (void)
{
}

time_t time_global = -1;

int
main (void)
{
  marker1 ();
  my_time (&time_global);
  marker2 ();
  return 0;
}

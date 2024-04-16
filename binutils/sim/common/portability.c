/* Portability shims for missing OS support.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

This file is part of the GNU Simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <unistd.h>

#include "portability.h"

#ifndef HAVE_GETEGID
int getegid(void)
{
  return 0;
}
#endif

#ifndef HAVE_GETEUID
int geteuid(void)
{
  return 0;
}
#endif

#ifndef HAVE_GETGID
int getgid(void)
{
  return 0;
}
#endif

#ifndef HAVE_GETUID
int getuid(void)
{
  return 0;
}
#endif

#ifndef HAVE_SETGID
int setgid(int gid)
{
  return -1;
}
#endif

#ifndef HAVE_SETUID
int setuid(int uid)
{
  return -1;
}
#endif

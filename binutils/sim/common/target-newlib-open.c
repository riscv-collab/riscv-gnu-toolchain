/* Target errno mappings for newlib/libgloss environment.
   Copyright 1995-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of simulators.

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

#include <fcntl.h>

#include "sim/callback.h"

/* This file is kept up-to-date via the gennltvals.py script.  Do not edit
   anything between the START & END comment blocks below.  */

CB_TARGET_DEFS_MAP cb_init_open_map[] = {
  /* gennltvals: START */
#ifdef O_ACCMODE
  { "O_ACCMODE", O_ACCMODE, (0|1|2) },
#endif
#ifdef O_APPEND
  { "O_APPEND", O_APPEND, 0x0008 },
#endif
#ifdef O_CLOEXEC
  { "O_CLOEXEC", O_CLOEXEC, 0x40000 },
#endif
#ifdef O_CREAT
  { "O_CREAT", O_CREAT, 0x0200 },
#endif
#ifdef O_DIRECT
  { "O_DIRECT", O_DIRECT, 0x80000 },
#endif
#ifdef O_DIRECTORY
  { "O_DIRECTORY", O_DIRECTORY, 0x200000 },
#endif
#ifdef O_EXCL
  { "O_EXCL", O_EXCL, 0x0800 },
#endif
#ifdef O_EXEC
  { "O_EXEC", O_EXEC, 0x400000 },
#endif
#ifdef O_NOCTTY
  { "O_NOCTTY", O_NOCTTY, 0x8000 },
#endif
#ifdef O_NOFOLLOW
  { "O_NOFOLLOW", O_NOFOLLOW, 0x100000 },
#endif
#ifdef O_NONBLOCK
  { "O_NONBLOCK", O_NONBLOCK, 0x4000 },
#endif
#ifdef O_RDONLY
  { "O_RDONLY", O_RDONLY, 0 },
#endif
#ifdef O_RDWR
  { "O_RDWR", O_RDWR, 2 },
#endif
#ifdef O_SEARCH
  { "O_SEARCH", O_SEARCH, 0x400000 },
#endif
#ifdef O_SYNC
  { "O_SYNC", O_SYNC, 0x2000 },
#endif
#ifdef O_TRUNC
  { "O_TRUNC", O_TRUNC, 0x0400 },
#endif
#ifdef O_WRONLY
  { "O_WRONLY", O_WRONLY, 1 },
#endif
  /* gennltvals: END */
  { NULL, -1, -1 },
};

/* Change ownership of a file.
   Copyright (C) 2004-2022 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert, 2004.  */

#include <config.h>

#include <sys/types.h>
#include <errno.h>

/* A trivial substitute for 'fchown'.

   DJGPP 2.03 and earlier (and perhaps later) don't have 'fchown',
   so we pretend no-one has permission for this operation. */

int
fchown (int fd, uid_t uid, gid_t gid)
{
  errno = EPERM;
  return -1;
}

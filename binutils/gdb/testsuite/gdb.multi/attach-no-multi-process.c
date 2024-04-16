/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

/* This program is intended to be started outside of GDB, and then
   attached to by GDB.  */

#include <unistd.h>

int
main ()
{
  /* Don't run forever in case GDB crashes and DejaGNU fails to kill
     this program.  */
  alarm (10);

  while (1)
    usleep (1);

  return 0;
}

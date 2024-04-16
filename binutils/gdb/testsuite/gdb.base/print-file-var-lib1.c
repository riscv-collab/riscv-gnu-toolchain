/* This testcase is part of GDB, the GNU debugger.
   Copyright 2012-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include "print-file-var.h"

ATTRIBUTE_VISIBILITY int this_version_id = 104;

START_EXTERN_C

int
get_version_1 (void)
{
  printf ("get_version_1: &this_version_id=%p, this_version_id=%d\n", &this_version_id, this_version_id);

  return this_version_id;
}

END_EXTERN_C

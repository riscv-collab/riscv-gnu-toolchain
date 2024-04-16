/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#include "gm-pervasive-typedef.h"

my_int use_of_my_int;

void
call_use_my_int_1 (my_int x)
{
  use_of_my_int = use_my_int (x);
}

void
call_use_my_int ()
{
  call_use_my_int_1 (42);
}

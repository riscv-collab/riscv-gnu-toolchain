/* Copyright 2011-2024 Free Software Foundation, Inc.

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

int twice (int i) { /* THIS LINE */
  /* We purposefully put the return type, function prototype and
     opening curly brace on the same line, in an effort to make sure
     that the function prologue would be associated to that line.  */
  return 2 * i;
}

int
main (void)
{
  int t = twice (1);
  return 0;
}

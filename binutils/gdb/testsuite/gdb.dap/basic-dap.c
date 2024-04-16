/* Copyright 2022-2024 Free Software Foundation, Inc.

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

int global_variable = 23;

void
function_breakpoint_here ()
{
  ++global_variable;
  ++global_variable;
}

void
do_not_stop_here ()
{
  /* This exists to test that breakpoints are cleared.  */
}

void
address_breakpoint_here ()
{
}

int
line_breakpoint_here ()
{
  do_not_stop_here ();		/* FIRST */
  function_breakpoint_here ();
  address_breakpoint_here ();
  return 0;			/* BREAK */
}


int
main ()
{
  return line_breakpoint_here ();
}

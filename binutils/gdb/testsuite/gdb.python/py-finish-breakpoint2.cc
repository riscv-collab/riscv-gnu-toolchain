/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <iostream>

int i;

void
throw_exception_1 (int e)
{
  i += 1; /* Finish breakpoint is set here.  */
  i += 1; /* Break before exception.  */
  throw new int (e);
}

void
throw_exception (int e)
{
  throw_exception_1 (e);
}

int
main (void)
{
  try
    {
      throw_exception_1 (10);
    }
  catch (const int *e)
    {
        std::cerr << "Exception #" << *e << std::endl;
    }
  i += 1; /* Break after exception 1.  */

  try
    {
      throw_exception (10);
    }
  catch (const int *e)
    {
        std::cerr << "Exception #" << *e << std::endl;
    }
  i += 1; /* Break after exception 2.  */

  return i;
}

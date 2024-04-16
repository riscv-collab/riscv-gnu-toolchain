/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

#include <iostream>
#include <signal.h>

using namespace std;

class SimpleException
{

public:

  void raise_signal (int dummy)
  {
    if (dummy > 0)
      raise(SIGABRT);
  }

  int no_throw_function ()
  {
    return 1;
  }

  void throw_function ()
  {
    throw 1;
  }

  int throw_function_with_handler ()
  {
    try
      {
	throw 1;
      }
    catch (...)
      {
	cout << "Handled" << endl;
      }

    return 2;
  }

  void call_throw_function_no_handler ()
  {
    throw_function ();
  }

  void call_throw_function_handler ()
  {
    throw_function_with_handler ();
  }
};
SimpleException exceptions;

int
main()
{
  /* Have to call these functions so GCC does not optimize them
     away.  */
  exceptions.raise_signal (-1);
  exceptions.no_throw_function ();
  exceptions.throw_function_with_handler ();
  exceptions.call_throw_function_handler ();
  try
    {
      exceptions.throw_function ();
      exceptions.call_throw_function_no_handler ();
    }
  catch (...)
    {
    }
  return 0;
}

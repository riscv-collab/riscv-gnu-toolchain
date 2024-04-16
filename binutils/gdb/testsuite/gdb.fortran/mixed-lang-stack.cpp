/* Copyright 2020-2024 Free Software Foundation, Inc.

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

#include <cstring>
#include <cstdlib>

class base_one
{
  int num1 = 1;
  int num2 = 2;
  int num3 = 3;
};

class base_two
{
public:
  const char *string = "Something in C++";
  float val = 3.5;
};

class derived_type : public base_one, base_two
{
public:
  derived_type ()
    : base_one (),
      base_two ()
  {
    /* Nothing.  */
  }

private:
  int xxx = 9;
  float yyy = 10.5;
};

static void mixed_func_1f ();
static void mixed_func_1g ();

extern "C"
{
  /* Entry point to be called from Fortran. */
  void
  mixed_func_1e ()
  {
    mixed_func_1f ();
  }

  /* The entry point back into Fortran.  */
  extern void mixed_func_1h_ ();
}

static void
mixed_func_1g (derived_type obj)
{
  mixed_func_1h_ ();
}

static void
mixed_func_1f () {
  derived_type obj;

  mixed_func_1g (obj);
}

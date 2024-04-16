/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

template<typename T>
struct magic
{
  T x;
};

struct object
{
  int a;
};

typedef magic<int> int_magic_t;

typedef object *object_p;

typedef const char *my_string_t;

static int
get_value (object_p obj)
{
  return obj->a;
}

static int
get_something (object_p obj)
{
  return obj->a;
}

static int
get_something (my_string_t msg)
{
  return strlen (msg);
}

static int
grab_it (int_magic_t *var)
{
  return var->x;
}

int
main ()
{
  magic<int> m;
  m.x = 4;

  object obj;
  obj.a = 0;

  int val = (get_value (&obj) + get_something (&obj)
	     + get_something ("abc") + grab_it (&m));
  return val;
}

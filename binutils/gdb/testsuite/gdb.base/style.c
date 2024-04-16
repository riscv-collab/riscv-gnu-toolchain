

/* The leading newlines here are intentional, do not remove.  They are used to
   test that the source highlighter doesn't strip them.  */
/* Copyright 2018-2024 Free Software Foundation, Inc.

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

#define SOME_MACRO 23

enum etype
{
  VALUE_ONE = 1,
  VALUE_TWO = 2
};

struct some_struct
{
  int int_field;
  char *string_field;
  enum etype e_field;
};

struct some_struct struct_value = { 23, "skidoo", VALUE_TWO };

struct just_bitfield
{
  unsigned int field : 3;
};

struct just_bitfield just_bitfield_value = { 4 };

int some_called_function (void)
{
  return 0;
}

int
main (int argc, char **argv)
{
  return some_called_function (); /* break here */
}

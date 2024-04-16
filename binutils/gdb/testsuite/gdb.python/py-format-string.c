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

#include <string.h>

typedef struct point
{
  int x;
  int y;
} point_t;

typedef struct
{
  point_t the_point;
} struct_point_t;

typedef union
{
  int an_int;
  char a_char;
} union_t;

typedef struct
{
  union_t the_union;
} struct_union_t;

typedef enum
{
  ENUM_FOO,
  ENUM_BAR,
} enum_t;

typedef void (*function_t) (int);

static void
my_function(int n)
{
}

#ifdef __cplusplus

struct Base
{
  Base (int a_) : a (a_) {}

  virtual int get_number () { return a; }

  int a;

  static int a_static_member;
};

int Base::a_static_member = 2019;

struct Deriv : Base
{
  Deriv (int b_) : Base (42), b (b_) {}

  virtual int get_number () { return b; }

  int b;
};

#endif

int global_symbol = 42;

int
main ()
{
  point_t a_point_t = { 42, 12 };
  point_t *a_point_t_pointer = &a_point_t;
#ifdef __cplusplus
  point_t &a_point_t_ref = a_point_t;
#endif
  struct point another_point = { 123, 456 };
  struct_point_t a_struct_with_point = { a_point_t };

  struct_union_t a_struct_with_union;
  /* Fill the union in an endianness-independent way.  */
  memset (&a_struct_with_union.the_union, 42,
	  sizeof (a_struct_with_union.the_union));

  enum_t an_enum = ENUM_BAR;

  const char *a_string = "hello world";
  const char *a_binary_string = "hello\0world";
  const char a_binary_string_array[] = "hello\0world";

  const int letters_repeat = 10;
  char a_big_string[26 * letters_repeat + 1];
  a_big_string[26 * letters_repeat] = '\0';
  for (int i = 0; i < letters_repeat; i++)
    for (char c = 'A'; c <= 'Z'; c++)
      a_big_string[i * 26 + c - 'A'] = c;

  int an_array[] = { 2, 3, 5 };

  int an_array_with_repetition[] = {
    1,					/*  1 time.   */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	/* 12 times.  */
    5, 5, 5,				/*  3 times   */
    };

  int *a_symbol_pointer = &global_symbol;

#ifdef __cplusplus
  Deriv a_deriv (123);
  Base &a_base_ref = a_deriv;
#endif

  return 0; /* break here */
}

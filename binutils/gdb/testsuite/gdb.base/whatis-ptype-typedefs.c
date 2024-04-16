/* This test program is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

/* Define typedefs of different types, for testing the "whatis" and
   "ptype" commands.  */

/* Helper macro used to consistently define variables/typedefs using
   the same name scheme.  BASE is the shared part of the name of all
   typedefs/variables generated.  Defines a variable of the given
   typedef type, and then a typedef of that typedef and a variable of
   that new typedef type.  The "double typedef" is useful to checking
   that whatis only strips one typedef level.  For example, if BASE is
   "int", we get:

  int_typedef v_int_typedef; // "v_" stands for variable of typedef type
  typedef int_typedef int_typedef2; // typedef-of-typedef
  int_typedef2 v_int_typedef2; // var of typedef-of-typedef
*/
#define DEF(base)				\
  base ## _typedef v_ ## base ## _typedef;	\
						\
  typedef base ## _typedef base ## _typedef2;	\
  base ## _typedef2 v_ ## base ## _typedef2

/* Void.  */

/* (Can't have variables of void type.)  */

typedef void void_typedef;
typedef void_typedef void_typedef2;

void_typedef *v_void_typedef_ptr;
void_typedef2 *v_void_typedef_ptr2;

/* Integers.  */

typedef int int_typedef;
DEF (int);

/* Floats.  */

typedef float float_typedef;
DEF (float);

/* Double floats.  */

typedef double double_typedef;
DEF (double);

/* Long doubles.  */

typedef long double long_double_typedef;
DEF (long_double);

/* Enums.  */

typedef enum colors {red, green, blue} colors_typedef;
DEF (colors);

/* Structures.  */

typedef struct t_struct
{
  int member;
} t_struct_typedef;
DEF (t_struct);

/* Unions.  */

typedef union t_union
{
  int member;
} t_union_typedef;
DEF (t_union);

/* Arrays.  */

typedef int int_array_typedef[3];
DEF (int_array);

/* An array the same size of t_struct_typedef, so we can test casting.  */
typedef unsigned char uchar_array_t_struct_typedef[sizeof (t_struct_typedef)];
DEF (uchar_array_t_struct);

/* A struct and a eunion the same size as t_struct, so we can test
   casting.  */

typedef struct t_struct_wrapper
{
  struct t_struct base;
} t_struct_wrapper_typedef;
DEF (t_struct_wrapper);

typedef union t_struct_union_wrapper
{
  struct t_struct base;
} t_struct_union_wrapper_typedef;
DEF (t_struct_union_wrapper);

/* Functions / function pointers.  */

typedef void func_ftype (void);
func_ftype *v_func_ftype;

typedef func_ftype func_ftype2;
func_ftype2 *v_func_ftype2;

/* C++ methods / method pointers.  */

#ifdef __cplusplus

namespace ns {

struct Struct { void method (); };
void Struct::method () {}

typedef Struct Struct_typedef;
DEF (Struct);

/* Typedefs/vars in a namespace.  */
typedef void (Struct::*method_ptr_typedef) ();
DEF (method_ptr);

}

/* Similar, but in the global namespace.  */
typedef ns::Struct ns_Struct_typedef;
DEF (ns_Struct);

typedef void (ns::Struct::*ns_method_ptr_typedef) ();
DEF (ns_method_ptr);

#endif

int
main (void)
{
  return 0;
}

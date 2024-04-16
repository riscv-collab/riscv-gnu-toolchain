/* Copyright 2019-2024 Free Software Foundation, Inc.

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

typedef int my_int_t;
typedef float my_float_t;
typedef my_int_t nested_int_t;
typedef my_float_t nested_float_t;

struct baz_t
{
  float f;
  double d;
};

typedef struct baz_t baz_t;
typedef struct baz_t baz;
typedef baz_t nested_baz_t;
typedef baz nested_baz;
typedef struct baz_t *baz_ptr;

enum enum_t
{
 AA, BB, CC
};

typedef enum enum_t my_enum_t;
typedef my_enum_t nested_enum_t;

typedef struct
{
  double d;
  float f;
} anon_struct_t;

typedef anon_struct_t nested_anon_struct_t;

typedef enum
{
 DD, EE, FF
} anon_enum_t;

typedef anon_enum_t nested_anon_enum_t;

union union_t
{
  int i;
  float f;
};

typedef union union_t nested_union_t;

typedef union
{
  int i;
  double d;
} anon_union_t;

typedef anon_union_t nested_anon_union_t;

volatile int var_a;
volatile float var_b;
volatile my_int_t var_c;
volatile my_float_t var_d;
volatile nested_int_t var_e;
volatile nested_float_t var_f;
volatile struct baz_t var_g;
volatile baz_t var_h;
volatile baz var_i;
volatile nested_baz_t var_j;
volatile nested_baz var_k;
volatile baz_ptr var_l;
volatile enum enum_t var_m;
volatile my_enum_t var_n;
volatile nested_enum_t var_o;
volatile anon_struct_t var_p;
volatile nested_anon_struct_t var_q;
volatile anon_enum_t var_r;
volatile nested_anon_enum_t var_s;
volatile union union_t var_t;
volatile nested_union_t var_u;
volatile anon_union_t var_v;
volatile nested_anon_union_t var_w;

#ifdef __cplusplus

class CL
{
  int a;
};

typedef CL my_cl;
typedef CL *my_ptr;

volatile CL var_cpp_a;
volatile my_cl var_cpp_b;
volatile my_ptr var_cpp_c;

#endif /* __cplusplus */

int
main ()
{
  asm ("" ::: "memory");
  return 0;
}

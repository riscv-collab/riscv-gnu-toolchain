/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

#include <stddef.h>
#define SIZE 5

struct foo
{
  int a;
};

typedef struct bar
{
  int x;
  struct foo y;
} BAR;

void
vla_factory (int n)
{
  struct vla_struct
  {
    int something;
    int vla_field[n];
  };
  /* Define a typedef for a VLA structure.  */
  typedef struct vla_struct vla_struct_typedef;
  vla_struct_typedef vla_struct_object;

  struct inner_vla_struct
  {
    int something;
    int vla_field[n];
    int after;
  } inner_vla_struct_object;

  /* Define a structure which uses a typedef for the VLA field
     to make sure that GDB creates the proper type for this field,
     preventing a possible assertion failure (see gdb/21356).  */
  struct vla_struct_typedef_struct_member
  {
    int something;
    vla_struct_typedef vla_object;
  } vla_struct_typedef_struct_member_object;

  union vla_union
  {
    int vla_field[n];
  } vla_union_object;

  /* Like vla_struct_typedef_struct_member but a union type.  */
  union vla_struct_typedef_union_member
  {
    int something;
    vla_struct_typedef vla_object;
  } vla_struct_typedef_union_member_object;
  int i;

  vla_struct_object.something = n;
  inner_vla_struct_object.something = n;
  inner_vla_struct_object.after = n;
  vla_struct_typedef_struct_member_object.something = n * 2;
  vla_struct_typedef_struct_member_object.vla_object.something = n * 3;
  vla_struct_typedef_union_member_object.vla_object.something = n + 1;

  for (i = 0; i < n; i++)
    {
      vla_struct_object.vla_field[i] = i*2;
      vla_union_object.vla_field[i] = i*2;
      inner_vla_struct_object.vla_field[i] = i*2;
      vla_struct_typedef_struct_member_object.vla_object.vla_field[i]
	= i * 3;
      vla_struct_typedef_union_member_object.vla_object.vla_field[i]
	= i * 3 - 1;
    }

  size_t vla_struct_object_size
    = sizeof(vla_struct_object);          /* vlas_filled */
  size_t vla_union_object_size = sizeof(vla_union_object);
  size_t inner_vla_struct_object_size = sizeof(inner_vla_struct_object);

  return;                                 /* break_end_of_vla_factory */
}

int
main (void)
{
  vla_factory(SIZE);
  return 0;
}

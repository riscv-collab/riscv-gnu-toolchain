/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

typedef int my_int;
typedef const my_int const_my_int;
typedef volatile my_int volatile_my_int;
typedef volatile const_my_int volatile_const_my_int;
typedef const volatile_my_int const_volatile_my_int;

my_int v_my_int (0);
__attribute__((used)) const_my_int v_const_my_int (1);
volatile_my_int v_volatile_my_int (2);
const_volatile_my_int v_const_volatile_my_int (3);
volatile_const_my_int v_volatile_const_my_int (4);
__attribute__((used)) const my_int v2_const_my_int (5);
volatile my_int v2_volatile_my_int (6);
const volatile my_int v2_const_volatile_my_int (7);
volatile const my_int v2_volatile_const_my_int (8);

int
main ()
{
  return 0;
}

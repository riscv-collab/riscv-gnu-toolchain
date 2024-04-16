/* Copyright (C) 2022-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

template<typename first, typename second>
class template_var1
{
  first me;
  second me2;
};

template<typename first, typename second>
class template_var2
{
  first me;
  second me2;
};

template<int val1, typename first, int val2, typename second>
class template_var3
{
  first me;
  second me2;
};

template<typename, typename second>
class template_var1;

template<typename, typename>
class template_var2;

template<int, typename, int, typename>
class template_var3;

int
main (int argc, char **argv)
{
  asm ("main_label: .globl main_label");

  template_var1<int, float> var1;
  template_var2<int, float> var2;
  template_var3<0, int, 11, float> var3;

  return 0;
}

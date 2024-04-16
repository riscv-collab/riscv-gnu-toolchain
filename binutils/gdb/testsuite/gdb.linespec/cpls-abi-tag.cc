/* This testcase is part of GDB, the GNU debugger.

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

#define ABI1 __attribute__ ((abi_tag ("tag1")))
#define ABI2 __attribute__ ((abi_tag ("tag2")))
#define ABI3 __attribute__ ((abi_tag ("tag3")))

void ABI1
test_abi_tag_function (int)
{
}

void ABI1
test_abi_tag_ovld_function ()
{
}

void ABI1
test_abi_tag_ovld_function (int)
{
}

/* Code for the overload functions, different ABI tag test.  */

void
test_abi_tag_ovld2_function ()
{
}

void ABI1
test_abi_tag_ovld2_function (short)
{
}

void ABI2
test_abi_tag_ovld2_function (int)
{
}

void ABI2
test_abi_tag_ovld2_function (long)
{
}

struct ABI1 test_abi_tag_struct
{
  ABI2 test_abi_tag_struct ();
  ABI2 ~test_abi_tag_struct ();
};

test_abi_tag_struct::test_abi_tag_struct ()
{}

test_abi_tag_struct::~test_abi_tag_struct ()
{}

ABI3 test_abi_tag_struct s;

/* Code for the abi-tag in parameters test.  */

struct ABI2 abi_tag_param_struct1
{};

struct ABI2 abi_tag_param_struct2
{};

void
test_abi_tag_in_params (abi_tag_param_struct1)
{}

void
test_abi_tag_in_params (abi_tag_param_struct1, abi_tag_param_struct2)
{}

int
main ()
{
  return 0;
}

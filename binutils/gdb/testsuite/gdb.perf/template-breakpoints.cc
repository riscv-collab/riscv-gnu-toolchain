/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#include <iostream>

template <int I, int J, int K, int VAL>
struct ThirdDimension
{
  int
  value () const
  {
    ThirdDimension<I, J, K - 1, VAL> d3;
    return d3.value();
  }
};

template <int I, int J, int VAL>
struct ThirdDimension<I, J, 0, VAL>
{
  int
  value () const
  {
    // Please note - this testcase sets a breakpoint on the following line.
    // It is therefore sensitive to line numbers. If any changes are made to
    // this file, please ensure that the testcase is updated to reflect this.
    std::cout << "Value: " << VAL << std::endl;
    return VAL;
  }
};

template <int I, int J, int K, int VAL>
struct SecondDimension
{
  int
  value () const
  {
    SecondDimension<I, J - 1, K, VAL> d1;
    ThirdDimension<I, J, K, VAL> d2;
    return d1.value() + d2.value();
  }
};

template <int I, int K, int VAL>
struct SecondDimension<I, 0, K, VAL>
{
  int
  value () const
  {
    ThirdDimension<I, 0, K, VAL> d2;
    return d2.value();
  }
};

template <int I, int J, int K, int VAL>
struct FirstDimension
{
  int
  value () const
  {
    FirstDimension<I - 1, J, K, VAL> d1;
    SecondDimension<I, J, K, VAL> d2;
    return d1.value() + d2.value();
  }
};

template <int J, int K, int VAL>
struct FirstDimension<0, J, K, VAL>
{
  int
  value () const
  {
    SecondDimension<0, J, K, VAL> d2;
    return d2.value();
  }
};

int
main (int argc, char *argv[])
{
  FirstDimension<EXPANSION_DEPTH, EXPANSION_DEPTH, EXPANSION_DEPTH, 1> product;
  std::cout << product.value() << std::endl;
  return 0;
}

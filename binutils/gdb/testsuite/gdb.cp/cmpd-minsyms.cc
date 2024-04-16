/* This test case is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

template <typename T>
class GDB
{
 public:
   static int simple (void) { return 0; }
   static int harder (T a) { return 1; }
   template <typename X>
   static X even_harder (T a) { return static_cast<X> (a); }
   int operator == (GDB const& other)
   { return 1; }
  void a (void) const { }
  void b (void) volatile { }
  void c (void) const volatile { }
};

int main(int argc, char **argv)
{
   GDB<int> a, b;
   a.a ();
   a.b ();
   a.c ();
   if (a == b)
     return GDB<char>::harder('a') + GDB<int>::harder(3)
	+ GDB<char>::even_harder<int> ('a');
   return GDB<int>::simple ();
}

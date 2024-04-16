/* Copyright 2015-2024 Free Software Foundation, Inc.

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

namespace N1
{
  namespace N2
  {
    namespace N3
    {
      namespace N4
      {
        static int n4static = 400;

        struct S4
        {
          static int s4static;
          int s4int_;
          S4 () : s4int_ (4) {};
          ~S4 () { --s4static; }

         int get_var () { return s4int_; }
         static int get_svar () { return s4static; }
        };
        int S4::s4static = 40;
      }
    }
  }
}

int
main ()
{
  using namespace N1::N2::N3::N4;

  S4 s4;
  int var = 1234;

  var += s4.s4int_; /* break here */
  return S4::get_svar () - 10 * s4.get_var ();
}

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

namespace NS1 {

namespace NS2 {

struct object
{
  object ()
  {
  }
};

typedef object *object_p;

template<typename T>
struct Templ1
{
  explicit Templ1 (object_p)
  {
  }

  template<typename I>
  static void
  static_method (object_p)
  {
  }
};

template<typename T, typename U>
struct Templ2
{
  explicit Templ2 (object_p)
  {
  }

  template<typename I>
  static void
  static_method (object_p)
  {
  }
};

template<typename T> using AliasTempl = Templ2<int, T>;

typedef Templ1<int> int_Templ1_t;

void
object_p_func (object_p)
{
}

void
int_Templ1_t_func (int_Templ1_t *)
{
}

} // namespace NS2

} // namespace NS1

/* These typedefs have the same name as some of the components within
   NS1 that they alias to, on purpose, to try to confuse GDB and cause
   recursion.  */
using NS2 = int;
using object = NS1::NS2::object;
using Templ1 = NS1::NS2::Templ1<unsigned>;
using Templ2 = NS1::NS2::Templ2<long, long>;
using AliasTempl = NS1::NS2::AliasTempl<int>;

NS2 ns2_int = 0;
object obj;
Templ1 templ1 (0);
NS1::NS2::int_Templ1_t int_templ1 (0);
AliasTempl alias (0);

int
main ()
{
  NS1::NS2::Templ1<int>::static_method<int> (0);
  NS1::NS2::AliasTempl<int>::static_method<int> (0);
  NS1::NS2::object_p_func (0);
  NS1::NS2::int_Templ1_t_func (0);

  return 0;
}

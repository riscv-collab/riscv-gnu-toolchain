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


/* One could use unique_ptr instead, but that requires a GCC which can
   support "-std=c++11".  */

template <typename T>
class smart_ptr
{
public:
  smart_ptr (T *obj) : _obj (obj) { }
  ~smart_ptr () { delete _obj; }

  T *operator-> ();

private:
  T *_obj;
};

template <typename T>
T *
smart_ptr<T>::operator-> ()
{
  return _obj;
}

class A
{
public:
  virtual int func ();
};

int
A::func ()
{
  return 3;
}

int
main ()
{
  A *a_ptr = 0;
  smart_ptr<A> a (new A);

  return a->func();  /* Break here  */
}

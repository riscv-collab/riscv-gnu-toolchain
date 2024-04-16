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
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#ifdef __cplusplus
class SimpleClass
{
 private:
  int i;

 public:
  void seti (int arg)
  {
    i = arg;
  }

  int valueofi (void)
  {
    return i; /* Break in class. */
  }
};
#endif

int qq = 72;			/* line of qq */

int func (int arg)
{
  int i = 2;
  i = i * arg; /* Block break here.  */
  return arg;
}

struct simple_struct
{
  int a;
};

int main (int argc, char *argv[])
{
#ifdef __cplusplus
  SimpleClass sclass;
#endif
  int a = 0;
  int result;
  struct simple_struct ss = { 10 };
  enum tag {one, two, three};
  enum tag t = one;

  result = func (42);

#ifdef __cplusplus
  sclass.seti (42);
  sclass.valueofi ();
#endif
  return 0; /* Break at end.  */
}

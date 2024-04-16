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

class A;
static int get_value (const A* a);

class A
{
public:
  typedef int ATYPE;

  A () : a_ (21) {}
  ATYPE get_var () { return a_; }
  ATYPE get_var (unsigned long a) { return 100; }
  ATYPE get_var (ATYPE a) { return 101; }
  ATYPE get_var (float a) { return 102; }
  ATYPE get_var (void *a) { return 103;}
  ATYPE get_var (A& lr) { return 104; }
  ATYPE get_var (A const& lr) { return 105; }

  ATYPE get_var1 (int n) { return a_ << n; }
  ATYPE get_var2 (int incr, unsigned n) { return (a_ + incr) << n; }

  static ATYPE get_1 (int a) { return a + 1; }
  static ATYPE get_2 (int a, int b) { return a + b + 2; }

  friend ATYPE get_value (const A*);

private:
  ATYPE a_;
};

static A::ATYPE
get_value (A::ATYPE a)
{
  return a;
}

static A::ATYPE
get_value (const A* a)
{
  return a->a_;
}

static A::ATYPE
get_value ()
{
  return 200;
}

typedef int (A::*PMF) (A::ATYPE);

int
main ()
{
  A *a = new A ();
  int var = 1234;
  float f = 1.23;
  unsigned long ul = 0xdeadbeef;
  A const* ac = a;

  PMF pmf = &A::get_var;
  PMF *pmf_p = &pmf;

  var -= a->get_var ();		// break here
  var -= a->get_var (1);
  var -= a->get_var (ul);
  var -= a->get_var (f);
  var -= a->get_var (a);
  var -= a->get_var (*a);
  var -= a->get_var (*ac);
  var -= a->get_var1 (1);
  var -= a->get_var2 (1, 2);
  var += (a->*pmf) (1);
  var -= (a->**pmf_p) (1);

  return var - A::get_1 (1) + A::get_2 (1, 2) + get_value ()
    + get_value (get_value ()) + get_value (a);
}

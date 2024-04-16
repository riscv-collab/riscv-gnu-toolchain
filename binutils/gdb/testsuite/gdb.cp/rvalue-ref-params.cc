/* This test script is part of GDB, the GNU debugger.

   Copyright 2006-2024 Free Software Foundation, Inc.

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

/* Rvalue reference parameter tests, based on ref-params.cc.  */

#include <utility>

struct Parent
{
  Parent (int id0) : id (id0) { }
  int id;
};

struct Child : public Parent
{
  Child (int id0) : Parent (id0) { }
};

int
f1 (Parent &&R)
{
  return R.id;			/* Set breakpoint marker3 here.  */
}

int
f2 (Child &&C)
{
  return f1 (std::move (C));                 /* Set breakpoint marker2 here.  */
}

int
f3 (int &&var_i)
{
  return var_i + 1;
}

int
f4 (float &&var_f)
{
  return static_cast <int> (var_f);
}

struct OtherParent
{
  OtherParent (int other_id0) : other_id (other_id0) { }
  int other_id;
};

struct MultiChild : public Parent, OtherParent
{
  MultiChild (int id0) : Parent (id0), OtherParent (id0 * 2) { }
};

int
mf1 (OtherParent &&R)
{
  return R.other_id;
}

int
mf2 (MultiChild &&C)
{
  return mf1 (std::move (C));
}

/* These are used from within GDB.  */
int global_int = 7;
float global_float = 3.5f;

int
main ()
{
  Child Q(40);
  Child &QR = Q;

  /* Set breakpoint marker1 here.  */

  f1 (Child (41));
  f2 (Child (42));

  MultiChild MQ (53);
  MultiChild &MQR = MQ;

  mf2 (std::move (MQ));			/* Set breakpoint MQ here.  */

  (void) f3 (-1);
  (void) f4 (3.5);

  return 0;
}

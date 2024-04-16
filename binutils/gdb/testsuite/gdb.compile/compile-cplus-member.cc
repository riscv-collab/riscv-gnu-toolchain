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
static int get_values (const A& a);

enum myenum {E_A = 10, E_B, E_C, E_D, E_E};

namespace N {
  typedef enum {NA = 20, NB, NC, ND} ANON_NE;
}

namespace {
  typedef enum {AA = 40, AB, AC, AD} ANON_E;
}

ANON_E g_e = AC;

class A
{
public:
  typedef int ATYPE;

  A () : public_ (1), protected_ (N::NB), private_ (3) {}
  ATYPE public_;
  static const myenum s_public_;
  friend ATYPE get_values (const A&);

protected:
  N::ANON_NE protected_;
  static N::ANON_NE s_protected_;

private:
  ATYPE private_;
  static myenum s_private_;
};

const myenum A::s_public_ = E_A;
N::ANON_NE A::s_protected_ = N::NA;
myenum A::s_private_ = E_C;

static A::ATYPE
get_values (const A& a)
{
  A::ATYPE val;

  val = a.public_ + a.private_;	// 1 + 3
  if (a.protected_ == N::NB)	// + 21
    val += 21;
  if (a.s_public_ == E_A)	// +10
    val += 10;
  if (a.s_protected_ == N::NA)	// +20
    val += 20;
  if (a.s_private_ == E_C)	// +30
    val += 30;
  if (g_e == AC)		// +40
    val += 40;
  return val;			// = 125
}

typedef int A::*PMI;

int
main ()
{
  A a;
  int var = 1234;
  PMI pmi = &A::public_;

  return a.*pmi + get_values (a);		// break here
}

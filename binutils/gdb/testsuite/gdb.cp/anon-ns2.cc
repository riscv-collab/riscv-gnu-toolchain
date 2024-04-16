/* This testcase is part of GDB, the GNU debugger.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Red Hat, originally written by Keith Seitz.  */

#include <stdlib.h>

namespace
{
  void doit1 (void) { } // doit1(void)
  void doit1 (int a) { } // doit1(int)
  void doit1 (char *a) { } // doit1(char *)

  class one
  {
  public:
    one (void) { } // one::one(void)
    one (int a) { } // one::one(int)
    one (char *a) { } // one::one(char *)
    static void doit (void) { } // one::doit(void)
  };

  namespace A
  {
    void doit2 (void) { } // A::doit2(void)
    void doit2 (int a) { } // A::doit2(int)
    void doit2 (char *a) { } // A::doit2(char *)

    class two
    {
    public:
      two (void) { } // A::two::two(void)
      two (int a) { } // A::two::two(int)
      two (char *a) { } // A::two::two(char *)
      static void doit (void) { } // A::two::doit(void)
    };

    namespace
    {
      namespace
      {
	void doit3 (void) { } // A::doit3(void)
	void doit3 (int a) { } // A::doit3(int)
	void doit3 (char *a) { } // A::doit3(char *)

	class three
	{
	public:
	  three (void) { } // A::three::three(void)
	  three (int a) { } // A::three::three(int)
	  three (char *a) { } // A::three::three(char *)
	  static void doit (void) { } // A::three::doit(void)
	};
      }
    }
  }
}

void
doit (void)
{
  one a, b (3), c (static_cast<char *> (NULL));
  one::doit ();
  A::two d, e (3), f (static_cast<char *> (NULL));
  A::two::doit ();
  A::three g, h (3), i (static_cast<char *> (NULL));
  A::three::doit ();
  doit1 ();
  doit1 (3);
  doit1 (static_cast<char *> (NULL));
  A::doit2 ();
  A::doit2 (3);
  A::doit2 (static_cast<char *> (NULL));
  A::doit3 ();
  A::doit3 (3);
  A::doit3 (static_cast<char *> (NULL));
}

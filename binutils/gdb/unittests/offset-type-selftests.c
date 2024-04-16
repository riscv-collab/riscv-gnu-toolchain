/* Self tests for offset types for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "defs.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/offset-type.h"
#include "gdbsupport/underlying.h"
#include "gdbsupport/valid-expr.h"

namespace selftests {
namespace offset_type {

DEFINE_OFFSET_TYPE (off_A, unsigned int);
DEFINE_OFFSET_TYPE (off_B, unsigned int);

/* First, compile-time tests that:

   - make sure that incorrect operations with mismatching types are
     caught at compile time.

   - make sure that the same operations but involving the right types
     do compile and that they return the correct type.
*/

#define CHECK_VALID(VALID, EXPR_TYPE, EXPR)		\
  CHECK_VALID_EXPR_2 (off_A, off_B, VALID, EXPR_TYPE, EXPR)

off_A lval_a {};
off_B lval_b {};

using undrl = std::underlying_type<off_A>::type;

/* Offset +/- underlying.  */

CHECK_VALID (true,  off_A,  off_A {} + undrl {});
CHECK_VALID (true,  off_A,  off_A {} - undrl {});
CHECK_VALID (true,  off_A,  undrl {} + off_A {});
CHECK_VALID (true,  off_A,  undrl {} - off_A {});

/* Add offset types.  Both same and different.  */

CHECK_VALID (false, void,   off_A {} + off_A {});
CHECK_VALID (false, void,   off_A {} + off_B {});

/* Subtract offset types.  Both same and different.  */

CHECK_VALID (false, void,   off_B {} - off_A {});
CHECK_VALID (true,  undrl,  off_A {} - off_A {});

/* Add/assign offset types.  Both same and different.  */

CHECK_VALID (false, void,   lval_a += off_A {});
CHECK_VALID (false, void,   lval_a += off_B {});
CHECK_VALID (false, void,   lval_a -= off_A {});
CHECK_VALID (false, void,   lval_a -= off_B {});

/* operator OP+= (offset, underlying), lvalue ref on the lhs. */

CHECK_VALID (true,  off_A&, lval_a += undrl {});
CHECK_VALID (true,  off_A&, lval_a -= undrl {});

/* operator OP+= (offset, underlying), rvalue ref on the lhs. */

CHECK_VALID (false, void,   off_A {} += undrl {});
CHECK_VALID (false, void,   off_A {} -= undrl {});

/* Rel ops, with same type.  */

CHECK_VALID (true,  bool,   off_A {} < off_A {});
CHECK_VALID (true,  bool,   off_A {} > off_A {});
CHECK_VALID (true,  bool,   off_A {} <= off_A {});
CHECK_VALID (true,  bool,   off_A {} >= off_A {});

/* Rel ops, with unrelated offset types.  */

CHECK_VALID (false, void,   off_A {} < off_B {});
CHECK_VALID (false, void,   off_A {} > off_B {});
CHECK_VALID (false, void,   off_A {} <= off_B {});
CHECK_VALID (false, void,   off_A {} >= off_B {});

/* Rel ops, with unrelated types.  */

CHECK_VALID (false, void,   off_A {} < undrl {});
CHECK_VALID (false, void,   off_A {} > undrl {});
CHECK_VALID (false, void,   off_A {} <= undrl {});
CHECK_VALID (false, void,   off_A {} >= undrl {});

static void
run_tests ()
{
  /* Test op+ and op-.  */
  {
    constexpr off_A a {};
    static_assert (to_underlying (a) == 0, "");

    {
      constexpr off_A res1 = a + 2;
      static_assert (to_underlying (res1) == 2, "");

      constexpr off_A res2 = res1 - 1;
      static_assert (to_underlying (res2) == 1, "");
    }

    {
      constexpr off_A res1 = 2 + a;
      static_assert (to_underlying (res1) == 2, "");

      constexpr off_A res2 = 3 - res1;
      static_assert (to_underlying (res2) == 1, "");
    }
  }

  /* Test op+= and op-=.  */
  {
    off_A o {};

    o += 10;
    SELF_CHECK (to_underlying (o) == 10);
    o -= 5;
    SELF_CHECK (to_underlying (o) == 5);
  }

  /* Test op-.  */
  {
    constexpr off_A o1 = (off_A) 10;
    constexpr off_A o2 = (off_A) 20;

    constexpr unsigned int delta = o2 - o1;

    static_assert (delta == 10, "");
  }

  /* Test <, <=, >, >=.  */
  {
    constexpr off_A o1 = (off_A) 10;
    constexpr off_A o2 = (off_A) 20;

    static_assert (o1 < o2, "");
    static_assert (!(o2 < o1), "");

    static_assert (o2 > o1, "");
    static_assert (!(o1 > o2), "");

    static_assert (o1 <= o2, "");
    static_assert (!(o2 <= o1), "");

    static_assert (o2 >= o1, "");
    static_assert (!(o1 >= o2), "");

    static_assert (o1 <= o1, "");
    static_assert (o1 >= o1, "");
  }
}

} /* namespace offset_type */
} /* namespace selftests */

void _initialize_offset_type_selftests ();
void
_initialize_offset_type_selftests ()
{
  selftests::register_test ("offset_type", selftests::offset_type::run_tests);
}

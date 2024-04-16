/* Self tests for enum-flags for GDB, the GNU debugger.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/enum-flags.h"
#include "gdbsupport/valid-expr.h"
#include "gdbsupport/selftest.h"

namespace selftests {
namespace enum_flags_tests {

/* The (real) enum types used in CHECK_VALID.  Their names match the
   template parameter names of the templates defined by CHECK_VALID to
   make it simpler to use.  They could be named differently.  */

/* A "real enum".  */
enum RE
  {
    RE_FLAG1 = 1 << 1,
    RE_FLAG2 = 1 << 2,
  };

/* Another "real enum".  */
enum RE2
  {
    RE2_FLAG1 = 1 << 1,
    RE2_FLAG2 = 1 << 2,
  };

/* An unsigned "real enum".  */
enum URE : unsigned
  {
    URE_FLAG1 = 1 << 1,
    URE_FLAG2 = 1 << 2,
    URE_FLAG3 = 0xffffffff,
  };

/* A non-flags enum.  */
enum NF
  {
    NF_FLAG1 = 1 << 1,
    NF_FLAG2 = 1 << 2,
  };

/* The corresponding "enum flags" types.  */
DEF_ENUM_FLAGS_TYPE (RE, EF);
DEF_ENUM_FLAGS_TYPE (RE2, EF2);
DEF_ENUM_FLAGS_TYPE (URE, UEF);

#if HAVE_IS_TRIVIALLY_COPYABLE

/* So that std::vectors of types that have enum_flags fields can
   reallocate efficiently memcpy.  */
static_assert (std::is_trivially_copyable<EF>::value);

#endif

/* A couple globals used as lvalues in the CHECK_VALID expressions
   below.  Their names (and types) match the uppercase type names
   exposed by CHECK_VALID just to make the expressions easier to
   follow.  */
static RE re ATTRIBUTE_UNUSED;
static EF ef ATTRIBUTE_UNUSED;

/* First, compile-time tests that:

   - make sure that incorrect operations with mismatching enum types
     are caught at compile time.

   - make sure that the same operations but involving the right enum
     types do compile and that they return the correct type.
*/

#define CHECK_VALID(VALID, EXPR_TYPE, EXPR)		\
  CHECK_VALID_EXPR_6 (EF, RE, EF2, RE2, UEF, URE, VALID, EXPR_TYPE, EXPR)

typedef std::underlying_type<RE>::type und;

/* Test construction / conversion from/to different types.  */

/* RE/EF -> underlying (explicit) */
CHECK_VALID (true,  und,  und (RE ()))
CHECK_VALID (true,  und,  und (EF ()))

/* RE/EF -> int (explicit) */
CHECK_VALID (true,  int,  int (RE ()))
CHECK_VALID (true,  int,  int (EF ()))

/* other -> RE */

/* You can construct a raw enum value from an int explicitly to punch
   a hole in the type system if need to.  */
CHECK_VALID (true,  RE,   RE (1))
CHECK_VALID (true,  RE,   RE (RE2 ()))
CHECK_VALID (false, void, RE (EF2 ()))
CHECK_VALID (true,  RE,   RE (RE ()))
CHECK_VALID (false, void, RE (EF ()))

/* other -> EF.  */

/* As expected, enum-flags is a stronger type than the backing raw
   enum.  Unlike with raw enums, you can't construct an enum flags
   from an integer nor from an unrelated enum type explicitly.  Add an
   intermediate conversion via the raw enum if you really need it.  */
CHECK_VALID (false, void, EF (1))
CHECK_VALID (false, void, EF (1u))
CHECK_VALID (false, void, EF (RE2 ()))
CHECK_VALID (false, void, EF (EF2 ()))
CHECK_VALID (true,  EF,   EF (RE ()))
CHECK_VALID (true,  EF,   EF (EF ()))

/* Test operators.  */

/* operator OP (raw_enum, int) */

CHECK_VALID (false, void, RE () | 1)
CHECK_VALID (false, void, RE () & 1)
CHECK_VALID (false, void, RE () ^ 1)

/* operator OP (int, raw_enum) */

CHECK_VALID (false, void, 1 | RE ())
CHECK_VALID (false, void, 1 & RE ())
CHECK_VALID (false, void, 1 ^ RE ())

/* operator OP (enum_flags, int) */

CHECK_VALID (false, void, EF () | 1)
CHECK_VALID (false, void, EF () & 1)
CHECK_VALID (false, void, EF () ^ 1)

/* operator OP (int, enum_flags) */

CHECK_VALID (false, void, 1 | EF ())
CHECK_VALID (false, void, 1 & EF ())
CHECK_VALID (false, void, 1 ^ EF ())

/* operator OP (raw_enum, raw_enum) */

CHECK_VALID (false, void, RE () | RE2 ())
CHECK_VALID (false, void, RE () & RE2 ())
CHECK_VALID (false, void, RE () ^ RE2 ())
CHECK_VALID (true,  RE,   RE () | RE ())
CHECK_VALID (true,  RE,   RE () & RE ())
CHECK_VALID (true,  RE,   RE () ^ RE ())

/* operator OP (enum_flags, raw_enum) */

CHECK_VALID (false, void, EF () | RE2 ())
CHECK_VALID (false, void, EF () & RE2 ())
CHECK_VALID (false, void, EF () ^ RE2 ())
CHECK_VALID (true,  EF,   EF () | RE ())
CHECK_VALID (true,  EF,   EF () & RE ())
CHECK_VALID (true,  EF,   EF () ^ RE ())

/* operator OP= (raw_enum, raw_enum), rvalue ref on the lhs. */

CHECK_VALID (false, void, RE () |= RE2 ())
CHECK_VALID (false, void, RE () &= RE2 ())
CHECK_VALID (false, void, RE () ^= RE2 ())
CHECK_VALID (false, void, RE () |= RE ())
CHECK_VALID (false, void, RE () &= RE ())
CHECK_VALID (false, void, RE () ^= RE ())

/* operator OP= (raw_enum, raw_enum), lvalue ref on the lhs. */

CHECK_VALID (false, void, re |= RE2 ())
CHECK_VALID (false, void, re &= RE2 ())
CHECK_VALID (false, void, re ^= RE2 ())
CHECK_VALID (true,  RE&,  re |= RE ())
CHECK_VALID (true,  RE&,  re &= RE ())
CHECK_VALID (true,  RE&,  re ^= RE ())

/* operator OP= (enum_flags, raw_enum), rvalue ref on the lhs.  */

CHECK_VALID (false, void, EF () |= RE2 ())
CHECK_VALID (false, void, EF () &= RE2 ())
CHECK_VALID (false, void, EF () ^= RE2 ())
CHECK_VALID (false, void, EF () |= RE ())
CHECK_VALID (false, void, EF () &= RE ())
CHECK_VALID (false, void, EF () ^= RE ())

/* operator OP= (enum_flags, raw_enum), lvalue ref on the lhs.  */

CHECK_VALID (false, void, ef |= RE2 ())
CHECK_VALID (false, void, ef &= RE2 ())
CHECK_VALID (false, void, ef ^= RE2 ())
CHECK_VALID (true,  EF&,  ef |= EF ())
CHECK_VALID (true,  EF&,  ef &= EF ())
CHECK_VALID (true,  EF&,  ef ^= EF ())

/* operator OP= (enum_flags, enum_flags), rvalue ref on the lhs.  */

CHECK_VALID (false, void, EF () |= EF2 ())
CHECK_VALID (false, void, EF () &= EF2 ())
CHECK_VALID (false, void, EF () ^= EF2 ())
CHECK_VALID (false, void, EF () |= EF ())
CHECK_VALID (false, void, EF () &= EF ())
CHECK_VALID (false, void, EF () ^= EF ())

/* operator OP= (enum_flags, enum_flags), lvalue ref on the lhs.  */

CHECK_VALID (false, void, ef |= EF2 ())
CHECK_VALID (false, void, ef &= EF2 ())
CHECK_VALID (false, void, ef ^= EF2 ())
CHECK_VALID (true,  EF&,  ef |= EF ())
CHECK_VALID (true,  EF&,  ef &= EF ())
CHECK_VALID (true,  EF&,  ef ^= EF ())

/* operator~ (raw_enum) */

CHECK_VALID (false,  void,   ~RE ())
CHECK_VALID (true,   URE,    ~URE ())

/* operator~ (enum_flags) */

CHECK_VALID (false,  void,   ~EF ())
CHECK_VALID (true,   UEF,    ~UEF ())

/* Check ternary operator.  This exercises implicit conversions.  */

CHECK_VALID (true,  EF,   true ? EF () : RE ())
CHECK_VALID (true,  EF,   true ? RE () : EF ())

/* These are valid, but it's not a big deal since you won't be able to
   assign the resulting integer to an enum or an enum_flags without a
   cast.

   The latter two tests are disabled on older GCCs because they
   incorrectly fail with gcc 4.8 and 4.9 at least.  Running the test
   outside a SFINAE context shows:

    invalid user-defined conversion from ‘EF’ to ‘RE2’

   They've been confirmed to compile/pass with gcc 5.3, gcc 7.1 and
   clang 3.7.  */

CHECK_VALID (true,  int,  true ? EF () : EF2 ())
CHECK_VALID (true,  int,  true ? EF2 () : EF ())
#if GCC_VERSION >= 5003 || defined __clang__
CHECK_VALID (true,  int,  true ? EF () : RE2 ())
CHECK_VALID (true,  int,  true ? RE2 () : EF ())
#endif

/* Same, but with an unsigned enum.  */

typedef unsigned int uns;

CHECK_VALID (true,  uns,  true ? EF () : UEF ())
CHECK_VALID (true,  uns,  true ? UEF () : EF ())
#if GCC_VERSION >= 5003 || defined __clang__
CHECK_VALID (true,  uns,  true ? EF () : URE ())
CHECK_VALID (true,  uns,  true ? URE () : EF ())
#endif

/* Unfortunately this can't work due to the way C++ computes the
   return type of the ternary conditional operator.  int isn't
   implicitly convertible to the raw enum type, so the type of the
   expression is int.  And then int is not implicitly convertible to
   enum_flags.

   GCC 4.8 fails to compile this test with:
     error: operands to ?: have different types ‘enum_flags<RE>’ and ‘int’
   Confirmed to work with gcc 4.9, 5.3 and clang 3.7.
*/
#if GCC_VERSION >= 4009 || defined __clang__
CHECK_VALID (false, void, true ? EF () : 0)
CHECK_VALID (false, void, true ? 0 : EF ())
#endif

/* Check that the ++/--/<</>>/<<=/>>= operators are deleted.  */

CHECK_VALID (false, void, RE ()++)
CHECK_VALID (false, void, ++RE ())
CHECK_VALID (false, void, --RE ())
CHECK_VALID (false, void, RE ()--)

CHECK_VALID (false, void, RE () << 1)
CHECK_VALID (false, void, RE () >> 1)
CHECK_VALID (false, void, EF () << 1)
CHECK_VALID (false, void, EF () >> 1)

CHECK_VALID (false, void, RE () <<= 1)
CHECK_VALID (false, void, RE () >>= 1)
CHECK_VALID (false, void, EF () <<= 1)
CHECK_VALID (false, void, EF () >>= 1)

/* Test comparison operators.  */

CHECK_VALID (false, void, EF () == EF2 ())
CHECK_VALID (false, void, EF () == RE2 ())
CHECK_VALID (false, void, RE () == EF2 ())

CHECK_VALID (true,  bool, EF (RE (1)) == EF (RE (1)))
CHECK_VALID (true,  bool, EF (RE (1)) == RE (1))
CHECK_VALID (true,  bool, RE (1)      == EF (RE (1)))

CHECK_VALID (false, void, EF () != EF2 ())
CHECK_VALID (false, void, EF () != RE2 ())
CHECK_VALID (false, void, RE () != EF2 ())

/* Disable -Wenum-compare due to:

   Clang:

    "error: comparison of two values with different enumeration types
    [-Werror,-Wenum-compare]"

   GCC:

    "error: comparison between ‘enum selftests::enum_flags_tests::RE’
     and ‘enum selftests::enum_flags_tests::RE2’
     [-Werror=enum-compare]"

   Not a big deal since misuses like these in GDB will be caught by
   -Werror anyway.  This check is here mainly for completeness.  */
#if defined __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wenum-compare"
#endif
CHECK_VALID (true,  bool, RE () == RE2 ())
CHECK_VALID (true,  bool, RE () != RE2 ())
#if defined __GNUC__
# pragma GCC diagnostic pop
#endif

CHECK_VALID (true,  bool, EF (RE (1)) != EF (RE (2)))
CHECK_VALID (true,  bool, EF (RE (1)) != RE (2))
CHECK_VALID (true,  bool, RE (1)      != EF (RE (2)))

CHECK_VALID (true,  bool, EF () == 0)

/* Check we didn't disable/delete comparison between non-flags enums
   and unrelated types by mistake.  */
CHECK_VALID (true,  bool, NF (1) == NF (1))
CHECK_VALID (true,  bool, NF (1) == int (1))
CHECK_VALID (true,  bool, NF (1) == char (1))

/* -------------------------------------------------------------------- */

/* Follows misc tests that exercise the API.  Some are compile time,
   when possible, others are run time.  */

enum test_flag
  {
    FLAG1 = 1 << 0,
    FLAG2 = 1 << 1,
    FLAG3 = 1 << 2,
    FLAG4 = 1 << 3,
  };

enum test_uflag : unsigned
  {
    UFLAG1 = 1 << 0,
    UFLAG2 = 1 << 1,
    UFLAG3 = 1 << 2,
    UFLAG4 = 1 << 3,
  };

DEF_ENUM_FLAGS_TYPE (test_flag, test_flags);
DEF_ENUM_FLAGS_TYPE (test_uflag, test_uflags);

/* to_string enumerator->string mapping functions used to test
   enum_flags::to_string.  These intentionally miss mapping a couple
   enumerators each (xFLAG2, xFLAG4).  */

static std::string
to_string_flags (test_flags flags)
{
  static constexpr test_flags::string_mapping mapping[] = {
    MAP_ENUM_FLAG (FLAG1),
    MAP_ENUM_FLAG (FLAG3),
  };
  return flags.to_string (mapping);
}

static std::string
to_string_uflags (test_uflags flags)
{
  static constexpr test_uflags::string_mapping mapping[] = {
    MAP_ENUM_FLAG (UFLAG1),
    MAP_ENUM_FLAG (UFLAG3),
  };
  return flags.to_string (mapping);
}

static void
self_test ()
{
  /* Check that default construction works.  */
  {
    constexpr test_flags f;

    static_assert (f == 0);
  }

  /* Check that assignment from zero works.  */
  {
    test_flags f (FLAG1);

    SELF_CHECK (f == FLAG1);

    f = 0;

    SELF_CHECK (f == 0);
  }

  /* Check that construction from zero works.  */
  {
    constexpr test_flags zero1 = 0;
    constexpr test_flags zero2 (0);
    constexpr test_flags zero3 {0};
    constexpr test_flags zero4 = {0};

    static_assert (zero1 == 0);
    static_assert (zero2 == 0);
    static_assert (zero3 == 0);
    static_assert (zero4 == 0);
  }

  /* Check construction from enum value.  */
  {
    static_assert (test_flags (FLAG1) == FLAG1);
    static_assert (test_flags (FLAG2) != FLAG1);
  }

  /* Check copy/assignment.  */
  {
    constexpr test_flags src = FLAG1;

    constexpr test_flags f1 = src;
    constexpr test_flags f2 (src);
    constexpr test_flags f3 {src};
    constexpr test_flags f4 = {src};

    static_assert (f1 == FLAG1);
    static_assert (f2 == FLAG1);
    static_assert (f3 == FLAG1);
    static_assert (f4 == FLAG1);
  }

  /* Check moving.  */
  {
    test_flags src = FLAG1;
    test_flags dst = 0;

    dst = std::move (src);
    SELF_CHECK (dst == FLAG1);
  }

  /* Check construction from an 'or' of multiple bits.  For this to
     work, operator| must be overridden to return an enum type.  The
     builtin version would return int instead and then the conversion
     to test_flags would fail.  */
  {
    constexpr test_flags f = FLAG1 | FLAG2;
    static_assert (f == (FLAG1 | FLAG2));
  }

  /* Similarly, check that "FLAG1 | FLAG2" on the rhs of an assignment
     operator works.  */
  {
    test_flags f = 0;
    f |= FLAG1 | FLAG2;
    SELF_CHECK (f == (FLAG1 | FLAG2));

    f &= FLAG1 | FLAG2;
    SELF_CHECK (f == (FLAG1 | FLAG2));

    f ^= FLAG1 | FLAG2;
    SELF_CHECK (f == 0);
  }

  /* Check explicit conversion to int works.  */
  {
    constexpr int some_bits (FLAG1 | FLAG2);

    /* And comparison with int works too.  */
    static_assert (some_bits == (FLAG1 | FLAG2));
    static_assert (some_bits == test_flags (FLAG1 | FLAG2));
  }

  /* Check operator| and operator|=.  Particularly interesting is
     making sure that putting the enum value on the lhs side of the
     expression works (FLAG | f).  */
  {
    test_flags f = FLAG1;
    f |= FLAG2;
    SELF_CHECK (f == (FLAG1 | FLAG2));
  }
  {
    test_flags f = FLAG1;
    f = f | FLAG2;
    SELF_CHECK (f == (FLAG1 | FLAG2));
  }
  {
    test_flags f = FLAG1;
    f = FLAG2 | f;
    SELF_CHECK (f == (FLAG1 | FLAG2));
  }

  /* Check the &/&= operators.  */
  {
    test_flags f = FLAG1 & FLAG2;
    SELF_CHECK (f == 0);

    f = FLAG1 | FLAG2;
    f &= FLAG2;
    SELF_CHECK (f == FLAG2);

    f = FLAG1 | FLAG2;
    f = f & FLAG2;
    SELF_CHECK (f == FLAG2);

    f = FLAG1 | FLAG2;
    f = FLAG2 & f;
    SELF_CHECK (f == FLAG2);
  }

  /* Check the ^/^= operators.  */
  {
    constexpr test_flags f = FLAG1 ^ FLAG2;
    static_assert (f == (FLAG1 ^ FLAG2));
  }

  {
    test_flags f = FLAG1 ^ FLAG2;
    f ^= FLAG3;
    SELF_CHECK (f == (FLAG1 | FLAG2 | FLAG3));
    f = f ^ FLAG3;
    SELF_CHECK (f == (FLAG1 | FLAG2));
    f = FLAG3 ^ f;
    SELF_CHECK (f == (FLAG1 | FLAG2 | FLAG3));
  }

  /* Check operator~.  Note this only compiles with unsigned
     flags.  */
  {
    constexpr test_uflags f1 = ~UFLAG1;
    constexpr test_uflags f2 = ~f1;
    static_assert (f2 == UFLAG1);
  }

  /* Check the ternary operator.  */

  {
    /* raw enum, raw enum */
    constexpr test_flags f1 = true ? FLAG1 : FLAG2;
    static_assert (f1 == FLAG1);
    constexpr test_flags f2 = false ? FLAG1 : FLAG2;
    static_assert (f2 == FLAG2);
  }

  {
    /* enum flags, raw enum */
    constexpr test_flags src = FLAG1;
    constexpr test_flags f1 = true ? src : FLAG2;
    static_assert (f1 == FLAG1);
    constexpr test_flags f2 = false ? src : FLAG2;
    static_assert (f2 == FLAG2);
  }

  {
    /* enum flags, enum flags */
    constexpr test_flags src1 = FLAG1;
    constexpr test_flags src2 = FLAG2;
    constexpr test_flags f1 = true ? src1 : src2;
    static_assert (f1 == src1);
    constexpr test_flags f2 = false ? src1 : src2;
    static_assert (f2 == src2);
  }

  /* Check that we can use flags in switch expressions (requires
     unambiguous conversion to integer).  Also check that we can use
     operator| in switch cases, where only constants are allowed.
     This should work because operator| is constexpr.  */
  {
    test_flags f = FLAG1 | FLAG2;
    bool ok = false;

    switch (f)
      {
      case FLAG1:
	break;
      case FLAG2:
	break;
      case FLAG1 | FLAG2:
	ok = true;
	break;
      }

    SELF_CHECK (ok);
  }

  /* Check string conversion.  */
  {
    SELF_CHECK (to_string_uflags (0)
		== "0x0 []");
    SELF_CHECK (to_string_uflags (UFLAG1)
		== "0x1 [UFLAG1]");
    SELF_CHECK (to_string_uflags (UFLAG1 | UFLAG3)
		== "0x5 [UFLAG1 UFLAG3]");
    SELF_CHECK (to_string_uflags (UFLAG1 | UFLAG2 | UFLAG3)
		== "0x7 [UFLAG1 UFLAG3 0x2]");
    SELF_CHECK (to_string_uflags (UFLAG2)
		== "0x2 [0x2]");
    /* Check that even with multiple unmapped flags, we only print one
       unmapped hex number (0xa, in this case).  */
    SELF_CHECK (to_string_uflags (UFLAG1 | UFLAG2 | UFLAG3 | UFLAG4)
		== "0xf [UFLAG1 UFLAG3 0xa]");

    SELF_CHECK (to_string_flags (0)
		== "0x0 []");
    SELF_CHECK (to_string_flags (FLAG1)
		== "0x1 [FLAG1]");
    SELF_CHECK (to_string_flags (FLAG1 | FLAG3)
		== "0x5 [FLAG1 FLAG3]");
    SELF_CHECK (to_string_flags (FLAG1 | FLAG2 | FLAG3)
		== "0x7 [FLAG1 FLAG3 0x2]");
    SELF_CHECK (to_string_flags (FLAG2)
		== "0x2 [0x2]");
    SELF_CHECK (to_string_flags (FLAG1 | FLAG2 | FLAG3 | FLAG4)
		== "0xf [FLAG1 FLAG3 0xa]");
  }
}

} /* namespace enum_flags_tests */
} /* namespace selftests */

void _initialize_enum_flags_selftests ();

void
_initialize_enum_flags_selftests ()
{
  selftests::register_test ("enum-flags",
			    selftests::enum_flags_tests::self_test);
}

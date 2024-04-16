/* Compile-time valid expression checker for GDB, the GNU debugger.

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

/* Helper macros used to build compile-time unit tests that make sure
   that invalid expressions that should not compile would not compile,
   and that expressions that should compile do compile, and have the
   right type.  This is mainly used to verify that some utility's API
   is really as safe as intended.  */

#ifndef COMMON_VALID_EXPR_H
#define COMMON_VALID_EXPR_H

#include "gdbsupport/preprocessor.h"
#include "gdbsupport/traits.h"

/* Macro that uses SFINAE magic to detect whether the EXPR expression
   is either valid or ill-formed, at compile time, without actually
   producing compile-time errors.  I.e., check that bad uses of the
   types (e.g., involving mismatching types) would be caught at
   compile time.  If the expression is valid, also check whether the
   expression has the right type.

   EXPR must be defined in terms of some of the template parameters,
   so that template substitution failure discards the overload instead
   of causing a real compile error.  TYPES is thus the list of types
   involved in the expression, and TYPENAMES is the same list, but
   with each element prefixed by "typename".  These are passed as
   template parameter types to the templates within the macro.

   VALID is a boolean that indicates whether the expression is
   supposed to be valid or invalid.

   EXPR_TYPE is the expected type of EXPR.  Only meaningful iff VALID
   is true.  If VALID is false, then you must pass "void" as expected
   type.

   Each invocation of the macro is wrapped in its own namespace to
   avoid ODR violations.  The generated namespace only includes the
   line number, so client code should wrap sets of calls in a
   test-specific namespace too, to fully guarantee uniqueness between
   the multiple clients in the codebase.  */
#define CHECK_VALID_EXPR_INT(TYPENAMES, TYPES, VALID, EXPR_TYPE, EXPR)	\
  namespace CONCAT (check_valid_expr, __LINE__) {			\
									\
  template <TYPENAMES, typename = decltype (EXPR)>			\
  struct archetype							\
  {									\
  };									\
									\
  static_assert (gdb::is_detected_exact<archetype<TYPES, EXPR_TYPE>,	\
		 archetype, TYPES>::value == VALID,			\
		 "");							\
  } /* namespace */

/* A few convenience macros that support expressions involving a
   varying numbers of types.  If you need more types, feel free to add
   another variant.  */

#define CHECK_VALID_EXPR_1(T1, VALID, EXPR_TYPE, EXPR)			\
  CHECK_VALID_EXPR_INT (ESC_PARENS (typename T1),			\
			ESC_PARENS (T1),				\
			VALID, EXPR_TYPE, EXPR)

#define CHECK_VALID_EXPR_2(T1, T2, VALID, EXPR_TYPE, EXPR)		\
  CHECK_VALID_EXPR_INT (ESC_PARENS(typename T1, typename T2),		\
			ESC_PARENS (T1, T2),				\
			VALID, EXPR_TYPE, EXPR)

#define CHECK_VALID_EXPR_3(T1, T2, T3, VALID, EXPR_TYPE, EXPR)		\
  CHECK_VALID_EXPR_INT (ESC_PARENS (typename T1, typename T2, typename T3), \
			ESC_PARENS (T1, T2, T3),				\
			VALID, EXPR_TYPE, EXPR)

#define CHECK_VALID_EXPR_4(T1, T2, T3, T4, VALID, EXPR_TYPE, EXPR)	\
  CHECK_VALID_EXPR_INT (ESC_PARENS (typename T1, typename T2,		\
				    typename T3, typename T4),		\
			ESC_PARENS (T1, T2, T3, T4),			\
			VALID, EXPR_TYPE, EXPR)

#define CHECK_VALID_EXPR_5(T1, T2, T3, T4, T5, VALID, EXPR_TYPE, EXPR)	\
  CHECK_VALID_EXPR_INT (ESC_PARENS (typename T1, typename T2,		\
				    typename T3, typename T4,		\
				    typename T5),			\
			ESC_PARENS (T1, T2, T3, T4, T5),		\
			VALID, EXPR_TYPE, EXPR)

#define CHECK_VALID_EXPR_6(T1, T2, T3, T4, T5, T6,			\
			   VALID, EXPR_TYPE, EXPR)			\
  CHECK_VALID_EXPR_INT (ESC_PARENS (typename T1, typename T2,		\
				    typename T3, typename T4,		\
				    typename T5, typename T6),		\
			ESC_PARENS (T1, T2, T3, T4, T5, T6),		\
			VALID, EXPR_TYPE, EXPR)

#endif /* COMMON_VALID_EXPR_H */

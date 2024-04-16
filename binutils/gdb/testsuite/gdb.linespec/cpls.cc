/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

/* Code for the all-param-prefixes test.  */

void
param_prefixes_test_long (long)
{}

void
param_prefixes_test_intp_intr (int *, int&)
{}

/* Code for the overload test.  */

void
overload_ambiguous_test (long)
{}

void
overload_ambiguous_test (int, int)
{}

void
overload_ambiguous_test (int, long)
{}

/* Code for the overload-2 test.  */

/* Generate functions/methods all with the same name, in different
   scopes, but all with different parameters.  */

struct overload2_arg1 {};
struct overload2_arg2 {};
struct overload2_arg3 {};
struct overload2_arg4 {};
struct overload2_arg5 {};
struct overload2_arg6 {};
struct overload2_arg7 {};
struct overload2_arg8 {};
struct overload2_arg9 {};
struct overload2_arga {};

#define GEN_OVERLOAD2_FUNCTIONS(ARG1, ARG2)		\
  void __attribute__ ((used))				\
  overload2_function (ARG1)				\
  {}							\
							\
  struct struct_overload2_test				\
  {							\
    void __attribute__ ((used))				\
      overload2_function (ARG2);			\
  };							\
							\
  void __attribute__ ((used))				\
  struct_overload2_test::overload2_function (ARG2)	\
  {}

/* In the global namespace.  */
GEN_OVERLOAD2_FUNCTIONS( overload2_arg1, overload2_arg2)

namespace
{
  /* In an anonymous namespace.  */
  GEN_OVERLOAD2_FUNCTIONS (overload2_arg3, overload2_arg4)
}

namespace ns_overload2_test
{
  /* In a namespace.  */
  GEN_OVERLOAD2_FUNCTIONS (overload2_arg5, overload2_arg6)

  namespace
  {
    /* In a nested anonymous namespace.  */
    GEN_OVERLOAD2_FUNCTIONS (overload2_arg7, overload2_arg8)

    namespace ns_overload2_test
    {
      /* In a nested namespace.  */
      GEN_OVERLOAD2_FUNCTIONS (overload2_arg9, overload2_arga)
    }
  }
}

/* Code for the overload-3 test.  */

#define GEN_OVERLOAD3_FUNCTIONS(ARG1, ARG2)		\
  void __attribute__ ((used))				\
  overload3_function (ARG1)				\
  {}							\
  void __attribute__ ((used))				\
  overload3_function (ARG2)				\
  {}							\
							\
  struct struct_overload3_test				\
  {							\
    void __attribute__ ((used))				\
      overload3_function (ARG1);			\
    void __attribute__ ((used))				\
      overload3_function (ARG2);			\
  };							\
							\
  void __attribute__ ((used))				\
  struct_overload3_test::overload3_function (ARG1)	\
  {}							\
  void __attribute__ ((used))				\
  struct_overload3_test::overload3_function (ARG2)	\
  {}

/* In the global namespace.  */
GEN_OVERLOAD3_FUNCTIONS (int, long)

namespace
{
  /* In an anonymous namespace.  */
  GEN_OVERLOAD3_FUNCTIONS (int, long)
}

namespace ns_overload3_test
{
  /* In a namespace.  */
  GEN_OVERLOAD3_FUNCTIONS (int, long)

  namespace
  {
    /* In a nested anonymous namespace.  */
    GEN_OVERLOAD3_FUNCTIONS (int, long)

    namespace ns_overload3_test
    {
      /* In a nested namespace.  */
      GEN_OVERLOAD3_FUNCTIONS (int, long)
    }
  }
}

/* Code for the template-function_foo (template parameter completion) tests.  */

template <typename T>
struct template_struct
{
  T template_overload_fn (T);
};

template <typename T>
T template_struct<T>::template_overload_fn (T t)
{
  return t;
}

template_struct<int> template_struct_int;
template_struct<long> template_struct_long;

/* Code for the template-parameter-overload test.  */

template <typename T>
void foo (T c) {}

template <typename T1, typename T2>
void foo (T1 a, T2 b) {}

template <typename T>
struct a
{
  void method () {}
};

template <typename T>
struct b
{
  void method () {}
};

template <typename T>
struct c
{
  void method () {}
};

template <typename T>
struct d
{
  void method () {};
};

template <typename T1, typename T2>
struct A
{
  void method () {}
};

template <typename T1, typename T2>
struct B
{
  void method () {}
};

namespace n
{
  struct na {};
  struct nb {};

  template <typename T1, typename T2>
  struct NA {};

  template <typename T1, typename T2>
  struct NB {};
};

static void
template_function_foo ()
{
  a<a<int>> aa;
  aa.method ();
  a<b<int>> ab;
  ab.method ();
  c<c<int>> cc;
  cc.method ();
  c<d<int>> cd;
  cd.method ();
  foo (aa);
  foo (ab);
  foo (cc);
  foo (cd);
  foo (aa, ab);
  foo (aa, cc);
  foo (aa, cd);

  A<a<b<int>>, c<d<int>>> Aabcd;
  Aabcd.method ();
  foo (Aabcd);

  A<a<b<int>>, a<a<int>>> Aabaa;
  Aabaa.method ();
  foo (Aabaa);

  A<a<b<int>>, a<b<int>>> Aabab;
  Aabab.method ();
  foo (Aabab);

  B<a<b<int>>, c<d<int>>> Babcd;
  Babcd.method ();
  foo (Babcd);

  foo (Aabcd, Babcd);
  foo (Aabcd, Aabaa);
  foo (Aabcd, Aabab);

  n::na na;
  n::nb nb;
  foo (na, nb);
  a<n::na> ana;
  b<n::nb> bnb;
  foo (ana, bnb);

  n::NA<n::na, n::nb> NAnanb;
  n::NB<n::na, n::nb> Nbnanb;
  foo (NAnanb, Nbnanb);
}

/* Code for the template2-ret-type tests.  */

template <typename T>
struct template2_ret_type {};

template <typename T>
struct template2_struct
{
  template <typename T2, typename T3>
  T template2_fn (T = T (), T2 t2 = T2 (), T3 t3 = T3 ());
};

template <typename T>
template <typename T2, typename T3>
T template2_struct<T>::template2_fn (T t, T2 t2, T3 t3)
{
  return T ();
}

template2_struct<template2_ret_type<int> > template2_struct_inst;

/* Code for the const-overload tests.  */

struct struct_with_const_overload
{
  void const_overload_fn ();
  void const_overload_fn () const;
};

void
struct_with_const_overload::const_overload_fn ()
{}

void
struct_with_const_overload::const_overload_fn () const
{}

void
not_overloaded_fn ()
{}

/* Code for the incomplete-scope-colon tests.  */

struct struct_incomplete_scope_colon_test
{
  void incomplete_scope_colon_test ();
};

void
struct_incomplete_scope_colon_test::incomplete_scope_colon_test ()
{}

namespace ns_incomplete_scope_colon_test
{
  void incomplete_scope_colon_test () {}
}

namespace ns2_incomplete_scope_colon_test
{
  struct struct_in_ns2_incomplete_scope_colon_test
  {
    void incomplete_scope_colon_test ();
  };

  void
  struct_in_ns2_incomplete_scope_colon_test::incomplete_scope_colon_test ()
  {}
}

/* Code for the anon-ns tests.  */

namespace
{
  void __attribute__ ((used)) anon_ns_function ()
  {}

  struct anon_ns_struct
  {
    void __attribute__ ((used)) anon_ns_function ();
  };

  void __attribute__ ((used))
  anon_ns_struct::anon_ns_function ()
  {}
}

namespace the_anon_ns_wrapper_ns
{

namespace
{
  void __attribute__ ((used)) anon_ns_function ()
  {}

  struct anon_ns_struct
  {
    void __attribute__ ((used)) anon_ns_function ();
  };

  void __attribute__ ((used))
  anon_ns_struct::anon_ns_function ()
  {}
}

} /* the_anon_ns_wrapper_ns */

/* Code for the global-ns-scope-op tests.  */

void global_ns_scope_op_function ()
{
}

/* Add a function with the same name to a namespace.  We want to test
   that "b ::global_ns_function" does NOT select it.  */
namespace the_global_ns_scope_op_ns
{
  void global_ns_scope_op_function ()
  {
  }
}

/* Code for the ambiguous-prefix tests.  */

/* Create a few functions/methods with the same "ambiguous_prefix_"
   prefix.  They in different scopes, but "b ambiguous_prefix_<tab>"
   should list them all, and figure out the LCD is
   ambiguous_prefix_.  */

void ambiguous_prefix_global_func ()
{
}

namespace the_ambiguous_prefix_ns
{
  void ambiguous_prefix_ns_func ()
  {
  }
}

struct the_ambiguous_prefix_struct
{
  void ambiguous_prefix_method ();
};

void
the_ambiguous_prefix_struct::ambiguous_prefix_method ()
{
}

/* Code for the function-labels test.  */

int
function_with_labels (int i)
{
  if (i > 0)
    {
    label1:
      return i + 20;
    }
  else
    {
    label2:
      return i + 10;
    }
}

/* Code for the no-data-symbols and if-expression tests.  */

int code_data = 0;

int another_data = 0;

/* A function that has a same "code" prefix as the global above.  We
   want to ensure that completing on "b code" doesn't offer the data
   symbol.  */
void
code_function ()
{
}

/* Code for the operator< tests.  */

enum foo_enum
  {
    foo_value
  };

bool operator<(foo_enum lhs, foo_enum rhs)
{
 label1:
	return false;
}

/* Code for the in-source-file-unconstrained /
   in-source-file-ambiguous tests.  */

int
file_constrained_test_cpls_function (int i)
{
  if (i > 0)
    {
    label1:
      return i + 20;
    }
  else
    {
    label2:
      return i + 10;
    }
}


int
main ()
{
  template2_struct_inst.template2_fn<int, int> ();
  template_struct_int.template_overload_fn(0);
  template_struct_long.template_overload_fn(0);
  template_function_foo ();

  return 0;
}

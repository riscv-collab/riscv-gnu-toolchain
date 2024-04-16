/* Self tests for array_view for GDB, the GNU debugger.

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
#include "gdbsupport/array-view.h"
#include <array>
#include <vector>

namespace selftests {
namespace array_view_tests {

/* Triviality checks.  */
#define CHECK_TRAIT(TRAIT)			\
  static_assert (std::TRAIT<gdb::array_view<gdb_byte>>::value, "")

#if HAVE_IS_TRIVIALLY_COPYABLE

CHECK_TRAIT (is_trivially_copyable);
CHECK_TRAIT (is_trivially_move_assignable);
CHECK_TRAIT (is_trivially_move_constructible);
CHECK_TRAIT (is_trivially_destructible);

#endif

#undef CHECK_TRAIT

/* Wrapper around std::is_convertible to make the code using it a bit
   shorter.  (With C++14 we'd use a variable template instead.)  */

template<typename From, typename To>
static constexpr bool
is_convertible ()
{
  return std::is_convertible<From, To>::value;
}

/* Check for implicit conversion to immutable and mutable views.  */

static constexpr bool
check_convertible ()
{
  using T = gdb_byte;
  using gdb::array_view;

  return (true
	  /* immutable array_view */
	  &&  is_convertible<const T (&) [1],	array_view<const T>> ()
	  &&  is_convertible<T (&) [1], 	array_view<const T>> ()
	  &&  is_convertible<const T, 		array_view<const T>> ()
	  &&  is_convertible<T, 		array_view<const T>> ()

	  /* mutable array_view */
	  &&  is_convertible<T (&) [1], 	array_view<T>> ()
	  && !is_convertible<const T (&) [1],	array_view<T>> ()
	  &&  is_convertible<T, 		array_view<T>> ()
	  && !is_convertible<const T,		array_view<T>> ()

	  /* While float is implicitly convertible to gdb_byte, we
	     don't want implicit float->array_view<gdb_byte>
	     conversion.  */
	  && !is_convertible<float, 		array_view<const T>> ()
	  && !is_convertible<float, 		array_view<T>> ());
}

static_assert (check_convertible (), "");

namespace no_slicing
{
struct A { int i; };
struct B : A { int j; };
struct C : A { int l; };

/* Check that there's no array->view conversion for arrays of derived types or
   subclasses.  */
static constexpr bool
check ()
{
  using gdb::array_view;

  return (true

	  /* array->view  */

	  &&  is_convertible <A (&)[1], array_view<A>> ()
	  && !is_convertible <B (&)[1], array_view<A>> ()
	  && !is_convertible <C (&)[1], array_view<A>> ()

	  && !is_convertible <A (&)[1], array_view<B>> ()
	  &&  is_convertible <B (&)[1], array_view<B>> ()
	  && !is_convertible <C (&)[1], array_view<B>> ()

	  /* elem->view  */

	  &&  is_convertible <A, array_view<A>> ()
	  && !is_convertible <B, array_view<A>> ()
	  && !is_convertible <C, array_view<A>> ()

	  && !is_convertible <A, array_view<B>> ()
	  &&  is_convertible <B, array_view<B>> ()
	  && !is_convertible <C, array_view<B>> ());
}

/* Check that there's no container->view conversion for containers of derived
   types or subclasses.  */

template<template<typename ...> class Container>
static constexpr bool
check_ctor_from_container ()
{
  using gdb::array_view;

  return (    is_convertible <Container<A>, array_view<A>> ()
	  && !is_convertible <Container<B>, array_view<A>> ()
	  && !is_convertible <Container<C>, array_view<A>> ()

	  && !is_convertible <Container<A>, array_view<B>> ()
	  &&  is_convertible <Container<B>, array_view<B>> ()
	  && !is_convertible <Container<C>, array_view<B>> ());
}

} /* namespace no_slicing */

/* std::array with only one template argument, so we can pass it to
   check_ctor_from_container.  */
template<typename T> using StdArray1 = std::array<T, 1>;

static_assert (no_slicing::check (), "");
static_assert (no_slicing::check_ctor_from_container<std::vector> (), "");
static_assert (no_slicing::check_ctor_from_container<StdArray1> (), "");
static_assert (no_slicing::check_ctor_from_container<gdb::array_view> (), "");

/* Check that array_view implicitly converts from std::vector.  */

static constexpr bool
check_convertible_from_std_vector ()
{
  using gdb::array_view;
  using T = gdb_byte;

  /* Note there's no such thing as std::vector<const T>.  */

  return (true
	  &&  is_convertible <std::vector<T>, array_view<T>> ()
	  &&  is_convertible <std::vector<T>, array_view<const T>> ());
}

static_assert (check_convertible_from_std_vector (), "");

/* Check that array_view implicitly converts from std::array.  */

static constexpr bool
check_convertible_from_std_array ()
{
  using gdb::array_view;
  using T = gdb_byte;

  /* Note: a non-const T view can't refer to a const T array.  */

  return (true
	  &&  is_convertible <std::array<T, 1>,		array_view<T>> ()
	  &&  is_convertible <std::array<T, 1>,		array_view<const T>> ()
	  && !is_convertible <std::array<const T, 1>,	array_view<T>> ()
	  &&  is_convertible <std::array<const T, 1>,	array_view<const T>> ());
}

static_assert (check_convertible_from_std_array (), "");

/* Check that VIEW views C (a container like std::vector/std::array)
   correctly.  */

template<typename View, typename Container>
static bool
check_container_view (const View &view, const Container &c)
{
  if (view.empty ())
    return false;
  if (view.size () != c.size ())
    return false;
  if (view.data () != c.data ())
    return false;
  for (size_t i = 0; i < c.size (); i++)
    {
      if (&view[i] != &c[i])
	return false;
      if (view[i] != c[i])
	return false;
    }
  return true;
}

/* Check that VIEW views E (an object of the type of a view element)
   correctly.  */

template<typename View, typename Elem>
static bool
check_elem_view (const View &view, const Elem &e)
{
  if (view.empty ())
    return false;
  if (view.size () != 1)
    return false;
  if (view.data () != &e)
    return false;
  if (&view[0] != &e)
    return false;
  if (view[0] != e)
    return false;
  return true;
}

/* Check for operator[].  The first overload is taken iff
   'view<T>()[0] = T()' is a valid expression.  */

template<typename View,
	 typename = decltype (std::declval<View> ()[0]
			      = std::declval<typename View::value_type> ())>
static bool
check_op_subscript (const View &view)
{
  return true;
}

/* This overload is taken iff 'view<T>()[0] = T()' is not a valid
   expression.  */

static bool
check_op_subscript (...)
{
  return false;
}

/* Check construction with pointer + size.  This is a template in
   order to test both gdb_byte and const gdb_byte.  */

template<typename T>
static void
check_ptr_size_ctor ()
{
  T data[] = {0x11, 0x22, 0x33, 0x44};

  gdb::array_view<T> view (data + 1, 2);

  SELF_CHECK (!view.empty ());
  SELF_CHECK (view.size () == 2);
  SELF_CHECK (view.data () == &data[1]);
  SELF_CHECK (view[0] == data[1]);
  SELF_CHECK (view[1] == data[2]);

  gdb::array_view<const T> cview (data + 1, 2);
  SELF_CHECK (!cview.empty ());
  SELF_CHECK (cview.size () == 2);
  SELF_CHECK (cview.data () == &data[1]);
  SELF_CHECK (cview[0] == data[1]);
  SELF_CHECK (cview[1] == data[2]);
}

/* Asserts std::is_constructible.  */

template<typename T, typename... Args>
static constexpr bool
require_not_constructible ()
{
  static_assert (!std::is_constructible<T, Args...>::value, "");

  /* constexpr functions can't return void in C++11 (N3444).  */
  return true;
};

/* Check the array_view<T>(PTR, SIZE) ctor, when T is a pointer.  */

static void
check_ptr_size_ctor2 ()
{
  struct A {};
  A an_a;

  A *array[] = { &an_a };
  const A * const carray[] = { &an_a };

  gdb::array_view<A *> v1 = {array, ARRAY_SIZE (array)};
  gdb::array_view<A *> v2 = {array, (char) ARRAY_SIZE (array)};
  gdb::array_view<A * const> v3 = {array, ARRAY_SIZE (array)};
  gdb::array_view<const A * const> cv1 = {carray, ARRAY_SIZE (carray)};

  require_not_constructible<gdb::array_view<A *>, decltype (carray), size_t> ();

  SELF_CHECK (v1[0] == array[0]);
  SELF_CHECK (v2[0] == array[0]);
  SELF_CHECK (v3[0] == array[0]);

  SELF_CHECK (!v1.empty ());
  SELF_CHECK (v1.size () == 1);
  SELF_CHECK (v1.data () == &array[0]);

  SELF_CHECK (cv1[0] == carray[0]);

  SELF_CHECK (!cv1.empty ());
  SELF_CHECK (cv1.size () == 1);
  SELF_CHECK (cv1.data () == &carray[0]);
}

/* Check construction with a pair of pointers.  This is a template in
   order to test both gdb_byte and const gdb_byte.  */

template<typename T>
static void
check_ptr_ptr_ctor ()
{
  T data[] = {0x11, 0x22, 0x33, 0x44};

  gdb::array_view<T> view (data + 1, data + 3);

  SELF_CHECK (!view.empty ());
  SELF_CHECK (view.size () == 2);
  SELF_CHECK (view.data () == &data[1]);
  SELF_CHECK (view[0] == data[1]);
  SELF_CHECK (view[1] == data[2]);

  gdb_byte array[] = {0x11, 0x22, 0x33, 0x44};
  const gdb_byte *p1 = array;
  gdb_byte *p2 = array + ARRAY_SIZE (array);
  gdb::array_view<const gdb_byte> view2 (p1, p2);
}

/* Check construction with a pair of pointers of mixed constness.  */

static void
check_ptr_ptr_mixed_cv ()
{
  gdb_byte array[] = {0x11, 0x22, 0x33, 0x44};
  const gdb_byte *cp = array;
  gdb_byte *p = array;
  gdb::array_view<const gdb_byte> view1 (cp, p);
  gdb::array_view<const gdb_byte> view2 (p, cp);
  SELF_CHECK (view1.empty ());
  SELF_CHECK (view2.empty ());
}

/* Check range-for support (i.e., begin()/end()).  This is a template
   in order to test both gdb_byte and const gdb_byte.  */

template<typename T>
static void
check_range_for ()
{
  T data[] = {1, 2, 3, 4};
  gdb::array_view<T> view (data);

  typename std::decay<T>::type sum = 0;
  for (auto &elem : view)
    sum += elem;
  SELF_CHECK (sum == 1 + 2 + 3 + 4);
}

/* Entry point.  */

static void
run_tests ()
{
  /* Empty views.  */
  {
    constexpr gdb::array_view<gdb_byte> view1;
    constexpr gdb::array_view<const gdb_byte> view2;

    static_assert (view1.empty (), "");
    static_assert (view1.data () == nullptr, "");
    static_assert (view1.size () == 0, "");
    static_assert (view2.empty (), "");
    static_assert (view2.size () == 0, "");
    static_assert (view2.data () == nullptr, "");
  }

  std::vector<gdb_byte> vec = {0x11, 0x22, 0x33, 0x44 };
  std::array<gdb_byte, 4> array = {{0x11, 0x22, 0x33, 0x44}};

  /* Various tests of views over std::vector.  */
  {
    gdb::array_view<gdb_byte> view = vec;
    SELF_CHECK (check_container_view (view, vec));
    gdb::array_view<const gdb_byte> cview = vec;
    SELF_CHECK (check_container_view (cview, vec));
  }

  /* Likewise, over std::array.  */
  {
    gdb::array_view<gdb_byte> view = array;
    SELF_CHECK (check_container_view (view, array));
    gdb::array_view<gdb_byte> cview = array;
    SELF_CHECK (check_container_view (cview, array));
  }

  /* op=(std::vector/std::array/elem) */
  {
    gdb::array_view<gdb_byte> view;

    view = vec;
    SELF_CHECK (check_container_view (view, vec));
    view = std::move (vec);
    SELF_CHECK (check_container_view (view, vec));

    view = array;
    SELF_CHECK (check_container_view (view, array));
    view = std::move (array);
    SELF_CHECK (check_container_view (view, array));

    gdb_byte elem = 0;
    view = elem;
    SELF_CHECK (check_elem_view (view, elem));
    view = std::move (elem);
    SELF_CHECK (check_elem_view (view, elem));
  }

  /* Test copy/move ctor and mutable->immutable conversion.  */
  {
    gdb_byte data[] = {0x11, 0x22, 0x33, 0x44};
    gdb::array_view<gdb_byte> view1 = data;
    gdb::array_view<gdb_byte> view2 = view1;
    gdb::array_view<gdb_byte> view3 = std::move (view1);
    gdb::array_view<const gdb_byte> cview1 = data;
    gdb::array_view<const gdb_byte> cview2 = cview1;
    gdb::array_view<const gdb_byte> cview3 = std::move (cview1);
    SELF_CHECK (view1[0] == data[0]);
    SELF_CHECK (view2[0] == data[0]);
    SELF_CHECK (view3[0] == data[0]);
    SELF_CHECK (cview1[0] == data[0]);
    SELF_CHECK (cview2[0] == data[0]);
    SELF_CHECK (cview3[0] == data[0]);
  }

  /* Same, but op=(view).  */
  {
    gdb_byte data[] = {0x55, 0x66, 0x77, 0x88};
    gdb::array_view<gdb_byte> view1;
    gdb::array_view<gdb_byte> view2;
    gdb::array_view<gdb_byte> view3;
    gdb::array_view<const gdb_byte> cview1;
    gdb::array_view<const gdb_byte> cview2;
    gdb::array_view<const gdb_byte> cview3;

    view1 = data;
    view2 = view1;
    view3 = std::move (view1);
    cview1 = data;
    cview2 = cview1;
    cview3 = std::move (cview1);
    SELF_CHECK (view1[0] == data[0]);
    SELF_CHECK (view2[0] == data[0]);
    SELF_CHECK (view3[0] == data[0]);
    SELF_CHECK (cview1[0] == data[0]);
    SELF_CHECK (cview2[0] == data[0]);
    SELF_CHECK (cview3[0] == data[0]);
  }

  /* op[] */
  {
    std::vector<gdb_byte> vec2 = {0x11, 0x22};
    gdb::array_view<gdb_byte> view = vec2;
    gdb::array_view<const gdb_byte> cview = vec2;

    /* Check that op[] on a non-const view of non-const T returns a
       mutable reference.  */
    view[0] = 0x33;
    SELF_CHECK (vec2[0] == 0x33);

    /* OTOH, check that assigning through op[] on a view of const T
       wouldn't compile.  */
    SELF_CHECK (!check_op_subscript (cview));
    /* For completeness.  */
    SELF_CHECK (check_op_subscript (view));
  }

  check_ptr_size_ctor<const gdb_byte> ();
  check_ptr_size_ctor<gdb_byte> ();
  check_ptr_size_ctor2 ();
  check_ptr_ptr_ctor<const gdb_byte> ();
  check_ptr_ptr_ctor<gdb_byte> ();
  check_ptr_ptr_mixed_cv ();

  check_range_for<gdb_byte> ();
  check_range_for<const gdb_byte> ();

  /* Check that the right ctor overloads are taken when the element is
     a container.  */
  {
    using Vec = std::vector<gdb_byte>;
    Vec vecs[3];

    gdb::array_view<Vec> view_array = vecs;
    SELF_CHECK (view_array.size () == 3);

    Vec elem;
    gdb::array_view<Vec> view_elem = elem;
    SELF_CHECK (view_elem.size () == 1);
  }

  /* gdb::make_array_view, int length.  */
  {
    gdb_byte data[] = {0x55, 0x66, 0x77, 0x88};
    int len = sizeof (data) / sizeof (data[0]);
    auto view = gdb::make_array_view (data, len);

    SELF_CHECK (view.data () == data);
    SELF_CHECK (view.size () == len);

    for (size_t i = 0; i < len; i++)
      SELF_CHECK (view[i] == data[i]);
  }

  /* Test slicing.  */
  {
    gdb_byte data[] = {0x55, 0x66, 0x77, 0x88, 0x99};
    gdb::array_view<gdb_byte> view = data;

    {
      auto slc = view.slice (1, 3);
      SELF_CHECK (slc.data () == data + 1);
      SELF_CHECK (slc.size () == 3);
      SELF_CHECK (slc[0] == data[1]);
      SELF_CHECK (slc[0] == view[1]);
    }

    {
      auto slc = view.slice (2);
      SELF_CHECK (slc.data () == data + 2);
      SELF_CHECK (slc.size () == 3);
      SELF_CHECK (slc[0] == view[2]);
      SELF_CHECK (slc[0] == data[2]);
    }
  }
}

template <typename T>
void
run_copy_test ()
{
  /* Test non-overlapping copy.  */
  {
    const std::vector<T> src_v = {1, 2, 3, 4};
    std::vector<T> dest_v (4, -1);

    SELF_CHECK (dest_v != src_v);
    copy (gdb::array_view<const T> (src_v), gdb::array_view<T> (dest_v));
    SELF_CHECK (dest_v == src_v);
  }

  /* Test overlapping copy, where the source is before the destination.  */
  {
    std::vector<T> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    gdb::array_view<T> v = vec;

    copy (v.slice (1, 4),
	  v.slice (2, 4));

    std::vector<T> expected = {1, 2, 2, 3, 4, 5, 7, 8};
    SELF_CHECK (vec == expected);
  }

  /* Test overlapping copy, where the source is after the destination.  */
  {
    std::vector<T> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    gdb::array_view<T> v = vec;

    copy (v.slice (2, 4),
	  v.slice (1, 4));

    std::vector<T> expected = {1, 3, 4, 5, 6, 6, 7, 8};
    SELF_CHECK (vec == expected);
  }

  /* Test overlapping copy, where the source is the same as the destination.  */
  {
    std::vector<T> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    gdb::array_view<T> v = vec;

    copy (v.slice (2, 4),
	  v.slice (2, 4));

    std::vector<T> expected = {1, 2, 3, 4, 5, 6, 7, 8};
    SELF_CHECK (vec == expected);
  }
}

/* Class with a non-trivial copy assignment operator, used to test the
   array_view copy function.  */
struct foo
{
  /* Can be implicitly constructed from an int, such that we can use the same
     templated test function to test against array_view<int> and
     array_view<foo>.  */
  foo (int n)
    : n (n)
  {}

  /* Needed to avoid -Wdeprecated-copy-with-user-provided-copy error with
     Clang.  */
  foo (const foo &other) = default;

  void operator= (const foo &other)
  {
    this->n = other.n;
    this->n_assign_op_called++;
  }

  bool operator==(const foo &other) const
  {
    return this->n == other.n;
  }

  int n;

  /* Number of times the assignment operator has been called.  */
  static int n_assign_op_called;
};

int foo::n_assign_op_called = 0;

/* Test the array_view copy free function.  */

static void
run_copy_tests ()
{
  /* Test with a trivial type.  */
  run_copy_test<int> ();

  /* Test with a non-trivial type.  */
  foo::n_assign_op_called = 0;
  run_copy_test<foo> ();

  /* Make sure that for the non-trivial type foo, the assignment operator was
     called an amount of times that makes sense.  */
  SELF_CHECK (foo::n_assign_op_called == 12);
}

} /* namespace array_view_tests */
} /* namespace selftests */

void _initialize_array_view_selftests ();
void
_initialize_array_view_selftests ()
{
  selftests::register_test ("array_view",
			    selftests::array_view_tests::run_tests);
  selftests::register_test ("array_view-copy",
			    selftests::array_view_tests::run_copy_tests);
}

/* Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_ARRAY_VIEW_H
#define COMMON_ARRAY_VIEW_H

#include "traits.h"
#include <algorithm>
#include <type_traits>
#include "gdbsupport/gdb_assert.h"

/* An array_view is an abstraction that provides a non-owning view
   over a sequence of contiguous objects.

   A way to put it is that array_view is to std::vector (and
   std::array and built-in arrays with rank==1) like std::string_view
   is to std::string.

   The main intent of array_view is to use it as function input
   parameter type, making it possible to pass in any sequence of
   contiguous objects, irrespective of whether the objects live on the
   stack or heap and what actual container owns them.  Implicit
   construction from the element type is supported too, making it easy
   to call functions that expect an array of elements when you only
   have one element (usually on the stack).  For example:

    struct A { .... };
    void function (gdb::array_view<A> as);

    std::vector<A> std_vec = ...;
    std::array<A, N> std_array = ...;
    A array[] = {...};
    A elem;

    function (std_vec);
    function (std_array);
    function (array);
    function (elem);

   Views can be either mutable or const.  A const view is simply
   created by specifying a const T as array_view template parameter,
   in which case operator[] of non-const array_view objects ends up
   returning const references.  Making the array_view itself const is
   analogous to making a pointer itself be const.  I.e., disables
   re-seating the view/pointer.

   Since array_view objects are small (pointer plus size), and
   designed to be trivially copyable, they should generally be passed
   around by value.

   You can find unit tests covering the whole API in
   unittests/array-view-selftests.c.  */

namespace gdb {

template <typename T>
class array_view
{
  /* True iff decayed T is the same as decayed U.  E.g., we want to
     say that 'T&' is the same as 'const T'.  */
  template <typename U>
  using IsDecayedT = typename std::is_same<typename std::decay<T>::type,
					   typename std::decay<U>::type>;

  /* True iff decayed T is the same as decayed U, and 'U *' is
     implicitly convertible to 'T *'.  This is a requirement for
     several methods.  */
  template <typename U>
  using DecayedConvertible = gdb::And<IsDecayedT<U>,
				      std::is_convertible<U *, T *>>;

public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using size_type = size_t;

  /* Default construction creates an empty view.  */
  constexpr array_view () noexcept
    : m_array (nullptr), m_size (0)
  {}

  /* Create an array view over a single object of the type of an
     array_view element.  The created view as size==1.  This is
     templated on U to allow constructing a array_view<const T> over a
     (non-const) T.  The "convertible" requirement makes sure that you
     can't create an array_view<T> over a const T.  */
  template<typename U,
	   typename = Requires<DecayedConvertible<U>>>
  constexpr array_view (U &elem) noexcept
    : m_array (&elem), m_size (1)
  {}

  /* Same as above, for rvalue references.  */
  template<typename U,
	   typename = Requires<DecayedConvertible<U>>>
  constexpr array_view (U &&elem) noexcept
    : m_array (&elem), m_size (1)
  {}

  /* Create an array view from a pointer to an array and an element
     count.  */
  template<typename U,
	   typename = Requires<DecayedConvertible<U>>>
  constexpr array_view (U *array, size_t size) noexcept
    : m_array (array), m_size (size)
  {}

  /* Create an array view from a range.  This is templated on both U
     an V to allow passing in a mix of 'const T *' and 'T *'.  */
  template<typename U, typename V,
	   typename = Requires<DecayedConvertible<U>>,
	   typename = Requires<DecayedConvertible<V>>>
  constexpr array_view (U *begin, V *end) noexcept
    : m_array (begin), m_size (end - begin)
  {}

  /* Create an array view from an array.  */
  template<typename U, size_t Size,
	   typename = Requires<DecayedConvertible<U>>>
  constexpr array_view (U (&array)[Size]) noexcept
    : m_array (array), m_size (Size)
  {}

  /* Create an array view from a contiguous container.  E.g.,
     std::vector and std::array.  */
  template<typename Container,
	   typename = Requires<gdb::Not<IsDecayedT<Container>>>,
	   typename
	     = Requires<DecayedConvertible
			<typename std::remove_pointer
			 <decltype (std::declval<Container> ().data ())
			 >::type>>,
	   typename
	     = Requires<std::is_convertible
			<decltype (std::declval<Container> ().size ()),
			 size_type>>>
  constexpr array_view (Container &&c) noexcept
    : m_array (c.data ()), m_size (c.size ())
  {}

  /* Observer methods.  */
  constexpr T *data () noexcept { return m_array; }
  constexpr const T *data () const noexcept { return m_array; }

  constexpr T *begin () noexcept { return m_array; }
  constexpr const T *begin () const noexcept { return m_array; }

  constexpr T *end () noexcept { return m_array + m_size; }
  constexpr const T *end () const noexcept { return m_array + m_size; }

  constexpr reference operator[] (size_t index) noexcept
  {
#if defined(_GLIBCXX_DEBUG)
    gdb_assert (index < m_size);
#endif
    return m_array[index];
  }
  constexpr const_reference operator[] (size_t index) const noexcept
  {
#if defined(_GLIBCXX_DEBUG)
    gdb_assert (index < m_size);
#endif
    return m_array[index];
  }

  constexpr size_type size () const noexcept { return m_size; }
  constexpr bool empty () const noexcept { return m_size == 0; }

  /* Slice an array view.  */

  /* Return a new array view over SIZE elements starting at START.  */
  [[nodiscard]]
  constexpr array_view<T> slice (size_type start, size_type size) const noexcept
  {
#if defined(_GLIBCXX_DEBUG)
    gdb_assert (start + size <= m_size);
#endif
    return {m_array + start, size};
  }

  /* Return a new array view over all the elements after START,
     inclusive.  */
  [[nodiscard]]
  constexpr array_view<T> slice (size_type start) const noexcept
  {
#if defined(_GLIBCXX_DEBUG)
    gdb_assert (start <= m_size);
#endif
    return {m_array + start, size () - start};
  }

private:
  T *m_array;
  size_type m_size;
};

/* Copy the contents referenced by the array view SRC to the array view DEST.

   The two array views must have the same length.  */

template <typename U, typename T>
void copy (gdb::array_view<U> src, gdb::array_view<T> dest)
{
  gdb_assert (dest.size () == src.size ());
  if (dest.data () < src.data ())
    std::copy (src.begin (), src.end (), dest.begin ());
  else if (dest.data () > src.data ())
    std::copy_backward (src.begin (), src.end (), dest.end ());
}

/* Compare LHS and RHS for (deep) equality.  That is, whether LHS and
   RHS have the same sizes, and whether each pair of elements of LHS
   and RHS at the same position compares equal.  */

template <typename T>
bool
operator== (const gdb::array_view<T> &lhs, const gdb::array_view<T> &rhs)
{
  if (lhs.size () != rhs.size ())
    return false;

  for (size_t i = 0; i < lhs.size (); i++)
    if (!(lhs[i] == rhs[i]))
      return false;

  return true;
}

/* Compare two array_views for inequality.  */

template <typename T>
bool
operator!= (const gdb::array_view<T> &lhs, const gdb::array_view<T> &rhs)
{
  return !(lhs == rhs);
}

/* Create an array view from a pointer to an array and an element
   count.

   This is useful as alternative to constructing an array_view using
   brace initialization when the size variable you have handy is of
   signed type, since otherwise without an explicit cast the code
   would be ill-formed.

   For example, with:

     extern void foo (int, int, gdb::array_view<value *>);

     value *args[2];
     int nargs;
     foo (1, 2, {values, nargs});

   You'd get:

     source.c:10: error: narrowing conversion of ‘nargs’ from ‘int’ to
     ‘size_t {aka long unsigned int}’ inside { } [-Werror=narrowing]

   You could fix it by writing the somewhat distracting explicit cast:

     foo (1, 2, {values, (size_t) nargs});

   Or by instantiating an array_view explicitly:

     foo (1, 2, gdb::array_view<value *>(values, nargs));

   Or, better, using make_array_view, which has the advantage of
   inferring the array_view element's type:

     foo (1, 2, gdb::make_array_view (values, nargs));
*/

template<typename U>
constexpr inline array_view<U>
make_array_view (U *array, size_t size) noexcept
{
  return {array, size};
}

} /* namespace gdb */

#endif

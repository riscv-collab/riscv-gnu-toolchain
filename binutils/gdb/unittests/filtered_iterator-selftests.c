/* Self tests for the filtered_iterator class.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/filtered-iterator.h"

#include <iterator>

namespace selftests {

/* An iterator class that iterates on integer arrays.  */

struct int_array_iterator
{
  using value_type = int;
  using reference = int &;
  using pointer = int *;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = int;

  /* Create an iterator that points at the first element of an integer
     array at ARRAY of size SIZE.  */
  int_array_iterator (int *array, size_t size)
    : m_array (array), m_size (size)
  {}

  /* Create a past-the-end iterator.  */
  int_array_iterator ()
    : m_array (nullptr), m_size (0)
  {}

  bool operator== (const int_array_iterator &other) const
  {
    /* If both are past-the-end, they are equal.  */
    if (m_array == nullptr && other.m_array == nullptr)
      return true;

    /* If just one of them is past-the-end, they are not equal.  */
    if (m_array == nullptr || other.m_array == nullptr)
      return false;

    /* If they are both not past-the-end, make sure they iterate on the
       same array (we shouldn't compare iterators that iterate on different
       things).  */
    SELF_CHECK (m_array == other.m_array);

    /* They are equal if they have the same current index.  */
    return m_cur_idx == other.m_cur_idx;
  }

  bool operator!= (const int_array_iterator &other) const
  {
    return !(*this == other);
  }

  void operator++ ()
  {
    /* Make sure nothing tries to increment a past the end iterator. */
    SELF_CHECK (m_cur_idx < m_size);

    m_cur_idx++;

    /* Mark the iterator as "past-the-end" if we have reached the end.  */
    if (m_cur_idx == m_size)
      m_array = nullptr;
  }

  int operator* () const
  {
    /* Make sure nothing tries to dereference a past the end iterator.  */
    SELF_CHECK (m_cur_idx < m_size);

    return m_array[m_cur_idx];
  }

private:
  /* A nullptr value in M_ARRAY indicates a past-the-end iterator.  */
  int *m_array;
  size_t m_size;
  size_t m_cur_idx = 0;
};

/* Filter to only keep the even numbers.  */

struct even_numbers_only
{
  bool operator() (int n)
  {
    return n % 2 == 0;
  }
};

/* Test typical usage.  */

static void
test_filtered_iterator ()
{
  int array[] = { 4, 4, 5, 6, 7, 8, 9 };
  std::vector<int> even_ints;
  const std::vector<int> expected_even_ints { 4, 4, 6, 8 };

  filtered_iterator<int_array_iterator, even_numbers_only>
    iter (array, ARRAY_SIZE (array));
  filtered_iterator<int_array_iterator, even_numbers_only> end;

  for (; iter != end; ++iter)
    even_ints.push_back (*iter);

  SELF_CHECK (even_ints == expected_even_ints);
}

/* Test operator== and operator!=. */

static void
test_filtered_iterator_eq ()
{
  int array[] = { 4, 4, 5, 6, 7, 8, 9 };

  filtered_iterator<int_array_iterator, even_numbers_only>
    iter1(array, ARRAY_SIZE (array));
  filtered_iterator<int_array_iterator, even_numbers_only>
    iter2(array, ARRAY_SIZE (array));

  /* They start equal.  */
  SELF_CHECK (iter1 == iter2);
  SELF_CHECK (!(iter1 != iter2));

  /* Advance 1, now they aren't equal (despite pointing to equal values).  */
  ++iter1;
  SELF_CHECK (!(iter1 == iter2));
  SELF_CHECK (iter1 != iter2);

  /* Advance 2, now they are equal again.  */
  ++iter2;
  SELF_CHECK (iter1 == iter2);
  SELF_CHECK (!(iter1 != iter2));
}

} /* namespace selftests */

void _initialize_filtered_iterator_selftests ();
void
_initialize_filtered_iterator_selftests ()
{
  selftests::register_test ("filtered_iterator",
			    selftests::test_filtered_iterator);
  selftests::register_test ("filtered_iterator_eq",
			    selftests::test_filtered_iterator_eq);
}

/* A safe iterator for GDB, the GNU debugger.
   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_SAFE_ITERATOR_H
#define COMMON_SAFE_ITERATOR_H

#include <type_traits>

/* A forward iterator that wraps Iterator, such that when iterating
   with iterator IT, it is possible to delete *IT without invalidating
   IT.  Suitably wrapped in a range type and used with range-for, this
   allow convenient patterns like this:

     // range_safe() returns a range type whose begin()/end() methods
     // return safe iterators.
     for (foo *f : range_safe ())
       {
	 if (f->should_delete ())
	   {
	     // The ++it operation implicitly done by the range-for is
	     // still OK after this.
	     delete f;
	   }
       }
*/

template<typename Iterator>
class basic_safe_iterator
{
public:
  typedef basic_safe_iterator self_type;
  typedef typename Iterator::value_type value_type;
  typedef typename Iterator::reference reference;
  typedef typename Iterator::pointer pointer;
  typedef typename Iterator::iterator_category iterator_category;
  typedef typename Iterator::difference_type difference_type;

  /* Construct the begin iterator using the given arguments; the end iterator is
     default constructed.  */
  template<typename... Args>
  explicit basic_safe_iterator (Args &&...args)
    : m_it (std::forward<Args> (args)...),
      m_next (m_it)
  {
    if (m_it != m_end)
      ++m_next;
  }

  /* Construct the iterator using the first argument, and construct
     the end iterator using the second argument.  */
  template<typename Arg>
  explicit basic_safe_iterator (Arg &&arg, Arg &&arg2)
    : m_it (std::forward<Arg> (arg)),
      m_next (m_it),
      m_end (std::forward<Arg> (arg2))
  {
    if (m_it != m_end)
      ++m_next;
  }

  /* Create a one-past-end iterator.  */
  basic_safe_iterator ()
  {}

  typename std::invoke_result<decltype(&Iterator::operator*), Iterator>::type
    operator* () const
  { return *m_it; }

  self_type &operator++ ()
  {
    m_it = m_next;
    if (m_it != m_end)
      ++m_next;
    return *this;
  }

  bool operator== (const self_type &other) const
  { return m_it == other.m_it; }

  bool operator!= (const self_type &other) const
  { return m_it != other.m_it; }

private:
  /* The current element.  */
  Iterator m_it {};

  /* The next element.  Always one element ahead of M_IT.  */
  Iterator m_next {};

  /* A one-past-end iterator.  */
  Iterator m_end {};
};

/* A range adapter that wraps another range, and then returns safe
   iterators wrapping the original range's iterators.  */

template<typename Range>
class basic_safe_range
{
public:

  typedef basic_safe_iterator<typename Range::iterator> iterator;

  explicit basic_safe_range (Range range)
    : m_range (range)
  {
  }

  iterator begin ()
  {
    return iterator (m_range.begin (), m_range.end ());
  }

  iterator end ()
  {
    return iterator (m_range.end (), m_range.end ());
  }

private:

  Range m_range;
};

#endif /* COMMON_SAFE_ITERATOR_H */

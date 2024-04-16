/* A forward filtered iterator for GDB, the GNU debugger.
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

#ifndef COMMON_FILTERED_ITERATOR_H
#define COMMON_FILTERED_ITERATOR_H

#include <type_traits>

/* A filtered iterator.  This wraps BaseIterator and automatically
   skips elements that FilterFunc filters out.  Requires that
   default-constructing a BaseIterator creates a valid one-past-end
   iterator.  */

template<typename BaseIterator, typename FilterFunc>
class filtered_iterator
{
public:
  typedef filtered_iterator self_type;
  typedef typename BaseIterator::value_type value_type;
  typedef typename BaseIterator::reference reference;
  typedef typename BaseIterator::pointer pointer;
  typedef typename BaseIterator::iterator_category iterator_category;
  typedef typename BaseIterator::difference_type difference_type;

  /* Construct by forwarding all arguments to the underlying
     iterator.  */
  template<typename... Args>
  explicit filtered_iterator (Args &&...args)
    : m_it (std::forward<Args> (args)...)
  { skip_filtered (); }

  /* Create a one-past-end iterator.  */
  filtered_iterator () = default;

  /* Need these as the variadic constructor would be a better match
     otherwise.  */
  filtered_iterator (filtered_iterator &) = default;
  filtered_iterator (const filtered_iterator &) = default;
  filtered_iterator (filtered_iterator &&) = default;
  filtered_iterator (const filtered_iterator &&other)
    : filtered_iterator (static_cast<const filtered_iterator &> (other))
  {}

  typename std::invoke_result<decltype(&BaseIterator::operator*),
			      BaseIterator>::type
    operator* () const
  { return *m_it; }

  self_type &operator++ ()
  {
    ++m_it;
    skip_filtered ();
    return *this;
  }

  bool operator== (const self_type &other) const
  { return m_it == other.m_it; }

  bool operator!= (const self_type &other) const
  { return m_it != other.m_it; }

private:

  void skip_filtered ()
  {
    for (; m_it != m_end; ++m_it)
      if (m_filter (*m_it))
	break;
  }

private:
  FilterFunc m_filter {};
  BaseIterator m_it {};
  BaseIterator m_end {};
};

#endif /* COMMON_FILTERED_ITERATOR_H */

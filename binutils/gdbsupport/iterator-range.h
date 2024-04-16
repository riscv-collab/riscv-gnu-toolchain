/* A range adapter that wraps begin / end iterators.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_ITERATOR_RANGE_H
#define GDBSUPPORT_ITERATOR_RANGE_H

/* A wrapper that allows using ranged for-loops on a range described by two
   iterators.  */

template <typename IteratorType>
struct iterator_range
{
  using iterator = IteratorType;

  /* Create an iterator_range using BEGIN as the begin iterator.

     Assume that the end iterator can be default-constructed.  */
  template <typename... Args>
  iterator_range (Args &&...args)
    : m_begin (std::forward<Args> (args)...)
  {}

  /* Create an iterator range using explicit BEGIN and END iterators.  */
  template <typename... Args>
  iterator_range (IteratorType begin, IteratorType end)
    : m_begin (std::move (begin)), m_end (std::move (end))
  {}

  /* Need these as the variadic constructor would be a better match
     otherwise.  */
  iterator_range (iterator_range &) = default;
  iterator_range (const iterator_range &) = default;
  iterator_range (iterator_range &&) = default;

  IteratorType begin () const
  { return m_begin; }

  IteratorType end () const
  { return m_end; }

  /* The number of items in this iterator_range.  */
  std::size_t size () const
  { return std::distance (m_begin, m_end); }

private:
  IteratorType m_begin, m_end;
};

#endif /* GDBSUPPORT_ITERATOR_RANGE_H */

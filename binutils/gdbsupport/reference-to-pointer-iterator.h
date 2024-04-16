/* An iterator wrapper that yields pointers instead of references.
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

#ifndef GDBSUPPORT_REFERENCE_TO_POINTER_ITERATOR_H
#define GDBSUPPORT_REFERENCE_TO_POINTER_ITERATOR_H

/* Wrap an iterator that yields references to objects so that it yields
   pointers to objects instead.

   This is useful for example to bridge the gap between iterators on intrusive
   lists, which yield references, and the rest of GDB, which for legacy reasons
   expects to iterate on pointers.  */

template <typename IteratorType>
struct reference_to_pointer_iterator
{
  using self_type = reference_to_pointer_iterator;
  using value_type = typename IteratorType::value_type *;
  using reference = typename IteratorType::value_type *&;
  using pointer = typename IteratorType::value_type **;
  using iterator_category = typename IteratorType::iterator_category;
  using difference_type = typename IteratorType::difference_type;

  /* Construct a reference_to_pointer_iterator, passing args to the underlying
     iterator.  */
  template <typename... Args>
  reference_to_pointer_iterator (Args &&...args)
    : m_it (std::forward<Args> (args)...)
  {}

  /* Create a past-the-end iterator.

     Assumes that default-constructing an underlying iterator creates a
     past-the-end iterator.  */
  reference_to_pointer_iterator ()
  {}

  /* Need these as the variadic constructor would be a better match
     otherwise.  */
  reference_to_pointer_iterator (reference_to_pointer_iterator &) = default;
  reference_to_pointer_iterator (const reference_to_pointer_iterator &) = default;
  reference_to_pointer_iterator (reference_to_pointer_iterator &&) = default;

  reference_to_pointer_iterator &operator= (const reference_to_pointer_iterator &) = default;
  reference_to_pointer_iterator &operator= (reference_to_pointer_iterator &&) = default;

  value_type operator* () const
  { return &*m_it; }

  self_type &operator++ ()
  {
    ++m_it;
    return *this;
  }

  self_type &operator++ (int)
  {
    m_it++;
    return *this;
  }

  self_type &operator-- ()
  {
    --m_it;
    return *this;
  }

  self_type &operator-- (int)
  {
    m_it--;
    return *this;
  }

  bool operator== (const self_type &other) const
  { return m_it == other.m_it; }

  bool operator!= (const self_type &other) const
  { return m_it != other.m_it; }

private:
  /* The underlying iterator.  */
  IteratorType m_it;
};

#endif /* GDBSUPPORT_REFERENCE_TO_POINTER_ITERATOR_H */

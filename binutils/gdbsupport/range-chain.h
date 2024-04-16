/* A range adapter that wraps multiple ranges
   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_RANGE_CHAIN_H
#define GDBSUPPORT_RANGE_CHAIN_H

/* A range adapter that presents a number of ranges as if it were a
   single range.  That is, iterating over a range_chain will iterate
   over each sub-range in order.  */
template<typename Range>
struct range_chain
{
  /* The type of the iterator that is created by this range.  */
  class iterator
  {
  public:

    iterator (const std::vector<Range> &ranges, size_t idx)
      : m_index (idx),
	m_ranges (ranges)
    {
      skip_empty ();
    }

    bool operator== (const iterator &other) const
    {
      if (m_index != other.m_index || &m_ranges != &other.m_ranges)
	return false;
      if (m_current.has_value () != other.m_current.has_value ())
	return false;
      if (m_current.has_value ())
	return *m_current == *other.m_current;
      return true;
    }

    bool operator!= (const iterator &other) const
    {
      return !(*this == other);
    }

    iterator &operator++ ()
    {
      ++*m_current;
      if (*m_current == m_ranges[m_index].end ())
	{
	  ++m_index;
	  skip_empty ();
	}
      return *this;
    }

    typename Range::iterator::value_type operator* () const
    {
      return **m_current;
    }

  private:
    /* Skip empty sub-ranges.  If this finds a valid sub-range,
       m_current is updated to point to its start; otherwise,
       m_current is reset.  */
    void skip_empty ()
    {
      for (; m_index < m_ranges.size (); ++m_index)
	{
	  m_current = m_ranges[m_index].begin ();
	  if (*m_current != m_ranges[m_index].end ())
	    return;
	}
      m_current.reset ();
    }

    /* Index into the vector indicating where the current iterator
       comes from.  */
    size_t m_index;
    /* The current iterator into one of the vector ranges.  If no
       value then this (outer) iterator is at the end of the overall
       range.  */
    std::optional<typename Range::iterator> m_current;
    /* Vector of ranges.  */
    const std::vector<Range> &m_ranges;
  };

  /* Create a new range_chain.  */
  template<typename T>
  range_chain (T &&ranges)
    : m_ranges (std::forward<T> (ranges))
  {
  }

  iterator begin () const
  {
    return iterator (m_ranges, 0);
  }

  iterator end () const
  {
    return iterator (m_ranges, m_ranges.size ());
  }

private:

  /* The sub-ranges.  */
  std::vector<Range> m_ranges;
};

#endif /* GDBSUPPORT_RANGE_CHAIN_H */

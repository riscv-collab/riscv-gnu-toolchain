/* C++ implementation of a binary search.

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


#ifndef GDBSUPPORT_GDB_BINARY_SEARCH_H
#define GDBSUPPORT_GDB_BINARY_SEARCH_H

#include <algorithm>

namespace gdb {

/* Implements a binary search using C++ iterators.
   This differs from std::binary_search in that it returns an iterator for
   the found element and in that the type of EL can be different from the
   type of the elements in the container.

   COMP is a C-style comparison function with signature:
   int comp(const value_type& a, const T& b);
   It should return -1, 0 or 1 if a is less than, equal to, or greater than
   b, respectively.
   [first, last) must be sorted.

   The return value is an iterator pointing to the found element, or LAST if
   no element was found.  */
template<typename It, typename T, typename Comp>
It binary_search (It first, It last, T el, Comp comp)
{
  auto lt = [&] (const typename std::iterator_traits<It>::value_type &a,
		 const T &b)
    { return comp (a, b) < 0; };

  auto lb = std::lower_bound (first, last, el, lt);
  if (lb != last)
    {
      if (comp (*lb, el) == 0)
	return lb;
    }
  return last;
}

} /* namespace gdb */

#endif /* GDBSUPPORT_GDB_BINARY_SEARCH_H */

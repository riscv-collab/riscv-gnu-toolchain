/* Some commonly-used VEC types.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_GDB_VECS_H
#define COMMON_GDB_VECS_H

/* Split STR, a list of DELIMITER-separated fields, into a char pointer vector.

   You may modify the returned strings.  */

extern std::vector<gdb::unique_xmalloc_ptr<char>>
  delim_string_to_char_ptr_vec (const char *str, char delimiter);

/* Like dirnames_to_char_ptr_vec, but append the directories to *VECP.  */

extern void dirnames_to_char_ptr_vec_append
  (std::vector<gdb::unique_xmalloc_ptr<char>> *vecp, const char *dirnames);

/* Split DIRNAMES by DIRNAME_SEPARATOR delimiter and return a list of all the
   elements in their original order.  For empty string ("") DIRNAMES return
   list of one empty string ("") element.

   You may modify the returned strings.  */

extern std::vector<gdb::unique_xmalloc_ptr<char>>
  dirnames_to_char_ptr_vec (const char *dirnames);

/* Remove the element pointed by iterator IT from VEC, not preserving the order
   of the remaining elements.  Return the removed element.  */

template <typename T>
T
unordered_remove (std::vector<T> &vec, typename std::vector<T>::iterator it)
{
  gdb_assert (it >= vec.begin () && it < vec.end ());

  T removed = std::move (*it);
  if (it != vec.end () - 1)
    *it = std::move (vec.back ());
  vec.pop_back ();

  return removed;
}

/* Remove the element at position IX from VEC, not preserving the order of the
   remaining elements.  Return the removed element.  */

template <typename T>
T
unordered_remove (std::vector<T> &vec, typename std::vector<T>::size_type ix)
{
  gdb_assert (ix < vec.size ());

  return unordered_remove (vec, vec.begin () + ix);
}

/* Remove the element at position IX from VEC, preserving the order the
   remaining elements.  Return the removed element.  */

template <typename T>
T
ordered_remove (std::vector<T> &vec, typename std::vector<T>::size_type ix)
{
  gdb_assert (ix < vec.size ());

  T removed = std::move (vec[ix]);
  vec.erase (vec.begin () + ix);

  return removed;
}

#endif /* COMMON_GDB_VECS_H */

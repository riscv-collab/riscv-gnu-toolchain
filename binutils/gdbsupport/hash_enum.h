/* A hasher for enums.

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

#ifndef COMMON_HASH_ENUM_H
#define COMMON_HASH_ENUM_H

/* A hasher for enums, which was missing in C++11:
    http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#2148
*/

namespace gdb {

/* Helper struct for hashing enum types.  */
template<typename T>
struct hash_enum
{
  typedef size_t result_type;
  typedef T argument_type;

  size_t operator() (T val) const noexcept
  {
    using underlying = typename std::underlying_type<T>::type;
    return std::hash<underlying> () (static_cast<underlying> (val));
  }
};

} /* namespace gdb */

#endif /* COMMON_HASH_ENUM_H */

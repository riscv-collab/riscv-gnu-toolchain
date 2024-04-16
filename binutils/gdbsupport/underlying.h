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

#ifndef COMMON_UNDERLYING_H
#define COMMON_UNDERLYING_H

#include <type_traits>

/* Convert an enum to its underlying value.  */

template<typename E>
constexpr typename std::underlying_type<E>::type
to_underlying (E val) noexcept
{
  return static_cast<typename std::underlying_type<E>::type> (val);
}

#endif

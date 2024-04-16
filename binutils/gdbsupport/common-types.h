/* Declarations for common types.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_COMMON_TYPES_H
#define COMMON_COMMON_TYPES_H

#include <inttypes.h>

/* * A byte from the program being debugged.  */
typedef unsigned char gdb_byte;

/* * An address in the program being debugged.  Host byte order.  */
typedef uint64_t CORE_ADDR;

/* Like a CORE_ADDR, but not directly convertible.  This is used to
   represent an unrelocated CORE_ADDR.  DEFINE_OFFSET_TYPE is not used
   here because there's no need to add or subtract values of this
   type.  */
enum class unrelocated_addr : CORE_ADDR { };

/* LONGEST must be at least as big as CORE_ADDR.  */

typedef int64_t LONGEST;
typedef uint64_t ULONGEST;

/* * The largest CORE_ADDR value.  */
#define CORE_ADDR_MAX (~(CORE_ADDR) 0)

/* * The largest ULONGEST value, 0xFFFFFFFFFFFFFFFF for 64-bits.  */
#define ULONGEST_MAX (~(ULONGEST) 0)

/* * The largest LONGEST value, 0x7FFFFFFFFFFFFFFF for 64-bits.  */
#define LONGEST_MAX ((LONGEST) (ULONGEST_MAX >> 1))

/* * The smallest LONGEST value, 0x8000000000000000 for 64-bits.  */
#define LONGEST_MIN ((LONGEST) (~(LONGEST) 0 ^ LONGEST_MAX))

enum tribool { TRIBOOL_UNKNOWN = -1, TRIBOOL_FALSE = 0, TRIBOOL_TRUE = 1 };

#endif /* COMMON_COMMON_TYPES_H */

/* Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#ifndef PACKED_H
#define PACKED_H

#include "traits.h"
#include <atomic>

/* Each instantiation and full specialization of the packed template
   defines a type that behaves like a given scalar type, but that has
   byte alignment, and, may optionally have a smaller size than the
   given scalar type.  This is typically used as alternative to
   bit-fields (and ENUM_BITFIELD), when the fields must have separate
   memory locations to avoid data races.  */

/* There are two implementations here -- one standard compliant, using
   a byte array for internal representation, and another that relies
   on bitfields and attribute packed (and attribute gcc_struct on
   Windows).  The latter is preferable, as it is more convenient when
   debugging GDB -- printing a struct packed variable prints its field
   using its natural type, which is particularly useful if the type is
   an enum -- but may not work on all compilers.  */

/* Clang targeting Windows does not support attribute gcc_struct, so
   we use the alternative byte array implementation there. */
#if defined _WIN32 && defined __clang__
# define PACKED_USE_ARRAY 1
#else
# define PACKED_USE_ARRAY 0
#endif

/* For the preferred implementation, we need gcc_struct on Windows, as
   otherwise the size of e.g., "packed<int, 1>" will be larger than
   what we want.  Clang targeting Windows does not support attribute
   gcc_struct.  */
#if !PACKED_USE_ARRAY && defined _WIN32 && !defined __clang__
# define ATTRIBUTE_GCC_STRUCT __attribute__((__gcc_struct__))
#else
# define ATTRIBUTE_GCC_STRUCT
#endif

template<typename T, size_t Bytes = sizeof (T)>
struct ATTRIBUTE_GCC_STRUCT packed
{
public:
  packed () noexcept = default;

  packed (T val)
  {
    static_assert (sizeof (ULONGEST) >= sizeof (T));

#if PACKED_USE_ARRAY
    ULONGEST tmp = val;
    for (int i = (Bytes - 1); i >= 0; --i)
      {
	m_bytes[i] = (gdb_byte) tmp;
	tmp >>= HOST_CHAR_BIT;
      }
#else
    m_val = val;
#endif

    /* Ensure size and aligment are what we expect.  */
    static_assert (sizeof (packed) == Bytes);
    static_assert (alignof (packed) == 1);

    /* Make sure packed can be wrapped with std::atomic.  */
#if HAVE_IS_TRIVIALLY_COPYABLE
    static_assert (std::is_trivially_copyable<packed>::value);
#endif
    static_assert (std::is_copy_constructible<packed>::value);
    static_assert (std::is_move_constructible<packed>::value);
    static_assert (std::is_copy_assignable<packed>::value);
    static_assert (std::is_move_assignable<packed>::value);
  }

  operator T () const noexcept
  {
#if PACKED_USE_ARRAY
    ULONGEST tmp = 0;
    for (int i = 0;;)
      {
	tmp |= m_bytes[i];
	if (++i == Bytes)
	  break;
	tmp <<= HOST_CHAR_BIT;
      }
    return (T) tmp;
#else
    return m_val;
#endif
  }

private:
#if PACKED_USE_ARRAY
  gdb_byte m_bytes[Bytes];
#else
  T m_val : (Bytes * HOST_CHAR_BIT) ATTRIBUTE_PACKED;
#endif
};

/* Add some comparisons between std::atomic<packed<T>> and packed<T>
   and T.  We need this because even though std::atomic<T> doesn't
   define these operators, the relational expressions still work via
   implicit conversions.  Those wouldn't work when wrapped in packed
   without these operators, because they'd require two implicit
   conversions to go from T to packed<T> to std::atomic<packed<T>>
   (and back), and C++ only does one.  */

#define PACKED_ATOMIC_OP(OP)						\
  template<typename T, size_t Bytes>					\
  bool operator OP (const std::atomic<packed<T, Bytes>> &lhs,		\
		    const std::atomic<packed<T, Bytes>> &rhs)		\
  {									\
    return lhs.load () OP rhs.load ();					\
  }									\
									\
  template<typename T, size_t Bytes>					\
  bool operator OP (T lhs, const std::atomic<packed<T, Bytes>> &rhs)	\
  {									\
    return lhs OP rhs.load ();						\
  }									\
									\
  template<typename T, size_t Bytes>					\
  bool operator OP (const std::atomic<packed<T, Bytes>> &lhs, T rhs)	\
  {									\
    return lhs.load () OP rhs;						\
  }									\
									\
  template<typename T, size_t Bytes>					\
  bool operator OP (const std::atomic<packed<T, Bytes>> &lhs,		\
		    packed<T, Bytes> rhs)				\
  {									\
    return lhs.load () OP rhs;						\
  }									\
									\
  template<typename T, size_t Bytes>					\
  bool operator OP (packed<T, Bytes> lhs,				\
		    const std::atomic<packed<T, Bytes>> &rhs)		\
  {									\
    return lhs OP rhs.load ();						\
  }

PACKED_ATOMIC_OP (==)
PACKED_ATOMIC_OP (!=)
PACKED_ATOMIC_OP (>)
PACKED_ATOMIC_OP (<)
PACKED_ATOMIC_OP (>=)
PACKED_ATOMIC_OP (<=)

#undef PACKED_ATOMIC_OP

#endif

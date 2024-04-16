/* std::unique_ptr specializations for GDB.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_GDB_UNIQUE_PTR_H
#define COMMON_GDB_UNIQUE_PTR_H

#include <memory>
#include <string>
#include "gdbsupport/gdb-xfree.h"

namespace gdb
{
/* Define gdb::unique_xmalloc_ptr, a std::unique_ptr that manages
   xmalloc'ed memory.  */

/* The deleter for std::unique_xmalloc_ptr.  Uses xfree.  */
template <typename T>
struct xfree_deleter
{
  void operator() (T *ptr) const { xfree (ptr); }
};

/* Same, for arrays.  */
template <typename T>
struct xfree_deleter<T[]>
{
  void operator() (T *ptr) const { xfree (ptr); }
};

/* Import the standard unique_ptr to our namespace with a custom
   deleter.  */

template<typename T> using unique_xmalloc_ptr
  = std::unique_ptr<T, xfree_deleter<T>>;

/* A no-op deleter.  */
template<typename T>
struct noop_deleter
{
  void operator() (T *ptr) const { }
};

} /* namespace gdb */

/* Dup STR and return a unique_xmalloc_ptr for the result.  */

static inline gdb::unique_xmalloc_ptr<char>
make_unique_xstrdup (const char *str)
{
  return gdb::unique_xmalloc_ptr<char> (xstrdup (str));
}

/* Dup the first N characters of STR and return a unique_xmalloc_ptr
   for the result.  The result is always \0-terminated.  */

static inline gdb::unique_xmalloc_ptr<char>
make_unique_xstrndup (const char *str, size_t n)
{
  return gdb::unique_xmalloc_ptr<char> (xstrndup (str, n));
}

/* An overload of operator+= for adding gdb::unique_xmalloc_ptr<char> to a
   std::string.  */

static inline std::string &
operator+= (std::string &lhs, const gdb::unique_xmalloc_ptr<char> &rhs)
{
  return lhs += rhs.get ();
}

/* An overload of operator+ for adding gdb::unique_xmalloc_ptr<char> to a
   std::string.  */

static inline std::string
operator+ (const std::string &lhs, const gdb::unique_xmalloc_ptr<char> &rhs)
{
  return lhs + rhs.get ();
}

#endif /* COMMON_GDB_UNIQUE_PTR_H */

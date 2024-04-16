/* Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_GDB_XFREE_H
#define GDBSUPPORT_GDB_XFREE_H

#include "gdbsupport/poison.h"

/* GDB uses this instead of 'free', it detects when it is called on an
   invalid type.  */

template <typename T>
static void
xfree (T *ptr)
{
  static_assert (IsFreeable<T>::value, "Trying to use xfree with a non-POD \
data type.  Use operator delete instead.");

  if (ptr != NULL)
#ifdef GNULIB_NAMESPACE
    GNULIB_NAMESPACE::free (ptr);	/* ARI: free */
#else
    free (ptr);				/* ARI: free */
#endif
}

#endif /* GDBSUPPORT_GDB_XFREE_H */

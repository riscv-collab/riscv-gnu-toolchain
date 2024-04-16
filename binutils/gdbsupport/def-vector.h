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

#ifndef COMMON_DEF_VECTOR_H
#define COMMON_DEF_VECTOR_H

#include <vector>
#include "gdbsupport/default-init-alloc.h"

namespace gdb {

/* A vector that uses an allocator that default constructs using
   default-initialization rather than value-initialization.  The idea
   is to use this when you don't want zero-initialization of elements
   of vectors of trivial types.  E.g., byte buffers.  */

template<typename T> using def_vector
  = std::vector<T, gdb::default_init_allocator<T>>;

} /* namespace gdb */

#endif /* COMMON_DEF_VECTOR_H */

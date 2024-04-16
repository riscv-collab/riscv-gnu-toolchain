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

#ifndef COMMON_BYTE_VECTOR_H
#define COMMON_BYTE_VECTOR_H

#include "gdbsupport/def-vector.h"

namespace gdb {

/* byte_vector is a gdb_byte std::vector with a custom allocator that
   unlike std::vector<gdb_byte> does not zero-initialize new elements
   by default when the vector is created/resized.  This is what you
   usually want when working with byte buffers, since if you're
   creating or growing a buffer you'll most surely want to fill it in
   with data, in which case zero-initialization would be a
   pessimization.  For example:

     gdb::byte_vector buf (some_large_size);
     fill_with_data (buf.data (), buf.size ());

   On the odd case you do need zero initialization, then you can still
   call the overloads that specify an explicit value, like:

     gdb::byte_vector buf (some_initial_size, 0);
     buf.resize (a_bigger_size, 0);

   (Or use std::vector<gdb_byte> instead.)

   Note that unlike std::vector<gdb_byte>, function local
   gdb::byte_vector objects constructed with an initial size like:

     gdb::byte_vector buf (some_size);
     fill_with_data (buf.data (), buf.size ());

   usually compile down to the exact same as:

     std::unique_ptr<byte[]> buf (new gdb_byte[some_size]);
     fill_with_data (buf.get (), some_size);

   with the former having the advantage of being a bit more readable,
   and providing the whole std::vector API, if you end up needing it.
*/
using byte_vector = gdb::def_vector<gdb_byte>;
using char_vector = gdb::def_vector<char>;

} /* namespace gdb */

#endif /* COMMON_DEF_VECTOR_H */

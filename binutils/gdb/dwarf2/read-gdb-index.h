/* Reading code for .gdb_index

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#ifndef DWARF2_READ_GDB_INDEX_H
#define DWARF2_READ_GDB_INDEX_H

#include "gdbsupport/function-view.h"

struct dwarf2_per_bfd;
struct dwarf2_per_objfile;
struct dwz_file;
struct objfile;

/* Callback types for dwarf2_read_gdb_index.  */

typedef gdb::function_view
    <gdb::array_view<const gdb_byte>(objfile *, dwarf2_per_bfd *)>
    get_gdb_index_contents_ftype;
typedef gdb::function_view
    <gdb::array_view<const gdb_byte>(objfile *, dwz_file *)>
    get_gdb_index_contents_dwz_ftype;

/* Read .gdb_index.  If everything went ok, initialize the "quick"
   elements of all the CUs and return 1.  Otherwise, return 0.  */

int dwarf2_read_gdb_index
  (dwarf2_per_objfile *per_objfile,
   get_gdb_index_contents_ftype get_gdb_index_contents,
   get_gdb_index_contents_dwz_ftype get_gdb_index_contents_dwz);

#endif /* DWARF2_READ_GDB_INDEX_H */

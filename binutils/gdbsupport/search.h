/* Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_SEARCH_H
#define COMMON_SEARCH_H

#include "gdbsupport/function-view.h"

/* This is needed by the unit test, so appears here.  */
#define SEARCH_CHUNK_SIZE 16000

/* The type of a callback function that can be used to read memory.
   Note that target_read_memory is not used here, because gdbserver
   wants to be able to examine trace data when searching, and
   target_read_memory does not do this.  */

typedef bool target_read_memory_ftype (CORE_ADDR, gdb_byte *, size_t);

/* Utility implementation of searching memory.  */
extern int simple_search_memory
  (gdb::function_view<target_read_memory_ftype> read_memory,
   CORE_ADDR start_addr,
   ULONGEST search_space_len,
   const gdb_byte *pattern,
   ULONGEST pattern_len,
   CORE_ADDR *found_addrp);

#endif /* COMMON_SEARCH_H */

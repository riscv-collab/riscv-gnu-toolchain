/* gdb_file_up, an RAII wrapper around FILE.
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_GDB_FILE
#define GDBSUPPORT_GDB_FILE

#include <memory>
#include <stdio.h>

struct gdb_file_deleter
{
  void operator() (FILE *file) const
  {
    fclose (file);
  }
};

/* A unique pointer to a FILE.  */

typedef std::unique_ptr<FILE, gdb_file_deleter> gdb_file_up;

#endif

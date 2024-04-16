/* Symbol manipulating routines for the remote server for GDB.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "gdbsupport/symbol.h"

/* See gdbsupport/symbol.h.  */

int
find_minimal_symbol_address (const char *name, CORE_ADDR *addr,
			     struct objfile *objfile)
{
  gdb_assert (objfile == NULL);

  return look_up_one_symbol (name, addr, 1) != 1;
}

/* Hash table wrappers for gdb.
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

#include "common-defs.h"
#include "gdb-hashtab.h"

/* Allocation function for the libiberty hash table which uses an
   obstack.  The obstack is passed as DATA.  */

void *
hashtab_obstack_allocate (void *data, size_t size, size_t count)
{
  size_t total = size * count;
  void *ptr = obstack_alloc ((struct obstack *) data, total);

  memset (ptr, 0, total);
  return ptr;
}

/* Trivial deallocation function for the libiberty splay tree and hash
   table - don't deallocate anything.  Rely on later deletion of the
   obstack.  DATA will be the obstack, although it is not needed
   here.  */

void
dummy_obstack_deallocate (void *object, void *data)
{
  return;
}

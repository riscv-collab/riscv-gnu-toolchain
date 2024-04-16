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

#ifndef GDBSUPPORT_GDB_HASHTAB_H
#define GDBSUPPORT_GDB_HASHTAB_H

#include "hashtab.h"

/* A deleter for a hash table.  */
struct htab_deleter
{
  void operator() (htab *ptr) const
  {
    htab_delete (ptr);
  }
};

/* A unique_ptr wrapper for htab_t.  */
typedef std::unique_ptr<htab, htab_deleter> htab_up;

/* A wrapper for 'delete' that can used as a hash table entry deletion
   function.  */
template<typename T>
void
htab_delete_entry (void *ptr)
{
  delete (T *) ptr;
}

/* Allocation and deallocation functions for the libiberty hash table
   which use obstacks.  */
void *hashtab_obstack_allocate (void *data, size_t size, size_t count);
void dummy_obstack_deallocate (void *object, void *data);

#endif /* GDBSUPPORT_GDB_HASHTAB_H */

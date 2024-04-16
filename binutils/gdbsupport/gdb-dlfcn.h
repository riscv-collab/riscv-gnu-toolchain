/* Platform independent shared object routines for GDB.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef GDB_DLFCN_H
#define GDB_DLFCN_H

/* A deleter that closes an open dynamic library.  */

struct dlclose_deleter
{
  void operator() (void *handle) const;
};

/* A unique pointer that points to a dynamic library.  */

typedef std::unique_ptr<void, dlclose_deleter> gdb_dlhandle_up;

/* Load the dynamic library file named FILENAME, and return a handle
   for that dynamic library.  Throw an error if the loading fails for
   any reason.  */

gdb_dlhandle_up gdb_dlopen (const char *filename);

/* Return the address of the symbol named SYMBOL inside the shared
   library whose handle is HANDLE.  Return NULL when the symbol could
   not be found.  */

void *gdb_dlsym (const gdb_dlhandle_up &handle, const char *symbol);

/* Return non-zero if the dynamic library functions are available on
   this platform.  */

int is_dl_available(void);

#endif /* GDB_DLFCN_H */

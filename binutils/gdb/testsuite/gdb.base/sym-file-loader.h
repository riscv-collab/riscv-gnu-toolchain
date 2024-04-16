/* Copyright 2013-2024 Free Software Foundation, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __SYM_FILE_LOADER__
#define __SYM_FILE_LOADER__

struct library;

/* Mini shared library loader.  No reallocation is performed
   for the sake of simplicity.  */

/* Load a library.  */

struct library *load_shlib (const char *file);

/* Unload a library.  */

void unload_shlib (struct library *lib);

/* Lookup the address of FUNC.  */

int lookup_function (struct library *lib, const char *func, void **addr);

/* Return the library's loaded text address.  */

int get_text_addr (struct library *lib, void **text_addr);

#endif

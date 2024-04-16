/* This testcase is part of GDB, the GNU debugger.
   Copyright 2012-2024 Free Software Foundation, Inc.

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

#ifdef SHLIB_NAME
# include <dlfcn.h>
#endif

#include <assert.h>
#include <stddef.h>

#include "print-file-var.h"

START_EXTERN_C

extern int get_version_1 (void);
extern int get_version_2 (void);

END_EXTERN_C

#if VERSION_ID_MAIN
ATTRIBUTE_VISIBILITY int this_version_id = 55;
#endif

int
main (void)
{
#if VERSION_ID_MAIN
  int vm = this_version_id;
#endif
  int v1 = get_version_1 ();
  int v2;

#ifdef SHLIB_NAME
  {
    void *handle = dlopen (SHLIB_NAME, RTLD_LAZY);
    int (*getver2) (void);

    assert (handle != NULL);

    getver2 = (int (*)(void)) dlsym (handle, "get_version_2");

    v2 = getver2 ();
  }
#else
  v2 = get_version_2 ();
#endif

  return 0; /* STOP */
}

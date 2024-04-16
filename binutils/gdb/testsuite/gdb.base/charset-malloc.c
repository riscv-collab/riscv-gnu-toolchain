/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

   Contributed by Red Hat, originally written by Jim Blandy.

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

   Please email any bugs, comments, and/or additions to this file to:
   bug-gdb@gnu.org  */

/* charset.c file cannot use a system include file as it has its own wchar_t
   definition which would be in a conflict.  Use this separate compilation
   unit.  */

#include <stdlib.h>

void
malloc_stub (void)
{
  /* charset.exp wants to allocate memory for constants.  So make sure malloc
     gets linked into the program.  */
  void *p = malloc (1);
  free (p);
}

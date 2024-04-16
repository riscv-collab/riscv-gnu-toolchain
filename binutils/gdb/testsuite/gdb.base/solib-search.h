/* This test program is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

#ifndef GDB_BASE_SOLIB_SEARCH_H
#define GDB_BASE_SOLIB_SEARCH_H

/* These functions create a call chain that traverses both libs.  */

extern void lib1_func1 (void);
extern void lib1_func3 (void);

extern void lib2_func2 (void);
extern void lib2_func4 (void);

extern void break_here (void);

#endif /* GDB_BASE_SOLIB_SEARCH_H */

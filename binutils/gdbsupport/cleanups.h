/* Cleanups.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_CLEANUPS_H
#define COMMON_CLEANUPS_H

/* Outside of cleanups.c, this is an opaque type.  */
struct cleanup;

/* NOTE: cagney/2000-03-04: This typedef is strictly for the
   make_cleanup function declarations below.  Do not use this typedef
   as a cast when passing functions into the make_cleanup() code.
   Instead either use a bounce function or add a wrapper function.
   Calling a f(char*) function with f(void*) is non-portable.  */
typedef void (make_cleanup_ftype) (void *);

/* Function type for the dtor in make_cleanup_dtor.  */
typedef void (make_cleanup_dtor_ftype) (void *);

extern struct cleanup *make_final_cleanup (make_cleanup_ftype *, void *);

extern void do_final_cleanups ();

#endif /* COMMON_CLEANUPS_H */

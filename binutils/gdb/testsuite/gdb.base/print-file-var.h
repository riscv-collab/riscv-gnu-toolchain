/* This testcase is part of GDB, the GNU debugger.
   Copyright 2019-2024 Free Software Foundation, Inc.

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

#ifndef PRINT_FILE_VAR_H
#define PRINT_FILE_VAR_H

#if HIDDEN
# define ATTRIBUTE_VISIBILITY __attribute__((visibility ("hidden")))
#else
# define ATTRIBUTE_VISIBILITY
#endif

#ifdef __cplusplus
# define START_EXTERN_C extern "C" {
# define END_EXTERN_C }
#else
# define START_EXTERN_C
# define END_EXTERN_C
#endif

#endif /* PRINT_FILE_VAR_H */

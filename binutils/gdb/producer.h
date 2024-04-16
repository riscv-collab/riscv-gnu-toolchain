/* Producer string parsers for GDB.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#ifndef PRODUCER_H
#define PRODUCER_H

/* Check for GCC >= 4.x according to the symtab->producer string.  Return minor
   version (x) of 4.x in such case.  If it is not GCC or it is GCC older than
   4.x return -1.  If it is GCC 5.x or higher return INT_MAX.  */
extern int producer_is_gcc_ge_4 (const char *producer);

/* Returns nonzero if the given PRODUCER string is GCC and sets the MAJOR
   and MINOR versions when not NULL.  Returns zero if the given PRODUCER
   is NULL or it isn't GCC.  */
extern int producer_is_gcc (const char *producer, int *major, int *minor);

/* Returns nonzero if the given PRODUCER string is GAS and sets the MAJOR
   and MINOR versions when not NULL.  Returns zero if the given PRODUCER
   is NULL or it isn't GAS.  */
bool producer_is_gas (const char *producer, int *major, int *minor);

/* Check for Intel compilers >= 19.0.  */
extern bool producer_is_icc_ge_19 (const char *producer);

/* Returns true if the given PRODUCER string is Intel or false
   otherwise.  Sets the MAJOR and MINOR versions when not NULL.  */
extern bool producer_is_icc (const char *producer, int *major, int *minor);

/* Returns true if the given PRODUCER string is LLVM (clang/flang) or
   false otherwise.*/
extern bool producer_is_llvm (const char *producer);

/* Returns true if the given PRODUCER string is clang, false otherwise.
   Sets MAJOR and MINOR accordingly, if not NULL.  */
extern bool producer_is_clang (const char *producer, int *major, int *minor);

#endif

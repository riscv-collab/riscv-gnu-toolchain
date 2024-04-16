/* Helper routines for C support in GDB.
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef C_SUPPORT_H
#define C_SUPPORT_H

#include "safe-ctype.h"

/* Like ISALPHA, but also returns true for the union of all UTF-8
   multi-byte sequence bytes and non-ASCII characters in
   extended-ASCII charsets (e.g., Latin1).  I.e., returns true if the
   high bit is set.  Note that not all UTF-8 ranges are allowed in C++
   identifiers, but we don't need to be pedantic so for simplicity we
   ignore that here.  Plus this avoids the complication of actually
   knowing what was the right encoding.  */

static inline bool
c_ident_is_alpha (unsigned char ch)
{
  return ISALPHA (ch) || ch >= 0x80;
}

/* Similarly, but Like ISALNUM.  */

static inline bool
c_ident_is_alnum (unsigned char ch)
{
  return ISALNUM (ch) || ch >= 0x80;
}

#endif /* C_SUPPORT_H */

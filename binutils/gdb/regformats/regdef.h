/* Register protocol definition structures for the GNU Debugger
   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#ifndef REGFORMATS_REGDEF_H
#define REGFORMATS_REGDEF_H

namespace gdb {

struct reg
{
  reg (int _offset)
    : name (""),
      offset (_offset),
      size (0)
  {}

  reg (const char *_name, int _offset, int _size)
    : name (_name),
      offset (_offset),
      size (_size)
  {}

  /* The name of this register - NULL for pad entries.  */
  const char *name;

  /* At the moment, both of the following bit counts must be divisible
     by eight (to match the representation as two hex digits) and divisible
     by the size of a byte (to match the layout of each register in
     memory).  */

  /* The offset (in bits) of the value of this register in the buffer.  */
  int offset;

  /* The size (in bits) of the value of this register, as transmitted.  */
  int size;

  bool operator== (const reg &other) const
  {
    return (strcmp (name, other.name) == 0
	    && offset == other.offset
	    && size == other.size);
  }

  bool operator!= (const reg &other) const
  {
    return !(*this == other);
  }
};

} /* namespace gdb */

#endif /* REGFORMATS_REGDEF_H */

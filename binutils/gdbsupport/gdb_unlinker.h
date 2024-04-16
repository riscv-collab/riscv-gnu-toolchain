/* Unlinking class

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_GDB_UNLINKER_H
#define COMMON_GDB_UNLINKER_H

namespace gdb
{

/* An object of this class holds a filename and, when the object goes
   of scope, the file is removed using unlink.

   A user of this class can request that the file be preserved using
   the "keep" method.  */
class unlinker
{
 public:

  unlinker (const char *filename) ATTRIBUTE_NONNULL (2)
    : m_filename (filename)
  {
    gdb_assert (filename != NULL);
  }

  ~unlinker ()
  {
    if (m_filename != NULL)
      unlink (m_filename);
  }

  /* Keep the file, rather than unlink it.  */
  void keep ()
  {
    m_filename = NULL;
  }

 private:

  const char *m_filename;
};

}

#endif /* COMMON_GDB_UNLINKER_H */

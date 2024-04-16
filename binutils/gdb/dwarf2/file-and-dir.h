/* DWARF file and directory

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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

#ifndef GDB_DWARF2_FILE_AND_DIR_H
#define GDB_DWARF2_FILE_AND_DIR_H

#include "objfiles.h"
#include "source.h"
#include <string>

/* The return type of find_file_and_directory.  Note, the enclosed
   string pointers are only valid while this object is valid.  */

struct file_and_directory
{
  file_and_directory (const char *name, const char *dir)
    : m_name (name),
      m_comp_dir (dir)
  {
  }

  /* Return true if the file name is unknown.  */
  bool is_unknown () const
  {
    return m_name == nullptr;
  }

  /* Set the compilation directory.  */
  void set_comp_dir (std::string &&dir)
  {
    m_comp_dir_storage = std::move (dir);
    m_comp_dir = nullptr;
  }

  /* Fetch the compilation directory.  This may return NULL in some
     circumstances.  Note that the return value here is not stable --
     it may change if this object is moved.  To get a stable pointer,
     you should call intern_comp_dir.  */
  const char *get_comp_dir () const
  {
    if (!m_comp_dir_storage.empty ())
      return m_comp_dir_storage.c_str ();
    return m_comp_dir;
  }

  /* If necessary, intern the compilation directory using OBJFILE's
     string cache.  Returns the compilation directory.  */
  const char *intern_comp_dir (struct objfile *objfile)
  {
    if (!m_comp_dir_storage.empty ())
      {
	m_comp_dir = objfile->intern (m_comp_dir_storage);
	m_comp_dir_storage.clear ();
      }
    return m_comp_dir;
  }

  /* Fetch the filename.  This never returns NULL.  */
  const char *get_name () const
  {
    return m_name == nullptr ? "<unknown>" : m_name;
  }

  /* Set the filename.  */
  void set_name (gdb::unique_xmalloc_ptr<char> name)
  {
    m_name_storage = std::move (name);
    m_name = m_name_storage.get ();
  }

  /* Return the full name, computing it if necessary.  */
  const char *get_fullname ()
  {
    if (m_fullname == nullptr)
      m_fullname = find_source_or_rewrite (get_name (), get_comp_dir ());
    return m_fullname.get ();
  }

  /* Forget the full name.  */
  void forget_fullname ()
  {
    m_fullname.reset ();
  }

private:

  /* The filename.  */
  const char *m_name;

  /* Storage for the filename, if needed.  */
  gdb::unique_xmalloc_ptr<char> m_name_storage;

  /* The compilation directory.  NULL if not known.  If we needed to
     compute a new string, it will be stored in the comp_dir_storage
     member, and this will be NULL.  Otherwise, points directly to the
     DW_AT_comp_dir string attribute.  */
  const char *m_comp_dir;

  /* The compilation directory, if it needed to be allocated.  */
  std::string m_comp_dir_storage;

  /* The full name.  */
  gdb::unique_xmalloc_ptr<char> m_fullname;
};

#endif /* GDB_DWARF2_FILE_AND_DIR_H */

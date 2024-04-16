/* RAII wrapper for buildargv

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_BUILDARGV_H
#define GDBSUPPORT_BUILDARGV_H

#include "libiberty.h"

/* A wrapper for an array of char* that was allocated in the way that
   'buildargv' does, and should be freed with 'freeargv'.  */

class gdb_argv
{
public:

  /* A constructor that initializes to NULL.  */

  gdb_argv ()
    : m_argv (NULL)
  {
  }

  /* A constructor that calls buildargv on STR.  STR may be NULL, in
     which case this object is initialized with a NULL array.  */

  explicit gdb_argv (const char *str)
    : m_argv (NULL)
  {
    reset (str);
  }

  /* A constructor that takes ownership of an existing array.  */

  explicit gdb_argv (char **array)
    : m_argv (array)
  {
  }

  gdb_argv (const gdb_argv &) = delete;
  gdb_argv &operator= (const gdb_argv &) = delete;

  gdb_argv &operator= (gdb_argv &&other)
  {
    freeargv (m_argv);
    m_argv = other.m_argv;
    other.m_argv = nullptr;
    return *this;
  }

  gdb_argv (gdb_argv &&other)
  {
    m_argv = other.m_argv;
    other.m_argv = nullptr;
  }

  ~gdb_argv ()
  {
    freeargv (m_argv);
  }

  /* Call buildargv on STR, storing the result in this object.  Any
     previous state is freed.  STR may be NULL, in which case this
     object is reset with a NULL array.  If buildargv fails due to
     out-of-memory, call malloc_failure.  Therefore, the value is
     guaranteed to be non-NULL, unless the parameter itself is
     NULL.  */

  void reset (const char *str)
  {
    char **argv = buildargv (str);
    freeargv (m_argv);
    m_argv = argv;
  }

  /* Return the underlying array.  */

  char **get ()
  {
    return m_argv;
  }

  const char * const * get () const
  {
    return m_argv;
  }

  /* Return the underlying array, transferring ownership to the
     caller.  */

  ATTRIBUTE_UNUSED_RESULT char **release ()
  {
    char **result = m_argv;
    m_argv = NULL;
    return result;
  }

  /* Return the number of items in the array.  */

  int count () const
  {
    return countargv (m_argv);
  }

  /* Index into the array.  */

  char *operator[] (int arg)
  {
    gdb_assert (m_argv != NULL);
    return m_argv[arg];
  }

  /* Return the arguments array as an array view.  */

  gdb::array_view<char *> as_array_view ()
  {
    return gdb::array_view<char *> (this->get (), this->count ());
  }

  gdb::array_view<const char * const> as_array_view () const
  {
    return gdb::array_view<const char * const> (this->get (), this->count ());
  }

  /* Append arguments to this array.  */
  void append (gdb_argv &&other)
  {
    int size = count ();
    int argc = other.count ();
    m_argv = XRESIZEVEC (char *, m_argv, (size + argc + 1));

    for (int argi = 0; argi < argc; argi++)
      {
	/* Transfer ownership of the string.  */
	m_argv[size++] = other.m_argv[argi];
	/* Ensure that destruction of OTHER works correctly.  */
	other.m_argv[argi] = nullptr;
      }
    m_argv[size] = nullptr;
  }

  /* Append arguments to this array.  */
  void append (const gdb_argv &other)
  {
    int size = count ();
    int argc = other.count ();
    m_argv = XRESIZEVEC (char *, m_argv, (size + argc + 1));

    for (int argi = 0; argi < argc; argi++)
      m_argv[size++] = xstrdup (other.m_argv[argi]);
    m_argv[size] = nullptr;
  }

  /* The iterator type.  */

  typedef char **iterator;

  /* Return an iterator pointing to the start of the array.  */

  iterator begin ()
  {
    return m_argv;
  }

  /* Return an iterator pointing to the end of the array.  */

  iterator end ()
  {
    return m_argv + count ();
  }

  bool operator!= (std::nullptr_t)
  {
    return m_argv != NULL;
  }

  bool operator== (std::nullptr_t)
  {
    return m_argv == NULL;
  }

private:

  /* The wrapped array.  */

  char **m_argv;
};

#endif /* GDBSUPPORT_BUILDARGV_H */

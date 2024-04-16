/* scoped_fd, automatically close a file descriptor

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_SCOPED_FD_H
#define COMMON_SCOPED_FD_H

#include <unistd.h>
#include "gdb_file.h"

/* A smart-pointer-like class to automatically close a file descriptor.  */

class scoped_fd
{
public:
  explicit scoped_fd (int fd = -1) noexcept : m_fd (fd) {}

  scoped_fd (scoped_fd &&other) noexcept
    : m_fd (other.m_fd)
  {
    other.m_fd = -1;
  }

  ~scoped_fd ()
  {
    if (m_fd >= 0)
      close (m_fd);
  }

  scoped_fd &operator= (scoped_fd &&other)
  {
    if (m_fd != other.m_fd)
      {
	if (m_fd >= 0)
	  close (m_fd);
	m_fd = other.m_fd;
	other.m_fd = -1;
      }
    return *this;
  }

  DISABLE_COPY_AND_ASSIGN (scoped_fd);

  ATTRIBUTE_UNUSED_RESULT int release () noexcept
  {
    int fd = m_fd;
    m_fd = -1;
    return fd;
  }

  /* Like release, but return a gdb_file_up that owns the file
     descriptor.  On success, this scoped_fd will be released.  On
     failure, return NULL and leave this scoped_fd in possession of
     the fd.  */
  gdb_file_up to_file (const char *mode) noexcept
  {
    gdb_file_up result (fdopen (m_fd, mode));
    if (result != nullptr)
      m_fd = -1;
    return result;
  }

  int get () const noexcept
  {
    return m_fd;
  }

private:
  int m_fd;
};

#endif /* COMMON_SCOPED_FD_H */

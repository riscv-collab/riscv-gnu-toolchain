/* scoped_mmap, automatically unmap files

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

#ifndef COMMON_SCOPED_MMAP_H
#define COMMON_SCOPED_MMAP_H

#ifdef HAVE_SYS_MMAN_H

#include <sys/mman.h>

/* A smart-pointer-like class to mmap() and automatically munmap() a memory
   mapping.  */

class scoped_mmap
{
public:
  scoped_mmap () noexcept : m_mem (MAP_FAILED), m_length (0) {}
  scoped_mmap (void *addr, size_t length, int prot, int flags, int fd,
	       off_t offset) noexcept : m_length (length)
  {
    m_mem = mmap (addr, m_length, prot, flags, fd, offset);
  }

  ~scoped_mmap ()
  {
    destroy ();
  }

  scoped_mmap (scoped_mmap &&rhs) noexcept
    : m_mem (rhs.m_mem),
      m_length (rhs.m_length)
  {
    rhs.m_mem = MAP_FAILED;
    rhs.m_length = 0;
  }

  DISABLE_COPY_AND_ASSIGN (scoped_mmap);

  ATTRIBUTE_UNUSED_RESULT void *release () noexcept
  {
    void *mem = m_mem;
    m_mem = MAP_FAILED;
    m_length = 0;
    return mem;
  }

  void reset (void *addr, size_t length, int prot, int flags, int fd,
	      off_t offset) noexcept
  {
    destroy ();

    m_length = length;
    m_mem = mmap (addr, m_length, prot, flags, fd, offset);
  }

  size_t size () const noexcept { return m_length; }
  void *get () const noexcept { return m_mem; }

private:
  void destroy ()
  {
    if (m_mem != MAP_FAILED)
      munmap (m_mem, m_length);
  }

  void *m_mem;
  size_t m_length;
};

/* Map FILENAME in memory.  Throw an error if anything goes wrong.  */
scoped_mmap mmap_file (const char *filename);

#endif /* HAVE_SYS_MMAN_H */

#endif /* COMMON_SCOPED_MMAP_H */

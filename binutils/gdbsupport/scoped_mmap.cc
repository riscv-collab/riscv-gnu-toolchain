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

#include "common-defs.h"
#include "scoped_mmap.h"
#include "scoped_fd.h"
#include "gdbsupport/filestuff.h"

#ifdef HAVE_SYS_MMAN_H

scoped_mmap
mmap_file (const char *filename)
{
  scoped_fd fd = gdb_open_cloexec (filename, O_RDONLY, 0);
  if (fd.get () < 0)
    perror_with_name (("open"));

  off_t size = lseek (fd.get (), 0, SEEK_END);
  if (size < 0)
    perror_with_name (("lseek"));

  /* We can't map an empty file.  */
  if (size == 0)
    error (_("file to mmap is empty"));

  scoped_mmap mmapped_file (nullptr, size, PROT_READ, MAP_PRIVATE, fd.get (), 0);
  if (mmapped_file.get () == MAP_FAILED)
    perror_with_name (("mmap"));

  return mmapped_file;
}

#endif /* HAVE_SYS_MMAN_H */

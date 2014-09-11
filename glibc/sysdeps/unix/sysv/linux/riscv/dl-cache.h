/* Support for reading /etc/ld.so.cache files written by Linux ldconfig.
   Copyright (C) 2003, 2007 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <ldconfig.h>

#define add_system_dir(dir) \
  do								\
    {								\
      size_t len = strlen (dir);				\
      char path[len + 3];					\
      memcpy (path, dir, len + 1);				\
      if (len >= 6						\
	  && (! memcmp (path + len - 6, "/lib64", 6)		\
	      || ! memcmp (path + len - 6, "/lib32", 6)))	\
	{							\
	  len -= 2;						\
	  path[len] = '\0';					\
	}							\
      add_dir (path);						\
      if (len >= 4 && ! memcmp (path + len - 4, "/lib", 4))	\
	{							\
	  memcpy (path + len, "32", 3);				\
	  add_dir (path);					\
	  memcpy (path + len, "64", 3);				\
	  add_dir (path);					\
	}							\
    } while (0)

#include_next <dl-cache.h>

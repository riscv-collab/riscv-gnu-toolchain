/* Low-level file-handling.
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

#ifndef COMMON_FILESTUFF_H
#define COMMON_FILESTUFF_H

#include <dirent.h>
#include <fcntl.h>
#include "gdb_file.h"
#include "scoped_fd.h"

/* Note all the file descriptors which are open when this is called.
   These file descriptors will not be closed by close_most_fds.  */

extern void notice_open_fds (void);

/* Mark a file descriptor as inheritable across an exec.  */

extern void mark_fd_no_cloexec (int fd);

/* Mark a file descriptor as no longer being inheritable across an
   exec.  This is only meaningful when FD was previously passed to
   mark_fd_no_cloexec.  */

extern void unmark_fd_no_cloexec (int fd);

/* Close all open file descriptors other than those marked by
   'notice_open_fds', and stdin, stdout, and stderr.  Errors that
   occur while closing are ignored.  */

extern void close_most_fds (void);

/* Like 'open', but ensures that the returned file descriptor has the
   close-on-exec flag set.  */

extern scoped_fd gdb_open_cloexec (const char *filename, int flags,
				   /* mode_t */ unsigned long mode);

/* Like mkstemp, but ensures that the file descriptor is
   close-on-exec.  */

static inline scoped_fd
gdb_mkostemp_cloexec (char *name_template, int flags = 0)
{
  /* gnulib provides a mkostemp replacement if needed.  */
  return scoped_fd (mkostemp (name_template, flags | O_CLOEXEC));
}

/* Convenience wrapper for the above, which takes the filename as an
   std::string.  */

static inline scoped_fd
gdb_open_cloexec (const std::string &filename, int flags,
		  /* mode_t */ unsigned long mode)
{
  return gdb_open_cloexec (filename.c_str (), flags, mode);
}

/* Like 'fopen', but ensures that the returned file descriptor has the
   close-on-exec flag set.  */

extern gdb_file_up gdb_fopen_cloexec (const char *filename,
				      const char *opentype);

/* Convenience wrapper for the above, which takes the filename as an
   std::string.  */

static inline gdb_file_up
gdb_fopen_cloexec (const std::string &filename, const char *opentype)
{
  return gdb_fopen_cloexec (filename.c_str (), opentype);
}

/* Like 'socketpair', but ensures that the returned file descriptors
   have the close-on-exec flag set.  */

extern int gdb_socketpair_cloexec (int domain, int style, int protocol,
				   int filedes[2]);

/* Like 'socket', but ensures that the returned file descriptor has
   the close-on-exec flag set.  */

extern int gdb_socket_cloexec (int domain, int style, int protocol);

/* Like 'pipe', but ensures that the returned file descriptors have
   the close-on-exec flag set.  */

extern int gdb_pipe_cloexec (int filedes[2]);

struct gdb_dir_deleter
{
  void operator() (DIR *dir) const
  {
    closedir (dir);
  }
};

/* A unique pointer to a DIR.  */

typedef std::unique_ptr<DIR, gdb_dir_deleter> gdb_dir_up;

/* Return true if the file NAME exists and is a regular file.
   If the result is false then *ERRNO_PTR is set to a useful value assuming
   we're expecting a regular file.  */
extern bool is_regular_file (const char *name, int *errno_ptr);


/* A cheap (as in low-quality) recursive mkdir.  Try to create all the
   parents directories up to DIR and DIR itself.  Stop if we hit an
   error along the way.  There is no attempt to remove created
   directories in case of failure.

   Returns false on failure and sets errno.  */

extern bool mkdir_recursive (const char *dir);

/* Read the entire content of file PATH into an std::string.  */

extern std::optional<std::string> read_text_file_to_string (const char *path);

#endif /* COMMON_FILESTUFF_H */

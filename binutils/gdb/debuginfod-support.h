/* debuginfod utilities for GDB.
   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#ifndef DEBUGINFOD_SUPPORT_H
#define DEBUGINFOD_SUPPORT_H

#include "gdbsupport/scoped_fd.h"

/* Query debuginfod servers for a source file associated with an
   executable with BUILD_ID.  BUILD_ID can be given as a binary blob or
   a null-terminated string.  If given as a binary blob, BUILD_ID_LEN
   should be the number of bytes.  If given as a null-terminated string,
   BUILD_ID_LEN should be 0.

   SRC_PATH should be the source file's absolute path that includes the
   compilation directory of the CU associated with the source file.
   For example if a CU's compilation directory is `/my/build` and the
   source file path is `/my/source/foo.c`, then SRC_PATH should be
   `/my/build/../source/foo.c`.

   If the file is successfully retrieved, return a file descriptor and store
   the file's local path in DESTNAME.  If unsuccessful, print an error message
   and return a negative errno.  If GDB is not built with debuginfod, this
   function returns -ENOSYS.  */

extern scoped_fd
debuginfod_source_query (const unsigned char *build_id,
			 int build_id_len,
			 const char *src_path,
			 gdb::unique_xmalloc_ptr<char> *destname);

/* Query debuginfod servers for a debug info file with BUILD_ID.
   BUILD_ID can be given as a binary blob or a null-terminated string.
   If given as a binary blob, BUILD_ID_LEN should be the number of bytes.
   If given as a null-terminated string, BUILD_ID_LEN should be 0.

   FILENAME should be the name or path of the main binary associated with
   the separate debug info.  It is used for printing messages to the user.

   If the file is successfully retrieved, return a file descriptor and store
   the file's local path in DESTNAME.  If unsuccessful, print an error message
   and return a negative errno.  If GDB is not built with debuginfod, this
   function returns -ENOSYS.  */

extern scoped_fd
debuginfod_debuginfo_query (const unsigned char *build_id,
			    int build_id_len,
			    const char *filename,
			    gdb::unique_xmalloc_ptr<char> *destname);

/* Query debuginfod servers for an executable file with BUILD_ID.
   BUILD_ID can be given as a binary blob or a null-terminated string.
   If given as a binary blob, BUILD_ID_LEN should be the number of bytes.
   If given as a null-terminated string, BUILD_ID_LEN should be 0.

   FILENAME should be the name or path associated with the executable.
   It is used for printing messages to the user.

   If the file is successfully retrieved, return a file descriptor and store
   the file's local path in DESTNAME.  If unsuccessful, print an error message
   and return a negative errno.  If GDB is not built with debuginfod, this
   function returns -ENOSYS.  */

extern scoped_fd debuginfod_exec_query (const unsigned char *build_id,
					int build_id_len,
					const char *filename,
					gdb::unique_xmalloc_ptr<char>
					  *destname);

/* Query debuginfod servers for the binary contents of a ELF/DWARF section
   from a file matching BUILD_ID.  BUILD_ID can be given as a binary blob
   or a null-terminated string.  If given as a binary blob, BUILD_ID_LEN
   should be the number of bytes.  If given as a null-terminated string,
   BUILD_ID_LEN should be 0.

   FILENAME should be the name or path associated with the file matching
   BUILD_ID.  It is used for printing messages to the user.

   SECTION_NAME should be the name of an ELF/DWARF section.

   If the file is successfully retrieved, return a file descriptor and store
   the file's local path in DESTNAME.  If unsuccessful, print an error message
   and return a negative errno.  If GDB is not built with debuginfod or
   libdebuginfod does not support section queries, this function returns
   -ENOSYS.  */

extern scoped_fd debuginfod_section_query (const unsigned char *build_id,
					   int build_id_len,
					   const char *filename,
					   const char *section_name,
					   gdb::unique_xmalloc_ptr<char>
					     *destname);
#endif /* DEBUGINFOD_SUPPORT_H */

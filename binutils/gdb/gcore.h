/* Support for reading/writing gcore files.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#if !defined (GCORE_H)
#define GCORE_H 1

#include "gdb_bfd.h"

struct thread_info;

extern gdb_bfd_ref_ptr create_gcore_bfd (const char *filename);
extern void write_gcore_file (bfd *obfd);
extern int objfile_find_memory_regions (struct target_ops *self,
					find_memory_region_ftype func,
					void *obfd);

/* Find the signalled thread.  In case there's more than one signalled
   thread, prefer the current thread, if it is signalled.  If no thread was
   signalled, default to the current thread, unless it has exited, in which
   case return NULL.  */

extern thread_info *gcore_find_signalled_thread ();

#endif /* GCORE_H */

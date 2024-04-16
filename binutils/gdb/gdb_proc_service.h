/* <proc_service.h> replacement for systems that don't have it.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef GDB_PROC_SERVICE_H
#define GDB_PROC_SERVICE_H

#include "gdbsupport/gdb_proc_service.h"

struct thread_info;

/* GDB specific structure that identifies the target process.  */
struct ps_prochandle
{
  /* The LWP we use for memory reads.  */
  thread_info *thread;
};

#endif /* gdb_proc_service.h */

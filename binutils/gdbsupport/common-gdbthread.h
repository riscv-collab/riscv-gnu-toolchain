/* Common multi-process/thread control defs for GDB and gdbserver.
   Copyright (C) 1987-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_COMMON_GDBTHREAD_H
#define COMMON_COMMON_GDBTHREAD_H

struct process_stratum_target;

/* Switch from one thread to another.  */
extern void switch_to_thread (process_stratum_target *proc_target,
			      ptid_t ptid);

#endif /* COMMON_COMMON_GDBTHREAD_H */

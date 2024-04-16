/* Job control and terminal related functions, for GDB and gdbserver
   when running under Unix.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_JOB_CONTROL_H
#define COMMON_JOB_CONTROL_H

/* Do we have job control?  Can be assumed to always be the same
   within a given run of GDB.  Use in gdb/inflow.c and
   gdbsupport/common-inflow.c.  */
extern int job_control;

/* Set the process group of the caller to its own pid, or do nothing
   if we lack job control.  */
extern int gdb_setpgid ();

/* Determine whether we have job control, and set variable JOB_CONTROL
   accordingly.  This function must be called before any use of
   JOB_CONTROL.  */
extern void have_job_control ();

#endif /* COMMON_JOB_CONTROL_H */

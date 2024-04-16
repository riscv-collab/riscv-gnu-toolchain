/* Linux-specific PROCFS manipulation routines.
   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef NAT_LINUX_PROCFS_H
#define NAT_LINUX_PROCFS_H

#include <unistd.h>

/* Return the TGID of LWPID from /proc/pid/status.  Returns -1 if not
   found.  Failure to open the /proc file results in a warning.  */

extern int linux_proc_get_tgid (pid_t lwpid);

/* Return the TracerPid of LWPID from /proc/pid/status.  Returns -1 if
   not found.  Does not warn on failure to open the /proc file.  */

extern pid_t linux_proc_get_tracerpid_nowarn (pid_t lwpid);

/* Detect `T (stopped)' in `/proc/PID/status'.
   Other states including `T (tracing stop)' are reported as false.  */

extern int linux_proc_pid_is_stopped (pid_t pid);

extern int linux_proc_pid_is_trace_stopped_nowarn (pid_t pid);

/* Return non-zero if PID is a zombie.  Failure to open the
   /proc/pid/status file results in a warning.  */

extern int linux_proc_pid_is_zombie (pid_t pid);

/* Return non-zero if PID is a zombie.  Does not warn on failure to
   open the /proc file.  */

extern int linux_proc_pid_is_zombie_nowarn (pid_t pid);

/* Return non-zero if /proc/PID/status indicates that PID is gone
   (i.e., in Z/Zombie or X/Dead state).  Failure to open the /proc
   file is assumed to indicate the thread is gone.  */

extern int linux_proc_pid_is_gone (pid_t pid);

/* Return a string giving the thread's name or NULL if the
   information is unavailable.  The returned value points to a statically
   allocated buffer.  The value therefore becomes invalid at the next
   linux_proc_tid_get_name call.  */

extern const char *linux_proc_tid_get_name (ptid_t ptid);

/* Callback function for linux_proc_attach_tgid_threads.  If the PTID
   thread is not yet known, try to attach to it and return true,
   otherwise return false.  */
typedef int (*linux_proc_attach_lwp_func) (ptid_t ptid);

/* If PID is a tgid, scan the /proc/PID/task/ directory for existing
   threads, and call FUNC for each thread found.  */
extern void linux_proc_attach_tgid_threads (pid_t pid,
					    linux_proc_attach_lwp_func func);

/* Return true if the /proc/PID/task/ directory exists.  */
extern int linux_proc_task_list_dir_exists (pid_t pid);

/* Return the full absolute name of the executable file that was run
   to create the process PID.  The returned value persists until this
   function is next called.  */

extern const char *linux_proc_pid_to_exec_file (int pid);

/* Display possible problems on this system.  Display them only once
   per GDB execution.  */

extern void linux_proc_init_warnings ();

#endif /* NAT_LINUX_PROCFS_H */

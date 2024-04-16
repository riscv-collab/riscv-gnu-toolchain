/* Inferior process information for the remote server for GDB.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_INFERIORS_H
#define GDBSERVER_INFERIORS_H

#include "gdbsupport/gdb_vecs.h"
#include "dll.h"
#include <list>

struct thread_info;
struct regcache;
struct target_desc;
struct sym_cache;
struct breakpoint;
struct raw_breakpoint;
struct fast_tracepoint_jump;
struct process_info_private;

struct process_info
{
  process_info (int pid_, int attached_)
  : pid (pid_), attached (attached_)
  {}

  /* This process' pid.  */
  int pid;

  /* Nonzero if this child process was attached rather than
     spawned.  */
  int attached;

  /* True if GDB asked us to detach from this process, but we remained
     attached anyway.  */
  int gdb_detached = 0;

  /* The symbol cache.  */
  struct sym_cache *symbol_cache = NULL;

  /* The list of memory breakpoints.  */
  struct breakpoint *breakpoints = NULL;

  /* The list of raw memory breakpoints.  */
  struct raw_breakpoint *raw_breakpoints = NULL;

  /* The list of installed fast tracepoints.  */
  struct fast_tracepoint_jump *fast_tracepoint_jumps = NULL;

  /* The list of syscalls to report, or just a single element, ANY_SYSCALL,
     for unfiltered syscall reporting.  */
  std::vector<int> syscalls_to_catch;

  const struct target_desc *tdesc = NULL;

  /* Private target data.  */
  struct process_info_private *priv = NULL;

  /* DLLs that are loaded for this proc.  */
  std::list<dll_info> all_dlls;

  /* Flag to mark that the DLL list has changed.  */
  bool dlls_changed = false;

  /* True if the inferior is starting up (inside startup_inferior),
     and we're nursing it along (through the shell) until it is ready
     to execute its first instruction.  Until that is done, we must
     not access inferior memory or registers, as we haven't determined
     the target architecture/description.  */
  bool starting_up = false;
};

/* Get the pid of PROC.  */

static inline int
pid_of (const process_info *proc)
{
  return proc->pid;
}

/* Return a pointer to the process that corresponds to the current
   thread (current_thread).  It is an error to call this if there is
   no current thread selected.  */

struct process_info *current_process (void);
struct process_info *get_thread_process (const struct thread_info *);

extern std::list<process_info *> all_processes;

/* Invoke FUNC for each process.  */

template <typename Func>
static void
for_each_process (Func func)
{
  std::list<process_info *>::iterator next, cur = all_processes.begin ();

  while (cur != all_processes.end ())
    {
      next = cur;
      next++;
      func (*cur);
      cur = next;
    }
}

/* Find the first process for which FUNC returns true.  Return NULL if no
   process satisfying FUNC is found.  */

template <typename Func>
static process_info *
find_process (Func func)
{
  std::list<process_info *>::iterator next, cur = all_processes.begin ();

  while (cur != all_processes.end ())
    {
      next = cur;
      next++;

      if (func (*cur))
	return *cur;

      cur = next;
    }

  return NULL;
}

extern struct thread_info *current_thread;

/* Return the first process in the processes list.  */
struct process_info *get_first_process (void);

struct process_info *add_process (int pid, int attached);
void remove_process (struct process_info *process);
struct process_info *find_process_pid (int pid);
int have_started_inferiors_p (void);
int have_attached_inferiors_p (void);

/* Switch to a thread of PROC.  */
void switch_to_process (process_info *proc);

void clear_inferiors (void);

void *thread_target_data (struct thread_info *);
struct regcache *thread_regcache_data (struct thread_info *);
void set_thread_regcache_data (struct thread_info *, struct regcache *);

/* Set the inferior current working directory.  If CWD is empty, unset
   the directory.  */
void set_inferior_cwd (std::string cwd);

#endif /* GDBSERVER_INFERIORS_H */

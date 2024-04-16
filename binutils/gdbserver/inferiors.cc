/* Inferior process information for the remote server for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#include "server.h"
#include "gdbsupport/common-inferior.h"
#include "gdbthread.h"
#include "dll.h"

std::list<process_info *> all_processes;
std::list<thread_info *> all_threads;

/* The current process.  */
static process_info *current_process_;

/* The current thread.  This is either a thread of CURRENT_PROCESS, or
   NULL.  */
struct thread_info *current_thread;

/* The current working directory used to start the inferior.

   Empty if not specified.  */
static std::string current_inferior_cwd;

struct thread_info *
add_thread (ptid_t thread_id, void *target_data)
{
  thread_info *new_thread = new thread_info (thread_id, target_data);

  all_threads.push_back (new_thread);

  if (current_thread == NULL)
    switch_to_thread (new_thread);

  return new_thread;
}

/* See gdbthread.h.  */

struct thread_info *
get_first_thread (void)
{
  if (!all_threads.empty ())
    return all_threads.front ();
  else
    return NULL;
}

struct thread_info *
find_thread_ptid (ptid_t ptid)
{
  return find_thread ([&] (thread_info *thread) {
    return thread->id == ptid;
  });
}

/* Find a thread associated with the given PROCESS, or NULL if no
   such thread exists.  */

static struct thread_info *
find_thread_process (const struct process_info *const process)
{
  return find_any_thread_of_pid (process->pid);
}

/* See gdbthread.h.  */

struct thread_info *
find_any_thread_of_pid (int pid)
{
  return find_thread (pid, [] (thread_info *thread) {
    return true;
  });
}

static void
free_one_thread (thread_info *thread)
{
  delete thread;
}

void
remove_thread (struct thread_info *thread)
{
  if (thread->btrace != NULL)
    target_disable_btrace (thread->btrace);

  discard_queued_stop_replies (ptid_of (thread));
  all_threads.remove (thread);
  if (current_thread == thread)
    switch_to_thread (nullptr);
  free_one_thread (thread);
}

void *
thread_target_data (struct thread_info *thread)
{
  return thread->target_data;
}

struct regcache *
thread_regcache_data (struct thread_info *thread)
{
  return thread->regcache_data;
}

void
set_thread_regcache_data (struct thread_info *thread, struct regcache *data)
{
  thread->regcache_data = data;
}

void
clear_inferiors (void)
{
  for_each_thread (free_one_thread);
  all_threads.clear ();

  clear_dlls ();

  switch_to_thread (nullptr);
  current_process_ = nullptr;
}

struct process_info *
add_process (int pid, int attached)
{
  process_info *process = new process_info (pid, attached);

  all_processes.push_back (process);

  return process;
}

/* Remove a process from the common process list and free the memory
   allocated for it.
   The caller is responsible for freeing private data first.  */

void
remove_process (struct process_info *process)
{
  clear_symbol_cache (&process->symbol_cache);
  free_all_breakpoints (process);
  gdb_assert (find_thread_process (process) == NULL);
  all_processes.remove (process);
  if (current_process () == process)
    switch_to_process (nullptr);
  delete process;
}

process_info *
find_process_pid (int pid)
{
  return find_process ([&] (process_info *process) {
    return process->pid == pid;
  });
}

/* Get the first process in the process list, or NULL if the list is empty.  */

process_info *
get_first_process (void)
{
  if (!all_processes.empty ())
    return all_processes.front ();
  else
    return NULL;
}

/* Return non-zero if there are any inferiors that we have created
   (as opposed to attached-to).  */

int
have_started_inferiors_p (void)
{
  return find_process ([] (process_info *process) {
    return !process->attached;
  }) != NULL;
}

/* Return non-zero if there are any inferiors that we have attached to.  */

int
have_attached_inferiors_p (void)
{
  return find_process ([] (process_info *process) {
    return process->attached;
  }) != NULL;
}

struct process_info *
get_thread_process (const struct thread_info *thread)
{
  return find_process_pid (thread->id.pid ());
}

struct process_info *
current_process (void)
{
  return current_process_;
}

/* See gdbsupport/common-gdbthread.h.  */

void
switch_to_thread (process_stratum_target *ops, ptid_t ptid)
{
  gdb_assert (ptid != minus_one_ptid);
  switch_to_thread (find_thread_ptid (ptid));
}

/* See gdbthread.h.  */

void
switch_to_thread (thread_info *thread)
{
  if (thread != nullptr)
    current_process_ = get_thread_process (thread);
  else
    current_process_ = nullptr;
  current_thread = thread;
}

/* See inferiors.h.  */

void
switch_to_process (process_info *proc)
{
  current_process_ = proc;
  current_thread = nullptr;
}

/* See gdbsupport/common-inferior.h.  */

const std::string &
get_inferior_cwd ()
{
  return current_inferior_cwd;
}

/* See inferiors.h.  */

void
set_inferior_cwd (std::string cwd)
{
  current_inferior_cwd = std::move (cwd);
}

scoped_restore_current_thread::scoped_restore_current_thread ()
{
  m_process = current_process_;
  m_thread = current_thread;
}

scoped_restore_current_thread::~scoped_restore_current_thread ()
{
  if (m_dont_restore)
    return;

  if (m_thread != nullptr)
    switch_to_thread (m_thread);
  else
    switch_to_process (m_process);
}

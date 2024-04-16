/* Observers

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include "gdbsupport/observable.h"
#include "target/waitstatus.h"

struct bpstat;
struct shobj;
struct objfile;
struct thread_info;
struct inferior;
struct process_stratum_target;
struct target_ops;
struct trace_state_variable;
struct program_space;

namespace gdb
{

namespace observers
{

/* The inferior has stopped for real.  The BS argument describes the
   breakpoints were are stopped at, if any.  Second argument
   PRINT_FRAME non-zero means display the location where the
   inferior has stopped.

   gdb notifies all normal_stop observers when the inferior execution
   has just stopped, the associated messages and annotations have been
   printed, and the control is about to be returned to the user.

   Note that the normal_stop notification is not emitted when the
   execution stops due to a breakpoint, and this breakpoint has a
   condition that is not met.  If the breakpoint has any associated
   commands list, the commands are executed after the notification is
   emitted.  */
extern observable<struct bpstat */* bs */, int /* print_frame */> normal_stop;

/* The inferior was stopped by a signal.  */
extern observable<enum gdb_signal /* siggnal */> signal_received;

/* The target's register contents have changed.  */
extern observable<struct target_ops */* target */> target_changed;

/* The executable being debugged by GDB in PSPACE has changed: The user
   decided to debug a different program, or the program he was debugging
   has been modified since being loaded by the debugger (by being
   recompiled, for instance).  The path to the new executable can be found
   by examining PSPACE->exec_filename.

   When RELOAD is true the path to the executable hasn't changed, but the
   file does appear to have changed, so GDB reloaded it, e.g. if the user
   recompiled the executable.  when RELOAD is false then the path to the
   executable has not changed.  */
extern observable<struct program_space */* pspace */,
		  bool /*reload */> executable_changed;

/* gdb has just connected to an inferior.  For 'run', gdb calls this
   observer while the inferior is still stopped at the entry-point
   instruction.  For 'attach' and 'core', gdb calls this observer
   immediately after connecting to the inferior, and before any
   information on the inferior has been printed.  */
extern observable<inferior */* inferior */> inferior_created;

/* The inferior EXEC_INF has exec'ed a new executable file.

   Execution continues in FOLLOW_INF, which may or may not be the same as
   EXEC_INF, depending on "set follow-exec-mode".  */
extern observable<inferior */* exec_inf */, inferior */* follow_inf */>
    inferior_execd;

/* The inferior PARENT_INF has forked.  If we are setting up an inferior for
   the child (because we follow only the child or we follow both), CHILD_INF
   is the child inferior.  Otherwise, CHILD_INF is nullptr.

   FORK_KIND is TARGET_WAITKIND_FORKED or TARGET_WAITKIND_VFORKED.  */
extern observable<inferior */* parent_inf */, inferior */* child_inf */,
		  target_waitkind /* fork_kind */> inferior_forked;

/* The shared library specified by SOLIB has been loaded.  Note that
   when gdb calls this observer, the library's symbols probably
   haven't been loaded yet.  */
extern observable<shobj &/* solib */> solib_loaded;

/* The shared library SOLIB has been unloaded from program space PSPACE.
   Note  when gdb calls this observer, the library's symbols have not
   been unloaded yet, and thus are still available.  */
extern observable<program_space *, const shobj &/* solib */> solib_unloaded;

/* The symbol file specified by OBJFILE has been loaded.  */
extern observable<struct objfile */* objfile */> new_objfile;

/*  All objfiles from PSPACE were removed.  */
extern observable<program_space */* pspace */> all_objfiles_removed;

/* The object file specified by OBJFILE is about to be freed.  */
extern observable<struct objfile */* objfile */> free_objfile;

/* The thread specified by T has been created.  */
extern observable<struct thread_info */* t */> new_thread;

/* The thread specified by T has exited.  EXIT_CODE is the thread's
   exit code, if available.  The SILENT argument indicates that GDB is
   removing the thread from its tables without wanting to notify the
   CLI about it.  */
extern observable<thread_info */* t */,
		  std::optional<ULONGEST> /* exit_code */,
		  bool /* silent */> thread_exit;

/* The thread specified by T has been deleted, with delete_thread.
   This is called just before the thread_info object is destroyed with
   operator delete.  */
extern observable<thread_info */* t */> thread_deleted;

/* An explicit stop request was issued to PTID.  If PTID equals
   minus_one_ptid, the request applied to all threads.  If
   ptid_is_pid(PTID) returns true, the request applied to all
   threads of the process pointed at by PTID.  Otherwise, the
   request applied to the single thread pointed at by PTID.  */
extern observable<ptid_t /* ptid */> thread_stop_requested;

/* The target was resumed.  The PTID parameter specifies which
   thread was resume, and may be RESUME_ALL if all threads are
   resumed.  */
extern observable<ptid_t /* ptid */> target_resumed;

/* The target is about to be proceeded.  */
extern observable<> about_to_proceed;

/* A new breakpoint B has been created.  */
extern observable<struct breakpoint */* b */> breakpoint_created;

/* A breakpoint has been destroyed.  The argument B is the
   pointer to the destroyed breakpoint.  */
extern observable<struct breakpoint */* b */> breakpoint_deleted;

/* A breakpoint has been modified in some way.  The argument B
   is the modified breakpoint.  */
extern observable<struct breakpoint */* b */> breakpoint_modified;

/* GDB has instantiated a new architecture, NEWARCH is a pointer to the new
   architecture.  */
extern observable<struct gdbarch */* newarch */> new_architecture;

/* The thread's ptid has changed.  The OLD_PTID parameter specifies
   the old value, and NEW_PTID specifies the new value.  */
extern observable<process_stratum_target * /* target */,
		  ptid_t /* old_ptid */, ptid_t /* new_ptid */>
  thread_ptid_changed;

/* The inferior INF has been added to the list of inferiors.  At
   this point, it might not be associated with any process.  */
extern observable<struct inferior */* inf */> inferior_added;

/* The inferior identified by INF has been attached to a
   process.  */
extern observable<struct inferior */* inf */> inferior_appeared;

/* Inferior INF is about to be detached.  */
extern observable<struct inferior */* inf */> inferior_pre_detach;

/* Either the inferior associated with INF has been detached from
   the process, or the process has exited.  */
extern observable<struct inferior */* inf */> inferior_exit;

/* The inferior INF has been removed from the list of inferiors.
   This method is called immediately before freeing INF.  */
extern observable<struct inferior */* inf */> inferior_removed;

/* The inferior CLONE has been created by cloning INF.  */
extern observable<struct inferior */* inf */, struct inferior */* clone */>
    inferior_cloned;

/* Bytes from DATA to DATA + LEN have been written to the inferior
   at ADDR.  */
extern observable<struct inferior */* inferior */, CORE_ADDR /* addr */,
		  ssize_t /* len */, const bfd_byte */* data */>
    memory_changed;

/* Called before a top-level prompt is displayed.  CURRENT_PROMPT is
   the current top-level prompt.  */
extern observable<const char */* current_prompt */> before_prompt;

/* Variable gdb_datadir has been set.  The value may not necessarily
   change.  */
extern observable<> gdb_datadir_changed;

/* An inferior function at ADDRESS is about to be called in thread
   THREAD.  */
extern observable<ptid_t /* thread */, CORE_ADDR /* address */>
    inferior_call_pre;

/* The inferior function at ADDRESS has just been called.  This
   observer is called even if the inferior exits during the call.
   THREAD is the thread in which the function was called, which may
   be different from the current thread.  */
extern observable<ptid_t /* thread */, CORE_ADDR /* address */>
    inferior_call_post;

/* A register in the inferior has been modified by the gdb user.  */
extern observable<frame_info_ptr /* frame */, int /* regnum */>
    register_changed;

/* The user-selected inferior, thread and/or frame has changed.  The
   user_select_what flag specifies if the inferior, thread and/or
   frame has changed.  */
extern observable<user_selected_what /* selection */>
    user_selected_context_changed;

/* This is notified when a styling setting has changed, content may need
   to be updated based on the new settings.  */
extern observable<> styling_changed;

/* The CLI's notion of the current source has changed.  This differs
   from user_selected_context_changed in that it is also set by the
   "list" command.  */
extern observable<> current_source_symtab_and_line_changed;

/* Called when GDB is about to exit.  */
extern observable<int> gdb_exiting;

/* When a connection is removed.  */
extern observable<process_stratum_target */* target */> connection_removed;

/* About to enter target_wait (). */
extern observable <ptid_t /* ptid */> target_pre_wait;

/* About to leave target_wait (). */
extern observable <ptid_t /* event_ptid */> target_post_wait;

/* New program space PSPACE was created.  */
extern observable <program_space */* pspace */> new_program_space;

/* The program space PSPACE is about to be deleted.  */
extern observable <program_space */* pspace */> free_program_space;

} /* namespace observers */

} /* namespace gdb */

#endif /* OBSERVABLE_H */

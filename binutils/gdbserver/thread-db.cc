/* Thread management interface, for the remote server for GDB.
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

#include "linux-low.h"

#include "debug.h"
#include "gdb_proc_service.h"
#include "nat/gdb_thread_db.h"
#include "gdbsupport/gdb_vecs.h"
#include "nat/linux-procfs.h"
#include "gdbsupport/scoped_restore.h"

#ifndef USE_LIBTHREAD_DB_DIRECTLY
#include <dlfcn.h>
#endif
#include <limits.h>
#include <ctype.h>

struct thread_db
{
  /* Structure that identifies the child process for the
     <proc_service.h> interface.  */
  struct ps_prochandle proc_handle;

  /* Connection to the libthread_db library.  */
  td_thragent_t *thread_agent;

  /* If this flag has been set, we've already asked GDB for all
     symbols we might need; assume symbol cache misses are
     failures.  */
  int all_symbols_looked_up;

#ifndef USE_LIBTHREAD_DB_DIRECTLY
  /* Handle of the libthread_db from dlopen.  */
  void *handle;
#endif

  /* Addresses of libthread_db functions.  */
  td_ta_new_ftype *td_ta_new_p;
  td_ta_map_lwp2thr_ftype *td_ta_map_lwp2thr_p;
  td_thr_get_info_ftype *td_thr_get_info_p;
  td_ta_thr_iter_ftype *td_ta_thr_iter_p;
  td_thr_tls_get_addr_ftype *td_thr_tls_get_addr_p;
  td_thr_tlsbase_ftype *td_thr_tlsbase_p;
  td_symbol_list_ftype *td_symbol_list_p;
};

static char *libthread_db_search_path;

static int find_one_thread (ptid_t);
static int find_new_threads_callback (const td_thrhandle_t *th_p, void *data);

static const char *
thread_db_err_str (td_err_e err)
{
  static char buf[64];

  switch (err)
    {
    case TD_OK:
      return "generic 'call succeeded'";
    case TD_ERR:
      return "generic error";
    case TD_NOTHR:
      return "no thread to satisfy query";
    case TD_NOSV:
      return "no sync handle to satisfy query";
    case TD_NOLWP:
      return "no LWP to satisfy query";
    case TD_BADPH:
      return "invalid process handle";
    case TD_BADTH:
      return "invalid thread handle";
    case TD_BADSH:
      return "invalid synchronization handle";
    case TD_BADTA:
      return "invalid thread agent";
    case TD_BADKEY:
      return "invalid key";
    case TD_NOMSG:
      return "no event message for getmsg";
    case TD_NOFPREGS:
      return "FPU register set not available";
    case TD_NOLIBTHREAD:
      return "application not linked with libthread";
    case TD_NOEVENT:
      return "requested event is not supported";
    case TD_NOCAPAB:
      return "capability not available";
    case TD_DBERR:
      return "debugger service failed";
    case TD_NOAPLIC:
      return "operation not applicable to";
    case TD_NOTSD:
      return "no thread-specific data for this thread";
    case TD_MALLOC:
      return "malloc failed";
    case TD_PARTIALREG:
      return "only part of register set was written/read";
    case TD_NOXREGS:
      return "X register set not available for this thread";
#ifdef HAVE_TD_VERSION
    case TD_VERSION:
      return "version mismatch between libthread_db and libpthread";
#endif
    default:
      xsnprintf (buf, sizeof (buf), "unknown thread_db error '%d'", err);
      return buf;
    }
}

#if 0
static char *
thread_db_state_str (td_thr_state_e state)
{
  static char buf[64];

  switch (state)
    {
    case TD_THR_STOPPED:
      return "stopped by debugger";
    case TD_THR_RUN:
      return "runnable";
    case TD_THR_ACTIVE:
      return "active";
    case TD_THR_ZOMBIE:
      return "zombie";
    case TD_THR_SLEEP:
      return "sleeping";
    case TD_THR_STOPPED_ASLEEP:
      return "stopped by debugger AND blocked";
    default:
      xsnprintf (buf, sizeof (buf), "unknown thread_db state %d", state);
      return buf;
    }
}
#endif

/* Get thread info about PTID.  */

static int
find_one_thread (ptid_t ptid)
{
  thread_info *thread = find_thread_ptid (ptid);
  lwp_info *lwp = get_thread_lwp (thread);
  if (lwp->thread_known)
    return 1;

  /* Get information about this thread.  libthread_db will need to read some
     memory, which will be done on the current process, so make PTID's process
     the current one.  */
  process_info *proc = find_process_pid (ptid.pid ());
  gdb_assert (proc != nullptr);

  scoped_restore_current_thread restore_thread;
  switch_to_process (proc);

  thread_db *thread_db = proc->priv->thread_db;
  td_thrhandle_t th;
  int lwpid = ptid.lwp ();
  td_err_e err = thread_db->td_ta_map_lwp2thr_p (thread_db->thread_agent, lwpid,
						 &th);
  if (err != TD_OK)
    error ("Cannot get thread handle for LWP %d: %s",
	   lwpid, thread_db_err_str (err));

  td_thrinfo_t ti;
  err = thread_db->td_thr_get_info_p (&th, &ti);
  if (err != TD_OK)
    error ("Cannot get thread info for LWP %d: %s",
	   lwpid, thread_db_err_str (err));

  threads_debug_printf ("Found thread %ld (LWP %d)",
			(unsigned long) ti.ti_tid, ti.ti_lid);

  if (lwpid != ti.ti_lid)
    {
      warning ("PID mismatch!  Expected %ld, got %ld",
	       (long) lwpid, (long) ti.ti_lid);
      return 0;
    }

  /* If the new thread ID is zero, a final thread ID will be available
     later.  Do not enable thread debugging yet.  */
  if (ti.ti_tid == 0)
    return 0;

  lwp->thread_known = 1;
  lwp->th = th;
  lwp->thread_handle = ti.ti_tid;

  return 1;
}

/* Attach a thread.  Return true on success.  */

static int
attach_thread (const td_thrhandle_t *th_p, td_thrinfo_t *ti_p)
{
  struct process_info *proc = current_process ();
  int pid = pid_of (proc);
  ptid_t ptid = ptid_t (pid, ti_p->ti_lid);
  struct lwp_info *lwp;
  int err;

  threads_debug_printf ("Attaching to thread %ld (LWP %d)",
			(unsigned long) ti_p->ti_tid, ti_p->ti_lid);
  err = the_linux_target->attach_lwp (ptid);
  if (err != 0)
    {
      std::string reason = linux_ptrace_attach_fail_reason_string (ptid, err);

      warning ("Could not attach to thread %ld (LWP %d): %s",
	       (unsigned long) ti_p->ti_tid, ti_p->ti_lid, reason.c_str ());

      return 0;
    }

  lwp = find_lwp_pid (ptid);
  gdb_assert (lwp != NULL);
  lwp->thread_known = 1;
  lwp->th = *th_p;
  lwp->thread_handle = ti_p->ti_tid;

  return 1;
}

/* Attach thread if we haven't seen it yet.
   Increment *COUNTER if we have attached a new thread.
   Return false on failure.  */

static int
maybe_attach_thread (const td_thrhandle_t *th_p, td_thrinfo_t *ti_p,
		     int *counter)
{
  struct lwp_info *lwp;

  lwp = find_lwp_pid (ptid_t (ti_p->ti_lid));
  if (lwp != NULL)
    return 1;

  if (!attach_thread (th_p, ti_p))
    return 0;

  if (counter != NULL)
    *counter += 1;

  return 1;
}

static int
find_new_threads_callback (const td_thrhandle_t *th_p, void *data)
{
  td_thrinfo_t ti;
  td_err_e err;
  struct thread_db *thread_db = current_process ()->priv->thread_db;

  err = thread_db->td_thr_get_info_p (th_p, &ti);
  if (err != TD_OK)
    error ("Cannot get thread info: %s", thread_db_err_str (err));

  if (ti.ti_lid == -1)
    {
      /* A thread with kernel thread ID -1 is either a thread that
	 exited and was joined, or a thread that is being created but
	 hasn't started yet, and that is reusing the tcb/stack of a
	 thread that previously exited and was joined.  (glibc marks
	 terminated and joined threads with kernel thread ID -1.  See
	 glibc PR17707.  */
      threads_debug_printf ("thread_db: skipping exited and "
			    "joined thread (0x%lx)",
			    (unsigned long) ti.ti_tid);
      return 0;
    }

  /* Check for zombies.  */
  if (ti.ti_state == TD_THR_UNKNOWN || ti.ti_state == TD_THR_ZOMBIE)
    return 0;

  if (!maybe_attach_thread (th_p, &ti, (int *) data))
    {
      /* Terminate iteration early: we might be looking at stale data in
	 the inferior.  The thread_db_find_new_threads will retry.  */
      return 1;
    }

  return 0;
}

static void
thread_db_find_new_threads (void)
{
  td_err_e err;
  ptid_t ptid = current_ptid;
  struct thread_db *thread_db = current_process ()->priv->thread_db;
  int loop, iteration;

  /* This function is only called when we first initialize thread_db.
     First locate the initial thread.  If it is not ready for
     debugging yet, then stop.  */
  if (find_one_thread (ptid) == 0)
    return;

  /* Require 4 successive iterations which do not find any new threads.
     The 4 is a heuristic: there is an inherent race here, and I have
     seen that 2 iterations in a row are not always sufficient to
     "capture" all threads.  */
  for (loop = 0, iteration = 0; loop < 4; ++loop, ++iteration)
    {
      int new_thread_count = 0;

      /* Iterate over all user-space threads to discover new threads.  */
      err = thread_db->td_ta_thr_iter_p (thread_db->thread_agent,
					 find_new_threads_callback,
					 &new_thread_count,
					 TD_THR_ANY_STATE,
					 TD_THR_LOWEST_PRIORITY,
					 TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS);
      threads_debug_printf ("Found %d threads in iteration %d.",
			    new_thread_count, iteration);

      if (new_thread_count != 0)
	{
	  /* Found new threads.  Restart iteration from beginning.  */
	  loop = -1;
	}
    }
  if (err != TD_OK)
    error ("Cannot find new threads: %s", thread_db_err_str (err));
}

/* Cache all future symbols that thread_db might request.  We can not
   request symbols at arbitrary states in the remote protocol, only
   when the client tells us that new symbols are available.  So when
   we load the thread library, make sure to check the entire list.  */

static void
thread_db_look_up_symbols (void)
{
  struct thread_db *thread_db = current_process ()->priv->thread_db;
  const char **sym_list;
  CORE_ADDR unused;

  for (sym_list = thread_db->td_symbol_list_p (); *sym_list; sym_list++)
    look_up_one_symbol (*sym_list, &unused, 1);

  /* We're not interested in any other libraries loaded after this
     point, only in symbols in libpthread.so.  */
  thread_db->all_symbols_looked_up = 1;
}

int
thread_db_look_up_one_symbol (const char *name, CORE_ADDR *addrp)
{
  struct thread_db *thread_db = current_process ()->priv->thread_db;
  int may_ask_gdb = !thread_db->all_symbols_looked_up;

  /* If we've passed the call to thread_db_look_up_symbols, then
     anything not in the cache must not exist; we're not interested
     in any libraries loaded after that point, only in symbols in
     libpthread.so.  It might not be an appropriate time to look
     up a symbol, e.g. while we're trying to fetch registers.  */
  return look_up_one_symbol (name, addrp, may_ask_gdb);
}

int
thread_db_get_tls_address (struct thread_info *thread, CORE_ADDR offset,
			   CORE_ADDR load_module, CORE_ADDR *address)
{
  psaddr_t addr;
  td_err_e err;
  struct lwp_info *lwp;
  struct process_info *proc;
  struct thread_db *thread_db;

  proc = get_thread_process (thread);
  thread_db = proc->priv->thread_db;

  /* If the thread layer is not (yet) initialized, fail.  */
  if (thread_db == NULL || !thread_db->all_symbols_looked_up)
    return TD_ERR;

  /* If td_thr_tls_get_addr is missing rather do not expect td_thr_tlsbase
     could work.  */
  if (thread_db->td_thr_tls_get_addr_p == NULL
      || (load_module == 0 && thread_db->td_thr_tlsbase_p == NULL))
    return -1;

  lwp = get_thread_lwp (thread);
  if (!lwp->thread_known)
    find_one_thread (thread->id);
  if (!lwp->thread_known)
    return TD_NOTHR;

  scoped_restore_current_thread restore_thread;
  switch_to_thread (thread);

  if (load_module != 0)
    {
      /* Note the cast through uintptr_t: this interface only works if
	 a target address fits in a psaddr_t, which is a host pointer.
	 So a 32-bit debugger can not access 64-bit TLS through this.  */
      err = thread_db->td_thr_tls_get_addr_p (&lwp->th,
					     (psaddr_t) (uintptr_t) load_module,
					      offset, &addr);
    }
  else
    {
      /* This code path handles the case of -static -pthread executables:
	 https://sourceware.org/ml/libc-help/2014-03/msg00024.html
	 For older GNU libc r_debug.r_map is NULL.  For GNU libc after
	 PR libc/16831 due to GDB PR threads/16954 LOAD_MODULE is also NULL.
	 The constant number 1 depends on GNU __libc_setup_tls
	 initialization of l_tls_modid to 1.  */
      err = thread_db->td_thr_tlsbase_p (&lwp->th, 1, &addr);
      addr = (char *) addr + offset;
    }

  if (err == TD_OK)
    {
      *address = (CORE_ADDR) (uintptr_t) addr;
      return 0;
    }
  else
    return err;
}

/* See linux-low.h.  */

bool
thread_db_thread_handle (ptid_t ptid, gdb_byte **handle, int *handle_len)
{
  struct thread_db *thread_db;
  struct lwp_info *lwp;
  thread_info *thread = find_thread_ptid (ptid);

  if (thread == NULL)
    return false;

  thread_db = get_thread_process (thread)->priv->thread_db;

  if (thread_db == NULL)
    return false;

  lwp = get_thread_lwp (thread);

  if (!lwp->thread_known && !find_one_thread (thread->id))
    return false;

  gdb_assert (lwp->thread_known);

  *handle = (gdb_byte *) &lwp->thread_handle;
  *handle_len = sizeof (lwp->thread_handle);
  return true;
}

#ifdef USE_LIBTHREAD_DB_DIRECTLY

static int
thread_db_load_search (void)
{
  td_err_e err;
  struct thread_db *tdb;
  struct process_info *proc = current_process ();

  gdb_assert (proc->priv->thread_db == NULL);

  tdb = XCNEW (struct thread_db);
  proc->priv->thread_db = tdb;

  tdb->td_ta_new_p = &td_ta_new;

  /* Attempt to open a connection to the thread library.  */
  err = tdb->td_ta_new_p (&tdb->proc_handle, &tdb->thread_agent);
  if (err != TD_OK)
    {
      threads_debug_printf ("td_ta_new(): %s", thread_db_err_str (err));
      free (tdb);
      proc->priv->thread_db = NULL;
      return 0;
    }

  tdb->td_ta_map_lwp2thr_p = &td_ta_map_lwp2thr;
  tdb->td_thr_get_info_p = &td_thr_get_info;
  tdb->td_ta_thr_iter_p = &td_ta_thr_iter;
  tdb->td_symbol_list_p = &td_symbol_list;

  /* These are not essential.  */
  tdb->td_thr_tls_get_addr_p = &td_thr_tls_get_addr;
  tdb->td_thr_tlsbase_p = &td_thr_tlsbase;

  return 1;
}

#else

static int
try_thread_db_load_1 (void *handle)
{
  td_err_e err;
  struct thread_db *tdb;
  struct process_info *proc = current_process ();

  gdb_assert (proc->priv->thread_db == NULL);

  tdb = XCNEW (struct thread_db);
  proc->priv->thread_db = tdb;

  tdb->handle = handle;

  /* Initialize pointers to the dynamic library functions we will use.
     Essential functions first.  */

#define CHK(required, a)					\
  do								\
    {								\
      if ((a) == NULL)						\
	{							\
	  threads_debug_printf ("dlsym: %s", dlerror ());	\
	  if (required)						\
	    {							\
	      free (tdb);					\
	      proc->priv->thread_db = NULL;			\
	      return 0;						\
	    }							\
	}							\
    }								\
  while (0)

#define TDB_DLSYM(tdb, func) \
  tdb->func ## _p = (func ## _ftype *) dlsym (tdb->handle, #func)

  CHK (1, TDB_DLSYM (tdb, td_ta_new));

  /* Attempt to open a connection to the thread library.  */
  err = tdb->td_ta_new_p (&tdb->proc_handle, &tdb->thread_agent);
  if (err != TD_OK)
    {
      threads_debug_printf ("td_ta_new(): %s", thread_db_err_str (err));
      free (tdb);
      proc->priv->thread_db = NULL;
      return 0;
    }

  CHK (1, TDB_DLSYM (tdb, td_ta_map_lwp2thr));
  CHK (1, TDB_DLSYM (tdb, td_thr_get_info));
  CHK (1, TDB_DLSYM (tdb, td_ta_thr_iter));
  CHK (1, TDB_DLSYM (tdb, td_symbol_list));

  /* These are not essential.  */
  CHK (0, TDB_DLSYM (tdb, td_thr_tls_get_addr));
  CHK (0, TDB_DLSYM (tdb, td_thr_tlsbase));

#undef CHK
#undef TDB_DLSYM

  return 1;
}

#ifdef HAVE_DLADDR

/* Lookup a library in which given symbol resides.
   Note: this is looking in the GDBSERVER process, not in the inferior.
   Returns library name, or NULL.  */

static const char *
dladdr_to_soname (const void *addr)
{
  Dl_info info;

  if (dladdr (addr, &info) != 0)
    return info.dli_fname;
  return NULL;
}

#endif

static int
try_thread_db_load (const char *library)
{
  void *handle;

  threads_debug_printf ("Trying host libthread_db library: %s.",
			library);
  handle = dlopen (library, RTLD_NOW);
  if (handle == NULL)
    {
      threads_debug_printf ("dlopen failed: %s.", dlerror ());
      return 0;
    }

#ifdef HAVE_DLADDR
  if (debug_threads && strchr (library, '/') == NULL)
    {
      void *td_init;

      td_init = dlsym (handle, "td_init");
      if (td_init != NULL)
	{
	  const char *const libpath = dladdr_to_soname (td_init);

	  if (libpath != NULL)
	    threads_debug_printf ("Host %s resolved to: %s.", library, libpath);
	}
    }
#endif

  if (try_thread_db_load_1 (handle))
    return 1;

  /* This library "refused" to work on current inferior.  */
  dlclose (handle);
  return 0;
}

/* Handle $sdir in libthread-db-search-path.
   Look for libthread_db in the system dirs, or wherever a plain
   dlopen(file_without_path) will look.
   The result is true for success.  */

static int
try_thread_db_load_from_sdir (void)
{
  return try_thread_db_load (LIBTHREAD_DB_SO);
}

/* Try to load libthread_db from directory DIR of length DIR_LEN.
   The result is true for success.  */

static int
try_thread_db_load_from_dir (const char *dir, size_t dir_len)
{
  char path[PATH_MAX];

  if (dir_len + 1 + strlen (LIBTHREAD_DB_SO) + 1 > sizeof (path))
    {
      char *cp = (char *) xmalloc (dir_len + 1);

      memcpy (cp, dir, dir_len);
      cp[dir_len] = '\0';
      warning (_("libthread-db-search-path component too long,"
		 " ignored: %s."), cp);
      free (cp);
      return 0;
    }

  memcpy (path, dir, dir_len);
  path[dir_len] = '/';
  strcpy (path + dir_len + 1, LIBTHREAD_DB_SO);
  return try_thread_db_load (path);
}

/* Search libthread_db_search_path for libthread_db which "agrees"
   to work on current inferior.
   The result is true for success.  */

static int
thread_db_load_search (void)
{
  int rc = 0;

  if (libthread_db_search_path == NULL)
    libthread_db_search_path = xstrdup (LIBTHREAD_DB_SEARCH_PATH);

  std::vector<gdb::unique_xmalloc_ptr<char>> dir_vec
    = dirnames_to_char_ptr_vec (libthread_db_search_path);

  for (const gdb::unique_xmalloc_ptr<char> &this_dir_up : dir_vec)
    {
      char *this_dir = this_dir_up.get ();
      const int pdir_len = sizeof ("$pdir") - 1;
      size_t this_dir_len;

      this_dir_len = strlen (this_dir);

      if (strncmp (this_dir, "$pdir", pdir_len) == 0
	  && (this_dir[pdir_len] == '\0'
	      || this_dir[pdir_len] == '/'))
	{
	  /* We don't maintain a list of loaded libraries so we don't know
	     where libpthread lives.  We *could* fetch the info, but we don't
	     do that yet.  Ignore it.  */
	}
      else if (strcmp (this_dir, "$sdir") == 0)
	{
	  if (try_thread_db_load_from_sdir ())
	    {
	      rc = 1;
	      break;
	    }
	}
      else
	{
	  if (try_thread_db_load_from_dir (this_dir, this_dir_len))
	    {
	      rc = 1;
	      break;
	    }
	}
    }

  threads_debug_printf ("thread_db_load_search returning %d", rc);
  return rc;
}

#endif  /* USE_LIBTHREAD_DB_DIRECTLY */

int
thread_db_init (void)
{
  struct process_info *proc = current_process ();

  /* FIXME drow/2004-10-16: This is the "overall process ID", which
     GNU/Linux calls tgid, "thread group ID".  When we support
     attaching to threads, the original thread may not be the correct
     thread.  We would have to get the process ID from /proc for NPTL.

     This isn't the only place in gdbserver that assumes that the first
     process in the list is the thread group leader.  */

  if (thread_db_load_search ())
    {
      /* It's best to avoid td_ta_thr_iter if possible.  That walks
	 data structures in the inferior's address space that may be
	 corrupted, or, if the target is running, the list may change
	 while we walk it.  In the latter case, it's possible that a
	 thread exits just at the exact time that causes GDBserver to
	 get stuck in an infinite loop.  As the kernel supports clone
	 events and /proc/PID/task/ exists, then we already know about
	 all threads in the process.  When we need info out of
	 thread_db on a given thread (e.g., for TLS), we'll use
	 find_one_thread then.  That uses thread_db entry points that
	 do not walk libpthread's thread list, so should be safe, as
	 well as more efficient.  */
      if (!linux_proc_task_list_dir_exists (pid_of (proc)))
	thread_db_find_new_threads ();
      thread_db_look_up_symbols ();
      return 1;
    }

  return 0;
}

/* Disconnect from libthread_db and free resources.  */

static void
disable_thread_event_reporting (struct process_info *proc)
{
  struct thread_db *thread_db = proc->priv->thread_db;
  if (thread_db)
    {
      td_err_e (*td_ta_clear_event_p) (const td_thragent_t *ta,
				       td_thr_events_t *event);

#ifndef USE_LIBTHREAD_DB_DIRECTLY
      td_ta_clear_event_p
	= (td_ta_clear_event_ftype *) dlsym (thread_db->handle,
					     "td_ta_clear_event");
#else
      td_ta_clear_event_p = &td_ta_clear_event;
#endif

      if (td_ta_clear_event_p != NULL)
	{
	  scoped_restore_current_thread restore_thread;
	  td_thr_events_t events;

	  switch_to_process (proc);

	  /* Set the process wide mask saying we aren't interested
	     in any events anymore.  */
	  td_event_fillset (&events);
	  (*td_ta_clear_event_p) (thread_db->thread_agent, &events);
	}
    }
}

void
thread_db_detach (struct process_info *proc)
{
  struct thread_db *thread_db = proc->priv->thread_db;

  if (thread_db)
    {
      disable_thread_event_reporting (proc);
    }
}

/* Disconnect from libthread_db and free resources.  */

void
thread_db_mourn (struct process_info *proc)
{
  struct thread_db *thread_db = proc->priv->thread_db;
  if (thread_db)
    {
      td_ta_delete_ftype *td_ta_delete_p;

#ifndef USE_LIBTHREAD_DB_DIRECTLY
      td_ta_delete_p = (td_ta_delete_ftype *) dlsym (thread_db->handle, "td_ta_delete");
#else
      td_ta_delete_p = &td_ta_delete;
#endif

      if (td_ta_delete_p != NULL)
	(*td_ta_delete_p) (thread_db->thread_agent);

#ifndef USE_LIBTHREAD_DB_DIRECTLY
      dlclose (thread_db->handle);
#endif  /* USE_LIBTHREAD_DB_DIRECTLY  */

      free (thread_db);
      proc->priv->thread_db = NULL;
    }
}

/* Handle "set libthread-db-search-path" monitor command and return 1.
   For any other command, return 0.  */

int
thread_db_handle_monitor_command (char *mon)
{
  const char *cmd = "set libthread-db-search-path";
  size_t cmd_len = strlen (cmd);

  if (strncmp (mon, cmd, cmd_len) == 0
      && (mon[cmd_len] == '\0'
	  || mon[cmd_len] == ' '))
    {
      const char *cp = mon + cmd_len;

      if (libthread_db_search_path != NULL)
	free (libthread_db_search_path);

      /* Skip leading space (if any).  */
      while (isspace (*cp))
	++cp;

      if (*cp == '\0')
	cp = LIBTHREAD_DB_SEARCH_PATH;
      libthread_db_search_path = xstrdup (cp);

      monitor_output ("libthread-db-search-path set to `");
      monitor_output (libthread_db_search_path);
      monitor_output ("'\n");
      return 1;
    }

  /* Tell server.c to perform default processing.  */
  return 0;
}

/* See linux-low.h.  */

void
thread_db_notice_clone (struct thread_info *parent_thr, ptid_t child_ptid)
{
  process_info *parent_proc = get_thread_process (parent_thr);
  struct thread_db *thread_db = parent_proc->priv->thread_db;

  /* If the thread layer isn't initialized, return.  It may just
     be that the program uses clone, but does not use libthread_db.  */
  if (thread_db == NULL || !thread_db->all_symbols_looked_up)
    return;

  /* find_one_thread calls into libthread_db which accesses memory via
     the current thread.  Temporarily switch to a thread we know is
     stopped.  */
  scoped_restore_current_thread restore_thread;
  switch_to_thread (parent_thr);

  if (!find_one_thread (child_ptid))
    warning ("Cannot find thread after clone.");
}

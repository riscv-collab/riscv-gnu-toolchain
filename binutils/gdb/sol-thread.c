/* Solaris threads debugging interface.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.

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

/* This module implements a sort of half target that sits between the
   machine-independent parts of GDB and the /proc interface (procfs.c)
   to provide access to the Solaris user-mode thread implementation.

   Solaris threads are true user-mode threads, which are invoked via
   the thr_* and pthread_* (native and POSIX respectively) interfaces.
   These are mostly implemented in user-space, with all thread context
   kept in various structures that live in the user's heap.  These
   should not be confused with lightweight processes (LWPs), which are
   implemented by the kernel, and scheduled without explicit
   intervention by the process.

   Just to confuse things a little, Solaris threads (both native and
   POSIX) are actually implemented using LWPs.  In general, there are
   going to be more threads than LWPs.  There is no fixed
   correspondence between a thread and an LWP.  When a thread wants to
   run, it gets scheduled onto the first available LWP and can
   therefore migrate from one LWP to another as time goes on.  A
   sleeping thread may not be associated with an LWP at all!

   To make it possible to mess with threads, Sun provides a library
   called libthread_db.so.1 (not to be confused with
   libthread_db.so.0, which doesn't have a published interface).  This
   interface has an upper part, which it provides, and a lower part
   which we provide.  The upper part consists of the td_* routines,
   which allow us to find all the threads, query their state, etc...
   The lower part consists of all of the ps_*, which are used by the
   td_* routines to read/write memory, manipulate LWPs, lookup
   symbols, etc...  The ps_* routines actually do most of their work
   by calling functions in procfs.c.  */

#include "defs.h"
#include <thread.h>
#include <proc_service.h>
#include <thread_db.h>
#include "gdbthread.h"
#include "target.h"
#include "inferior.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include "gdbcmd.h"
#include "gdbcore.h"
#include "regcache.h"
#include "solib.h"
#include "symfile.h"
#include "observable.h"
#include "procfs.h"
#include "symtab.h"
#include "minsyms.h"
#include "objfiles.h"

static const target_info thread_db_target_info = {
  "solaris-threads",
  N_("Solaris threads and pthread."),
  N_("Solaris threads and pthread support.")
};

class sol_thread_target final : public target_ops
{
public:
  const target_info &info () const override
  { return thread_db_target_info; }

  strata stratum () const override { return thread_stratum; }

  void detach (inferior *, int) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;
  void resume (ptid_t, int, enum gdb_signal) override;
  void mourn_inferior () override;
  std::string pid_to_str (ptid_t) override;
  ptid_t get_ada_task_ptid (long lwp, ULONGEST thread) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  bool thread_alive (ptid_t ptid) override;
  void update_thread_list () override;
};

static sol_thread_target sol_thread_ops;

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

/* This struct is defined by us, but mainly used for the proc_service
   interface.  We don't have much use for it, except as a handy place
   to get a real PID for memory accesses.  */

struct ps_prochandle
{
  ptid_t ptid;
};

struct string_map
{
  int num;
  const char *str;
};

static struct ps_prochandle main_ph;
static td_thragent_t *main_ta;
static int sol_thread_active = 0;

/* Default definitions: These must be defined in tm.h if they are to
   be shared with a process module such as procfs.  */

/* Types of the libthread_db functions.  */

typedef void (td_log_ftype)(const int on_off);
typedef td_err_e (td_ta_new_ftype)(const struct ps_prochandle *ph_p,
				   td_thragent_t **ta_pp);
typedef td_err_e (td_ta_delete_ftype)(td_thragent_t *ta_p);
typedef td_err_e (td_init_ftype)(void);
typedef td_err_e (td_ta_get_ph_ftype)(const td_thragent_t *ta_p,
				      struct ps_prochandle **ph_pp);
typedef td_err_e (td_ta_get_nthreads_ftype)(const td_thragent_t *ta_p,
					    int *nthread_p);
typedef td_err_e (td_ta_tsd_iter_ftype)(const td_thragent_t *ta_p,
					td_key_iter_f *cb, void *cbdata_p);
typedef td_err_e (td_ta_thr_iter_ftype)(const td_thragent_t *ta_p,
					td_thr_iter_f *cb, void *cbdata_p,
					td_thr_state_e state, int ti_pri,
					sigset_t *ti_sigmask_p,
					unsigned ti_user_flags);
typedef td_err_e (td_thr_validate_ftype)(const td_thrhandle_t *th_p);
typedef td_err_e (td_thr_tsd_ftype)(const td_thrhandle_t * th_p,
				    const thread_key_t key, void **data_pp);
typedef td_err_e (td_thr_get_info_ftype)(const td_thrhandle_t *th_p,
					 td_thrinfo_t *ti_p);
typedef td_err_e (td_thr_getfpregs_ftype)(const td_thrhandle_t *th_p,
					  prfpregset_t *fpregset);
typedef td_err_e (td_thr_getxregsize_ftype)(const td_thrhandle_t *th_p,
					    int *xregsize);
typedef td_err_e (td_thr_getxregs_ftype)(const td_thrhandle_t *th_p,
					 const caddr_t xregset);
typedef td_err_e (td_thr_sigsetmask_ftype)(const td_thrhandle_t *th_p,
					   const sigset_t ti_sigmask);
typedef td_err_e (td_thr_setprio_ftype)(const td_thrhandle_t *th_p,
					const int ti_pri);
typedef td_err_e (td_thr_setsigpending_ftype)(const td_thrhandle_t *th_p,
					      const uchar_t ti_pending_flag,
					      const sigset_t ti_pending);
typedef td_err_e (td_thr_setfpregs_ftype)(const td_thrhandle_t *th_p,
					  const prfpregset_t *fpregset);
typedef td_err_e (td_thr_setxregs_ftype)(const td_thrhandle_t *th_p,
					 const caddr_t xregset);
typedef td_err_e (td_ta_map_id2thr_ftype)(const td_thragent_t *ta_p,
					  thread_t tid,
					  td_thrhandle_t *th_p);
typedef td_err_e (td_ta_map_lwp2thr_ftype)(const td_thragent_t *ta_p,
					   lwpid_t lwpid,
					   td_thrhandle_t *th_p);
typedef td_err_e (td_thr_getgregs_ftype)(const td_thrhandle_t *th_p,
					 prgregset_t regset);
typedef td_err_e (td_thr_setgregs_ftype)(const td_thrhandle_t *th_p,
					 const prgregset_t regset);

/* Pointers to routines from libthread_db resolved by dlopen().  */

static td_log_ftype *p_td_log;
static td_ta_new_ftype *p_td_ta_new;
static td_ta_delete_ftype *p_td_ta_delete;
static td_init_ftype *p_td_init;
static td_ta_get_ph_ftype *p_td_ta_get_ph;
static td_ta_get_nthreads_ftype *p_td_ta_get_nthreads;
static td_ta_tsd_iter_ftype *p_td_ta_tsd_iter;
static td_ta_thr_iter_ftype *p_td_ta_thr_iter;
static td_thr_validate_ftype *p_td_thr_validate;
static td_thr_tsd_ftype *p_td_thr_tsd;
static td_thr_get_info_ftype *p_td_thr_get_info;
static td_thr_getfpregs_ftype *p_td_thr_getfpregs;
static td_thr_getxregsize_ftype *p_td_thr_getxregsize;
static td_thr_getxregs_ftype *p_td_thr_getxregs;
static td_thr_sigsetmask_ftype *p_td_thr_sigsetmask;
static td_thr_setprio_ftype *p_td_thr_setprio;
static td_thr_setsigpending_ftype *p_td_thr_setsigpending;
static td_thr_setfpregs_ftype *p_td_thr_setfpregs;
static td_thr_setxregs_ftype *p_td_thr_setxregs;
static td_ta_map_id2thr_ftype *p_td_ta_map_id2thr;
static td_ta_map_lwp2thr_ftype *p_td_ta_map_lwp2thr;
static td_thr_getgregs_ftype *p_td_thr_getgregs;
static td_thr_setgregs_ftype *p_td_thr_setgregs;


/* Return the libthread_db error string associated with ERRCODE.  If
   ERRCODE is unknown, return an appropriate message.  */

static const char *
td_err_string (td_err_e errcode)
{
  static struct string_map td_err_table[] =
  {
    { TD_OK, "generic \"call succeeded\"" },
    { TD_ERR, "generic error." },
    { TD_NOTHR, "no thread can be found to satisfy query" },
    { TD_NOSV, "no synch. variable can be found to satisfy query" },
    { TD_NOLWP, "no lwp can be found to satisfy query" },
    { TD_BADPH, "invalid process handle" },
    { TD_BADTH, "invalid thread handle" },
    { TD_BADSH, "invalid synchronization handle" },
    { TD_BADTA, "invalid thread agent" },
    { TD_BADKEY, "invalid key" },
    { TD_NOMSG, "td_thr_event_getmsg() called when there was no message" },
    { TD_NOFPREGS, "FPU register set not available for given thread" },
    { TD_NOLIBTHREAD, "application not linked with libthread" },
    { TD_NOEVENT, "requested event is not supported" },
    { TD_NOCAPAB, "capability not available" },
    { TD_DBERR, "Debugger service failed" },
    { TD_NOAPLIC, "Operation not applicable to" },
    { TD_NOTSD, "No thread specific data for this thread" },
    { TD_MALLOC, "Malloc failed" },
    { TD_PARTIALREG, "Only part of register set was written/read" },
    { TD_NOXREGS, "X register set not available for given thread" }
  };
  const int td_err_size = sizeof td_err_table / sizeof (struct string_map);
  int i;
  static char buf[50];

  for (i = 0; i < td_err_size; i++)
    if (td_err_table[i].num == errcode)
      return td_err_table[i].str;

  xsnprintf (buf, sizeof (buf), "Unknown libthread_db error code: %d",
	     errcode);

  return buf;
}

/* Return the libthread_db state string associated with STATECODE.
   If STATECODE is unknown, return an appropriate message.  */

static const char *
td_state_string (td_thr_state_e statecode)
{
  static struct string_map td_thr_state_table[] =
  {
    { TD_THR_ANY_STATE, "any state" },
    { TD_THR_UNKNOWN, "unknown" },
    { TD_THR_STOPPED, "stopped" },
    { TD_THR_RUN, "run" },
    { TD_THR_ACTIVE, "active" },
    { TD_THR_ZOMBIE, "zombie" },
    { TD_THR_SLEEP, "sleep" },
    { TD_THR_STOPPED_ASLEEP, "stopped asleep" }
  };
  const int td_thr_state_table_size =
    sizeof td_thr_state_table / sizeof (struct string_map);
  int i;
  static char buf[50];

  for (i = 0; i < td_thr_state_table_size; i++)
    if (td_thr_state_table[i].num == statecode)
      return td_thr_state_table[i].str;

  xsnprintf (buf, sizeof (buf), "Unknown libthread_db state code: %d",
	     statecode);

  return buf;
}


/* Convert a POSIX or Solaris thread ID into a LWP ID.  If THREAD_ID
   doesn't exist, that's an error.  If it's an inactive thread, return
   DEFAULT_LWP.

   NOTE: This function probably shouldn't call error().  */

static ptid_t
thread_to_lwp (ptid_t thread_id, int default_lwp)
{
  td_thrinfo_t ti;
  td_thrhandle_t th;
  td_err_e val;

  if (thread_id.lwp_p ())
    return thread_id;		/* It's already an LWP ID.  */

  /* It's a thread.  Convert to LWP.  */

  val = p_td_ta_map_id2thr (main_ta, thread_id.tid (), &th);
  if (val == TD_NOTHR)
    return ptid_t (-1);	/* Thread must have terminated.  */
  else if (val != TD_OK)
    error (_("thread_to_lwp: td_ta_map_id2thr %s"), td_err_string (val));

  val = p_td_thr_get_info (&th, &ti);
  if (val == TD_NOTHR)
    return ptid_t (-1);	/* Thread must have terminated.  */
  else if (val != TD_OK)
    error (_("thread_to_lwp: td_thr_get_info: %s"), td_err_string (val));

  if (ti.ti_state != TD_THR_ACTIVE)
    {
      if (default_lwp != -1)
	return ptid_t (default_lwp);
      error (_("thread_to_lwp: thread state not active: %s"),
	     td_state_string (ti.ti_state));
    }

  return ptid_t (thread_id.pid (), ti.ti_lid, 0);
}

/* Convert an LWP ID into a POSIX or Solaris thread ID.  If LWP_ID
   doesn't exists, that's an error.

   NOTE: This function probably shouldn't call error().  */

static ptid_t
lwp_to_thread (ptid_t lwp)
{
  td_thrinfo_t ti;
  td_thrhandle_t th;
  td_err_e val;

  if (lwp.tid_p ())
    return lwp;			/* It's already a thread ID.  */

  /* It's an LWP.  Convert it to a thread ID.  */

  if (!target_thread_alive (lwp))
    return ptid_t (-1);	/* Must be a defunct LPW.  */

  val = p_td_ta_map_lwp2thr (main_ta, lwp.lwp (), &th);
  if (val == TD_NOTHR)
    return ptid_t (-1);	/* Thread must have terminated.  */
  else if (val != TD_OK)
    error (_("lwp_to_thread: td_ta_map_lwp2thr: %s."), td_err_string (val));

  val = p_td_thr_validate (&th);
  if (val == TD_NOTHR)
    return lwp;			/* Unknown to libthread; just return LPW,  */
  else if (val != TD_OK)
    error (_("lwp_to_thread: td_thr_validate: %s."), td_err_string (val));

  val = p_td_thr_get_info (&th, &ti);
  if (val == TD_NOTHR)
    return ptid_t (-1);	/* Thread must have terminated.  */
  else if (val != TD_OK)
    error (_("lwp_to_thread: td_thr_get_info: %s."), td_err_string (val));

  return ptid_t (lwp.pid (), 0 , ti.ti_tid);
}


/* Most target vector functions from here on actually just pass
   through to the layer beneath, as they don't need to do anything
   specific for threads.  */

/* Take a program previously attached to and detaches it.  The program
   resumes execution and will no longer stop on signals, etc.  We'd
   better not have left any breakpoints in the program or it'll die
   when it hits one.  For this to work, it may be necessary for the
   process to have been previously attached.  It *might* work if the
   program was started via the normal ptrace (PTRACE_TRACEME).  */

void
sol_thread_target::detach (inferior *inf, int from_tty)
{
  target_ops *beneath = this->beneath ();

  sol_thread_active = 0;
  inferior_ptid = ptid_t (main_ph.ptid.pid ());
  inf->unpush_target (this);
  beneath->detach (inf, from_tty);
}

/* Resume execution of process PTID.  If STEP is nonzero, then just
   single step it.  If SIGNAL is nonzero, restart it with that signal
   activated.  We may have to convert PTID from a thread ID to an LWP
   ID for procfs.  */

void
sol_thread_target::resume (ptid_t ptid, int step, enum gdb_signal signo)
{
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  inferior_ptid = thread_to_lwp (inferior_ptid, main_ph.ptid.pid ());
  if (inferior_ptid.pid () == -1)
    inferior_ptid = procfs_first_available ();

  if (ptid.pid () != -1)
    {
      ptid_t save_ptid = ptid;

      ptid = thread_to_lwp (ptid, -2);
      if (ptid.pid () == -2)		/* Inactive thread.  */
	error (_("This version of Solaris can't start inactive threads."));
      if (info_verbose && ptid.pid () == -1)
	warning (_("Specified thread %s seems to have terminated"),
		 pulongest (save_ptid.tid ()));
    }

  beneath ()->resume (ptid, step, signo);
}

/* Wait for any threads to stop.  We may have to convert PTID from a
   thread ID to an LWP ID, and vice versa on the way out.  */

ptid_t
sol_thread_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			 target_wait_flags options)
{
  if (ptid.pid () != -1)
    {
      ptid_t ptid_for_warning = ptid;

      ptid = thread_to_lwp (ptid, -2);
      if (ptid.pid () == -2)		/* Inactive thread.  */
	error (_("This version of Solaris can't start inactive threads."));
      if (info_verbose && ptid.pid () == -1)
	warning (_("Specified thread %s seems to have terminated"),
		 pulongest (ptid_for_warning.tid ()));
    }

  ptid_t rtnval = beneath ()->wait (ptid, ourstatus, options);

  if (ourstatus->kind () != TARGET_WAITKIND_EXITED)
    {
      /* Map the LWP of interest back to the appropriate thread ID.  */
      ptid_t thr_ptid = lwp_to_thread (rtnval);
      if (thr_ptid.pid () != -1)
	rtnval = thr_ptid;

      /* See if we have a new thread.  */
      if (rtnval.tid_p ())
	{
	  thread_info *thr = current_inferior ()->find_thread (rtnval);
	  if (thr == NULL || thr->state == THREAD_EXITED)
	    {
	      process_stratum_target *proc_target
		= current_inferior ()->process_target ();
	      add_thread (proc_target, rtnval);
	    }
	}
    }

  /* During process initialization, we may get here without the thread
     package being initialized, since that can only happen after we've
     found the shared libs.  */

  return rtnval;
}

void
sol_thread_target::fetch_registers (struct regcache *regcache, int regnum)
{
  thread_t thread;
  td_thrhandle_t thandle;
  td_err_e val;
  prgregset_t gregset;
  prfpregset_t fpregset;
  gdb_gregset_t *gregset_p = &gregset;
  gdb_fpregset_t *fpregset_p = &fpregset;
  ptid_t ptid = regcache->ptid ();

  if (!ptid.tid_p ())
    {
      /* It's an LWP; pass the request on to the layer beneath.  */
      beneath ()->fetch_registers (regcache, regnum);
      return;
    }

  /* Solaris thread: convert PTID into a td_thrhandle_t.  */
  thread = ptid.tid ();
  if (thread == 0)
    error (_("sol_thread_fetch_registers: thread == 0"));

  val = p_td_ta_map_id2thr (main_ta, thread, &thandle);
  if (val != TD_OK)
    error (_("sol_thread_fetch_registers: td_ta_map_id2thr: %s"),
	   td_err_string (val));

  /* Get the general-purpose registers.  */

  val = p_td_thr_getgregs (&thandle, gregset);
  if (val != TD_OK && val != TD_PARTIALREG)
    error (_("sol_thread_fetch_registers: td_thr_getgregs %s"),
	   td_err_string (val));

  /* For SPARC, TD_PARTIALREG means that only %i0...%i7, %l0..%l7, %pc
     and %sp are saved (by a thread context switch).  */

  /* And, now the floating-point registers.  */

  val = p_td_thr_getfpregs (&thandle, &fpregset);
  if (val != TD_OK && val != TD_NOFPREGS)
    error (_("sol_thread_fetch_registers: td_thr_getfpregs %s"),
	   td_err_string (val));

  /* Note that we must call supply_gregset and supply_fpregset *after*
     calling the td routines because the td routines call ps_lget*
     which affect the values stored in the registers array.  */

  supply_gregset (regcache, (const gdb_gregset_t *) gregset_p);
  supply_fpregset (regcache, (const gdb_fpregset_t *) fpregset_p);
}

void
sol_thread_target::store_registers (struct regcache *regcache, int regnum)
{
  thread_t thread;
  td_thrhandle_t thandle;
  td_err_e val;
  prgregset_t gregset;
  prfpregset_t fpregset;
  ptid_t ptid = regcache->ptid ();

  if (!ptid.tid_p ())
    {
      /* It's an LWP; pass the request on to the layer beneath.  */
      beneath ()->store_registers (regcache, regnum);
      return;
    }

  /* Solaris thread: convert PTID into a td_thrhandle_t.  */
  thread = ptid.tid ();

  val = p_td_ta_map_id2thr (main_ta, thread, &thandle);
  if (val != TD_OK)
    error (_("sol_thread_store_registers: td_ta_map_id2thr %s"),
	   td_err_string (val));

  if (regnum != -1)
    {
      val = p_td_thr_getgregs (&thandle, gregset);
      if (val != TD_OK)
	error (_("sol_thread_store_registers: td_thr_getgregs %s"),
	       td_err_string (val));
      val = p_td_thr_getfpregs (&thandle, &fpregset);
      if (val != TD_OK)
	error (_("sol_thread_store_registers: td_thr_getfpregs %s"),
	       td_err_string (val));
    }

  fill_gregset (regcache, (gdb_gregset_t *) &gregset, regnum);
  fill_fpregset (regcache, (gdb_fpregset_t *) &fpregset, regnum);

  val = p_td_thr_setgregs (&thandle, gregset);
  if (val != TD_OK)
    error (_("sol_thread_store_registers: td_thr_setgregs %s"),
	   td_err_string (val));
  val = p_td_thr_setfpregs (&thandle, &fpregset);
  if (val != TD_OK)
    error (_("sol_thread_store_registers: td_thr_setfpregs %s"),
	   td_err_string (val));
}

/* Perform partial transfers on OBJECT.  See target_read_partial and
   target_write_partial for details of each variant.  One, and only
   one, of readbuf or writebuf must be non-NULL.  */

enum target_xfer_status
sol_thread_target::xfer_partial (enum target_object object,
				 const char *annex, gdb_byte *readbuf,
				 const gdb_byte *writebuf,
				 ULONGEST offset, ULONGEST len,
				 ULONGEST *xfered_len)
{
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  if (inferior_ptid.tid_p () || !target_thread_alive (inferior_ptid))
    {
      /* It's either a thread or an LWP that isn't alive.  Any live
	 LWP will do so use the first available.

	 NOTE: We don't need to call switch_to_thread; we're just
	 reading memory.  */
      inferior_ptid = procfs_first_available ();
    }

  return beneath ()->xfer_partial (object, annex, readbuf,
				   writebuf, offset, len, xfered_len);
}

static void
check_for_thread_db (void)
{
  td_err_e err;
  ptid_t ptid;

  /* Don't attempt to use thread_db for remote targets.  */
  if (!(target_can_run () || core_bfd))
    return;

  /* Do nothing if we couldn't load libthread_db.so.1.  */
  if (p_td_ta_new == NULL)
    return;

  if (sol_thread_active)
    /* Nothing to do.  The thread library was already detected and the
       target vector was already activated.  */
    return;

  /* Now, initialize libthread_db.  This needs to be done after the
     shared libraries are located because it needs information from
     the user's thread library.  */

  err = p_td_init ();
  if (err != TD_OK)
    {
      warning (_("sol_thread_new_objfile: td_init: %s"), td_err_string (err));
      return;
    }

  /* Now attempt to open a connection to the thread library.  */
  err = p_td_ta_new (&main_ph, &main_ta);
  switch (err)
    {
    case TD_NOLIBTHREAD:
      /* No thread library was detected.  */
      break;

    case TD_OK:
      gdb_printf (_("[Thread debugging using libthread_db enabled]\n"));

      /* The thread library was detected.  Activate the sol_thread target.  */
      current_inferior ()->push_target (&sol_thread_ops);
      sol_thread_active = 1;

      main_ph.ptid = inferior_ptid; /* Save for xfer_memory.  */
      ptid = lwp_to_thread (inferior_ptid);
      if (ptid.pid () != -1)
	inferior_ptid = ptid;

      target_update_thread_list ();
      break;

    default:
      warning (_("Cannot initialize thread debugging library: %s"),
	       td_err_string (err));
      break;
    }
}

/* This routine is called whenever a new symbol table is read in, or
   when all symbol tables are removed.  libthread_db can only be
   initialized when it finds the right variables in libthread.so.
   Since it's a shared library, those variables don't show up until
   the library gets mapped and the symbol table is read in.  */

static void
sol_thread_new_objfile (struct objfile *objfile)
{
  check_for_thread_db ();
}

/* Clean up after the inferior dies.  */

void
sol_thread_target::mourn_inferior ()
{
  target_ops *beneath = this->beneath ();

  sol_thread_active = 0;

  current_inferior ()->unpush_target (this);

  beneath->mourn_inferior ();
}

/* Return true if PTID is still active in the inferior.  */

bool
sol_thread_target::thread_alive (ptid_t ptid)
{
  if (ptid.tid_p ())
    {
      /* It's a (user-level) thread.  */
      td_err_e val;
      td_thrhandle_t th;
      int pid;

      pid = ptid.tid ();
      val = p_td_ta_map_id2thr (main_ta, pid, &th);
      if (val != TD_OK)
	return false;		/* Thread not found.  */
      val = p_td_thr_validate (&th);
      if (val != TD_OK)
	return false;		/* Thread not valid.  */
      return true;		/* Known thread.  */
    }
  else
    {
      /* It's an LPW; pass the request on to the layer below.  */
      return beneath ()->thread_alive (ptid);
    }
}


/* These routines implement the lower half of the thread_db interface,
   i.e. the ps_* routines.  */

/* The next four routines are called by libthread_db to tell us to
   stop and stop a particular process or lwp.  Since GDB ensures that
   these are all stopped by the time we call anything in thread_db,
   these routines need to do nothing.  */

/* Process stop.  */

ps_err_e
ps_pstop (struct ps_prochandle *ph)
{
  return PS_OK;
}

/* Process continue.  */

ps_err_e
ps_pcontinue (struct ps_prochandle *ph)
{
  return PS_OK;
}

/* LWP stop.  */

ps_err_e
ps_lstop (struct ps_prochandle *ph, lwpid_t lwpid)
{
  return PS_OK;
}

/* LWP continue.  */

ps_err_e
ps_lcontinue (struct ps_prochandle *ph, lwpid_t lwpid)
{
  return PS_OK;
}

/* Looks up the symbol LD_SYMBOL_NAME in the debugger's symbol table.  */

ps_err_e
ps_pglobal_lookup (struct ps_prochandle *ph, const char *ld_object_name,
		   const char *ld_symbol_name, psaddr_t *ld_symbol_addr)
{
  struct bound_minimal_symbol ms;

  ms = lookup_minimal_symbol (ld_symbol_name, NULL, NULL);
  if (!ms.minsym)
    return PS_NOSYM;

  *ld_symbol_addr = ms.value_address ();
  return PS_OK;
}

/* Common routine for reading and writing memory.  */

static ps_err_e
rw_common (int dowrite, const struct ps_prochandle *ph, psaddr_t addr,
	   gdb_byte *buf, int size)
{
  int ret;

  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  if (inferior_ptid.tid_p () || !target_thread_alive (inferior_ptid))
    {
      /* It's either a thread or an LWP that isn't alive.  Any live
	 LWP will do so use the first available.

	 NOTE: We don't need to call switch_to_thread; we're just
	 reading memory.  */
      inferior_ptid = procfs_first_available ();
    }

#if defined (__sparcv9)
  /* For Sparc64 cross Sparc32, make sure the address has not been
     accidentally sign-extended (or whatever) to beyond 32 bits.  */
  if (bfd_get_arch_size (current_program_space->exec_bfd ()) == 32)
    addr &= 0xffffffff;
#endif

  if (dowrite)
    ret = target_write_memory (addr, (gdb_byte *) buf, size);
  else
    ret = target_read_memory (addr, (gdb_byte *) buf, size);

  return (ret == 0 ? PS_OK : PS_ERR);
}

/* Copies SIZE bytes from target process .data segment to debugger memory.  */

ps_err_e
ps_pdread (struct ps_prochandle *ph, psaddr_t addr, void *buf, size_t size)
{
  return rw_common (0, ph, addr, (gdb_byte *) buf, size);
}

/* Copies SIZE bytes from debugger memory .data segment to target process.  */

ps_err_e
ps_pdwrite (struct ps_prochandle *ph, psaddr_t addr,
	    const void *buf, size_t size)
{
  return rw_common (1, ph, addr, (gdb_byte *) buf, size);
}

/* Copies SIZE bytes from target process .text segment to debugger memory.  */

ps_err_e
ps_ptread (struct ps_prochandle *ph, psaddr_t addr, void *buf, size_t size)
{
  return rw_common (0, ph, addr, (gdb_byte *) buf, size);
}

/* Copies SIZE bytes from debugger memory .text segment to target process.  */

ps_err_e
ps_ptwrite (struct ps_prochandle *ph, psaddr_t addr,
	    const void *buf, size_t size)
{
  return rw_common (1, ph, addr, (gdb_byte *) buf, size);
}

/* Get general-purpose registers for LWP.  */

ps_err_e
ps_lgetregs (struct ps_prochandle *ph, lwpid_t lwpid, prgregset_t gregset)
{
  ptid_t ptid = ptid_t (current_inferior ()->pid, lwpid, 0);
  regcache *regcache = get_thread_arch_regcache (current_inferior (), ptid,
						 current_inferior ()->arch ());

  target_fetch_registers (regcache, -1);
  fill_gregset (regcache, (gdb_gregset_t *) gregset, -1);

  return PS_OK;
}

/* Set general-purpose registers for LWP.  */

ps_err_e
ps_lsetregs (struct ps_prochandle *ph, lwpid_t lwpid,
	     const prgregset_t gregset)
{
  ptid_t ptid = ptid_t (current_inferior ()->pid, lwpid, 0);
  regcache *regcache = get_thread_arch_regcache (current_inferior (), ptid,
						 current_inferior ()->arch ());

  supply_gregset (regcache, (const gdb_gregset_t *) gregset);
  target_store_registers (regcache, -1);

  return PS_OK;
}

/* Log a message (sends to gdb_stderr).  */

void
ps_plog (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);

  gdb_vprintf (gdb_stderr, fmt, args);
}

/* Get size of extra register set.  Currently a noop.  */

ps_err_e
ps_lgetxregsize (struct ps_prochandle *ph, lwpid_t lwpid, int *xregsize)
{
  return PS_OK;
}

/* Get extra register set.  Currently a noop.  */

ps_err_e
ps_lgetxregs (struct ps_prochandle *ph, lwpid_t lwpid, caddr_t xregset)
{
  return PS_OK;
}

/* Set extra register set.  Currently a noop.  */

ps_err_e
ps_lsetxregs (struct ps_prochandle *ph, lwpid_t lwpid, caddr_t xregset)
{
  return PS_OK;
}

/* Get floating-point registers for LWP.  */

ps_err_e
ps_lgetfpregs (struct ps_prochandle *ph, lwpid_t lwpid,
	       prfpregset_t *fpregset)
{
  ptid_t ptid = ptid_t (current_inferior ()->pid, lwpid, 0);
  regcache *regcache = get_thread_arch_regcache (current_inferior (), ptid,
						 current_inferior ()->arch ());

  target_fetch_registers (regcache, -1);
  fill_fpregset (regcache, (gdb_fpregset_t *) fpregset, -1);

  return PS_OK;
}

/* Set floating-point regs for LWP.  */

ps_err_e
ps_lsetfpregs (struct ps_prochandle *ph, lwpid_t lwpid,
	       const prfpregset_t * fpregset)
{
  ptid_t ptid = ptid_t (current_inferior ()->pid, lwpid, 0);
  regcache *regcache = get_thread_arch_regcache (current_inferior (), ptid,
						 current_inferior ()->arch ());

  supply_fpregset (regcache, (const gdb_fpregset_t *) fpregset);
  target_store_registers (regcache, -1);

  return PS_OK;
}

/* Identify process as 32-bit or 64-bit.  At the moment we're using
   BFD to do this.  There might be a more Solaris-specific
   (e.g. procfs) method, but this ought to work.  */

ps_err_e
ps_pdmodel (struct ps_prochandle *ph, int *data_model)
{
  if (current_program_space->exec_bfd () == 0)
    *data_model = PR_MODEL_UNKNOWN;
  else if (bfd_get_arch_size (current_program_space->exec_bfd ()) == 32)
    *data_model = PR_MODEL_ILP32;
  else
    *data_model = PR_MODEL_LP64;

  return PS_OK;
}


/* Convert PTID to printable form.  */

std::string
sol_thread_target::pid_to_str (ptid_t ptid)
{
  if (ptid.tid_p ())
    {
      ptid_t lwp;

      lwp = thread_to_lwp (ptid, -2);

      if (lwp.pid () == -1)
	return string_printf ("Thread %s (defunct)",
			      pulongest (ptid.tid ()));
      else if (lwp.pid () != -2)
	return string_printf ("Thread %s (LWP %ld)",
			      pulongest (ptid.tid ()), lwp.lwp ());
      else
	return string_printf ("Thread %s          ",
			      pulongest (ptid.tid ()));
    }
  else if (ptid.lwp () != 0)
    return string_printf ("LWP    %ld        ", ptid.lwp ());
  else
    return string_printf ("process %d    ", ptid.pid ());
}


/* Worker bee for update_thread_list.  Callback function that gets
   called once per user-level thread (i.e. not for LWP's).  */

static int
sol_update_thread_list_callback (const td_thrhandle_t *th, void *ignored)
{
  td_err_e retval;
  td_thrinfo_t ti;

  retval = p_td_thr_get_info (th, &ti);
  if (retval != TD_OK)
    return -1;

  ptid_t ptid = ptid_t (current_inferior ()->pid, 0, ti.ti_tid);
  thread_info *thr = current_inferior ()->find_thread (ptid);
  if (thr == NULL || thr->state == THREAD_EXITED)
    {
      process_stratum_target *proc_target
	= current_inferior ()->process_target ();
      add_thread (proc_target, ptid);
    }

  return 0;
}

void
sol_thread_target::update_thread_list ()
{
  /* Delete dead threads.  */
  prune_threads ();

  /* Find any new LWP's.  */
  beneath ()->update_thread_list ();

  /* Then find any new user-level threads.  */
  p_td_ta_thr_iter (main_ta, sol_update_thread_list_callback, (void *) 0,
		    TD_THR_ANY_STATE, TD_THR_LOWEST_PRIORITY,
		    TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS);
}

/* Worker bee for the "info sol-thread" command.  This is a callback
   function that gets called once for each Solaris user-level thread
   (i.e. not for LWPs) in the inferior.  Print anything interesting
   that we can think of.  */

static int
info_cb (const td_thrhandle_t *th, void *s)
{
  td_err_e ret;
  td_thrinfo_t ti;

  ret = p_td_thr_get_info (th, &ti);
  if (ret == TD_OK)
    {
      gdb_printf ("%s thread #%d, lwp %d, ",
		  ti.ti_type == TD_THR_SYSTEM ? "system" : "user  ",
		  ti.ti_tid, ti.ti_lid);
      switch (ti.ti_state)
	{
	default:
	case TD_THR_UNKNOWN:
	  gdb_printf ("<unknown state>");
	  break;
	case TD_THR_STOPPED:
	  gdb_printf ("(stopped)");
	  break;
	case TD_THR_RUN:
	  gdb_printf ("(run)    ");
	  break;
	case TD_THR_ACTIVE:
	  gdb_printf ("(active) ");
	  break;
	case TD_THR_ZOMBIE:
	  gdb_printf ("(zombie) ");
	  break;
	case TD_THR_SLEEP:
	  gdb_printf ("(asleep) ");
	  break;
	case TD_THR_STOPPED_ASLEEP:
	  gdb_printf ("(stopped asleep)");
	  break;
	}
      /* Print thr_create start function.  */
      if (ti.ti_startfunc != 0)
	{
	  const struct bound_minimal_symbol msym
	    = lookup_minimal_symbol_by_pc (ti.ti_startfunc);

	  gdb_printf ("   startfunc=%s",
		      msym.minsym
		      ? msym.minsym->print_name ()
		      : paddress (current_inferior ()->arch (),
				  ti.ti_startfunc));
	}

      /* If thread is asleep, print function that went to sleep.  */
      if (ti.ti_state == TD_THR_SLEEP)
	{
	  const struct bound_minimal_symbol msym
	    = lookup_minimal_symbol_by_pc (ti.ti_pc);

	  gdb_printf ("   sleepfunc=%s",
		      msym.minsym
		      ? msym.minsym->print_name ()
		      : paddress (current_inferior ()->arch (), ti.ti_pc));
	}

      gdb_printf ("\n");
    }
  else
    warning (_("info sol-thread: failed to get info for thread."));

  return 0;
}

/* List some state about each Solaris user-level thread in the
   inferior.  */

static void
info_solthreads (const char *args, int from_tty)
{
  p_td_ta_thr_iter (main_ta, info_cb, (void *) args,
		    TD_THR_ANY_STATE, TD_THR_LOWEST_PRIORITY,
		    TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS);
}

/* Callback routine used to find a thread based on the TID part of
   its PTID.  */

static int
thread_db_find_thread_from_tid (struct thread_info *thread, void *data)
{
  ULONGEST *tid = (ULONGEST *) data;

  if (thread->ptid.tid () == *tid)
    return 1;

  return 0;
}

ptid_t
sol_thread_target::get_ada_task_ptid (long lwp, ULONGEST thread)
{
  struct thread_info *thread_info =
    iterate_over_threads (thread_db_find_thread_from_tid, &thread);

  if (thread_info == NULL)
    {
      /* The list of threads is probably not up to date.  Find any
	 thread that is missing from the list, and try again.  */
      update_thread_list ();
      thread_info = iterate_over_threads (thread_db_find_thread_from_tid,
					  &thread);
    }

  gdb_assert (thread_info != NULL);

  return (thread_info->ptid);
}

void _initialize_sol_thread ();
void
_initialize_sol_thread ()
{
  void *dlhandle;

  dlhandle = dlopen ("libthread_db.so.1", RTLD_NOW);
  if (!dlhandle)
    goto die;

#define resolve(X) \
  if (!(p_##X = (X ## _ftype *) dlsym (dlhandle, #X)))	\
    goto die;

  resolve (td_log);
  resolve (td_ta_new);
  resolve (td_ta_delete);
  resolve (td_init);
  resolve (td_ta_get_ph);
  resolve (td_ta_get_nthreads);
  resolve (td_ta_tsd_iter);
  resolve (td_ta_thr_iter);
  resolve (td_thr_validate);
  resolve (td_thr_tsd);
  resolve (td_thr_get_info);
  resolve (td_thr_getfpregs);
  resolve (td_thr_getxregsize);
  resolve (td_thr_getxregs);
  resolve (td_thr_sigsetmask);
  resolve (td_thr_setprio);
  resolve (td_thr_setsigpending);
  resolve (td_thr_setfpregs);
  resolve (td_thr_setxregs);
  resolve (td_ta_map_id2thr);
  resolve (td_ta_map_lwp2thr);
  resolve (td_thr_getgregs);
  resolve (td_thr_setgregs);

  add_cmd ("sol-threads", class_maintenance, info_solthreads,
	   _("Show info on Solaris user threads."), &maintenanceinfolist);

  /* Hook into new_objfile notification.  */
  gdb::observers::new_objfile.attach (sol_thread_new_objfile, "sol-thread");
  return;

 die:
  gdb_printf (gdb_stderr, "\
[GDB will not be able to debug user-mode threads: %s]\n", dlerror ());

  if (dlhandle)
    dlclose (dlhandle);

  return;
}

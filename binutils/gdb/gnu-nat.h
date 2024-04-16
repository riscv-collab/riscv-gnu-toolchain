/* Common things used by the various *gnu-nat.c files
   Copyright (C) 1995-2024 Free Software Foundation, Inc.

   Written by Miles Bader <miles@gnu.ai.mit.edu>

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

#ifndef GNU_NAT_H
#define GNU_NAT_H

#include "defs.h"

/* Work around conflict between Mach's 'thread_info' function, and GDB's
   'thread_info' class.  Make the former available as 'mach_thread_info'.  */
#define thread_info mach_thread_info
/* Mach headers are not yet ready for C++ compilation.  */
extern "C"
{
#include <mach.h>
}
#undef thread_info
/* Divert 'mach_thread_info' to the original Mach 'thread_info' function.  */
extern __typeof__ (mach_thread_info) mach_thread_info asm ("thread_info");

#include <unistd.h>

#include "inf-child.h"

struct inf;

extern struct inf *gnu_current_inf;

/* Converts a GDB pid to a struct proc.  */
struct proc *inf_tid_to_thread (struct inf *inf, int tid);

typedef void (inf_threads_ftype) (struct proc *thread, void *arg);

/* Call F for every thread in inferior INF, passing ARG as second parameter.  */
void inf_threads (struct inf *inf, inf_threads_ftype *f, void *arg);

/* Makes sure that INF's thread list is synced with the actual process.  */
int inf_update_procs (struct inf *inf);

/* A proc is either a thread, or the task (there can only be one task proc
   because it always has the same TID, PROC_TID_TASK).  */
struct proc
  {
    thread_t port;		/* The task or thread port.  */
    int tid;			/* The GDB pid (actually a thread id).  */
    int num;			/* An id number for threads, to print.  */

    mach_port_t saved_exc_port;	/* The task/thread's real exception port.  */
    mach_port_t exc_port;	/* Our replacement, which for.  */

    int sc;			/* Desired suspend count.   */
    int cur_sc;			/* Implemented suspend count.  */
    int run_sc;			/* Default sc when the program is running.  */
    int pause_sc;		/* Default sc when gdb has control.  */
    int resume_sc;		/* Sc resulting from the last resume.  */
    int detach_sc;		/* SC to leave around when detaching
				   from program.  */

    thread_state_data_t state;	/* Registers, &c.  */
    int state_valid:1;		/* True if STATE is up to date.  */
    int state_changed:1;

    int aborted:1;		/* True if thread_abort has been called.  */
    int dead:1;			/* We happen to know it's actually dead.  */

    /* Bit mask of registers fetched by gdb.  This is used when we re-fetch
       STATE after aborting the thread, to detect that gdb may have out-of-date
       information.  */
    unsigned long fetched_regs;

    struct inf *inf;		/* Where we come from.  */

    struct proc *next;
  };

/* The task has a thread entry with this TID.  */
#define PROC_TID_TASK 	(-1)

#define proc_is_task(proc) ((proc)->tid == PROC_TID_TASK)
#define proc_is_thread(proc) ((proc)->tid != PROC_TID_TASK)

extern int __proc_pid (struct proc *proc);

/* Return printable description of proc.  */
extern char *proc_string (struct proc *proc);

#define proc_debug(_proc, msg, args...) \
  do { struct proc *__proc = (_proc); \
       debug ("{proc %d/%d %s}: " msg, \
	      __proc_pid (__proc), __proc->tid, \
	      host_address_to_string (__proc) , ##args); } while (0)

extern bool gnu_debug_flag;

#define debug(msg, args...) \
 do { if (gnu_debug_flag) \
     gdb_printf (gdb_stdlog, "%s:%d: " msg "\r\n",		\
		 __FILE__ , __LINE__ , ##args); } while (0)

/* A prototype generic GNU/Hurd target.  The client can override it
   with local methods.  */

struct gnu_nat_target : public inf_child_target
{
  void attach (const char *, int) override;
  bool attach_no_wait () override
  { return true; }

  void detach (inferior *, int) override;
  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;
  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  int find_memory_regions (find_memory_region_ftype func, void *data)
    override;
  void kill () override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;
  void mourn_inferior () override;
  bool thread_alive (ptid_t ptid) override;
  std::string pid_to_str (ptid_t) override;
  void stop (ptid_t) override;

  void inf_validate_procs (struct inf *inf);
  void inf_suspend (struct inf *inf);
  void inf_set_traced (struct inf *inf, int on);
  void steal_exc_port (struct proc *proc, mach_port_t name);

  /* Make sure that the state field in PROC is up to date, and return a
     pointer to it, or 0 if something is wrong.  If WILL_MODIFY is true,
     makes sure that the thread is stopped and aborted first, and sets
     the state_changed field in PROC to true.  */
  thread_state_t proc_get_state (struct proc *proc, int will_modify);

private:
  void inf_clear_wait (struct inf *inf);
  void inf_cleanup (struct inf *inf);
  void inf_startup (struct inf *inf, int pid);
  int inf_update_suspends (struct inf *inf);
  void inf_set_pid (struct inf *inf, pid_t pid);
  void inf_steal_exc_ports (struct inf *inf);
  void inf_validate_procinfo (struct inf *inf);
  void inf_validate_task_sc (struct inf *inf);
  void inf_restore_exc_ports (struct inf *inf);
  void inf_set_threads_resume_sc (struct inf *inf,
				  struct proc *run_thread,
				  int run_others);
  int inf_set_threads_resume_sc_for_signal_thread (struct inf *inf);
  void inf_resume (struct inf *inf);
  void inf_set_step_thread (struct inf *inf, struct proc *proc);
  void inf_detach (struct inf *inf);
  void inf_attach (struct inf *inf, int pid);
  void inf_signal (struct inf *inf, enum gdb_signal sig);
  void inf_continue (struct inf *inf);

  struct proc *make_proc (struct inf *inf, mach_port_t port, int tid);
  void proc_abort (struct proc *proc, int force);
  struct proc *_proc_free (struct proc *proc);
  int proc_update_sc (struct proc *proc);
  kern_return_t proc_get_exception_port (struct proc *proc, mach_port_t * port);
  kern_return_t proc_set_exception_port (struct proc *proc, mach_port_t port);
  mach_port_t _proc_get_exc_port (struct proc *proc);
  void proc_steal_exc_port (struct proc *proc, mach_port_t exc_port);
  void proc_restore_exc_port (struct proc *proc);
  int proc_trace (struct proc *proc, int set);
};

/* The final/concrete instance.  */
extern gnu_nat_target *gnu_target;

#endif /* GNU_NAT_H */

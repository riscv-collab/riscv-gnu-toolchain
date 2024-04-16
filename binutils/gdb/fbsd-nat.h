/* Native-dependent code for FreeBSD.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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

#ifndef FBSD_NAT_H
#define FBSD_NAT_H

#include <optional>
#include "inf-ptrace.h"
#include "regcache.h"
#include "regset.h"
#include <osreldate.h>
#include <sys/proc.h>

#include <list>

/* FreeBSD kernels 11.3 and later report valid si_code values for
   SIGTRAP on all architectures.  Older FreeBSD kernels that supported
   TRAP_BRKPT did not report valid values for MIPS and sparc64.  Even
   older kernels without TRAP_BRKPT support did not report valid
   values on any architecture.  */
#if (__FreeBSD_kernel_version >= 1102502) || (__FreeBSD_version >= 1102502)
# define USE_SIGTRAP_SIGINFO
#elif defined(TRAP_BRKPT)
# if !defined(__mips__) && !defined(__sparc64__)
#  define USE_SIGTRAP_SIGINFO
# endif
#endif

/* A prototype FreeBSD target.  */

class fbsd_nat_target : public inf_ptrace_target
{
public:
  const char *pid_to_exec_file (int pid) override;

  int find_memory_regions (find_memory_region_ftype func, void *data) override;

  bool info_proc (const char *, enum info_proc_what) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  bool thread_alive (ptid_t ptid) override;
  std::string pid_to_str (ptid_t) override;

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME
  const char *thread_name (struct thread_info *) override;
#endif

  void update_thread_list () override;

  bool can_async_p () override;

  void async (bool) override;

  thread_control_capabilities get_thread_control_capabilities () override
  { return tc_schedlock; }

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void attach (const char *, int) override;

  void detach (inferior *, int) override;

  void kill () override;

  void mourn_inferior () override;

  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void post_attach (int) override;

#ifdef USE_SIGTRAP_SIGINFO
  bool supports_stopped_by_sw_breakpoint () override;
  bool stopped_by_sw_breakpoint () override;
#endif

  void follow_exec (inferior *, ptid_t, const char *) override;

#ifdef TDP_RFPPWAIT
  void follow_fork (inferior *, ptid_t, target_waitkind, bool, bool) override;

  int insert_fork_catchpoint (int) override;
  int remove_fork_catchpoint (int) override;

  int insert_vfork_catchpoint (int) override;
  int remove_vfork_catchpoint (int) override;
#endif

  int insert_exec_catchpoint (int) override;
  int remove_exec_catchpoint (int) override;

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
  int set_syscall_catchpoint (int, bool, int, gdb::array_view<const int>)
    override;
#endif

  bool supports_multi_process () override;

  bool supports_disable_randomization () override;

  /* Methods meant to be overridden by arch-specific target
     classes.  */

  virtual void low_new_fork (ptid_t parent, pid_t child)
  {}

  /* The method to call, if any, when a thread is destroyed.  */
  virtual void low_delete_thread (thread_info *)
  {}

  /* Hook to call prior to resuming a thread.  */
  virtual void low_prepare_to_resume (thread_info *)
  {}

protected:

  void post_startup_inferior (ptid_t) override;

private:
  ptid_t wait_1 (ptid_t, struct target_waitstatus *, target_wait_flags);

  void resume_one_process (ptid_t, int, enum gdb_signal);

  void stop_process (inferior *);

  /* Helper routines for use in fetch_registers and store_registers in
     subclasses.  These routines fetch and store a single set of
     registers described by REGSET.  The REGSET's 'regmap' field must
     point to an array of 'struct regcache_map_entry'.  The valid
     register numbers in the register map are relative to REGBASE.

     FETCH_OP is a ptrace operation to fetch the set of registers from
     a native thread.  STORE_OP is a ptrace operation to store the set
     of registers to a native thread.

     The caller must provide storage for the set of registers in REGS,
     and SIZE is the size of the storage.

     Returns true if the register set was transferred due to a
     matching REGNUM.  */

  bool fetch_register_set (struct regcache *regcache, int regnum, int fetch_op,
			   const struct regset *regset, int regbase, void *regs,
			   size_t size);

  bool store_register_set (struct regcache *regcache, int regnum, int fetch_op,
			   int store_op, const struct regset *regset,
			   int regbase, void *regs, size_t size);

  /* Helper routines which use PT_GETREGSET and PT_SETREGSET for the
     specified NOTE instead of regset-specific fetch and store
     ops.  */

  bool fetch_regset (struct regcache *regcache, int regnum, int note,
		     const struct regset *regset, int regbase, void *regs,
		     size_t size);

  bool store_regset (struct regcache *regcache, int regnum, int note,
		     const struct regset *regset, int regbase, void *regs,
		     size_t size);

protected:
  /* Wrapper versions of the above helpers which accept a register set
     type such as 'struct reg' or 'struct fpreg'.  */

  template <class Regset>
  bool fetch_register_set (struct regcache *regcache, int regnum, int fetch_op,
			   const struct regset *regset, int regbase = 0)
  {
    Regset regs;
    return fetch_register_set (regcache, regnum, fetch_op, regset, regbase,
			       &regs, sizeof (regs));
  }

  template <class Regset>
  bool store_register_set (struct regcache *regcache, int regnum, int fetch_op,
			   int store_op, const struct regset *regset,
			   int regbase = 0)
  {
    Regset regs;
    return store_register_set (regcache, regnum, fetch_op, store_op, regset,
			       regbase, &regs, sizeof (regs));
  }

  /* Helper routine for use in read_description in subclasses.  This
     routine checks if the register set for the specified NOTE is
     present for a given PTID.  If the register set is present, the
     the size of the register set is returned.  If the register set is
     not present, zero is returned.  */

  size_t have_regset (ptid_t ptid, int note);

  /* Wrapper versions of the PT_GETREGSET and PT_REGSET helpers which
     accept a register set type.  */

  template <class Regset>
  bool fetch_regset (struct regcache *regcache, int regnum, int note,
		     const struct regset *regset, int regbase = 0)
  {
    Regset regs;
    return fetch_regset (regcache, regnum, note, regset, regbase, &regs,
			 sizeof (regs));
  }

  template <class Regset>
  bool store_regset (struct regcache *regcache, int regnum, int note,
		     const struct regset *regset, int regbase = 0)
  {
    Regset regs;
    return store_regset (regcache, regnum, note, regset, regbase, &regs,
			 sizeof (regs));
  }

private:
  /* If an event is triggered asynchronously (fake vfork_done events)
     or occurs when the core is not expecting it, a pending event is
     created.  This event is then returned by a future call to the
     target wait method.  */

  struct pending_event
  {
    pending_event (const ptid_t &_ptid, const target_waitstatus &_status) :
      ptid (_ptid), status (_status) {}

    ptid_t ptid;
    target_waitstatus status;
  };

  /* Add a new pending event to the list.  */

  void add_pending_event (const ptid_t &ptid, const target_waitstatus &status);

  /* Return true if there is a pending event matching FILTER.  */

  bool have_pending_event (ptid_t filter);

  /* Check if there is a pending event for a resumed process matching
     FILTER.  If there is a matching event, the event is removed from
     the pending list and returned.  */

  std::optional<pending_event> take_pending_event (ptid_t filter);

  /* List of pending events.  */

  std::list<pending_event> m_pending_events;

  /* If this thread has a pending fork event, there is a child process
     GDB is attached to that the core of GDB doesn't know about.
     Detach from it.  */

  void detach_fork_children (thread_info *tp);

  /* Detach from any child processes associated with pending fork events
     for a stopped process.  Returns true if the process has terminated
     and false if it is still alive.  */

  bool detach_fork_children (inferior *inf);
};

/* Fetch the signal information for PTID and store it in *SIGINFO.
   Return true if successful.  */
bool fbsd_nat_get_siginfo (ptid_t ptid, siginfo_t *siginfo);

#endif /* fbsd-nat.h */

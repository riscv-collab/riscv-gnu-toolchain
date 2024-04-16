/* Target-vector operations for controlling windows child processes, for GDB.

   Copyright (C) 1995-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions, A Red Hat Company.

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

/* Originally by Steve Chamberlain, sac@cygnus.com */

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "infrun.h"
#include "target.h"
#include "gdbcore.h"
#include "command.h"
#include "completer.h"
#include "regcache.h"
#include "top.h"
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <windows.h>
#include <imagehlp.h>
#ifdef __CYGWIN__
#include <wchar.h>
#include <sys/cygwin.h>
#include <cygwin/version.h>
#endif
#include <algorithm>
#include <vector>
#include <queue>

#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdb_bfd.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbthread.h"
#include "gdbcmd.h"
#include <unistd.h>
#include "exec.h"
#include "solist.h"
#include "solib.h"
#include "xml-support.h"
#include "inttypes.h"

#include "i386-tdep.h"
#include "i387-tdep.h"

#include "windows-tdep.h"
#include "windows-nat.h"
#include "x86-nat.h"
#include "complaints.h"
#include "inf-child.h"
#include "gdbsupport/gdb_tilde_expand.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/gdb_wait.h"
#include "nat/windows-nat.h"
#include "gdbsupport/symbol.h"
#include "ser-event.h"
#include "inf-loop.h"

using namespace windows_nat;

/* Maintain a linked list of "so" information.  */
struct windows_solib
{
  LPVOID load_addr = 0;
  CORE_ADDR text_offset = 0;

  /* Original name.  */
  std::string original_name;
  /* Expanded form of the name.  */
  std::string name;
};

struct windows_per_inferior : public windows_process_info
{
  windows_thread_info *thread_rec (ptid_t ptid,
				   thread_disposition_type disposition) override;
  int handle_output_debug_string (struct target_waitstatus *ourstatus) override;
  void handle_load_dll (const char *dll_name, LPVOID base) override;
  void handle_unload_dll () override;
  bool handle_access_violation (const EXCEPTION_RECORD *rec) override;


  int have_saved_context = 0;	/* True if we've saved context from a
				   cygwin signal.  */

  uintptr_t dr[8] {};

  int windows_initialization_done = 0;

  std::vector<std::unique_ptr<windows_thread_info>> thread_list;

  /* Counts of things.  */
  int saw_create = 0;
  int open_process_used = 0;
#ifdef __x86_64__
  void *wow64_dbgbreak = nullptr;
#endif

  /* This vector maps GDB's idea of a register's number into an offset
     in the windows exception context vector.

     It also contains the bit mask needed to load the register in question.

     The contents of this table can only be computed by the units
     that provide CPU-specific support for Windows native debugging.

     One day we could read a reg, we could inspect the context we
     already have loaded, if it doesn't have the bit set that we need,
     we read that set of registers in using GetThreadContext.  If the
     context already contains what we need, we just unpack it.  Then to
     write a register, first we have to ensure that the context contains
     the other regs of the group, and then we copy the info in and set
     out bit.  */

  const int *mappings = nullptr;

  /* The function to use in order to determine whether a register is
     a segment register or not.  */
  segment_register_p_ftype *segment_register_p = nullptr;

  std::vector<windows_solib> solibs;

#ifdef __CYGWIN__
  CONTEXT saved_context {};	/* Contains the saved context from a
				   cygwin signal.  */

  /* The starting and ending address of the cygwin1.dll text segment.  */
  CORE_ADDR cygwin_load_start = 0;
  CORE_ADDR cygwin_load_end = 0;
#endif /* __CYGWIN__ */
};

/* The current process.  */
static windows_per_inferior windows_process;

#undef STARTUPINFO

#ifndef __CYGWIN__
# define __PMAX	(MAX_PATH + 1)
# define STARTUPINFO STARTUPINFOA
#else
# define __PMAX	PATH_MAX
#   define STARTUPINFO STARTUPINFOW
#endif

/* If we're not using the old Cygwin header file set, define the
   following which never should have been in the generic Win32 API
   headers in the first place since they were our own invention...  */
#ifndef _GNU_H_WINDOWS_H
enum
  {
    FLAG_TRACE_BIT = 0x100,
  };
#endif

#ifndef CONTEXT_EXTENDED_REGISTERS
/* This macro is only defined on ia32.  It only makes sense on this target,
   so define it as zero if not already defined.  */
#define CONTEXT_EXTENDED_REGISTERS 0
#endif

#define CONTEXT_DEBUGGER_DR CONTEXT_FULL | CONTEXT_FLOATING_POINT \
	| CONTEXT_SEGMENTS | CONTEXT_DEBUG_REGISTERS \
	| CONTEXT_EXTENDED_REGISTERS

#define DR6_CLEAR_VALUE 0xffff0ff0

/* The string sent by cygwin when it processes a signal.
   FIXME: This should be in a cygwin include file.  */
#ifndef _CYGWIN_SIGNAL_STRING
#define _CYGWIN_SIGNAL_STRING "cYgSiGw00f"
#endif

#define CHECK(x)	check (x, __FILE__,__LINE__)
#define DEBUG_EXEC(fmt, ...) \
  debug_prefixed_printf_cond (debug_exec, "windows exec", fmt, ## __VA_ARGS__)
#define DEBUG_EVENTS(fmt, ...) \
  debug_prefixed_printf_cond (debug_events, "windows events", fmt, \
			      ## __VA_ARGS__)
#define DEBUG_MEM(fmt, ...) \
  debug_prefixed_printf_cond (debug_memory, "windows mem", fmt, \
			      ## __VA_ARGS__)
#define DEBUG_EXCEPT(fmt, ...) \
  debug_prefixed_printf_cond (debug_exceptions, "windows except", fmt, \
			      ## __VA_ARGS__)

static void cygwin_set_dr (int i, CORE_ADDR addr);
static void cygwin_set_dr7 (unsigned long val);
static CORE_ADDR cygwin_get_dr (int i);
static unsigned long cygwin_get_dr6 (void);
static unsigned long cygwin_get_dr7 (void);

/* User options.  */
static bool new_console = false;
#ifdef __CYGWIN__
static bool cygwin_exceptions = false;
#endif
static bool new_group = true;
static bool debug_exec = false;		/* show execution */
static bool debug_events = false;	/* show events from kernel */
static bool debug_memory = false;	/* show target memory accesses */
static bool debug_exceptions = false;	/* show target exceptions */
static bool useshell = false;		/* use shell for subprocesses */

/* See windows_nat_target::resume to understand why this is commented
   out.  */
#if 0
/* This vector maps the target's idea of an exception (extracted
   from the DEBUG_EVENT structure) to GDB's idea.  */

struct xlate_exception
  {
    DWORD them;
    enum gdb_signal us;
  };

static const struct xlate_exception xlate[] =
{
  {EXCEPTION_ACCESS_VIOLATION, GDB_SIGNAL_SEGV},
  {STATUS_STACK_OVERFLOW, GDB_SIGNAL_SEGV},
  {EXCEPTION_BREAKPOINT, GDB_SIGNAL_TRAP},
  {DBG_CONTROL_C, GDB_SIGNAL_INT},
  {EXCEPTION_SINGLE_STEP, GDB_SIGNAL_TRAP},
  {STATUS_FLOAT_DIVIDE_BY_ZERO, GDB_SIGNAL_FPE}
};

#endif /* 0 */

struct windows_nat_target final : public x86_nat_target<inf_child_target>
{
  windows_nat_target ();

  void close () override;

  void attach (const char *, int) override;

  bool attach_no_wait () override
  { return true; }

  void detach (inferior *, int) override;

  void resume (ptid_t, int , enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  bool stopped_by_sw_breakpoint () override
  {
    windows_thread_info *th
      = windows_process.thread_rec (inferior_ptid, DONT_INVALIDATE_CONTEXT);
    return th->stopped_at_software_breakpoint;
  }

  bool supports_stopped_by_sw_breakpoint () override
  {
    return true;
  }

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void files_info () override;

  void kill () override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void mourn_inferior () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  void interrupt () override;
  void pass_ctrlc () override;

  const char *pid_to_exec_file (int pid) override;

  ptid_t get_ada_task_ptid (long lwp, ULONGEST thread) override;

  bool get_tib_address (ptid_t ptid, CORE_ADDR *addr) override;

  const char *thread_name (struct thread_info *) override;

  ptid_t get_windows_debug_event (int pid, struct target_waitstatus *ourstatus,
				  target_wait_flags options);

  void do_initial_windows_stuff (DWORD pid, bool attaching);

  bool supports_disable_randomization () override
  {
    return disable_randomization_available ();
  }

  bool can_async_p () override
  {
    return true;
  }

  bool is_async_p () override
  {
    return m_is_async;
  }

  void async (bool enable) override;

  int async_wait_fd () override
  {
    return serial_event_fd (m_wait_event);
  }

private:

  windows_thread_info *add_thread (ptid_t ptid, HANDLE h, void *tlb,
				   bool main_thread_p);
  void delete_thread (ptid_t ptid, DWORD exit_code, bool main_thread_p);
  DWORD fake_create_process ();

  BOOL windows_continue (DWORD continue_status, int id, int killed,
			 bool last_call = false);

  /* Helper function to start process_thread.  */
  static DWORD WINAPI process_thread_starter (LPVOID self);

  /* This function implements the background thread that starts
     inferiors and waits for events.  */
  void process_thread ();

  /* Push FUNC onto the queue of requests for process_thread, and wait
     until it has been called.  On Windows, certain debugging
     functions can only be called by the thread that started (or
     attached to) the inferior.  These are all done in the worker
     thread, via calls to this method.  If FUNC returns true,
     process_thread will wait for debug events when FUNC returns.  */
  void do_synchronously (gdb::function_view<bool ()> func);

  /* This waits for a debug event, dispatching to the worker thread as
     needed.  */
  void wait_for_debug_event_main_thread (DEBUG_EVENT *event);

  /* Queue used to send requests to process_thread.  This is
     implicitly locked.  */
  std::queue<gdb::function_view<bool ()>> m_queue;

  /* Event used to signal process_thread that an item has been
     pushed.  */
  HANDLE m_pushed_event;
  /* Event used by process_thread to indicate that it has processed a
     single function call.  */
  HANDLE m_response_event;

  /* Serial event used to communicate wait event availability to the
     main loop.  */
  serial_event *m_wait_event;

  /* The last debug event, when M_WAIT_EVENT has been set.  */
  DEBUG_EVENT m_last_debug_event {};
  /* True if a debug event is pending.  */
  std::atomic<bool> m_debug_event_pending { false };

  /* True if currently in async mode.  */
  bool m_is_async = false;
};

static void
check (BOOL ok, const char *file, int line)
{
  if (!ok)
    {
      unsigned err = (unsigned) GetLastError ();
      gdb_printf ("error return %s:%d was %u: %s\n", file, line,
		  err, strwinerror (err));
    }
}

windows_nat_target::windows_nat_target ()
  : m_pushed_event (CreateEvent (nullptr, false, false, nullptr)),
    m_response_event (CreateEvent (nullptr, false, false, nullptr)),
    m_wait_event (make_serial_event ())
{
  HANDLE bg_thread = CreateThread (nullptr, 64 * 1024,
				   process_thread_starter, this, 0, nullptr);
  CloseHandle (bg_thread);
}

void
windows_nat_target::async (bool enable)
{
  if (enable == is_async_p ())
    return;

  if (enable)
    add_file_handler (async_wait_fd (),
		      [] (int, gdb_client_data)
		      {
			inferior_event_handler (INF_REG_EVENT);
		      },
		      nullptr, "windows_nat_target");
  else
    delete_file_handler (async_wait_fd ());

  m_is_async = enable;
}

/* A wrapper for WaitForSingleObject that issues a warning if
   something unusual happens.  */
static void
wait_for_single (HANDLE handle, DWORD howlong)
{
  while (true)
    {
      DWORD r = WaitForSingleObject (handle, howlong);
      if (r == WAIT_OBJECT_0)
	return;
      if (r == WAIT_FAILED)
	{
	  unsigned err = (unsigned) GetLastError ();
	  warning ("WaitForSingleObject failed (code %u): %s",
		   err, strwinerror (err));
	}
      else
	warning ("unexpected result from WaitForSingleObject: %u",
		 (unsigned) r);
    }
}

DWORD WINAPI
windows_nat_target::process_thread_starter (LPVOID self)
{
  ((windows_nat_target *) self)->process_thread ();
  return 0;
}

void
windows_nat_target::process_thread ()
{
  while (true)
    {
      wait_for_single (m_pushed_event, INFINITE);

      gdb::function_view<bool ()> func = std::move (m_queue.front ());
      m_queue.pop ();

      bool should_wait = func ();
      SetEvent (m_response_event);

      if (should_wait)
	{
	  if (!m_debug_event_pending)
	    {
	      wait_for_debug_event (&m_last_debug_event, INFINITE);
	      m_debug_event_pending = true;
	    }
	  serial_event_set (m_wait_event);
	}
   }
}

void
windows_nat_target::do_synchronously (gdb::function_view<bool ()> func)
{
  m_queue.emplace (std::move (func));
  SetEvent (m_pushed_event);
  wait_for_single (m_response_event, INFINITE);
}

void
windows_nat_target::wait_for_debug_event_main_thread (DEBUG_EVENT *event)
{
  do_synchronously ([&] ()
    {
      if (m_debug_event_pending)
	{
	  *event = m_last_debug_event;
	  m_debug_event_pending = false;
	  serial_event_clear (m_wait_event);
	}
      else
	wait_for_debug_event (event, INFINITE);
      return false;
    });
}

/* See nat/windows-nat.h.  */

windows_thread_info *
windows_per_inferior::thread_rec
     (ptid_t ptid, thread_disposition_type disposition)
{
  for (auto &th : thread_list)
    if (th->tid == ptid.lwp ())
      {
	if (!th->suspended)
	  {
	    switch (disposition)
	      {
	      case DONT_INVALIDATE_CONTEXT:
		/* Nothing.  */
		break;
	      case INVALIDATE_CONTEXT:
		if (ptid.lwp () != current_event.dwThreadId)
		  th->suspend ();
		th->reload_context = true;
		break;
	      case DONT_SUSPEND:
		th->reload_context = true;
		th->suspended = -1;
		break;
	      }
	  }
	return th.get ();
      }

  return NULL;
}

/* Add a thread to the thread list.

   PTID is the ptid of the thread to be added.
   H is its Windows handle.
   TLB is its thread local base.
   MAIN_THREAD_P should be true if the thread to be added is
   the main thread, false otherwise.  */

windows_thread_info *
windows_nat_target::add_thread (ptid_t ptid, HANDLE h, void *tlb,
				bool main_thread_p)
{
  windows_thread_info *th;

  gdb_assert (ptid.lwp () != 0);

  if ((th = windows_process.thread_rec (ptid, DONT_INVALIDATE_CONTEXT)))
    return th;

  CORE_ADDR base = (CORE_ADDR) (uintptr_t) tlb;
#ifdef __x86_64__
  /* For WOW64 processes, this is actually the pointer to the 64bit TIB,
     and the 32bit TIB is exactly 2 pages after it.  */
  if (windows_process.wow64_process)
    base += 0x2000;
#endif
  th = new windows_thread_info (ptid.lwp (), h, base);
  windows_process.thread_list.emplace_back (th);

  /* Add this new thread to the list of threads.

     To be consistent with what's done on other platforms, we add
     the main thread silently (in reality, this thread is really
     more of a process to the user than a thread).  */
  if (main_thread_p)
    add_thread_silent (this, ptid);
  else
    ::add_thread (this, ptid);

  /* It's simplest to always set this and update the debug
     registers.  */
  th->debug_registers_changed = true;

  return th;
}

/* Clear out any old thread list and reinitialize it to a
   pristine state.  */
static void
windows_init_thread_list (void)
{
  DEBUG_EVENTS ("called");
  windows_process.thread_list.clear ();
}

/* Delete a thread from the list of threads.

   PTID is the ptid of the thread to be deleted.
   EXIT_CODE is the thread's exit code.
   MAIN_THREAD_P should be true if the thread to be deleted is
   the main thread, false otherwise.  */

void
windows_nat_target::delete_thread (ptid_t ptid, DWORD exit_code,
				   bool main_thread_p)
{
  DWORD id;

  gdb_assert (ptid.lwp () != 0);

  id = ptid.lwp ();

  /* Note that no notification was printed when the main thread was
     created, and thus, unless in verbose mode, we should be symmetrical,
     and avoid an exit notification for the main thread here as well.  */

  bool silent = (main_thread_p && !info_verbose);
  thread_info *to_del = this->find_thread (ptid);
  delete_thread_with_exit_code (to_del, exit_code, silent);

  auto iter = std::find_if (windows_process.thread_list.begin (),
			    windows_process.thread_list.end (),
			    [=] (std::unique_ptr<windows_thread_info> &th)
			    {
			      return th->tid == id;
			    });

  if (iter != windows_process.thread_list.end ())
    windows_process.thread_list.erase (iter);
}

/* Fetches register number R from the given windows_thread_info,
   and supplies its value to the given regcache.

   This function assumes that R is non-negative.  A failed assertion
   is raised if that is not true.

   This function assumes that TH->RELOAD_CONTEXT is not set, meaning
   that the windows_thread_info has an up-to-date context.  A failed
   assertion is raised if that assumption is violated.  */

static void
windows_fetch_one_register (struct regcache *regcache,
			    windows_thread_info *th, int r)
{
  gdb_assert (r >= 0);
  gdb_assert (!th->reload_context);

  char *context_ptr = (char *) &th->context;
#ifdef __x86_64__
  if (windows_process.wow64_process)
    context_ptr = (char *) &th->wow64_context;
#endif

  char *context_offset = context_ptr + windows_process.mappings[r];
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  gdb_assert (!gdbarch_read_pc_p (gdbarch));
  gdb_assert (gdbarch_pc_regnum (gdbarch) >= 0);
  gdb_assert (!gdbarch_write_pc_p (gdbarch));

  if (r == I387_FISEG_REGNUM (tdep))
    {
      long l = *((long *) context_offset) & 0xffff;
      regcache->raw_supply (r, (char *) &l);
    }
  else if (r == I387_FOP_REGNUM (tdep))
    {
      long l = (*((long *) context_offset) >> 16) & ((1 << 11) - 1);
      regcache->raw_supply (r, (char *) &l);
    }
  else if (windows_process.segment_register_p (r))
    {
      /* GDB treats segment registers as 32bit registers, but they are
	 in fact only 16 bits long.  Make sure we do not read extra
	 bits from our source buffer.  */
      long l = *((long *) context_offset) & 0xffff;
      regcache->raw_supply (r, (char *) &l);
    }
  else
    {
      if (th->stopped_at_software_breakpoint
	  && !th->pc_adjusted
	  && r == gdbarch_pc_regnum (gdbarch))
	{
	  int size = register_size (gdbarch, r);
	  if (size == 4)
	    {
	      uint32_t value;
	      memcpy (&value, context_offset, size);
	      value -= gdbarch_decr_pc_after_break (gdbarch);
	      memcpy (context_offset, &value, size);
	    }
	  else
	    {
	      gdb_assert (size == 8);
	      uint64_t value;
	      memcpy (&value, context_offset, size);
	      value -= gdbarch_decr_pc_after_break (gdbarch);
	      memcpy (context_offset, &value, size);
	    }
	  /* Make sure we only rewrite the PC a single time.  */
	  th->pc_adjusted = true;
	}
      regcache->raw_supply (r, context_offset);
    }
}

void
windows_nat_target::fetch_registers (struct regcache *regcache, int r)
{
  windows_thread_info *th
    = windows_process.thread_rec (regcache->ptid (), INVALIDATE_CONTEXT);

  /* Check if TH exists.  Windows sometimes uses a non-existent
     thread id in its events.  */
  if (th == NULL)
    return;

  if (th->reload_context)
    {
#ifdef __CYGWIN__
      if (windows_process.have_saved_context)
	{
	  /* Lie about where the program actually is stopped since
	     cygwin has informed us that we should consider the signal
	     to have occurred at another location which is stored in
	     "saved_context.  */
	  memcpy (&th->context, &windows_process.saved_context,
		  __COPY_CONTEXT_SIZE);
	  windows_process.have_saved_context = 0;
	}
      else
#endif
#ifdef __x86_64__
      if (windows_process.wow64_process)
	{
	  th->wow64_context.ContextFlags = CONTEXT_DEBUGGER_DR;
	  CHECK (Wow64GetThreadContext (th->h, &th->wow64_context));
	  /* Copy dr values from that thread.
	     But only if there were not modified since last stop.
	     PR gdb/2388 */
	  if (!th->debug_registers_changed)
	    {
	      windows_process.dr[0] = th->wow64_context.Dr0;
	      windows_process.dr[1] = th->wow64_context.Dr1;
	      windows_process.dr[2] = th->wow64_context.Dr2;
	      windows_process.dr[3] = th->wow64_context.Dr3;
	      windows_process.dr[6] = th->wow64_context.Dr6;
	      windows_process.dr[7] = th->wow64_context.Dr7;
	    }
	}
      else
#endif
	{
	  th->context.ContextFlags = CONTEXT_DEBUGGER_DR;
	  CHECK (GetThreadContext (th->h, &th->context));
	  /* Copy dr values from that thread.
	     But only if there were not modified since last stop.
	     PR gdb/2388 */
	  if (!th->debug_registers_changed)
	    {
	      windows_process.dr[0] = th->context.Dr0;
	      windows_process.dr[1] = th->context.Dr1;
	      windows_process.dr[2] = th->context.Dr2;
	      windows_process.dr[3] = th->context.Dr3;
	      windows_process.dr[6] = th->context.Dr6;
	      windows_process.dr[7] = th->context.Dr7;
	    }
	}
      th->reload_context = false;
    }

  if (r < 0)
    for (r = 0; r < gdbarch_num_regs (regcache->arch()); r++)
      windows_fetch_one_register (regcache, th, r);
  else
    windows_fetch_one_register (regcache, th, r);
}

/* Collect the register number R from the given regcache, and store
   its value into the corresponding area of the given thread's context.

   This function assumes that R is non-negative.  A failed assertion
   assertion is raised if that is not true.  */

static void
windows_store_one_register (const struct regcache *regcache,
			    windows_thread_info *th, int r)
{
  gdb_assert (r >= 0);

  char *context_ptr = (char *) &th->context;
#ifdef __x86_64__
  if (windows_process.wow64_process)
    context_ptr = (char *) &th->wow64_context;
#endif

  regcache->raw_collect (r, context_ptr + windows_process.mappings[r]);
}

/* Store a new register value into the context of the thread tied to
   REGCACHE.  */

void
windows_nat_target::store_registers (struct regcache *regcache, int r)
{
  windows_thread_info *th
    = windows_process.thread_rec (regcache->ptid (), INVALIDATE_CONTEXT);

  /* Check if TH exists.  Windows sometimes uses a non-existent
     thread id in its events.  */
  if (th == NULL)
    return;

  if (r < 0)
    for (r = 0; r < gdbarch_num_regs (regcache->arch ()); r++)
      windows_store_one_register (regcache, th, r);
  else
    windows_store_one_register (regcache, th, r);
}

/* See nat/windows-nat.h.  */

static windows_solib *
windows_make_so (const char *name, LPVOID load_addr)
{
#ifndef __CYGWIN__
  char *p;
  char buf[__PMAX];
  char cwd[__PMAX];
  WIN32_FIND_DATA w32_fd;
  HANDLE h = FindFirstFile(name, &w32_fd);

  if (h == INVALID_HANDLE_VALUE)
    strcpy (buf, name);
  else
    {
      FindClose (h);
      strcpy (buf, name);
      if (GetCurrentDirectory (MAX_PATH + 1, cwd))
	{
	  p = strrchr (buf, '\\');
	  if (p)
	    p[1] = '\0';
	  SetCurrentDirectory (buf);
	  GetFullPathName (w32_fd.cFileName, MAX_PATH, buf, &p);
	  SetCurrentDirectory (cwd);
	}
    }
  if (strcasecmp (buf, "ntdll.dll") == 0)
    {
      GetSystemDirectory (buf, sizeof (buf));
      strcat (buf, "\\ntdll.dll");
    }
#else
  wchar_t buf[__PMAX];

  buf[0] = 0;
  if (access (name, F_OK) != 0)
    {
      if (strcasecmp (name, "ntdll.dll") == 0)
	{
	  GetSystemDirectoryW (buf, sizeof (buf) / sizeof (wchar_t));
	  wcscat (buf, L"\\ntdll.dll");
	}
    }
#endif
  windows_process.solibs.emplace_back ();
  windows_solib *so = &windows_process.solibs.back ();
  so->load_addr = load_addr;
  so->original_name = name;
#ifndef __CYGWIN__
  so->name = buf;
#else
  if (buf[0])
    {
      char cname[SO_NAME_MAX_PATH_SIZE];
      cygwin_conv_path (CCP_WIN_W_TO_POSIX, buf, cname,
			SO_NAME_MAX_PATH_SIZE);
      so->name = cname;
    }
  else
    {
      char *rname = realpath (name, NULL);
      if (rname && strlen (rname) < SO_NAME_MAX_PATH_SIZE)
	{
	  so->name = rname;
	  free (rname);
	}
      else
	{
	  warning (_("dll path for \"%s\" too long or inaccessible"), name);
	  so->name = so->original_name;
	}
    }
  /* Record cygwin1.dll .text start/end.  */
  size_t len = sizeof ("/cygwin1.dll") - 1;
  if (so->name.size () >= len
      && strcasecmp (so->name.c_str () + so->name.size () - len,
		     "/cygwin1.dll") == 0)
    {
      asection *text = NULL;

      gdb_bfd_ref_ptr abfd (gdb_bfd_open (so->name.c_str(), "pei-i386"));

      if (abfd == NULL)
	return so;

      if (bfd_check_format (abfd.get (), bfd_object))
	text = bfd_get_section_by_name (abfd.get (), ".text");

      if (!text)
	return so;

      /* The symbols in a dll are offset by 0x1000, which is the
	 offset from 0 of the first byte in an image - because of the
	 file header and the section alignment.  */
      windows_process.cygwin_load_start = (CORE_ADDR) (uintptr_t) ((char *)
								   load_addr + 0x1000);
      windows_process.cygwin_load_end = windows_process.cygwin_load_start +
	bfd_section_size (text);
    }
#endif

  return so;
}

/* See nat/windows-nat.h.  */

void
windows_per_inferior::handle_load_dll (const char *dll_name, LPVOID base)
{
  windows_solib *solib = windows_make_so (dll_name, base);
  DEBUG_EVENTS ("Loading dll \"%s\" at %s.", solib->name.c_str (),
		host_address_to_string (solib->load_addr));
}

/* See nat/windows-nat.h.  */

void
windows_per_inferior::handle_unload_dll ()
{
  LPVOID lpBaseOfDll = current_event.u.UnloadDll.lpBaseOfDll;

  auto iter = std::remove_if (windows_process.solibs.begin (),
			      windows_process.solibs.end (),
			      [&] (windows_solib &lib)
    {
      if (lib.load_addr == lpBaseOfDll)
	{
	  DEBUG_EVENTS ("Unloading dll \"%s\".", lib.name.c_str ());
	  return true;
	}
      return false;
    });

  if (iter != windows_process.solibs.end ())
    {
      windows_process.solibs.erase (iter, windows_process.solibs.end ());
      return;
    }

  /* We did not find any DLL that was previously loaded at this address,
     so register a complaint.  We do not report an error, because we have
     observed that this may be happening under some circumstances.  For
     instance, running 32bit applications on x64 Windows causes us to receive
     4 mysterious UNLOAD_DLL_DEBUG_EVENTs during the startup phase (these
     events are apparently caused by the WOW layer, the interface between
     32bit and 64bit worlds).  */
  complaint (_("dll starting at %s not found."),
	     host_address_to_string (lpBaseOfDll));
}

/* Clear list of loaded DLLs.  */
static void
windows_clear_solib (void)
{
  windows_process.solibs.clear ();
}

static void
signal_event_command (const char *args, int from_tty)
{
  uintptr_t event_id = 0;
  char *endargs = NULL;

  if (args == NULL)
    error (_("signal-event requires an argument (integer event id)"));

  event_id = strtoumax (args, &endargs, 10);

  if ((errno == ERANGE) || (event_id == 0) || (event_id > UINTPTR_MAX) ||
      ((HANDLE) event_id == INVALID_HANDLE_VALUE))
    error (_("Failed to convert `%s' to event id"), args);

  SetEvent ((HANDLE) event_id);
  CloseHandle ((HANDLE) event_id);
}

/* See nat/windows-nat.h.  */

int
windows_per_inferior::handle_output_debug_string
     (struct target_waitstatus *ourstatus)
{
  int retval = 0;

  gdb::unique_xmalloc_ptr<char> s
    = (target_read_string
       ((CORE_ADDR) (uintptr_t) current_event.u.DebugString.lpDebugStringData,
	1024));
  if (s == nullptr || !*(s.get ()))
    /* nothing to do */;
  else if (!startswith (s.get (), _CYGWIN_SIGNAL_STRING))
    {
#ifdef __CYGWIN__
      if (!startswith (s.get (), "cYg"))
#endif
	{
	  char *p = strchr (s.get (), '\0');

	  if (p > s.get () && *--p == '\n')
	    *p = '\0';
	  warning (("%s"), s.get ());
	}
    }
#ifdef __CYGWIN__
  else
    {
      /* Got a cygwin signal marker.  A cygwin signal is followed by
	 the signal number itself and then optionally followed by the
	 thread id and address to saved context within the DLL.  If
	 these are supplied, then the given thread is assumed to have
	 issued the signal and the context from the thread is assumed
	 to be stored at the given address in the inferior.  Tell gdb
	 to treat this like a real signal.  */
      char *p;
      int sig = strtol (s.get () + sizeof (_CYGWIN_SIGNAL_STRING) - 1, &p, 0);
      gdb_signal gotasig = gdb_signal_from_host (sig);

      if (gotasig)
	{
	  LPCVOID x;
	  SIZE_T n;

	  ourstatus->set_stopped (gotasig);
	  retval = strtoul (p, &p, 0);
	  if (!retval)
	    retval = current_event.dwThreadId;
	  else if ((x = (LPCVOID) (uintptr_t) strtoull (p, NULL, 0))
		   && ReadProcessMemory (handle, x,
					 &saved_context,
					 __COPY_CONTEXT_SIZE, &n)
		   && n == __COPY_CONTEXT_SIZE)
	    have_saved_context = 1;
	}
    }
#endif

  return retval;
}

static int
display_selector (HANDLE thread, DWORD sel)
{
  LDT_ENTRY info;
  BOOL ret;
#ifdef __x86_64__
  if (windows_process.wow64_process)
    ret = Wow64GetThreadSelectorEntry (thread, sel, &info);
  else
#endif
    ret = GetThreadSelectorEntry (thread, sel, &info);
  if (ret)
    {
      int base, limit;
      gdb_printf ("0x%03x: ", (unsigned) sel);
      if (!info.HighWord.Bits.Pres)
	{
	  gdb_puts ("Segment not present\n");
	  return 0;
	}
      base = (info.HighWord.Bits.BaseHi << 24) +
	     (info.HighWord.Bits.BaseMid << 16)
	     + info.BaseLow;
      limit = (info.HighWord.Bits.LimitHi << 16) + info.LimitLow;
      if (info.HighWord.Bits.Granularity)
	limit = (limit << 12) | 0xfff;
      gdb_printf ("base=0x%08x limit=0x%08x", base, limit);
      if (info.HighWord.Bits.Default_Big)
	gdb_puts(" 32-bit ");
      else
	gdb_puts(" 16-bit ");
      switch ((info.HighWord.Bits.Type & 0xf) >> 1)
	{
	case 0:
	  gdb_puts ("Data (Read-Only, Exp-up");
	  break;
	case 1:
	  gdb_puts ("Data (Read/Write, Exp-up");
	  break;
	case 2:
	  gdb_puts ("Unused segment (");
	  break;
	case 3:
	  gdb_puts ("Data (Read/Write, Exp-down");
	  break;
	case 4:
	  gdb_puts ("Code (Exec-Only, N.Conf");
	  break;
	case 5:
	  gdb_puts ("Code (Exec/Read, N.Conf");
	  break;
	case 6:
	  gdb_puts ("Code (Exec-Only, Conf");
	  break;
	case 7:
	  gdb_puts ("Code (Exec/Read, Conf");
	  break;
	default:
	  gdb_printf ("Unknown type 0x%lx",
		      (unsigned long) info.HighWord.Bits.Type);
	}
      if ((info.HighWord.Bits.Type & 0x1) == 0)
	gdb_puts(", N.Acc");
      gdb_puts (")\n");
      if ((info.HighWord.Bits.Type & 0x10) == 0)
	gdb_puts("System selector ");
      gdb_printf ("Privilege level = %ld. ",
		  (unsigned long) info.HighWord.Bits.Dpl);
      if (info.HighWord.Bits.Granularity)
	gdb_puts ("Page granular.\n");
      else
	gdb_puts ("Byte granular.\n");
      return 1;
    }
  else
    {
      DWORD err = GetLastError ();
      if (err == ERROR_NOT_SUPPORTED)
	gdb_printf ("Function not supported\n");
      else
	gdb_printf ("Invalid selector 0x%x.\n", (unsigned) sel);
      return 0;
    }
}

static void
display_selectors (const char * args, int from_tty)
{
  if (inferior_ptid == null_ptid)
    {
      gdb_puts ("Impossible to display selectors now.\n");
      return;
    }

  windows_thread_info *current_windows_thread
    = windows_process.thread_rec (inferior_ptid, DONT_INVALIDATE_CONTEXT);

  if (!args)
    {
#ifdef __x86_64__
      if (windows_process.wow64_process)
	{
	  gdb_puts ("Selector $cs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegCs);
	  gdb_puts ("Selector $ds\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegDs);
	  gdb_puts ("Selector $es\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegEs);
	  gdb_puts ("Selector $ss\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegSs);
	  gdb_puts ("Selector $fs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegFs);
	  gdb_puts ("Selector $gs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->wow64_context.SegGs);
	}
      else
#endif
	{
	  gdb_puts ("Selector $cs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegCs);
	  gdb_puts ("Selector $ds\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegDs);
	  gdb_puts ("Selector $es\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegEs);
	  gdb_puts ("Selector $ss\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegSs);
	  gdb_puts ("Selector $fs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegFs);
	  gdb_puts ("Selector $gs\n");
	  display_selector (current_windows_thread->h,
			    current_windows_thread->context.SegGs);
	}
    }
  else
    {
      int sel;
      sel = parse_and_eval_long (args);
      gdb_printf ("Selector \"%s\"\n",args);
      display_selector (current_windows_thread->h, sel);
    }
}

/* See nat/windows-nat.h.  */

bool
windows_per_inferior::handle_access_violation
     (const EXCEPTION_RECORD *rec)
{
#ifdef __CYGWIN__
  /* See if the access violation happened within the cygwin DLL
     itself.  Cygwin uses a kind of exception handling to deal with
     passed-in invalid addresses.  gdb should not treat these as real
     SEGVs since they will be silently handled by cygwin.  A real SEGV
     will (theoretically) be caught by cygwin later in the process and
     will be sent as a cygwin-specific-signal.  So, ignore SEGVs if
     they show up within the text segment of the DLL itself.  */
  const char *fn;
  CORE_ADDR addr = (CORE_ADDR) (uintptr_t) rec->ExceptionAddress;

  if ((!cygwin_exceptions && (addr >= cygwin_load_start
			      && addr < cygwin_load_end))
      || (find_pc_partial_function (addr, &fn, NULL, NULL)
	  && startswith (fn, "KERNEL32!IsBad")))
    return true;
#endif
  return false;
}

/* Resume thread specified by ID, or all artificially suspended
   threads, if we are continuing execution.  KILLED non-zero means we
   have killed the inferior, so we should ignore weird errors due to
   threads shutting down.  LAST_CALL is true if we expect this to be
   the last call to continue the inferior -- we are either mourning it
   or detaching.  */
BOOL
windows_nat_target::windows_continue (DWORD continue_status, int id,
				      int killed, bool last_call)
{
  windows_process.desired_stop_thread_id = id;

  if (windows_process.matching_pending_stop (debug_events))
    {
      /* There's no need to really continue, because there's already
	 another event pending.  However, we do need to inform the
	 event loop of this.  */
      serial_event_set (m_wait_event);
      return TRUE;
    }

  for (auto &th : windows_process.thread_list)
    if (id == -1 || id == (int) th->tid)
      {
#ifdef __x86_64__
	if (windows_process.wow64_process)
	  {
	    if (th->debug_registers_changed)
	      {
		th->wow64_context.ContextFlags |= CONTEXT_DEBUG_REGISTERS;
		th->wow64_context.Dr0 = windows_process.dr[0];
		th->wow64_context.Dr1 = windows_process.dr[1];
		th->wow64_context.Dr2 = windows_process.dr[2];
		th->wow64_context.Dr3 = windows_process.dr[3];
		th->wow64_context.Dr6 = DR6_CLEAR_VALUE;
		th->wow64_context.Dr7 = windows_process.dr[7];
		th->debug_registers_changed = false;
	      }
	    if (th->wow64_context.ContextFlags)
	      {
		DWORD ec = 0;

		if (GetExitCodeThread (th->h, &ec)
		    && ec == STILL_ACTIVE)
		  {
		    BOOL status = Wow64SetThreadContext (th->h,
							 &th->wow64_context);

		    if (!killed)
		      CHECK (status);
		  }
		th->wow64_context.ContextFlags = 0;
	      }
	  }
	else
#endif
	  {
	    if (th->debug_registers_changed)
	      {
		th->context.ContextFlags |= CONTEXT_DEBUG_REGISTERS;
		th->context.Dr0 = windows_process.dr[0];
		th->context.Dr1 = windows_process.dr[1];
		th->context.Dr2 = windows_process.dr[2];
		th->context.Dr3 = windows_process.dr[3];
		th->context.Dr6 = DR6_CLEAR_VALUE;
		th->context.Dr7 = windows_process.dr[7];
		th->debug_registers_changed = false;
	      }
	    if (th->context.ContextFlags)
	      {
		DWORD ec = 0;

		if (GetExitCodeThread (th->h, &ec)
		    && ec == STILL_ACTIVE)
		  {
		    BOOL status = SetThreadContext (th->h, &th->context);

		    if (!killed)
		      CHECK (status);
		  }
		th->context.ContextFlags = 0;
	      }
	  }
	th->resume ();
      }
    else
      {
	/* When single-stepping a specific thread, other threads must
	   be suspended.  */
	th->suspend ();
      }

  std::optional<unsigned> err;
  do_synchronously ([&] ()
    {
      if (!continue_last_debug_event (continue_status, debug_events))
	err = (unsigned) GetLastError ();
      /* On the last call, do not block waiting for an event that will
	 never come.  */
      return !last_call;
    });

  if (err.has_value ())
    throw_winerror_with_name (_("Failed to resume program execution"
				" - ContinueDebugEvent failed"),
			      *err);

  return TRUE;
}

/* Called in pathological case where Windows fails to send a
   CREATE_PROCESS_DEBUG_EVENT after an attach.  */
DWORD
windows_nat_target::fake_create_process ()
{
  windows_process.handle
    = OpenProcess (PROCESS_ALL_ACCESS, FALSE,
		   windows_process.current_event.dwProcessId);
  if (windows_process.handle != NULL)
    windows_process.open_process_used = 1;
  else
    {
      unsigned err = (unsigned) GetLastError ();
      throw_winerror_with_name (_("OpenProcess call failed"), err);
      /*  We can not debug anything in that case.  */
    }
  add_thread (ptid_t (windows_process.current_event.dwProcessId, 0,
			      windows_process.current_event.dwThreadId),
		      windows_process.current_event.u.CreateThread.hThread,
		      windows_process.current_event.u.CreateThread.lpThreadLocalBase,
		      true /* main_thread_p */);
  return windows_process.current_event.dwThreadId;
}

void
windows_nat_target::resume (ptid_t ptid, int step, enum gdb_signal sig)
{
  windows_thread_info *th;
  DWORD continue_status = DBG_CONTINUE;

  /* A specific PTID means `step only this thread id'.  */
  int resume_all = ptid == minus_one_ptid;

  /* If we're continuing all threads, it's the current inferior that
     should be handled specially.  */
  if (resume_all)
    ptid = inferior_ptid;

  if (sig != GDB_SIGNAL_0)
    {
      if (windows_process.current_event.dwDebugEventCode
	  != EXCEPTION_DEBUG_EVENT)
	{
	  DEBUG_EXCEPT ("Cannot continue with signal %d here.", sig);
	}
      else if (sig == windows_process.last_sig)
	continue_status = DBG_EXCEPTION_NOT_HANDLED;
      else
#if 0
/* This code does not seem to work, because
  the kernel does probably not consider changes in the ExceptionRecord
  structure when passing the exception to the inferior.
  Note that this seems possible in the exception handler itself.  */
	{
	  for (const xlate_exception &x : xlate)
	    if (x.us == sig)
	      {
		current_event.u.Exception.ExceptionRecord.ExceptionCode
		  = x.them;
		continue_status = DBG_EXCEPTION_NOT_HANDLED;
		break;
	      }
	  if (continue_status == DBG_CONTINUE)
	    {
	      DEBUG_EXCEPT ("Cannot continue with signal %d.", sig);
	    }
	}
#endif
      DEBUG_EXCEPT ("Can only continue with received signal %d.",
		    windows_process.last_sig);
    }

  windows_process.last_sig = GDB_SIGNAL_0;

  DEBUG_EXEC ("pid=%d, tid=0x%x, step=%d, sig=%d",
	      ptid.pid (), (unsigned) ptid.lwp (), step, sig);

  /* Get context for currently selected thread.  */
  th = windows_process.thread_rec (inferior_ptid, DONT_INVALIDATE_CONTEXT);
  if (th)
    {
#ifdef __x86_64__
      if (windows_process.wow64_process)
	{
	  if (step)
	    {
	      /* Single step by setting t bit.  */
	      regcache *regcache = get_thread_regcache (inferior_thread ());
	      struct gdbarch *gdbarch = regcache->arch ();
	      fetch_registers (regcache, gdbarch_ps_regnum (gdbarch));
	      th->wow64_context.EFlags |= FLAG_TRACE_BIT;
	    }

	  if (th->wow64_context.ContextFlags)
	    {
	      if (th->debug_registers_changed)
		{
		  th->wow64_context.Dr0 = windows_process.dr[0];
		  th->wow64_context.Dr1 = windows_process.dr[1];
		  th->wow64_context.Dr2 = windows_process.dr[2];
		  th->wow64_context.Dr3 = windows_process.dr[3];
		  th->wow64_context.Dr6 = DR6_CLEAR_VALUE;
		  th->wow64_context.Dr7 = windows_process.dr[7];
		  th->debug_registers_changed = false;
		}
	      CHECK (Wow64SetThreadContext (th->h, &th->wow64_context));
	      th->wow64_context.ContextFlags = 0;
	    }
	}
      else
#endif
	{
	  if (step)
	    {
	      /* Single step by setting t bit.  */
	      regcache *regcache = get_thread_regcache (inferior_thread ());
	      struct gdbarch *gdbarch = regcache->arch ();
	      fetch_registers (regcache, gdbarch_ps_regnum (gdbarch));
	      th->context.EFlags |= FLAG_TRACE_BIT;
	    }

	  if (th->context.ContextFlags)
	    {
	      if (th->debug_registers_changed)
		{
		  th->context.Dr0 = windows_process.dr[0];
		  th->context.Dr1 = windows_process.dr[1];
		  th->context.Dr2 = windows_process.dr[2];
		  th->context.Dr3 = windows_process.dr[3];
		  th->context.Dr6 = DR6_CLEAR_VALUE;
		  th->context.Dr7 = windows_process.dr[7];
		  th->debug_registers_changed = false;
		}
	      CHECK (SetThreadContext (th->h, &th->context));
	      th->context.ContextFlags = 0;
	    }
	}
    }

  /* Allow continuing with the same signal that interrupted us.
     Otherwise complain.  */

  if (resume_all)
    windows_continue (continue_status, -1, 0);
  else
    windows_continue (continue_status, ptid.lwp (), 0);
}

/* Interrupt the inferior.  */

void
windows_nat_target::interrupt ()
{
  DEBUG_EVENTS ("interrupt");
#ifdef __x86_64__
  if (windows_process.wow64_process)
    {
      /* Call DbgUiRemoteBreakin of the 32bit ntdll.dll in the target process.
	 DebugBreakProcess would call the one of the 64bit ntdll.dll, which
	 can't be correctly handled by gdb.  */
      if (windows_process.wow64_dbgbreak == nullptr)
	{
	  CORE_ADDR addr;
	  if (!find_minimal_symbol_address ("ntdll!DbgUiRemoteBreakin",
					    &addr, 0))
	    windows_process.wow64_dbgbreak = (void *) addr;
	}

      if (windows_process.wow64_dbgbreak != nullptr)
	{
	  HANDLE thread = CreateRemoteThread (windows_process.handle, NULL,
					      0, (LPTHREAD_START_ROUTINE)
					      windows_process.wow64_dbgbreak,
					      NULL, 0, NULL);
	  if (thread)
	    {
	      CloseHandle (thread);
	      return;
	    }
	}
    }
  else
#endif
    if (DebugBreakProcess (windows_process.handle))
      return;
  warning (_("Could not interrupt program.  "
	     "Press Ctrl-c in the program console."));
}

void
windows_nat_target::pass_ctrlc ()
{
  interrupt ();
}

/* Get the next event from the child.  Returns the thread ptid.  */

ptid_t
windows_nat_target::get_windows_debug_event
     (int pid, struct target_waitstatus *ourstatus, target_wait_flags options)
{
  DWORD continue_status, event_code;
  DWORD thread_id = 0;

  /* If there is a relevant pending stop, report it now.  See the
     comment by the definition of "pending_stops" for details on why
     this is needed.  */
  std::optional<pending_stop> stop
    = windows_process.fetch_pending_stop (debug_events);
  if (stop.has_value ())
    {
      thread_id = stop->thread_id;
      *ourstatus = stop->status;

      ptid_t ptid (windows_process.current_event.dwProcessId, thread_id);
      windows_thread_info *th
	= windows_process.thread_rec (ptid, INVALIDATE_CONTEXT);
      th->reload_context = true;

      return ptid;
    }

  windows_process.last_sig = GDB_SIGNAL_0;
  DEBUG_EVENT *current_event = &windows_process.current_event;

  if ((options & TARGET_WNOHANG) != 0 && !m_debug_event_pending)
    {
      ourstatus->set_ignore ();
      return minus_one_ptid;
    }

  wait_for_debug_event_main_thread (&windows_process.current_event);

  continue_status = DBG_CONTINUE;

  event_code = windows_process.current_event.dwDebugEventCode;
  ourstatus->set_spurious ();
  windows_process.have_saved_context = 0;

  switch (event_code)
    {
    case CREATE_THREAD_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "CREATE_THREAD_DEBUG_EVENT");
      if (windows_process.saw_create != 1)
	{
	  inferior *inf = find_inferior_pid (this, current_event->dwProcessId);
	  if (!windows_process.saw_create && inf->attach_flag)
	    {
	      /* Kludge around a Windows bug where first event is a create
		 thread event.  Caused when attached process does not have
		 a main thread.  */
	      thread_id = fake_create_process ();
	      if (thread_id)
		windows_process.saw_create++;
	    }
	  break;
	}
      /* Record the existence of this thread.  */
      thread_id = current_event->dwThreadId;
      add_thread
	(ptid_t (current_event->dwProcessId, current_event->dwThreadId, 0),
	 current_event->u.CreateThread.hThread,
	 current_event->u.CreateThread.lpThreadLocalBase,
	 false /* main_thread_p */);

      break;

    case EXIT_THREAD_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "EXIT_THREAD_DEBUG_EVENT");
      delete_thread (ptid_t (current_event->dwProcessId,
			     current_event->dwThreadId, 0),
		     current_event->u.ExitThread.dwExitCode,
		     false /* main_thread_p */);
      break;

    case CREATE_PROCESS_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "CREATE_PROCESS_DEBUG_EVENT");
      CloseHandle (current_event->u.CreateProcessInfo.hFile);
      if (++windows_process.saw_create != 1)
	break;

      windows_process.handle = current_event->u.CreateProcessInfo.hProcess;
      /* Add the main thread.  */
      add_thread
	(ptid_t (current_event->dwProcessId,
		 current_event->dwThreadId, 0),
	 current_event->u.CreateProcessInfo.hThread,
	 current_event->u.CreateProcessInfo.lpThreadLocalBase,
	 true /* main_thread_p */);
      thread_id = current_event->dwThreadId;
      break;

    case EXIT_PROCESS_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "EXIT_PROCESS_DEBUG_EVENT");
      if (!windows_process.windows_initialization_done)
	{
	  target_terminal::ours ();
	  target_mourn_inferior (inferior_ptid);
	  error (_("During startup program exited with code 0x%x."),
		 (unsigned int) current_event->u.ExitProcess.dwExitCode);
	}
      else if (windows_process.saw_create == 1)
	{
	  delete_thread (ptid_t (current_event->dwProcessId,
				 current_event->dwThreadId, 0),
			 0, true /* main_thread_p */);
	  DWORD exit_status = current_event->u.ExitProcess.dwExitCode;
	  /* If the exit status looks like a fatal exception, but we
	     don't recognize the exception's code, make the original
	     exit status value available, to avoid losing
	     information.  */
	  int exit_signal
	    = WIFSIGNALED (exit_status) ? WTERMSIG (exit_status) : -1;
	  if (exit_signal == -1)
	    ourstatus->set_exited (exit_status);
	  else
	    ourstatus->set_signalled (gdb_signal_from_host (exit_signal));

	  thread_id = current_event->dwThreadId;
	}
      break;

    case LOAD_DLL_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "LOAD_DLL_DEBUG_EVENT");
      CloseHandle (current_event->u.LoadDll.hFile);
      if (windows_process.saw_create != 1
	  || ! windows_process.windows_initialization_done)
	break;
      try
	{
	  windows_process.dll_loaded_event ();
	}
      catch (const gdb_exception &ex)
	{
	  exception_print (gdb_stderr, ex);
	}
      ourstatus->set_loaded ();
      thread_id = current_event->dwThreadId;
      break;

    case UNLOAD_DLL_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "UNLOAD_DLL_DEBUG_EVENT");
      if (windows_process.saw_create != 1
	  || ! windows_process.windows_initialization_done)
	break;
      try
	{
	  windows_process.handle_unload_dll ();
	}
      catch (const gdb_exception &ex)
	{
	  exception_print (gdb_stderr, ex);
	}
      ourstatus->set_loaded ();
      thread_id = current_event->dwThreadId;
      break;

    case EXCEPTION_DEBUG_EVENT:
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "EXCEPTION_DEBUG_EVENT");
      if (windows_process.saw_create != 1)
	break;
      switch (windows_process.handle_exception (ourstatus, debug_exceptions))
	{
	case HANDLE_EXCEPTION_UNHANDLED:
	default:
	  continue_status = DBG_EXCEPTION_NOT_HANDLED;
	  break;
	case HANDLE_EXCEPTION_HANDLED:
	  thread_id = current_event->dwThreadId;
	  break;
	case HANDLE_EXCEPTION_IGNORED:
	  continue_status = DBG_CONTINUE;
	  break;
	}
      break;

    case OUTPUT_DEBUG_STRING_EVENT:	/* Message from the kernel.  */
      DEBUG_EVENTS ("kernel event for pid=%u tid=0x%x code=%s",
		    (unsigned) current_event->dwProcessId,
		    (unsigned) current_event->dwThreadId,
		    "OUTPUT_DEBUG_STRING_EVENT");
      if (windows_process.saw_create != 1)
	break;
      thread_id = windows_process.handle_output_debug_string (ourstatus);
      break;

    default:
      if (windows_process.saw_create != 1)
	break;
      gdb_printf ("gdb: kernel event for pid=%u tid=0x%x\n",
		  (unsigned) current_event->dwProcessId,
		  (unsigned) current_event->dwThreadId);
      gdb_printf ("                 unknown event code %u\n",
		  (unsigned) current_event->dwDebugEventCode);
      break;
    }

  if (!thread_id || windows_process.saw_create != 1)
    {
      CHECK (windows_continue (continue_status,
			       windows_process.desired_stop_thread_id, 0));
    }
  else if (windows_process.desired_stop_thread_id != -1
	   && windows_process.desired_stop_thread_id != thread_id)
    {
      /* Pending stop.  See the comment by the definition of
	 "pending_stops" for details on why this is needed.  */
      DEBUG_EVENTS ("get_windows_debug_event - "
		    "unexpected stop in 0x%x (expecting 0x%x)",
		    thread_id, windows_process.desired_stop_thread_id);

      if (current_event->dwDebugEventCode == EXCEPTION_DEBUG_EVENT
	  && ((current_event->u.Exception.ExceptionRecord.ExceptionCode
	       == EXCEPTION_BREAKPOINT)
	      || (current_event->u.Exception.ExceptionRecord.ExceptionCode
		  == STATUS_WX86_BREAKPOINT))
	  && windows_process.windows_initialization_done)
	{
	  ptid_t ptid = ptid_t (current_event->dwProcessId, thread_id, 0);
	  windows_thread_info *th
	    = windows_process.thread_rec (ptid, INVALIDATE_CONTEXT);
	  th->stopped_at_software_breakpoint = true;
	  th->pc_adjusted = false;
	}
      windows_process.pending_stops.push_back
	({thread_id, *ourstatus, windows_process.current_event});
      thread_id = 0;
      CHECK (windows_continue (continue_status,
			       windows_process.desired_stop_thread_id, 0));
    }

  if (thread_id == 0)
    return null_ptid;
  return ptid_t (windows_process.current_event.dwProcessId, thread_id, 0);
}

/* Wait for interesting events to occur in the target process.  */
ptid_t
windows_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			  target_wait_flags options)
{
  int pid = -1;

  /* We loop when we get a non-standard exception rather than return
     with a SPURIOUS because resume can try and step or modify things,
     which needs a current_thread->h.  But some of these exceptions mark
     the birth or death of threads, which mean that the current thread
     isn't necessarily what you think it is.  */

  while (1)
    {
      ptid_t result = get_windows_debug_event (pid, ourstatus, options);

      if (result != null_ptid)
	{
	  if (ourstatus->kind () != TARGET_WAITKIND_EXITED
	      && ourstatus->kind () !=  TARGET_WAITKIND_SIGNALLED)
	    {
	      windows_thread_info *th
		= windows_process.thread_rec (result, INVALIDATE_CONTEXT);

	      if (th != nullptr)
		{
		  th->stopped_at_software_breakpoint = false;
		  if (windows_process.current_event.dwDebugEventCode
		      == EXCEPTION_DEBUG_EVENT
		      && ((windows_process.current_event.u.Exception.ExceptionRecord.ExceptionCode
			   == EXCEPTION_BREAKPOINT)
			  || (windows_process.current_event.u.Exception.ExceptionRecord.ExceptionCode
			      == STATUS_WX86_BREAKPOINT))
		      && windows_process.windows_initialization_done)
		    {
		      th->stopped_at_software_breakpoint = true;
		      th->pc_adjusted = false;
		    }
		}
	    }

	  return result;
	}
      else
	{
	  int detach = 0;

	  if (deprecated_ui_loop_hook != NULL)
	    detach = deprecated_ui_loop_hook (0);

	  if (detach)
	    kill ();
	}
    }
}

void
windows_nat_target::do_initial_windows_stuff (DWORD pid, bool attaching)
{
  int i;
  struct inferior *inf;

  windows_process.last_sig = GDB_SIGNAL_0;
  windows_process.open_process_used = 0;
  for (i = 0;
       i < sizeof (windows_process.dr) / sizeof (windows_process.dr[0]);
       i++)
    windows_process.dr[i] = 0;
#ifdef __CYGWIN__
  windows_process.cygwin_load_start = 0;
  windows_process.cygwin_load_end = 0;
#endif
  windows_process.current_event.dwProcessId = pid;
  memset (&windows_process.current_event, 0,
	  sizeof (windows_process.current_event));
  inf = current_inferior ();
  if (!inf->target_is_pushed (this))
    inf->push_target (this);
  disable_breakpoints_in_shlibs ();
  windows_clear_solib ();
  clear_proceed_status (0);
  init_wait_for_inferior ();

#ifdef __x86_64__
  windows_process.ignore_first_breakpoint
    = !attaching && windows_process.wow64_process;

  if (!windows_process.wow64_process)
    {
      windows_process.mappings  = amd64_mappings;
      windows_process.segment_register_p = amd64_windows_segment_register_p;
    }
  else
#endif
    {
      windows_process.mappings  = i386_mappings;
      windows_process.segment_register_p = i386_windows_segment_register_p;
    }

  inferior_appeared (inf, pid);
  inf->attach_flag = attaching;

  target_terminal::init ();
  target_terminal::inferior ();

  windows_process.windows_initialization_done = 0;

  ptid_t last_ptid;

  while (1)
    {
      struct target_waitstatus status;

      last_ptid = this->wait (minus_one_ptid, &status, 0);

      /* Note windows_wait returns TARGET_WAITKIND_SPURIOUS for thread
	 events.  */
      if (status.kind () != TARGET_WAITKIND_LOADED
	  && status.kind () != TARGET_WAITKIND_SPURIOUS)
	break;

      this->resume (minus_one_ptid, 0, GDB_SIGNAL_0);
    }

  switch_to_thread (this->find_thread (last_ptid));

  /* Now that the inferior has been started and all DLLs have been mapped,
     we can iterate over all DLLs and load them in.

     We avoid doing it any earlier because, on certain versions of Windows,
     LOAD_DLL_DEBUG_EVENTs are sometimes not complete.  In particular,
     we have seen on Windows 8.1 that the ntdll.dll load event does not
     include the DLL name, preventing us from creating an associated SO.
     A possible explanation is that ntdll.dll might be mapped before
     the SO info gets created by the Windows system -- ntdll.dll is
     the first DLL to be reported via LOAD_DLL_DEBUG_EVENT and other DLLs
     do not seem to suffer from that problem.

     Rather than try to work around this sort of issue, it is much
     simpler to just ignore DLL load/unload events during the startup
     phase, and then process them all in one batch now.  */
  windows_process.add_all_dlls ();

  windows_process.windows_initialization_done = 1;
  return;
}

/* Try to set or remove a user privilege to the current process.  Return -1
   if that fails, the previous setting of that privilege otherwise.

   This code is copied from the Cygwin source code and rearranged to allow
   dynamically loading of the needed symbols from advapi32 which is only
   available on NT/2K/XP.  */
static int
set_process_privilege (const char *privilege, BOOL enable)
{
  HANDLE token_hdl = NULL;
  LUID restore_priv;
  TOKEN_PRIVILEGES new_priv, orig_priv;
  int ret = -1;
  DWORD size;

  if (!OpenProcessToken (GetCurrentProcess (),
			 TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
			 &token_hdl))
    goto out;

  if (!LookupPrivilegeValueA (NULL, privilege, &restore_priv))
    goto out;

  new_priv.PrivilegeCount = 1;
  new_priv.Privileges[0].Luid = restore_priv;
  new_priv.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;

  if (!AdjustTokenPrivileges (token_hdl, FALSE, &new_priv,
			      sizeof orig_priv, &orig_priv, &size))
    goto out;
#if 0
  /* Disabled, otherwise every `attach' in an unprivileged user session
     would raise the "Failed to get SE_DEBUG_NAME privilege" warning in
     windows_attach().  */
  /* AdjustTokenPrivileges returns TRUE even if the privilege could not
     be enabled.  GetLastError () returns an correct error code, though.  */
  if (enable && GetLastError () == ERROR_NOT_ALL_ASSIGNED)
    goto out;
#endif

  ret = orig_priv.Privileges[0].Attributes == SE_PRIVILEGE_ENABLED ? 1 : 0;

out:
  if (token_hdl)
    CloseHandle (token_hdl);

  return ret;
}

/* Attach to process PID, then initialize for debugging it.  */

void
windows_nat_target::attach (const char *args, int from_tty)
{
  DWORD pid;

  pid = parse_pid_to_attach (args);

  if (set_process_privilege (SE_DEBUG_NAME, TRUE) < 0)
    warning ("Failed to get SE_DEBUG_NAME privilege\n"
	     "This can cause attach to fail on Windows NT/2K/XP");

  windows_init_thread_list ();
  windows_process.saw_create = 0;

  std::optional<unsigned> err;
  do_synchronously ([&] ()
    {
      BOOL ok = DebugActiveProcess (pid);

#ifdef __CYGWIN__
      if (!ok)
	{
	  /* Try fall back to Cygwin pid.  */
	  pid = cygwin_internal (CW_CYGWIN_PID_TO_WINPID, pid);

	  if (pid > 0)
	    ok = DebugActiveProcess (pid);
	}
#endif

      if (!ok)
	err = (unsigned) GetLastError ();

      return true;
    });

  if (err.has_value ())
    {
      std::string msg = string_printf (_("Can't attach to process %u"),
				       (unsigned) pid);
      throw_winerror_with_name (msg.c_str (), *err);
    }

  DebugSetProcessKillOnExit (FALSE);

  target_announce_attach (from_tty, pid);

#ifdef __x86_64__
  HANDLE h = OpenProcess (PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (h != NULL)
    {
      BOOL wow64;
      if (IsWow64Process (h, &wow64))
	windows_process.wow64_process = wow64;
      CloseHandle (h);
    }
#endif

  do_initial_windows_stuff (pid, 1);
  target_terminal::ours ();
}

void
windows_nat_target::detach (inferior *inf, int from_tty)
{
  windows_continue (DBG_CONTINUE, -1, 0, true);

  std::optional<unsigned> err;
  do_synchronously ([&] ()
    {
      if (!DebugActiveProcessStop (windows_process.current_event.dwProcessId))
	err = (unsigned) GetLastError ();
      else
	DebugSetProcessKillOnExit (FALSE);
      return false;
    });

  if (err.has_value ())
    {
      std::string msg
	= string_printf (_("Can't detach process %u"),
			 (unsigned) windows_process.current_event.dwProcessId);
      throw_winerror_with_name (msg.c_str (), *err);
    }

  target_announce_detach (from_tty);

  x86_cleanup_dregs ();
  switch_to_no_thread ();
  detach_inferior (inf);

  maybe_unpush_target ();
}

/* The pid_to_exec_file target_ops method for this platform.  */

const char *
windows_nat_target::pid_to_exec_file (int pid)
{
  return windows_process.pid_to_exec_file (pid);
}

/* Print status information about what we're accessing.  */

void
windows_nat_target::files_info ()
{
  struct inferior *inf = current_inferior ();

  gdb_printf ("\tUsing the running image of %s %s.\n",
	      inf->attach_flag ? "attached" : "child",
	      target_pid_to_str (ptid_t (inf->pid)).c_str ());
}

/* Modify CreateProcess parameters for use of a new separate console.
   Parameters are:
   *FLAGS: DWORD parameter for general process creation flags.
   *SI: STARTUPINFO structure, for which the console window size and
   console buffer size is filled in if GDB is running in a console.
   to create the new console.
   The size of the used font is not available on all versions of
   Windows OS.  Furthermore, the current font might not be the default
   font, but this is still better than before.
   If the windows and buffer sizes are computed,
   SI->DWFLAGS is changed so that this information is used
   by CreateProcess function.  */

static void
windows_set_console_info (STARTUPINFO *si, DWORD *flags)
{
  HANDLE hconsole = CreateFile ("CONOUT$", GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

  if (hconsole != INVALID_HANDLE_VALUE)
    {
      CONSOLE_SCREEN_BUFFER_INFO sbinfo;
      COORD font_size;
      CONSOLE_FONT_INFO cfi;

      GetCurrentConsoleFont (hconsole, FALSE, &cfi);
      font_size = GetConsoleFontSize (hconsole, cfi.nFont);
      GetConsoleScreenBufferInfo(hconsole, &sbinfo);
      si->dwXSize = sbinfo.srWindow.Right - sbinfo.srWindow.Left + 1;
      si->dwYSize = sbinfo.srWindow.Bottom - sbinfo.srWindow.Top + 1;
      if (font_size.X)
	si->dwXSize *= font_size.X;
      else
	si->dwXSize *= 8;
      if (font_size.Y)
	si->dwYSize *= font_size.Y;
      else
	si->dwYSize *= 12;
      si->dwXCountChars = sbinfo.dwSize.X;
      si->dwYCountChars = sbinfo.dwSize.Y;
      si->dwFlags |= STARTF_USESIZE | STARTF_USECOUNTCHARS;
    }
  *flags |= CREATE_NEW_CONSOLE;
}

#ifndef __CYGWIN__
/* Function called by qsort to sort environment strings.  */

static int
envvar_cmp (const void *a, const void *b)
{
  const char **p = (const char **) a;
  const char **q = (const char **) b;
  return strcasecmp (*p, *q);
}
#endif

#ifdef __CYGWIN__
static void
clear_win32_environment (char **env)
{
  int i;
  size_t len;
  wchar_t *copy = NULL, *equalpos;

  for (i = 0; env[i] && *env[i]; i++)
    {
      len = mbstowcs (NULL, env[i], 0) + 1;
      copy = (wchar_t *) xrealloc (copy, len * sizeof (wchar_t));
      mbstowcs (copy, env[i], len);
      equalpos = wcschr (copy, L'=');
      if (equalpos)
	*equalpos = L'\0';
      SetEnvironmentVariableW (copy, NULL);
    }
  xfree (copy);
}
#endif

#ifndef __CYGWIN__

/* Redirection of inferior I/O streams for native MS-Windows programs.
   Unlike on Unix, where this is handled by invoking the inferior via
   the shell, on MS-Windows we need to emulate the cmd.exe shell.

   The official documentation of the cmd.exe redirection features is here:

     http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/redirection.mspx

   (That page talks about Windows XP, but there's no newer
   documentation, so we assume later versions of cmd.exe didn't change
   anything.)

   Caveat: the documentation on that page seems to include a few lies.
   For example, it describes strange constructs 1<&2 and 2<&1, which
   seem to work only when 1>&2 resp. 2>&1 would make sense, and so I
   think the cmd.exe parser of the redirection symbols simply doesn't
   care about the < vs > distinction in these cases.  Therefore, the
   supported features are explicitly documented below.

   The emulation below aims at supporting all the valid use cases
   supported by cmd.exe, which include:

     < FILE    redirect standard input from FILE
     0< FILE   redirect standard input from FILE
     <&N       redirect standard input from file descriptor N
     0<&N      redirect standard input from file descriptor N
     > FILE    redirect standard output to FILE
     >> FILE   append standard output to FILE
     1>> FILE  append standard output to FILE
     >&N       redirect standard output to file descriptor N
     1>&N      redirect standard output to file descriptor N
     >>&N      append standard output to file descriptor N
     1>>&N     append standard output to file descriptor N
     2> FILE   redirect standard error to FILE
     2>> FILE  append standard error to FILE
     2>&N      redirect standard error to file descriptor N
     2>>&N     append standard error to file descriptor N

     Note that using N > 2 in the above construct is supported, but
     requires that the corresponding file descriptor be open by some
     means elsewhere or outside GDB.  Also note that using ">&0" or
     "<&2" will generally fail, because the file descriptor redirected
     from is normally open in an incompatible mode (e.g., FD 0 is open
     for reading only).  IOW, use of such tricks is not recommended;
     you are on your own.

     We do NOT support redirection of file descriptors above 2, as in
     "3>SOME-FILE", because MinGW compiled programs don't (supporting
     that needs special handling in the startup code that MinGW
     doesn't have).  Pipes are also not supported.

     As for invalid use cases, where the redirection contains some
     error, the emulation below will detect that and produce some
     error and/or failure.  But the behavior in those cases is not
     bug-for-bug compatible with what cmd.exe does in those cases.
     That's because what cmd.exe does then is not well defined, and
     seems to be a side effect of the cmd.exe parsing of the command
     line more than anything else.  For example, try redirecting to an
     invalid file name, as in "> foo:bar".

     There are also minor syntactic deviations from what cmd.exe does
     in some corner cases.  For example, it doesn't support the likes
     of "> &foo" to mean redirect to file named literally "&foo"; we
     do support that here, because that, too, sounds like some issue
     with the cmd.exe parser.  Another nicety is that we support
     redirection targets that use file names with forward slashes,
     something cmd.exe doesn't -- this comes in handy since GDB
     file-name completion can be used when typing the command line for
     the inferior.  */

/* Support routines for redirecting standard handles of the inferior.  */

/* Parse a single redirection spec, open/duplicate the specified
   file/fd, and assign the appropriate value to one of the 3 standard
   file descriptors. */
static int
redir_open (const char *redir_string, int *inp, int *out, int *err)
{
  int *fd, ref_fd = -2;
  int mode;
  const char *fname = redir_string + 1;
  int rc = *redir_string;

  switch (rc)
    {
    case '0':
      fname++;
      [[fallthrough]];
    case '<':
      fd = inp;
      mode = O_RDONLY;
      break;
    case '1': case '2':
      fname++;
      [[fallthrough]];
    case '>':
      fd = (rc == '2') ? err : out;
      mode = O_WRONLY | O_CREAT;
      if (*fname == '>')
	{
	  fname++;
	  mode |= O_APPEND;
	}
      else
	mode |= O_TRUNC;
      break;
    default:
      return -1;
    }

  if (*fname == '&' && '0' <= fname[1] && fname[1] <= '9')
    {
      /* A reference to a file descriptor.  */
      char *fdtail;
      ref_fd = (int) strtol (fname + 1, &fdtail, 10);
      if (fdtail > fname + 1 && *fdtail == '\0')
	{
	  /* Don't allow redirection when open modes are incompatible.  */
	  if ((ref_fd == 0 && (fd == out || fd == err))
	      || ((ref_fd == 1 || ref_fd == 2) && fd == inp))
	    {
	      errno = EPERM;
	      return -1;
	    }
	  if (ref_fd == 0)
	    ref_fd = *inp;
	  else if (ref_fd == 1)
	    ref_fd = *out;
	  else if (ref_fd == 2)
	    ref_fd = *err;
	}
      else
	{
	  errno = EBADF;
	  return -1;
	}
    }
  else
    fname++;	/* skip the separator space */
  /* If the descriptor is already open, close it.  This allows
     multiple specs of redirections for the same stream, which is
     somewhat nonsensical, but still valid and supported by cmd.exe.
     (But cmd.exe only opens a single file in this case, the one
     specified by the last redirection spec on the command line.)  */
  if (*fd >= 0)
    _close (*fd);
  if (ref_fd == -2)
    {
      *fd = _open (fname, mode, _S_IREAD | _S_IWRITE);
      if (*fd < 0)
	return -1;
    }
  else if (ref_fd == -1)
    *fd = -1;	/* reset to default destination */
  else
    {
      *fd = _dup (ref_fd);
      if (*fd < 0)
	return -1;
    }
  /* _open just sets a flag for O_APPEND, which won't be passed to the
     inferior, so we need to actually move the file pointer.  */
  if ((mode & O_APPEND) != 0)
    _lseek (*fd, 0L, SEEK_END);
  return 0;
}

/* Canonicalize a single redirection spec and set up the corresponding
   file descriptor as specified.  */
static int
redir_set_redirection (const char *s, int *inp, int *out, int *err)
{
  char buf[__PMAX + 2 + 5]; /* extra space for quotes & redirection string */
  char *d = buf;
  const char *start = s;
  int quote = 0;

  *d++ = *s++;	/* copy the 1st character, < or > or a digit */
  if ((*start == '>' || *start == '1' || *start == '2')
      && *s == '>')
    {
      *d++ = *s++;
      if (*s == '>' && *start != '>')
	*d++ = *s++;
    }
  else if (*start == '0' && *s == '<')
    *d++ = *s++;
  /* cmd.exe recognizes "&N" only immediately after the redirection symbol.  */
  if (*s != '&')
    {
      while (isspace (*s))  /* skip whitespace before file name */
	s++;
      *d++ = ' ';	    /* separate file name with a single space */
    }

  /* Copy the file name.  */
  while (*s)
    {
      /* Remove quoting characters from the file name in buf[].  */
      if (*s == '"')	/* could support '..' quoting here */
	{
	  if (!quote)
	    quote = *s++;
	  else if (*s == quote)
	    {
	      quote = 0;
	      s++;
	    }
	  else
	    *d++ = *s++;
	}
      else if (*s == '\\')
	{
	  if (s[1] == '"')	/* could support '..' here */
	    s++;
	  *d++ = *s++;
	}
      else if (isspace (*s) && !quote)
	break;
      else
	*d++ = *s++;
      if (d - buf >= sizeof (buf) - 1)
	{
	  errno = ENAMETOOLONG;
	  return 0;
	}
    }
  *d = '\0';

  /* Windows doesn't allow redirection characters in file names, so we
     can bail out early if they use them, or if there's no target file
     name after the redirection symbol.  */
  if (d[-1] == '>' || d[-1] == '<')
    {
      errno = ENOENT;
      return 0;
    }
  if (redir_open (buf, inp, out, err) == 0)
    return s - start;
  return 0;
}

/* Parse the command line for redirection specs and prepare the file
   descriptors for the 3 standard streams accordingly.  */
static bool
redirect_inferior_handles (const char *cmd_orig, char *cmd,
			   int *inp, int *out, int *err)
{
  const char *s = cmd_orig;
  char *d = cmd;
  int quote = 0;
  bool retval = false;

  while (isspace (*s))
    *d++ = *s++;

  while (*s)
    {
      if (*s == '"')	/* could also support '..' quoting here */
	{
	  if (!quote)
	    quote = *s;
	  else if (*s == quote)
	    quote = 0;
	}
      else if (*s == '\\')
	{
	  if (s[1] == '"')	/* escaped quote char */
	    s++;
	}
      else if (!quote)
	{
	  /* Process a single redirection candidate.  */
	  if (*s == '<' || *s == '>'
	      || ((*s == '1' || *s == '2') && s[1] == '>')
	      || (*s == '0' && s[1] == '<'))
	    {
	      int skip = redir_set_redirection (s, inp, out, err);

	      if (skip <= 0)
		return false;
	      retval = true;
	      s += skip;
	    }
	}
      if (*s)
	*d++ = *s++;
    }
  *d = '\0';
  return retval;
}
#endif	/* !__CYGWIN__ */

/* Start an inferior windows child process and sets inferior_ptid to its pid.
   EXEC_FILE is the file to run.
   ALLARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  Errors reported with error().  */

void
windows_nat_target::create_inferior (const char *exec_file,
				     const std::string &origallargs,
				     char **in_env, int from_tty)
{
  STARTUPINFO si;
#ifdef __CYGWIN__
  wchar_t real_path[__PMAX];
  wchar_t shell[__PMAX]; /* Path to shell */
  wchar_t infcwd[__PMAX];
  const char *sh;
  wchar_t *toexec;
  wchar_t *cygallargs;
  wchar_t *args;
  char **old_env = NULL;
  PWCHAR w32_env;
  size_t len;
  int tty;
  int ostdin, ostdout, ostderr;
#else  /* !__CYGWIN__ */
  char shell[__PMAX]; /* Path to shell */
  const char *toexec;
  char *args, *allargs_copy;
  size_t args_len, allargs_len;
  int fd_inp = -1, fd_out = -1, fd_err = -1;
  HANDLE tty = INVALID_HANDLE_VALUE;
  bool redirected = false;
  char *w32env;
  char *temp;
  size_t envlen;
  int i;
  size_t envsize;
  char **env;
#endif	/* !__CYGWIN__ */
  const char *allargs = origallargs.c_str ();
  PROCESS_INFORMATION pi;
  std::optional<unsigned> ret;
  DWORD flags = 0;
  const std::string &inferior_tty = current_inferior ()->tty ();

  if (!exec_file)
    error (_("No executable specified, use `target exec'."));

  const char *inferior_cwd = current_inferior ()->cwd ().c_str ();
  std::string expanded_infcwd;
  if (*inferior_cwd == '\0')
    inferior_cwd = nullptr;
  else
    {
      expanded_infcwd = gdb_tilde_expand (inferior_cwd);
      /* Mirror slashes on inferior's cwd.  */
      std::replace (expanded_infcwd.begin (), expanded_infcwd.end (),
		    '/', '\\');
      inferior_cwd = expanded_infcwd.c_str ();
    }

  memset (&si, 0, sizeof (si));
  si.cb = sizeof (si);

  if (new_group)
    flags |= CREATE_NEW_PROCESS_GROUP;

  if (new_console)
    windows_set_console_info (&si, &flags);

#ifdef __CYGWIN__
  if (!useshell)
    {
      flags |= DEBUG_ONLY_THIS_PROCESS;
      if (cygwin_conv_path (CCP_POSIX_TO_WIN_W, exec_file, real_path,
			    __PMAX * sizeof (wchar_t)) < 0)
	error (_("Error starting executable: %d"), errno);
      toexec = real_path;
      len = mbstowcs (NULL, allargs, 0) + 1;
      if (len == (size_t) -1)
	error (_("Error starting executable: %d"), errno);
      cygallargs = (wchar_t *) alloca (len * sizeof (wchar_t));
      mbstowcs (cygallargs, allargs, len);
    }
  else
    {
      sh = get_shell ();
      if (cygwin_conv_path (CCP_POSIX_TO_WIN_W, sh, shell, __PMAX) < 0)
	error (_("Error starting executable via shell: %d"), errno);
      len = sizeof (L" -c 'exec  '") + mbstowcs (NULL, exec_file, 0)
	    + mbstowcs (NULL, allargs, 0) + 2;
      cygallargs = (wchar_t *) alloca (len * sizeof (wchar_t));
      swprintf (cygallargs, len, L" -c 'exec %s %s'", exec_file, allargs);
      toexec = shell;
      flags |= DEBUG_PROCESS;
    }

  if (inferior_cwd != NULL
      && cygwin_conv_path (CCP_POSIX_TO_WIN_W, inferior_cwd,
			   infcwd, strlen (inferior_cwd)) < 0)
    error (_("Error converting inferior cwd: %d"), errno);

  args = (wchar_t *) alloca ((wcslen (toexec) + wcslen (cygallargs) + 2)
			     * sizeof (wchar_t));
  wcscpy (args, toexec);
  wcscat (args, L" ");
  wcscat (args, cygallargs);

#ifdef CW_CVT_ENV_TO_WINENV
  /* First try to create a direct Win32 copy of the POSIX environment. */
  w32_env = (PWCHAR) cygwin_internal (CW_CVT_ENV_TO_WINENV, in_env);
  if (w32_env != (PWCHAR) -1)
    flags |= CREATE_UNICODE_ENVIRONMENT;
  else
    /* If that fails, fall back to old method tweaking GDB's environment. */
#endif	/* CW_CVT_ENV_TO_WINENV */
    {
      /* Reset all Win32 environment variables to avoid leftover on next run. */
      clear_win32_environment (environ);
      /* Prepare the environment vars for CreateProcess.  */
      old_env = environ;
      environ = in_env;
      cygwin_internal (CW_SYNC_WINENV);
      w32_env = NULL;
    }

  if (inferior_tty.empty ())
    tty = ostdin = ostdout = ostderr = -1;
  else
    {
      tty = open (inferior_tty.c_str (), O_RDWR | O_NOCTTY);
      if (tty < 0)
	{
	  warning_filename_and_errno (inferior_tty.c_str (), errno);
	  ostdin = ostdout = ostderr = -1;
	}
      else
	{
	  ostdin = dup (0);
	  ostdout = dup (1);
	  ostderr = dup (2);
	  dup2 (tty, 0);
	  dup2 (tty, 1);
	  dup2 (tty, 2);
	}
    }

  windows_init_thread_list ();
  do_synchronously ([&] ()
    {
      if (!create_process (nullptr, args, flags, w32_env,
			   inferior_cwd != nullptr ? infcwd : nullptr,
			   disable_randomization,
			   &si, &pi))
	ret = (unsigned) GetLastError ();
      return true;
    });

  if (w32_env)
    /* Just free the Win32 environment, if it could be created. */
    free (w32_env);
  else
    {
      /* Reset all environment variables to avoid leftover on next run. */
      clear_win32_environment (in_env);
      /* Restore normal GDB environment variables.  */
      environ = old_env;
      cygwin_internal (CW_SYNC_WINENV);
    }

  if (tty >= 0)
    {
      ::close (tty);
      dup2 (ostdin, 0);
      dup2 (ostdout, 1);
      dup2 (ostderr, 2);
      ::close (ostdin);
      ::close (ostdout);
      ::close (ostderr);
    }
#else  /* !__CYGWIN__ */
  allargs_len = strlen (allargs);
  allargs_copy = strcpy ((char *) alloca (allargs_len + 1), allargs);
  if (strpbrk (allargs_copy, "<>") != NULL)
    {
      int e = errno;
      errno = 0;
      redirected =
	redirect_inferior_handles (allargs, allargs_copy,
				   &fd_inp, &fd_out, &fd_err);
      if (errno)
	warning (_("Error in redirection: %s."), safe_strerror (errno));
      else
	errno = e;
      allargs_len = strlen (allargs_copy);
    }
  /* If not all the standard streams are redirected by the command
     line, use INFERIOR_TTY for those which aren't.  */
  if (!inferior_tty.empty ()
      && !(fd_inp >= 0 && fd_out >= 0 && fd_err >= 0))
    {
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof(sa);
      sa.lpSecurityDescriptor = 0;
      sa.bInheritHandle = TRUE;
      tty = CreateFileA (inferior_tty.c_str (), GENERIC_READ | GENERIC_WRITE,
			 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (tty == INVALID_HANDLE_VALUE)
	{
	  unsigned err = (unsigned) GetLastError ();
	  warning (_("Warning: Failed to open TTY %s, error %#x: %s"),
		   inferior_tty.c_str (), err, strwinerror (err));
	}
    }
  if (redirected || tty != INVALID_HANDLE_VALUE)
    {
      if (fd_inp >= 0)
	si.hStdInput = (HANDLE) _get_osfhandle (fd_inp);
      else if (tty != INVALID_HANDLE_VALUE)
	si.hStdInput = tty;
      else
	si.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
      if (fd_out >= 0)
	si.hStdOutput = (HANDLE) _get_osfhandle (fd_out);
      else if (tty != INVALID_HANDLE_VALUE)
	si.hStdOutput = tty;
      else
	si.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
      if (fd_err >= 0)
	si.hStdError = (HANDLE) _get_osfhandle (fd_err);
      else if (tty != INVALID_HANDLE_VALUE)
	si.hStdError = tty;
      else
	si.hStdError = GetStdHandle (STD_ERROR_HANDLE);
      si.dwFlags |= STARTF_USESTDHANDLES;
    }

  toexec = exec_file;
  /* Build the command line, a space-separated list of tokens where
     the first token is the name of the module to be executed.
     To avoid ambiguities introduced by spaces in the module name,
     we quote it.  */
  args_len = strlen (toexec) + 2 /* quotes */ + allargs_len + 2;
  args = (char *) alloca (args_len);
  xsnprintf (args, args_len, "\"%s\" %s", toexec, allargs_copy);

  flags |= DEBUG_ONLY_THIS_PROCESS;

  /* CreateProcess takes the environment list as a null terminated set of
     strings (i.e. two nulls terminate the list).  */

  /* Get total size for env strings.  */
  for (envlen = 0, i = 0; in_env[i] && *in_env[i]; i++)
    envlen += strlen (in_env[i]) + 1;

  envsize = sizeof (in_env[0]) * (i + 1);
  env = (char **) alloca (envsize);
  memcpy (env, in_env, envsize);
  /* Windows programs expect the environment block to be sorted.  */
  qsort (env, i, sizeof (char *), envvar_cmp);

  w32env = (char *) alloca (envlen + 1);

  /* Copy env strings into new buffer.  */
  for (temp = w32env, i = 0; env[i] && *env[i]; i++)
    {
      strcpy (temp, env[i]);
      temp += strlen (temp) + 1;
    }

  /* Final nil string to terminate new env.  */
  *temp = 0;

  windows_init_thread_list ();
  do_synchronously ([&] ()
    {
      if (!create_process (nullptr, /* image */
			   args,	/* command line */
			   flags,	/* start flags */
			   w32env,	/* environment */
			   inferior_cwd, /* current directory */
			   disable_randomization,
			   &si,
			   &pi))
	ret = (unsigned) GetLastError ();
      return true;
    });
  if (tty != INVALID_HANDLE_VALUE)
    CloseHandle (tty);
  if (fd_inp >= 0)
    _close (fd_inp);
  if (fd_out >= 0)
    _close (fd_out);
  if (fd_err >= 0)
    _close (fd_err);
#endif	/* !__CYGWIN__ */

  if (ret.has_value ())
    {
      std::string msg = _("Error creating process ") + std::string (exec_file);
      throw_winerror_with_name (msg.c_str (), *ret);
    }

#ifdef __x86_64__
  BOOL wow64;
  if (IsWow64Process (pi.hProcess, &wow64))
    windows_process.wow64_process = wow64;
#endif

  CloseHandle (pi.hThread);
  CloseHandle (pi.hProcess);

  if (useshell && shell[0] != '\0')
    windows_process.saw_create = -1;
  else
    windows_process.saw_create = 0;

  do_initial_windows_stuff (pi.dwProcessId, 0);

  /* windows_continue (DBG_CONTINUE, -1, 0); */
}

void
windows_nat_target::mourn_inferior ()
{
  (void) windows_continue (DBG_CONTINUE, -1, 0, true);
  x86_cleanup_dregs();
  if (windows_process.open_process_used)
    {
      CHECK (CloseHandle (windows_process.handle));
      windows_process.open_process_used = 0;
    }
  windows_process.siginfo_er.ExceptionCode = 0;
  inf_child_target::mourn_inferior ();
}

/* Helper for windows_xfer_partial that handles memory transfers.
   Arguments are like target_xfer_partial.  */

static enum target_xfer_status
windows_xfer_memory (gdb_byte *readbuf, const gdb_byte *writebuf,
		     ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  SIZE_T done = 0;
  BOOL success;
  DWORD lasterror = 0;

  if (writebuf != NULL)
    {
      DEBUG_MEM ("write target memory, %s bytes at %s",
		 pulongest (len), core_addr_to_string (memaddr));
      success = WriteProcessMemory (windows_process.handle,
				    (LPVOID) (uintptr_t) memaddr, writebuf,
				    len, &done);
      if (!success)
	lasterror = GetLastError ();
      FlushInstructionCache (windows_process.handle,
			     (LPCVOID) (uintptr_t) memaddr, len);
    }
  else
    {
      DEBUG_MEM ("read target memory, %s bytes at %s",
		 pulongest (len), core_addr_to_string (memaddr));
      success = ReadProcessMemory (windows_process.handle,
				   (LPCVOID) (uintptr_t) memaddr, readbuf,
				   len, &done);
      if (!success)
	lasterror = GetLastError ();
    }
  *xfered_len = (ULONGEST) done;
  if (!success && lasterror == ERROR_PARTIAL_COPY && done > 0)
    return TARGET_XFER_OK;
  else
    return success ? TARGET_XFER_OK : TARGET_XFER_E_IO;
}

void
windows_nat_target::kill ()
{
  CHECK (TerminateProcess (windows_process.handle, 0));

  for (;;)
    {
      if (!windows_continue (DBG_CONTINUE, -1, 1))
	break;
      wait_for_debug_event_main_thread (&windows_process.current_event);
      if (windows_process.current_event.dwDebugEventCode
	  == EXIT_PROCESS_DEBUG_EVENT)
	break;
    }

  target_mourn_inferior (inferior_ptid);	/* Or just windows_mourn_inferior?  */
}

void
windows_nat_target::close ()
{
  DEBUG_EVENTS ("inferior_ptid=%d\n", inferior_ptid.pid ());
  async (false);
}

/* Convert pid to printable format.  */
std::string
windows_nat_target::pid_to_str (ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("Thread %d.0x%lx", ptid.pid (), ptid.lwp ());

  return normal_pid_to_str (ptid);
}

static enum target_xfer_status
windows_xfer_shared_libraries (struct target_ops *ops,
			       enum target_object object, const char *annex,
			       gdb_byte *readbuf, const gdb_byte *writebuf,
			       ULONGEST offset, ULONGEST len,
			       ULONGEST *xfered_len)
{
  if (writebuf)
    return TARGET_XFER_E_IO;

  std::string xml = "<library-list>\n";
  for (windows_solib &so : windows_process.solibs)
    windows_xfer_shared_library (so.name.c_str (),
				 (CORE_ADDR) (uintptr_t) so.load_addr,
				 &so.text_offset,
				 current_inferior ()->arch (), xml);
  xml += "</library-list>\n";

  ULONGEST len_avail = xml.size ();
  if (offset >= len_avail)
    len = 0;
  else
    {
      if (len > len_avail - offset)
	len = len_avail - offset;
      memcpy (readbuf, xml.data () + offset, len);
    }

  *xfered_len = (ULONGEST) len;
  return len != 0 ? TARGET_XFER_OK : TARGET_XFER_EOF;
}

/* Helper for windows_nat_target::xfer_partial that handles signal info.  */

static enum target_xfer_status
windows_xfer_siginfo (gdb_byte *readbuf, ULONGEST offset, ULONGEST len,
		      ULONGEST *xfered_len)
{
  char *buf = (char *) &windows_process.siginfo_er;
  size_t bufsize = sizeof (windows_process.siginfo_er);

#ifdef __x86_64__
  EXCEPTION_RECORD32 er32;
  if (windows_process.wow64_process)
    {
      buf = (char *) &er32;
      bufsize = sizeof (er32);

      er32.ExceptionCode = windows_process.siginfo_er.ExceptionCode;
      er32.ExceptionFlags = windows_process.siginfo_er.ExceptionFlags;
      er32.ExceptionRecord
	= (uintptr_t) windows_process.siginfo_er.ExceptionRecord;
      er32.ExceptionAddress
	= (uintptr_t) windows_process.siginfo_er.ExceptionAddress;
      er32.NumberParameters = windows_process.siginfo_er.NumberParameters;
      int i;
      for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
	er32.ExceptionInformation[i]
	  = windows_process.siginfo_er.ExceptionInformation[i];
    }
#endif

  if (windows_process.siginfo_er.ExceptionCode == 0)
    return TARGET_XFER_E_IO;

  if (readbuf == nullptr)
    return TARGET_XFER_E_IO;

  if (offset > bufsize)
    return TARGET_XFER_E_IO;

  if (offset + len > bufsize)
    len = bufsize - offset;

  memcpy (readbuf, buf + offset, len);
  *xfered_len = len;

  return TARGET_XFER_OK;
}

enum target_xfer_status
windows_nat_target::xfer_partial (enum target_object object,
				  const char *annex, gdb_byte *readbuf,
				  const gdb_byte *writebuf, ULONGEST offset,
				  ULONGEST len, ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return windows_xfer_memory (readbuf, writebuf, offset, len, xfered_len);

    case TARGET_OBJECT_LIBRARIES:
      return windows_xfer_shared_libraries (this, object, annex, readbuf,
					    writebuf, offset, len, xfered_len);

    case TARGET_OBJECT_SIGNAL_INFO:
      return windows_xfer_siginfo (readbuf, offset, len, xfered_len);

    default:
      if (beneath () == NULL)
	{
	  /* This can happen when requesting the transfer of unsupported
	     objects before a program has been started (and therefore
	     with the current_target having no target beneath).  */
	  return TARGET_XFER_E_IO;
	}
      return beneath ()->xfer_partial (object, annex,
				       readbuf, writebuf, offset, len,
				       xfered_len);
    }
}

/* Provide thread local base, i.e. Thread Information Block address.
   Returns 1 if ptid is found and sets *ADDR to thread_local_base.  */

bool
windows_nat_target::get_tib_address (ptid_t ptid, CORE_ADDR *addr)
{
  windows_thread_info *th;

  th = windows_process.thread_rec (ptid, DONT_INVALIDATE_CONTEXT);
  if (th == NULL)
    return false;

  if (addr != NULL)
    *addr = th->thread_local_base;

  return true;
}

ptid_t
windows_nat_target::get_ada_task_ptid (long lwp, ULONGEST thread)
{
  return ptid_t (inferior_ptid.pid (), lwp, 0);
}

/* Implementation of the to_thread_name method.  */

const char *
windows_nat_target::thread_name (struct thread_info *thr)
{
  windows_thread_info *th
    = windows_process.thread_rec (thr->ptid,
				  DONT_INVALIDATE_CONTEXT);
  return th->thread_name ();
}


void _initialize_windows_nat ();
void
_initialize_windows_nat ()
{
  x86_dr_low.set_control = cygwin_set_dr7;
  x86_dr_low.set_addr = cygwin_set_dr;
  x86_dr_low.get_addr = cygwin_get_dr;
  x86_dr_low.get_status = cygwin_get_dr6;
  x86_dr_low.get_control = cygwin_get_dr7;

  /* x86_dr_low.debug_register_length field is set by
     calling x86_set_debug_register_length function
     in processor windows specific native file.  */

  /* The target is not a global specifically to avoid a C++ "static
     initializer fiasco" situation.  */
  add_inf_child_target (new windows_nat_target);

#ifdef __CYGWIN__
  cygwin_internal (CW_SET_DOS_FILE_WARNING, 0);
#endif

  add_com ("signal-event", class_run, signal_event_command, _("\
Signal a crashed process with event ID, to allow its debugging.\n\
This command is needed in support of setting up GDB as JIT debugger on \
MS-Windows.  The command should be invoked from the GDB command line using \
the '-ex' command-line option.  The ID of the event that blocks the \
crashed process will be supplied by the Windows JIT debugging mechanism."));

#ifdef __CYGWIN__
  add_setshow_boolean_cmd ("shell", class_support, &useshell, _("\
Set use of shell to start subprocess."), _("\
Show use of shell to start subprocess."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("cygwin-exceptions", class_support,
			   &cygwin_exceptions, _("\
Break when an exception is detected in the Cygwin DLL itself."), _("\
Show whether gdb breaks on exceptions in the Cygwin DLL itself."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);
#endif

  add_setshow_boolean_cmd ("new-console", class_support, &new_console, _("\
Set creation of new console when creating child process."), _("\
Show creation of new console when creating child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("new-group", class_support, &new_group, _("\
Set creation of new group when creating child process."), _("\
Show creation of new group when creating child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugexec", class_support, &debug_exec, _("\
Set whether to display execution in child process."), _("\
Show whether to display execution in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugevents", class_support, &debug_events, _("\
Set whether to display kernel events in child process."), _("\
Show whether to display kernel events in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugmemory", class_support, &debug_memory, _("\
Set whether to display memory accesses in child process."), _("\
Show whether to display memory accesses in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("debugexceptions", class_support,
			   &debug_exceptions, _("\
Set whether to display kernel exceptions in child process."), _("\
Show whether to display kernel exceptions in child process."), NULL,
			   NULL,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  init_w32_command_list ();

  add_cmd ("selector", class_info, display_selectors,
	   _("Display selectors infos."),
	   &info_w32_cmdlist);

  if (!initialize_loadable ())
    {
      /* This will probably fail on Windows 9x/Me.  Let the user know
	 that we're missing some functionality.  */
      warning(_("\
cannot automatically find executable file or library to read symbols.\n\
Use \"file\" or \"dll\" command to load executable/libraries directly."));
    }
}

/* Hardware watchpoint support, adapted from go32-nat.c code.  */

/* Pass the address ADDR to the inferior in the I'th debug register.
   Here we just store the address in dr array, the registers will be
   actually set up when windows_continue is called.  */
static void
cygwin_set_dr (int i, CORE_ADDR addr)
{
  if (i < 0 || i > 3)
    internal_error (_("Invalid register %d in cygwin_set_dr.\n"), i);
  windows_process.dr[i] = addr;

  for (auto &th : windows_process.thread_list)
    th->debug_registers_changed = true;
}

/* Pass the value VAL to the inferior in the DR7 debug control
   register.  Here we just store the address in D_REGS, the watchpoint
   will be actually set up in windows_wait.  */
static void
cygwin_set_dr7 (unsigned long val)
{
  windows_process.dr[7] = (CORE_ADDR) val;

  for (auto &th : windows_process.thread_list)
    th->debug_registers_changed = true;
}

/* Get the value of debug register I from the inferior.  */

static CORE_ADDR
cygwin_get_dr (int i)
{
  return windows_process.dr[i];
}

/* Get the value of the DR6 debug status register from the inferior.
   Here we just return the value stored in dr[6]
   by the last call to thread_rec for current_event.dwThreadId id.  */
static unsigned long
cygwin_get_dr6 (void)
{
  return (unsigned long) windows_process.dr[6];
}

/* Get the value of the DR7 debug status register from the inferior.
   Here we just return the value stored in dr[7] by the last call to
   thread_rec for current_event.dwThreadId id.  */

static unsigned long
cygwin_get_dr7 (void)
{
  return (unsigned long) windows_process.dr[7];
}

/* Determine if the thread referenced by "ptid" is alive
   by "polling" it.  If WaitForSingleObject returns WAIT_OBJECT_0
   it means that the thread has died.  Otherwise it is assumed to be alive.  */

bool
windows_nat_target::thread_alive (ptid_t ptid)
{
  gdb_assert (ptid.lwp () != 0);

  windows_thread_info *th
    = windows_process.thread_rec (ptid, DONT_INVALIDATE_CONTEXT);
  return WaitForSingleObject (th->h, 0) != WAIT_OBJECT_0;
}

void _initialize_check_for_gdb_ini ();
void
_initialize_check_for_gdb_ini ()
{
  char *homedir;
  if (inhibit_gdbinit)
    return;

  homedir = getenv ("HOME");
  if (homedir)
    {
      char *p;
      char *oldini = (char *) alloca (strlen (homedir) +
				      sizeof ("gdb.ini") + 1);
      strcpy (oldini, homedir);
      p = strchr (oldini, '\0');
      if (p > oldini && !IS_DIR_SEPARATOR (p[-1]))
	*p++ = '/';
      strcpy (p, "gdb.ini");
      if (access (oldini, 0) == 0)
	{
	  int len = strlen (oldini);
	  char *newini = (char *) alloca (len + 2);

	  xsnprintf (newini, len + 2, "%.*s.gdbinit",
		     (int) (len - (sizeof ("gdb.ini") - 1)), oldini);
	  warning (_("obsolete '%s' found. Rename to '%s'."), oldini, newini);
	}
    }
}

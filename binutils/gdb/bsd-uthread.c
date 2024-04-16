/* BSD user-level threads support.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "inferior.h"
#include "objfiles.h"
#include "observable.h"
#include "regcache.h"
#include "solib.h"
#include "solist.h"
#include "symfile.h"
#include "target.h"

#include "gdbsupport/gdb_obstack.h"

#include "bsd-uthread.h"

static const target_info bsd_uthread_target_info = {
  "bsd-uthreads",
  N_("BSD user-level threads"),
  N_("BSD user-level threads")
};

struct bsd_uthread_target final : public target_ops
{
  const target_info &info () const override
  { return bsd_uthread_target_info; }

  strata stratum () const override { return thread_stratum; }

  void close () override;

  void mourn_inferior () override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;
  void resume (ptid_t, int, enum gdb_signal) override;

  bool thread_alive (ptid_t ptid) override;

  void update_thread_list () override;

  const char *extra_thread_info (struct thread_info *) override;

  std::string pid_to_str (ptid_t) override;
};

static bsd_uthread_target bsd_uthread_ops;


/* Architecture-specific operations.  */

struct bsd_uthread_ops
{
  /* Supply registers for an inactive thread to a register cache.  */
  void (*supply_uthread)(struct regcache *, int, CORE_ADDR) = nullptr;

  /* Collect registers for an inactive thread from a register cache.  */
  void (*collect_uthread)(const struct regcache *, int, CORE_ADDR) = nullptr;
};

/* Per-architecture data key.  */
static const registry<gdbarch>::key<struct bsd_uthread_ops> bsd_uthread_data;

static struct bsd_uthread_ops *
get_bsd_uthread (struct gdbarch *gdbarch)
{
  struct bsd_uthread_ops *ops = bsd_uthread_data.get (gdbarch);
  if (ops == nullptr)
    ops = bsd_uthread_data.emplace (gdbarch);
  return ops;
}

/* Set the function that supplies registers from an inactive thread
   for architecture GDBARCH to SUPPLY_UTHREAD.  */

void
bsd_uthread_set_supply_uthread (struct gdbarch *gdbarch,
				void (*supply_uthread) (struct regcache *,
							int, CORE_ADDR))
{
  struct bsd_uthread_ops *ops = get_bsd_uthread (gdbarch);

  ops->supply_uthread = supply_uthread;
}

/* Set the function that collects registers for an inactive thread for
   architecture GDBARCH to SUPPLY_UTHREAD.  */

void
bsd_uthread_set_collect_uthread (struct gdbarch *gdbarch,
			 void (*collect_uthread) (const struct regcache *,
						  int, CORE_ADDR))
{
  struct bsd_uthread_ops *ops = get_bsd_uthread (gdbarch);

  ops->collect_uthread = collect_uthread;
}

/* Magic number to help recognize a valid thread structure.  */
#define BSD_UTHREAD_PTHREAD_MAGIC	0xd09ba115

/* Check whether the thread structure at ADDR is valid.  */

static void
bsd_uthread_check_magic (CORE_ADDR addr)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  ULONGEST magic = read_memory_unsigned_integer (addr, 4, byte_order);

  if (magic != BSD_UTHREAD_PTHREAD_MAGIC)
    error (_("Bad magic"));
}

/* Thread states.  */
#define BSD_UTHREAD_PS_RUNNING	0
#define BSD_UTHREAD_PS_DEAD	18

/* Address of the pointer to the thread structure for the running
   thread.  */
static CORE_ADDR bsd_uthread_thread_run_addr;

/* Address of the list of all threads.  */
static CORE_ADDR bsd_uthread_thread_list_addr;

/* Offsets of various "interesting" bits in the thread structure.  */
static int bsd_uthread_thread_state_offset = -1;
static int bsd_uthread_thread_next_offset = -1;
static int bsd_uthread_thread_ctx_offset;

/* Name of shared threads library.  */
static std::string bsd_uthread_solib_name;

/* Non-zero if the thread stratum implemented by this module is active.  */
static int bsd_uthread_active;

static CORE_ADDR
bsd_uthread_lookup_address (const char *name, struct objfile *objfile)
{
  struct bound_minimal_symbol sym;

  sym = lookup_minimal_symbol (name, NULL, objfile);
  if (sym.minsym)
    return sym.value_address ();

  return 0;
}

static int
bsd_uthread_lookup_offset (const char *name, struct objfile *objfile)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  CORE_ADDR addr;

  addr = bsd_uthread_lookup_address (name, objfile);
  if (addr == 0)
    return 0;

  return read_memory_unsigned_integer (addr, 4, byte_order);
}

static CORE_ADDR
bsd_uthread_read_memory_address (CORE_ADDR addr)
{
  type *ptr_type
    = builtin_type (current_inferior ()->arch ())->builtin_data_ptr;
  return read_memory_typed_address (addr, ptr_type);
}

/* If OBJFILE contains the symbols corresponding to one of the
   supported user-level threads libraries, activate the thread stratum
   implemented by this module.  */

static int
bsd_uthread_activate (struct objfile *objfile)
{
  gdbarch *gdbarch = current_inferior ()->arch ();
  struct bsd_uthread_ops *ops = get_bsd_uthread (gdbarch);

  /* Skip if the thread stratum has already been activated.  */
  if (bsd_uthread_active)
    return 0;

  /* There's no point in enabling this module if no
     architecture-specific operations are provided.  */
  if (!ops->supply_uthread)
    return 0;

  bsd_uthread_thread_run_addr =
    bsd_uthread_lookup_address ("_thread_run", objfile);
  if (bsd_uthread_thread_run_addr == 0)
    return 0;

  bsd_uthread_thread_list_addr =
    bsd_uthread_lookup_address ("_thread_list", objfile);
  if (bsd_uthread_thread_list_addr == 0)
    return 0;

  bsd_uthread_thread_state_offset =
    bsd_uthread_lookup_offset ("_thread_state_offset", objfile);
  if (bsd_uthread_thread_state_offset == 0)
    return 0;

  bsd_uthread_thread_next_offset =
    bsd_uthread_lookup_offset ("_thread_next_offset", objfile);
  if (bsd_uthread_thread_next_offset == 0)
    return 0;

  bsd_uthread_thread_ctx_offset =
    bsd_uthread_lookup_offset ("_thread_ctx_offset", objfile);

  current_inferior ()->push_target (&bsd_uthread_ops);
  bsd_uthread_active = 1;
  return 1;
}

/* Cleanup due to deactivation.  */

void
bsd_uthread_target::close ()
{
  bsd_uthread_active = 0;
  bsd_uthread_thread_run_addr = 0;
  bsd_uthread_thread_list_addr = 0;
  bsd_uthread_thread_state_offset = 0;
  bsd_uthread_thread_next_offset = 0;
  bsd_uthread_thread_ctx_offset = 0;
  bsd_uthread_solib_name.clear ();
}

/* Deactivate the thread stratum implemented by this module.  */

static void
bsd_uthread_deactivate (void)
{
  /* Skip if the thread stratum has already been deactivated.  */
  if (!bsd_uthread_active)
    return;

  current_inferior ()->unpush_target (&bsd_uthread_ops);
}

static void
bsd_uthread_inferior_created (inferior *inf)
{
  bsd_uthread_activate (NULL);
}

/* Likely candidates for the threads library.  */
static const char * const bsd_uthread_solib_names[] =
{
  "/usr/lib/libc_r.so",		/* FreeBSD */
  "/usr/lib/libpthread.so",	/* OpenBSD */
  NULL
};

static void
bsd_uthread_solib_loaded (shobj &so)
{
  const char * const *names = bsd_uthread_solib_names;

  for (names = bsd_uthread_solib_names; *names; names++)
    {
      if (startswith (so.so_original_name, *names))
	{
	  solib_read_symbols (so, 0);

	  if (bsd_uthread_activate (so.objfile))
	    {
	      bsd_uthread_solib_name = so.so_original_name;
	      return;
	    }
	}
    }
}

static void
bsd_uthread_solib_unloaded (program_space *pspace, const shobj &so)
{
  if (bsd_uthread_solib_name.empty ())
    return;

  if (so.so_original_name == bsd_uthread_solib_name)
    bsd_uthread_deactivate ();
}

void
bsd_uthread_target::mourn_inferior ()
{
  beneath ()->mourn_inferior ();
  bsd_uthread_deactivate ();
}

void
bsd_uthread_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct bsd_uthread_ops *uthread_ops = get_bsd_uthread (gdbarch);
  ptid_t ptid = regcache->ptid ();
  CORE_ADDR addr = ptid.tid ();
  CORE_ADDR active_addr;
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  /* We are doing operations (e.g. reading memory) that rely on
     inferior_ptid.  */
  inferior_ptid = ptid;

  /* Always fetch the appropriate registers from the layer beneath.  */
  beneath ()->fetch_registers (regcache, regnum);

  /* FIXME: That might have gotten us more than we asked for.  Make
     sure we overwrite all relevant registers with values from the
     thread structure.  This can go once we fix the underlying target.  */
  regnum = -1;

  active_addr = bsd_uthread_read_memory_address (bsd_uthread_thread_run_addr);
  if (addr != 0 && addr != active_addr)
    {
      bsd_uthread_check_magic (addr);
      uthread_ops->supply_uthread (regcache, regnum,
				   addr + bsd_uthread_thread_ctx_offset);
    }
}

void
bsd_uthread_target::store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct bsd_uthread_ops *uthread_ops = get_bsd_uthread (gdbarch);
  ptid_t ptid = regcache->ptid ();
  CORE_ADDR addr = ptid.tid ();
  CORE_ADDR active_addr;
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  /* We are doing operations (e.g. reading memory) that rely on
     inferior_ptid.  */
  inferior_ptid = ptid;

  active_addr = bsd_uthread_read_memory_address (bsd_uthread_thread_run_addr);
  if (addr != 0 && addr != active_addr)
    {
      bsd_uthread_check_magic (addr);
      uthread_ops->collect_uthread (regcache, regnum,
				    addr + bsd_uthread_thread_ctx_offset);
    }
  else
    {
      /* Updating the thread that is currently running; pass the
	 request to the layer beneath.  */
      beneath ()->store_registers (regcache, regnum);
    }
}

ptid_t
bsd_uthread_target::wait (ptid_t ptid, struct target_waitstatus *status,
			  target_wait_flags options)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  CORE_ADDR addr;
  process_stratum_target *beneath
    = as_process_stratum_target (this->beneath ());

  /* Pass the request to the layer beneath.  */
  ptid = beneath->wait (ptid, status, options);

  /* If the process is no longer alive, there's no point in figuring
     out the thread ID.  It will fail anyway.  */
  if (status->kind () == TARGET_WAITKIND_SIGNALLED
      || status->kind () == TARGET_WAITKIND_EXITED)
    return ptid;

  /* Fetch the corresponding thread ID, and augment the returned
     process ID with it.  */
  addr = bsd_uthread_read_memory_address (bsd_uthread_thread_run_addr);
  if (addr != 0)
    {
      gdb_byte buf[4];

      /* FIXME: For executables linked statically with the threads
	 library, we end up here before the program has actually been
	 executed.  In that case ADDR will be garbage since it has
	 been read from the wrong virtual memory image.  */
      if (target_read_memory (addr, buf, 4) == 0)
	{
	  ULONGEST magic = extract_unsigned_integer (buf, 4, byte_order);
	  if (magic == BSD_UTHREAD_PTHREAD_MAGIC)
	    ptid = ptid_t (ptid.pid (), 0, addr);
	}
    }

  /* If INFERIOR_PTID doesn't have a tid member yet, and we now have a
     ptid with tid set, then ptid is still the initial thread of
     the process.  Notify GDB core about it.  */
  if (inferior_ptid.tid () == 0
      && ptid.tid () != 0 && !in_thread_list (beneath, ptid))
    thread_change_ptid (beneath, inferior_ptid, ptid);

  /* Don't let the core see a ptid without a corresponding thread.  */
  thread_info *thread = beneath->find_thread (ptid);
  if (thread == NULL || thread->state == THREAD_EXITED)
    add_thread (beneath, ptid);

  return ptid;
}

void
bsd_uthread_target::resume (ptid_t ptid, int step, enum gdb_signal sig)
{
  /* Pass the request to the layer beneath.  */
  beneath ()->resume (ptid, step, sig);
}

bool
bsd_uthread_target::thread_alive (ptid_t ptid)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  CORE_ADDR addr = ptid.tid ();

  if (addr != 0)
    {
      int offset = bsd_uthread_thread_state_offset;
      ULONGEST state;

      bsd_uthread_check_magic (addr);

      state = read_memory_unsigned_integer (addr + offset, 4, byte_order);
      if (state == BSD_UTHREAD_PS_DEAD)
	return false;
    }

  return beneath ()->thread_alive (ptid);
}

void
bsd_uthread_target::update_thread_list ()
{
  pid_t pid = inferior_ptid.pid ();
  int offset = bsd_uthread_thread_next_offset;
  CORE_ADDR addr;

  prune_threads ();

  addr = bsd_uthread_read_memory_address (bsd_uthread_thread_list_addr);
  while (addr != 0)
    {
      ptid_t ptid = ptid_t (pid, 0, addr);

      process_stratum_target *proc_target
	= as_process_stratum_target (this->beneath ());
      thread_info *thread = proc_target->find_thread (ptid);
      if (thread == nullptr || thread->state == THREAD_EXITED)
	{
	  /* If INFERIOR_PTID doesn't have a tid member yet, then ptid
	     is still the initial thread of the process.  Notify GDB
	     core about it.  */
	  if (inferior_ptid.tid () == 0)
	    thread_change_ptid (proc_target, inferior_ptid, ptid);
	  else
	    add_thread (proc_target, ptid);
	}

      addr = bsd_uthread_read_memory_address (addr + offset);
    }
}

/* Possible states a thread can be in.  */
static const char * const bsd_uthread_state[] =
{
  "RUNNING",
  "SIGTHREAD",
  "MUTEX_WAIT",
  "COND_WAIT",
  "FDLR_WAIT",
  "FDLW_WAIT",
  "FDR_WAIT",
  "FDW_WAIT",
  "FILE_WAIT",
  "POLL_WAIT",
  "SELECT_WAIT",
  "SLEEP_WAIT",
  "WAIT_WAIT",
  "SIGSUSPEND",
  "SIGWAIT",
  "SPINBLOCK",
  "JOIN",
  "SUSPENDED",
  "DEAD",
  "DEADLOCK"
};

/* Return a string describing th state of the thread specified by
   INFO.  */

const char *
bsd_uthread_target::extra_thread_info (thread_info *info)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  CORE_ADDR addr = info->ptid.tid ();

  if (addr != 0)
    {
      int offset = bsd_uthread_thread_state_offset;
      ULONGEST state;

      state = read_memory_unsigned_integer (addr + offset, 4, byte_order);
      if (state < ARRAY_SIZE (bsd_uthread_state))
	return bsd_uthread_state[state];
    }

  return NULL;
}

std::string
bsd_uthread_target::pid_to_str (ptid_t ptid)
{
  if (ptid.tid () != 0)
    return string_printf ("process %d, thread 0x%s",
			  ptid.pid (),
			  phex_nz (ptid.tid (), sizeof (ULONGEST)));

  return normal_pid_to_str (ptid);
}

void _initialize_bsd_uthread ();
void
_initialize_bsd_uthread ()
{
  gdb::observers::inferior_created.attach (bsd_uthread_inferior_created,
					   "bsd-uthread");
  gdb::observers::solib_loaded.attach (bsd_uthread_solib_loaded,
				       "bsd-uthread");
  gdb::observers::solib_unloaded.attach (bsd_uthread_solib_unloaded,
					 "bsd-uthread");
}

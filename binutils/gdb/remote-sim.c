/* Generic remote debugging interface for simulators.

   Copyright (C) 1993-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support.
   Steve Chamberlain (sac@cygnus.com).

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
#include "gdb_bfd.h"
#include "inferior.h"
#include "infrun.h"
#include "value.h"
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include "terminal.h"
#include "target.h"
#include "process-stratum-target.h"
#include "gdbcore.h"
#include "sim/callback.h"
#include "sim/sim.h"
#include "command.h"
#include "regcache.h"
#include "sim-regno.h"
#include "arch-utils.h"
#include "readline/readline.h"
#include "gdbthread.h"
#include "gdbsupport/byte-vector.h"
#include "memory-map.h"
#include "remote.h"
#include "gdbsupport/buildargv.h"

/* Prototypes */

static void init_callbacks (void);

static void end_callbacks (void);

static int gdb_os_write_stdout (host_callback *, const char *, int);

static void gdb_os_flush_stdout (host_callback *);

static int gdb_os_write_stderr (host_callback *, const char *, int);

static void gdb_os_flush_stderr (host_callback *);

static int gdb_os_poll_quit (host_callback *);

/* gdb_printf is depreciated.  */
static void gdb_os_printf_filtered (host_callback *, const char *, ...);

static void gdb_os_vprintf_filtered (host_callback *, const char *, va_list);

static void gdb_os_evprintf_filtered (host_callback *, const char *, va_list);

static void gdb_os_error (host_callback *, const char *, ...)
     ATTRIBUTE_NORETURN;

/* Naming convention:

   sim_* are the interface to the simulator (see remote-sim.h).
   gdbsim_* are stuff which is internal to gdb.  */

/* Value of the next pid to allocate for an inferior.  As indicated
   elsewhere, its initial value is somewhat arbitrary; it's critical
   though that it's not zero or negative.  */
static int next_pid;
#define INITIAL_PID 42000

/* Simulator-specific, per-inferior state.  */
struct sim_inferior_data {
  explicit sim_inferior_data (SIM_DESC desc)
    : gdbsim_desc (desc),
      remote_sim_ptid (next_pid, 0, next_pid)
  {
    gdb_assert (remote_sim_ptid != null_ptid);
    ++next_pid;
  }

  ~sim_inferior_data ();

  /* Flag which indicates whether or not the program has been loaded.  */
  bool program_loaded = false;

  /* Simulator descriptor for this inferior.  */
  SIM_DESC gdbsim_desc;

  /* This is the ptid we use for this particular simulator instance.  Its
     value is somewhat arbitrary, as the simulator target don't have a
     notion of tasks or threads, but we need something non-null to place
     in inferior_ptid.  For simulators which permit multiple instances,
     we also need a unique identifier to use for each inferior.  */
  ptid_t remote_sim_ptid;

  /* Signal with which to resume.  */
  enum gdb_signal resume_siggnal = GDB_SIGNAL_0;

  /* Flag which indicates whether resume should step or not.  */
  bool resume_step = false;
};

static const target_info gdbsim_target_info = {
  "sim",
  N_("simulator"),
  N_("Use the compiled-in simulator.")
};

struct gdbsim_target final
  : public memory_breakpoint_target<process_stratum_target>
{
  gdbsim_target () = default;

  const target_info &info () const override
  { return gdbsim_target_info; }

  void close () override;

  void detach (inferior *inf, int) override;

  void resume (ptid_t, int, enum gdb_signal) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
  void prepare_to_store (struct regcache *) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void files_info () override;

  void kill () override;

  void load (const char *, int) override;

  bool can_create_inferior () override { return true; }
  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void mourn_inferior () override;

  void interrupt () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  bool has_all_memory ()  override;
  bool has_memory ()  override;
  std::vector<mem_region> memory_map () override;

private:
  sim_inferior_data *get_inferior_data_by_ptid (ptid_t ptid,
						int sim_instance_needed);
  void resume_one_inferior (inferior *inf, bool step, gdb_signal siggnal);
  void close_one_inferior (inferior *inf);
};

static struct gdbsim_target gdbsim_ops;

static const registry<inferior>::key<sim_inferior_data> sim_inferior_data_key;

/* Flag indicating the "open" status of this module.  It's set true
   in gdbsim_open() and false in gdbsim_close().  */
static bool gdbsim_is_open = false;

/* Argument list to pass to sim_open().  It is allocated in gdbsim_open()
   and deallocated in gdbsim_close().  The lifetime needs to extend beyond
   the call to gdbsim_open() due to the fact that other sim instances other
   than the first will be allocated after the gdbsim_open() call.  */
static char **sim_argv = NULL;

/* OS-level callback functions for write, flush, etc.  */
static host_callback gdb_callback;
static int callbacks_initialized = 0;

/* Flags indicating whether or not a sim instance is needed.  One of these
   flags should be passed to get_sim_inferior_data().  */

enum {SIM_INSTANCE_NOT_NEEDED = 0, SIM_INSTANCE_NEEDED = 1};

/* Obtain pointer to per-inferior simulator data, allocating it if necessary.
   Attempt to open the sim if SIM_INSTANCE_NEEDED is true.  */

static struct sim_inferior_data *
get_sim_inferior_data (struct inferior *inf, int sim_instance_needed)
{
  SIM_DESC sim_desc = NULL;
  struct sim_inferior_data *sim_data = sim_inferior_data_key.get (inf);

  /* Try to allocate a new sim instance, if needed.  We do this ahead of
     a potential allocation of a sim_inferior_data struct in order to
     avoid needlessly allocating that struct in the event that the sim
     instance allocation fails.  */
  if (sim_instance_needed == SIM_INSTANCE_NEEDED
      && (sim_data == NULL || sim_data->gdbsim_desc == NULL))
    {
      sim_desc = sim_open (SIM_OPEN_DEBUG, &gdb_callback,
			   current_program_space->exec_bfd (), sim_argv);
      if (sim_desc == NULL)
	error (_("Unable to create simulator instance for inferior %d."),
	       inf->num);

      /* Check if the sim descriptor is the same as that of another
	 inferior.  */
      for (inferior *other_inf : all_inferiors ())
	{
	  sim_inferior_data *other_sim_data
	    = sim_inferior_data_key.get (other_inf);

	  if (other_sim_data != NULL
	      && other_sim_data->gdbsim_desc == sim_desc)
	    {
	      /* We don't close the descriptor due to the fact that it's
		 shared with some other inferior.  If we were to close it,
		 that might needlessly muck up the other inferior.  Of
		 course, it's possible that the damage has already been
		 done...  Note that it *will* ultimately be closed during
		 cleanup of the other inferior.  */
	      sim_desc = NULL;
	      error (
_("Inferior %d and inferior %d would have identical simulator state.\n"
 "(This simulator does not support the running of more than one inferior.)"),
		     inf->num, other_inf->num);
	    }
	}
    }

  if (sim_data == NULL)
    {
      sim_data = sim_inferior_data_key.emplace (inf, sim_desc);
    }
  else if (sim_desc)
    {
      /* This handles the case where sim_data was allocated prior to
	 needing a sim instance.  */
      sim_data->gdbsim_desc = sim_desc;
    }


  return sim_data;
}

/* Return pointer to per-inferior simulator data using PTID to find the
   inferior in question.  Return NULL when no inferior is found or
   when ptid has a zero or negative pid component.  */

sim_inferior_data *
gdbsim_target::get_inferior_data_by_ptid (ptid_t ptid,
					  int sim_instance_needed)
{
  struct inferior *inf;
  int pid = ptid.pid ();

  if (pid <= 0)
    return NULL;

  inf = find_inferior_pid (this, pid);

  if (inf)
    return get_sim_inferior_data (inf, sim_instance_needed);
  else
    return NULL;
}

/* Free the per-inferior simulator data.  */

sim_inferior_data::~sim_inferior_data ()
{
  if (gdbsim_desc)
    sim_close (gdbsim_desc, 0);
}

static void
dump_mem (const gdb_byte *buf, int len)
{
  gdb_puts ("\t", gdb_stdlog);

  if (len == 8 || len == 4)
    {
      uint32_t l[2];

      memcpy (l, buf, len);
      gdb_printf (gdb_stdlog, "0x%08x", l[0]);
      if (len == 8)
	gdb_printf (gdb_stdlog, " 0x%08x", l[1]);
    }
  else
    {
      int i;

      for (i = 0; i < len; i++)
	gdb_printf (gdb_stdlog, "0x%02x ", buf[i]);
    }

  gdb_puts ("\n", gdb_stdlog);
}

/* Initialize gdb_callback.  */

static void
init_callbacks (void)
{
  if (!callbacks_initialized)
    {
      gdb_callback = default_callback;
      gdb_callback.init (&gdb_callback);
      gdb_callback.write_stdout = gdb_os_write_stdout;
      gdb_callback.flush_stdout = gdb_os_flush_stdout;
      gdb_callback.write_stderr = gdb_os_write_stderr;
      gdb_callback.flush_stderr = gdb_os_flush_stderr;
      gdb_callback.printf_filtered = gdb_os_printf_filtered;
      gdb_callback.vprintf_filtered = gdb_os_vprintf_filtered;
      gdb_callback.evprintf_filtered = gdb_os_evprintf_filtered;
      gdb_callback.error = gdb_os_error;
      gdb_callback.poll_quit = gdb_os_poll_quit;
      gdb_callback.magic = HOST_CALLBACK_MAGIC;
      callbacks_initialized = 1;
    }
}

/* Release callbacks (free resources used by them).  */

static void
end_callbacks (void)
{
  if (callbacks_initialized)
    {
      gdb_callback.shutdown (&gdb_callback);
      callbacks_initialized = 0;
    }
}

/* GDB version of os_write_stdout callback.  */

static int
gdb_os_write_stdout (host_callback *p, const char *buf, int len)
{
  gdb_stdtarg->write (buf, len);
  return len;
}

/* GDB version of os_flush_stdout callback.  */

static void
gdb_os_flush_stdout (host_callback *p)
{
  gdb_stdtarg->flush ();
}

/* GDB version of os_write_stderr callback.  */

static int
gdb_os_write_stderr (host_callback *p, const char *buf, int len)
{
  int i;
  char b[2];

  for (i = 0; i < len; i++)
    {
      b[0] = buf[i];
      b[1] = 0;
      gdb_stdtargerr->puts (b);
    }
  return len;
}

/* GDB version of os_flush_stderr callback.  */

static void
gdb_os_flush_stderr (host_callback *p)
{
  gdb_stdtargerr->flush ();
}

/* GDB version of gdb_printf callback.  */

static void ATTRIBUTE_PRINTF (2, 3)
gdb_os_printf_filtered (host_callback * p, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  gdb_vprintf (gdb_stdout, format, args);
  va_end (args);
}

/* GDB version of error gdb_vprintf.  */

static void ATTRIBUTE_PRINTF (2, 0)
gdb_os_vprintf_filtered (host_callback * p, const char *format, va_list ap)
{
  gdb_vprintf (gdb_stdout, format, ap);
}

/* GDB version of error evprintf_filtered.  */

static void ATTRIBUTE_PRINTF (2, 0)
gdb_os_evprintf_filtered (host_callback * p, const char *format, va_list ap)
{
  gdb_vprintf (gdb_stderr, format, ap);
}

/* GDB version of error callback.  */

static void ATTRIBUTE_PRINTF (2, 3)
gdb_os_error (host_callback * p, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  verror (format, args);
  va_end (args);
}

int
one2one_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  /* Only makes sense to supply raw registers.  */
  gdb_assert (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch));
  return regnum;
}

void
gdbsim_target::fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct inferior *inf = find_inferior_ptid (this, regcache->ptid ());
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (inf, SIM_INSTANCE_NEEDED);

  if (regno == -1)
    {
      for (regno = 0; regno < gdbarch_num_regs (gdbarch); regno++)
	fetch_registers (regcache, regno);
      return;
    }

  switch (gdbarch_register_sim_regno (gdbarch, regno))
    {
    case LEGACY_SIM_REGNO_IGNORE:
      break;
    case SIM_REGNO_DOES_NOT_EXIST:
      {
	/* For moment treat a `does not exist' register the same way
	   as an ``unavailable'' register.  */
	regcache->raw_supply_zeroed (regno);
	break;
      }

    default:
      {
	static int warn_user = 1;
	int regsize = register_size (gdbarch, regno);
	gdb::byte_vector buf (regsize, 0);
	int nr_bytes;

	gdb_assert (regno >= 0 && regno < gdbarch_num_regs (gdbarch));
	nr_bytes = sim_fetch_register (sim_data->gdbsim_desc,
				       gdbarch_register_sim_regno
					 (gdbarch, regno),
				       buf.data (), regsize);
	if (nr_bytes > 0 && nr_bytes != regsize && warn_user)
	  {
	    gdb_printf (gdb_stderr,
			"Size of register %s (%d/%d) "
			"incorrect (%d instead of %d))",
			gdbarch_register_name (gdbarch, regno),
			regno,
			gdbarch_register_sim_regno (gdbarch, regno),
			nr_bytes, regsize);
	    warn_user = 0;
	  }
	/* FIXME: cagney/2002-05-27: Should check `nr_bytes == 0'
	   indicating that GDB and the SIM have different ideas about
	   which registers are fetchable.  */
	/* Else if (nr_bytes < 0): an old simulator, that doesn't
	   think to return the register size.  Just assume all is ok.  */
	regcache->raw_supply (regno, buf.data ());
	if (remote_debug)
	  {
	    gdb_printf (gdb_stdlog,
			"gdbsim_fetch_register: %d", regno);
	    /* FIXME: We could print something more intelligible.  */
	    dump_mem (buf.data (), regsize);
	  }
	break;
      }
    }
}


void
gdbsim_target::store_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct inferior *inf = find_inferior_ptid (this, regcache->ptid ());
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (inf, SIM_INSTANCE_NEEDED);

  if (regno == -1)
    {
      for (regno = 0; regno < gdbarch_num_regs (gdbarch); regno++)
	store_registers (regcache, regno);
      return;
    }
  else if (gdbarch_register_sim_regno (gdbarch, regno) >= 0)
    {
      int regsize = register_size (gdbarch, regno);
      gdb::byte_vector tmp (regsize);
      int nr_bytes;

      regcache->cooked_read (regno, tmp.data ());
      nr_bytes = sim_store_register (sim_data->gdbsim_desc,
				     gdbarch_register_sim_regno
				       (gdbarch, regno),
				     tmp.data (), regsize);

      if (nr_bytes > 0 && nr_bytes != regsize)
	internal_error (_("Register size different to expected"));
      if (nr_bytes < 0)
	internal_error (_("Register %d not updated"), regno);
      if (nr_bytes == 0)
	warning (_("Register %s not updated"),
		 gdbarch_register_name (gdbarch, regno));

      if (remote_debug)
	{
	  gdb_printf (gdb_stdlog, "gdbsim_store_register: %d", regno);
	  /* FIXME: We could print something more intelligible.  */
	  dump_mem (tmp.data (), regsize);
	}
    }
}

/* Kill the running program.  This may involve closing any open files
   and releasing other resources acquired by the simulated program.  */

void
gdbsim_target::kill ()
{
  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_kill\n");

  /* There is no need to `kill' running simulator - the simulator is
     not running.  Mourning it is enough.  */
  target_mourn_inferior (inferior_ptid);
}

/* Load an executable file into the target process.  This is expected to
   not only bring new code into the target process, but also to update
   GDB's symbol tables to match.  */

void
gdbsim_target::load (const char *args, int fromtty)
{
  const char *prog;
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);

  if (args == NULL)
      error_no_arg (_("program to load"));

  gdb_argv argv (args);

  prog = tilde_expand (argv[0]);

  if (argv[1] != NULL)
    error (_("GDB sim does not yet support a load offset."));

  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_load: prog \"%s\"\n", prog);

  /* FIXME: We will print two messages on error.
     Need error to either not print anything if passed NULL or need
     another routine that doesn't take any arguments.  */
  if (sim_load (sim_data->gdbsim_desc, prog, NULL, fromtty) == SIM_RC_FAIL)
    error (_("unable to load program"));

  /* FIXME: If a load command should reset the targets registers then
     a call to sim_create_inferior() should go here.  */

  sim_data->program_loaded = true;
}


/* Start an inferior process and set inferior_ptid to its pid.
   EXEC_FILE is the file to run.
   ARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  Errors reported with error().
   On VxWorks and various standalone systems, we ignore exec_file.  */
/* This is called not only when we first attach, but also when the
   user types "run" after having attached.  */

void
gdbsim_target::create_inferior (const char *exec_file,
				const std::string &allargs,
				char **env, int from_tty)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);
  int len;
  char *arg_buf;
  const char *args = allargs.c_str ();

  if (exec_file == 0 || current_program_space->exec_bfd () == 0)
    warning (_("No executable file specified."));
  if (!sim_data->program_loaded)
    warning (_("No program loaded."));

  if (remote_debug)
    gdb_printf (gdb_stdlog,
		"gdbsim_create_inferior: exec_file \"%s\", args \"%s\"\n",
		(exec_file ? exec_file : "(NULL)"),
		args);

  if (inferior_ptid == sim_data->remote_sim_ptid)
    kill ();
  remove_breakpoints ();
  init_wait_for_inferior ();

  gdb_argv built_argv;
  if (exec_file != NULL)
    {
      len = strlen (exec_file) + 1 + allargs.size () + 1 + /*slop */ 10;
      arg_buf = (char *) alloca (len);
      arg_buf[0] = '\0';
      strcat (arg_buf, exec_file);
      strcat (arg_buf, " ");
      strcat (arg_buf, args);
      built_argv.reset (arg_buf);
    }

  if (sim_create_inferior (sim_data->gdbsim_desc,
			   current_program_space->exec_bfd (),
			   built_argv.get (), env)
      != SIM_RC_OK)
    error (_("Unable to create sim inferior."));

  inferior_appeared (current_inferior (),
		     sim_data->remote_sim_ptid.pid ());
  thread_info *thr = add_thread_silent (this, sim_data->remote_sim_ptid);
  switch_to_thread (thr);

  insert_breakpoints ();	/* Needed to get correct instruction
				   in cache.  */

  clear_proceed_status (0);
}

/* The open routine takes the rest of the parameters from the command,
   and (if successful) pushes a new target onto the stack.
   Targets should supply this routine, if only to provide an error message.  */
/* Called when selecting the simulator.  E.g. (gdb) target sim name.  */

static void
gdbsim_target_open (const char *args, int from_tty)
{
  int len;
  char *arg_buf;
  struct sim_inferior_data *sim_data;
  SIM_DESC gdbsim_desc;

  const char *sysroot = gdb_sysroot.c_str ();
  if (is_target_filename (sysroot))
    sysroot += strlen (TARGET_SYSROOT_PREFIX);

  if (remote_debug)
    gdb_printf (gdb_stdlog,
		"gdbsim_open: args \"%s\"\n", args ? args : "(null)");

  /* Ensure that the sim target is not on the target stack.  This is
     necessary, because if it is on the target stack, the call to
     push_target below will invoke sim_close(), thus freeing various
     state (including a sim instance) that we allocate prior to
     invoking push_target().  We want to delay the push_target()
     operation until after we complete those operations which could
     error out.  */
  if (gdbsim_is_open)
    current_inferior ()->unpush_target (&gdbsim_ops);

  len = (7 + 1			/* gdbsim */
	 + strlen (" -E little")
	 + strlen (" --architecture=xxxxxxxxxx")
	 + strlen (" --sysroot=") + strlen (sysroot) +
	 + (args ? strlen (args) : 0)
	 + 50) /* slack */ ;
  arg_buf = (char *) alloca (len);
  strcpy (arg_buf, "gdbsim");	/* 7 */
  /* Specify the byte order for the target when it is explicitly
     specified by the user (not auto detected).  */
  switch (selected_byte_order ())
    {
    case BFD_ENDIAN_BIG:
      strcat (arg_buf, " -E big");
      break;
    case BFD_ENDIAN_LITTLE:
      strcat (arg_buf, " -E little");
      break;
    case BFD_ENDIAN_UNKNOWN:
      break;
    }
  /* Specify the architecture of the target when it has been
     explicitly specified */
  if (selected_architecture_name () != NULL)
    {
      strcat (arg_buf, " --architecture=");
      strcat (arg_buf, selected_architecture_name ());
    }
  /* Pass along gdb's concept of the sysroot.  */
  strcat (arg_buf, " --sysroot=");
  strcat (arg_buf, sysroot);
  /* finally, any explicit args */
  if (args)
    {
      strcat (arg_buf, " ");	/* 1 */
      strcat (arg_buf, args);
    }

  gdb_argv argv (arg_buf);
  sim_argv = argv.release ();

  init_callbacks ();
  gdbsim_desc = sim_open (SIM_OPEN_DEBUG, &gdb_callback,
			  current_program_space->exec_bfd (), sim_argv);

  if (gdbsim_desc == 0)
    {
      freeargv (sim_argv);
      sim_argv = NULL;
      error (_("unable to create simulator instance"));
    }

  /* Reset the pid numberings for this batch of sim instances.  */
  next_pid = INITIAL_PID;

  /* Allocate the inferior data, but do not allocate a sim instance
     since we've already just done that.  */
  sim_data = get_sim_inferior_data (current_inferior (),
				    SIM_INSTANCE_NOT_NEEDED);

  sim_data->gdbsim_desc = gdbsim_desc;

  current_inferior ()->push_target (&gdbsim_ops);
  gdb_printf ("Connected to the simulator.\n");

  /* There's nothing running after "target sim" or "load"; not until
     "run".  */
  switch_to_no_thread ();

  gdbsim_is_open = true;
}

/* Helper for gdbsim_target::close.  */

void
gdbsim_target::close_one_inferior (inferior *inf)
{
  struct sim_inferior_data *sim_data = sim_inferior_data_key.get (inf);
  if (sim_data != NULL)
    {
      ptid_t ptid = sim_data->remote_sim_ptid;

      sim_inferior_data_key.clear (inf);

      /* Having a ptid allocated and stored in remote_sim_ptid does
	 not mean that a corresponding inferior was ever created.
	 Thus we need to verify the existence of an inferior using the
	 pid in question before setting inferior_ptid via
	 switch_to_thread() or mourning the inferior.  */
      if (find_inferior_ptid (this, ptid) != NULL)
	{
	  switch_to_thread (this, ptid);
	  generic_mourn_inferior ();
	}
    }
}

/* Close out all files and local state before this target loses control.  */

void
gdbsim_target::close ()
{
  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_close\n");

  for (inferior *inf : all_inferiors (this))
    close_one_inferior (inf);

  if (sim_argv != NULL)
    {
      freeargv (sim_argv);
      sim_argv = NULL;
    }

  end_callbacks ();

  gdbsim_is_open = false;
}

/* Takes a program previously attached to and detaches it.
   The program may resume execution (some targets do, some don't) and will
   no longer stop on signals, etc.  We better not have left any breakpoints
   in the program or it'll die when it hits one.  FROM_TTY says whether to be
   verbose or not.  */
/* Terminate the open connection to the remote debugger.
   Use this when you want to detach and do something else with your gdb.  */

void
gdbsim_target::detach (inferior *inf, int from_tty)
{
  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_detach\n");

  inf->unpush_target (this);		/* calls gdbsim_close to do the real work */
  if (from_tty)
    gdb_printf ("Ending simulator %s debugging\n", target_shortname ());
}

/* Resume execution of the target process.  STEP says whether to single-step
   or to run free; SIGGNAL is the signal value (e.g. SIGINT) to be given
   to the target, or zero for no signal.  */

void
gdbsim_target::resume_one_inferior (inferior *inf, bool step,
				    gdb_signal siggnal)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (inf, SIM_INSTANCE_NOT_NEEDED);

  if (sim_data)
    {
      sim_data->resume_siggnal = siggnal;
      sim_data->resume_step = step;

      if (remote_debug)
	gdb_printf (gdb_stdlog,
		    _("gdbsim_resume: pid %d, step %d, signal %d\n"),
		    inf->pid, step, siggnal);
    }
}

void
gdbsim_target::resume (ptid_t ptid, int step, enum gdb_signal siggnal)
{
  struct sim_inferior_data *sim_data
    = get_inferior_data_by_ptid (ptid, SIM_INSTANCE_NOT_NEEDED);

  /* We don't access any sim_data members within this function.
     What's of interest is whether or not the call to
     get_sim_inferior_data_by_ptid(), above, is able to obtain a
     non-NULL pointer.  If it managed to obtain a non-NULL pointer, we
     know we have a single inferior to consider.  If it's NULL, we
     either have multiple inferiors to resume or an error condition.  */

  if (sim_data)
    resume_one_inferior (find_inferior_ptid (this, ptid), step, siggnal);
  else if (ptid == minus_one_ptid)
    {
      for (inferior *inf : all_inferiors (this))
	resume_one_inferior (inf, step, siggnal);
    }
  else
    error (_("The program is not being run."));
}

/* Notify the simulator of an asynchronous request to interrupt.

   The simulator shall ensure that the interrupt request is eventually
   delivered to the simulator.  If the call is made while the
   simulator is not running then the interrupt request is processed when
   the simulator is next resumed.

   For simulators that do not support this operation, just abort.  */

void
gdbsim_target::interrupt ()
{
  for (inferior *inf : all_inferiors ())
    {
      sim_inferior_data *sim_data
	= get_sim_inferior_data (inf, SIM_INSTANCE_NEEDED);

      if (sim_data != nullptr && !sim_stop (sim_data->gdbsim_desc))
	  quit ();
    }
}

/* GDB version of os_poll_quit callback.
   Taken from gdb/util.c - should be in a library.  */

static int
gdb_os_poll_quit (host_callback *p)
{
  if (deprecated_ui_loop_hook != NULL)
    deprecated_ui_loop_hook (0);

  if (check_quit_flag ())	/* gdb's idea of quit */
    return 1;
  return 0;
}

/* Wait for inferior process to do something.  Return pid of child,
   or -1 in case of error; store status through argument pointer STATUS,
   just as `wait' would.  */

static void
gdbsim_cntrl_c (int signo)
{
  gdbsim_ops.interrupt ();
}

ptid_t
gdbsim_target::wait (ptid_t ptid, struct target_waitstatus *status,
		     target_wait_flags options)
{
  struct sim_inferior_data *sim_data;
  static sighandler_t prev_sigint;
  int sigrc = 0;
  enum sim_stop reason = sim_running;

  /* This target isn't able to (yet) resume more than one inferior at a time.
     When ptid is minus_one_ptid, just use the current inferior.  If we're
     given an explicit pid, we'll try to find it and use that instead.  */
  if (ptid == minus_one_ptid)
    sim_data = get_sim_inferior_data (current_inferior (),
				      SIM_INSTANCE_NEEDED);
  else
    {
      sim_data = get_inferior_data_by_ptid (ptid, SIM_INSTANCE_NEEDED);
      if (sim_data == NULL)
	error (_("Unable to wait for pid %d.  Inferior not found."),
	       ptid.pid ());
    }

  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_wait\n");

#if defined (HAVE_SIGACTION) && defined (SA_RESTART)
  {
    struct sigaction sa, osa;
    sa.sa_handler = gdbsim_cntrl_c;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction (SIGINT, &sa, &osa);
    prev_sigint = osa.sa_handler;
  }
#else
  prev_sigint = signal (SIGINT, gdbsim_cntrl_c);
#endif
  sim_resume (sim_data->gdbsim_desc, sim_data->resume_step ? 1 : 0,
	      sim_data->resume_siggnal);

  signal (SIGINT, prev_sigint);
  sim_data->resume_step = false;

  sim_stop_reason (sim_data->gdbsim_desc, &reason, &sigrc);

  switch (reason)
    {
    case sim_exited:
      status->set_exited (sigrc);
      break;
    case sim_stopped:
      switch (sigrc)
	{
	case GDB_SIGNAL_ABRT:
	  quit ();
	  break;
	case GDB_SIGNAL_INT:
	case GDB_SIGNAL_TRAP:
	default:
	  status->set_stopped ((gdb_signal) sigrc);
	  break;
	}
      break;
    case sim_signalled:
      status->set_signalled ((gdb_signal) sigrc);
      break;
    case sim_running:
    case sim_polling:
      /* FIXME: Is this correct?  */
      break;
    }

  return sim_data->remote_sim_ptid;
}

/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On machines
   which store all the registers in one fell swoop, this makes sure
   that registers contains all the registers from the program being
   debugged.  */

void
gdbsim_target::prepare_to_store (struct regcache *regcache)
{
  /* Do nothing, since we can store individual regs.  */
}

/* Helper for gdbsim_xfer_partial that handles memory transfers.
   Arguments are like target_xfer_partial.  */

static enum target_xfer_status
gdbsim_xfer_memory (struct target_ops *target,
		    gdb_byte *readbuf, const gdb_byte *writebuf,
		    ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);
  int l;

  /* If this target doesn't have memory yet, return 0 causing the
     request to be passed to a lower target, hopefully an exec
     file.  */
  if (!target->has_memory ())
    return TARGET_XFER_EOF;

  if (!sim_data->program_loaded)
    error (_("No program loaded."));

  /* Note that we obtained the sim_data pointer above using
     SIM_INSTANCE_NOT_NEEDED.  We do this so that we don't needlessly
     allocate a sim instance prior to loading a program.   If we
     get to this point in the code though, gdbsim_desc should be
     non-NULL.  (Note that a sim instance is needed in order to load
     the program...)  */
  gdb_assert (sim_data->gdbsim_desc != NULL);

  if (remote_debug)
    gdb_printf (gdb_stdlog,
		"gdbsim_xfer_memory: readbuf %s, writebuf %s, "
		"memaddr %s, len %s\n",
		host_address_to_string (readbuf),
		host_address_to_string (writebuf),
		paddress (current_inferior ()->arch (), memaddr),
		pulongest (len));

  if (writebuf)
    {
      if (remote_debug && len > 0)
	dump_mem (writebuf, len);
      l = sim_write (sim_data->gdbsim_desc, memaddr, writebuf, len);
    }
  else
    {
      l = sim_read (sim_data->gdbsim_desc, memaddr, readbuf, len);
      if (remote_debug && len > 0)
	dump_mem (readbuf, len);
    }
  if (l > 0)
    {
      *xfered_len = (ULONGEST) l;
      return TARGET_XFER_OK;
    }
  else if (l == 0)
    return TARGET_XFER_EOF;
  else
    return TARGET_XFER_E_IO;
}

/* Target to_xfer_partial implementation.  */

enum target_xfer_status
gdbsim_target::xfer_partial (enum target_object object,
			     const char *annex, gdb_byte *readbuf,
			     const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
			     ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return gdbsim_xfer_memory (this, readbuf, writebuf, offset, len,
				 xfered_len);

    default:
      return TARGET_XFER_E_IO;
    }
}

void
gdbsim_target::files_info ()
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);
  const char *file = "nothing";

  if (current_program_space->exec_bfd ())
    file = bfd_get_filename (current_program_space->exec_bfd ());

  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_files_info: file \"%s\"\n", file);

  if (current_program_space->exec_bfd ())
    {
      gdb_printf ("\tAttached to %s running program %s\n",
		  target_shortname (), file);
      sim_info (sim_data->gdbsim_desc, 0);
    }
}

/* Clear the simulator's notion of what the break points are.  */

void
gdbsim_target::mourn_inferior ()
{
  if (remote_debug)
    gdb_printf (gdb_stdlog, "gdbsim_mourn_inferior:\n");

  remove_breakpoints ();
  generic_mourn_inferior ();
}

/* Pass the command argument through to the simulator verbatim.  The
   simulator must do any command interpretation work.  */

static void
simulator_command (const char *args, int from_tty)
{
  struct sim_inferior_data *sim_data;

  /* We use inferior_data() instead of get_sim_inferior_data() here in
     order to avoid attaching a sim_inferior_data struct to an
     inferior unnecessarily.  The reason we take such care here is due
     to the fact that this function, simulator_command(), may be called
     even when the sim target is not active.  If we were to use
     get_sim_inferior_data() here, it is possible that this call would
     be made either prior to gdbsim_open() or after gdbsim_close(),
     thus allocating memory that would not be garbage collected until
     the ultimate destruction of the associated inferior.  */

  sim_data  = sim_inferior_data_key.get (current_inferior ());
  if (sim_data == NULL || sim_data->gdbsim_desc == NULL)
    {

      /* PREVIOUSLY: The user may give a command before the simulator
	 is opened. [...] (??? assuming of course one wishes to
	 continue to allow commands to be sent to unopened simulators,
	 which isn't entirely unreasonable).  */

      /* The simulator is a builtin abstraction of a remote target.
	 Consistent with that model, access to the simulator, via sim
	 commands, is restricted to the period when the channel to the
	 simulator is open.  */

      error (_("Not connected to the simulator target"));
    }

  sim_do_command (sim_data->gdbsim_desc, args);

  /* Invalidate the register cache, in case the simulator command does
     something funny.  */
  registers_changed ();
}

static void
sim_command_completer (struct cmd_list_element *ignore,
		       completion_tracker &tracker,
		       const char *text, const char *word)
{
  struct sim_inferior_data *sim_data;

  sim_data = sim_inferior_data_key.get (current_inferior ());
  if (sim_data == NULL || sim_data->gdbsim_desc == NULL)
    return;

  /* sim_complete_command returns a NULL-terminated malloc'ed array of
     malloc'ed strings.  */
  struct sim_completions_deleter
  {
    void operator() (char **ptr) const
    {
      for (size_t i = 0; ptr[i] != NULL; i++)
	xfree (ptr[i]);
      xfree (ptr);
    }
  };

  std::unique_ptr<char *[], sim_completions_deleter> sim_completions
    (sim_complete_command (sim_data->gdbsim_desc, text, word));
  if (sim_completions == NULL)
    return;

  /* Count the elements and add completions from tail to head because
     below we'll swap elements out of the array in case add_completion
     throws and the deleter deletes until it finds a NULL element.  */
  size_t count = 0;
  while (sim_completions[count] != NULL)
    count++;

  for (size_t i = count; i > 0; i--)
    {
      gdb::unique_xmalloc_ptr<char> match (sim_completions[i - 1]);
      sim_completions[i - 1] = NULL;
      tracker.add_completion (std::move (match));
    }
}

/* Check to see if a thread is still alive.  */

bool
gdbsim_target::thread_alive (ptid_t ptid)
{
  struct sim_inferior_data *sim_data
    = get_inferior_data_by_ptid (ptid, SIM_INSTANCE_NOT_NEEDED);

  if (sim_data == NULL)
    return false;

  if (ptid == sim_data->remote_sim_ptid)
    /* The simulators' task is always alive.  */
    return true;

  return false;
}

/* Convert a thread ID to a string.  */

std::string
gdbsim_target::pid_to_str (ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Simulator memory may be accessed after the program has been loaded.  */

bool
gdbsim_target::has_all_memory ()
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  if (!sim_data->program_loaded)
    return false;

  return true;
}

bool
gdbsim_target::has_memory ()
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NOT_NEEDED);

  if (!sim_data->program_loaded)
    return false;

  return true;
}

/* Get memory map from the simulator.  */

std::vector<mem_region>
gdbsim_target::memory_map ()
{
  struct sim_inferior_data *sim_data
    = get_sim_inferior_data (current_inferior (), SIM_INSTANCE_NEEDED);
  std::vector<mem_region> result;
  gdb::unique_xmalloc_ptr<char> text (sim_memory_map (sim_data->gdbsim_desc));

  if (text != nullptr)
    result = parse_memory_map (text.get ());

  return result;
}

void _initialize_remote_sim ();
void
_initialize_remote_sim ()
{
  struct cmd_list_element *c;

  add_target (gdbsim_target_info, gdbsim_target_open);

  c = add_com ("sim", class_obscure, simulator_command,
	       _("Send a command to the simulator."));
  set_cmd_completer (c, sim_command_completer);
}

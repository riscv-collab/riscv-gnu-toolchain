/* MI Command Set.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "arch-utils.h"
#include "target.h"
#include "inferior.h"
#include "infrun.h"
#include "top.h"
#include "ui.h"
#include "gdbthread.h"
#include "mi-cmds.h"
#include "mi-parse.h"
#include "mi-getopt.h"
#include "mi-console.h"
#include "ui-out.h"
#include "mi-out.h"
#include "interps.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "gdbcore.h"
#include "value.h"
#include "regcache.h"
#include "frame.h"
#include "mi-main.h"
#include "mi-interp.h"
#include "language.h"
#include "valprint.h"
#include "osdata.h"
#include "gdbsupport/gdb_splay_tree.h"
#include "tracepoint.h"
#include "ada-lang.h"
#include "linespec.h"
#include "extension.h"
#include "gdbcmd.h"
#include "observable.h"
#include <optional>
#include "gdbsupport/byte-vector.h"

#include <ctype.h>
#include "gdbsupport/run-time-clock.h"
#include <chrono>
#include "progspace-and-thread.h"
#include "gdbsupport/rsp-low.h"
#include <algorithm>
#include <set>
#include <map>

enum
  {
    FROM_TTY = 0
  };

/* Debug flag */
static int mi_debug_p;

/* This is used to pass the current command timestamp down to
   continuation routines.  */
static struct mi_timestamp *current_command_ts;

static int do_timings = 0;

/* Few commands would like to know if options like --thread-group were
   explicitly specified.  This variable keeps the current parsed
   command including all option, and make it possible.  */
static struct mi_parse *current_context;

static void mi_cmd_execute (struct mi_parse *parse);

static void mi_execute_async_cli_command (const char *cli_command,
					  const char *const *argv, int argc);
static bool register_changed_p (int regnum, readonly_detached_regcache *,
			       readonly_detached_regcache *);
static void output_register (frame_info_ptr, int regnum, int format,
			     int skip_unavailable);

/* Controls whether the frontend wants MI in async mode.  */
static bool mi_async = false;

/* The set command writes to this variable.  If the inferior is
   executing, mi_async is *not* updated.  */
static bool mi_async_1 = false;

static void
set_mi_async_command (const char *args, int from_tty,
		      struct cmd_list_element *c)
{
  if (have_live_inferiors ())
    {
      mi_async_1 = mi_async;
      error (_("Cannot change this setting while the inferior is running."));
    }

  mi_async = mi_async_1;
}

static void
show_mi_async_command (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c,
		       const char *value)
{
  gdb_printf (file,
	      _("Whether MI is in asynchronous mode is %s.\n"),
	      value);
}

/* A wrapper for target_can_async_p that takes the MI setting into
   account.  */

int
mi_async_p (void)
{
  return mi_async && target_can_async_p ();
}

/* Command implementations.  FIXME: Is this libgdb?  No.  This is the MI
   layer that calls libgdb.  Any operation used in the below should be
   formalized.  */

static void timestamp (struct mi_timestamp *tv);

static void print_diff (struct ui_file *file, struct mi_timestamp *start,
			struct mi_timestamp *end);

void
mi_cmd_gdb_exit (const char *command, const char *const *argv, int argc)
{
  struct mi_interp *mi = as_mi_interp (current_interpreter ());

  /* If the current interpreter is not an MI interpreter, then just
     don't bother printing anything.  This case can arise from using
     the Python gdb.execute_mi function -- but here the result does
     not matter, as gdb is about to exit anyway.  */
  if (mi != nullptr)
    {
      /* We have to print everything right here because we never return.  */
      if (mi->current_token)
	gdb_puts (mi->current_token, mi->raw_stdout);
      gdb_puts ("^exit\n", mi->raw_stdout);
      mi_out_put (current_uiout, mi->raw_stdout);
      gdb_flush (mi->raw_stdout);
    }
  /* FIXME: The function called is not yet a formal libgdb function.  */
  quit_force (NULL, FROM_TTY);
}

void
mi_cmd_exec_next (const char *command, const char *const *argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-next", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("next", argv, argc);
}

void
mi_cmd_exec_next_instruction (const char *command, const char *const *argv,
			      int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-nexti", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("nexti", argv, argc);
}

void
mi_cmd_exec_step (const char *command, const char *const *argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-step", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("step", argv, argc);
}

void
mi_cmd_exec_step_instruction (const char *command, const char *const *argv,
			      int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-stepi", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("stepi", argv, argc);
}

void
mi_cmd_exec_finish (const char *command, const char *const *argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  if (argc > 0 && strcmp(argv[0], "--reverse") == 0)
    mi_execute_async_cli_command ("reverse-finish", argv + 1, argc - 1);
  else
    mi_execute_async_cli_command ("finish", argv, argc);
}

void
mi_cmd_exec_return (const char *command, const char *const *argv, int argc)
{
  /* This command doesn't really execute the target, it just pops the
     specified number of frames.  */
  if (argc)
    /* Call return_command with from_tty argument equal to 0 so as to
       avoid being queried.  */
    return_command (*argv, 0);
  else
    /* Call return_command with from_tty argument equal to 0 so as to
       avoid being queried.  */
    return_command (NULL, 0);

  /* Because we have called return_command with from_tty = 0, we need
     to print the frame here.  */
  print_stack_frame (get_selected_frame (NULL), 1, LOC_AND_ADDRESS, 1);
}

void
mi_cmd_exec_jump (const char *args, const char *const *argv, int argc)
{
  /* FIXME: Should call a libgdb function, not a cli wrapper.  */
  mi_execute_async_cli_command ("jump", argv, argc);
}

static void
proceed_thread (struct thread_info *thread, int pid)
{
  if (thread->state != THREAD_STOPPED)
    return;

  if (pid != 0 && thread->ptid.pid () != pid)
    return;

  switch_to_thread (thread);
  clear_proceed_status (0);
  proceed ((CORE_ADDR) -1, GDB_SIGNAL_DEFAULT);
}

static int
proceed_thread_callback (struct thread_info *thread, void *arg)
{
  int pid = *(int *)arg;

  proceed_thread (thread, pid);
  return 0;
}

static void
exec_continue (const char *const *argv, int argc)
{
  prepare_execution_command (current_inferior ()->top_target (), mi_async_p ());


  if (non_stop)
    {
      /* In non-stop mode, 'resume' always resumes a single thread.
	 Therefore, to resume all threads of the current inferior, or
	 all threads in all inferiors, we need to iterate over
	 threads.

	 See comment on infcmd.c:proceed_thread_callback for rationale.  */
      if (current_context->all || current_context->thread_group != -1)
	{
	  scoped_restore_current_thread restore_thread;
	  scoped_disable_commit_resumed disable_commit_resumed
	    ("MI continue all threads in non-stop");
	  int pid = 0;

	  if (!current_context->all)
	    {
	      struct inferior *inf
		= find_inferior_id (current_context->thread_group);

	      pid = inf->pid;
	    }

	  iterate_over_threads (proceed_thread_callback, &pid);
	  disable_commit_resumed.reset_and_commit ();
	}
      else
	{
	  continue_1 (0);
	}
    }
  else
    {
      scoped_restore save_multi = make_scoped_restore (&sched_multi);

      if (current_context->all)
	{
	  sched_multi = 1;
	  continue_1 (0);
	}
      else
	{
	  /* In all-stop mode, -exec-continue traditionally resumed
	     either all threads, or one thread, depending on the
	     'scheduler-locking' variable.  Let's continue to do the
	     same.  */
	  continue_1 (1);
	}
    }
}

static void
exec_reverse_continue (const char *const *argv, int argc)
{
  enum exec_direction_kind dir = execution_direction;

  if (dir == EXEC_REVERSE)
    error (_("Already in reverse mode."));

  if (!target_can_execute_reverse ())
    error (_("Target %s does not support this command."), target_shortname ());

  scoped_restore save_exec_dir = make_scoped_restore (&execution_direction,
						      EXEC_REVERSE);
  exec_continue (argv, argc);
}

void
mi_cmd_exec_continue (const char *command, const char *const *argv, int argc)
{
  if (argc > 0 && strcmp (argv[0], "--reverse") == 0)
    exec_reverse_continue (argv + 1, argc - 1);
  else
    exec_continue (argv, argc);
}

static int
interrupt_thread_callback (struct thread_info *thread, void *arg)
{
  int pid = *(int *)arg;

  if (thread->state != THREAD_RUNNING)
    return 0;

  if (thread->ptid.pid () != pid)
    return 0;

  target_stop (thread->ptid);
  return 0;
}

/* Interrupt the execution of the target.  Note how we must play
   around with the token variables, in order to display the current
   token in the result of the interrupt command, and the previous
   execution token when the target finally stops.  See comments in
   mi_cmd_execute.  */

void
mi_cmd_exec_interrupt (const char *command, const char *const *argv, int argc)
{
  /* In all-stop mode, everything stops, so we don't need to try
     anything specific.  */
  if (!non_stop)
    {
      interrupt_target_1 (0);
      return;
    }

  if (current_context->all)
    {
      /* This will interrupt all threads in all inferiors.  */
      interrupt_target_1 (1);
    }
  else if (current_context->thread_group != -1)
    {
      struct inferior *inf = find_inferior_id (current_context->thread_group);

      scoped_disable_commit_resumed disable_commit_resumed
	("interrupting all threads of thread group");

      iterate_over_threads (interrupt_thread_callback, &inf->pid);
    }
  else
    {
      /* Interrupt just the current thread -- either explicitly
	 specified via --thread or whatever was current before
	 MI command was sent.  */
      interrupt_target_1 (0);
    }
}

/* Start the execution of the given inferior.

   START_P indicates whether the program should be stopped when reaching the
   main subprogram (similar to what the CLI "start" command does).  */

static void
run_one_inferior (inferior *inf, bool start_p)
{
  const char *run_cmd = start_p ? "start" : "run";
  struct target_ops *run_target = find_run_target ();
  bool async_p = mi_async && target_can_async_p (run_target);

  if (inf->pid != 0)
    {
      thread_info *tp = any_thread_of_inferior (inf);
      if (tp == NULL)
	error (_("Inferior has no threads."));

      switch_to_thread (tp);
    }
  else
    switch_to_inferior_no_thread (inf);
  mi_execute_cli_command (run_cmd, async_p,
			  async_p ? "&" : NULL);
}

void
mi_cmd_exec_run (const char *command, const char *const *argv, int argc)
{
  int start_p = 0;

  /* Parse the command options.  */
  enum opt
    {
      START_OPT,
    };
  static const struct mi_opt opts[] =
    {
	{"-start", START_OPT, 0},
	{NULL, 0, 0},
    };

  int oind = 0;
  const char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-exec-run", argc, argv, opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case START_OPT:
	  start_p = 1;
	  break;
	}
    }

  /* This command does not accept any argument.  Make sure the user
     did not provide any.  */
  if (oind != argc)
    error (_("Invalid argument: %s"), argv[oind]);

  if (current_context->all)
    {
      scoped_restore_current_pspace_and_thread restore_pspace_thread;

      for (inferior *inf : all_inferiors ())
	run_one_inferior (inf, start_p);
    }
  else
    {
      const char *run_cmd = start_p ? "start" : "run";
      struct target_ops *run_target = find_run_target ();
      bool async_p = mi_async && target_can_async_p (run_target);

      mi_execute_cli_command (run_cmd, async_p,
			      async_p ? "&" : NULL);
    }
}


static int
find_thread_of_process (struct thread_info *ti, void *p)
{
  int pid = *(int *)p;

  if (ti->ptid.pid () == pid && ti->state != THREAD_EXITED)
    return 1;

  return 0;
}

void
mi_cmd_target_detach (const char *command, const char *const *argv, int argc)
{
  if (argc != 0 && argc != 1)
    error (_("Usage: -target-detach [pid | thread-group]"));

  if (argc == 1)
    {
      struct thread_info *tp;
      char *end;
      int pid;

      /* First see if we are dealing with a thread-group id.  */
      if (*argv[0] == 'i')
	{
	  struct inferior *inf;
	  int id = strtoul (argv[0] + 1, &end, 0);

	  if (*end != '\0')
	    error (_("Invalid syntax of thread-group id '%s'"), argv[0]);

	  inf = find_inferior_id (id);
	  if (!inf)
	    error (_("Non-existent thread-group id '%d'"), id);

	  pid = inf->pid;
	}
      else
	{
	  /* We must be dealing with a pid.  */
	  pid = strtol (argv[0], &end, 10);

	  if (*end != '\0')
	    error (_("Invalid identifier '%s'"), argv[0]);
	}

      /* Pick any thread in the desired process.  Current
	 target_detach detaches from the parent of inferior_ptid.  */
      tp = iterate_over_threads (find_thread_of_process, &pid);
      if (!tp)
	error (_("Thread group is empty"));

      switch_to_thread (tp);
    }

  detach_command (NULL, 0);
}

void
mi_cmd_target_flash_erase (const char *command, const char *const *argv,
			   int argc)
{
  flash_erase_command (NULL, 0);
}

void
mi_cmd_thread_select (const char *command, const char *const *argv, int argc)
{
  if (argc != 1)
    error (_("-thread-select: USAGE: threadnum."));

  int num = value_as_long (parse_and_eval (argv[0]));
  thread_info *thr = find_thread_global_id (num);
  if (thr == NULL)
    error (_("Thread ID %d not known."), num);

  thread_select (argv[0], thr);

  print_selected_thread_frame (current_uiout,
			       USER_SELECTED_THREAD | USER_SELECTED_FRAME);
}

void
mi_cmd_thread_list_ids (const char *command, const char *const *argv, int argc)
{
  if (argc != 0)
    error (_("-thread-list-ids: No arguments required."));

  int num = 0;
  int current_thread = -1;

  update_thread_list ();

  {
    ui_out_emit_tuple tuple_emitter (current_uiout, "thread-ids");

    for (thread_info *tp : all_non_exited_threads ())
      {
	if (tp->ptid == inferior_ptid)
	  current_thread = tp->global_num;

	num++;
	current_uiout->field_signed ("thread-id", tp->global_num);
      }
  }

  if (current_thread != -1)
    current_uiout->field_signed ("current-thread-id", current_thread);
  current_uiout->field_signed ("number-of-threads", num);
}

void
mi_cmd_thread_info (const char *command, const char *const *argv, int argc)
{
  if (argc != 0 && argc != 1)
    error (_("Invalid MI command"));

  print_thread_info (current_uiout, argv[0], -1);
}

struct collect_cores_data
{
  int pid;
  std::set<int> cores;
};

static int
collect_cores (struct thread_info *ti, void *xdata)
{
  struct collect_cores_data *data = (struct collect_cores_data *) xdata;

  if (ti->ptid.pid () == data->pid)
    {
      int core = target_core_of_thread (ti->ptid);

      if (core != -1)
	data->cores.insert (core);
    }

  return 0;
}

struct print_one_inferior_data
{
  int recurse;
  const std::set<int> *inferiors;
};

static void
print_one_inferior (struct inferior *inferior, bool recurse,
		    const std::set<int> &ids)
{
  struct ui_out *uiout = current_uiout;

  if (ids.empty () || (ids.find (inferior->pid) != ids.end ()))
    {
      struct collect_cores_data data;
      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      uiout->field_fmt ("id", "i%d", inferior->num);
      uiout->field_string ("type", "process");
      if (inferior->has_exit_code)
	uiout->field_string ("exit-code",
			     int_string (inferior->exit_code, 8, 0, 0, 1));
      if (inferior->pid != 0)
	uiout->field_signed ("pid", inferior->pid);

      if (inferior->pspace->exec_filename != nullptr)
	{
	  uiout->field_string ("executable",
			       inferior->pspace->exec_filename.get ());
	}

      if (inferior->pid != 0)
	{
	  data.pid = inferior->pid;
	  iterate_over_threads (collect_cores, &data);
	}

      if (!data.cores.empty ())
	{
	  ui_out_emit_list list_emitter (uiout, "cores");

	  for (int b : data.cores)
	    uiout->field_signed (NULL, b);
	}

      if (recurse)
	print_thread_info (uiout, NULL, inferior->pid);
    }
}

/* Output a field named 'cores' with a list as the value.  The
   elements of the list are obtained by splitting 'cores' on
   comma.  */

static void
output_cores (struct ui_out *uiout, const char *field_name, const char *xcores)
{
  ui_out_emit_list list_emitter (uiout, field_name);
  auto cores = make_unique_xstrdup (xcores);
  char *p = cores.get ();
  char *saveptr;

  for (p = strtok_r (p, ",", &saveptr); p;  p = strtok_r (NULL, ",", &saveptr))
    uiout->field_string (NULL, p);
}

static void
list_available_thread_groups (const std::set<int> &ids, int recurse)
{
  struct ui_out *uiout = current_uiout;

  /* This keeps a map from integer (pid) to vector of struct osdata_item.
     The vector contains information about all threads for the given pid.  */
  std::map<int, std::vector<osdata_item>> tree;

  /* get_osdata will throw if it cannot return data.  */
  std::unique_ptr<osdata> data = get_osdata ("processes");

  if (recurse)
    {
      std::unique_ptr<osdata> threads = get_osdata ("threads");

      for (const osdata_item &item : threads->items)
	{
	  const std::string *pid = get_osdata_column (item, "pid");
	  int pid_i = strtoul (pid->c_str (), NULL, 0);

	  tree[pid_i].push_back (item);
	}
    }

  ui_out_emit_list list_emitter (uiout, "groups");

  for (const osdata_item &item : data->items)
    {
      const std::string *pid = get_osdata_column (item, "pid");
      const std::string *cmd = get_osdata_column (item, "command");
      const std::string *user = get_osdata_column (item, "user");
      const std::string *cores = get_osdata_column (item, "cores");

      int pid_i = strtoul (pid->c_str (), NULL, 0);

      /* At present, the target will return all available processes
	 and if information about specific ones was required, we filter
	 undesired processes here.  */
      if (!ids.empty () && ids.find (pid_i) == ids.end ())
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      uiout->field_string ("id", *pid);
      uiout->field_string ("type", "process");
      if (cmd)
	uiout->field_string ("description", *cmd);
      if (user)
	uiout->field_string ("user", *user);
      if (cores)
	output_cores (uiout, "cores", cores->c_str ());

      if (recurse)
	{
	  auto n = tree.find (pid_i);
	  if (n != tree.end ())
	    {
	      std::vector<osdata_item> &children = n->second;

	      ui_out_emit_list thread_list_emitter (uiout, "threads");

	      for (const osdata_item &child : children)
		{
		  ui_out_emit_tuple inner_tuple_emitter (uiout, NULL);
		  const std::string *tid = get_osdata_column (child, "tid");
		  const std::string *tcore = get_osdata_column (child, "core");

		  uiout->field_string ("id", *tid);
		  if (tcore)
		    uiout->field_string ("core", *tcore);
		}
	    }
	}
    }
}

void
mi_cmd_list_thread_groups (const char *command, const char *const *argv,
			   int argc)
{
  struct ui_out *uiout = current_uiout;
  int available = 0;
  int recurse = 0;
  std::set<int> ids;

  enum opt
  {
    AVAILABLE_OPT, RECURSE_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"-available", AVAILABLE_OPT, 0},
      {"-recurse", RECURSE_OPT, 1},
      { 0, 0, 0 }
    };

  int oind = 0;
  const char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-list-thread-groups", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case AVAILABLE_OPT:
	  available = 1;
	  break;
	case RECURSE_OPT:
	  if (strcmp (oarg, "0") == 0)
	    ;
	  else if (strcmp (oarg, "1") == 0)
	    recurse = 1;
	  else
	    error (_("only '0' and '1' are valid values "
		     "for the '--recurse' option"));
	  break;
	}
    }

  for (; oind < argc; ++oind)
    {
      char *end;
      int inf;

      if (*(argv[oind]) != 'i')
	error (_("invalid syntax of group id '%s'"), argv[oind]);

      inf = strtoul (argv[oind] + 1, &end, 0);

      if (*end != '\0')
	error (_("invalid syntax of group id '%s'"), argv[oind]);
      ids.insert (inf);
    }

  if (available)
    {
      list_available_thread_groups (ids, recurse);
    }
  else if (ids.size () == 1)
    {
      /* Local thread groups, single id.  */
      int id = *(ids.begin ());
      struct inferior *inf = find_inferior_id (id);

      if (!inf)
	error (_("Non-existent thread group id '%d'"), id);

      print_thread_info (uiout, NULL, inf->pid);
    }
  else
    {
      /* Local thread groups.  Either no explicit ids -- and we
	 print everything, or several explicit ids.  In both cases,
	 we print more than one group, and have to use 'groups'
	 as the top-level element.  */
      ui_out_emit_list list_emitter (uiout, "groups");
      update_thread_list ();
      for (inferior *inf : all_inferiors ())
	print_one_inferior (inf, recurse, ids);
    }
}

void
mi_cmd_data_list_register_names (const char *command, const char *const *argv,
				 int argc)
{
  struct gdbarch *gdbarch;
  struct ui_out *uiout = current_uiout;
  int regnum, numregs;
  int i;

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  gdbarch = get_current_arch ();
  numregs = gdbarch_num_cooked_regs (gdbarch);

  ui_out_emit_list list_emitter (uiout, "register-names");

  if (argc == 0)		/* No args, just do all the regs.  */
    {
      for (regnum = 0;
	   regnum < numregs;
	   regnum++)
	{
	  if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    uiout->field_string (NULL, "");
	  else
	    uiout->field_string (NULL, gdbarch_register_name (gdbarch, regnum));
	}
    }

  /* Else, list of register #s, just do listed regs.  */
  for (i = 0; i < argc; i++)
    {
      regnum = atoi (argv[i]);
      if (regnum < 0 || regnum >= numregs)
	error (_("bad register number"));

      if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	uiout->field_string (NULL, "");
      else
	uiout->field_string (NULL, gdbarch_register_name (gdbarch, regnum));
    }
}

void
mi_cmd_data_list_changed_registers (const char *command,
				    const char *const *argv, int argc)
{
  static std::unique_ptr<readonly_detached_regcache> this_regs;
  struct ui_out *uiout = current_uiout;
  std::unique_ptr<readonly_detached_regcache> prev_regs;
  struct gdbarch *gdbarch;
  int regnum, numregs;
  int i;

  /* The last time we visited this function, the current frame's
     register contents were saved in THIS_REGS.  Move THIS_REGS over
     to PREV_REGS, and refresh THIS_REGS with the now-current register
     contents.  */

  prev_regs = std::move (this_regs);
  this_regs = frame_save_as_regcache (get_selected_frame (NULL));

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  gdbarch = this_regs->arch ();
  numregs = gdbarch_num_cooked_regs (gdbarch);

  ui_out_emit_list list_emitter (uiout, "changed-registers");

  if (argc == 0)
    {
      /* No args, just do all the regs.  */
      for (regnum = 0;
	   regnum < numregs;
	   regnum++)
	{
	  if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    continue;

	  if (register_changed_p (regnum, prev_regs.get (),
				  this_regs.get ()))
	    uiout->field_signed (NULL, regnum);
	}
    }

  /* Else, list of register #s, just do listed regs.  */
  for (i = 0; i < argc; i++)
    {
      regnum = atoi (argv[i]);

      if (regnum >= 0
	  && regnum < numregs
	  && *gdbarch_register_name (gdbarch, regnum) != '\000')
	{
	  if (register_changed_p (regnum, prev_regs.get (),
				  this_regs.get ()))
	    uiout->field_signed (NULL, regnum);
	}
      else
	error (_("bad register number"));
    }
}

static bool
register_changed_p (int regnum, readonly_detached_regcache *prev_regs,
		    readonly_detached_regcache *this_regs)
{
  struct gdbarch *gdbarch = this_regs->arch ();
  struct value *prev_value, *this_value;

  /* First time through or after gdbarch change consider all registers
     as changed.  */
  if (!prev_regs || prev_regs->arch () != gdbarch)
    return true;

  /* Get register contents and compare.  */
  prev_value = prev_regs->cooked_read_value (regnum);
  this_value = this_regs->cooked_read_value (regnum);
  gdb_assert (prev_value != NULL);
  gdb_assert (this_value != NULL);

  auto ret = !prev_value->contents_eq (0, this_value, 0,
				       register_size (gdbarch, regnum));

  release_value (prev_value);
  release_value (this_value);
  return ret;
}

/* Return a list of register number and value pairs.  The valid
   arguments expected are: a letter indicating the format in which to
   display the registers contents.  This can be one of: x
   (hexadecimal), d (decimal), N (natural), t (binary), o (octal), r
   (raw).  After the format argument there can be a sequence of
   numbers, indicating which registers to fetch the content of.  If
   the format is the only argument, a list of all the registers with
   their values is returned.  */

void
mi_cmd_data_list_register_values (const char *command, const char *const *argv,
				  int argc)
{
  struct ui_out *uiout = current_uiout;
  frame_info_ptr frame;
  struct gdbarch *gdbarch;
  int regnum, numregs, format;
  int i;
  int skip_unavailable = 0;
  int oind = 0;
  enum opt
  {
    SKIP_UNAVAILABLE,
  };
  static const struct mi_opt opts[] =
    {
      {"-skip-unavailable", SKIP_UNAVAILABLE, 0},
      { 0, 0, 0 }
    };

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  while (1)
    {
      const char *oarg;
      int opt = mi_getopt ("-data-list-register-values", argc, argv,
			   opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case SKIP_UNAVAILABLE:
	  skip_unavailable = 1;
	  break;
	}
    }

  if (argc - oind < 1)
    error (_("-data-list-register-values: Usage: "
	     "-data-list-register-values [--skip-unavailable] <format>"
	     " [<regnum1>...<regnumN>]"));

  format = (int) argv[oind][0];

  frame = get_selected_frame (NULL);
  gdbarch = get_frame_arch (frame);
  numregs = gdbarch_num_cooked_regs (gdbarch);

  ui_out_emit_list list_emitter (uiout, "register-values");

  if (argc - oind == 1)
    {
      /* No args, beside the format: do all the regs.  */
      for (regnum = 0;
	   regnum < numregs;
	   regnum++)
	{
	  if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    continue;

	  output_register (frame, regnum, format, skip_unavailable);
	}
    }

  /* Else, list of register #s, just do listed regs.  */
  for (i = 1 + oind; i < argc; i++)
    {
      regnum = atoi (argv[i]);

      if (regnum >= 0
	  && regnum < numregs
	  && *gdbarch_register_name (gdbarch, regnum) != '\000')
	output_register (frame, regnum, format, skip_unavailable);
      else
	error (_("bad register number"));
    }
}

/* Output one register REGNUM's contents in the desired FORMAT.  If
   SKIP_UNAVAILABLE is true, skip the register if it is
   unavailable.  */

static void
output_register (frame_info_ptr frame, int regnum, int format,
		 int skip_unavailable)
{
  struct ui_out *uiout = current_uiout;
  value *val
    = value_of_register (regnum, get_next_frame_sentinel_okay (frame));
  struct value_print_options opts;

  if (skip_unavailable && !val->entirely_available ())
    return;

  ui_out_emit_tuple tuple_emitter (uiout, NULL);
  uiout->field_signed ("number", regnum);

  if (format == 'N')
    format = 0;

  if (format == 'r')
    format = 'z';

  string_file stb;

  get_formatted_print_options (&opts, format);
  opts.deref_ref = true;
  common_val_print (val, &stb, 0, &opts, current_language);
  uiout->field_stream ("value", stb);
}

/* Write given values into registers. The registers and values are
   given as pairs.  The corresponding MI command is
   -data-write-register-values <format>
			       [<regnum1> <value1>...<regnumN> <valueN>] */
void
mi_cmd_data_write_register_values (const char *command,
				   const char *const *argv, int argc)
{
  struct gdbarch *gdbarch;
  int numregs, i;

  /* Note that the test for a valid register must include checking the
     gdbarch_register_name because gdbarch_num_regs may be allocated
     for the union of the register sets within a family of related
     processors.  In this case, some entries of gdbarch_register_name
     will change depending upon the particular processor being
     debugged.  */

  regcache *regcache = get_thread_regcache (inferior_thread ());
  gdbarch = regcache->arch ();
  numregs = gdbarch_num_cooked_regs (gdbarch);

  if (argc == 0)
    error (_("-data-write-register-values: Usage: -data-write-register-"
	     "values <format> [<regnum1> <value1>...<regnumN> <valueN>]"));

  if (!target_has_registers ())
    error (_("-data-write-register-values: No registers."));

  if (!(argc - 1))
    error (_("-data-write-register-values: No regs and values specified."));

  if ((argc - 1) % 2)
    error (_("-data-write-register-values: "
	     "Regs and vals are not in pairs."));

  for (i = 1; i < argc; i = i + 2)
    {
      int regnum = atoi (argv[i]);

      if (regnum >= 0 && regnum < numregs
	  && *gdbarch_register_name (gdbarch, regnum) != '\0')
	{
	  LONGEST value;

	  /* Get the value as a number.  */
	  value = parse_and_eval_address (argv[i + 1]);

	  /* Write it down.  */
	  regcache_cooked_write_signed (regcache, regnum, value);
	}
      else
	error (_("bad register number"));
    }
}

/* Evaluate the value of the argument.  The argument is an
   expression. If the expression contains spaces it needs to be
   included in double quotes.  */

void
mi_cmd_data_evaluate_expression (const char *command, const char *const *argv,
				 int argc)
{
  struct value *val;
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  if (argc != 1)
    error (_("-data-evaluate-expression: "
	     "Usage: -data-evaluate-expression expression"));

  expression_up expr = parse_expression (argv[0]);

  val = expr->evaluate ();

  string_file stb;

  /* Print the result of the expression evaluation.  */
  get_user_print_options (&opts);
  opts.deref_ref = false;
  common_val_print (val, &stb, 0, &opts, current_language);

  uiout->field_stream ("value", stb);
}

/* This is the -data-read-memory command.

   ADDR: start address of data to be dumped.
   WORD-FORMAT: a char indicating format for the ``word''.  See
   the ``x'' command.
   WORD-SIZE: size of each ``word''; 1,2,4, or 8 bytes.
   NR_ROW: Number of rows.
   NR_COL: The number of columns (words per row).
   ASCHAR: (OPTIONAL) Append an ascii character dump to each row.  Use
   ASCHAR for unprintable characters.

   Reads SIZE*NR_ROW*NR_COL bytes starting at ADDR from memory and
   displays them.  Returns:

   {addr="...",rowN={wordN="..." ,... [,ascii="..."]}, ...}

   Returns:
   The number of bytes read is SIZE*ROW*COL.  */

void
mi_cmd_data_read_memory (const char *command, const char *const *argv,
			 int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct ui_out *uiout = current_uiout;
  CORE_ADDR addr;
  long total_bytes, nr_cols, nr_rows;
  char word_format;
  struct type *word_type;
  long word_size;
  char word_asize;
  char aschar;
  int nr_bytes;
  long offset = 0;
  int oind = 0;
  const char *oarg;
  enum opt
  {
    OFFSET_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"o", OFFSET_OPT, 1},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-data-read-memory", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OFFSET_OPT:
	  offset = atol (oarg);
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (argc < 5 || argc > 6)
    error (_("-data-read-memory: Usage: "
	     "ADDR WORD-FORMAT WORD-SIZE NR-ROWS NR-COLS [ASCHAR]."));

  /* Extract all the arguments. */

  /* Start address of the memory dump.  */
  addr = parse_and_eval_address (argv[0]) + offset;
  /* The format character to use when displaying a memory word.  See
     the ``x'' command.  */
  word_format = argv[1][0];
  /* The size of the memory word.  */
  word_size = atol (argv[2]);
  switch (word_size)
    {
    case 1:
      word_type = builtin_type (gdbarch)->builtin_int8;
      word_asize = 'b';
      break;
    case 2:
      word_type = builtin_type (gdbarch)->builtin_int16;
      word_asize = 'h';
      break;
    case 4:
      word_type = builtin_type (gdbarch)->builtin_int32;
      word_asize = 'w';
      break;
    case 8:
      word_type = builtin_type (gdbarch)->builtin_int64;
      word_asize = 'g';
      break;
    default:
      word_type = builtin_type (gdbarch)->builtin_int8;
      word_asize = 'b';
    }
  /* The number of rows.  */
  nr_rows = atol (argv[3]);
  if (nr_rows <= 0)
    error (_("-data-read-memory: invalid number of rows."));

  /* Number of bytes per row.  */
  nr_cols = atol (argv[4]);
  if (nr_cols <= 0)
    error (_("-data-read-memory: invalid number of columns."));

  /* The un-printable character when printing ascii.  */
  if (argc == 6)
    aschar = *argv[5];
  else
    aschar = 0;

  /* Create a buffer and read it in.  */
  total_bytes = word_size * nr_rows * nr_cols;

  gdb::byte_vector mbuf (total_bytes);

  nr_bytes = target_read (current_inferior ()->top_target (),
			  TARGET_OBJECT_MEMORY, NULL,
			  mbuf.data (), addr, total_bytes);
  if (nr_bytes <= 0)
    error (_("Unable to read memory."));

  /* Output the header information.  */
  uiout->field_core_addr ("addr", gdbarch, addr);
  uiout->field_signed ("nr-bytes", nr_bytes);
  uiout->field_signed ("total-bytes", total_bytes);
  uiout->field_core_addr ("next-row", gdbarch, addr + word_size * nr_cols);
  uiout->field_core_addr ("prev-row", gdbarch, addr - word_size * nr_cols);
  uiout->field_core_addr ("next-page", gdbarch, addr + total_bytes);
  uiout->field_core_addr ("prev-page", gdbarch, addr - total_bytes);

  /* Build the result as a two dimensional table.  */
  {
    int row;
    int row_byte;

    string_file stream;

    ui_out_emit_list list_emitter (uiout, "memory");
    for (row = 0, row_byte = 0;
	 row < nr_rows;
	 row++, row_byte += nr_cols * word_size)
      {
	int col;
	int col_byte;
	struct value_print_options print_opts;

	ui_out_emit_tuple tuple_emitter (uiout, NULL);
	uiout->field_core_addr ("addr", gdbarch, addr + row_byte);
	/* ui_out_field_core_addr_symbolic (uiout, "saddr", addr +
	   row_byte); */
	{
	  ui_out_emit_list list_data_emitter (uiout, "data");
	  get_formatted_print_options (&print_opts, word_format);
	  for (col = 0, col_byte = row_byte;
	       col < nr_cols;
	       col++, col_byte += word_size)
	    {
	      if (col_byte + word_size > nr_bytes)
		{
		  uiout->field_string (NULL, "N/A");
		}
	      else
		{
		  stream.clear ();
		  print_scalar_formatted (&mbuf[col_byte], word_type,
					  &print_opts, word_asize, &stream);
		  uiout->field_stream (NULL, stream);
		}
	    }
	}

	if (aschar)
	  {
	    int byte;

	    stream.clear ();
	    for (byte = row_byte;
		 byte < row_byte + word_size * nr_cols; byte++)
	      {
		if (byte >= nr_bytes)
		  stream.putc ('X');
		else if (mbuf[byte] < 32 || mbuf[byte] > 126)
		  stream.putc (aschar);
		else
		  stream.putc (mbuf[byte]);
	      }
	    uiout->field_stream ("ascii", stream);
	  }
      }
  }
}

void
mi_cmd_data_read_memory_bytes (const char *command, const char *const *argv,
			       int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct ui_out *uiout = current_uiout;
  CORE_ADDR addr;
  LONGEST length;
  long offset = 0;
  int unit_size = gdbarch_addressable_memory_unit_size (gdbarch);
  int oind = 0;
  const char *oarg;
  enum opt
  {
    OFFSET_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"o", OFFSET_OPT, 1},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-data-read-memory-bytes", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OFFSET_OPT:
	  offset = atol (oarg);
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (argc != 2)
    error (_("Usage: [ -o OFFSET ] ADDR LENGTH."));

  addr = parse_and_eval_address (argv[0]) + offset;
  length = atol (argv[1]);

  std::vector<memory_read_result> result
    = read_memory_robust (current_inferior ()->top_target (), addr, length);

  if (result.size () == 0)
    error (_("Unable to read memory."));

  ui_out_emit_list list_emitter (uiout, "memory");
  for (const memory_read_result &read_result : result)
    {
      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      uiout->field_core_addr ("begin", gdbarch, read_result.begin);
      uiout->field_core_addr ("offset", gdbarch, read_result.begin - addr);
      uiout->field_core_addr ("end", gdbarch, read_result.end);

      std::string data = bin2hex (read_result.data.get (),
				  (read_result.end - read_result.begin)
				  * unit_size);
      uiout->field_string ("contents", data);
    }
}

/* Implementation of the -data-write_memory command.

   COLUMN_OFFSET: optional argument. Must be preceded by '-o'. The
   offset from the beginning of the memory grid row where the cell to
   be written is.
   ADDR: start address of the row in the memory grid where the memory
   cell is, if OFFSET_COLUMN is specified.  Otherwise, the address of
   the location to write to.
   FORMAT: a char indicating format for the ``word''.  See
   the ``x'' command.
   WORD_SIZE: size of each ``word''; 1,2,4, or 8 bytes
   VALUE: value to be written into the memory address.

   Writes VALUE into ADDR + (COLUMN_OFFSET * WORD_SIZE).

   Prints nothing.  */

void
mi_cmd_data_write_memory (const char *command, const char *const *argv,
			  int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR addr;
  long word_size;
  /* FIXME: ezannoni 2000-02-17 LONGEST could possibly not be big
     enough when using a compiler other than GCC.  */
  LONGEST value;
  long offset = 0;
  int oind = 0;
  const char *oarg;
  enum opt
  {
    OFFSET_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"o", OFFSET_OPT, 1},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-data-write-memory", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OFFSET_OPT:
	  offset = atol (oarg);
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (argc != 4)
    error (_("-data-write-memory: Usage: "
	     "[-o COLUMN_OFFSET] ADDR FORMAT WORD-SIZE VALUE."));

  /* Extract all the arguments.  */
  /* Start address of the memory dump.  */
  addr = parse_and_eval_address (argv[0]);
  /* The size of the memory word.  */
  word_size = atol (argv[2]);

  /* Calculate the real address of the write destination.  */
  addr += (offset * word_size);

  /* Get the value as a number.  */
  value = parse_and_eval_address (argv[3]);
  /* Get the value into an array.  */
  gdb::byte_vector buffer (word_size);
  store_signed_integer (buffer.data (), word_size, byte_order, value);
  /* Write it down to memory.  */
  write_memory_with_notification (addr, buffer.data (), word_size);
}

/* Implementation of the -data-write-memory-bytes command.

   ADDR: start address
   DATA: string of bytes to write at that address
   COUNT: number of bytes to be filled (decimal integer).  */

void
mi_cmd_data_write_memory_bytes (const char *command, const char *const *argv,
				int argc)
{
  CORE_ADDR addr;
  const char *cdata;
  size_t len_hex, len_bytes, len_units, i, steps, remaining_units;
  long int count_units;
  int unit_size;

  if (argc != 2 && argc != 3)
    error (_("Usage: ADDR DATA [COUNT]."));

  addr = parse_and_eval_address (argv[0]);
  cdata = argv[1];
  len_hex = strlen (cdata);
  unit_size = gdbarch_addressable_memory_unit_size (get_current_arch ());

  if (len_hex % (unit_size * 2) != 0)
    error (_("Hex-encoded '%s' must represent an integral number of "
	     "addressable memory units."),
	   cdata);

  len_bytes = len_hex / 2;
  len_units = len_bytes / unit_size;

  if (argc == 3)
    count_units = strtoul (argv[2], NULL, 10);
  else
    count_units = len_units;

  gdb::byte_vector databuf (len_bytes);

  for (i = 0; i < len_bytes; ++i)
    {
      int x;
      if (sscanf (cdata + i * 2, "%02x", &x) != 1)
	error (_("Invalid argument"));
      databuf[i] = (gdb_byte) x;
    }

  gdb::byte_vector data;
  if (len_units < count_units)
    {
      /* Pattern is made of less units than count:
	 repeat pattern to fill memory.  */
      data = gdb::byte_vector (count_units * unit_size);

      /* Number of times the pattern is entirely repeated.  */
      steps = count_units / len_units;
      /* Number of remaining addressable memory units.  */
      remaining_units = count_units % len_units;
      for (i = 0; i < steps; i++)
	memcpy (&data[i * len_bytes], &databuf[0], len_bytes);

      if (remaining_units > 0)
	memcpy (&data[steps * len_bytes], &databuf[0],
		remaining_units * unit_size);
    }
  else
    {
      /* Pattern is longer than or equal to count:
	 just copy count addressable memory units.  */
      data = std::move (databuf);
    }

  write_memory_with_notification (addr, data.data (), count_units);
}

void
mi_cmd_enable_timings (const char *command, const char *const *argv, int argc)
{
  if (argc == 0)
    do_timings = 1;
  else if (argc == 1)
    {
      if (strcmp (argv[0], "yes") == 0)
	do_timings = 1;
      else if (strcmp (argv[0], "no") == 0)
	do_timings = 0;
      else
	goto usage_error;
    }
  else
    goto usage_error;

  return;

 usage_error:
  error (_("-enable-timings: Usage: %s {yes|no}"), command);
}

void
mi_cmd_list_features (const char *command, const char *const *argv, int argc)
{
  if (argc == 0)
    {
      struct ui_out *uiout = current_uiout;

      ui_out_emit_list list_emitter (uiout, "features");
      uiout->field_string (NULL, "frozen-varobjs");
      uiout->field_string (NULL, "pending-breakpoints");
      uiout->field_string (NULL, "thread-info");
      uiout->field_string (NULL, "data-read-memory-bytes");
      uiout->field_string (NULL, "breakpoint-notifications");
      uiout->field_string (NULL, "ada-task-info");
      uiout->field_string (NULL, "language-option");
      uiout->field_string (NULL, "info-gdb-mi-command");
      uiout->field_string (NULL, "undefined-command-error-code");
      uiout->field_string (NULL, "exec-run-start-option");
      uiout->field_string (NULL, "data-disassemble-a-option");
      uiout->field_string (NULL, "simple-values-ref-types");

      if (ext_lang_initialized_p (get_ext_lang_defn (EXT_LANG_PYTHON)))
	uiout->field_string (NULL, "python");

      return;
    }

  error (_("-list-features should be passed no arguments"));
}

void
mi_cmd_list_target_features (const char *command, const char *const *argv,
			     int argc)
{
  if (argc == 0)
    {
      struct ui_out *uiout = current_uiout;

      ui_out_emit_list list_emitter (uiout, "features");
      if (mi_async_p ())
	uiout->field_string (NULL, "async");
      if (target_can_execute_reverse ())
	uiout->field_string (NULL, "reverse");
      return;
    }

  error (_("-list-target-features should be passed no arguments"));
}

void
mi_cmd_add_inferior (const char *command, const char *const *argv, int argc)
{
  bool no_connection = false;

  /* Parse the command options.  */
  enum opt
    {
      NO_CONNECTION_OPT,
    };
  static const struct mi_opt opts[] =
    {
	{"-no-connection", NO_CONNECTION_OPT, 0},
	{NULL, 0, 0},
    };

  int oind = 0;
  const char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-add-inferior", argc, argv, opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case NO_CONNECTION_OPT:
	  no_connection = true;
	  break;
	}
    }

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  inferior *inf = add_inferior_with_spaces ();

  switch_to_inferior_and_push_target (inf, no_connection,
				      current_inferior ());

  current_uiout->field_fmt ("inferior", "i%d", inf->num);

  process_stratum_target *proc_target = inf->process_target ();

  if (proc_target != nullptr)
    {
      ui_out_emit_tuple tuple_emitter (current_uiout, "connection");
      current_uiout->field_unsigned ("number", proc_target->connection_number);
      current_uiout->field_string ("name", proc_target->shortname ());
    }
}

void
mi_cmd_remove_inferior (const char *command, const char *const *argv, int argc)
{
  int id;
  struct inferior *inf_to_remove;

  if (argc != 1)
    error (_("-remove-inferior should be passed a single argument"));

  id = mi_parse_thread_group_id (argv[0]);

  inf_to_remove = find_inferior_id (id);
  if (inf_to_remove == NULL)
    error (_("the specified thread group does not exist"));

  if (inf_to_remove->pid != 0)
    error (_("cannot remove an active inferior"));

  if (inf_to_remove == current_inferior ())
    {
      struct thread_info *tp = 0;
      struct inferior *new_inferior = NULL;

      for (inferior *inf : all_inferiors ())
	{
	  if (inf != inf_to_remove)
	    new_inferior = inf;
	}

      if (new_inferior == NULL)
	error (_("Cannot remove last inferior"));

      set_current_inferior (new_inferior);
      if (new_inferior->pid != 0)
	tp = any_thread_of_inferior (new_inferior);
      if (tp != NULL)
	switch_to_thread (tp);
      else
	switch_to_no_thread ();
      set_current_program_space (new_inferior->pspace);
    }

  delete_inferior (inf_to_remove);
}



/* Execute a command within a safe environment.
   Return <0 for error; >=0 for ok.

   args->action will tell mi_execute_command what action
   to perform after the given command has executed (display/suppress
   prompt, display error).  */

static void
captured_mi_execute_command (struct mi_interp *mi, struct ui_out *uiout,
			     struct mi_parse *context)
{
  if (do_timings)
    current_command_ts = context->cmd_start;

  scoped_restore save_token
    = make_scoped_restore (&mi->current_token, context->token.c_str ());

  mi->running_result_record_printed = 0;
  mi->mi_proceeded = 0;
  switch (context->op)
    {
    case MI_COMMAND:
      /* A MI command was read from the input stream.  */
      if (mi_debug_p)
	gdb_printf (gdb_stdlog,
		    " token=`%s' command=`%s' args=`%s'\n",
		    context->token.c_str (), context->command.get (),
		    context->args ());

      mi_cmd_execute (context);

      /* Print the result if there were no errors.

	 Remember that on the way out of executing a command, you have
	 to directly use the mi_interp's uiout, since the command
	 could have reset the interpreter, in which case the current
	 uiout will most likely crash in the mi_out_* routines.  */
      if (!mi->running_result_record_printed)
	{
	  gdb_puts (context->token.c_str (), mi->raw_stdout);
	  /* There's no particularly good reason why target-connect results
	     in not ^done.  Should kill ^connected for MI3.  */
	  gdb_puts (strcmp (context->command.get (), "target-select") == 0
		    ? "^connected" : "^done", mi->raw_stdout);
	  mi_out_put (uiout, mi->raw_stdout);
	  mi_out_rewind (uiout);
	  mi_print_timing_maybe (mi->raw_stdout);
	  gdb_puts ("\n", mi->raw_stdout);
	}
      else
	/* The command does not want anything to be printed.  In that
	   case, the command probably should not have written anything
	   to uiout, but in case it has written something, discard it.  */
	mi_out_rewind (uiout);
      break;

    case CLI_COMMAND:
      {
	const char *argv[2];

	/* A CLI command was read from the input stream.  */
	/* This "feature" will be removed as soon as we have a
	   complete set of mi commands.  */
	/* Echo the command on the console.  */
	gdb_printf (gdb_stdlog, "%s\n", context->command.get ());
	/* Call the "console" interpreter.  */
	argv[0] = INTERP_CONSOLE;
	argv[1] = context->command.get ();
	mi_cmd_interpreter_exec ("-interpreter-exec", argv, 2);

	/* If we changed interpreters, DON'T print out anything.  */
	if (current_interp_named_p (INTERP_MI)
	    || current_interp_named_p (INTERP_MI2)
	    || current_interp_named_p (INTERP_MI3)
	    || current_interp_named_p (INTERP_MI4))
	  {
	    if (!mi->running_result_record_printed)
	      {
		gdb_puts (context->token.c_str (), mi->raw_stdout);
		gdb_puts ("^done", mi->raw_stdout);
		mi_out_put (uiout, mi->raw_stdout);
		mi_out_rewind (uiout);
		mi_print_timing_maybe (mi->raw_stdout);
		gdb_puts ("\n", mi->raw_stdout);
	      }
	    else
	      mi_out_rewind (uiout);
	  }
	break;
      }
    }
}

/* Print a gdb exception to the MI output stream.  */

static void
mi_print_exception (struct mi_interp *mi, const char *token,
		    const struct gdb_exception &exception)
{
  gdb_puts (token, mi->raw_stdout);
  gdb_puts ("^error,msg=\"", mi->raw_stdout);
  if (exception.message == NULL)
    gdb_puts ("unknown error", mi->raw_stdout);
  else
    mi->raw_stdout->putstr (exception.what (), '"');
  gdb_puts ("\"", mi->raw_stdout);

  switch (exception.error)
    {
      case UNDEFINED_COMMAND_ERROR:
	gdb_puts (",code=\"undefined-command\"", mi->raw_stdout);
	break;
    }

  gdb_puts ("\n", mi->raw_stdout);
}

void
mi_execute_command (const char *cmd, int from_tty)
{
  std::string token;
  std::unique_ptr<struct mi_parse> command;

  /* This is to handle EOF (^D). We just quit gdb.  */
  /* FIXME: we should call some API function here.  */
  if (cmd == 0)
    quit_force (NULL, from_tty);

  target_log_command (cmd);

  struct mi_interp *mi
    = gdb::checked_static_cast<mi_interp *> (command_interp ());
  try
    {
      command = std::make_unique<mi_parse> (cmd, &token);
    }
  catch (const gdb_exception &exception)
    {
      mi_print_exception (mi, token.c_str (), exception);
    }

  if (command != NULL)
    {
      command->token = std::move (token);

      if (do_timings)
	{
	  command->cmd_start = new mi_timestamp ();
	  timestamp (command->cmd_start);
	}

      try
	{
	  captured_mi_execute_command (mi, current_uiout, command.get ());
	}
      catch (const gdb_exception &result)
	{
	  /* Like in start_event_loop, enable input and force display
	     of the prompt.  Otherwise, any command that calls
	     async_disable_stdin, and then throws, will leave input
	     disabled.  */
	  async_enable_stdin ();
	  current_ui->prompt_state = PROMPT_NEEDED;

	  /* The command execution failed and error() was called
	     somewhere.  */
	  mi_print_exception (mi, command->token.c_str (), result);
	  mi_out_rewind (current_uiout);

	  /* Throw to a higher level catch for SIGTERM sent to GDB.  */
	  if (result.reason == RETURN_FORCED_QUIT)
	    throw;
	}

      bpstat_do_actions ();

    }
}

/* See mi-cmds.h.  */

void
mi_execute_command (mi_parse *context)
{
  if (context->op != MI_COMMAND)
    error (_("Command is not an MI command"));

  mi_interp *mi = as_mi_interp (current_interpreter ());

  /* The current interpreter may not be MI, for instance when using
     the Python gdb.execute_mi function.  */
  if (mi != nullptr)
    scoped_restore save_token = make_scoped_restore (&mi->current_token,
						     context->token.c_str ());

  scoped_restore save_debug = make_scoped_restore (&mi_debug_p, 0);

  mi_cmd_execute (context);
}

/* Captures the current user selected context state, that is the current
   thread and frame.  Later we can then check if the user selected context
   has changed at all.  */

struct user_selected_context
{
  /* Constructor.  */
  user_selected_context ()
    : m_previous_ptid (inferior_ptid)
  {
    save_selected_frame (&m_previous_frame_id, &m_previous_frame_level);
  }

  /* Return true if the user selected context has changed since this object
     was created.  */
  bool has_changed () const
  {
    /* Did the selected thread change?  */
    if (m_previous_ptid != null_ptid && inferior_ptid != null_ptid
	&& m_previous_ptid != inferior_ptid)
      return true;

    /* Grab details of the currently selected frame, for comparison.  */
    frame_id current_frame_id;
    int current_frame_level;
    save_selected_frame (&current_frame_id, &current_frame_level);

    /* Did the selected frame level change?  */
    if (current_frame_level != m_previous_frame_level)
      return true;

    /* Did the selected frame id change?  If the innermost frame is
       selected then the level will be -1, and the frame-id will be
       null_frame_id.  As comparing null_frame_id with itself always
       reports not-equal, we only do the equality test if we have something
       other than the innermost frame selected.  */
    if (current_frame_level != -1
	&& current_frame_id != m_previous_frame_id)
      return true;

    /* Nothing changed!  */
    return false;
  }
private:
  /* The previously selected thread.  This might be null_ptid if there was
     no previously selected thread.  */
  ptid_t m_previous_ptid;

  /* The previously selected frame.  If the innermost frame is selected, or
     no frame is selected, then the frame_id will be null_frame_id, and the
     level will be -1.  */
  frame_id m_previous_frame_id;
  int m_previous_frame_level;
};

static void
mi_cmd_execute (struct mi_parse *parse)
{
  scoped_value_mark cleanup = prepare_execute_command ();

  if (parse->all && parse->thread_group != -1)
    error (_("Cannot specify --thread-group together with --all"));

  if (parse->all && parse->thread != -1)
    error (_("Cannot specify --thread together with --all"));

  if (parse->thread_group != -1 && parse->thread != -1)
    error (_("Cannot specify --thread together with --thread-group"));

  if (parse->frame != -1 && parse->thread == -1)
    error (_("Cannot specify --frame without --thread"));

  if (parse->thread_group != -1)
    {
      struct inferior *inf = find_inferior_id (parse->thread_group);
      struct thread_info *tp = 0;

      if (!inf)
	error (_("Invalid thread group for the --thread-group option"));

      set_current_inferior (inf);
      /* This behaviour means that if --thread-group option identifies
	 an inferior with multiple threads, then a random one will be
	 picked.  This is not a problem -- frontend should always
	 provide --thread if it wishes to operate on a specific
	 thread.  */
      if (inf->pid != 0)
	tp = any_live_thread_of_inferior (inf);
      if (tp != NULL)
	switch_to_thread (tp);
      else
	switch_to_no_thread ();
      set_current_program_space (inf->pspace);
    }

  user_selected_context current_user_selected_context;

  std::optional<scoped_restore_current_thread> thread_saver;
  if (parse->thread != -1)
    {
      thread_info *tp = find_thread_global_id (parse->thread);

      if (tp == NULL)
	error (_("Invalid thread id: %d"), parse->thread);

      if (tp->state == THREAD_EXITED)
	error (_("Thread id: %d has terminated"), parse->thread);

      if (parse->cmd->preserve_user_selected_context ())
	thread_saver.emplace ();

      switch_to_thread (tp);
    }

  std::optional<scoped_restore_selected_frame> frame_saver;
  if (parse->frame != -1)
    {
      frame_info_ptr fid;
      int frame = parse->frame;

      fid = find_relative_frame (get_current_frame (), &frame);
      if (frame == 0)
	{
	  if (parse->cmd->preserve_user_selected_context ())
	    frame_saver.emplace ();

	  select_frame (fid);
	}
      else
	error (_("Invalid frame id: %d"), frame);
    }

  std::optional<scoped_restore_current_language> lang_saver;
  if (parse->language != language_unknown)
    {
      lang_saver.emplace ();
      set_language (parse->language);
    }

  current_context = parse;

  gdb_assert (parse->cmd != nullptr);

  std::optional<scoped_restore_tmpl<int>> restore_suppress_notification
    = parse->cmd->do_suppress_notification ();

  parse->cmd->invoke (parse);

  if (!parse->cmd->preserve_user_selected_context ()
      && current_user_selected_context.has_changed ())
    interps_notify_user_selected_context_changed
      (USER_SELECTED_THREAD | USER_SELECTED_FRAME);
}

/* See mi-main.h.  */

void
mi_execute_cli_command (const char *cmd, bool args_p, const char *args)
{
  if (cmd != nullptr)
    {
      std::string run (cmd);

      if (args_p)
	run = run + " " + args;
      else
	gdb_assert (args == nullptr);

      if (mi_debug_p)
	gdb_printf (gdb_stdlog, "cli=%s run=%s\n",
		    cmd, run.c_str ());

      execute_command (run.c_str (), 0 /* from_tty */ );
    }
}

void
mi_execute_async_cli_command (const char *cli_command, const char *const *argv,
			      int argc)
{
  std::string run = cli_command;

  if (argc)
    run = run + " " + *argv;
  if (mi_async_p ())
    run += "&";

  execute_command (run.c_str (), 0 /* from_tty */ );
}

void
mi_load_progress (const char *section_name,
		  unsigned long sent_so_far,
		  unsigned long total_section,
		  unsigned long total_sent,
		  unsigned long grand_total)
{
  using namespace std::chrono;
  static steady_clock::time_point last_update;
  static char *previous_sect_name = NULL;
  int new_section;
  struct mi_interp *mi = as_mi_interp (current_interpreter ());

  /* If the current interpreter is not an MI interpreter, then just
     don't bother printing anything.  */
  if (mi == nullptr)
    return;

  /* This function is called through deprecated_show_load_progress
     which means uiout may not be correct.  Fix it for the duration
     of this function.  */

  auto uiout = mi_out_new (current_interpreter ()->name ());
  if (uiout == nullptr)
    return;

  scoped_restore save_uiout
    = make_scoped_restore (&current_uiout, uiout.get ());

  new_section = (previous_sect_name ?
		 strcmp (previous_sect_name, section_name) : 1);
  if (new_section)
    {
      xfree (previous_sect_name);
      previous_sect_name = xstrdup (section_name);

      if (mi->current_token)
	gdb_puts (mi->current_token, mi->raw_stdout);
      gdb_puts ("+download", mi->raw_stdout);
      {
	ui_out_emit_tuple tuple_emitter (uiout.get (), NULL);
	uiout->field_string ("section", section_name);
	uiout->field_signed ("section-size", total_section);
	uiout->field_signed ("total-size", grand_total);
      }
      mi_out_put (uiout.get (), mi->raw_stdout);
      gdb_puts ("\n", mi->raw_stdout);
      gdb_flush (mi->raw_stdout);
    }

  steady_clock::time_point time_now = steady_clock::now ();
  if (time_now - last_update > milliseconds (500))
    {
      last_update = time_now;
      if (mi->current_token)
	gdb_puts (mi->current_token, mi->raw_stdout);
      gdb_puts ("+download", mi->raw_stdout);
      {
	ui_out_emit_tuple tuple_emitter (uiout.get (), NULL);
	uiout->field_string ("section", section_name);
	uiout->field_signed ("section-sent", sent_so_far);
	uiout->field_signed ("section-size", total_section);
	uiout->field_signed ("total-sent", total_sent);
	uiout->field_signed ("total-size", grand_total);
      }
      mi_out_put (uiout.get (), mi->raw_stdout);
      gdb_puts ("\n", mi->raw_stdout);
      gdb_flush (mi->raw_stdout);
    }
}

static void
timestamp (struct mi_timestamp *tv)
{
  using namespace std::chrono;

  tv->wallclock = steady_clock::now ();
  run_time_clock::now (tv->utime, tv->stime);
}

static void
print_diff_now (struct ui_file *file, struct mi_timestamp *start)
{
  struct mi_timestamp now;

  timestamp (&now);
  print_diff (file, start, &now);
}

void
mi_print_timing_maybe (struct ui_file *file)
{
  /* If the command is -enable-timing then do_timings may be true
     whilst current_command_ts is not initialized.  */
  if (do_timings && current_command_ts)
    print_diff_now (file, current_command_ts);
}

static void
print_diff (struct ui_file *file, struct mi_timestamp *start,
	    struct mi_timestamp *end)
{
  using namespace std::chrono;

  duration<double> wallclock = end->wallclock - start->wallclock;
  duration<double> utime = end->utime - start->utime;
  duration<double> stime = end->stime - start->stime;

  gdb_printf
    (file,
     ",time={wallclock=\"%0.5f\",user=\"%0.5f\",system=\"%0.5f\"}",
     wallclock.count (), utime.count (), stime.count ());
}

void
mi_cmd_trace_define_variable (const char *command, const char *const *argv,
			      int argc)
{
  LONGEST initval = 0;
  struct trace_state_variable *tsv;
  const char *name;

  if (argc != 1 && argc != 2)
    error (_("Usage: -trace-define-variable VARIABLE [VALUE]"));

  name = argv[0];
  if (*name++ != '$')
    error (_("Name of trace variable should start with '$'"));

  validate_trace_state_variable_name (name);

  tsv = find_trace_state_variable (name);
  if (!tsv)
    tsv = create_trace_state_variable (name);

  if (argc == 2)
    initval = value_as_long (parse_and_eval (argv[1]));

  tsv->initial_value = initval;
}

void
mi_cmd_trace_list_variables (const char *command, const char *const *argv,
			     int argc)
{
  if (argc != 0)
    error (_("-trace-list-variables: no arguments allowed"));

  tvariables_info_1 ();
}

void
mi_cmd_trace_find (const char *command, const char *const *argv, int argc)
{
  const char *mode;

  if (argc == 0)
    error (_("trace selection mode is required"));

  mode = argv[0];

  if (strcmp (mode, "none") == 0)
    {
      tfind_1 (tfind_number, -1, 0, 0, 0);
      return;
    }

  check_trace_running (current_trace_status ());

  if (strcmp (mode, "frame-number") == 0)
    {
      if (argc != 2)
	error (_("frame number is required"));
      tfind_1 (tfind_number, atoi (argv[1]), 0, 0, 0);
    }
  else if (strcmp (mode, "tracepoint-number") == 0)
    {
      if (argc != 2)
	error (_("tracepoint number is required"));
      tfind_1 (tfind_tp, atoi (argv[1]), 0, 0, 0);
    }
  else if (strcmp (mode, "pc") == 0)
    {
      if (argc != 2)
	error (_("PC is required"));
      tfind_1 (tfind_pc, 0, parse_and_eval_address (argv[1]), 0, 0);
    }
  else if (strcmp (mode, "pc-inside-range") == 0)
    {
      if (argc != 3)
	error (_("Start and end PC are required"));
      tfind_1 (tfind_range, 0, parse_and_eval_address (argv[1]),
	       parse_and_eval_address (argv[2]), 0);
    }
  else if (strcmp (mode, "pc-outside-range") == 0)
    {
      if (argc != 3)
	error (_("Start and end PC are required"));
      tfind_1 (tfind_outside, 0, parse_and_eval_address (argv[1]),
	       parse_and_eval_address (argv[2]), 0);
    }
  else if (strcmp (mode, "line") == 0)
    {
      if (argc != 2)
	error (_("Line is required"));

      std::vector<symtab_and_line> sals
	= decode_line_with_current_source (argv[1],
					   DECODE_LINE_FUNFIRSTLINE);
      const symtab_and_line &sal = sals[0];

      if (sal.symtab == 0)
	error (_("Could not find the specified line"));

      CORE_ADDR start_pc, end_pc;
      if (sal.line > 0 && find_line_pc_range (sal, &start_pc, &end_pc))
	tfind_1 (tfind_range, 0, start_pc, end_pc - 1, 0);
      else
	error (_("Could not find the specified line"));
    }
  else
    error (_("Invalid mode '%s'"), mode);

  if (has_stack_frames () || get_traceframe_number () >= 0)
    print_stack_frame (get_selected_frame (NULL), 1, LOC_AND_ADDRESS, 1);
}

void
mi_cmd_trace_save (const char *command, const char *const *argv, int argc)
{
  int target_saves = 0;
  int generate_ctf = 0;
  const char *filename;
  int oind = 0;
  const char *oarg;

  enum opt
  {
    TARGET_SAVE_OPT, CTF_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"r", TARGET_SAVE_OPT, 0},
      {"ctf", CTF_OPT, 0},
      { 0, 0, 0 }
    };

  while (1)
    {
      int opt = mi_getopt ("-trace-save", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case TARGET_SAVE_OPT:
	  target_saves = 1;
	  break;
	case CTF_OPT:
	  generate_ctf = 1;
	  break;
	}
    }

  if (argc - oind != 1)
    error (_("Exactly one argument required "
	     "(file in which to save trace data)"));

  filename = argv[oind];

  if (generate_ctf)
    trace_save_ctf (filename, target_saves);
  else
    trace_save_tfile (filename, target_saves);
}

void
mi_cmd_trace_start (const char *command, const char *const *argv, int argc)
{
  start_tracing (NULL);
}

void
mi_cmd_trace_status (const char *command, const char *const *argv, int argc)
{
  trace_status_mi (0);
}

void
mi_cmd_trace_stop (const char *command, const char *const *argv, int argc)
{
  stop_tracing (NULL);
  trace_status_mi (1);
}

/* Implement the "-ada-task-info" command.  */

void
mi_cmd_ada_task_info (const char *command, const char *const *argv, int argc)
{
  if (argc != 0 && argc != 1)
    error (_("Invalid MI command"));

  print_ada_task_info (current_uiout, argv[0], current_inferior ());
}

/* Print EXPRESSION according to VALUES.  */

static void
print_variable_or_computed (const char *expression, enum print_values values)
{
  struct value *val;
  struct ui_out *uiout = current_uiout;

  string_file stb;

  expression_up expr = parse_expression (expression);

  if (values == PRINT_SIMPLE_VALUES)
    val = expr->evaluate_type ();
  else
    val = expr->evaluate ();

  std::optional<ui_out_emit_tuple> tuple_emitter;
  if (values != PRINT_NO_VALUES)
    tuple_emitter.emplace (uiout, nullptr);
  uiout->field_string ("name", expression);

  switch (values)
    {
    case PRINT_SIMPLE_VALUES:
      type_print (val->type (), "", &stb, -1);
      uiout->field_stream ("type", stb);
      if (mi_simple_type_p (val->type ()))
	{
	  struct value_print_options opts;

	  get_no_prettyformat_print_options (&opts);
	  opts.deref_ref = true;
	  common_val_print (val, &stb, 0, &opts, current_language);
	  uiout->field_stream ("value", stb);
	}
      break;
    case PRINT_ALL_VALUES:
      {
	struct value_print_options opts;

	get_no_prettyformat_print_options (&opts);
	opts.deref_ref = true;
	common_val_print (val, &stb, 0, &opts, current_language);
	uiout->field_stream ("value", stb);
      }
      break;
    }
}

/* Implement the "-trace-frame-collected" command.  */

void
mi_cmd_trace_frame_collected (const char *command, const char *const *argv,
			      int argc)
{
  struct bp_location *tloc;
  int stepping_frame;
  struct collection_list *clist;
  struct collection_list tracepoint_list, stepping_list;
  struct traceframe_info *tinfo;
  int oind = 0;
  enum print_values var_print_values = PRINT_ALL_VALUES;
  enum print_values comp_print_values = PRINT_ALL_VALUES;
  int registers_format = 'x';
  int memory_contents = 0;
  struct ui_out *uiout = current_uiout;
  enum opt
  {
    VAR_PRINT_VALUES,
    COMP_PRINT_VALUES,
    REGISTERS_FORMAT,
    MEMORY_CONTENTS,
  };
  static const struct mi_opt opts[] =
    {
      {"-var-print-values", VAR_PRINT_VALUES, 1},
      {"-comp-print-values", COMP_PRINT_VALUES, 1},
      {"-registers-format", REGISTERS_FORMAT, 1},
      {"-memory-contents", MEMORY_CONTENTS, 0},
      { 0, 0, 0 }
    };

  while (1)
    {
      const char *oarg;
      int opt = mi_getopt ("-trace-frame-collected", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case VAR_PRINT_VALUES:
	  var_print_values = mi_parse_print_values (oarg);
	  break;
	case COMP_PRINT_VALUES:
	  comp_print_values = mi_parse_print_values (oarg);
	  break;
	case REGISTERS_FORMAT:
	  registers_format = oarg[0];
	  break;
	case MEMORY_CONTENTS:
	  memory_contents = 1;
	  break;
	}
    }

  if (oind != argc)
    error (_("Usage: -trace-frame-collected "
	     "[--var-print-values PRINT_VALUES] "
	     "[--comp-print-values PRINT_VALUES] "
	     "[--registers-format FORMAT]"
	     "[--memory-contents]"));

  /* This throws an error is not inspecting a trace frame.  */
  tloc = get_traceframe_location (&stepping_frame);

  /* This command only makes sense for the current frame, not the
     selected frame.  */
  scoped_restore_current_thread restore_thread;
  select_frame (get_current_frame ());

  encode_actions (tloc, &tracepoint_list, &stepping_list);

  if (stepping_frame)
    clist = &stepping_list;
  else
    clist = &tracepoint_list;

  tinfo = get_traceframe_info ();

  /* Explicitly wholly collected variables.  */
  {
    ui_out_emit_list list_emitter (uiout, "explicit-variables");
    const std::vector<std::string> &wholly_collected
      = clist->wholly_collected ();
    for (size_t i = 0; i < wholly_collected.size (); i++)
      {
	const std::string &str = wholly_collected[i];
	print_variable_or_computed (str.c_str (), var_print_values);
      }
  }

  /* Computed expressions.  */
  {
    ui_out_emit_list list_emitter (uiout, "computed-expressions");

    const std::vector<std::string> &computed = clist->computed ();
    for (size_t i = 0; i < computed.size (); i++)
      {
	const std::string &str = computed[i];
	print_variable_or_computed (str.c_str (), comp_print_values);
      }
  }

  /* Registers.  Given pseudo-registers, and that some architectures
     (like MIPS) actually hide the raw registers, we don't go through
     the trace frame info, but instead consult the register cache for
     register availability.  */
  {
    frame_info_ptr frame;
    struct gdbarch *gdbarch;
    int regnum;
    int numregs;

    ui_out_emit_list list_emitter (uiout, "registers");

    frame = get_selected_frame (NULL);
    gdbarch = get_frame_arch (frame);
    numregs = gdbarch_num_cooked_regs (gdbarch);

    for (regnum = 0; regnum < numregs; regnum++)
      {
	if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	  continue;

	output_register (frame, regnum, registers_format, 1);
      }
  }

  /* Trace state variables.  */
  {
    ui_out_emit_list list_emitter (uiout, "tvars");

    for (int tvar : tinfo->tvars)
      {
	struct trace_state_variable *tsv;

	tsv = find_trace_state_variable_by_number (tvar);

	ui_out_emit_tuple tuple_emitter (uiout, NULL);

	if (tsv != NULL)
	  {
	    uiout->field_fmt ("name", "$%s", tsv->name.c_str ());

	    tsv->value_known = target_get_trace_state_variable_value (tsv->number,
								      &tsv->value);
	    uiout->field_signed ("current", tsv->value);
	  }
	else
	  {
	    uiout->field_skip ("name");
	    uiout->field_skip ("current");
	  }
      }
  }

  /* Memory.  */
  {
    std::vector<mem_range> available_memory;

    traceframe_available_memory (&available_memory, 0, ULONGEST_MAX);

    ui_out_emit_list list_emitter (uiout, "memory");

    for (const mem_range &r : available_memory)
      {
	gdbarch *gdbarch = current_inferior ()->arch ();

	ui_out_emit_tuple tuple_emitter (uiout, NULL);

	uiout->field_core_addr ("address", gdbarch, r.start);
	uiout->field_signed ("length", r.length);

	gdb::byte_vector data (r.length);

	if (memory_contents)
	  {
	    if (target_read_memory (r.start, data.data (), r.length) == 0)
	      {
		std::string data_str = bin2hex (data.data (), r.length);
		uiout->field_string ("contents", data_str);
	      }
	    else
	      uiout->field_skip ("contents");
	  }
      }
  }
}

/* See mi/mi-main.h.  */

void
mi_cmd_fix_multi_location_breakpoint_output (const char *command,
					     const char *const *argv,
					     int argc)
{
  fix_multi_location_breakpoint_output_globally = true;
}

/* See mi/mi-main.h.  */

void
mi_cmd_fix_breakpoint_script_output (const char *command,
				     const char *const *argv, int argc)
{
  fix_breakpoint_script_output_globally = true;
}

/* Implement the "-complete" command.  */

void
mi_cmd_complete (const char *command, const char *const *argv, int argc)
{
  if (argc != 1)
    error (_("Usage: -complete COMMAND"));

  if (max_completions == 0)
    error (_("max-completions is zero, completion is disabled."));

  int quote_char = '\0';
  const char *word;

  completion_result result = complete (argv[0], &word, &quote_char);

  std::string arg_prefix (argv[0], word - argv[0]);

  struct ui_out *uiout = current_uiout;

  if (result.number_matches > 0)
    uiout->field_fmt ("completion", "%s%s",
		      arg_prefix.c_str (),result.match_list[0]);

  {
    ui_out_emit_list completions_emitter (uiout, "matches");

    if (result.number_matches == 1)
      uiout->field_fmt (NULL, "%s%s",
			arg_prefix.c_str (), result.match_list[0]);
    else
      {
	result.sort_match_list ();
	for (size_t i = 0; i < result.number_matches; i++)
	  {
	    uiout->field_fmt (NULL, "%s%s",
			      arg_prefix.c_str (), result.match_list[i + 1]);
	  }
      }
  }
  uiout->field_string ("max_completions_reached",
		       result.number_matches == max_completions ? "1" : "0");
}

/* See mi-main.h.  */
int
mi_parse_thread_group_id (const char *id)
{
  if (*id != 'i')
    error (_("thread group id should start with an 'i'"));

  char *end;
  long num = strtol (id + 1, &end, 10);

  if (*end != '\0' || num > INT_MAX)
    error (_("invalid thread group id '%s'"), id);

  return (int) num;
}

void _initialize_mi_main ();
void
_initialize_mi_main ()
{
  set_show_commands mi_async_cmds
    = add_setshow_boolean_cmd ("mi-async", class_run,
			       &mi_async_1, _("\
Set whether MI asynchronous mode is enabled."), _("\
Show whether MI asynchronous mode is enabled."), _("\
Tells GDB whether MI should be in asynchronous mode."),
			       set_mi_async_command,
			       show_mi_async_command,
			       &setlist, &showlist);

  /* Alias old "target-async" to "mi-async".  */
  cmd_list_element *set_target_async_cmd
    = add_alias_cmd ("target-async", mi_async_cmds.set, class_run, 0, &setlist);
  deprecate_cmd (set_target_async_cmd, "set mi-async");

  cmd_list_element *show_target_async_cmd
    = add_alias_cmd ("target-async", mi_async_cmds.show, class_run, 0,
		     &showlist);
  deprecate_cmd (show_target_async_cmd, "show mi-async");
}

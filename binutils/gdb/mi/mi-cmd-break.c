/* MI Command Set - breakpoint and watchpoint commands.
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
#include "mi-cmds.h"
#include "ui-out.h"
#include "mi-out.h"
#include "breakpoint.h"
#include "mi-getopt.h"
#include "observable.h"
#include "mi-main.h"
#include "mi-cmd-break.h"
#include "language.h"
#include "location.h"
#include "linespec.h"
#include "gdbsupport/gdb_obstack.h"
#include <ctype.h>
#include "tracepoint.h"

enum
  {
    FROM_TTY = 0
  };

/* True if MI breakpoint observers have been registered.  */

static int mi_breakpoint_observers_installed;

/* Control whether breakpoint_notify may act.  */

static int mi_can_breakpoint_notify;

/* Output a single breakpoint, when allowed.  */

static void
breakpoint_notify (struct breakpoint *b)
{
  if (mi_can_breakpoint_notify)
    {
      try
	{
	  print_breakpoint (b);
	}
      catch (const gdb_exception_error &ex)
	{
	  exception_print (gdb_stderr, ex);
	}
    }
}

enum bp_type
  {
    REG_BP,
    HW_BP,
    REGEXP_BP
  };

/* Arrange for all new breakpoints and catchpoints to be reported to
   CURRENT_UIOUT until the destructor of the returned scoped_restore
   is run.

   Note that MI output will be probably invalid if more than one
   breakpoint is created inside one MI command.  */

scoped_restore_tmpl<int>
setup_breakpoint_reporting (void)
{
  if (! mi_breakpoint_observers_installed)
    {
      gdb::observers::breakpoint_created.attach (breakpoint_notify,
						 "mi-cmd-break");
      mi_breakpoint_observers_installed = 1;
    }

  return make_scoped_restore (&mi_can_breakpoint_notify, 1);
}


/* Convert arguments in ARGV to a string suitable for parsing by
   dprintf like "FORMAT",ARG,ARG... and return it.  */

static std::string
mi_argv_to_format (const char *const *argv, int argc)
{
  int i;
  std::string result;

  /* Convert ARGV[0] to format string and save to FORMAT.  */
  result += '\"';
  for (i = 0; argv[0][i] != '\0'; i++)
    {
      switch (argv[0][i])
	{
	case '\\':
	  result += "\\\\";
	  break;
	case '\a':
	  result += "\\a";
	  break;
	case '\b':
	  result += "\\b";
	  break;
	case '\f':
	  result += "\\f";
	  break;
	case '\n':
	  result += "\\n";
	  break;
	case '\r':
	  result += "\\r";
	  break;
	case '\t':
	  result += "\\t";
	  break;
	case '\v':
	  result += "\\v";
	  break;
	case '"':
	  result += "\\\"";
	  break;
	default:
	  if (isprint (argv[0][i]))
	    result += argv[0][i];
	  else
	    {
	      char tmp[5];

	      xsnprintf (tmp, sizeof (tmp), "\\%o",
			 (unsigned char) argv[0][i]);
	      result += tmp;
	    }
	  break;
	}
    }
  result += '\"';

  /* Append other arguments.  */
  for (i = 1; i < argc; i++)
    {
      result += ',';
      result += argv[i];
    }

  return result;
}

/* Insert breakpoint.
   If dprintf is true, it will insert dprintf.
   If not, it will insert other type breakpoint.  */

static void
mi_cmd_break_insert_1 (int dprintf, const char *command,
		       const char *const *argv, int argc)
{
  const char *address = NULL;
  int hardware = 0;
  int temp_p = 0;
  int thread = -1;
  int thread_group = -1;
  int ignore_count = 0;
  const char *condition = NULL;
  int pending = 0;
  int enabled = 1;
  int tracepoint = 0;
  symbol_name_match_type match_type = symbol_name_match_type::WILD;
  enum bptype type_wanted;
  location_spec_up locspec;
  const struct breakpoint_ops *ops;
  int is_explicit = 0;
  std::unique_ptr<explicit_location_spec> explicit_loc
    (new explicit_location_spec ());
  std::string extra_string;
  bool force_condition = false;

  enum opt
    {
      HARDWARE_OPT, TEMP_OPT, CONDITION_OPT,
      IGNORE_COUNT_OPT, THREAD_OPT, THREAD_GROUP_OPT,
      PENDING_OPT, DISABLE_OPT,
      TRACEPOINT_OPT,
      FORCE_CONDITION_OPT,
      QUALIFIED_OPT,
      EXPLICIT_SOURCE_OPT, EXPLICIT_FUNC_OPT,
      EXPLICIT_LABEL_OPT, EXPLICIT_LINE_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"h", HARDWARE_OPT, 0},
    {"t", TEMP_OPT, 0},
    {"c", CONDITION_OPT, 1},
    {"i", IGNORE_COUNT_OPT, 1},
    {"p", THREAD_OPT, 1},
    {"g", THREAD_GROUP_OPT, 1},
    {"f", PENDING_OPT, 0},
    {"d", DISABLE_OPT, 0},
    {"a", TRACEPOINT_OPT, 0},
    {"-force-condition", FORCE_CONDITION_OPT, 0},
    {"-qualified", QUALIFIED_OPT, 0},
    {"-source" , EXPLICIT_SOURCE_OPT, 1},
    {"-function", EXPLICIT_FUNC_OPT, 1},
    {"-label", EXPLICIT_LABEL_OPT, 1},
    {"-line", EXPLICIT_LINE_OPT, 1},
    { 0, 0, 0 }
  };

  /* Parse arguments. It could be -r or -h or -t, <location> or ``--''
     to denote the end of the option list. */
  int oind = 0;
  const char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-break-insert", argc, argv,
			   opts, &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case TEMP_OPT:
	  temp_p = 1;
	  break;
	case HARDWARE_OPT:
	  hardware = 1;
	  break;
	case CONDITION_OPT:
	  condition = oarg;
	  break;
	case IGNORE_COUNT_OPT:
	  ignore_count = atol (oarg);
	  break;
	case THREAD_OPT:
	  thread = atol (oarg);
	  if (!valid_global_thread_id (thread))
	    error (_("Unknown thread %d."), thread);
	  break;
	case THREAD_GROUP_OPT:
	  thread_group = mi_parse_thread_group_id (oarg);
	  break;
	case PENDING_OPT:
	  pending = 1;
	  break;
	case DISABLE_OPT:
	  enabled = 0;
	  break;
	case TRACEPOINT_OPT:
	  tracepoint = 1;
	  break;
	case QUALIFIED_OPT:
	  match_type = symbol_name_match_type::FULL;
	  break;
	case EXPLICIT_SOURCE_OPT:
	  is_explicit = 1;
	  explicit_loc->source_filename = make_unique_xstrdup (oarg);
	  break;
	case EXPLICIT_FUNC_OPT:
	  is_explicit = 1;
	  explicit_loc->function_name = make_unique_xstrdup (oarg);
	  break;
	case EXPLICIT_LABEL_OPT:
	  is_explicit = 1;
	  explicit_loc->label_name = make_unique_xstrdup (oarg);
	  break;
	case EXPLICIT_LINE_OPT:
	  is_explicit = 1;
	  explicit_loc->line_offset = linespec_parse_line_offset (oarg);
	  break;
	case FORCE_CONDITION_OPT:
	  force_condition = true;
	  break;
	}
    }

  if (oind >= argc && !is_explicit)
    error (_("-%s-insert: Missing <location>"),
	   dprintf ? "dprintf" : "break");
  if (dprintf)
    {
      int format_num = is_explicit ? oind : oind + 1;

      if (hardware || tracepoint)
	error (_("-dprintf-insert: does not support -h or -a"));
      if (format_num >= argc)
	error (_("-dprintf-insert: Missing <format>"));

      extra_string = mi_argv_to_format (argv + format_num, argc - format_num);
      address = argv[oind];
    }
  else
    {
      if (is_explicit)
	{
	  if (oind < argc)
	    error (_("-break-insert: Garbage following explicit location"));
	}
      else
	{
	  if (oind < argc - 1)
	    error (_("-break-insert: Garbage following <location>"));
	  address = argv[oind];
	}
    }

  /* Now we have what we need, let's insert the breakpoint!  */
  scoped_restore restore_breakpoint_reporting = setup_breakpoint_reporting ();

  if (tracepoint)
    {
      /* Note that to request a fast tracepoint, the client uses the
	 "hardware" flag, although there's nothing of hardware related to
	 fast tracepoints -- one can implement slow tracepoints with
	 hardware breakpoints, but fast tracepoints are always software.
	 "fast" is a misnomer, actually, "jump" would be more appropriate.
	 A simulator or an emulator could conceivably implement fast
	 regular non-jump based tracepoints.  */
      type_wanted = hardware ? bp_fast_tracepoint : bp_tracepoint;
      ops = breakpoint_ops_for_location_spec (nullptr, true);
    }
  else if (dprintf)
    {
      type_wanted = bp_dprintf;
      ops = &code_breakpoint_ops;
    }
  else
    {
      type_wanted = hardware ? bp_hardware_breakpoint : bp_breakpoint;
      ops = &code_breakpoint_ops;
    }

  if (is_explicit)
    {
      /* Error check -- we must have one of the other
	 parameters specified.  */
      if (explicit_loc->source_filename != NULL
	  && explicit_loc->function_name == NULL
	  && explicit_loc->label_name == NULL
	  && explicit_loc->line_offset.sign == LINE_OFFSET_UNKNOWN)
	error (_("-%s-insert: --source option requires --function, --label,"
		 " or --line"), dprintf ? "dprintf" : "break");

      explicit_loc->func_name_match_type = match_type;

      locspec = std::move (explicit_loc);
    }
  else
    {
      locspec = string_to_location_spec_basic (&address, current_language,
					       match_type);
      if (*address)
	error (_("Garbage '%s' at end of location"), address);
    }

  create_breakpoint (get_current_arch (), locspec.get (), condition,
		     thread, thread_group,
		     extra_string.c_str (),
		     force_condition,
		     0 /* condition and thread are valid.  */,
		     temp_p, type_wanted,
		     ignore_count,
		     pending ? AUTO_BOOLEAN_TRUE : AUTO_BOOLEAN_FALSE,
		     ops, 0, enabled, 0, 0);
}

/* Implements the -break-insert command.
   See the MI manual for the list of possible options.  */

void
mi_cmd_break_insert (const char *command, const char *const *argv, int argc)
{
  mi_cmd_break_insert_1 (0, command, argv, argc);
}

/* Implements the -dprintf-insert command.
   See the MI manual for the list of possible options.  */

void
mi_cmd_dprintf_insert (const char *command, const char *const *argv, int argc)
{
  mi_cmd_break_insert_1 (1, command, argv, argc);
}

/* Implements the -break-condition command.
   See the MI manual for the list of options.  */

void
mi_cmd_break_condition (const char *command, const char *const *argv,
			int argc)
{
  enum option
    {
      FORCE_CONDITION_OPT,
    };

  static const struct mi_opt opts[] =
  {
    {"-force", FORCE_CONDITION_OPT, 0},
    { 0, 0, 0 }
  };

  /* Parse arguments.  */
  int oind = 0;
  const char *oarg;
  bool force_condition = false;

  while (true)
    {
      int opt = mi_getopt ("-break-condition", argc, argv,
			   opts, &oind, &oarg);
      if (opt < 0)
	break;

      switch (opt)
	{
	case FORCE_CONDITION_OPT:
	  force_condition = true;
	  break;
	}
    }

  /* There must be at least one more arg: a bpnum.  */
  if (oind >= argc)
    error (_("-break-condition: Missing the <number> argument"));

  int bpnum = atoi (argv[oind]);

  /* The rest form the condition expr.  */
  std::string expr = "";
  for (int i = oind + 1; i < argc; ++i)
    {
      expr += argv[i];
      if (i + 1 < argc)
	expr += " ";
    }

  set_breakpoint_condition (bpnum, expr.c_str (), 0 /* from_tty */,
			    force_condition);
}

enum wp_type
{
  REG_WP,
  READ_WP,
  ACCESS_WP
};

void
mi_cmd_break_passcount (const char *command, const char *const *argv,
			int argc)
{
  int n;
  int p;
  struct tracepoint *t;

  if (argc != 2)
    error (_("Usage: tracepoint-number passcount"));

  n = atoi (argv[0]);
  p = atoi (argv[1]);
  t = get_tracepoint (n);

  if (t)
    {
      t->pass_count = p;
      notify_breakpoint_modified (t);
    }
  else
    {
      error (_("Could not find tracepoint %d"), n);
    }
}

/* Insert a watchpoint. The type of watchpoint is specified by the
   first argument: 
   -break-watch <expr> --> insert a regular wp.  
   -break-watch -r <expr> --> insert a read watchpoint.
   -break-watch -a <expr> --> insert an access wp.  */

void
mi_cmd_break_watch (const char *command, const char *const *argv, int argc)
{
  const char *expr = NULL;
  enum wp_type type = REG_WP;
  enum opt
    {
      READ_OPT, ACCESS_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"r", READ_OPT, 0},
    {"a", ACCESS_OPT, 0},
    { 0, 0, 0 }
  };

  /* Parse arguments. */
  int oind = 0;
  const char *oarg;

  while (1)
    {
      int opt = mi_getopt ("-break-watch", argc, argv,
			   opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case READ_OPT:
	  type = READ_WP;
	  break;
	case ACCESS_OPT:
	  type = ACCESS_WP;
	  break;
	}
    }
  if (oind >= argc)
    error (_("-break-watch: Missing <expression>"));
  if (oind < argc - 1)
    error (_("-break-watch: Garbage following <expression>"));
  expr = argv[oind];

  /* Now we have what we need, let's insert the watchpoint!  */
  switch (type)
    {
    case REG_WP:
      watch_command_wrapper (expr, FROM_TTY, false);
      break;
    case READ_WP:
      rwatch_command_wrapper (expr, FROM_TTY, false);
      break;
    case ACCESS_WP:
      awatch_command_wrapper (expr, FROM_TTY, false);
      break;
    default:
      error (_("-break-watch: Unknown watchpoint type."));
    }
}

void
mi_cmd_break_commands (const char *command, const char *const *argv, int argc)
{
  counted_command_line break_command;
  char *endptr;
  int bnum;
  struct breakpoint *b;

  if (argc < 1)
    error (_("USAGE: %s <BKPT> [<COMMAND> [<COMMAND>...]]"), command);

  bnum = strtol (argv[0], &endptr, 0);
  if (endptr == argv[0])
    error (_("breakpoint number argument \"%s\" is not a number."),
	   argv[0]);
  else if (*endptr != '\0')
    error (_("junk at the end of breakpoint number argument \"%s\"."),
	   argv[0]);

  b = get_breakpoint (bnum);
  if (b == NULL)
    error (_("breakpoint %d not found."), bnum);

  int count = 1;
  auto reader
    = [&] (std::string &buffer)
      {
	const char *result = nullptr;
	if (count < argc)
	  result = argv[count++];
	return result;
      };

  if (is_tracepoint (b))
    {
      tracepoint *t = gdb::checked_static_cast<tracepoint *> (b);
      break_command = read_command_lines_1 (reader, 1,
					    [=] (const char *line)
					    {
					      validate_actionline (line, t);
					    });
    }
  else
    break_command = read_command_lines_1 (reader, 1, 0);

  breakpoint_set_commands (b, std::move (break_command));
}


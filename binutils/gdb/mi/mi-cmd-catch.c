/* MI Command Set - catch commands.
   Copyright (C) 2012-2024 Free Software Foundation, Inc.

   Contributed by Intel Corporation.

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
#include "breakpoint.h"
#include "ada-lang.h"
#include "mi-cmds.h"
#include "mi-getopt.h"
#include "mi-cmd-break.h"

/* Handler for the -catch-assert command.  */

void
mi_cmd_catch_assert (const char *cmd, const char *const *argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch();
  std::string condition;
  int enabled = 1;
  int temp = 0;

  int oind = 0;
  const char *oarg;

  enum opt
    {
      OPT_CONDITION, OPT_DISABLED, OPT_TEMP,
    };
  static const struct mi_opt opts[] =
    {
      { "c", OPT_CONDITION, 1},
      { "d", OPT_DISABLED, 0 },
      { "t", OPT_TEMP, 0 },
      { 0, 0, 0 }
    };

  for (;;)
    {
      int opt = mi_getopt ("-catch-assert", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;

      switch ((enum opt) opt)
	{
	case OPT_CONDITION:
	  condition.assign (oarg);
	  break;
	case OPT_DISABLED:
	  enabled = 0;
	  break;
	case OPT_TEMP:
	  temp = 1;
	  break;
	}
    }

  /* This command does not accept any argument.  Make sure the user
     did not provide any.  */
  if (oind != argc)
    error (_("Invalid argument: %s"), argv[oind]);

  scoped_restore restore_breakpoint_reporting = setup_breakpoint_reporting ();
  create_ada_exception_catchpoint (gdbarch, ada_catch_assert, std::string (),
				   condition, temp, enabled, 0);
}

/* Handler for the -catch-exception command.  */

void
mi_cmd_catch_exception (const char *cmd, const char *const *argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch();
  std::string condition;
  int enabled = 1;
  std::string exception_name;
  int temp = 0;
  enum ada_exception_catchpoint_kind ex_kind = ada_catch_exception;

  int oind = 0;
  const char *oarg;

  enum opt
    {
      OPT_CONDITION, OPT_DISABLED, OPT_EXCEPTION_NAME, OPT_TEMP,
      OPT_UNHANDLED,
    };
  static const struct mi_opt opts[] =
    {
      { "c", OPT_CONDITION, 1},
      { "d", OPT_DISABLED, 0 },
      { "e", OPT_EXCEPTION_NAME, 1 },
      { "t", OPT_TEMP, 0 },
      { "u", OPT_UNHANDLED, 0},
      { 0, 0, 0 }
    };

  for (;;)
    {
      int opt = mi_getopt ("-catch-exception", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;

      switch ((enum opt) opt)
	{
	case OPT_CONDITION:
	  condition.assign (oarg);
	  break;
	case OPT_DISABLED:
	  enabled = 0;
	  break;
	case OPT_EXCEPTION_NAME:
	  exception_name = oarg;
	  break;
	case OPT_TEMP:
	  temp = 1;
	  break;
	case OPT_UNHANDLED:
	  ex_kind = ada_catch_exception_unhandled;
	  break;
	}
    }

  /* This command does not accept any argument.  Make sure the user
     did not provide any.  */
  if (oind != argc)
    error (_("Invalid argument: %s"), argv[oind]);

  /* Specifying an exception name does not make sense when requesting
     an unhandled exception breakpoint.  */
  if (ex_kind == ada_catch_exception_unhandled && !exception_name.empty ())
    error (_("\"-e\" and \"-u\" are mutually exclusive"));

  scoped_restore restore_breakpoint_reporting = setup_breakpoint_reporting ();
  create_ada_exception_catchpoint (gdbarch, ex_kind,
				   std::move (exception_name),
				   condition, temp, enabled, 0);
}

/* Handler for the -catch-handlers command.  */

void
mi_cmd_catch_handlers (const char *cmd, const char *const *argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  std::string condition;
  int enabled = 1;
  std::string exception_name;
  int temp = 0;

  int oind = 0;
  const char *oarg;

  enum opt
    {
      OPT_CONDITION, OPT_DISABLED, OPT_EXCEPTION_NAME, OPT_TEMP
    };
  static const struct mi_opt opts[] =
    {
      { "c", OPT_CONDITION, 1},
      { "d", OPT_DISABLED, 0 },
      { "e", OPT_EXCEPTION_NAME, 1 },
      { "t", OPT_TEMP, 0 },
      { 0, 0, 0 }
    };

  for (;;)
    {
      int opt = mi_getopt ("-catch-handlers", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;

      switch ((enum opt) opt)
	{
	case OPT_CONDITION:
	  condition.assign (oarg);
	  break;
	case OPT_DISABLED:
	  enabled = 0;
	  break;
	case OPT_EXCEPTION_NAME:
	  exception_name = oarg;
	  break;
	case OPT_TEMP:
	  temp = 1;
	  break;
	}
    }

  /* This command does not accept any argument.  Make sure the user
     did not provide any.  */
  if (oind != argc)
    error (_("Invalid argument: %s"), argv[oind]);

  scoped_restore restore_breakpoint_reporting
    = setup_breakpoint_reporting ();
  create_ada_exception_catchpoint (gdbarch, ada_catch_handlers,
				   std::move (exception_name),
				   condition, temp, enabled, 0);
}

/* Common path for the -catch-load and -catch-unload.  */

static void
mi_catch_load_unload (int load, const char *const *argv, int argc)
{
  const char *actual_cmd = load ? "-catch-load" : "-catch-unload";
  int temp = 0;
  int enabled = 1;
  int oind = 0;
  const char *oarg;
  enum opt
    {
      OPT_TEMP,
      OPT_DISABLED,
    };
  static const struct mi_opt opts[] =
    {
      { "t", OPT_TEMP, 0 },
      { "d", OPT_DISABLED, 0 },
      { 0, 0, 0 }
    };

  for (;;)
    {
      int opt = mi_getopt (actual_cmd, argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;

      switch ((enum opt) opt)
	{
	case OPT_TEMP:
	  temp = 1;
	  break;
	case OPT_DISABLED:
	  enabled = 0;
	  break;
	}
    }

  if (oind >= argc)
    error (_("-catch-load/unload: Missing <library name>"));
  if (oind < argc -1)
    error (_("-catch-load/unload: Garbage following the <library name>"));

  scoped_restore restore_breakpoint_reporting = setup_breakpoint_reporting ();
  add_solib_catchpoint (argv[oind], load, temp, enabled);
}

/* Handler for the -catch-load.  */

void
mi_cmd_catch_load (const char *cmd, const char *const *argv, int argc)
{
  mi_catch_load_unload (1, argv, argc);
}


/* Handler for the -catch-unload.  */

void
mi_cmd_catch_unload (const char *cmd, const char *const *argv, int argc)
{
  mi_catch_load_unload (0, argv, argc);
}

/* Core handler for -catch-throw, -catch-rethrow, and -catch-catch
   commands.  The argument handling for all of these is identical, we just
   pass KIND through to GDB's core to select the correct event type.  */

static void
mi_cmd_catch_exception_event (enum exception_event_kind kind,
			      const char *cmd, const char *const *argv,
			      int argc)
{
  const char *regex = NULL;
  bool temp = false;
  int oind = 0;
  const char *oarg;
  enum opt
    {
      OPT_TEMP,
      OPT_REGEX,
    };
  static const struct mi_opt opts[] =
    {
      { "t", OPT_TEMP, 0 },
      { "r", OPT_REGEX, 1 },
      { 0, 0, 0 }
    };

  for (;;)
    {
      int opt = mi_getopt (cmd, argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;

      switch ((enum opt) opt)
	{
	case OPT_TEMP:
	  temp = true;
	  break;
	case OPT_REGEX:
	  regex = oarg;
	  break;
	}
    }

  scoped_restore restore_breakpoint_reporting = setup_breakpoint_reporting ();
  catch_exception_event (kind, regex, temp, 0 /* from_tty */);
}

/* Handler for -catch-throw.  */

void
mi_cmd_catch_throw (const char *cmd, const char *const *argv, int argc)
{
  mi_cmd_catch_exception_event (EX_EVENT_THROW, cmd, argv, argc);
}

/* Handler for -catch-rethrow.  */

void
mi_cmd_catch_rethrow (const char *cmd, const char *const *argv, int argc)
{
  mi_cmd_catch_exception_event (EX_EVENT_RETHROW, cmd, argv, argc);
}

/* Handler for -catch-catch.  */

void
mi_cmd_catch_catch (const char *cmd, const char *const *argv, int argc)
{
  mi_cmd_catch_exception_event (EX_EVENT_CATCH, cmd, argv, argc);
}


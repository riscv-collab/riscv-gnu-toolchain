/* MI Command Set - environment commands.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by Red Hat Inc.

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
#include "inferior.h"
#include "value.h"
#include "mi-out.h"
#include "mi-cmds.h"
#include "mi-getopt.h"
#include "symtab.h"
#include "target.h"
#include "gdbsupport/environ.h"
#include "command.h"
#include "ui-out.h"
#include "top.h"
#include <sys/stat.h>
#include "source.h"

static const char path_var_name[] = "PATH";
static char *orig_path = NULL;

/* The following is copied from mi-main.c so for m1 and below we can
   perform old behavior and use cli commands.  If ARGS is non-null,
   append it to the CMD.  */

static void
env_execute_cli_command (const char *cmd, const char *args)
{
  if (cmd != 0)
    {
      gdb::unique_xmalloc_ptr<char> run;

      if (args != NULL)
	run = xstrprintf ("%s %s", cmd, args);
      else
	run.reset (xstrdup (cmd));
      execute_command ( /*ui */ run.get (), 0 /*from_tty */ );
    }
}

/* Print working directory.  */

void
mi_cmd_env_pwd (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;

  if (argc > 0)
    error (_("-environment-pwd: No arguments allowed"));
	  
  gdb::unique_xmalloc_ptr<char> cwd (getcwd (NULL, 0));
  if (cwd == NULL)
    error (_("-environment-pwd: error finding name of working directory: %s"),
	   safe_strerror (errno));

  uiout->field_string ("cwd", cwd.get ());
}

/* Change working directory.  */

void
mi_cmd_env_cd (const char *command, const char *const *argv, int argc)
{
  if (argc == 0 || argc > 1)
    error (_("-environment-cd: Usage DIRECTORY"));
	  
  env_execute_cli_command ("cd", argv[0]);
}

static void
env_mod_path (const char *dirname, std::string &which_path)
{
  if (dirname == 0 || dirname[0] == '\0')
    return;

  /* Call add_path with last arg 0 to indicate not to parse for 
     separator characters.  */
  add_path (dirname, which_path, 0);
}

/* Add one or more directories to start of executable search path.  */

void
mi_cmd_env_path (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  const char *env;
  int reset = 0;
  int oind = 0;
  int i;
  const char *oarg;
  enum opt
    {
      RESET_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"r", RESET_OPT, 0},
    { 0, 0, 0 }
  };

  dont_repeat ();

  while (1)
    {
      int opt = mi_getopt ("-environment-path", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case RESET_OPT:
	  reset = 1;
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  std::string exec_path;
  if (reset)
    {
      /* Reset implies resetting to original path first.  */
      exec_path = orig_path;
    }
  else
    {
      /* Otherwise, get current path to modify.  */
      env = current_inferior ()->environment.get (path_var_name);

      /* Can be null if path is not set.  */
      if (!env)
	env = "";

      exec_path = env;
    }

  for (i = argc - 1; i >= 0; --i)
    env_mod_path (argv[i], exec_path);

  current_inferior ()->environment.set (path_var_name, exec_path.c_str ());
  env = current_inferior ()->environment.get (path_var_name);
  uiout->field_string ("path", env);
}

/* Add zero or more directories to the front of the source path.  */

void
mi_cmd_env_dir (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  int i;
  int oind = 0;
  int reset = 0;
  const char *oarg;
  enum opt
    {
      RESET_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"r", RESET_OPT, 0},
    { 0, 0, 0 }
  };

  dont_repeat ();

  while (1)
    {
      int opt = mi_getopt ("-environment-directory", argc, argv, opts,
			   &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case RESET_OPT:
	  reset = 1;
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  if (reset)
    {
      /* Reset means setting to default path first.  */
      init_source_path ();
    }

  for (i = argc - 1; i >= 0; --i)
    env_mod_path (argv[i], source_path);

  uiout->field_string ("source-path", source_path);
  forget_cached_source_info ();
}

/* Set the inferior terminal device name.  */

void
mi_cmd_inferior_tty_set (const char *command, const char *const *argv,
			 int argc)
{
  if (argc > 0)
    current_inferior ()->set_tty (argv[0]);
  else
    current_inferior ()->set_tty ("");
}

/* Print the inferior terminal device name.  */

void
mi_cmd_inferior_tty_show (const char *command, const char *const *argv,
			  int argc)
{
  if ( !mi_valid_noargs ("-inferior-tty-show", argc, argv))
    error (_("-inferior-tty-show: Usage: No args"));

  const std::string &inferior_tty = current_inferior ()->tty ();
  if (!inferior_tty.empty ())
    current_uiout->field_string ("inferior_tty_terminal", inferior_tty);
}

void _initialize_mi_cmd_env ();
void 
_initialize_mi_cmd_env ()
{
  const char *env;

  /* We want original execution path to reset to, if desired later.
     At this point, current inferior is not created, so cannot use
     current_inferior ()->environment.  We use getenv here because it
     is not necessary to create a whole new gdb_environ just for one
     variable.  */
  env = getenv (path_var_name);

  /* Can be null if path is not set.  */
  if (!env)
    env = "";
  orig_path = xstrdup (env);
}

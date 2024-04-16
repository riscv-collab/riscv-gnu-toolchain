/* General GDB/Guile code.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "breakpoint.h"
#include "cli/cli-cmds.h"
#include "cli/cli-script.h"
#include "cli/cli-utils.h"
#include "command.h"
#include "gdbcmd.h"
#include "top.h"
#include "ui.h"
#include "extension-priv.h"
#include "utils.h"
#include "gdbsupport/version.h"
#ifdef HAVE_GUILE
#include "guile.h"
#include "guile-internal.h"
#endif
#include <signal.h>
#include "gdbsupport/block-signals.h"

/* The Guile version we're using.
   We *could* use the macros in libguile/version.h but that would preclude
   handling the user switching in a different version with, e.g.,
   LD_LIBRARY_PATH (using a different version than what gdb was compiled with
   is not something to be done lightly, but can be useful).  */
int gdbscm_guile_major_version;
int gdbscm_guile_minor_version;
int gdbscm_guile_micro_version;

#ifdef HAVE_GUILE
/* The guile subdirectory within gdb's data-directory.  */
static const char *guile_datadir;
#endif

/* Declared constants and enum for guile exception printing.  */
const char gdbscm_print_excp_none[] = "none";
const char gdbscm_print_excp_full[] = "full";
const char gdbscm_print_excp_message[] = "message";

/* "set guile print-stack" choices.  */
static const char *const guile_print_excp_enums[] =
  {
    gdbscm_print_excp_none,
    gdbscm_print_excp_full,
    gdbscm_print_excp_message,
    NULL
  };

/* The exception printing variable.  'full' if we want to print the
   error message and stack, 'none' if we want to print nothing, and
   'message' if we only want to print the error message.  'message' is
   the default.  */
const char *gdbscm_print_excp = gdbscm_print_excp_message;


#ifdef HAVE_GUILE

static void gdbscm_initialize (const struct extension_language_defn *);
static int gdbscm_initialized (const struct extension_language_defn *);
static void gdbscm_eval_from_control_command
  (const struct extension_language_defn *, struct command_line *);
static script_sourcer_func gdbscm_source_script;
static void gdbscm_set_backtrace (int enable);

int gdb_scheme_initialized;

/* Symbol for setting documentation strings.  */
SCM gdbscm_documentation_symbol;

/* Keywords used by various functions.  */
static SCM from_tty_keyword;
static SCM to_string_keyword;

/* The name of the various modules (without the surrounding parens).  */
const char gdbscm_module_name[] = "gdb";
const char gdbscm_init_module_name[] = "gdb";

/* The name of the bootstrap file.  */
static const char boot_scm_filename[] = "boot.scm";

/* The interface between gdb proper and loading of python scripts.  */

static const struct extension_language_script_ops guile_extension_script_ops =
{
  gdbscm_source_script,
  gdbscm_source_objfile_script,
  gdbscm_execute_objfile_script,
  gdbscm_auto_load_enabled
};

/* The interface between gdb proper and guile scripting.  */

static const struct extension_language_ops guile_extension_ops =
{
  gdbscm_initialize,
  gdbscm_initialized,

  gdbscm_eval_from_control_command,

  NULL, /* gdbscm_start_type_printers, */
  NULL, /* gdbscm_apply_type_printers, */
  NULL, /* gdbscm_free_type_printers, */

  gdbscm_apply_val_pretty_printer,

  NULL, /* gdbscm_apply_frame_filter, */

  gdbscm_preserve_values,

  gdbscm_breakpoint_has_cond,
  gdbscm_breakpoint_cond_says_stop,

  NULL, /* gdbscm_set_quit_flag, */
  NULL, /* gdbscm_check_quit_flag, */
  NULL, /* gdbscm_before_prompt, */
  NULL, /* gdbscm_get_matching_xmethod_workers */
  NULL, /* gdbscm_colorize */
  NULL, /* gdbscm_print_insn */
};
#endif

/* The main struct describing GDB's interface to the Guile
   extension language.  */
extern const struct extension_language_defn extension_language_guile =
{
  EXT_LANG_GUILE,
  "guile",
  "Guile",

  ".scm",
  "-gdb.scm",

  guile_control,

#ifdef HAVE_GUILE
  &guile_extension_script_ops,
  &guile_extension_ops
#else
  NULL,
  NULL
#endif
};

#ifdef HAVE_GUILE
/* Implementation of the gdb "guile-repl" command.  */

static void
guile_repl_command (const char *arg, int from_tty)
{
  scoped_restore restore_async = make_scoped_restore (&current_ui->async, 0);

  arg = skip_spaces (arg);

  /* This explicitly rejects any arguments for now.
     "It is easier to relax a restriction than impose one after the fact."
     We would *like* to be able to pass arguments to the interactive shell
     but that's not what python-interactive does.  Until there is time to
     sort it out, we forbid arguments.  */

  if (arg && *arg)
    error (_("guile-repl currently does not take any arguments."));
  else
    {
      dont_repeat ();
      gdbscm_enter_repl ();
    }
}

/* Implementation of the gdb "guile" command.
   Note: Contrary to the Python version this displays the result.
   Have to see which is better.

   TODO: Add the result to Guile's history?  */

static void
guile_command (const char *arg, int from_tty)
{
  scoped_restore restore_async = make_scoped_restore (&current_ui->async, 0);

  arg = skip_spaces (arg);

  if (arg && *arg)
    {
      gdb::unique_xmalloc_ptr<char> msg = gdbscm_safe_eval_string (arg, 1);

      if (msg != NULL)
	error ("%s", msg.get ());
    }
  else
    {
      counted_command_line l = get_command_line (guile_control, "");

      execute_control_command_untraced (l.get ());
    }
}

/* Given a command_line, return a command string suitable for passing
   to Guile.  Lines in the string are separated by newlines.  The return
   value is allocated using xmalloc and the caller is responsible for
   freeing it.  */

static char *
compute_scheme_string (struct command_line *l)
{
  struct command_line *iter;
  char *script = NULL;
  int size = 0;
  int here;

  for (iter = l; iter; iter = iter->next)
    size += strlen (iter->line) + 1;

  script = (char *) xmalloc (size + 1);
  here = 0;
  for (iter = l; iter; iter = iter->next)
    {
      int len = strlen (iter->line);

      strcpy (&script[here], iter->line);
      here += len;
      script[here++] = '\n';
    }
  script[here] = '\0';
  return script;
}

/* Take a command line structure representing a "guile" command, and
   evaluate its body using the Guile interpreter.
   This is the extension_language_ops.eval_from_control_command "method".  */

static void
gdbscm_eval_from_control_command
  (const struct extension_language_defn *extlang, struct command_line *cmd)
{
  char *script;

  if (cmd->body_list_1 != nullptr)
    error (_("Invalid \"guile\" block structure."));

  script = compute_scheme_string (cmd->body_list_0.get ());
  gdb::unique_xmalloc_ptr<char> msg = gdbscm_safe_eval_string (script, 0);
  xfree (script);
  if (msg != NULL)
    error ("%s", msg.get ());
}

/* Read a file as Scheme code.
   This is the extension_language_script_ops.script_sourcer "method".
   FILE is the file to run.  FILENAME is name of the file FILE.
   This does not throw any errors.  If an exception occurs an error message
   is printed.  */

static void
gdbscm_source_script (const struct extension_language_defn *extlang,
		      FILE *file, const char *filename)
{
  gdb::unique_xmalloc_ptr<char> msg = gdbscm_safe_source_script (filename);

  if (msg != NULL)
    gdb_printf (gdb_stderr, "%s\n", msg.get ());
}

/* (execute string [#:from-tty boolean] [#:to-string boolean])
   A Scheme function which evaluates a string using the gdb CLI.  */

static SCM
gdbscm_execute_gdb_command (SCM command_scm, SCM rest)
{
  int from_tty_arg_pos = -1, to_string_arg_pos = -1;
  int from_tty = 0, to_string = 0;
  const SCM keywords[] = { from_tty_keyword, to_string_keyword, SCM_BOOL_F };
  char *command;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#tt",
			      command_scm, &command, rest,
			      &from_tty_arg_pos, &from_tty,
			      &to_string_arg_pos, &to_string);

  return gdbscm_wrap ([=]
    {
      gdb::unique_xmalloc_ptr<char> command_holder (command);
      std::string to_string_res;

      scoped_restore restore_async = make_scoped_restore (&current_ui->async,
							  0);

      scoped_restore preventer = prevent_dont_repeat ();
      if (to_string)
	execute_command_to_string (to_string_res, command, from_tty, false);
      else
	execute_command (command, from_tty);

      /* Do any commands attached to breakpoint we stopped at.  */
      bpstat_do_actions ();

      if (to_string)
	return gdbscm_scm_from_c_string (to_string_res.c_str ());
      return SCM_UNSPECIFIED;
    });
}

/* (data-directory) -> string */

static SCM
gdbscm_data_directory (void)
{
  return gdbscm_scm_from_c_string (gdb_datadir.c_str ());
}

/* (guile-data-directory) -> string */

static SCM
gdbscm_guile_data_directory (void)
{
  return gdbscm_scm_from_c_string (guile_datadir);
}

/* (gdb-version) -> string */

static SCM
gdbscm_gdb_version (void)
{
  return gdbscm_scm_from_c_string (version);
}

/* (host-config) -> string */

static SCM
gdbscm_host_config (void)
{
  return gdbscm_scm_from_c_string (host_name);
}

/* (target-config) -> string */

static SCM
gdbscm_target_config (void)
{
  return gdbscm_scm_from_c_string (target_name);
}

#else /* ! HAVE_GUILE */

/* Dummy implementation of the gdb "guile-repl" and "guile"
   commands. */

static void
guile_repl_command (const char *arg, int from_tty)
{
  arg = skip_spaces (arg);
  if (arg && *arg)
    error (_("guile-repl currently does not take any arguments."));
  error (_("Guile scripting is not supported in this copy of GDB."));
}

static void
guile_command (const char *arg, int from_tty)
{
  arg = skip_spaces (arg);
  if (arg && *arg)
    error (_("Guile scripting is not supported in this copy of GDB."));
  else
    {
      /* Even if Guile isn't enabled, we still have to slurp the
	 command list to the corresponding "end".  */
      counted_command_line l = get_command_line (guile_control, "");

      execute_control_command_untraced (l.get ());
    }
}

#endif /* ! HAVE_GUILE */

/* Lists for 'set,show,info guile' commands.  */

static struct cmd_list_element *set_guile_list;
static struct cmd_list_element *show_guile_list;
static struct cmd_list_element *info_guile_list;


/* Initialization.  */

#ifdef HAVE_GUILE

static const scheme_function misc_guile_functions[] =
{
  { "execute", 1, 0, 1, as_a_scm_t_subr (gdbscm_execute_gdb_command),
  "\
Execute the given GDB command.\n\
\n\
  Arguments: string [#:to-string boolean] [#:from-tty boolean]\n\
    If #:from-tty is true then the command executes as if entered\n\
    from the keyboard.  The default is false (#f).\n\
    If #:to-string is true then the result is returned as a string.\n\
    Otherwise output is sent to the current output port,\n\
    which is the default.\n\
  Returns: The result of the command if #:to-string is true.\n\
    Otherwise returns unspecified." },

  { "data-directory", 0, 0, 0, as_a_scm_t_subr (gdbscm_data_directory),
    "\
Return the name of GDB's data directory." },

  { "guile-data-directory", 0, 0, 0,
    as_a_scm_t_subr (gdbscm_guile_data_directory),
    "\
Return the name of the Guile directory within GDB's data directory." },

  { "gdb-version", 0, 0, 0, as_a_scm_t_subr (gdbscm_gdb_version),
    "\
Return GDB's version string." },

  { "host-config", 0, 0, 0, as_a_scm_t_subr (gdbscm_host_config),
    "\
Return the name of the host configuration." },

  { "target-config", 0, 0, 0, as_a_scm_t_subr (gdbscm_target_config),
    "\
Return the name of the target configuration." },

  END_FUNCTIONS
};

/* Load BOOT_SCM_FILE, the first Scheme file that gets loaded.  */

static SCM
boot_guile_support (void *boot_scm_file)
{
  /* Load boot.scm without compiling it (there's no need to compile it).
     The other files should have been compiled already, and boot.scm is
     expected to adjust '%load-compiled-path' accordingly.  If they haven't
     been compiled, Guile will auto-compile them. The important thing to keep
     in mind is that there's a >= 100x speed difference between compiled and
     non-compiled files.  */
  return scm_c_primitive_load ((const char *) boot_scm_file);
}

/* Return non-zero if ARGS has the "standard" format for throw args.
   The standard format is:
   (function format-string (format-string-args-list) ...).
   FUNCTION is #f if no function was recorded.  */

static int
standard_throw_args_p (SCM args)
{
  if (gdbscm_is_true (scm_list_p (args))
      && scm_ilength (args) >= 3)
    {
      /* The function in which the error occurred.  */
      SCM arg0 = scm_list_ref (args, scm_from_int (0));
      /* The format string.  */
      SCM arg1 = scm_list_ref (args, scm_from_int (1));
      /* The arguments of the format string.  */
      SCM arg2 = scm_list_ref (args, scm_from_int (2));

      if ((scm_is_string (arg0) || gdbscm_is_false (arg0))
	  && scm_is_string (arg1)
	  && gdbscm_is_true (scm_list_p (arg2)))
	return 1;
    }

  return 0;
}

/* Print the error recorded in a "standard" throw args.  */

static void
print_standard_throw_error (SCM args)
{
  /* The function in which the error occurred.  */
  SCM arg0 = scm_list_ref (args, scm_from_int (0));
  /* The format string.  */
  SCM arg1 = scm_list_ref (args, scm_from_int (1));
  /* The arguments of the format string.  */
  SCM arg2 = scm_list_ref (args, scm_from_int (2));

  /* ARG0 is #f if no function was recorded.  */
  if (gdbscm_is_true (arg0))
    {
      scm_simple_format (scm_current_error_port (),
			 scm_from_latin1_string (_("Error in function ~s:~%")),
			 scm_list_1 (arg0));
    }
  scm_simple_format (scm_current_error_port (), arg1, arg2);
}

/* Print the error message recorded in KEY, ARGS, the arguments to throw.
   Normally we let Scheme print the error message.
   This function is used when Scheme initialization fails.
   We can still use the Scheme C API though.  */

static void
print_throw_error (SCM key, SCM args)
{
  /* IWBN to call gdbscm_print_exception_with_stack here, but Guile didn't
     boot successfully so play it safe and avoid it.  The "format string" and
     its args are embedded in ARGS, but the content of ARGS depends on KEY.
     Make sure ARGS has the expected canonical content before trying to use
     it.  */
  if (standard_throw_args_p (args))
    print_standard_throw_error (args);
  else
    {
      scm_simple_format (scm_current_error_port (),
			 scm_from_latin1_string (_("Throw to key `~a' with args `~s'.~%")),
			 scm_list_2 (key, args));
    }
}

/* Handle an exception thrown while loading BOOT_SCM_FILE.  */

static SCM
handle_boot_error (void *boot_scm_file, SCM key, SCM args)
{
  gdb_printf (gdb_stderr, ("Exception caught while booting Guile.\n"));

  print_throw_error (key, args);

  gdb_printf (gdb_stderr, "\n");
  warning (_("Could not complete Guile gdb module initialization from:\n"
	     "%s.\n"
	     "Limited Guile support is available.\n"
	     "Suggest passing --data-directory=/path/to/gdb/data-directory."),
	   (const char *) boot_scm_file);

  return SCM_UNSPECIFIED;
}

/* Load gdb/boot.scm, the Scheme side of GDB/Guile support.
   Note: This function assumes it's called within the gdb module.  */

static void
initialize_scheme_side (void)
{
  char *boot_scm_path;

  guile_datadir = concat (gdb_datadir.c_str (), SLASH_STRING, "guile",
			  (char *) NULL);
  boot_scm_path = concat (guile_datadir, SLASH_STRING, "gdb",
			  SLASH_STRING, boot_scm_filename, (char *) NULL);

  scm_c_catch (SCM_BOOL_T, boot_guile_support, boot_scm_path,
	       handle_boot_error, boot_scm_path, NULL, NULL);

  xfree (boot_scm_path);
}

/* Install the gdb scheme module.
   The result is a boolean indicating success.
   If initializing the gdb module fails an error message is printed.
   Note: This function runs in the context of the gdb module.  */

static void
initialize_gdb_module (void *data)
{
  /* Computing these is a pain, so only do it once.
     Also, do it here and save the result so that obtaining the values
     is thread-safe.  */
  gdbscm_guile_major_version = gdbscm_scm_string_to_int (scm_major_version ());
  gdbscm_guile_minor_version = gdbscm_scm_string_to_int (scm_minor_version ());
  gdbscm_guile_micro_version = gdbscm_scm_string_to_int (scm_micro_version ());

  /* The documentation symbol needs to be defined before any calls to
     gdbscm_define_{variables,functions}.  */
  gdbscm_documentation_symbol = scm_from_latin1_symbol ("documentation");

  /* The smob and exception support must be initialized early.  */
  gdbscm_initialize_smobs ();
  gdbscm_initialize_exceptions ();

  /* The rest are initialized in alphabetical order.  */
  gdbscm_initialize_arches ();
  gdbscm_initialize_auto_load ();
  gdbscm_initialize_blocks ();
  gdbscm_initialize_breakpoints ();
  gdbscm_initialize_commands ();
  gdbscm_initialize_disasm ();
  gdbscm_initialize_frames ();
  gdbscm_initialize_iterators ();
  gdbscm_initialize_lazy_strings ();
  gdbscm_initialize_math ();
  gdbscm_initialize_objfiles ();
  gdbscm_initialize_parameters ();
  gdbscm_initialize_ports ();
  gdbscm_initialize_pretty_printers ();
  gdbscm_initialize_pspaces ();
  gdbscm_initialize_strings ();
  gdbscm_initialize_symbols ();
  gdbscm_initialize_symtabs ();
  gdbscm_initialize_types ();
  gdbscm_initialize_values ();

  gdbscm_define_functions (misc_guile_functions, 1);

  from_tty_keyword = scm_from_latin1_keyword ("from-tty");
  to_string_keyword = scm_from_latin1_keyword ("to-string");

  initialize_scheme_side ();

  gdb_scheme_initialized = 1;
}

/* Utility to call scm_c_define_module+initialize_gdb_module from
   within scm_with_guile.  */

static void *
call_initialize_gdb_module (void *data)
{
  /* Most of the initialization is done by initialize_gdb_module.
     It is called via scm_c_define_module so that the initialization is
     performed within the desired module.  */
  scm_c_define_module (gdbscm_module_name, initialize_gdb_module, NULL);

#if HAVE_GUILE_MANUAL_FINALIZATION
  scm_run_finalizers ();
#endif

  return NULL;
}

/* A callback to initialize Guile after gdb has finished all its
   initialization.  This is the extension_language_ops.initialize "method".  */

static void
gdbscm_initialize (const struct extension_language_defn *extlang)
{
#if HAVE_GUILE
  /* The Python support puts the C side in module "_gdb", leaving the
     Python side to define module "gdb" which imports "_gdb".  There is
     evidently no similar convention in Guile so we skip this.  */

#if HAVE_GUILE_MANUAL_FINALIZATION
  /* Our SMOB free functions are not thread-safe, as GDB itself is not
     intended to be thread-safe.  Disable automatic finalization so that
     finalizers aren't run in other threads.  */
  scm_set_automatic_finalization_enabled (0);
#endif

  /* Before we initialize Guile, block signals needed by gdb (especially
     SIGCHLD).  This is done so that all threads created during Guile
     initialization have SIGCHLD blocked.  PR 17247.  Really libgc and
     Guile should do this, but we need to work with libgc 7.4.x.  */
  {
    gdb::block_signals blocker;

    /* There are libguile versions (f.i. v3.0.5) that by default call
       mp_get_memory_functions during initialization to install custom
       libgmp memory functions.  This is considered a bug and should be
       fixed starting v3.0.6.
       Before gdb commit 880ae75a2b7 "gdb delay guile initialization until
       gdbscm_finish_initialization", that bug had no effect for gdb,
       because gdb subsequently called mp_get_memory_functions to install
       its own custom functions in _initialize_gmp_utils.  However, since
       aforementioned gdb commit the initialization order is reversed,
       allowing libguile to install a custom malloc that is incompatible
       with the custom free as used in gmp-utils.c, resulting in a
       "double free or corruption (out)" error.
       Work around the libguile bug by disabling the installation of the
       libgmp memory functions by guile initialization.  */

    /* The scm_install_gmp_memory_functions variable should be removed after
       version 3.0, so limit usage to 3.0 and before.  */
#if SCM_MAJOR_VERSION < 3 || (SCM_MAJOR_VERSION == 3 && SCM_MINOR_VERSION == 0)
    /* This variable is deprecated in Guile 3.0.8 and later but remains
       available in the whole 3.0 series.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    scm_install_gmp_memory_functions = 0;
#pragma GCC diagnostic pop
#endif

    /* scm_with_guile is the most portable way to initialize Guile.  Plus
       we need to initialize the Guile support while in Guile mode (e.g.,
       called from within a call to scm_with_guile).  */
    scm_with_guile (call_initialize_gdb_module, NULL);
  }

  /* Set Guile's backtrace to match the "set guile print-stack" default.
     [N.B. The two settings are still separate.]  But only do this after
     we've initialized Guile, it's nice to see a backtrace if there's an
     error during initialization.  OTOH, if the error is that gdb/init.scm
     wasn't found because gdb is being run from the build tree, the
     backtrace is more noise than signal.  Sigh.  */
  gdbscm_set_backtrace (0);
#endif

  /* Restore the environment to the user interaction one.  */
  scm_set_current_module (scm_interaction_environment ());
}

/* The extension_language_ops.initialized "method".  */

static int
gdbscm_initialized (const struct extension_language_defn *extlang)
{
  return gdb_scheme_initialized;
}

/* Enable or disable Guile backtraces.  */

static void
gdbscm_set_backtrace (int enable)
{
  static const char disable_bt[] = "(debug-disable 'backtrace)";
  static const char enable_bt[] = "(debug-enable 'backtrace)";

  if (enable)
    gdbscm_safe_eval_string (enable_bt, 0);
  else
    gdbscm_safe_eval_string (disable_bt, 0);
}

#endif /* HAVE_GUILE */

/* See guile.h.  */
cmd_list_element *guile_cmd_element = nullptr;

/* Install the various gdb commands used by Guile.  */

static void
install_gdb_commands (void)
{
  cmd_list_element *guile_repl_cmd
    = add_com ("guile-repl", class_obscure, guile_repl_command,
#ifdef HAVE_GUILE
	   _("\
Start an interactive Guile prompt.\n\
\n\
To return to GDB, type the EOF character (e.g., Ctrl-D on an empty\n\
prompt) or ,quit.")
#else /* HAVE_GUILE */
	   _("\
Start a Guile interactive prompt.\n\
\n\
Guile scripting is not supported in this copy of GDB.\n\
This command is only a placeholder.")
#endif /* HAVE_GUILE */
	   );
  add_com_alias ("gr", guile_repl_cmd, class_obscure, 1);

  /* Since "help guile" is easy to type, and intuitive, we add general help
     in using GDB+Guile to this command.  */
  guile_cmd_element = add_com ("guile", class_obscure, guile_command,
#ifdef HAVE_GUILE
	   _("\
Evaluate one or more Guile expressions.\n\
\n\
The expression(s) can be given as an argument, for instance:\n\
\n\
    guile (display 23)\n\
\n\
The result of evaluating the last expression is printed.\n\
\n\
If no argument is given, the following lines are read and passed\n\
to Guile for evaluation.  Type a line containing \"end\" to indicate\n\
the end of the set of expressions.\n\
\n\
The Guile GDB module must first be imported before it can be used.\n\
Do this with:\n\
(gdb) guile (use-modules (gdb))\n\
or if you want to import the (gdb) module with a prefix, use:\n\
(gdb) guile (use-modules ((gdb) #:renamer (symbol-prefix-proc 'gdb:)))\n\
\n\
The Guile interactive session, started with the \"guile-repl\"\n\
command, provides extensive help and apropos capabilities.\n\
Type \",help\" once in a Guile interactive session.")
#else /* HAVE_GUILE */
	   _("\
Evaluate a Guile expression.\n\
\n\
Guile scripting is not supported in this copy of GDB.\n\
This command is only a placeholder.")
#endif /* HAVE_GUILE */
	   );
  add_com_alias ("gu", guile_cmd_element, class_obscure, 1);

  set_show_commands setshow_guile_cmds
    = add_setshow_prefix_cmd ("guile", class_obscure,
			      _("\
Prefix command for Guile preference settings."),
			      _("\
Prefix command for Guile preference settings."),
			      &set_guile_list, &show_guile_list,
			      &setlist, &showlist);

  add_alias_cmd ("gu", setshow_guile_cmds.set, class_obscure, 1, &setlist);
  add_alias_cmd ("gu", setshow_guile_cmds.show, class_obscure, 1, &showlist);

  cmd_list_element *info_guile_cmd
    = add_basic_prefix_cmd ("guile", class_obscure,
			    _("Prefix command for Guile info displays."),
			    &info_guile_list, 0, &infolist);
  add_info_alias ("gu", info_guile_cmd, 1);

  /* The name "print-stack" is carried over from Python.
     A better name is "print-exception".  */
  add_setshow_enum_cmd ("print-stack", no_class, guile_print_excp_enums,
			&gdbscm_print_excp, _("\
Set mode for Guile exception printing on error."), _("\
Show the mode of Guile exception printing on error."), _("\
none  == no stack or message will be printed.\n\
full == a message and a stack will be printed.\n\
message == an error message without a stack will be printed."),
			NULL, NULL,
			&set_guile_list, &show_guile_list);
}

void _initialize_guile ();
void
_initialize_guile ()
{
  install_gdb_commands ();
}

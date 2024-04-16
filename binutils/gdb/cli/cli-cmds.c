/* GDB CLI commands.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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
#include "readline/tilde.h"
#include "completer.h"
#include "target.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbcmd.h"
#include "gdbsupport/gdb_regex.h"
#include "gdb_vfork.h"
#include "linespec.h"
#include "expression.h"
#include "frame.h"
#include "value.h"
#include "language.h"
#include "filenames.h"
#include "objfiles.h"
#include "source.h"
#include "disasm.h"
#include "tracepoint.h"
#include "gdbsupport/filestuff.h"
#include "location.h"
#include "block.h"
#include "valprint.h"

#include "ui-out.h"
#include "interps.h"

#include "top.h"
#include "ui.h"
#include "cli/cli-decode.h"
#include "cli/cli-script.h"
#include "cli/cli-setshow.h"
#include "cli/cli-cmds.h"
#include "cli/cli-style.h"
#include "cli/cli-utils.h"
#include "cli/cli-style.h"

#include "extension.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/gdb_tilde_expand.h"

#ifdef TUI
#include "tui/tui.h"
#endif

#include <fcntl.h>
#include <algorithm>
#include <string>

/* Prototypes for local utility functions */

static void print_sal_location (const symtab_and_line &sal);

static void ambiguous_line_spec (gdb::array_view<const symtab_and_line> sals,
				 const char *format, ...)
  ATTRIBUTE_PRINTF (2, 3);

static void filter_sals (std::vector<symtab_and_line> &);


/* See cli-cmds.h. */
unsigned int max_user_call_depth = 1024;

/* Define all cmd_list_elements.  */

/* Chain containing all defined commands.  */

struct cmd_list_element *cmdlist;

/* Chain containing all defined info subcommands.  */

struct cmd_list_element *infolist;

/* Chain containing all defined enable subcommands.  */

struct cmd_list_element *enablelist;

/* Chain containing all defined disable subcommands.  */

struct cmd_list_element *disablelist;

/* Chain containing all defined stop subcommands.  */

struct cmd_list_element *stoplist;

/* Chain containing all defined delete subcommands.  */

struct cmd_list_element *deletelist;

/* Chain containing all defined detach subcommands.  */

struct cmd_list_element *detachlist;

/* Chain containing all defined kill subcommands.  */

struct cmd_list_element *killlist;

/* Chain containing all defined set subcommands */

struct cmd_list_element *setlist;

/* Chain containing all defined unset subcommands */

struct cmd_list_element *unsetlist;

/* Chain containing all defined show subcommands.  */

struct cmd_list_element *showlist;

/* Chain containing all defined \"set history\".  */

struct cmd_list_element *sethistlist;

/* Chain containing all defined \"show history\".  */

struct cmd_list_element *showhistlist;

/* Chain containing all defined \"unset history\".  */

struct cmd_list_element *unsethistlist;

/* Chain containing all defined maintenance subcommands.  */

struct cmd_list_element *maintenancelist;

/* Chain containing all defined "maintenance info" subcommands.  */

struct cmd_list_element *maintenanceinfolist;

/* Chain containing all defined "maintenance print" subcommands.  */

struct cmd_list_element *maintenanceprintlist;

/* Chain containing all defined "maintenance check" subcommands.  */

struct cmd_list_element *maintenancechecklist;

/* Chain containing all defined "maintenance flush" subcommands.  */

struct cmd_list_element *maintenanceflushlist;

struct cmd_list_element *setprintlist;

struct cmd_list_element *showprintlist;

struct cmd_list_element *setdebuglist;

struct cmd_list_element *showdebuglist;

struct cmd_list_element *setchecklist;

struct cmd_list_element *showchecklist;

struct cmd_list_element *setsourcelist;

struct cmd_list_element *showsourcelist;

/* Command tracing state.  */

int source_verbose = 0;
bool trace_commands = false;

/* 'script-extension' option support.  */

static const char script_ext_off[] = "off";
static const char script_ext_soft[] = "soft";
static const char script_ext_strict[] = "strict";

static const char *const script_ext_enums[] = {
  script_ext_off,
  script_ext_soft,
  script_ext_strict,
  NULL
};

static const char *script_ext_mode = script_ext_soft;


/* User-controllable flag to suppress event notification on CLI.  */

static bool user_wants_cli_suppress_notification = false;

/* Utility used everywhere when at least one argument is needed and
   none is supplied.  */

void
error_no_arg (const char *why)
{
  error (_("Argument required (%s)."), why);
}

/* This implements the "info" prefix command.  Normally such commands
   are automatically handled by add_basic_prefix_cmd, but in this case
   a separate command is used so that it can be hooked into by
   gdb-gdb.gdb.  */

static void
info_command (const char *arg, int from_tty)
{
  help_list (infolist, "info ", all_commands, gdb_stdout);
}

/* See cli/cli-cmds.h.  */

void
with_command_1 (const char *set_cmd_prefix,
		cmd_list_element *setlist, const char *args, int from_tty)
{
  if (args == nullptr)
    error (_("Missing arguments."));

  const char *delim = strstr (args, "--");
  const char *nested_cmd = nullptr;

  if (delim == args)
    error (_("Missing setting before '--' delimiter"));

  if (delim == nullptr || *skip_spaces (&delim[2]) == '\0')
    nested_cmd = repeat_previous ();

  cmd_list_element *set_cmd = lookup_cmd (&args, setlist, set_cmd_prefix,
					  nullptr,
					  /*allow_unknown=*/ 0,
					  /*ignore_help_classes=*/ 1);
  gdb_assert (set_cmd != nullptr);

  if (!set_cmd->var.has_value ())
    error (_("Cannot use this setting with the \"with\" command"));

  std::string temp_value
    = (delim == nullptr ? args : std::string (args, delim - args));

  if (nested_cmd == nullptr)
    nested_cmd = skip_spaces (delim + 2);

  gdb_assert (set_cmd->var.has_value ());
  std::string org_value = get_setshow_command_value_string (*set_cmd->var);

  /* Tweak the setting to the new temporary value.  */
  do_set_command (temp_value.c_str (), from_tty, set_cmd);

  try
    {
      scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

      /* Execute the nested command.  */
      execute_command (nested_cmd, from_tty);
    }
  catch (const gdb_exception &ex)
    {
      /* Restore the setting and rethrow.  If restoring the setting
	 throws, swallow the new exception and warn.  There's nothing
	 else we can reasonably do.  */
      try
	{
	  do_set_command (org_value.c_str (), from_tty, set_cmd);
	}
      catch (const gdb_exception &ex2)
	{
	  warning (_("Couldn't restore setting: %s"), ex2.what ());
	}

      throw;
    }

  /* Restore the setting.  */
  do_set_command (org_value.c_str (), from_tty, set_cmd);
}

/* See cli/cli-cmds.h.  */

void
with_command_completer_1 (const char *set_cmd_prefix,
			  completion_tracker &tracker,
			  const char *text)
{
  tracker.set_use_custom_word_point (true);

  const char *delim = strstr (text, "--");

  /* If we're still not past the "--" delimiter, complete the "with"
     command as if it was a "set" command.  */
  if (delim == text
      || delim == nullptr
      || !isspace (delim[-1])
      || !(isspace (delim[2]) || delim[2] == '\0'))
    {
      std::string new_text = std::string (set_cmd_prefix) + text;
      tracker.advance_custom_word_point_by (-(int) strlen (set_cmd_prefix));
      complete_nested_command_line (tracker, new_text.c_str ());
      return;
    }

  /* We're past the "--" delimiter.  Complete on the sub command.  */
  const char *nested_cmd = skip_spaces (delim + 2);
  tracker.advance_custom_word_point_by (nested_cmd - text);
  complete_nested_command_line (tracker, nested_cmd);
}

/* The "with" command.  */

static void
with_command (const char *args, int from_tty)
{
  with_command_1 ("set ", setlist, args, from_tty);
}

/* "with" command completer.  */

static void
with_command_completer (struct cmd_list_element *ignore,
			completion_tracker &tracker,
			const char *text, const char * /*word*/)
{
  with_command_completer_1 ("set ", tracker,  text);
}

/* Look up the contents of TEXT as a command usable with default args.
   Throws an error if no such command is found.
   Return the found command and advances TEXT past the found command.
   If the found command is a postfix command, set *PREFIX_CMD to its
   prefix command.  */

static struct cmd_list_element *
lookup_cmd_for_default_args (const char **text,
			     struct cmd_list_element **prefix_cmd)
{
  const char *orig_text = *text;
  struct cmd_list_element *lcmd;

  if (*text == nullptr || skip_spaces (*text) == nullptr)
    error (_("ALIAS missing."));

  /* We first use lookup_cmd to verify TEXT unambiguously identifies
     a command.  */
  lcmd = lookup_cmd (text, cmdlist, "", NULL,
		     /*allow_unknown=*/ 0,
		     /*ignore_help_classes=*/ 1);

  /* Note that we accept default args for prefix commands,
     as a prefix command can also be a valid usable
     command accepting some arguments.
     For example, "thread apply" applies a command to a
     list of thread ids, and is also the prefix command for
     thread apply all.  */

  /* We have an unambiguous command for which default args
     can be specified.  What remains after having found LCMD
     is either spaces, or the default args character.  */

  /* We then use lookup_cmd_composition to detect if the user
     has specified an alias, and find the possible prefix_cmd
     of cmd.  */
  struct cmd_list_element *alias, *cmd;
  lookup_cmd_composition
    (std::string (orig_text, *text - orig_text).c_str (),
     &alias, prefix_cmd, &cmd);
  gdb_assert (cmd != nullptr);
  gdb_assert (cmd == lcmd);
  if (alias != nullptr)
    cmd = alias;

  return cmd;
}

/* Provide documentation on command or list given by COMMAND.  FROM_TTY
   is ignored.  */

static void
help_command (const char *command, int from_tty)
{
  help_cmd (command, gdb_stdout);
}


/* Note: The "complete" command is used by Emacs to implement completion.
   [Is that why this function writes output with *_unfiltered?]  */

static void
complete_command (const char *arg, int from_tty)
{
  dont_repeat ();

  if (max_completions == 0)
    {
      /* Only print this for non-mi frontends.  An MI frontend may not
	 be able to handle this.  */
      if (!current_uiout->is_mi_like_p ())
	{
	  printf_unfiltered (_("max-completions is zero,"
			       " completion is disabled.\n"));
	}
      return;
    }

  if (arg == NULL)
    arg = "";

  int quote_char = '\0';
  const char *word;

  completion_result result = complete (arg, &word, &quote_char);

  if (result.number_matches != 0)
    {
      std::string arg_prefix (arg, word - arg);

      if (result.number_matches == 1)
	printf_unfiltered ("%s%s\n", arg_prefix.c_str (), result.match_list[0]);
      else
	{
	  result.sort_match_list ();

	  for (size_t i = 0; i < result.number_matches; i++)
	    {
	      printf_unfiltered ("%s%s",
				 arg_prefix.c_str (),
				 result.match_list[i + 1]);
	      if (quote_char)
		printf_unfiltered ("%c", quote_char);
	      printf_unfiltered ("\n");
	    }
	}

      if (result.number_matches == max_completions)
	{
	  /* ARG_PREFIX and WORD are included in the output so that emacs
	     will include the message in the output.  */
	  printf_unfiltered (_("%s%s %s\n"),
			     arg_prefix.c_str (), word,
			     get_max_completions_reached_message ());
	}
    }
}

int
is_complete_command (struct cmd_list_element *c)
{
  return cmd_simple_func_eq (c, complete_command);
}

static void
show_version (const char *args, int from_tty)
{
  print_gdb_version (gdb_stdout, true);
  gdb_printf ("\n");
}

static void
show_configuration (const char *args, int from_tty)
{
  print_gdb_configuration (gdb_stdout);
}

/* Handle the quit command.  */

void
quit_command (const char *args, int from_tty)
{
  int exit_code = 0;

  /* An optional expression may be used to cause gdb to terminate with
     the value of that expression.  */
  if (args)
    {
      struct value *val = parse_and_eval (args);

      exit_code = (int) value_as_long (val);
    }

  if (!quit_confirm ())
    error (_("Not confirmed."));

  try
    {
      query_if_trace_running (from_tty);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == TARGET_CLOSE_ERROR)
	/* We don't care about this since we're quitting anyway, so keep
	   quitting.  */
	exception_print (gdb_stderr, ex);
      else
	/* Rethrow, to properly handle error (_("Not confirmed.")).  */
	throw;
    }

  quit_force (args ? &exit_code : NULL, from_tty);
}

static void
pwd_command (const char *args, int from_tty)
{
  if (args)
    error (_("The \"pwd\" command does not take an argument: %s"), args);

  gdb::unique_xmalloc_ptr<char> cwd (getcwd (NULL, 0));

  if (cwd == NULL)
    error (_("Error finding name of working directory: %s"),
	   safe_strerror (errno));

  if (strcmp (cwd.get (), current_directory) != 0)
    gdb_printf (_("Working directory %ps\n (canonically %ps).\n"),
		styled_string (file_name_style.style (),
			       current_directory),
		styled_string (file_name_style.style (), cwd.get ()));
  else
    gdb_printf (_("Working directory %ps.\n"),
		styled_string (file_name_style.style (),
			       current_directory));
}

void
cd_command (const char *dir, int from_tty)
{
  int len;
  /* Found something other than leading repetitions of "/..".  */
  int found_real_path;
  char *p;

  /* If the new directory is absolute, repeat is a no-op; if relative,
     repeat might be useful but is more likely to be a mistake.  */
  dont_repeat ();

  gdb::unique_xmalloc_ptr<char> dir_holder
    (tilde_expand (dir != NULL ? dir : "~"));
  dir = dir_holder.get ();

  if (chdir (dir) < 0)
    perror_with_name (dir);

#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  /* There's too much mess with DOSish names like "d:", "d:.",
     "d:./foo" etc.  Instead of having lots of special #ifdef'ed code,
     simply get the canonicalized name of the current directory.  */
  gdb::unique_xmalloc_ptr<char> cwd (getcwd (NULL, 0));
  dir = cwd.get ();
#endif

  len = strlen (dir);
  if (IS_DIR_SEPARATOR (dir[len - 1]))
    {
      /* Remove the trailing slash unless this is a root directory
	 (including a drive letter on non-Unix systems).  */
      if (!(len == 1)		/* "/" */
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
	  && !(len == 3 && dir[1] == ':') /* "d:/" */
#endif
	  )
	len--;
    }

  dir_holder.reset (savestring (dir, len));
  if (IS_ABSOLUTE_PATH (dir_holder.get ()))
    {
      xfree (current_directory);
      current_directory = dir_holder.release ();
    }
  else
    {
      if (IS_DIR_SEPARATOR (current_directory[strlen (current_directory) - 1]))
	current_directory = concat (current_directory, dir_holder.get (),
				    (char *) NULL);
      else
	current_directory = concat (current_directory, SLASH_STRING,
				    dir_holder.get (), (char *) NULL);
    }

  /* Now simplify any occurrences of `.' and `..' in the pathname.  */

  found_real_path = 0;
  for (p = current_directory; *p;)
    {
      if (IS_DIR_SEPARATOR (p[0]) && p[1] == '.'
	  && (p[2] == 0 || IS_DIR_SEPARATOR (p[2])))
	memmove (p, p + 2, strlen (p + 2) + 1);
      else if (IS_DIR_SEPARATOR (p[0]) && p[1] == '.' && p[2] == '.'
	       && (p[3] == 0 || IS_DIR_SEPARATOR (p[3])))
	{
	  if (found_real_path)
	    {
	      /* Search backwards for the directory just before the "/.."
		 and obliterate it and the "/..".  */
	      char *q = p;

	      while (q != current_directory && !IS_DIR_SEPARATOR (q[-1]))
		--q;

	      if (q == current_directory)
		/* current_directory is
		   a relative pathname ("can't happen"--leave it alone).  */
		++p;
	      else
		{
		  memmove (q - 1, p + 3, strlen (p + 3) + 1);
		  p = q - 1;
		}
	    }
	  else
	    /* We are dealing with leading repetitions of "/..", for
	       example "/../..", which is the Mach super-root.  */
	    p += 3;
	}
      else
	{
	  found_real_path = 1;
	  ++p;
	}
    }

  forget_cached_source_info ();

  if (from_tty)
    pwd_command ((char *) 0, 1);
}

/* Show the current value of the 'script-extension' option.  */

static void
show_script_ext_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Script filename extension recognition is \"%s\".\n"),
	      value);
}

/* Try to open SCRIPT_FILE.
   If successful, the full path name is stored in *FULL_PATHP,
   and the stream is returned.
   If not successful, return NULL; errno is set for the last file
   we tried to open.

   If SEARCH_PATH is non-zero, and the file isn't found in cwd,
   search for it in the source search path.  */

std::optional<open_script>
find_and_open_script (const char *script_file, int search_path)
{
  int fd;
  openp_flags search_flags = OPF_TRY_CWD_FIRST | OPF_RETURN_REALPATH;
  std::optional<open_script> opened;

  gdb::unique_xmalloc_ptr<char> file (tilde_expand (script_file));

  if (search_path)
    search_flags |= OPF_SEARCH_IN_PATH;

  /* Search for and open 'file' on the search path used for source
     files.  Put the full location in *FULL_PATHP.  */
  gdb::unique_xmalloc_ptr<char> full_path;
  fd = openp (source_path.c_str (), search_flags,
	      file.get (), O_RDONLY, &full_path);

  if (fd == -1)
    return opened;

  FILE *result = fdopen (fd, FOPEN_RT);
  if (result == NULL)
    {
      int save_errno = errno;

      close (fd);
      errno = save_errno;
    }
  else
    opened.emplace (gdb_file_up (result), std::move (full_path));

  return opened;
}

/* Load script FILE, which has already been opened as STREAM.
   FILE_TO_OPEN is the form of FILE to use if one needs to open the file.
   This is provided as FILE may have been found via the source search path.
   An important thing to note here is that FILE may be a symlink to a file
   with a different or non-existing suffix, and thus one cannot infer the
   extension language from FILE_TO_OPEN.  */

static void
source_script_from_stream (FILE *stream, const char *file,
			   const char *file_to_open)
{
  if (script_ext_mode != script_ext_off)
    {
      const struct extension_language_defn *extlang
	= get_ext_lang_of_file (file);

      if (extlang != NULL)
	{
	  if (ext_lang_present_p (extlang))
	    {
	      script_sourcer_func *sourcer
		= ext_lang_script_sourcer (extlang);

	      gdb_assert (sourcer != NULL);
	      sourcer (extlang, stream, file_to_open);
	      return;
	    }
	  else if (script_ext_mode == script_ext_soft)
	    {
	      /* Assume the file is a gdb script.
		 This is handled below.  */
	    }
	  else
	    throw_ext_lang_unsupported (extlang);
	}
    }

  script_from_file (stream, file);
}

/* Worker to perform the "source" command.
   Load script FILE.
   If SEARCH_PATH is non-zero, and the file isn't found in cwd,
   search for it in the source search path.  */

static void
source_script_with_search (const char *file, int from_tty, int search_path)
{

  if (file == NULL || *file == 0)
    error (_("source command requires file name of file to source."));

  std::optional<open_script> opened = find_and_open_script (file, search_path);
  if (!opened)
    {
      /* The script wasn't found, or was otherwise inaccessible.
	 If the source command was invoked interactively, throw an
	 error.  Otherwise (e.g. if it was invoked by a script),
	 just emit a warning, rather than cause an error.  */
      if (from_tty)
	perror_with_name (file);
      else
	{
	  perror_warning_with_name (file);
	  return;
	}
    }

  /* The python support reopens the file, so we need to pass full_path here
     in case the file was found on the search path.  It's useful to do this
     anyway so that error messages show the actual file used.  But only do
     this if we (may have) used search_path, as printing the full path in
     errors for the non-search case can be more noise than signal.  */
  const char *file_to_open;
  std::string tilde_expanded_file;
  if (search_path)
    file_to_open = opened->full_path.get ();
  else
    {
      tilde_expanded_file = gdb_tilde_expand (file);
      file_to_open = tilde_expanded_file.c_str ();
    }
  source_script_from_stream (opened->stream.get (), file, file_to_open);
}

/* Wrapper around source_script_with_search to export it to main.c
   for use in loading .gdbinit scripts.  */

void
source_script (const char *file, int from_tty)
{
  source_script_with_search (file, from_tty, 0);
}

static void
source_command (const char *args, int from_tty)
{
  const char *file = args;
  int search_path = 0;

  scoped_restore save_source_verbose = make_scoped_restore (&source_verbose);

  /* -v causes the source command to run in verbose mode.
     -s causes the file to be searched in the source search path,
     even if the file name contains a '/'.
     We still have to be able to handle filenames with spaces in a
     backward compatible way, so buildargv is not appropriate.  */

  if (args)
    {
      while (args[0] != '\0')
	{
	  /* Make sure leading white space does not break the
	     comparisons.  */
	  args = skip_spaces (args);

	  if (args[0] != '-')
	    break;

	  if (args[1] == 'v' && isspace (args[2]))
	    {
	      source_verbose = 1;

	      /* Skip passed -v.  */
	      args = &args[3];
	    }
	  else if (args[1] == 's' && isspace (args[2]))
	    {
	      search_path = 1;

	      /* Skip passed -s.  */
	      args = &args[3];
	    }
	  else
	    break;
	}

      file = skip_spaces (args);
    }

  source_script_with_search (file, from_tty, search_path);
}


static void
echo_command (const char *text, int from_tty)
{
  const char *p = text;
  int c;

  if (text)
    while ((c = *p++) != '\0')
      {
	if (c == '\\')
	  {
	    /* \ at end of argument is used after spaces
	       so they won't be lost.  */
	    if (*p == 0)
	      return;

	    c = parse_escape (get_current_arch (), &p);
	    if (c >= 0)
	      gdb_printf ("%c", c);
	  }
	else
	  gdb_printf ("%c", c);
      }

  gdb_stdout->reset_style ();

  /* Force this output to appear now.  */
  gdb_flush (gdb_stdout);
}

/* Sets the last launched shell command convenience variables based on
   EXIT_STATUS.  */

static void
exit_status_set_internal_vars (int exit_status)
{
  struct internalvar *var_code = lookup_internalvar ("_shell_exitcode");
  struct internalvar *var_signal = lookup_internalvar ("_shell_exitsignal");

  clear_internalvar (var_code);
  clear_internalvar (var_signal);

  /* Keep the logic here in sync with shell_internal_fn.  */

  if (WIFEXITED (exit_status))
    set_internalvar_integer (var_code, WEXITSTATUS (exit_status));
#ifdef __MINGW32__
  else if (WIFSIGNALED (exit_status) && WTERMSIG (exit_status) == -1)
    {
      /* The -1 condition can happen on MinGW, if we don't recognize
	 the fatal exception code encoded in the exit status; see
	 gdbsupport/gdb_wait.c.  We don't want to lose information in
	 the exit status in that case.  Record it as a normal exit
	 with the full exit status, including the higher 0xC0000000
	 bits.  */
      set_internalvar_integer (var_code, exit_status);
    }
#endif
  else if (WIFSIGNALED (exit_status))
    set_internalvar_integer (var_signal, WTERMSIG (exit_status));
  else
    warning (_("unexpected shell command exit status %d"), exit_status);
}

/* Run ARG under the shell, and return the exit status.  If ARG is
   NULL, run an interactive shell.  */

static int
run_under_shell (const char *arg, int from_tty)
{
#if defined(CANT_FORK) || \
      (!defined(HAVE_WORKING_VFORK) && !defined(HAVE_WORKING_FORK))
  /* If ARG is NULL, they want an inferior shell, but `system' just
     reports if the shell is available when passed a NULL arg.  */
  int rc = system (arg ? arg : "");

  if (!arg)
    arg = "inferior shell";

  if (rc == -1)
    gdb_printf (gdb_stderr, "Cannot execute %s: %s\n", arg,
		safe_strerror (errno));
  else if (rc)
    gdb_printf (gdb_stderr, "%s exited with status %d\n", arg, rc);
#ifdef GLOBAL_CURDIR
  /* Make sure to return to the directory GDB thinks it is, in case
     the shell command we just ran changed it.  */
  chdir (current_directory);
#endif
  return rc;
#else /* Can fork.  */
  int status, pid;

  if ((pid = vfork ()) == 0)
    {
      const char *p, *user_shell = get_shell ();

      close_most_fds ();

      /* Get the name of the shell for arg0.  */
      p = lbasename (user_shell);

      if (!arg)
	execl (user_shell, p, (char *) 0);
      else
	execl (user_shell, p, "-c", arg, (char *) 0);

      gdb_printf (gdb_stderr, "Cannot execute %s: %s\n", user_shell,
		  safe_strerror (errno));
      _exit (0177);
    }

  if (pid != -1)
    waitpid (pid, &status, 0);
  else
    error (_("Fork failed"));
  return status;
#endif /* Can fork.  */
}

/* Escape out to the shell to run ARG.  If ARG is NULL, launch and
   interactive shell.  Sets $_shell_exitcode and $_shell_exitsignal
   convenience variables based on the exits status.  */

static void
shell_escape (const char *arg, int from_tty)
{
  int status = run_under_shell (arg, from_tty);
  exit_status_set_internal_vars (status);
}

/* Implementation of the "shell" command.  */

static void
shell_command (const char *arg, int from_tty)
{
  shell_escape (arg, from_tty);
}

static void
edit_command (const char *arg, int from_tty)
{
  struct symtab_and_line sal;
  struct symbol *sym;
  const char *editor;
  const char *fn;

  /* Pull in the current default source line if necessary.  */
  if (arg == 0)
    {
      set_default_source_symtab_and_line ();
      sal = get_current_source_symtab_and_line ();
    }

  /* Bare "edit" edits file with present line.  */

  if (arg == 0)
    {
      if (sal.symtab == 0)
	error (_("No default source file yet."));
      sal.line += get_lines_to_list () / 2;
    }
  else
    {
      const char *arg1;

      /* Now should only be one argument -- decode it in SAL.  */
      arg1 = arg;
      location_spec_up locspec = string_to_location_spec (&arg1,
							  current_language);

      if (*arg1)
	error (_("Junk at end of line specification."));

      std::vector<symtab_and_line> sals = decode_line_1 (locspec.get (),
							 DECODE_LINE_LIST_MODE,
							 NULL, NULL, 0);

      filter_sals (sals);
      if (sals.empty ())
	{
	  /*  C++  */
	  return;
	}
      if (sals.size () > 1)
	{
	  ambiguous_line_spec (sals,
			       _("Specified line is ambiguous:\n"));
	  return;
	}

      sal = sals[0];

      /* If line was specified by address, first print exactly which
	 line, and which file.  In this case, sal.symtab == 0 means
	 address is outside of all known source files, not that user
	 failed to give a filename.  */
      if (*arg == '*')
	{
	  struct gdbarch *gdbarch;

	  if (sal.symtab == 0)
	    error (_("No source file for address %s."),
		   paddress (get_current_arch (), sal.pc));

	  gdbarch = sal.symtab->compunit ()->objfile ()->arch ();
	  sym = find_pc_function (sal.pc);
	  if (sym)
	    gdb_printf ("%s is in %s (%s:%d).\n",
			paddress (gdbarch, sal.pc),
			sym->print_name (),
			symtab_to_filename_for_display (sal.symtab),
			sal.line);
	  else
	    gdb_printf ("%s is at %s:%d.\n",
			paddress (gdbarch, sal.pc),
			symtab_to_filename_for_display (sal.symtab),
			sal.line);
	}

      /* If what was given does not imply a symtab, it must be an
	 undebuggable symbol which means no source code.  */

      if (sal.symtab == 0)
	error (_("No line number known for %s."), arg);
    }

  if ((editor = getenv ("EDITOR")) == NULL)
    editor = "/bin/ex";

  fn = symtab_to_fullname (sal.symtab);

  /* Quote the file name, in case it has whitespace or other special
     characters.  */
  gdb::unique_xmalloc_ptr<char> p
    = xstrprintf ("%s +%d \"%s\"", editor, sal.line, fn);
  shell_escape (p.get (), from_tty);
}

/* The options for the "pipe" command.  */

struct pipe_cmd_opts
{
  /* For "-d".  */
  std::string delimiter;
};

static const gdb::option::option_def pipe_cmd_option_defs[] = {

  gdb::option::string_option_def<pipe_cmd_opts> {
    "d",
    [] (pipe_cmd_opts *opts) { return &opts->delimiter; },
    nullptr,
    N_("Indicates to use the specified delimiter string to separate\n\
COMMAND from SHELL_COMMAND, in alternative to |.  This is useful in\n\
case COMMAND contains a | character."),
  },

};

/* Create an option_def_group for the "pipe" command's options, with
   OPTS as context.  */

static inline gdb::option::option_def_group
make_pipe_cmd_options_def_group (pipe_cmd_opts *opts)
{
  return {{pipe_cmd_option_defs}, opts};
}

/* Implementation of the "pipe" command.  */

static void
pipe_command (const char *arg, int from_tty)
{
  pipe_cmd_opts opts;

  auto grp = make_pipe_cmd_options_def_group (&opts);
  gdb::option::process_options
    (&arg, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp);

  const char *delim = "|";
  if (!opts.delimiter.empty ())
    delim = opts.delimiter.c_str ();

  const char *command = arg;
  if (command == nullptr)
    error (_("Missing COMMAND"));

  arg = strstr (arg, delim);

  if (arg == nullptr)
    error (_("Missing delimiter before SHELL_COMMAND"));

  std::string gdb_cmd (command, arg - command);

  arg += strlen (delim); /* Skip the delimiter.  */

  if (gdb_cmd.empty ())
    gdb_cmd = repeat_previous ();

  const char *shell_command = skip_spaces (arg);
  if (*shell_command == '\0')
    error (_("Missing SHELL_COMMAND"));

  FILE *to_shell_command = popen (shell_command, "w");

  if (to_shell_command == nullptr)
    error (_("Error launching \"%s\""), shell_command);

  try
    {
      stdio_file pipe_file (to_shell_command);

      execute_command_to_ui_file (&pipe_file, gdb_cmd.c_str (), from_tty);
    }
  catch (...)
    {
      pclose (to_shell_command);
      throw;
    }

  int exit_status = pclose (to_shell_command);

  if (exit_status < 0)
    error (_("shell command \"%s\" failed: %s"), shell_command,
	   safe_strerror (errno));
  exit_status_set_internal_vars (exit_status);
}

/* Completer for the pipe command.  */

static void
pipe_command_completer (struct cmd_list_element *ignore,
			completion_tracker &tracker,
			const char *text, const char *word_ignored)
{
  pipe_cmd_opts opts;

  const char *org_text = text;
  auto grp = make_pipe_cmd_options_def_group (&opts);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp))
    return;

  const char *delimiter = "|";
  if (!opts.delimiter.empty ())
    delimiter = opts.delimiter.c_str ();

  /* Check if we're past option values already.  */
  if (text > org_text && !isspace (text[-1]))
    return;

  const char *delim = strstr (text, delimiter);

  /* If we're still not past the delimiter, complete the gdb
     command.  */
  if (delim == nullptr || delim == text)
    {
      complete_nested_command_line (tracker, text);
      return;
    }

  /* We're past the delimiter.  What follows is a shell command, which
     we don't know how to complete.  */
}

/* Helper for the list_command function.  Prints the lines around (and
   including) line stored in CURSAL.  ARG contains the arguments used in
   the command invocation, and is used to determine a special case when
   printing backwards.  */
static void
list_around_line (const char *arg, symtab_and_line cursal)
{
  int first;

  first = std::max (cursal.line - get_lines_to_list () / 2, 1);

  /* A small special case --- if listing backwards, and we
     should list only one line, list the preceding line,
     instead of the exact line we've just shown after e.g.,
     stopping for a breakpoint.  */
  if (arg != NULL && arg[0] == '-'
      && get_lines_to_list () == 1 && first > 1)
    first -= 1;

  print_source_lines (cursal.symtab, source_lines_range (first), 0);
}

static void
list_command (const char *arg, int from_tty)
{
  struct symbol *sym;
  const char *arg1;
  int no_end = 1;
  int dummy_end = 0;
  int dummy_beg = 0;
  int linenum_beg = 0;
  const char *p;

  /* Pull in the current default source line if necessary.  */
  if (arg == NULL || ((arg[0] == '+' || arg[0] == '-' || arg[0] == '.') && arg[1] == '\0'))
    {
      set_default_source_symtab_and_line ();
      symtab_and_line cursal = get_current_source_symtab_and_line ();

      /* If this is the first "list" since we've set the current
	 source line, center the listing around that line.  */
      if (get_first_line_listed () == 0 && (arg == nullptr || arg[0] != '.'))
	{
	  list_around_line (arg, cursal);
	}

      /* "l" and "l +" lists the next few lines, unless we're listing past
	 the end of the file.  */
      else if (arg == nullptr || arg[0] == '+')
	{
	  if (last_symtab_line (cursal.symtab) >= cursal.line)
	    print_source_lines (cursal.symtab,
				source_lines_range (cursal.line), 0);
	  else
	    {
	      error (_("End of the file was already reached, use \"list .\" to"
		       " list the current location again"));
	    }
	}

      /* "l -" lists previous ten lines, the ones before the ten just
	 listed.  */
      else if (arg[0] == '-')
	{
	  if (get_first_line_listed () == 1)
	    error (_("Already at the start of %s."),
		   symtab_to_filename_for_display (cursal.symtab));
	  source_lines_range range (get_first_line_listed (),
				    source_lines_range::BACKWARD);
	  print_source_lines (cursal.symtab, range, 0);
	}

      /* "list ." lists the default location again.  */
      else if (arg[0] == '.')
	{
	  if (target_has_stack ())
	    {
	      /* Find the current line by getting the PC of the currently
		 selected frame, and finding the line associated to it.  */
	      frame_info_ptr frame = get_selected_frame (nullptr);
	      CORE_ADDR curr_pc = get_frame_pc (frame);
	      cursal = find_pc_line (curr_pc, 0);
	    }
	  else
	    {
	      /* The inferior is not running, so reset the current source
		 location to the default (usually the main function).  */
	      clear_current_source_symtab_and_line ();
	      set_default_source_symtab_and_line ();
	      cursal = get_current_source_symtab_and_line ();
	    }
	  list_around_line (arg, cursal);
	  /* Set the repeat args so just pressing "enter" after using "list ."
	     will print the following lines instead of the same lines again. */
	  if (from_tty)
	    set_repeat_arguments ("");
	}

      return;
    }

  /* Now if there is only one argument, decode it in SAL
     and set NO_END.
     If there are two arguments, decode them in SAL and SAL_END
     and clear NO_END; however, if one of the arguments is blank,
     set DUMMY_BEG or DUMMY_END to record that fact.  */

  if (!have_full_symbols () && !have_partial_symbols ())
    error (_("No symbol table is loaded.  Use the \"file\" command."));

  std::vector<symtab_and_line> sals;
  symtab_and_line sal, sal_end;

  arg1 = arg;
  if (*arg1 == ',')
    dummy_beg = 1;
  else
    {
      location_spec_up locspec
	= string_to_location_spec (&arg1, current_language);

      /* We know that the ARG string is not empty, yet the attempt to
	 parse a location spec from the string consumed no characters.
	 This most likely means that the first thing in ARG looks like
	 a location spec condition, and so the string_to_location_spec
	 call stopped parsing.  */
      if (arg1 == arg)
	error (_("Junk at end of line specification."));

      sals = decode_line_1 (locspec.get (), DECODE_LINE_LIST_MODE,
			    NULL, NULL, 0);
      filter_sals (sals);
      if (sals.empty ())
	{
	  /*  C++  */
	  return;
	}

      sal = sals[0];
    }

  /* Record whether the BEG arg is all digits.  */

  for (p = arg; p != arg1 && *p >= '0' && *p <= '9'; p++);
  linenum_beg = (p == arg1);

  /* Save the range of the first argument, in case we need to let the
     user know it was ambiguous.  */
  const char *beg = arg;
  size_t beg_len = arg1 - beg;

  while (*arg1 == ' ' || *arg1 == '\t')
    arg1++;
  if (*arg1 == ',')
    {
      no_end = 0;
      if (sals.size () > 1)
	{
	  ambiguous_line_spec (sals,
			       _("Specified first line '%.*s' is ambiguous:\n"),
			       (int) beg_len, beg);
	  return;
	}
      arg1++;
      while (*arg1 == ' ' || *arg1 == '\t')
	arg1++;
      if (*arg1 == 0)
	dummy_end = 1;
      else
	{
	  /* Save the last argument, in case we need to let the user
	     know it was ambiguous.  */
	  const char *end_arg = arg1;

	  location_spec_up locspec
	    = string_to_location_spec (&arg1, current_language);

	  if (*arg1)
	    error (_("Junk at end of line specification."));

	  std::vector<symtab_and_line> sals_end
	    = (dummy_beg
	       ? decode_line_1 (locspec.get (), DECODE_LINE_LIST_MODE,
				NULL, NULL, 0)
	       : decode_line_1 (locspec.get (), DECODE_LINE_LIST_MODE,
				NULL, sal.symtab, sal.line));

	  filter_sals (sals_end);
	  if (sals_end.empty ())
	    return;
	  if (sals_end.size () > 1)
	    {
	      ambiguous_line_spec (sals_end,
				   _("Specified last line '%s' is ambiguous:\n"),
				   end_arg);
	      return;
	    }
	  sal_end = sals_end[0];
	}
    }

  if (*arg1)
    error (_("Junk at end of line specification."));

  if (!no_end && !dummy_beg && !dummy_end
      && sal.symtab != sal_end.symtab)
    error (_("Specified first and last lines are in different files."));
  if (dummy_beg && dummy_end)
    error (_("Two empty args do not say what lines to list."));

  /* If line was specified by address,
     first print exactly which line, and which file.

     In this case, sal.symtab == 0 means address is outside of all
     known source files, not that user failed to give a filename.  */
  if (*arg == '*')
    {
      struct gdbarch *gdbarch;

      if (sal.symtab == 0)
	error (_("No source file for address %s."),
	       paddress (get_current_arch (), sal.pc));

      gdbarch = sal.symtab->compunit ()->objfile ()->arch ();
      sym = find_pc_function (sal.pc);
      if (sym)
	gdb_printf ("%s is in %s (%s:%d).\n",
		    paddress (gdbarch, sal.pc),
		    sym->print_name (),
		    symtab_to_filename_for_display (sal.symtab), sal.line);
      else
	gdb_printf ("%s is at %s:%d.\n",
		    paddress (gdbarch, sal.pc),
		    symtab_to_filename_for_display (sal.symtab), sal.line);
    }

  /* If line was not specified by just a line number, and it does not
     imply a symtab, it must be an undebuggable symbol which means no
     source code.  */

  if (!linenum_beg && sal.symtab == 0)
    error (_("No line number known for %s."), arg);

  /* If this command is repeated with RET,
     turn it into the no-arg variant.  */

  if (from_tty)
    set_repeat_arguments ("");

  if (dummy_beg && sal_end.symtab == 0)
    error (_("No default source file yet.  Do \"help list\"."));
  if (dummy_beg)
    {
      source_lines_range range (sal_end.line + 1,
				source_lines_range::BACKWARD);
      print_source_lines (sal_end.symtab, range, 0);
    }
  else if (sal.symtab == 0)
    error (_("No default source file yet.  Do \"help list\"."));
  else if (no_end)
    {
      for (int i = 0; i < sals.size (); i++)
	{
	  sal = sals[i];
	  int first_line = sal.line - get_lines_to_list () / 2;
	  if (first_line < 1)
	    first_line = 1;
	  if (sals.size () > 1)
	    print_sal_location (sal);
	  print_source_lines (sal.symtab, source_lines_range (first_line), 0);
	}
    }
  else if (dummy_end)
    print_source_lines (sal.symtab, source_lines_range (sal.line), 0);
  else
    print_source_lines (sal.symtab,
			source_lines_range (sal.line, (sal_end.line + 1)),
			0);
}

/* Subroutine of disassemble_command to simplify it.
   Perform the disassembly.
   NAME is the name of the function if known, or NULL.
   [LOW,HIGH) are the range of addresses to disassemble.
   BLOCK is the block to disassemble; it needs to be provided
   when non-contiguous blocks are disassembled; otherwise
   it can be NULL.
   MIXED is non-zero to print source with the assembler.  */

static void
print_disassembly (struct gdbarch *gdbarch, const char *name,
		   CORE_ADDR low, CORE_ADDR high,
		   const struct block *block,
		   gdb_disassembly_flags flags)
{
#if defined(TUI)
  if (tui_is_window_visible (DISASSEM_WIN))
    tui_show_assembly (gdbarch, low);
  else
#endif
    {
      gdb_printf (_("Dump of assembler code "));
      if (name != NULL)
	gdb_printf (_("for function %ps:\n"),
		    styled_string (function_name_style.style (), name));
      if (block == nullptr || block->is_contiguous ())
	{
	  if (name == NULL)
	    gdb_printf (_("from %ps to %ps:\n"),
			styled_string (address_style.style (),
				       paddress (gdbarch, low)),
			styled_string (address_style.style (),
				       paddress (gdbarch, high)));

	  /* Dump the specified range.  */
	  gdb_disassembly (gdbarch, current_uiout, flags, -1, low, high);
	}
      else
	{
	  for (const blockrange &range : block->ranges ())
	    {
	      CORE_ADDR range_low = range.start ();
	      CORE_ADDR range_high = range.end ();

	      gdb_printf (_("Address range %ps to %ps:\n"),
			  styled_string (address_style.style (),
					 paddress (gdbarch, range_low)),
			  styled_string (address_style.style (),
					 paddress (gdbarch, range_high)));
	      gdb_disassembly (gdbarch, current_uiout, flags, -1,
			       range_low, range_high);
	    }
	}
      gdb_printf (_("End of assembler dump.\n"));
    }
}

/* Subroutine of disassemble_command to simplify it.
   Print a disassembly of the current function according to FLAGS.  */

static void
disassemble_current_function (gdb_disassembly_flags flags)
{
  frame_info_ptr frame;
  struct gdbarch *gdbarch;
  CORE_ADDR low, high, pc;
  const char *name;
  const struct block *block;

  frame = get_selected_frame (_("No frame selected."));
  gdbarch = get_frame_arch (frame);
  pc = get_frame_address_in_block (frame);
  if (find_pc_partial_function (pc, &name, &low, &high, &block) == 0)
    error (_("No function contains program counter for selected frame."));
#if defined(TUI)
  /* NOTE: cagney/2003-02-13 The `tui_active' was previously
     `tui_version'.  */
  if (tui_active)
    /* FIXME: cagney/2004-02-07: This should be an observer.  */
    low = tui_get_low_disassembly_address (gdbarch, low, pc);
#endif
  low += gdbarch_deprecated_function_start_offset (gdbarch);

  print_disassembly (gdbarch, name, low, high, block, flags);
}

/* Dump a specified section of assembly code.

   Usage:
     disassemble [/mrs]
       - dump the assembly code for the function of the current pc
     disassemble [/mrs] addr
       - dump the assembly code for the function at ADDR
     disassemble [/mrs] low,high
     disassemble [/mrs] low,+length
       - dump the assembly code in the range [LOW,HIGH), or [LOW,LOW+length)

   A /m modifier will include source code with the assembly in a
   "source centric" view.  This view lists only the file of the first insn,
   even if other source files are involved (e.g., inlined functions), and
   the output is in source order, even with optimized code.  This view is
   considered deprecated as it hasn't been useful in practice.

   A /r modifier will include raw instructions in hex with the assembly.

   A /b modifier is similar to /r except the instruction bytes are printed
   as separate bytes with no grouping, or endian switching.

   A /s modifier will include source code with the assembly, like /m, with
   two important differences:
   1) The output is still in pc address order.
   2) File names and contents for all relevant source files are displayed.  */

static void
disassemble_command (const char *arg, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  CORE_ADDR low, high;
  const general_symbol_info *symbol = nullptr;
  const char *name;
  CORE_ADDR pc;
  gdb_disassembly_flags flags;
  const char *p;
  const struct block *block = nullptr;

  p = arg;
  name = NULL;
  flags = 0;

  if (p && *p == '/')
    {
      ++p;

      if (*p == '\0')
	error (_("Missing modifier."));

      while (*p && ! isspace (*p))
	{
	  switch (*p++)
	    {
	    case 'm':
	      flags |= DISASSEMBLY_SOURCE_DEPRECATED;
	      break;
	    case 'r':
	      flags |= DISASSEMBLY_RAW_INSN;
	      break;
	    case 'b':
	      flags |= DISASSEMBLY_RAW_BYTES;
	      break;
	    case 's':
	      flags |= DISASSEMBLY_SOURCE;
	      break;
	    default:
	      error (_("Invalid disassembly modifier."));
	    }
	}

      p = skip_spaces (p);
    }

  if ((flags & (DISASSEMBLY_SOURCE_DEPRECATED | DISASSEMBLY_SOURCE))
      == (DISASSEMBLY_SOURCE_DEPRECATED | DISASSEMBLY_SOURCE))
    error (_("Cannot specify both /m and /s."));

  if ((flags & (DISASSEMBLY_RAW_INSN | DISASSEMBLY_RAW_BYTES))
      == (DISASSEMBLY_RAW_INSN | DISASSEMBLY_RAW_BYTES))
    error (_("Cannot specify both /r and /b."));

  if (! p || ! *p)
    {
      flags |= DISASSEMBLY_OMIT_FNAME;
      disassemble_current_function (flags);
      return;
    }

  pc = value_as_address (parse_to_comma_and_eval (&p));
  if (p[0] == ',')
    ++p;
  if (p[0] == '\0')
    {
      /* One argument.  */
      if (!find_pc_partial_function_sym (pc, &symbol, &low, &high, &block))
	error (_("No function contains specified address."));

      if (asm_demangle)
	name = symbol->print_name ();
      else
	name = symbol->linkage_name ();

#if defined(TUI)
      /* NOTE: cagney/2003-02-13 The `tui_active' was previously
	 `tui_version'.  */
      if (tui_active)
	/* FIXME: cagney/2004-02-07: This should be an observer.  */
	low = tui_get_low_disassembly_address (gdbarch, low, pc);
#endif
      low += gdbarch_deprecated_function_start_offset (gdbarch);
      flags |= DISASSEMBLY_OMIT_FNAME;
    }
  else
    {
      /* Two arguments.  */
      int incl_flag = 0;
      low = pc;
      p = skip_spaces (p);
      if (p[0] == '+')
	{
	  ++p;
	  incl_flag = 1;
	}
      high = parse_and_eval_address (p);
      if (incl_flag)
	high += low;
    }

  print_disassembly (gdbarch, name, low, high, block, flags);
}

/* Command completion for the disassemble command.  */

static void
disassemble_command_completer (struct cmd_list_element *ignore,
			       completion_tracker &tracker,
			       const char *text, const char * /* word */)
{
  if (skip_over_slash_fmt (tracker, &text))
    return;

  const char *word = advance_to_expression_complete_word_point (tracker, text);
  expression_completer (ignore, tracker, text, word);
}

static void
make_command (const char *arg, int from_tty)
{
  if (arg == 0)
    shell_escape ("make", from_tty);
  else
    {
      std::string cmd = std::string ("make ") + arg;

      shell_escape (cmd.c_str (), from_tty);
    }
}

static void
show_user (const char *args, int from_tty)
{
  struct cmd_list_element *c;

  if (args)
    {
      const char *comname = args;

      c = lookup_cmd (&comname, cmdlist, "", NULL, 0, 1);
      if (!cli_user_command_p (c))
	error (_("Not a user command."));
      show_user_1 (c, "", args, gdb_stdout);
    }
  else
    {
      for (c = cmdlist; c; c = c->next)
	{
	  if (cli_user_command_p (c) || c->is_prefix ())
	    show_user_1 (c, "", c->name, gdb_stdout);
	}
    }
}

/* Return true if COMMAND or any of its sub-commands is a user defined command.
   This is a helper function for show_user_completer.  */

static bool
has_user_subcmd (struct cmd_list_element *command)
{
  if (cli_user_command_p (command))
    return true;

  /* Alias command can yield false positive.  Ignore them as the targeted
     command should be reachable anyway.  */
  if (command->is_alias ())
    return false;

  if (command->is_prefix ())
    for (struct cmd_list_element *subcommand = *command->subcommands;
	 subcommand != nullptr;
	 subcommand = subcommand->next)
      if (has_user_subcmd (subcommand))
	return true;

  return false;
}

/* Implement completer for the 'show user' command.  */

static void
show_user_completer (cmd_list_element *,
		     completion_tracker &tracker, const char *text,
		     const char *word)
{
  struct cmd_list_element *cmd_group = cmdlist;

  /* TEXT can contain a chain of commands and subcommands.  Follow the
     commands chain until we reach the point where the user wants a
     completion.  */
  while (word > text)
    {
      const char *curr_cmd = text;
      const char *after = skip_to_space (text);
      const size_t curr_cmd_len = after - text;
      text = skip_spaces (after);

      for (struct cmd_list_element *c = cmd_group; c != nullptr; c = c->next)
	{
	  if (strlen (c->name) == curr_cmd_len
	      && strncmp (c->name, curr_cmd, curr_cmd_len) == 0)
	    {
	      if (c->subcommands == nullptr)
		/* We arrived after a command with no child, so nothing more
		   to complete.  */
		return;

	      cmd_group = *c->subcommands;
	      break;
	    }
	}
    }

  const int wordlen = strlen (word);
  for (struct cmd_list_element *c = cmd_group; c != nullptr; c = c->next)
    if (has_user_subcmd (c))
      {
	if (strncmp (c->name, word, wordlen) == 0)
	  tracker.add_completion
	    (gdb::unique_xmalloc_ptr<char> (xstrdup (c->name)));
      }
}

/* Search through names of commands and documentations for a certain
   regular expression.  */

static void
apropos_command (const char *arg, int from_tty)
{
  bool verbose = arg && check_for_argument (&arg, "-v", 2);

  if (arg == NULL || *arg == '\0')
    error (_("REGEXP string is empty"));

  compiled_regex pattern (arg, REG_ICASE,
			  _("Error in regular expression"));

  apropos_cmd (gdb_stdout, cmdlist, verbose, pattern);
}

/* The options for the "alias" command.  */

struct alias_opts
{
  /* For "-a".  */
  bool abbrev_flag = false;
};

static const gdb::option::option_def alias_option_defs[] = {

  gdb::option::flag_option_def<alias_opts> {
    "a",
    [] (alias_opts *opts) { return &opts->abbrev_flag; },
    N_("Specify that ALIAS is an abbreviation of COMMAND.\n\
Abbreviations are not used in command completion."),
  },

};

/* Create an option_def_group for the "alias" options, with
   A_OPTS as context.  */

static gdb::option::option_def_group
make_alias_options_def_group (alias_opts *a_opts)
{
  return {{alias_option_defs}, a_opts};
}

/* Completer for the "alias_command".  */

static void
alias_command_completer (struct cmd_list_element *ignore,
			 completion_tracker &tracker,
			 const char *text, const char *word)
{
  const auto grp = make_alias_options_def_group (nullptr);

  tracker.set_use_custom_word_point (true);

  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp))
    return;

  const char *delim = strchr (text, '=');

  /* If we're past the "=" delimiter, complete the
     "alias ALIAS = COMMAND [DEFAULT-ARGS...]" as if the user is
     typing COMMAND DEFAULT-ARGS...  */
  if (delim != text
      && delim != nullptr
      && isspace (delim[-1])
      && (isspace (delim[1]) || delim[1] == '\0'))
    {
      std::string new_text = std::string (delim + 1);

      tracker.advance_custom_word_point_by (delim + 1 - text);
      complete_nested_command_line (tracker, new_text.c_str ());
      return;
    }

  /* We're not yet past the "=" delimiter.  Complete a command, as
     the user might type an alias following a prefix command.  */
  complete_nested_command_line (tracker, text);
}

/* Subroutine of alias_command to simplify it.
   Return the first N elements of ARGV flattened back to a string
   with a space separating each element.
   ARGV may not be NULL.
   This does not take care of quoting elements in case they contain spaces
   on purpose.  */

static std::string
argv_to_string (char **argv, int n)
{
  int i;
  std::string result;

  gdb_assert (argv != NULL);
  gdb_assert (n >= 0 && n <= countargv (argv));

  for (i = 0; i < n; ++i)
    {
      if (i > 0)
	result += " ";
      result += argv[i];
    }

  return result;
}

/* Subroutine of alias_command to simplify it.
   Verifies that COMMAND can have an alias:
      COMMAND must exist.
      COMMAND must not have default args.
   This last condition is to avoid the following:
     alias aaa = backtrace -full
     alias bbb = aaa -past-main
   as (at least currently), alias default args are not cumulative
   and the user would expect bbb to execute 'backtrace -full -past-main'
   while it will execute 'backtrace -past-main'.  */

static cmd_list_element *
validate_aliased_command (const char *command)
{
  std::string default_args;
  cmd_list_element *c
    = lookup_cmd_1 (& command, cmdlist, NULL, &default_args, 1);

  if (c == NULL || c == (struct cmd_list_element *) -1)
    error (_("Invalid command to alias to: %s"), command);

  if (!default_args.empty ())
    error (_("Cannot define an alias of an alias that has default args"));

  return c;
}

/* Called when "alias" was incorrectly used.  */

static void
alias_usage_error (void)
{
  error (_("Usage: alias [-a] [--] ALIAS = COMMAND [DEFAULT-ARGS...]"));
}

/* Make an alias of an existing command.  */

static void
alias_command (const char *args, int from_tty)
{
  alias_opts a_opts;

  auto grp = make_alias_options_def_group (&a_opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp);

  int i, alias_argc, command_argc;
  const char *equals;
  const char *alias, *command;

  if (args == NULL || strchr (args, '=') == NULL)
    alias_usage_error ();

  equals = strchr (args, '=');
  std::string args2 (args, equals - args);

  gdb_argv built_alias_argv (args2.c_str ());

  const char *default_args = equals + 1;
  struct cmd_list_element *c_command_prefix;

  lookup_cmd_for_default_args (&default_args, &c_command_prefix);
  std::string command_argv_str (equals + 1,
				default_args == nullptr
				? strlen (equals + 1)
				: default_args - equals - 1);
  gdb_argv command_argv (command_argv_str.c_str ());

  char **alias_argv = built_alias_argv.get ();

  if (alias_argv[0] == NULL || command_argv[0] == NULL
      || *alias_argv[0] == '\0' || *command_argv[0] == '\0')
    alias_usage_error ();

  for (i = 0; alias_argv[i] != NULL; ++i)
    {
      if (! valid_user_defined_cmd_name_p (alias_argv[i]))
	{
	  if (i == 0)
	    error (_("Invalid command name: %s"), alias_argv[i]);
	  else
	    error (_("Invalid command element name: %s"), alias_argv[i]);
	}
    }

  alias_argc = countargv (alias_argv);
  command_argc = command_argv.count ();

  /* COMMAND must exist, and cannot have default args.
     Reconstruct the command to remove any extraneous spaces,
     for better error messages.  */
  std::string command_string (argv_to_string (command_argv.get (),
					      command_argc));
  command = command_string.c_str ();
  cmd_list_element *target_cmd = validate_aliased_command (command);

  /* ALIAS must not exist.  */
  std::string alias_string (argv_to_string (alias_argv, alias_argc));
  alias = alias_string.c_str ();
  {
    cmd_list_element *alias_cmd, *prefix_cmd, *cmd;

    if (lookup_cmd_composition (alias, &alias_cmd, &prefix_cmd, &cmd))
      {
	const char *alias_name = alias_argv[alias_argc-1];

	/* If we found an existing ALIAS_CMD, check that the prefix differ or
	   the name differ.  */

	if (alias_cmd != nullptr
	    && alias_cmd->prefix == prefix_cmd
	    && strcmp (alias_name, alias_cmd->name) == 0)
	  error (_("Alias already exists: %s"), alias);

	/* Check ALIAS differs from the found CMD.  */

	if (cmd->prefix == prefix_cmd
	    && strcmp (alias_name, cmd->name) == 0)
	  error (_("Alias %s is the name of an existing command"), alias);
      }
  }


  struct cmd_list_element *alias_cmd;

  /* If ALIAS is one word, it is an alias for the entire COMMAND.
     Example: alias spe = set print elements

     Otherwise ALIAS and COMMAND must have the same number of words,
     and every word except the last must identify the same prefix command;
     and the last word of ALIAS is made an alias of the last word of COMMAND.
     Example: alias set print elms = set pr elem
     Note that unambiguous abbreviations are allowed.  */

  if (alias_argc == 1)
    {
      /* add_cmd requires *we* allocate space for name, hence the xstrdup.  */
      alias_cmd = add_com_alias (xstrdup (alias_argv[0]), target_cmd,
				 class_alias, a_opts.abbrev_flag);
    }
  else
    {
      const char *alias_prefix, *command_prefix;
      struct cmd_list_element *c_alias, *c_command;

      if (alias_argc != command_argc)
	error (_("Mismatched command length between ALIAS and COMMAND."));

      /* Create copies of ALIAS and COMMAND without the last word,
	 and use that to verify the leading elements give the same
	 prefix command.  */
      std::string alias_prefix_string (argv_to_string (alias_argv,
						       alias_argc - 1));
      std::string command_prefix_string (argv_to_string (command_argv.get (),
							 command_argc - 1));
      alias_prefix = alias_prefix_string.c_str ();
      command_prefix = command_prefix_string.c_str ();

      c_command = lookup_cmd_1 (& command_prefix, cmdlist, NULL, NULL, 1);
      /* We've already tried to look up COMMAND.  */
      gdb_assert (c_command != NULL
		  && c_command != (struct cmd_list_element *) -1);
      gdb_assert (c_command->is_prefix ());
      c_alias = lookup_cmd_1 (& alias_prefix, cmdlist, NULL, NULL, 1);
      if (c_alias != c_command)
	error (_("ALIAS and COMMAND prefixes do not match."));

      /* add_cmd requires *we* allocate space for name, hence the xstrdup.  */
      alias_cmd = add_alias_cmd (xstrdup (alias_argv[alias_argc - 1]),
				 target_cmd, class_alias, a_opts.abbrev_flag,
				 c_command->subcommands);
    }

  gdb_assert (alias_cmd != nullptr);
  gdb_assert (alias_cmd->default_args.empty ());
  if (default_args != nullptr)
    {
      default_args = skip_spaces (default_args);

      alias_cmd->default_args = default_args;
    }
}

/* Print the file / line number / symbol name of the location
   specified by SAL.  */

static void
print_sal_location (const symtab_and_line &sal)
{
  scoped_restore_current_program_space restore_pspace;
  set_current_program_space (sal.pspace);

  const char *sym_name = NULL;
  if (sal.symbol != NULL)
    sym_name = sal.symbol->print_name ();
  gdb_printf (_("file: \"%s\", line number: %d, symbol: \"%s\"\n"),
	      symtab_to_filename_for_display (sal.symtab),
	      sal.line, sym_name != NULL ? sym_name : "???");
}

/* Print a list of files and line numbers which a user may choose from
   in order to list a function which was specified ambiguously (as
   with `list classname::overloadedfuncname', for example).  The SALS
   array provides the filenames and line numbers.  FORMAT is a
   printf-style format string used to tell the user what was
   ambiguous.  */

static void
ambiguous_line_spec (gdb::array_view<const symtab_and_line> sals,
		     const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  gdb_vprintf (format, ap);
  va_end (ap);

  for (const auto &sal : sals)
    print_sal_location (sal);
}

/* Comparison function for filter_sals.  Returns a qsort-style
   result.  */

static int
cmp_symtabs (const symtab_and_line &sala, const symtab_and_line &salb)
{
  const char *dira = sala.symtab->compunit ()->dirname ();
  const char *dirb = salb.symtab->compunit ()->dirname ();
  int r;

  if (dira == NULL)
    {
      if (dirb != NULL)
	return -1;
    }
  else if (dirb == NULL)
    {
      if (dira != NULL)
	return 1;
    }
  else
    {
      r = filename_cmp (dira, dirb);
      if (r)
	return r;
    }

  r = filename_cmp (sala.symtab->filename, salb.symtab->filename);
  if (r)
    return r;

  if (sala.line < salb.line)
    return -1;
  return sala.line == salb.line ? 0 : 1;
}

/* Remove any SALs that do not match the current program space, or
   which appear to be "file:line" duplicates.  */

static void
filter_sals (std::vector<symtab_and_line> &sals)
{
  /* Remove SALs that do not match.  */
  auto from = std::remove_if (sals.begin (), sals.end (),
			      [&] (const symtab_and_line &sal)
    { return (sal.pspace != current_program_space || sal.symtab == NULL); });

  /* Remove dups.  */
  std::sort (sals.begin (), from,
	     [] (const symtab_and_line &sala, const symtab_and_line &salb)
   { return cmp_symtabs (sala, salb) < 0; });

  from = std::unique (sals.begin (), from,
		      [&] (const symtab_and_line &sala,
			   const symtab_and_line &salb)
    { return cmp_symtabs (sala, salb) == 0; });

  sals.erase (from, sals.end ());
}

static void
show_info_verbose (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c,
		   const char *value)
{
  if (info_verbose)
    gdb_printf (file,
		_("Verbose printing of informational messages is %s.\n"),
		value);
  else
    gdb_printf (file, _("Verbosity is %s.\n"), value);
}

static void
show_history_expansion_p (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("History expansion on command input is %s.\n"),
	      value);
}

static void
show_max_user_call_depth (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("The max call depth for user-defined commands is %s.\n"),
	      value);
}

/* Implement 'show suppress-cli-notifications'.  */

static void
show_suppress_cli_notifications (ui_file *file, int from_tty,
				 cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Suppression of printing CLI notifications "
		      "is %s.\n"), value);
}

/* Implement 'set suppress-cli-notifications'.  */

static void
set_suppress_cli_notifications (const char *args, int from_tty,
				cmd_list_element *c)
{
  cli_suppress_notification.user_selected_context
    = user_wants_cli_suppress_notification;
  cli_suppress_notification.normal_stop
    = user_wants_cli_suppress_notification;
}

/* Returns the cmd_list_element in SHOWLIST corresponding to the first
   argument of ARGV, which must contain one single value.
   Throws an error if no value provided, or value not correct.
   FNNAME is used in the error message.  */

static cmd_list_element *
setting_cmd (const char *fnname, struct cmd_list_element *showlist,
	     int argc, struct value **argv)
{
  if (argc == 0)
    error (_("You must provide an argument to %s"), fnname);
  if (argc != 1)
    error (_("You can only provide one argument to %s"), fnname);

  struct type *type0 = check_typedef (argv[0]->type ());

  if (type0->code () != TYPE_CODE_ARRAY
      && type0->code () != TYPE_CODE_STRING)
    error (_("First argument of %s must be a string."), fnname);

  /* Not all languages null-terminate their strings, by moving the string
     content into a std::string we ensure that a null-terminator is added.
     For languages that do add a null-terminator the std::string might end
     up with two null characters at the end, but that's harmless.  */
  const std::string setting ((const char *) argv[0]->contents ().data (),
			     type0->length ());
  const char *a0 = setting.c_str ();
  cmd_list_element *cmd = lookup_cmd (&a0, showlist, "", NULL, -1, 0);

  if (cmd == nullptr || cmd->type != show_cmd)
    {
      gdb_assert (showlist->prefix != nullptr);
      std::vector<std::string> components
	= showlist->prefix->command_components ();
      std::string full_name = components[0];
      for (int i = 1; i < components.size (); ++i)
	full_name += " " + components[i];
      error (_("First argument of %s must be a valid setting of the "
	       "'%s' command."), fnname, full_name.c_str ());
    }

  return cmd;
}

/* Builds a value from the show CMD.  */

static struct value *
value_from_setting (const setting &var, struct gdbarch *gdbarch)
{
  switch (var.type ())
    {
    case var_uinteger:
    case var_integer:
    case var_pinteger:
      {
	LONGEST value
	  = (var.type () == var_uinteger
	     ? static_cast<LONGEST> (var.get<unsigned int> ())
	     : static_cast<LONGEST> (var.get<int> ()));

	if (var.extra_literals () != nullptr)
	  for (const literal_def *l = var.extra_literals ();
	       l->literal != nullptr;
	       l++)
	    if (value == l->use)
	      {
		if (l->val.has_value ())
		  value = *l->val;
		else
		  return value::allocate (builtin_type (gdbarch)->builtin_void);
		break;
	      }

	if (var.type () == var_uinteger)
	  return
	    value_from_ulongest (builtin_type (gdbarch)->builtin_unsigned_int,
				 static_cast<unsigned int> (value));
	else
	  return
	    value_from_longest (builtin_type (gdbarch)->builtin_int,
				static_cast<int> (value));
      }
    case var_boolean:
      return value_from_longest (builtin_type (gdbarch)->builtin_int,
				 var.get<bool> () ? 1 : 0);
    case var_auto_boolean:
      {
	int val;

	switch (var.get<enum auto_boolean> ())
	  {
	  case AUTO_BOOLEAN_TRUE:
	    val = 1;
	    break;
	  case AUTO_BOOLEAN_FALSE:
	    val = 0;
	    break;
	  case AUTO_BOOLEAN_AUTO:
	    val = -1;
	    break;
	  default:
	    gdb_assert_not_reached ("invalid var_auto_boolean");
	  }
	return value_from_longest (builtin_type (gdbarch)->builtin_int,
				   val);
      }
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      {
	const char *value;
	size_t len;
	if (var.type () == var_enum)
	  {
	    value = var.get<const char *> ();
	    len = strlen (value);
	  }
	else
	  {
	    const std::string &st = var.get<std::string> ();
	    value = st.c_str ();
	    len = st.length ();
	  }

	return current_language->value_string (gdbarch, value, len);
      }
    default:
      gdb_assert_not_reached ("bad var_type");
    }
}

/* Implementation of the convenience function $_gdb_setting.  */

static struct value *
gdb_setting_internal_fn (struct gdbarch *gdbarch,
			 const struct language_defn *language,
			 void *cookie, int argc, struct value **argv)
{
  cmd_list_element *show_cmd
    = setting_cmd ("$_gdb_setting", showlist, argc, argv);

  gdb_assert (show_cmd->var.has_value ());

  return value_from_setting (*show_cmd->var, gdbarch);
}

/* Implementation of the convenience function $_gdb_maint_setting.  */

static struct value *
gdb_maint_setting_internal_fn (struct gdbarch *gdbarch,
			       const struct language_defn *language,
			       void *cookie, int argc, struct value **argv)
{
  cmd_list_element *show_cmd
    = setting_cmd ("$_gdb_maint_setting", maintenance_show_cmdlist, argc, argv);

  gdb_assert (show_cmd->var.has_value ());

  return value_from_setting (*show_cmd->var, gdbarch);
}

/* Builds a string value from the show CMD.  */

static struct value *
str_value_from_setting (const setting &var, struct gdbarch *gdbarch)
{
  switch (var.type ())
    {
    case var_uinteger:
    case var_integer:
    case var_pinteger:
    case var_boolean:
    case var_auto_boolean:
      {
	std::string cmd_val = get_setshow_command_value_string (var);

	return current_language->value_string (gdbarch, cmd_val.c_str (),
					       cmd_val.size ());
      }

    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      /* For these cases, we do not use get_setshow_command_value_string,
	 as this function handle some characters specially, e.g. by
	 escaping quotevar.  So, we directly use the var string value,
	 similarly to the value_from_setting code for these casevar.  */
      {
	const char *value;
	size_t len;
	if (var.type () == var_enum)
	  {
	    value = var.get<const char *> ();
	    len = strlen (value);
	  }
	else
	  {
	    const std::string &st = var.get<std::string> ();
	    value = st.c_str ();
	    len = st.length ();
	  }

	return current_language->value_string (gdbarch, value, len);
      }
    default:
      gdb_assert_not_reached ("bad var_type");
    }
}

/* Implementation of the convenience function $_gdb_setting_str.  */

static struct value *
gdb_setting_str_internal_fn (struct gdbarch *gdbarch,
			     const struct language_defn *language,
			     void *cookie, int argc, struct value **argv)
{
  cmd_list_element *show_cmd
    = setting_cmd ("$_gdb_setting_str", showlist, argc, argv);

  gdb_assert (show_cmd->var.has_value ());

  return str_value_from_setting (*show_cmd->var, gdbarch);
}


/* Implementation of the convenience function $_gdb_maint_setting_str.  */

static struct value *
gdb_maint_setting_str_internal_fn (struct gdbarch *gdbarch,
				   const struct language_defn *language,
				   void *cookie, int argc, struct value **argv)
{
  cmd_list_element *show_cmd
    = setting_cmd ("$_gdb_maint_setting_str", maintenance_show_cmdlist, argc,
		   argv);

  gdb_assert (show_cmd->var.has_value ());

  return str_value_from_setting (*show_cmd->var, gdbarch);
}

/* Implementation of the convenience function $_shell.  */

static struct value *
shell_internal_fn (struct gdbarch *gdbarch,
		   const struct language_defn *language,
		   void *cookie, int argc, struct value **argv)
{
  if (argc != 1)
    error (_("You must provide one argument for $_shell."));

  value *val = argv[0];
  struct type *type = check_typedef (val->type ());

  if (!language->is_string_type_p (type))
    error (_("Argument must be a string."));

  value_print_options opts;
  get_no_prettyformat_print_options (&opts);

  string_file stream;
  value_print (val, &stream, &opts);

  /* We should always have two quote chars, which we'll strip.  */
  gdb_assert (stream.size () >= 2);

  /* Now strip them.  We don't need the original string, so it's
     cheaper to do it in place, avoiding a string allocation.  */
  std::string str = stream.release ();
  str[str.size () - 1] = 0;
  const char *command = str.c_str () + 1;

  int exit_status = run_under_shell (command, 0);

  struct type *int_type = builtin_type (gdbarch)->builtin_int;

  /* Keep the logic here in sync with
     exit_status_set_internal_vars.  */

  if (WIFEXITED (exit_status))
    return value_from_longest (int_type, WEXITSTATUS (exit_status));
#ifdef __MINGW32__
  else if (WIFSIGNALED (exit_status) && WTERMSIG (exit_status) == -1)
    {
      /* See exit_status_set_internal_vars.  */
      return value_from_longest (int_type, exit_status);
    }
#endif
  else if (WIFSIGNALED (exit_status))
    {
      /* (0x80 | SIGNO) is what most (all?) POSIX-like shells set as
	 exit code on fatal signal termination.  */
      return value_from_longest (int_type, 0x80 | WTERMSIG (exit_status));
    }
  else
    return value::allocate_optimized_out (int_type);
}

void _initialize_cli_cmds ();
void
_initialize_cli_cmds ()
{
  struct cmd_list_element *c;

  /* Define the classes of commands.
     They will appear in the help list in alphabetical order.  */

  add_cmd ("internals", class_maintenance, _("\
Maintenance commands.\n\
Some gdb commands are provided just for use by gdb maintainers.\n\
These commands are subject to frequent change, and may not be as\n\
well documented as user commands."),
	   &cmdlist);
  add_cmd ("obscure", class_obscure, _("Obscure features."), &cmdlist);
  add_cmd ("aliases", class_alias,
	   _("User-defined aliases of other commands."), &cmdlist);
  add_cmd ("user-defined", class_user, _("\
User-defined commands.\n\
The commands in this class are those defined by the user.\n\
Use the \"define\" command to define a command."), &cmdlist);
  add_cmd ("support", class_support, _("Support facilities."), &cmdlist);
  add_cmd ("status", class_info, _("Status inquiries."), &cmdlist);
  add_cmd ("files", class_files, _("Specifying and examining files."),
	   &cmdlist);
  add_cmd ("breakpoints", class_breakpoint,
	   _("Making program stop at certain points."), &cmdlist);
  add_cmd ("data", class_vars, _("Examining data."), &cmdlist);
  add_cmd ("stack", class_stack, _("\
Examining the stack.\n\
The stack is made up of stack frames.  Gdb assigns numbers to stack frames\n\
counting from zero for the innermost (currently executing) frame.\n\n\
At any time gdb identifies one frame as the \"selected\" frame.\n\
Variable lookups are done with respect to the selected frame.\n\
When the program being debugged stops, gdb selects the innermost frame.\n\
The commands below can be used to select other frames by number or address."),
	   &cmdlist);
#ifdef TUI
  add_cmd ("text-user-interface", class_tui,
	   _("TUI is the GDB text based interface.\n\
In TUI mode, GDB can display several text windows showing\n\
the source file, the processor registers, the program disassembly, ..."), &cmdlist);
#endif
  add_cmd ("running", class_run, _("Running the program."), &cmdlist);

  /* Define general commands.  */

  add_com ("pwd", class_files, pwd_command, _("\
Print working directory.\n\
This is used for your program as well."));

  c = add_cmd ("cd", class_files, cd_command, _("\
Set working directory to DIR for debugger.\n\
The debugger's current working directory specifies where scripts and other\n\
files that can be loaded by GDB are located.\n\
In order to change the inferior's current working directory, the recommended\n\
way is to use the \"set cwd\" command."), &cmdlist);
  set_cmd_completer (c, filename_completer);

  add_com ("echo", class_support, echo_command, _("\
Print a constant string.  Give string as argument.\n\
C escape sequences may be used in the argument.\n\
No newline is added at the end of the argument;\n\
use \"\\n\" if you want a newline to be printed.\n\
Since leading and trailing whitespace are ignored in command arguments,\n\
if you want to print some you must use \"\\\" before leading whitespace\n\
to be printed or after trailing whitespace."));

  add_setshow_enum_cmd ("script-extension", class_support,
			script_ext_enums, &script_ext_mode, _("\
Set mode for script filename extension recognition."), _("\
Show mode for script filename extension recognition."), _("\
off  == no filename extension recognition (all sourced files are GDB scripts)\n\
soft == evaluate script according to filename extension, fallback to GDB script"
  "\n\
strict == evaluate script according to filename extension, error if not supported"
  ),
			NULL,
			show_script_ext_mode,
			&setlist, &showlist);

  cmd_list_element *quit_cmd
    = add_com ("quit", class_support, quit_command, _("\
Exit gdb.\n\
Usage: quit [EXPR] or exit [EXPR]\n\
The optional expression EXPR, if present, is evaluated and the result\n\
used as GDB's exit code.  The default is zero."));
  cmd_list_element *help_cmd
    = add_com ("help", class_support, help_command,
	       _("Print list of commands."));
  set_cmd_completer (help_cmd, command_completer);
  add_com_alias ("q", quit_cmd, class_support, 1);
  add_com_alias ("exit", quit_cmd, class_support, 1);
  add_com_alias ("h", help_cmd, class_support, 1);

  add_setshow_boolean_cmd ("verbose", class_support, &info_verbose, _("\
Set verbosity."), _("\
Show verbosity."), NULL,
			   set_verbose,
			   show_info_verbose,
			   &setlist, &showlist);

  add_setshow_prefix_cmd
    ("history", class_support,
     _("Generic command for setting command history parameters."),
     _("Generic command for showing command history parameters."),
     &sethistlist, &showhistlist, &setlist, &showlist);

  add_setshow_boolean_cmd ("expansion", no_class, &history_expansion_p, _("\
Set history expansion on command input."), _("\
Show history expansion on command input."), _("\
Without an argument, history expansion is enabled."),
			   NULL,
			   show_history_expansion_p,
			   &sethistlist, &showhistlist);

  cmd_list_element *info_cmd
    = add_prefix_cmd ("info", class_info, info_command, _("\
Generic command for showing things about the program being debugged."),
		      &infolist, 0, &cmdlist);
  add_com_alias ("i", info_cmd, class_info, 1);
  add_com_alias ("inf", info_cmd, class_info, 1);

  add_com ("complete", class_obscure, complete_command,
	   _("List the completions for the rest of the line as a command."));

  c = add_show_prefix_cmd ("show", class_info, _("\
Generic command for showing things about the debugger."),
			   &showlist, 0, &cmdlist);
  /* Another way to get at the same thing.  */
  add_alias_cmd ("set", c, class_info, 0, &infolist);

  cmd_list_element *with_cmd
    = add_com ("with", class_vars, with_command, _("\
Temporarily set SETTING to VALUE, run COMMAND, and restore SETTING.\n\
Usage: with SETTING [VALUE] [-- COMMAND]\n\
Usage: w SETTING [VALUE] [-- COMMAND]\n\
With no COMMAND, repeats the last executed command.\n\
\n\
SETTING is any setting you can change with the \"set\" subcommands.\n\
E.g.:\n\
  with language pascal -- print obj\n\
  with print elements unlimited -- print obj\n\
\n\
You can change multiple settings using nested with, and use\n\
abbreviations for commands and/or values.  E.g.:\n\
  w la p -- w p el u -- p obj"));
  set_cmd_completer_handle_brkchars (with_cmd, with_command_completer);
  add_com_alias ("w", with_cmd, class_vars, 1);

  add_internal_function ("_gdb_setting_str", _("\
$_gdb_setting_str - returns the value of a GDB setting as a string.\n\
Usage: $_gdb_setting_str (setting)\n\
\n\
auto-boolean values are \"off\", \"on\", \"auto\".\n\
boolean values are \"off\", \"on\".\n\
Some integer settings accept an unlimited value, returned\n\
as \"unlimited\"."),
			 gdb_setting_str_internal_fn, NULL);

  add_internal_function ("_gdb_setting", _("\
$_gdb_setting - returns the value of a GDB setting.\n\
Usage: $_gdb_setting (setting)\n\
auto-boolean values are \"off\", \"on\", \"auto\".\n\
boolean values are \"off\", \"on\".\n\
Some integer settings accept an unlimited value, returned\n\
as 0 or -1 depending on the setting."),
			 gdb_setting_internal_fn, NULL);

  add_internal_function ("_gdb_maint_setting_str", _("\
$_gdb_maint_setting_str - returns the value of a GDB maintenance setting as a string.\n\
Usage: $_gdb_maint_setting_str (setting)\n\
\n\
auto-boolean values are \"off\", \"on\", \"auto\".\n\
boolean values are \"off\", \"on\".\n\
Some integer settings accept an unlimited value, returned\n\
as \"unlimited\"."),
			 gdb_maint_setting_str_internal_fn, NULL);

  add_internal_function ("_gdb_maint_setting", _("\
$_gdb_maint_setting - returns the value of a GDB maintenance setting.\n\
Usage: $_gdb_maint_setting (setting)\n\
auto-boolean values are \"off\", \"on\", \"auto\".\n\
boolean values are \"off\", \"on\".\n\
Some integer settings accept an unlimited value, returned\n\
as 0 or -1 depending on the setting."),
			 gdb_maint_setting_internal_fn, NULL);

  add_internal_function ("_shell", _("\
$_shell - execute a shell command and return the result.\n\
\n\
    Usage: $_shell (COMMAND)\n\
\n\
    Arguments:\n\
\n\
      COMMAND: The command to execute.  Must be a string.\n\
\n\
    Returns:\n\
      The command's exit code: zero on success, non-zero otherwise."),
			 shell_internal_fn, NULL);

  add_cmd ("commands", no_set_class, show_commands, _("\
Show the history of commands you typed.\n\
You can supply a command number to start with, or a `+' to start after\n\
the previous command number shown."),
	   &showlist);

  add_cmd ("version", no_set_class, show_version,
	   _("Show what version of GDB this is."), &showlist);

  add_cmd ("configuration", no_set_class, show_configuration,
	   _("Show how GDB was configured at build time."), &showlist);

  add_setshow_prefix_cmd ("debug", no_class,
			  _("Generic command for setting gdb debugging flags."),
			  _("Generic command for showing gdb debugging flags."),
			  &setdebuglist, &showdebuglist,
			  &setlist, &showlist);

  cmd_list_element *shell_cmd
    = add_com ("shell", class_support, shell_command, _("\
Execute the rest of the line as a shell command.\n\
With no arguments, run an inferior shell."));
  set_cmd_completer (shell_cmd, filename_completer);

  add_com_alias ("!", shell_cmd, class_support, 0);

  c = add_com ("edit", class_files, edit_command, _("\
Edit specified file or function.\n\
With no argument, edits file containing most recent line listed.\n\
Editing targets can be specified in these ways:\n\
  FILE:LINENUM, to edit at that line in that file,\n\
  FUNCTION, to edit at the beginning of that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
  *ADDRESS, to edit at the line containing that address.\n\
Uses EDITOR environment variable contents as editor (or ex as default)."));

  c->completer = location_completer;

  cmd_list_element *pipe_cmd
    = add_com ("pipe", class_support, pipe_command, _("\
Send the output of a gdb command to a shell command.\n\
Usage: | [COMMAND] | SHELL_COMMAND\n\
Usage: | -d DELIM COMMAND DELIM SHELL_COMMAND\n\
Usage: pipe [COMMAND] | SHELL_COMMAND\n\
Usage: pipe -d DELIM COMMAND DELIM SHELL_COMMAND\n\
\n\
Executes COMMAND and sends its output to SHELL_COMMAND.\n\
\n\
The -d option indicates to use the string DELIM to separate COMMAND\n\
from SHELL_COMMAND, in alternative to |.  This is useful in\n\
case COMMAND contains a | character.\n\
\n\
With no COMMAND, repeat the last executed command\n\
and send its output to SHELL_COMMAND."));
  set_cmd_completer_handle_brkchars (pipe_cmd, pipe_command_completer);
  add_com_alias ("|", pipe_cmd, class_support, 0);

  cmd_list_element *list_cmd
    = add_com ("list", class_files, list_command, _("\
List specified function or line.\n\
With no argument, lists ten more lines after or around previous listing.\n\
\"list +\" lists the ten lines following a previous ten-line listing.\n\
\"list -\" lists the ten lines before a previous ten-line listing.\n\
\"list .\" lists ten lines around the point of execution in the current frame.\n\
One argument specifies a line, and ten lines are listed around that line.\n\
Two arguments with comma between specify starting and ending lines to list.\n\
Lines can be specified in these ways:\n\
  LINENUM, to list around that line in current file,\n\
  FILE:LINENUM, to list around that line in that file,\n\
  FUNCTION, to list around beginning of that function,\n\
  FILE:FUNCTION, to distinguish among like-named static functions.\n\
  *ADDRESS, to list around the line containing that address.\n\
With two args, if one is empty, it stands for ten lines away from\n\
the other arg.\n\
\n\
By default, when a single location is given, display ten lines.\n\
This can be changed using \"set listsize\", and the current value\n\
can be shown using \"show listsize\"."));

  add_com_alias ("l", list_cmd, class_files, 1);

  c = add_com ("disassemble", class_vars, disassemble_command, _("\
Disassemble a specified section of memory.\n\
Usage: disassemble[/m|/r|/s] START [, END]\n\
Default is the function surrounding the pc of the selected frame.\n\
\n\
With a /s modifier, source lines are included (if available).\n\
In this mode, the output is displayed in PC address order, and\n\
file names and contents for all relevant source files are displayed.\n\
\n\
With a /m modifier, source lines are included (if available).\n\
This view is \"source centric\": the output is in source line order,\n\
regardless of any optimization that is present.  Only the main source file\n\
is displayed, not those of, e.g., any inlined functions.\n\
This modifier hasn't proved useful in practice and is deprecated\n\
in favor of /s.\n\
\n\
With a /r modifier, raw instructions in hex are included.\n\
\n\
With a single argument, the function surrounding that address is dumped.\n\
Two arguments (separated by a comma) are taken as a range of memory to dump,\n\
  in the form of \"start,end\", or \"start,+length\".\n\
\n\
Note that the address is interpreted as an expression, not as a location\n\
like in the \"break\" command.\n\
So, for example, if you want to disassemble function bar in file foo.c\n\
you must type \"disassemble 'foo.c'::bar\" and not \"disassemble foo.c:bar\"."));
  set_cmd_completer_handle_brkchars (c, disassemble_command_completer);

  c = add_com ("make", class_support, make_command, _("\
Run the ``make'' program using the rest of the line as arguments."));
  set_cmd_completer (c, filename_completer);
  c = add_cmd ("user", no_class, show_user, _("\
Show definitions of non-python/scheme user defined commands.\n\
Argument is the name of the user defined command.\n\
With no argument, show definitions of all user defined commands."), &showlist);
  set_cmd_completer (c, show_user_completer);
  add_com ("apropos", class_support, apropos_command, _("\
Search for commands matching a REGEXP.\n\
Usage: apropos [-v] REGEXP\n\
Flag -v indicates to produce a verbose output, showing full documentation\n\
of the matching commands."));

  add_setshow_uinteger_cmd ("max-user-call-depth", no_class,
			   &max_user_call_depth, _("\
Set the max call depth for non-python/scheme user-defined commands."), _("\
Show the max call depth for non-python/scheme user-defined commands."), NULL,
			    NULL,
			    show_max_user_call_depth,
			    &setlist, &showlist);

  add_setshow_boolean_cmd ("trace-commands", no_class, &trace_commands, _("\
Set tracing of GDB CLI commands."), _("\
Show state of GDB CLI command tracing."), _("\
When 'on', each command is displayed as it is executed."),
			   NULL,
			   NULL,
			   &setlist, &showlist);

  const auto alias_opts = make_alias_options_def_group (nullptr);

  static std::string alias_help
    = gdb::option::build_help (_("\
Define a new command that is an alias of an existing command.\n\
Usage: alias [-a] [--] ALIAS = COMMAND [DEFAULT-ARGS...]\n\
ALIAS is the name of the alias command to create.\n\
COMMAND is the command being aliased to.\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
GDB will automatically prepend the provided DEFAULT-ARGS to the list\n\
of arguments explicitly provided when using ALIAS.\n\
Use \"help aliases\" to list all user defined aliases and their default args.\n\
\n\
Examples:\n\
Make \"spe\" an alias of \"set print elements\":\n\
  alias spe = set print elements\n\
Make \"elms\" an alias of \"elements\" in the \"set print\" command:\n\
  alias -a set print elms = set print elements\n\
Make \"btf\" an alias of \"backtrace -full -past-entry -past-main\" :\n\
  alias btf = backtrace -full -past-entry -past-main\n\
Make \"wLapPeu\" an alias of 2 nested \"with\":\n\
  alias wLapPeu = with language pascal -- with print elements unlimited --"),
			       alias_opts);

  c = add_com ("alias", class_support, alias_command,
	       alias_help.c_str ());

  set_cmd_completer_handle_brkchars (c, alias_command_completer);

  add_setshow_boolean_cmd ("suppress-cli-notifications", no_class,
			   &user_wants_cli_suppress_notification,
			   _("\
Set whether printing notifications on CLI is suppressed."), _("\
Show whether printing notifications on CLI is suppressed."), _("\
When on, printing notifications (such as inferior/thread switch)\n\
on CLI is suppressed."),
			   set_suppress_cli_notifications,
			   show_suppress_cli_notifications,
			   &setlist,
			   &showlist);

  const char *source_help_text = xstrprintf (_("\
Read commands from a file named FILE.\n\
\n\
Usage: source [-s] [-v] FILE\n\
-s: search for the script in the source search path,\n\
    even if FILE contains directories.\n\
-v: each command in FILE is echoed as it is executed.\n\
\n\
Note that the file \"%s\" is read automatically in this way\n\
when GDB is started."), GDBINIT).release ();
  c = add_cmd ("source", class_support, source_command,
	       source_help_text, &cmdlist);
  set_cmd_completer (c, filename_completer);
}

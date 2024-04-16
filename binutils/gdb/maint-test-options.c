/* Maintenance commands for testing the options framework.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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
#include "gdbcmd.h"
#include "cli/cli-option.h"

/* This file defines three "maintenance test-options" subcommands to
   exercise TAB-completion and option processing:

    (gdb) maint test-options require-delimiter
    (gdb) maint test-options unknown-is-error
    (gdb) maint test-options unknown-is-operand

   And a fourth one to help with TAB-completion testing.

    (gdb) maint show test-options-completion-result

   Each of the test-options subcommands exercise
   gdb::option::process_options with a different enum
   process_options_mode value.  Examples for commands they model:

   - "print" and "compile print", are like "require-delimiter",
      because they accept random expressions as argument.

   - "backtrace" and "frame/thread apply" are like
     "unknown-is-operand", because "-" is a valid command.

   - "compile file" and "compile code" are like "unknown-is-error".

   These commands allow exercising all aspects of option processing
   without having to pick some existing command.  That should be more
   stable going forward than relying on an existing user command,
   since if we picked say "print", that command or its options could
   change in future, and then we'd be left with having to pick some
   other command or option to exercise some non-command-specific
   option processing detail.  Also, actual user commands have side
   effects that we're not interested in when we're focusing on unit
   testing the options machinery.  BTW, a maintenance command is used
   as a sort of unit test driver instead of actual "maint selftest"
   unit tests, since we need to go all the way via gdb including
   readline, for proper testing of TAB completion.

   These maintenance commands support options of all the different
   available kinds of commands (boolean, enum, flag, string, uinteger):

    (gdb) maint test-options require-delimiter -[TAB]
    -bool                -pinteger-unlimited  -xx1
    -enum                -string              -xx2
    -flag                -uinteger-unlimited

    (gdb) maint test-options require-delimiter -bool o[TAB]
    off  on
    (gdb) maint test-options require-delimiter -enum [TAB]
    xxx  yyy  zzz
    (gdb) maint test-options require-delimiter -uinteger-unlimited [TAB]
    NUMBER     unlimited

   '-xx1' and '-xx2' are flag options too.  They exist in order to
   test ambiguous option names, like '-xx'.

  Invoking the commands makes them print out the options parsed:

   (gdb) maint test-options unknown-is-error -flag -enum yyy cmdarg
   -flag 1 -xx1 0 -xx2 0 -bool 0 -enum yyy -uint-unl 0 -pint-unl 0 -string '' -- cmdarg

   (gdb) maint test-options require-delimiter -flag -enum yyy cmdarg
   -flag 0 -xx1 0 -xx2 0 -bool 0 -enum xxx -uint-unl 0 -pint-unl 0 -string '' -- -flag -enum yyy cmdarg
   (gdb) maint test-options require-delimiter -flag -enum yyy cmdarg --
   Unrecognized option at: cmdarg --
   (gdb) maint test-options require-delimiter -flag -enum yyy -- cmdarg
   -flag 1 -xx1 0 -xx2 0 -bool 0 -enum yyy -uint-unl 0 -pint-unl 0 -string '' -- cmdarg

  The "maint show test-options-completion-result" command exists in
  order to do something similar for completion:

   (gdb) maint test-options unknown-is-error -flag -b 0 -enum yyy OPERAND[TAB]
   (gdb) maint show test-options-completion-result
   0 OPERAND

   (gdb) maint test-options unknown-is-error -flag -b 0 -enum yyy[TAB]
   (gdb) maint show test-options-completion-result
   1

   (gdb) maint test-options require-dash -unknown[TAB]
   (gdb) maint show test-options-completion-result
   1

  Here, "1" means the completion function processed the whole input
  line, and that the command shouldn't do anything with the arguments,
  since there are no operands.  While "0" indicates that there are
  operands after options.  The text after "0" is the operands.

  This level of detail is particularly important because getting the
  completion function's entry point to return back to the caller the
  right pointer into the operand is quite tricky in several
  scenarios.  */

/* Enum values for the "maintenance test-options" commands.  */
const char test_options_enum_values_xxx[] = "xxx";
const char test_options_enum_values_yyy[] = "yyy";
const char test_options_enum_values_zzz[] = "zzz";
static const char *const test_options_enum_values_choices[] =
{
  test_options_enum_values_xxx,
  test_options_enum_values_yyy,
  test_options_enum_values_zzz,
  NULL
};

/* Option data for the "maintenance test-options" commands.  */

struct test_options_opts
{
  bool flag_opt = false;
  bool xx1_opt = false;
  bool xx2_opt = false;
  bool boolean_opt = false;
  const char *enum_opt = test_options_enum_values_xxx;
  unsigned int uint_unl_opt = 0;
  int pint_unl_opt = 0;
  std::string string_opt;

  test_options_opts () = default;

  DISABLE_COPY_AND_ASSIGN (test_options_opts);

  /* Dump the options to FILE.  ARGS is the remainder unprocessed
     arguments.  */
  void dump (ui_file *file, const char *args) const
  {
    gdb_printf (file,
		_("-flag %d -xx1 %d -xx2 %d -bool %d "
		  "-enum %s -uint-unl %s -pint-unl %s -string '%s' -- %s\n"),
		flag_opt,
		xx1_opt,
		xx2_opt,
		boolean_opt,
		enum_opt,
		(uint_unl_opt == UINT_MAX
		 ? "unlimited"
		 : pulongest (uint_unl_opt)),
		(pint_unl_opt == -1
		 ? "unlimited"
		 : plongest (pint_unl_opt)),
		string_opt.c_str (),
		args);
  }
};

/* Option definitions for the "maintenance test-options" commands.  */

static const gdb::option::option_def test_options_option_defs[] = {

  /* A flag option.  */
  gdb::option::flag_option_def<test_options_opts> {
    "flag",
    [] (test_options_opts *opts) { return &opts->flag_opt; },
    N_("A flag option."),
  },

  /* A couple flags with similar names, for "ambiguous option names"
     testing.  */
  gdb::option::flag_option_def<test_options_opts> {
    "xx1",
    [] (test_options_opts *opts) { return &opts->xx1_opt; },
    N_("A flag option."),
  },
  gdb::option::flag_option_def<test_options_opts> {
    "xx2",
    [] (test_options_opts *opts) { return &opts->xx2_opt; },
    N_("A flag option."),
  },

  /* A boolean option.  */
  gdb::option::boolean_option_def<test_options_opts> {
    "bool",
    [] (test_options_opts *opts) { return &opts->boolean_opt; },
    nullptr, /* show_cmd_cb */
    N_("A boolean option."),
  },

  /* An enum option.  */
  gdb::option::enum_option_def<test_options_opts> {
    "enum",
    test_options_enum_values_choices,
    [] (test_options_opts *opts) { return &opts->enum_opt; },
    nullptr, /* show_cmd_cb */
    N_("An enum option."),
  },

  /* A uinteger + "unlimited" option.  */
  gdb::option::uinteger_option_def<test_options_opts> {
    "uinteger-unlimited",
    [] (test_options_opts *opts) { return &opts->uint_unl_opt; },
    uinteger_unlimited_literals,
    nullptr, /* show_cmd_cb */
    N_("A uinteger option."),
    nullptr, /* show_doc */
    N_("A help doc that spawns\nmultiple lines."),
  },

  /* A pinteger + "unlimited" option.  */
  gdb::option::pinteger_option_def<test_options_opts> {
    "pinteger-unlimited",
    [] (test_options_opts *opts) { return &opts->pint_unl_opt; },
    pinteger_unlimited_literals,
    nullptr, /* show_cmd_cb */
    N_("A pinteger-unlimited option."),
    nullptr, /* show_doc */
    nullptr, /* help_doc */
  },

  /* A string option.  */
  gdb::option::string_option_def<test_options_opts> {
    "string",
    [] (test_options_opts *opts) { return &opts->string_opt; },
    nullptr, /* show_cmd_cb */
    N_("A string option."),
  },
};

/* Create an option_def_group for the test_options_opts options, with
   OPTS as context.  */

static inline gdb::option::option_def_group
make_test_options_options_def_group (test_options_opts *opts)
{
  return {{test_options_option_defs}, opts};
}

/* Implementation of the "maintenance test-options
   require-delimiter/unknown-is-error/unknown-is-operand" commands.
   Each of the commands maps to a different enum process_options_mode
   enumerator.  The test strategy is simply processing the options in
   a number of scenarios, and printing back the parsed result.  */

static void
maintenance_test_options_command_mode (const char *args,
				       gdb::option::process_options_mode mode)
{
  test_options_opts opts;

  gdb::option::process_options (&args, mode,
				make_test_options_options_def_group (&opts));

  if (args == nullptr)
    args = "";
  else
    args = skip_spaces (args);

  opts.dump (gdb_stdout, args);
}

/* Variable used by the "maintenance show
   test-options-completion-result" command.  This variable is stored
   by the completer of the "maint test-options" subcommands.

   If the completer returned false, this includes the text at the word
   point after gdb::option::complete_options returns.  If true, then
   this includes a dump of the processed options.  */
static std::string maintenance_test_options_command_completion_text;

/* The "maintenance show test-options-completion-result" command.  */

static void
maintenance_show_test_options_completion_result (const char *args,
						 int from_tty)
{
  gdb_puts (maintenance_test_options_command_completion_text.c_str ());
}

/* Save the completion result in the global variables read by the
   "maintenance test-options require-delimiter" command.  */

static void
save_completion_result (const test_options_opts &opts, bool res,
			const char *text)
{
  if (res)
    {
      string_file stream;

      stream.puts ("1 ");
      opts.dump (&stream, text);
      maintenance_test_options_command_completion_text = stream.release ();
    }
  else
    {
      maintenance_test_options_command_completion_text
	= string_printf ("0 %s\n", text);
    }
}

/* Implementation of completer for the "maintenance test-options
   require-delimiter/unknown-is-error/unknown-is-operand" commands.
   Each of the commands maps to a different enum process_options_mode
   enumerator.  */

static void
maintenance_test_options_completer_mode (completion_tracker &tracker,
					 const char *text,
					 gdb::option::process_options_mode mode)
{
  test_options_opts opts;

  try
    {
      bool res = (gdb::option::complete_options
		  (tracker, &text, mode,
		   make_test_options_options_def_group (&opts)));

      save_completion_result (opts, res, text);
    }
  catch (const gdb_exception_error &ex)
    {
      save_completion_result (opts, true, text);
      throw;
    }
}

/* Implementation of the "maintenance test-options require-delimiter"
   command.  */

static void
maintenance_test_options_require_delimiter_command (const char *args,
						    int from_tty)
{
  maintenance_test_options_command_mode
    (args, gdb::option::PROCESS_OPTIONS_REQUIRE_DELIMITER);
}

/* Implementation of the "maintenance test-options
   unknown-is-error" command.  */

static void
maintenance_test_options_unknown_is_error_command (const char *args,
						   int from_tty)
{
  maintenance_test_options_command_mode
    (args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR);
}

/* Implementation of the "maintenance test-options
   unknown-is-operand" command.  */

static void
maintenance_test_options_unknown_is_operand_command (const char *args,
						     int from_tty)
{
  maintenance_test_options_command_mode
    (args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND);
}

/* Completer for the "maintenance test-options require-delimiter"
   command.  */

static void
maintenance_test_options_require_delimiter_command_completer
  (cmd_list_element *ignore, completion_tracker &tracker,
   const char *text, const char *word)
{
  maintenance_test_options_completer_mode
    (tracker, text, gdb::option::PROCESS_OPTIONS_REQUIRE_DELIMITER);
}

/* Completer for the "maintenance test-options unknown-is-error"
   command.  */

static void
maintenance_test_options_unknown_is_error_command_completer
  (cmd_list_element *ignore, completion_tracker &tracker,
   const char *text, const char *word)
{
  maintenance_test_options_completer_mode
    (tracker, text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR);
}

/* Completer for the "maintenance test-options unknown-is-operand"
   command.  */

static void
maintenance_test_options_unknown_is_operand_command_completer
  (cmd_list_element *ignore, completion_tracker &tracker,
   const char *text, const char *word)
{
  maintenance_test_options_completer_mode
    (tracker, text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND);
}

/* Command list for maint test-options.  */
static cmd_list_element *maintenance_test_options_list;


void _initialize_maint_test_options ();
void
_initialize_maint_test_options ()
{
  cmd_list_element *cmd;

  add_basic_prefix_cmd ("test-options", no_class,
			_("\
Generic command for testing the options infrastructure."),
			&maintenance_test_options_list,
			0, &maintenancelist);

  const auto def_group = make_test_options_options_def_group (nullptr);

  static const std::string help_require_delim_str
    = gdb::option::build_help (_("\
Command used for testing options processing.\n\
Usage: maint test-options require-delimiter [[OPTION]... --] [OPERAND]...\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
If you specify any command option, you must use a double dash (\"--\")\n\
to mark the end of option processing."),
			       def_group);

  static const std::string help_unknown_is_error_str
    = gdb::option::build_help (_("\
Command used for testing options processing.\n\
Usage: maint test-options unknown-is-error [OPTION]... [OPERAND]...\n\
\n\
Options:\n\
%OPTIONS%"),
			       def_group);

  static const std::string help_unknown_is_operand_str
    = gdb::option::build_help (_("\
Command used for testing options processing.\n\
Usage: maint test-options unknown-is-operand [OPTION]... [OPERAND]...\n\
\n\
Options:\n\
%OPTIONS%"),
			       def_group);

  cmd = add_cmd ("require-delimiter", class_maintenance,
		 maintenance_test_options_require_delimiter_command,
		 help_require_delim_str.c_str (),
		 &maintenance_test_options_list);
  set_cmd_completer_handle_brkchars
    (cmd, maintenance_test_options_require_delimiter_command_completer);

  cmd = add_cmd ("unknown-is-error", class_maintenance,
		 maintenance_test_options_unknown_is_error_command,
		 help_unknown_is_error_str.c_str (),
		 &maintenance_test_options_list);
  set_cmd_completer_handle_brkchars
    (cmd, maintenance_test_options_unknown_is_error_command_completer);

  cmd = add_cmd ("unknown-is-operand", class_maintenance,
		 maintenance_test_options_unknown_is_operand_command,
		 help_unknown_is_operand_str.c_str (),
		 &maintenance_test_options_list);
  set_cmd_completer_handle_brkchars
    (cmd, maintenance_test_options_unknown_is_operand_command_completer);

  add_cmd ("test-options-completion-result", class_maintenance,
	   maintenance_show_test_options_completion_result,
	   _("\
Show maintenance test-options completion result.\n\
Shows the results of completing\n\
\"maint test-options require-delimiter\",\n\
\"maint test-options unknown-is-error\", or\n\
\"maint test-options unknown-is-operand\"."),
	   &maintenance_show_cmdlist);
}

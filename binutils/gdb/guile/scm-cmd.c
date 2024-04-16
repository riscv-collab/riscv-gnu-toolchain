/* GDB commands implemented in Scheme.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "charset.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "completer.h"
#include "guile-internal.h"

/* The <gdb:command> smob.

   Note: Commands are added to gdb using a two step process:
   1) Call make-command to create a <gdb:command> object.
   2) Call register-command! to add the command to gdb.
   It is done this way so that the constructor, make-command, doesn't have
   any side-effects.  This means that the smob needs to store everything
   that was passed to make-command.  */

struct command_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The name of the command, as passed to make-command.  */
  char *name;

  /* The last word of the command.
     This is needed because add_cmd requires us to allocate space
     for it. :-(  */
  char *cmd_name;

  /* Non-zero if this is a prefix command.  */
  int is_prefix;

  /* One of the COMMAND_* constants.  */
  enum command_class cmd_class;

  /* The documentation for the command.  */
  char *doc;

  /* The corresponding gdb command object.
     This is NULL if the command has not been registered yet, or
     is no longer registered.  */
  struct cmd_list_element *command;

  /* A prefix command requires storage for a list of its sub-commands.
     A pointer to this is passed to add_prefix_command, and to add_cmd
     for sub-commands of that prefix.
     This is NULL if the command has not been registered yet, or
     is no longer registered.  If this command is not a prefix
     command, then this field is unused.  */
  struct cmd_list_element *sub_list;

  /* The procedure to call to invoke the command.
     (lambda (self arg from-tty) ...).
     Its result is unspecified.  */
  SCM invoke;

  /* Either #f, one of the COMPLETE_* constants, or a procedure to call to
     perform command completion.  Called as (lambda (self text word) ...).  */
  SCM complete;

  /* The <gdb:command> object we are contained in, needed to protect/unprotect
     the object since a reference to it comes from non-gc-managed space
     (the command context pointer).  */
  SCM containing_scm;
};

static const char command_smob_name[] = "gdb:command";

/* The tag Guile knows the objfile smob by.  */
static scm_t_bits command_smob_tag;

/* Keywords used by make-command.  */
static SCM invoke_keyword;
static SCM command_class_keyword;
static SCM completer_class_keyword;
static SCM prefix_p_keyword;
static SCM doc_keyword;

/* Struct representing built-in completion types.  */
struct cmdscm_completer
{
  /* Scheme symbol name.  */
  const char *name;
  /* Completion function.  */
  completer_ftype *completer;
};

static const struct cmdscm_completer cmdscm_completers[] =
{
  { "COMPLETE_NONE", noop_completer },
  { "COMPLETE_FILENAME", filename_completer },
  { "COMPLETE_LOCATION", location_completer },
  { "COMPLETE_COMMAND", command_completer },
  { "COMPLETE_SYMBOL", symbol_completer },
  { "COMPLETE_EXPRESSION", expression_completer },
};

#define N_COMPLETERS (sizeof (cmdscm_completers) \
		      / sizeof (cmdscm_completers[0]))

static int cmdscm_is_valid (command_smob *);

/* Administrivia for command smobs.  */

/* The smob "print" function for <gdb:command>.  */

static int
cmdscm_print_command_smob (SCM self, SCM port, scm_print_state *pstate)
{
  command_smob *c_smob = (command_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s", command_smob_name);

  gdbscm_printf (port, " %s",
		 c_smob->name != NULL ? c_smob->name : "{unnamed}");

  if (! cmdscm_is_valid (c_smob))
    scm_puts (" {invalid}", port);

  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:command> object.
   It's empty in the sense that a command still needs to be associated
   with it.  */

static SCM
cmdscm_make_command_smob (void)
{
  command_smob *c_smob = (command_smob *)
    scm_gc_malloc (sizeof (command_smob), command_smob_name);
  SCM c_scm;

  memset (c_smob, 0, sizeof (*c_smob));
  c_smob->cmd_class = no_class;
  c_smob->invoke = SCM_BOOL_F;
  c_smob->complete = SCM_BOOL_F;
  c_scm = scm_new_smob (command_smob_tag, (scm_t_bits) c_smob);
  c_smob->containing_scm = c_scm;
  gdbscm_init_gsmob (&c_smob->base);

  return c_scm;
}

/* Clear the COMMAND pointer in C_SMOB and unprotect the object from GC.  */

static void
cmdscm_release_command (command_smob *c_smob)
{
  c_smob->command = NULL;
  scm_gc_unprotect_object (c_smob->containing_scm);
}

/* Return non-zero if SCM is a command smob.  */

static int
cmdscm_is_command (SCM scm)
{
  return SCM_SMOB_PREDICATE (command_smob_tag, scm);
}

/* (command? scm) -> boolean */

static SCM
gdbscm_command_p (SCM scm)
{
  return scm_from_bool (cmdscm_is_command (scm));
}

/* Returns the <gdb:command> object in SELF.
   Throws an exception if SELF is not a <gdb:command> object.  */

static SCM
cmdscm_get_command_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (cmdscm_is_command (self), self, arg_pos, func_name,
		   command_smob_name);

  return self;
}

/* Returns a pointer to the command smob of SELF.
   Throws an exception if SELF is not a <gdb:command> object.  */

static command_smob *
cmdscm_get_command_smob_arg_unsafe (SCM self, int arg_pos,
				    const char *func_name)
{
  SCM c_scm = cmdscm_get_command_arg_unsafe (self, arg_pos, func_name);
  command_smob *c_smob = (command_smob *) SCM_SMOB_DATA (c_scm);

  return c_smob;
}

/* Return non-zero if command C_SMOB is valid.  */

static int
cmdscm_is_valid (command_smob *c_smob)
{
  return c_smob->command != NULL;
}

/* Returns a pointer to the command smob of SELF.
   Throws an exception if SELF is not a valid <gdb:command> object.  */

static command_smob *
cmdscm_get_valid_command_smob_arg_unsafe (SCM self, int arg_pos,
					  const char *func_name)
{
  command_smob *c_smob
    = cmdscm_get_command_smob_arg_unsafe (self, arg_pos, func_name);

  if (!cmdscm_is_valid (c_smob))
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("<gdb:command>"));
    }

  return c_smob;
}

/* Scheme functions for GDB commands.  */

/* (command-valid? <gdb:command>) -> boolean
   Returns #t if SELF is still valid.  */

static SCM
gdbscm_command_valid_p (SCM self)
{
  command_smob *c_smob
    = cmdscm_get_command_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (cmdscm_is_valid (c_smob));
}

/* (dont-repeat cmd) -> unspecified
   Scheme function which wraps dont_repeat.  */

static SCM
gdbscm_dont_repeat (SCM self)
{
  /* We currently don't need anything from SELF, but still verify it.
     Call for side effects.  */
  cmdscm_get_valid_command_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  dont_repeat ();

  return SCM_UNSPECIFIED;
}

/* The make-command function.  */

/* Called if the gdb cmd_list_element is destroyed.  */

static void
cmdscm_destroyer (struct cmd_list_element *self, void *context)
{
  command_smob *c_smob = (command_smob *) context;

  cmdscm_release_command (c_smob);
}

/* Called by gdb to invoke the command.  */

static void
cmdscm_function (const char *args, int from_tty, cmd_list_element *command)
{
  command_smob *c_smob/*obj*/ = (command_smob *) command->context ();
  SCM arg_scm, tty_scm, result;

  gdb_assert (c_smob != NULL);

  if (args == NULL)
    args = "";
  arg_scm = gdbscm_scm_from_string (args, strlen (args), host_charset (), 1);
  if (gdbscm_is_exception (arg_scm))
    error (_("Could not convert arguments to Scheme string."));

  tty_scm = scm_from_bool (from_tty);

  result = gdbscm_safe_call_3 (c_smob->invoke, c_smob->containing_scm,
			       arg_scm, tty_scm, gdbscm_user_error_p);

  if (gdbscm_is_exception (result))
    {
      /* Don't print the stack if this was an error signalled by the command
	 itself.  */
      if (gdbscm_user_error_p (gdbscm_exception_key (result)))
	{
	  gdb::unique_xmalloc_ptr<char> msg
	    = gdbscm_exception_message_to_string (result);

	  error ("%s", msg.get ());
	}
      else
	{
	  gdbscm_print_gdb_exception (SCM_BOOL_F, result);
	  error (_("Error occurred in Scheme-implemented GDB command."));
	}
    }
}

/* Subroutine of cmdscm_completer to simplify it.
   Print an error message indicating that COMPLETION is a bad completion
   result.  */

static void
cmdscm_bad_completion_result (const char *msg, SCM completion)
{
  SCM port = scm_current_error_port ();

  scm_puts (msg, port);
  scm_display (completion, port);
  scm_newline (port);
}

/* Subroutine of cmdscm_completer to simplify it.
   Validate COMPLETION and add to RESULT.
   If an error occurs print an error message.
   The result is a boolean indicating success.  */

static int
cmdscm_add_completion (SCM completion, completion_tracker &tracker)
{
  SCM except_scm;

  if (!scm_is_string (completion))
    {
      /* Inform the user, but otherwise ignore the entire result.  */
      cmdscm_bad_completion_result (_("Bad text from completer: "),
				    completion);
      return 0;
    }

  gdb::unique_xmalloc_ptr<char> item
    = gdbscm_scm_to_string (completion, NULL, host_charset (), 1,
			    &except_scm);
  if (item == NULL)
    {
      /* Inform the user, but otherwise ignore the entire result.  */
      gdbscm_print_gdb_exception (SCM_BOOL_F, except_scm);
      return 0;
    }

  tracker.add_completion (std::move (item));

  return 1;
}

/* Called by gdb for command completion.  */

static void
cmdscm_completer (struct cmd_list_element *command,
		  completion_tracker &tracker,
		  const char *text, const char *word)
{
  command_smob *c_smob/*obj*/ = (command_smob *) command->context ();
  SCM completer_result_scm;
  SCM text_scm, word_scm;

  gdb_assert (c_smob != NULL);
  gdb_assert (gdbscm_is_procedure (c_smob->complete));

  text_scm = gdbscm_scm_from_string (text, strlen (text), host_charset (),
				     1);
  if (gdbscm_is_exception (text_scm))
    error (_("Could not convert \"text\" argument to Scheme string."));
  word_scm = gdbscm_scm_from_string (word, strlen (word), host_charset (),
				     1);
  if (gdbscm_is_exception (word_scm))
    error (_("Could not convert \"word\" argument to Scheme string."));

  completer_result_scm
    = gdbscm_safe_call_3 (c_smob->complete, c_smob->containing_scm,
			  text_scm, word_scm, NULL);

  if (gdbscm_is_exception (completer_result_scm))
    {
      /* Inform the user, but otherwise ignore.  */
      gdbscm_print_gdb_exception (SCM_BOOL_F, completer_result_scm);
      return;
    }

  if (gdbscm_is_true (scm_list_p (completer_result_scm)))
    {
      SCM list = completer_result_scm;

      while (!scm_is_eq (list, SCM_EOL))
	{
	  SCM next = scm_car (list);

	  if (!cmdscm_add_completion (next, tracker))
	    break;

	  list = scm_cdr (list);
	}
    }
  else if (itscm_is_iterator (completer_result_scm))
    {
      SCM iter = completer_result_scm;
      SCM next = itscm_safe_call_next_x (iter, NULL);

      while (gdbscm_is_true (next))
	{
	  if (gdbscm_is_exception (next))
	    {
	      /* Inform the user.  */
	      gdbscm_print_gdb_exception (SCM_BOOL_F, completer_result_scm);
	      break;
	    }

	  if (cmdscm_add_completion (next, tracker))
	    break;

	  next = itscm_safe_call_next_x (iter, NULL);
	}
    }
  else
    {
      /* Inform the user, but otherwise ignore.  */
      cmdscm_bad_completion_result (_("Bad completer result: "),
				    completer_result_scm);
    }
}

/* Helper for gdbscm_make_command which locates the command list to use and
   pulls out the command name.

   NAME is the command name list.  The final word in the list is the
   name of the new command.  All earlier words must be existing prefix
   commands.

   *BASE_LIST is set to the final prefix command's list of
   *sub-commands.

   START_LIST is the list in which the search starts.

   This function returns the xmalloc()d name of the new command.
   On error a Scheme exception is thrown.  */

char *
gdbscm_parse_command_name (const char *name,
			   const char *func_name, int arg_pos,
			   struct cmd_list_element ***base_list,
			   struct cmd_list_element **start_list)
{
  struct cmd_list_element *elt;
  int len = strlen (name);
  int i, lastchar;
  char *msg;

  /* Skip trailing whitespace.  */
  for (i = len - 1; i >= 0 && (name[i] == ' ' || name[i] == '\t'); --i)
    ;
  if (i < 0)
    {
      gdbscm_out_of_range_error (func_name, arg_pos,
				 gdbscm_scm_from_c_string (name),
				 _("no command name found"));
    }
  lastchar = i;

  /* Find first character of the final word.  */
  for (; i > 0 && valid_cmd_char_p (name[i - 1]); --i)
    ;
  gdb::unique_xmalloc_ptr<char> result ((char *) xmalloc (lastchar - i + 2));
  memcpy (result.get (), &name[i], lastchar - i + 1);
  result.get ()[lastchar - i + 1] = '\0';

  /* Skip whitespace again.  */
  for (--i; i >= 0 && (name[i] == ' ' || name[i] == '\t'); --i)
    ;
  if (i < 0)
    {
      *base_list = start_list;
      return result.release ();
    }

  gdb::unique_xmalloc_ptr<char> prefix_text ((char *) xmalloc (i + 2));
  memcpy (prefix_text.get (), name, i + 1);
  prefix_text.get ()[i + 1] = '\0';

  const char *prefix_text2 = prefix_text.get ();
  elt = lookup_cmd_1 (&prefix_text2, *start_list, NULL, NULL, 1);
  if (elt == NULL || elt == CMD_LIST_AMBIGUOUS)
    {
      msg = xstrprintf (_("could not find command prefix '%s'"),
			prefix_text.get ()).release ();
      scm_dynwind_begin ((scm_t_dynwind_flags) 0);
      gdbscm_dynwind_xfree (msg);
      gdbscm_out_of_range_error (func_name, arg_pos,
				 gdbscm_scm_from_c_string (name), msg);
    }

  if (elt->is_prefix ())
    {
      *base_list = elt->subcommands;
      return result.release ();
    }

  msg = xstrprintf (_("'%s' is not a prefix command"),
		    prefix_text.get ()).release ();
  scm_dynwind_begin ((scm_t_dynwind_flags) 0);
  gdbscm_dynwind_xfree (msg);
  gdbscm_out_of_range_error (func_name, arg_pos,
			     gdbscm_scm_from_c_string (name), msg);
  /* NOTREACHED */
}

static const scheme_integer_constant command_classes[] =
{
  /* Note: alias and user are special; pseudo appears to be unused,
     and there is no reason to expose tui, I think.  */
  { "COMMAND_NONE", no_class },
  { "COMMAND_RUNNING", class_run },
  { "COMMAND_DATA", class_vars },
  { "COMMAND_STACK", class_stack },
  { "COMMAND_FILES", class_files },
  { "COMMAND_SUPPORT", class_support },
  { "COMMAND_STATUS", class_info },
  { "COMMAND_BREAKPOINTS", class_breakpoint },
  { "COMMAND_TRACEPOINTS", class_trace },
  { "COMMAND_OBSCURE", class_obscure },
  { "COMMAND_MAINTENANCE", class_maintenance },
  { "COMMAND_USER", class_user },

  END_INTEGER_CONSTANTS
};

/* Return non-zero if command_class is a valid command class.  */

int
gdbscm_valid_command_class_p (int command_class)
{
  int i;

  for (i = 0; command_classes[i].name != NULL; ++i)
    {
      if (command_classes[i].value == command_class)
	return 1;
    }

  return 0;
}

/* Return a normalized form of command NAME.
   That is tabs are replaced with spaces and multiple spaces are replaced
   with a single space.
   If WANT_TRAILING_SPACE is non-zero, add one space at the end.  This is for
   prefix commands.
   but that is the caller's responsibility.
   Space for the result is allocated on the GC heap.  */

char *
gdbscm_canonicalize_command_name (const char *name, int want_trailing_space)
{
  int i, out, seen_word;
  char *result
    = (char *) scm_gc_malloc_pointerless (strlen (name) + 2, FUNC_NAME);

  i = out = seen_word = 0;
  while (name[i])
    {
      /* Skip whitespace.  */
      while (name[i] == ' ' || name[i] == '\t')
	++i;
      /* Copy non-whitespace characters.  */
      if (name[i])
	{
	  if (seen_word)
	    result[out++] = ' ';
	  while (name[i] && name[i] != ' ' && name[i] != '\t')
	    result[out++] = name[i++];
	  seen_word = 1;
	}
    }
  if (want_trailing_space)
    result[out++] = ' ';
  result[out] = '\0';

  return result;
}

/* (make-command name [#:invoke lambda]
     [#:command-class class] [#:completer-class completer]
     [#:prefix? <bool>] [#:doc <string>]) -> <gdb:command>

   NAME is the name of the command.  It may consist of multiple words,
   in which case the final word is the name of the new command, and
   earlier words must be prefix commands.

   INVOKE is a procedure of three arguments that performs the command when
   invoked: (lambda (self arg from-tty) ...).
   Its result is unspecified.

   CLASS is the kind of command.  It must be one of the COMMAND_*
   constants defined in the gdb module.  If not specified, "no_class" is used.

   COMPLETER is the kind of completer.  It must be either:
     #f - completion is not supported for this command.
     One of the COMPLETE_* constants defined in the gdb module.
     A procedure of three arguments: (lambda (self text word) ...).
       Its result is one of:
	 A list of strings.
	 A <gdb:iterator> object that returns the set of possible completions,
	 ending with #f.
	 TODO(dje): Once PR 16699 is fixed, add support for returning
	 a COMPLETE_* constant.
   If not specified, then completion is not supported for this command.

   If PREFIX is #t, then this command is a prefix command.

   DOC is the doc string for the command.

   The result is the <gdb:command> Scheme object.
   The command is not available to be used yet, however.
   It must still be added to gdb with register-command!.  */

static SCM
gdbscm_make_command (SCM name_scm, SCM rest)
{
  const SCM keywords[] = {
    invoke_keyword, command_class_keyword, completer_class_keyword,
    prefix_p_keyword, doc_keyword, SCM_BOOL_F
  };
  int invoke_arg_pos = -1, command_class_arg_pos = 1;
  int completer_class_arg_pos = -1, is_prefix_arg_pos = -1;
  int doc_arg_pos = -1;
  char *s;
  char *name;
  enum command_class command_class = no_class;
  SCM completer_class = SCM_BOOL_F;
  int is_prefix = 0;
  char *doc = NULL;
  SCM invoke = SCM_BOOL_F;
  SCM c_scm;
  command_smob *c_smob;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#OiOts",
			      name_scm, &name, rest,
			      &invoke_arg_pos, &invoke,
			      &command_class_arg_pos, &command_class,
			      &completer_class_arg_pos, &completer_class,
			      &is_prefix_arg_pos, &is_prefix,
			      &doc_arg_pos, &doc);

  if (doc == NULL)
    doc = xstrdup (_("This command is not documented."));

  s = name;
  name = gdbscm_canonicalize_command_name (s, is_prefix);
  xfree (s);
  s = doc;
  doc = gdbscm_gc_xstrdup (s);
  xfree (s);

  if (is_prefix
      ? name[0] == ' '
      : name[0] == '\0')
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, name_scm,
				 _("no command name found"));
    }

  if (gdbscm_is_true (invoke))
    {
      SCM_ASSERT_TYPE (gdbscm_is_procedure (invoke), invoke,
		       invoke_arg_pos, FUNC_NAME, _("procedure"));
    }

  if (!gdbscm_valid_command_class_p (command_class))
    {
      gdbscm_out_of_range_error (FUNC_NAME, command_class_arg_pos,
				 scm_from_int (command_class),
				 _("invalid command class argument"));
    }

  SCM_ASSERT_TYPE (gdbscm_is_false (completer_class)
		   || scm_is_integer (completer_class)
		   || gdbscm_is_procedure (completer_class),
		   completer_class, completer_class_arg_pos, FUNC_NAME,
		   _("integer or procedure"));
  if (scm_is_integer (completer_class)
      && !scm_is_signed_integer (completer_class, 0, N_COMPLETERS - 1))
    {
      gdbscm_out_of_range_error (FUNC_NAME, completer_class_arg_pos,
				 completer_class,
				 _("invalid completion type argument"));
    }

  c_scm = cmdscm_make_command_smob ();
  c_smob = (command_smob *) SCM_SMOB_DATA (c_scm);
  c_smob->name = name;
  c_smob->is_prefix = is_prefix;
  c_smob->cmd_class = command_class;
  c_smob->doc = doc;
  c_smob->invoke = invoke;
  c_smob->complete = completer_class;

  return c_scm;
}

/* (register-command! <gdb:command>) -> unspecified

   It is an error to register a command more than once.  */

static SCM
gdbscm_register_command_x (SCM self)
{
  command_smob *c_smob
    = cmdscm_get_command_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  char *cmd_name;
  struct cmd_list_element **cmd_list;
  struct cmd_list_element *cmd = NULL;

  if (cmdscm_is_valid (c_smob))
    scm_misc_error (FUNC_NAME, _("command is already registered"), SCM_EOL);

  cmd_name = gdbscm_parse_command_name (c_smob->name, FUNC_NAME, SCM_ARG1,
					&cmd_list, &cmdlist);
  c_smob->cmd_name = gdbscm_gc_xstrdup (cmd_name);
  xfree (cmd_name);

  gdbscm_gdb_exception exc {};
  try
    {
      if (c_smob->is_prefix)
	{
	  /* If we have our own "invoke" method, then allow unknown
	     sub-commands.  */
	  int allow_unknown = gdbscm_is_true (c_smob->invoke);

	  cmd = add_prefix_cmd (c_smob->cmd_name, c_smob->cmd_class,
				NULL, c_smob->doc, &c_smob->sub_list,
				allow_unknown, cmd_list);
	}
      else
	{
	  cmd = add_cmd (c_smob->cmd_name, c_smob->cmd_class,
			 c_smob->doc, cmd_list);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (exc);

  /* Note: At this point the command exists in gdb.
     So no more errors after this point.  */

  /* There appears to be no API to set this.  */
  cmd->func = cmdscm_function;
  cmd->destroyer = cmdscm_destroyer;

  c_smob->command = cmd;
  cmd->set_context (c_smob);

  if (gdbscm_is_true (c_smob->complete))
    {
      set_cmd_completer (cmd,
			 scm_is_integer (c_smob->complete)
			 ? cmdscm_completers[scm_to_int (c_smob->complete)].completer
			 : cmdscm_completer);
    }

  /* The owner of this command is not in GC-controlled memory, so we need
     to protect it from GC until the command is deleted.  */
  scm_gc_protect_object (c_smob->containing_scm);

  return SCM_UNSPECIFIED;
}

/* Initialize the Scheme command support.  */

static const scheme_function command_functions[] =
{
  { "make-command", 1, 0, 1, as_a_scm_t_subr (gdbscm_make_command),
    "\
Make a GDB command object.\n\
\n\
  Arguments: name [#:invoke lambda]\n\
      [#:command-class <class>] [#:completer-class <completer>]\n\
      [#:prefix? <bool>] [#:doc string]\n\
    name: The name of the command.  It may consist of multiple words,\n\
      in which case the final word is the name of the new command, and\n\
      earlier words must be prefix commands.\n\
    invoke: A procedure of three arguments to perform the command.\n\
      (lambda (self arg from-tty) ...)\n\
      Its result is unspecified.\n\
    class: The class of the command, one of COMMAND_*.\n\
      The default is COMMAND_NONE.\n\
    completer: The kind of completer, #f, one of COMPLETE_*, or a procedure\n\
      to perform the completion: (lambda (self text word) ...).\n\
    prefix?: If true then the command is a prefix command.\n\
    doc: The \"doc string\" of the command.\n\
  Returns: <gdb:command> object" },

  { "register-command!", 1, 0, 0, as_a_scm_t_subr (gdbscm_register_command_x),
    "\
Register a <gdb:command> object with GDB." },

  { "command?", 1, 0, 0, as_a_scm_t_subr (gdbscm_command_p),
    "\
Return #t if the object is a <gdb:command> object." },

  { "command-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_command_valid_p),
    "\
Return #t if the <gdb:command> object is valid." },

  { "dont-repeat", 1, 0, 0, as_a_scm_t_subr (gdbscm_dont_repeat),
    "\
Prevent command repetition when user enters an empty line.\n\
\n\
  Arguments: <gdb:command>\n\
  Returns: unspecified" },

  END_FUNCTIONS
};

/* Initialize the 'commands' code.  */

void
gdbscm_initialize_commands (void)
{
  int i;

  command_smob_tag
    = gdbscm_make_smob_type (command_smob_name, sizeof (command_smob));
  scm_set_smob_print (command_smob_tag, cmdscm_print_command_smob);

  gdbscm_define_integer_constants (command_classes, 1);
  gdbscm_define_functions (command_functions, 1);

  for (i = 0; i < N_COMPLETERS; ++i)
    {
      scm_c_define (cmdscm_completers[i].name, scm_from_int (i));
      scm_c_export (cmdscm_completers[i].name, NULL);
    }

  invoke_keyword = scm_from_latin1_keyword ("invoke");
  command_class_keyword = scm_from_latin1_keyword ("command-class");
  completer_class_keyword = scm_from_latin1_keyword ("completer-class");
  prefix_p_keyword = scm_from_latin1_keyword ("prefix?");
  doc_keyword = scm_from_latin1_keyword ("doc");
}

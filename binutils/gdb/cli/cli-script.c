/* GDB CLI command scripting.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "value.h"
#include <ctype.h>

#include "ui-out.h"
#include "top.h"
#include "ui.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-script.h"
#include "cli/cli-style.h"
#include "gdbcmd.h"

#include "extension.h"
#include "interps.h"
#include "compile/compile.h"
#include <string_view>
#include "python/python.h"
#include "guile/guile.h"

#include <vector>

/* Prototypes for local functions.  */

static enum command_control_type
recurse_read_control_structure
    (read_next_line_ftype read_next_line_func,
     struct command_line *current_cmd,
     gdb::function_view<void (const char *)> validator);

static void do_define_command (const char *comname, int from_tty,
			       const counted_command_line *commands);

static void do_document_command (const char *comname, int from_tty,
				 const counted_command_line *commands);

static const char *read_next_line (std::string &buffer);

/* Level of control structure when reading.  */
static int control_level;

/* Level of control structure when executing.  */
static int command_nest_depth = 1;

/* This is to prevent certain commands being printed twice.  */
static int suppress_next_print_command_trace = 0;

/* Command element for the 'while' command.  */
static cmd_list_element *while_cmd_element = nullptr;

/* Command element for the 'if' command.  */
static cmd_list_element *if_cmd_element = nullptr;

/* Command element for the 'define' command.  */
static cmd_list_element *define_cmd_element = nullptr;

/* Command element for the 'document' command.  */
static cmd_list_element *document_cmd_element = nullptr;

/* Structure for arguments to user defined functions.  */

class user_args
{
public:
  /* Save the command line and store the locations of arguments passed
     to the user defined function.  */
  explicit user_args (const char *line);

  /* Insert the stored user defined arguments into the $arg arguments
     found in LINE.  */
  std::string insert_args (const char *line) const;

private:
  /* Disable copy/assignment.  (Since the elements of A point inside
     COMMAND, copying would need to reconstruct the A vector in the
     new copy.)  */
  user_args (const user_args &) =delete;
  user_args &operator= (const user_args &) =delete;

  /* It is necessary to store a copy of the command line to ensure
     that the arguments are not overwritten before they are used.  */
  std::string m_command_line;

  /* The arguments.  Each element points inside M_COMMAND_LINE.  */
  std::vector<std::string_view> m_args;
};

/* The stack of arguments passed to user defined functions.  We need a
   stack because user-defined functions can call other user-defined
   functions.  */
static std::vector<std::unique_ptr<user_args>> user_args_stack;

/* An RAII-base class used to push/pop args on the user args
   stack.  */
struct scoped_user_args_level
{
  /* Parse the command line and push the arguments in the user args
     stack.  */
  explicit scoped_user_args_level (const char *line)
  {
    user_args_stack.emplace_back (new user_args (line));
  }

  /* Pop the current user arguments from the stack.  */
  ~scoped_user_args_level ()
  {
    user_args_stack.pop_back ();
  }
};


/* Return non-zero if TYPE is a multi-line command (i.e., is terminated
   by "end").  */

static int
multi_line_command_p (enum command_control_type type)
{
  switch (type)
    {
    case if_control:
    case while_control:
    case while_stepping_control:
    case commands_control:
    case compile_control:
    case python_control:
    case guile_control:
    case define_control:
    case document_control:
      return 1;
    default:
      return 0;
    }
}

/* Allocate, initialize a new command line structure for one of the
   control commands (if/while).  */

static command_line_up
build_command_line (enum command_control_type type, const char *args)
{
  if (args == NULL || *args == '\0')
    {
      if (type == if_control)
	error (_("if command requires an argument."));
      else if (type == while_control)
	error (_("while command requires an argument."));
      else if (type == define_control)
	error (_("define command requires an argument."));
      else if (type == document_control)
	error (_("document command requires an argument."));
    }
  gdb_assert (args != NULL);

  return command_line_up (new command_line (type, xstrdup (args)));
}

/* Build and return a new command structure for the control commands
   such as "if" and "while".  */

counted_command_line
get_command_line (enum command_control_type type, const char *arg)
{
  /* Allocate and build a new command line structure.  */
  counted_command_line cmd (build_command_line (type, arg).release (),
			    command_lines_deleter ());

  /* Read in the body of this command.  */
  if (recurse_read_control_structure (read_next_line, cmd.get (), 0)
      == invalid_control)
    {
      warning (_("Error reading in canned sequence of commands."));
      return NULL;
    }

  return cmd;
}

/* Recursively print a command (including full control structures).  */

void
print_command_lines (struct ui_out *uiout, struct command_line *cmd,
		     unsigned int depth)
{
  struct command_line *list;

  list = cmd;
  while (list)
    {
      if (depth)
	uiout->spaces (2 * depth);

      /* A simple command, print it and continue.  */
      if (list->control_type == simple_control)
	{
	  uiout->field_string (NULL, list->line);
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      /* loop_continue to jump to the start of a while loop, print it
	 and continue. */
      if (list->control_type == continue_control)
	{
	  uiout->field_string (NULL, "loop_continue");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      /* loop_break to break out of a while loop, print it and
	 continue.  */
      if (list->control_type == break_control)
	{
	  uiout->field_string (NULL, "loop_break");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      /* A while command.  Recursively print its subcommands and
	 continue.  */
      if (list->control_type == while_control
	  || list->control_type == while_stepping_control)
	{
	  /* For while-stepping, the line includes the 'while-stepping'
	     token.  See comment in process_next_line for explanation.
	     Here, take care not print 'while-stepping' twice.  */
	  if (list->control_type == while_control)
	    uiout->field_fmt (NULL, "while %s", list->line);
	  else
	    uiout->field_string (NULL, list->line);
	  uiout->text ("\n");
	  print_command_lines (uiout, list->body_list_0.get (), depth + 1);
	  if (depth)
	    uiout->spaces (2 * depth);
	  uiout->field_string (NULL, "end");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      /* An if command.  Recursively print both arms before
	 continuing.  */
      if (list->control_type == if_control)
	{
	  uiout->field_fmt (NULL, "if %s", list->line);
	  uiout->text ("\n");
	  /* The true arm.  */
	  print_command_lines (uiout, list->body_list_0.get (), depth + 1);

	  /* Show the false arm if it exists.  */
	  if (list->body_list_1 != nullptr)
	    {
	      if (depth)
		uiout->spaces (2 * depth);
	      uiout->field_string (NULL, "else");
	      uiout->text ("\n");
	      print_command_lines (uiout, list->body_list_1.get (), depth + 1);
	    }

	  if (depth)
	    uiout->spaces (2 * depth);
	  uiout->field_string (NULL, "end");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      /* A commands command.  Print the breakpoint commands and
	 continue.  */
      if (list->control_type == commands_control)
	{
	  if (*(list->line))
	    uiout->field_fmt (NULL, "commands %s", list->line);
	  else
	    uiout->field_string (NULL, "commands");
	  uiout->text ("\n");
	  print_command_lines (uiout, list->body_list_0.get (), depth + 1);
	  if (depth)
	    uiout->spaces (2 * depth);
	  uiout->field_string (NULL, "end");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      if (list->control_type == python_control)
	{
	  uiout->field_string (NULL, "python");
	  uiout->text ("\n");
	  /* Don't indent python code at all.  */
	  print_command_lines (uiout, list->body_list_0.get (), 0);
	  if (depth)
	    uiout->spaces (2 * depth);
	  uiout->field_string (NULL, "end");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      if (list->control_type == compile_control)
	{
	  uiout->field_string (NULL, "compile expression");
	  uiout->text ("\n");
	  print_command_lines (uiout, list->body_list_0.get (), 0);
	  if (depth)
	    uiout->spaces (2 * depth);
	  uiout->field_string (NULL, "end");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      if (list->control_type == guile_control)
	{
	  uiout->field_string (NULL, "guile");
	  uiout->text ("\n");
	  print_command_lines (uiout, list->body_list_0.get (), depth + 1);
	  if (depth)
	    uiout->spaces (2 * depth);
	  uiout->field_string (NULL, "end");
	  uiout->text ("\n");
	  list = list->next;
	  continue;
	}

      /* Ignore illegal command type and try next.  */
      list = list->next;
    }				/* while (list) */
}

/* Handle pre-post hooks.  */

class scoped_restore_hook_in
{
public:

  scoped_restore_hook_in (struct cmd_list_element *c)
    : m_cmd (c)
  {
  }

  ~scoped_restore_hook_in ()
  {
    m_cmd->hook_in = 0;
  }

  scoped_restore_hook_in (const scoped_restore_hook_in &) = delete;
  scoped_restore_hook_in &operator= (const scoped_restore_hook_in &) = delete;

private:

  struct cmd_list_element *m_cmd;
};

void
execute_cmd_pre_hook (struct cmd_list_element *c)
{
  if ((c->hook_pre) && (!c->hook_in))
    {
      scoped_restore_hook_in restore_hook (c);
      c->hook_in = 1; /* Prevent recursive hooking.  */
      execute_user_command (c->hook_pre, nullptr);
    }
}

void
execute_cmd_post_hook (struct cmd_list_element *c)
{
  if ((c->hook_post) && (!c->hook_in))
    {
      scoped_restore_hook_in restore_hook (c);
      c->hook_in = 1; /* Prevent recursive hooking.  */
      execute_user_command (c->hook_post, nullptr);
    }
}

/* See cli-script.h.  */

void
execute_control_commands (struct command_line *cmdlines, int from_tty)
{
  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);
  scoped_restore save_nesting
    = make_scoped_restore (&command_nest_depth, command_nest_depth + 1);

  while (cmdlines)
    {
      enum command_control_type ret = execute_control_command (cmdlines,
							       from_tty);
      if (ret != simple_control && ret != break_control)
	{
	  warning (_("Error executing canned sequence of commands."));
	  break;
	}
      cmdlines = cmdlines->next;
    }
}

/* See cli-script.h.  */

std::string
execute_control_commands_to_string (struct command_line *commands,
				    int from_tty)
{
  std::string result;

  execute_fn_to_string (result, [&] ()
    {
      execute_control_commands (commands, from_tty);
    }, false);

  return result;
}

void
execute_user_command (struct cmd_list_element *c, const char *args)
{
  counted_command_line cmdlines_copy;

  /* Ensure that the user commands can't be deleted while they are
     executing.  */
  cmdlines_copy = c->user_commands;
  if (cmdlines_copy == 0)
    /* Null command */
    return;
  struct command_line *cmdlines = cmdlines_copy.get ();

  scoped_user_args_level push_user_args (args);

  if (user_args_stack.size () > max_user_call_depth)
    error (_("Max user call depth exceeded -- command aborted."));

  /* Set the instream to nullptr, indicating execution of a
     user-defined function.  */
  scoped_restore restore_instream
    = make_scoped_restore (&current_ui->instream, nullptr);

  execute_control_commands (cmdlines, 0);
}

/* This function is called every time GDB prints a prompt.  It ensures
   that errors and the like do not confuse the command tracing.  */

void
reset_command_nest_depth (void)
{
  command_nest_depth = 1;

  /* Just in case.  */
  suppress_next_print_command_trace = 0;
}

/* Print the command, prefixed with '+' to represent the call depth.
   This is slightly complicated because this function may be called
   from execute_command and execute_control_command.  Unfortunately
   execute_command also prints the top level control commands.
   In these cases execute_command will call execute_control_command
   via while_command or if_command.  Inner levels of 'if' and 'while'
   are dealt with directly.  Therefore we can use these functions
   to determine whether the command has been printed already or not.  */
ATTRIBUTE_PRINTF (1, 2)
void
print_command_trace (const char *fmt, ...)
{
  int i;

  if (suppress_next_print_command_trace)
    {
      suppress_next_print_command_trace = 0;
      return;
    }

  if (!source_verbose && !trace_commands)
    return;

  for (i=0; i < command_nest_depth; i++)
    gdb_printf ("+");

  va_list args;

  va_start (args, fmt);
  gdb_vprintf (fmt, args);
  va_end (args);
  gdb_puts ("\n");
}

/* Helper for execute_control_command.  */

static enum command_control_type
execute_control_command_1 (struct command_line *cmd, int from_tty)
{
  struct command_line *current;
  int loop;
  enum command_control_type ret;

  /* Start by assuming failure, if a problem is detected, the code
     below will simply "break" out of the switch.  */
  ret = invalid_control;

  switch (cmd->control_type)
    {
    case simple_control:
      {
	/* A simple command, execute it and return.  */
	std::string new_line = insert_user_defined_cmd_args (cmd->line);
	execute_command (new_line.c_str (), from_tty);
	ret = cmd->control_type;
	break;
      }

    case continue_control:
      print_command_trace ("loop_continue");

      /* Return for "continue", and "break" so we can either
	 continue the loop at the top, or break out.  */
      ret = cmd->control_type;
      break;

    case break_control:
      print_command_trace ("loop_break");

      /* Return for "continue", and "break" so we can either
	 continue the loop at the top, or break out.  */
      ret = cmd->control_type;
      break;

    case while_control:
      {
	print_command_trace ("while %s", cmd->line);

	/* Parse the loop control expression for the while statement.  */
	std::string new_line = insert_user_defined_cmd_args (cmd->line);
	expression_up expr = parse_expression (new_line.c_str ());

	ret = simple_control;
	loop = 1;

	/* Keep iterating so long as the expression is true.  */
	while (loop == 1)
	  {
	    bool cond_result;

	    QUIT;

	    /* Evaluate the expression.  */
	    {
	      scoped_value_mark mark;
	      value *val = expr->evaluate ();
	      cond_result = value_true (val);
	    }

	    /* If the value is false, then break out of the loop.  */
	    if (!cond_result)
	      break;

	    /* Execute the body of the while statement.  */
	    current = cmd->body_list_0.get ();
	    while (current)
	      {
		scoped_restore save_nesting
		  = make_scoped_restore (&command_nest_depth, command_nest_depth + 1);
		ret = execute_control_command_1 (current, from_tty);

		/* If we got an error, or a "break" command, then stop
		   looping.  */
		if (ret == invalid_control || ret == break_control)
		  {
		    loop = 0;
		    break;
		  }

		/* If we got a "continue" command, then restart the loop
		   at this point.  */
		if (ret == continue_control)
		  break;

		/* Get the next statement.  */
		current = current->next;
	      }
	  }

	/* Reset RET so that we don't recurse the break all the way down.  */
	if (ret == break_control)
	  ret = simple_control;

	break;
      }

    case if_control:
      {
	print_command_trace ("if %s", cmd->line);

	/* Parse the conditional for the if statement.  */
	std::string new_line = insert_user_defined_cmd_args (cmd->line);
	expression_up expr = parse_expression (new_line.c_str ());

	current = NULL;
	ret = simple_control;

	/* Evaluate the conditional.  */
	{
	  scoped_value_mark mark;
	  value *val = expr->evaluate ();

	  /* Choose which arm to take commands from based on the value
	     of the conditional expression.  */
	  if (value_true (val))
	    current = cmd->body_list_0.get ();
	  else if (cmd->body_list_1 != nullptr)
	    current = cmd->body_list_1.get ();
	}

	/* Execute commands in the given arm.  */
	while (current)
	  {
	    scoped_restore save_nesting
	      = make_scoped_restore (&command_nest_depth, command_nest_depth + 1);
	    ret = execute_control_command_1 (current, from_tty);

	    /* If we got an error, get out.  */
	    if (ret != simple_control)
	      break;

	    /* Get the next statement in the body.  */
	    current = current->next;
	  }

	break;
      }

    case commands_control:
      {
	/* Breakpoint commands list, record the commands in the
	   breakpoint's command list and return.  */
	std::string new_line = insert_user_defined_cmd_args (cmd->line);
	ret = commands_from_control_command (new_line.c_str (), cmd);
	break;
      }

    case compile_control:
      eval_compile_command (cmd, NULL, cmd->control_u.compile.scope,
			    cmd->control_u.compile.scope_data);
      ret = simple_control;
      break;

    case define_control:
      print_command_trace ("define %s", cmd->line);
      do_define_command (cmd->line, 0, &cmd->body_list_0);
      ret = simple_control;
      break;

    case document_control:
      print_command_trace ("document %s", cmd->line);
      do_document_command (cmd->line, 0, &cmd->body_list_0);
      ret = simple_control;
      break;

    case python_control:
    case guile_control:
      {
	eval_ext_lang_from_control_command (cmd);
	ret = simple_control;
	break;
      }

    default:
      warning (_("Invalid control type in canned commands structure."));
      break;
    }

  return ret;
}

enum command_control_type
execute_control_command (struct command_line *cmd, int from_tty)
{
  if (!current_uiout->is_mi_like_p ())
    return execute_control_command_1 (cmd, from_tty);

  /* Make sure we use the console uiout.  It's possible that we are executing
     breakpoint commands while running the MI interpreter.  */
  interp *console = interp_lookup (current_ui, INTERP_CONSOLE);
  scoped_restore save_uiout
    = make_scoped_restore (&current_uiout, console->interp_ui_out ());

  return execute_control_command_1 (cmd, from_tty);
}

/* Like execute_control_command, but first set
   suppress_next_print_command_trace.  */

enum command_control_type
execute_control_command_untraced (struct command_line *cmd)
{
  suppress_next_print_command_trace = 1;
  return execute_control_command (cmd);
}


/* "while" command support.  Executes a body of statements while the
   loop condition is nonzero.  */

static void
while_command (const char *arg, int from_tty)
{
  control_level = 1;
  counted_command_line command = get_command_line (while_control, arg);

  if (command == NULL)
    return;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  execute_control_command_untraced (command.get ());
}

/* "if" command support.  Execute either the true or false arm depending
   on the value of the if conditional.  */

static void
if_command (const char *arg, int from_tty)
{
  control_level = 1;
  counted_command_line command = get_command_line (if_control, arg);

  if (command == NULL)
    return;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  execute_control_command_untraced (command.get ());
}

/* Bind the incoming arguments for a user defined command to $arg0,
   $arg1 ... $argN.  */

user_args::user_args (const char *command_line)
{
  const char *p;

  if (command_line == NULL)
    return;

  m_command_line = command_line;
  p = m_command_line.c_str ();

  while (*p)
    {
      const char *start_arg;
      int squote = 0;
      int dquote = 0;
      int bsquote = 0;

      /* Strip whitespace.  */
      while (*p == ' ' || *p == '\t')
	p++;

      /* P now points to an argument.  */
      start_arg = p;

      /* Get to the end of this argument.  */
      while (*p)
	{
	  if (((*p == ' ' || *p == '\t')) && !squote && !dquote && !bsquote)
	    break;
	  else
	    {
	      if (bsquote)
		bsquote = 0;
	      else if (*p == '\\')
		bsquote = 1;
	      else if (squote)
		{
		  if (*p == '\'')
		    squote = 0;
		}
	      else if (dquote)
		{
		  if (*p == '"')
		    dquote = 0;
		}
	      else
		{
		  if (*p == '\'')
		    squote = 1;
		  else if (*p == '"')
		    dquote = 1;
		}
	      p++;
	    }
	}

      m_args.emplace_back (start_arg, p - start_arg);
    }
}

/* Given character string P, return a point to the first argument
   ($arg), or NULL if P contains no arguments.  */

static const char *
locate_arg (const char *p)
{
  while ((p = strchr (p, '$')))
    {
      if (startswith (p, "$arg")
	  && (isdigit (p[4]) || p[4] == 'c'))
	return p;
      p++;
    }
  return NULL;
}

/* See cli-script.h.  */

std::string
insert_user_defined_cmd_args (const char *line)
{
  /* If we are not in a user-defined command, treat $argc, $arg0, et
     cetera as normal convenience variables.  */
  if (user_args_stack.empty ())
    return line;

  const std::unique_ptr<user_args> &args = user_args_stack.back ();
  return args->insert_args (line);
}

/* Insert the user defined arguments stored in user_args into the $arg
   arguments found in line.  */

std::string
user_args::insert_args (const char *line) const
{
  std::string new_line;
  const char *p;

  while ((p = locate_arg (line)))
    {
      new_line.append (line, p - line);

      if (p[4] == 'c')
	{
	  new_line += std::to_string (m_args.size ());
	  line = p + 5;
	}
      else
	{
	  char *tmp;
	  unsigned long i;

	  errno = 0;
	  i = strtoul (p + 4, &tmp, 10);
	  if ((i == 0 && tmp == p + 4) || errno != 0)
	    line = p + 4;
	  else if (i >= m_args.size ())
	    error (_("Missing argument %ld in user function."), i);
	  else
	    {
	      new_line.append (m_args[i].data (), m_args[i].length ());
	      line = tmp;
	    }
	}
    }
  /* Don't forget the tail.  */
  new_line.append (line);

  return new_line;
}


/* Read next line from stdin.  Passed to read_command_line_1 and
   recurse_read_control_structure whenever we need to read commands
   from stdin.  */

static const char *
read_next_line (std::string &buffer)
{
  struct ui *ui = current_ui;
  char *prompt_ptr, control_prompt[256];
  int i = 0;
  int from_tty = ui->instream == ui->stdin_stream;

  if (control_level >= 254)
    error (_("Control nesting too deep!"));

  /* Set a prompt based on the nesting of the control commands.  */
  if (from_tty
      || (ui->instream == 0 && deprecated_readline_hook != NULL))
    {
      for (i = 0; i < control_level; i++)
	control_prompt[i] = ' ';
      control_prompt[i] = '>';
      control_prompt[i + 1] = '\0';
      prompt_ptr = (char *) &control_prompt[0];
    }
  else
    prompt_ptr = NULL;

  return command_line_input (buffer, prompt_ptr, "commands");
}

/* Given an input line P, skip the command and return a pointer to the
   first argument.  */

static const char *
line_first_arg (const char *p)
{
  const char *first_arg = p + find_command_name_length (p);

  return skip_spaces (first_arg); 
}

/* Process one input line.  If the command is an "end", return such an
   indication to the caller.  If PARSE_COMMANDS is true, strip leading
   whitespace (trailing whitespace is always stripped) in the line,
   attempt to recognize GDB control commands, and also return an
   indication if the command is an "else" or a nop.

   Otherwise, only "end" is recognized.  */

static enum misc_command_type
process_next_line (const char *p, command_line_up *command,
		   int parse_commands,
		   gdb::function_view<void (const char *)> validator)

{
  const char *p_end;
  const char *p_start;
  int not_handled = 0;

  /* Not sure what to do here.  */
  if (p == NULL)
    return end_command;

  /* Strip trailing whitespace.  */
  p_end = p + strlen (p);
  while (p_end > p && (p_end[-1] == ' ' || p_end[-1] == '\t'))
    p_end--;

  p_start = p;
  /* Strip leading whitespace.  */
  while (p_start < p_end && (*p_start == ' ' || *p_start == '\t'))
    p_start++;

  /* 'end' is always recognized, regardless of parse_commands value.
     We also permit whitespace before end and after.  */
  if (p_end - p_start == 3 && startswith (p_start, "end"))
    return end_command;

  if (parse_commands)
    {
      /* Resolve command abbreviations (e.g. 'ws' for 'while-stepping').  */
      const char *cmd_name = p;
      struct cmd_list_element *cmd
	= lookup_cmd_1 (&cmd_name, cmdlist, NULL, NULL, 1);
      cmd_name = skip_spaces (cmd_name);
      bool inline_cmd = *cmd_name != '\0';

      /* If commands are parsed, we skip initial spaces.  Otherwise,
	 which is the case for Python commands and documentation
	 (see the 'document' command), spaces are preserved.  */
      p = p_start;

      /* Blanks and comments don't really do anything, but we need to
	 distinguish them from else, end and other commands which can
	 be executed.  */
      if (p_end == p || p[0] == '#')
	return nop_command;

      /* Is the else clause of an if control structure?  */
      if (p_end - p == 4 && startswith (p, "else"))
	return else_command;

      /* Check for while, if, break, continue, etc and build a new
	 command line structure for them.  */
      if (cmd == while_stepping_cmd_element)
	{
	  /* Because validate_actionline and encode_action lookup
	     command's line as command, we need the line to
	     include 'while-stepping'.

	     For 'ws' alias, the command will have 'ws', not expanded
	     to 'while-stepping'.  This is intentional -- we don't
	     really want frontend to send a command list with 'ws',
	     and next break-info returning command line with
	     'while-stepping'.  This should work, but might cause the
	     breakpoint to be marked as changed while it's actually
	     not.  */
	  *command = build_command_line (while_stepping_control, p);
	}
      else if (cmd == while_cmd_element)
	*command = build_command_line (while_control, line_first_arg (p));
      else if (cmd == if_cmd_element)
	*command = build_command_line (if_control, line_first_arg (p));
      else if (cmd == commands_cmd_element)
	*command = build_command_line (commands_control, line_first_arg (p));
      else if (cmd == define_cmd_element)
	*command = build_command_line (define_control, line_first_arg (p));
      else if (cmd == document_cmd_element)
	*command = build_command_line (document_control, line_first_arg (p));
      else if (cmd == python_cmd_element && !inline_cmd)
	{
	  /* Note that we ignore the inline "python command" form
	     here.  */
	  *command = build_command_line (python_control, "");
	}
      else if (cmd == compile_cmd_element && !inline_cmd)
	{
	  /* Note that we ignore the inline "compile command" form
	     here.  */
	  *command = build_command_line (compile_control, "");
	  (*command)->control_u.compile.scope = COMPILE_I_INVALID_SCOPE;
	}
      else if (cmd == guile_cmd_element && !inline_cmd)
	{
	  /* Note that we ignore the inline "guile command" form here.  */
	  *command = build_command_line (guile_control, "");
	}
      else if (p_end - p == 10 && startswith (p, "loop_break"))
	*command = command_line_up (new command_line (break_control));
      else if (p_end - p == 13 && startswith (p, "loop_continue"))
	*command = command_line_up (new command_line (continue_control));
      else
	not_handled = 1;
    }

  if (!parse_commands || not_handled)
    {
      /* A normal command.  */
      *command = command_line_up (new command_line (simple_control,
						    savestring (p, p_end - p)));
    }

  if (validator)
    validator ((*command)->line);

  /* Nothing special.  */
  return ok_command;
}

/* Recursively read in the control structures and create a
   command_line structure from them.  Use read_next_line_func to
   obtain lines of the command.  */

static enum command_control_type
recurse_read_control_structure (read_next_line_ftype read_next_line_func,
				struct command_line *current_cmd,
				gdb::function_view<void (const char *)> validator)
{
  enum misc_command_type val;
  enum command_control_type ret;
  struct command_line *child_tail;
  counted_command_line *current_body = &current_cmd->body_list_0;
  command_line_up next;

  child_tail = nullptr;

  /* Sanity checks.  */
  if (current_cmd->control_type == simple_control)
    error (_("Recursed on a simple control type."));

  /* Read lines from the input stream and build control structures.  */
  while (1)
    {
      dont_repeat ();

      std::string buffer;
      next = nullptr;
      val = process_next_line (read_next_line_func (buffer), &next,
			       current_cmd->control_type != python_control
			       && current_cmd->control_type != guile_control
			       && current_cmd->control_type != compile_control,
			       validator);

      /* Just skip blanks and comments.  */
      if (val == nop_command)
	continue;

      if (val == end_command)
	{
	  if (multi_line_command_p (current_cmd->control_type))
	    {
	      /* Success reading an entire canned sequence of commands.  */
	      ret = simple_control;
	      break;
	    }
	  else
	    {
	      ret = invalid_control;
	      break;
	    }
	}

      /* Not the end of a control structure.  */
      if (val == else_command)
	{
	  if (current_cmd->control_type == if_control
	      && current_body == &current_cmd->body_list_0)
	    {
	      current_body = &current_cmd->body_list_1;
	      child_tail = nullptr;
	      continue;
	    }
	  else
	    {
	      ret = invalid_control;
	      break;
	    }
	}

      /* Transfer ownership of NEXT to the command's body list.  */
      if (child_tail != nullptr)
	{
	  child_tail->next = next.release ();
	  child_tail = child_tail->next;
	}
      else
	{
	  child_tail = next.get ();
	  *current_body = counted_command_line (next.release (),
						command_lines_deleter ());
	}

      /* If the latest line is another control structure, then recurse
	 on it.  */
      if (multi_line_command_p (child_tail->control_type))
	{
	  control_level++;
	  ret = recurse_read_control_structure (read_next_line_func,
						child_tail,
						validator);
	  control_level--;

	  if (ret != simple_control)
	    break;
	}
    }

  dont_repeat ();

  return ret;
}

/* Read lines from the input stream and accumulate them in a chain of
   struct command_line's, which is then returned.  For input from a
   terminal, the special command "end" is used to mark the end of the
   input, and is not included in the returned chain of commands.

   If PARSE_COMMANDS is true, strip leading whitespace (trailing whitespace
   is always stripped) in the line and attempt to recognize GDB control
   commands.  Otherwise, only "end" is recognized.  */

#define END_MESSAGE "End with a line saying just \"end\"."

counted_command_line
read_command_lines (const char *prompt_arg, int from_tty, int parse_commands,
		    gdb::function_view<void (const char *)> validator)
{
  if (from_tty && current_ui->input_interactive_p ())
    {
      if (deprecated_readline_begin_hook)
	{
	  /* Note - intentional to merge messages with no newline.  */
	  (*deprecated_readline_begin_hook) ("%s  %s\n", prompt_arg,
					     END_MESSAGE);
	}
      else
	printf_unfiltered ("%s\n%s\n", prompt_arg, END_MESSAGE);
    }


  /* Reading commands assumes the CLI behavior, so temporarily
     override the current interpreter with CLI.  */
  counted_command_line head (nullptr, command_lines_deleter ());
  if (current_interp_named_p (INTERP_CONSOLE))
    head = read_command_lines_1 (read_next_line, parse_commands,
				 validator);
  else
    {
      scoped_restore_interp interp_restorer (INTERP_CONSOLE);

      head = read_command_lines_1 (read_next_line, parse_commands,
				   validator);
    }

  if (from_tty && current_ui->input_interactive_p ()
      && deprecated_readline_end_hook)
    {
      (*deprecated_readline_end_hook) ();
    }
  return (head);
}

/* Act the same way as read_command_lines, except that each new line is
   obtained using READ_NEXT_LINE_FUNC.  */

counted_command_line
read_command_lines_1 (read_next_line_ftype read_next_line_func,
		      int parse_commands,
		      gdb::function_view<void (const char *)> validator)
{
  struct command_line *tail;
  counted_command_line head (nullptr, command_lines_deleter ());
  enum command_control_type ret;
  enum misc_command_type val;
  command_line_up next;

  control_level = 0;
  tail = NULL;

  while (1)
    {
      dont_repeat ();

      std::string buffer;
      val = process_next_line (read_next_line_func (buffer), &next, parse_commands,
			       validator);

      /* Ignore blank lines or comments.  */
      if (val == nop_command)
	continue;

      if (val == end_command)
	{
	  ret = simple_control;
	  break;
	}

      if (val != ok_command)
	{
	  ret = invalid_control;
	  break;
	}

      if (multi_line_command_p (next->control_type))
	{
	  control_level++;
	  ret = recurse_read_control_structure (read_next_line_func, next.get (),
						validator);
	  control_level--;

	  if (ret == invalid_control)
	    break;
	}

      /* Transfer ownership of NEXT to the HEAD list.  */
      if (tail)
	{
	  tail->next = next.release ();
	  tail = tail->next;
	}
      else
	{
	  tail = next.get ();
	  head = counted_command_line (next.release (),
				       command_lines_deleter ());
	}
    }

  dont_repeat ();

  if (ret == invalid_control)
    return NULL;

  return head;
}

/* Free a chain of struct command_line's.  */

void
free_command_lines (struct command_line **lptr)
{
  struct command_line *l = *lptr;
  struct command_line *next;

  while (l)
    {
      next = l->next;
      delete l;
      l = next;
    }
  *lptr = NULL;
}

/* Validate that *COMNAME is a valid name for a command.  Return the
   containing command list, in case it starts with a prefix command.
   The prefix must already exist.  *COMNAME is advanced to point after
   any prefix, and a NUL character overwrites the space after the
   prefix.  */

static struct cmd_list_element **
validate_comname (const char **comname)
{
  struct cmd_list_element **list = &cmdlist;
  const char *p, *last_word;

  if (*comname == 0)
    error_no_arg (_("name of command to define"));

  /* Find the last word of the argument.  */
  p = *comname + strlen (*comname);
  while (p > *comname && isspace (p[-1]))
    p--;
  while (p > *comname && !isspace (p[-1]))
    p--;
  last_word = p;

  /* Find the corresponding command list.  */
  if (last_word != *comname)
    {
      struct cmd_list_element *c;

      /* Separate the prefix and the command.  */
      std::string prefix (*comname, last_word - 1);
      const char *tem = prefix.c_str ();

      c = lookup_cmd (&tem, cmdlist, "", NULL, 0, 1);
      if (!c->is_prefix ())
	error (_("\"%s\" is not a prefix command."), prefix.c_str ());

      list = c->subcommands;
      *comname = last_word;
    }

  p = *comname;
  while (*p)
    {
      if (!valid_cmd_char_p (*p))
	error (_("Junk in argument list: \"%s\""), p);
      p++;
    }

  return list;
}

/* This is just a placeholder in the command data structures.  */
static void
user_defined_command (const char *ignore, int from_tty)
{
}

/* Define a user-defined command.  If COMMANDS is NULL, then this is a
   top-level call and the commands will be read using
   read_command_lines.  Otherwise, it is a "define" command in an
   existing command and the commands are provided.  In the
   non-top-level case, various prompts and warnings are disabled.  */

static void
do_define_command (const char *comname, int from_tty,
		   const counted_command_line *commands)
{
  enum cmd_hook_type
    {
      CMD_NO_HOOK = 0,
      CMD_PRE_HOOK,
      CMD_POST_HOOK
    };
  struct cmd_list_element *c, *newc, *hookc = 0, **list;
  const char *comfull;
  int  hook_type      = CMD_NO_HOOK;
  int  hook_name_size = 0;
   
#define	HOOK_STRING	"hook-"
#define	HOOK_LEN 5
#define HOOK_POST_STRING "hookpost-"
#define HOOK_POST_LEN    9

  comfull = comname;
  list = validate_comname (&comname);

  c = lookup_cmd_exact (comname, *list);

  if (c && commands == nullptr)
    {
      int q;

      if (c->theclass == class_user || c->theclass == class_alias)
	{
	  /* if C is a prefix command that was previously defined,
	     tell the user its subcommands will be kept, and ask
	     if ok to redefine the command.  */
	  if (c->is_prefix ())
	    q = (c->user_commands.get () == nullptr
		 || query (_("Keeping subcommands of prefix command \"%s\".\n"
			     "Redefine command \"%s\"? "), c->name, c->name));
	  else
	    q = query (_("Redefine command \"%s\"? "), c->name);
	}
      else
	q = query (_("Really redefine built-in command \"%s\"? "), c->name);
      if (!q)
	error (_("Command \"%s\" not redefined."), c->name);
    }

  /* If this new command is a hook, then mark the command which it
     is hooking.  Note that we allow hooking `help' commands, so that
     we can hook the `stop' pseudo-command.  */

  if (!strncmp (comname, HOOK_STRING, HOOK_LEN))
    {
       hook_type      = CMD_PRE_HOOK;
       hook_name_size = HOOK_LEN;
    }
  else if (!strncmp (comname, HOOK_POST_STRING, HOOK_POST_LEN))
    {
      hook_type      = CMD_POST_HOOK;
      hook_name_size = HOOK_POST_LEN;
    }

  if (hook_type != CMD_NO_HOOK)
    {
      /* Look up cmd it hooks.  */
      hookc = lookup_cmd_exact (comname + hook_name_size, *list,
				/* ignore_help_classes = */ false);
      if (!hookc && commands == nullptr)
	{
	  warning (_("Your new `%s' command does not "
		     "hook any existing command."),
		   comfull);
	  if (!query (_("Proceed? ")))
	    error (_("Not confirmed."));
	}
    }

  comname = xstrdup (comname);

  counted_command_line cmds;
  if (commands == nullptr)
    {
      std::string prompt
	= string_printf ("Type commands for definition of \"%s\".", comfull);
      cmds = read_command_lines (prompt.c_str (), from_tty, 1, 0);
    }
  else
    cmds = *commands;

  {
    struct cmd_list_element **c_subcommands
      = c == nullptr ? nullptr : c->subcommands;

    newc = add_cmd (comname, class_user, user_defined_command,
		    (c != nullptr && c->theclass == class_user)
		    ? c->doc : xstrdup ("User-defined."), list);
    newc->user_commands = std::move (cmds);

    /* If we define or re-define a command that was previously defined
       as a prefix, keep the prefix information.  */
    if (c_subcommands != nullptr)
      {
	newc->subcommands = c_subcommands;
	/* allow_unknown: see explanation in equivalent logic in
	   define_prefix_command ().  */
	newc->allow_unknown = newc->user_commands.get () != nullptr;
    }
  }

  /* If this new command is a hook, then mark both commands as being
     tied.  */
  if (hookc)
    {
      switch (hook_type)
	{
	case CMD_PRE_HOOK:
	  hookc->hook_pre  = newc;  /* Target gets hooked.  */
	  newc->hookee_pre = hookc; /* We are marked as hooking target cmd.  */
	  break;
	case CMD_POST_HOOK:
	  hookc->hook_post  = newc;  /* Target gets hooked.  */
	  newc->hookee_post = hookc; /* We are marked as hooking
					target cmd.  */
	  break;
	default:
	  /* Should never come here as hookc would be 0.  */
	  internal_error (_("bad switch"));
	}
    }
}

static void
define_command (const char *comname, int from_tty)
{
  do_define_command (comname, from_tty, nullptr);
}

/* Document a user-defined command or user defined alias.  If COMMANDS is NULL,
   then this is a top-level call and the document will be read using
   read_command_lines.  Otherwise, it is a "document" command in an existing
   command and the commands are provided.  */
static void
do_document_command (const char *comname, int from_tty,
		     const counted_command_line *commands)
{
  struct cmd_list_element *alias, *prefix_cmd, *c;
  const char *comfull;

  comfull = comname;
  validate_comname (&comname);

  lookup_cmd_composition (comfull, &alias, &prefix_cmd, &c);
  if (c == nullptr)
    error (_("Undefined command: \"%s\"."), comfull);

  if (c->theclass != class_user
      && (alias == nullptr || alias->theclass != class_alias))
    {
      if (alias == nullptr)
	error (_("Command \"%s\" is built-in."), comfull);
      else
	error (_("Alias \"%s\" is built-in."), comfull);
    }

  /* If we found an alias of class_alias, the user is documenting this
     user-defined alias.  */
  if (alias != nullptr)
    c = alias;

  counted_command_line doclines;

  if (commands == nullptr)
    {
      std::string prompt
	= string_printf ("Type documentation for \"%s\".", comfull);
      doclines = read_command_lines (prompt.c_str (), from_tty, 0, 0);
    }
  else
    doclines = *commands;

  if (c->doc_allocated)
    xfree ((char *) c->doc);

  {
    struct command_line *cl1;
    int len = 0;
    char *doc;

    for (cl1 = doclines.get (); cl1; cl1 = cl1->next)
      len += strlen (cl1->line) + 1;

    doc = (char *) xmalloc (len + 1);
    *doc = 0;

    for (cl1 = doclines.get (); cl1; cl1 = cl1->next)
      {
	strcat (doc, cl1->line);
	if (cl1->next)
	  strcat (doc, "\n");
      }

    c->doc = doc;
    c->doc_allocated = 1;
  }
}

static void
document_command (const char *comname, int from_tty)
{
  do_document_command (comname, from_tty, nullptr);
}

/* Implementation of the "define-prefix" command.  */

static void
define_prefix_command (const char *comname, int from_tty)
{
  struct cmd_list_element *c, **list;
  const char *comfull;

  comfull = comname;
  list = validate_comname (&comname);

  c = lookup_cmd_exact (comname, *list);

  if (c != nullptr && c->theclass != class_user)
    error (_("Command \"%s\" is built-in."), comfull);

  if (c != nullptr && c->is_prefix ())
    {
      /* c is already a user defined prefix command.  */
      return;
    }

  /* If the command does not exist at all, create it.  */
  if (c == nullptr)
    {
      comname = xstrdup (comname);
      c = add_cmd (comname, class_user, user_defined_command,
		   xstrdup ("User-defined."), list);
    }

  /* Allocate the c->subcommands, which marks the command as a prefix
     command.  */
  c->subcommands = new struct cmd_list_element*;
  *(c->subcommands) = nullptr;
  /* If the prefix command C is not a command, then it must be followed
     by known subcommands.  Otherwise, if C is also a normal command,
     it can be followed by C args that must not cause a 'subcommand'
     not recognised error, and thus we must allow unknown.  */
  c->allow_unknown = c->user_commands.get () != nullptr;
}


/* Used to implement source_command.  */

void
script_from_file (FILE *stream, const char *file)
{
  if (stream == NULL)
    internal_error (_("called with NULL file pointer!"));

  scoped_restore restore_line_number
    = make_scoped_restore (&source_line_number, 0);
  scoped_restore restore_file
    = make_scoped_restore<std::string, const std::string &> (&source_file_name,
							     file);

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  try
    {
      read_command_file (stream);
    }
  catch (const gdb_exception_error &e)
    {
      /* Re-throw the error, but with the file name information
	 prepended.  */
      throw_error (e.error,
		   _("%s:%d: Error in sourced command file:\n%s"),
		   source_file_name.c_str (), source_line_number,
		   e.what ());
    }
}

/* Print the definition of user command C to STREAM.  Or, if C is a
   prefix command, show the definitions of all user commands under C
   (recursively).  PREFIX and NAME combined are the name of the
   current command.  */
void
show_user_1 (struct cmd_list_element *c, const char *prefix, const char *name,
	     struct ui_file *stream)
{
  if (cli_user_command_p (c))
    {
      struct command_line *cmdlines = c->user_commands.get ();

      gdb_printf (stream, "User %scommand \"",
		  c->is_prefix () ? "prefix" : "");
      fprintf_styled (stream, title_style.style (), "%s%s",
		      prefix, name);
      gdb_printf (stream, "\":\n");
      if (cmdlines)
	{
	  print_command_lines (current_uiout, cmdlines, 1);
	  gdb_puts ("\n", stream);
	}
    }

  if (c->is_prefix ())
    {
      const std::string prefixname = c->prefixname ();

      for (c = *c->subcommands; c != NULL; c = c->next)
	if (c->theclass == class_user || c->is_prefix ())
	  show_user_1 (c, prefixname.c_str (), c->name, gdb_stdout);
    }

}

void _initialize_cli_script ();
void
_initialize_cli_script ()
{
  struct cmd_list_element *c;

  /* "document", "define" and "define-prefix" use command_completer,
     as this helps the user to either type the command name and/or
     its prefixes.  */
  document_cmd_element = add_com ("document", class_support, document_command,
				  _("\
Document a user-defined command or user-defined alias.\n\
Give command or alias name as argument.  Give documentation on following lines.\n\
End with a line of just \"end\"."));
  set_cmd_completer (document_cmd_element, command_completer);
  define_cmd_element = add_com ("define", class_support, define_command, _("\
Define a new command name.  Command name is argument.\n\
Definition appears on following lines, one command per line.\n\
End with a line of just \"end\".\n\
Use the \"document\" command to give documentation for the new command.\n\
Commands defined in this way may accept an unlimited number of arguments\n\
accessed via $arg0 .. $argN.  $argc tells how many arguments have\n\
been passed."));
  set_cmd_completer (define_cmd_element, command_completer);
  c = add_com ("define-prefix", class_support, define_prefix_command,
	   _("\
Define or mark a command as a user-defined prefix command.\n\
User defined prefix commands can be used as prefix commands for\n\
other user defined commands.\n\
If the command already exists, it is changed to a prefix command."));
  set_cmd_completer (c, command_completer);

  while_cmd_element = add_com ("while", class_support, while_command, _("\
Execute nested commands WHILE the conditional expression is non zero.\n\
The conditional expression must follow the word `while' and must in turn be\n\
followed by a new line.  The nested commands must be entered one per line,\n\
and should be terminated by the word `end'."));

  if_cmd_element = add_com ("if", class_support, if_command, _("\
Execute nested commands once IF the conditional expression is non zero.\n\
The conditional expression must follow the word `if' and must in turn be\n\
followed by a new line.  The nested commands must be entered one per line,\n\
and should be terminated by the word 'else' or `end'.  If an else clause\n\
is used, the same rules apply to its nested commands as to the first ones."));
}

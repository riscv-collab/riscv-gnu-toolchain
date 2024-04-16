/* Handle set and show GDB commands.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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
#include "readline/tilde.h"
#include "value.h"
#include <ctype.h>
#include "arch-utils.h"
#include "observable.h"
#include "interps.h"

#include "ui-out.h"

#include "cli/cli-decode.h"
#include "cli/cli-cmds.h"
#include "cli/cli-setshow.h"
#include "cli/cli-utils.h"

/* Return true if the change of command parameter should be notified.  */

static bool
notify_command_param_changed_p (bool param_changed, struct cmd_list_element *c)
{
  if (!param_changed)
    return false;

  return c->theclass != class_maintenance && c->theclass != class_obscure;
}


static enum auto_boolean
parse_auto_binary_operation (const char *arg)
{
  if (arg != NULL && *arg != '\0')
    {
      int length = strlen (arg);

      while (isspace (arg[length - 1]) && length > 0)
	length--;

      /* Note that "o" is ambiguous.  */

      if ((length == 2 && strncmp (arg, "on", length) == 0)
	  || strncmp (arg, "1", length) == 0
	  || strncmp (arg, "yes", length) == 0
	  || strncmp (arg, "enable", length) == 0)
	return AUTO_BOOLEAN_TRUE;
      else if ((length >= 2 && strncmp (arg, "off", length) == 0)
	       || strncmp (arg, "0", length) == 0
	       || strncmp (arg, "no", length) == 0
	       || strncmp (arg, "disable", length) == 0)
	return AUTO_BOOLEAN_FALSE;
      else if (strncmp (arg, "auto", length) == 0
	       || (length > 1 && strncmp (arg, "-1", length) == 0))
	return AUTO_BOOLEAN_AUTO;
    }
  error (_("\"on\", \"off\" or \"auto\" expected."));
  return AUTO_BOOLEAN_AUTO; /* Pacify GCC.  */
}

/* See cli-setshow.h.  */

int
parse_cli_boolean_value (const char **arg)
{
  const char *p = skip_to_space (*arg);
  size_t length = p - *arg;

  /* Note that "o" is ambiguous.  */

  if ((length == 2 && strncmp (*arg, "on", length) == 0)
      || strncmp (*arg, "1", length) == 0
      || strncmp (*arg, "yes", length) == 0
      || strncmp (*arg, "enable", length) == 0)
    {
      *arg = skip_spaces (*arg + length);
      return 1;
    }
  else if ((length >= 2 && strncmp (*arg, "off", length) == 0)
	   || strncmp (*arg, "0", length) == 0
	   || strncmp (*arg, "no", length) == 0
	   || strncmp (*arg, "disable", length) == 0)
    {
      *arg = skip_spaces (*arg + length);
      return 0;
    }
  else
    return -1;
}

/* See cli-setshow.h.  */

int
parse_cli_boolean_value (const char *arg)
{
  if (!arg || !*arg)
    return 1;

  int b = parse_cli_boolean_value (&arg);
  if (b >= 0 && *arg != '\0')
    return -1;

  return b;
}


static void
deprecated_show_value_hack (struct ui_file *ignore_file,
			    int ignore_from_tty,
			    struct cmd_list_element *c,
			    const char *value)
{
  /* If there's no command or value, don't try to print it out.  */
  if (c == NULL || value == NULL)
    return;

  /* Print doc minus "Show " at start.  Tell print_doc_line that
     this is for a 'show value' prefix.  */
  print_doc_line (gdb_stdout, c->doc + 5, true);

  gdb_assert (c->var.has_value ());

  switch (c->var->type ())
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      gdb_printf ((" is \"%s\".\n"), value);
      break;

    default:
      gdb_printf ((" is %s.\n"), value);
      break;
    }
}

/* Returns true and the value in VAL if ARG is an accepted literal.  */

static bool
get_literal_val (LONGEST &val, const literal_def *extra_literals,
		 const char **arg, bool expression)
{
  *arg = skip_spaces (*arg);

  const char *unl_start = *arg;

  const char *p = skip_to_space (*arg);

  size_t len = p - *arg;

  if (len > 0 && extra_literals != nullptr)
    for (const literal_def *l = extra_literals;
	 l->literal != nullptr;
	 l++)
      if (strncmp (l->literal, *arg, len) == 0)
	{
	  *arg += len;

	  /* If parsing an expression (i.e., parsing for a "set" command),
	     anything after the literal is junk.  For options, anything
	     after the literal might be a command argument or another
	     option.  */
	  if (expression)
	    {
	      const char *after = skip_spaces (*arg);
	      if (*after != '\0')
		error (_("Junk after \"%.*s\": %s"),
		       (int) len, unl_start, after);
	    }

	  val = l->use;
	  return true;
	}

  return false;
}

/* See cli-setshow.h.  */

LONGEST
parse_cli_var_integer (var_types var_type, const literal_def *extra_literals,
		       const char **arg, bool expression)
{
  LONGEST val;

  if (*arg == nullptr || **arg == '\0')
    {
      if (extra_literals == nullptr)
	error_no_arg (_("integer to set it to"));
      else
	{
	  std::string buffer = "";
	  size_t count = 0;

	  for (const literal_def *l = extra_literals;
	       l->literal != nullptr;
	       l++, count++)
	    {
	      if (count != 0)
		buffer += ", ";
	      buffer = buffer + '"' + l->literal + '"';
	    }
	  if (count > 1)
	    error_no_arg
	      (string_printf (_("integer to set it to, or one of: %s"),
			      buffer.c_str ()).c_str ());
	  else
	    error_no_arg
	      (string_printf (_("integer to set it to, or %s"),
			      buffer.c_str ()).c_str ());
	}
    }

  if (!get_literal_val (val, extra_literals, arg, expression))
    {
      if (expression)
	val = parse_and_eval_long (*arg);
      else
	val = get_ulongest (arg);

      enum tribool allowed = TRIBOOL_UNKNOWN;
      if (extra_literals != nullptr)
	{
	  for (const literal_def *l = extra_literals;
	       l->literal != nullptr;
	       l++)
	    if (l->val.has_value () && val == *l->val)
	      {
		allowed = TRIBOOL_TRUE;
		val = l->use;
		break;
	      }
	    else if (val == l->use)
	      allowed = TRIBOOL_FALSE;
	}

      if (allowed == TRIBOOL_UNKNOWN)
	{
	  if (val > UINT_MAX || val < INT_MIN
	      || (var_type == var_uinteger && val < 0)
	      || (var_type == var_integer && val > INT_MAX)
	      || (var_type == var_pinteger && val < 0)
	      || (var_type == var_pinteger && val > INT_MAX))
	    allowed = TRIBOOL_FALSE;
	}
      if (allowed == TRIBOOL_FALSE)
	error (_("integer %s out of range"), plongest (val));
    }

  return val;
}

/* See cli-setshow.h.  */

const char *
parse_cli_var_enum (const char **args, const char *const *enums)
{
  /* If no argument was supplied, print an informative error
     message.  */
  if (args == NULL || *args == NULL || **args == '\0')
    {
      std::string msg;

      for (size_t i = 0; enums[i]; i++)
	{
	  if (i != 0)
	    msg += ", ";
	  msg += enums[i];
	}
      error (_("Requires an argument. Valid arguments are %s."),
	     msg.c_str ());
    }

  const char *p = skip_to_space (*args);
  size_t len = p - *args;

  int nmatches = 0;
  const char *match = NULL;
  for (size_t i = 0; enums[i]; i++)
    if (strncmp (*args, enums[i], len) == 0)
      {
	if (enums[i][len] == '\0')
	  {
	    match = enums[i];
	    nmatches = 1;
	    break; /* Exact match.  */
	  }
	else
	  {
	    match = enums[i];
	    nmatches++;
	  }
      }

  if (nmatches == 0)
    error (_("Undefined item: \"%.*s\"."), (int) len, *args);

  if (nmatches > 1)
    error (_("Ambiguous item \"%.*s\"."), (int) len, *args);

  *args += len;
  return match;
}

/* Do a "set" command.  ARG is NULL if no argument, or the
   text of the argument, and FROM_TTY is nonzero if this command is
   being entered directly by the user (i.e. these are just like any
   other command).  C is the command list element for the command.  */

void
do_set_command (const char *arg, int from_tty, struct cmd_list_element *c)
{
  /* A flag to indicate the option is changed or not.  */
  bool option_changed = false;

  gdb_assert (c->type == set_cmd);

  if (arg == NULL)
    arg = "";

  gdb_assert (c->var.has_value ());

  switch (c->var->type ())
    {
    case var_string:
      {
	std::string newobj;
	const char *p;
	int ch;

	newobj.reserve (strlen (arg));
	p = arg;
	while ((ch = *p++) != '\000')
	  {
	    if (ch == '\\')
	      {
		/* \ at end of argument is used after spaces
		   so they won't be lost.  */
		/* This is obsolete now that we no longer strip
		   trailing whitespace and actually, the backslash
		   didn't get here in my test, readline or
		   something did something funky with a backslash
		   right before a newline.  */
		if (*p == 0)
		  break;
		ch = parse_escape (get_current_arch (), &p);
		if (ch == 0)
		  break;	/* C loses */
		else if (ch > 0)
		  newobj.push_back (ch);
	      }
	    else
	      newobj.push_back (ch);
	  }
	newobj.shrink_to_fit ();

	option_changed = c->var->set<std::string> (std::move (newobj));
      }
      break;
    case var_string_noescape:
      option_changed = c->var->set<std::string> (std::string (arg));
      break;
    case var_filename:
      if (*arg == '\0')
	error_no_arg (_("filename to set it to."));
      [[fallthrough]];
    case var_optional_filename:
      {
	char *val = NULL;

	if (*arg != '\0')
	  {
	    /* Clear trailing whitespace of filename.  */
	    const char *ptr = arg + strlen (arg) - 1;

	    while (ptr >= arg && (*ptr == ' ' || *ptr == '\t'))
	      ptr--;
	    gdb::unique_xmalloc_ptr<char> copy
	      = make_unique_xstrndup (arg, ptr + 1 - arg);

	    val = tilde_expand (copy.get ());
	  }
	else
	  val = xstrdup ("");

	option_changed
	  = c->var->set<std::string> (std::string (val));
	xfree (val);
      }
      break;
    case var_boolean:
      {
	int val = parse_cli_boolean_value (arg);

	if (val < 0)
	  error (_("\"on\" or \"off\" expected."));

	option_changed = c->var->set<bool> (val);
      }
      break;
    case var_auto_boolean:
      option_changed = c->var->set<enum auto_boolean> (parse_auto_binary_operation (arg));
      break;
    case var_uinteger:
      option_changed
	= c->var->set<unsigned int> (parse_cli_var_integer (c->var->type (),
							    c->var->
							    extra_literals (),
							    &arg, true));
      break;
    case var_integer:
    case var_pinteger:
      option_changed
	= c->var->set<int> (parse_cli_var_integer (c->var->type (),
						   c->var->extra_literals (),
						   &arg, true));
      break;
    case var_enum:
      {
	const char *end_arg = arg;
	const char *match = parse_cli_var_enum (&end_arg, c->enums);

	int len = end_arg - arg;
	const char *after = skip_spaces (end_arg);
	if (*after != '\0')
	  error (_("Junk after item \"%.*s\": %s"), len, arg, after);

	option_changed = c->var->set<const char *> (match);
      }
      break;
    default:
      error (_("gdb internal error: bad var_type in do_setshow_command"));
    }

  c->func (NULL, from_tty, c);

  if (notify_command_param_changed_p (option_changed, c))
    {
      char *name, *cp;
      struct cmd_list_element **cmds;
      struct cmd_list_element *p;
      int i;
      int length = 0;

      /* Compute the whole multi-word command options.  If user types command
	 'set foo bar baz on', c->name is 'baz', and GDB can't pass "bar" to
	 command option change notification, because it is confusing.  We can
	 trace back through field 'prefix' to compute the whole options,
	 and pass "foo bar baz" to notification.  */

      for (i = 0, p = c; p != NULL; i++)
	{
	  length += strlen (p->name);
	  length++;

	  p = p->prefix;
	}
      cp = name = (char *) xmalloc (length);
      cmds = XNEWVEC (struct cmd_list_element *, i);

      /* Track back through filed 'prefix' and cache them in CMDS.  */
      for (i = 0, p = c; p != NULL; i++)
	{
	  cmds[i] = p;
	  p = p->prefix;
	}

      /* Don't trigger any observer notification if subcommands is not
	 setlist.  */
      i--;
      if (cmds[i]->subcommands != &setlist)
	{
	  xfree (cmds);
	  xfree (name);

	  return;
	}
      /* Traverse them in the reversed order, and copy their names into
	 NAME.  */
      for (i--; i >= 0; i--)
	{
	  memcpy (cp, cmds[i]->name, strlen (cmds[i]->name));
	  cp += strlen (cmds[i]->name);

	  if (i != 0)
	    {
	      cp[0] = ' ';
	      cp++;
	    }
	}
      cp[0] = 0;

      xfree (cmds);

      switch (c->var->type ())
	{
	case var_string:
	case var_string_noescape:
	case var_filename:
	case var_optional_filename:
	  interps_notify_param_changed
	    (name, c->var->get<std::string> ().c_str ());
	  break;
	case var_enum:
	  interps_notify_param_changed
	    (name, c->var->get<const char *> ());
	  break;
	case var_boolean:
	  {
	    const char *opt = c->var->get<bool> () ? "on" : "off";

	    interps_notify_param_changed (name, opt);
	  }
	  break;
	case var_auto_boolean:
	  {
	    const char *s
	      = auto_boolean_enums[c->var->get<enum auto_boolean> ()];

	    interps_notify_param_changed (name, s);
	  }
	  break;
	case var_uinteger:
	  {
	    char s[64];

	    xsnprintf (s, sizeof s, "%u", c->var->get<unsigned int> ());
	    interps_notify_param_changed (name, s);
	  }
	  break;
	case var_integer:
	case var_pinteger:
	  {
	    char s[64];

	    xsnprintf (s, sizeof s, "%d", c->var->get<int> ());
	    interps_notify_param_changed (name, s);
	  }
	  break;
	}
      xfree (name);
    }
}

/* See cli/cli-setshow.h.  */

std::string
get_setshow_command_value_string (const setting &var)
{
  string_file stb;

  switch (var.type ())
    {
    case var_string:
      {
	std::string value = var.get<std::string> ();
	if (!value.empty ())
	  stb.putstr (value.c_str (), '"');
      }
      break;
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
      stb.puts (var.get<std::string> ().c_str ());
      break;
    case var_enum:
      {
	const char *value = var.get<const char *> ();
	if (value != nullptr)
	  stb.puts (value);
      }
      break;
    case var_boolean:
      stb.puts (var.get<bool> () ? "on" : "off");
      break;
    case var_auto_boolean:
      switch (var.get<enum auto_boolean> ())
	{
	case AUTO_BOOLEAN_TRUE:
	  stb.puts ("on");
	  break;
	case AUTO_BOOLEAN_FALSE:
	  stb.puts ("off");
	  break;
	case AUTO_BOOLEAN_AUTO:
	  stb.puts ("auto");
	  break;
	default:
	  gdb_assert_not_reached ("invalid var_auto_boolean");
	  break;
	}
      break;
    case var_uinteger:
    case var_integer:
    case var_pinteger:
      {
	bool printed = false;
	const LONGEST value
	  = (var.type () == var_uinteger
	     ? static_cast<LONGEST> (var.get<unsigned int> ())
	     : static_cast<LONGEST> (var.get<int> ()));

	if (var.extra_literals () != nullptr)
	  for (const literal_def *l = var.extra_literals ();
	       l->literal != nullptr;
	       l++)
	    if (value == l->use)
	      {
		stb.puts (l->literal);
		printed = true;
		break;
	      }
	if (!printed)
	  {
	    if (var.type () == var_uinteger)
	      stb.printf ("%u", static_cast<unsigned int> (value));
	    else
	      stb.printf ("%d", static_cast<int> (value));
	  }
      }
      break;
    default:
      gdb_assert_not_reached ("bad var_type");
    }

  return stb.release ();
}


/* Do a "show" command.  ARG is NULL if no argument, or the
   text of the argument, and FROM_TTY is nonzero if this command is
   being entered directly by the user (i.e. these are just like any
   other command).  C is the command list element for the command.  */

void
do_show_command (const char *arg, int from_tty, struct cmd_list_element *c)
{
  struct ui_out *uiout = current_uiout;

  gdb_assert (c->type == show_cmd);
  gdb_assert (c->var.has_value ());

  std::string val = get_setshow_command_value_string (*c->var);

  /* FIXME: cagney/2005-02-10: There should be MI and CLI specific
     versions of code to print the value out.  */

  if (uiout->is_mi_like_p ())
    uiout->field_string ("value", val);
  else
    {
      if (c->show_value_func != NULL)
	c->show_value_func (gdb_stdout, from_tty, c, val.c_str ());
      else
	deprecated_show_value_hack (gdb_stdout, from_tty, c, val.c_str ());
    }

  c->func (NULL, from_tty, c);
}

/* Show all the settings in a list of show commands.  */

void
cmd_show_list (struct cmd_list_element *list, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  ui_out_emit_tuple tuple_emitter (uiout, "showlist");
  for (; list != NULL; list = list->next)
    {
      /* We skip show command aliases to avoid showing duplicated values.  */

      /* If we find a prefix, run its list, prefixing our output by its
	 prefix (with "show " skipped).  */
      if (list->is_prefix () && !list->is_alias ())
	{
	  ui_out_emit_tuple optionlist_emitter (uiout, "optionlist");
	  std::string prefixname = list->prefixname ();
	  const char *new_prefix = strstr (prefixname.c_str (), "show ") + 5;

	  if (uiout->is_mi_like_p ())
	    uiout->field_string ("prefix", new_prefix);
	  cmd_show_list (*list->subcommands, from_tty);
	}
      else if (list->theclass != no_set_class && !list->is_alias ())
	{
	  ui_out_emit_tuple option_emitter (uiout, "option");

	  if (list->prefix != nullptr)
	    {
	      /* If we find a prefix, output it (with "show " skipped).  */
	      std::string prefixname = list->prefix->prefixname ();
	      prefixname = (!list->prefix->is_prefix () ? ""
			    : strstr (prefixname.c_str (), "show ") + 5);
	      uiout->text (prefixname);
	    }
	  uiout->field_string ("name", list->name);
	  uiout->text (":  ");
	  if (list->type == show_cmd)
	    do_show_command (NULL, from_tty, list);
	  else
	    cmd_func (list, NULL, from_tty);
	}
    }
}



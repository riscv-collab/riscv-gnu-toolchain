/* CLI options framework, for GDB.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "cli/cli-option.h"
#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include "cli/cli-setshow.h"
#include "command.h"
#include <vector>

namespace gdb {
namespace option {

/* An option's value.  Which field is active depends on the option's
   type.  */
union option_value
{
  /* For var_boolean options.  */
  bool boolean;

  /* For var_uinteger options.  */
  unsigned int uinteger;

  /* For var_integer and var_pinteger options.  */
  int integer;

  /* For var_enum options.  */
  const char *enumeration;

  /* For var_string options.  This is malloc-allocated.  */
  std::string *string;
};

/* Holds an options definition and its value.  */
struct option_def_and_value
{
  /* The option definition.  */
  const option_def &option;

  /* A context.  */
  void *ctx;

  /* The option's value, if any.  */
  std::optional<option_value> value;

  /* Constructor.  */
  option_def_and_value (const option_def &option_, void *ctx_,
			std::optional<option_value> &&value_ = {})
    : option (option_),
      ctx (ctx_),
      value (std::move (value_))
  {
    clear_value (option_, value_);
  }

  /* Move constructor.  Need this because for some types the values
     are allocated on the heap.  */
  option_def_and_value (option_def_and_value &&rval)
    : option (rval.option),
      ctx (rval.ctx),
      value (std::move (rval.value))
  {
    clear_value (rval.option, rval.value);
  }

  DISABLE_COPY_AND_ASSIGN (option_def_and_value);

  ~option_def_and_value ()
  {
    if (value.has_value ())
      {
	if (option.type == var_string)
	  delete value->string;
      }
  }

private:

  /* Clear the option_value, without releasing it.  This is used after
     the value has been moved to some other option_def_and_value
     instance.  This is needed because for some types the value is
     allocated on the heap, so we must clear the pointer in the
     source, to avoid a double free.  */
  static void clear_value (const option_def &option,
			   std::optional<option_value> &value)
  {
    if (value.has_value ())
      {
	if (option.type == var_string)
	  value->string = nullptr;
      }
  }
};

static void save_option_value_in_ctx (std::optional<option_def_and_value> &ov);

/* Info passed around when handling completion.  */
struct parse_option_completion_info
{
  /* The completion word.  */
  const char *word;

  /* The tracker.  */
  completion_tracker &tracker;
};

/* If ARGS starts with "-", look for a "--" delimiter.  If one is
   found, then interpret everything up until the "--" as command line
   options.  Otherwise, interpret unknown input as the beginning of
   the command's operands.  */

static const char *
find_end_options_delimiter (const char *args)
{
  if (args[0] == '-')
    {
      const char *p = args;

      p = skip_spaces (p);
      while (*p)
	{
	  if (check_for_argument (&p, "--"))
	    return p;
	  else
	    p = skip_to_space (p);
	  p = skip_spaces (p);
	}
    }

  return nullptr;
}

/* Complete TEXT/WORD on all options in OPTIONS_GROUP.  */

static void
complete_on_options (gdb::array_view<const option_def_group> options_group,
		     completion_tracker &tracker,
		     const char *text, const char *word)
{
  size_t textlen = strlen (text);
  for (const auto &grp : options_group)
    for (const auto &opt : grp.options)
      if (strncmp (opt.name, text, textlen) == 0)
	{
	  tracker.add_completion
	    (make_completion_match_str (opt.name, text, word));
	}
}

/* See cli-option.h.  */

void
complete_on_all_options (completion_tracker &tracker,
			 gdb::array_view<const option_def_group> options_group)
{
  static const char opt[] = "-";
  complete_on_options (options_group, tracker, opt + 1, opt);
}

/* Parse ARGS, guided by OPTIONS_GROUP.  HAVE_DELIMITER is true if the
   whole ARGS line included the "--" options-terminator delimiter.  */

static std::optional<option_def_and_value>
parse_option (gdb::array_view<const option_def_group> options_group,
	      process_options_mode mode,
	      bool have_delimiter,
	      const char **args,
	      parse_option_completion_info *completion = nullptr)
{
  if (*args == nullptr)
    return {};
  else if (**args != '-')
    {
      if (have_delimiter)
	error (_("Unrecognized option at: %s"), *args);
      return {};
    }
  else if (check_for_argument (args, "--"))
    return {};

  /* Skip the initial '-'.  */
  const char *arg = *args + 1;

  const char *after = skip_to_space (arg);
  size_t len = after - arg;
  const option_def *match = nullptr;
  void *match_ctx = nullptr;

  for (const auto &grp : options_group)
    {
      for (const auto &o : grp.options)
	{
	  if (strncmp (o.name, arg, len) == 0)
	    {
	      if (match != nullptr)
		{
		  if (completion != nullptr && arg[len] == '\0')
		    {
		      complete_on_options (options_group,
					   completion->tracker,
					   arg, completion->word);
		      return {};
		    }

		  error (_("Ambiguous option at: -%s"), arg);
		}

	      match = &o;
	      match_ctx = grp.ctx;

	      if ((isspace (arg[len]) || arg[len] == '\0')
		  && strlen (o.name) == len)
		break; /* Exact match.  */
	    }
	}
    }

  if (match == nullptr)
    {
      if (have_delimiter || mode != PROCESS_OPTIONS_UNKNOWN_IS_OPERAND)
	error (_("Unrecognized option at: %s"), *args);

      return {};
    }

  if (completion != nullptr && arg[len] == '\0')
    {
      complete_on_options (options_group, completion->tracker,
			   arg, completion->word);
      return {};
    }

  *args += 1 + len;
  *args = skip_spaces (*args);
  if (completion != nullptr)
    completion->word = *args;

  switch (match->type)
    {
    case var_boolean:
      {
	if (!match->have_argument)
	  {
	    option_value val;
	    val.boolean = true;
	    return option_def_and_value {*match, match_ctx, val};
	  }

	const char *val_str = *args;
	int res;

	if (**args == '\0' && completion != nullptr)
	  {
	    /* Complete on both "on/off" and more options.  */

	    if (mode == PROCESS_OPTIONS_REQUIRE_DELIMITER)
	      {
		complete_on_enum (completion->tracker,
				  boolean_enums, val_str, val_str);
		complete_on_all_options (completion->tracker, options_group);
	      }
	    return option_def_and_value {*match, match_ctx};
	  }
	else if (**args == '-')
	  {
	    /* Treat:
		 "cmd -boolean-option -another-opt..."
	       as:
		 "cmd -boolean-option on -another-opt..."
	     */
	    res = 1;
	  }
	else if (**args == '\0')
	  {
	    /* Treat:
		 (1) "cmd -boolean-option "
	       as:
		 (1) "cmd -boolean-option on"
	     */
	    res = 1;
	  }
	else
	  {
	    res = parse_cli_boolean_value (args);
	    if (res < 0)
	      {
		const char *end = skip_to_space (*args);
		if (completion != nullptr)
		  {
		    if (*end == '\0')
		      {
			complete_on_enum (completion->tracker,
					  boolean_enums, val_str, val_str);
			return option_def_and_value {*match, match_ctx};
		      }
		  }

		if (have_delimiter)
		  error (_("Value given for `-%s' is not a boolean: %.*s"),
			 match->name, (int) (end - val_str), val_str);
		/* The user didn't separate options from operands
		   using "--", so treat this unrecognized value as the
		   start of the operands.  This makes "frame apply all
		   -past-main CMD" work.  */
		return option_def_and_value {*match, match_ctx};
	      }
	    else if (completion != nullptr && **args == '\0')
	      {
		/* While "cmd -boolean [TAB]" only offers "on" and
		   "off", the boolean option actually accepts "1",
		   "yes", etc. as boolean values.  We complete on all
		   of those instead of BOOLEAN_ENUMS here to make
		   these work:

		    "p -object 1[TAB]" -> "p -object 1 "
		    "p -object ye[TAB]" -> "p -object yes "

		   Etc.  Note that it's important that the space is
		   auto-appended.  Otherwise, if we only completed on
		   on/off here, then it might look to the user like
		   "1" isn't valid, like:
		   "p -object 1[TAB]" -> "p -object 1" (i.e., nothing happens).
		*/
		static const char *const all_boolean_enums[] = {
		  "on", "off",
		  "yes", "no",
		  "enable", "disable",
		  "0", "1",
		  nullptr,
		};
		complete_on_enum (completion->tracker, all_boolean_enums,
				  val_str, val_str);
		return {};
	      }
	  }

	option_value val;
	val.boolean = res;
	return option_def_and_value {*match, match_ctx, val};
      }
    case var_uinteger:
    case var_integer:
    case var_pinteger:
      {
	if (completion != nullptr && match->extra_literals != nullptr)
	  {
	    /* Convenience to let the user know what the option can
	       accept.  Make sure there's no common prefix between
	       "NUMBER" and all the strings when adding new ones,
	       so that readline doesn't do a partial match.  */
	    if (**args == '\0')
	      {
		completion->tracker.add_completion
		  (make_unique_xstrdup ("NUMBER"));
		for (const literal_def *l = match->extra_literals;
		     l->literal != nullptr;
		     l++)
		  completion->tracker.add_completion
		    (make_unique_xstrdup (l->literal));
		return {};
	      }
	    else
	      {
		bool completions = false;
		for (const literal_def *l = match->extra_literals;
		     l->literal != nullptr;
		     l++)
		  if (startswith (l->literal, *args))
		    {
		      completion->tracker.add_completion
			(make_unique_xstrdup (l->literal));
		      completions = true;
		    }
		if (completions)
		  return {};
	      }
	  }

	LONGEST v = parse_cli_var_integer (match->type,
					   match->extra_literals,
					   args, false);
	option_value val;
	if (match->type == var_uinteger)
	  val.uinteger = v;
	else
	  val.integer = v;
	return option_def_and_value {*match, match_ctx, val};
      }
    case var_enum:
      {
	if (completion != nullptr)
	  {
	    const char *after_arg = skip_to_space (*args);
	    if (*after_arg == '\0')
	      {
		complete_on_enum (completion->tracker,
				  match->enums, *args, *args);
		if (completion->tracker.have_completions ())
		  return {};

		/* If we don't have completions, let the
		   non-completion path throw on invalid enum value
		   below, so that completion processing stops.  */
	      }
	  }

	if (check_for_argument (args, "--"))
	  {
	    /* Treat e.g., "backtrace -entry-values --" as if there
	       was no argument after "-entry-values".  This makes
	       parse_cli_var_enum throw an error with a suggestion of
	       what are the valid options.  */
	    args = nullptr;
	  }

	option_value val;
	val.enumeration = parse_cli_var_enum (args, match->enums);
	return option_def_and_value {*match, match_ctx, val};
      }
    case var_string:
      {
	if (check_for_argument (args, "--"))
	  {
	    /* Treat e.g., "maint test-options -string --" as if there
	       was no argument after "-string".  */
	    error (_("-%s requires an argument"), match->name);
	  }

	const char *arg_start = *args;
	std::string str = extract_string_maybe_quoted (args);
	if (*args == arg_start)
	  error (_("-%s requires an argument"), match->name);

	option_value val;
	val.string = new std::string (std::move (str));
	return option_def_and_value {*match, match_ctx, val};
      }

    default:
      /* Not yet.  */
      gdb_assert_not_reached ("option type not supported");
    }

  return {};
}

/* See cli-option.h.  */

bool
complete_options (completion_tracker &tracker,
		  const char **args,
		  process_options_mode mode,
		  gdb::array_view<const option_def_group> options_group)
{
  const char *text = *args;

  tracker.set_use_custom_word_point (true);

  const char *delimiter = find_end_options_delimiter (text);
  bool have_delimiter = delimiter != nullptr;

  if (text[0] == '-' && (!have_delimiter || *delimiter == '\0'))
    {
      parse_option_completion_info completion_info {nullptr, tracker};

      while (1)
	{
	  *args = skip_spaces (*args);
	  completion_info.word = *args;

	  if (strcmp (*args, "-") == 0)
	    {
	      complete_on_options (options_group, tracker, *args + 1,
				   completion_info.word);
	    }
	  else if (strcmp (*args, "--") == 0)
	    {
	      tracker.add_completion (make_unique_xstrdup (*args));
	    }
	  else if (**args == '-')
	    {
	      std::optional<option_def_and_value> ov
		= parse_option (options_group, mode, have_delimiter,
				args, &completion_info);
	      if (!ov && !tracker.have_completions ())
		{
		  tracker.advance_custom_word_point_by (*args - text);
		  return mode == PROCESS_OPTIONS_REQUIRE_DELIMITER;
		}

	      if (ov
		  && ov->option.type == var_boolean
		  && !ov->value.has_value ())
		{
		  /* Looked like a boolean option, but we failed to
		     parse the value.  If this command requires a
		     delimiter, this value can't be the start of the
		     operands, so return true.  Otherwise, if the
		     command doesn't require a delimiter return false
		     so that the caller tries to complete on the
		     operand.  */
		  tracker.advance_custom_word_point_by (*args - text);
		  return mode == PROCESS_OPTIONS_REQUIRE_DELIMITER;
		}

	      /* If we parsed an option with an argument, and reached
		 the end of the input string with no trailing space,
		 return true, so that our callers don't try to
		 complete anything by themselves.  E.g., this makes it
		 so that with:

		  (gdb) frame apply all -limit 10[TAB]

		 we don't try to complete on command names.  */
	      if (ov
		  && !tracker.have_completions ()
		  && **args == '\0'
		  && *args > text && !isspace ((*args)[-1]))
		{
		  tracker.advance_custom_word_point_by
		    (*args - text);
		  return true;
		}

	      /* If the caller passed in a context, then it is
		 interested in the option argument values.  */
	      if (ov && ov->ctx != nullptr)
		save_option_value_in_ctx (ov);
	    }
	  else
	    {
	      tracker.advance_custom_word_point_by
		(completion_info.word - text);

	      /* If the command requires a delimiter, but we haven't
		 seen one, then return true, so that the caller
		 doesn't try to complete on whatever follows options,
		 which for these commands should only be done if
		 there's a delimiter.  */
	      if (mode == PROCESS_OPTIONS_REQUIRE_DELIMITER
		  && !have_delimiter)
		{
		  /* If we reached the end of the input string, then
		     offer all options, since that's all the user can
		     type (plus "--").  */
		  if (completion_info.word[0] == '\0')
		    complete_on_all_options (tracker, options_group);
		  return true;
		}
	      else
		return false;
	    }

	  if (tracker.have_completions ())
	    {
	      tracker.advance_custom_word_point_by
		(completion_info.word - text);
	      return true;
	    }
	}
    }
  else if (delimiter != nullptr)
    {
      tracker.advance_custom_word_point_by (delimiter - text);
      *args = delimiter;
      return false;
    }

  return false;
}

/* Save the parsed value in the option's context.  */

static void
save_option_value_in_ctx (std::optional<option_def_and_value> &ov)
{
  switch (ov->option.type)
    {
    case var_boolean:
      {
	bool value = ov->value.has_value () ? ov->value->boolean : true;
	*ov->option.var_address.boolean (ov->option, ov->ctx) = value;
      }
      break;
    case var_uinteger:
      *ov->option.var_address.uinteger (ov->option, ov->ctx)
	= ov->value->uinteger;
      break;
    case var_integer:
    case var_pinteger:
      *ov->option.var_address.integer (ov->option, ov->ctx)
	= ov->value->integer;
      break;
    case var_enum:
      *ov->option.var_address.enumeration (ov->option, ov->ctx)
	= ov->value->enumeration;
      break;
    case var_string:
      *ov->option.var_address.string (ov->option, ov->ctx)
	= std::move (*ov->value->string);
      break;
    default:
      gdb_assert_not_reached ("unhandled option type");
    }
}

/* See cli-option.h.  */

bool
process_options (const char **args,
		 process_options_mode mode,
		 gdb::array_view<const option_def_group> options_group)
{
  if (*args == nullptr)
    return false;

  /* If ARGS starts with "-", look for a "--" sequence.  If one is
     found, then interpret everything up until the "--" as
     'gdb::option'-style command line options.  Otherwise, interpret
     ARGS as possibly the command's operands.  */
  bool have_delimiter = find_end_options_delimiter (*args) != nullptr;

  if (mode == PROCESS_OPTIONS_REQUIRE_DELIMITER && !have_delimiter)
    return false;

  bool processed_any = false;

  while (1)
    {
      *args = skip_spaces (*args);

      auto ov = parse_option (options_group, mode, have_delimiter, args);
      if (!ov)
	{
	  if (processed_any)
	    return true;
	  return false;
	}

      processed_any = true;

      save_option_value_in_ctx (ov);
    }
}

/* Helper for build_help.  Return a fragment of a help string showing
   OPT's possible values.  Returns NULL if OPT doesn't take an
   argument.  */

static const char *
get_val_type_str (const option_def &opt, std::string &buffer)
{
  if (!opt.have_argument)
    return nullptr;

  switch (opt.type)
    {
    case var_boolean:
      return "[on|off]";
    case var_uinteger:
    case var_integer:
    case var_pinteger:
      {
	buffer = "NUMBER";
	if (opt.extra_literals != nullptr)
	  for (const literal_def *l = opt.extra_literals;
	       l->literal != nullptr;
	       l++)
	    {
	      buffer += '|';
	      buffer += l->literal;
	    }
	return buffer.c_str ();
      }
    case var_enum:
      {
	buffer = "";
	for (size_t i = 0; opt.enums[i] != nullptr; i++)
	  {
	    if (i != 0)
	      buffer += "|";
	    buffer += opt.enums[i];
	  }
	return buffer.c_str ();
      }
    case var_string:
      return "STRING";
    default:
      return nullptr;
    }
}

/* Helper for build_help.  Appends an indented version of DOC into
   HELP.  */

static void
append_indented_doc (const char *doc, std::string &help)
{
  const char *p = doc;
  const char *n = strchr (p, '\n');

  while (n != nullptr)
    {
      help += "    ";
      help.append (p, n - p + 1);
      p = n + 1;
      n = strchr (p, '\n');
    }
  help += "    ";
  help += p;
}

/* Fill HELP with an auto-generated "help" string fragment for
   OPTIONS.  */

static void
build_help_option (gdb::array_view<const option_def> options,
		   std::string &help)
{
  std::string buffer;

  for (const auto &o : options)
    {
      if (o.set_doc == nullptr)
	continue;

      help += "  -";
      help += o.name;

      const char *val_type_str = get_val_type_str (o, buffer);
      if (val_type_str != nullptr)
	{
	  help += ' ';
	  help += val_type_str;
	}
      help += "\n";
      append_indented_doc (o.set_doc, help);
      if (o.help_doc != nullptr)
	{
	  help += "\n";
	  append_indented_doc (o.help_doc, help);
	}
    }
}

/* See cli-option.h.  */

std::string
build_help (const char *help_tmpl,
	    gdb::array_view<const option_def_group> options_group)
{
  bool need_newlines = false;
  std::string help_str;

  const char *p = strstr (help_tmpl, "%OPTIONS%");
  help_str.assign (help_tmpl, p);

  for (const auto &grp : options_group)
    for (const auto &opt : grp.options)
      {
	if (need_newlines)
	  help_str += "\n\n";
	else
	  need_newlines = true;
	build_help_option (opt, help_str);
      }

  p += strlen ("%OPTIONS%");
  help_str.append (p);

  return help_str;
}

/* See cli-option.h.  */

void
add_setshow_cmds_for_options (command_class cmd_class,
			      void *data,
			      gdb::array_view<const option_def> options,
			      struct cmd_list_element **set_list,
			      struct cmd_list_element **show_list)
{
  for (const auto &option : options)
    {
      if (option.type == var_boolean)
	{
	  add_setshow_boolean_cmd (option.name, cmd_class,
				   option.var_address.boolean (option, data),
				   option.set_doc, option.show_doc,
				   option.help_doc,
				   nullptr, option.show_cmd_cb,
				   set_list, show_list);
	}
      else if (option.type == var_uinteger)
	{
	  add_setshow_uinteger_cmd (option.name, cmd_class,
				    option.var_address.uinteger (option, data),
				    option.extra_literals,
				    option.set_doc, option.show_doc,
				    option.help_doc,
				    nullptr, option.show_cmd_cb,
				    set_list, show_list);
	}
      else if (option.type == var_integer)
	{
	  add_setshow_integer_cmd (option.name, cmd_class,
				   option.var_address.integer (option, data),
				   option.extra_literals,
				   option.set_doc, option.show_doc,
				   option.help_doc,
				   nullptr, option.show_cmd_cb,
				   set_list, show_list);
	}
      else if (option.type == var_pinteger)
	{
	  add_setshow_pinteger_cmd (option.name, cmd_class,
				    option.var_address.integer (option, data),
				    option.extra_literals,
				    option.set_doc, option.show_doc,
				    option.help_doc,
				    nullptr, option.show_cmd_cb,
				    set_list, show_list);
	}
      else if (option.type == var_enum)
	{
	  add_setshow_enum_cmd (option.name, cmd_class,
				option.enums,
				option.var_address.enumeration (option, data),
				option.set_doc, option.show_doc,
				option.help_doc,
				nullptr, option.show_cmd_cb,
				set_list, show_list);
	}
      else if (option.type == var_string)
	{
	  add_setshow_string_cmd (option.name, cmd_class,
				  option.var_address.string (option, data),
				  option.set_doc, option.show_doc,
				  option.help_doc,
				  nullptr, option.show_cmd_cb,
				  set_list, show_list);
	}
      else
	gdb_assert_not_reached ("option type not handled");
    }
}

} /* namespace option */
} /* namespace gdb */

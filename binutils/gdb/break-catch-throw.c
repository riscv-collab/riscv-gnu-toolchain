/* Everything about catch/throw catchpoints, for GDB.

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
#include "arch-utils.h"
#include <ctype.h>
#include "breakpoint.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "annotate.h"
#include "valprint.h"
#include "cli/cli-utils.h"
#include "completer.h"
#include "gdbsupport/gdb_obstack.h"
#include "mi/mi-common.h"
#include "linespec.h"
#include "probe.h"
#include "objfiles.h"
#include "cp-abi.h"
#include "gdbsupport/gdb_regex.h"
#include "cp-support.h"
#include "location.h"
#include "cli/cli-decode.h"

/* Each spot where we may place an exception-related catchpoint has
   two names: the SDT probe point and the function name.  This
   structure holds both.  */

struct exception_names
{
  /* The name of the probe point to try, in the form accepted by
     'parse_probes'.  */

  const char *probe;

  /* The name of the corresponding function.  */

  const char *function;
};

/* Names of the probe points and functions on which to break.  This is
   indexed by exception_event_kind.  */
static const struct exception_names exception_functions[] =
{
  { "-probe-stap libstdcxx:throw", "__cxa_throw" },
  { "-probe-stap libstdcxx:rethrow", "__cxa_rethrow" },
  { "-probe-stap libstdcxx:catch", "__cxa_begin_catch" }
};

/* The type of an exception catchpoint.  Unlike most catchpoints, this
   one is implemented with code breakpoints, so it inherits struct
   code_breakpoint, not struct catchpoint.  */

struct exception_catchpoint : public code_breakpoint
{
  exception_catchpoint (struct gdbarch *gdbarch,
			bool temp, const char *cond_string_,
			enum exception_event_kind kind_,
			std::string &&except_rx)
    : code_breakpoint (gdbarch, bp_catchpoint, temp, cond_string_),
      kind (kind_),
      exception_rx (std::move (except_rx)),
      pattern (exception_rx.empty ()
	       ? nullptr
	       : new compiled_regex (exception_rx.c_str (), REG_NOSUB,
				     _("invalid type-matching regexp")))
  {
    pspace = current_program_space;
    re_set ();
  }

  void re_set () override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  bool print_one (const bp_location **) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;
  void print_one_detail (struct ui_out *) const override;
  void check_status (struct bpstat *bs) override;
  struct bp_location *allocate_location () override;

  /* The kind of exception catchpoint.  */

  enum exception_event_kind kind;

  /* If not empty, a string holding the source form of the regular
     expression to match against.  */

  std::string exception_rx;

  /* If non-NULL, a compiled regular expression which is used to
     determine which exceptions to stop on.  */

  std::unique_ptr<compiled_regex> pattern;
};

/* See breakpoint.h.  */

bool
is_exception_catchpoint (breakpoint *bp)
{
  return dynamic_cast<exception_catchpoint *> (bp) != nullptr;
}



/* A helper function that fetches exception probe arguments.  This
   fills in *ARG0 (if non-NULL) and *ARG1 (which must be non-NULL).
   It will throw an exception on any kind of failure.  */

static void
fetch_probe_arguments (struct value **arg0, struct value **arg1)
{
  frame_info_ptr frame = get_selected_frame (_("No frame selected"));
  CORE_ADDR pc = get_frame_pc (frame);
  struct bound_probe pc_probe;
  unsigned n_args;

  pc_probe = find_probe_by_pc (pc);
  if (pc_probe.prob == NULL)
    error (_("did not find exception probe (does libstdcxx have SDT probes?)"));

  if (pc_probe.prob->get_provider () != "libstdcxx"
      || (pc_probe.prob->get_name () != "catch"
	  && pc_probe.prob->get_name () != "throw"
	  && pc_probe.prob->get_name () != "rethrow"))
    error (_("not stopped at a C++ exception catchpoint"));

  n_args = pc_probe.prob->get_argument_count (get_frame_arch (frame));
  if (n_args < 2)
    error (_("C++ exception catchpoint has too few arguments"));

  if (arg0 != NULL)
    *arg0 = pc_probe.prob->evaluate_argument (0, frame);
  *arg1 = pc_probe.prob->evaluate_argument (1, frame);

  if ((arg0 != NULL && *arg0 == NULL) || *arg1 == NULL)
    error (_("error computing probe argument at c++ exception catchpoint"));
}



/* Implement the 'check_status' method.  */

void
exception_catchpoint::check_status (struct bpstat *bs)
{
  std::string type_name;

  this->breakpoint::check_status (bs);
  if (!bs->stop)
    return;

  if (this->pattern == NULL)
    return;

  const char *name = nullptr;
  gdb::unique_xmalloc_ptr<char> canon;
  try
    {
      struct value *typeinfo_arg;

      fetch_probe_arguments (NULL, &typeinfo_arg);
      type_name = cplus_typename_from_type_info (typeinfo_arg);

      canon = cp_canonicalize_string (type_name.c_str ());
      name = (canon != nullptr
	      ? canon.get ()
	      : type_name.c_str ());
    }
  catch (const gdb_exception_error &e)
    {
      exception_print (gdb_stderr, e);
    }

  if (name != nullptr)
    {
      if (this->pattern->exec (name, 0, NULL, 0) != 0)
	bs->stop = false;
    }
}

/* Implement the 're_set' method.  */

void
exception_catchpoint::re_set ()
{
  std::vector<symtab_and_line> sals;
  struct program_space *filter_pspace = current_program_space;

  /* We first try to use the probe interface.  */
  try
    {
      location_spec_up locspec
	= new_probe_location_spec (exception_functions[kind].probe);
      sals = parse_probes (locspec.get (), filter_pspace, NULL);
    }
  catch (const gdb_exception_error &e)
    {
      /* Using the probe interface failed.  Let's fallback to the normal
	 catchpoint mode.  */
      try
	{
	  location_spec_up locspec
	    = (new_explicit_location_spec_function
	       (exception_functions[kind].function));
	  sals = this->decode_location_spec (locspec.get (), filter_pspace);
	}
      catch (const gdb_exception_error &ex)
	{
	  /* NOT_FOUND_ERROR just means the breakpoint will be
	     pending, so let it through.  */
	  if (ex.error != NOT_FOUND_ERROR)
	    throw;
	}
    }

  update_breakpoint_locations (this, filter_pspace, sals, {});
}

enum print_stop_action
exception_catchpoint::print_it (const bpstat *bs) const
{
  struct ui_out *uiout = current_uiout;
  int bp_temp;

  annotate_catchpoint (number);
  maybe_print_thread_hit_breakpoint (uiout);

  bp_temp = disposition == disp_del;
  uiout->text (bp_temp ? "Temporary catchpoint "
		       : "Catchpoint ");
  print_num_locno (bs, uiout);
  uiout->text ((kind == EX_EVENT_THROW ? " (exception thrown), "
		: (kind == EX_EVENT_CATCH ? " (exception caught), "
		   : " (exception rethrown), ")));
  if (uiout->is_mi_like_p ())
    {
      uiout->field_string ("reason",
			   async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      uiout->field_string ("disp", bpdisp_text (disposition));
    }
  return PRINT_SRC_AND_LOC;
}

bool
exception_catchpoint::print_one (const bp_location **last_loc) const
{
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);

  if (opts.addressprint)
    uiout->field_skip ("addr");
  annotate_field (5);

  switch (kind)
    {
    case EX_EVENT_THROW:
      uiout->field_string ("what", "exception throw");
      if (uiout->is_mi_like_p ())
	uiout->field_string ("catch-type", "throw");
      break;

    case EX_EVENT_RETHROW:
      uiout->field_string ("what", "exception rethrow");
      if (uiout->is_mi_like_p ())
	uiout->field_string ("catch-type", "rethrow");
      break;

    case EX_EVENT_CATCH:
      uiout->field_string ("what", "exception catch");
      if (uiout->is_mi_like_p ())
	uiout->field_string ("catch-type", "catch");
      break;
    }

  return true;
}

/* Implement the 'print_one_detail' method.  */

void
exception_catchpoint::print_one_detail (struct ui_out *uiout) const
{
  if (!exception_rx.empty ())
    {
      uiout->text (_("\tmatching: "));
      uiout->field_string ("regexp", exception_rx);
      uiout->text ("\n");
    }
}

void
exception_catchpoint::print_mention () const
{
  struct ui_out *uiout = current_uiout;
  int bp_temp;

  bp_temp = disposition == disp_del;
  uiout->message ("%s %d %s",
		  (bp_temp ? _("Temporary catchpoint ") : _("Catchpoint")),
		  number,
		  (kind == EX_EVENT_THROW
		   ? _("(throw)") : (kind == EX_EVENT_CATCH
				     ? _("(catch)") : _("(rethrow)"))));
}

/* Implement the "print_recreate" method for throw and catch
   catchpoints.  */

void
exception_catchpoint::print_recreate (struct ui_file *fp) const
{
  int bp_temp;

  bp_temp = disposition == disp_del;
  gdb_printf (fp, bp_temp ? "tcatch " : "catch ");
  switch (kind)
    {
    case EX_EVENT_THROW:
      gdb_printf (fp, "throw");
      break;
    case EX_EVENT_CATCH:
      gdb_printf (fp, "catch");
      break;
    case EX_EVENT_RETHROW:
      gdb_printf (fp, "rethrow");
      break;
    }
  print_recreate_thread (fp);
}

/* Implement the "allocate_location" method for throw and catch
   catchpoints.  */

bp_location *
exception_catchpoint::allocate_location ()
{
  return new bp_location (this, bp_loc_software_breakpoint);
}

static void
handle_gnu_v3_exceptions (int tempflag, std::string &&except_rx,
			  const char *cond_string,
			  enum exception_event_kind ex_event, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  std::unique_ptr<exception_catchpoint> cp
    (new exception_catchpoint (gdbarch, tempflag, cond_string,
			       ex_event, std::move (except_rx)));

  install_breakpoint (0, std::move (cp), 1);
}

/* Look for an "if" token in *STRING.  The "if" token must be preceded
   by whitespace.
   
   If there is any non-whitespace text between *STRING and the "if"
   token, then it is returned in a newly-xmalloc'd string.  Otherwise,
   this returns NULL.
   
   STRING is updated to point to the "if" token, if it exists, or to
   the end of the string.  */

static std::string
extract_exception_regexp (const char **string)
{
  const char *start;
  const char *last, *last_space;

  start = skip_spaces (*string);

  last = start;
  last_space = start;
  while (*last != '\0')
    {
      const char *if_token = last;

      /* Check for the "if".  */
      if (check_for_argument (&if_token, "if", 2))
	break;

      /* No "if" token here.  Skip to the next word start.  */
      last_space = skip_to_space (last);
      last = skip_spaces (last_space);
    }

  *string = last;
  if (last_space > start)
    return std::string (start, last_space - start);
  return std::string ();
}

/* See breakpoint.h.  */

void
catch_exception_event (enum exception_event_kind ex_event,
		       const char *arg, bool tempflag, int from_tty)
{
  const char *cond_string = NULL;

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  std::string except_rx = extract_exception_regexp (&arg);

  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  if (ex_event != EX_EVENT_THROW
      && ex_event != EX_EVENT_CATCH
      && ex_event != EX_EVENT_RETHROW)
    error (_("Unsupported or unknown exception event; cannot catch it"));

  handle_gnu_v3_exceptions (tempflag, std::move (except_rx), cond_string,
			    ex_event, from_tty);
}

/* Implementation of "catch catch" command.  */

static void
catch_catch_command (const char *arg, int from_tty,
		     struct cmd_list_element *command)
{
  bool tempflag = command->context () == CATCH_TEMPORARY;

  catch_exception_event (EX_EVENT_CATCH, arg, tempflag, from_tty);
}

/* Implementation of "catch throw" command.  */

static void
catch_throw_command (const char *arg, int from_tty,
		     struct cmd_list_element *command)
{
  bool tempflag = command->context () == CATCH_TEMPORARY;

  catch_exception_event (EX_EVENT_THROW, arg, tempflag, from_tty);
}

/* Implementation of "catch rethrow" command.  */

static void
catch_rethrow_command (const char *arg, int from_tty,
		       struct cmd_list_element *command)
{
  bool tempflag = command->context () == CATCH_TEMPORARY;

  catch_exception_event (EX_EVENT_RETHROW, arg, tempflag, from_tty);
}



/* Implement the 'make_value' method for the $_exception
   internalvar.  */

static struct value *
compute_exception (struct gdbarch *argc, struct internalvar *var, void *ignore)
{
  struct value *arg0, *arg1;
  struct type *obj_type;

  fetch_probe_arguments (&arg0, &arg1);

  /* ARG0 is a pointer to the exception object.  ARG1 is a pointer to
     the std::type_info for the exception.  Now we find the type from
     the type_info and cast the result.  */
  obj_type = cplus_type_from_type_info (arg1);
  return value_ind (value_cast (make_pointer_type (obj_type, NULL), arg0));
}

/* Implementation of the '$_exception' variable.  */

static const struct internalvar_funcs exception_funcs =
{
  compute_exception,
  NULL,
};



void _initialize_break_catch_throw ();
void
_initialize_break_catch_throw ()
{
  /* Add catch and tcatch sub-commands.  */
  add_catch_command ("catch", _("\
Catch an exception, when caught."),
		     catch_catch_command,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("throw", _("\
Catch an exception, when thrown."),
		     catch_throw_command,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("rethrow", _("\
Catch an exception, when rethrown."),
		     catch_rethrow_command,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);

  create_internalvar_type_lazy ("_exception", &exception_funcs, NULL);
}

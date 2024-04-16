/* Everything about signal catchpoints, for GDB.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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
#include "infrun.h"
#include "annotate.h"
#include "valprint.h"
#include "cli/cli-utils.h"
#include "completer.h"
#include "cli/cli-style.h"
#include "cli/cli-decode.h"

#include <string>

#define INTERNAL_SIGNAL(x) ((x) == GDB_SIGNAL_TRAP || (x) == GDB_SIGNAL_INT)

/* An instance of this type is used to represent a signal
   catchpoint.  */

struct signal_catchpoint : public catchpoint
{
  signal_catchpoint (struct gdbarch *gdbarch, bool temp,
		     std::vector<gdb_signal> &&sigs,
		     bool catch_all_)
    : catchpoint (gdbarch, temp, nullptr),
      signals_to_be_caught (std::move (sigs)),
      catch_all (catch_all_)
  {
  }

  int insert_location (struct bp_location *) override;
  int remove_location (struct bp_location *,
		       enum remove_bp_reason reason) override;
  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace,
		      CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  bool print_one (const bp_location **) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;
  bool explains_signal (enum gdb_signal) override;

  /* Signal numbers used for the 'catch signal' feature.  If no signal
     has been specified for filtering, it is empty.  Otherwise,
     it holds a list of all signals to be caught.  */

  std::vector<gdb_signal> signals_to_be_caught;

  /* If SIGNALS_TO_BE_CAUGHT is empty, then all "ordinary" signals are
     caught.  If CATCH_ALL is true, then internal signals are caught
     as well.  If SIGNALS_TO_BE_CAUGHT is not empty, then this field
     is ignored.  */

  bool catch_all;
};

/* Count of each signal.  */

static unsigned int signal_catch_counts[GDB_SIGNAL_LAST];



/* A convenience wrapper for gdb_signal_to_name that returns the
   integer value if the name is not known.  */

static const char *
signal_to_name_or_int (enum gdb_signal sig)
{
  const char *result = gdb_signal_to_name (sig);

  if (strcmp (result, "?") == 0)
    result = plongest (sig);

  return result;
}



/* Implement the "insert_location" method for signal catchpoints.  */

int
signal_catchpoint::insert_location (struct bp_location *bl)
{
  signal_catchpoint *c
    = gdb::checked_static_cast<signal_catchpoint *> (bl->owner);

  if (!c->signals_to_be_caught.empty ())
    {
      for (gdb_signal iter : c->signals_to_be_caught)
	++signal_catch_counts[iter];
    }
  else
    {
      for (int i = 0; i < GDB_SIGNAL_LAST; ++i)
	{
	  if (c->catch_all || !INTERNAL_SIGNAL (i))
	    ++signal_catch_counts[i];
	}
    }

  signal_catch_update (signal_catch_counts);

  return 0;
}

/* Implement the "remove_location" method for signal catchpoints.  */

int
signal_catchpoint::remove_location (struct bp_location *bl,
				    enum remove_bp_reason reason)
{
  signal_catchpoint *c
    = gdb::checked_static_cast<signal_catchpoint *> (bl->owner);

  if (!c->signals_to_be_caught.empty ())
    {
      for (gdb_signal iter : c->signals_to_be_caught)
	{
	  gdb_assert (signal_catch_counts[iter] > 0);
	  --signal_catch_counts[iter];
	}
    }
  else
    {
      for (int i = 0; i < GDB_SIGNAL_LAST; ++i)
	{
	  if (c->catch_all || !INTERNAL_SIGNAL (i))
	    {
	      gdb_assert (signal_catch_counts[i] > 0);
	      --signal_catch_counts[i];
	    }
	}
    }

  signal_catch_update (signal_catch_counts);

  return 0;
}

/* Implement the "breakpoint_hit" method for signal catchpoints.  */

int
signal_catchpoint::breakpoint_hit (const struct bp_location *bl,
				   const address_space *aspace,
				   CORE_ADDR bp_addr,
				   const target_waitstatus &ws)
{
  const signal_catchpoint *c
    = gdb::checked_static_cast<const signal_catchpoint *> (bl->owner);
  gdb_signal signal_number;

  if (ws.kind () != TARGET_WAITKIND_STOPPED)
    return 0;

  signal_number = ws.sig ();

  /* If we are catching specific signals in this breakpoint, then we
     must guarantee that the called signal is the same signal we are
     catching.  */
  if (!c->signals_to_be_caught.empty ())
    {
      for (gdb_signal iter : c->signals_to_be_caught)
	if (signal_number == iter)
	  return 1;
      /* Not the same.  */
      return 0;
    }
  else
    return c->catch_all || !INTERNAL_SIGNAL (signal_number);
}

/* Implement the "print_it" method for signal catchpoints.  */

enum print_stop_action
signal_catchpoint::print_it (const bpstat *bs) const
{
  struct target_waitstatus last;
  const char *signal_name;
  struct ui_out *uiout = current_uiout;

  get_last_target_status (nullptr, nullptr, &last);

  signal_name = signal_to_name_or_int (last.sig ());

  annotate_catchpoint (number);
  maybe_print_thread_hit_breakpoint (uiout);

  gdb_printf (_("Catchpoint %d (signal %s), "), number, signal_name);

  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" method for signal catchpoints.  */

bool
signal_catchpoint::print_one (const bp_location **last_loc) const
{
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns
     not line up too nicely with the headers, but the effect
     is relatively readable).  */
  if (opts.addressprint)
    uiout->field_skip ("addr");
  annotate_field (5);

  if (signals_to_be_caught.size () > 1)
    uiout->text ("signals \"");
  else
    uiout->text ("signal \"");

  if (!signals_to_be_caught.empty ())
    {
      std::string text;

      bool first = true;
      for (gdb_signal iter : signals_to_be_caught)
	{
	  const char *name = signal_to_name_or_int (iter);

	  if (!first)
	    text += " ";
	  first = false;

	  text += name;
	}
      uiout->field_string ("what", text);
    }
  else
    uiout->field_string ("what",
			 catch_all ? "<any signal>" : "<standard signals>",
			 metadata_style.style ());
  uiout->text ("\" ");

  if (uiout->is_mi_like_p ())
    uiout->field_string ("catch-type", "signal");

  return true;
}

/* Implement the "print_mention" method for signal catchpoints.  */

void
signal_catchpoint::print_mention () const
{
  if (!signals_to_be_caught.empty ())
    {
      if (signals_to_be_caught.size () > 1)
	gdb_printf (_("Catchpoint %d (signals"), number);
      else
	gdb_printf (_("Catchpoint %d (signal"), number);

      for (gdb_signal iter : signals_to_be_caught)
	{
	  const char *name = signal_to_name_or_int (iter);

	  gdb_printf (" %s", name);
	}
      gdb_printf (")");
    }
  else if (catch_all)
    gdb_printf (_("Catchpoint %d (any signal)"), number);
  else
    gdb_printf (_("Catchpoint %d (standard signals)"), number);
}

/* Implement the "print_recreate" method for signal catchpoints.  */

void
signal_catchpoint::print_recreate (struct ui_file *fp) const
{
  gdb_printf (fp, "catch signal");

  if (!signals_to_be_caught.empty ())
    {
      for (gdb_signal iter : signals_to_be_caught)
	gdb_printf (fp, " %s", signal_to_name_or_int (iter));
    }
  else if (catch_all)
    gdb_printf (fp, " all");
  gdb_putc ('\n', fp);
}

/* Implement the "explains_signal" method for signal catchpoints.  */

bool
signal_catchpoint::explains_signal (enum gdb_signal sig)
{
  return true;
}

/* Create a new signal catchpoint.  TEMPFLAG is true if this should be
   a temporary catchpoint.  FILTER is the list of signals to catch; it
   can be empty, meaning all signals.  CATCH_ALL is a flag indicating
   whether signals used internally by gdb should be caught; it is only
   valid if FILTER is NULL.  If FILTER is empty and CATCH_ALL is zero,
   then internal signals like SIGTRAP are not caught.  */

static void
create_signal_catchpoint (int tempflag, std::vector<gdb_signal> &&filter,
			  bool catch_all)
{
  struct gdbarch *gdbarch = get_current_arch ();

  std::unique_ptr<signal_catchpoint> c
    (new signal_catchpoint (gdbarch, tempflag, std::move (filter), catch_all));

  install_breakpoint (0, std::move (c), 1);
}


/* Splits the argument using space as delimiter.  Returns a filter
   list, which is empty if no filtering is required.  */

static std::vector<gdb_signal>
catch_signal_split_args (const char *arg, bool *catch_all)
{
  std::vector<gdb_signal> result;
  bool first = true;

  while (*arg != '\0')
    {
      int num;
      gdb_signal signal_number;
      char *endptr;

      std::string one_arg = extract_arg (&arg);
      if (one_arg.empty ())
	break;

      /* Check for the special flag "all".  */
      if (one_arg == "all")
	{
	  arg = skip_spaces (arg);
	  if (*arg != '\0' || !first)
	    error (_("'all' cannot be caught with other signals"));
	  *catch_all = true;
	  gdb_assert (result.empty ());
	  return result;
	}

      first = false;

      /* Check if the user provided a signal name or a number.  */
      num = (int) strtol (one_arg.c_str (), &endptr, 0);
      if (*endptr == '\0')
	signal_number = gdb_signal_from_command (num);
      else
	{
	  signal_number = gdb_signal_from_name (one_arg.c_str ());
	  if (signal_number == GDB_SIGNAL_UNKNOWN)
	    error (_("Unknown signal name '%s'."), one_arg.c_str ());
	}

      result.push_back (signal_number);
    }

  result.shrink_to_fit ();
  return result;
}

/* Implement the "catch signal" command.  */

static void
catch_signal_command (const char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  int tempflag;
  bool catch_all = false;
  std::vector<gdb_signal> filter;

  tempflag = command->context () == CATCH_TEMPORARY;

  arg = skip_spaces (arg);

  /* The allowed syntax is:
     catch signal
     catch signal <name | number> [<name | number> ... <name | number>]

     Let's check if there's a signal name.  */

  if (arg != NULL)
    filter = catch_signal_split_args (arg, &catch_all);

  create_signal_catchpoint (tempflag, std::move (filter), catch_all);
}

void _initialize_break_catch_sig ();
void
_initialize_break_catch_sig ()
{
  add_catch_command ("signal", _("\
Catch signals by their names and/or numbers.\n\
Usage: catch signal [[NAME|NUMBER] [NAME|NUMBER]...|all]\n\
Arguments say which signals to catch.  If no arguments\n\
are given, every \"normal\" signal will be caught.\n\
The argument \"all\" means to also catch signals used by GDB.\n\
Arguments, if given, should be one or more signal names\n\
(if your system supports that), or signal numbers."),
		     catch_signal_command,
		     signal_completer,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
}

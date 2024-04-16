/* Everything about load/unload catchpoints, for GDB.

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

#include "annotate.h"
#include "arch-utils.h"
#include "breakpoint.h"
#include "cli/cli-decode.h"
#include "mi/mi-common.h"
#include "progspace.h"
#include "solist.h"
#include "target.h"
#include "valprint.h"

/* An instance of this type is used to represent an solib catchpoint.
   A breakpoint is really of this type iff its ops pointer points to
   CATCH_SOLIB_BREAKPOINT_OPS.  */

struct solib_catchpoint : public catchpoint
{
  solib_catchpoint (struct gdbarch *gdbarch, bool temp,
		    const char *cond_string,
		    bool is_load_, const char *arg)
    : catchpoint (gdbarch, temp, cond_string),
      is_load (is_load_),
      regex (arg == nullptr ? nullptr : make_unique_xstrdup (arg)),
      compiled (arg == nullptr
		? nullptr
		: new compiled_regex (arg, REG_NOSUB, _("Invalid regexp")))
  {
  }

  int insert_location (struct bp_location *) override;
  int remove_location (struct bp_location *,
		       enum remove_bp_reason reason) override;
  int breakpoint_hit (const struct bp_location *bl,
		      const address_space *aspace,
		      CORE_ADDR bp_addr,
		      const target_waitstatus &ws) override;
  void check_status (struct bpstat *bs) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  bool print_one (const bp_location **) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;

  /* True for "catch load", false for "catch unload".  */
  bool is_load;

  /* Regular expression to match, if any.  COMPILED is only valid when
     REGEX is non-NULL.  */
  gdb::unique_xmalloc_ptr<char> regex;
  std::unique_ptr<compiled_regex> compiled;
};

int
solib_catchpoint::insert_location (struct bp_location *ignore)
{
  return 0;
}

int
solib_catchpoint::remove_location (struct bp_location *ignore,
				   enum remove_bp_reason reason)
{
  return 0;
}

int
solib_catchpoint::breakpoint_hit (const struct bp_location *bl,
				  const address_space *aspace,
				  CORE_ADDR bp_addr,
				  const target_waitstatus &ws)
{
  if (ws.kind () == TARGET_WAITKIND_LOADED)
    return 1;

  for (breakpoint &other : all_breakpoints ())
    {
      if (&other == bl->owner)
	continue;

      if (other.type != bp_shlib_event)
	continue;

      if (pspace != NULL && other.pspace != pspace)
	continue;

      for (bp_location &other_bl : other.locations ())
	{
	  if (other.breakpoint_hit (&other_bl, aspace, bp_addr, ws))
	    return 1;
	}
    }

  return 0;
}

void
solib_catchpoint::check_status (struct bpstat *bs)
{
  if (is_load)
    {
      for (shobj *iter : current_program_space->added_solibs)
	{
	  if (!regex
	      || compiled->exec (iter->so_name.c_str (), 0, nullptr, 0) == 0)
	    return;
	}
    }
  else
    {
      for (const std::string &iter : current_program_space->deleted_solibs)
	{
	  if (!regex
	      || compiled->exec (iter.c_str (), 0, NULL, 0) == 0)
	    return;
	}
    }

  bs->stop = false;
  bs->print_it = print_it_noop;
}

enum print_stop_action
solib_catchpoint::print_it (const bpstat *bs) const
{
  struct ui_out *uiout = current_uiout;

  annotate_catchpoint (this->number);
  maybe_print_thread_hit_breakpoint (uiout);
  if (this->disposition == disp_del)
    uiout->text ("Temporary catchpoint ");
  else
    uiout->text ("Catchpoint ");
  uiout->field_signed ("bkptno", this->number);
  uiout->text ("\n");
  if (uiout->is_mi_like_p ())
    uiout->field_string ("disp", bpdisp_text (this->disposition));
  print_solib_event (true);
  return PRINT_SRC_AND_LOC;
}

bool
solib_catchpoint::print_one (const bp_location **locs) const
{
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);
  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    {
      annotate_field (4);
      uiout->field_skip ("addr");
    }

  std::string msg;
  annotate_field (5);
  if (is_load)
    {
      if (regex)
	msg = string_printf (_("load of library matching %s"),
			     regex.get ());
      else
	msg = _("load of library");
    }
  else
    {
      if (regex)
	msg = string_printf (_("unload of library matching %s"),
			     regex.get ());
      else
	msg = _("unload of library");
    }
  uiout->field_string ("what", msg);

  if (uiout->is_mi_like_p ())
    uiout->field_string ("catch-type", is_load ? "load" : "unload");

  return true;
}

void
solib_catchpoint::print_mention () const
{
  gdb_printf (_("Catchpoint %d (%s)"), number,
	      is_load ? "load" : "unload");
}

void
solib_catchpoint::print_recreate (struct ui_file *fp) const
{
  gdb_printf (fp, "%s %s",
	      disposition == disp_del ? "tcatch" : "catch",
	      is_load ? "load" : "unload");
  if (regex)
    gdb_printf (fp, " %s", regex.get ());
  gdb_printf (fp, "\n");
}

/* See breakpoint.h.  */

void
add_solib_catchpoint (const char *arg, bool is_load, bool is_temp, bool enabled)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);
  if (*arg == '\0')
    arg = nullptr;

  auto c = std::make_unique<solib_catchpoint> (gdbarch, is_temp, nullptr,
					       is_load, arg);

  c->enable_state = enabled ? bp_enabled : bp_disabled;

  install_breakpoint (0, std::move (c), 1);
}

/* A helper function that does all the work for "catch load" and
   "catch unload".  */

static void
catch_load_or_unload (const char *arg, int from_tty, int is_load,
		      struct cmd_list_element *command)
{
  const int enabled = 1;
  bool temp = command->context () == CATCH_TEMPORARY;

  add_solib_catchpoint (arg, is_load, temp, enabled);
}

static void
catch_load_command_1 (const char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  catch_load_or_unload (arg, from_tty, 1, command);
}

static void
catch_unload_command_1 (const char *arg, int from_tty,
			struct cmd_list_element *command)
{
  catch_load_or_unload (arg, from_tty, 0, command);
}

void _initialize_break_catch_load ();
void
_initialize_break_catch_load ()
{
  add_catch_command ("load", _("Catch loads of shared libraries.\n\
Usage: catch load [REGEX]\n\
If REGEX is given, only stop for libraries matching the regular expression."),
		     catch_load_command_1,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("unload", _("Catch unloads of shared libraries.\n\
Usage: catch unload [REGEX]\n\
If REGEX is given, only stop for libraries matching the regular expression."),
		     catch_unload_command_1,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
}

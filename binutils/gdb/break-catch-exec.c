/* Everything about exec catchpoints, for GDB.

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
#include "inferior.h"
#include "mi/mi-common.h"
#include "target.h"
#include "valprint.h"

/* Exec catchpoints.  */

/* An instance of this type is used to represent an exec catchpoint.
   A breakpoint is really of this type iff its ops pointer points to
   CATCH_EXEC_BREAKPOINT_OPS.  */

struct exec_catchpoint : public catchpoint
{
  exec_catchpoint (struct gdbarch *gdbarch, bool temp, const char *cond_string)
    : catchpoint (gdbarch, temp, cond_string)
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

  /* Filename of a program whose exec triggered this catchpoint.
     This field is only valid immediately after this catchpoint has
     triggered.  */
  gdb::unique_xmalloc_ptr<char> exec_pathname;
};

int
exec_catchpoint::insert_location (struct bp_location *bl)
{
  return target_insert_exec_catchpoint (inferior_ptid.pid ());
}

int
exec_catchpoint::remove_location (struct bp_location *bl,
				  enum remove_bp_reason reason)
{
  return target_remove_exec_catchpoint (inferior_ptid.pid ());
}

int
exec_catchpoint::breakpoint_hit (const struct bp_location *bl,
				 const address_space *aspace,
				 CORE_ADDR bp_addr,
				 const target_waitstatus &ws)
{
  if (ws.kind () != TARGET_WAITKIND_EXECD)
    return 0;

  exec_pathname = make_unique_xstrdup (ws.execd_pathname ());
  return 1;
}

enum print_stop_action
exec_catchpoint::print_it (const bpstat *bs) const
{
  struct ui_out *uiout = current_uiout;

  annotate_catchpoint (number);
  maybe_print_thread_hit_breakpoint (uiout);
  if (disposition == disp_del)
    uiout->text ("Temporary catchpoint ");
  else
    uiout->text ("Catchpoint ");
  if (uiout->is_mi_like_p ())
    {
      uiout->field_string ("reason", async_reason_lookup (EXEC_ASYNC_EXEC));
      uiout->field_string ("disp", bpdisp_text (disposition));
    }
  uiout->field_signed ("bkptno", number);
  uiout->text (" (exec'd ");
  uiout->field_string ("new-exec", exec_pathname.get ());
  uiout->text ("), ");

  return PRINT_SRC_AND_LOC;
}

bool
exec_catchpoint::print_one (const bp_location **last_loc) const
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
  uiout->text ("exec");
  if (exec_pathname != NULL)
    {
      uiout->text (", program \"");
      uiout->field_string ("what", exec_pathname.get ());
      uiout->text ("\" ");
    }

  if (uiout->is_mi_like_p ())
    uiout->field_string ("catch-type", "exec");

  return true;
}

void
exec_catchpoint::print_mention () const
{
  gdb_printf (_("Catchpoint %d (exec)"), number);
}

/* Implement the "print_recreate" method for exec catchpoints.  */

void
exec_catchpoint::print_recreate (struct ui_file *fp) const
{
  gdb_printf (fp, "catch exec");
  print_recreate_thread (fp);
}

/* This function attempts to parse an optional "if <cond>" clause
   from the arg string.  If one is not found, it returns NULL.

   Else, it returns a pointer to the condition string.  (It does not
   attempt to evaluate the string against a particular block.)  And,
   it updates arg to point to the first character following the parsed
   if clause in the arg string.  */

const char *
ep_parse_optional_if_clause (const char **arg)
{
  const char *cond_string;

  if (((*arg)[0] != 'i') || ((*arg)[1] != 'f') || !isspace ((*arg)[2]))
    return NULL;

  /* Skip the "if" keyword.  */
  (*arg) += 2;

  /* Skip any extra leading whitespace, and record the start of the
     condition string.  */
  *arg = skip_spaces (*arg);
  cond_string = *arg;

  /* Assume that the condition occupies the remainder of the arg
     string.  */
  (*arg) += strlen (cond_string);

  return cond_string;
}

/* Commands to deal with catching events, such as signals, exceptions,
   process start/exit, etc.  */

static void
catch_exec_command_1 (const char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const char *cond_string = NULL;
  bool temp = command->context () == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  /* The allowed syntax is:
     catch exec
     catch exec if <cond>

     First, check if there's an if clause.  */
  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  std::unique_ptr<exec_catchpoint> c
    (new exec_catchpoint (gdbarch, temp, cond_string));

  install_breakpoint (0, std::move (c), 1);
}

void _initialize_break_catch_exec ();
void
_initialize_break_catch_exec ()
{
  add_catch_command ("exec", _("Catch calls to exec."),
		     catch_exec_command_1,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
}

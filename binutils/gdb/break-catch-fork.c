/* Everything about vfork catchpoints, for GDB.

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

/* An instance of this type is used to represent a fork or vfork
   catchpoint.  A breakpoint is really of this type iff its ops pointer points
   to CATCH_FORK_BREAKPOINT_OPS.  */

struct fork_catchpoint : public catchpoint
{
  fork_catchpoint (struct gdbarch *gdbarch, bool temp,
		   const char *cond_string, bool is_vfork_)
    : catchpoint (gdbarch, temp, cond_string),
      is_vfork (is_vfork_)
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

  /* True if the breakpoint is for vfork, false for fork.  */
  bool is_vfork;

  /* Process id of a child process whose forking triggered this
     catchpoint.  This field is only valid immediately after this
     catchpoint has triggered.  */
  ptid_t forked_inferior_pid = null_ptid;
};

/* Implement the "insert" method for fork catchpoints.  */

int
fork_catchpoint::insert_location (struct bp_location *bl)
{
  if (is_vfork)
    return target_insert_vfork_catchpoint (inferior_ptid.pid ());
  else
    return target_insert_fork_catchpoint (inferior_ptid.pid ());
}

/* Implement the "remove" method for fork catchpoints.  */

int
fork_catchpoint::remove_location (struct bp_location *bl,
				  enum remove_bp_reason reason)
{
  if (is_vfork)
    return target_remove_vfork_catchpoint (inferior_ptid.pid ());
  else
    return target_remove_fork_catchpoint (inferior_ptid.pid ());
}

/* Implement the "breakpoint_hit" method for fork catchpoints.  */

int
fork_catchpoint::breakpoint_hit (const struct bp_location *bl,
				 const address_space *aspace,
				 CORE_ADDR bp_addr,
				 const target_waitstatus &ws)
{
  if (ws.kind () != (is_vfork
		     ? TARGET_WAITKIND_VFORKED
		     : TARGET_WAITKIND_FORKED))
    return 0;

  forked_inferior_pid = ws.child_ptid ();
  return 1;
}

/* Implement the "print_it" method for fork catchpoints.  */

enum print_stop_action
fork_catchpoint::print_it (const bpstat *bs) const
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
      uiout->field_string ("reason",
			   async_reason_lookup (is_vfork
						? EXEC_ASYNC_VFORK
						: EXEC_ASYNC_FORK));
      uiout->field_string ("disp", bpdisp_text (disposition));
    }
  uiout->field_signed ("bkptno", number);
  if (is_vfork)
    uiout->text (" (vforked process ");
  else
    uiout->text (" (forked process ");
  uiout->field_signed ("newpid", forked_inferior_pid.pid ());
  uiout->text ("), ");
  return PRINT_SRC_AND_LOC;
}

/* Implement the "print_one" method for fork catchpoints.  */

bool
fork_catchpoint::print_one (const bp_location **last_loc) const
{
  struct value_print_options opts;
  struct ui_out *uiout = current_uiout;

  get_user_print_options (&opts);

  /* Field 4, the address, is omitted (which makes the columns not
     line up too nicely with the headers, but the effect is relatively
     readable).  */
  if (opts.addressprint)
    uiout->field_skip ("addr");
  annotate_field (5);
  const char *name = is_vfork ? "vfork" : "fork";
  uiout->text (name);
  if (forked_inferior_pid != null_ptid)
    {
      uiout->text (", process ");
      uiout->field_signed ("what", forked_inferior_pid.pid ());
      uiout->spaces (1);
    }

  if (uiout->is_mi_like_p ())
    uiout->field_string ("catch-type", name);

  return true;
}

/* Implement the "print_mention" method for fork catchpoints.  */

void
fork_catchpoint::print_mention () const
{
  gdb_printf (_("Catchpoint %d (%s)"), number,
	      is_vfork ? "vfork" : "fork");
}

/* Implement the "print_recreate" method for fork catchpoints.  */

void
fork_catchpoint::print_recreate (struct ui_file *fp) const
{
  gdb_printf (fp, "catch %s", is_vfork ? "vfork" : "fork");
  print_recreate_thread (fp);
}

static void
create_fork_vfork_event_catchpoint (struct gdbarch *gdbarch,
				    bool temp, const char *cond_string,
				    bool is_vfork)
{
  std::unique_ptr<fork_catchpoint> c
    (new fork_catchpoint (gdbarch, temp, cond_string, is_vfork));

  install_breakpoint (0, std::move (c), 1);
}

enum catch_fork_kind
{
  catch_fork_temporary, catch_vfork_temporary,
  catch_fork_permanent, catch_vfork_permanent
};

static void
catch_fork_command_1 (const char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const char *cond_string = NULL;
  catch_fork_kind fork_kind;

  fork_kind = (catch_fork_kind) (uintptr_t) command->context ();
  bool temp = (fork_kind == catch_fork_temporary
	       || fork_kind == catch_vfork_temporary);

  if (!arg)
    arg = "";
  arg = skip_spaces (arg);

  /* The allowed syntax is:
     catch [v]fork
     catch [v]fork if <cond>

     First, check if there's an if clause.  */
  cond_string = ep_parse_optional_if_clause (&arg);

  if ((*arg != '\0') && !isspace (*arg))
    error (_("Junk at end of arguments."));

  /* If this target supports it, create a fork or vfork catchpoint
     and enable reporting of such events.  */
  switch (fork_kind)
    {
    case catch_fork_temporary:
    case catch_fork_permanent:
      create_fork_vfork_event_catchpoint (gdbarch, temp, cond_string, false);
      break;
    case catch_vfork_temporary:
    case catch_vfork_permanent:
      create_fork_vfork_event_catchpoint (gdbarch, temp, cond_string, true);
      break;
    default:
      error (_("unsupported or unknown fork kind; cannot catch it"));
      break;
    }
}

void _initialize_break_catch_fork ();
void
_initialize_break_catch_fork ()
{
  add_catch_command ("fork", _("Catch calls to fork."),
		     catch_fork_command_1,
		     NULL,
		     (void *) (uintptr_t) catch_fork_permanent,
		     (void *) (uintptr_t) catch_fork_temporary);
  add_catch_command ("vfork", _("Catch calls to vfork."),
		     catch_fork_command_1,
		     NULL,
		     (void *) (uintptr_t) catch_vfork_permanent,
		     (void *) (uintptr_t) catch_vfork_temporary);
}

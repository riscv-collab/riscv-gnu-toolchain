/* Reverse execution and reverse debugging.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "top.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include "inferior.h"
#include "infrun.h"
#include "regcache.h"

/* User interface:
   reverse-step, reverse-next etc.  */

/* exec_reverse_once -- accepts an arbitrary gdb command (string), 
   and executes it with exec-direction set to 'reverse'.

   Used to implement reverse-next etc. commands.  */

static void
exec_reverse_once (const char *cmd, const char *args, int from_tty)
{
  enum exec_direction_kind dir = execution_direction;

  if (dir == EXEC_REVERSE)
    error (_("Already in reverse mode.  Use '%s' or 'set exec-dir forward'."),
	   cmd);

  if (!target_can_execute_reverse ())
    error (_("Target %s does not support this command."), target_shortname ());

  std::string reverse_command = string_printf ("%s %s", cmd, args ? args : "");
  scoped_restore restore_exec_dir
    = make_scoped_restore (&execution_direction, EXEC_REVERSE);
  execute_command (reverse_command.c_str (), from_tty);
}

static void
reverse_step (const char *args, int from_tty)
{
  exec_reverse_once ("step", args, from_tty);
}

static void
reverse_stepi (const char *args, int from_tty)
{
  exec_reverse_once ("stepi", args, from_tty);
}

static void
reverse_next (const char *args, int from_tty)
{
  exec_reverse_once ("next", args, from_tty);
}

static void
reverse_nexti (const char *args, int from_tty)
{
  exec_reverse_once ("nexti", args, from_tty);
}

static void
reverse_continue (const char *args, int from_tty)
{
  exec_reverse_once ("continue", args, from_tty);
}

static void
reverse_finish (const char *args, int from_tty)
{
  exec_reverse_once ("finish", args, from_tty);
}

/* Data structures for a bookmark list.  */

struct bookmark {
  int number = 0;
  CORE_ADDR pc = 0;
  struct symtab_and_line sal;
  gdb::unique_xmalloc_ptr<gdb_byte> opaque_data;
};

static std::vector<struct bookmark> all_bookmarks;
static int bookmark_count;

/* save_bookmark_command -- implement "bookmark" command.
   Call target method to get a bookmark identifier.
   Insert bookmark identifier into list.

   Identifier will be a malloc string (gdb_byte *).
   Up to us to free it as required.  */

static void
save_bookmark_command (const char *args, int from_tty)
{
  /* Get target's idea of a bookmark.  */
  gdb_byte *bookmark_id = target_get_bookmark (args, from_tty);
  regcache *regcache = get_thread_regcache (inferior_thread ());
  gdbarch *gdbarch = regcache->arch ();

  /* CR should not cause another identical bookmark.  */
  dont_repeat ();

  if (bookmark_id == NULL)
    error (_("target_get_bookmark failed."));

  /* Set up a bookmark struct.  */
  all_bookmarks.emplace_back ();
  bookmark &b = all_bookmarks.back ();
  b.number = ++bookmark_count;
  b.pc = regcache_read_pc (regcache);
  b.sal = find_pc_line (b.pc, 0);
  b.sal.pspace = get_frame_program_space (get_current_frame ());
  b.opaque_data.reset (bookmark_id);

  gdb_printf (_("Saved bookmark %d at %s\n"), b.number,
	      paddress (gdbarch, b.sal.pc));
}

/* Implement "delete bookmark" command.  */

static bool
delete_one_bookmark (int num)
{
  /* Find bookmark with corresponding number.  */
  for (auto iter = all_bookmarks.begin ();
       iter != all_bookmarks.end ();
       ++iter)
    {
      if (iter->number == num)
	{
	  all_bookmarks.erase (iter);
	  return true;
	}
    }
  return false;
}

static void
delete_all_bookmarks ()
{
  all_bookmarks.clear ();
}

static void
delete_bookmark_command (const char *args, int from_tty)
{
  if (all_bookmarks.empty ())
    {
      warning (_("No bookmarks."));
      return;
    }

  if (args == NULL || args[0] == '\0')
    {
      if (from_tty && !query (_("Delete all bookmarks? ")))
	return;
      delete_all_bookmarks ();
      return;
    }

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();
      if (!delete_one_bookmark (num))
	/* Not found.  */
	warning (_("No bookmark #%d."), num);
    }
}

/* Implement "goto-bookmark" command.  */

static void
goto_bookmark_command (const char *args, int from_tty)
{
  unsigned long num;
  const char *p = args;

  if (args == NULL || args[0] == '\0')
    error (_("Command requires an argument."));

  if (startswith (args, "start")
      || startswith (args, "begin")
      || startswith (args, "end"))
    {
      /* Special case.  Give target opportunity to handle.  */
      target_goto_bookmark ((gdb_byte *) args, from_tty);
      return;
    }

  if (args[0] == '\'' || args[0] == '\"')
    {
      /* Special case -- quoted string.  Pass on to target.  */
      if (args[strlen (args) - 1] != args[0])
	error (_("Unbalanced quotes: %s"), args);
      target_goto_bookmark ((gdb_byte *) args, from_tty);
      return;
    }

  /* General case.  Bookmark identified by bookmark number.  */
  num = get_number (&args);

  if (num == 0)
    error (_("goto-bookmark: invalid bookmark number '%s'."), p);

  for (const bookmark &iter : all_bookmarks)
    {
      if (iter.number == num)
	{
	  /* Found.  Send to target method.  */
	  target_goto_bookmark (iter.opaque_data.get (), from_tty);
	  return;
	}
    }
  /* Not found.  */
  error (_("goto-bookmark: no bookmark found for '%s'."), p);
}

static int
bookmark_1 (int bnum)
{
  gdbarch *gdbarch = get_thread_regcache (inferior_thread ())->arch ();
  int matched = 0;

  for (const bookmark &iter : all_bookmarks)
    {
      if (bnum == -1 || bnum == iter.number)
	{
	  gdb_printf ("   %d       %s    '%s'\n",
		      iter.number,
		      paddress (gdbarch, iter.pc),
		      iter.opaque_data.get ());
	  matched++;
	}
    }

  if (bnum > 0 && matched == 0)
    gdb_printf ("No bookmark #%d\n", bnum);

  return matched;
}

/* Implement "info bookmarks" command.  */

static void
info_bookmarks_command (const char *args, int from_tty)
{
  if (all_bookmarks.empty ())
    gdb_printf (_("No bookmarks.\n"));
  else if (args == NULL || *args == '\0')
    bookmark_1 (-1);
  else
    {
      number_or_range_parser parser (args);
      while (!parser.finished ())
	{
	  int bnum = parser.get_number ();
	  bookmark_1 (bnum);
	}
    }
}

void _initialize_reverse ();
void
_initialize_reverse ()
{
  cmd_list_element *reverse_step_cmd
   = add_com ("reverse-step", class_run, reverse_step, _("\
Step program backward until it reaches the beginning of another source line.\n\
Argument N means do this N times (or till program stops for another reason)."));
  add_com_alias ("rs", reverse_step_cmd, class_run, 1);

  cmd_list_element *reverse_next_cmd
    = add_com ("reverse-next", class_run, reverse_next, _("\
Step program backward, proceeding through subroutine calls.\n\
Like the \"reverse-step\" command as long as subroutine calls do not happen;\n\
when they do, the call is treated as one instruction.\n\
Argument N means do this N times (or till program stops for another reason)."));
  add_com_alias ("rn", reverse_next_cmd, class_run, 1);

  cmd_list_element *reverse_stepi_cmd
    = add_com ("reverse-stepi", class_run, reverse_stepi, _("\
Step backward exactly one instruction.\n\
Argument N means do this N times (or till program stops for another reason)."));
  add_com_alias ("rsi", reverse_stepi_cmd, class_run, 0);

  cmd_list_element *reverse_nexti_cmd
    = add_com ("reverse-nexti", class_run, reverse_nexti, _("\
Step backward one instruction, but proceed through called subroutines.\n\
Argument N means do this N times (or till program stops for another reason)."));
  add_com_alias ("rni", reverse_nexti_cmd, class_run, 0);

  cmd_list_element *reverse_continue_cmd
    = add_com ("reverse-continue", class_run, reverse_continue, _("\
Continue program being debugged but run it in reverse.\n\
If proceeding from breakpoint, a number N may be used as an argument,\n\
which means to set the ignore count of that breakpoint to N - 1 (so that\n\
the breakpoint won't break until the Nth time it is reached)."));
  add_com_alias ("rc", reverse_continue_cmd, class_run, 0);

  add_com ("reverse-finish", class_run, reverse_finish, _("\
Execute backward until just before selected stack frame is called."));

  add_com ("bookmark", class_bookmark, save_bookmark_command, _("\
Set a bookmark in the program's execution history.\n\
A bookmark represents a point in the execution history \n\
that can be returned to at a later point in the debug session."));
  add_info ("bookmarks", info_bookmarks_command, _("\
Status of user-settable bookmarks.\n\
Bookmarks are user-settable markers representing a point in the \n\
execution history that can be returned to later in the same debug \n\
session."));
  add_cmd ("bookmark", class_bookmark, delete_bookmark_command, _("\
Delete a bookmark from the bookmark list.\n\
Argument is a bookmark number or numbers,\n\
 or no argument to delete all bookmarks."),
	   &deletelist);
  add_com ("goto-bookmark", class_bookmark, goto_bookmark_command, _("\
Go to an earlier-bookmarked point in the program's execution history.\n\
Argument is the bookmark number of a bookmark saved earlier by using \n\
the 'bookmark' command, or the special arguments:\n\
  start (beginning of recording)\n\
  end   (end of recording)"));
}

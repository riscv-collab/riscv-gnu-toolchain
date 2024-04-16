/* General functions for the WDB TUI.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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
#include "gdbcmd.h"
#include "tui/tui.h"
#include "tui/tui-hooks.h"
#include "tui/tui-command.h"
#include "tui/tui-data.h"
#include "tui/tui-layout.h"
#include "tui/tui-io.h"
#include "tui/tui-regs.h"
#include "tui/tui-status.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-winsource.h"
#include "tui/tui-source.h"
#include "target.h"
#include "frame.h"
#include "breakpoint.h"
#include "inferior.h"
#include "symtab.h"
#include "source.h"
#include "terminal.h"
#include "top.h"
#include "ui.h"

#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>

#include "gdb_curses.h"
#include "interps.h"

/* See tui.h.  */

bool debug_tui = false;

/* Implement 'show debug tui'.  */

static void
show_tui_debug (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("TUI debugging is \"%s\".\n"), value);
}

/* This redefines CTRL if it is not already defined, so it must come
   after terminal state releated include files like <term.h> and
   "gdb_curses.h".  */
#include "readline/readline.h"

/* Tells whether the TUI is active or not.  */
bool tui_active = false;
static bool tui_finish_init = true;

enum tui_key_mode tui_current_key_mode = TUI_COMMAND_MODE;

struct tui_char_command
{
  unsigned char key;
  const char *cmd;
};

/* Key mapping to gdb commands when the TUI is using the single key
   mode.  */
static const struct tui_char_command tui_commands[] = {
  { 'c', "continue" },
  { 'C', "reverse-continue" },
  { 'd', "down" },
  { 'f', "finish" },
  { 'F', "reverse-finish" },
  { 'n', "next" },
  { 'N', "reverse-next" },
  { 'o', "nexti" },
  { 'O', "reverse-nexti" },
  { 'r', "run" },
  { 's', "step" },
  { 'S', "reverse-step" },
  { 'i', "stepi" },
  { 'I', "reverse-stepi" },
  { 'u', "up" },
  { 'v', "info locals" },
  { 'w', "where" },
  { 0, 0 },
};

static Keymap tui_keymap;
static Keymap tui_readline_standard_keymap;

/* TUI readline command.
   Switch the output mode between TUI/standard gdb.  */
static int
tui_rl_switch_mode (int notused1, int notused2)
{

  /* Don't let exceptions escape.  We're in the middle of a readline
     callback that isn't prepared for that.  */
  try
    {
      if (tui_active)
	{
	  tui_disable ();
	  rl_prep_terminal (0);
	}
      else
	{
	  /* If tui_enable throws, we'll re-prep below.  */
	  rl_deprep_terminal ();
	  tui_enable ();
	}
    }
  catch (const gdb_exception_forced_quit &ex)
    {
      /* Ideally, we'd do a 'throw' here, but as noted above, we can't
	 do that, so, instead, we'll set the necessary flags so that
	 a later QUIT check will restart the forced quit.  */
      set_force_quit_flag ();
    }
  catch (const gdb_exception &ex)
    {
      exception_print (gdb_stderr, ex);

      if (!tui_active)
	rl_prep_terminal (0);
    }

  /* Clear the readline in case switching occurred in middle of
     something.  */
  if (rl_end)
    rl_kill_text (0, rl_end);

  /* Since we left the curses mode, the terminal mode is restored to
     some previous state.  That state may not be suitable for readline
     to work correctly (it may be restored in line mode).  We force an
     exit of the current readline so that readline is re-entered and
     it will be able to setup the terminal for its needs.  By
     re-entering in readline, we also redisplay its prompt in the
     non-curses mode.  */
  rl_newline (1, '\n');

  /* Make sure the \n we are returning does not repeat the last
     command.  */
  dont_repeat ();
  return 0;
}

/* TUI readline command.
   Change the TUI layout to show a next layout.
   This function is bound to CTRL-X 2.  It is intended to provide
   a functionality close to the Emacs split-window command.  */
static int
tui_rl_change_windows (int notused1, int notused2)
{
  if (!tui_active)
    tui_rl_switch_mode (0 /* notused */, 0 /* notused */);

  if (tui_active)
    tui_next_layout ();

  return 0;
}

/* TUI readline command.
   Delete the second TUI window to only show one.  */
static int
tui_rl_delete_other_windows (int notused1, int notused2)
{
  if (!tui_active)
    tui_rl_switch_mode (0 /* notused */, 0 /* notused */);

  if (tui_active)
    tui_remove_some_windows ();

  return 0;
}

/* TUI readline command.
   Switch the active window to give the focus to a next window.  */
static int
tui_rl_other_window (int count, int key)
{
  struct tui_win_info *win_info;

  if (!tui_active)
    tui_rl_switch_mode (0 /* notused */, 0 /* notused */);

  win_info = tui_next_win (tui_win_with_focus ());
  if (win_info)
    tui_set_win_focus_to (win_info);
  return 0;
}

/* TUI readline command.
   Execute the gdb command bound to the specified key.  */
static int
tui_rl_command_key (int count, int key)
{
  int i;

  reinitialize_more_filter ();
  for (i = 0; tui_commands[i].cmd; i++)
    {
      if (tui_commands[i].key == key)
	{
	  /* Insert the command in the readline buffer.
	     Avoid calling the gdb command here since it creates
	     a possible recursion on readline if prompt_for_continue
	     is called (See PR 9584).  The command will also appear
	     in the readline history which turns out to be better.  */
	  rl_insert_text (tui_commands[i].cmd);
	  rl_newline (1, '\n');

	  /* Switch to gdb command mode while executing the command.
	     This way the gdb's continue prompt will be displayed.  */
	  tui_set_key_mode (TUI_ONE_COMMAND_MODE);
	  return 0;
	}
    }
  return 0;
}

/* TUI readline command.
   Temporarily leave the TUI SingleKey mode to allow editing
   a gdb command with the normal readline.  Once the command
   is executed, the TUI SingleKey mode is installed back.  */
static int
tui_rl_command_mode (int count, int key)
{
  tui_set_key_mode (TUI_ONE_COMMAND_MODE);
  return rl_insert (count, key);
}

/* TUI readline command.
   Switch between TUI SingleKey mode and gdb readline editing.  */
static int
tui_rl_next_keymap (int notused1, int notused2)
{
  if (!tui_active)
    tui_rl_switch_mode (0 /* notused */, 0 /* notused */);

  if (rl_end)
    {
      rl_end = 0;
      rl_point = 0;
      rl_mark = 0;
    }

  tui_set_key_mode (tui_current_key_mode == TUI_COMMAND_MODE
		    ? TUI_SINGLE_KEY_MODE : TUI_COMMAND_MODE);
  return 0;
}

/* Readline hook to redisplay ourself the gdb prompt.
   In the SingleKey mode, the prompt is not printed so that
   the command window is cleaner.  It will be displayed if
   we temporarily leave the SingleKey mode.  */
static int
tui_rl_startup_hook (void)
{
  if (tui_current_key_mode != TUI_COMMAND_MODE
      && !gdb_in_secondary_prompt_p (current_ui))
    tui_set_key_mode (TUI_SINGLE_KEY_MODE);
  return 0;
}

/* Change the TUI key mode by installing the appropriate readline
   keymap.  */
void
tui_set_key_mode (enum tui_key_mode mode)
{
  tui_current_key_mode = mode;
  rl_set_keymap (mode == TUI_SINGLE_KEY_MODE
		 ? tui_keymap : tui_readline_standard_keymap);
  tui_show_status_content ();
}

/* Initialize readline and configure the keymap for the switching
   key shortcut.  */
void
tui_ensure_readline_initialized ()
{
  static bool initialized;

  if (initialized)
    return;
  initialized = true;

  int i;
  Keymap tui_ctlx_keymap;

  rl_add_defun ("tui-switch-mode", tui_rl_switch_mode, -1);
  rl_add_defun ("next-keymap", tui_rl_next_keymap, -1);
  rl_add_defun ("tui-delete-other-windows", tui_rl_delete_other_windows, -1);
  rl_add_defun ("tui-change-windows", tui_rl_change_windows, -1);
  rl_add_defun ("tui-other-window", tui_rl_other_window, -1);

  tui_keymap = rl_make_bare_keymap ();

  /* The named keymap feature was added in Readline 8.0.  */
#if RL_READLINE_VERSION >= 0x800
  rl_set_keymap_name ("SingleKey", tui_keymap);
#endif

  tui_ctlx_keymap = rl_make_bare_keymap ();
  tui_readline_standard_keymap = rl_get_keymap ();

  for (i = 0; tui_commands[i].cmd; i++)
    rl_bind_key_in_map (tui_commands[i].key, tui_rl_command_key, tui_keymap);

  rl_generic_bind (ISKMAP, "\\C-x", (char*) tui_ctlx_keymap, tui_keymap);

  /* Bind all other keys to tui_rl_command_mode so that we switch
     temporarily from SingleKey mode and can enter a gdb command.  */
  for (i = ' '; i < 0x7f; i++)
    {
      int j;

      for (j = 0; tui_commands[j].cmd; j++)
	if (tui_commands[j].key == i)
	  break;

      if (tui_commands[j].cmd)
	continue;

      rl_bind_key_in_map (i, tui_rl_command_mode, tui_keymap);
    }

  rl_bind_key_in_map ('a', tui_rl_switch_mode, emacs_ctlx_keymap);
  rl_bind_key_in_map ('a', tui_rl_switch_mode, tui_ctlx_keymap);
  rl_bind_key_in_map ('A', tui_rl_switch_mode, emacs_ctlx_keymap);
  rl_bind_key_in_map ('A', tui_rl_switch_mode, tui_ctlx_keymap);
  rl_bind_key_in_map (CTRL ('A'), tui_rl_switch_mode, emacs_ctlx_keymap);
  rl_bind_key_in_map (CTRL ('A'), tui_rl_switch_mode, tui_ctlx_keymap);
  rl_bind_key_in_map ('1', tui_rl_delete_other_windows, emacs_ctlx_keymap);
  rl_bind_key_in_map ('1', tui_rl_delete_other_windows, tui_ctlx_keymap);
  rl_bind_key_in_map ('2', tui_rl_change_windows, emacs_ctlx_keymap);
  rl_bind_key_in_map ('2', tui_rl_change_windows, tui_ctlx_keymap);
  rl_bind_key_in_map ('o', tui_rl_other_window, emacs_ctlx_keymap);
  rl_bind_key_in_map ('o', tui_rl_other_window, tui_ctlx_keymap);
  rl_bind_key_in_map ('q', tui_rl_next_keymap, tui_keymap);
  rl_bind_key_in_map ('s', tui_rl_next_keymap, emacs_ctlx_keymap);
  rl_bind_key_in_map ('s', tui_rl_next_keymap, tui_ctlx_keymap);

  /* Initialize readline after the above.  */
  rl_initialize ();
}

/* Return the TERM variable from the environment, or "<unset>"
   if not set.  */

static const char *
gdb_getenv_term (void)
{
  const char *term;

  term = getenv ("TERM");
  if (term != NULL)
    return term;
  return "<unset>";
}

/* Enter in the tui mode (curses).
   When in normal mode, it installs the tui hooks in gdb, redirects
   the gdb output, configures the readline to work in tui mode.
   When in curses mode, it does nothing.  */
void
tui_enable (void)
{
  TUI_SCOPED_DEBUG_ENTER_EXIT;

  if (tui_active)
    return;

  /* To avoid to initialize curses when gdb starts, there is a deferred
     curses initialization.  This initialization is made only once
     and the first time the curses mode is entered.  */
  if (tui_finish_init)
    {
      WINDOW *w;
      SCREEN *s;
#ifndef __MINGW32__
       const char *cap;
#endif
      const char *interp;

      /* If the top level interpreter is not the console/tui (e.g.,
	 MI), enabling curses will certainly lose.  */
      interp = top_level_interpreter ()->name ();
      if (strcmp (interp, INTERP_TUI) != 0)
	error (_("Cannot enable the TUI when the interpreter is '%s'"), interp);

      /* Don't try to setup curses (and print funny control
	 characters) if we're not outputting to a terminal.  */
      if (!gdb_stderr->isatty ())
	error (_("Cannot enable the TUI when output is not a terminal"));

      s = newterm (NULL, stdout, stdin);
#ifdef __MINGW32__
      /* The MinGW port of ncurses requires $TERM to be unset in order
	 to activate the Windows console driver.  */
      if (s == NULL)
	s = newterm ((char *) "unknown", stdout, stdin);
#endif
      if (s == NULL)
	{
	  error (_("Cannot enable the TUI: error opening terminal [TERM=%s]"),
		 gdb_getenv_term ());
	}
      w = stdscr;
      if (has_colors ())
	{
#ifdef HAVE_USE_DEFAULT_COLORS
	  /* Ncurses extension to help with resetting to the default
	     color.  */
	  use_default_colors ();
#endif
	  start_color ();
	}

      /* Check required terminal capabilities.  The MinGW port of
	 ncurses does have them, but doesn't expose them through "cup".  */
#ifndef __MINGW32__
      cap = tigetstr ((char *) "cup");
      if (cap == NULL || cap == (char *) -1 || *cap == '\0')
	{
	  endwin ();
	  delscreen (s);
	  error (_("Cannot enable the TUI: "
		   "terminal doesn't support cursor addressing [TERM=%s]"),
		 gdb_getenv_term ());
	}
#endif

      /* We must mark the tui sub-system active before trying to setup the
	 current layout as tui windows defined by an extension language
	 rely on this flag being true in order to know that the window
	 they are creating is currently valid.  */
      tui_active = true;

      cbreak ();
      noecho ();
      /* timeout (1); */
      nodelay(w, FALSE);
      nl();
      keypad (w, TRUE);
      tui_set_term_height_to (LINES);
      tui_set_term_width_to (COLS);
      def_prog_mode ();

      tui_show_frame_info (0);
      tui_set_initial_layout ();
      tui_set_win_focus_to (TUI_SRC_WIN);
      keypad (TUI_CMD_WIN->handle.get (), TRUE);
      wrefresh (TUI_CMD_WIN->handle.get ());
      tui_finish_init = false;
    }
  else
    {
      /* Save the current gdb setting of the terminal.
	 Curses will restore this state when endwin() is called.  */
      def_shell_mode ();
      clearok (stdscr, TRUE);

      tui_active = true;
    }

  gdb_assert (tui_active);

  if (tui_update_variables ())
    tui_rehighlight_all ();

  tui_setup_io (1);

  /* Resize windows before anything might display/refresh a
     window.  */
  if (tui_win_resized ())
    {
      tui_set_win_resized_to (false);
      tui_resize_all ();
    }

  /* Install the TUI specific hooks.  This must be done after the call to
     tui_display_main so that we don't detect the symtab changed event it
     can cause.  */
  tui_install_hooks ();
  rl_startup_hook = tui_rl_startup_hook;

  /* Restore TUI keymap.  */
  tui_set_key_mode (tui_current_key_mode);

  /* Refresh the screen.  */
  tui_refresh_all_win ();

  /* Update gdb's knowledge of its terminal.  */
  gdb_save_tty_state ();
  tui_update_gdb_sizes ();
}

/* Leave the tui mode.
   Remove the tui hooks and configure the gdb output and readline
   back to their original state.  The curses mode is left so that
   the terminal setting is restored to the point when we entered.  */
void
tui_disable (void)
{
  TUI_SCOPED_DEBUG_ENTER_EXIT;

  if (!tui_active)
    return;

  /* Restore initial readline keymap.  */
  rl_set_keymap (tui_readline_standard_keymap);

  /* Remove TUI hooks.  */
  tui_remove_hooks ();
  rl_startup_hook = 0;
  rl_already_prompted = 0;

#ifdef NCURSES_MOUSE_VERSION
  mousemask (0, NULL);
#endif

  /* Leave curses and restore previous gdb terminal setting.  */
  endwin ();

  /* gdb terminal has changed, update gdb internal copy of it
     so that terminal management with the inferior works.  */
  tui_setup_io (0);

  /* Update gdb's knowledge of its terminal.  */
  gdb_save_tty_state ();

  tui_active = false;
  tui_update_gdb_sizes ();
}

/* Command wrapper for enabling tui mode.  */

static void
tui_enable_command (const char *args, int from_tty)
{
  tui_enable ();
}

/* Command wrapper for leaving tui mode.  */

static void
tui_disable_command (const char *args, int from_tty)
{
  tui_disable ();
}

void
tui_show_assembly (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  tui_suppress_output suppress;
  tui_add_win_to_layout (DISASSEM_WIN);
  tui_update_source_windows_with_addr (gdbarch, addr);
}

bool
tui_is_window_visible (enum tui_win_type type)
{
  if (!tui_active)
    return false;

  if (tui_win_list[type] == nullptr)
    return false;

  return tui_win_list[type]->is_visible ();
}

bool
tui_get_command_dimension (unsigned int *width, 
			   unsigned int *height)
{
  if (!tui_active || (TUI_CMD_WIN == NULL))
    return false;
  
  *width = TUI_CMD_WIN->width;
  *height = TUI_CMD_WIN->height;
  return true;
}

void _initialize_tui ();
void
_initialize_tui ()
{
  struct cmd_list_element **tuicmd;

  tuicmd = tui_get_cmd_list ();

  add_cmd ("enable", class_tui, tui_enable_command,
	   _("Enable TUI display mode.\n\
Usage: tui enable"),
	   tuicmd);
  add_cmd ("disable", class_tui, tui_disable_command,
	   _("Disable TUI display mode.\n\
Usage: tui disable"),
	   tuicmd);

  /* Debug this tui internals.  */
  add_setshow_boolean_cmd ("tui", class_maintenance, &debug_tui,  _("\
Set tui debugging."), _("\
Show tui debugging."), _("\
When true, tui specific internal debugging is enabled."),
			   NULL,
			   show_tui_debug,
			   &setdebuglist, &showdebuglist);
}

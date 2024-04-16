/* TUI window generic functions.

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

/* This module contains procedures for handling tui window functions
   like resize, scrolling, scrolling, changing focus, etc.

   Author: Susan B. Macchia  */

#include "defs.h"
#include "command.h"
#include "symtab.h"
#include "breakpoint.h"
#include "frame.h"
#include "cli/cli-cmds.h"
#include "cli/cli-style.h"
#include "top.h"
#include "source.h"
#include "gdbsupport/event-loop.h"
#include "gdbcmd.h"
#include "async-event.h"
#include "utils.h"

#include "tui/tui.h"
#include "tui/tui-io.h"
#include "tui/tui-command.h"
#include "tui/tui-data.h"
#include "tui/tui-layout.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-status.h"
#include "tui/tui-regs.h"
#include "tui/tui-disasm.h"
#include "tui/tui-source.h"
#include "tui/tui-winsource.h"
#include "tui/tui-win.h"

#include "gdb_curses.h"
#include <ctype.h>
#include "readline/readline.h"
#include <string_view>

#include <signal.h>

static void tui_set_tab_width_command (const char *, int);
static void tui_refresh_all_command (const char *, int);
static void tui_all_windows_info (const char *, int);
static void tui_scroll_forward_command (const char *, int);
static void tui_scroll_backward_command (const char *, int);
static void tui_scroll_left_command (const char *, int);
static void tui_scroll_right_command (const char *, int);
static void parse_scrolling_args (const char *, 
				  struct tui_win_info **, 
				  int *);


#ifndef ACS_LRCORNER
#  define ACS_LRCORNER '+'
#endif
#ifndef ACS_LLCORNER
#  define ACS_LLCORNER '+'
#endif
#ifndef ACS_ULCORNER
#  define ACS_ULCORNER '+'
#endif
#ifndef ACS_URCORNER
#  define ACS_URCORNER '+'
#endif
#ifndef ACS_HLINE
#  define ACS_HLINE '-'
#endif
#ifndef ACS_VLINE
#  define ACS_VLINE '|'
#endif

/* Possible values for tui-border-kind variable.  */
static const char *const tui_border_kind_enums[] = {
  "space",
  "ascii",
  "acs",
  NULL
};

/* Possible values for tui-border-mode and tui-active-border-mode.  */
static const char *const tui_border_mode_enums[] = {
  "normal",
  "standout",
  "reverse",
  "half",
  "half-standout",
  "bold",
  "bold-standout",
  NULL
};

struct tui_translate
{
  const char *name;
  int value;
};

/* Translation table for border-mode variables.
   The list of values must be terminated by a NULL.  */
static struct tui_translate tui_border_mode_translate[] = {
  { "normal",		A_NORMAL },
  { "standout",		A_STANDOUT },
  { "reverse",		A_REVERSE },
  { "half",		A_DIM },
  { "half-standout",	A_DIM | A_STANDOUT },
  { "bold",		A_BOLD },
  { "bold-standout",	A_BOLD | A_STANDOUT },
  { 0, 0 }
};

/* Translation tables for border-kind (acs excluded), one for vline, hline and
   corners (see wborder, border curses operations).  */
static struct tui_translate tui_border_kind_translate_vline[] = {
  { "space",    ' ' },
  { "ascii",    '|' },
  { 0, 0 }
};

static struct tui_translate tui_border_kind_translate_hline[] = {
  { "space",    ' ' },
  { "ascii",    '-' },
  { 0, 0 }
};

static struct tui_translate tui_border_kind_translate_corner[] = {
  { "space",    ' ' },
  { "ascii",    '+' },
  { 0, 0 }
};


/* Tui configuration variables controlled with set/show command.  */
static const char *tui_active_border_mode = "bold-standout";
static void
show_tui_active_border_mode (struct ui_file *file,
			     int from_tty,
			     struct cmd_list_element *c, 
			     const char *value)
{
  gdb_printf (file, _("\
The attribute mode to use for the active TUI window border is \"%s\".\n"),
	      value);
}

static const char *tui_border_mode = "normal";
static void
show_tui_border_mode (struct ui_file *file, 
		      int from_tty,
		      struct cmd_list_element *c, 
		      const char *value)
{
  gdb_printf (file, _("\
The attribute mode to use for the TUI window borders is \"%s\".\n"),
	      value);
}

static const char *tui_border_kind = "acs";
static void
show_tui_border_kind (struct ui_file *file, 
		      int from_tty,
		      struct cmd_list_element *c, 
		      const char *value)
{
  gdb_printf (file, _("The kind of border for TUI windows is \"%s\".\n"),
	      value);
}

/* Implementation of the "set/show style tui-current-position" commands.  */

bool style_tui_current_position = false;

static void
show_style_tui_current_position (ui_file *file,
				 int from_tty,
				 cmd_list_element *c,
				 const char *value)
{
  gdb_printf (file, _("\
Styling the text highlighted by the TUI's current position indicator is %s.\n"),
		    value);
}

static void
set_style_tui_current_position (const char *ignore, int from_tty,
				cmd_list_element *c)
{
  if (TUI_SRC_WIN != nullptr)
    TUI_SRC_WIN->refill ();
  if (TUI_DISASM_WIN != nullptr)
    TUI_DISASM_WIN->refill ();
}

/* Tui internal configuration variables.  These variables are updated
   by tui_update_variables to reflect the tui configuration
   variables.  */
chtype tui_border_vline;
chtype tui_border_hline;
chtype tui_border_ulcorner;
chtype tui_border_urcorner;
chtype tui_border_llcorner;
chtype tui_border_lrcorner;

int tui_border_attrs;
int tui_active_border_attrs;

/* Identify the item in the translation table, and return the corresponding value.  */
static int
translate (const char *name, struct tui_translate *table)
{
  while (table->name)
    {
      if (name && strcmp (table->name, name) == 0)
	return table->value;
      table++;
    }

  gdb_assert_not_reached ("");
}

/* Translate NAME to a value.  If NAME is "acs", use ACS_CHAR.  Otherwise, use
   translation table TABLE. */
static int
translate_acs (const char *name, struct tui_translate *table, int acs_char)
{
  /* The ACS characters are determined at run time by curses terminal
     management.  */
  if (strcmp (name, "acs") == 0)
    return acs_char;

  return translate (name, table);
}

/* Update the tui internal configuration according to gdb settings.
   Returns 1 if the configuration has changed and the screen should
   be redrawn.  */
bool
tui_update_variables ()
{
  bool need_redraw = false;
  int val;

  val = translate (tui_border_mode, tui_border_mode_translate);
  need_redraw |= assign_return_if_changed<int> (tui_border_attrs, val);

  val = translate (tui_active_border_mode, tui_border_mode_translate);
  need_redraw |= assign_return_if_changed<int> (tui_active_border_attrs, val);

  /* If one corner changes, all characters are changed.  Only check the first
     one.  */
  val = translate_acs (tui_border_kind, tui_border_kind_translate_corner,
		       ACS_LRCORNER);
  need_redraw |= assign_return_if_changed<chtype> (tui_border_lrcorner, val);

  tui_border_llcorner
    = translate_acs (tui_border_kind, tui_border_kind_translate_corner,
		     ACS_LLCORNER);

  tui_border_ulcorner
    = translate_acs (tui_border_kind, tui_border_kind_translate_corner,
		     ACS_ULCORNER);

  tui_border_urcorner =
    translate_acs (tui_border_kind, tui_border_kind_translate_corner,
		   ACS_URCORNER);

  tui_border_hline
    = translate_acs (tui_border_kind, tui_border_kind_translate_hline,
		     ACS_HLINE);

  tui_border_vline
    = translate_acs (tui_border_kind, tui_border_kind_translate_vline,
		     ACS_VLINE);

  return need_redraw;
}

static struct cmd_list_element *tuilist;

struct cmd_list_element **
tui_get_cmd_list (void)
{
  if (tuilist == 0)
    add_basic_prefix_cmd ("tui", class_tui,
			  _("Text User Interface commands."),
			  &tuilist, 0, &cmdlist);
  return &tuilist;
}

/* The set_func hook of "set tui ..." commands that affect the window
   borders on the TUI display.  */

static void
tui_set_var_cmd (const char *null_args,
		 int from_tty, struct cmd_list_element *c)
{
  if (tui_update_variables () && tui_active)
    tui_rehighlight_all ();
}



/* True if TUI resizes should print a message.  This is used by the
   test suite.  */

static bool resize_message;

static void
show_tui_resize_message (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("TUI resize messaging is %s.\n"), value);
}



/* Generic window name completion function.  Complete window name pointed
   to by TEXT and WORD.

   If EXCLUDE_CANNOT_FOCUS_P is true, then windows that can't take focus
   will be excluded from the completions, otherwise they will be included.

   If INCLUDE_NEXT_PREV_P is true then the special window names 'next' and
   'prev' will also be considered as possible completions of the window
   name.  This is independent of EXCLUDE_CANNOT_FOCUS_P.  */

static void
window_name_completer (completion_tracker &tracker,
		       bool include_next_prev_p,
		       bool exclude_cannot_focus_p,
		       const char *text, const char *word)
{
  std::vector<const char *> completion_name_vec;

  for (tui_win_info *win_info : all_tui_windows ())
    {
      const char *completion_name = NULL;

      /* Don't include an invisible window.  */
      if (!win_info->is_visible ())
	continue;

      /* If requested, exclude windows that can't be focused.  */
      if (exclude_cannot_focus_p && !win_info->can_focus ())
	continue;

      completion_name = win_info->name ();
      gdb_assert (completion_name != NULL);
      completion_name_vec.push_back (completion_name);
    }

  /* If no windows are considered visible then the TUI has not yet been
     initialized.  But still "focus src" and "focus cmd" will work because
     invoking the focus command will entail initializing the TUI which sets the
     default layout to "src".  */
  if (completion_name_vec.empty ())
    {
      completion_name_vec.push_back (SRC_NAME);
      completion_name_vec.push_back (CMD_NAME);
    }

  if (include_next_prev_p)
    {
      completion_name_vec.push_back ("next");
      completion_name_vec.push_back ("prev");
    }


  completion_name_vec.push_back (NULL);
  complete_on_enum (tracker, completion_name_vec.data (), text, word);
}

/* Complete possible window names to focus on.  TEXT is the complete text
   entered so far, WORD is the word currently being completed.  */

static void
focus_completer (struct cmd_list_element *ignore,
		 completion_tracker &tracker,
		 const char *text, const char *word)
{
  window_name_completer (tracker, true, true, text, word);
}

/* Complete possible window names for winheight command.  TEXT is the
   complete text entered so far, WORD is the word currently being
   completed.  */

static void
winheight_completer (struct cmd_list_element *ignore,
		     completion_tracker &tracker,
		     const char *text, const char *word)
{
  /* The first word is the window name.  That we can complete.  Subsequent
     words can't be completed.  */
  if (word != text)
    return;

  window_name_completer (tracker, false, false, text, word);
}

/* Update gdb's knowledge of the terminal size.  */
void
tui_update_gdb_sizes (void)
{
  int width, height;

  if (tui_active)
    {
      width = TUI_CMD_WIN->width;
      height = TUI_CMD_WIN->height;
    }
  else
    {
      width = tui_term_width ();
      height = tui_term_height ();
    }

  set_screen_width_and_height (width, height);
}


void
tui_win_info::forward_scroll (int num_to_scroll)
{
  if (num_to_scroll == 0)
    num_to_scroll = height - 3;

  do_scroll_vertical (num_to_scroll);
}

void
tui_win_info::backward_scroll (int num_to_scroll)
{
  if (num_to_scroll == 0)
    num_to_scroll = height - 3;

  do_scroll_vertical (-num_to_scroll);
}


void
tui_win_info::left_scroll (int num_to_scroll)
{
  if (num_to_scroll == 0)
    num_to_scroll = 1;

  do_scroll_horizontal (num_to_scroll);
}


void
tui_win_info::right_scroll (int num_to_scroll)
{
  if (num_to_scroll == 0)
    num_to_scroll = 1;

  do_scroll_horizontal (-num_to_scroll);
}


void
tui_refresh_all_win (void)
{
  clearok (curscr, TRUE);
  tui_refresh_all ();
}

void
tui_rehighlight_all (void)
{
  for (tui_win_info *win_info : all_tui_windows ())
    win_info->check_and_display_highlight_if_needed ();
}

/* Resize all the windows based on the terminal size.  This function
   gets called from within the readline SIGWINCH handler.  */
void
tui_resize_all (void)
{
  int height_diff, width_diff;
  int screenheight, screenwidth;

  rl_get_screen_size (&screenheight, &screenwidth);
  screenwidth += readline_hidden_cols;

  width_diff = screenwidth - tui_term_width ();
  height_diff = screenheight - tui_term_height ();
  if (height_diff || width_diff)
    {
#ifdef HAVE_RESIZE_TERM
      resize_term (screenheight, screenwidth);
#endif      
      /* Turn keypad off while we resize.  */
      keypad (TUI_CMD_WIN->handle.get (), FALSE);
      tui_update_gdb_sizes ();
      tui_set_term_height_to (screenheight);
      tui_set_term_width_to (screenwidth);

      /* erase + clearok are used instead of a straightforward clear as
	 AIX 5.3 does not define clear.  */
      erase ();
      clearok (curscr, TRUE);
      /* Apply the current layout.  The 'false' here allows the command
	 window to resize proportionately with containing terminal, rather
	 than maintaining a fixed size.  */
      tui_apply_current_layout (false); /* Turn keypad back on.  */
      keypad (TUI_CMD_WIN->handle.get (), TRUE);
    }
}

#ifdef SIGWINCH
/* Token for use by TUI's asynchronous SIGWINCH handler.  */
static struct async_signal_handler *tui_sigwinch_token;

/* TUI's SIGWINCH signal handler.  */
static void
tui_sigwinch_handler (int signal)
{
  mark_async_signal_handler (tui_sigwinch_token);
  tui_set_win_resized_to (true);
}

/* Callback for asynchronously resizing TUI following a SIGWINCH signal.  */
static void
tui_async_resize_screen (gdb_client_data arg)
{
  rl_resize_terminal ();

  if (!tui_active)
    {
      int screen_height, screen_width;

      rl_get_screen_size (&screen_height, &screen_width);
      screen_width += readline_hidden_cols;
      set_screen_width_and_height (screen_width, screen_height);

      /* win_resized is left set so that the next call to tui_enable()
	 resizes the TUI windows.  */
    }
  else
    {
      tui_set_win_resized_to (false);
      tui_resize_all ();
      tui_refresh_all_win ();
      tui_update_gdb_sizes ();
      if (resize_message)
	{
	  static int count;
	  printf_unfiltered ("@@ resize done %d, size = %dx%d\n", count,
			     tui_term_width (), tui_term_height ());
	  ++count;
	}
      tui_redisplay_readline ();
    }
}
#endif

/* Initialize TUI's SIGWINCH signal handler.  Note that the handler is not
   uninstalled when we exit TUI, so the handler should not assume that TUI is
   always active.  */
void
tui_initialize_win (void)
{
#ifdef SIGWINCH
  tui_sigwinch_token
    = create_async_signal_handler (tui_async_resize_screen, NULL,
				   "tui-sigwinch");

  {
#ifdef HAVE_SIGACTION
    struct sigaction old_winch;

    memset (&old_winch, 0, sizeof (old_winch));
    old_winch.sa_handler = &tui_sigwinch_handler;
#ifdef SA_RESTART
    old_winch.sa_flags = SA_RESTART;
#endif
    sigaction (SIGWINCH, &old_winch, NULL);
#else
    signal (SIGWINCH, &tui_sigwinch_handler);
#endif
  }
#endif
}


static void
tui_scroll_forward_command (const char *arg, int from_tty)
{
  int num_to_scroll = 1;
  struct tui_win_info *win_to_scroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg == NULL)
    parse_scrolling_args (arg, &win_to_scroll, NULL);
  else
    parse_scrolling_args (arg, &win_to_scroll, &num_to_scroll);
  win_to_scroll->forward_scroll (num_to_scroll);
}


static void
tui_scroll_backward_command (const char *arg, int from_tty)
{
  int num_to_scroll = 1;
  struct tui_win_info *win_to_scroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg == NULL)
    parse_scrolling_args (arg, &win_to_scroll, NULL);
  else
    parse_scrolling_args (arg, &win_to_scroll, &num_to_scroll);
  win_to_scroll->backward_scroll (num_to_scroll);
}


static void
tui_scroll_left_command (const char *arg, int from_tty)
{
  int num_to_scroll;
  struct tui_win_info *win_to_scroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  parse_scrolling_args (arg, &win_to_scroll, &num_to_scroll);
  win_to_scroll->left_scroll (num_to_scroll);
}


static void
tui_scroll_right_command (const char *arg, int from_tty)
{
  int num_to_scroll;
  struct tui_win_info *win_to_scroll;

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  parse_scrolling_args (arg, &win_to_scroll, &num_to_scroll);
  win_to_scroll->right_scroll (num_to_scroll);
}


/* Answer the window represented by name.  */
static struct tui_win_info *
tui_partial_win_by_name (std::string_view name)
{
  struct tui_win_info *best = nullptr;

  for (tui_win_info *item : all_tui_windows ())
    {
      const char *cur_name = item->name ();

      if (name == cur_name)
	return item;
      if (startswith (cur_name, name))
	{
	  if (best != nullptr)
	    error (_("Window name \"%*s\" is ambiguous"),
		   (int) name.size (), name.data ());
	  best = item;
	}
    }

  return best;
}

/* Set focus to the window named by 'arg'.  */
static void
tui_set_focus_command (const char *arg, int from_tty)
{
  tui_enable ();

  if (arg == NULL)
    error_no_arg (_("name of window to focus"));

  struct tui_win_info *win_info = NULL;

  if (startswith ("next", arg))
    win_info = tui_next_win (tui_win_with_focus ());
  else if (startswith ("prev", arg))
    win_info = tui_prev_win (tui_win_with_focus ());
  else
    win_info = tui_partial_win_by_name (arg);

  if (win_info == nullptr)
    {
      /* When WIN_INFO is nullptr this can either mean that the window name
	 is unknown to GDB, or that the window is not in the current
	 layout.  To try and help the user, give a different error
	 depending on which of these is the case.  */
      std::string matching_window_name;
      bool is_ambiguous = false;

      for (const std::string &name : all_known_window_names ())
	{
	  /* Look through all windows in the current layout, if the window
	     is in the current layout then we're not interested is it.  */
	  for (tui_win_info *item : all_tui_windows ())
	    if (item->name () == name)
	      continue;

	  if (startswith (name, arg))
	    {
	      if (matching_window_name.empty ())
		matching_window_name = name;
	      else
		is_ambiguous = true;
	    }
	};

      if (!matching_window_name.empty ())
	{
	  if (is_ambiguous)
	    error (_("No windows matching \"%s\" in the current layout"),
		   arg);
	  else
	    error (_("Window \"%s\" is not in the current layout"),
		   matching_window_name.c_str ());
	}
      else
	error (_("Unrecognized window name \"%s\""), arg);
    }

  /* If a window is part of the current layout then it will have a
     tui_win_info associated with it and be visible, otherwise, there will
     be no tui_win_info and the above error will have been raised.  */
  gdb_assert (win_info->is_visible ());

  if (!win_info->can_focus ())
    error (_("Window \"%s\" cannot be focused"), arg);

  tui_set_win_focus_to (win_info);
  gdb_printf (_("Focus set to %s window.\n"),
	      tui_win_with_focus ()->name ());
}

static void
tui_all_windows_info (const char *arg, int from_tty)
{
  if (!tui_active)
    {
      gdb_printf (_("The TUI is not active.\n"));
      return;
    }

  struct tui_win_info *win_with_focus = tui_win_with_focus ();
  struct ui_out *uiout = current_uiout;

  ui_out_emit_table table_emitter (uiout, 4, -1, "tui-windows");
  uiout->table_header (10, ui_left, "name", "Name");
  uiout->table_header (5, ui_right, "lines", "Lines");
  uiout->table_header (7, ui_right, "columns", "Columns");
  uiout->table_header (10, ui_left, "focus", "Focus");
  uiout->table_body ();

  for (tui_win_info *win_info : all_tui_windows ())
    if (win_info->is_visible ())
      {
	ui_out_emit_tuple tuple_emitter (uiout, nullptr);

	uiout->field_string ("name", win_info->name ());
	uiout->field_signed ("lines", win_info->height);
	uiout->field_signed ("columns", win_info->width);
	if (win_with_focus == win_info)
	  uiout->field_string ("focus", _("(has focus)"));
	else
	  uiout->field_skip ("focus");
	uiout->text ("\n");
      }
}


static void
tui_refresh_all_command (const char *arg, int from_tty)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();

  tui_refresh_all_win ();
}

#define DEFAULT_TAB_LEN         8

/* The tab width that should be used by the TUI.  */

unsigned int tui_tab_width = DEFAULT_TAB_LEN;

/* The tab width as set by the user.  */

static unsigned int internal_tab_width = DEFAULT_TAB_LEN;

/* After the tab width is set, call this to update the relevant
   windows.  */

static void
update_tab_width ()
{
  for (tui_win_info *win_info : all_tui_windows ())
    {
      if (win_info->is_visible ())
	win_info->update_tab_width ();
    }
}

/* Callback for "set tui tab-width".  */

static void
tui_set_tab_width (const char *ignore,
		   int from_tty, struct cmd_list_element *c)
{
  if (internal_tab_width == 0)
    {
      internal_tab_width = tui_tab_width;
      error (_("Tab width must not be 0"));
    }

  tui_tab_width = internal_tab_width;
  update_tab_width ();
}

/* Callback for "show tui tab-width".  */

static void
tui_show_tab_width (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("TUI tab width is %s spaces.\n"), value);

}

/* See tui-win.h.  */

bool compact_source = false;

/* Callback for "set tui compact-source".  */

static void
tui_set_compact_source (const char *ignore, int from_tty,
			struct cmd_list_element *c)
{
  if (TUI_SRC_WIN != nullptr)
    TUI_SRC_WIN->refill ();
}

/* Callback for "show tui compact-source".  */

static void
tui_show_compact_source (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("TUI source window compactness is %s.\n"), value);
}

bool tui_enable_mouse = true;

/* Implement 'show tui mouse-events'.  */

static void
show_tui_mouse_events (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("TUI mouse events are %s.\n"), value);
}

/* Set the tab width of the specified window.  */
static void
tui_set_tab_width_command (const char *arg, int from_tty)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg != NULL)
    {
      int ts;

      ts = atoi (arg);
      if (ts <= 0)
	warning (_("Tab widths greater than 0 must be specified."));
      else
	{
	  internal_tab_width = ts;
	  tui_tab_width = ts;

	  update_tab_width ();
	}
    }
}

/* Helper function for the user commands to adjust a window's width or
   height.  The ARG string contains the command line arguments from the
   user, which should give the name of a window, and how to adjust the
   size.

   When SET_WIDTH_P is true the width of the window is adjusted based on
   ARG, and when SET_WIDTH_P is false, the height of the window is adjusted
   based on ARG.

   On invalid input, or if the size can't be adjusted as requested, then an
   error is thrown, otherwise, the window sizes are adjusted, and the
   windows redrawn.  */

static void
tui_set_win_size (const char *arg, bool set_width_p)
{
  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  if (arg == NULL)
    error_no_arg (_("name of window"));

  const char *buf = arg;
  const char *buf_ptr = buf;
  int new_size;
  struct tui_win_info *win_info;

  buf_ptr = skip_to_space (buf_ptr);

  /* Validate the window name.  */
  std::string_view wname (buf, buf_ptr - buf);
  win_info = tui_partial_win_by_name (wname);

  if (win_info == NULL)
    error (_("Unrecognized window name \"%s\""), arg);
  if (!win_info->is_visible ())
    error (_("Window \"%s\" is not visible"), arg);

  /* Process the size.  */
  buf_ptr = skip_spaces (buf_ptr);

  if (*buf_ptr != '\0')
    {
      bool negate = false;
      bool fixed_size = true;
      int input_no;;

      if (*buf_ptr == '+' || *buf_ptr == '-')
	{
	  if (*buf_ptr == '-')
	    negate = true;
	  fixed_size = false;
	  buf_ptr++;
	}
      input_no = atoi (buf_ptr);
      if (input_no > 0)
	{
	  if (negate)
	    input_no *= (-1);
	  if (fixed_size)
	    new_size = input_no;
	  else
	    {
	      int curr_size;
	      if (set_width_p)
		curr_size = win_info->width;
	      else
		curr_size = win_info->height;
	      new_size = curr_size + input_no;
	    }

	  /* Now change the window's height, and adjust
	     all other windows around it.  */
	  if (set_width_p)
	    tui_adjust_window_width (win_info, new_size);
	  else
	    tui_adjust_window_height (win_info, new_size);
	  tui_update_gdb_sizes ();
	}
      else
	{
	  if (set_width_p)
	    error (_("Invalid window width specified"));
	  else
	    error (_("Invalid window height specified"));
	}
    }
}

/* Implement the 'tui window height' command (alias 'winheight').  */

static void
tui_set_win_height_command (const char *arg, int from_tty)
{
  /* Pass false as the final argument to set the height.  */
  tui_set_win_size (arg, false);
}

/* Implement the 'tui window width' command (alias 'winwidth').  */

static void
tui_set_win_width_command (const char *arg, int from_tty)
{
  /* Pass true as the final argument to set the width.  */
  tui_set_win_size (arg, true);
}

/* See tui-data.h.  */

int
tui_win_info::max_height () const
{
  return tui_term_height ();
}

/* See tui-data.h.  */

int
tui_win_info::max_width () const
{
  return tui_term_width ();
}

static void
parse_scrolling_args (const char *arg, 
		      struct tui_win_info **win_to_scroll,
		      int *num_to_scroll)
{
  if (num_to_scroll)
    *num_to_scroll = 0;
  *win_to_scroll = tui_win_with_focus ();

  /* First set up the default window to scroll, in case there is no
     window name arg.  */
  if (arg != NULL)
    {
      char *buf_ptr;

      /* Process the number of lines to scroll.  */
      std::string copy = arg;
      buf_ptr = &copy[0];
      if (isdigit (*buf_ptr))
	{
	  char *num_str;

	  num_str = buf_ptr;
	  buf_ptr = strchr (buf_ptr, ' ');
	  if (buf_ptr != NULL)
	    {
	      *buf_ptr = '\0';
	      if (num_to_scroll)
		*num_to_scroll = atoi (num_str);
	      buf_ptr++;
	    }
	  else if (num_to_scroll)
	    *num_to_scroll = atoi (num_str);
	}

      /* Process the window name if one is specified.  */
      if (buf_ptr != NULL)
	{
	  const char *wname;

	  wname = skip_spaces (buf_ptr);

	  if (*wname != '\0')
	    {
	      *win_to_scroll = tui_partial_win_by_name (wname);

	      if (*win_to_scroll == NULL)
		error (_("Unrecognized window `%s'"), wname);
	      if (!(*win_to_scroll)->is_visible ())
		error (_("Window is not visible"));
	      else if (*win_to_scroll == TUI_CMD_WIN)
		*win_to_scroll = *(tui_source_windows ().begin ());
	    }
	}
    }
}

/* The list of 'tui window' sub-commands.  */

static cmd_list_element *tui_window_cmds = nullptr;

/* Called to implement 'tui window'.  */

static void
tui_window_command (const char *args, int from_tty)
{
  help_list (tui_window_cmds, "tui window ", all_commands, gdb_stdout);
}

/* See tui-win.h.  */

bool tui_left_margin_verbose = false;

/* Function to initialize gdb commands, for tui window
   manipulation.  */

void _initialize_tui_win ();
void
_initialize_tui_win ()
{
  static struct cmd_list_element *tui_setlist;
  static struct cmd_list_element *tui_showlist;

  /* Define the classes of commands.
     They will appear in the help list in the reverse of this order.  */
  add_setshow_prefix_cmd ("tui", class_tui,
			  _("TUI configuration variables."),
			  _("TUI configuration variables."),
			  &tui_setlist, &tui_showlist,
			  &setlist, &showlist);

  cmd_list_element *refresh_cmd
    = add_cmd ("refresh", class_tui, tui_refresh_all_command,
	       _("Refresh the terminal display."),
	       tui_get_cmd_list ());
  add_com_alias ("refresh", refresh_cmd, class_tui, 0);

  cmd_list_element *tabset_cmd
    = add_com ("tabset", class_tui, tui_set_tab_width_command, _("\
Set the width (in characters) of tab stops.\n\
Usage: tabset N"));
  deprecate_cmd (tabset_cmd, "set tui tab-width");

  /* Setup the 'tui window' list of command.  */
  add_prefix_cmd ("window", class_tui, tui_window_command,
		  _("Text User Interface window commands."),
		  &tui_window_cmds, 1, tui_get_cmd_list ());

  cmd_list_element *winheight_cmd
    = add_cmd ("height", class_tui, tui_set_win_height_command, _("\
Set or modify the height of a specified window.\n\
Usage: tui window height WINDOW-NAME [+ | -] NUM-LINES\n\
Use \"info win\" to see the names of the windows currently being displayed."),
	       &tui_window_cmds);
  add_com_alias ("winheight", winheight_cmd, class_tui, 0);
  add_com_alias ("wh", winheight_cmd, class_tui, 0);
  set_cmd_completer (winheight_cmd, winheight_completer);

  cmd_list_element *winwidth_cmd
    = add_cmd ("width", class_tui, tui_set_win_width_command, _("\
Set or modify the width of a specified window.\n\
Usage: tui window width WINDOW-NAME [+ | -] NUM-LINES\n\
Use \"info win\" to see the names of the windows currently being displayed."),
	       &tui_window_cmds);
  add_com_alias ("winwidth", winwidth_cmd, class_tui, 0);
  set_cmd_completer (winwidth_cmd, winheight_completer);

  add_info ("win", tui_all_windows_info,
	    _("List of all displayed windows.\n\
Usage: info win"));
  cmd_list_element *focus_cmd
    = add_cmd ("focus", class_tui, tui_set_focus_command, _("\
Set focus to named window or next/prev window.\n\
Usage: tui focus [WINDOW-NAME | next | prev]\n\
Use \"info win\" to see the names of the windows currently being displayed."),
	       tui_get_cmd_list ());
  add_com_alias ("focus", focus_cmd, class_tui, 0);
  add_com_alias ("fs", focus_cmd, class_tui, 0);
  set_cmd_completer (focus_cmd, focus_completer);
  add_com ("+", class_tui, tui_scroll_forward_command, _("\
Scroll window forward.\n\
Usage: + [N] [WIN]\n\
Scroll window WIN N lines forwards.  Both WIN and N are optional, N\n\
defaults to 1, and WIN defaults to the currently focused window."));
  add_com ("-", class_tui, tui_scroll_backward_command, _("\
Scroll window backward.\n\
Usage: - [N] [WIN]\n\
Scroll window WIN N lines backwards.  Both WIN and N are optional, N\n\
defaults to 1, and WIN defaults to the currently focused window."));
  add_com ("<", class_tui, tui_scroll_left_command, _("\
Scroll window text to the left.\n\
Usage: < [N] [WIN]\n\
Scroll window WIN N characters left.  Both WIN and N are optional, N\n\
defaults to 1, and WIN defaults to the currently focused window."));
  add_com (">", class_tui, tui_scroll_right_command, _("\
Scroll window text to the right.\n\
Usage: > [N] [WIN]\n\
Scroll window WIN N characters right.  Both WIN and N are optional, N\n\
defaults to 1, and WIN defaults to the currently focused window."));

  /* Define the tui control variables.  */
  add_setshow_enum_cmd ("border-kind", no_class, tui_border_kind_enums,
			&tui_border_kind, _("\
Set the kind of border for TUI windows."), _("\
Show the kind of border for TUI windows."), _("\
This variable controls the border of TUI windows:\n\
   space           use a white space\n\
   ascii           use ascii characters + - | for the border\n\
   acs             use the Alternate Character Set"),
			tui_set_var_cmd,
			show_tui_border_kind,
			&tui_setlist, &tui_showlist);

  const std::string help_attribute_mode (_("\
   normal          normal display\n\
   standout        use highlight mode of terminal\n\
   reverse         use reverse video mode\n\
   half            use half bright\n\
   half-standout   use half bright and standout mode\n\
   bold            use extra bright or bold\n\
   bold-standout   use extra bright or bold with standout mode"));

  const std::string help_tui_border_mode
    = (_("\
This variable controls the attributes to use for the window borders:\n")
       + help_attribute_mode);

  add_setshow_enum_cmd ("border-mode", no_class, tui_border_mode_enums,
			&tui_border_mode, _("\
Set the attribute mode to use for the TUI window borders."), _("\
Show the attribute mode to use for the TUI window borders."),
			help_tui_border_mode.c_str (),
			tui_set_var_cmd,
			show_tui_border_mode,
			&tui_setlist, &tui_showlist);

  const std::string help_tui_active_border_mode
    = (_("\
This variable controls the attributes to use for the active window borders:\n")
       + help_attribute_mode);

  add_setshow_enum_cmd ("active-border-mode", no_class, tui_border_mode_enums,
			&tui_active_border_mode, _("\
Set the attribute mode to use for the active TUI window border."), _("\
Show the attribute mode to use for the active TUI window border."),
			help_tui_active_border_mode.c_str (),
			tui_set_var_cmd,
			show_tui_active_border_mode,
			&tui_setlist, &tui_showlist);

  add_setshow_zuinteger_cmd ("tab-width", no_class,
			     &internal_tab_width, _("\
Set the tab width, in characters, for the TUI."), _("\
Show the tab width, in characters, for the TUI."), _("\
This variable controls how many spaces are used to display a tab character."),
			     tui_set_tab_width, tui_show_tab_width,
			     &tui_setlist, &tui_showlist);

  add_setshow_boolean_cmd ("tui-resize-message", class_maintenance,
			   &resize_message, _("\
Set TUI resize messaging."), _("\
Show TUI resize messaging."), _("\
When enabled GDB will print a message when the terminal is resized."),
			   nullptr,
			   show_tui_resize_message,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("compact-source", class_tui,
			   &compact_source, _("\
Set whether the TUI source window is compact."), _("\
Show whether the TUI source window is compact."), _("\
This variable controls whether the TUI source window is shown\n\
in a compact form.  The compact form uses less horizontal space."),
			   tui_set_compact_source, tui_show_compact_source,
			   &tui_setlist, &tui_showlist);

  add_setshow_boolean_cmd ("mouse-events", class_tui,
			   &tui_enable_mouse, _("\
Set whether TUI mode handles mouse clicks."), _("\
Show whether TUI mode handles mouse clicks."), _("\
When on (default), mouse clicks control the TUI and can be accessed by Python\n\
extensions.  When off, mouse clicks are handled by the terminal, enabling\n\
terminal-native text selection."),
			   nullptr,
			   show_tui_mouse_events,
			   &tui_setlist, &tui_showlist);

  add_setshow_boolean_cmd ("tui-current-position", class_maintenance,
			   &style_tui_current_position, _("\
Set whether to style text highlighted by the TUI's current position indicator."),
			   _("\
Show whether to style text highlighted by the TUI's current position indicator."),
			   _("\
When enabled, the source and assembly code highlighted by the TUI's current\n\
position indicator is styled."),
			   set_style_tui_current_position,
			   show_style_tui_current_position,
			   &style_set_list,
			   &style_show_list);

  add_setshow_boolean_cmd ("tui-left-margin-verbose", class_maintenance,
			   &tui_left_margin_verbose, _("\
Set whether the left margin should use '_' and '0' instead of spaces."),
			   _("\
Show whether the left margin should use '_' and '0' instead of spaces."),
			   _("\
When enabled, the left margin will use '_' and '0' instead of spaces."),
			   nullptr,
			   nullptr,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  tui_border_style.changed.attach (tui_rehighlight_all, "tui-win");
  tui_active_border_style.changed.attach (tui_rehighlight_all, "tui-win");
}

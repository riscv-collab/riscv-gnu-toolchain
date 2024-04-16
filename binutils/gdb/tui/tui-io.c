/* TUI support I/O functions.

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
#include "target.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "command.h"
#include "top.h"
#include "ui.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-command.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-file.h"
#include "tui/tui-out.h"
#include "ui-out.h"
#include "cli-out.h"
#include <fcntl.h>
#include <signal.h>
#ifdef __MINGW32__
#include <windows.h>
#endif
#include "gdbsupport/filestuff.h"
#include "completer.h"
#include "gdb_curses.h"
#include <map>
#include "pager.h"
#include "gdbsupport/gdb-checked-static-cast.h"

/* This redefines CTRL if it is not already defined, so it must come
   after terminal state releated include files like <term.h> and
   "gdb_curses.h".  */
#include "readline/readline.h"

#ifdef __MINGW32__
static SHORT ncurses_norm_attr;
#endif

static int tui_getc (FILE *fp);

static int
key_is_start_sequence (int ch)
{
  return (ch == 27);
}

/* Use definition from readline 4.3.  */
#undef CTRL_CHAR
#define CTRL_CHAR(c) \
     ((c) < control_character_threshold && (((c) & 0x80) == 0))

/* This file controls the IO interactions between gdb and curses.
   When the TUI is enabled, gdb has two modes a curses and a standard
   mode.

   In curses mode, the gdb outputs are made in a curses command
   window.  For this, the gdb_stdout and gdb_stderr are redirected to
   the specific ui_file implemented by TUI.  The output is handled by
   tui_puts().  The input is also controlled by curses with
   tui_getc().  The readline library uses this function to get its
   input.  Several readline hooks are installed to redirect readline
   output to the TUI (see also the note below).

   In normal mode, the gdb outputs are restored to their origin, that
   is as if TUI is not used.  Readline also uses its original getc()
   function with stdin.

   Note SCz/2001-07-21: the current readline is not clean in its
   management of the output.  Even if we install a redisplay handler,
   it sometimes writes on a stdout file.  It is important to redirect
   every output produced by readline, otherwise the curses window will
   be garbled.  This is implemented with a pipe that TUI reads and
   readline writes to.  A gdb input handler is created so that reading
   the pipe is handled automatically.  This will probably not work on
   non-Unix platforms.  The best fix is to make readline clean enough
   so that is never write on stdout.

   Note SCz/2002-09-01: we now use more readline hooks and it seems
   that with them we don't need the pipe anymore (verified by creating
   the pipe and closing its end so that write causes a SIGPIPE).  The
   old pipe code is still there and can be conditionally removed by
   #undef TUI_USE_PIPE_FOR_READLINE.  */

/* For gdb 5.3, prefer to continue the pipe hack as a backup wheel.  */
#ifdef HAVE_PIPE
#define TUI_USE_PIPE_FOR_READLINE
#endif
/* #undef TUI_USE_PIPE_FOR_READLINE */

/* TUI output files.  */
static struct ui_file *tui_stdout;
static struct ui_file *tui_stderr;
static struct ui_file *tui_stdlog;
struct ui_out *tui_out;

/* GDB output files in non-curses mode.  */
static struct ui_file *tui_old_stdout;
static struct ui_file *tui_old_stderr;
static struct ui_file *tui_old_stdlog;
cli_ui_out *tui_old_uiout;

/* Readline previous hooks.  */
static rl_getc_func_t *tui_old_rl_getc_function;
static rl_voidfunc_t *tui_old_rl_redisplay_function;
static rl_vintfunc_t *tui_old_rl_prep_terminal;
static rl_voidfunc_t *tui_old_rl_deprep_terminal;
static rl_compdisp_func_t *tui_old_rl_display_matches_hook;
static int tui_old_rl_echoing_p;

/* Readline output stream.
   Should be removed when readline is clean.  */
static FILE *tui_rl_outstream;
static FILE *tui_old_rl_outstream;
#ifdef TUI_USE_PIPE_FOR_READLINE
static int tui_readline_pipe[2];
#endif

/* Print a character in the curses command window.  The output is
   buffered.  It is up to the caller to refresh the screen if
   necessary.  */

static void
do_tui_putc (WINDOW *w, char c)
{
  /* Expand TABs, since ncurses on MS-Windows doesn't.  */
  if (c == '\t')
    {
      int col;

      col = getcurx (w);
      do
	{
	  waddch (w, ' ');
	  col++;
	}
      while ((col % 8) != 0);
    }
  else
    waddch (w, c);
}

/* Update the cached value of the command window's start line based on
   the window's current Y coordinate.  */

static void
update_cmdwin_start_line ()
{
  TUI_CMD_WIN->start_line = getcury (TUI_CMD_WIN->handle.get ());
}

/* Print a character in the curses command window.  The output is
   buffered.  It is up to the caller to refresh the screen if
   necessary.  */

static void
tui_putc (char c)
{
  do_tui_putc (TUI_CMD_WIN->handle.get (), c);
  update_cmdwin_start_line ();
}

/* This maps colors to their corresponding color index.  */

static std::map<ui_file_style::color, int> color_map;

/* This holds a pair of colors and is used to track the mapping
   between a color pair index and the actual colors.  */

struct color_pair
{
  int fg;
  int bg;

  bool operator< (const color_pair &o) const
  {
    return fg < o.fg || (fg == o.fg && bg < o.bg);
  }
};

/* This maps pairs of colors to their corresponding color pair
   index.  */

static std::map<color_pair, int> color_pair_map;

/* This is indexed by ANSI color offset from the base color, and holds
   the corresponding curses color constant.  */

static const int curses_colors[] = {
  COLOR_BLACK,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW,
  COLOR_BLUE,
  COLOR_MAGENTA,
  COLOR_CYAN,
  COLOR_WHITE
};

/* Given a color, find its index.  */

static bool
get_color (const ui_file_style::color &color, int *result)
{
  if (color.is_none ())
    *result = -1;
  else if (color.is_basic ())
    *result = curses_colors[color.get_value ()];
  else
    {
      auto it = color_map.find (color);
      if (it == color_map.end ())
	{
	  /* The first 8 colors are standard.  */
	  int next = color_map.size () + 8;
	  if (next >= COLORS)
	    return false;
	  uint8_t rgb[3];
	  color.get_rgb (rgb);
	  /* We store RGB as 0..255, but curses wants 0..1000.  */
	  if (init_color (next, rgb[0] * 1000 / 255, rgb[1] * 1000 / 255,
			  rgb[2] * 1000 / 255) == ERR)
	    return false;
	  color_map[color] = next;
	  *result = next;
	}
      else
	*result = it->second;
    }
  return true;
}

/* The most recently emitted color pair.  */

static int last_color_pair = -1;

/* The most recently applied style.  */

static ui_file_style last_style;

/* If true, we're highlighting the current source line in reverse
   video mode.  */
static bool reverse_mode_p = false;

/* The background/foreground colors before we entered reverse
   mode.  */
static ui_file_style::color reverse_save_bg (ui_file_style::NONE);
static ui_file_style::color reverse_save_fg (ui_file_style::NONE);

/* Given two colors, return their color pair index; making a new one
   if necessary.  */

static int
get_color_pair (int fg, int bg)
{
  color_pair c = { fg, bg };
  auto it = color_pair_map.find (c);
  if (it == color_pair_map.end ())
    {
      /* Color pair 0 is our default color, so new colors start at
	 1.  */
      int next = color_pair_map.size () + 1;
      /* Curses has a limited number of available color pairs.  Fall
	 back to the default if we've used too many.  */
      if (next >= COLOR_PAIRS)
	return 0;
      init_pair (next, fg, bg);
      color_pair_map[c] = next;
      return next;
    }
  return it->second;
}

/* Apply STYLE to W.  */

void
tui_apply_style (WINDOW *w, ui_file_style style)
{
  /* Reset.  */
  wattron (w, A_NORMAL);
  wattroff (w, A_BOLD);
  wattroff (w, A_DIM);
  wattroff (w, A_REVERSE);
  if (last_color_pair != -1)
    wattroff (w, COLOR_PAIR (last_color_pair));
  wattron (w, COLOR_PAIR (0));

  const ui_file_style::color &fg = style.get_foreground ();
  const ui_file_style::color &bg = style.get_background ();
  if (!fg.is_none () || !bg.is_none ())
    {
      int fgi, bgi;
      if (get_color (fg, &fgi) && get_color (bg, &bgi))
	{
#ifdef __MINGW32__
	  /* MS-Windows port of ncurses doesn't support implicit
	     default foreground and background colors, so we must
	     specify them explicitly when needed, using the colors we
	     saw at startup.  */
	  if (fgi == -1)
	    fgi = ncurses_norm_attr & 15;
	  if (bgi == -1)
	    bgi = (ncurses_norm_attr >> 4) & 15;
#endif
	  int pair = get_color_pair (fgi, bgi);
	  if (last_color_pair != -1)
	    wattroff (w, COLOR_PAIR (last_color_pair));
	  wattron (w, COLOR_PAIR (pair));
	  last_color_pair = pair;
	}
    }

  switch (style.get_intensity ())
    {
    case ui_file_style::NORMAL:
      break;

    case ui_file_style::BOLD:
      wattron (w, A_BOLD);
      break;

    case ui_file_style::DIM:
      wattron (w, A_DIM);
      break;

    default:
      gdb_assert_not_reached ("invalid intensity");
    }

  if (style.is_reverse ())
    wattron (w, A_REVERSE);

  last_style = style;
}

/* Apply an ANSI escape sequence from BUF to W.  BUF must start with
   the ESC character.  If BUF does not start with an ANSI escape,
   return 0.  Otherwise, apply the sequence if it is recognized, or
   simply ignore it if not.  In this case, the number of bytes read
   from BUF is returned.  */

static size_t
apply_ansi_escape (WINDOW *w, const char *buf)
{
  ui_file_style style = last_style;
  size_t n_read;

  if (!style.parse (buf, &n_read))
    return n_read;

  if (reverse_mode_p)
    {
      if (!style_tui_current_position)
	return n_read;

      /* We want to reverse _only_ the default foreground/background
	 colors.  If the foreground color is not the default (because
	 the text was styled), we want to leave it as is.  If e.g.,
	 the terminal is fg=BLACK, and bg=WHITE, and the style wants
	 to print text in RED, we want to reverse the background color
	 (print in BLACK), but still print the text in RED.  To do
	 that, we enable the A_REVERSE attribute, and re-reverse the
	 parsed-style's fb/bg colors.

	 Notes on the approach:

	  - there's no portable way to know which colors the default
	    fb/bg colors map to.

	  - this approach does the right thing even if you change the
	    terminal colors while GDB is running -- the reversed
	    colors automatically adapt.
      */
      if (!style.is_default ())
	{
	  ui_file_style::color bg = style.get_background ();
	  ui_file_style::color fg = style.get_foreground ();
	  style.set_fg (bg);
	  style.set_bg (fg);
	}

      /* Enable A_REVERSE.  */
      style.set_reverse (true);
    }

  tui_apply_style (w, style);
  return n_read;
}

/* See tui.io.h.  */

void
tui_set_reverse_mode (WINDOW *w, bool reverse)
{
  ui_file_style style = last_style;

  reverse_mode_p = reverse;

  if (reverse)
    {
      reverse_save_bg = style.get_background ();
      reverse_save_fg = style.get_foreground ();

      if (!style_tui_current_position)
	{
	  /* Switch to default style (reversed) while highlighting the
	     current position.  */
	  style = {};
	}
    }
  else
    {
      style.set_bg (reverse_save_bg);
      style.set_fg (reverse_save_fg);
    }

  style.set_reverse (reverse);

  tui_apply_style (w, style);
}

/* Print LENGTH characters from the buffer pointed to by BUF to the
   curses command window.  The output is buffered.  It is up to the
   caller to refresh the screen if necessary.  */

void
tui_write (const char *buf, size_t length)
{
  /* We need this to be \0-terminated for the regexp matching.  */
  std::string copy (buf, length);
  tui_puts (copy.c_str ());
}

/* Print a string in the curses command window.  The output is
   buffered.  It is up to the caller to refresh the screen if
   necessary.  */

void
tui_puts (const char *string, WINDOW *w)
{
  if (w == nullptr)
    w = TUI_CMD_WIN->handle.get ();

  while (true)
    {
      const char *next = strpbrk (string, "\n\1\2\033\t");

      /* Print the plain text prefix.  */
      size_t n_chars = next == nullptr ? strlen (string) : next - string;
      if (n_chars > 0)
	waddnstr (w, string, n_chars);

      /* We finished.  */
      if (next == nullptr)
	break;

      char c = *next;
      switch (c)
	{
	case '\1':
	case '\2':
	  /* Ignore these, they are readline escape-marking
	     sequences.  */
	  ++next;
	  break;

	case '\n':
	case '\t':
	  do_tui_putc (w, c);
	  ++next;
	  break;

	case '\033':
	  {
	    size_t bytes_read = apply_ansi_escape (w, next);
	    if (bytes_read > 0)
	      next += bytes_read;
	    else
	      {
		/* Just drop the escape.  */
		++next;
	      }
	  }
	  break;

	default:
	  gdb_assert_not_reached ("missing case in tui_puts");
	}

      string = next;
    }

  if (TUI_CMD_WIN != nullptr && w == TUI_CMD_WIN->handle.get ())
    update_cmdwin_start_line ();
}

static void
tui_puts_internal (WINDOW *w, const char *string, int *height)
{
  char c;
  int prev_col = 0;
  bool saw_nl = false;

  while ((c = *string++) != 0)
    {
      if (c == '\1' || c == '\2')
	{
	  /* Ignore these, they are readline escape-marking
	     sequences.  */
	  continue;
	}

      if (c == '\033')
	{
	  size_t bytes_read = apply_ansi_escape (w, string - 1);
	  if (bytes_read > 0)
	    {
	      string = string + bytes_read - 1;
	      continue;
	    }
	}

      if (c == '\n')
	saw_nl = true;

      do_tui_putc (w, c);

      if (height != nullptr)
	{
	  int col = getcurx (w);
	  if (col <= prev_col)
	    ++*height;
	  prev_col = col;
	}
    }

  if (TUI_CMD_WIN != nullptr && w == TUI_CMD_WIN->handle.get ())
    update_cmdwin_start_line ();
  if (saw_nl)
    wrefresh (w);
}

/* Readline callback.
   Redisplay the command line with its prompt after readline has
   changed the edited text.  */
void
tui_redisplay_readline (void)
{
  int prev_col;
  int height;
  int col;
  int c_pos;
  int c_line;
  int in;
  WINDOW *w;
  const char *prompt;
  int start_line;

  /* Detect when we temporarily left SingleKey and now the readline
     edit buffer is empty, automatically restore the SingleKey
     mode.  The restore must only be done if the command has finished.
     The command could call prompt_for_continue and we must not
     restore SingleKey so that the prompt and normal keymap are used.  */
  if (tui_current_key_mode == TUI_ONE_COMMAND_MODE && rl_end == 0
      && !gdb_in_secondary_prompt_p (current_ui))
    tui_set_key_mode (TUI_SINGLE_KEY_MODE);

  if (tui_current_key_mode == TUI_SINGLE_KEY_MODE)
    prompt = "";
  else
    prompt = rl_display_prompt;
  
  c_pos = -1;
  c_line = -1;
  w = TUI_CMD_WIN->handle.get ();
  start_line = TUI_CMD_WIN->start_line;
  wmove (w, start_line, 0);
  prev_col = 0;
  height = 1;
  if (prompt != nullptr)
    tui_puts_internal (w, prompt, &height);

  prev_col = getcurx (w);
  for (in = 0; in <= rl_end; in++)
    {
      unsigned char c;
      
      if (in == rl_point)
	{
	  getyx (w, c_line, c_pos);
	}

      if (in == rl_end)
	break;

      c = (unsigned char) rl_line_buffer[in];
      if (CTRL_CHAR (c) || c == RUBOUT)
	{
	  waddch (w, '^');
	  waddch (w, CTRL_CHAR (c) ? UNCTRL (c) : '?');
	}
      else if (c == '\t')
	{
	  /* Expand TABs, since ncurses on MS-Windows doesn't.  */
	  col = getcurx (w);
	  do
	    {
	      waddch (w, ' ');
	      col++;
	    } while ((col % 8) != 0);
	}
      else
	{
	  waddch (w, c);
	}
      if (c == '\n')
	TUI_CMD_WIN->start_line = getcury (w);
      col = getcurx (w);
      if (col < prev_col)
	height++;
      prev_col = col;
    }
  wclrtobot (w);
  TUI_CMD_WIN->start_line = getcury (w);
  if (c_line >= 0)
    wmove (w, c_line, c_pos);
  TUI_CMD_WIN->start_line -= height - 1;

  wrefresh (w);
  fflush(stdout);
}

/* Readline callback to prepare the terminal.  It is called once each
   time we enter readline.  Terminal is already setup in curses
   mode.  */
static void
tui_prep_terminal (int notused1)
{
#ifdef NCURSES_MOUSE_VERSION
  if (tui_enable_mouse)
    mousemask (ALL_MOUSE_EVENTS, NULL);
#endif
}

/* Readline callback to restore the terminal.  It is called once each
   time we leave readline.  There is nothing to do in curses mode.  */
static void
tui_deprep_terminal (void)
{
#ifdef NCURSES_MOUSE_VERSION
  mousemask (0, NULL);
#endif
}

#ifdef TUI_USE_PIPE_FOR_READLINE
/* Read readline output pipe and feed the command window with it.
   Should be removed when readline is clean.  */
static void
tui_readline_output (int error, gdb_client_data data)
{
  int size;
  char buf[256];

  size = read (tui_readline_pipe[0], buf, sizeof (buf) - 1);
  if (size > 0 && tui_active)
    {
      buf[size] = 0;
      tui_puts (buf);
    }
}
#endif

/* TUI version of displayer.crlf.  */

static void
tui_mld_crlf (const struct match_list_displayer *displayer)
{
  tui_putc ('\n');
}

/* TUI version of displayer.putch.  */

static void
tui_mld_putch (const struct match_list_displayer *displayer, int ch)
{
  tui_putc (ch);
}

/* TUI version of displayer.puts.  */

static void
tui_mld_puts (const struct match_list_displayer *displayer, const char *s)
{
  tui_puts (s);
}

/* TUI version of displayer.flush.  */

static void
tui_mld_flush (const struct match_list_displayer *displayer)
{
  wrefresh (TUI_CMD_WIN->handle.get ());
}

/* TUI version of displayer.erase_entire_line.  */

static void
tui_mld_erase_entire_line (const struct match_list_displayer *displayer)
{
  WINDOW *w = TUI_CMD_WIN->handle.get ();
  int cur_y = getcury (w);

  wmove (w, cur_y, 0);
  wclrtoeol (w);
  wmove (w, cur_y, 0);
}

/* TUI version of displayer.beep.  */

static void
tui_mld_beep (const struct match_list_displayer *displayer)
{
  beep ();
}

/* A wrapper for wgetch that enters nonl mode.  We We normally want
  curses' "nl" mode, but when reading from the user, we'd like to
  differentiate between C-j and C-m, because some users bind these
  keys differently in their .inputrc.  So, put curses into nonl mode
  just when reading from the user.  See PR tui/20819.  */

static int
gdb_wgetch (WINDOW *win)
{
  nonl ();
  int r = wgetch (win);
  nl ();
  return r;
}

/* Helper function for tui_mld_read_key.
   This temporarily replaces tui_getc for use during tab-completion
   match list display.  */

static int
tui_mld_getc (FILE *fp)
{
  WINDOW *w = TUI_CMD_WIN->handle.get ();
  int c = gdb_wgetch (w);

  return c;
}

/* TUI version of displayer.read_key.  */

static int
tui_mld_read_key (const struct match_list_displayer *displayer)
{
  /* We can't use tui_getc as we need NEWLINE to not get emitted.  */
  scoped_restore restore_getc_function
    = make_scoped_restore (&rl_getc_function, tui_mld_getc);
  return rl_read_key ();
}

/* TUI version of rl_completion_display_matches_hook.
   See gdb_display_match_list for a description of the arguments.  */

static void
tui_rl_display_match_list (char **matches, int len, int max)
{
  struct match_list_displayer displayer;

  rl_get_screen_size (&displayer.height, &displayer.width);
  displayer.crlf = tui_mld_crlf;
  displayer.putch = tui_mld_putch;
  displayer.puts = tui_mld_puts;
  displayer.flush = tui_mld_flush;
  displayer.erase_entire_line = tui_mld_erase_entire_line;
  displayer.beep = tui_mld_beep;
  displayer.read_key = tui_mld_read_key;

  gdb_display_match_list (matches, len, max, &displayer);
}

/* Setup the IO for curses or non-curses mode.
   - In non-curses mode, readline and gdb use the standard input and
   standard output/error directly.
   - In curses mode, the standard output/error is controlled by TUI
   with the tui_stdout and tui_stderr.  The output is redirected in
   the curses command window.  Several readline callbacks are installed
   so that readline asks for its input to the curses command window
   with wgetch().  */
void
tui_setup_io (int mode)
{
  extern int _rl_echoing_p;

  if (mode)
    {
      /* Ensure that readline has been initialized before saving any
	 of its variables.  */
      tui_ensure_readline_initialized ();

      /* Redirect readline to TUI.  */
      tui_old_rl_redisplay_function = rl_redisplay_function;
      tui_old_rl_deprep_terminal = rl_deprep_term_function;
      tui_old_rl_prep_terminal = rl_prep_term_function;
      tui_old_rl_getc_function = rl_getc_function;
      tui_old_rl_display_matches_hook = rl_completion_display_matches_hook;
      tui_old_rl_outstream = rl_outstream;
      tui_old_rl_echoing_p = _rl_echoing_p;
      rl_redisplay_function = tui_redisplay_readline;
      rl_deprep_term_function = tui_deprep_terminal;
      rl_prep_term_function = tui_prep_terminal;
      rl_getc_function = tui_getc;
      _rl_echoing_p = 0;
      rl_outstream = tui_rl_outstream;
      rl_prompt = 0;
      rl_completion_display_matches_hook = tui_rl_display_match_list;
      rl_already_prompted = 0;

      /* Keep track of previous gdb output.  */
      tui_old_stdout = gdb_stdout;
      tui_old_stderr = gdb_stderr;
      tui_old_stdlog = gdb_stdlog;
      tui_old_uiout = gdb::checked_static_cast<cli_ui_out *> (current_uiout);

      /* Reconfigure gdb output.  */
      gdb_stdout = tui_stdout;
      gdb_stderr = tui_stderr;
      gdb_stdlog = tui_stdlog;
      gdb_stdtarg = gdb_stderr;
      gdb_stdtargerr = gdb_stderr;
      current_uiout = tui_out;

      /* Save tty for SIGCONT.  */
      savetty ();
    }
  else
    {
      /* Restore gdb output.  */
      gdb_stdout = tui_old_stdout;
      gdb_stderr = tui_old_stderr;
      gdb_stdlog = tui_old_stdlog;
      gdb_stdtarg = gdb_stderr;
      gdb_stdtargerr = gdb_stderr;
      current_uiout = tui_old_uiout;

      /* Restore readline.  */
      rl_redisplay_function = tui_old_rl_redisplay_function;
      rl_deprep_term_function = tui_old_rl_deprep_terminal;
      rl_prep_term_function = tui_old_rl_prep_terminal;
      rl_getc_function = tui_old_rl_getc_function;
      rl_completion_display_matches_hook = tui_old_rl_display_matches_hook;
      rl_outstream = tui_old_rl_outstream;
      _rl_echoing_p = tui_old_rl_echoing_p;
      rl_already_prompted = 0;

      /* Save tty for SIGCONT.  */
      savetty ();

      /* Clean up color information.  */
      last_color_pair = -1;
      last_style = ui_file_style ();
      color_map.clear ();
      color_pair_map.clear ();
    }
}

#ifdef SIGCONT
/* Catch SIGCONT to restore the terminal and refresh the screen.  */
static void
tui_cont_sig (int sig)
{
  if (tui_active)
    {
      /* Restore the terminal setting because another process (shell)
	 might have changed it.  */
      resetty ();

      /* Force a refresh of the screen.  */
      tui_refresh_all_win ();
    }
  signal (sig, tui_cont_sig);
}
#endif

/* Initialize the IO for gdb in curses mode.  */
void
tui_initialize_io (void)
{
#ifdef SIGCONT
  signal (SIGCONT, tui_cont_sig);
#endif

  /* Create tui output streams.  */
  tui_stdout = new pager_file (new tui_file (stdout, true));
  tui_stderr = new tui_file (stderr, false);
  tui_stdlog = new timestamped_file (tui_stderr);
  tui_out = new tui_ui_out (tui_stdout);

  /* Create the default UI.  */
  tui_old_uiout = new cli_ui_out (gdb_stdout);

#ifdef TUI_USE_PIPE_FOR_READLINE
  /* Temporary solution for readline writing to stdout: redirect
     readline output in a pipe, read that pipe and output the content
     in the curses command window.  */
  if (gdb_pipe_cloexec (tui_readline_pipe) != 0)
    error (_("Cannot create pipe for readline"));

  tui_rl_outstream = fdopen (tui_readline_pipe[1], "w");
  if (tui_rl_outstream == 0)
    error (_("Cannot redirect readline output"));

  setvbuf (tui_rl_outstream, NULL, _IOLBF, 0);

#ifdef O_NONBLOCK
  (void) fcntl (tui_readline_pipe[0], F_SETFL, O_NONBLOCK);
#else
#ifdef O_NDELAY
  (void) fcntl (tui_readline_pipe[0], F_SETFL, O_NDELAY);
#endif
#endif
  add_file_handler (tui_readline_pipe[0], tui_readline_output, 0, "tui");
#else
  tui_rl_outstream = stdout;
#endif

#ifdef __MINGW32__
  /* MS-Windows port of ncurses doesn't support default foreground and
     background colors, so we must record the default colors at startup.  */
  HANDLE hstdout = (HANDLE)_get_osfhandle (fileno (stdout));
  DWORD cmode;
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (hstdout != INVALID_HANDLE_VALUE
      && GetConsoleMode (hstdout, &cmode) != 0
      && GetConsoleScreenBufferInfo (hstdout, &csbi))
    ncurses_norm_attr = csbi.wAttributes;
#endif
}

/* Dispatch the correct tui function based upon the mouse event.  */

#ifdef NCURSES_MOUSE_VERSION

static void
tui_dispatch_mouse_event ()
{
  MEVENT mev;
  if (getmouse (&mev) != OK)
    return;

  for (tui_win_info *wi : all_tui_windows ())
    if (mev.x > wi->x && mev.x < wi->x + wi->width - 1
	&& mev.y > wi->y && mev.y < wi->y + wi->height - 1)
      {
	if ((mev.bstate & BUTTON1_CLICKED) != 0
	    || (mev.bstate & BUTTON2_CLICKED) != 0
	    || (mev.bstate & BUTTON3_CLICKED) != 0)
	  {
	    int button = (mev.bstate & BUTTON1_CLICKED) != 0 ? 1
	      :         ((mev.bstate & BUTTON2_CLICKED) != 0 ? 2
			 : 3);
	    wi->click (mev.x - wi->x - 1, mev.y - wi->y - 1, button);
	  }
#ifdef BUTTON5_PRESSED
	else if ((mev.bstate & BUTTON4_PRESSED) != 0)
	  wi->backward_scroll (3);
	else if ((mev.bstate & BUTTON5_PRESSED) != 0)
	  wi->forward_scroll (3);
#endif
	break;
      }
}

#endif

/* Dispatch the correct tui function based upon the control
   character.  */
static unsigned int
tui_dispatch_ctrl_char (unsigned int ch)
{
  struct tui_win_info *win_info = tui_win_with_focus ();

  /* If no window has the focus, or if the focus window can't scroll,
     just pass the character through.  */
  if (win_info == NULL || !win_info->can_scroll ())
    return ch;

  switch (ch)
    {
    case KEY_NPAGE:
      win_info->forward_scroll (0);
      break;
    case KEY_PPAGE:
      win_info->backward_scroll (0);
      break;
    case KEY_DOWN:
    case KEY_SF:
      win_info->forward_scroll (1);
      break;
    case KEY_UP:
    case KEY_SR:
      win_info->backward_scroll (1);
      break;
    case KEY_RIGHT:
      win_info->left_scroll (1);
      break;
    case KEY_LEFT:
      win_info->right_scroll (1);
      break;
    default:
      /* We didn't recognize the character as a control character, so pass it
	 through.  */
      return ch;
    }

  /* We intercepted the control character, so return 0 (which readline
     will interpret as a no-op).  */
  return 0;
}

/* See tui-io.h.   */

void
tui_inject_newline_into_command_window ()
{
  gdb_assert (tui_active);

  WINDOW *w = TUI_CMD_WIN->handle.get ();

  /* When hitting return with an empty input, gdb executes the last
     command.  If we emit a newline, this fills up the command window
     with empty lines with gdb prompt at beginning.  Instead of that,
     stay on the same line but provide a visual effect to show the
     user we recognized the command.  */
  if (rl_end == 0 && !gdb_in_secondary_prompt_p (current_ui))
    {
      wmove (w, getcury (w), 0);

      /* Clear the line.  This will blink the gdb prompt since
	 it will be redrawn at the same line.  */
      wclrtoeol (w);
      wrefresh (w);
      napms (20);
    }
  else
    {
      /* Move cursor to the end of the command line before emitting the
	 newline.  We need to do so because when ncurses outputs a newline
	 it truncates any text that appears past the end of the cursor.  */
      int px, py;
      getyx (w, py, px);
      px += rl_end - rl_point;
      py += px / TUI_CMD_WIN->width;
      px %= TUI_CMD_WIN->width;
      wmove (w, py, px);
      tui_putc ('\n');
    }
}

/* If we're passing an escape sequence to readline, this points to a
   string holding the remaining characters of the sequence to pass.
   We advance the pointer one character at a time until '\0' is
   reached.  */
static const char *cur_seq = nullptr;

/* Set CUR_SEQ to point at the current sequence to pass to readline,
   setup to call the input handler again so we complete the sequence
   shortly, and return the first character to start the sequence.  */

static int
start_sequence (const char *seq)
{
  call_stdin_event_handler_again_p = 1;
  cur_seq = seq + 1;
  return seq[0];
}

/* Main worker for tui_getc.  Get a character from the command window.
   This is called from the readline package, but wrapped in a
   try/catch by tui_getc.  */

static int
tui_getc_1 (FILE *fp)
{
  int ch;
  WINDOW *w;

  w = TUI_CMD_WIN->handle.get ();

#ifdef TUI_USE_PIPE_FOR_READLINE
  /* Flush readline output.  */
  tui_readline_output (0, 0);
#endif

  /* We enable keypad mode so that curses's wgetch processes mouse
     escape sequences.  In keypad mode, wgetch also processes the
     escape sequences for keys such as up/down etc. and returns KEY_UP
     / KEY_DOWN etc.  When we have the focus on the command window
     though, we want to pass the raw up/down etc. escape codes to
     readline so readline understands them.  */
  if (cur_seq != nullptr)
    {
      ch = *cur_seq++;

      /* If we've reached the end of the string, we're done with the
	 sequence.  Otherwise, setup to get back here again for
	 another character.  */
      if (*cur_seq == '\0')
	cur_seq = nullptr;
      else
	call_stdin_event_handler_again_p = 1;
      return ch;
    }
  else
    ch = gdb_wgetch (w);

  /* Handle prev/next/up/down here.  */
  ch = tui_dispatch_ctrl_char (ch);

#ifdef NCURSES_MOUSE_VERSION
  if (ch == KEY_MOUSE)
    {
      tui_dispatch_mouse_event ();
      return 0;
    }
#endif

  /* Translate curses keys back to escape sequences so that readline
     can understand them.  We do this irrespective of which window has
     the focus.  If e.g., we're focused on a non-command window, then
     the up/down keys will already have been filtered by
     tui_dispatch_ctrl_char.  Keys that haven't been intercepted will
     be passed down to readline.  */
  if (current_ui->command_editing)
    {
      /* For the standard arrow keys + home/end, hardcode sequences
	 readline understands.  See bind_arrow_keys_internal in
	 readline/readline.c.  */
      switch (ch)
	{
	case KEY_UP:
	  return start_sequence ("\033[A");
	case KEY_DOWN:
	  return start_sequence ("\033[B");
	case KEY_RIGHT:
	  return start_sequence ("\033[C");
	case KEY_LEFT:
	  return start_sequence ("\033[D");
	case KEY_HOME:
	  return start_sequence ("\033[H");
	case KEY_END:
	  return start_sequence ("\033[F");

	/* del and ins are unfortunately not hardcoded in readline for
	   all systems.  */

	case KEY_DC: /* del */
#ifdef __MINGW32__
	  return start_sequence ("\340S");
#else
	  return start_sequence ("\033[3~");
#endif

	case KEY_IC: /* ins */
#if defined __MINGW32__
	  return start_sequence ("\340R");
#else
	  return start_sequence ("\033[2~");
#endif
	}

      /* Keycodes above KEY_MAX are not guaranteed to be stable.
	 Compare keyname instead.  */
      if (ch >= KEY_MAX)
	{
	  std::string_view name;
	  const char *name_str = keyname (ch);
	  if (name_str != nullptr)
	    name = std::string_view (name_str);

	  /* The following sequences are hardcoded in readline as
	     well.  */

	  /* ctrl-arrow keys */
	  if (name == "kLFT5") /* ctrl-left */
	    return start_sequence ("\033[1;5D");
	  else if (name == "kRIT5") /* ctrl-right */
	    return start_sequence ("\033[1;5C");
	  else if (name == "kDC5") /* ctrl-del */
	    return start_sequence ("\033[3;5~");

	  /* alt-arrow keys */
	  else if (name == "kLFT3") /* alt-left */
	    return start_sequence ("\033[1;3D");
	  else if (name == "kRIT3") /* alt-right */
	    return start_sequence ("\033[1;3C");
	}
    }

  /* Handle the CTRL-L refresh for each window.  */
  if (ch == '\f')
    {
      tui_refresh_all_win ();
      return ch;
    }

  if (ch == KEY_BACKSPACE)
    return '\b';

  if (current_ui->command_editing && key_is_start_sequence (ch))
    {
      int ch_pending;

      nodelay (w, TRUE);
      ch_pending = gdb_wgetch (w);
      nodelay (w, FALSE);

      /* If we have pending input following a start sequence, call the stdin
	 event handler again because ncurses may have already read and stored
	 the input into its internal buffer, meaning that we won't get an stdin
	 event for it.  If we don't compensate for this missed stdin event, key
	 sequences as Alt_F (^[f) will not behave promptly.

	 (We only compensates for the missed 2nd byte of a key sequence because
	 2-byte sequences are by far the most commonly used. ncurses may have
	 buffered a larger, 3+-byte key sequence though it remains to be seen
	 whether it is useful to compensate for all the bytes of such
	 sequences.)  */
      if (ch_pending != ERR)
	{
	  ungetch (ch_pending);
	  call_stdin_event_handler_again_p = 1;
	}
    }

  if (ch > 0xff)
    {
      /* Readline doesn't understand non-8-bit curses keys, filter
	 them out.  */
      return 0;
    }

  return ch;
}

/* Get a character from the command window.  This is called from the
   readline package.  */

static int
tui_getc (FILE *fp)
{
  try
    {
      return tui_getc_1 (fp);
    }
  catch (const gdb_exception_forced_quit &ex)
    {
      /* As noted below, it's not safe to let an exception escape
	 to newline, so, for this case, reset the quit flag for
	 later QUIT checking.  */
      set_force_quit_flag ();
      return 0;
    }
  catch (const gdb_exception &ex)
    {
      /* Just in case, don't ever let an exception escape to readline.
	 This shouldn't ever happen, but if it does, print the
	 exception instead of just crashing GDB.  */
      exception_print (gdb_stderr, ex);

      /* If we threw an exception, it's because we recognized the
	 character.  */
      return 0;
    }
}

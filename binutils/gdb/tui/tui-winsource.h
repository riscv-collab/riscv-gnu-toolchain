/* TUI display source/assembly window.

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

#ifndef TUI_TUI_WINSOURCE_H
#define TUI_TUI_WINSOURCE_H

#include "tui/tui-data.h"
#include "symtab.h"

enum tui_line_or_address_kind
{
  LOA_LINE,
  LOA_ADDRESS
};

/* Structure describing source line or line address.  */
struct tui_line_or_address
{
  enum tui_line_or_address_kind loa;
  union
    {
      int line_no;
      CORE_ADDR addr;
    } u;
};

/* Flags to tell what kind of breakpoint is at current line.  */
enum tui_bp_flag
{
  TUI_BP_ENABLED = 0x01,
  TUI_BP_DISABLED = 0x02,
  TUI_BP_HIT = 0x04,
  TUI_BP_CONDITIONAL = 0x08,
  TUI_BP_HARDWARE = 0x10
};

DEF_ENUM_FLAGS_TYPE (enum tui_bp_flag, tui_bp_flags);

/* Position of breakpoint markers in the exec info string.  */
#define TUI_BP_HIT_POS      0
#define TUI_BP_BREAK_POS    1
#define TUI_EXEC_POS        2
#define TUI_EXECINFO_SIZE   3

/* Elements in the Source/Disassembly Window.  */
struct tui_source_element
{
  tui_source_element ()
  {
    line_or_addr.loa = LOA_LINE;
    line_or_addr.u.line_no = 0;
  }

  DISABLE_COPY_AND_ASSIGN (tui_source_element);

  tui_source_element (tui_source_element &&other)
    : line (std::move (other.line)),
      line_or_addr (other.line_or_addr),
      is_exec_point (other.is_exec_point),
      break_mode (other.break_mode)
  {
  }

  std::string line;
  struct tui_line_or_address line_or_addr;
  bool is_exec_point = false;
  tui_bp_flags break_mode = 0;
};


/* The base class for all source-like windows, namely the source and
   disassembly windows.  */

struct tui_source_window_base : public tui_win_info
{
protected:
  tui_source_window_base ();
  ~tui_source_window_base ();

  DISABLE_COPY_AND_ASSIGN (tui_source_window_base);

  void do_scroll_horizontal (int num_to_scroll) override;

  /* Erase the content and display STRING.  */
  void do_erase_source_content (const char *string);

  void rerender () override;

  virtual bool set_contents (struct gdbarch *gdbarch,
			     const struct symtab_and_line &sal) = 0;

  /* Return the number of extra margin characters needed by this
     instance.  */
  virtual int extra_margin () const
  {
    return 0;
  }

  /* Display the line number in the window margin.  OFFSET indicates
     which line to display; it is 0-based, with 0 meaning the line at
     the top of the window.  */
  virtual void show_line_number (int offset) const
  {
  }

  /* Redraw the complete line of a source or disassembly window.  */
  void show_source_line (int lineno);

  /* Where to start generating content from.  */
  struct tui_line_or_address m_start_line_or_addr;

  /* Architecture associated with code at this location.  */
  struct gdbarch *m_gdbarch = nullptr;

  std::vector<tui_source_element> m_content;

  /* Length of longest line to be displayed.  */
  int m_max_length;

public:

  /* Refill the source window's source cache and update it.  If this
     is a disassembly window, then just update it.  */
  void refill ();

  /* Set the location of the execution point.  */
  void set_is_exec_point_at (struct tui_line_or_address l);

  void update_tab_width () override;

  virtual bool location_matches_p (struct bp_location *loc, int line_no) = 0;

  /* Fill in the left margin of the current window with execution indicator
     information, e.g. breakpoint indicators, and line numbers.  When
     REFRESH_P is true this function will call refresh_window to ensure
     updates are written to the screen, otherwise the refresh is skipped,
     which will leave the on screen contents out of date.  When passing
     false for REFRESH_P you should be planning to call refresh_window
     yourself.  */
  void update_exec_info (bool refresh_p = true);

  /* Update the window to display the given location.  Does nothing if
     the location is already displayed.  */
  virtual void maybe_update (frame_info_ptr fi, symtab_and_line sal) = 0;

  void update_source_window_as_is  (struct gdbarch *gdbarch,
				    const struct symtab_and_line &sal);
  void update_source_window (struct gdbarch *gdbarch,
			     const struct symtab_and_line &sal);

  /* Scan the source window and the breakpoints to update the
     break_mode information for each line.  Returns true if something
     changed and the execution window must be refreshed.  See
     tui_update_all_breakpoint_info for a description of
     BEING_DELETED.  */
  bool update_breakpoint_info (struct breakpoint *being_deleted,
			       bool current_only);

  /* Erase the source content.  */
  virtual void erase_source_content () = 0;

  void refresh_window () override;

  /* Return the start address and gdbarch.  */
  virtual void display_start_addr (struct gdbarch **gdbarch_p,
				   CORE_ADDR *addr_p) = 0;

  /* Function to ensure that the source or disassembly window
     reflects the input address.  Single window variant of
     update_source_windows_with_addr.  */
  void update_source_window_with_addr (struct gdbarch *, CORE_ADDR);

private:

  /* Used for horizontal scroll.  */
  int m_horizontal_offset = 0;

  /* Check that the current values of M_HORIZONTAL_OFFSET and M_PAD_OFFSET
     make sense given the current M_MAX_LENGTH (content width), WIDTH
     (window size), and window margins.  After calling this function
     M_HORIZONTAL_OFFSET and M_PAD_OFFSET might have been adjusted to
     reduce unnecessary whitespace on the right side of the window.

     If M_PAD_OFFSET is adjusted then this function returns true
     indicating that the pad contents need to be reloaded by calling
     show_source_content.  If M_PAD_OFFSET is not adjusted then this
     function returns false, the window contents might still need
     redrawing if M_HORIZONTAL_OFFSET was adjusted, but right now, this
     function is only called in contexts where the window is going to be
     redrawn anyway.  */
  bool validate_scroll_offsets ();

  /* Return the size of the left margin space, this is the space used to
     display things like breakpoint markers.  */
  int left_margin () const
  { return TUI_EXECINFO_SIZE + extra_margin (); }

  /* Return the width of the area that is available for window content.
     This is the window width minus the borders and the left margin, which
     is used for displaying things like breakpoint markers.  */
  int view_width () const
  { return width - left_margin () - box_size (); }

  void show_source_content ();

  /* Write STRING to the window M_PAD, but skip the first SKIP printable
     characters.  Any escape sequences within the first SKIP characters are
     still processed though.  This means if we have this string:

     "\033[31mABCDEFGHIJKLM\033[0m"

     and call this function with a skip value of 3, then we effectively
     write this string to M_PAD:

     "\033[31mDEFGHIJKLM\033[0m"

     the initial escape that sets the color will still be applied.  */
  void puts_to_pad_with_skip (const char *string, int skip);

  /* Called when the user "set style enabled" setting is changed.  */
  void style_changed ();

  /* A token used to register and unregister an observer.  */
  gdb::observers::token m_observable;

  /* Pad to hold some, or all, of the window contents.  Content is then
     copied from this pad to the screen as the user scrolls horizontally,
     this avoids the need to recalculate the screen contents each time the
     user does a horizontal scroll.  */
  std::unique_ptr<WINDOW, curses_deleter> m_pad;

  /* When M_PAD was allocated, this holds the width that was initially
     asked for.  If we ask for a very large pad then the allocation may
     fail, and we might instead allocate a narrower pad.  */
  int m_pad_requested_width = 0;

  /* If M_PAD is not as wide as the content (so less than M_MAX_LENGTH)
     then this value indicates the offset at which the pad contents begin.  */
  int m_pad_offset = 0;
};


/* A wrapper for a TUI window iterator that only iterates over source
   windows.  */

struct tui_source_window_iterator
{
public:

  typedef std::vector<tui_win_info *>::iterator inner_iterator;

  typedef tui_source_window_iterator self_type;
  typedef struct tui_source_window_base *value_type;
  typedef struct tui_source_window_base *&reference;
  typedef struct tui_source_window_base **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  explicit tui_source_window_iterator (const inner_iterator &it,
				       const inner_iterator &end)
    : m_iter (it),
      m_end (end)
  {
    advance ();
  }

  explicit tui_source_window_iterator (const inner_iterator &it)
    : m_iter (it)
  {
  }

  bool operator!= (const self_type &other) const
  {
    return m_iter != other.m_iter;
  }

  value_type operator* () const
  {
    return dynamic_cast<tui_source_window_base *> (*m_iter);
  }

  self_type &operator++ ()
  {
    ++m_iter;
    advance ();
    return *this;
  }

private:

  void advance ()
  {
    while (m_iter != m_end
	   && dynamic_cast<tui_source_window_base *> (*m_iter) == nullptr)
      ++m_iter;
  }

  inner_iterator m_iter;
  inner_iterator m_end;
};

/* A range adapter for source windows.  */

struct tui_source_windows
{
  /* Work around Wmaybe-uninitialized warning with g++ 11.0.0, see also
     PR gcc/96295.  Note that "tui_source_windows () = default" doesn't work
     around the warning.  */
  tui_source_windows () {}

  tui_source_window_iterator begin () const
  {
    return tui_source_window_iterator (tui_windows.begin (),
				       tui_windows.end ());
  }

  tui_source_window_iterator end () const
  {
    return tui_source_window_iterator (tui_windows.end ());
  }
};

/* Update the execution windows to show the active breakpoints.  This
   is called whenever a breakpoint is inserted, removed or has its
   state changed.  Normally BEING_DELETED is nullptr; if not nullptr,
   it indicates a breakpoint that is in the process of being deleted,
   and which should therefore be ignored by the update.  This is done
   because the relevant observer is notified before the breakpoint is
   removed from the list of breakpoints.  */
extern void tui_update_all_breakpoint_info (struct breakpoint *being_deleted);

/* Function to display the "main" routine.  */
extern void tui_display_main (void);
extern void tui_update_source_windows_with_addr (struct gdbarch *, CORE_ADDR);
extern void tui_update_source_windows_with_line (struct symtab_and_line sal);

/* Extract some source text from PTR.  Returns a string holding the
   desired text.  PTR is updated to point to the start of the next
   line.  If LENGTH is non-NULL, then the length of the line is stored
   there.  Escape sequences are not counted against the length.
   Actually an approximation is used -- each byte of a multi-byte
   sequence counts as a character here.  */

extern std::string tui_copy_source_line (const char **ptr,
					 int *length = nullptr);

/* Constant definitions. */
#define SCROLL_THRESHOLD 2	/* Threshold for lazy scroll.  */

#endif /* TUI_TUI_WINSOURCE_H */

/* I/O, string, cleanup, and other random utilities for GDB.
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

#ifndef UTILS_H
#define UTILS_H

#include "exceptions.h"
#include "gdbsupport/array-view.h"
#include "gdbsupport/scoped_restore.h"
#include <chrono>

struct completion_match_for_lcd;
class compiled_regex;

/* String utilities.  */

extern bool sevenbit_strings;

/* Modes of operation for strncmp_iw_with_mode.  */

enum class strncmp_iw_mode
{
/* Do a strcmp() type operation on STRING1 and STRING2, ignoring any
   differences in whitespace.  Returns 0 if they match, non-zero if
   they don't (slightly different than strcmp()'s range of return
   values).  */
  NORMAL,

  /* Like NORMAL, but also apply the strcmp_iw hack.  I.e.,
     string1=="FOO(PARAMS)" matches string2=="FOO".  */
  MATCH_PARAMS,
};

/* Helper for strcmp_iw and strncmp_iw.  Exported so that languages
   can implement both NORMAL and MATCH_PARAMS variants in a single
   function and defer part of the work to strncmp_iw_with_mode.

   LANGUAGE is used to implement some context-sensitive
   language-specific comparisons.  For example, for C++,
   "string1=operator()" should not match "string2=operator" even in
   MATCH_PARAMS mode.

   MATCH_FOR_LCD is passed down so that the function can mark parts of
   the symbol name as ignored for completion matching purposes (e.g.,
   to handle abi tags).  If IGNORE_TEMPLATE_PARAMS is true, all template
   parameter lists will be ignored when language is C++.  */

extern int strncmp_iw_with_mode
  (const char *string1, const char *string2, size_t string2_len,
   strncmp_iw_mode mode, enum language language,
   completion_match_for_lcd *match_for_lcd = NULL,
   bool ignore_template_params = false);

/* Do a strncmp() type operation on STRING1 and STRING2, ignoring any
   differences in whitespace.  STRING2_LEN is STRING2's length.
   Returns 0 if STRING1 matches STRING2_LEN characters of STRING2,
   non-zero otherwise (slightly different than strncmp()'s range of
   return values).  Note: passes language_minimal to
   strncmp_iw_with_mode, and should therefore be avoided if a more
   suitable language is available.  */
extern int strncmp_iw (const char *string1, const char *string2,
		       size_t string2_len);

/* Do a strcmp() type operation on STRING1 and STRING2, ignoring any
   differences in whitespace.  Returns 0 if they match, non-zero if
   they don't (slightly different than strcmp()'s range of return
   values).

   As an extra hack, string1=="FOO(ARGS)" matches string2=="FOO".
   This "feature" is useful when searching for matching C++ function
   names (such as if the user types 'break FOO', where FOO is a
   mangled C++ function).

   Note: passes language_minimal to strncmp_iw_with_mode, and should
   therefore be avoided if a more suitable language is available.  */
extern int strcmp_iw (const char *string1, const char *string2);

extern int strcmp_iw_ordered (const char *, const char *);

/* Reset the prompt_for_continue clock.  */
void reset_prompt_for_continue_wait_time (void);
/* Return the time spent in prompt_for_continue.  */
std::chrono::steady_clock::duration get_prompt_for_continue_wait_time ();

/* Parsing utilities.  */

extern int parse_pid_to_attach (const char *args);

extern int parse_escape (struct gdbarch *, const char **);


/* Cleanup utilities.  */

extern void init_page_info (void);

/* Temporarily set BATCH_FLAG and the associated unlimited terminal size.
   Restore when destroyed.  */

struct set_batch_flag_and_restore_page_info
{
public:

  set_batch_flag_and_restore_page_info ();
  ~set_batch_flag_and_restore_page_info ();

  DISABLE_COPY_AND_ASSIGN (set_batch_flag_and_restore_page_info);

private:

  /* Note that this doesn't use scoped_restore, because it's important
     to control the ordering of operations in the destruction, and it
     was simpler to avoid introducing a new ad hoc class.  */
  unsigned m_save_lines_per_page;
  unsigned m_save_chars_per_line;
  int m_save_batch_flag;
};


/* Path utilities.  */

extern int gdb_filename_fnmatch (const char *pattern, const char *string,
				 int flags);

extern void substitute_path_component (char **stringp, const char *from,
				       const char *to);

std::string ldirname (const char *filename);

extern int count_path_elements (const char *path);

extern const char *strip_leading_path_elements (const char *path, int n);

/* GDB output, ui_file utilities.  */

struct ui_file;

extern int query (const char *, ...) ATTRIBUTE_PRINTF (1, 2);
extern int nquery (const char *, ...) ATTRIBUTE_PRINTF (1, 2);
extern int yquery (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

extern void begin_line (void);

extern void wrap_here (int);

extern void reinitialize_more_filter (void);

/* Return the number of characters in a line.  */

extern int get_chars_per_line ();

extern bool pagination_enabled;

/* A flag indicating whether to timestamp debugging messages.  */
extern bool debug_timestamp;

extern struct ui_file **current_ui_gdb_stdout_ptr (void);
extern struct ui_file **current_ui_gdb_stdin_ptr (void);
extern struct ui_file **current_ui_gdb_stderr_ptr (void);
extern struct ui_file **current_ui_gdb_stdlog_ptr (void);

/* Flush STREAM.  */
extern void gdb_flush (struct ui_file *stream);

/* The current top level's ui_file streams.  */

/* Normal results */
#define gdb_stdout (*current_ui_gdb_stdout_ptr ())
/* Input stream */
#define gdb_stdin (*current_ui_gdb_stdin_ptr ())
/* Serious error notifications.  This bypasses the pager, if one is in
   use.  */
#define gdb_stderr (*current_ui_gdb_stderr_ptr ())
/* Log/debug/trace messages that bypasses the pager, if one is in
   use.  */
#define gdb_stdlog (*current_ui_gdb_stdlog_ptr ())

/* Truly global ui_file streams.  These are all defined in main.c.  */

/* Target output that should bypass the pager, if one is in use.  */
extern struct ui_file *gdb_stdtarg;
extern struct ui_file *gdb_stdtargerr;
extern struct ui_file *gdb_stdtargin;

/* Set the screen dimensions to WIDTH and HEIGHT.  */

extern void set_screen_width_and_height (int width, int height);

/* Generic stdio-like operations.  */

extern void gdb_puts (const char *, struct ui_file *);

extern void gdb_putc (int c, struct ui_file *);

extern void gdb_putc (int c);

extern void gdb_puts (const char *);

extern void puts_tabular (char *string, int width, int right);

/* Generic printf-like operations.  As an extension over plain
   printf, these support some GDB-specific format specifiers.
   Particularly useful here are the styling formatters: '%p[', '%p]'
   and '%ps'.  See ui_out::message for details.  */

extern void gdb_vprintf (const char *, va_list) ATTRIBUTE_PRINTF (1, 0);

extern void gdb_vprintf (struct ui_file *, const char *, va_list)
  ATTRIBUTE_PRINTF (2, 0);

extern void gdb_printf (struct ui_file *, const char *, ...)
  ATTRIBUTE_PRINTF (2, 3);

extern void gdb_printf (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

extern void printf_unfiltered (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

extern void print_spaces (int, struct ui_file *);

extern const char *n_spaces (int);

/* Return nonzero if filtered printing is initialized.  */
extern int filtered_printing_initialized (void);

/* Like gdb_printf, but styles the output according to STYLE,
   when appropriate.  */

extern void fprintf_styled (struct ui_file *stream,
			    const ui_file_style &style,
			    const char *fmt,
			    ...)
  ATTRIBUTE_PRINTF (3, 4);

/* Like gdb_puts, but styles the output according to STYLE, when
   appropriate.  */

extern void fputs_styled (const char *linebuffer,
			  const ui_file_style &style,
			  struct ui_file *stream);

/* Like fputs_styled, but uses highlight_style to highlight the
   parts of STR that match HIGHLIGHT.  */

extern void fputs_highlighted (const char *str, const compiled_regex &highlight,
			       struct ui_file *stream);

/* Convert CORE_ADDR to string in platform-specific manner.
   This is usually formatted similar to 0x%lx.  */
extern const char *paddress (struct gdbarch *gdbarch, CORE_ADDR addr);

/* Return a string representation in hexadecimal notation of ADDRESS,
   which is suitable for printing.  */

extern const char *print_core_address (struct gdbarch *gdbarch,
				       CORE_ADDR address);

extern CORE_ADDR string_to_core_addr (const char *my_string);

extern void fprintf_symbol (struct ui_file *, const char *,
			    enum language, int);

extern void perror_warning_with_name (const char *string);

/* Issue a warning formatted as '<filename>: <explanation>', where
   <filename> is FILENAME with filename styling applied.  As such, don't
   pass anything more than a filename in this string.  The <explanation>
   is a string returned from calling safe_strerror(SAVED_ERRNO).  */

extern void warning_filename_and_errno (const char *filename,
					int saved_errno);

/* Warnings and error messages.  */

extern void (*deprecated_error_begin_hook) (void);

/* Message to be printed before the warning message, when a warning occurs.  */

extern const char *warning_pre_print;

extern void demangler_vwarning (const char *file, int line,
			       const char *, va_list ap)
     ATTRIBUTE_PRINTF (3, 0);

extern void demangler_warning (const char *file, int line,
			      const char *, ...) ATTRIBUTE_PRINTF (3, 4);


/* Misc. utilities.  */

#ifdef HAVE_WAITPID
extern pid_t wait_to_die_with_timeout (pid_t pid, int *status, int timeout);
#endif

extern int myread (int, char *, int);

/* Resource limits used by getrlimit and setrlimit.  */

enum resource_limit_kind
  {
    LIMIT_CUR,
    LIMIT_MAX
  };

/* Check whether GDB will be able to dump core using the dump_core
   function.  Returns zero if GDB cannot or should not dump core.
   If LIMIT_KIND is LIMIT_CUR the user's soft limit will be respected.
   If LIMIT_KIND is LIMIT_MAX only the hard limit will be respected.  */

extern int can_dump_core (enum resource_limit_kind limit_kind);

/* Print a warning that we cannot dump core.  */

extern void warn_cant_dump_core (const char *reason);

/* Dump core trying to increase the core soft limit to hard limit
   first.  */

extern void dump_core (void);

/* Copy NBITS bits from SOURCE to DEST starting at the given bit
   offsets.  Use the bit order as specified by BITS_BIG_ENDIAN.
   Source and destination buffers must not overlap.  */

extern void copy_bitwise (gdb_byte *dest, ULONGEST dest_offset,
			  const gdb_byte *source, ULONGEST source_offset,
			  ULONGEST nbits, int bits_big_endian);

/* When readline decides that the terminal cannot auto-wrap lines, it reduces
   the width of the reported screen width by 1.  This variable indicates
   whether that's the case or not, allowing us to add it back where
   necessary.  See _rl_term_autowrap in readline/terminal.c.  */

extern int readline_hidden_cols;

/* Assign VAL to LVAL, and set CHANGED to true if the assignment changed
   LVAL.  */

template<typename T>
void
assign_set_if_changed (T &lval, const T &val, bool &changed)
{
  if (lval == val)
    return;

  lval = val;
  changed = true;
}

/* Assign VAL to LVAL, and return true if the assignment changed LVAL.  */

template<typename T>
bool
assign_return_if_changed (T &lval, const T &val)
{
  if (lval == val)
    return false;

  lval = val;
  return true;
}

/* In some cases GDB needs to try several different solutions to a problem,
   if any of the solutions work then as far as the user is concerned the
   problem is solved, and GDB should continue without warnings.  However,
   if none of the solutions work then GDB should emit any warnings that
   occurred while trying each possible solution.

   One example of this is locating separate debug info.  There are several
   different approaches for this; following the .gnu_debuglink, a build-id
   based lookup, or using debuginfod.  If any works, and debug info is
   located, then the user doesn't want to see warnings from the earlier
   approaches that were tried and failed.

   However, GDB should emit all the warnings using separate calls to
   warning -- this ensures that each warning is formatted on its own line,
   and that any styling is emitted correctly.

   This class helps with deferring warnings.  Warnings can be added to an
   instance of this class with the 'warn' function, and all warnings can be
   emitted with a single call to 'emit'.  */

struct deferred_warnings
{
  deferred_warnings ()
    : m_can_style (gdb_stderr->can_emit_style_escape ())
  {
  }

  /* Add a warning to the list of deferred warnings.  */
  void warn (const char *format, ...) ATTRIBUTE_PRINTF(2,3)
  {
    /* Generate the warning text into a string_file.  */
    string_file msg (m_can_style);

    va_list args;
    va_start (args, format);
    msg.vprintf (format, args);
    va_end (args);

    /* Move the text into the list of deferred warnings.  */
    m_warnings.emplace_back (std::move (msg));
  }

  /* Emit all warnings.  */
  void emit () const
  {
    for (const auto &w : m_warnings)
      warning ("%s", w.c_str ());
  }

private:

  /* True if gdb_stderr supports styling at the moment this object is
     constructed.  This is done just once so that objects of this type
     can be used off the main thread.  */
  bool m_can_style;

  /* The list of all deferred warnings.  */
  std::vector<string_file> m_warnings;
};

#endif /* UTILS_H */

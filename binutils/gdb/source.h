/* List lines of source files for GDB, the GNU debugger.
   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#ifndef SOURCE_H
#define SOURCE_H

#include "gdbsupport/scoped_fd.h"

struct symtab;

/* See openp function definition for their description.  */

enum openp_flag
{
  OPF_TRY_CWD_FIRST = 0x01,
  OPF_SEARCH_IN_PATH = 0x02,
  OPF_RETURN_REALPATH = 0x04,
};

DEF_ENUM_FLAGS_TYPE(openp_flag, openp_flags);

extern int openp (const char *, openp_flags, const char *, int,
		  gdb::unique_xmalloc_ptr<char> *);

extern int source_full_path_of (const char *, gdb::unique_xmalloc_ptr<char> *);

extern void mod_path (const char *, std::string &);

extern void add_path (const char *, char **, int);
extern void add_path (const char *, std::string &, int);

extern void directory_switch (const char *, int);

extern std::string source_path;

extern void init_source_path (void);

/* This function is capable of finding the absolute path to a
   source file, and opening it, provided you give it a FILENAME.  Both the
   DIRNAME and FULLNAME are only added suggestions on where to find the file.

   FILENAME should be the filename to open.
   DIRNAME is the compilation directory of a particular source file.
	   Only some debug formats provide this info.
   FULLNAME can be the last known absolute path to the file in question.
     Space for the path must have been malloc'd.  If a path substitution
     is applied we free the old value and set a new one.

   On Success
     A valid file descriptor is returned (the return value is positive).
     FULLNAME is set to the absolute path to the file just opened.
     The caller is responsible for freeing FULLNAME.

   On Failure
     An invalid file descriptor is returned.  The value of this file
     descriptor is a negative errno indicating the reason for the failure.
     FULLNAME is set to NULL.  */
extern scoped_fd find_and_open_source (const char *filename,
				       const char *dirname,
				       gdb::unique_xmalloc_ptr<char> *fullname);

/* A wrapper for find_and_open_source that returns the full name.  If
   the full name cannot be found, a full name is constructed based on
   the parameters, passing them through rewrite_source_path.  */

extern gdb::unique_xmalloc_ptr<char> find_source_or_rewrite
     (const char *filename, const char *dirname);

/* Open a source file given a symtab S.  Returns a file descriptor or
   negative errno indicating the reason for the failure.  */
extern scoped_fd open_source_file (struct symtab *s);

extern gdb::unique_xmalloc_ptr<char> rewrite_source_path (const char *path);

extern const char *symtab_to_fullname (struct symtab *s);

/* Returns filename without the compile directory part, basename or absolute
   filename.  It depends on 'set filename-display' value.  */
extern const char *symtab_to_filename_for_display (struct symtab *symtab);

/* Return the first line listed by print_source_lines.  Used by
   command interpreters to request listing from a previous point.  If
   0, then no source lines have yet been listed since the last time
   the current source line was changed.  */
extern int get_first_line_listed (void);

/* Return the default number of lines to print with commands like the
   cli "list".  The caller of print_source_lines must use this to
   calculate the end line and use it in the call to print_source_lines
   as it does not automatically use this value.  */
extern int get_lines_to_list (void);

/* Return the current source file for listing and next line to list.
   NOTE: The returned sal pc and end fields are not valid.  */
extern struct symtab_and_line get_current_source_symtab_and_line (void);

/* If the current source file for listing is not set, try and get a default.
   Usually called before get_current_source_symtab_and_line() is called.
   It may err out if a default cannot be determined.
   We must be cautious about where it is called, as it can recurse as the
   process of determining a new default may call the caller!
   Use get_current_source_symtab_and_line only to get whatever
   we have without erroring out or trying to get a default.  */
extern void set_default_source_symtab_and_line (void);

/* Return the current default file for listing and next line to list
   (the returned sal pc and end fields are not valid.)
   and set the current default to whatever is in SAL.
   NOTE: The returned sal pc and end fields are not valid.  */
extern symtab_and_line set_current_source_symtab_and_line
  (const symtab_and_line &sal);

/* Reset any information stored about a default file and line to print.  */
extern void clear_current_source_symtab_and_line (void);

/* Add a source path substitution rule.  */
extern void add_substitute_path_rule (const char *, const char *);

/* Flags passed as 4th argument to print_source_lines.  */
enum print_source_lines_flag
  {
    /* Do not print an error message.  */
    PRINT_SOURCE_LINES_NOERROR = (1 << 0),

    /* Print the filename in front of the source lines.  */
    PRINT_SOURCE_LINES_FILENAME = (1 << 1)
  };
DEF_ENUM_FLAGS_TYPE (enum print_source_lines_flag, print_source_lines_flags);

/* Show source lines from the file of symtab S, starting with line
   number LINE and stopping before line number STOPLINE.  If this is
   not the command line version, then the source is shown in the source
   window otherwise it is simply printed.  */
extern void print_source_lines (struct symtab *s, int line, int stopline,
				print_source_lines_flags flags);

/* Wrap up the logic to build a line number range for passing to
   print_source_lines when using get_lines_to_list.  An instance of this
   class can be built from a single line number and a direction (forward or
   backward) the range is then computed using get_lines_to_list.  */
class source_lines_range
{
public:
  /* When constructing the range from a single line number, does the line
     range extend forward, or backward.  */
  enum direction
  {
   FORWARD,
   BACKWARD
  };

  /* Construct a SOURCE_LINES_RANGE starting at STARTLINE and extending in
   direction DIR.  The number of lines is from GET_LINES_TO_LIST.  If the
   direction is backward then the start is actually (STARTLINE -
   GET_LINES_TO_LIST).  There is also logic in place to ensure the start
   is always 1 or more, and the end will be at most INT_MAX.  */
  explicit source_lines_range (int startline, direction dir = FORWARD);

  /* Construct a SOURCE_LINES_RANGE from STARTLINE to STOPLINE.  */
  explicit source_lines_range (int startline, int stopline)
    : m_startline (startline),
      m_stopline (stopline)
  { /* Nothing.  */ }

  /* Return the line to start listing from.  */
  int startline () const
  { return m_startline; }

  /* Return the line after the last line that should be listed.  */
  int stopline () const
  { return m_stopline; }

private:

  /* The start and end of the range.  */
  int m_startline;
  int m_stopline;
};

/* Get the number of the last line in the given symtab.  */
extern int last_symtab_line (struct symtab *s);

/* Check if the line LINE can be found in the symtab S, so that it can be
   printed.  */
extern bool can_print_line (struct symtab *s, int line);

/* Variation of previous print_source_lines that takes a range instead of a
   start and end line number.  */
extern void print_source_lines (struct symtab *s, source_lines_range r,
				print_source_lines_flags flags);

/* Forget what we learned about line positions in source files, and
   which directories contain them; must check again now since files
   may be found in a different directory now.  */
extern void forget_cached_source_info (void);

/* Find a source file default for the "list" command.  This should
   only be called when the user actually tries to use the default,
   since we produce an error if we can't find a reasonable default.
   Also, since this can cause symbols to be read, doing it before we
   need to would make things slower than necessary.  */
extern void select_source_symtab ();

#endif

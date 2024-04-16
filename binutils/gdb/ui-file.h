/* UI_FILE - a generic STDIO like output stream.
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

#ifndef UI_FILE_H
#define UI_FILE_H

#include <string>
#include "ui-style.h"

/* The abstract ui_file base class.  */

class ui_file
{
public:
  ui_file ();
  virtual ~ui_file () = 0;

  ui_file (ui_file &&other) = default;

  /* Public non-virtual API.  */

  void printf (const char *, ...) ATTRIBUTE_PRINTF (2, 3);

  /* Print a NUL-terminated string whose delimiter is QUOTER.  Note
     that these routines should only be called for printing things
     which are independent of the language of the program being
     debugged.

     This will normally escape backslashes and instances of QUOTER.
     If QUOTER is 0, it won't escape backslashes or any quoting
     character.  As a side effect, if you pass the backslash character
     as the QUOTER, this will escape backslashes as usual, but not any
     other quoting character.  */
  void putstr (const char *str, int quoter);

  /* Like putstr, but only print the first N characters of STR.  If
     ASYNC_SAFE is true, then the output is done via the
     write_async_safe method.  */
  void putstrn (const char *str, int n, int quoter, bool async_safe = false);

  void putc (int c);

  void vprintf (const char *, va_list) ATTRIBUTE_PRINTF (2, 0);

  /* Methods below are both public, and overridable by ui_file
     subclasses.  */

  virtual void write (const char *buf, long length_buf) = 0;

  /* This version of "write" is safe for use in signal handlers.  It's
     not guaranteed that all existing output will have been flushed
     first.  Implementations are also free to ignore some or all of
     the request.  puts_async is not provided as the async versions
     are rarely used, no point in having both for a rarely used
     interface.  */
  virtual void write_async_safe (const char *buf, long length_buf)
  { gdb_assert_not_reached ("write_async_safe"); }

  /* Some ui_files override this to provide a efficient implementation
     that avoids a strlen.  */
  virtual void puts (const char *str)
  { this->write (str, strlen (str)); }

  virtual long read (char *buf, long length_buf)
  { gdb_assert_not_reached ("can't read from this file type"); }

  virtual bool isatty ()
  { return false; }

  /* true indicates terminal output behaviour such as cli_styling.
     This default implementation indicates to do terminal output
     behaviour if the UI_FILE is a tty.  A derived class can override
     TERM_OUT to have cli_styling behaviour without being a tty.  */
  virtual bool term_out ()
  { return isatty (); }

  /* true if ANSI escapes can be used on STREAM.  */
  virtual bool can_emit_style_escape ()
  { return false; }

  virtual void flush ()
  {}

  /* If this object has an underlying file descriptor, then return it.
     Otherwise, return -1.  */
  virtual int fd () const
  { return -1; }

  /* Indicate that if the next sequence of characters overflows the
     line, a newline should be inserted here rather than when it hits
     the end.  If INDENT is non-zero, it is a number of spaces to be
     printed to indent the wrapped part on the next line.

     If the line is already overfull, we immediately print a newline and
     the indentation, and disable further wrapping.

     If we don't know the width of lines, but we know the page height,
     we must not wrap words, but should still keep track of newlines
     that were explicitly printed.

     This routine is guaranteed to force out any output which has been
     squirreled away in the wrap_buffer, so wrap_here (0) can be
     used to force out output from the wrap_buffer.  */
  virtual void wrap_here (int indent)
  {
  }

  /* Emit an ANSI style escape for STYLE.  */
  virtual void emit_style_escape (const ui_file_style &style);

  /* Rest the current output style to the empty style.  */
  virtual void reset_style ();

  /* Print STR, bypassing any paging that might be done by this
     ui_file.  Note that nearly no code should call this -- it's
     intended for use by gdb_printf, but nothing else.  */
  virtual void puts_unfiltered (const char *str)
  {
    this->puts (str);
  }

protected:

  /* The currently applied style.  */
  ui_file_style m_applied_style;

private:

  /* Helper function for putstr and putstrn.  Print the character C on
     this stream as part of the contents of a literal string whose
     delimiter is QUOTER.  */
  void printchar (int c, int quoter, bool async_safe);
};

typedef std::unique_ptr<ui_file> ui_file_up;

/* A ui_file that writes to nowhere.  */

class null_file : public ui_file
{
public:
  void write (const char *buf, long length_buf) override;
  void write_async_safe (const char *buf, long sizeof_buf) override;
  void puts (const char *str) override;
};

/* A preallocated null_file stream.  */
extern null_file null_stream;

extern int gdb_console_fputs (const char *, FILE *);

/* A std::string-based ui_file.  Can be used as a scratch buffer for
   collecting output.  */

class string_file : public ui_file
{
public:
  /* Construct a string_file to collect 'raw' output, i.e. without
     'terminal' behaviour such as cli_styling.  */
  string_file () : m_term_out (false) {};
  /* If TERM_OUT, construct a string_file with terminal output behaviour
     such as cli_styling)
     else collect 'raw' output like the previous constructor.  */
  explicit string_file (bool term_out) : m_term_out (term_out) {};
  ~string_file () override;

  string_file (string_file &&other) = default;

  /* Override ui_file methods.  */

  void write (const char *buf, long length_buf) override;

  long read (char *buf, long length_buf) override
  { gdb_assert_not_reached ("a string_file is not readable"); }

  bool term_out () override;
  bool can_emit_style_escape () override;

  /* string_file-specific public API.  */

  /* Accesses the std::string containing the entire output collected
     so far.  */
  const std::string &string () { return m_string; }

  /* Return an std::string containing the entire output collected so far.

     The internal buffer is cleared, such that it's ready to build a new
     string.  */
  std::string release ()
  {
    std::string ret = std::move (m_string);
    m_string.clear ();
    return ret;
  }

  /* Set the internal buffer contents to STR.  Any existing contents are
     discarded.  */
  string_file &operator= (std::string &&str)
  {
    m_string = std::move (str);
    return *this;
  }

  /* Provide a few convenience methods with the same API as the
     underlying std::string.  */
  const char *data () const { return m_string.data (); }
  const char *c_str () const { return m_string.c_str (); }
  size_t size () const { return m_string.size (); }
  bool empty () const { return m_string.empty (); }
  void clear () { return m_string.clear (); }

private:
  /* The internal buffer.  */
  std::string m_string;

  bool m_term_out;
};

/* A ui_file implementation that maps directly onto <stdio.h>'s FILE.
   A stdio_file can either own its underlying file, or not.  If it
   owns the file, then destroying the stdio_file closes the underlying
   file, otherwise it is left open.  */

class stdio_file : public ui_file
{
public:
  /* Create a ui_file from a previously opened FILE.  CLOSE_P
     indicates whether the underlying file should be closed when the
     stdio_file is destroyed.  */
  explicit stdio_file (FILE *file, bool close_p = false);

  /* Create an stdio_file that is not managing any file yet.  Call
     open to actually open something.  */
  stdio_file ();

  ~stdio_file () override;

  /* Open NAME in mode MODE, and own the resulting file.  Returns true
     on success, false otherwise.  If the stdio_file previously owned
     a file, it is closed.  */
  bool open (const char *name, const char *mode);

  void flush () override;

  void write (const char *buf, long length_buf) override;

  void write_async_safe (const char *buf, long length_buf) override;

  void puts (const char *) override;

  long read (char *buf, long length_buf) override;

  bool isatty () override;

  bool can_emit_style_escape () override;

  /* Return the underlying file descriptor.  */
  int fd () const override
  { return m_fd; }

private:
  /* Sets the internal stream to FILE, and saves the FILE's file
     descriptor in M_FD.  */
  void set_stream (FILE *file);

  /* The file.  */
  FILE *m_file;

  /* The associated file descriptor is extracted ahead of time for
     stdio_file::write_async_safe's benefit, in case fileno isn't
     async-safe.  */
  int m_fd;

  /* If true, M_FILE is closed on destruction.  */
  bool m_close_p;
};

typedef std::unique_ptr<stdio_file> stdio_file_up;

/* Like stdio_file, but specifically for stderr.

   This exists because there is no real line-buffering on Windows, see
   <http://msdn.microsoft.com/en-us/library/86cebhfs%28v=vs.71%29.aspx>
   so the stdout is either fully-buffered or non-buffered.  We can't
   make stdout non-buffered, because of two concerns:

    1. Non-buffering hurts performance.
    2. Non-buffering may change GDB's behavior when it is interacting
       with a front-end, such as Emacs.

   We leave stdout as fully buffered, but flush it first when
   something is written to stderr.

   Note that the 'write_async_safe' method is not overridden, because
   there's no way to flush a stream in an async-safe manner.
   Fortunately, it doesn't really matter, because:

    1. That method is only used for printing internal debug output
       from signal handlers.

    2. Windows hosts don't have a concept of async-safeness.  Signal
       handlers run in a separate thread, so they can call the regular
       non-async-safe output routines freely.
*/
class stderr_file : public stdio_file
{
public:
  explicit stderr_file (FILE *stream);

  /* Override the output routines to flush gdb_stdout before deferring
     to stdio_file for the actual outputting.  */
  void write (const char *buf, long length_buf) override;
  void puts (const char *linebuffer) override;
};

/* A ui_file implementation that maps onto two ui-file objects.  */

class tee_file : public ui_file
{
public:
  /* Create a file which writes to both ONE and TWO.  Ownership of
     both files is up to the user.  */
  tee_file (ui_file *one, ui_file *two);
  ~tee_file () override;

  void write (const char *buf, long length_buf) override;
  void write_async_safe (const char *buf, long length_buf) override;
  void puts (const char *) override;

  bool isatty () override;
  bool term_out () override;
  bool can_emit_style_escape () override;
  void flush () override;

  void emit_style_escape (const ui_file_style &style) override
  {
    m_one->emit_style_escape (style);
    m_two->emit_style_escape (style);
  }

  void reset_style () override
  {
    m_one->reset_style ();
    m_two->reset_style ();
  }

  void puts_unfiltered (const char *str) override
  {
    m_one->puts_unfiltered (str);
    m_two->puts_unfiltered (str);
  }

private:
  /* The two underlying ui_files.  */
  ui_file *m_one;
  ui_file *m_two;
};

/* A ui_file implementation that filters out terminal escape
   sequences.  */

class no_terminal_escape_file : public stdio_file
{
public:
  no_terminal_escape_file ()
  {
  }

  /* Like the stdio_file methods, but these filter out terminal escape
     sequences.  */
  void write (const char *buf, long length_buf) override;
  void puts (const char *linebuffer) override;

  void emit_style_escape (const ui_file_style &style) override
  {
  }

  void reset_style () override
  {
  }
};

/* A base class for ui_file types that wrap another ui_file.  */

class wrapped_file : public ui_file
{
public:

  bool isatty () override
  { return m_stream->isatty (); }

  bool term_out () override
  { return m_stream->term_out (); }

  bool can_emit_style_escape () override
  { return m_stream->can_emit_style_escape (); }

  void flush () override
  { m_stream->flush (); }

  void wrap_here (int indent) override
  { m_stream->wrap_here (indent); }

  void emit_style_escape (const ui_file_style &style) override
  { m_stream->emit_style_escape (style); }

  /* Rest the current output style to the empty style.  */
  void reset_style () override
  { m_stream->reset_style (); }

  int fd () const override
  { return m_stream->fd (); }

  void puts_unfiltered (const char *str) override
  { m_stream->puts_unfiltered (str); }

  void write_async_safe (const char *buf, long length_buf) override
  { return m_stream->write_async_safe (buf, length_buf); }

protected:

  /* Note that this class does not assume ownership of the stream.
     However, a subclass may choose to, by adding a 'delete' to its
     destructor.  */
  explicit wrapped_file (ui_file *stream)
    : m_stream (stream)
  {
  }

  /* The underlying stream.  */
  ui_file *m_stream;
};

/* A ui_file that optionally puts a timestamp at the start of each
   line of output.  */

class timestamped_file : public wrapped_file
{
public:
  explicit timestamped_file (ui_file *stream)
    : wrapped_file (stream)
  {
  }

  DISABLE_COPY_AND_ASSIGN (timestamped_file);

  void write (const char *buf, long len) override;

private:

  /* True if the next output should be timestamped.  */
  bool m_needs_timestamp = true;
};

#endif

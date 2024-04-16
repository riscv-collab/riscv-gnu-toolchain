/* Support for connecting Guile's stdio to GDB's.
   as well as r/w memory via ports.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "gdbsupport/gdb_select.h"
#include "ui.h"
#include "target.h"
#include "guile-internal.h"
#include <optional>

#ifdef HAVE_POLL
#if defined (HAVE_POLL_H)
#include <poll.h>
#elif defined (HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif
#endif

/* Whether we're using Guile < 2.2 and its clumsy port API.  */

#define USING_GUILE_BEFORE_2_2				\
  (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION == 0)


/* A ui-file for sending output to Guile.  */

class ioscm_file_port : public ui_file
{
public:
  /* Return a ui_file that writes to PORT.  */
  explicit ioscm_file_port (SCM port);

  void flush () override;
  void write (const char *buf, long length_buf) override;

private:
  SCM m_port;
};

/* Data for a memory port.  */

struct ioscm_memory_port
{
  /* Bounds of memory range this port is allowed to access: [start, end).
     This means that 0xff..ff is not accessible.  I can live with that.  */
  CORE_ADDR start, end;

  /* (end - start), recorded for convenience.  */
  ULONGEST size;

  /* Think of this as the lseek value maintained by the kernel.
     This value is always in the range [0, size].  */
  ULONGEST current;

#if USING_GUILE_BEFORE_2_2
  /* The size of the internal r/w buffers.
     Scheme ports aren't a straightforward mapping to memory r/w.
     Generally the user specifies how much to r/w and all access is
     unbuffered.  We don't try to provide equivalent access, but we allow
     the user to specify these values to help get something similar.  */
  unsigned read_buf_size, write_buf_size;
#endif
};

/* Copies of the original system input/output/error ports.
   These are recorded for debugging purposes.  */
static SCM orig_input_port_scm;
static SCM orig_output_port_scm;
static SCM orig_error_port_scm;

/* This is the stdio port descriptor, scm_ptob_descriptor.  */
#if USING_GUILE_BEFORE_2_2
static scm_t_bits stdio_port_desc;
#else
static scm_t_port_type *stdio_port_desc;
#endif

/* Note: scm_make_port_type takes a char * instead of a const char *.  */
static /*const*/ char stdio_port_desc_name[] = "gdb:stdio-port";

/* Names of each gdb port.  */
static const char input_port_name[] = "gdb:stdin";
static const char output_port_name[] = "gdb:stdout";
static const char error_port_name[] = "gdb:stderr";

/* This is the actual port used from Guile.
   We don't expose these to the user though, to ensure they're not
   overwritten.  */
static SCM input_port_scm;
static SCM output_port_scm;
static SCM error_port_scm;

/* Internal enum for specifying output port.  */
enum oport { GDB_STDOUT, GDB_STDERR };

/* This is the memory port descriptor, scm_ptob_descriptor.  */
#if USING_GUILE_BEFORE_2_2
static scm_t_bits memory_port_desc;
#else
static scm_t_port_type *memory_port_desc;
#endif

/* Note: scm_make_port_type takes a char * instead of a const char *.  */
static /*const*/ char memory_port_desc_name[] = "gdb:memory-port";

#if USING_GUILE_BEFORE_2_2

/* The default amount of memory to fetch for each read/write request.
   Scheme ports don't provide a way to specify the size of a read,
   which is important to us to minimize the number of inferior interactions,
   which over a remote link can be important.  To compensate we augment the
   port API with a new function that let's the user specify how much the next
   read request should fetch.  This is the initial value for each new port.  */
static const unsigned default_read_buf_size = 16;
static const unsigned default_write_buf_size = 16;

/* Arbitrarily limit memory port buffers to 1 byte to 4K.  */
static const unsigned min_memory_port_buf_size = 1;
static const unsigned max_memory_port_buf_size = 4096;

/* "out of range" error message for buf sizes.  */
static gdb::unique_xmalloc_ptr<char> out_of_range_buf_size;

#else

/* The maximum values to use for get_natural_buffer_sizes.  */
static const unsigned natural_buf_size = 16;

#endif

/* Keywords used by open-memory.  */
static SCM mode_keyword;
static SCM start_keyword;
static SCM size_keyword;

/* Helper to do the low level work of opening a port.  */

#if USING_GUILE_BEFORE_2_2

static SCM
ioscm_open_port (scm_t_bits port_type, long mode_bits, scm_t_bits stream)
{
  SCM port;

#if 0 /* TODO: Guile doesn't export this.  What to do?  */
  scm_i_scm_pthread_mutex_lock (&scm_i_port_table_mutex);
#endif

  port = scm_new_port_table_entry (port_type);

  SCM_SET_CELL_TYPE (port, port_type | mode_bits);
  SCM_SETSTREAM (port, stream);

#if 0 /* TODO: Guile doesn't export this.  What to do?  */
  scm_i_pthread_mutex_unlock (&scm_i_port_table_mutex);
#endif

  return port;
}

#else

static SCM
ioscm_open_port (scm_t_port_type *port_type, long mode_bits, scm_t_bits stream)
{
  return scm_c_make_port (port_type, mode_bits, stream);
}

#endif


/* Support for connecting Guile's stdio ports to GDB's stdio ports.  */

/* Print a string S, length SIZE, but don't escape characters, except
   nul.  */

static void
fputsn_filtered (const char *s, size_t size, struct ui_file *stream)
{
  size_t i;

  for (i = 0; i < size; ++i)
    {
      if (s[i] == '\0')
	gdb_puts ("\\000", stream);
      else
	gdb_putc (s[i], stream);
    }
}

#if USING_GUILE_BEFORE_2_2

/* The scm_t_ptob_descriptor.input_waiting "method".
   Return a lower bound on the number of bytes available for input.  */

static int
ioscm_input_waiting (SCM port)
{
  int fdes = 0;

  if (! scm_is_eq (port, input_port_scm))
    return 0;

#ifdef HAVE_POLL
  {
    /* This is copied from libguile/fports.c.  */
    struct pollfd pollfd = { fdes, POLLIN, 0 };
    static int use_poll = -1;

    if (use_poll < 0)
      {
	/* This is copied from event-loop.c: poll cannot be used for stdin on
	   m68k-motorola-sysv.  */
	struct pollfd test_pollfd = { fdes, POLLIN, 0 };

	if (poll (&test_pollfd, 1, 0) == 1 && (test_pollfd.revents & POLLNVAL))
	  use_poll = 0;
	else
	  use_poll = 1;
      }

    if (use_poll)
      {
	/* Guile doesn't export SIGINT hooks like Python does.
	   For now pass EINTR to scm_syserror, that's what fports.c does.  */
	if (poll (&pollfd, 1, 0) < 0)
	  scm_syserror (FUNC_NAME);

	return pollfd.revents & POLLIN ? 1 : 0;
      }
  }
  /* Fall through.  */
#endif

  {
    struct timeval timeout;
    fd_set input_fds;
    int num_fds = fdes + 1;
    int num_found;

    memset (&timeout, 0, sizeof (timeout));
    FD_ZERO (&input_fds);
    FD_SET (fdes, &input_fds);

    num_found = interruptible_select (num_fds,
				      &input_fds, NULL, NULL,
				      &timeout);
    if (num_found < 0)
      {
	/* Guile doesn't export SIGINT hooks like Python does.
	   For now pass EINTR to scm_syserror, that's what fports.c does.  */
	scm_syserror (FUNC_NAME);
      }
    return num_found > 0 && FD_ISSET (fdes, &input_fds);
  }
}

/* The scm_t_ptob_descriptor.fill_input "method".  */

static int
ioscm_fill_input (SCM port)
{
  /* Borrowed from libguile/fports.c.  */
  long count;
  scm_t_port *pt = SCM_PTAB_ENTRY (port);

  /* If we're called on stdout,stderr, punt.  */
  if (! scm_is_eq (port, input_port_scm))
    return (scm_t_wchar) EOF; /* Set errno and return -1?  */

  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  count = gdb_stdin->read ((char *) pt->read_buf, pt->read_buf_size);
  if (count == -1)
    scm_syserror (FUNC_NAME);
  if (count == 0)
    return (scm_t_wchar) EOF;

  pt->read_pos = pt->read_buf;
  pt->read_end = pt->read_buf + count;
  return *pt->read_buf;
}

/* Write to gdb's stdout or stderr.  */

static void
ioscm_write (SCM port, const void *data, size_t size)
{

  /* If we're called on stdin, punt.  */
  if (scm_is_eq (port, input_port_scm))
    return;

  gdbscm_gdb_exception exc {};
  try
    {
      if (scm_is_eq (port, error_port_scm))
	fputsn_filtered ((const char *) data, size, gdb_stderr);
      else
	fputsn_filtered ((const char *) data, size, gdb_stdout);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
}

/* Flush gdb's stdout or stderr.  */

static void
ioscm_flush (SCM port)
{
  /* If we're called on stdin, punt.  */
  if (scm_is_eq (port, input_port_scm))
    return;

  if (scm_is_eq (port, error_port_scm))
    gdb_flush (gdb_stderr);
  else
    gdb_flush (gdb_stdout);
}

#else /* !USING_GUILE_BEFORE_2_2 */

/* Read up to COUNT bytes into bytevector DST at offset START.  Return the
   number of bytes read, zero for the end of file.  */

static size_t
ioscm_read_from_port (SCM port, SCM dst, size_t start, size_t count)
{
  long read;
  char *read_buf;

  /* If we're called on stdout,stderr, punt.  */
  if (! scm_is_eq (port, input_port_scm))
    return 0;

  gdb_flush (gdb_stdout);
  gdb_flush (gdb_stderr);

  read_buf = (char *) SCM_BYTEVECTOR_CONTENTS (dst) + start;
  read = gdb_stdin->read (read_buf, count);
  if (read == -1)
    scm_syserror (FUNC_NAME);

  return (size_t) read;
}

/* Write to gdb's stdout or stderr.  */

static size_t
ioscm_write (SCM port, SCM src, size_t start, size_t count)
{
  const char *data = (char *) SCM_BYTEVECTOR_CONTENTS (src) + start;

  /* If we're called on stdin, punt.  */
  if (scm_is_eq (port, input_port_scm))
    return 0;

  gdbscm_gdb_exception exc {};
  try
    {
      if (scm_is_eq (port, error_port_scm))
	fputsn_filtered ((const char *) data, count, gdb_stderr);
      else
	fputsn_filtered ((const char *) data, count, gdb_stdout);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (exc);

  return count;
}

#endif /* !USING_GUILE_BEFORE_2_2 */

/* Initialize the gdb stdio port type.

   N.B. isatty? will fail on these ports, it is only supported for file
   ports.  IWBN if we could "subclass" file ports.  */

static void
ioscm_init_gdb_stdio_port (void)
{
  stdio_port_desc = scm_make_port_type (stdio_port_desc_name,
#if USING_GUILE_BEFORE_2_2
					ioscm_fill_input,
#else
					ioscm_read_from_port,
#endif
					ioscm_write);

#if USING_GUILE_BEFORE_2_2
  scm_set_port_input_waiting (stdio_port_desc, ioscm_input_waiting);
  scm_set_port_flush (stdio_port_desc, ioscm_flush);
#else
  scm_set_port_read_wait_fd (stdio_port_desc, STDIN_FILENO);
#endif
}

#define GDB_STDIO_BUFFER_DEFAULT_SIZE 1024

#if USING_GUILE_BEFORE_2_2

/* Subroutine of ioscm_make_gdb_stdio_port to simplify it.
   Set up the buffers of port PORT.
   MODE_BITS are the mode bits of PORT.  */

static void
ioscm_init_stdio_buffers (SCM port, long mode_bits)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  int size = mode_bits & SCM_BUF0 ? 0 : GDB_STDIO_BUFFER_DEFAULT_SIZE;
  int writing = (mode_bits & SCM_WRTNG) != 0;

  /* This is heavily copied from scm_fport_buffer_add.  */

  if (!writing && size > 0)
    {
      pt->read_buf
	= (unsigned char *) scm_gc_malloc_pointerless (size, "port buffer");
      pt->read_pos = pt->read_end = pt->read_buf;
      pt->read_buf_size = size;
    }
  else
    {
      pt->read_pos = pt->read_buf = pt->read_end = &pt->shortbuf;
      pt->read_buf_size = 1;
    }

  if (writing && size > 0)
    {
      pt->write_buf
	= (unsigned char *) scm_gc_malloc_pointerless (size, "port buffer");
      pt->write_pos = pt->write_buf;
      pt->write_buf_size = size;
    }
  else
    {
      pt->write_buf = pt->write_pos = &pt->shortbuf;
      pt->write_buf_size = 1;
    }
  pt->write_end = pt->write_buf + pt->write_buf_size;
}

#else

static void
ioscm_init_stdio_buffers (SCM port, long mode_bits)
{
  if (mode_bits & SCM_BUF0)
    scm_setvbuf (port, scm_from_utf8_symbol ("none"), scm_from_size_t (0));
  else
    scm_setvbuf (port, scm_from_utf8_symbol ("block"),
		 scm_from_size_t (GDB_STDIO_BUFFER_DEFAULT_SIZE));
}

#endif

/* Create a gdb stdio port.  */

static SCM
ioscm_make_gdb_stdio_port (int fd)
{
  int is_a_tty = isatty (fd);
  const char *name;
  const char *mode_str;
  long mode_bits;
  SCM port;

  switch (fd)
    {
    case 0:
      name = input_port_name;
      mode_str = is_a_tty ? "r0" : "r";
      break;
    case 1:
      name = output_port_name;
      mode_str = is_a_tty ? "w0" : "w";
      break;
    case 2:
      name = error_port_name;
      mode_str = is_a_tty ? "w0" : "w";
      break;
    default:
      gdb_assert_not_reached ("bad stdio file descriptor");
    }

  mode_bits = scm_mode_bits ((char *) mode_str);
  port = ioscm_open_port (stdio_port_desc, mode_bits, 0);

  scm_set_port_filename_x (port, gdbscm_scm_from_c_string (name));

  ioscm_init_stdio_buffers (port, mode_bits);

  return port;
}

/* (stdio-port? object) -> boolean */

static SCM
gdbscm_stdio_port_p (SCM scm)
{
#if USING_GUILE_BEFORE_2_2
  /* This is copied from SCM_FPORTP.  */
  return scm_from_bool (!SCM_IMP (scm)
			&& (SCM_TYP16 (scm) == stdio_port_desc));
#else
  return scm_from_bool (SCM_PORTP (scm)
			&& (SCM_PORT_TYPE (scm) == stdio_port_desc));
#endif
}

/* GDB's ports are accessed via functions to keep them read-only.  */

/* (input-port) -> port */

static SCM
gdbscm_input_port (void)
{
  return input_port_scm;
}

/* (output-port) -> port */

static SCM
gdbscm_output_port (void)
{
  return output_port_scm;
}

/* (error-port) -> port */

static SCM
gdbscm_error_port (void)
{
  return error_port_scm;
}

/* Support for sending GDB I/O to Guile ports.  */

ioscm_file_port::ioscm_file_port (SCM port)
  : m_port (port)
{}

void
ioscm_file_port::flush ()
{
}

void
ioscm_file_port::write (const char *buffer, long length_buffer)
{
  scm_c_write (m_port, buffer, length_buffer);
}


/* Helper routine for with-{output,error}-to-port.  */

static SCM
ioscm_with_output_to_port_worker (SCM port, SCM thunk, enum oport oport,
				  const char *func_name)
{
  SCM result;

  SCM_ASSERT_TYPE (gdbscm_is_true (scm_output_port_p (port)), port,
		   SCM_ARG1, func_name, _("output port"));
  SCM_ASSERT_TYPE (gdbscm_is_true (scm_thunk_p (thunk)), thunk,
		   SCM_ARG2, func_name, _("thunk"));

  set_batch_flag_and_restore_page_info save_page_info;

  scoped_restore restore_async = make_scoped_restore (&current_ui->async, 0);

  ui_file_up port_file (new ioscm_file_port (port));

  scoped_restore save_file = make_scoped_restore (oport == GDB_STDERR
						  ? &gdb_stderr : &gdb_stdout);

  {
    std::optional<ui_out_redirect_pop> redirect_popper;
    if (oport == GDB_STDERR)
      gdb_stderr = port_file.get ();
    else
      {
	redirect_popper.emplace (current_uiout, port_file.get ());

	gdb_stdout = port_file.get ();
      }

    result = gdbscm_safe_call_0 (thunk, NULL);
  }

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (%with-gdb-output-to-port port thunk) -> object
   This function is experimental.
   IWBN to not include "gdb" in the name, but it would collide with a standard
   procedure, and it's common to import the gdb module without a prefix.
   There are ways around this, but they're more cumbersome.

   This has % in the name because it's experimental, and we want the
   user-visible version to come from module (gdb experimental).  */

static SCM
gdbscm_percent_with_gdb_output_to_port (SCM port, SCM thunk)
{
  return ioscm_with_output_to_port_worker (port, thunk, GDB_STDOUT, FUNC_NAME);
}

/* (%with-gdb-error-to-port port thunk) -> object
   This function is experimental.
   IWBN to not include "gdb" in the name, but it would collide with a standard
   procedure, and it's common to import the gdb module without a prefix.
   There are ways around this, but they're more cumbersome.

   This has % in the name because it's experimental, and we want the
   user-visible version to come from module (gdb experimental).  */

static SCM
gdbscm_percent_with_gdb_error_to_port (SCM port, SCM thunk)
{
  return ioscm_with_output_to_port_worker (port, thunk, GDB_STDERR, FUNC_NAME);
}

/* Support for r/w memory via ports.  */

/* Perform an "lseek" to OFFSET,WHENCE on memory port IOMEM.
   OFFSET must be in the range [0,size].
   The result is non-zero for success, zero for failure.  */

static int
ioscm_lseek_address (ioscm_memory_port *iomem, LONGEST offset, int whence)
{
  CORE_ADDR new_current;

  gdb_assert (iomem->current <= iomem->size);

  switch (whence)
    {
    case SEEK_CUR:
      /* Catch over/underflow.  */
      if ((offset < 0 && iomem->current + offset > iomem->current)
	  || (offset > 0 && iomem->current + offset < iomem->current))
	return 0;
      new_current = iomem->current + offset;
      break;
    case SEEK_SET:
      new_current = offset;
      break;
    case SEEK_END:
      if (offset == 0)
	{
	  new_current = iomem->size;
	  break;
	}
      /* TODO: Not supported yet.  */
      return 0;
    default:
      return 0;
    }

  if (new_current > iomem->size)
    return 0;
  iomem->current = new_current;
  return 1;
}

#if USING_GUILE_BEFORE_2_2

/* "fill_input" method for memory ports.  */

static int
gdbscm_memory_port_fill_input (SCM port)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  size_t to_read;

  /* "current" is the offset of the first byte we want to read.  */
  gdb_assert (iomem->current <= iomem->size);
  if (iomem->current == iomem->size)
    return EOF;

  /* Don't read outside the allowed memory range.  */
  to_read = pt->read_buf_size;
  if (to_read > iomem->size - iomem->current)
    to_read = iomem->size - iomem->current;

  if (target_read_memory (iomem->start + iomem->current, pt->read_buf,
			  to_read) != 0)
    gdbscm_memory_error (FUNC_NAME, _("error reading memory"), SCM_EOL);

  iomem->current += to_read;
  pt->read_pos = pt->read_buf;
  pt->read_end = pt->read_buf + to_read;
  return *pt->read_buf;
}

/* "end_input" method for memory ports.
   Clear the read buffer and adjust the file position for unread bytes.  */

static void
gdbscm_memory_port_end_input (SCM port, int offset)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  size_t remaining = pt->read_end - pt->read_pos;

  /* Note: Use of "int offset" is specified by Guile ports API.  */
  if ((offset < 0 && remaining + offset > remaining)
      || (offset > 0 && remaining + offset < remaining))
    {
      gdbscm_out_of_range_error (FUNC_NAME, 0, scm_from_int (offset),
				 _("overflow in offset calculation"));
    }
  offset += remaining;

  if (offset > 0)
    {
      pt->read_pos = pt->read_end;
      /* Throw error if unread-char used at beginning of file
	 then attempting to write.  Seems correct.  */
      if (!ioscm_lseek_address (iomem, -offset, SEEK_CUR))
	{
	  gdbscm_out_of_range_error (FUNC_NAME, 0, scm_from_int (offset),
				     _("bad offset"));
	}
    }

  pt->rw_active = SCM_PORT_NEITHER;
}

/* "flush" method for memory ports.  */

static void
gdbscm_memory_port_flush (SCM port)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  size_t to_write = pt->write_pos - pt->write_buf;

  if (to_write == 0)
    return;

  /* There's no way to indicate a short write, so if the request goes past
     the end of the port's memory range, flag an error.  */
  if (to_write > iomem->size - iomem->current)
    {
      gdbscm_out_of_range_error (FUNC_NAME, 0,
				 gdbscm_scm_from_ulongest (to_write),
				 _("writing beyond end of memory range"));
    }

  if (target_write_memory (iomem->start + iomem->current, pt->write_buf,
			   to_write) != 0)
    gdbscm_memory_error (FUNC_NAME, _("error writing memory"), SCM_EOL);

  iomem->current += to_write;
  pt->write_pos = pt->write_buf;
  pt->rw_active = SCM_PORT_NEITHER;
}

/* "seek" method for memory ports.  */

static scm_t_off
gdbscm_memory_port_seek (SCM port, scm_t_off offset, int whence)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  CORE_ADDR result;
  int rc;

  if (pt->rw_active == SCM_PORT_WRITE)
    {
      if (offset != 0 || whence != SEEK_CUR)
	{
	  gdbscm_memory_port_flush (port);
	  rc = ioscm_lseek_address (iomem, offset, whence);
	  result = iomem->current;
	}
      else
	{
	  /* Read current position without disturbing the buffer,
	     but flag an error if what's in the buffer goes outside the
	     allowed range.  */
	  CORE_ADDR current = iomem->current;
	  size_t delta = pt->write_pos - pt->write_buf;

	  if (current + delta < current
	      || current + delta > iomem->size)
	    rc = 0;
	  else
	    {
	      result = current + delta;
	      rc = 1;
	    }
	}
    }
  else if (pt->rw_active == SCM_PORT_READ)
    {
      if (offset != 0 || whence != SEEK_CUR)
	{
	  scm_end_input (port);
	  rc = ioscm_lseek_address (iomem, offset, whence);
	  result = iomem->current;
	}
      else
	{
	  /* Read current position without disturbing the buffer
	     (particularly the unread-char buffer).  */
	  CORE_ADDR current = iomem->current;
	  size_t remaining = pt->read_end - pt->read_pos;

	  if (current - remaining > current
	      || current - remaining < iomem->start)
	    rc = 0;
	  else
	    {
	      result = current - remaining;
	      rc = 1;
	    }

	  if (rc != 0 && pt->read_buf == pt->putback_buf)
	    {
	      size_t saved_remaining = pt->saved_read_end - pt->saved_read_pos;

	      if (result - saved_remaining > result
		  || result - saved_remaining < iomem->start)
		rc = 0;
	      else
		result -= saved_remaining;
	    }
	}
    }
  else /* SCM_PORT_NEITHER */
    {
      rc = ioscm_lseek_address (iomem, offset, whence);
      result = iomem->current;
    }

  if (rc == 0)
    {
      gdbscm_out_of_range_error (FUNC_NAME, 0,
				 gdbscm_scm_from_longest (offset),
				 _("bad seek"));
    }

  /* TODO: The Guile API doesn't support 32x64.  We can't fix that here,
     and there's no need to throw an error if the new address can't be
     represented in a scm_t_off.  But we could return something less
     clumsy.  */
  return result;
}

/* "write" method for memory ports.  */

static void
gdbscm_memory_port_write (SCM port, const void *void_data, size_t size)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  const gdb_byte *data = (const gdb_byte *) void_data;

  /* There's no way to indicate a short write, so if the request goes past
     the end of the port's memory range, flag an error.  */
  if (size > iomem->size - iomem->current)
    {
      gdbscm_out_of_range_error (FUNC_NAME, 0, gdbscm_scm_from_ulongest (size),
				 _("writing beyond end of memory range"));
    }

  if (pt->write_buf == &pt->shortbuf)
    {
      /* Unbuffered port.  */
      if (target_write_memory (iomem->start + iomem->current, data, size) != 0)
	gdbscm_memory_error (FUNC_NAME, _("error writing memory"), SCM_EOL);
      iomem->current += size;
      return;
    }

  /* Note: The edge case of what to do when the buffer exactly fills is
     debatable.  Guile flushes when the buffer exactly fills up, so we
     do too.  It's counter-intuitive to my mind, but in case there's a
     subtlety somewhere that depends on this, we do the same.  */

  {
    size_t space = pt->write_end - pt->write_pos;

    if (size < space)
      {
	/* Data fits in buffer, and does not fill it.  */
	memcpy (pt->write_pos, data, size);
	pt->write_pos += size;
      }
    else
      {
	memcpy (pt->write_pos, data, space);
	pt->write_pos = pt->write_end;
	gdbscm_memory_port_flush (port);
	{
	  const gdb_byte *ptr = data + space;
	  size_t remaining = size - space;

	  if (remaining >= pt->write_buf_size)
	    {
	      if (target_write_memory (iomem->start + iomem->current, ptr,
				       remaining) != 0)
		gdbscm_memory_error (FUNC_NAME, _("error writing memory"),
				     SCM_EOL);
	      iomem->current += remaining;
	    }
	  else
	    {
	      memcpy (pt->write_pos, ptr, remaining);
	      pt->write_pos += remaining;
	    }
	}
      }
  }
}

/* "close" method for memory ports.  */

static int
gdbscm_memory_port_close (SCM port)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);

  gdbscm_memory_port_flush (port);

  if (pt->read_buf == pt->putback_buf)
    pt->read_buf = pt->saved_read_buf;
  if (pt->read_buf != &pt->shortbuf)
    xfree (pt->read_buf);
  if (pt->write_buf != &pt->shortbuf)
    xfree (pt->write_buf);
  scm_gc_free (iomem, sizeof (*iomem), "memory port");

  return 0;
}

/* "free" method for memory ports.  */

static size_t
gdbscm_memory_port_free (SCM port)
{
  gdbscm_memory_port_close (port);

  return 0;
}

/* Re-initialize a memory port, updating its read/write buffer sizes.
   An exception is thrown if the port is unbuffered.
   TODO: Allow switching buffered/unbuffered.
   An exception is also thrown if data is still buffered, except in the case
   where the buffer size isn't changing (since that's just a nop).  */

static void
ioscm_reinit_memory_port (SCM port, size_t read_buf_size,
			  size_t write_buf_size, const char *func_name)
{
  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);

  gdb_assert (read_buf_size >= min_memory_port_buf_size
	      && read_buf_size <= max_memory_port_buf_size);
  gdb_assert (write_buf_size >= min_memory_port_buf_size
	      && write_buf_size <= max_memory_port_buf_size);

  /* First check if the port is unbuffered.  */

  if (pt->read_buf == &pt->shortbuf)
    {
      gdb_assert (pt->write_buf == &pt->shortbuf);
      scm_misc_error (func_name, _("port is unbuffered: ~a"),
		      scm_list_1 (port));
    }

  /* Next check if anything is buffered.  */

  if (read_buf_size != pt->read_buf_size
      && pt->read_end != pt->read_buf)
    {
      scm_misc_error (func_name, _("read buffer not empty: ~a"),
		      scm_list_1 (port));
    }

  if (write_buf_size != pt->write_buf_size
      && pt->write_pos != pt->write_buf)
    {
      scm_misc_error (func_name, _("write buffer not empty: ~a"),
		      scm_list_1 (port));
    }

  /* Now we can update the buffer sizes, but only if the size has changed.  */

  if (read_buf_size != pt->read_buf_size)
    {
      iomem->read_buf_size = read_buf_size;
      pt->read_buf_size = read_buf_size;
      xfree (pt->read_buf);
      pt->read_buf = (unsigned char *) xmalloc (pt->read_buf_size);
      pt->read_pos = pt->read_end = pt->read_buf;
    }

  if (write_buf_size != pt->write_buf_size)
    {
      iomem->write_buf_size = write_buf_size;
      pt->write_buf_size = write_buf_size;
      xfree (pt->write_buf);
      pt->write_buf = (unsigned char *) xmalloc (pt->write_buf_size);
      pt->write_pos = pt->write_buf;
      pt->write_end = pt->write_buf + pt->write_buf_size;
    }
}

#else /* !USING_GUILE_BEFORE_2_2 */

/* The semantics get weird if the buffer size is larger than the port range,
   so provide a better default buffer size.  */

static void
gdbscm_get_natural_buffer_sizes (SCM port, size_t *read_size,
				 size_t *write_size)
{
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);

  size_t size = natural_buf_size;
  if (iomem != NULL && iomem->size < size)
    size = iomem->size;
  *read_size = *write_size = size;
}

/* Read up to COUNT bytes into bytevector DST at offset START.  Return the
   number of bytes read, zero for the end of file.  */

static size_t
gdbscm_memory_port_read (SCM port, SCM dst, size_t start, size_t count)
{
  gdb_byte *read_buf;
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);

  /* "current" is the offset of the first byte we want to read.  */
  gdb_assert (iomem->current <= iomem->size);
  if (iomem->current == iomem->size)
    return 0;

  /* Don't read outside the allowed memory range.  */
  if (count > iomem->size - iomem->current)
    count = iomem->size - iomem->current;

  read_buf = (gdb_byte *) SCM_BYTEVECTOR_CONTENTS (dst) + start;
  if (target_read_memory (iomem->start + iomem->current, read_buf,
			  count) != 0)
    gdbscm_memory_error (FUNC_NAME, _("error reading memory"), SCM_EOL);

  iomem->current += count;
  return count;
}

static size_t
gdbscm_memory_port_write (SCM port, SCM src, size_t start, size_t count)
{
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  const gdb_byte *data =
    (const gdb_byte *) SCM_BYTEVECTOR_CONTENTS (src) + start;

  /* If the request goes past the end of the port's memory range, flag an
     error.  */
  if (count > iomem->size - iomem->current)
    gdbscm_out_of_range_error (FUNC_NAME, 0, scm_from_size_t (count),
			       _("writing beyond end of memory range"));

  if (target_write_memory (iomem->start + iomem->current, data,
			   count) != 0)
    gdbscm_memory_error (FUNC_NAME, _("error writing memory"),
			 SCM_EOL);

  iomem->current += count;

  return count;
}

static scm_t_off
gdbscm_memory_port_seek (SCM port, scm_t_off offset, int whence)
{
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  int rc;

  rc = ioscm_lseek_address (iomem, offset, whence);
  if (rc == 0)
    gdbscm_out_of_range_error (FUNC_NAME, 0,
			       gdbscm_scm_from_longest (offset),
			       _("bad seek"));

  /* TODO: The Guile API doesn't support 32x64.  We can't fix that here,
     and there's no need to throw an error if the new address can't be
     represented in a scm_t_off.  But we could return something less
     clumsy.  */
  return iomem->current;
}

static void
gdbscm_memory_port_close (SCM port)
{
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);
  scm_gc_free (iomem, sizeof (*iomem), "memory port");
  SCM_SETSTREAM (port, NULL);
}

#endif /* !USING_GUILE_BEFORE_2_2 */

/* "print" method for memory ports.  */

static int
gdbscm_memory_port_print (SCM exp, SCM port, scm_print_state *pstate)
{
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (exp);

  scm_puts ("#<", port);
  scm_print_port_mode (exp, port);
  /* scm_print_port_mode includes a trailing space.  */
  gdbscm_printf (port, "%s %s-%s", memory_port_desc_name,
		 hex_string (iomem->start), hex_string (iomem->end));
  scm_putc ('>', port);
  return 1;
}

/* Create the port type used for memory.  */

static void
ioscm_init_memory_port_type (void)
{
  memory_port_desc = scm_make_port_type (memory_port_desc_name,
#if USING_GUILE_BEFORE_2_2
					 gdbscm_memory_port_fill_input,
#else
					 gdbscm_memory_port_read,
#endif
					 gdbscm_memory_port_write);

#if USING_GUILE_BEFORE_2_2
  scm_set_port_end_input (memory_port_desc, gdbscm_memory_port_end_input);
  scm_set_port_flush (memory_port_desc, gdbscm_memory_port_flush);
  scm_set_port_free (memory_port_desc, gdbscm_memory_port_free);
#else
  scm_set_port_get_natural_buffer_sizes (memory_port_desc,
					 gdbscm_get_natural_buffer_sizes);
#endif
  scm_set_port_seek (memory_port_desc, gdbscm_memory_port_seek);
  scm_set_port_close (memory_port_desc, gdbscm_memory_port_close);
  scm_set_port_print (memory_port_desc, gdbscm_memory_port_print);
}

/* Helper for gdbscm_open_memory to parse the mode bits.
   An exception is thrown if MODE is invalid.  */

static long
ioscm_parse_mode_bits (const char *func_name, const char *mode)
{
  const char *p;
  long mode_bits;

  if (*mode != 'r' && *mode != 'w')
    {
      gdbscm_out_of_range_error (func_name, 0,
				 gdbscm_scm_from_c_string (mode),
				 _("bad mode string"));
    }
  for (p = mode + 1; *p != '\0'; ++p)
    {
      switch (*p)
	{
	case '0':
	case 'b':
	case '+':
	  break;
	default:
	  gdbscm_out_of_range_error (func_name, 0,
				     gdbscm_scm_from_c_string (mode),
				     _("bad mode string"));
	}
    }

  /* Kinda awkward to convert the mode from SCM -> string only to have Guile
     convert it back to SCM, but that's the API we have to work with.  */
  mode_bits = scm_mode_bits ((char *) mode);

  return mode_bits;
}

/* Return the memory object to be used as a "stream" associated with a memory
   port for the START--END range.  */

static ioscm_memory_port *
ioscm_init_memory_port_stream (CORE_ADDR start, CORE_ADDR end)
{
  ioscm_memory_port *iomem;

  gdb_assert (start <= end);

  iomem = (ioscm_memory_port *) scm_gc_malloc_pointerless (sizeof (*iomem),
							   "memory port");

  iomem->start = start;
  iomem->end = end;
  iomem->size = end - start;
  iomem->current = 0;

  return iomem;
}

#if USING_GUILE_BEFORE_2_2

/* Helper for gdbscm_open_memory to finish initializing the port.
   The port has address range [start,end).
   This means that address of 0xff..ff is not accessible.
   I can live with that.  */

static void
ioscm_init_memory_port_buffers (SCM port)
{
  ioscm_memory_port *iomem = (ioscm_memory_port *) SCM_STREAM (port);

  int buffered = (SCM_CELL_WORD_0 (port) & SCM_BUF0) == 0;
  if (buffered)
    {
      iomem->read_buf_size = default_read_buf_size;
      iomem->write_buf_size = default_write_buf_size;
    }
  else
    {
      iomem->read_buf_size = 1;
      iomem->write_buf_size = 1;
    }

  scm_t_port *pt = SCM_PTAB_ENTRY (port);
  /* Match the expectation of `binary-port?'.  */
  pt->encoding = NULL;
  pt->rw_random = 1;
  pt->read_buf_size = iomem->read_buf_size;
  pt->write_buf_size = iomem->write_buf_size;
  if (buffered)
    {
      pt->read_buf = (unsigned char *) xmalloc (pt->read_buf_size);
      pt->write_buf = (unsigned char *) xmalloc (pt->write_buf_size);
    }
  else
    {
      pt->read_buf = &pt->shortbuf;
      pt->write_buf = &pt->shortbuf;
    }
  pt->read_pos = pt->read_end = pt->read_buf;
  pt->write_pos = pt->write_buf;
  pt->write_end = pt->write_buf + pt->write_buf_size;
}

#endif

/* (open-memory [#:mode string] [#:start address] [#:size integer]) -> port
   Return a port that can be used for reading and writing memory.
   MODE is a string, and must be one of "r", "w", or "r+".
   "0" may be appended to MODE to mark the port as unbuffered.
   For compatibility "b" (binary) may also be appended, but we ignore it:
   memory ports are binary only.

   The chunk of memory that can be accessed can be bounded.
   If both START,SIZE are unspecified, all of memory can be accessed
   (except 0xff..ff).  If only START is specified, all of memory from that
   point on can be accessed (except 0xff..ff).  If only SIZE if specified,
   all memory in [0,SIZE) can be accessed.  If both are specified, all memory
   in [START,START+SIZE) can be accessed.

   Note: If it becomes useful enough we can later add #:end as an alternative
   to #:size.  For now it is left out.

   The result is a Scheme port, and its semantics are a bit odd for accessing
   memory (e.g., unget), but we don't try to hide this.  It's a port.

   N.B. Seeks on the port must be in the range [0,size].
   This is for similarity with bytevector ports, and so that one can seek
   to the first byte.  */

static SCM
gdbscm_open_memory (SCM rest)
{
  const SCM keywords[] = {
    mode_keyword, start_keyword, size_keyword, SCM_BOOL_F
  };
  char *mode = NULL;
  CORE_ADDR start = 0;
  CORE_ADDR end;
  int mode_arg_pos = -1, start_arg_pos = -1, size_arg_pos = -1;
  ULONGEST size;
  SCM port;
  long mode_bits;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "#sUU", rest,
			      &mode_arg_pos, &mode,
			      &start_arg_pos, &start,
			      &size_arg_pos, &size);

  scm_dynwind_begin ((scm_t_dynwind_flags) 0);

  if (mode == NULL)
    mode = xstrdup ("r");
  scm_dynwind_free (mode);

  if (size_arg_pos > 0)
    {
      /* For now be strict about start+size overflowing.  If it becomes
	 a nuisance we can relax things later.  */
      if (start + size < start)
	{
	  gdbscm_out_of_range_error (FUNC_NAME, 0,
				scm_list_2 (gdbscm_scm_from_ulongest (start),
					    gdbscm_scm_from_ulongest (size)),
				     _("start+size overflows"));
	}
      end = start + size;
    }
  else
    end = ~(CORE_ADDR) 0;

  mode_bits = ioscm_parse_mode_bits (FUNC_NAME, mode);

  /* Edge case: empty range -> unbuffered.
     There's no need to disallow empty ranges, but we need an unbuffered port
     to get the semantics right.  */
  if (size == 0)
    mode_bits |= SCM_BUF0;

  auto stream = ioscm_init_memory_port_stream (start, end);
  port = ioscm_open_port (memory_port_desc, mode_bits,
			  (scm_t_bits) stream);

#if USING_GUILE_BEFORE_2_2
  ioscm_init_memory_port_buffers (port);
#endif

  scm_dynwind_end ();

  /* TODO: Set the file name as "memory-start-end"?  */
  return port;
}

/* Return non-zero if OBJ is a memory port.  */

static int
gdbscm_is_memory_port (SCM obj)
{
#if USING_GUILE_BEFORE_2_2
  return !SCM_IMP (obj) && (SCM_TYP16 (obj) == memory_port_desc);
#else
  return SCM_PORTP (obj) && (SCM_PORT_TYPE (obj) == memory_port_desc);
#endif
}

/* (memory-port? obj) -> boolean */

static SCM
gdbscm_memory_port_p (SCM obj)
{
  return scm_from_bool (gdbscm_is_memory_port (obj));
}

/* (memory-port-range port) -> (start end) */

static SCM
gdbscm_memory_port_range (SCM port)
{
  ioscm_memory_port *iomem;

  SCM_ASSERT_TYPE (gdbscm_is_memory_port (port), port, SCM_ARG1, FUNC_NAME,
		   memory_port_desc_name);

  iomem = (ioscm_memory_port *) SCM_STREAM (port);
  return scm_list_2 (gdbscm_scm_from_ulongest (iomem->start),
		     gdbscm_scm_from_ulongest (iomem->end));
}

/* (memory-port-read-buffer-size port) -> integer */

static SCM
gdbscm_memory_port_read_buffer_size (SCM port)
{
#if USING_GUILE_BEFORE_2_2
  ioscm_memory_port *iomem;

  SCM_ASSERT_TYPE (gdbscm_is_memory_port (port), port, SCM_ARG1, FUNC_NAME,
		   memory_port_desc_name);

  iomem = (ioscm_memory_port *) SCM_STREAM (port);
  return scm_from_uint (iomem->read_buf_size);
#else
  return scm_from_uint (0);
#endif
}

/* (set-memory-port-read-buffer-size! port size) -> unspecified
   An exception is thrown if read data is still buffered or if the port
   is unbuffered.  */

static SCM
gdbscm_set_memory_port_read_buffer_size_x (SCM port, SCM size)
{
#if USING_GUILE_BEFORE_2_2
  ioscm_memory_port *iomem;

  SCM_ASSERT_TYPE (gdbscm_is_memory_port (port), port, SCM_ARG1, FUNC_NAME,
		   memory_port_desc_name);
  SCM_ASSERT_TYPE (scm_is_integer (size), size, SCM_ARG2, FUNC_NAME,
		   _("integer"));

  if (!scm_is_unsigned_integer (size, min_memory_port_buf_size,
				max_memory_port_buf_size))
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2, size,
				 out_of_range_buf_size.get ());
    }

  iomem = (ioscm_memory_port *) SCM_STREAM (port);
  ioscm_reinit_memory_port (port, scm_to_uint (size), iomem->write_buf_size,
			    FUNC_NAME);

  return SCM_UNSPECIFIED;
#else
  return scm_setvbuf (port, scm_from_utf8_symbol ("block"), size);
#endif
}

/* (memory-port-write-buffer-size port) -> integer */

static SCM
gdbscm_memory_port_write_buffer_size (SCM port)
{
#if USING_GUILE_BEFORE_2_2
  ioscm_memory_port *iomem;

  SCM_ASSERT_TYPE (gdbscm_is_memory_port (port), port, SCM_ARG1, FUNC_NAME,
		   memory_port_desc_name);

  iomem = (ioscm_memory_port *) SCM_STREAM (port);
  return scm_from_uint (iomem->write_buf_size);
#else
  return scm_from_uint (0);
#endif
}

/* (set-memory-port-write-buffer-size! port size) -> unspecified
   An exception is thrown if write data is still buffered or if the port
   is unbuffered.  */

static SCM
gdbscm_set_memory_port_write_buffer_size_x (SCM port, SCM size)
{
#if USING_GUILE_BEFORE_2_2
  ioscm_memory_port *iomem;

  SCM_ASSERT_TYPE (gdbscm_is_memory_port (port), port, SCM_ARG1, FUNC_NAME,
		   memory_port_desc_name);
  SCM_ASSERT_TYPE (scm_is_integer (size), size, SCM_ARG2, FUNC_NAME,
		   _("integer"));

  if (!scm_is_unsigned_integer (size, min_memory_port_buf_size,
				max_memory_port_buf_size))
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2, size,
				 out_of_range_buf_size.get ());
    }

  iomem = (ioscm_memory_port *) SCM_STREAM (port);
  ioscm_reinit_memory_port (port, iomem->read_buf_size, scm_to_uint (size),
			    FUNC_NAME);

  return SCM_UNSPECIFIED;
#else
  return scm_setvbuf (port, scm_from_utf8_symbol ("block"), size);
#endif
}

/* Initialize gdb ports.  */

static const scheme_function port_functions[] =
{
  { "input-port", 0, 0, 0, as_a_scm_t_subr (gdbscm_input_port),
    "\
Return gdb's input port." },

  { "output-port", 0, 0, 0, as_a_scm_t_subr (gdbscm_output_port),
    "\
Return gdb's output port." },

  { "error-port", 0, 0, 0, as_a_scm_t_subr (gdbscm_error_port),
    "\
Return gdb's error port." },

  { "stdio-port?", 1, 0, 0, as_a_scm_t_subr (gdbscm_stdio_port_p),
    "\
Return #t if the object is a gdb:stdio-port." },

  { "open-memory", 0, 0, 1, as_a_scm_t_subr (gdbscm_open_memory),
    "\
Return a port that can be used for reading/writing inferior memory.\n\
\n\
  Arguments: [#:mode string] [#:start address] [#:size integer]\n\
  Returns: A port object." },

  { "memory-port?", 1, 0, 0, as_a_scm_t_subr (gdbscm_memory_port_p),
    "\
Return #t if the object is a memory port." },

  { "memory-port-range", 1, 0, 0, as_a_scm_t_subr (gdbscm_memory_port_range),
    "\
Return the memory range of the port as (start end)." },

  { "memory-port-read-buffer-size", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_memory_port_read_buffer_size),
    "\
Return the size of the read buffer for the memory port." },

  { "set-memory-port-read-buffer-size!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_memory_port_read_buffer_size_x),
    "\
Set the size of the read buffer for the memory port.\n\
\n\
  Arguments: port integer\n\
  Returns: unspecified." },

  { "memory-port-write-buffer-size", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_memory_port_write_buffer_size),
    "\
Return the size of the write buffer for the memory port." },

  { "set-memory-port-write-buffer-size!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_memory_port_write_buffer_size_x),
    "\
Set the size of the write buffer for the memory port.\n\
\n\
  Arguments: port integer\n\
  Returns: unspecified." },

  END_FUNCTIONS
};

static const scheme_function private_port_functions[] =
{
#if 0 /* TODO */
  { "%with-gdb-input-from-port", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_percent_with_gdb_input_from_port),
    "\
Temporarily set GDB's input port to PORT and then invoke THUNK.\n\
\n\
  Arguments: port thunk\n\
  Returns: The result of calling THUNK.\n\
\n\
This procedure is experimental." },
#endif

  { "%with-gdb-output-to-port", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_percent_with_gdb_output_to_port),
    "\
Temporarily set GDB's output port to PORT and then invoke THUNK.\n\
\n\
  Arguments: port thunk\n\
  Returns: The result of calling THUNK.\n\
\n\
This procedure is experimental." },

  { "%with-gdb-error-to-port", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_percent_with_gdb_error_to_port),
    "\
Temporarily set GDB's error port to PORT and then invoke THUNK.\n\
\n\
  Arguments: port thunk\n\
  Returns: The result of calling THUNK.\n\
\n\
This procedure is experimental." },

  END_FUNCTIONS
};

void
gdbscm_initialize_ports (void)
{
  /* Save the original stdio ports for debugging purposes.  */

  orig_input_port_scm = scm_current_input_port ();
  orig_output_port_scm = scm_current_output_port ();
  orig_error_port_scm = scm_current_error_port ();

  /* Set up the stdio ports.  */

  ioscm_init_gdb_stdio_port ();
  input_port_scm = ioscm_make_gdb_stdio_port (0);
  output_port_scm = ioscm_make_gdb_stdio_port (1);
  error_port_scm = ioscm_make_gdb_stdio_port (2);

  /* Set up memory ports.  */

  ioscm_init_memory_port_type ();

  /* Install the accessor functions.  */

  gdbscm_define_functions (port_functions, 1);
  gdbscm_define_functions (private_port_functions, 0);

  /* Keyword args for open-memory.  */

  mode_keyword = scm_from_latin1_keyword ("mode");
  start_keyword = scm_from_latin1_keyword ("start");
  size_keyword = scm_from_latin1_keyword ("size");

#if USING_GUILE_BEFORE_2_2
  /* Error message text for "out of range" memory port buffer sizes.  */

  out_of_range_buf_size = xstrprintf ("size not between %u - %u",
				      min_memory_port_buf_size,
				      max_memory_port_buf_size);
#endif
}

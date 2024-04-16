/* Remote target callback routines.
   Copyright 1995-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

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

/* This file provides a standard way for targets to talk to the host OS
   level.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <errno.h>
#include <fcntl.h>
/* For PIPE_BUF.  */
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ansidecl.h"
/* For xmalloc.  */
#include "libiberty.h"

#include "sim/callback.h"

#ifndef PIPE_BUF
#define PIPE_BUF 512
#endif

extern CB_TARGET_DEFS_MAP cb_init_syscall_map[];
extern CB_TARGET_DEFS_MAP cb_init_errno_map[];
extern CB_TARGET_DEFS_MAP cb_init_signal_map[];
extern CB_TARGET_DEFS_MAP cb_init_open_map[];

/* Make sure the FD provided is ok.  If not, return non-zero
   and set errno. */

static int
fdbad (host_callback *p, int fd)
{
  if (fd < 0 || fd > MAX_CALLBACK_FDS || p->fd_buddy[fd] < 0)
    {
      p->last_errno = EBADF;
      return -1;
    }
  return 0;
}

static int
fdmap (host_callback *p, int fd)
{
  return p->fdmap[fd];
}

static int
os_close (host_callback *p, int fd)
{
  int result;
  int i, next;

  result = fdbad (p, fd);
  if (result)
    return result;
  /* If this file descripter has one or more buddies (originals /
     duplicates from a dup), just remove it from the circular list.  */
  for (i = fd; (next = p->fd_buddy[i]) != fd; )
    i = next;
  if (fd != i)
    p->fd_buddy[i] = p->fd_buddy[fd];
  else
    {
      if (p->ispipe[fd])
	{
	  int other = p->ispipe[fd];
	  int reader, writer;

	  if (other > 0)
	    {
	      /* Closing the read side.  */
	      reader = fd;
	      writer = other;
	    }
	  else
	    {
	      /* Closing the write side.  */
	      writer = fd;
	      reader = -other;
	    }

	  /* If there was data in the buffer, make a last "now empty"
	     call, then deallocate data.  */
	  if (p->pipe_buffer[writer].buffer != NULL)
	    {
	      (*p->pipe_empty) (p, reader, writer);
	      free (p->pipe_buffer[writer].buffer);
	      p->pipe_buffer[writer].buffer = NULL;
	    }

	  /* Clear pipe data for this side.  */
	  p->pipe_buffer[fd].size = 0;
	  p->ispipe[fd] = 0;

	  /* If this was the first close, mark the other side as the
	     only remaining side.  */
	  if (fd != abs (other))
	    p->ispipe[abs (other)] = -other;
	  p->fd_buddy[fd] = -1;
	  return 0;
	}

      result = close (fdmap (p, fd));
      p->last_errno = errno;
    }
  p->fd_buddy[fd] = -1;

  return result;
}


/* taken from gdb/util.c:notice_quit() - should be in a library */


#if defined(_MSC_VER)
static int
os_poll_quit (host_callback *p)
{
  /* NB - this will not compile! */
  int k = win32pollquit ();
  if (k == 1)
    return 1;
  else if (k == 2)
    return 1;
  return 0;
}
#else
#define os_poll_quit 0
#endif /* defined(_MSC_VER) */

static int
os_get_errno (host_callback *p)
{
  return cb_host_to_target_errno (p, p->last_errno);
}


static int
os_isatty (host_callback *p, int fd)
{
  int result;

  result = fdbad (p, fd);
  if (result)
    return result;

  result = isatty (fdmap (p, fd));
  p->last_errno = errno;
  return result;
}

static int64_t
os_lseek (host_callback *p, int fd, int64_t off, int way)
{
  int64_t result;

  result = fdbad (p, fd);
  if (result)
    return result;

  result = lseek (fdmap (p, fd), off, way);
  p->last_errno = errno;
  return result;
}

static int
os_open (host_callback *p, const char *name, int flags)
{
  int i;
  for (i = 0; i < MAX_CALLBACK_FDS; i++)
    {
      if (p->fd_buddy[i] < 0)
	{
	  int f = open (name, cb_target_to_host_open (p, flags), 0644);
	  if (f < 0)
	    {
	      p->last_errno = errno;
	      return f;
	    }
	  p->fd_buddy[i] = i;
	  p->fdmap[i] = f;
	  return i;
	}
    }
  p->last_errno = EMFILE;
  return -1;
}

static int
os_read (host_callback *p, int fd, char *buf, int len)
{
  int result;

  result = fdbad (p, fd);
  if (result)
    return result;
  if (p->ispipe[fd])
    {
      int writer = p->ispipe[fd];

      /* Can't read from the write-end.  */
      if (writer < 0)
	{
	  p->last_errno = EBADF;
	  return -1;
	}

      /* Nothing to read if nothing is written.  */
      if (p->pipe_buffer[writer].size == 0)
	return 0;

      /* Truncate read request size to buffer size minus what's already
         read.  */
      if (len > p->pipe_buffer[writer].size - p->pipe_buffer[fd].size)
	len = p->pipe_buffer[writer].size - p->pipe_buffer[fd].size;

      memcpy (buf, p->pipe_buffer[writer].buffer + p->pipe_buffer[fd].size,
	      len);

      /* Account for what we just read.  */
      p->pipe_buffer[fd].size += len;

      /* If we've read everything, empty and deallocate the buffer and
	 signal buffer-empty to client.  (This isn't expected to be a
	 hot path in the simulator, so we don't hold on to the buffer.)  */
      if (p->pipe_buffer[fd].size == p->pipe_buffer[writer].size)
	{
	  free (p->pipe_buffer[writer].buffer);
	  p->pipe_buffer[writer].buffer = NULL;
	  p->pipe_buffer[fd].size = 0;
	  p->pipe_buffer[writer].size = 0;
	  (*p->pipe_empty) (p, fd, writer);
	}

      return len;
    }

  result = read (fdmap (p, fd), buf, len);
  p->last_errno = errno;
  return result;
}

static int
os_read_stdin (host_callback *p, char *buf, int len)
{
  int result;

  result = read (0, buf, len);
  p->last_errno = errno;
  return result;
}

static int
os_write (host_callback *p, int fd, const char *buf, int len)
{
  int result;
  int real_fd;

  result = fdbad (p, fd);
  if (result)
    return result;

  if (p->ispipe[fd])
    {
      int reader = -p->ispipe[fd];

      /* Can't write to the read-end.  */
      if (reader < 0)
	{
	  p->last_errno = EBADF;
	  return -1;
	}

      /* Can't write to pipe with closed read end.
	 FIXME: We should send a SIGPIPE.  */
      if (reader == fd)
	{
	  p->last_errno = EPIPE;
	  return -1;
	}

      /* As a sanity-check, we bail out it the buffered contents is much
	 larger than the size of the buffer on the host.  We don't want
	 to run out of memory in the simulator due to a target program
	 bug if we can help it.  Unfortunately, regarding the value that
	 reaches the simulated program, it's no use returning *less*
	 than the requested amount, because cb_syscall loops calling
	 this function until the whole amount is done.  */
      if (p->pipe_buffer[fd].size + len > 10 * PIPE_BUF)
	{
	  p->last_errno = EFBIG;
	  return -1;
	}

      p->pipe_buffer[fd].buffer
	= xrealloc (p->pipe_buffer[fd].buffer, p->pipe_buffer[fd].size + len);
      memcpy (p->pipe_buffer[fd].buffer + p->pipe_buffer[fd].size,
	      buf, len);
      p->pipe_buffer[fd].size += len;

      (*p->pipe_nonempty) (p, reader, fd);
      return len;
    }

  real_fd = fdmap (p, fd);
  switch (real_fd)
    {
    default:
      result = write (real_fd, buf, len);
      p->last_errno = errno;
      break;
    case 1:
      result = p->write_stdout (p, buf, len);
      break;
    case 2:
      result = p->write_stderr (p, buf, len);
      break;
    }
  return result;
}

static int
os_write_stdout (host_callback *p ATTRIBUTE_UNUSED, const char *buf, int len)
{
  return fwrite (buf, 1, len, stdout);
}

static void
os_flush_stdout (host_callback *p ATTRIBUTE_UNUSED)
{
  fflush (stdout);
}

static int
os_write_stderr (host_callback *p ATTRIBUTE_UNUSED, const char *buf, int len)
{
  return fwrite (buf, 1, len, stderr);
}

static void
os_flush_stderr (host_callback *p ATTRIBUTE_UNUSED)
{
  fflush (stderr);
}

static int
os_rename (host_callback *p, const char *f1, const char *f2)
{
  int result;

  result = rename (f1, f2);
  p->last_errno = errno;
  return result;
}


static int
os_system (host_callback *p, const char *s)
{
  int result;

  result = system (s);
  p->last_errno = errno;
  return result;
}

static int64_t
os_time (host_callback *p)
{
  int64_t result;

  result = time (NULL);
  p->last_errno = errno;
  return result;
}


static int
os_unlink (host_callback *p, const char *f1)
{
  int result;

  result = unlink (f1);
  p->last_errno = errno;
  return result;
}

static int
os_stat (host_callback *p, const char *file, struct stat *buf)
{
  int result;

  /* ??? There is an issue of when to translate to the target layout.
     One could do that inside this function, or one could have the
     caller do it.  It's more flexible to let the caller do it, though
     I'm not sure the flexibility will ever be useful.  */
  result = stat (file, buf);
  p->last_errno = errno;
  return result;
}

static int
os_fstat (host_callback *p, int fd, struct stat *buf)
{
  int result;

  if (fdbad (p, fd))
    return -1;

  if (p->ispipe[fd])
    {
#if defined (HAVE_STRUCT_STAT_ST_ATIME) || defined (HAVE_STRUCT_STAT_ST_CTIME) || defined (HAVE_STRUCT_STAT_ST_MTIME)
      time_t t = (*p->time) (p);
#endif

      /* We have to fake the struct stat contents, since the pipe is
	 made up in the simulator.  */
      memset (buf, 0, sizeof (*buf));

#ifdef HAVE_STRUCT_STAT_ST_MODE
      buf->st_mode = S_IFIFO;
#endif

      /* If more accurate tracking than current-time is needed (for
	 example, on GNU/Linux we get accurate numbers), the p->time
	 callback (which may be something other than os_time) should
	 happen for each read and write, and we'd need to keep track of
	 atime, ctime and mtime.  */
#ifdef HAVE_STRUCT_STAT_ST_ATIME
      buf->st_atime = t;
#endif
#ifdef HAVE_STRUCT_STAT_ST_CTIME
      buf->st_ctime = t;
#endif
#ifdef HAVE_STRUCT_STAT_ST_MTIME
      buf->st_mtime = t;
#endif
      return 0;
    }

  /* ??? There is an issue of when to translate to the target layout.
     One could do that inside this function, or one could have the
     caller do it.  It's more flexible to let the caller do it, though
     I'm not sure the flexibility will ever be useful.  */
  result = fstat (fdmap (p, fd), buf);
  p->last_errno = errno;
  return result;
}

static int
os_lstat (host_callback *p, const char *file, struct stat *buf)
{
  int result;

  /* NOTE: hpn/2004-12-12: Same issue here as with os_fstat.  */
#ifdef HAVE_LSTAT
  result = lstat (file, buf);
#else
  result = stat (file, buf);
#endif
  p->last_errno = errno;
  return result;
}

static int
os_ftruncate (host_callback *p, int fd, int64_t len)
{
  int result;

  result = fdbad (p, fd);
  if (p->ispipe[fd])
    {
      p->last_errno = EINVAL;
      return -1;
    }
  if (result)
    return result;
#ifdef HAVE_FTRUNCATE
  result = ftruncate (fdmap (p, fd), len);
  p->last_errno = errno;
#else
  p->last_errno = EINVAL;
  result = -1;
#endif
  return result;
}

static int
os_truncate (host_callback *p, const char *file, int64_t len)
{
#ifdef HAVE_TRUNCATE
  int result;

  result = truncate (file, len);
  p->last_errno = errno;
  return result;
#else
  p->last_errno = EINVAL;
  return -1;
#endif
}

static int
os_getpid (host_callback *p)
{
  int result;

  result = getpid ();
  /* POSIX says getpid always succeeds.  */
  p->last_errno = 0;
  return result;
}

static int
os_kill (host_callback *p, int pid, int signum)
{
#ifdef HAVE_KILL
  int result;

  result = kill (pid, signum);
  p->last_errno = errno;
  return result;
#else
  p->last_errno = ENOSYS;
  return -1;
#endif
}

static int
os_pipe (host_callback *p, int *filedes)
{
  int i;

  /* We deliberately don't use fd 0.  It's probably stdin anyway.  */
  for (i = 1; i < MAX_CALLBACK_FDS; i++)
    {
      int j;

      if (p->fd_buddy[i] < 0)
	for (j = i + 1; j < MAX_CALLBACK_FDS; j++)
	  if (p->fd_buddy[j] < 0)
	    {
	      /* Found two free fd:s.  Set stat to allocated and mark
		 pipeness.  */
	      p->fd_buddy[i] = i;
	      p->fd_buddy[j] = j;
	      p->ispipe[i] = j;
	      p->ispipe[j] = -i;
	      filedes[0] = i;
	      filedes[1] = j;

	      /* Poison the FD map to make bugs apparent.  */
	      p->fdmap[i] = -1;
	      p->fdmap[j] = -1;
	      return 0;
	    }
    }

  p->last_errno = EMFILE;
  return -1;
}

/* Stub functions for pipe support.  They should always be overridden in
   targets using the pipe support, but that's up to the target.  */

/* Called when the simulator says that the pipe at (reader, writer) is
   now empty (so the writer should leave its waiting state).  */

static void
os_pipe_empty (host_callback *p, int reader, int writer)
{
}

/* Called when the simulator says the pipe at (reader, writer) is now
   non-empty (so the writer should wait).  */

static void
os_pipe_nonempty (host_callback *p, int reader, int writer)
{
}

static int
os_shutdown (host_callback *p)
{
  int i, next, j;
  for (i = 0; i < MAX_CALLBACK_FDS; i++)
    {
      int do_close = 1;

      /* Zero out all pipe state.  Don't call callbacks for non-empty
	 pipes; the target program has likely terminated at this point
	 or we're called at initialization time.  */
      p->ispipe[i] = 0;
      p->pipe_buffer[i].size = 0;
      p->pipe_buffer[i].buffer = NULL;

      next = p->fd_buddy[i];
      if (next < 0)
	continue;
      do
	{
	  j = next;
	  if (j == MAX_CALLBACK_FDS)
	    do_close = 0;
	  next = p->fd_buddy[j];
	  p->fd_buddy[j] = -1;
	  /* At the initial call of os_init, we got -1, 0, 0, 0, ...  */
	  if (next < 0)
	    {
	      p->fd_buddy[i] = -1;
	      do_close = 0;
	      break;
	    }
	}
      while (j != i);
      if (do_close)
	close (p->fdmap[i]);
    }
  return 1;
}

static int
os_init (host_callback *p)
{
  int i;

  os_shutdown (p);
  for (i = 0; i < 3; i++)
    {
      p->fdmap[i] = i;
      p->fd_buddy[i] = i - 1;
    }
  p->fd_buddy[0] = MAX_CALLBACK_FDS;
  p->fd_buddy[MAX_CALLBACK_FDS] = 2;

  p->syscall_map = cb_init_syscall_map;
  p->errno_map = cb_init_errno_map;
  p->signal_map = cb_init_signal_map;
  p->open_map = cb_init_open_map;

  return 1;
}

/* DEPRECATED */

/* VARARGS */
static void ATTRIBUTE_PRINTF (2, 3)
os_printf_filtered (host_callback *p ATTRIBUTE_UNUSED, const char *format, ...)
{
  va_list args;
  va_start (args, format);

  vfprintf (stdout, format, args);
  va_end (args);
}

/* VARARGS */
static void ATTRIBUTE_PRINTF (2, 0)
os_vprintf_filtered (host_callback *p ATTRIBUTE_UNUSED, const char *format, va_list args)
{
  vprintf (format, args);
}

/* VARARGS */
static void ATTRIBUTE_PRINTF (2, 0)
os_evprintf_filtered (host_callback *p ATTRIBUTE_UNUSED, const char *format, va_list args)
{
  vfprintf (stderr, format, args);
}

/* VARARGS */
static void ATTRIBUTE_PRINTF (2, 3) ATTRIBUTE_NORETURN
os_error (host_callback *p ATTRIBUTE_UNUSED, const char *format, ...)
{
  va_list args;
  va_start (args, format);

  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  exit (1);
}

host_callback default_callback =
{
  os_close,
  os_get_errno,
  os_isatty,
  os_lseek,
  os_open,
  os_read,
  os_read_stdin,
  os_rename,
  os_system,
  os_time,
  os_unlink,
  os_write,
  os_write_stdout,
  os_flush_stdout,
  os_write_stderr,
  os_flush_stderr,

  os_stat,
  os_fstat,
  os_lstat,

  os_ftruncate,
  os_truncate,

  os_getpid,
  os_kill,

  os_pipe,
  os_pipe_empty,
  os_pipe_nonempty,

  os_poll_quit,

  os_shutdown,
  os_init,

  os_printf_filtered,  /* deprecated */

  os_vprintf_filtered,
  os_evprintf_filtered,
  os_error,

  0, 		/* last errno */

  { 0, },	/* fdmap */
  { -1, },	/* fd_buddy */
  { 0, },	/* ispipe */
  { { 0, 0 }, }, /* pipe_buffer */

  0, /* syscall_map */
  0, /* errno_map */
  0, /* open_map */
  0, /* signal_map */
  0, /* stat_map */

  /* Defaults expected to be overridden at initialization, where needed.  */
  BFD_ENDIAN_UNKNOWN, /* target_endian */
  NULL, /* argv */
  NULL, /* envp */
  4, /* target_sizeof_int */

  HOST_CALLBACK_MAGIC,
};

/* Read in a file describing the target's system call values.
   E.g. maybe someone will want to use something other than newlib.
   This assumes that the basic system call recognition and value passing/
   returning is supported.  So maybe some coding/recompilation will be
   necessary, but not as much.

   If an error occurs, the existing mapping is not changed.  */

CB_RC
cb_read_target_syscall_maps (host_callback *cb, const char *file)
{
  CB_TARGET_DEFS_MAP *syscall_map, *errno_map, *open_map, *signal_map;
  const char *stat_map;
  FILE *f;

  if ((f = fopen (file, "r")) == NULL)
    return CB_RC_ACCESS;

  /* ... read in and parse file ... */

  fclose (f);
  return CB_RC_NO_MEM; /* FIXME:wip */

  /* Free storage allocated for any existing maps.  */
  if (cb->syscall_map)
    free (cb->syscall_map);
  if (cb->errno_map)
    free (cb->errno_map);
  if (cb->open_map)
    free (cb->open_map);
  if (cb->signal_map)
    free (cb->signal_map);
  if (cb->stat_map)
    free ((void *) cb->stat_map);

  cb->syscall_map = syscall_map;
  cb->errno_map = errno_map;
  cb->open_map = open_map;
  cb->signal_map = signal_map;
  cb->stat_map = stat_map;

  return CB_RC_OK;
}

/* General utility functions to search a map for a value.  */

static const CB_TARGET_DEFS_MAP *
cb_target_map_entry (const CB_TARGET_DEFS_MAP map[], int target_val)
{
  const CB_TARGET_DEFS_MAP *m;

  for (m = &map[0]; m->target_val != -1; ++m)
    if (m->target_val == target_val)
      return m;

  return NULL;
}

static const CB_TARGET_DEFS_MAP *
cb_host_map_entry (const CB_TARGET_DEFS_MAP map[], int host_val)
{
  const CB_TARGET_DEFS_MAP *m;

  for (m = &map[0]; m->host_val != -1; ++m)
    if (m->host_val == host_val)
      return m;

  return NULL;
}

/* Translate the target's version of a syscall number to the host's.
   This isn't actually the host's version, rather a canonical form.
   ??? Perhaps this should be renamed to ..._canon_syscall.  */

int
cb_target_to_host_syscall (host_callback *cb, int target_val)
{
  const CB_TARGET_DEFS_MAP *m =
    cb_target_map_entry (cb->syscall_map, target_val);

  return m ? m->host_val : -1;
}

/* FIXME: sort tables if large.
   Alternatively, an obvious improvement for errno conversion is
   to machine generate a function with a large switch().  */

/* Translate the host's version of errno to the target's.  */

int
cb_host_to_target_errno (host_callback *cb, int host_val)
{
  const CB_TARGET_DEFS_MAP *m = cb_host_map_entry (cb->errno_map, host_val);

  /* ??? Which error to return in this case is up for grabs.
     Note that some missing values may have standard alternatives.
     For now return 0 and require caller to deal with it.  */
  return m ? m->target_val : 0;
}

/* Given a set of target bitmasks for the open system call,
   return the host equivalent.
   Mapping open flag values is best done by looping so there's no need
   to machine generate this function.  */

int
cb_target_to_host_open (host_callback *cb, int target_val)
{
  int host_val = 0;
  CB_TARGET_DEFS_MAP *m;
  int o_rdonly = 0;
  int o_wronly = 0;
  int o_rdwr = 0;
  int o_binary = 0;
  int o_rdwrmask;

  /* O_RDONLY can be (and usually is) 0 which needs to be treated specially.  */
  for (m = &cb->open_map[0]; m->host_val != -1; ++m)
    {
      if (!strcmp (m->name, "O_RDONLY"))
	o_rdonly = m->target_val;
      else if (!strcmp (m->name, "O_WRONLY"))
	o_wronly = m->target_val;
      else if (!strcmp (m->name, "O_RDWR"))
	o_rdwr = m->target_val;
      else if (!strcmp (m->name, "O_BINARY"))
	o_binary = m->target_val;
    }
  o_rdwrmask = o_rdonly | o_wronly | o_rdwr;

  for (m = &cb->open_map[0]; m->host_val != -1; ++m)
    {
      if (m->target_val == o_rdonly || m->target_val == o_wronly
	  || m->target_val == o_rdwr)
	{
	  if ((target_val & o_rdwrmask) == m->target_val)
	    host_val |= m->host_val;
	  /* Handle the host/target differentiating between binary and
             text mode.  Only one case is of importance */
#ifdef O_BINARY
	  if (o_binary == 0)
	    host_val |= O_BINARY;
#endif
	}
      else
	{
	  if ((m->target_val & target_val) == m->target_val)
	    host_val |= m->host_val;
	}
    }

  return host_val;
}

/* Translate the target's version of a signal number to the host's.
   This isn't actually the host's version, rather a canonical form.
   ??? Perhaps this should be renamed to ..._canon_signal.  */

int
cb_target_to_host_signal (host_callback *cb, int target_val)
{
  const CB_TARGET_DEFS_MAP *m =
    cb_target_map_entry (cb->signal_map, target_val);

  return m ? m->host_val : -1;
}

/* Utility for e.g. cb_host_to_target_stat to store values in the target's
   stat struct.

   ??? The "val" must be as big as target word size.  */

void
cb_store_target_endian (host_callback *cb, char *p, int size, long val)
{
  if (cb->target_endian == BFD_ENDIAN_BIG)
    {
      p += size;
      while (size-- > 0)
	{
	  *--p = val;
	  val >>= 8;
	}
    }
  else
    {
      while (size-- > 0)
	{
	  *p++ = val;
	  val >>= 8;
	}
    }
}

/* Translate a host's stat struct into a target's.
   If HS is NULL, just compute the length of the buffer required,
   TS is ignored.

   The result is the size of the target's stat struct,
   or zero if an error occurred during the translation.  */

int
cb_host_to_target_stat (host_callback *cb, const struct stat *hs, void *ts)
{
  const char *m = cb->stat_map;
  char *p;

  if (hs == NULL)
    ts = NULL;
  p = ts;

  while (m)
    {
      char *q = strchr (m, ',');
      int size;

      /* FIXME: Use sscanf? */
      if (q == NULL)
	{
	  /* FIXME: print error message */
	  return 0;
	}
      size = atoi (q + 1);
      if (size == 0)
	{
	  /* FIXME: print error message */
	  return 0;
	}

      if (hs != NULL)
	{
	  if (0)
	    ;
	  /* Defined here to avoid emacs indigestion on a lone "else".  */
#undef ST_x
#define ST_x(FLD)					\
	  else if (strncmp (m, #FLD, q - m) == 0)	\
	    cb_store_target_endian (cb, p, size, hs->FLD)

#ifdef HAVE_STRUCT_STAT_ST_DEV
	  ST_x (st_dev);
#endif
#ifdef HAVE_STRUCT_STAT_ST_INO
	  ST_x (st_ino);
#endif
#ifdef HAVE_STRUCT_STAT_ST_MODE
	  ST_x (st_mode);
#endif
#ifdef HAVE_STRUCT_STAT_ST_NLINK
	  ST_x (st_nlink);
#endif
#ifdef HAVE_STRUCT_STAT_ST_UID
	  ST_x (st_uid);
#endif
#ifdef HAVE_STRUCT_STAT_ST_GID
	  ST_x (st_gid);
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
	  ST_x (st_rdev);
#endif
#ifdef HAVE_STRUCT_STAT_ST_SIZE
	  ST_x (st_size);
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	  ST_x (st_blksize);
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
	  ST_x (st_blocks);
#endif
#ifdef HAVE_STRUCT_STAT_ST_ATIME
	  ST_x (st_atime);
#endif
#ifdef HAVE_STRUCT_STAT_ST_MTIME
	  ST_x (st_mtime);
#endif
#ifdef HAVE_STRUCT_STAT_ST_CTIME
	  ST_x (st_ctime);
#endif
#undef ST_x
	  /* FIXME:wip */
	  else
	    /* Unsupported field, store 0.  */
	    cb_store_target_endian (cb, p, size, 0);
	}

      p += size;
      m = strchr (q, ':');
      if (m)
	++m;
    }

  return p - (char *) ts;
}

int
cb_is_stdin (host_callback *cb, int fd)
{
  return fdbad (cb, fd) ? 0 : fdmap (cb, fd) == 0;
}

int
cb_is_stdout (host_callback *cb, int fd)
{
  return fdbad (cb, fd) ? 0 : fdmap (cb, fd) == 1;
}

int
cb_is_stderr (host_callback *cb, int fd)
{
  return fdbad (cb, fd) ? 0 : fdmap (cb, fd) == 2;
}

const char *
cb_host_str_syscall (host_callback *cb, int host_val)
{
  const CB_TARGET_DEFS_MAP *m = cb_host_map_entry (cb->syscall_map, host_val);

  return m ? m->name : NULL;
}

const char *
cb_host_str_errno (host_callback *cb, int host_val)
{
  const CB_TARGET_DEFS_MAP *m = cb_host_map_entry (cb->errno_map, host_val);

  return m ? m->name : NULL;
}

const char *
cb_host_str_signal (host_callback *cb, int host_val)
{
  const CB_TARGET_DEFS_MAP *m = cb_host_map_entry (cb->signal_map, host_val);

  return m ? m->name : NULL;
}

const char *
cb_target_str_syscall (host_callback *cb, int target_val)
{
  const CB_TARGET_DEFS_MAP *m =
    cb_target_map_entry (cb->syscall_map, target_val);

  return m ? m->name : NULL;
}

const char *
cb_target_str_errno (host_callback *cb, int target_val)
{
  const CB_TARGET_DEFS_MAP *m =
    cb_target_map_entry (cb->errno_map, target_val);

  return m ? m->name : NULL;
}

const char *
cb_target_str_signal (host_callback *cb, int target_val)
{
  const CB_TARGET_DEFS_MAP *m =
    cb_target_map_entry (cb->signal_map, target_val);

  return m ? m->name : NULL;
}

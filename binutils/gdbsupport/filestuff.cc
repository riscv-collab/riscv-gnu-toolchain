/* Low-level file-handling.
   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "filestuff.h"
#include "gdb_vecs.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

#ifdef USE_WIN32API
#include <winsock2.h>
#include <windows.h>
#define HAVE_SOCKETS 1
#elif defined HAVE_SYS_SOCKET_H
#include <sys/socket.h>
/* Define HAVE_F_GETFD if we plan to use F_GETFD.  */
#define HAVE_F_GETFD F_GETFD
#define HAVE_SOCKETS 1
#endif

#ifdef HAVE_KINFO_GETFILE
#include <sys/user.h>
#include <libutil.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef O_NOINHERIT
#define O_NOINHERIT 0
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif



#ifndef HAVE_FDWALK

#include <dirent.h>

/* Replacement for fdwalk, if the system doesn't define it.  Walks all
   open file descriptors (though this implementation may walk closed
   ones as well, depending on the host platform's capabilities) and
   call FUNC with ARG.  If FUNC returns non-zero, stops immediately
   and returns the same value.  Otherwise, returns zero when
   finished.  */

static int
fdwalk (int (*func) (void *, int), void *arg)
{
  /* Checking __linux__ isn't great but it isn't clear what would be
     better.  There doesn't seem to be a good way to check for this in
     configure.  */
#ifdef __linux__
  DIR *dir;

  dir = opendir ("/proc/self/fd");
  if (dir != NULL)
    {
      struct dirent *entry;
      int result = 0;

      for (entry = readdir (dir); entry != NULL; entry = readdir (dir))
	{
	  long fd;
	  char *tail;

	  errno = 0;
	  fd = strtol (entry->d_name, &tail, 10);
	  if (*tail != '\0' || errno != 0)
	    continue;
	  if ((int) fd != fd)
	    {
	      /* What can we do here really?  */
	      continue;
	    }

	  if (fd == dirfd (dir))
	    continue;

	  result = func (arg, fd);
	  if (result != 0)
	    break;
	}

      closedir (dir);
      return result;
    }
  /* We may fall through to the next case.  */
#endif
#ifdef HAVE_KINFO_GETFILE
  int nfd;
  gdb::unique_xmalloc_ptr<struct kinfo_file[]> fdtbl
    (kinfo_getfile (getpid (), &nfd));
  if (fdtbl != NULL)
    {
      for (int i = 0; i < nfd; i++)
	{
	  if (fdtbl[i].kf_fd >= 0)
	    {
	      int result = func (arg, fdtbl[i].kf_fd);
	      if (result != 0)
		return result;
	    }
	}
      return 0;
    }
  /* We may fall through to the next case.  */
#endif

  {
    int max, fd;

#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_NOFILE)
    struct rlimit rlim;

    if (getrlimit (RLIMIT_NOFILE, &rlim) == 0 && rlim.rlim_max != RLIM_INFINITY)
      max = rlim.rlim_max;
    else
#endif
      {
#ifdef _SC_OPEN_MAX
	max = sysconf (_SC_OPEN_MAX);
#else
	/* Whoops.  */
	return 0;
#endif /* _SC_OPEN_MAX */
      }

    for (fd = 0; fd < max; ++fd)
      {
	struct stat sb;
	int result;

	/* Only call FUNC for open fds.  */
	if (fstat (fd, &sb) == -1)
	  continue;

	result = func (arg, fd);
	if (result != 0)
	  return result;
      }

    return 0;
  }
}

#endif /* HAVE_FDWALK */



/* A vector holding all the fds open when notice_open_fds was called.  We
   don't use a hashtab because we don't expect there to be many open fds.  */

static std::vector<int> open_fds;

/* An fdwalk callback function used by notice_open_fds.  It puts the
   given file descriptor into the vec.  */

static int
do_mark_open_fd (void *ignore, int fd)
{
  open_fds.push_back (fd);
  return 0;
}

/* See filestuff.h.  */

void
notice_open_fds (void)
{
  fdwalk (do_mark_open_fd, NULL);
}

/* See filestuff.h.  */

void
mark_fd_no_cloexec (int fd)
{
  do_mark_open_fd (NULL, fd);
}

/* See filestuff.h.  */

void
unmark_fd_no_cloexec (int fd)
{
  auto it = std::remove (open_fds.begin (), open_fds.end (), fd);

  if (it != open_fds.end ())
    open_fds.erase (it);
  else
    gdb_assert_not_reached ("fd not found in open_fds");
}

/* Helper function for close_most_fds that closes the file descriptor
   if appropriate.  */

static int
do_close (void *ignore, int fd)
{
  for (int val : open_fds)
    {
      if (fd == val)
	{
	  /* Keep this one open.  */
	  return 0;
	}
    }

  close (fd);
  return 0;
}

/* See filestuff.h.  */

void
close_most_fds (void)
{
  fdwalk (do_close, NULL);
}



/* This is a tri-state flag.  When zero it means we haven't yet tried
   O_CLOEXEC.  When positive it means that O_CLOEXEC works on this
   host.  When negative, it means that O_CLOEXEC doesn't work.  We
   track this state because, while gdb might have been compiled
   against a libc that supplies O_CLOEXEC, there is no guarantee that
   the kernel supports it.  */

static int trust_o_cloexec;

/* Mark FD as close-on-exec, ignoring errors.  Update
   TRUST_O_CLOEXEC.  */

static void
mark_cloexec (int fd)
{
#ifdef HAVE_F_GETFD
  int old = fcntl (fd, F_GETFD, 0);

  if (old != -1)
    {
      fcntl (fd, F_SETFD, old | FD_CLOEXEC);

      if (trust_o_cloexec == 0)
	{
	  if ((old & FD_CLOEXEC) != 0)
	    trust_o_cloexec = 1;
	  else
	    trust_o_cloexec = -1;
	}
    }
#endif /* HAVE_F_GETFD */
}

/* Depending on TRUST_O_CLOEXEC, mark FD as close-on-exec.  */

static void
maybe_mark_cloexec (int fd)
{
  if (trust_o_cloexec <= 0)
    mark_cloexec (fd);
}

#ifdef HAVE_SOCKETS

/* Like maybe_mark_cloexec, but for callers that use SOCK_CLOEXEC.  */

static void
socket_mark_cloexec (int fd)
{
  if (SOCK_CLOEXEC == 0 || trust_o_cloexec <= 0)
    mark_cloexec (fd);
}

#endif



/* See filestuff.h.  */

scoped_fd
gdb_open_cloexec (const char *filename, int flags, unsigned long mode)
{
  scoped_fd fd (open (filename, flags | O_CLOEXEC, mode));

  if (fd.get () >= 0)
    maybe_mark_cloexec (fd.get ());

  return fd;
}

/* See filestuff.h.  */

gdb_file_up
gdb_fopen_cloexec (const char *filename, const char *opentype)
{
  FILE *result;
  /* Probe for "e" support once.  But, if we can tell the operating
     system doesn't know about close on exec mode "e" without probing,
     skip it.  E.g., the Windows runtime issues an "Invalid parameter
     passed to C runtime function" OutputDebugString warning for
     unknown modes.  Assume that if O_CLOEXEC is zero, then "e" isn't
     supported.  On MinGW, O_CLOEXEC is an alias of O_NOINHERIT, and
     "e" isn't supported.  */
  static int fopen_e_ever_failed_einval =
    O_CLOEXEC == 0 || O_CLOEXEC == O_NOINHERIT;

  if (!fopen_e_ever_failed_einval)
    {
      char *copy;

      copy = (char *) alloca (strlen (opentype) + 2);
      strcpy (copy, opentype);
      /* This is a glibc extension but we try it unconditionally on
	 this path.  */
      strcat (copy, "e");
      result = fopen (filename, copy);

      if (result == NULL && errno == EINVAL)
	{
	  result = fopen (filename, opentype);
	  if (result != NULL)
	    fopen_e_ever_failed_einval = 1;
	}
    }
  else
    result = fopen (filename, opentype);

  if (result != NULL)
    maybe_mark_cloexec (fileno (result));

  return gdb_file_up (result);
}

#ifdef HAVE_SOCKETS
/* See filestuff.h.  */

int
gdb_socketpair_cloexec (int domain, int style, int protocol,
			int filedes[2])
{
#ifdef HAVE_SOCKETPAIR
  int result = socketpair (domain, style | SOCK_CLOEXEC, protocol, filedes);

  if (result != -1)
    {
      socket_mark_cloexec (filedes[0]);
      socket_mark_cloexec (filedes[1]);
    }

  return result;
#else
  gdb_assert_not_reached ("socketpair not available on this host");
#endif
}

/* See filestuff.h.  */

int
gdb_socket_cloexec (int domain, int style, int protocol)
{
  int result = socket (domain, style | SOCK_CLOEXEC, protocol);

  if (result != -1)
    socket_mark_cloexec (result);

  return result;
}
#endif

/* See filestuff.h.  */

int
gdb_pipe_cloexec (int filedes[2])
{
  int result;

#ifdef HAVE_PIPE2
  result = pipe2 (filedes, O_CLOEXEC);
  if (result != -1)
    {
      maybe_mark_cloexec (filedes[0]);
      maybe_mark_cloexec (filedes[1]);
    }
#else
#ifdef HAVE_PIPE
  result = pipe (filedes);
  if (result != -1)
    {
      mark_cloexec (filedes[0]);
      mark_cloexec (filedes[1]);
    }
#else /* HAVE_PIPE */
  gdb_assert_not_reached ("pipe not available on this host");
#endif /* HAVE_PIPE */
#endif /* HAVE_PIPE2 */

  return result;
}

/* See gdbsupport/filestuff.h.  */

bool
is_regular_file (const char *name, int *errno_ptr)
{
  struct stat st;
  const int status = stat (name, &st);

  /* Stat should never fail except when the file does not exist.
     If stat fails, analyze the source of error and return true
     unless the file does not exist, to avoid returning false results
     on obscure systems where stat does not work as expected.  */

  if (status != 0)
    {
      if (errno != ENOENT)
	return true;
      *errno_ptr = ENOENT;
      return false;
    }

  if (S_ISREG (st.st_mode))
    return true;

  if (S_ISDIR (st.st_mode))
    *errno_ptr = EISDIR;
  else
    *errno_ptr = EINVAL;
  return false;
}

/* See gdbsupport/filestuff.h.  */

bool
mkdir_recursive (const char *dir)
{
  auto holder = make_unique_xstrdup (dir);
  char * const start = holder.get ();
  char *component_start = start;
  char *component_end = start;

  while (1)
    {
      /* Find the beginning of the next component.  */
      while (*component_start == '/')
	component_start++;

      /* Are we done?  */
      if (*component_start == '\0')
	return true;

      /* Find the slash or null-terminator after this component.  */
      component_end = component_start;
      while (*component_end != '/' && *component_end != '\0')
	component_end++;

      /* Temporarily replace the slash with a null terminator, so we can create
	 the directory up to this component.  */
      char saved_char = *component_end;
      *component_end = '\0';

      /* If we get EEXIST and the existing path is a directory, then we're
	 happy.  If it exists, but it's a regular file and this is not the last
	 component, we'll fail at the next component.  If this is the last
	 component, the caller will fail with ENOTDIR when trying to
	 open/create a file under that path.  */
      if (mkdir (start, 0700) != 0)
	if (errno != EEXIST)
	  return false;

      /* Restore the overwritten char.  */
      *component_end = saved_char;
      component_start = component_end;
    }
}

/* See gdbsupport/filestuff.h.  */

std::optional<std::string>
read_text_file_to_string (const char *path)
{
  gdb_file_up file = gdb_fopen_cloexec (path, "r");
  if (file == nullptr)
    return {};

  std::string res;
  for (;;)
    {
      std::string::size_type start_size = res.size ();
      constexpr int chunk_size = 1024;

      /* Resize to accommodate CHUNK_SIZE bytes.  */
      res.resize (start_size + chunk_size);

      int n = fread (&res[start_size], 1, chunk_size, file.get ());
      if (n == chunk_size)
	continue;

      gdb_assert (n < chunk_size);

      /* Less than CHUNK means EOF or error.  If it's an error, return
	 no value.  */
      if (ferror (file.get ()))
	return {};

      /* Resize the string according to the data we read.  */
      res.resize (start_size + n);
      break;
    }

  return res;
}

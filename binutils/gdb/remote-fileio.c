/* Remote File-I/O communications

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

/* See the GDB User Guide for details of the GDB remote protocol.  */

#include "defs.h"
#include "gdbcmd.h"
#include "remote.h"
#include "gdbsupport/gdb_wait.h"
#include <sys/stat.h>
#include "remote-fileio.h"
#include "gdbsupport/event-loop.h"
#include "target.h"
#include "filenames.h"
#include "gdbsupport/filestuff.h"

#include <fcntl.h>
#include "gdbsupport/gdb_sys_time.h"
#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif
#include <signal.h>

static struct {
  int *fd_map;
  int fd_map_size;
} remote_fio_data;

#define FIO_FD_INVALID		-1
#define FIO_FD_CONSOLE_IN	-2
#define FIO_FD_CONSOLE_OUT	-3

static int remote_fio_system_call_allowed = 0;

static int
remote_fileio_init_fd_map (void)
{
  int i;

  if (!remote_fio_data.fd_map)
    {
      remote_fio_data.fd_map = XNEWVEC (int, 10);
      remote_fio_data.fd_map_size = 10;
      remote_fio_data.fd_map[0] = FIO_FD_CONSOLE_IN;
      remote_fio_data.fd_map[1] = FIO_FD_CONSOLE_OUT;
      remote_fio_data.fd_map[2] = FIO_FD_CONSOLE_OUT;
      for (i = 3; i < 10; ++i)
	remote_fio_data.fd_map[i] = FIO_FD_INVALID;
    }
  return 3;
}

static int
remote_fileio_resize_fd_map (void)
{
  int i = remote_fio_data.fd_map_size;

  if (!remote_fio_data.fd_map)
    return remote_fileio_init_fd_map ();
  remote_fio_data.fd_map_size += 10;
  remote_fio_data.fd_map =
    (int *) xrealloc (remote_fio_data.fd_map,
		      remote_fio_data.fd_map_size * sizeof (int));
  for (; i < remote_fio_data.fd_map_size; i++)
    remote_fio_data.fd_map[i] = FIO_FD_INVALID;
  return remote_fio_data.fd_map_size - 10;
}

static int
remote_fileio_next_free_fd (void)
{
  int i;

  for (i = 0; i < remote_fio_data.fd_map_size; ++i)
    if (remote_fio_data.fd_map[i] == FIO_FD_INVALID)
      return i;
  return remote_fileio_resize_fd_map ();
}

static int
remote_fileio_fd_to_targetfd (int fd)
{
  int target_fd = remote_fileio_next_free_fd ();

  remote_fio_data.fd_map[target_fd] = fd;
  return target_fd;
}

static int
remote_fileio_map_fd (int target_fd)
{
  remote_fileio_init_fd_map ();
  if (target_fd < 0 || target_fd >= remote_fio_data.fd_map_size)
    return FIO_FD_INVALID;
  return remote_fio_data.fd_map[target_fd];
}

static void
remote_fileio_close_target_fd (int target_fd)
{
  remote_fileio_init_fd_map ();
  if (target_fd >= 0 && target_fd < remote_fio_data.fd_map_size)
    remote_fio_data.fd_map[target_fd] = FIO_FD_INVALID;
}

static int
remote_fileio_oflags_to_host (long flags)
{
  int hflags = 0;

  if (flags & FILEIO_O_CREAT)
    hflags |= O_CREAT;
  if (flags & FILEIO_O_EXCL)
    hflags |= O_EXCL;
  if (flags & FILEIO_O_TRUNC)
    hflags |= O_TRUNC;
  if (flags & FILEIO_O_APPEND)
    hflags |= O_APPEND;
  if (flags & FILEIO_O_RDONLY)
    hflags |= O_RDONLY;
  if (flags & FILEIO_O_WRONLY)
    hflags |= O_WRONLY;
  if (flags & FILEIO_O_RDWR)
    hflags |= O_RDWR;
/* On systems supporting binary and text mode, always open files in
   binary mode.  */
#ifdef O_BINARY
  hflags |= O_BINARY;
#endif
  return hflags;
}

static mode_t
remote_fileio_mode_to_host (long mode, int open_call)
{
  mode_t hmode = 0;

  if (!open_call)
    {
      if (mode & FILEIO_S_IFREG)
	hmode |= S_IFREG;
      if (mode & FILEIO_S_IFDIR)
	hmode |= S_IFDIR;
      if (mode & FILEIO_S_IFCHR)
	hmode |= S_IFCHR;
    }
  if (mode & FILEIO_S_IRUSR)
    hmode |= S_IRUSR;
  if (mode & FILEIO_S_IWUSR)
    hmode |= S_IWUSR;
  if (mode & FILEIO_S_IXUSR)
    hmode |= S_IXUSR;
#ifdef S_IRGRP
  if (mode & FILEIO_S_IRGRP)
    hmode |= S_IRGRP;
#endif
#ifdef S_IWGRP
  if (mode & FILEIO_S_IWGRP)
    hmode |= S_IWGRP;
#endif
#ifdef S_IXGRP
  if (mode & FILEIO_S_IXGRP)
    hmode |= S_IXGRP;
#endif
  if (mode & FILEIO_S_IROTH)
    hmode |= S_IROTH;
#ifdef S_IWOTH
  if (mode & FILEIO_S_IWOTH)
    hmode |= S_IWOTH;
#endif
#ifdef S_IXOTH
  if (mode & FILEIO_S_IXOTH)
    hmode |= S_IXOTH;
#endif
  return hmode;
}

static int
remote_fileio_seek_flag_to_host (long num, int *flag)
{
  if (!flag)
    return 0;
  switch (num)
    {
      case FILEIO_SEEK_SET:
	*flag = SEEK_SET;
	break;
      case FILEIO_SEEK_CUR:
	*flag =  SEEK_CUR;
	break;
      case FILEIO_SEEK_END:
	*flag =  SEEK_END;
	break;
      default:
	return -1;
    }
  return 0;
}

static int
remote_fileio_extract_long (char **buf, LONGEST *retlong)
{
  char *c;
  int sign = 1;

  if (!buf || !*buf || !**buf || !retlong)
    return -1;
  c = strchr (*buf, ',');
  if (c)
    *c++ = '\0';
  else
    c = strchr (*buf, '\0');
  while (strchr ("+-", **buf))
    {
      if (**buf == '-')
	sign = -sign;
      ++*buf;
    }
  for (*retlong = 0; **buf; ++*buf)
    {
      *retlong <<= 4;
      if (**buf >= '0' && **buf <= '9')
	*retlong += **buf - '0';
      else if (**buf >= 'a' && **buf <= 'f')
	*retlong += **buf - 'a' + 10;
      else if (**buf >= 'A' && **buf <= 'F')
	*retlong += **buf - 'A' + 10;
      else
	return -1;
    }
  *retlong *= sign;
  *buf = c;
  return 0;
}

static int
remote_fileio_extract_int (char **buf, long *retint)
{
  int ret;
  LONGEST retlong;

  if (!retint)
    return -1;
  ret = remote_fileio_extract_long (buf, &retlong);
  if (!ret)
    *retint = (long) retlong;
  return ret;
}

static int
remote_fileio_extract_ptr_w_len (char **buf, CORE_ADDR *ptrval, int *length)
{
  char *c;
  LONGEST retlong;

  if (!buf || !*buf || !**buf || !ptrval || !length)
    return -1;
  c = strchr (*buf, '/');
  if (!c)
    return -1;
  *c++ = '\0';
  if (remote_fileio_extract_long (buf, &retlong))
    return -1;
  *ptrval = (CORE_ADDR) retlong;
  *buf = c;
  if (remote_fileio_extract_long (buf, &retlong))
    return -1;
  *length = (int) retlong;
  return 0;
}

static void
remote_fileio_to_fio_long (LONGEST num, fio_long_t fnum)
{
  host_to_bigendian (num, (char *) fnum, 8);
}

static void
remote_fileio_to_fio_timeval (struct timeval *tv, struct fio_timeval *ftv)
{
  host_to_fileio_time (tv->tv_sec, ftv->ftv_sec);
  remote_fileio_to_fio_long (tv->tv_usec, ftv->ftv_usec);
}

/* The quit handler originally installed.  */
static quit_handler_ftype *remote_fileio_o_quit_handler;

/* What to do on a QUIT call while handling a file I/O request.  We
   throw a quit exception, which is caught by remote_fileio_request
   and translated to an EINTR reply back to the target.  */

static void
remote_fileio_quit_handler (void)
{
  if (check_quit_flag ())
    quit ();
}

static void
remote_fileio_reply (remote_target *remote, int retcode, int error)
{
  char buf[32];
  int ctrl_c = check_quit_flag ();

  strcpy (buf, "F");
  if (retcode < 0)
    {
      strcat (buf, "-");
      retcode = -retcode;
    }
  sprintf (buf + strlen (buf), "%x", retcode);
  if (error || ctrl_c)
    {
      if (error && ctrl_c)
	error = FILEIO_EINTR;
      if (error < 0)
	{
	  strcat (buf, "-");
	  error = -error;
	}
      sprintf (buf + strlen (buf), ",%x", error);
      if (ctrl_c)
	strcat (buf, ",C");
    }
  quit_handler = remote_fileio_o_quit_handler;
  putpkt (remote, buf);
}

static void
remote_fileio_ioerror (remote_target *remote)
{
  remote_fileio_reply (remote, -1, FILEIO_EIO);
}

static void
remote_fileio_badfd (remote_target *remote)
{
  remote_fileio_reply (remote, -1, FILEIO_EBADF);
}

static void
remote_fileio_return_errno (remote_target *remote, int retcode)
{
  remote_fileio_reply (remote, retcode, retcode < 0
		       ? host_to_fileio_error (errno) : 0);
}

static void
remote_fileio_return_success (remote_target *remote, int retcode)
{
  remote_fileio_reply (remote, retcode, 0);
}

static void
remote_fileio_func_open (remote_target *remote, char *buf)
{
  CORE_ADDR ptrval;
  int length;
  long num;
  int flags, fd;
  mode_t mode;
  char *pathname;
  struct stat st;

  /* 1. Parameter: Ptr to pathname / length incl. trailing zero.  */
  if (remote_fileio_extract_ptr_w_len (&buf, &ptrval, &length))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  /* 2. Parameter: open flags */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  flags = remote_fileio_oflags_to_host (num);
  /* 3. Parameter: open mode */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  mode = remote_fileio_mode_to_host (num, 1);

  /* Request pathname.  */
  pathname = (char *) alloca (length);
  if (target_read_memory (ptrval, (gdb_byte *) pathname, length) != 0)
    {
      remote_fileio_ioerror (remote);
      return;
    }

  /* Check if pathname exists and is not a regular file or directory.  If so,
     return an appropriate error code.  Same for trying to open directories
     for writing.  */
  if (!stat (pathname, &st))
    {
      if (!S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode))
	{
	  remote_fileio_reply (remote, -1, FILEIO_ENODEV);
	  return;
	}
      if (S_ISDIR (st.st_mode)
	  && ((flags & O_WRONLY) == O_WRONLY || (flags & O_RDWR) == O_RDWR))
	{
	  remote_fileio_reply (remote, -1, FILEIO_EISDIR);
	  return;
	}
    }

  fd = gdb_open_cloexec (pathname, flags, mode).release ();
  if (fd < 0)
    {
      remote_fileio_return_errno (remote, -1);
      return;
    }

  fd = remote_fileio_fd_to_targetfd (fd);
  remote_fileio_return_success (remote, fd);
}

static void
remote_fileio_func_close (remote_target *remote, char *buf)
{
  long num;
  int fd;

  /* Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  fd = remote_fileio_map_fd ((int) num);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd (remote);
      return;
    }

  if (fd != FIO_FD_CONSOLE_IN && fd != FIO_FD_CONSOLE_OUT && close (fd))
    remote_fileio_return_errno (remote, -1);
  remote_fileio_close_target_fd ((int) num);
  remote_fileio_return_success (remote, 0);
}

static void
remote_fileio_func_read (remote_target *remote, char *buf)
{
  long target_fd, num;
  LONGEST lnum;
  CORE_ADDR ptrval;
  int fd, ret;
  gdb_byte *buffer;
  size_t length;
  off_t old_offset, new_offset;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd (remote);
      return;
    }
  /* 2. Parameter: buffer pointer */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  ptrval = (CORE_ADDR) lnum;
  /* 3. Parameter: buffer length */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  length = (size_t) num;

  switch (fd)
    {
      case FIO_FD_CONSOLE_OUT:
	remote_fileio_badfd (remote);
	return;
      case FIO_FD_CONSOLE_IN:
	{
	  static char *remaining_buf = NULL;
	  static int remaining_length = 0;

	  buffer = (gdb_byte *) xmalloc (16384);
	  if (remaining_buf)
	    {
	      if (remaining_length > length)
		{
		  memcpy (buffer, remaining_buf, length);
		  memmove (remaining_buf, remaining_buf + length,
			   remaining_length - length);
		  remaining_length -= length;
		  ret = length;
		}
	      else
		{
		  memcpy (buffer, remaining_buf, remaining_length);
		  xfree (remaining_buf);
		  remaining_buf = NULL;
		  ret = remaining_length;
		}
	    }
	  else
	    {
	      /* Windows (at least XP and Server 2003) has difficulty
		 with large reads from consoles.  If a handle is
		 backed by a real console device, overly large reads
		 from the handle will fail and set errno == ENOMEM.
		 On a Windows Server 2003 system where I tested,
		 reading 26608 bytes from the console was OK, but
		 anything above 26609 bytes would fail.  The limit has
		 been observed to vary on different systems.  So, we
		 limit this read to something smaller than that - by a
		 safe margin, in case the limit depends on system
		 resources or version.  */
	      ret = gdb_stdtargin->read ((char *) buffer, 16383);
	      if (ret > 0 && (size_t)ret > length)
		{
		  remaining_buf = (char *) xmalloc (ret - length);
		  remaining_length = ret - length;
		  memcpy (remaining_buf, buffer + length, remaining_length);
		  ret = length;
		}
	    }
	}
	break;
      default:
	buffer = (gdb_byte *) xmalloc (length);
	/* POSIX defines EINTR behaviour of read in a weird way.  It's allowed
	   for read() to return -1 even if "some" bytes have been read.  It
	   has been corrected in SUSv2 but that doesn't help us much...
	   Therefore a complete solution must check how many bytes have been
	   read on EINTR to return a more reliable value to the target */
	old_offset = lseek (fd, 0, SEEK_CUR);
	ret = read (fd, buffer, length);
	if (ret < 0 && errno == EINTR)
	  {
	    new_offset = lseek (fd, 0, SEEK_CUR);
	    /* If some data has been read, return the number of bytes read.
	       The Ctrl-C flag is set in remote_fileio_reply() anyway.  */
	    if (old_offset != new_offset)
	      ret = new_offset - old_offset;
	  }
	break;
    }

  if (ret > 0)
    {
      errno = target_write_memory (ptrval, buffer, ret);
      if (errno != 0)
	ret = -1;
    }

  if (ret < 0)
    remote_fileio_return_errno (remote, -1);
  else
    remote_fileio_return_success (remote, ret);

  xfree (buffer);
}

static void
remote_fileio_func_write (remote_target *remote, char *buf)
{
  long target_fd, num;
  LONGEST lnum;
  CORE_ADDR ptrval;
  int fd, ret;
  gdb_byte *buffer;
  size_t length;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd (remote);
      return;
    }
  /* 2. Parameter: buffer pointer */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  ptrval = (CORE_ADDR) lnum;
  /* 3. Parameter: buffer length */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  length = (size_t) num;
    
  buffer = (gdb_byte *) xmalloc (length);
  if (target_read_memory (ptrval, buffer, length) != 0)
    {
      xfree (buffer);
      remote_fileio_ioerror (remote);
      return;
    }

  switch (fd)
    {
      case FIO_FD_CONSOLE_IN:
	remote_fileio_badfd (remote);
	xfree (buffer);
	return;
      case FIO_FD_CONSOLE_OUT:
	{
	  ui_file *file = target_fd == 1 ? gdb_stdtarg : gdb_stdtargerr;
	  file->write ((char *) buffer, length);
	  file->flush ();
	  ret = length;
	}
	break;
      default:
	ret = write (fd, buffer, length);
	if (ret < 0 && errno == EACCES)
	  errno = EBADF; /* Cygwin returns EACCESS when writing to a
			    R/O file.  */
	break;
    }

  if (ret < 0)
    remote_fileio_return_errno (remote, -1);
  else
    remote_fileio_return_success (remote, ret);

  xfree (buffer);
}

static void
remote_fileio_func_lseek (remote_target *remote, char *buf)
{
  long num;
  LONGEST lnum;
  int fd, flag;
  off_t offset, ret;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  fd = remote_fileio_map_fd ((int) num);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd (remote);
      return;
    }
  else if (fd == FIO_FD_CONSOLE_IN || fd == FIO_FD_CONSOLE_OUT)
    {
      remote_fileio_reply (remote, -1, FILEIO_ESPIPE);
      return;
    }

  /* 2. Parameter: offset */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  offset = (off_t) lnum;
  /* 3. Parameter: flag */
  if (remote_fileio_extract_int (&buf, &num))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  if (remote_fileio_seek_flag_to_host (num, &flag))
    {
      remote_fileio_reply (remote, -1, FILEIO_EINVAL);
      return;
    }
  
  ret = lseek (fd, offset, flag);

  if (ret == (off_t) -1)
    remote_fileio_return_errno (remote, -1);
  else
    remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_rename (remote_target *remote, char *buf)
{
  CORE_ADDR old_ptr, new_ptr;
  int old_len, new_len;
  char *oldpath, *newpath;
  int ret, of, nf;
  struct stat ost, nst;

  /* 1. Parameter: Ptr to oldpath / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &old_ptr, &old_len))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  
  /* 2. Parameter: Ptr to newpath / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &new_ptr, &new_len))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  
  /* Request oldpath using 'm' packet */
  oldpath = (char *) alloca (old_len);
  if (target_read_memory (old_ptr, (gdb_byte *) oldpath, old_len) != 0)
    {
      remote_fileio_ioerror (remote);
      return;
    }
  
  /* Request newpath using 'm' packet */
  newpath = (char *) alloca (new_len);
  if (target_read_memory (new_ptr, (gdb_byte *) newpath, new_len) != 0)
    {
      remote_fileio_ioerror (remote);
      return;
    }
  
  /* Only operate on regular files and directories.  */
  of = stat (oldpath, &ost);
  nf = stat (newpath, &nst);
  if ((!of && !S_ISREG (ost.st_mode) && !S_ISDIR (ost.st_mode))
      || (!nf && !S_ISREG (nst.st_mode) && !S_ISDIR (nst.st_mode)))
    {
      remote_fileio_reply (remote, -1, FILEIO_EACCES);
      return;
    }

  ret = rename (oldpath, newpath);

  if (ret == -1)
    {
      /* Special case: newpath is a non-empty directory.  Some systems
	 return ENOTEMPTY, some return EEXIST.  We coerce that to be
	 always EEXIST.  */
      if (errno == ENOTEMPTY)
	errno = EEXIST;
#ifdef __CYGWIN__
      /* Workaround some Cygwin problems with correct errnos.  */
      if (errno == EACCES)
	{
	  if (!of && !nf && S_ISDIR (nst.st_mode))
	    {
	      if (S_ISREG (ost.st_mode))
		errno = EISDIR;
	      else
		{
		  char oldfullpath[PATH_MAX];
		  char newfullpath[PATH_MAX];
		  int len;

		  cygwin_conv_path (CCP_WIN_A_TO_POSIX, oldpath, oldfullpath,
				    PATH_MAX);
		  cygwin_conv_path (CCP_WIN_A_TO_POSIX, newpath, newfullpath,
				    PATH_MAX);
		  len = strlen (oldfullpath);
		  if (IS_DIR_SEPARATOR (newfullpath[len])
		      && !filename_ncmp (oldfullpath, newfullpath, len))
		    errno = EINVAL;
		  else
		    errno = EEXIST;
		}
	    }
	}
#endif

      remote_fileio_return_errno (remote, -1);
    }
  else
    remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_unlink (remote_target *remote, char *buf)
{
  CORE_ADDR ptrval;
  int length;
  char *pathname;
  int ret;
  struct stat st;

  /* Parameter: Ptr to pathname / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &ptrval, &length))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  /* Request pathname using 'm' packet */
  pathname = (char *) alloca (length);
  if (target_read_memory (ptrval, (gdb_byte *) pathname, length) != 0)
    {
      remote_fileio_ioerror (remote);
      return;
    }

  /* Only operate on regular files (and directories, which allows to return
     the correct return code).  */
  if (!stat (pathname, &st) && !S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode))
    {
      remote_fileio_reply (remote, -1, FILEIO_ENODEV);
      return;
    }

  ret = unlink (pathname);

  if (ret == -1)
    remote_fileio_return_errno (remote, -1);
  else
    remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_stat (remote_target *remote, char *buf)
{
  CORE_ADDR statptr, nameptr;
  int ret, namelength;
  char *pathname;
  LONGEST lnum;
  struct stat st;
  struct fio_stat fst;

  /* 1. Parameter: Ptr to pathname / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &nameptr, &namelength))
    {
      remote_fileio_ioerror (remote);
      return;
    }

  /* 2. Parameter: Ptr to struct stat */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  statptr = (CORE_ADDR) lnum;
  
  /* Request pathname using 'm' packet */
  pathname = (char *) alloca (namelength);
  if (target_read_memory (nameptr, (gdb_byte *) pathname, namelength) != 0)
    {
      remote_fileio_ioerror (remote);
      return;
    }

  ret = stat (pathname, &st);

  if (ret == -1)
    {
      remote_fileio_return_errno (remote, -1);
      return;
    }
  /* Only operate on regular files and directories.  */
  if (!ret && !S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode))
    {
      remote_fileio_reply (remote, -1, FILEIO_EACCES);
      return;
    }
  if (statptr)
    {
      host_to_fileio_stat (&st, &fst);
      host_to_fileio_uint (0, fst.fst_dev);

      errno = target_write_memory (statptr, (gdb_byte *) &fst, sizeof fst);
      if (errno != 0)
	{
	  remote_fileio_return_errno (remote, -1);
	  return;
	}
    }
  remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_fstat (remote_target *remote, char *buf)
{
  CORE_ADDR ptrval;
  int fd, ret;
  long target_fd;
  LONGEST lnum;
  struct stat st;
  struct fio_stat fst;
  struct timeval tv;

  /* 1. Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  if (fd == FIO_FD_INVALID)
    {
      remote_fileio_badfd (remote);
      return;
    }
  /* 2. Parameter: Ptr to struct stat */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  ptrval = (CORE_ADDR) lnum;

  if (fd == FIO_FD_CONSOLE_IN || fd == FIO_FD_CONSOLE_OUT)
    {
      host_to_fileio_uint (1, fst.fst_dev);
      memset (&st, 0, sizeof (st));
      st.st_mode = S_IFCHR | (fd == FIO_FD_CONSOLE_IN ? S_IRUSR : S_IWUSR);
      st.st_nlink = 1;
#ifdef HAVE_GETUID
      st.st_uid = getuid ();
#endif
#ifdef HAVE_GETGID
      st.st_gid = getgid ();
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
      st.st_blksize = 512;
#endif
#if HAVE_STRUCT_STAT_ST_BLOCKS
      st.st_blocks = 0;
#endif
      if (!gettimeofday (&tv, NULL))
	st.st_atime = st.st_mtime = st.st_ctime = tv.tv_sec;
      else
	st.st_atime = st.st_mtime = st.st_ctime = (time_t) 0;
      ret = 0;
    }
  else
    ret = fstat (fd, &st);

  if (ret == -1)
    {
      remote_fileio_return_errno (remote, -1);
      return;
    }
  if (ptrval)
    {
      host_to_fileio_stat (&st, &fst);

      errno = target_write_memory (ptrval, (gdb_byte *) &fst, sizeof fst);
      if (errno != 0)
	{
	  remote_fileio_return_errno (remote, -1);
	  return;
	}
    }
  remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_gettimeofday (remote_target *remote, char *buf)
{
  LONGEST lnum;
  CORE_ADDR ptrval;
  int ret;
  struct timeval tv;
  struct fio_timeval ftv;

  /* 1. Parameter: struct timeval pointer */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  ptrval = (CORE_ADDR) lnum;
  /* 2. Parameter: some pointer value...  */
  if (remote_fileio_extract_long (&buf, &lnum))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  /* ...which has to be NULL.  */
  if (lnum)
    {
      remote_fileio_reply (remote, -1, FILEIO_EINVAL);
      return;
    }

  ret = gettimeofday (&tv, NULL);

  if (ret == -1)
    {
      remote_fileio_return_errno (remote, -1);
      return;
    }

  if (ptrval)
    {
      remote_fileio_to_fio_timeval (&tv, &ftv);

      errno = target_write_memory (ptrval, (gdb_byte *) &ftv, sizeof ftv);
      if (errno != 0)
	{
	  remote_fileio_return_errno (remote, -1);
	  return;
	}
    }
  remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_isatty (remote_target *remote, char *buf)
{
  long target_fd;
  int fd;

  /* Parameter: file descriptor */
  if (remote_fileio_extract_int (&buf, &target_fd))
    {
      remote_fileio_ioerror (remote);
      return;
    }
  fd = remote_fileio_map_fd ((int) target_fd);
  int ret = fd == FIO_FD_CONSOLE_IN || fd == FIO_FD_CONSOLE_OUT ? 1 : 0;
  remote_fileio_return_success (remote, ret);
}

static void
remote_fileio_func_system (remote_target *remote, char *buf)
{
  CORE_ADDR ptrval;
  int ret, length;
  char *cmdline = NULL;

  /* Parameter: Ptr to commandline / length incl. trailing zero */
  if (remote_fileio_extract_ptr_w_len (&buf, &ptrval, &length))
    {
      remote_fileio_ioerror (remote);
      return;
    }

  if (length)
    {
      /* Request commandline using 'm' packet */
      cmdline = (char *) alloca (length);
      if (target_read_memory (ptrval, (gdb_byte *) cmdline, length) != 0)
	{
	  remote_fileio_ioerror (remote);
	  return;
	}
    }
  
  /* Check if system(3) has been explicitly allowed using the
     `set remote system-call-allowed 1' command.  If length is 0,
     indicating a NULL parameter to the system call, return zero to
     indicate a shell is not available.  Otherwise fail with EPERM.  */
  if (!remote_fio_system_call_allowed)
    {
      if (!length)
	remote_fileio_return_success (remote, 0);
      else
	remote_fileio_reply (remote, -1, FILEIO_EPERM);
      return;
    }

  ret = system (cmdline);

  if (!length)
    remote_fileio_return_success (remote, ret);
  else if (ret == -1)
    remote_fileio_return_errno (remote, -1);
  else
    remote_fileio_return_success (remote, WEXITSTATUS (ret));
}

static struct {
  const char *name;
  void (*func)(remote_target *remote, char *);
} remote_fio_func_map[] = {
  { "open", remote_fileio_func_open },
  { "close", remote_fileio_func_close },
  { "read", remote_fileio_func_read },
  { "write", remote_fileio_func_write },
  { "lseek", remote_fileio_func_lseek },
  { "rename", remote_fileio_func_rename },
  { "unlink", remote_fileio_func_unlink },
  { "stat", remote_fileio_func_stat },
  { "fstat", remote_fileio_func_fstat },
  { "gettimeofday", remote_fileio_func_gettimeofday },
  { "isatty", remote_fileio_func_isatty },
  { "system", remote_fileio_func_system },
  { NULL, NULL }
};

static void
do_remote_fileio_request (remote_target *remote, char *buf)
{
  char *c;
  int idx;

  quit_handler = remote_fileio_quit_handler;

  c = strchr (++buf, ',');
  if (c)
    *c++ = '\0';
  else
    c = strchr (buf, '\0');
  for (idx = 0; remote_fio_func_map[idx].name; ++idx)
    if (!strcmp (remote_fio_func_map[idx].name, buf))
      break;
  if (!remote_fio_func_map[idx].name)
    remote_fileio_reply (remote, -1, FILEIO_ENOSYS);
  else
    remote_fio_func_map[idx].func (remote, c);
}

/* Close any open descriptors, and reinitialize the file mapping.  */

void
remote_fileio_reset (void)
{
  int ix;

  for (ix = 0; ix != remote_fio_data.fd_map_size; ix++)
    {
      int fd = remote_fio_data.fd_map[ix];

      if (fd >= 0)
	close (fd);
    }
  if (remote_fio_data.fd_map)
    {
      xfree (remote_fio_data.fd_map);
      remote_fio_data.fd_map = NULL;
      remote_fio_data.fd_map_size = 0;
    }
}

/* Handle a file I/O request.  BUF points to the packet containing the
   request.  CTRLC_PENDING_P should be nonzero if the target has not
   acknowledged the Ctrl-C sent asynchronously earlier.  */

void
remote_fileio_request (remote_target *remote, char *buf, int ctrlc_pending_p)
{
  /* Save the previous quit handler, so we can restore it.  No need
     for a cleanup since we catch all exceptions below.  Note that the
     quit handler is also restored by remote_fileio_reply just before
     pushing a packet.  */
  remote_fileio_o_quit_handler = quit_handler;

  if (ctrlc_pending_p)
    {
      /* If the target hasn't responded to the Ctrl-C sent
	 asynchronously earlier, take this opportunity to send the
	 Ctrl-C synchronously.  */
      set_quit_flag ();
      remote_fileio_reply (remote, -1, FILEIO_EINTR);
    }
  else
    {
      try
	{
	  do_remote_fileio_request (remote, buf);
	}
      catch (const gdb_exception_forced_quit &ex)
	{
	  throw;
	}
      catch (const gdb_exception_quit &ex)
	{
	  remote_fileio_reply (remote, -1, FILEIO_EINTR);
	}
      catch (const gdb_exception &ex)
	{
	  remote_fileio_reply (remote, -1, FILEIO_EIO);
	}
    }

  quit_handler = remote_fileio_o_quit_handler;
}


/* Unpack an fio_uint_t.  */

static unsigned int
remote_fileio_to_host_uint (fio_uint_t fnum)
{
  return extract_unsigned_integer ((gdb_byte *) fnum, 4,
				   BFD_ENDIAN_BIG);
}

/* Unpack an fio_ulong_t.  */

static ULONGEST
remote_fileio_to_host_ulong (fio_ulong_t fnum)
{
  return extract_unsigned_integer ((gdb_byte *) fnum, 8,
				   BFD_ENDIAN_BIG);
}

/* Unpack an fio_mode_t.  */

static mode_t
remote_fileio_to_host_mode (fio_mode_t fnum)
{
  return remote_fileio_mode_to_host (remote_fileio_to_host_uint (fnum),
				     0);
}

/* Unpack an fio_time_t.  */

static time_t
remote_fileio_to_host_time (fio_time_t fnum)
{
  return remote_fileio_to_host_uint (fnum);
}


/* See remote-fileio.h.  */

void
remote_fileio_to_host_stat (struct fio_stat *fst, struct stat *st)
{
  memset (st, 0, sizeof (struct stat));

  st->st_dev = remote_fileio_to_host_uint (fst->fst_dev);
  st->st_ino = remote_fileio_to_host_uint (fst->fst_ino);
  st->st_mode = remote_fileio_to_host_mode (fst->fst_mode);
  st->st_nlink = remote_fileio_to_host_uint (fst->fst_nlink);
  st->st_uid = remote_fileio_to_host_uint (fst->fst_uid);
  st->st_gid = remote_fileio_to_host_uint (fst->fst_gid);
  st->st_rdev = remote_fileio_to_host_uint (fst->fst_rdev);
  st->st_size = remote_fileio_to_host_ulong (fst->fst_size);
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
  st->st_blksize = remote_fileio_to_host_ulong (fst->fst_blksize);
#endif
#if HAVE_STRUCT_STAT_ST_BLOCKS
  st->st_blocks = remote_fileio_to_host_ulong (fst->fst_blocks);
#endif
  st->st_atime = remote_fileio_to_host_time (fst->fst_atime);
  st->st_mtime = remote_fileio_to_host_time (fst->fst_mtime);
  st->st_ctime = remote_fileio_to_host_time (fst->fst_ctime);
}


static void
set_system_call_allowed (const char *args, int from_tty)
{
  if (args)
    {
      char *arg_end;
      int val = strtoul (args, &arg_end, 10);

      if (*args && *arg_end == '\0')
	{
	  remote_fio_system_call_allowed = !!val;
	  return;
	}
    }
  error (_("Illegal argument for \"set remote system-call-allowed\" command"));
}

static void
show_system_call_allowed (const char *args, int from_tty)
{
  if (args)
    error (_("Garbage after \"show remote "
	     "system-call-allowed\" command: `%s'"), args);
  gdb_printf ("Calling host system(3) call from target is %sallowed\n",
	      remote_fio_system_call_allowed ? "" : "not ");
}

void
initialize_remote_fileio (struct cmd_list_element **remote_set_cmdlist,
			  struct cmd_list_element **remote_show_cmdlist)
{
  add_cmd ("system-call-allowed", no_class,
	   set_system_call_allowed,
	   _("Set if the host system(3) call is allowed for the target."),
	   remote_set_cmdlist);
  add_cmd ("system-call-allowed", no_class,
	   show_system_call_allowed,
	   _("Show if the host system(3) call is allowed for the target."),
	   remote_show_cmdlist);
}

/* File-I/O functions for GDB, the GNU debugger.

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

#include "common-defs.h"
#include "fileio.h"
#include <sys/stat.h>
#include <fcntl.h>

/* See fileio.h.  */

fileio_error
host_to_fileio_error (int error)
{
  switch (error)
    {
      case EPERM:
	return FILEIO_EPERM;
      case ENOENT:
	return FILEIO_ENOENT;
      case EINTR:
	return FILEIO_EINTR;
      case EIO:
	return FILEIO_EIO;
      case EBADF:
	return FILEIO_EBADF;
      case EACCES:
	return FILEIO_EACCES;
      case EFAULT:
	return FILEIO_EFAULT;
      case EBUSY:
	return FILEIO_EBUSY;
      case EEXIST:
	return FILEIO_EEXIST;
      case ENODEV:
	return FILEIO_ENODEV;
      case ENOTDIR:
	return FILEIO_ENOTDIR;
      case EISDIR:
	return FILEIO_EISDIR;
      case EINVAL:
	return FILEIO_EINVAL;
      case ENFILE:
	return FILEIO_ENFILE;
      case EMFILE:
	return FILEIO_EMFILE;
      case EFBIG:
	return FILEIO_EFBIG;
      case ENOSPC:
	return FILEIO_ENOSPC;
      case ESPIPE:
	return FILEIO_ESPIPE;
      case EROFS:
	return FILEIO_EROFS;
      case ENOSYS:
	return FILEIO_ENOSYS;
      case ENAMETOOLONG:
	return FILEIO_ENAMETOOLONG;
    }
  return FILEIO_EUNKNOWN;
}

/* See fileio.h.  */

int
fileio_error_to_host (fileio_error errnum)
{
  switch (errnum)
    {
      case FILEIO_EPERM:
	return EPERM;
      case FILEIO_ENOENT:
	return ENOENT;
      case FILEIO_EINTR:
	return EINTR;
      case FILEIO_EIO:
	return EIO;
      case FILEIO_EBADF:
	return EBADF;
      case FILEIO_EACCES:
	return EACCES;
      case FILEIO_EFAULT:
	return EFAULT;
      case FILEIO_EBUSY:
	return EBUSY;
      case FILEIO_EEXIST:
	return EEXIST;
      case FILEIO_ENODEV:
	return ENODEV;
      case FILEIO_ENOTDIR:
	return ENOTDIR;
      case FILEIO_EISDIR:
	return EISDIR;
      case FILEIO_EINVAL:
	return EINVAL;
      case FILEIO_ENFILE:
	return ENFILE;
      case FILEIO_EMFILE:
	return EMFILE;
      case FILEIO_EFBIG:
	return EFBIG;
      case FILEIO_ENOSPC:
	return ENOSPC;
      case FILEIO_ESPIPE:
	return ESPIPE;
      case FILEIO_EROFS:
	return EROFS;
      case FILEIO_ENOSYS:
	return ENOSYS;
      case FILEIO_ENAMETOOLONG:
	return ENAMETOOLONG;
    }
  return -1;
}

/* See fileio.h.  */

int
fileio_to_host_openflags (int fileio_open_flags, int *open_flags_p)
{
  int open_flags = 0;

  if (fileio_open_flags & ~FILEIO_O_SUPPORTED)
    return -1;

  if (fileio_open_flags & FILEIO_O_CREAT)
    open_flags |= O_CREAT;
  if (fileio_open_flags & FILEIO_O_EXCL)
    open_flags |= O_EXCL;
  if (fileio_open_flags & FILEIO_O_TRUNC)
    open_flags |= O_TRUNC;
  if (fileio_open_flags & FILEIO_O_APPEND)
    open_flags |= O_APPEND;
  if (fileio_open_flags & FILEIO_O_RDONLY)
    open_flags |= O_RDONLY;
  if (fileio_open_flags & FILEIO_O_WRONLY)
    open_flags |= O_WRONLY;
  if (fileio_open_flags & FILEIO_O_RDWR)
    open_flags |= O_RDWR;
  /* On systems supporting binary and text mode, always open files
     in binary mode. */
#ifdef O_BINARY
  open_flags |= O_BINARY;
#endif

  *open_flags_p = open_flags;
  return 0;
}

/* See fileio.h.  */

int
fileio_to_host_mode (int fileio_mode, mode_t *mode_p)
{
  mode_t mode = 0;

  if (fileio_mode & ~FILEIO_S_SUPPORTED)
    return -1;

  if (fileio_mode & FILEIO_S_IFREG)
    mode |= S_IFREG;
  if (fileio_mode & FILEIO_S_IFDIR)
    mode |= S_IFDIR;
  if (fileio_mode & FILEIO_S_IFCHR)
    mode |= S_IFCHR;
  if (fileio_mode & FILEIO_S_IRUSR)
    mode |= S_IRUSR;
  if (fileio_mode & FILEIO_S_IWUSR)
    mode |= S_IWUSR;
  if (fileio_mode & FILEIO_S_IXUSR)
    mode |= S_IXUSR;
#ifdef S_IRGRP
  if (fileio_mode & FILEIO_S_IRGRP)
    mode |= S_IRGRP;
#endif
#ifdef S_IWGRP
  if (fileio_mode & FILEIO_S_IWGRP)
    mode |= S_IWGRP;
#endif
#ifdef S_IXGRP
  if (fileio_mode & FILEIO_S_IXGRP)
    mode |= S_IXGRP;
#endif
  if (fileio_mode & FILEIO_S_IROTH)
    mode |= S_IROTH;
#ifdef S_IWOTH
  if (fileio_mode & FILEIO_S_IWOTH)
    mode |= S_IWOTH;
#endif
#ifdef S_IXOTH
  if (fileio_mode & FILEIO_S_IXOTH)
    mode |= S_IXOTH;
#endif

  *mode_p = mode;
  return 0;
}

/* Convert a host-format mode_t into a bitmask of File-I/O flags.  */

static LONGEST
fileio_mode_pack (mode_t mode)
{
  mode_t tmode = 0;

  if (S_ISREG (mode))
    tmode |= FILEIO_S_IFREG;
  if (S_ISDIR (mode))
    tmode |= FILEIO_S_IFDIR;
  if (S_ISCHR (mode))
    tmode |= FILEIO_S_IFCHR;
  if (mode & S_IRUSR)
    tmode |= FILEIO_S_IRUSR;
  if (mode & S_IWUSR)
    tmode |= FILEIO_S_IWUSR;
  if (mode & S_IXUSR)
    tmode |= FILEIO_S_IXUSR;
#ifdef S_IRGRP
  if (mode & S_IRGRP)
    tmode |= FILEIO_S_IRGRP;
#endif
#ifdef S_IWGRP
  if (mode & S_IWGRP)
    tmode |= FILEIO_S_IWGRP;
#endif
#ifdef S_IXGRP
  if (mode & S_IXGRP)
    tmode |= FILEIO_S_IXGRP;
#endif
  if (mode & S_IROTH)
    tmode |= FILEIO_S_IROTH;
#ifdef S_IWOTH
  if (mode & S_IWOTH)
    tmode |= FILEIO_S_IWOTH;
#endif
#ifdef S_IXOTH
  if (mode & S_IXOTH)
    tmode |= FILEIO_S_IXOTH;
#endif
  return tmode;
}

/* Pack a host-format mode_t into an fio_mode_t.  */

static void
host_to_fileio_mode (mode_t num, fio_mode_t fnum)
{
  host_to_bigendian (fileio_mode_pack (num), (char *) fnum, 4);
}

/* Pack a host-format integer into an fio_ulong_t.  */

static void
host_to_fileio_ulong (LONGEST num, fio_ulong_t fnum)
{
  host_to_bigendian (num, (char *) fnum, 8);
}

/* See fileio.h.  */

void
host_to_fileio_stat (struct stat *st, struct fio_stat *fst)
{
  LONGEST blksize;

  host_to_fileio_uint ((long) st->st_dev, fst->fst_dev);
  host_to_fileio_uint ((long) st->st_ino, fst->fst_ino);
  host_to_fileio_mode (st->st_mode, fst->fst_mode);
  host_to_fileio_uint ((long) st->st_nlink, fst->fst_nlink);
  host_to_fileio_uint ((long) st->st_uid, fst->fst_uid);
  host_to_fileio_uint ((long) st->st_gid, fst->fst_gid);
  host_to_fileio_uint ((long) st->st_rdev, fst->fst_rdev);
  host_to_fileio_ulong ((LONGEST) st->st_size, fst->fst_size);
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
  blksize = st->st_blksize;
#else
  blksize = 512;
#endif
  host_to_fileio_ulong (blksize, fst->fst_blksize);
#if HAVE_STRUCT_STAT_ST_BLOCKS
  host_to_fileio_ulong ((LONGEST) st->st_blocks, fst->fst_blocks);
#else
  /* FIXME: This is correct for DJGPP, but other systems that don't
     have st_blocks, if any, might prefer 512 instead of st_blksize.
     (eliz, 30-12-2003)  */
  host_to_fileio_ulong (((LONGEST) st->st_size + blksize - 1)
			/ blksize,
			fst->fst_blocks);
#endif
  host_to_fileio_time (st->st_atime, fst->fst_atime);
  host_to_fileio_time (st->st_mtime, fst->fst_mtime);
  host_to_fileio_time (st->st_ctime, fst->fst_ctime);
}

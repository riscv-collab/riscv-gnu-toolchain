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

#ifndef COMMON_FILEIO_H
#define COMMON_FILEIO_H

#include <sys/stat.h>

/* The following flags are defined to be independent of the host
   as well as the target side implementation of these constants.
   All constants are defined with a leading FILEIO_ in the name
   to allow the usage of these constants together with the
   corresponding implementation dependent constants in one module. */

/* open(2) flags */
#define FILEIO_O_RDONLY           0x0
#define FILEIO_O_WRONLY           0x1
#define FILEIO_O_RDWR             0x2
#define FILEIO_O_APPEND           0x8
#define FILEIO_O_CREAT          0x200
#define FILEIO_O_TRUNC          0x400
#define FILEIO_O_EXCL           0x800
#define FILEIO_O_SUPPORTED      (FILEIO_O_RDONLY | FILEIO_O_WRONLY| \
				 FILEIO_O_RDWR   | FILEIO_O_APPEND| \
				 FILEIO_O_CREAT  | FILEIO_O_TRUNC| \
				 FILEIO_O_EXCL)

/* mode_t bits */
#define FILEIO_S_IFREG        0100000
#define FILEIO_S_IFDIR         040000
#define FILEIO_S_IFCHR         020000
#define FILEIO_S_IRUSR           0400
#define FILEIO_S_IWUSR           0200
#define FILEIO_S_IXUSR           0100
#define FILEIO_S_IRWXU           0700
#define FILEIO_S_IRGRP            040
#define FILEIO_S_IWGRP            020
#define FILEIO_S_IXGRP            010
#define FILEIO_S_IRWXG            070
#define FILEIO_S_IROTH             04
#define FILEIO_S_IWOTH             02
#define FILEIO_S_IXOTH             01
#define FILEIO_S_IRWXO             07
#define FILEIO_S_SUPPORTED         (FILEIO_S_IFREG|FILEIO_S_IFDIR|  \
				    FILEIO_S_IRWXU|FILEIO_S_IRWXG|  \
				    FILEIO_S_IRWXO)

/* lseek(2) flags */
#define FILEIO_SEEK_SET             0
#define FILEIO_SEEK_CUR             1
#define FILEIO_SEEK_END             2

/* errno values */
enum fileio_error
{
  FILEIO_SUCCESS      =    0,
  FILEIO_EPERM        =    1,
  FILEIO_ENOENT       =    2,
  FILEIO_EINTR        =    4,
  FILEIO_EIO          =    5,
  FILEIO_EBADF        =    9,
  FILEIO_EACCES       =   13,
  FILEIO_EFAULT       =   14,
  FILEIO_EBUSY        =   16,
  FILEIO_EEXIST       =   17,
  FILEIO_ENODEV       =   19,
  FILEIO_ENOTDIR      =   20,
  FILEIO_EISDIR       =   21,
  FILEIO_EINVAL       =   22,
  FILEIO_ENFILE       =   23,
  FILEIO_EMFILE       =   24,
  FILEIO_EFBIG        =   27,
  FILEIO_ENOSPC       =   28,
  FILEIO_ESPIPE       =   29,
  FILEIO_EROFS        =   30,
  FILEIO_ENOSYS       =   88,
  FILEIO_ENAMETOOLONG =   91,
  FILEIO_EUNKNOWN     = 9999,
};

#define FIO_INT_LEN   4
#define FIO_UINT_LEN  4
#define FIO_MODE_LEN  4
#define FIO_TIME_LEN  4
#define FIO_LONG_LEN  8
#define FIO_ULONG_LEN 8

typedef char fio_int_t[FIO_INT_LEN];
typedef char fio_uint_t[FIO_UINT_LEN];
typedef char fio_mode_t[FIO_MODE_LEN];
typedef char fio_time_t[FIO_TIME_LEN];
typedef char fio_long_t[FIO_LONG_LEN];
typedef char fio_ulong_t[FIO_ULONG_LEN];

/* Struct stat as used in protocol.  For complete independence
   of host/target systems, it's defined as an array with offsets
   to the members. */

struct fio_stat
{
  fio_uint_t  fst_dev;
  fio_uint_t  fst_ino;
  fio_mode_t  fst_mode;
  fio_uint_t  fst_nlink;
  fio_uint_t  fst_uid;
  fio_uint_t  fst_gid;
  fio_uint_t  fst_rdev;
  fio_ulong_t fst_size;
  fio_ulong_t fst_blksize;
  fio_ulong_t fst_blocks;
  fio_time_t  fst_atime;
  fio_time_t  fst_mtime;
  fio_time_t  fst_ctime;
};

struct fio_timeval
{
  fio_time_t  ftv_sec;
  fio_long_t  ftv_usec;
};

/* Convert a host-format errno value to a File-I/O error number.  */

extern fileio_error host_to_fileio_error (int error);

/* Convert a File-I/O error number to a host-format errno value.  */

extern int fileio_error_to_host (fileio_error errnum);

/* Convert File-I/O open flags FFLAGS to host format, storing
   the result in *FLAGS.  Return 0 on success, -1 on error.  */

extern int fileio_to_host_openflags (int fflags, int *flags);

/* Convert File-I/O mode FMODE to host format, storing
   the result in *MODE.  Return 0 on success, -1 on error.  */

extern int fileio_to_host_mode (int fmode, mode_t *mode);

/* Pack a host-format integer into a byte buffer in big-endian
   format.  BYTES specifies the size of the integer to pack in
   bytes.  */

static inline void
host_to_bigendian (LONGEST num, char *buf, int bytes)
{
  int i;

  for (i = 0; i < bytes; ++i)
    buf[i] = (num >> (8 * (bytes - i - 1))) & 0xff;
}

/* Pack a host-format integer into an fio_uint_t.  */

static inline void
host_to_fileio_uint (long num, fio_uint_t fnum)
{
  host_to_bigendian ((LONGEST) num, (char *) fnum, 4);
}

/* Pack a host-format time_t into an fio_time_t.  */

static inline void
host_to_fileio_time (time_t num, fio_time_t fnum)
{
  host_to_bigendian ((LONGEST) num, (char *) fnum, 4);
}

/* Pack a host-format struct stat into a struct fio_stat.  */

extern void host_to_fileio_stat (struct stat *st, struct fio_stat *fst);

#endif /* COMMON_FILEIO_H */

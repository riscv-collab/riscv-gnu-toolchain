/* Target errno mappings for newlib/libgloss environment.
   Copyright 1995-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <errno.h>

#include "sim/callback.h"

/* This file is kept up-to-date via the gennltvals.py script.  Do not edit
   anything between the START & END comment blocks below.  */

CB_TARGET_DEFS_MAP cb_init_errno_map[] = {
  /* gennltvals: START */
#ifdef E2BIG
  { "E2BIG", E2BIG, 7 },
#endif
#ifdef EACCES
  { "EACCES", EACCES, 13 },
#endif
#ifdef EADDRINUSE
  { "EADDRINUSE", EADDRINUSE, 112 },
#endif
#ifdef EADDRNOTAVAIL
  { "EADDRNOTAVAIL", EADDRNOTAVAIL, 125 },
#endif
#ifdef EADV
  { "EADV", EADV, 68 },
#endif
#ifdef EAFNOSUPPORT
  { "EAFNOSUPPORT", EAFNOSUPPORT, 106 },
#endif
#ifdef EAGAIN
  { "EAGAIN", EAGAIN, 11 },
#endif
#ifdef EALREADY
  { "EALREADY", EALREADY, 120 },
#endif
#ifdef EBADE
  { "EBADE", EBADE, 50 },
#endif
#ifdef EBADF
  { "EBADF", EBADF, 9 },
#endif
#ifdef EBADFD
  { "EBADFD", EBADFD, 81 },
#endif
#ifdef EBADMSG
  { "EBADMSG", EBADMSG, 77 },
#endif
#ifdef EBADR
  { "EBADR", EBADR, 51 },
#endif
#ifdef EBADRQC
  { "EBADRQC", EBADRQC, 54 },
#endif
#ifdef EBADSLT
  { "EBADSLT", EBADSLT, 55 },
#endif
#ifdef EBFONT
  { "EBFONT", EBFONT, 57 },
#endif
#ifdef EBUSY
  { "EBUSY", EBUSY, 16 },
#endif
#ifdef ECANCELED
  { "ECANCELED", ECANCELED, 140 },
#endif
#ifdef ECHILD
  { "ECHILD", ECHILD, 10 },
#endif
#ifdef ECHRNG
  { "ECHRNG", ECHRNG, 37 },
#endif
#ifdef ECOMM
  { "ECOMM", ECOMM, 70 },
#endif
#ifdef ECONNABORTED
  { "ECONNABORTED", ECONNABORTED, 113 },
#endif
#ifdef ECONNREFUSED
  { "ECONNREFUSED", ECONNREFUSED, 111 },
#endif
#ifdef ECONNRESET
  { "ECONNRESET", ECONNRESET, 104 },
#endif
#ifdef EDEADLK
  { "EDEADLK", EDEADLK, 45 },
#endif
#ifdef EDEADLOCK
  { "EDEADLOCK", EDEADLOCK, 56 },
#endif
#ifdef EDESTADDRREQ
  { "EDESTADDRREQ", EDESTADDRREQ, 121 },
#endif
#ifdef EDOM
  { "EDOM", EDOM, 33 },
#endif
#ifdef EDOTDOT
  { "EDOTDOT", EDOTDOT, 76 },
#endif
#ifdef EDQUOT
  { "EDQUOT", EDQUOT, 132 },
#endif
#ifdef EEXIST
  { "EEXIST", EEXIST, 17 },
#endif
#ifdef EFAULT
  { "EFAULT", EFAULT, 14 },
#endif
#ifdef EFBIG
  { "EFBIG", EFBIG, 27 },
#endif
#ifdef EFTYPE
  { "EFTYPE", EFTYPE, 79 },
#endif
#ifdef EHOSTDOWN
  { "EHOSTDOWN", EHOSTDOWN, 117 },
#endif
#ifdef EHOSTUNREACH
  { "EHOSTUNREACH", EHOSTUNREACH, 118 },
#endif
#ifdef EIDRM
  { "EIDRM", EIDRM, 36 },
#endif
#ifdef EILSEQ
  { "EILSEQ", EILSEQ, 138 },
#endif
#ifdef EINPROGRESS
  { "EINPROGRESS", EINPROGRESS, 119 },
#endif
#ifdef EINTR
  { "EINTR", EINTR, 4 },
#endif
#ifdef EINVAL
  { "EINVAL", EINVAL, 22 },
#endif
#ifdef EIO
  { "EIO", EIO, 5 },
#endif
#ifdef EISCONN
  { "EISCONN", EISCONN, 127 },
#endif
#ifdef EISDIR
  { "EISDIR", EISDIR, 21 },
#endif
#ifdef EL2HLT
  { "EL2HLT", EL2HLT, 44 },
#endif
#ifdef EL2NSYNC
  { "EL2NSYNC", EL2NSYNC, 38 },
#endif
#ifdef EL3HLT
  { "EL3HLT", EL3HLT, 39 },
#endif
#ifdef EL3RST
  { "EL3RST", EL3RST, 40 },
#endif
#ifdef ELBIN
  { "ELBIN", ELBIN, 75 },
#endif
#ifdef ELIBACC
  { "ELIBACC", ELIBACC, 83 },
#endif
#ifdef ELIBBAD
  { "ELIBBAD", ELIBBAD, 84 },
#endif
#ifdef ELIBEXEC
  { "ELIBEXEC", ELIBEXEC, 87 },
#endif
#ifdef ELIBMAX
  { "ELIBMAX", ELIBMAX, 86 },
#endif
#ifdef ELIBSCN
  { "ELIBSCN", ELIBSCN, 85 },
#endif
#ifdef ELNRNG
  { "ELNRNG", ELNRNG, 41 },
#endif
#ifdef ELOOP
  { "ELOOP", ELOOP, 92 },
#endif
#ifdef EMFILE
  { "EMFILE", EMFILE, 24 },
#endif
#ifdef EMLINK
  { "EMLINK", EMLINK, 31 },
#endif
#ifdef EMSGSIZE
  { "EMSGSIZE", EMSGSIZE, 122 },
#endif
#ifdef EMULTIHOP
  { "EMULTIHOP", EMULTIHOP, 74 },
#endif
#ifdef ENAMETOOLONG
  { "ENAMETOOLONG", ENAMETOOLONG, 91 },
#endif
#ifdef ENETDOWN
  { "ENETDOWN", ENETDOWN, 115 },
#endif
#ifdef ENETRESET
  { "ENETRESET", ENETRESET, 126 },
#endif
#ifdef ENETUNREACH
  { "ENETUNREACH", ENETUNREACH, 114 },
#endif
#ifdef ENFILE
  { "ENFILE", ENFILE, 23 },
#endif
#ifdef ENOANO
  { "ENOANO", ENOANO, 53 },
#endif
#ifdef ENOBUFS
  { "ENOBUFS", ENOBUFS, 105 },
#endif
#ifdef ENOCSI
  { "ENOCSI", ENOCSI, 43 },
#endif
#ifdef ENODATA
  { "ENODATA", ENODATA, 61 },
#endif
#ifdef ENODEV
  { "ENODEV", ENODEV, 19 },
#endif
#ifdef ENOENT
  { "ENOENT", ENOENT, 2 },
#endif
#ifdef ENOEXEC
  { "ENOEXEC", ENOEXEC, 8 },
#endif
#ifdef ENOLCK
  { "ENOLCK", ENOLCK, 46 },
#endif
#ifdef ENOLINK
  { "ENOLINK", ENOLINK, 67 },
#endif
#ifdef ENOMEDIUM
  { "ENOMEDIUM", ENOMEDIUM, 135 },
#endif
#ifdef ENOMEM
  { "ENOMEM", ENOMEM, 12 },
#endif
#ifdef ENOMSG
  { "ENOMSG", ENOMSG, 35 },
#endif
#ifdef ENONET
  { "ENONET", ENONET, 64 },
#endif
#ifdef ENOPKG
  { "ENOPKG", ENOPKG, 65 },
#endif
#ifdef ENOPROTOOPT
  { "ENOPROTOOPT", ENOPROTOOPT, 109 },
#endif
#ifdef ENOSPC
  { "ENOSPC", ENOSPC, 28 },
#endif
#ifdef ENOSR
  { "ENOSR", ENOSR, 63 },
#endif
#ifdef ENOSTR
  { "ENOSTR", ENOSTR, 60 },
#endif
#ifdef ENOSYS
  { "ENOSYS", ENOSYS, 88 },
#endif
#ifdef ENOTBLK
  { "ENOTBLK", ENOTBLK, 15 },
#endif
#ifdef ENOTCONN
  { "ENOTCONN", ENOTCONN, 128 },
#endif
#ifdef ENOTDIR
  { "ENOTDIR", ENOTDIR, 20 },
#endif
#ifdef ENOTEMPTY
  { "ENOTEMPTY", ENOTEMPTY, 90 },
#endif
#ifdef ENOTRECOVERABLE
  { "ENOTRECOVERABLE", ENOTRECOVERABLE, 141 },
#endif
#ifdef ENOTSOCK
  { "ENOTSOCK", ENOTSOCK, 108 },
#endif
#ifdef ENOTSUP
  { "ENOTSUP", ENOTSUP, 134 },
#endif
#ifdef ENOTTY
  { "ENOTTY", ENOTTY, 25 },
#endif
#ifdef ENOTUNIQ
  { "ENOTUNIQ", ENOTUNIQ, 80 },
#endif
#ifdef ENXIO
  { "ENXIO", ENXIO, 6 },
#endif
#ifdef EOPNOTSUPP
  { "EOPNOTSUPP", EOPNOTSUPP, 95 },
#endif
#ifdef EOVERFLOW
  { "EOVERFLOW", EOVERFLOW, 139 },
#endif
#ifdef EOWNERDEAD
  { "EOWNERDEAD", EOWNERDEAD, 142 },
#endif
#ifdef EPERM
  { "EPERM", EPERM, 1 },
#endif
#ifdef EPFNOSUPPORT
  { "EPFNOSUPPORT", EPFNOSUPPORT, 96 },
#endif
#ifdef EPIPE
  { "EPIPE", EPIPE, 32 },
#endif
#ifdef EPROCLIM
  { "EPROCLIM", EPROCLIM, 130 },
#endif
#ifdef EPROTO
  { "EPROTO", EPROTO, 71 },
#endif
#ifdef EPROTONOSUPPORT
  { "EPROTONOSUPPORT", EPROTONOSUPPORT, 123 },
#endif
#ifdef EPROTOTYPE
  { "EPROTOTYPE", EPROTOTYPE, 107 },
#endif
#ifdef ERANGE
  { "ERANGE", ERANGE, 34 },
#endif
#ifdef EREMCHG
  { "EREMCHG", EREMCHG, 82 },
#endif
#ifdef EREMOTE
  { "EREMOTE", EREMOTE, 66 },
#endif
#ifdef EROFS
  { "EROFS", EROFS, 30 },
#endif
#ifdef ESHUTDOWN
  { "ESHUTDOWN", ESHUTDOWN, 110 },
#endif
#ifdef ESOCKTNOSUPPORT
  { "ESOCKTNOSUPPORT", ESOCKTNOSUPPORT, 124 },
#endif
#ifdef ESPIPE
  { "ESPIPE", ESPIPE, 29 },
#endif
#ifdef ESRCH
  { "ESRCH", ESRCH, 3 },
#endif
#ifdef ESRMNT
  { "ESRMNT", ESRMNT, 69 },
#endif
#ifdef ESTALE
  { "ESTALE", ESTALE, 133 },
#endif
#ifdef ESTRPIPE
  { "ESTRPIPE", ESTRPIPE, 143 },
#endif
#ifdef ETIME
  { "ETIME", ETIME, 62 },
#endif
#ifdef ETIMEDOUT
  { "ETIMEDOUT", ETIMEDOUT, 116 },
#endif
#ifdef ETOOMANYREFS
  { "ETOOMANYREFS", ETOOMANYREFS, 129 },
#endif
#ifdef ETXTBSY
  { "ETXTBSY", ETXTBSY, 26 },
#endif
#ifdef EUNATCH
  { "EUNATCH", EUNATCH, 42 },
#endif
#ifdef EUSERS
  { "EUSERS", EUSERS, 131 },
#endif
#ifdef EWOULDBLOCK
  { "EWOULDBLOCK", EWOULDBLOCK, 11 },
#endif
#ifdef EXDEV
  { "EXDEV", EXDEV, 18 },
#endif
#ifdef EXFULL
  { "EXFULL", EXFULL, 52 },
#endif
  /* gennltvals: END */
  { NULL, -1, -1 },
};

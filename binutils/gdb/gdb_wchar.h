/* Wide characters for gdb
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef GDB_WCHAR_H
#define GDB_WCHAR_H

/* We handle three different modes here.
   
   Capable systems have the full suite: wchar_t support and iconv
   (perhaps via GNU libiconv).  On these machines, full functionality
   is available.  Note that full functionality is dependent on us
   being able to convert from an arbitrary encoding to wchar_t.  In
   practice this means we look for __STDC_ISO_10646__ (where we know
   the name of the wchar_t encoding) or GNU libiconv, where we can use
   "wchar_t".
   
   DJGPP is known to have libiconv but not wchar_t support.  On
   systems like this, we use the narrow character functions.  The full
   functionality is available to the user, but many characters (those
   outside the narrow range) will be displayed as escapes.
   
   Finally, some systems do not have iconv, or are really broken
   (e.g., Solaris, which almost has all of this working, but where
   just enough is broken to make it too hard to use).  Here we provide
   a phony iconv which only handles a single character set, and we
   provide wrappers for the wchar_t functionality we use.  */


#if defined (HAVE_ICONV)
#include <iconv.h>
#else
/* This define is used elsewhere so we don't need to duplicate the
   same checking logic in multiple places.  */
#define PHONY_ICONV
#endif

#include <wchar.h>
#include <wctype.h>

/* We use "btowc" as a sentinel to detect functioning wchar_t support.
   We check for either __STDC_ISO_10646__ or a new-enough libiconv in
   order to ensure we can convert to and from wchar_t.  We choose
   libiconv version 0x108 because it is the first version with
   iconvlist.  */
#if defined (HAVE_ICONV) && defined (HAVE_BTOWC) \
  && (defined (__STDC_ISO_10646__) \
      || (defined (_LIBICONV_VERSION) && _LIBICONV_VERSION >= 0x108))

typedef wchar_t gdb_wchar_t;
typedef wint_t gdb_wint_t;

#define gdb_wcslen wcslen
#define gdb_iswprint iswprint
#define gdb_iswxdigit iswxdigit
#define gdb_btowc btowc
#define gdb_WEOF WEOF

#define LCST(X) L ## X

/* If __STDC_ISO_10646__ is defined, then the host wchar_t is UCS-4.
   We exploit this fact in the hope that there are hosts that define
   this but which do not support "wchar_t" as an encoding argument to
   iconv_open.  We put the endianness into the encoding name to avoid
   hosts that emit a BOM when the unadorned name is used.  */
#if defined (__STDC_ISO_10646__)
#define USE_INTERMEDIATE_ENCODING_FUNCTION
#define INTERMEDIATE_ENCODING intermediate_encoding ()
const char *intermediate_encoding (void);

#elif defined (_LIBICONV_VERSION) && _LIBICONV_VERSION >= 0x108
#define INTERMEDIATE_ENCODING "wchar_t"
#else
/* This shouldn't happen, because the earlier #if should have filtered
   out this case.  */
#error "Neither __STDC_ISO_10646__ nor _LIBICONV_VERSION defined"
#endif

#else

/* If we got here and have wchar_t support, we might be on a system
   with some problem.  So, we just disable everything.  */
#if defined (HAVE_BTOWC)
#define PHONY_ICONV
#endif

typedef char gdb_wchar_t;
typedef int gdb_wint_t;

#define gdb_wcslen strlen
#define gdb_iswprint isprint
#define gdb_iswxdigit isxdigit
#define gdb_btowc /* empty */
#define gdb_WEOF EOF

#define LCST(X) X

/* If we are using the narrow character set, we want to use the host
   narrow encoding as our intermediate encoding.  However, if we are
   also providing a phony iconv, we might as well just stick with
   "wchar_t".  */
#ifdef PHONY_ICONV
#define INTERMEDIATE_ENCODING "wchar_t"
#else
#define INTERMEDIATE_ENCODING host_charset ()
#endif

#endif

#endif /* GDB_WCHAR_H */

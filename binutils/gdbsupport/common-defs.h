/* Common definitions.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_COMMON_DEFS_H
#define COMMON_COMMON_DEFS_H

#include <gdbsupport/config.h>

#undef PACKAGE_NAME
#undef PACKAGE
#undef PACKAGE_VERSION
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

#include "gnulib/config.h"

/* From:
    https://www.gnu.org/software/gnulib/manual/html_node/stdint_002eh.html

   "On some hosts that predate C++11, when using C++ one must define
   __STDC_CONSTANT_MACROS to make visible the definitions of constant
   macros such as INTMAX_C, and one must define __STDC_LIMIT_MACROS to
   make visible the definitions of limit macros such as INTMAX_MAX.".

   And:
    https://www.gnu.org/software/gnulib/manual/html_node/inttypes_002eh.html

   "On some hosts that predate C++11, when using C++ one must define
   __STDC_FORMAT_MACROS to make visible the declarations of format
   macros such as PRIdMAX."

   Must do this before including any system header, since other system
   headers may include stdint.h/inttypes.h.  */
#define __STDC_CONSTANT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#define __STDC_FORMAT_MACROS 1

/* Some distros enable _FORTIFY_SOURCE by default, which on occasion
   has caused build failures with -Wunused-result when a patch is
   developed on a distro that does not enable _FORTIFY_SOURCE.  We
   enable it here in order to try to catch these problems earlier;
   plus this seems like a reasonable safety measure.  The check for
   optimization is required because _FORTIFY_SOURCE only works when
   optimization is enabled.  If _FORTIFY_SOURCE is already defined,
   then we don't do anything.  Also, on MinGW, fortify requires
   linking to -lssp, and to avoid the hassle of checking for
   that and linking to it statically, we just don't define
   _FORTIFY_SOURCE there.  */

#if (!defined _FORTIFY_SOURCE && defined __OPTIMIZE__ && __OPTIMIZE__ > 0 \
     && !defined(__MINGW32__))
#define _FORTIFY_SOURCE 2
#endif

/* We don't support Windows versions before XP, so we define
   _WIN32_WINNT correspondingly to ensure the Windows API headers
   expose the required symbols.

   NOTE: this must be kept in sync with common.m4.  */
#if defined (__MINGW32__) || defined (__CYGWIN__)
# ifdef _WIN32_WINNT
#  if _WIN32_WINNT < 0x0501
#   undef _WIN32_WINNT
#   define _WIN32_WINNT 0x0501
#  endif
# else
#  define _WIN32_WINNT 0x0501
# endif
#endif	/* __MINGW32__ || __CYGWIN__ */

#include <stdarg.h>
#include <stdio.h>

/* Include both cstdlib and stdlib.h to ensure we have standard functions
   defined both in the std:: namespace and in the global namespace.  */
#include <cstdlib>
#include <stdlib.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <errno.h>
#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "ansidecl.h"
/* This is defined by ansidecl.h, but we prefer gnulib's version.  On
   MinGW, gnulib might enable __USE_MINGW_ANSI_STDIO, which may or not
   require use of attribute gnu_printf instead of printf.  gnulib
   checks that at configure time.  Since _GL_ATTRIBUTE_FORMAT_PRINTF_STANDARD
   is compatible with ATTRIBUTE_PRINTF, simply use it.  */
#undef ATTRIBUTE_PRINTF
#define ATTRIBUTE_PRINTF _GL_ATTRIBUTE_FORMAT_PRINTF_STANDARD

/* This is defined by ansidecl.h, but we disable the attribute.

   Say a developer starts out with:
   ...
   extern void foo (void *ptr) __attribute__((nonnull (1)));
   void foo (void *ptr) {}
   ...
   with the idea in mind to catch:
   ...
   foo (nullptr);
   ...
   at compile time with -Werror=nonnull, and then adds:
   ...
    void foo (void *ptr) {
   +  gdb_assert (ptr != nullptr);
    }
   ...
   to catch:
   ...
   foo (variable_with_nullptr_value);
   ...
   at runtime as well.

   Said developer then verifies that the assert works (using -O0), and commits
   the code.

   Some other developer then checks out the code and accidentally writes some
   variant of:
   ...
   foo (variable_with_nullptr_value);
   ...
   and builds with -O2, and ... the assert doesn't trigger, because it's
   optimized away by gcc.

   There's no suppported recipe to prevent the assertion from being optimized
   away (other than: build with -O0, or remove the nonnull attribute).  Note
   that -fno-delete-null-pointer-checks does not help.  A patch was submitted
   to improve gcc documentation to point this out more clearly (
   https://gcc.gnu.org/pipermail/gcc-patches/2021-July/576218.html ).  The
   patch also mentions a possible workaround that obfuscates the pointer
   using:
   ...
    void foo (void *ptr) {
   +  asm ("" : "+r"(ptr));
      gdb_assert (ptr != nullptr);
    }
   ...
   but that still requires the developer to manually add this in all cases
   where that's necessary.

   A warning was added to detect the situation: -Wnonnull-compare, which does
   help in detecting those cases, but each new gcc release may indicate a new
   batch of locations that needs fixing, which means we've added a maintenance
   burden.

   We could try to deal with the problem more proactively by introducing a
   gdb_assert variant like:
   ...
   void gdb_assert_non_null (void *ptr) {
      asm ("" : "+r"(ptr));
      gdb_assert (ptr != nullptr);
    }
    void foo (void *ptr) {
      gdb_assert_nonnull (ptr);
    }
   ...
   and make it a coding style to use it everywhere, but again, maintenance
   burden.

   With all these things considered, for now we go with the solution with the
   least maintenance burden: disable the attribute, such that we reliably deal
   with it everywhere.  */
#undef ATTRIBUTE_NONNULL
#define ATTRIBUTE_NONNULL(m)

#if GCC_VERSION >= 3004
#define ATTRIBUTE_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
#else
#define ATTRIBUTE_UNUSED_RESULT
#endif

#if (GCC_VERSION > 4000)
#define ATTRIBUTE_USED __attribute__ ((__used__))
#else
#define ATTRIBUTE_USED
#endif

#include "libiberty.h"
#include "pathmax.h"
#include "gdb/signals.h"
#include "gdb_locale.h"
#include "ptid.h"
#include "common-types.h"
#include "common-utils.h"
#include "gdb_assert.h"
#include "errors.h"
#include "print-utils.h"
#include "common-debug.h"
#include "cleanups.h"
#include "common-exceptions.h"
#include "gdbsupport/poison.h"

/* Pull in gdb::unique_xmalloc_ptr.  */
#include "gdbsupport/gdb_unique_ptr.h"

/* sbrk on macOS is not useful for our purposes, since sbrk(0) always
   returns the same value.  brk/sbrk on macOS is just an emulation
   that always returns a pointer to a 4MB section reserved for
   that.  */

#if defined (HAVE_SBRK) && !__APPLE__
#define HAVE_USEFUL_SBRK 1
#endif

#endif /* COMMON_COMMON_DEFS_H */

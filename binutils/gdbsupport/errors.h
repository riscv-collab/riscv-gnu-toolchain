/* Declarations for error-reporting facilities.

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

#ifndef COMMON_ERRORS_H
#define COMMON_ERRORS_H

/* A problem was detected, but the requested operation can still
   proceed.  A warning message is constructed using a printf- or
   vprintf-style argument list.  The function "vwarning" must be
   provided by the client.  */

extern void warning (const char *fmt, ...)
     ATTRIBUTE_PRINTF (1, 2);

extern void vwarning (const char *fmt, va_list args)
     ATTRIBUTE_PRINTF (1, 0);

/* A non-predictable, non-fatal error was detected.  The requested
   operation cannot proceed.  An error message is constructed using
   a printf- or vprintf-style argument list.  These functions do not
   return.  The function "verror" must be provided by the client.  */

extern void error (const char *fmt, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);

extern void verror (const char *fmt, va_list args)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 0);

/* An internal error was detected.  Internal errors indicate
   programming errors such as assertion failures, as opposed to
   more general errors beyond the application's control.  These
   functions do not return.  An error message is constructed using
   a printf- or vprintf-style argument list.  FILE and LINE
   indicate the file and line number where the programming error
   was detected.  Most client code should call the internal_error
   wrapper macro instead, which expands the source location
   automatically.  The function "internal_verror" must be provided
   by the client.  */

extern void internal_error_loc (const char *file, int line,
				const char *fmt, ...)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (3, 4);

#define internal_error(fmt, ...)				\
  internal_error_loc (__FILE__, __LINE__, fmt, ##__VA_ARGS__)

extern void internal_verror (const char *file, int line,
			     const char *fmt, va_list args)
     ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (3, 0);

/* An internal problem was detected, but the requested operation can
   still proceed.  Internal warnings indicate programming errors as
   opposed to more general issues beyond the application's control.
   A warning message is constructed using a printf- or vprintf-style
   argument list.  The function "internal_vwarning" must be provided
   by the client.  */

extern void internal_warning_loc (const char *file, int line,
				  const char *fmt, ...)
     ATTRIBUTE_PRINTF (3, 4);

#define internal_warning(fmt, ...)				\
  internal_warning_loc (__FILE__, __LINE__, fmt, ##__VA_ARGS__)

extern void internal_vwarning (const char *file, int line,
			       const char *fmt, va_list args)
     ATTRIBUTE_PRINTF (3, 0);


/* Return a newly allocated string, containing the PREFIX followed
   by the system error message for errno (separated by a colon).
   If ERRNUM is given, then use it in place of errno.  */

extern std::string perror_string (const char *prefix, int errnum = 0);

/* Like "error", but the error message is constructed by combining
   STRING with the system error message for errno.  If ERRNUM is given,
   then use it in place of errno.  This function does not return.  */

extern void perror_with_name (const char *string, int errnum = 0)
    ATTRIBUTE_NORETURN;

/* Call this function to handle memory allocation failures.  This
   function does not return.  This function must be provided by the
   client.  */

extern void malloc_failure (long size) ATTRIBUTE_NORETURN;

/* Flush stdout and stderr.  Must be provided by the client.  */

extern void flush_streams ();

#if defined(USE_WIN32API) || defined(__CYGWIN__)

/* Map the Windows error number in ERROR to a locale-dependent error
   message string and return a pointer to it.  Typically, the values
   for ERROR come from GetLastError.

   The string pointed to shall not be modified by the application,
   but may be overwritten by a subsequent call to strwinerror

   The strwinerror function does not change the current setting
   of GetLastError.  */

extern const char *strwinerror (ULONGEST error);

/* Like perror_with_name, but for Windows errors.  Throw an exception
   including STRING and the system text for the given error
   number.  */

extern void throw_winerror_with_name (const char *string, ULONGEST err)
  ATTRIBUTE_NORETURN;

#endif /* USE_WIN32API */

#endif /* COMMON_ERRORS_H */

/* Error reporting facilities.

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

#include "common-defs.h"
#include "errors.h"
#if defined (USE_WIN32API) || defined(__CYGWIN__)
#include <windows.h>
#endif /* USE_WIN32API */

/* See gdbsupport/errors.h.  */

void
warning (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vwarning (fmt, ap);
  va_end (ap);
}

/* See gdbsupport/errors.h.  */

void
error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  verror (fmt, ap);
  va_end (ap);
}

/* See gdbsupport/errors.h.  */

void
internal_error_loc (const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  internal_verror (file, line, fmt, ap);
  va_end (ap);
}

/* See gdbsupport/errors.h.  */

void
internal_warning_loc (const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  internal_vwarning (file, line, fmt, ap);
  va_end (ap);
}

/* See errors.h.  */

std::string
perror_string (const char *prefix, int errnum)
{
  const char *err;

  if (errnum != 0)
    err = safe_strerror (errnum);
  else
    err = safe_strerror (errno);
  return std::string (prefix) + ": " + err;
}

/* See errors.h.  */

void
perror_with_name (const char *string, int errnum)
{
  std::string combined = perror_string (string, errnum);

  error (_("%s."), combined.c_str ());
}

#if defined (USE_WIN32API) || defined(__CYGWIN__)

/* See errors.h.  */

const char *
strwinerror (ULONGEST error)
{
  static char buf[1024];
  TCHAR *msgbuf;
  DWORD lasterr = GetLastError ();
  DWORD chars = FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM
			       | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			       NULL,
			       error,
			       0, /* Default language */
			       (LPTSTR) &msgbuf,
			       0,
			       NULL);
  if (chars != 0)
    {
      /* If there is an \r\n appended, zap it.  */
      if (chars >= 2
	  && msgbuf[chars - 2] == '\r'
	  && msgbuf[chars - 1] == '\n')
	{
	  chars -= 2;
	  msgbuf[chars] = 0;
	}

      if (chars > ARRAY_SIZE (buf) - 1)
	{
	  chars = ARRAY_SIZE (buf) - 1;
	  msgbuf [chars] = 0;
	}

#ifdef UNICODE
      wcstombs (buf, msgbuf, chars + 1);
#else
      strncpy (buf, msgbuf, chars + 1);
#endif
      LocalFree (msgbuf);
    }
  else
    sprintf (buf, "unknown win32 error (%u)", (unsigned) error);

  SetLastError (lasterr);
  return buf;
}

/* See errors.h.  */

void
throw_winerror_with_name (const char *string, ULONGEST err)
{
  error (_("%s (error %d): %s"), string, err, strwinerror (err));
}

#endif /* USE_WIN32API */

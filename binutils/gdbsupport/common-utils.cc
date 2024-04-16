/* Shared general utility routines for GDB, the GNU debugger.

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
#include "common-utils.h"
#include "host-defs.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include "gdbsupport/gdb-xfree.h"

void *
xzalloc (size_t size)
{
  return xcalloc (1, size);
}

/* Like asprintf/vasprintf but get an internal_error if the call
   fails. */

gdb::unique_xmalloc_ptr<char>
xstrprintf (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  gdb::unique_xmalloc_ptr<char> ret = xstrvprintf (format, args);
  va_end (args);
  return ret;
}

gdb::unique_xmalloc_ptr<char>
xstrvprintf (const char *format, va_list ap)
{
  char *ret = NULL;
  int status = vasprintf (&ret, format, ap);

  /* NULL is returned when there was a memory allocation problem, or
     any other error (for instance, a bad format string).  A negative
     status (the printed length) with a non-NULL buffer should never
     happen, but just to be sure.  */
  if (ret == NULL || status < 0)
    internal_error (_("vasprintf call failed"));
  return gdb::unique_xmalloc_ptr<char> (ret);
}

int
xsnprintf (char *str, size_t size, const char *format, ...)
{
  va_list args;
  int ret;

  va_start (args, format);
  ret = vsnprintf (str, size, format, args);
  gdb_assert (ret < size);
  va_end (args);

  return ret;
}

/* See documentation in common-utils.h.  */

std::string
string_printf (const char* fmt, ...)
{
  va_list vp;
  int size;

  va_start (vp, fmt);
  size = vsnprintf (NULL, 0, fmt, vp);
  va_end (vp);

  std::string str (size, '\0');

  /* C++11 and later guarantee std::string uses contiguous memory and
     always includes the terminating '\0'.  */
  va_start (vp, fmt);
  vsprintf (&str[0], fmt, vp);	/* ARI: vsprintf */
  va_end (vp);

  return str;
}

/* See documentation in common-utils.h.  */

std::string
string_vprintf (const char* fmt, va_list args)
{
  va_list vp;
  size_t size;

  va_copy (vp, args);
  size = vsnprintf (NULL, 0, fmt, vp);
  va_end (vp);

  std::string str (size, '\0');

  /* C++11 and later guarantee std::string uses contiguous memory and
     always includes the terminating '\0'.  */
  vsprintf (&str[0], fmt, args); /* ARI: vsprintf */

  return str;
}


/* See documentation in common-utils.h.  */

std::string &
string_appendf (std::string &str, const char *fmt, ...)
{
  va_list vp;

  va_start (vp, fmt);
  string_vappendf (str, fmt, vp);
  va_end (vp);

  return str;
}


/* See documentation in common-utils.h.  */

std::string &
string_vappendf (std::string &str, const char *fmt, va_list args)
{
  va_list vp;
  int grow_size;

  va_copy (vp, args);
  grow_size = vsnprintf (NULL, 0, fmt, vp);
  va_end (vp);

  size_t curr_size = str.size ();
  str.resize (curr_size + grow_size);

  /* C++11 and later guarantee std::string uses contiguous memory and
     always includes the terminating '\0'.  */
  vsprintf (&str[curr_size], fmt, args); /* ARI: vsprintf */

  return str;
}

char *
savestring (const char *ptr, size_t len)
{
  char *p = (char *) xmalloc (len + 1);

  memcpy (p, ptr, len);
  p[len] = 0;
  return p;
}

/* See documentation in common-utils.h.  */

std::string
extract_string_maybe_quoted (const char **arg)
{
  bool squote = false;
  bool dquote = false;
  bool bsquote = false;
  std::string result;
  const char *p = *arg;

  /* Find the start of the argument.  */
  p = skip_spaces (p);

  /* Parse p similarly to gdb_argv buildargv function.  */
  while (*p != '\0')
    {
      if (ISSPACE (*p) && !squote && !dquote && !bsquote)
	break;
      else
	{
	  if (bsquote)
	    {
	      bsquote = false;
	      result += *p;
	    }
	  else if (*p == '\\')
	    bsquote = true;
	  else if (squote)
	    {
	      if (*p == '\'')
		squote = false;
	      else
		result += *p;
	    }
	  else if (dquote)
	    {
	      if (*p == '"')
		dquote = false;
	      else
		result += *p;
	    }
	  else
	    {
	      if (*p == '\'')
		squote = true;
	      else if (*p == '"')
		dquote = true;
	      else
		result += *p;
	    }
	  p++;
	}
    }

  *arg = p;
  return result;
}

/* The bit offset of the highest byte in a ULONGEST, for overflow
   checking.  */

#define HIGH_BYTE_POSN ((sizeof (ULONGEST) - 1) * HOST_CHAR_BIT)

/* True (non-zero) iff DIGIT is a valid digit in radix BASE,
   where 2 <= BASE <= 36.  */

static int
is_digit_in_base (unsigned char digit, int base)
{
  if (!ISALNUM (digit))
    return 0;
  if (base <= 10)
    return (ISDIGIT (digit) && digit < base + '0');
  else
    return (ISDIGIT (digit) || TOLOWER (digit) < base - 10 + 'a');
}

static int
digit_to_int (unsigned char c)
{
  if (ISDIGIT (c))
    return c - '0';
  else
    return TOLOWER (c) - 'a' + 10;
}

/* As for strtoul, but for ULONGEST results.  */

ULONGEST
strtoulst (const char *num, const char **trailer, int base)
{
  unsigned int high_part;
  ULONGEST result;
  int minus = 0;
  int i = 0;

  /* Skip leading whitespace.  */
  while (ISSPACE (num[i]))
    i++;

  /* Handle prefixes.  */
  if (num[i] == '+')
    i++;
  else if (num[i] == '-')
    {
      minus = 1;
      i++;
    }

  if (base == 0 || base == 16)
    {
      if (num[i] == '0' && (num[i + 1] == 'x' || num[i + 1] == 'X'))
	{
	  i += 2;
	  if (base == 0)
	    base = 16;
	}
    }

  if (base == 0 && num[i] == '0')
    base = 8;

  if (base == 0)
    base = 10;

  if (base < 2 || base > 36)
    {
      errno = EINVAL;
      return 0;
    }

  result = high_part = 0;
  for (; is_digit_in_base (num[i], base); i += 1)
    {
      result = result * base + digit_to_int (num[i]);
      high_part = high_part * base + (unsigned int) (result >> HIGH_BYTE_POSN);
      result &= ((ULONGEST) 1 << HIGH_BYTE_POSN) - 1;
      if (high_part > 0xff)
	{
	  errno = ERANGE;
	  result = ~ (ULONGEST) 0;
	  high_part = 0;
	  minus = 0;
	  break;
	}
    }

  if (trailer != NULL)
    *trailer = &num[i];

  result = result + ((ULONGEST) high_part << HIGH_BYTE_POSN);
  if (minus)
    return -result;
  else
    return result;
}

/* See documentation in common-utils.h.  */

char *
skip_spaces (char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && ISSPACE (*chp))
    chp++;
  return chp;
}

/* A const-correct version of the above.  */

const char *
skip_spaces (const char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && ISSPACE (*chp))
    chp++;
  return chp;
}

/* See documentation in common-utils.h.  */

const char *
skip_to_space (const char *chp)
{
  if (chp == NULL)
    return NULL;
  while (*chp && !ISSPACE (*chp))
    chp++;
  return chp;
}

/* See documentation in common-utils.h.  */

char *
skip_to_space (char *chp)
{
  return (char *) skip_to_space ((const char *) chp);
}

/* See gdbsupport/common-utils.h.  */

void
free_vector_argv (std::vector<char *> &v)
{
  for (char *el : v)
    xfree (el);

  v.clear ();
}

/* See gdbsupport/common-utils.h.  */

ULONGEST
align_up (ULONGEST v, int n)
{
  /* Check that N is really a power of two.  */
  gdb_assert (n && (n & (n-1)) == 0);
  return (v + n - 1) & -n;
}

/* See gdbsupport/common-utils.h.  */

ULONGEST
align_down (ULONGEST v, int n)
{
  /* Check that N is really a power of two.  */
  gdb_assert (n && (n & (n-1)) == 0);
  return (v & -n);
}

/* See gdbsupport/common-utils.h.  */

int
fromhex (int a)
{
  if (a >= '0' && a <= '9')
    return a - '0';
  else if (a >= 'a' && a <= 'f')
    return a - 'a' + 10;
  else if (a >= 'A' && a <= 'F')
    return a - 'A' + 10;
  else
    error (_("Invalid hex digit %d"), a);
}

/* See gdbsupport/common-utils.h.  */

int
hex2bin (const char *hex, gdb_byte *bin, int count)
{
  int i;

  for (i = 0; i < count; i++)
    {
      if (hex[0] == 0 || hex[1] == 0)
	{
	  /* Hex string is short, or of uneven length.
	     Return the count that has been converted so far.  */
	  return i;
	}
      *bin++ = fromhex (hex[0]) * 16 + fromhex (hex[1]);
      hex += 2;
    }
  return i;
}

/* See gdbsupport/common-utils.h.  */

gdb::byte_vector
hex2bin (const char *hex)
{
  size_t bin_len = strlen (hex) / 2;
  gdb::byte_vector bin (bin_len);

  hex2bin (hex, bin.data (), bin_len);

  return bin;
}

/* See gdbsupport/common-utils.h.  */

std::string
bytes_to_string (gdb::array_view<const gdb_byte> bytes)
{
  std::string ret;

  for (size_t i = 0; i < bytes.size (); i++)
    {
      if (i == 0)
	ret += string_printf ("%02x", bytes[i]);
      else
	ret += string_printf (" %02x", bytes[i]);
    }

  return ret;
}

/* Shared helper routines for manipulating XML.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "xml-utils.h"

/* See xml-utils.h.  */

std::string
xml_escape_text (const char *text)
{
  std::string result;

  xml_escape_text_append (result, text);

  return result;
}

/* See xml-utils.h.  */

void
xml_escape_text_append (std::string &result, const char *text)
{
  /* Expand the result.  */
  for (int i = 0; text[i] != '\0'; i++)
    switch (text[i])
      {
      case '\'':
	result += "&apos;";
	break;
      case '\"':
	result += "&quot;";
	break;
      case '&':
	result += "&amp;";
	break;
      case '<':
	result += "&lt;";
	break;
      case '>':
	result += "&gt;";
	break;
      default:
	result += text[i];
	break;
      }
}

/* See xml-utils.h.  */

void
string_xml_appendf (std::string &buffer, const char *format, ...)
{
  va_list ap;
  const char *f;
  const char *prev;
  int percent = 0;

  va_start (ap, format);

  prev = format;
  for (f = format; *f; f++)
    {
      if (percent)
	{
	  char buf[32];
	  char *str = buf;
	  const char *f_old = f;

	  switch (*f)
	    {
	    case 's':
	      str = va_arg (ap, char *);
	      break;
	    case 'd':
	      sprintf (str, "%d", va_arg (ap, int));
	      break;
	    case 'u':
	      sprintf (str, "%u", va_arg (ap, unsigned int));
	      break;
	    case 'x':
	      sprintf (str, "%x", va_arg (ap, unsigned int));
	      break;
	    case 'o':
	      sprintf (str, "%o", va_arg (ap, unsigned int));
	      break;
	    case 'l':
	      f++;
	      switch (*f)
		{
		case 'd':
		  sprintf (str, "%ld", va_arg (ap, long));
		  break;
		case 'u':
		  sprintf (str, "%lu", va_arg (ap, unsigned long));
		  break;
		case 'x':
		  sprintf (str, "%lx", va_arg (ap, unsigned long));
		  break;
		case 'o':
		  sprintf (str, "%lo", va_arg (ap, unsigned long));
		  break;
		case 'l':
		  f++;
		  switch (*f)
		    {
		    case 'd':
		      sprintf (str, "%" PRId64,
			       (int64_t) va_arg (ap, long long));
		      break;
		    case 'u':
		      sprintf (str, "%" PRIu64,
			       (uint64_t) va_arg (ap, unsigned long long));
		      break;
		    case 'x':
		      sprintf (str, "%" PRIx64,
			       (uint64_t) va_arg (ap, unsigned long long));
		      break;
		    case 'o':
		      sprintf (str, "%" PRIo64,
			       (uint64_t) va_arg (ap, unsigned long long));
		      break;
		    default:
		      str = 0;
		      break;
		    }
		  break;
		default:
		  str = 0;
		  break;
		}
	      break;
	    default:
	      str = 0;
	      break;
	    }

	  if (str)
	    {
	      buffer.append (prev, f_old - prev - 1);
	      xml_escape_text_append (buffer, str);
	      prev = f + 1;
	    }
	  percent = 0;
	}
      else if (*f == '%')
	percent = 1;
    }

  buffer.append (prev);
  va_end (ap);
}

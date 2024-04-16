/* Functions to deal with the inferior being executed on GDB or
   GDBserver.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/common-inferior.h"

/* See common-inferior.h.  */

bool startup_with_shell = true;

/* See common-inferior.h.  */

std::string
construct_inferior_arguments (gdb::array_view<char * const> argv)
{
  std::string result;

  if (startup_with_shell)
    {
#ifdef __MINGW32__
      /* This holds all the characters considered special to the
	 Windows shells.  */
      static const char special[] = "\"!&*|[]{}<>?`~^=;, \t\n";
      static const char quote = '"';
#else
      /* This holds all the characters considered special to the
	 typical Unix shells.  We include `^' because the SunOS
	 /bin/sh treats it as a synonym for `|'.  */
      static const char special[] = "\"!#$&*()\\|[]{}<>?'`~^; \t\n";
      static const char quote = '\'';
#endif
      for (int i = 0; i < argv.size (); ++i)
	{
	  if (i > 0)
	    result += ' ';

	  /* Need to handle empty arguments specially.  */
	  if (argv[i][0] == '\0')
	    {
	      result += quote;
	      result += quote;
	    }
	  else
	    {
#ifdef __MINGW32__
	      bool quoted = false;

	      if (strpbrk (argv[i], special))
		{
		  quoted = true;
		  result += quote;
		}
#endif
	      for (char *cp = argv[i]; *cp; ++cp)
		{
		  if (*cp == '\n')
		    {
		      /* A newline cannot be quoted with a backslash (it
			 just disappears), only by putting it inside
			 quotes.  */
		      result += quote;
		      result += '\n';
		      result += quote;
		    }
		  else
		    {
#ifdef __MINGW32__
		      if (*cp == quote)
#else
		      if (strchr (special, *cp) != NULL)
#endif
			result += '\\';
		      result += *cp;
		    }
		}
#ifdef __MINGW32__
	      if (quoted)
		result += quote;
#endif
	    }
	}
    }
  else
    {
      /* In this case we can't handle arguments that contain spaces,
	 tabs, or newlines -- see breakup_args().  */
      for (char *arg : argv)
	{
	  char *cp = strchr (arg, ' ');
	  if (cp == NULL)
	    cp = strchr (arg, '\t');
	  if (cp == NULL)
	    cp = strchr (arg, '\n');
	  if (cp != NULL)
	    error (_("can't handle command-line "
		     "argument containing whitespace"));
	}

      for (int i = 0; i < argv.size (); ++i)
	{
	  if (i > 0)
	    result += " ";
	  result += argv[i];
	}
    }

  return result;
}

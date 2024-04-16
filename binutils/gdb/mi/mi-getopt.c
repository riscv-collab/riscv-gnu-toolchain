/* MI Command Set - MI Option Parser.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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

#include "defs.h"
#include "mi-getopt.h"
/* See comments about mi_getopt and mi_getopt_silent in mi-getopt.h.
   When there is an unknown option, if ERROR_ON_UNKNOWN is true,
   throw an error, otherwise return -1.  */

static int
mi_getopt_1 (const char *prefix, int argc, const char *const *argv,
	     const struct mi_opt *opts, int *oind, const char **oarg,
	     int error_on_unknown)
{
  const char *arg;
  const struct mi_opt *opt;

  /* We assume that argv/argc are ok.  */
  if (*oind > argc || *oind < 0)
    internal_error (_("mi_getopt_long: oind out of bounds"));
  if (*oind == argc)
    return -1;
  arg = argv[*oind];
  /* ``--''? */
  if (strcmp (arg, "--") == 0)
    {
      *oind += 1;
      *oarg = NULL;
      return -1;
    }
  /* End of option list.  */
  if (arg[0] != '-')
    {
      *oarg = NULL;
      return -1;
    }
  /* Look the option up.  */
  for (opt = opts; opt->name != NULL; opt++)
    {
      if (strcmp (opt->name, arg + 1) != 0)
	continue;
      if (opt->arg_p)
	{
	  /* A non-simple oarg option.  */
	  if (argc < *oind + 2)
	    error (_("%s: Option %s requires an argument"), prefix, arg);
	  *oarg = argv[(*oind) + 1];
	  *oind = (*oind) + 2;
	  return opt->index;
	}
      else
	{
	  *oarg = NULL;
	  *oind = (*oind) + 1;
	  return opt->index;
	}
    }

  if (error_on_unknown)
    error (_("%s: Unknown option ``%s''"), prefix, arg + 1);
  else
    return -1;
}

int
mi_getopt (const char *prefix,
	   int argc, const char *const *argv,
	   const struct mi_opt *opts,
	   int *oind, const char **oarg)
{
  return mi_getopt_1 (prefix, argc, argv, opts, oind, oarg, 1);
}

int
mi_getopt_allow_unknown (const char *prefix, int argc,
			 const char *const *argv,
			 const struct mi_opt *opts, int *oind,
			 const char **oarg)
{
  return mi_getopt_1 (prefix, argc, argv, opts, oind, oarg, 0);
}

int 
mi_valid_noargs (const char *prefix, int argc, const char *const *argv)
{
  int oind = 0;
  const char *oarg;
  static const struct mi_opt opts[] =
    {
      { 0, 0, 0 }
    };

  if (mi_getopt (prefix, argc, argv, opts, &oind, &oarg) == -1)
    return 1;
  else
    return 0;
}

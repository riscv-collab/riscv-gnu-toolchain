/* Copyright 2016-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#ifndef OUTPUT_TXT
# define OUTPUT_TXT "output.txt"
#endif

static void
perror_and_exit (const char *s)
{
  perror (s);
  exit (1);
}

int
main (int argc, char **argv)
{
  int i;
  FILE *out;
  sigset_t sigset;
  int res;

  res = sigprocmask (0,  NULL, &sigset);
  if (res != 0)
    perror_and_exit ("sigprocmask");

  if (argc > 1)
    out = stdout;
  else
    {
      out = fopen (OUTPUT_TXT, "w");
      if (out == NULL)
	perror_and_exit ("fopen");
    }

  for (i = 1; i < NSIG; i++)
    {
      struct sigaction oldact;

      fprintf (out, "signal %d: ", i);

      res = sigaction (i, NULL, &oldact);
      if (res == -1 && errno == EINVAL)
	{
	  /* Some signal numbers in the range are invalid.  E.g.,
	     signals 32 and 33 on GNU/Linux.  */
	  fprintf (out, "invalid");
	}
      else if (res == -1)
	{
	  perror_and_exit ("sigaction");
	}
      else
	{
	  int m;

	  fprintf (out, "sigaction={sa_handler=");

	  if (oldact.sa_handler == SIG_DFL)
	    fprintf (out, "SIG_DFL");
	  else if (oldact.sa_handler == SIG_IGN)
	    fprintf (out, "SIG_IGN");
	  else
	    abort ();

	  fprintf (out, ", sa_mask=");
	  for (m = 1; m < NSIG; m++)
	    fprintf (out, "%c", sigismember (&oldact.sa_mask, m) ? '1' : '0');

	  fprintf (out, ", sa_flags=%d", oldact.sa_flags);

	  fprintf (out, "}, masked=%d", sigismember (&sigset, i));
	}
      fprintf (out, "\n");
    }

  if (out != stdout)
    fclose (out);

  return 0;
}

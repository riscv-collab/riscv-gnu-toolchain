/* This is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/* Default READMORE method.  */
#define READMORE_METHOD_DEFAULT 2

/* Default READMORE sleep time in miliseconds.  */
#define READMORE_SLEEP_DEFAULT 10

/* Helper function.  Initialize *METHOD according to environment variable
   READMORE_METHOD, and *SLEEP according to environment variable
   READMORE_SLEEP.  */

static void
init_readmore (int *method, unsigned int *sleep, FILE **log)
{
  char *env = getenv ("READMORE_METHOD");
  if (env == NULL)
    *method = READMORE_METHOD_DEFAULT;
  else if (strcmp (env, "1") == 0)
    *method = 1;
  else if (strcmp (env, "2") == 0)
    *method = 2;
  else
    /* Default.  */
    *method = READMORE_METHOD_DEFAULT;

  env = getenv ("READMORE_SLEEP");
  if (env == NULL)
    *sleep = READMORE_SLEEP_DEFAULT;
  else
    *sleep = atoi (env);

  env = getenv ("READMORE_LOG");
  if (env == NULL)
    *log = NULL;
  else
    *log = fopen (env, "w");
}

/* Wrap 'read', and modify it's behaviour using READ1 or READMORE style.  */

ssize_t
read (int fd, void *buf, size_t count)
{
  static ssize_t (*read2) (int fd, void *buf, size_t count) = NULL;
  static FILE *log;
  int readmore;
#ifdef READMORE
  readmore = 1;
#else
  readmore = 0;
#endif
  static int readmore_method;
  static unsigned int readmore_sleep;
  if (read2 == NULL)
    {
      /* Use setenv (v, "", 1) rather than unsetenv (v) to work around
         https://core.tcl-lang.org/tcl/tktview?name=67fd4f973a "incorrect
	 results of 'info exists' when unset env var in one interp and check
	 for existence from another interp".  */
      setenv ("LD_PRELOAD", "", 1);
      read2 = dlsym (RTLD_NEXT, "read");
      if (readmore)
	init_readmore (&readmore_method, &readmore_sleep, &log);
    }

  /* Only modify 'read' behaviour when reading from the terminal.  */
  if (isatty (fd) == 0)
    goto fallback;

  if (!readmore)
    {
      /* READ1.  Force read to return only one byte at a time.  */
      return read2 (fd, buf, 1);
    }

  if (readmore_method == 1)
    {
      /* READMORE, method 1.  Wait a little before doing a read.  */
      usleep (readmore_sleep * 1000);
      return read2 (fd, buf, count);
    }

  if (readmore_method == 2)
    {
      /* READMORE, method 2.  After doing a read, either return or wait
	 a little and do another read, and so on.  */
      ssize_t res, total;
      int iteration;
      int max_iterations = -1;

      total = 0;
      for (iteration = 1; ; iteration++)
	{
	  res = read2 (fd, (char *)buf + total, count - total);
	  if (log != NULL)
	    fprintf (log,
		     "READ (%d): fd: %d, COUNT: %zd, RES: %zd, ERRNO: %s\n",
		     iteration, fd, count - total, res,
		     res == -1 ? strerror (errno) : "none");
	  if (res == -1)
	    {
	      if (iteration == 1)
		{
		  /* Error on first read, report.  */
		  total = -1;
		  break;
		}

	      if (total > 0
		  && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO))
		{
		  /* Ignore error, but don't try anymore reading.  */
		  errno = 0;
		  break;
		}

	      /* Other error, report back.  */
	      total = -1;
	      break;
	    }

	  total += res;
	  if (total == count)
	    /* Buf full, no need to do any more reading.  */
	    break;

	  /* Handle end-of-file.  */
	  if (res == 0)
	    break;

	  if (iteration == max_iterations)
	    break;

	  usleep (readmore_sleep * 1000);
	}

      if (log)
	fprintf (log, "READ returning: RES: %zd, ERRNO: %s\n",
		 total, total == -1 ? strerror (errno) : "none");
      return total;
    }

 fallback:
  /* Fallback, regular read.  */
  return read2 (fd, buf, count);
}

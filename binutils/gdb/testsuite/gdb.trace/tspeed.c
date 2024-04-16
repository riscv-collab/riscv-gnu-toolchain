/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

/* This program tests tracepoint speed. It consists of two identical
   loops, which in normal execution will run for exactly the same
   amount of time. A tracepoint in the second loop will slow it down
   by some amount, and then the program will report the slowdown
   observed.  */

/* While primarily designed for the testsuite, it can also be used
   for interactive testing.  */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

int trace_speed_test (void);

/* We mark these globals as volatile so the speed-measuring loops
   don't get totally emptied out at high optimization levels.  */

volatile int globfoo, globfoo2, globfoo3;

volatile short globarr[80000];

int init_iters = 10 * 1000;

int iters;

int max_iters = 1000 * 1000 * 1000;

int numtps = 1;

unsigned long long now2, now3, now4, now5;
int total1, total2, idelta, mindelta, nsdelta;
int nspertp = 0;

/* Return CPU usage (both user and system - trap-based tracepoints use
   a bunch of system time).  */

unsigned long long
myclock ()
{
  struct timeval tm;
  gettimeofday (&tm, NULL);
  return (((unsigned long long) tm.tv_sec) * 1000000) + tm.tv_usec;
}

int
main(int argc, char **argv)
{
  int problem;

  iters = init_iters;

  while (1)
    {
      numtps = 1;  /* set pre-run breakpoint here */

      /* Keep trying the speed test, with more iterations, until
	 we get to a reasonable number.  */
      while ((problem = trace_speed_test()))
	{
	  /* If iteration isn't working, give up.  */
	  if (iters > max_iters)
	    {
	      printf ("Gone over %d iterations, giving up\n", max_iters);
	      break;
	    }
	  if (problem < 0)
	    {
	      printf ("Negative times, giving up\n");
	      break;
	    }

	  iters *= 2;
	  printf ("Doubled iterations to %d\n", iters);
	}

      printf ("Tracepoint time is %d ns\n", nspertp);

      /* This is for the benefit of interactive testing and attaching,
	 keeps the program from pegging the machine.  */
      sleep (1);  /* set post-run breakpoint here */

      /* Issue a little bit of output periodically, so we can see if
	 program is alive or hung.  */
      printf ("%s keeping busy, clock=%llu\n", argv[0], myclock ());
    }
  return 0;
}

int
trace_speed_test (void)
{
  int i;

  /* Overall loop run time deltas under 1 ms are likely noise and
     should be ignored.  */
  mindelta = 1000;

  // The bodies of the two loops following must be identical.

  now2 = myclock ();
  globfoo2 = 1;
  for (i = 0; i < iters; ++i)
    {
      globfoo2 *= 45;
      globfoo2 += globfoo + globfoo3; 
      globfoo2 *= globfoo + globfoo3; 
      globfoo2 -= globarr[4] + globfoo3; 
      globfoo2 *= globfoo + globfoo3; 
      globfoo2 += globfoo + globfoo3; 
    }
  now3 = myclock ();
  total1 = now3 - now2;

  now4 = myclock ();
  globfoo2 = 1;
  for (i = 0; i < iters; ++i)
    {
      globfoo2 *= 45;
      globfoo2 += globfoo + globfoo3;  /* set tracepoint here */
      globfoo2 *= globfoo + globfoo3; 
      globfoo2 -= globarr[4] + globfoo3; 
      globfoo2 *= globfoo + globfoo3; 
      globfoo2 += globfoo + globfoo3; 
    }
  now5 = myclock ();
  total2 = now5 - now4;

  /* Report on the test results.  */

  nspertp = 0;

  idelta = total2 - total1;

  printf ("Loops took %d usec and %d usec, delta is %d usec, %d iterations\n",
	  total1, total2, idelta, iters);

  /* If the second loop seems to run faster, things are weird so give up.  */
  if (idelta < 0)
    return -1;

  if (idelta > mindelta
      /* Total test time should be between 15 and 30 seconds.  */
      && (total1 + total2) > (15 * 1000000)
      && (total1 + total2) < (30 * 1000000))
    {
      nsdelta = (((unsigned long long) idelta) * 1000) / iters;
      printf ("Second loop took %d ns longer per iter than first\n", nsdelta);
      nspertp = nsdelta / numtps;
      printf ("%d ns per tracepoint\n", nspertp);
      printf ("Base iteration time %d ns\n",
	      ((int) (((unsigned long long) total1) * 1000) / iters));
      printf ("Total test time %d secs\n", ((int) ((now5 - now2) / 1000000)));

      /* Speed test ran with no problem.  */
      return 0;
    }

  /* The test run was too brief, or otherwise not useful.  */
  return 1;
}

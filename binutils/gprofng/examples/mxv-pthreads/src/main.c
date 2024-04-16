/* Copyright (C) 2021-2023 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/*
* -----------------------------------------------------------------------------
* This program implements the multiplication of an m by n matrix with a vector
* of length n.  The Posix Threads parallel programming model is used to
* parallelize the core matrix-vector multiplication algorithm.
* -----------------------------------------------------------------------------
*/

#include "mydefs.h"

int main (int argc, char **argv)
{
  bool verbose = false;

  thread_data *thread_data_arguments;
  pthread_t   *pthread_ids;

  int64_t remainder_rows;
  int64_t rows_per_thread;
  int64_t active_threads;

  int64_t number_of_rows;
  int64_t number_of_columns;
  int64_t number_of_threads;
  int64_t repeat_count;

  double  **A;
  double  *b;
  double  *c;
  double  *ref;

  int64_t errors;

/*
* -----------------------------------------------------------------------------
* Start the ball rolling - Get the user options and parse them.
* -----------------------------------------------------------------------------
*/
  (void) get_user_options (
			argc,
			argv,
			&number_of_rows,
			&number_of_columns,
			&repeat_count,
			&number_of_threads,
			&verbose);

  if (verbose) printf ("Verbose mode enabled\n");

/*
* -----------------------------------------------------------------------------
* Allocate storage for all data structures.
* -----------------------------------------------------------------------------
*/
  (void) allocate_data (
		number_of_threads, number_of_rows,
		number_of_columns, &A, &b, &c, &ref,
		&thread_data_arguments, &pthread_ids);

  if (verbose) printf ("Allocated data structures\n");

/*
* -----------------------------------------------------------------------------
* Initialize the data.
* -----------------------------------------------------------------------------
*/
  (void) init_data (number_of_rows, number_of_columns, A, b, c, ref);

  if (verbose) printf ("Initialized matrix and vectors\n");

/*
* -----------------------------------------------------------------------------
* Determine the main workload settings.
* -----------------------------------------------------------------------------
*/
  (void) get_workload_stats (
		number_of_threads, number_of_rows,
		number_of_columns, &rows_per_thread,
		&remainder_rows, &active_threads);

  if (verbose) printf ("Defined workload distribution\n");

  for (int64_t TID=active_threads; TID<number_of_threads; TID++)
    {
      thread_data_arguments[TID].do_work      = false;
    }
  for (int64_t TID=0; TID<active_threads; TID++)
    {
      thread_data_arguments[TID].thread_id    = TID;
      thread_data_arguments[TID].verbose      = verbose;
      thread_data_arguments[TID].do_work      = true;
      thread_data_arguments[TID].repeat_count = repeat_count;

      (void) determine_work_per_thread (
		TID, rows_per_thread, remainder_rows,
		&thread_data_arguments[TID].row_index_start,
		&thread_data_arguments[TID].row_index_end);

      thread_data_arguments[TID].m = number_of_rows;
      thread_data_arguments[TID].n = number_of_columns;
      thread_data_arguments[TID].b = b;
      thread_data_arguments[TID].c = c;
      thread_data_arguments[TID].A = A;
    }

  if (verbose) printf ("Assigned work to threads\n");

/*
* -----------------------------------------------------------------------------
* Create and execute the threads.  Note that this means that there will be
* <t+1> threads, with <t> the number of threads specified on the commandline,
* or the default if the -t option was not used.
*
* Per the pthread_create () call, the threads start executing right away.
* -----------------------------------------------------------------------------
*/
  for (int TID=0; TID<active_threads; TID++)
    {
      if (pthread_create (&pthread_ids[TID], NULL, driver_mxv,
	  (void *) &thread_data_arguments[TID]) != 0)
	{
	  printf ("Error creating thread %d\n", TID);
	  perror ("pthread_create"); exit (-1);
	}
      else
	{
	  if (verbose) printf ("Thread %d has been created\n", TID);
	}
    }
/*
* -----------------------------------------------------------------------------
* Wait for all threads to finish.
* -----------------------------------------------------------------------------
*/
  for (int TID=0; TID<active_threads; TID++)
    {
      pthread_join (pthread_ids[TID], NULL);
    }

  if (verbose)
    {
      printf ("Matrix vector multiplication has completed\n");
      printf ("Verify correctness of result\n");
    }

/*
* -----------------------------------------------------------------------------
* Check the numerical results.
* -----------------------------------------------------------------------------
*/
  if ((errors = check_results (number_of_rows, number_of_columns,
				c, ref)) == 0)
    {
      if (verbose) printf ("Error check passed\n");
    }
  else
    {
      printf ("Error: %ld differences in the results detected\n", errors);
    }

/*
* -----------------------------------------------------------------------------
* Print a summary of the execution.
* -----------------------------------------------------------------------------
*/
  print_all_results (number_of_rows, number_of_columns, number_of_threads,
		     errors);

/*
* -----------------------------------------------------------------------------
* Release the allocated memory and end execution.
* -----------------------------------------------------------------------------
*/
  free (A);
  free (b);
  free (c);
  free (ref);
  free (pthread_ids);

  return (0);
}

/*
* -----------------------------------------------------------------------------
* Parse user options and set variables accordingly.  In case of an error, print
* a message, but do not bail out yet.  In this way we can catch multiple input
* errors.
* -----------------------------------------------------------------------------
*/
int get_user_options (int argc, char *argv[],
		      int64_t *number_of_rows,
		      int64_t *number_of_columns,
		      int64_t *repeat_count,
		      int64_t *number_of_threads,
		      bool    *verbose)
{
  int      opt;
  int      errors		     = 0;
  int64_t  default_number_of_threads = 1;
  int64_t  default_rows		     = 2000;
  int64_t  default_columns	     = 3000;
  int64_t  default_repeat_count      = 200;
  bool     default_verbose	     = false;

  *number_of_rows    = default_rows;
  *number_of_columns = default_columns;
  *number_of_threads = default_number_of_threads;
  *repeat_count      = default_repeat_count;
  *verbose	     = default_verbose;

  while ((opt = getopt (argc, argv, "m:n:r:t:vh")) != -1)
    {
      switch (opt)
	{
	  case 'm':
	    *number_of_rows = atol (optarg);
	    break;
	  case 'n':
	    *number_of_columns = atol (optarg);
	    break;
	  case 'r':
	    *repeat_count = atol (optarg);
	    break;
	  case 't':
	    *number_of_threads = atol (optarg);
	    break;
	  case 'v':
	    *verbose = true;
	    break;
	  case 'h':
	  default:
	    printf ("Usage: %s " \
		"[-m <number of rows>] " \
		"[-n <number of columns] [-r <repeat count>] " \
		"[-t <number of threads] [-v] [-h]\n", argv[0]);
	    printf ("\t-m - number of rows, default = %ld\n",
		default_rows);
	    printf ("\t-n - number of columns, default = %ld\n",
		default_columns);
	    printf ("\t-r - the number of times the algorithm is " \
		"repeatedly executed, default = %ld\n",
		default_repeat_count);
	    printf ("\t-t - the number of threads used, default = %ld\n",
		default_number_of_threads);
	    printf ("\t-v - enable verbose mode, %s by default\n",
		(default_verbose) ? "on" : "off");
	    printf ("\t-h - print this usage overview and exit\n");

	   exit (0);
	   break;
	}
    }

/*
* -----------------------------------------------------------------------------
* Check for errors and bail out in case of problems.
* -----------------------------------------------------------------------------
*/
  if (*number_of_rows <= 0)
    {
      errors++;
      printf ("Error: The number of rows is %ld but should be strictly " \
	      "positive\n", *number_of_rows);
    }
  if (*number_of_columns <= 0)
    {
      errors++;
      printf ("Error: The number of columns is %ld but should be strictly " \
	      "positive\n", *number_of_columns);
    }
  if (*repeat_count <= 0)
    {
      errors++;
      printf ("Error: The repeat count is %ld but should be strictly " \
	      "positive\n", *repeat_count);
    }
  if (*number_of_threads <= 0)
    {
      errors++;
      printf ("Error: The number of threads is %ld but should be strictly " \
	      "positive\n", *number_of_threads);
    }
  if (errors != 0)
    {
      printf ("There are %d input error (s)\n", errors); exit (-1);
    }

  return (errors);
}

/*
* -----------------------------------------------------------------------------
* Print a summary of the execution status.
* -----------------------------------------------------------------------------
*/
void print_all_results (int64_t number_of_rows,
			int64_t number_of_columns,
			int64_t number_of_threads,
			int64_t errors)
{
  printf ("mxv: error check %s - rows = %ld columns = %ld threads = %ld\n",
	  (errors == 0) ? "passed" : "failed",
	  number_of_rows, number_of_columns, number_of_threads);
}

/*
* -----------------------------------------------------------------------------
* Check whether the computations produced the correct results.
* -----------------------------------------------------------------------------
*/
int64_t check_results (int64_t m, int64_t n, double *c, double *ref)
{
  char    *marker;
  int64_t errors = 0;
  double  relerr;
  double  TOL   = 100.0 * DBL_EPSILON;
  double  SMALL = 100.0 * DBL_MIN;

  if ((marker=(char *)malloc (m*sizeof (char))) == NULL)
    {
      perror ("array marker");
      exit (-1);
    }

  for (int64_t i=0; i<m; i++)
  {
    if (fabs (ref[i]) > SMALL)
      {
	relerr = fabs ((c[i]-ref[i])/ref[i]);
      }
    else
      {
	relerr = fabs ((c[i]-ref[i]));
      }
    if (relerr <= TOL)
      {
	marker[i] = ' ';
      }
    else
      {
	errors++;
	marker[i] = '*';
      }
  }
  if (errors > 0)
  {
    printf ("Found %ld differences in results for m = %ld n = %ld:\n",
		errors,m,n);
    for (int64_t i=0; i<m; i++)
      printf ("  %c c[%ld] = %f ref[%ld] = %f\n",marker[i],i,c[i],i,ref[i]);
  }

  return (errors);
}

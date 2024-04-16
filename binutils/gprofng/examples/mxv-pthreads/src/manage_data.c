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

#include "mydefs.h"

bool verbose;

/*
* -----------------------------------------------------------------------------
* This function allocates the data and sets up the data structures to be used
* in the remainder.
* -----------------------------------------------------------------------------
*/
void allocate_data (int active_threads,
		    int64_t number_of_rows,
		    int64_t number_of_columns,
		    double ***A,
		    double **b,
		    double **c,
		    double **ref,
		    thread_data **thread_data_arguments,
		    pthread_t **pthread_ids)
{
  if ((*b = (double *) malloc (number_of_columns * sizeof (double))) == NULL)
    {
      printf ("Error: allocation of vector b failed\n");
      perror ("vector b");
      exit (-1);
    }
  else
    {
      if (verbose) printf ("Vector b allocated\n");
    }

  if ((*c = (double *) malloc (number_of_rows * sizeof (double))) == NULL)
    {
      printf ("Error: allocation of vector c failed\n");
      perror ("vector c");
      exit (-1);
    }
  else
    {
      if (verbose) printf ("Vector c allocated\n");
    }

  if ((*ref = (double *) malloc (number_of_rows * sizeof (double))) == NULL)
    {
      printf ("Error: allocation of vector ref failed\n");
      perror ("vector ref");
      exit (-1);
    }

  if ((*A = (double **) malloc (number_of_rows * sizeof (double))) == NULL)
    {
      printf ("Error: allocation of matrix A failed\n");
      perror ("matrix A");
      exit (-1);
    }
  else
    {
      for (int64_t i=0; i<number_of_rows; i++)
	{
	  if (((*A)[i] = (double *) malloc (number_of_columns
					* sizeof (double))) == NULL)
	    {
	      printf ("Error: allocation of matrix A columns failed\n");
	      perror ("matrix A[i]");
	      exit (-1);
	    }
	}
      if (verbose) printf ("Matrix A allocated\n");
    }


  if ((*thread_data_arguments = (thread_data *) malloc ((active_threads)
				* sizeof (thread_data))) == NULL)
    {
      perror ("malloc thread_data_arguments");
      exit (-1);
    }
  else
    {
      if (verbose) printf ("Structure thread_data_arguments allocated\n");
    }

  if ((*pthread_ids = (pthread_t *) malloc ((active_threads)
				* sizeof (pthread_t))) == NULL)
    {
      perror ("malloc pthread_ids");
      exit (-1);
    }
  else
    {
      if (verbose) printf ("Structure pthread_ids allocated\n");
    }
}

/*
* -----------------------------------------------------------------------------
* This function initializes the data.
* -----------------------------------------------------------------------------
*/
void init_data (int64_t m,
		int64_t n,
		double **restrict A,
		double *restrict b,
		double *restrict c,
		double *restrict ref)
{

  (void) srand48 (2020L);

  for (int64_t j=0; j<n; j++)
    b[j] = 1.0;

  for (int64_t i=0; i<m; i++)
    {
      ref[i] = n*i;
      c[i]   = -2022;
      for (int64_t j=0; j<n; j++)
	A[i][j] = drand48 ();
    }

  for (int64_t i=0; i<m; i++)
    {
      double row_sum = 0.0;
      for (int64_t j=0; j<n; j++)
	row_sum += A[i][j];
      ref[i] = row_sum;
    }
}

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

/*
* -----------------------------------------------------------------------------
* Driver for the core computational part.
* -----------------------------------------------------------------------------
*/
void *driver_mxv (void *thread_arguments)
{
  thread_data *local_data;

  local_data = (thread_data *) thread_arguments;

  bool     do_work	  = local_data->do_work;
  int64_t  repeat_count   = local_data->repeat_count;
  int64_t row_index_start = local_data->row_index_start;
  int64_t row_index_end   = local_data->row_index_end;
  int64_t m		  = local_data->m;
  int64_t n		  = local_data->n;
  double  *b		  = local_data->b;
  double  *c		  = local_data->c;
  double  **A		  = local_data->A;

  if (do_work)
    {
      for (int64_t r=0; r<repeat_count; r++)
	{
	  (void)  mxv_core (row_index_start, row_index_end, m, n, A, b, c);
	}
    }

  return (0);
}

/*
* -----------------------------------------------------------------------------
* Computational heart of the algorithm.
*
* Disable inlining to avoid the repeat count loop is removed by the compiler.
* This is only done to make for a more interesting call tree.
* -----------------------------------------------------------------------------
*/
void __attribute__ ((noinline)) mxv_core (int64_t row_index_start,
					  int64_t row_index_end,
					  int64_t m,
					  int64_t n,
					  double **restrict A,
					  double *restrict b,
					  double *restrict c)
{
  for (int64_t i=row_index_start; i<=row_index_end; i++)
    {
      double row_sum = 0.0;
      for (int64_t j=0; j<n; j++)
	row_sum += A[i][j] * b[j];
      c[i] = row_sum;
    }
}

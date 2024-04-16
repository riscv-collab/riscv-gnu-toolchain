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
* This function determines the number of rows each thread will be working on
* and also how many threads will be active.
* -----------------------------------------------------------------------------
*/
void get_workload_stats (int64_t number_of_threads,
			 int64_t number_of_rows,
			 int64_t number_of_columns,
			 int64_t *rows_per_thread,
			 int64_t *remainder_rows,
			 int64_t *active_threads)
{
  if (number_of_threads <= number_of_rows)
    {
      *remainder_rows  = number_of_rows%number_of_threads;
      *rows_per_thread = (number_of_rows - (*remainder_rows))/number_of_threads;
    }
  else
    {
      *remainder_rows  = 0;
      *rows_per_thread = 1;
    }

  *active_threads = number_of_threads < number_of_rows
		? number_of_threads : number_of_rows;

  if (verbose)
    {
      printf ("Rows per thread = %ld remainder = %ld\n",
		*rows_per_thread, *remainder_rows);
      printf ("Number of active threads = %ld\n", *active_threads);
    }
}

/*
* -----------------------------------------------------------------------------
* This function determines which rows each thread will be working on.
* -----------------------------------------------------------------------------
*/
void determine_work_per_thread (int64_t TID, int64_t rows_per_thread,
				int64_t remainder_rows,
				int64_t *row_index_start,
				int64_t *row_index_end)
{
  int64_t chunk_per_thread;

  if (TID < remainder_rows)
    {
      chunk_per_thread = rows_per_thread + 1;
      *row_index_start = TID * chunk_per_thread;
      *row_index_end   = (TID + 1) * chunk_per_thread - 1;
    }
  else
    {
      chunk_per_thread = rows_per_thread;
      *row_index_start = remainder_rows * (rows_per_thread + 1)
			 + (TID - remainder_rows) * chunk_per_thread;
      *row_index_end   = remainder_rows * (rows_per_thread + 1)
			 + (TID - remainder_rows) * chunk_per_thread
			 + chunk_per_thread - 1;
    }

  if (verbose)
    {
      printf ("TID = %ld row_index_start = %ld row_index_end = %ld\n",
		TID, *row_index_start, *row_index_end);
    }
}

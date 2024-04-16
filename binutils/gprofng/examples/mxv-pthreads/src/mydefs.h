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

#ifndef ALREADY_INCLUDED
#define ALREADY_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <malloc.h>
#include <pthread.h>

struct thread_arguments_data {
	int     thread_id;
	bool    verbose;
	bool    do_work;
	int64_t repeat_count;
	int64_t row_index_start;
	int64_t row_index_end;
	int64_t m;
	int64_t n;
	double  *b;
	double  *c;
	double  **A;
};

typedef struct thread_arguments_data thread_data;

void *driver_mxv (void *thread_arguments);

void __attribute__ ((noinline)) mxv_core (int64_t row_index_start,
					  int64_t row_index_end,
					  int64_t m,
					  int64_t n,
					  double **restrict A,
					  double *restrict b,
					  double *restrict c);

int get_user_options (int     argc,
		      char    *argv[],
		      int64_t *number_of_rows,
		      int64_t *number_of_columns,
		      int64_t *repeat_count,
		      int64_t *number_of_threads,
		      bool    *verbose);

void init_data (int64_t m,
		int64_t n,
		double  **restrict A,
		double  *restrict b,
		double  *restrict c,
		double  *restrict ref);

void  allocate_data (int	 active_threads,
		     int64_t     number_of_rows,
		     int64_t     number_of_columns,
		     double      ***A,
		     double      **b,
		     double      **c,
		     double      **ref,
		     thread_data **thread_data_arguments,
		     pthread_t   **pthread_ids);

int64_t check_results (int64_t m,
		       int64_t n,
		       double *c,
		       double *ref);

void get_workload_stats (int64_t number_of_threads,
			 int64_t number_of_rows,
			 int64_t number_of_columns,
			 int64_t *rows_per_thread,
			 int64_t *remainder_rows,
			 int64_t *active_threads);

void determine_work_per_thread (int64_t TID,
				int64_t rows_per_thread,
				int64_t remainder_rows,
				int64_t *row_index_start,
				int64_t *row_index_end);

void mxv (int64_t m,
	  int64_t n,
	  double **restrict A,
	  double *restrict b,
	  double *restrict c);

void print_all_results (int64_t number_of_rows,
			int64_t number_of_columns,
			int64_t number_of_threads,
			int64_t errors);

extern bool verbose;

#endif

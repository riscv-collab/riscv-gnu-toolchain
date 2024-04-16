/* Simulator system call support.

   Copyright 2002-2024 Free Software Foundation, Inc.

   This file is part of simulators.

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

#ifndef SIM_SYSCALL_H
#define SIM_SYSCALL_H

struct cb_syscall;

/* Perform a syscall on the behalf of the target program.  The error/result are
   normalized into a single value (like a lot of operating systems do).  If you
   want the split values, see the other function below.

   Note: While cb_syscall requires you handle the exit syscall yourself, that is
   not the case with these helpers.

   Note: Types here match the gdb callback interface.  */
long sim_syscall (SIM_CPU *, int func, long arg1, long arg2, long arg3,
		  long arg4);

/* Same as sim_syscall, but return the split values by referenced.  */
void sim_syscall_multi (SIM_CPU *, int func, long arg1, long arg2, long arg3,
			long arg4, long *result, long *result2, int *errcode);

/* Simple memory callbacks for cb_syscall's read_mem/write_mem that assume
   cb_syscall's p1 and p2 are set to the SIM_DESC and SIM_CPU respectively.  */
int sim_syscall_read_mem (host_callback *, struct cb_syscall *, unsigned long,
			  char *, int);
int sim_syscall_write_mem (host_callback *, struct cb_syscall *, unsigned long,
			   const char *, int);

#endif

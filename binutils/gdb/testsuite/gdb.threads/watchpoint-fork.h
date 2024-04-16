/* Test case for forgotten hw-watchpoints after fork()-off of a process.

   Copyright 2012-2024 Free Software Foundation, Inc.

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
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* pthread_yield is a GNU extension.  */
#define _GNU_SOURCE

#ifdef THREAD
#include <pthread.h>

extern volatile int step;
extern pthread_t thread;
#endif /* THREAD */

extern volatile int var;

extern void marker (void);
extern void forkoff (int nr);

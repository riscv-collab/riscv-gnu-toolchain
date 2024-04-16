/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <pthread.h>

extern __thread void *so_extern;
extern __thread void *so_extern2;

static void *
tls_ptr (void *p)
{
   so_extern = &so_extern;
   so_extern2 = &so_extern2; /* break here to check result */

   return NULL;
}

int
main (void)
{
   pthread_t threads[2];

   tls_ptr (NULL);

   pthread_create (&threads[0], NULL, tls_ptr, NULL);
   pthread_create (&threads[1], NULL, tls_ptr, NULL);

   pthread_join (threads[0], NULL);
   pthread_join (threads[1], NULL);

   tls_ptr (NULL);

   return 0;
}


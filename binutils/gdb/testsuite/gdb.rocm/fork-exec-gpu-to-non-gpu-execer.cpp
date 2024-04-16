/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

#include <hip/hip_runtime.h>
#include <unistd.h>

#define CHECK(cmd)                                                           \
  {                                                                          \
    hipError_t error = cmd;                                                  \
    if (error != hipSuccess)                                                 \
      {                                                                      \
	fprintf (stderr, "error: '%s'(%d) at %s:%d\n",                       \
		 hipGetErrorString (error), error, __FILE__, __LINE__);      \
	exit (EXIT_FAILURE);                                                 \
      }                                                                      \
  }

__global__ static void
kernel1 ()
{}

__device__ static void
break_here_execer ()
{
}

__global__ static void
kernel2 ()
{
  break_here_execer ();
}

int
main ()
{
  /* Launch a first kernel to make sure the runtime is active by the time we
     call fork.  */
  kernel1<<<1, 1>>> ();

  /* fork + exec while the runtime is active.  */
  if (FORK () == 0)
    {
      int ret = execl (EXECEE, EXECEE, NULL);
      perror ("exec");
      abort ();
    }

  kernel2<<<1, 1>>> ();

  CHECK (hipDeviceSynchronize ());
  return 0;
}

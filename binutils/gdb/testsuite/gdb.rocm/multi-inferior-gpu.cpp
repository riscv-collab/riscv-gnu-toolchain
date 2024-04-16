/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <hip/hip_runtime.h>

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

__global__ void
kern ()
{
  asm ("s_sleep 1");
}

/* Spawn one child process per detected GPU.  */

static int
parent (int argc, char **argv)
{
  /* Identify how many GPUs we have, and spawn one child for each.  */
  int num_devices;
  CHECK (hipGetDeviceCount (&num_devices));

  /* Break here.  */

  for (int i = 0; i < num_devices; i++)
    {
      char n[32] = {};
      snprintf (n, sizeof (n), "%d", i);
      pid_t pid = fork ();
      if (pid == -1)
	{
	  perror ("Fork failed");
	  return -1;
	}

      if (pid == 0)
	{
	  /* Exec to force the child to re-initialize the ROCm runtime.  */
	  if (execl (argv[0], argv[0], n, nullptr) == -1)
	    {
	      perror ("Failed to exec");
	      return -1;
	    }
	}
    }

  /* Wait for all children.  */
  while (true)
    {
      int ws;
      pid_t ret = waitpid (-1, &ws, 0);
      if (ret == -1 && errno == ECHILD)
	break;
    }

  /* Last break here.  */
  return 0;
}

static int
child (int argc, char **argv)
{
  int dev_number;
  if (sscanf (argv[1], "%d", &dev_number) != 1)
    {
      fprintf (stderr, "Invalid argument \"%s\"\n", argv[1]);
      return -1;
    }

  CHECK (hipSetDevice (dev_number));
  kern<<<1, 1>>> ();
  CHECK (hipDeviceSynchronize ());
  return 0;
}

/* When called with no argument, identify how many AMDGPU devices are
   available on the system and spawn one worker process per GPU.  If a
   command-line argument is provided, it is the index of the GPU to use.  */

int
main (int argc, char **argv)
{
  if (argc <= 1)
    return parent (argc, argv);
  else
    return child (argc, argv);
}

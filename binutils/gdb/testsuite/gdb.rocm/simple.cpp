/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

#include "hip/hip_runtime.h"
#include <cassert>

__global__ void
do_an_addition (int a, int b, int *out)
{
  *out = a + b;
}

int
main ()
{
  int *result_ptr, result;

  /* Allocate memory for the device to write the result to.  */
  hipError_t error = hipMalloc (&result_ptr, sizeof (int));
  assert (error == hipSuccess);

  /* Run `do_an_addition` on one workgroup containing one work item.  */
  do_an_addition<<<dim3(1), dim3(1), 0, 0>>> (1, 2, result_ptr);

  /* Copy result from device to host.  Note that this acts as a synchronization
     point, waiting for the kernel dispatch to complete.  */
  error = hipMemcpyDtoH (&result, result_ptr, sizeof (int));
  assert (error == hipSuccess);

  printf ("result is %d\n", result);
  assert (result == 3);

  return 0;
}

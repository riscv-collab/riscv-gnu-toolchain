/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Ken Werner <ken.werner@de.ibm.com>  */

/* Simple OpenCL application that executes a kernel on the default device
   in a data parallel fashion.  The filename of the OpenCL program source
   should be specified using the CL_SOURCE define.  The name of the kernel
   routine is expected to be "testkernel".  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>
#include "cl_util.h"

#ifndef CL_SOURCE
#error "Please specify the OpenCL source file using the CL_SOURCE define"
#endif

#define STRINGIFY(S) _STRINGIFY(S)
#define _STRINGIFY(S) #S

#define SIZE 16

int
main ()
{
  int err, i;
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_context_properties context_props[3];
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;
  cl_mem buffer;

  size_t len;
  const char *program_source = NULL;
  char *device_extensions = NULL;
  char kernel_build_opts[256];
  size_t size = sizeof (cl_int) * SIZE;
  const size_t global_work_size[] = {SIZE, 0, 0}; /* size of each dimension */
  cl_int *data;

  /* In order to see which devices the OpenCL implementation on your platform
     provides you may issue a call to the print_clinfo () fuction.  */

  /* Initialize the data the OpenCl program operates on.  */
  data = (cl_int*) calloc (1, size);
  if (data == NULL)
    {
      fprintf (stderr, "calloc failed\n");
      exit (EXIT_FAILURE);
    }

  /* Pick the first platform.  */
  CHK (clGetPlatformIDs (1, &platform, NULL));
  /* Get the default device and create context.  */
  CHK (clGetDeviceIDs (platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL));
  context_props[0] = CL_CONTEXT_PLATFORM;
  context_props[1] = (cl_context_properties) platform;
  context_props[2] = 0;
  context = clCreateContext (context_props, 1, &device, NULL, NULL, &err);
  CHK_ERR ("clCreateContext", err);
  queue = clCreateCommandQueue (context, device, 0, &err);
  CHK_ERR ("clCreateCommandQueue", err);

  /* Query OpenCL extensions of that device.  */
  CHK (clGetDeviceInfo (device, CL_DEVICE_EXTENSIONS, 0, NULL, &len));
  device_extensions = (char *) malloc (len);
  CHK (clGetDeviceInfo (device, CL_DEVICE_EXTENSIONS, len, device_extensions,
			NULL));
  strcpy (kernel_build_opts, "-Werror -cl-opt-disable");
  if (strstr (device_extensions, "cl_khr_fp64") != NULL)
    strcpy (kernel_build_opts + strlen (kernel_build_opts),
	    " -D HAVE_cl_khr_fp64");
  if (strstr (device_extensions, "cl_khr_fp16") != NULL)
    strcpy (kernel_build_opts + strlen (kernel_build_opts),
	    " -D HAVE_cl_khr_fp16");

  /* Read the OpenCL kernel source into the main memory.  */
  program_source = read_file (STRINGIFY (CL_SOURCE), &len);
  if (program_source == NULL)
    {
      fprintf (stderr, "file does not exist: %s\n", STRINGIFY (CL_SOURCE));
      exit (EXIT_FAILURE);
    }

  /* Build the OpenCL kernel.  */
  program = clCreateProgramWithSource (context, 1, &program_source,
				       &len, &err);
  free ((void*) program_source);
  CHK_ERR ("clCreateProgramWithSource", err);
  err = clBuildProgram (program, 0, NULL, kernel_build_opts, NULL,
			NULL);
  if (err != CL_SUCCESS)
    {
      size_t len;
      char *clbuild_log = NULL;
      CHK (clGetProgramBuildInfo (program, device, CL_PROGRAM_BUILD_LOG, 0,
				  NULL, &len));
      clbuild_log = malloc (len);
      if (clbuild_log)
	{
	  CHK (clGetProgramBuildInfo (program, device, CL_PROGRAM_BUILD_LOG,
				      len, clbuild_log, NULL));
	  fprintf (stderr, "clBuildProgram failed with:\n%s\n", clbuild_log);
 	  free (clbuild_log);
        }
      exit (EXIT_FAILURE);
  }

  /* In some cases it might be handy to save the OpenCL program binaries to do
     further analysis on them.  In order to do so you may call the following
     function: save_program_binaries (program);.  */

  kernel = clCreateKernel (program, "testkernel", &err);
  CHK_ERR ("clCreateKernel", err);

  /* Setup the input data for the kernel.  */
  buffer = clCreateBuffer (context, CL_MEM_USE_HOST_PTR, size, data, &err);
  CHK_ERR ("clCreateBuffer", err);

  /* Execute the kernel (data parallel).  */
  CHK (clSetKernelArg (kernel, 0, sizeof (buffer), &buffer));
  CHK (clEnqueueNDRangeKernel (queue, kernel, 1, NULL, global_work_size, NULL,
			       0, NULL, NULL));

  /* Fetch the results (blocking).  */
  CHK (clEnqueueReadBuffer (queue, buffer, CL_TRUE, 0, size, data, 0, NULL,
			    NULL));

  /* Compare the results.  */
  for (i = 0; i < SIZE; i++)
    {
      if (data[i] != 0x1)
	{
	  fprintf (stderr, "error: data[%d]: %d != 0x1\n", i, data[i]);
	  exit (EXIT_FAILURE);
	}
    }

  /* Cleanup.  */
  CHK (clReleaseMemObject (buffer));
  CHK (clReleaseKernel (kernel));
  CHK (clReleaseProgram (program));
  CHK (clReleaseCommandQueue (queue));
  CHK (clReleaseContext (context));
  free (data);

  return 0;
}

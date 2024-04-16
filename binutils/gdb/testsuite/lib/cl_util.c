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

/* Utility macros and functions for OpenCL applications.  */

#include "cl_util.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

const char *get_clerror_string (int errcode)
{
  switch (errcode)
    {
    case CL_SUCCESS:
      return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
      return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
      return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
      return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
      return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
      return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
      return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
      return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
      return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
      return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
      return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
      return "CL_MAP_FAILURE";
    case CL_INVALID_VALUE:
      return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
      return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
      return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
      return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
      return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
      return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
      return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
      return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
      return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
      return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
      return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
      return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
      return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
      return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
      return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
      return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
      return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
      return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:
      return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
      return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
      return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
      return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
      return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
      return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
      return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
      return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
      return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
      return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
      return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
      return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
      return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:
      return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
      return "CL_INVALID_MIP_LEVEL";
#ifndef CL_PLATFORM_NVIDIA
    case CL_INVALID_GLOBAL_WORK_SIZE:
      return "CL_INVALID_GLOBAL_WORK_SIZE";
#endif
    default:
      return "Unknown";
    };
}


void print_clinfo ()
{
  char *s = NULL;
  size_t len;
  unsigned i, j;
  cl_uint platform_count;
  cl_platform_id *platforms;

  /* Determine number of OpenCL Platforms available.  */
  clGetPlatformIDs (0, NULL, &platform_count);
  printf ("number of OpenCL Platforms available:\t%d\n", platform_count);
  /* Get platforms.  */
  platforms
    = (cl_platform_id*) malloc (sizeof (cl_platform_id) * platform_count);
  if (platforms == NULL)
    {
      fprintf (stderr, "malloc failed\n");
      exit (EXIT_FAILURE);
    }
  clGetPlatformIDs (platform_count, platforms, NULL);

  /* Querying platforms.  */
  for (i = 0; i < platform_count; i++)
    {
      cl_device_id *devices;
      cl_uint device_count;
      cl_device_id default_dev;
      printf (" OpenCL Platform:                       %d\n", i);

#define PRINT_PF_INFO(PARM)\
      clGetPlatformInfo (platforms[i], PARM, 0, NULL, &len); \
      s = realloc (s, len); \
      clGetPlatformInfo (platforms[i], PARM, len, s, NULL); \
      printf ("  %-36s%s\n", #PARM ":", s);

      PRINT_PF_INFO (CL_PLATFORM_PROFILE)
      PRINT_PF_INFO (CL_PLATFORM_VERSION)
      PRINT_PF_INFO (CL_PLATFORM_NAME)
      PRINT_PF_INFO (CL_PLATFORM_VENDOR)
      PRINT_PF_INFO (CL_PLATFORM_EXTENSIONS)
#undef PRINT_PF_INFO

      clGetDeviceIDs (platforms[i], CL_DEVICE_TYPE_DEFAULT, 1, &default_dev,
		      NULL);
      clGetDeviceInfo (default_dev, CL_DEVICE_NAME, 0, NULL, &len);
      s = realloc (s, len);
      clGetDeviceInfo (default_dev, CL_DEVICE_NAME, len, s, NULL);
      printf ("  CL_DEVICE_TYPE_DEFAULT:             %s\n", s);

      /* Determine number of devices.  */
      clGetDeviceIDs (platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &device_count);
      printf ("\n  number of OpenCL Devices available:   %d\n", device_count);
      /* Get devices.  */
      devices = (cl_device_id*) malloc (sizeof (cl_device_id) * device_count);
      if (devices == NULL)
	{
	  fprintf (stderr, "malloc failed\n");
	  exit (EXIT_FAILURE);
	}
      clGetDeviceIDs (platforms[i], CL_DEVICE_TYPE_ALL, device_count, devices,
		      NULL);

      /* Querying devices.  */
      for (j = 0; j < device_count; j++)
	{
	  cl_device_type dtype;
	  cl_device_mem_cache_type mctype;
	  cl_device_local_mem_type mtype;
	  cl_device_fp_config fpcfg;
	  cl_device_exec_capabilities xcap;
	  cl_command_queue_properties qprops;
	  cl_bool clbool;
	  cl_uint cluint;
	  cl_ulong clulong;
	  size_t sizet;
	  size_t workitem_size[3];
	  printf ("   OpenCL Device:                       %d\n", j);

#define PRINT_DEV_INFO(PARM)\
	  clGetDeviceInfo (devices[j], PARM, 0, NULL, &len); \
	  s = realloc (s, len); \
	  clGetDeviceInfo (devices[j], PARM, len, s, NULL); \
	  printf ("    %-41s%s\n", #PARM ":", s);

	  PRINT_DEV_INFO (CL_DEVICE_NAME)
	  PRINT_DEV_INFO (CL_DRIVER_VERSION)
	  PRINT_DEV_INFO (CL_DEVICE_VENDOR)
	  clGetDeviceInfo (devices[j], CL_DEVICE_VENDOR_ID, sizeof (cluint),
			   &cluint, NULL);
	  printf ("    CL_DEVICE_VENDOR_ID:                     %d\n", cluint);

	  clGetDeviceInfo (devices[j], CL_DEVICE_TYPE, sizeof (dtype), &dtype, NULL);
	  if (dtype & CL_DEVICE_TYPE_CPU)
	    printf ("    CL_DEVICE_TYPE:                          CL_DEVICE_TYPE_CPU\n");
	  if (dtype & CL_DEVICE_TYPE_GPU)
	    printf ("    CL_DEVICE_TYPE:                          CL_DEVICE_TYPE_GPU\n");
	  if (dtype & CL_DEVICE_TYPE_ACCELERATOR)
	    printf ("    CL_DEVICE_TYPE:                          CL_DEVICE_TYPE_ACCELERATOR\n");
	  if (dtype & CL_DEVICE_TYPE_DEFAULT)
	    printf ("    CL_DEVICE_TYPE:                          CL_DEVICE_TYPE_DEFAULT\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MAX_CLOCK_FREQUENCY:           %d\n", cluint);

	  PRINT_DEV_INFO (CL_DEVICE_PROFILE)
	  PRINT_DEV_INFO (CL_DEVICE_EXTENSIONS)

	  clGetDeviceInfo (devices[j], CL_DEVICE_AVAILABLE, sizeof (clbool), &clbool, NULL);
	  if (clbool == CL_TRUE)
	    printf ("    CL_DEVICE_AVAILABLE:                     CL_TRUE\n");
	  else
	    printf ("    CL_DEVICE_AVAILABLE:                     CL_FALSE\n");
	  clGetDeviceInfo (devices[j], CL_DEVICE_ENDIAN_LITTLE, sizeof (clbool), &clbool, NULL);
	  if (clbool == CL_TRUE)
	    printf ("    CL_DEVICE_ENDIAN_LITTLE:                 CL_TRUE\n");
	  else
	    printf ("    CL_DEVICE_ENDIAN_LITTLE:                 CL_FALSE\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MAX_COMPUTE_UNITS:             %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof (sizet), &sizet, NULL);
	  printf ("    CL_DEVICE_MAX_WORK_GROUP_SIZE:           %d\n", sizet);
	  clGetDeviceInfo (devices[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:      %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof (workitem_size), &workitem_size, NULL);
	  printf ("    CL_DEVICE_MAX_WORK_ITEM_SIZES:           %d / %d / %d\n", workitem_size[0], workitem_size[1], workitem_size[2]);

	  clGetDeviceInfo (devices[j], CL_DEVICE_ADDRESS_BITS, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_ADDRESS_BITS:                  %d\n", cluint);

	  clGetDeviceInfo (devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof (clulong), &clulong, NULL);
	  printf ("    CL_DEVICE_MAX_MEM_ALLOC_SIZE:            %llu\n", clulong);
	  clGetDeviceInfo (devices[j], CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MEM_BASE_ADDR_ALIGN:           %d\n", cluint);
	  clGetDeviceInfo(devices[j], CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:      %d\n", cluint);
	  clGetDeviceInfo(devices[j], CL_DEVICE_MAX_PARAMETER_SIZE, sizeof (sizet), &sizet, NULL);
	  printf ("    CL_DEVICE_MAX_PARAMETER_SIZE:            %d\n", sizet);
	  clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof (clulong), &clulong, NULL);
	  printf ("    CL_DEVICE_GLOBAL_MEM_SIZE:               %llu\n", clulong);

	  clGetDeviceInfo (devices[j], CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, sizeof (mctype), &mctype, NULL);
	  if (mctype & CL_NONE)
	    printf ("    CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:         CL_NONE\n");
	  if (mctype & CL_READ_ONLY_CACHE)
	    printf ("    CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:         CL_READ_ONLY_CACHE\n");
	  if (mctype & CL_READ_WRITE_CACHE)
	    printf ("    CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:         CL_READ_WRITE_CACHE\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof (clulong), &clulong, NULL);
	  printf ("    CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:         %llu\n", clulong);
	  clGetDeviceInfo (devices[j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:     %d\n", cluint);

	  clGetDeviceInfo (devices[j], CL_DEVICE_LOCAL_MEM_TYPE, sizeof (mtype), &mtype, NULL);
	  if (mtype & CL_LOCAL)
	    printf ("    CL_DEVICE_LOCAL_MEM_TYPE:                CL_LOCAL\n");
	  if (mtype & CL_GLOBAL)
	    printf ("    CL_DEVICE_LOCAL_MEM_TYPE:                CL_GLOBAL\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:      %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_MEM_BASE_ADDR_ALIGN:           %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:   %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:  %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:    %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:   %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:  %d\n", cluint);
	  clGetDeviceInfo (devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof (cluint), &cluint, NULL);
	  printf ("    CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: %d\n", cluint);

	  clGetDeviceInfo (devices[j], CL_DEVICE_SINGLE_FP_CONFIG, sizeof (fpcfg), &fpcfg, NULL);
	  if (fpcfg & CL_FP_DENORM)
	    printf ("    CL_DEVICE_SINGLE_FP_CONFIG:              CL_FP_DENORM\n");
	  if (fpcfg & CL_FP_INF_NAN)
	    printf ("    CL_DEVICE_SINGLE_FP_CONFIG:              CL_FP_INF_NAN\n");
	  if (fpcfg & CL_FP_ROUND_TO_NEAREST)
	    printf ("    CL_DEVICE_SINGLE_FP_CONFIG:              CL_FP_ROUND_TO_NEAREST\n");
	  if (fpcfg & CL_FP_ROUND_TO_ZERO)
	    printf ("    CL_DEVICE_SINGLE_FP_CONFIG:              CL_FP_ROUND_TO_ZERO\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_EXECUTION_CAPABILITIES, sizeof (xcap), &xcap, NULL);
	  if (xcap & CL_EXEC_KERNEL )
	    printf ("    CL_DEVICE_EXECUTION_CAPABILITIES:        CL_EXEC_KERNEL\n");
	  if (xcap & CL_EXEC_NATIVE_KERNEL)
	    printf ("    CL_DEVICE_EXECUTION_CAPABILITIES:        CL_EXEC_NATIVE_KERNEL\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_QUEUE_PROPERTIES, sizeof (qprops), &qprops, NULL);
	  if (qprops & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
	    printf ("    CL_DEVICE_QUEUE_PROPERTIES:              CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE\n");
	  if (qprops & CL_QUEUE_PROFILING_ENABLE)
	    printf ("    CL_DEVICE_QUEUE_PROPERTIES:              CL_QUEUE_PROFILING_ENABLE\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_PROFILING_TIMER_RESOLUTION, sizeof (sizet), &sizet, NULL);
	  printf ("    CL_DEVICE_PROFILING_TIMER_RESOLUTION:    %d\n", sizet);

	  clGetDeviceInfo (devices[j], CL_DEVICE_COMPILER_AVAILABLE, sizeof (clbool), &clbool, NULL);
	  if (clbool == CL_TRUE)
	    printf ("    CL_DEVICE_COMPILER_AVAILABLE:            CL_TRUE\n");
	  else
	    printf ("    CL_DEVICE_COMPILER_AVAILABLE:            CL_FALSE\n");
	  clGetDeviceInfo (devices[j], CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof (clbool), &clbool, NULL);
	  if (clbool == CL_TRUE)
	    printf ("    CL_DEVICE_ERROR_CORRECTION_SUPPORT:      CL_TRUE\n");
	  else
	    printf ("    CL_DEVICE_ERROR_CORRECTION_SUPPORT:      CL_FALSE\n");

	  clGetDeviceInfo (devices[j], CL_DEVICE_IMAGE_SUPPORT, sizeof (clbool), &clbool, NULL);
	  if (clbool == CL_FALSE)
	    {
	      printf ("    CL_DEVICE_IMAGE_SUPPORT:                 CL_FALSE\n");
	    }
	  else
	    {
	      printf ("    CL_DEVICE_IMAGE_SUPPORT:                 CL_TRUE\n");
	      clGetDeviceInfo (devices[j], CL_DEVICE_MAX_SAMPLERS, sizeof (cluint), &cluint, NULL);
	      printf ("    CL_DEVICE_MAX_SAMPLERS:                  %d\n", cluint);
	      clGetDeviceInfo (devices[j], CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof (cluint), &cluint, NULL);
	      printf ("    CL_DEVICE_MAX_READ_IMAGE_ARGS:           %d\n", cluint);
	      clGetDeviceInfo (devices[j], CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof (cluint), &cluint, NULL);
	      printf ("    CL_DEVICE_MAX_WRITE_IMAGE_ARGS:          %d\n", cluint);
	      clGetDeviceInfo (devices[j], CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof (sizet), &sizet, NULL);
	      printf ("    CL_DEVICE_IMAGE2D_MAX_WIDTH:             %d\n", sizet);
	      clGetDeviceInfo (devices[j], CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof (sizet), &sizet, NULL);
	      printf ("    CL_DEVICE_IMAGE2D_MAX_HEIGHT:            %d\n", sizet);
	      clGetDeviceInfo (devices[j], CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof (sizet), &sizet, NULL);
	      printf ("    CL_DEVICE_IMAGE3D_MAX_WIDTH:             %d\n", sizet);
	      clGetDeviceInfo (devices[j], CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof (sizet), &sizet, NULL);
	      printf ("    CL_DEVICE_IMAGE3D_MAX_HEIGHT:            %d\n", sizet);
	      clGetDeviceInfo (devices[j], CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof (sizet), &sizet, NULL);
	      printf ("    CL_DEVICE_IMAGE3D_MAX_DEPTH:             %d\n", sizet);
	    }
#undef PRINT_DEV_INFO
	} /* devices */
      free (devices);
    } /* platforms */
  free (s);
  free (platforms);
}


const char *
read_file (const char * const filename, size_t *size)
{
  char *buf = NULL;
  FILE *fd;
  struct stat st;
  if (stat (filename, &st) == -1)
    {
      /* Check if the file exists.  */
      if (errno == ENOENT)
	return buf;
      perror ("stat failed");
      exit (EXIT_FAILURE);
    }
  buf = (char *) malloc (st.st_size);
  if (buf == NULL)
    {
      fprintf (stderr, "malloc failed\n");
      exit (EXIT_FAILURE);
    }
  fd = fopen (filename, "r");
  if (fd == NULL)
    {
      perror ("fopen failed");
      free (buf);
      exit (EXIT_FAILURE);
    }
  if (fread (buf, st.st_size, 1, fd) != 1)
    {
      fprintf (stderr, "fread failed\n");
      free (buf);
      fclose (fd);
      exit (EXIT_FAILURE);
    }
  fclose (fd);
  *size = st.st_size;
  return buf;
}


void
save_program_binaries (cl_program program)
{
  cl_device_id *devices;
  cl_uint device_count;
  size_t *sizes;
  unsigned char **binaries;
  unsigned i, j;

  /* Query the amount of devices for the given program.  */
  CHK (clGetProgramInfo (program, CL_PROGRAM_NUM_DEVICES, sizeof (cl_uint),
			&device_count, NULL));

  /* Get the sizes of the binaries.  */
  sizes = (size_t*) malloc (sizeof (size_t) * device_count);
  if (sizes == NULL)
    {
      fprintf (stderr, "malloc failed\n");
      exit (EXIT_FAILURE);
    }
  CHK (clGetProgramInfo (program, CL_PROGRAM_BINARY_SIZES, sizeof (sizes),
			 sizes, NULL));

  /* Get the binaries.  */
  binaries
    = (unsigned char **) malloc (sizeof (unsigned char *) * device_count);
  if (binaries == NULL)
    {
      fprintf (stderr, "malloc failed\n");
      exit (EXIT_FAILURE);
    }
  for (i = 0; i < device_count; i++)
    {
      binaries[i] = (unsigned char *) malloc (sizes[i]);
      if (binaries[i] == NULL)
	{
	  fprintf (stderr, "malloc failed\n");
	  exit (EXIT_FAILURE);
	}
    }
  CHK (clGetProgramInfo (program, CL_PROGRAM_BINARIES, sizeof (binaries),
			 binaries, NULL));

  /* Get the devices for the given program to extract the file names.  */
  devices = (cl_device_id*) malloc (sizeof (cl_device_id) * device_count);
  if (devices == NULL)
    {
      fprintf (stderr, "malloc failed\n");
      exit (EXIT_FAILURE);
    }
  CHK (clGetProgramInfo (program, CL_PROGRAM_DEVICES, sizeof (devices),
			 devices, NULL));

  for (i = 0; i < device_count; i++)
    {
      FILE *fd;
      char *dev_name = NULL;
      size_t len;
      CHK (clGetDeviceInfo (devices[i], CL_DEVICE_NAME, 0, NULL, &len));
      dev_name = malloc (len);
      if (dev_name == NULL)
	{
	  fprintf (stderr, "malloc failed\n");
	  exit (EXIT_FAILURE);
	}
      CHK (clGetDeviceInfo (devices[i], CL_DEVICE_NAME, len, dev_name, NULL));
      /* Convert spaces to underscores.  */
      for (j = 0; j < strlen (dev_name); j++)
	{
	  if (dev_name[j] == ' ')
	    dev_name[j] = '_';
	}

      /*  Save the binaries.  */
      printf ("saving program binary for device: %s\n", dev_name);
      /* Save binaries[i].  */
      fd = fopen (dev_name, "w");
      if (fd == NULL)
	{
	  perror ("fopen failed");
	  exit (EXIT_FAILURE);
	}
      if (fwrite (binaries[i], sizes[i], 1, fd) != 1)
	{
	  fprintf (stderr, "fwrite failed\n");
	  for (j = i; j < device_count; j++)
	    free (binaries[j]);
	  fclose (fd);
	  exit (EXIT_FAILURE);
	}
      fclose (fd);
      free (binaries[i]);
      free (dev_name);
      free (sizes);
    }
  free (devices);
  free (binaries);
}

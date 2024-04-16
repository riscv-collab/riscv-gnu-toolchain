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

#ifndef CL_UTIL_H
#define CL_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <stdio.h>

/* Executes the given OpenCL function and checks its return value.
   In case of failure (rc != CL_SUCCESS) an error string will be
   printed to stderr and the program will be terminated.  This Macro
   is only intended for OpenCL routines which return cl_int.  */

#define CHK(func)\
{\
  int rc = (func);\
  CHK_ERR (#func, rc);\
}

/* Macro that checks an OpenCL error code.  In case of failure
   (err != CL_SUCCESS) an error string will be printed to stderr
   including the prefix and the program will be terminated.  This
   Macro is only intended to use in conjunction with OpenCL routines
   which take a pointer to a cl_int as an argument to place their
   error code.  */

#define CHK_ERR(prefix, err)\
if (err != CL_SUCCESS)\
  {\
    fprintf (stderr, "CHK_ERR (%s, %d)\n", prefix, err);\
    fprintf (stderr, "%s:%d error: %s\n", __FILE__, __LINE__,\
	     get_clerror_string (err));\
    exit (EXIT_FAILURE);\
  };

/* Return a pointer to a string that describes the error code specified
   by the errcode argument.  */

extern const char *get_clerror_string (int errcode);

/* Prints OpenCL information to stdout.  */

extern void print_clinfo ();

/* Reads a given file into the memory and returns a pointer to the data or NULL
   if the file does not exist.  FILENAME specifies the location of the file to
   be read.  SIZE is an output parameter that returns the size of the  file in
   bytes.  */

extern const char *read_file (const char * const filename, size_t *size);

/* Saves all program binaries of the given OpenCL PROGRAM.  The file
   names are extracted from the devices.  */

extern void save_program_binaries (cl_program program);

#ifdef __cplusplus
}
#endif

#endif /* CL_UTIL_H */

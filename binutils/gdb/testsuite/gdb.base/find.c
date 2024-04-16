/* Testcase for the find command.
   This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

   Please email any bugs, comments, and/or additions to this file to:
   bug-gdb@gnu.org  */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* According to C99 <stdint.h> has to provide these identifiers as
   types, but is also free to define macros shadowing the typedefs.
   This is the case with some C library implementations.  Undefine
   them to make sure the types are used and included in debug output.  */
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t

#define CHUNK_SIZE 16000 /* same as findcmd.c's */
#define BUF_SIZE (2 * CHUNK_SIZE) /* at least two chunks */

static int8_t int8_search_buf[100];
static int16_t int16_search_buf[100];
static int32_t int32_search_buf[100];
static int64_t int64_search_buf[100];

static char *search_buf;
static int search_buf_size;

static int x;

static void
stop_here ()
{
  x = 1; // stop here
}

static void
init_bufs ()
{
  search_buf_size = BUF_SIZE;
  search_buf = (char *) malloc (search_buf_size);
  if (search_buf == NULL)
    exit (1);
  memset (search_buf, 'x', search_buf_size);
}

int
main ()
{
  init_bufs ();

  stop_here ();

  /* Reference variables.  */
  x = int8_search_buf[0];
  x = int16_search_buf[0];
  x = int32_search_buf[0];
  x = int64_search_buf[0];

  return 0;
}

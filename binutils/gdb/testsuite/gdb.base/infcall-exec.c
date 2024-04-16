/* This test program is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static void
set_path (int argc, char ** argv)
{
  if (argc < 1)
    return;

  char path[PATH_MAX];
  strcpy (path, argv[0]);
  int len = strlen (path);

  /* Make a path name out of an exec name.  */
  int i;
  for (i = len - 1; i >= 0; i--)
    {
      char c = path[i];
      if (c == '/' || c == '\\')
	{
	  path[i] = '\0';
	  break;
	}
    }
  len = i;

  if (len == 0)
    return;

  /* Prefix with "PATH=".  */
  const char *prefix = "PATH=";
  int prefix_len = strlen (prefix);
  memmove (path + prefix_len, path, len);
  path[prefix_len + len] = '\0';
  memcpy (path, prefix, prefix_len);

  printf ("PATH SETTING: '%s'\n", path);
  putenv (path);
}

int
main (int argc, char ** argv)
{
  set_path (argc, argv);
  const char *prog = "infcall-exec2";

  int res = execlp (prog, prog, (char *) 0); /* break here */
  assert (res != -1);

  return 0;
}

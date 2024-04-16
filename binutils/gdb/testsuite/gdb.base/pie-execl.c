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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

static void pie_execl_marker (void);

int
main (int argc, char **argv)
{
  setbuf (stdout, NULL);

#if BIN == 1
  if (argc == 2)
    {
      printf ("pie-execl: re-exec: %s\n", argv[1]);
      execl (argv[1], argv[1], NULL);
      assert (0);
    }
#endif

  pie_execl_marker ();

  return 0;
}

/* pie_execl_marker must be on a different address than in `pie-execl2.c'.  */

volatile int v;

static void
pie_execl_marker (void)
{
  v = 1;
}

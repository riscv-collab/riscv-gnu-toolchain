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

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

volatile int dont_exit_just_yet = 1;

#define XSTR(s) STR(s)
#define STR(s) #s

volatile int with_checkpoint = 0;

int
main ()
{
  alarm (300);

  if (with_checkpoint)
    {
      /* We open a file and move file pos from 0 to 1.  We set the checkpoint
	 when pos is 0, and restart it when pos is 1.  This makes sure that
	 restarting the checkpoint excercises calling lseek in the
	 inferior. */

      /* Open a file. */
      const char *filename = XSTR (BINFILE);
      FILE *fp = fopen (filename, "r");

      volatile int checkpoint_here = 0; /* Checkpoint here.  */

      if (fp != NULL)
	{
	  /* Move file pos from 0 to 1.  */
	  int res = fseek (fp, 1, SEEK_SET);
	  assert (res == 0);
	}
    }

  /* Spin until GDB releases us.  */
  while (dont_exit_just_yet)
    usleep (100000);

  _exit (0);
}

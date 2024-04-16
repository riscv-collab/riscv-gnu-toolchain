/* This testcase is part of GDB, the GNU debugger.

   Copyright 2005-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

long lines = 0;

int main()
{
  char linebuf[128];
  FILE *in, *out;
  char *tmp = &linebuf[0];
  long i;
  int c = 0;

  in  = fopen (PI_TXT, "r");
  out = fopen (COPY1_TXT, "w");

  if (!in || !out)
    {
      fprintf (stderr, "File open failed\n");
      return 1;
    }

  for (i = 0; ; i++)
    {
      if (ftell (in) != i)
	fprintf (stderr, "Input error at %ld\n", i);
      if (ftell (out) != i)
	fprintf (stderr, "Output error at %ld\n", i);
      c = fgetc (in);
      if (c == '\n')
	lines++;	/* breakpoint 1 */
      if (c == EOF)
	break;
      fputc (c, out);
    }
  printf ("Copy complete.\n");	/* breakpoint 2 */
  fclose (in);
  fclose (out);
  printf ("Deleting copy.\n");	/* breakpoint 3 */
  unlink (COPY1_TXT);
  return 0;			/* breakpoint 4 */
}

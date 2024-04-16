/* This testcase is part of GDB, the GNU debugger.

   Copyright 1997-2024 Free Software Foundation, Inc.

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

#include <limits.h>
#include <string.h>

int
main (int argc, char ** argv)
{
  int  pid;

  /* A statement before vfork to make sure a breakpoint on main isn't
     set on vfork below.  */
  pid = 1 + argc;
  pid = vfork (); /* VFORK */
  if (pid == 0) {
    char prog[PATH_MAX];
    int len;

    strcpy (prog, argv[0]);
    len = strlen (prog);
    /* Replace "foll-vfork" with "vforked-prog".  */
    memcpy (prog + len - 10, "vforked-prog", 12);
    prog[len + 2] = 0;

    execlp (prog, prog, (char *) 0);
    perror ("exec failed");
    _exit (1);
  }
  else {
    const char *volatile s = "I'm the proud parent of child";
  }
}

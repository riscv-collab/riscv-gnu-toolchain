/* Copyright 2013-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

/* The size of these variables is chosen so that gold will add some padding
   to the TLS program header (total size of 16 bytes on x86_64) which strip
   will remove (bringing it down to 9 bytes).  */

__thread long j;
__thread char i;

void
break_here (void)
{
  *(volatile int *) 0 = 0;
}

void
foo (void)
{
  break_here ();
}

void
bar (void)
{
  foo ();
}

int
main (void)
{
  bar ();
  return 0;
}

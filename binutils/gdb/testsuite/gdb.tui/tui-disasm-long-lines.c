/* This testcase is part of GDB, the GNU debugger.

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

#define LONGER_NAME(x) x ## x
#define LONGER(x) LONGER_NAME(x)
#define LONGNAME1 d_this_identifier_of_32_chars_an
#define LONGNAME2 LONGER (LONGER (LONGER (LONGER (LONGER (LONGNAME1)))))

/* Construct a long identifier name.  If SHORT_IDENTIFIERS is set, limit
   it to 1024 chars.  */

#ifdef SHORT_IDENTIFIERS
#define LONGNAME3 LONGNAME2
#else
#define LONGNAME3 LONGER (LONGER (LONGER (LONGER (LONGER (LONGNAME2)))))
#endif

void LONGNAME3 (void);

int
main ()
{
  LONGNAME3 ();
  return 0;
}

/* Function with a long name.  Placing it after main makes it more likely
   to be shown in the disassembly window on startup.  */

void
LONGNAME3 (void)
{
}

/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

#ifdef TYPE_NOSPACE_long_long
#define TYPE long long
#elif defined TYPE_NOSPACE_signed_char
#define TYPE signed char
#else
#define TYPE TYPE_NOSPACE
#endif

extern TYPE func (void);

static void
marker (void)
{
}

TYPE t;

int
main (void)
{
  t = func ();

  marker ();

  return 0;
}

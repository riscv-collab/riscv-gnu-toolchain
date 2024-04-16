/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

#include <wchar.h>

void
do_nothing (wchar_t *c)
{
}

int
main (void)
{
  int i;
  wchar_t narrow = 97;
  wchar_t single = 0xbeef;
  wchar_t simple[] = L"facile";
  wchar_t difficile[] = { 0xdead, 0xbeef, 0xfeed, 0xface};
  wchar_t mixed[] = {L'f', 0xdead, L'a', L'c', 0xfeed, 0xface};
  wchar_t *cent = L"\242";
  wchar_t repeat[128];
  wchar_t *repeat_p = repeat;

  repeat[0] = 0;
  wcscat (repeat, L"A");
  for (i = 0; i < 21; ++i)
    wcscat (repeat, cent);
  wcscat (repeat, L"B");

  do_nothing (&narrow); /* START */
  do_nothing (&single);
  do_nothing (simple);
  do_nothing (difficile);
  do_nothing (mixed);
}


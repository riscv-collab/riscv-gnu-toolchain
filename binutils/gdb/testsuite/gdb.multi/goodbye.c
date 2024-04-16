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

int gglob = 2;

int glob = 45;

int verylongfun()
{
  glob += 2;
  glob *= 2;
  glob += 3;
  glob *= 3;
  glob += 4;
  glob *= 4;
  glob += 5;
  glob *= 5;
  glob += 6;
  glob *= 6;
  glob += 7;
  glob *= 7;
  glob += 8;
  glob *= 8;
  glob += 9;
  glob *= 9;
  return 0;
}

void
mailand()
{
  glob = 46;
}

int
foo(int x) {
  return x + 92;
}

void
goodbye() {
  ++glob;
}

int
main() {
  mailand();
  foo(glob);
  verylongfun();
  goodbye();
}

void commonfun() { mailand(); } /* from goodbye */

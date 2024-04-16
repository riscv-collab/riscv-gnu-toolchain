/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

int testglob = 0;

int testglob_not_collected = 10;

const int constglob = 10000;

const int constglob_not_collected = 100;

static void
start (void)
{}

static void
end (void)
{}

int
main (void)
{
  testglob++;
  testglob_not_collected++;

  start ();

  testglob++;
  testglob_not_collected++;
  end ();
  return 0;
}

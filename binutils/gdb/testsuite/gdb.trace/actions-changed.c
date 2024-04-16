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

int
end (int i)
{
  return 0;
}

int
subr2 (int parm)
{
  int keeping, busy;

  keeping = parm + parm;
  busy = keeping * keeping;

  return busy;
}

int
subr (int parm)
{
  int keeping, busy;

  keeping = parm + parm;
  busy = keeping * keeping;

  return busy;
}

int
main()
{
  subr (1);
  end (1);

  subr (2);
  end (2);

  subr (3);
  end (3);

  subr (4);
  end (4);

  subr (5);
  subr2 (5);
  end (5);

  subr (6);
  subr2 (6);
  end (6);

  return 0;
}

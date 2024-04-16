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

template<typename T>
void
throwit (T val)
{
  throw val;
}

template<typename T>
void
rethrowit (T val)
{
  try
    {
      try
	{
	  throwit (val);
	}
      catch (...)
	{
	  throw;
	}
    }
  catch (...)
    {
      // Ignore.
    }
}

struct maude
{
  int mv;

  maude (int x) : mv (x) { }
};

int
main (int argc, char **argv)
{
  maude mm (77);
  maude &mmm (mm);

  rethrowit ("hi bob");
  rethrowit (23);
  rethrowit (mm);
  rethrowit (mmm);

  return 0;
}

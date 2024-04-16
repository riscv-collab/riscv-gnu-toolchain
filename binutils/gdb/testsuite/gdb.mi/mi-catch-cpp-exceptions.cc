/* Copyright 2019-2024 Free Software Foundation, Inc.

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

class my_exception
{
private:
  int m_value;

public:
  my_exception (int v)
    : m_value (v)
  {
    /* Nothing.  */
  }
};

void
bar ()
{
  my_exception ex (4);
  throw ex;	/* Throw 1.  */
}

void
foo ()
{
  for (int i = 0; i < 2; ++i)
    {
      try
	{
	  bar ();
	}
      catch (const my_exception &ex)	/* Catch 1.  */
	{
	  if (i == 1)
	    throw;	/* Throw 2.  */
	}
    }
}

int
main ()
{
  for (int i = 0; i < 2; ++i)
    {
      try
	{
	  foo ();
	}
      catch (const my_exception &ex)	/* Catch 2.  */
	{
	  if (i == 1)
	    return 1;	/* Stop here.  */
	}
    }

  return 0;
}


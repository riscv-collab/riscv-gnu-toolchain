/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

class foo
{
public:
  static int bar (void)
  {
    int i = 5;
    bool first = true;

  to_the_top:  /* bar:to_the_top */
    while (1)
      {
	if (i == 1)
	  {
	    if (first)
	      {
		first = false;
		goto to_the_top;
	      }
	    else
	      goto get_out_of_here;
	  }

	--i;
      }

  get_out_of_here: /* bar:get_out_of_here */
    return i;
  }

  int baz (int a)
  {
    int i = a;
    bool first = true;

  to_the_top: /* baz:to_the_top */
    while (1)
      {
	if (i == 1)
	  {
	    if (first)
	      {
		first = false;
		goto to_the_top;
	      }
	    else
	      goto get_out_of_here;
	  }

	--i;
      }

  get_out_of_here: /* baz:get_out_of_here */
    return i;
  }
};

int
main (void)
{
  foo f;
  return f.baz (foo::bar () + 3); 
}


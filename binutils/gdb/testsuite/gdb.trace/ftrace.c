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

#include "trace-common.h"

int globvar;

static void
begin (void)
{}

static void
marker (int anarg)
{
  FAST_TRACEPOINT_LABEL(set_point);

  ++anarg;

  /* Set up a known 4-byte instruction so we can try to set a shorter
     fast tracepoint at it.  */
  asm ("    .global " SYMBOL(four_byter) "\n"
       SYMBOL(four_byter) ":\n"
#if (defined __i386__)
       "    cmpl $0x1,0x8(%ebp) \n"
#endif
       );
}

static void
end (void)
{}

int
main ()
{
  begin ();

  for (globvar = 1; globvar < 11; ++globvar)
    {
      marker (globvar * 100);
    }

  end ();
  return 0;
}

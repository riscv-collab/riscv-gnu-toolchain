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
#include <inttypes.h>

int64_t globvar;

static void
begin (void)
{
}

static void
marker (int8_t arg8, int16_t arg16, int32_t arg32, int64_t arg64)
{
  FAST_TRACEPOINT_LABEL(set_point);
}

static void
end (void)
{
}

int
main ()
{
  begin ();

  for (globvar = 1; globvar < 11; ++globvar)
    marker (globvar, globvar + (1 << 8), globvar + (1 << 16),
	    globvar + (1LL << 32));

  end ();
  return 0;
}

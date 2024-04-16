/*  This file is part of the program psim.

    Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.

    */

/* TODO: Convert callers to lf_printf like common igen/.  */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "dumpf.h"

#include <stdlib.h>
#include <string.h>

void
dumpf (int indent, const char *msg, ...)
{
  va_list ap;
  for (; indent > 0; indent--)
    printf(" ");
  va_start(ap, msg);
  vprintf(msg, ap);
  va_end(ap);
}

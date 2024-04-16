/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lf.h"
#include "lf-ppc.h"

int
lf_print__c_code(lf *file,
		 const char *code)
{
  int nr = 0;
  const char *chp = code;
  int in_bit_field = 0;
  while (*chp != '\0') {
    if (*chp == '\t')
      chp++;
    if (*chp == '#')
      lf_indent_suppress(file);
    while (*chp != '\0' && *chp != '\n') {
      if (chp[0] == '{' && !isspace(chp[1])) {
	in_bit_field = 1;
	nr += lf_putchr(file, '_');
      }
      else if (in_bit_field && chp[0] == ':') {
	nr += lf_putchr(file, '_');
      }
      else if (in_bit_field && *chp == '}') {
	nr += lf_putchr(file, '_');
	in_bit_field = 0;
      }
      else {
	nr += lf_putchr(file, *chp);
      }
      chp++;
    }
    if (in_bit_field)
      ERROR("bit field paren miss match some where\n");
    if (*chp == '\n') {
      nr += lf_putchr(file, '\n');
      chp++;
    }
  }
  nr += lf_putchr(file, '\n');
  return nr;
}

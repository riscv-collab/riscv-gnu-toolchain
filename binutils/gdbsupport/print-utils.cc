/* Cell-based print utility routines for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "print-utils.h"
/* Temporary storage using circular buffer.  */

/* Number of cells in the circular buffer.  */
#define NUMCELLS 16

/* Return the next entry in the circular buffer.  */

char *
get_print_cell (void)
{
  static char buf[NUMCELLS][PRINT_CELL_SIZE];
  static int cell = 0;

  if (++cell >= NUMCELLS)
    cell = 0;
  return buf[cell];
}

static char *
decimal2str (const char *sign, ULONGEST addr, int width)
{
  /* Steal code from valprint.c:print_decimal().  Should this worry
     about the real size of addr as the above does?  */
  unsigned long temp[3];
  char *str = get_print_cell ();
  int i = 0;

  do
    {
      temp[i] = addr % (1000 * 1000 * 1000);
      addr /= (1000 * 1000 * 1000);
      i++;
      width -= 9;
    }
  while (addr != 0 && i < (sizeof (temp) / sizeof (temp[0])));

  width += 9;
  if (width < 0)
    width = 0;

  switch (i)
    {
    case 1:
      xsnprintf (str, PRINT_CELL_SIZE, "%s%0*lu", sign, width, temp[0]);
      break;
    case 2:
      xsnprintf (str, PRINT_CELL_SIZE, "%s%0*lu%09lu", sign, width,
		 temp[1], temp[0]);
      break;
    case 3:
      xsnprintf (str, PRINT_CELL_SIZE, "%s%0*lu%09lu%09lu", sign, width,
		 temp[2], temp[1], temp[0]);
      break;
    default:
      internal_error (_("failed internal consistency check"));
    }

  return str;
}

static char *
octal2str (ULONGEST addr, int width)
{
  unsigned long temp[3];
  char *str = get_print_cell ();
  int i = 0;

  do
    {
      temp[i] = addr % (0100000 * 0100000);
      addr /= (0100000 * 0100000);
      i++;
      width -= 10;
    }
  while (addr != 0 && i < (sizeof (temp) / sizeof (temp[0])));

  width += 10;
  if (width < 0)
    width = 0;

  switch (i)
    {
    case 1:
      if (temp[0] == 0)
	xsnprintf (str, PRINT_CELL_SIZE, "%*o", width, 0);
      else
	xsnprintf (str, PRINT_CELL_SIZE, "0%0*lo", width, temp[0]);
      break;
    case 2:
      xsnprintf (str, PRINT_CELL_SIZE, "0%0*lo%010lo", width, temp[1], temp[0]);
      break;
    case 3:
      xsnprintf (str, PRINT_CELL_SIZE, "0%0*lo%010lo%010lo", width,
		 temp[2], temp[1], temp[0]);
      break;
    default:
      internal_error (_("failed internal consistency check"));
    }

  return str;
}

/* See print-utils.h.  */

char *
pulongest (ULONGEST u)
{
  return decimal2str ("", u, 0);
}

/* See print-utils.h.  */

char *
plongest (LONGEST l)
{
  if (l < 0)
    return decimal2str ("-", -l, 0);
  else
    return decimal2str ("", l, 0);
}

/* Eliminate warning from compiler on 32-bit systems.  */
static int thirty_two = 32;

/* See print-utils.h.  */

char *
phex (ULONGEST l, int sizeof_l)
{
  char *str;

  switch (sizeof_l)
    {
    case 8:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%08lx%08lx",
		 (unsigned long) (l >> thirty_two),
		 (unsigned long) (l & 0xffffffff));
      break;
    case 4:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%08lx", (unsigned long) l);
      break;
    case 2:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%04x", (unsigned short) (l & 0xffff));
      break;
    case 1:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%02x", (unsigned short) (l & 0xff));
      break;
    default:
      str = phex (l, sizeof (l));
      break;
    }

  return str;
}

/* See print-utils.h.  */

char *
phex_nz (ULONGEST l, int sizeof_l)
{
  char *str;

  switch (sizeof_l)
    {
    case 8:
      {
	unsigned long high = (unsigned long) (l >> thirty_two);

	str = get_print_cell ();
	if (high == 0)
	  xsnprintf (str, PRINT_CELL_SIZE, "%lx",
		     (unsigned long) (l & 0xffffffff));
	else
	  xsnprintf (str, PRINT_CELL_SIZE, "%lx%08lx", high,
		     (unsigned long) (l & 0xffffffff));
	break;
      }
    case 4:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%lx", (unsigned long) l);
      break;
    case 2:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%x", (unsigned short) (l & 0xffff));
      break;
    case 1:
      str = get_print_cell ();
      xsnprintf (str, PRINT_CELL_SIZE, "%x", (unsigned short) (l & 0xff));
      break;
    default:
      str = phex_nz (l, sizeof (l));
      break;
    }

  return str;
}

/* See print-utils.h.  */

char *
hex_string (LONGEST num)
{
  char *result = get_print_cell ();

  xsnprintf (result, PRINT_CELL_SIZE, "0x%s", phex_nz (num, sizeof (num)));
  return result;
}

/* See print-utils.h.  */

char *
hex_string_custom (LONGEST num, int width)
{
  char *result = get_print_cell ();
  char *result_end = result + PRINT_CELL_SIZE - 1;
  const char *hex = phex_nz (num, sizeof (num));
  int hex_len = strlen (hex);

  if (hex_len > width)
    width = hex_len;
  if (width + 2 >= PRINT_CELL_SIZE)
    internal_error (_("\
hex_string_custom: insufficient space to store result"));

  strcpy (result_end - width - 2, "0x");
  memset (result_end - width, '0', width);
  strcpy (result_end - hex_len, hex);
  return result_end - width - 2;
}

/* See print-utils.h.  */

char *
int_string (LONGEST val, int radix, int is_signed, int width,
	    int use_c_format)
{
  switch (radix)
    {
    case 16:
      {
	char *result;

	if (width == 0)
	  result = hex_string (val);
	else
	  result = hex_string_custom (val, width);
	if (! use_c_format)
	  result += 2;
	return result;
      }
    case 10:
      {
	if (is_signed && val < 0)
	  /* Cast to unsigned before negating, to prevent runtime error:
	     negation of -9223372036854775808 cannot be represented in type
	     'long int'; cast to an unsigned type to negate this value to
	     itself.  */
	  return decimal2str ("-", -(ULONGEST)val, width);
	else
	  return decimal2str ("", val, width);
      }
    case 8:
      {
	char *result = octal2str (val, width);

	if (use_c_format || val == 0)
	  return result;
	else
	  return result + 1;
      }
    default:
      internal_error (_("failed internal consistency check"));
    }
}

/* See print-utils.h.  */

const char *
core_addr_to_string (const CORE_ADDR addr)
{
  char *str = get_print_cell ();

  strcpy (str, "0x");
  strcat (str, phex (addr, sizeof (addr)));
  return str;
}

/* See print-utils.h.  */

const char *
core_addr_to_string_nz (const CORE_ADDR addr)
{
  char *str = get_print_cell ();

  strcpy (str, "0x");
  strcat (str, phex_nz (addr, sizeof (addr)));
  return str;
}

/* See print-utils.h.  */

const char *
host_address_to_string_1 (const void *addr)
{
  char *str = get_print_cell ();

  xsnprintf (str, PRINT_CELL_SIZE, "0x%s",
	     phex_nz ((uintptr_t) addr, sizeof (addr)));
  return str;
}

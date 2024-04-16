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

#ifndef COMMON_PRINT_UTILS_H
#define COMMON_PRINT_UTILS_H

/* How many characters (including the terminating null byte) fit in a
   cell.  */
#define PRINT_CELL_SIZE 50

/* %u for ULONGEST.  The result is stored in a circular static buffer,
   NUMCELLS deep.  */

extern char *pulongest (ULONGEST u);

/* %d for LONGEST.  The result is stored in a circular static buffer,
   NUMCELLS deep.  */

extern char *plongest (LONGEST l);

/* Convert a ULONGEST into a HEX string, like %lx, with leading zeros.
   The result is stored in a circular static buffer, NUMCELLS deep.  */

extern char *phex (ULONGEST l, int sizeof_l);

/* Convert a ULONGEST into a HEX string, like %lx, without leading zeros.
   The result is  stored in a circular static buffer, NUMCELLS deep.  */

extern char *phex_nz (ULONGEST l, int sizeof_l);

/* Converts a LONGEST to a C-format hexadecimal literal and stores it
   in a static string.  Returns a pointer to this string.  */

extern char *hex_string (LONGEST num);

/* Converts a LONGEST number to a C-format hexadecimal literal and
   stores it in a static string.  Returns a pointer to this string
   that is valid until the next call.  The number is padded on the
   left with 0s to at least WIDTH characters.  */

extern char *hex_string_custom (LONGEST num, int width);

/* Convert VAL to a numeral in the given radix.  For
 * radix 10, IS_SIGNED may be true, indicating a signed quantity;
 * otherwise VAL is interpreted as unsigned.  If WIDTH is supplied, 
 * it is the minimum width (0-padded if needed).  USE_C_FORMAT means
 * to use C format in all cases.  If it is false, then 'x' 
 * and 'o' formats do not include a prefix (0x or leading 0).  */

extern char *int_string (LONGEST val, int radix, int is_signed, int width, 
			 int use_c_format);	

/* Convert a CORE_ADDR into a string.  */

extern const char *core_addr_to_string (const CORE_ADDR addr);

extern const char *core_addr_to_string_nz (const CORE_ADDR addr);

extern const char *host_address_to_string_1 (const void *addr);

/* Wrapper that avoids adding a pointless cast to all callers.  */
#define host_address_to_string(ADDR) \
  host_address_to_string_1 ((const void *) (ADDR))

/* Return the next entry in the circular print buffer.  */

extern char *get_print_cell (void);

#endif /* COMMON_PRINT_UTILS_H */

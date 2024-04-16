/* Low-level RSP routines for GDB, the GNU debugger.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_RSP_LOW_H
#define COMMON_RSP_LOW_H

/* Convert number NIB to a hex digit.  */

extern int tohex (int nib);

/* Write a character representing the low order four bits of NIBBLE in
   hex to *BUF.  Returns BUF+1.  */

extern char *pack_nibble (char *buf, int nibble);

/* Write the low byte of BYTE in hex to *BUF.  Returns BUF+2.  */

extern char *pack_hex_byte (char *pkt, int byte);

/* Read hex digits from BUFF and convert to a number, which is stored
   in RESULT.  Reads until a non-hex digit is seen.  Returns a pointer
   to the terminating character.  */

extern const char *unpack_varlen_hex (const char *buff, ULONGEST *result);

/* Like hex2bin, but return a std::string.  */

extern std::string hex2str (const char *hex);

/* Like hex2bin, but return a std::string.  */

extern std::string hex2str (const char *hex, int count);

/* Convert some bytes to a hexadecimal representation.  BIN holds the
   bytes to convert.  COUNT says how many bytes to convert.  The
   resulting characters are stored in HEX, followed by a NUL
   character.  Returns the number of bytes actually converted.  */

extern int bin2hex (const gdb_byte *bin, char *hex, int count);

extern int bin2hex (gdb::array_view<gdb_byte> bin, char *hex);

/* Overloaded version of bin2hex that returns a std::string.  */

extern std::string bin2hex (const gdb_byte *bin, int count);

/* Convert BUFFER, binary data at least LEN_UNITS addressable memory units
   long, into escaped binary data in OUT_BUF.  Only copy memory units that fit
   completely in OUT_BUF.  Set *OUT_LEN_UNITS to the number of units from
   BUFFER successfully encoded in OUT_BUF, and return the number of bytes used
   in OUT_BUF.  The total number of bytes in the output buffer will be at most
   OUT_MAXLEN_BYTES.  This function properly escapes '*', and so is suitable
   for the server side as well as the client.  */

extern int remote_escape_output (const gdb_byte *buffer, int len_units,
				 int unit_size, gdb_byte *out_buf,
				 int *out_len_units, int out_maxlen_bytes);

/* Convert BUFFER, escaped data LEN bytes long, into binary data
   in OUT_BUF.  Return the number of bytes written to OUT_BUF.
   Raise an error if the total number of bytes exceeds OUT_MAXLEN.

   This function reverses remote_escape_output.  */

extern int remote_unescape_input (const gdb_byte *buffer, int len,
				  gdb_byte *out_buf, int out_maxlen);

#endif /* COMMON_RSP_LOW_H */

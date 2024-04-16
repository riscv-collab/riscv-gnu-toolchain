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

#include "common-defs.h"
#include "rsp-low.h"

/* See rsp-low.h.  */

int
tohex (int nib)
{
  if (nib < 10)
    return '0' + nib;
  else
    return 'a' + nib - 10;
}

/* Encode 64 bits in 16 chars of hex.  */

static const char hexchars[] = "0123456789abcdef";

static int
ishex (int ch, int *val)
{
  if ((ch >= 'a') && (ch <= 'f'))
    {
      *val = ch - 'a' + 10;
      return 1;
    }
  if ((ch >= 'A') && (ch <= 'F'))
    {
      *val = ch - 'A' + 10;
      return 1;
    }
  if ((ch >= '0') && (ch <= '9'))
    {
      *val = ch - '0';
      return 1;
    }
  return 0;
}

/* See rsp-low.h.  */

char *
pack_nibble (char *buf, int nibble)
{
  *buf++ = hexchars[(nibble & 0x0f)];
  return buf;
}

/* See rsp-low.h.  */

char *
pack_hex_byte (char *pkt, int byte)
{
  *pkt++ = hexchars[(byte >> 4) & 0xf];
  *pkt++ = hexchars[(byte & 0xf)];
  return pkt;
}

/* See rsp-low.h.  */

const char *
unpack_varlen_hex (const char *buff,	/* packet to parse */
		   ULONGEST *result)
{
  int nibble;
  ULONGEST retval = 0;

  while (ishex (*buff, &nibble))
    {
      buff++;
      retval = retval << 4;
      retval |= nibble & 0x0f;
    }
  *result = retval;
  return buff;
}

/* See rsp-low.h.  */

std::string
hex2str (const char *hex)
{
  return hex2str (hex, strlen (hex));
}

/* See rsp-low.h.  */

std::string
hex2str (const char *hex, int count)
{
  std::string ret;

  ret.reserve (count);
  for (size_t i = 0; i < count; ++i)
    {
      if (hex[0] == '\0' || hex[1] == '\0')
	{
	  /* Hex string is short, or of uneven length.  Return what we
	     have so far.  */
	  return ret;
	}
      ret += fromhex (hex[0]) * 16 + fromhex (hex[1]);
      hex += 2;
    }

  return ret;
}

/* See rsp-low.h.  */

int
bin2hex (const gdb_byte *bin, char *hex, int count)
{
  int i;

  for (i = 0; i < count; i++)
    {
      *hex++ = tohex ((*bin >> 4) & 0xf);
      *hex++ = tohex (*bin++ & 0xf);
    }
  *hex = 0;
  return i;
}

/* See rsp-low.h.  */

int
bin2hex (gdb::array_view<gdb_byte> bin, char *hex)
{
  return bin2hex (bin.data (), hex, bin.size ());
}

/* See rsp-low.h.  */

std::string
bin2hex (const gdb_byte *bin, int count)
{
  std::string ret;

  ret.reserve (count * 2);
  for (int i = 0; i < count; ++i)
    {
      ret += tohex ((*bin >> 4) & 0xf);
      ret += tohex (*bin++ & 0xf);
    }

  return ret;
}

/* Return whether byte B needs escaping when sent as part of binary data.  */

static int
needs_escaping (gdb_byte b)
{
  return b == '$' || b == '#' || b == '}' || b == '*';
}

/* See rsp-low.h.  */

int
remote_escape_output (const gdb_byte *buffer, int len_units, int unit_size,
		      gdb_byte *out_buf, int *out_len_units,
		      int out_maxlen_bytes)
{
  int input_unit_index, output_byte_index = 0, byte_index_in_unit;
  int number_escape_bytes_needed;

  /* Try to copy integral addressable memory units until
     (1) we run out of space or
     (2) we copied all of them.  */
  for (input_unit_index = 0;
       input_unit_index < len_units;
       input_unit_index++)
    {
      /* Find out how many escape bytes we need for this unit.  */
      number_escape_bytes_needed = 0;
      for (byte_index_in_unit = 0;
	   byte_index_in_unit < unit_size;
	   byte_index_in_unit++)
	{
	  int idx = input_unit_index * unit_size + byte_index_in_unit;
	  gdb_byte b = buffer[idx];
	  if (needs_escaping (b))
	    number_escape_bytes_needed++;
	}

      /* Check if we have room to fit this escaped unit.  */
      if (output_byte_index + unit_size + number_escape_bytes_needed >
	    out_maxlen_bytes)
	  break;

      /* Copy the unit byte per byte, adding escapes.  */
      for (byte_index_in_unit = 0;
	   byte_index_in_unit < unit_size;
	   byte_index_in_unit++)
	{
	  int idx = input_unit_index * unit_size + byte_index_in_unit;
	  gdb_byte b = buffer[idx];
	  if (needs_escaping (b))
	    {
	      out_buf[output_byte_index++] = '}';
	      out_buf[output_byte_index++] = b ^ 0x20;
	    }
	  else
	    out_buf[output_byte_index++] = b;
	}
    }

  *out_len_units = input_unit_index;
  return output_byte_index;
}

/* See rsp-low.h.  */

int
remote_unescape_input (const gdb_byte *buffer, int len,
		       gdb_byte *out_buf, int out_maxlen)
{
  int input_index, output_index;
  int escaped;

  output_index = 0;
  escaped = 0;
  for (input_index = 0; input_index < len; input_index++)
    {
      gdb_byte b = buffer[input_index];

      if (output_index + 1 > out_maxlen)
	error (_("Received too much data from the target."));

      if (escaped)
	{
	  out_buf[output_index++] = b ^ 0x20;
	  escaped = 0;
	}
      else if (b == '}')
	escaped = 1;
      else
	out_buf[output_index++] = b;
    }

  if (escaped)
    error (_("Unmatched escape character in target response."));

  return output_index;
}


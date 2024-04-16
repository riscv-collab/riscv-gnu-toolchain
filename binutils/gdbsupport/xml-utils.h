/* Shared helper routines for manipulating XML.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_XML_UTILS_H
#define COMMON_XML_UTILS_H

/* Return a string with special characters from TEXT replaced by entity
   references.  */

extern std::string xml_escape_text (const char *text);

/* Append TEXT to RESULT, with special characters replaced by entity
   references.  */

extern void xml_escape_text_append (std::string &result, const char *text);

/* Simple printf to string function.  Current implemented formatters:
   %s - append an xml escaped text to BUFFER.
   %d - append an signed integer to BUFFER.
   %u - append an unsigned integer to BUFFER.
   %x - append an unsigned integer formatted in hexadecimal to BUFFER.
   %o - append an unsigned integer formatted in octal to BUFFER.  */

void string_xml_appendf (std::string &buffer, const char *format, ...)
  ATTRIBUTE_PRINTF (2, 3);

#endif /* COMMON_XML_UTILS_H */

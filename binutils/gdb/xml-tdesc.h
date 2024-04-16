/* XML target description support for GDB.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

   Contributed by CodeSourcery.

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

#ifndef XML_TDESC_H
#define XML_TDESC_H

#include <optional>
#include <string>

struct target_ops;
struct target_desc;

/* Read an XML target description from FILENAME.  Parse it, and return
   the parsed description.  */

const struct target_desc *file_read_description_xml (const char *filename);

/* Read an XML target description using OPS.  Parse it, and return the
   parsed description.  */

const struct target_desc *target_read_description_xml (struct target_ops *);

/* Fetches an XML target description using OPS, processing includes,
   but not parsing it.  Used to dump whole tdesc as a single XML file.
   Returns the description on success, and a disengaged optional
   otherwise.  */
std::optional<std::string> target_fetch_description_xml (target_ops *ops);

/* Take an xml string, parse it, and return the parsed description.  Does not
   handle a string containing includes.  */

const struct target_desc *string_read_description_xml (const char *xml);

#endif /* XML_TDESC_H */


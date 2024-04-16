/* Routines for handling XML memory maps provided by target.

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

#include "defs.h"
#include "memory-map.h"

#if !defined(HAVE_LIBEXPAT)

std::vector<mem_region>
parse_memory_map (const char *memory_map)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML memory map; XML support was disabled "
		 "at compile time"));
    }

  return std::vector<mem_region> ();
}

#else /* HAVE_LIBEXPAT */

#include "xml-support.h"

/* Internal parsing data passed to all XML callbacks.  */
struct memory_map_parsing_data
{
  memory_map_parsing_data (std::vector<mem_region> *memory_map_)
  : memory_map (memory_map_)
  {}

  std::vector<mem_region> *memory_map;

  std::string property_name;
};

/* Handle the start of a <memory> element.  */

static void
memory_map_start_memory (struct gdb_xml_parser *parser,
			 const struct gdb_xml_element *element,
			 void *user_data,
			 std::vector<gdb_xml_value> &attributes)
{
  struct memory_map_parsing_data *data
    = (struct memory_map_parsing_data *) user_data;
  ULONGEST *start_p, *length_p, *type_p;

  start_p
    = (ULONGEST *) xml_find_attribute (attributes, "start")->value.get ();
  length_p
    = (ULONGEST *) xml_find_attribute (attributes, "length")->value.get ();
  type_p
    = (ULONGEST *) xml_find_attribute (attributes, "type")->value.get ();

  data->memory_map->emplace_back (*start_p, *start_p + *length_p,
				  (enum mem_access_mode) *type_p);
}

/* Handle the end of a <memory> element.  Verify that any necessary
   children were present.  */

static void
memory_map_end_memory (struct gdb_xml_parser *parser,
		       const struct gdb_xml_element *element,
		       void *user_data, const char *body_text)
{
  struct memory_map_parsing_data *data
    = (struct memory_map_parsing_data *) user_data;
  const mem_region &r = data->memory_map->back ();

  if (r.attrib.mode == MEM_FLASH && r.attrib.blocksize == -1)
    gdb_xml_error (parser, _("Flash block size is not set"));
}

/* Handle the start of a <property> element by saving the name
   attribute for later.  */

static void
memory_map_start_property (struct gdb_xml_parser *parser,
			   const struct gdb_xml_element *element,
			   void *user_data,
			   std::vector<gdb_xml_value> &attributes)
{
  struct memory_map_parsing_data *data
    = (struct memory_map_parsing_data *) user_data;
  char *name;

  name = (char *) xml_find_attribute (attributes, "name")->value.get ();
  data->property_name.assign (name);
}

/* Handle the end of a <property> element and its value.  */

static void
memory_map_end_property (struct gdb_xml_parser *parser,
			 const struct gdb_xml_element *element,
			 void *user_data, const char *body_text)
{
  struct memory_map_parsing_data *data
    = (struct memory_map_parsing_data *) user_data;

  if (data->property_name == "blocksize")
    {
      mem_region &r = data->memory_map->back ();

      r.attrib.blocksize = gdb_xml_parse_ulongest (parser, body_text);
    }
  else
    gdb_xml_debug (parser, _("Unknown property \"%s\""),
		   data->property_name.c_str ());
}

/* The allowed elements and attributes for an XML memory map.  */

const struct gdb_xml_attribute property_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element memory_children[] = {
  { "property", property_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    memory_map_start_property, memory_map_end_property },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_enum memory_type_enum[] = {
  { "ram", MEM_RW },
  { "rom", MEM_RO },
  { "flash", MEM_FLASH },
  { NULL, 0 }
};

const struct gdb_xml_attribute memory_attributes[] = {
  { "start", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "length", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "type", GDB_XML_AF_NONE, gdb_xml_parse_attr_enum, &memory_type_enum },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element memory_map_children[] = {
  { "memory", memory_attributes, memory_children, GDB_XML_EF_REPEATABLE,
    memory_map_start_memory, memory_map_end_memory },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_element memory_map_elements[] = {
  { "memory-map", NULL, memory_map_children, GDB_XML_EF_NONE,
    NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

std::vector<mem_region>
parse_memory_map (const char *memory_map)
{
  std::vector<mem_region> ret;
  memory_map_parsing_data data (&ret);

  if (gdb_xml_parse_quick (_("target memory map"), NULL, memory_map_elements,
			   memory_map, &data) == 0)
    {
      /* Parsed successfully, keep the result.  */
      return ret;
    }

  return std::vector<mem_region> ();
}

#endif /* HAVE_LIBEXPAT */

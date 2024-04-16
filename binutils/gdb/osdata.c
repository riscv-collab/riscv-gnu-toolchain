/* Routines for handling XML generic OS data provided by target.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "xml-support.h"
#include "osdata.h"
#include "ui-out.h"
#include "gdbcmd.h"

#if !defined(HAVE_LIBEXPAT)

std::unique_ptr<osdata>
osdata_parse (const char *xml)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML OS data; XML support was disabled "
		"at compile time"));
    }

  return NULL;
}

#else /* HAVE_LIBEXPAT */

/* Internal parsing data passed to all XML callbacks.  */
struct osdata_parsing_data
{
  std::unique_ptr<struct osdata> osdata;
  std::string property_name;
};

/* Handle the start of a <osdata> element.  */

static void
osdata_start_osdata (struct gdb_xml_parser *parser,
			const struct gdb_xml_element *element,
			void *user_data,
			std::vector<gdb_xml_value> &attributes)
{
  struct osdata_parsing_data *data = (struct osdata_parsing_data *) user_data;

  if (data->osdata != NULL)
    gdb_xml_error (parser, _("Seen more than on osdata element"));

  char *type = (char *) xml_find_attribute (attributes, "type")->value.get ();
  data->osdata.reset (new struct osdata (std::string (type)));
}

/* Handle the start of a <item> element.  */

static void
osdata_start_item (struct gdb_xml_parser *parser,
		  const struct gdb_xml_element *element,
		  void *user_data,
		  std::vector<gdb_xml_value> &attributes)
{
  struct osdata_parsing_data *data = (struct osdata_parsing_data *) user_data;
  data->osdata->items.emplace_back ();
}

/* Handle the start of a <column> element.  */

static void
osdata_start_column (struct gdb_xml_parser *parser,
		    const struct gdb_xml_element *element,
		    void *user_data,
		    std::vector<gdb_xml_value> &attributes)
{
  struct osdata_parsing_data *data = (struct osdata_parsing_data *) user_data;
  const char *name
    = (const char *) xml_find_attribute (attributes, "name")->value.get ();

  data->property_name.assign (name);
}

/* Handle the end of a <column> element.  */

static void
osdata_end_column (struct gdb_xml_parser *parser,
		  const struct gdb_xml_element *element,
		  void *user_data, const char *body_text)
{
  osdata_parsing_data *data = (struct osdata_parsing_data *) user_data;
  struct osdata *osdata = data->osdata.get ();
  osdata_item &item = osdata->items.back ();

  item.columns.emplace_back (std::move (data->property_name),
			     std::string (body_text));
}

/* The allowed elements and attributes for OS data object.
   The root element is a <osdata>.  */

const struct gdb_xml_attribute column_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element item_children[] = {
  { "column", column_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    osdata_start_column, osdata_end_column },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_attribute osdata_attributes[] = {
  { "type", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element osdata_children[] = {
  { "item", NULL, item_children,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    osdata_start_item, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

const struct gdb_xml_element osdata_elements[] = {
  { "osdata", osdata_attributes, osdata_children,
    GDB_XML_EF_NONE, osdata_start_osdata, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

std::unique_ptr<osdata>
osdata_parse (const char *xml)
{
  osdata_parsing_data data;

  if (gdb_xml_parse_quick (_("osdata"), "osdata.dtd",
			   osdata_elements, xml, &data) == 0)
    {
      /* Parsed successfully, don't need to delete the result.  */
      return std::move (data.osdata);
    }

  return NULL;
}
#endif

std::unique_ptr<osdata>
get_osdata (const char *type)
{
  std::unique_ptr<osdata> osdata;
  std::optional<gdb::char_vector> xml = target_get_osdata (type);

  if (xml)
    {
      if ((*xml)[0] == '\0')
	{
	  if (type)
	    warning (_("Empty data returned by target.  Wrong osdata type?"));
	  else
	    warning (_("Empty type list returned by target.  No type data?"));
	}
      else
	osdata = osdata_parse (xml->data ());
    }

  if (osdata == NULL)
    error (_("Can not fetch data now."));

  return osdata;
}

const std::string *
get_osdata_column (const osdata_item &item, const char *name)
{
  for (const osdata_column &col : item.columns)
    if (col.name == name)
      return &col.value;

  return NULL;
}

void
info_osdata (const char *type)
{
  struct ui_out *uiout = current_uiout;
  struct osdata_item *last = NULL;
  int ncols = 0;
  int col_to_skip = -1;

  if (type == NULL)
    type = "";

  std::unique_ptr<osdata> osdata = get_osdata (type);

  int nrows = osdata->items.size ();

  if (*type == '\0' && nrows == 0)
    error (_("Available types of OS data not reported."));
  
  if (!osdata->items.empty ())
    {
      last = &osdata->items.back ();
      ncols = last->columns.size ();

      /* As a special case, scan the listing of available data types
	 for a column named "Title", and only include it with MI
	 output; this column's normal use is for titles for interface
	 elements like menus, and it clutters up CLI output.  */
      if (*type == '\0' && !uiout->is_mi_like_p ())
	{
	  for (int ix = 0; ix < last->columns.size (); ix++)
	    {
	      if (last->columns[ix].name == "Title")
		col_to_skip = ix;
	    }
	  /* Be sure to reduce the total column count, otherwise
	     internal errors ensue.  */
	  if (col_to_skip >= 0)
	    --ncols;
	}
    }

  ui_out_emit_table table_emitter (uiout, ncols, nrows, "OSDataTable");

  /* With no columns/items, we just output an empty table, but we
     still output the table.  This matters for MI.  */
  if (ncols == 0)
    return;

  if (last != NULL && !last->columns.empty ())
    {
      for (int ix = 0; ix < last->columns.size (); ix++)
	{
	  char col_name[32];

	  if (ix == col_to_skip)
	    continue;

	  snprintf (col_name, 32, "col%d", ix);
	  uiout->table_header (10, ui_left,
			       col_name, last->columns[ix].name.c_str ());
	}
    }

  uiout->table_body ();

  if (nrows != 0)
    {
      for (const osdata_item &item : osdata->items)
       {
	 {
	   ui_out_emit_tuple tuple_emitter (uiout, "item");

	   for (int ix_cols = 0; ix_cols < item.columns.size (); ix_cols++)
	     {
	       char col_name[32];

	       if (ix_cols == col_to_skip)
		 continue;

	       snprintf (col_name, 32, "col%d", ix_cols);
	       uiout->field_string (col_name, item.columns[ix_cols].value);
	     }
	 }

	 uiout->text ("\n");
       }
    }
}

static void
info_osdata_command (const char *arg, int from_tty)
{
  info_osdata (arg);
}

void _initialize_osdata ();
void
_initialize_osdata ()
{
  add_info ("os", info_osdata_command,
	   _("Show OS data ARG."));
}

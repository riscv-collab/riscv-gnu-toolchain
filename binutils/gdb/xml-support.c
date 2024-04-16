/* Helper routines for parsing XML using Expat.

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
#include "gdbcmd.h"
#include "xml-builtin.h"
#include "xml-support.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include <vector>
#include <string>

/* Debugging flag.  */
static bool debug_xml;

/* The contents of this file are only useful if XML support is
   available.  */
#ifdef HAVE_LIBEXPAT

#include "gdb_expat.h"

/* The maximum depth of <xi:include> nesting.  No need to be miserly,
   we just want to avoid running out of stack on loops.  */
#define MAX_XINCLUDE_DEPTH 30

/* Simplified XML parser infrastructure.  */

/* A parsing level -- used to keep track of the current element
   nesting.  */
struct scope_level
{
  explicit scope_level (const gdb_xml_element *elements_ = NULL)
    : elements (elements_),
      element (NULL),
      seen (0)
  {}

  /* Elements we allow at this level.  */
  const struct gdb_xml_element *elements;

  /* The element which we are within.  */
  const struct gdb_xml_element *element;

  /* Mask of which elements we've seen at this level (used for
     optional and repeatable checking).  */
  unsigned int seen;

  /* Body text accumulation.  */
  std::string body;
};

/* The parser itself, and our additional state.  */
struct gdb_xml_parser
{
  gdb_xml_parser (const char *name,
		  const gdb_xml_element *elements,
		  void *user_data);
  ~gdb_xml_parser();

  /* Associate DTD_NAME, which must be the name of a compiled-in DTD,
     with the parser.  */
  void use_dtd (const char *dtd_name);

  /* Return the name of the expected / default DTD, if specified.  */
  const char *dtd_name ()
  { return m_dtd_name; }

  /* Invoke the parser on BUFFER.  BUFFER is the data to parse, which
     should be NUL-terminated.

     The return value is 0 for success or -1 for error.  It may throw,
     but only if something unexpected goes wrong during parsing; parse
     errors will be caught, warned about, and reported as failure.  */
  int parse (const char *buffer);

  /* Issue a debugging message.  */
  void vdebug (const char *format, va_list ap)
    ATTRIBUTE_PRINTF (2, 0);

  /* Issue an error message, and stop parsing.  */
  void verror (const char *format, va_list ap)
    ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (2, 0);

  void body_text (const XML_Char *text, int length);
  void start_element (const XML_Char *name, const XML_Char **attrs);
  void end_element (const XML_Char *name);

  /* Return the name of this parser.  */
  const char *name ()
  { return m_name; }

  /* Return the user's callback data, for handlers.  */
  void *user_data ()
  { return m_user_data; };

  /* Are we the special <xi:include> parser?  */
  void set_is_xinclude (bool is_xinclude)
  { m_is_xinclude = is_xinclude; }

  /* A thrown error, if any.  */
  void set_error (gdb_exception &&error)
  {
    m_error = std::move (error);
#ifdef HAVE_XML_STOPPARSER
    XML_StopParser (m_expat_parser, XML_FALSE);
#endif
  }

  /* Return the underlying expat parser.  */
  XML_Parser expat_parser ()
  { return m_expat_parser; }

private:
  /* The underlying expat parser.  */
  XML_Parser m_expat_parser;

  /* Name of this parser.  */
  const char *m_name;

  /* The user's callback data, for handlers.  */
  void *m_user_data;

  /* Scoping stack.  */
  std::vector<scope_level> m_scopes;

/* A thrown error, if any.  */
  struct gdb_exception m_error;

  /* The line of the thrown error, or 0.  */
  int m_last_line;

  /* The name of the expected / default DTD, if specified.  */
  const char *m_dtd_name;

  /* Are we the special <xi:include> parser?  */
  bool m_is_xinclude;
};

/* Process some body text.  We accumulate the text for later use; it's
   wrong to do anything with it immediately, because a single block of
   text might be broken up into multiple calls to this function.  */

void
gdb_xml_parser::body_text (const XML_Char *text, int length)
{
  if (m_error.reason < 0)
    return;

  scope_level &scope = m_scopes.back ();
  scope.body.append (text, length);
}

static void
gdb_xml_body_text (void *data, const XML_Char *text, int length)
{
  struct gdb_xml_parser *parser = (struct gdb_xml_parser *) data;

  parser->body_text (text, length);
}

/* Issue a debugging message from one of PARSER's handlers.  */

void
gdb_xml_parser::vdebug (const char *format, va_list ap)
{
  int line = XML_GetCurrentLineNumber (m_expat_parser);

  std::string message = string_vprintf (format, ap);
  if (line)
    gdb_printf (gdb_stderr, "%s (line %d): %s\n",
		m_name, line, message.c_str ());
  else
    gdb_printf (gdb_stderr, "%s: %s\n",
		m_name, message.c_str ());
}

void
gdb_xml_debug (struct gdb_xml_parser *parser, const char *format, ...)
{
  if (!debug_xml)
    return;

  va_list ap;
  va_start (ap, format);
  parser->vdebug (format, ap);
  va_end (ap);
}

/* Issue an error message from one of PARSER's handlers, and stop
   parsing.  */

void
gdb_xml_parser::verror (const char *format, va_list ap)
{
  int line = XML_GetCurrentLineNumber (m_expat_parser);

  m_last_line = line;
  throw_verror (XML_PARSE_ERROR, format, ap);
}

void
gdb_xml_error (struct gdb_xml_parser *parser, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  parser->verror (format, ap);
  va_end (ap);
}

/* Find the attribute named NAME in the set of parsed attributes
   ATTRIBUTES.  Returns NULL if not found.  */

struct gdb_xml_value *
xml_find_attribute (std::vector<gdb_xml_value> &attributes,
		    const char *name)
{
  for (gdb_xml_value &value : attributes)
    if (strcmp (value.name, name) == 0)
      return &value;

  return NULL;
}

/* Handle the start of an element.  NAME is the element, and ATTRS are
   the names and values of this element's attributes.  */

void
gdb_xml_parser::start_element (const XML_Char *name,
			       const XML_Char **attrs)
{
  if (m_error.reason < 0)
    return;

  const struct gdb_xml_element *element;
  const struct gdb_xml_attribute *attribute;
  unsigned int seen;

  /* Push an error scope.  If we return or throw an exception before
     filling this in, it will tell us to ignore children of this
     element.  Note we don't take a reference to the element yet
     because further below we'll process the element which may recurse
     back here and push more elements to the vector.  When the
     recursion unrolls all such elements will have been popped back
     already, but if one of those pushes reallocates the vector,
     previous element references will be invalidated.  */
  m_scopes.emplace_back ();

  /* Get a reference to the current scope.  */
  scope_level &scope = m_scopes[m_scopes.size () - 2];

  gdb_xml_debug (this, _("Entering element <%s>"), name);

  /* Find this element in the list of the current scope's allowed
     children.  Record that we've seen it.  */

  seen = 1;
  for (element = scope.elements; element && element->name;
       element++, seen <<= 1)
    if (strcmp (element->name, name) == 0)
      break;

  if (element == NULL || element->name == NULL)
    {
      /* If we're working on XInclude, <xi:include> can be the child
	 of absolutely anything.  Copy the previous scope's element
	 list into the new scope even if there was no match.  */
      if (m_is_xinclude)
	{
	  XML_DefaultCurrent (m_expat_parser);

	  scope_level &unknown_scope = m_scopes.back ();
	  unknown_scope.elements = scope.elements;
	  return;
	}

      gdb_xml_debug (this, _("Element <%s> unknown"), name);
      return;
    }

  if (!(element->flags & GDB_XML_EF_REPEATABLE) && (seen & scope.seen))
    gdb_xml_error (this, _("Element <%s> only expected once"), name);

  scope.seen |= seen;

  std::vector<gdb_xml_value> attributes;

  for (attribute = element->attributes;
       attribute != NULL && attribute->name != NULL;
       attribute++)
    {
      const char *val = NULL;
      const XML_Char **p;
      void *parsed_value;

      for (p = attrs; *p != NULL; p += 2)
	if (!strcmp (attribute->name, p[0]))
	  {
	    val = p[1];
	    break;
	  }

      if (*p != NULL && val == NULL)
	{
	  gdb_xml_debug (this, _("Attribute \"%s\" missing a value"),
			 attribute->name);
	  continue;
	}

      if (*p == NULL && !(attribute->flags & GDB_XML_AF_OPTIONAL))
	{
	  gdb_xml_error (this, _("Required attribute \"%s\" of "
				   "<%s> not specified"),
			 attribute->name, element->name);
	  continue;
	}

      if (*p == NULL)
	continue;

      gdb_xml_debug (this, _("Parsing attribute %s=\"%s\""),
		     attribute->name, val);

      if (attribute->handler)
	parsed_value = attribute->handler (this, attribute, val);
      else
	parsed_value = xstrdup (val);

      attributes.emplace_back (attribute->name, parsed_value);
    }

  /* Check for unrecognized attributes.  */
  if (debug_xml)
    {
      const XML_Char **p;

      for (p = attrs; *p != NULL; p += 2)
	{
	  for (attribute = element->attributes;
	       attribute != NULL && attribute->name != NULL;
	       attribute++)
	    if (strcmp (attribute->name, *p) == 0)
	      break;

	  if (attribute == NULL || attribute->name == NULL)
	    gdb_xml_debug (this, _("Ignoring unknown attribute %s"), *p);
	}
    }

  /* Call the element handler if there is one.  */
  if (element->start_handler)
    element->start_handler (this, element, m_user_data, attributes);

  /* Fill in a new scope level.  Note that we must delay getting a
     back reference till here because above we might have recursed,
     which may have reallocated the vector which invalidates
     iterators/pointers/references.  */
  scope_level &new_scope = m_scopes.back ();
  new_scope.element = element;
  new_scope.elements = element->children;
}

/* Wrapper for gdb_xml_start_element, to prevent throwing exceptions
   through expat.  */

static void
gdb_xml_start_element_wrapper (void *data, const XML_Char *name,
			       const XML_Char **attrs)
{
  struct gdb_xml_parser *parser = (struct gdb_xml_parser *) data;

  try
    {
      parser->start_element (name, attrs);
    }
  catch (gdb_exception &ex)
    {
      parser->set_error (std::move (ex));
    }
}

/* Handle the end of an element.  NAME is the current element.  */

void
gdb_xml_parser::end_element (const XML_Char *name)
{
  if (m_error.reason < 0)
    return;

  struct scope_level *scope = &m_scopes.back ();
  const struct gdb_xml_element *element;
  unsigned int seen;

  gdb_xml_debug (this, _("Leaving element <%s>"), name);

  for (element = scope->elements, seen = 1;
       element != NULL && element->name != NULL;
       element++, seen <<= 1)
    if ((scope->seen & seen) == 0
	&& (element->flags & GDB_XML_EF_OPTIONAL) == 0)
      gdb_xml_error (this, _("Required element <%s> is missing"),
		     element->name);

  /* Call the element processor.  */
  if (scope->element != NULL && scope->element->end_handler)
    {
      const char *body;

      if (scope->body.empty ())
	body = "";
      else
	{
	  int length;

	  length = scope->body.size ();
	  body = scope->body.c_str ();

	  /* Strip leading and trailing whitespace.  */
	  while (length > 0 && ISSPACE (body[length - 1]))
	    length--;
	  scope->body.erase (length);
	  while (*body && ISSPACE (*body))
	    body++;
	}

      scope->element->end_handler (this, scope->element,
				   m_user_data, body);
    }
  else if (scope->element == NULL)
    XML_DefaultCurrent (m_expat_parser);

  /* Pop the scope level.  */
  m_scopes.pop_back ();
}

/* Wrapper for gdb_xml_end_element, to prevent throwing exceptions
   through expat.  */

static void
gdb_xml_end_element_wrapper (void *data, const XML_Char *name)
{
  struct gdb_xml_parser *parser = (struct gdb_xml_parser *) data;

  try
    {
      parser->end_element (name);
    }
  catch (gdb_exception &ex)
    {
      parser->set_error (std::move (ex));
    }
}

/* Free a parser and all its associated state.  */

gdb_xml_parser::~gdb_xml_parser ()
{
  XML_ParserFree (m_expat_parser);
}

/* Initialize a parser.  */

gdb_xml_parser::gdb_xml_parser (const char *name,
				const gdb_xml_element *elements,
				void *user_data)
  : m_name (name),
    m_user_data (user_data),
    m_last_line (0),
    m_dtd_name (NULL),
    m_is_xinclude (false)
{
  m_expat_parser = XML_ParserCreateNS (NULL, '!');
  if (m_expat_parser == NULL)
    malloc_failure (0);

  XML_SetUserData (m_expat_parser, this);

  /* Set the callbacks.  */
  XML_SetElementHandler (m_expat_parser, gdb_xml_start_element_wrapper,
			 gdb_xml_end_element_wrapper);
  XML_SetCharacterDataHandler (m_expat_parser, gdb_xml_body_text);

  /* Initialize the outer scope.  */
  m_scopes.emplace_back (elements);
}

/* External entity handler.  The only external entities we support
   are those compiled into GDB (we do not fetch entities from the
   target).  */

static int XMLCALL
gdb_xml_fetch_external_entity (XML_Parser expat_parser,
			       const XML_Char *context,
			       const XML_Char *base,
			       const XML_Char *systemId,
			       const XML_Char *publicId)
{
  XML_Parser entity_parser;
  const char *text;
  enum XML_Status status;

  if (systemId == NULL)
    {
      gdb_xml_parser *parser
	= (gdb_xml_parser *) XML_GetUserData (expat_parser);

      text = fetch_xml_builtin (parser->dtd_name ());
      if (text == NULL)
	internal_error (_("could not locate built-in DTD %s"),
			parser->dtd_name ());
    }
  else
    {
      text = fetch_xml_builtin (systemId);
      if (text == NULL)
	return XML_STATUS_ERROR;
    }

  entity_parser = XML_ExternalEntityParserCreate (expat_parser,
						  context, NULL);

  /* Don't use our handlers for the contents of the DTD.  Just let expat
     process it.  */
  XML_SetElementHandler (entity_parser, NULL, NULL);
  XML_SetDoctypeDeclHandler (entity_parser, NULL, NULL);
  XML_SetXmlDeclHandler (entity_parser, NULL);
  XML_SetDefaultHandler (entity_parser, NULL);
  XML_SetUserData (entity_parser, NULL);

  status = XML_Parse (entity_parser, text, strlen (text), 1);

  XML_ParserFree (entity_parser);
  return status;
}

void
gdb_xml_parser::use_dtd (const char *dtd_name)
{
  enum XML_Error err;

  m_dtd_name = dtd_name;

  XML_SetParamEntityParsing (m_expat_parser,
			     XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
  XML_SetExternalEntityRefHandler (m_expat_parser,
				   gdb_xml_fetch_external_entity);

  /* Even if no DTD is provided, use the built-in DTD anyway.  */
  err = XML_UseForeignDTD (m_expat_parser, XML_TRUE);
  if (err != XML_ERROR_NONE)
    internal_error (_("XML_UseForeignDTD failed: %s"),
		    XML_ErrorString (err));
}

/* Invoke PARSER on BUFFER.  BUFFER is the data to parse, which
   should be NUL-terminated.

   The return value is 0 for success or -1 for error.  It may throw,
   but only if something unexpected goes wrong during parsing; parse
   errors will be caught, warned about, and reported as failure.  */

int
gdb_xml_parser::parse (const char *buffer)
{
  enum XML_Status status;
  const char *error_string;

  gdb_xml_debug (this, _("Starting:\n%s"), buffer);

  status = XML_Parse (m_expat_parser, buffer, strlen (buffer), 1);

  if (status == XML_STATUS_OK && m_error.reason == 0)
    return 0;

  if (m_error.reason == RETURN_ERROR
      && m_error.error == XML_PARSE_ERROR)
    {
      gdb_assert (m_error.message != NULL);
      error_string = m_error.what ();
    }
  else if (status == XML_STATUS_ERROR)
    {
      enum XML_Error err = XML_GetErrorCode (m_expat_parser);

      error_string = XML_ErrorString (err);
    }
  else
    {
      gdb_assert (m_error.reason < 0);
      throw_exception (std::move (m_error));
    }

  if (m_last_line != 0)
    warning (_("while parsing %s (at line %d): %s"), m_name,
	     m_last_line, error_string);
  else
    warning (_("while parsing %s: %s"), m_name, error_string);

  return -1;
}

int
gdb_xml_parse_quick (const char *name, const char *dtd_name,
		     const struct gdb_xml_element *elements,
		     const char *document, void *user_data)
{
  gdb_xml_parser parser (name, elements, user_data);
  if (dtd_name != NULL)
    parser.use_dtd (dtd_name);
  return parser.parse (document);
}

/* Parse a field VALSTR that we expect to contain an integer value.
   The integer is returned in *VALP.  The string is parsed with an
   equivalent to strtoul.

   Returns 0 for success, -1 for error.  */

static int
xml_parse_unsigned_integer (const char *valstr, ULONGEST *valp)
{
  const char *endptr;
  ULONGEST result;

  if (*valstr == '\0')
    return -1;

  result = strtoulst (valstr, &endptr, 0);
  if (*endptr != '\0')
    return -1;

  *valp = result;
  return 0;
}

/* Parse an integer string into a ULONGEST and return it, or call
   gdb_xml_error if it could not be parsed.  */

ULONGEST
gdb_xml_parse_ulongest (struct gdb_xml_parser *parser, const char *value)
{
  ULONGEST result;

  if (xml_parse_unsigned_integer (value, &result) != 0)
    gdb_xml_error (parser, _("Can't convert \"%s\" to an integer"), value);

  return result;
}

/* Parse an integer attribute into a ULONGEST.  */

void *
gdb_xml_parse_attr_ulongest (struct gdb_xml_parser *parser,
			     const struct gdb_xml_attribute *attribute,
			     const char *value)
{
  ULONGEST result;
  void *ret;

  if (xml_parse_unsigned_integer (value, &result) != 0)
    gdb_xml_error (parser, _("Can't convert %s=\"%s\" to an integer"),
		   attribute->name, value);

  ret = XNEW (ULONGEST);
  memcpy (ret, &result, sizeof (result));
  return ret;
}

/* A handler_data for yes/no boolean values.  */

const struct gdb_xml_enum gdb_xml_enums_boolean[] = {
  { "yes", 1 },
  { "no", 0 },
  { NULL, 0 }
};

/* Map NAME to VALUE.  A struct gdb_xml_enum * should be saved as the
   value of handler_data when using gdb_xml_parse_attr_enum to parse a
   fixed list of possible strings.  The list is terminated by an entry
   with NAME == NULL.  */

void *
gdb_xml_parse_attr_enum (struct gdb_xml_parser *parser,
			 const struct gdb_xml_attribute *attribute,
			 const char *value)
{
  const struct gdb_xml_enum *enums
    = (const struct gdb_xml_enum *) attribute->handler_data;
  void *ret;

  for (enums = (const struct gdb_xml_enum *) attribute->handler_data;
       enums->name != NULL; enums++)
    if (strcasecmp (enums->name, value) == 0)
      break;

  if (enums->name == NULL)
    gdb_xml_error (parser, _("Unknown attribute value %s=\"%s\""),
		 attribute->name, value);

  ret = xmalloc (sizeof (enums->value));
  memcpy (ret, &enums->value, sizeof (enums->value));
  return ret;
}


/* XInclude processing.  This is done as a separate step from actually
   parsing the document, so that we can produce a single combined XML
   document - e.g. to hand to a front end or to simplify comparing two
   documents.  We make extensive use of XML_DefaultCurrent, to pass
   input text directly into the output without reformatting or
   requoting it.

   We output the DOCTYPE declaration for the first document unchanged,
   if present, and discard DOCTYPEs from included documents.  Only the
   one we pass through here is used when we feed the result back to
   expat.  The XInclude standard explicitly does not discuss
   validation of the result; we choose to apply the same DTD applied
   to the outermost document.

   We can not simply include the external DTD subset in the document
   as an internal subset, because <!IGNORE> and <!INCLUDE> are valid
   only in external subsets.  But if we do not pass the DTD into the
   output at all, default values will not be filled in.

   We don't pass through any <?xml> declaration because we generate
   UTF-8, not whatever the input encoding was.  */

struct xinclude_parsing_data
{
  xinclude_parsing_data (std::string &output_,
			 xml_fetch_another fetcher_,
			 int include_depth_)
    : output (output_),
      skip_depth (0),
      include_depth (include_depth_),
      fetcher (fetcher_)
  {}

  /* Where the output goes.  */
  std::string &output;

  /* A count indicating whether we are in an element whose
     children should not be copied to the output, and if so,
     how deep we are nested.  This is used for anything inside
     an xi:include, and for the DTD.  */
  int skip_depth;

  /* The number of <xi:include> elements currently being processed,
     to detect loops.  */
  int include_depth;

  /* A function to call to obtain additional features, and its
     baton.  */
  xml_fetch_another fetcher;
};

static void
xinclude_start_include (struct gdb_xml_parser *parser,
			const struct gdb_xml_element *element,
			void *user_data,
			std::vector<gdb_xml_value> &attributes)
{
  struct xinclude_parsing_data *data
    = (struct xinclude_parsing_data *) user_data;
  char *href = (char *) xml_find_attribute (attributes, "href")->value.get ();

  gdb_xml_debug (parser, _("Processing XInclude of \"%s\""), href);

  if (data->include_depth > MAX_XINCLUDE_DEPTH)
    gdb_xml_error (parser, _("Maximum XInclude depth (%d) exceeded"),
		   MAX_XINCLUDE_DEPTH);

  std::optional<gdb::char_vector> text = data->fetcher (href);
  if (!text)
    gdb_xml_error (parser, _("Could not load XML document \"%s\""), href);

  if (!xml_process_xincludes (data->output, parser->name (),
			      text->data (), data->fetcher,
			      data->include_depth + 1))
    gdb_xml_error (parser, _("Parsing \"%s\" failed"), href);

  data->skip_depth++;
}

static void
xinclude_end_include (struct gdb_xml_parser *parser,
		      const struct gdb_xml_element *element,
		      void *user_data, const char *body_text)
{
  struct xinclude_parsing_data *data
    = (struct xinclude_parsing_data *) user_data;

  data->skip_depth--;
}

static void XMLCALL
xml_xinclude_default (void *data_, const XML_Char *s, int len)
{
  struct gdb_xml_parser *parser = (struct gdb_xml_parser *) data_;
  xinclude_parsing_data *data = (xinclude_parsing_data *) parser->user_data ();

  /* If we are inside of e.g. xi:include or the DTD, don't save this
     string.  */
  if (data->skip_depth)
    return;

  /* Otherwise just add it to the end of the document we're building
     up.  */
  data->output.append (s, len);
}

static void XMLCALL
xml_xinclude_start_doctype (void *data_, const XML_Char *doctypeName,
			    const XML_Char *sysid, const XML_Char *pubid,
			    int has_internal_subset)
{
  struct gdb_xml_parser *parser = (struct gdb_xml_parser *) data_;
  xinclude_parsing_data *data = (xinclude_parsing_data *) parser->user_data ();

  /* Don't print out the doctype, or the contents of the DTD internal
     subset, if any.  */
  data->skip_depth++;
}

static void XMLCALL
xml_xinclude_end_doctype (void *data_)
{
  struct gdb_xml_parser *parser = (struct gdb_xml_parser *) data_;
  xinclude_parsing_data *data = (xinclude_parsing_data *) parser->user_data ();

  data->skip_depth--;
}

static void XMLCALL
xml_xinclude_xml_decl (void *data_, const XML_Char *version,
		       const XML_Char *encoding, int standalone)
{
  /* Do nothing - this function prevents the default handler from
     being called, thus suppressing the XML declaration from the
     output.  */
}

const struct gdb_xml_attribute xinclude_attributes[] = {
  { "href", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

const struct gdb_xml_element xinclude_elements[] = {
  { "http://www.w3.org/2001/XInclude!include", xinclude_attributes, NULL,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    xinclude_start_include, xinclude_end_include },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* The main entry point for <xi:include> processing.  */

bool
xml_process_xincludes (std::string &result,
		       const char *name, const char *text,
		       xml_fetch_another fetcher, int depth)
{
  xinclude_parsing_data data (result, fetcher, depth);

  gdb_xml_parser parser (name, xinclude_elements, &data);
  parser.set_is_xinclude (true);

  XML_SetCharacterDataHandler (parser.expat_parser (), NULL);
  XML_SetDefaultHandler (parser.expat_parser (), xml_xinclude_default);

  /* Always discard the XML version declarations; the only important
     thing this provides is encoding, and our result will have been
     converted to UTF-8.  */
  XML_SetXmlDeclHandler (parser.expat_parser (), xml_xinclude_xml_decl);

  if (depth > 0)
    /* Discard the doctype for included documents.  */
    XML_SetDoctypeDeclHandler (parser.expat_parser (),
			       xml_xinclude_start_doctype,
			       xml_xinclude_end_doctype);

  parser.use_dtd ("xinclude.dtd");

  if (parser.parse (text) == 0)
    {
      if (depth == 0)
	gdb_xml_debug (&parser, _("XInclude processing succeeded."));
      return true;
    }

  return false;
}
#endif /* HAVE_LIBEXPAT */


/* Return an XML document which was compiled into GDB, from
   the given FILENAME, or NULL if the file was not compiled in.  */

const char *
fetch_xml_builtin (const char *filename)
{
  const char *const (*p)[2];

  for (p = xml_builtin; (*p)[0]; p++)
    if (strcmp ((*p)[0], filename) == 0)
      return (*p)[1];

  return NULL;
}

/* A to_xfer_partial helper function which reads XML files which were
   compiled into GDB.  The target may call this function from its own
   to_xfer_partial handler, after converting object and annex to the
   appropriate filename.  */

LONGEST
xml_builtin_xfer_partial (const char *filename,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  ULONGEST offset, LONGEST len)
{
  const char *buf;
  LONGEST len_avail;

  gdb_assert (readbuf != NULL && writebuf == NULL);
  gdb_assert (filename != NULL);

  buf = fetch_xml_builtin (filename);
  if (buf == NULL)
    return -1;

  len_avail = strlen (buf);
  if (offset >= len_avail)
    return 0;

  if (len > len_avail - offset)
    len = len_avail - offset;
  memcpy (readbuf, buf + offset, len);
  return len;
}


static void
show_debug_xml (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("XML debugging is %s.\n"), value);
}

std::optional<gdb::char_vector>
xml_fetch_content_from_file (const char *filename, const char *dirname)
{
  gdb_file_up file;

  if (dirname != nullptr && *dirname != '\0')
    {
      gdb::unique_xmalloc_ptr<char> fullname
	(concat (dirname, "/", filename, (char *) NULL));

      file = gdb_fopen_cloexec (fullname.get (), FOPEN_RB);
    }
  else
    file = gdb_fopen_cloexec (filename, FOPEN_RB);

  if (file == NULL)
    return {};

  /* Read in the whole file.  */

  size_t len;

  if (fseek (file.get (), 0, SEEK_END) == -1)
    perror_with_name (_("seek to end of file"));
  len = ftell (file.get ());
  rewind (file.get ());

  gdb::char_vector text (len + 1);

  if (fread (text.data (), 1, len, file.get ()) != len
      || ferror (file.get ()))
    {
      warning (_("Read error from \"%s\""), filename);
      return {};
    }

  text.back () = '\0';
  return text;
}

void _initialize_xml_support ();
void _initialize_xml_support ();
void
_initialize_xml_support ()
{
  add_setshow_boolean_cmd ("xml", class_maintenance, &debug_xml,
			   _("Set XML parser debugging."),
			   _("Show XML parser debugging."),
			   _("When set, debugging messages for XML parsers "
			     "are displayed."),
			   NULL, show_debug_xml,
			   &setdebuglist, &showdebuglist);
}

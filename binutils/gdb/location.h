/* Data structures and API for location specs in GDB.
   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

#ifndef LOCATION_H
#define LOCATION_H

#include "symtab.h"

struct language_defn;
struct location_spec;

/* An enumeration of possible signs for a line offset.  */

enum offset_relative_sign
{
  /* No sign  */
  LINE_OFFSET_NONE,

  /* A plus sign ("+")  */
  LINE_OFFSET_PLUS,

  /* A minus sign ("-")  */
  LINE_OFFSET_MINUS,

  /* A special "sign" for unspecified offset.  */
  LINE_OFFSET_UNKNOWN
};

/* A line offset in a location.  */

struct line_offset
{
  /* Line offset and any specified sign.  */
  int offset = 0;
  enum offset_relative_sign sign = LINE_OFFSET_UNKNOWN;
};

/* An enumeration of the various ways to specify a location spec.  */

enum location_spec_type
{
  /* A traditional linespec.  */
  LINESPEC_LOCATION_SPEC,

  /* An address location spec.  */
  ADDRESS_LOCATION_SPEC,

  /* An explicit location spec.  */
  EXPLICIT_LOCATION_SPEC,

  /* A probe location spec.  */
  PROBE_LOCATION_SPEC
};

/* A unique pointer for location_spec.  */
typedef std::unique_ptr<location_spec> location_spec_up;

/* The base class for all location specs used to resolve actual
   locations in the inferior.  */

struct location_spec
{
  virtual ~location_spec () = default;

  /* Clone this object.  */
  virtual location_spec_up clone () const = 0;

  /* Return true if this location spec is empty, false otherwise.  */
  virtual bool empty_p () const = 0;

  /* Return a string representation of this location.

     This function may return NULL for unspecified linespecs, e.g,
     LINESPEC_LOCATION_SPEC and spec_string is NULL.

     The result is cached in the locspec.  */
  const char *to_string () const
  {
    if (m_as_string.empty ())
      m_as_string = compute_string ();
    if (m_as_string.empty ())
      return nullptr;
    return m_as_string.c_str ();
  }

  /* Set this location spec's string representation.  */
  void set_string (std::string &&string)
  {
    m_as_string = std::move (string);
  }

  /* Return this location spec's type.  */
  enum location_spec_type type () const
  {
    return m_type;
  }

protected:

  explicit location_spec (enum location_spec_type t)
    : m_type (t)
  {
  }

  location_spec (enum location_spec_type t, std::string &&str)
    : m_as_string (std::move (str)),
      m_type (t)
  {
  }

  location_spec (const location_spec &other)
    : m_as_string (other.m_as_string),
      m_type (other.m_type)
  {
  }

  /* Compute the string representation of this object.  This is called
     by to_string when needed.  */
  virtual std::string compute_string () const = 0;

  /* Cached string representation of this location spec.  This is
     used, e.g., to save location specs to file.  */
  mutable std::string m_as_string;

private:
  /* The type of this location specification.  */
  enum location_spec_type m_type;
};

/* A "normal" linespec.  */

struct linespec_location_spec : public location_spec
{
  linespec_location_spec (const char **linespec,
			  symbol_name_match_type match_type);

  location_spec_up clone () const override;

  bool empty_p () const override;

  /* Whether the function name is fully-qualified or not.  */
  symbol_name_match_type match_type;

  /* The linespec.  */
  gdb::unique_xmalloc_ptr<char> spec_string;

protected:
  linespec_location_spec (const linespec_location_spec &other);

  std::string compute_string () const override;
};

/* An address in the inferior.  */
struct address_location_spec : public location_spec
{
  address_location_spec (CORE_ADDR addr, const char *addr_string,
			 int addr_string_len);

  location_spec_up clone () const override;

  bool empty_p () const override;

  CORE_ADDR address;

protected:
  address_location_spec (const address_location_spec &other);

  std::string compute_string () const override;
};

/* An explicit location spec.  This structure is used to bypass the
   parsing done on linespecs.  It still has the same requirements
   as linespecs, though.  For example, source_filename requires
   at least one other field.  */

struct explicit_location_spec : public location_spec
{
  explicit explicit_location_spec (const char *function_name);

  explicit_location_spec ()
    : explicit_location_spec (nullptr)
  {
  }

  location_spec_up clone () const override;

  bool empty_p () const override;

  /* Return a linespec string representation of this explicit location
     spec.  The explicit location spec must already be
     canonicalized/valid.  */
  std::string to_linespec () const;

  /* The source filename.  */
  gdb::unique_xmalloc_ptr<char> source_filename;

  /* The function name.  */
  gdb::unique_xmalloc_ptr<char> function_name;

  /* Whether the function name is fully-qualified or not.  */
  symbol_name_match_type func_name_match_type
    = symbol_name_match_type::WILD;

  /* The name of a label.  */
  gdb::unique_xmalloc_ptr<char> label_name;

  /* A line offset relative to the start of the symbol
     identified by the above fields or the current symtab
     if the other fields are NULL.  */
  struct line_offset line_offset;

protected:
  explicit_location_spec (const explicit_location_spec &other);

  std::string compute_string () const override;
};

/* A probe.  */
struct probe_location_spec : public location_spec
{
  explicit probe_location_spec (std::string &&probe);

  location_spec_up clone () const override;

  bool empty_p () const override;

protected:
  probe_location_spec (const probe_location_spec &other) = default;

  std::string compute_string () const override;
};

/* Create a new linespec location spec.  */

extern location_spec_up new_linespec_location_spec
  (const char **linespec, symbol_name_match_type match_type);

/* Return the given location_spec as a linespec_location_spec.
   LOCSPEC must be of type LINESPEC_LOCATION_SPEC.  */

extern const linespec_location_spec *
  as_linespec_location_spec (const location_spec *locspec);

/* Create a new address location spec.
   ADDR is the address corresponding to this location_spec.
   ADDR_STRING, a string of ADDR_STRING_LEN characters, is
   the expression that was parsed to determine the address ADDR.  */

extern location_spec_up new_address_location_spec (CORE_ADDR addr,
						   const char *addr_string,
						   int addr_string_len);

/* Return the given location_spec as an address_location_spec.
   LOCSPEC must be of type ADDRESS_LOCATION_SPEC.  */

const address_location_spec *
  as_address_location_spec (const location_spec *locspec);

/* Create a new probe location.  */

extern location_spec_up new_probe_location_spec (std::string &&probe);

/* Assuming LOCSPEC is of type PROBE_LOCATION_SPEC, return LOCSPEC
   cast to probe_location_spec.  */

const probe_location_spec *
  as_probe_location_spec (const location_spec *locspec);

/* Create a new explicit location with explicit FUNCTION_NAME.  All
   other fields are defaulted.  */

static inline location_spec_up
new_explicit_location_spec_function (const char *function_name)
{
  explicit_location_spec *spec = new explicit_location_spec (function_name);
  return location_spec_up (spec);
}

/* Assuming LOCSPEC is of type EXPLICIT_LOCATION_SPEC, return LOCSPEC
   cast to explicit_location_spec.  */

const explicit_location_spec *
  as_explicit_location_spec (const location_spec *locspec);
explicit_location_spec *
  as_explicit_location_spec (location_spec *locspec);

/* Attempt to convert the input string in *ARGP into a location_spec.
   ARGP is advanced past any processed input.  Always returns a non-nullptr
   location_spec unique pointer object.

   This function may call error() if *ARGP looks like properly formed, but
   invalid, input, e.g., if it is called with missing argument parameters
   or invalid options.

   This function is intended to be used by CLI commands and will parse
   explicit location specs in a CLI-centric way.  Other interfaces should use
   string_to_location_spec_basic if they want to maintain support for
   legacy specifications of probe, address, and linespec location specs.

   MATCH_TYPE should be either WILD or FULL.  If -q/--qualified is specified
   in the input string, it will take precedence over this parameter.  */

extern location_spec_up string_to_location_spec
  (const char **argp, const struct language_defn *language,
   symbol_name_match_type match_type = symbol_name_match_type::WILD);

/* Like string_to_location_spec, but does not attempt to parse
   explicit location specs.  MATCH_TYPE indicates how function names
   should be matched.  */

extern location_spec_up
  string_to_location_spec_basic (const char **argp,
				 const struct language_defn *language,
				 symbol_name_match_type match_type);

/* Structure filled in by string_to_explicit_location_spec to aid the
   completer.  */
struct explicit_completion_info
{
  /* Pointer to the last option found.  E.g., in "b -sou src.c -fun
     func", LAST_OPTION is left pointing at "-fun func".  */
  const char *last_option = NULL;

  /* These point to the start and end of a quoted argument, iff the
     last argument was quoted.  If parsing finds an incomplete quoted
     string (e.g., "break -function 'fun"), then QUOTED_ARG_START is
     set to point to the opening \', and QUOTED_ARG_END is left NULL.
     If the last option is not quoted, then both are set to NULL. */
  const char *quoted_arg_start = NULL;
  const char *quoted_arg_end = NULL;

  /* True if we saw an explicit location spec option, as opposed to
     only flags that affect both explicit location specs and
     linespecs, like "-qualified".  */
  bool saw_explicit_location_spec_option = false;
};

/* Attempt to convert the input string in *ARGP into an explicit
   location spec.  ARGP is advanced past any processed input.  Returns
   a location_spec (malloc'd) if an explicit location spec was
   successfully found in *ARGP, NULL otherwise.

   If COMPLETION_INFO is NULL, this function may call error() if *ARGP
   looks like improperly formed input, e.g., if it is called with
   missing argument parameters or invalid options.  If COMPLETION_INFO
   is not NULL, this function will not throw any exceptions.  */

extern location_spec_up
  string_to_explicit_location_spec (const char **argp,
				    const struct language_defn *language,
				    explicit_completion_info *completion_info);

#endif /* LOCATION_H */

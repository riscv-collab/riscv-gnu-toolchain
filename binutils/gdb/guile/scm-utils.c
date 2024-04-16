/* General utility routines for GDB/Scheme code.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "guile-internal.h"

/* Define VARIABLES in the gdb module.  */

void
gdbscm_define_variables (const scheme_variable *variables, int is_public)
{
  const scheme_variable *sv;

  for (sv = variables; sv->name != NULL; ++sv)
    {
      scm_c_define (sv->name, sv->value);
      if (is_public)
	scm_c_export (sv->name, NULL);
    }
}

/* Define FUNCTIONS in the gdb module.  */

void
gdbscm_define_functions (const scheme_function *functions, int is_public)
{
  const scheme_function *sf;

  for (sf = functions; sf->name != NULL; ++sf)
    {
      SCM proc = scm_c_define_gsubr (sf->name, sf->required, sf->optional,
				     sf->rest, sf->func);

      scm_set_procedure_property_x (proc, gdbscm_documentation_symbol,
				    gdbscm_scm_from_c_string (sf->doc_string));
      if (is_public)
	scm_c_export (sf->name, NULL);
    }
}

/* Define CONSTANTS in the gdb module.  */

void
gdbscm_define_integer_constants (const scheme_integer_constant *constants,
				 int is_public)
{
  const scheme_integer_constant *sc;

  for (sc = constants; sc->name != NULL; ++sc)
    {
      scm_c_define (sc->name, scm_from_int (sc->value));
      if (is_public)
	scm_c_export (sc->name, NULL);
    }
}

/* scm_printf, alas it doesn't exist.  */

void
gdbscm_printf (SCM port, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  std::string string = string_vprintf (format, args);
  va_end (args);
  scm_puts (string.c_str (), port);
}

/* Utility for calling from gdb to "display" an SCM object.  */

void
gdbscm_debug_display (SCM obj)
{
  SCM port = scm_current_output_port ();

  scm_display (obj, port);
  scm_newline (port);
  scm_force_output (port);
}

/* Utility for calling from gdb to "write" an SCM object.  */

void
gdbscm_debug_write (SCM obj)
{
  SCM port = scm_current_output_port ();

  scm_write (obj, port);
  scm_newline (port);
  scm_force_output (port);
}

/* Subroutine of gdbscm_parse_function_args to simplify it.
   Return the number of keyword arguments.  */

static int
count_keywords (const SCM *keywords)
{
  int i;

  if (keywords == NULL)
    return 0;
  for (i = 0; keywords[i] != SCM_BOOL_F; ++i)
    continue;

  return i;
}

/* Subroutine of gdbscm_parse_function_args to simplify it.
   Validate an argument format string.
   The result is a boolean indicating if "." was seen.  */

static int
validate_arg_format (const char *format)
{
  const char *p;
  int length = strlen (format);
  int optional_position = -1;
  int keyword_position = -1;
  int dot_seen = 0;

  gdb_assert (length > 0);

  for (p = format; *p != '\0'; ++p)
    {
      switch (*p)
	{
	case 's':
	case 't':
	case 'i':
	case 'u':
	case 'l':
	case 'n':
	case 'L':
	case 'U':
	case 'O':
	  break;
	case '|':
	  gdb_assert (keyword_position < 0);
	  gdb_assert (optional_position < 0);
	  optional_position = p - format;
	  break;
	case '#':
	  gdb_assert (keyword_position < 0);
	  keyword_position = p - format;
	  break;
	case '.':
	  gdb_assert (p[1] == '\0');
	  dot_seen = 1;
	  break;
	default:
	  gdb_assert_not_reached ("invalid argument format character");
	}
    }

  return dot_seen;
}

/* Our version of SCM_ASSERT_TYPE that calls gdbscm_make_type_error.  */
#define CHECK_TYPE(ok, arg, position, func_name, expected_type)		\
  do {									\
    if (!(ok))								\
      {									\
	return gdbscm_make_type_error ((func_name), (position), (arg),	\
				       (expected_type));		\
      }									\
  } while (0)

/* Subroutine of gdbscm_parse_function_args to simplify it.
   Check the type of ARG against FORMAT_CHAR and extract the value.
   POSITION is the position of ARG in the argument list.
   The result is #f upon success or a <gdb:exception> object.  */

static SCM
extract_arg (char format_char, SCM arg, void *argp,
	     const char *func_name, int position)
{
  switch (format_char)
    {
    case 's':
      {
	char **arg_ptr = (char **) argp;

	CHECK_TYPE (gdbscm_is_true (scm_string_p (arg)), arg, position,
		    func_name, _("string"));
	*arg_ptr = gdbscm_scm_to_c_string (arg).release ();
	break;
      }
    case 't':
      {
	int *arg_ptr = (int *) argp;

	/* While in Scheme, anything non-#f is "true", we're strict.  */
	CHECK_TYPE (gdbscm_is_bool (arg), arg, position, func_name,
		    _("boolean"));
	*arg_ptr = gdbscm_is_true (arg);
	break;
      }
    case 'i':
      {
	int *arg_ptr = (int *) argp;

	CHECK_TYPE (scm_is_signed_integer (arg, INT_MIN, INT_MAX),
		    arg, position, func_name, _("int"));
	*arg_ptr = scm_to_int (arg);
	break;
      }
    case 'u':
      {
	int *arg_ptr = (int *) argp;

	CHECK_TYPE (scm_is_unsigned_integer (arg, 0, UINT_MAX),
		    arg, position, func_name, _("unsigned int"));
	*arg_ptr = scm_to_uint (arg);
	break;
      }
    case 'l':
      {
	long *arg_ptr = (long *) argp;

	CHECK_TYPE (scm_is_signed_integer (arg, LONG_MIN, LONG_MAX),
		    arg, position, func_name, _("long"));
	*arg_ptr = scm_to_long (arg);
	break;
      }
    case 'n':
      {
	unsigned long *arg_ptr = (unsigned long *) argp;

	CHECK_TYPE (scm_is_unsigned_integer (arg, 0, ULONG_MAX),
		    arg, position, func_name, _("unsigned long"));
	*arg_ptr = scm_to_ulong (arg);
	break;
      }
    case 'L':
      {
	LONGEST *arg_ptr = (LONGEST *) argp;

	CHECK_TYPE (scm_is_signed_integer (arg, INT64_MIN, INT64_MAX),
		    arg, position, func_name, _("LONGEST"));
	*arg_ptr = gdbscm_scm_to_longest (arg);
	break;
      }
    case 'U':
      {
	ULONGEST *arg_ptr = (ULONGEST *) argp;

	CHECK_TYPE (scm_is_unsigned_integer (arg, 0, UINT64_MAX),
		    arg, position, func_name, _("ULONGEST"));
	*arg_ptr = gdbscm_scm_to_ulongest (arg);
	break;
      }
    case 'O':
      {
	SCM *arg_ptr = (SCM *) argp;

	*arg_ptr = arg;
	break;
      }
    default:
      gdb_assert_not_reached ("invalid argument format character");
    }

  return SCM_BOOL_F;
}

#undef CHECK_TYPE

/* Look up KEYWORD in KEYWORD_LIST.
   The result is the index of the keyword in the list or -1 if not found.  */

static int
lookup_keyword (const SCM *keyword_list, SCM keyword)
{
  int i = 0;

  while (keyword_list[i] != SCM_BOOL_F)
    {
      if (scm_is_eq (keyword_list[i], keyword))
	return i;
      ++i;
    }

  return -1;
}


/* Helper for gdbscm_parse_function_args that does most of the work,
   in a separate function wrapped with gdbscm_wrap so that we can use
   non-trivial-dtor objects here.  The result is #f upon success or a
   <gdb:exception> object otherwise.  */

static SCM
gdbscm_parse_function_args_1 (const char *func_name,
			      int beginning_arg_pos,
			      const SCM *keywords,
			      const char *format, va_list args)
{
  const char *p;
  int i, have_rest, num_keywords, position;
  int have_optional = 0;
  SCM status;
  SCM rest = SCM_EOL;
  /* Keep track of malloc'd strings.  We need to free them upon error.  */
  std::vector<char *> allocated_strings;

  have_rest = validate_arg_format (format);
  num_keywords = count_keywords (keywords);

  p = format;
  position = beginning_arg_pos;

  /* Process required, optional arguments.  */

  while (*p && *p != '#' && *p != '.')
    {
      SCM arg;
      void *arg_ptr;

      if (*p == '|')
	{
	  have_optional = 1;
	  ++p;
	  continue;
	}

      arg = va_arg (args, SCM);
      if (!have_optional || !SCM_UNBNDP (arg))
	{
	  arg_ptr = va_arg (args, void *);
	  status = extract_arg (*p, arg, arg_ptr, func_name, position);
	  if (!gdbscm_is_false (status))
	    goto fail;
	  if (*p == 's')
	    allocated_strings.push_back (*(char **) arg_ptr);
	}
      ++p;
      ++position;
    }

  /* Process keyword arguments.  */

  if (have_rest || num_keywords > 0)
    rest = va_arg (args, SCM);

  if (num_keywords > 0)
    {
      SCM *keyword_args = XALLOCAVEC (SCM, num_keywords);
      int *keyword_positions = XALLOCAVEC (int, num_keywords);

      gdb_assert (*p == '#');
      ++p;

      for (i = 0; i < num_keywords; ++i)
	{
	  keyword_args[i] = SCM_UNSPECIFIED;
	  keyword_positions[i] = -1;
	}

      while (scm_is_pair (rest)
	     && scm_is_keyword (scm_car (rest)))
	{
	  SCM keyword = scm_car (rest);

	  i = lookup_keyword (keywords, keyword);
	  if (i < 0)
	    {
	      status = gdbscm_make_error (scm_arg_type_key, func_name,
					  _("Unrecognized keyword: ~a"),
					  scm_list_1 (keyword), keyword);
	      goto fail;
	    }
	  if (!scm_is_pair (scm_cdr (rest)))
	    {
	      status = gdbscm_make_error
		(scm_arg_type_key, func_name,
		 _("Missing value for keyword argument"),
		 scm_list_1 (keyword), keyword);
	      goto fail;
	    }
	  keyword_args[i] = scm_cadr (rest);
	  keyword_positions[i] = position + 1;
	  rest = scm_cddr (rest);
	  position += 2;
	}

      for (i = 0; i < num_keywords; ++i)
	{
	  int *arg_pos_ptr = va_arg (args, int *);
	  void *arg_ptr = va_arg (args, void *);
	  SCM arg = keyword_args[i];

	  if (! scm_is_eq (arg, SCM_UNSPECIFIED))
	    {
	      *arg_pos_ptr = keyword_positions[i];
	      status = extract_arg (p[i], arg, arg_ptr, func_name,
				    keyword_positions[i]);
	      if (!gdbscm_is_false (status))
		goto fail;
	      if (p[i] == 's')
		allocated_strings.push_back (*(char **) arg_ptr);
	    }
	}
    }

  /* Process "rest" arguments.  */

  if (have_rest)
    {
      if (num_keywords > 0)
	{
	  SCM *rest_ptr = va_arg (args, SCM *);

	  *rest_ptr = rest;
	}
    }
  else
    {
      if (! scm_is_null (rest))
	{
	  status = gdbscm_make_error (scm_args_number_key, func_name,
				      _("Too many arguments"),
				      SCM_EOL, SCM_BOOL_F);
	  goto fail;
	}
    }

  /* Return anything not-an-exception.  */
  return SCM_BOOL_F;

 fail:
  for (char *ptr : allocated_strings)
    xfree (ptr);

  /* Return the exception, which gdbscm_wrap takes care of
     throwing.  */
  return status;
}

/* Utility to parse required, optional, and keyword arguments to Scheme
   functions.  Modelled on PyArg_ParseTupleAndKeywords, but no attempt is made
   at similarity or functionality.
   There is no result, if there's an error a Scheme exception is thrown.

   Guile provides scm_c_bind_keyword_arguments, and feel free to use it.
   This is for times when we want a bit more parsing.

   BEGINNING_ARG_POS is the position of the first argument passed to this
   routine.  It should be one of the SCM_ARGn values.  It could be > SCM_ARG1
   if the caller chooses not to parse one or more required arguments.

   KEYWORDS may be NULL if there are no keywords.

   FORMAT:
   s - string -> char *, malloc'd
   t - boolean (gdb uses "t", for biT?) -> int
   i - int
   u - unsigned int
   l - long
   n - unsigned long
   L - longest
   U - unsigned longest
   O - random scheme object
   | - indicates the next set is for optional arguments
   # - indicates the next set is for keyword arguments (must follow |)
   . - indicates "rest" arguments are present, this character must appear last

   FORMAT must match the definition from scm_c_{make,define}_gsubr.
   Required and optional arguments appear in order in the format string.
   Afterwards, keyword-based arguments are processed.  There must be as many
   remaining characters in the format string as their are keywords.
   Except for "|#.", the number of characters in the format string must match
   #required + #optional + #keywords.

   The function is required to be defined in a compatible manner:
   #required-args and #optional-arguments must match, and rest-arguments
   must be specified if keyword args are desired, and/or regular "rest" args.

   Example:  For this function,
   scm_c_define_gsubr ("execute", 2, 3, 1, foo);
   the format string + keyword list could be any of:
   1) "ss|ttt#tt", { "key1", "key2", NULL }
   2) "ss|ttt.", { NULL }
   3) "ss|ttt#t.", { "key1", NULL }

   For required and optional args pass the SCM of the argument, and a
   pointer to the value to hold the parsed result (type depends on format
   char).  After that pass the SCM containing the "rest" arguments followed
   by pointers to values to hold parsed keyword arguments, and if specified
   a pointer to hold the remaining contents of "rest".

   For keyword arguments pass two pointers: the first is a pointer to an int
   that will contain the position of the argument in the arg list, and the
   second will contain result of processing the argument.  The int pointed
   to by the first value should be initialized to -1.  It can then be used
   to tell whether the keyword was present.

   If both keyword and rest arguments are present, the caller must pass a
   pointer to contain the new value of rest (after keyword args have been
   removed).

   There's currently no way, that I know of, to specify default values for
   optional arguments in C-provided functions.  At the moment they're a
   work-in-progress.  The caller should test SCM_UNBNDP for each optional
   argument.  Unbound optional arguments are ignored.  */

void
gdbscm_parse_function_args (const char *func_name,
			    int beginning_arg_pos,
			    const SCM *keywords,
			    const char *format, ...)
{
  va_list args;
  va_start (args, format);

  gdbscm_wrap (gdbscm_parse_function_args_1, func_name,
	       beginning_arg_pos, keywords, format, args);

  va_end (args);
}


/* Return longest L as a scheme object.  */

SCM
gdbscm_scm_from_longest (LONGEST l)
{
  return scm_from_int64 (l);
}

/* Convert scheme object L to LONGEST.
   It is an error to call this if L is not an integer in range of LONGEST.
   (because the underlying Scheme function will thrown an exception,
   which is not part of our contract with the caller).  */

LONGEST
gdbscm_scm_to_longest (SCM l)
{
  return scm_to_int64 (l);
}

/* Return unsigned longest L as a scheme object.  */

SCM
gdbscm_scm_from_ulongest (ULONGEST l)
{
  return scm_from_uint64 (l);
}

/* Convert scheme object U to ULONGEST.
   It is an error to call this if U is not an integer in range of ULONGEST
   (because the underlying Scheme function will thrown an exception,
   which is not part of our contract with the caller).  */

ULONGEST
gdbscm_scm_to_ulongest (SCM u)
{
  return scm_to_uint64 (u);
}

/* Same as scm_dynwind_free, but uses xfree.  */

void
gdbscm_dynwind_xfree (void *ptr)
{
  scm_dynwind_unwind_handler (xfree, ptr, SCM_F_WIND_EXPLICITLY);
}

/* Return non-zero if PROC is a procedure.  */

int
gdbscm_is_procedure (SCM proc)
{
  return gdbscm_is_true (scm_procedure_p (proc));
}

/* Same as xstrdup, but the string is allocated on the GC heap.  */

char *
gdbscm_gc_xstrdup (const char *str)
{
  size_t len = strlen (str);
  char *result
    = (char *) scm_gc_malloc_pointerless (len + 1, "gdbscm_gc_xstrdup");

  strcpy (result, str);
  return result;
}

/* Return a duplicate of ARGV living on the GC heap.  */

const char * const *
gdbscm_gc_dup_argv (char **argv)
{
  int i, len;
  size_t string_space;
  char *p, **result;

  for (len = 0, string_space = 0; argv[len] != NULL; ++len)
    string_space += strlen (argv[len]) + 1;

  /* Allocating "pointerless" works because the pointers are all
     self-contained within the object.  */
  result = (char **) scm_gc_malloc_pointerless (((len + 1) * sizeof (char *))
						+ string_space,
						"parameter enum list");
  p = (char *) &result[len + 1];

  for (i = 0; i < len; ++i)
    {
      result[i] = p;
      strcpy (p, argv[i]);
      p += strlen (p) + 1;
    }
  result[i] = NULL;

  return (const char * const *) result;
}

/* Return non-zero if the version of Guile being used it at least
   MAJOR.MINOR.MICRO.  */

int
gdbscm_guile_version_is_at_least (int major, int minor, int micro)
{
  if (major > gdbscm_guile_major_version)
    return 0;
  if (major < gdbscm_guile_major_version)
    return 1;
  if (minor > gdbscm_guile_minor_version)
    return 0;
  if (minor < gdbscm_guile_minor_version)
    return 1;
  if (micro > gdbscm_guile_micro_version)
    return 0;
  return 1;
}

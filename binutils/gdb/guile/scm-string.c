/* GDB/Scheme charset interface.

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
#include "charset.h"
#include "guile-internal.h"
#include "gdbsupport/buildargv.h"

/* Convert STRING to an int.
   STRING must be a valid integer.  */

int
gdbscm_scm_string_to_int (SCM string)
{
  char *s = scm_to_latin1_string (string);
  int r = atoi (s);

  free (s);
  return r;
}

/* Convert a C (latin1) string to an SCM string.
   "latin1" is chosen because Guile won't throw an exception.  */

SCM
gdbscm_scm_from_c_string (const char *string)
{
  return scm_from_latin1_string (string);
}

/* Convert an SCM string to a C (latin1) string.
   "latin1" is chosen because Guile won't throw an exception.
   It is an error to call this if STRING is not a string.  */

gdb::unique_xmalloc_ptr<char>
gdbscm_scm_to_c_string (SCM string)
{
  return gdb::unique_xmalloc_ptr<char> (scm_to_latin1_string (string));
}

/* Use printf to construct a Scheme string.  */

SCM
gdbscm_scm_from_printf (const char *format, ...)
{
  va_list args;
  SCM result;

  va_start (args, format);
  std::string string = string_vprintf (format, args);
  va_end (args);
  result = scm_from_latin1_string (string.c_str ());

  return result;
}

/* Struct to pass data from gdbscm_scm_to_string to
   gdbscm_call_scm_to_stringn.  */

struct scm_to_stringn_data
{
  SCM string;
  size_t *lenp;
  const char *charset;
  scm_t_string_failed_conversion_handler conversion_kind;
  char *result;
};

/* Helper for gdbscm_scm_to_string to call scm_to_stringn
   from within scm_c_catch.  */

static SCM
gdbscm_call_scm_to_stringn (void *datap)
{
  struct scm_to_stringn_data *data = (struct scm_to_stringn_data *) datap;

  data->result = scm_to_stringn (data->string, data->lenp, data->charset,
				 data->conversion_kind);
  return SCM_BOOL_F;
}

/* Convert an SCM string to a string in charset CHARSET.
   This function is guaranteed to not throw an exception.

   If LENP is NULL then the returned string is NUL-terminated,
   and an exception is thrown if the string contains embedded NULs.
   Otherwise the string is not guaranteed to be NUL-terminated, but worse
   there's no space to put a NUL if we wanted to (scm_to_stringn limitation).

   If STRICT is non-zero, and there's a conversion error, then a
   <gdb:exception> object is stored in *EXCEPT_SCMP, and NULL is returned.
   If STRICT is zero, then escape sequences are used for characters that
   can't be converted, and EXCEPT_SCMP may be passed as NULL.

   It is an error to call this if STRING is not a string.  */

gdb::unique_xmalloc_ptr<char>
gdbscm_scm_to_string (SCM string, size_t *lenp,
		      const char *charset, int strict, SCM *except_scmp)
{
  struct scm_to_stringn_data data;
  SCM scm_result;

  data.string = string;
  data.lenp = lenp;
  data.charset = charset;
  data.conversion_kind = (strict
			  ? SCM_FAILED_CONVERSION_ERROR
			  : SCM_FAILED_CONVERSION_ESCAPE_SEQUENCE);
  data.result = NULL;

  scm_result = gdbscm_call_guile (gdbscm_call_scm_to_stringn, &data, NULL);

  if (gdbscm_is_false (scm_result))
    {
      gdb_assert (data.result != NULL);
      return gdb::unique_xmalloc_ptr<char> (data.result);
    }
  gdb_assert (gdbscm_is_exception (scm_result));
  *except_scmp = scm_result;
  return NULL;
}

/* Struct to pass data from gdbscm_scm_from_string to
   gdbscm_call_scm_from_stringn.  */

struct scm_from_stringn_data
{
  const char *string;
  size_t len;
  const char *charset;
  scm_t_string_failed_conversion_handler conversion_kind;
  SCM result;
};

/* Helper for gdbscm_scm_from_string to call scm_from_stringn
   from within scm_c_catch.  */

static SCM
gdbscm_call_scm_from_stringn (void *datap)
{
  struct scm_from_stringn_data *data = (struct scm_from_stringn_data *) datap;

  data->result = scm_from_stringn (data->string, data->len, data->charset,
				   data->conversion_kind);
  return SCM_BOOL_F;
}

/* Convert STRING to a Scheme string in charset CHARSET.
   This function is guaranteed to not throw an exception.

   If STRICT is non-zero, and there's a conversion error, then a
   <gdb:exception> object is returned.
   If STRICT is zero, then question marks are used for characters that
   can't be converted (limitation of underlying Guile conversion support).  */

SCM
gdbscm_scm_from_string (const char *string, size_t len,
			const char *charset, int strict)
{
  struct scm_from_stringn_data data;
  SCM scm_result;

  data.string = string;
  data.len = len;
  data.charset = charset;
  /* The use of SCM_FAILED_CONVERSION_QUESTION_MARK is specified by Guile.  */
  data.conversion_kind = (strict
			  ? SCM_FAILED_CONVERSION_ERROR
			  : SCM_FAILED_CONVERSION_QUESTION_MARK);
  data.result = SCM_UNDEFINED;

  scm_result = gdbscm_call_guile (gdbscm_call_scm_from_stringn, &data, NULL);

  if (gdbscm_is_false (scm_result))
    {
      gdb_assert (!SCM_UNBNDP (data.result));
      return data.result;
    }
  gdb_assert (gdbscm_is_exception (scm_result));
  return scm_result;
}

/* Convert an SCM string to a host string.
   This function is guaranteed to not throw an exception.

   If LENP is NULL then the returned string is NUL-terminated,
   and if the string contains embedded NULs then NULL is returned with
   an exception object stored in *EXCEPT_SCMP.
   Otherwise the string is not guaranteed to be NUL-terminated, but worse
   there's no space to put a NUL if we wanted to (scm_to_stringn limitation).

   Returns NULL if there is a conversion error, with the exception object
   stored in *EXCEPT_SCMP.
   It is an error to call this if STRING is not a string.  */

gdb::unique_xmalloc_ptr<char>
gdbscm_scm_to_host_string (SCM string, size_t *lenp, SCM *except_scmp)
{
  return gdbscm_scm_to_string (string, lenp, host_charset (), 1, except_scmp);
}

/* Convert a host string to an SCM string.
   This function is guaranteed to not throw an exception.
   Returns a <gdb:exception> object if there's a conversion error.  */

SCM
gdbscm_scm_from_host_string (const char *string, size_t len)
{
  return gdbscm_scm_from_string (string, len, host_charset (), 1);
}

/* (string->argv string) -> list
   Return list of strings split up according to GDB's argv parsing rules.
   This is useful when writing GDB commands in Scheme.  */

static SCM
gdbscm_string_to_argv (SCM string_scm)
{
  char *string;
  SCM result = SCM_EOL;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "s",
			      string_scm, &string);

  if (string == NULL || *string == '\0')
    {
      xfree (string);
      return SCM_EOL;
    }

  gdb_argv c_argv (string);
  for (char *arg : c_argv)
    result = scm_cons (gdbscm_scm_from_c_string (arg), result);

  xfree (string);

  return scm_reverse_x (result, SCM_EOL);
}

/* Initialize the Scheme charset interface to GDB.  */

static const scheme_function string_functions[] =
{
  { "string->argv", 1, 0, 0, as_a_scm_t_subr (gdbscm_string_to_argv),
  "\
Convert a string to a list of strings split up according to\n\
gdb's argv parsing rules." },

  END_FUNCTIONS
};

void
gdbscm_initialize_strings (void)
{
  gdbscm_define_functions (string_functions, 1);
}

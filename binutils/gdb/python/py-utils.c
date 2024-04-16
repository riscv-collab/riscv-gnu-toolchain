/* General utility routines for GDB/Python.

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
#include "top.h"
#include "charset.h"
#include "value.h"
#include "python-internal.h"

/* Converts a Python 8-bit string to a unicode string object.  Assumes the
   8-bit string is in the host charset.  If an error occurs during conversion,
   returns NULL with a python exception set.

   As an added bonus, the functions accepts a unicode string and returns it
   right away, so callers don't need to check which kind of string they've
   got.  In Python 3, all strings are Unicode so this case is always the
   one that applies.

   If the given object is not one of the mentioned string types, NULL is
   returned, with the TypeError python exception set.  */
gdbpy_ref<>
python_string_to_unicode (PyObject *obj)
{
  PyObject *unicode_str;

  /* If obj is already a unicode string, just return it.
     I wish life was always that simple...  */
  if (PyUnicode_Check (obj))
    {
      unicode_str = obj;
      Py_INCREF (obj);
    }
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Expected a string object."));
      unicode_str = NULL;
    }

  return gdbpy_ref<> (unicode_str);
}

/* Returns a newly allocated string with the contents of the given unicode
   string object converted to CHARSET.  If an error occurs during the
   conversion, NULL will be returned and a python exception will be
   set.  */
static gdb::unique_xmalloc_ptr<char>
unicode_to_encoded_string (PyObject *unicode_str, const char *charset)
{
  /* Translate string to named charset.  */
  gdbpy_ref<> string (PyUnicode_AsEncodedString (unicode_str, charset, NULL));
  if (string == NULL)
    return NULL;

  return gdb::unique_xmalloc_ptr<char>
    (xstrdup (PyBytes_AsString (string.get ())));
}

/* Returns a PyObject with the contents of the given unicode string
   object converted to a named charset.  If an error occurs during
   the conversion, NULL will be returned and a python exception will
   be set.  */
static gdbpy_ref<>
unicode_to_encoded_python_string (PyObject *unicode_str, const char *charset)
{
  /* Translate string to named charset.  */
  return gdbpy_ref<> (PyUnicode_AsEncodedString (unicode_str, charset, NULL));
}

/* Returns a newly allocated string with the contents of the given
   unicode string object converted to the target's charset.  If an
   error occurs during the conversion, NULL will be returned and a
   python exception will be set.  */
gdb::unique_xmalloc_ptr<char>
unicode_to_target_string (PyObject *unicode_str)
{
  return (unicode_to_encoded_string
	  (unicode_str,
	   target_charset (gdbpy_enter::get_gdbarch ())));
}

/* Returns a PyObject with the contents of the given unicode string
   object converted to the target's charset.  If an error occurs
   during the conversion, NULL will be returned and a python exception
   will be set.  */
static gdbpy_ref<>
unicode_to_target_python_string (PyObject *unicode_str)
{
  return (unicode_to_encoded_python_string
	  (unicode_str,
	   target_charset (gdbpy_enter::get_gdbarch ())));
}

/* Converts a python string (8-bit or unicode) to a target string in
   the target's charset.  Returns NULL on error, with a python
   exception set.  */
gdb::unique_xmalloc_ptr<char>
python_string_to_target_string (PyObject *obj)
{
  gdbpy_ref<> str = python_string_to_unicode (obj);
  if (str == NULL)
    return NULL;

  return unicode_to_target_string (str.get ());
}

/* Converts a python string (8-bit or unicode) to a target string in the
   target's charset.  Returns NULL on error, with a python exception
   set.

   In Python 3, the returned object is a "bytes" object (not a string).  */
gdbpy_ref<>
python_string_to_target_python_string (PyObject *obj)
{
  gdbpy_ref<> str = python_string_to_unicode (obj);
  if (str == NULL)
    return str;

  return unicode_to_target_python_string (str.get ());
}

/* Converts a python string (8-bit or unicode) to a target string in
   the host's charset.  Returns NULL on error, with a python exception
   set.  */
gdb::unique_xmalloc_ptr<char>
python_string_to_host_string (PyObject *obj)
{
  gdbpy_ref<> str = python_string_to_unicode (obj);
  if (str == NULL)
    return NULL;

  return unicode_to_encoded_string (str.get (), host_charset ());
}

/* Convert a host string to a python string.  */

gdbpy_ref<>
host_string_to_python_string (const char *str)
{
  return gdbpy_ref<> (PyUnicode_Decode (str, strlen (str), host_charset (),
					NULL));
}

/* Return true if OBJ is a Python string or unicode object, false
   otherwise.  */

int
gdbpy_is_string (PyObject *obj)
{
  return PyUnicode_Check (obj);
}

/* Return the string representation of OBJ, i.e., str (obj).
   If the result is NULL a python error occurred, the caller must clear it.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_obj_to_string (PyObject *obj)
{
  gdbpy_ref<> str_obj (PyObject_Str (obj));

  if (str_obj != NULL)
    return python_string_to_host_string (str_obj.get ());

  return NULL;
}

/* See python-internal.h.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_err_fetch::to_string () const
{
  /* There are a few cases to consider.
     For example:
     value is a string when PyErr_SetString is used.
     value is not a string when raise "foo" is used, instead it is None
     and type is "foo".
     So the algorithm we use is to print `str (value)' if it's not
     None, otherwise we print `str (type)'.
     Using str (aka PyObject_Str) will fetch the error message from
     gdb.GdbError ("message").  */

  if (m_error_value.get () != nullptr && m_error_value.get () != Py_None)
    return gdbpy_obj_to_string (m_error_value.get ());
  else
    return gdbpy_obj_to_string (m_error_type.get ());
}

/* See python-internal.h.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_err_fetch::type_to_string () const
{
  return gdbpy_obj_to_string (m_error_type.get ());
}

/* Convert a GDB exception to the appropriate Python exception.

   This sets the Python error indicator.  */

void
gdbpy_convert_exception (const struct gdb_exception &exception)
{
  PyObject *exc_class;

  if (exception.reason == RETURN_QUIT)
    exc_class = PyExc_KeyboardInterrupt;
  else if (exception.reason == RETURN_FORCED_QUIT)
    quit_force (NULL, 0);
  else if (exception.error == MEMORY_ERROR)
    exc_class = gdbpy_gdb_memory_error;
  else
    exc_class = gdbpy_gdb_error;

  PyErr_Format (exc_class, "%s", exception.what ());
}

/* Converts OBJ to a CORE_ADDR value.

   Returns 0 on success or -1 on failure, with a Python exception set.
*/

int
get_addr_from_python (PyObject *obj, CORE_ADDR *addr)
{
  if (gdbpy_is_value_object (obj))
    {

      try
	{
	  *addr = value_as_address (value_object_to_value (obj));
	}
      catch (const gdb_exception &except)
	{
	  GDB_PY_SET_HANDLE_EXCEPTION (except);
	}
    }
  else
    {
      gdbpy_ref<> num (PyNumber_Long (obj));
      gdb_py_ulongest val;

      if (num == NULL)
	return -1;

      val = gdb_py_long_as_ulongest (num.get ());
      if (PyErr_Occurred ())
	return -1;

      if (sizeof (val) > sizeof (CORE_ADDR) && ((CORE_ADDR) val) != val)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("Overflow converting to address."));
	  return -1;
	}

      *addr = val;
    }

  return 0;
}

/* Convert a LONGEST to the appropriate Python object -- either an
   integer object or a long object, depending on its value.  */

gdbpy_ref<>
gdb_py_object_from_longest (LONGEST l)
{
  if (sizeof (l) > sizeof (long))
    return gdbpy_ref<> (PyLong_FromLongLong (l));
  return gdbpy_ref<> (PyLong_FromLong (l));
}

/* Convert a ULONGEST to the appropriate Python object -- either an
   integer object or a long object, depending on its value.  */

gdbpy_ref<>
gdb_py_object_from_ulongest (ULONGEST l)
{
  if (sizeof (l) > sizeof (unsigned long))
    return gdbpy_ref<> (PyLong_FromUnsignedLongLong (l));
  return gdbpy_ref<> (PyLong_FromUnsignedLong (l));
}

/* Like PyLong_AsLong, but returns 0 on failure, 1 on success, and puts
   the value into an out parameter.  */

int
gdb_py_int_as_long (PyObject *obj, long *result)
{
  *result = PyLong_AsLong (obj);
  return ! (*result == -1 && PyErr_Occurred ());
}



/* Generic implementation of the __dict__ attribute for objects that
   have a dictionary.  The CLOSURE argument should be the type object.
   This only handles positive values for tp_dictoffset.  */

PyObject *
gdb_py_generic_dict (PyObject *self, void *closure)
{
  PyObject *result;
  PyTypeObject *type_obj = (PyTypeObject *) closure;
  char *raw_ptr;

  raw_ptr = (char *) self + type_obj->tp_dictoffset;
  result = * (PyObject **) raw_ptr;

  Py_INCREF (result);
  return result;
}

/* Like PyModule_AddObject, but does not steal a reference to
   OBJECT.  */

int
gdb_pymodule_addobject (PyObject *module, const char *name, PyObject *object)
{
  int result;

  Py_INCREF (object);
  result = PyModule_AddObject (module, name, object);
  if (result < 0)
    Py_DECREF (object);
  return result;
}

/* See python-internal.h.  */

void
gdbpy_error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  std::string str = string_vprintf (fmt, ap);
  va_end (ap);

  const char *msg = str.c_str ();
  if (msg != nullptr && *msg != '\0')
    error (_("Error occurred in Python: %s"), msg);
  else
    error (_("Error occurred in Python."));
}

/* Handle a Python exception when the special gdb.GdbError treatment
   is desired.  This should only be called when an exception is set.
   If the exception is a gdb.GdbError, throw a gdb exception with the
   exception text.  For other exceptions, print the Python stack and
   then throw a gdb exception.  */

void
gdbpy_handle_exception ()
{
  gdbpy_err_fetch fetched_error;
  gdb::unique_xmalloc_ptr<char> msg = fetched_error.to_string ();

  if (msg == NULL)
    {
      /* An error occurred computing the string representation of the
	 error message.  This is rare, but we should inform the user.  */
      gdb_printf (_("An error occurred in Python "
		    "and then another occurred computing the "
		    "error message.\n"));
      gdbpy_print_stack ();
    }

  /* Don't print the stack for gdb.GdbError exceptions.
     It is generally used to flag user errors.

     We also don't want to print "Error occurred in Python command"
     for user errors.  However, a missing message for gdb.GdbError
     exceptions is arguably a bug, so we flag it as such.  */

  if (fetched_error.type_matches (PyExc_KeyboardInterrupt))
    throw_quit ("Quit");
  else if (! fetched_error.type_matches (gdbpy_gdberror_exc)
	   || msg == NULL || *msg == '\0')
    {
      fetched_error.restore ();
      gdbpy_print_stack ();
      if (msg != NULL && *msg != '\0')
	error (_("Error occurred in Python: %s"), msg.get ());
      else
	error (_("Error occurred in Python."));
    }
  else
    error ("%s", msg.get ());
}

/* See python-internal.h.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_fix_doc_string_indentation (gdb::unique_xmalloc_ptr<char> doc)
{
  /* A structure used to track the white-space information on each line of
     DOC.  */
  struct line_whitespace
  {
    /* Constructor.  OFFSET is the offset from the start of DOC, WS_COUNT
       is the number of whitespace characters starting at OFFSET.  */
    line_whitespace (size_t offset, int ws_count)
      : m_offset (offset),
	m_ws_count (ws_count)
    { /* Nothing.  */ }

    /* The offset from the start of DOC.  */
    size_t offset () const
    { return m_offset; }

    /* The number of white-space characters at the start of this line.  */
    int ws () const
    { return m_ws_count; }

  private:
    /* The offset from the start of DOC to the first character of this
       line.  */
    size_t m_offset;

    /* White space count on this line, the first character of this
       whitespace is at OFFSET.  */
    int m_ws_count;
  };

  /* Count the number of white-space character starting at TXT.  We
     currently only count true single space characters, things like tabs,
     newlines, etc are not counted.  */
  auto count_whitespace = [] (const char *txt) -> int
  {
    int count = 0;

    while (*txt == ' ')
      {
	++txt;
	++count;
      }

    return count;
  };

  /* In MIN_WHITESPACE we track the smallest number of whitespace
     characters seen at the start of a line (that has actual content), this
     is the number of characters that we can delete off all lines without
     altering the relative indentation of all lines in DOC.

     The first line often has no indentation, but instead starts immediates
     after the 3-quotes marker within the Python doc string, so, if the
     first line has zero white-space then we just ignore it, and don't set
     MIN_WHITESPACE to zero.

     Lines without any content should (ideally) have no white-space at
     all, but if they do then they might have an artificially low number
     (user left a single stray space at the start of an otherwise blank
     line), we don't consider lines without content when updating the
     MIN_WHITESPACE value.  */
  std::optional<int> min_whitespace;

  /* The index into WS_INFO at which the processing of DOC can be
     considered "all done", that is, after this point there are no further
     lines with useful content and we should just stop.  */
  std::optional<size_t> all_done_idx;

  /* White-space information for each line in DOC.  */
  std::vector<line_whitespace> ws_info;

  /* Now look through DOC and collect the required information.  */
  const char *tmp = doc.get ();
  while (*tmp != '\0')
    {
      /* Add an entry for the offset to the start of this line, and how
	 much white-space there is at the start of this line.  */
      size_t offset = tmp - doc.get ();
      int ws_count = count_whitespace (tmp);
      ws_info.emplace_back (offset, ws_count);

      /* Skip over the white-space.  */
      tmp += ws_count;

      /* Remember where the content of this line starts, and skip forward
	 to either the end of this line (newline) or the end of the DOC
	 string (null character), whichever comes first.  */
      const char *content_start = tmp;
      while (*tmp != '\0' && *tmp != '\n')
	++tmp;

      /* If this is not the first line, and if this line has some content,
	 then update MIN_WHITESPACE, this reflects the smallest number of
	 whitespace characters we can delete from all lines without
	 impacting the relative indentation of all the lines of DOC.  */
      if (offset > 0 && tmp > content_start)
	{
	  if (!min_whitespace.has_value ())
	    min_whitespace = ws_count;
	  else
	    min_whitespace = std::min (*min_whitespace, ws_count);
	}

      /* Each time we encounter a line that has some content we update
	 ALL_DONE_IDX to be the index of the next line.  If the last lines
	 of DOC don't contain any content then ALL_DONE_IDX will be left
	 pointing at an earlier line.  When we rewrite DOC, when we reach
	 ALL_DONE_IDX then we can stop, the allows us to trim any blank
	 lines from the end of DOC.  */
      if (tmp > content_start)
	all_done_idx = ws_info.size ();

      /* If we reached a newline then skip forward to the start of the next
	 line.  The other possibility at this point is that we're at the
	 very end of the DOC string (null terminator).  */
      if (*tmp == '\n')
	++tmp;
    }

  /* We found no lines with content, fail safe by just returning the
     original documentation string.  */
  if (!all_done_idx.has_value () || !min_whitespace.has_value ())
    return doc;

  /* Setup DST and SRC, both pointing into the DOC string.  We're going to
     rewrite DOC in-place, as we only ever make DOC shorter (by removing
     white-space), thus we know this will not overflow.  */
  char *dst = doc.get ();
  char *src = doc.get ();

  /* Array indices used with DST, SRC, and WS_INFO respectively.  */
  size_t dst_offset = 0;
  size_t src_offset = 0;
  size_t ws_info_offset = 0;

  /* Now, walk over the source string, this is the original DOC.  */
  while (src[src_offset] != '\0')
    {
      /* If we are at the start of the next line (in WS_INFO), then we may
	 need to skip some white-space characters.  */
      if (src_offset == ws_info[ws_info_offset].offset ())
	{
	  /* If a line has leading white-space then we need to skip over
	     some number of characters now.  */
	  if (ws_info[ws_info_offset].ws () > 0)
	    {
	      /* If the line is entirely white-space then we skip all of
		 the white-space, the next character to copy will be the
		 newline or null character.  Otherwise, we skip the just
		 some portion of the leading white-space.  */
	      if (src[src_offset + ws_info[ws_info_offset].ws ()] == '\n'
		  || src[src_offset + ws_info[ws_info_offset].ws ()] == '\0')
		src_offset += ws_info[ws_info_offset].ws ();
	      else
		src_offset += std::min (*min_whitespace,
					ws_info[ws_info_offset].ws ());

	      /* If we skipped white-space, and are now at the end of the
		 input, then we're done.  */
	      if (src[src_offset] == '\0')
		break;
	    }
	  if (ws_info_offset < (ws_info.size () - 1))
	    ++ws_info_offset;
	  if (ws_info_offset > *all_done_idx)
	    break;
	}

      /* Don't copy a newline to the start of the DST string, this would
	 result in a leading blank line.  But in all other cases, copy the
	 next character into the destination string.  */
      if ((dst_offset > 0 || src[src_offset] != '\n'))
	{
	  dst[dst_offset] = src[src_offset];
	  ++dst_offset;
	}

      /* Move to the next source character.  */
      ++src_offset;
    }

  /* Remove the trailing newline character(s), and ensure we have a null
     terminator in place.  */
  while (dst_offset > 1 && dst[dst_offset - 1] == '\n')
    --dst_offset;
  dst[dst_offset] = '\0';

  return doc;
}

/* See python-internal.h.  */

PyObject *
gdb_py_invalid_object_repr (PyObject *self)
{
  return PyUnicode_FromFormat ("<%s (invalid)>", Py_TYPE (self)->tp_name);
}

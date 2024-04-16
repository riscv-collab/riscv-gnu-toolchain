/* Python pretty-printing

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
#include "objfiles.h"
#include "symtab.h"
#include "language.h"
#include "valprint.h"
#include "extension-priv.h"
#include "python.h"
#include "python-internal.h"
#include "cli/cli-style.h"

extern PyTypeObject printer_object_type;

/* Return type of print_string_repr.  */

enum gdbpy_string_repr_result
  {
    /* The string method returned None.  */
    string_repr_none,
    /* The string method had an error.  */
    string_repr_error,
    /* Everything ok.  */
    string_repr_ok
  };

/* If non-null, points to options that are in effect while
   printing.  */
const struct value_print_options *gdbpy_current_print_options;

/* Helper function for find_pretty_printer which iterates over a list,
   calls each function and inspects output.  This will return a
   printer object if one recognizes VALUE.  If no printer is found, it
   will return None.  On error, it will set the Python error and
   return NULL.  */

static gdbpy_ref<>
search_pp_list (PyObject *list, PyObject *value)
{
  Py_ssize_t pp_list_size, list_index;

  pp_list_size = PyList_Size (list);
  for (list_index = 0; list_index < pp_list_size; list_index++)
    {
      PyObject *function = PyList_GetItem (list, list_index);
      if (! function)
	return NULL;

      /* Skip if disabled.  */
      if (PyObject_HasAttr (function, gdbpy_enabled_cst))
	{
	  gdbpy_ref<> attr (PyObject_GetAttr (function, gdbpy_enabled_cst));
	  int cmp;

	  if (attr == NULL)
	    return NULL;
	  cmp = PyObject_IsTrue (attr.get ());
	  if (cmp == -1)
	    return NULL;

	  if (!cmp)
	    continue;
	}

      gdbpy_ref<> printer (PyObject_CallFunctionObjArgs (function, value,
							 NULL));
      if (printer == NULL)
	return NULL;
      else if (printer != Py_None)
	return printer;
    }

  return gdbpy_ref<>::new_reference (Py_None);
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in all objfiles.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static PyObject *
find_pretty_printer_from_objfiles (PyObject *value)
{
  for (objfile *obj : current_program_space->objfiles ())
    {
      gdbpy_ref<> objf = objfile_to_objfile_object (obj);
      if (objf == NULL)
	{
	  /* Ignore the error and continue.  */
	  PyErr_Clear ();
	  continue;
	}

      gdbpy_ref<> pp_list (objfpy_get_printers (objf.get (), NULL));
      gdbpy_ref<> function (search_pp_list (pp_list.get (), value));

      /* If there is an error in any objfile list, abort the search and exit.  */
      if (function == NULL)
	return NULL;

      if (function != Py_None)
	return function.release ();
    }

  Py_RETURN_NONE;
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the current program space.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static gdbpy_ref<>
find_pretty_printer_from_progspace (PyObject *value)
{
  gdbpy_ref<> obj = pspace_to_pspace_object (current_program_space);

  if (obj == NULL)
    return NULL;
  gdbpy_ref<> pp_list (pspy_get_printers (obj.get (), NULL));
  return search_pp_list (pp_list.get (), value);
}

/* Subroutine of find_pretty_printer to simplify it.
   Look for a pretty-printer to print VALUE in the gdb module.
   The result is NULL if there's an error and the search should be terminated.
   The result is Py_None, suitably inc-ref'd, if no pretty-printer was found.
   Otherwise the result is the pretty-printer function, suitably inc-ref'd.  */

static gdbpy_ref<>
find_pretty_printer_from_gdb (PyObject *value)
{
  /* Fetch the global pretty printer list.  */
  if (gdb_python_module == NULL
      || ! PyObject_HasAttrString (gdb_python_module, "pretty_printers"))
    return gdbpy_ref<>::new_reference (Py_None);
  gdbpy_ref<> pp_list (PyObject_GetAttrString (gdb_python_module,
					       "pretty_printers"));
  if (pp_list == NULL || ! PyList_Check (pp_list.get ()))
    return gdbpy_ref<>::new_reference (Py_None);

  return search_pp_list (pp_list.get (), value);
}

/* Find the pretty-printing constructor function for VALUE.  If no
   pretty-printer exists, return None.  If one exists, return a new
   reference.  On error, set the Python error and return NULL.  */

static gdbpy_ref<>
find_pretty_printer (PyObject *value)
{
  /* Look at the pretty-printer list for each objfile
     in the current program-space.  */
  gdbpy_ref<> function (find_pretty_printer_from_objfiles (value));
  if (function == NULL || function != Py_None)
    return function;

  /* Look at the pretty-printer list for the current program-space.  */
  function = find_pretty_printer_from_progspace (value);
  if (function == NULL || function != Py_None)
    return function;

  /* Look at the pretty-printer list in the gdb module.  */
  return find_pretty_printer_from_gdb (value);
}

/* Pretty-print a single value, via the printer object PRINTER.
   If the function returns a string, a PyObject containing the string
   is returned.  If the function returns Py_NONE that means the pretty
   printer returned the Python None as a value.  Otherwise, if the
   function returns a value,  *OUT_VALUE is set to the value, and NULL
   is returned.  On error, *OUT_VALUE is set to NULL, NULL is
   returned, with a python exception set.  */

static gdbpy_ref<>
pretty_print_one_value (PyObject *printer, struct value **out_value)
{
  gdbpy_ref<> result;

  *out_value = NULL;
  try
    {
      if (!PyObject_HasAttr (printer, gdbpy_to_string_cst))
	result = gdbpy_ref<>::new_reference (Py_None);
      else
	{
	  result.reset (PyObject_CallMethodObjArgs (printer, gdbpy_to_string_cst,
						    NULL));
	  if (result != NULL)
	    {
	      if (! gdbpy_is_string (result.get ())
		  && ! gdbpy_is_lazy_string (result.get ())
		  && result != Py_None)
		{
		  *out_value = convert_value_from_python (result.get ());
		  if (PyErr_Occurred ())
		    *out_value = NULL;
		  result = NULL;
		}
	    }
	}
    }
  catch (const gdb_exception &except)
    {
    }

  return result;
}

/* Return the display hint for the object printer, PRINTER.  Return
   NULL if there is no display_hint method, or if the method did not
   return a string.  On error, print stack trace and return NULL.  On
   success, return an xmalloc()d string.  */
gdb::unique_xmalloc_ptr<char>
gdbpy_get_display_hint (PyObject *printer)
{
  gdb::unique_xmalloc_ptr<char> result;

  if (! PyObject_HasAttr (printer, gdbpy_display_hint_cst))
    return NULL;

  gdbpy_ref<> hint (PyObject_CallMethodObjArgs (printer, gdbpy_display_hint_cst,
						NULL));
  if (hint != NULL)
    {
      if (gdbpy_is_string (hint.get ()))
	{
	  result = python_string_to_host_string (hint.get ());
	  if (result == NULL)
	    gdbpy_print_stack ();
	}
    }
  else
    gdbpy_print_stack ();

  return result;
}

/* A wrapper for gdbpy_print_stack that ignores MemoryError.  */

static void
print_stack_unless_memory_error (struct ui_file *stream)
{
  if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
    {
      gdbpy_err_fetch fetched_error;
      gdb::unique_xmalloc_ptr<char> msg = fetched_error.to_string ();

      if (msg == NULL || *msg == '\0')
	fprintf_styled (stream, metadata_style.style (),
			_("<error reading variable>"));
      else
	fprintf_styled (stream, metadata_style.style (),
			_("<error reading variable: %s>"), msg.get ());
    }
  else
    gdbpy_print_stack ();
}

/* Helper for gdbpy_apply_val_pretty_printer which calls to_string and
   formats the result.  */

static enum gdbpy_string_repr_result
print_string_repr (PyObject *printer, const char *hint,
		   struct ui_file *stream, int recurse,
		   const struct value_print_options *options,
		   const struct language_defn *language,
		   struct gdbarch *gdbarch)
{
  struct value *replacement = NULL;
  enum gdbpy_string_repr_result result = string_repr_ok;

  gdbpy_ref<> py_str = pretty_print_one_value (printer, &replacement);
  if (py_str != NULL)
    {
      if (py_str == Py_None)
	result = string_repr_none;
      else if (gdbpy_is_lazy_string (py_str.get ()))
	{
	  CORE_ADDR addr;
	  long length;
	  struct type *type;
	  gdb::unique_xmalloc_ptr<char> encoding;
	  struct value_print_options local_opts = *options;

	  gdbpy_extract_lazy_string (py_str.get (), &addr, &type,
				     &length, &encoding);

	  local_opts.addressprint = false;
	  val_print_string (type, encoding.get (), addr, (int) length,
			    stream, &local_opts);
	}
      else
	{
	  gdbpy_ref<> string
	    = python_string_to_target_python_string (py_str.get ());
	  if (string != NULL)
	    {
	      char *output;
	      long length;
	      struct type *type;

	      output = PyBytes_AS_STRING (string.get ());
	      length = PyBytes_GET_SIZE (string.get ());
	      type = builtin_type (gdbarch)->builtin_char;

	      if (hint && !strcmp (hint, "string"))
		language->printstr (stream, type, (gdb_byte *) output,
				    length, NULL, 0, options);
	      else
		gdb_puts (output, stream);
	    }
	  else
	    {
	      result = string_repr_error;
	      print_stack_unless_memory_error (stream);
	    }
	}
    }
  else if (replacement)
    {
      struct value_print_options opts = *options;

      opts.addressprint = false;
      common_val_print (replacement, stream, recurse, &opts, language);
    }
  else
    {
      result = string_repr_error;
      print_stack_unless_memory_error (stream);
    }

  return result;
}

/* Helper for gdbpy_apply_val_pretty_printer that formats children of the
   printer, if any exist.  If is_py_none is true, then nothing has
   been printed by to_string, and format output accordingly. */
static void
print_children (PyObject *printer, const char *hint,
		struct ui_file *stream, int recurse,
		const struct value_print_options *options,
		const struct language_defn *language,
		int is_py_none)
{
  int is_map, is_array, done_flag, pretty;
  unsigned int i;

  if (! PyObject_HasAttr (printer, gdbpy_children_cst))
    return;

  /* If we are printing a map or an array, we want some special
     formatting.  */
  is_map = hint && ! strcmp (hint, "map");
  is_array = hint && ! strcmp (hint, "array");

  gdbpy_ref<> children (PyObject_CallMethodObjArgs (printer, gdbpy_children_cst,
						    NULL));
  if (children == NULL)
    {
      print_stack_unless_memory_error (stream);
      return;
    }

  gdbpy_ref<> iter (PyObject_GetIter (children.get ()));
  if (iter == NULL)
    {
      print_stack_unless_memory_error (stream);
      return;
    }

  /* Use the prettyformat_arrays option if we are printing an array,
     and the pretty option otherwise.  */
  if (is_array)
    pretty = options->prettyformat_arrays;
  else
    {
      if (options->prettyformat == Val_prettyformat)
	pretty = 1;
      else
	pretty = options->prettyformat_structs;
    }

  done_flag = 0;
  for (i = 0; i < options->print_max; ++i)
    {
      PyObject *py_v;
      const char *name;

      gdbpy_ref<> item (PyIter_Next (iter.get ()));
      if (item == NULL)
	{
	  if (PyErr_Occurred ())
	    print_stack_unless_memory_error (stream);
	  /* Set a flag so we can know whether we printed all the
	     available elements.  */
	  else	
	    done_flag = 1;
	  break;
	}

      if (! PyTuple_Check (item.get ()) || PyTuple_Size (item.get ()) != 2)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("Result of children iterator not a tuple"
			     " of two elements."));
	  gdbpy_print_stack ();
	  continue;
	}
      if (! PyArg_ParseTuple (item.get (), "sO", &name, &py_v))
	{
	  /* The user won't necessarily get a stack trace here, so provide
	     more context.  */
	  if (gdbpy_print_python_errors_p ())
	    gdb_printf (gdb_stderr,
			_("Bad result from children iterator.\n"));
	  gdbpy_print_stack ();
	  continue;
	}

      /* Print initial "=" to separate print_string_repr output and
	 children.  For other elements, there are three cases:
	 1. Maps.  Print a "," after each value element.
	 2. Arrays.  Always print a ",".
	 3. Other.  Always print a ",".  */
      if (i == 0)
	{
	  if (!is_py_none)
	    gdb_puts (" = ", stream);
	}
      else if (! is_map || i % 2 == 0)
	gdb_puts (pretty ? "," : ", ", stream);

      /* Skip printing children if max_depth has been reached.  This check
	 is performed after print_string_repr and the "=" separator so that
	 these steps are not skipped if the variable is located within the
	 permitted depth.  */
      if (val_print_check_max_depth (stream, recurse, options, language))
	return;
      else if (i == 0)
	/* Print initial "{" to bookend children.  */
	gdb_puts ("{", stream);

      /* In summary mode, we just want to print "= {...}" if there is
	 a value.  */
      if (options->summary)
	{
	  /* This increment tricks the post-loop logic to print what
	     we want.  */
	  ++i;
	  /* Likewise.  */
	  pretty = 0;
	  break;
	}

      if (! is_map || i % 2 == 0)
	{
	  if (pretty)
	    {
	      gdb_puts ("\n", stream);
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  else
	    stream->wrap_here (2 + 2 *recurse);
	}

      if (is_map && i % 2 == 0)
	gdb_puts ("[", stream);
      else if (is_array)
	{
	  /* We print the index, not whatever the child method
	     returned as the name.  */
	  if (options->print_array_indexes)
	    gdb_printf (stream, "[%d] = ", i);
	}
      else if (! is_map)
	{
	  gdb_puts (name, stream);
	  gdb_puts (" = ", stream);
	}

      if (gdbpy_is_lazy_string (py_v))
	{
	  CORE_ADDR addr;
	  struct type *type;
	  long length;
	  gdb::unique_xmalloc_ptr<char> encoding;
	  struct value_print_options local_opts = *options;

	  gdbpy_extract_lazy_string (py_v, &addr, &type, &length, &encoding);

	  local_opts.addressprint = false;
	  val_print_string (type, encoding.get (), addr, (int) length, stream,
			    &local_opts);
	}
      else if (gdbpy_is_string (py_v))
	{
	  gdb::unique_xmalloc_ptr<char> output;

	  output = python_string_to_host_string (py_v);
	  if (!output)
	    gdbpy_print_stack ();
	  else
	    gdb_puts (output.get (), stream);
	}
      else
	{
	  struct value *value = convert_value_from_python (py_v);

	  if (value == NULL)
	    {
	      gdbpy_print_stack ();
	      error (_("Error while executing Python code."));
	    }
	  else
	    {
	      /* When printing the key of a map we allow one additional
		 level of depth.  This means the key will print before the
		 value does.  */
	      struct value_print_options opt = *options;
	      if (is_map && i % 2 == 0
		  && opt.max_depth != -1
		  && opt.max_depth < INT_MAX)
		++opt.max_depth;
	      common_val_print (value, stream, recurse + 1, &opt, language);
	    }
	}

      if (is_map && i % 2 == 0)
	gdb_puts ("] = ", stream);
    }

  if (i)
    {
      if (!done_flag)
	{
	  if (pretty)
	    {
	      gdb_puts ("\n", stream);
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  gdb_puts ("...", stream);
	}
      if (pretty)
	{
	  gdb_puts ("\n", stream);
	  print_spaces (2 * recurse, stream);
	}
      gdb_puts ("}", stream);
    }
}

enum ext_lang_rc
gdbpy_apply_val_pretty_printer (const struct extension_language_defn *extlang,
				struct value *value,
				struct ui_file *stream, int recurse,
				const struct value_print_options *options,
				const struct language_defn *language)
{
  struct type *type = value->type ();
  struct gdbarch *gdbarch = type->arch ();
  enum gdbpy_string_repr_result print_result;

  if (value->lazy ())
    value->fetch_lazy ();

  /* No pretty-printer support for unavailable values.  */
  if (!value->bytes_available (0, type->length ()))
    return EXT_LANG_RC_NOP;

  if (!gdb_python_initialized)
    return EXT_LANG_RC_NOP;

  gdbpy_enter enter_py (gdbarch, language);

  gdbpy_ref<> val_obj (value_to_value_object (value));
  if (val_obj == NULL)
    {
      print_stack_unless_memory_error (stream);
      return EXT_LANG_RC_ERROR;
    }

  /* Find the constructor.  */
  gdbpy_ref<> printer (find_pretty_printer (val_obj.get ()));
  if (printer == NULL)
    {
      print_stack_unless_memory_error (stream);
      return EXT_LANG_RC_ERROR;
    }

  if (printer == Py_None)
    return EXT_LANG_RC_NOP;

  scoped_restore set_options = make_scoped_restore (&gdbpy_current_print_options,
						    options);

  /* If we are printing a map, we want some special formatting.  */
  gdb::unique_xmalloc_ptr<char> hint (gdbpy_get_display_hint (printer.get ()));

  /* Print the section */
  print_result = print_string_repr (printer.get (), hint.get (), stream,
				    recurse, options, language, gdbarch);
  if (print_result != string_repr_error)
    print_children (printer.get (), hint.get (), stream, recurse, options,
		    language, print_result == string_repr_none);

  if (PyErr_Occurred ())
    print_stack_unless_memory_error (stream);
  return EXT_LANG_RC_OK;
}


/* Apply a pretty-printer for the varobj code.  PRINTER_OBJ is the
   print object.  It must have a 'to_string' method (but this is
   checked by varobj, not here) which takes no arguments and
   returns a string.  The printer will return a value and in the case
   of a Python string being returned, this function will return a
   PyObject containing the string.  For any other type, *REPLACEMENT is
   set to the replacement value and this function returns NULL.  On
   error, *REPLACEMENT is set to NULL and this function also returns
   NULL.  */
gdbpy_ref<>
apply_varobj_pretty_printer (PyObject *printer_obj,
			     struct value **replacement,
			     struct ui_file *stream,
			     const value_print_options *opts)
{
  scoped_restore set_options = make_scoped_restore (&gdbpy_current_print_options,
						    opts);

  *replacement = NULL;
  gdbpy_ref<> py_str = pretty_print_one_value (printer_obj, replacement);

  if (*replacement == NULL && py_str == NULL)
    print_stack_unless_memory_error (stream);

  return py_str;
}

/* Find a pretty-printer object for the varobj module.  Returns a new
   reference to the object if successful; returns NULL if not.  VALUE
   is the value for which a printer tests to determine if it
   can pretty-print the value.  */
gdbpy_ref<>
gdbpy_get_varobj_pretty_printer (struct value *value)
{
  gdbpy_ref<> val_obj (value_to_value_object (value));
  if (val_obj == NULL)
    return NULL;

  return find_pretty_printer (val_obj.get ());
}

/* A Python function which wraps find_pretty_printer and instantiates
   the resulting class.  This accepts a Value argument and returns a
   pretty printer instance, or None.  This function is useful as an
   argument to the MI command -var-set-visualizer.  */
PyObject *
gdbpy_default_visualizer (PyObject *self, PyObject *args)
{
  PyObject *val_obj;
  struct value *value;

  if (! PyArg_ParseTuple (args, "O", &val_obj))
    return NULL;
  value = value_object_to_value (val_obj);
  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Argument must be a gdb.Value."));
      return NULL;
    }

  return find_pretty_printer (val_obj).release ();
}

/* Helper function to set a boolean in a dictionary.  */
static int
set_boolean (PyObject *dict, const char *name, bool val)
{
  gdbpy_ref<> val_obj (PyBool_FromLong (val));
  if (val_obj == nullptr)
    return -1;
  return PyDict_SetItemString (dict, name, val_obj.get ());
}

/* Helper function to set an integer in a dictionary.  */
static int
set_unsigned (PyObject *dict, const char *name, unsigned int val)
{
  gdbpy_ref<> val_obj = gdb_py_object_from_ulongest (val);
  if (val_obj == nullptr)
    return -1;
  return PyDict_SetItemString (dict, name, val_obj.get ());
}

/* Implement gdb.print_options.  */
PyObject *
gdbpy_print_options (PyObject *unused1, PyObject *unused2)
{
  gdbpy_ref<> result (PyDict_New ());
  if (result == nullptr)
    return nullptr;

  value_print_options opts;
  gdbpy_get_print_options (&opts);

  if (set_boolean (result.get (), "raw",
		   opts.raw) < 0
      || set_boolean (result.get (), "pretty_arrays",
		      opts.prettyformat_arrays) < 0
      || set_boolean (result.get (), "pretty_structs",
		      opts.prettyformat_structs) < 0
      || set_boolean (result.get (), "array_indexes",
		      opts.print_array_indexes) < 0
      || set_boolean (result.get (), "symbols",
		      opts.symbol_print) < 0
      || set_boolean (result.get (), "unions",
		      opts.unionprint) < 0
      || set_boolean (result.get (), "address",
		      opts.addressprint) < 0
      || set_boolean (result.get (), "deref_refs",
		      opts.deref_ref) < 0
      || set_boolean (result.get (), "actual_objects",
		      opts.objectprint) < 0
      || set_boolean (result.get (), "static_members",
		      opts.static_field_print) < 0
      || set_boolean (result.get (), "deref_refs",
		      opts.deref_ref) < 0
      || set_boolean (result.get (), "nibbles",
		      opts.nibblesprint) < 0
      || set_boolean (result.get (), "summary",
		      opts.summary) < 0
      || set_unsigned (result.get (), "max_elements",
		       opts.print_max) < 0
      || set_unsigned (result.get (), "max_depth",
		       opts.max_depth) < 0
      || set_unsigned (result.get (), "repeat_threshold",
		       opts.repeat_count_threshold) < 0)
    return nullptr;

  if (opts.format != 0)
    {
      char str[2] = { (char) opts.format, 0 };
      gdbpy_ref<> fmtstr = host_string_to_python_string (str);
      if (fmtstr == nullptr)
	return nullptr;
      if (PyDict_SetItemString (result.get (), "format", fmtstr.get ()) < 0)
	return nullptr;
    }

  return result.release ();
}

/* Helper function that either finds the prevailing print options, or
   calls get_user_print_options.  */
void
gdbpy_get_print_options (value_print_options *opts)
{
  if (gdbpy_current_print_options != nullptr)
    *opts = *gdbpy_current_print_options;
  else
    get_user_print_options (opts);
}

/* A ValuePrinter is just a "tag", so it has no state other than that
   required by Python.  */
struct printer_object
{
  PyObject_HEAD
};

/* The ValuePrinter type object.  */
PyTypeObject printer_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.ValuePrinter",		/*tp_name*/
  sizeof (printer_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash*/
  0,				  /*tp_call*/
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "GDB value printer object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  0,				  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  PyType_GenericNew,		  /* tp_new */
};

/* Set up the ValuePrinter type.  */

static int
gdbpy_initialize_prettyprint ()
{
  if (PyType_Ready (&printer_object_type) < 0)
    return -1;
  return gdb_pymodule_addobject (gdb_module, "ValuePrinter",
				 (PyObject *) &printer_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_prettyprint);

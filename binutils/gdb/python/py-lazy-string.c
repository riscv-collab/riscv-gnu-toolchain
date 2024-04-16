/* Python interface to lazy strings.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "python-internal.h"
#include "charset.h"
#include "value.h"
#include "valprint.h"
#include "language.h"

struct lazy_string_object {
  PyObject_HEAD

  /*  Holds the address of the lazy string.  */
  CORE_ADDR address;

  /*  Holds the encoding that will be applied to the string
      when the string is printed by GDB.  If the encoding is set
      to None then GDB will select the most appropriate
      encoding when the sting is printed.  */
  char *encoding;

  /* If TYPE is an array: If the length is known, then this value is the
     array's length, otherwise it is -1.
     If TYPE is not an array: Then this value represents the string's length.
     In either case, if the value is -1 then the string will be fetched and
     encoded up to the first null of appropriate width.  */
  long length;

  /* This attribute holds the type of the string.
     For example if the lazy string was created from a C "char*" then TYPE
     represents a C "char*".
     To get the type of the character in the string call
     stpy_lazy_string_elt_type.
     This is recorded as a PyObject so that we take advantage of support for
     preserving the type should its owning objfile go away.  */
  PyObject *type;
};

extern PyTypeObject lazy_string_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("lazy_string_object");

static PyObject *
stpy_get_address (PyObject *self, void *closure)
{
  lazy_string_object *self_string = (lazy_string_object *) self;

  return gdb_py_object_from_ulongest (self_string->address).release ();
}

static PyObject *
stpy_get_encoding (PyObject *self, void *closure)
{
  lazy_string_object *self_string = (lazy_string_object *) self;
  PyObject *result;

  /* An encoding can be set to NULL by the user, so check before
     attempting a Python FromString call.  If NULL return Py_None.  */
  if (self_string->encoding)
    result = PyUnicode_FromString (self_string->encoding);
  else
    {
      result = Py_None;
      Py_INCREF (result);
    }

  return result;
}

static PyObject *
stpy_get_length (PyObject *self, void *closure)
{
  lazy_string_object *self_string = (lazy_string_object *) self;

  return gdb_py_object_from_longest (self_string->length).release ();
}

static PyObject *
stpy_get_type (PyObject *self, void *closure)
{
  lazy_string_object *str_obj = (lazy_string_object *) self;

  Py_INCREF (str_obj->type);
  return str_obj->type;
}

static PyObject *
stpy_convert_to_value (PyObject *self, PyObject *args)
{
  lazy_string_object *self_string = (lazy_string_object *) self;

  if (self_string->address == 0)
    {
      PyErr_SetString (gdbpy_gdb_memory_error,
		       _("Cannot create a value from NULL."));
      return NULL;
    }

  PyObject *result = nullptr;
  try
    {
      scoped_value_mark free_values;

      struct type *type = type_object_to_type (self_string->type);
      struct type *realtype;
      struct value *val;

      gdb_assert (type != NULL);
      realtype = check_typedef (type);
      switch (realtype->code ())
	{
	case TYPE_CODE_PTR:
	  /* If a length is specified we need to convert this to an array
	     of the specified size.  */
	  if (self_string->length != -1)
	    {
	      /* PR 20786: There's no way to specify an array of length zero.
		 Record a length of [0,-1] which is how Ada does it.  Anything
		 we do is broken, but this is one possible solution.  */
	      type = lookup_array_range_type (realtype->target_type (),
					      0, self_string->length - 1);
	      val = value_at_lazy (type, self_string->address);
	    }
	  else
	    val = value_from_pointer (type, self_string->address);
	  break;
	default:
	  val = value_at_lazy (type, self_string->address);
	  break;
	}

      result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

static void
stpy_dealloc (PyObject *self)
{
  lazy_string_object *self_string = (lazy_string_object *) self;

  xfree (self_string->encoding);
  Py_TYPE (self)->tp_free (self);
}

/* Low level routine to create a <gdb.LazyString> object.

   Note: If TYPE is an array, LENGTH either must be -1 (meaning to use the
   size of the array, which may itself be unknown in which case a length of
   -1 is still used) or must be the length of the array.  */

PyObject *
gdbpy_create_lazy_string_object (CORE_ADDR address, long length,
				 const char *encoding, struct type *type)
{
  lazy_string_object *str_obj = NULL;
  struct type *realtype;

  if (length < -1)
    {
      PyErr_SetString (PyExc_ValueError, _("Invalid length."));
      return NULL;
    }

  if (address == 0 && length != 0)
    {
      PyErr_SetString (gdbpy_gdb_memory_error,
		       _("Cannot create a lazy string with address 0x0, " \
			 "and a non-zero length."));
      return NULL;
    }

  if (!type)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("A lazy string's type cannot be NULL."));
      return NULL;
    }

  realtype = check_typedef (type);
  switch (realtype->code ())
    {
    case TYPE_CODE_ARRAY:
      {
	LONGEST array_length = -1;
	LONGEST low_bound, high_bound;

	if (get_array_bounds (realtype, &low_bound, &high_bound))
	  array_length = high_bound - low_bound + 1;
	if (length == -1)
	  length = array_length;
	else if (length != array_length)
	  {
	    PyErr_SetString (PyExc_ValueError, _("Invalid length."));
	    return NULL;
	  }
	break;
      }
    }

  str_obj = PyObject_New (lazy_string_object, &lazy_string_object_type);
  if (!str_obj)
    return NULL;

  str_obj->address = address;
  str_obj->length = length;
  if (encoding == NULL || !strcmp (encoding, ""))
    str_obj->encoding = NULL;
  else
    str_obj->encoding = xstrdup (encoding);
  str_obj->type = type_to_type_object (type);

  return (PyObject *) str_obj;
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_lazy_string (void)
{
  if (PyType_Ready (&lazy_string_object_type) < 0)
    return -1;

  Py_INCREF (&lazy_string_object_type);
  return 0;
}

/* Determine whether the printer object pointed to by OBJ is a
   Python lazy string.  */
int
gdbpy_is_lazy_string (PyObject *result)
{
  return PyObject_TypeCheck (result, &lazy_string_object_type);
}

/* Return the type of a character in lazy string LAZY.  */

static struct type *
stpy_lazy_string_elt_type (lazy_string_object *lazy)
{
  struct type *type = type_object_to_type (lazy->type);
  struct type *realtype;

  gdb_assert (type != NULL);
  realtype = check_typedef (type);

  switch (realtype->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
      return realtype->target_type ();
    default:
      /* This is done to preserve existing behaviour.  PR 20769.
	 E.g., gdb.parse_and_eval("my_int_variable").lazy_string().type.  */
      return realtype;
    }
}

/* Extract the parameters from the lazy string object STRING.
   ENCODING may be set to NULL, if no encoding is found.  */

void
gdbpy_extract_lazy_string (PyObject *string, CORE_ADDR *addr,
			   struct type **str_elt_type,
			   long *length,
			   gdb::unique_xmalloc_ptr<char> *encoding)
{
  lazy_string_object *lazy;

  gdb_assert (gdbpy_is_lazy_string (string));

  lazy = (lazy_string_object *) string;

  *addr = lazy->address;
  *str_elt_type = stpy_lazy_string_elt_type (lazy);
  *length = lazy->length;
  encoding->reset (lazy->encoding ? xstrdup (lazy->encoding) : NULL);
}

/* __str__ for LazyString.  */

static PyObject *
stpy_str (PyObject *self)
{
  lazy_string_object *str = (lazy_string_object *) self;

  struct value_print_options opts;
  get_user_print_options (&opts);
  opts.addressprint = false;

  string_file stream;
  try
    {
      struct type *type = stpy_lazy_string_elt_type (str);
      val_print_string (type, str->encoding, str->address, str->length,
			&stream, &opts);
    }
  catch (const gdb_exception &exc)
    {
      GDB_PY_HANDLE_EXCEPTION (exc);
    }

  return host_string_to_python_string (stream.c_str ()).release ();
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_lazy_string);



static PyMethodDef lazy_string_object_methods[] = {
  { "value", stpy_convert_to_value, METH_NOARGS,
    "Create a (lazy) value that contains a pointer to the string." },
  {NULL}  /* Sentinel */
};


static gdb_PyGetSetDef lazy_string_object_getset[] = {
  { "address", stpy_get_address, NULL, "Address of the string.", NULL },
  { "encoding", stpy_get_encoding, NULL, "Encoding of the string.", NULL },
  { "length", stpy_get_length, NULL, "Length of the string.", NULL },
  { "type", stpy_get_type, NULL, "Type associated with the string.", NULL },
  { NULL }  /* Sentinel */
};

PyTypeObject lazy_string_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.LazyString",	          /*tp_name*/
  sizeof (lazy_string_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  stpy_dealloc,                   /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  stpy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,             /*tp_flags*/
  "GDB lazy string object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,			          /* tp_iter */
  0,				  /* tp_iternext */
  lazy_string_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  lazy_string_object_getset	  /* tp_getset */
};

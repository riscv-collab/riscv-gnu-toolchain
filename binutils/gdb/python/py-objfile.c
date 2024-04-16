/* Python interface to objfiles.

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
#include "python-internal.h"
#include "charset.h"
#include "objfiles.h"
#include "language.h"
#include "build-id.h"
#include "symtab.h"
#include "python.h"
#include "inferior.h"

struct objfile_object
{
  PyObject_HEAD

  /* The corresponding objfile.  */
  struct objfile *objfile;

  /* Dictionary holding user-added attributes.
     This is the __dict__ attribute of the object.  */
  PyObject *dict;

  /* The pretty-printer list of functions.  */
  PyObject *printers;

  /* The frame filter list of functions.  */
  PyObject *frame_filters;

  /* The list of frame unwinders.  */
  PyObject *frame_unwinders;

  /* The type-printer list.  */
  PyObject *type_printers;

  /* The debug method matcher list.  */
  PyObject *xmethods;
};

extern PyTypeObject objfile_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("objfile_object");

/* Clear the OBJFILE pointer in an Objfile object and remove the
   reference.  */
struct objfpy_deleter
{
  void operator() (objfile_object *obj)
  {
    gdbpy_enter enter_py;
    gdbpy_ref<objfile_object> object (obj);
    object->objfile = nullptr;
  }
};

static const registry<objfile>::key<objfile_object, objfpy_deleter>
     objfpy_objfile_data_key;

/* Require that OBJF be a valid objfile.  */
#define OBJFPY_REQUIRE_VALID(obj)				\
  do {								\
    if (!(obj)->objfile)					\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Objfile no longer exists."));	\
	return NULL;						\
      }								\
  } while (0)



/* An Objfile method which returns the objfile's file name, or None.  */

static PyObject *
objfpy_get_filename (PyObject *self, void *closure)
{
  objfile_object *obj = (objfile_object *) self;

  if (obj->objfile)
    return (host_string_to_python_string (objfile_name (obj->objfile))
	    .release ());
  Py_RETURN_NONE;
}

/* An Objfile method which returns the objfile's file name, as specified
   by the user, or None.  */

static PyObject *
objfpy_get_username (PyObject *self, void *closure)
{
  objfile_object *obj = (objfile_object *) self;

  if (obj->objfile)
    {
      const char *username = obj->objfile->original_name;

      return host_string_to_python_string (username).release ();
    }

  Py_RETURN_NONE;
}

/* Get the 'is_file' attribute.  */

static PyObject *
objfpy_get_is_file (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  if (self->objfile != nullptr)
    return PyBool_FromLong ((self->objfile->flags & OBJF_NOT_FILENAME) == 0);
  Py_RETURN_NONE;
}

/* If SELF is a separate debug-info file, return the "backlink" field.
   Otherwise return None.  */

static PyObject *
objfpy_get_owner (PyObject *self, void *closure)
{
  objfile_object *obj = (objfile_object *) self;
  struct objfile *objfile = obj->objfile;
  struct objfile *owner;

  OBJFPY_REQUIRE_VALID (obj);

  owner = objfile->separate_debug_objfile_backlink;
  if (owner != NULL)
    return objfile_to_objfile_object (owner).release ();
  Py_RETURN_NONE;
}

/* An Objfile method which returns the objfile's build id, or None.  */

static PyObject *
objfpy_get_build_id (PyObject *self, void *closure)
{
  objfile_object *obj = (objfile_object *) self;
  struct objfile *objfile = obj->objfile;
  const struct bfd_build_id *build_id = NULL;

  OBJFPY_REQUIRE_VALID (obj);

  try
    {
      build_id = build_id_bfd_get (objfile->obfd.get ());
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (build_id != NULL)
    {
      std::string hex_form = bin2hex (build_id->data, build_id->size);

      return host_string_to_python_string (hex_form.c_str ()).release ();
    }

  Py_RETURN_NONE;
}

/* An Objfile method which returns the objfile's progspace, or None.  */

static PyObject *
objfpy_get_progspace (PyObject *self, void *closure)
{
  objfile_object *obj = (objfile_object *) self;

  if (obj->objfile)
    return pspace_to_pspace_object (obj->objfile->pspace).release ();

  Py_RETURN_NONE;
}

static void
objfpy_dealloc (PyObject *o)
{
  objfile_object *self = (objfile_object *) o;

  Py_XDECREF (self->dict);
  Py_XDECREF (self->printers);
  Py_XDECREF (self->frame_filters);
  Py_XDECREF (self->frame_unwinders);
  Py_XDECREF (self->type_printers);
  Py_XDECREF (self->xmethods);
  Py_TYPE (self)->tp_free (self);
}

/* Initialize an objfile_object.
   The result is a boolean indicating success.  */

static int
objfpy_initialize (objfile_object *self)
{
  self->objfile = NULL;

  self->dict = PyDict_New ();
  if (self->dict == NULL)
    return 0;

  self->printers = PyList_New (0);
  if (self->printers == NULL)
    return 0;

  self->frame_filters = PyDict_New ();
  if (self->frame_filters == NULL)
    return 0;

  self->frame_unwinders = PyList_New (0);
  if (self->frame_unwinders == NULL)
    return 0;

  self->type_printers = PyList_New (0);
  if (self->type_printers == NULL)
    return 0;

  self->xmethods = PyList_New (0);
  if (self->xmethods == NULL)
    return 0;

  return 1;
}

static PyObject *
objfpy_new (PyTypeObject *type, PyObject *args, PyObject *keywords)
{
  gdbpy_ref<objfile_object> self ((objfile_object *) type->tp_alloc (type, 0));

  if (self != NULL)
    {
      if (!objfpy_initialize (self.get ()))
	return NULL;
    }

  return (PyObject *) self.release ();
}

PyObject *
objfpy_get_printers (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->printers);
  return self->printers;
}

static int
objfpy_set_printers (PyObject *o, PyObject *value, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the pretty_printers attribute."));
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The pretty_printers attribute must be a list."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->printers);
  Py_INCREF (value);
  self->printers = value;

  return 0;
}

/* Return the Python dictionary attribute containing frame filters for
   this object file.  */
PyObject *
objfpy_get_frame_filters (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->frame_filters);
  return self->frame_filters;
}

/* Set this object file's frame filters dictionary to FILTERS.  */
static int
objfpy_set_frame_filters (PyObject *o, PyObject *filters, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  if (! filters)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the frame filters attribute."));
      return -1;
    }

  if (! PyDict_Check (filters))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The frame_filters attribute must be a dictionary."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->frame_filters);
  Py_INCREF (filters);
  self->frame_filters = filters;

  return 0;
}

/* Return the frame unwinders attribute for this object file.  */

PyObject *
objfpy_get_frame_unwinders (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->frame_unwinders);
  return self->frame_unwinders;
}

/* Set this object file's frame unwinders list to UNWINDERS.  */

static int
objfpy_set_frame_unwinders (PyObject *o, PyObject *unwinders, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  if (!unwinders)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the frame unwinders attribute."));
      return -1;
    }

  if (!PyList_Check (unwinders))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The frame_unwinders attribute must be a list."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->frame_unwinders);
  Py_INCREF (unwinders);
  self->frame_unwinders = unwinders;

  return 0;
}

/* Get the 'type_printers' attribute.  */

static PyObject *
objfpy_get_type_printers (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->type_printers);
  return self->type_printers;
}

/* Get the 'xmethods' attribute.  */

PyObject *
objfpy_get_xmethods (PyObject *o, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  Py_INCREF (self->xmethods);
  return self->xmethods;
}

/* Set the 'type_printers' attribute.  */

static int
objfpy_set_type_printers (PyObject *o, PyObject *value, void *ignore)
{
  objfile_object *self = (objfile_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete the type_printers attribute."));
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The type_printers attribute must be a list."));
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->type_printers);
  Py_INCREF (value);
  self->type_printers = value;

  return 0;
}

/* Implementation of gdb.Objfile.is_valid (self) -> Boolean.
   Returns True if this object file still exists in GDB.  */

static PyObject *
objfpy_is_valid (PyObject *self, PyObject *args)
{
  objfile_object *obj = (objfile_object *) self;

  if (! obj->objfile)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Objfile.add_separate_debug_file (self, string). */

static PyObject *
objfpy_add_separate_debug_file (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "file_name", NULL };
  objfile_object *obj = (objfile_object *) self;
  const char *file_name;

  OBJFPY_REQUIRE_VALID (obj);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s", keywords, &file_name))
    return NULL;

  try
    {
      gdb_bfd_ref_ptr abfd (symfile_bfd_open (file_name));

      symbol_file_add_separate (abfd, file_name, 0, obj->objfile);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Implementation of
  gdb.Objfile.lookup_global_symbol (self, string [, domain]) -> gdb.Symbol.  */

static PyObject *
objfpy_lookup_global_symbol (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", "domain", NULL };
  objfile_object *obj = (objfile_object *) self;
  const char *symbol_name;
  int domain = VAR_DOMAIN;

  OBJFPY_REQUIRE_VALID (obj);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &symbol_name,
					&domain))
    return nullptr;

  try
    {
      struct symbol *sym = lookup_global_symbol_from_objfile
	(obj->objfile, GLOBAL_BLOCK, symbol_name, (domain_enum) domain).symbol;
      if (sym == nullptr)
	Py_RETURN_NONE;

      return symbol_to_symbol_object (sym);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Implementation of
  gdb.Objfile.lookup_static_symbol (self, string [, domain]) -> gdb.Symbol.  */

static PyObject *
objfpy_lookup_static_symbol (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", "domain", NULL };
  objfile_object *obj = (objfile_object *) self;
  const char *symbol_name;
  int domain = VAR_DOMAIN;

  OBJFPY_REQUIRE_VALID (obj);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &symbol_name,
					&domain))
    return nullptr;

  try
    {
      struct symbol *sym = lookup_global_symbol_from_objfile
	(obj->objfile, STATIC_BLOCK, symbol_name, (domain_enum) domain).symbol;
      if (sym == nullptr)
	Py_RETURN_NONE;

      return symbol_to_symbol_object (sym);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Implement repr() for gdb.Objfile.  */

static PyObject *
objfpy_repr (PyObject *self_)
{
  objfile_object *self = (objfile_object *) self_;
  objfile *obj = self->objfile;

  if (obj == nullptr)
    return gdb_py_invalid_object_repr (self_);

  return PyUnicode_FromFormat ("<gdb.Objfile filename=%s>",
			       objfile_name (obj));
}

/* Subroutine of gdbpy_lookup_objfile_by_build_id to simplify it.
   Return non-zero if STRING is a potentially valid build id.  */

static int
objfpy_build_id_ok (const char *string)
{
  size_t i, n = strlen (string);

  if (n % 2 != 0)
    return 0;
  for (i = 0; i < n; ++i)
    {
      if (!isxdigit (string[i]))
	return 0;
    }
  return 1;
}

/* Subroutine of gdbpy_lookup_objfile_by_build_id to simplify it.
   Returns non-zero if BUILD_ID matches STRING.
   It is assumed that objfpy_build_id_ok (string) returns TRUE.  */

static int
objfpy_build_id_matches (const struct bfd_build_id *build_id,
			 const char *string)
{
  size_t i;

  if (strlen (string) != 2 * build_id->size)
    return 0;

  for (i = 0; i < build_id->size; ++i)
    {
      char c1 = string[i * 2], c2 = string[i * 2 + 1];
      int byte = (fromhex (c1) << 4) | fromhex (c2);

      if (byte != build_id->data[i])
	return 0;
    }

  return 1;
}

/* Implementation of gdb.lookup_objfile.  */

PyObject *
gdbpy_lookup_objfile (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", "by_build_id", NULL };
  const char *name;
  PyObject *by_build_id_obj = NULL;
  int by_build_id;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O!", keywords,
					&name, &PyBool_Type, &by_build_id_obj))
    return NULL;

  by_build_id = 0;
  if (by_build_id_obj != NULL)
    {
      int cmp = PyObject_IsTrue (by_build_id_obj);

      if (cmp < 0)
	return NULL;
      by_build_id = cmp;
    }

  if (by_build_id && !objfpy_build_id_ok (name))
    {
      PyErr_SetString (PyExc_TypeError, _("Not a valid build id."));
      return NULL;
    }

  struct objfile *objfile = nullptr;
  if (by_build_id)
    gdbarch_iterate_over_objfiles_in_search_order
      (current_inferior ()->arch (),
       [&objfile, name] (struct objfile *obj)
	 {
	   /* Don't return separate debug files.  */
	   if (obj->separate_debug_objfile_backlink != nullptr)
	     return 0;

	   bfd *obfd = obj->obfd.get ();
	   if (obfd == nullptr)
	     return 0;

	   const bfd_build_id *obfd_build_id = build_id_bfd_get (obfd);
	   if (obfd_build_id == nullptr)
	     return 0;

	   if (!objfpy_build_id_matches (obfd_build_id, name))
	     return 0;

	   objfile = obj;
	   return 1;
	 }, gdbpy_current_objfile);
  else
    gdbarch_iterate_over_objfiles_in_search_order
      (current_inferior ()->arch (),
       [&objfile, name] (struct objfile *obj)
	 {
	   /* Don't return separate debug files.  */
	   if (obj->separate_debug_objfile_backlink != nullptr)
	     return 0;

	   if ((obj->flags & OBJF_NOT_FILENAME) != 0)
	     return 0;

	   const char *filename = objfile_filename (obj);
	   if (filename != NULL
	       && compare_filenames_for_search (filename, name))
	     {
	       objfile = obj;
	       return 1;
	     }

	   if (compare_filenames_for_search (obj->original_name, name))
	     {
	       objfile = obj;
	       return 1;
	     }

	   return 0;
	 }, gdbpy_current_objfile);

  if (objfile != NULL)
    return objfile_to_objfile_object (objfile).release ();

  PyErr_SetString (PyExc_ValueError, _("Objfile not found."));
  return NULL;
}



/* Return a new reference to the Python object of type Objfile
   representing OBJFILE.  If the object has already been created,
   return it.  Otherwise, create it.  Return NULL and set the Python
   error on failure.  */

gdbpy_ref<>
objfile_to_objfile_object (struct objfile *objfile)
{
  PyObject *result
    = (PyObject *) objfpy_objfile_data_key.get (objfile);
  if (result == NULL)
    {
      gdbpy_ref<objfile_object> object
	((objfile_object *) PyObject_New (objfile_object, &objfile_object_type));
      if (object == NULL)
	return NULL;
      if (!objfpy_initialize (object.get ()))
	return NULL;

      object->objfile = objfile;
      objfpy_objfile_data_key.set (objfile, object.get ());
      result = (PyObject *) object.release ();
    }

  return gdbpy_ref<>::new_reference (result);
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_objfile (void)
{
  if (PyType_Ready (&objfile_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Objfile",
				 (PyObject *) &objfile_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_objfile);



static PyMethodDef objfile_object_methods[] =
{
  { "is_valid", objfpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this object file is valid, false if not." },

  { "add_separate_debug_file", (PyCFunction) objfpy_add_separate_debug_file,
    METH_VARARGS | METH_KEYWORDS,
    "add_separate_debug_file (file_name).\n\
Add FILE_NAME to the list of files containing debug info for the objfile." },

  { "lookup_global_symbol", (PyCFunction) objfpy_lookup_global_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_global_symbol (name [, domain]).\n\
Look up a global symbol in this objfile and return it." },

  { "lookup_static_symbol", (PyCFunction) objfpy_lookup_static_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_static_symbol (name [, domain]).\n\
Look up a static-linkage global symbol in this objfile and return it." },

  { NULL }
};

static gdb_PyGetSetDef objfile_getset[] =
{
  { "__dict__", gdb_py_generic_dict, NULL,
    "The __dict__ for this objfile.", &objfile_object_type },
  { "filename", objfpy_get_filename, NULL,
    "The objfile's filename, or None.", NULL },
  { "username", objfpy_get_username, NULL,
    "The name of the objfile as provided by the user, or None.", NULL },
  { "owner", objfpy_get_owner, NULL,
    "The objfile owner of separate debug info objfiles, or None.",
    NULL },
  { "build_id", objfpy_get_build_id, NULL,
    "The objfile's build id, or None.", NULL },
  { "progspace", objfpy_get_progspace, NULL,
    "The objfile's progspace, or None.", NULL },
  { "pretty_printers", objfpy_get_printers, objfpy_set_printers,
    "Pretty printers.", NULL },
  { "frame_filters", objfpy_get_frame_filters,
    objfpy_set_frame_filters, "Frame Filters.", NULL },
  { "frame_unwinders", objfpy_get_frame_unwinders,
    objfpy_set_frame_unwinders, "Frame Unwinders", NULL },
  { "type_printers", objfpy_get_type_printers, objfpy_set_type_printers,
    "Type printers.", NULL },
  { "xmethods", objfpy_get_xmethods, NULL,
    "Debug methods.", NULL },
  { "is_file", objfpy_get_is_file, nullptr,
    "Whether this objfile came from a file.", nullptr },
  { NULL }
};

PyTypeObject objfile_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Objfile",		  /*tp_name*/
  sizeof (objfile_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  objfpy_dealloc,		  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  objfpy_repr,			  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB objfile object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  objfile_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  objfile_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  offsetof (objfile_object, dict), /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  objfpy_new,			  /* tp_new */
};

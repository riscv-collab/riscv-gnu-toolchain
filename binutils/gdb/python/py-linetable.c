/* Python interface to line tables.

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

#include "defs.h"
#include "python-internal.h"

struct linetable_entry_object {
  PyObject_HEAD
  /* The line table source line.  */
  int line;
  /* The pc associated with the source line.  */
  CORE_ADDR pc;
};

extern PyTypeObject linetable_entry_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("linetable_entry_object");

struct linetable_object {
  PyObject_HEAD
  /* The symtab python object.  We store the Python object here as the
     underlying symtab can become invalid, and we have to run validity
     checks on it.  */
  PyObject *symtab;
};

extern PyTypeObject linetable_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("linetable_object");

struct ltpy_iterator_object {
  PyObject_HEAD
  /* The current entry in the line table for the iterator  */
  int current_index;
  /* Pointer back to the original source line table object.  Needed to
     check if the line table is still valid, and has not been invalidated
     when an object file has been freed.  */
  PyObject *source;
};

extern PyTypeObject ltpy_iterator_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("ltpy_iterator_object");

/* Internal helper function to extract gdb.Symtab from a gdb.LineTable
   object.  */

static PyObject *
get_symtab (PyObject *linetable)
{
  linetable_object *lt = (linetable_object *) linetable;

  return lt->symtab;
}

#define LTPY_REQUIRE_VALID(lt_obj, symtab)				\
  do {									\
    symtab = symtab_object_to_symtab (get_symtab (lt_obj));		\
    if (symtab == NULL)							\
      {									\
	  PyErr_SetString (PyExc_RuntimeError,				\
			   _("Symbol Table in line table is invalid."));\
	  return NULL;							\
	}								\
  } while (0)


/* Helper function to create a line table object that wraps a
   gdb.Symtab object.  */

PyObject *
symtab_to_linetable_object (PyObject *symtab)
{
  linetable_object *ltable;

  ltable = PyObject_New (linetable_object, &linetable_object_type);
  if (ltable != NULL)
    {
      ltable->symtab = symtab;
      Py_INCREF (symtab);
    }
  return (PyObject *) ltable;
}

/* Internal helper function to build a line table object from a line
   and an address.  */

static PyObject *
build_linetable_entry (int line, CORE_ADDR address)
{
  linetable_entry_object *obj;

  obj = PyObject_New (linetable_entry_object,
		      &linetable_entry_object_type);
  if (obj != NULL)
    {
      obj->line = line;
      obj->pc = address;
    }

  return (PyObject *) obj;
}

/* Internal helper function to build a Python Tuple from a vector.
   A line table entry can have multiple PCs for a given source line.
   Construct a Tuple of all entries for the given source line, LINE
   from the line table PCS.  Construct one line table entry object per
   address.  */

static PyObject *
build_line_table_tuple_from_pcs (int line, const std::vector<CORE_ADDR> &pcs)
{
  int i;

  if (pcs.size () < 1)
    Py_RETURN_NONE;

  gdbpy_ref<> tuple (PyTuple_New (pcs.size ()));

  if (tuple == NULL)
    return NULL;

  for (i = 0; i < pcs.size (); ++i)
    {
      CORE_ADDR pc = pcs[i];
      gdbpy_ref<> obj (build_linetable_entry (line, pc));

      if (obj == NULL)
	return NULL;
      else if (PyTuple_SetItem (tuple.get (), i, obj.release ()) != 0)
	return NULL;
    }

  return tuple.release ();
}

/* Implementation of gdb.LineTable.line (self) -> Tuple.  Returns a
   tuple of LineTableEntry objects associated with this line from the
   in the line table.  */

static PyObject *
ltpy_get_pcs_for_line (PyObject *self, PyObject *args)
{
  struct symtab *symtab;
  gdb_py_longest py_line;
  const linetable_entry *best_entry = nullptr;
  std::vector<CORE_ADDR> pcs;

  LTPY_REQUIRE_VALID (self, symtab);

  if (! PyArg_ParseTuple (args, GDB_PY_LL_ARG, &py_line))
    return NULL;

  try
    {
      pcs = find_pcs_for_symtab_line (symtab, py_line, &best_entry);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return build_line_table_tuple_from_pcs (py_line, pcs);
}

/* Implementation of gdb.LineTable.has_line (self, line) -> Boolean.
   Returns a Python Boolean indicating whether a source line has any
   line table entries corresponding to it.  */

static PyObject *
ltpy_has_line (PyObject *self, PyObject *args)
{
  struct symtab *symtab;
  gdb_py_longest py_line;
  int index;

  LTPY_REQUIRE_VALID (self, symtab);

  if (! PyArg_ParseTuple (args, GDB_PY_LL_ARG, &py_line))
    return NULL;

  if (symtab->linetable () == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Linetable information not found in symbol table"));
      return NULL;
    }

  for (index = 0; index < symtab->linetable ()->nitems; index++)
    {
      const linetable_entry *item = &(symtab->linetable ()->item[index]);
      if (item->line == py_line)
	  Py_RETURN_TRUE;
    }

  Py_RETURN_FALSE;
}

/* Implementation of gdb.LineTable.source_lines (self) -> List.
   Returns a Python List that contains source line entries in the
   line table.  This function will just return the source lines
   without corresponding addresses.  */

static PyObject *
ltpy_get_all_source_lines (PyObject *self, PyObject *args)
{
  struct symtab *symtab;
  Py_ssize_t index;

  LTPY_REQUIRE_VALID (self, symtab);

  if (symtab->linetable () == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Linetable information not found in symbol table"));
      return NULL;
    }

  gdbpy_ref<> source_dict (PyDict_New ());
  if (source_dict == NULL)
    return NULL;

  for (index = 0; index < symtab->linetable ()->nitems; index++)
    {
      const linetable_entry *item = &(symtab->linetable ()->item[index]);

      /* 0 is used to signify end of line table information.  Do not
	 include in the source set. */
      if (item->line > 0)
	{
	  gdbpy_ref<> line = gdb_py_object_from_longest (item->line);

	  if (line == NULL)
	    return NULL;

	  if (PyDict_SetItem (source_dict.get (), line.get (), Py_None) == -1)
	    return NULL;
	}
    }

  return PyDict_Keys (source_dict.get ());
}

/* Implementation of gdb.LineTable.is_valid (self) -> Boolean.
   Returns True if this line table object still exists in GDB.  */

static PyObject *
ltpy_is_valid (PyObject *self, PyObject *args)
{
  struct symtab *symtab = NULL;

  symtab = symtab_object_to_symtab (get_symtab (self));

  if (symtab == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Deconstructor for the line table object.  Decrement the reference
   to the symbol table object before calling the default free.  */

static void
ltpy_dealloc (PyObject *self)
{
  linetable_object *obj = (linetable_object *) self;

  Py_DECREF (obj->symtab);
  Py_TYPE (self)->tp_free (self);
}

/* Initialize LineTable, LineTableEntry and LineTableIterator
   objects.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_linetable (void)
{
  if (PyType_Ready (&linetable_object_type) < 0)
    return -1;
  if (PyType_Ready (&linetable_entry_object_type) < 0)
    return -1;
  if (PyType_Ready (&ltpy_iterator_object_type) < 0)
    return -1;

  Py_INCREF (&linetable_object_type);
  Py_INCREF (&linetable_entry_object_type);
  Py_INCREF (&ltpy_iterator_object_type);

  if (gdb_pymodule_addobject (gdb_module, "LineTable",
			      (PyObject *) &linetable_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "LineTableEntry",
			      (PyObject *) &linetable_entry_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "LineTableIterator",
			      (PyObject *) &ltpy_iterator_object_type) < 0)
    return -1;

  return 0;
}

/* LineTable entry object get functions.  */

/* Implementation of gdb.LineTableEntry.line (self) -> Long.  Returns
   a long integer associated with the line table entry.  */

static PyObject *
ltpy_entry_get_line (PyObject *self, void *closure)
{
  linetable_entry_object *obj = (linetable_entry_object *) self;

  return gdb_py_object_from_longest (obj->line).release ();
}

/* Implementation of gdb.LineTableEntry.pc (self) -> Long.  Returns a
   a long integer associated with the PC of the line table entry.  */

static PyObject *
ltpy_entry_get_pc (PyObject *self, void *closure)
{
  linetable_entry_object *obj = (linetable_entry_object *) self;

  return gdb_py_object_from_ulongest (obj->pc).release ();
}

/* LineTable iterator functions.  */

/* Return a new line table iterator.  */

static PyObject *
ltpy_iter (PyObject *self)
{
  ltpy_iterator_object *ltpy_iter_obj;
  struct symtab *symtab = NULL;

  LTPY_REQUIRE_VALID (self, symtab);

  ltpy_iter_obj = PyObject_New (ltpy_iterator_object,
				&ltpy_iterator_object_type);
  if (ltpy_iter_obj == NULL)
    return NULL;

  ltpy_iter_obj->current_index = 0;
  ltpy_iter_obj->source = self;

  Py_INCREF (self);
  return (PyObject *) ltpy_iter_obj;
}

static void
ltpy_iterator_dealloc (PyObject *obj)
{
  ltpy_iterator_object *iter_obj = (ltpy_iterator_object *) obj;

  Py_DECREF (iter_obj->source);
  Py_TYPE (obj)->tp_free (obj);
}

/* Return a reference to the line table iterator.  */

static PyObject *
ltpy_iterator (PyObject *self)
{
  ltpy_iterator_object *iter_obj = (ltpy_iterator_object *) self;
  struct symtab *symtab;

  LTPY_REQUIRE_VALID (iter_obj->source, symtab);

  Py_INCREF (self);
  return self;
}

/* Return the next line table entry in the iteration through the line
   table data structure.  */

static PyObject *
ltpy_iternext (PyObject *self)
{
  ltpy_iterator_object *iter_obj = (ltpy_iterator_object *) self;
  struct symtab *symtab;
  PyObject *obj;

  LTPY_REQUIRE_VALID (iter_obj->source, symtab);

  if (iter_obj->current_index >= symtab->linetable ()->nitems)
    {
      PyErr_SetNone (PyExc_StopIteration);
      return NULL;
    }

  const linetable_entry *item
    = &(symtab->linetable ()->item[iter_obj->current_index]);

  /* Skip over internal entries such as 0.  0 signifies the end of
     line table data and is not useful to the API user.  */
  while (item->line < 1)
    {
      iter_obj->current_index++;

      /* Exit if the internal value is the last item in the line table.  */
      if (iter_obj->current_index >= symtab->linetable ()->nitems)
	{
	  PyErr_SetNone (PyExc_StopIteration);
	  return NULL;
	}
      item = &(symtab->linetable ()->item[iter_obj->current_index]);
    }

  struct objfile *objfile = symtab->compunit ()->objfile ();
  obj = build_linetable_entry (item->line, item->pc (objfile));
  iter_obj->current_index++;

  return obj;
}

/* Implementation of gdb.LineTableIterator.is_valid (self) -> Boolean.
   Returns True if this line table iterator object still exists in
   GDB.  */

static PyObject *
ltpy_iter_is_valid (PyObject *self, PyObject *args)
{
  struct symtab *symtab = NULL;
  ltpy_iterator_object *iter_obj = (ltpy_iterator_object *) self;

  symtab = symtab_object_to_symtab (get_symtab (iter_obj->source));

  if (symtab == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_linetable);



static PyMethodDef linetable_object_methods[] = {
  { "line", ltpy_get_pcs_for_line, METH_VARARGS,
    "line (lineno) -> Tuple\n\
Return executable locations for a given source line." },
  { "has_line", ltpy_has_line, METH_VARARGS,
    "has_line (lineno) -> Boolean\n\
Return TRUE if this line has executable information, FALSE if not." },
  { "source_lines", ltpy_get_all_source_lines, METH_NOARGS,
    "source_lines () -> List\n\
Return a list of all executable source lines." },
  { "is_valid", ltpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return True if this LineTable is valid, False if not." },
  {NULL}  /* Sentinel */
};

PyTypeObject linetable_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.LineTable",	          /*tp_name*/
  sizeof (linetable_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  ltpy_dealloc,                   /*tp_dealloc*/
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
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,             /*tp_flags*/
  "GDB line table object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  ltpy_iter,			  /* tp_iter */
  0,				  /* tp_iternext */
  linetable_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  0,	                          /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,    			  /* tp_init */
  0,				  /* tp_alloc */
};

static PyMethodDef ltpy_iterator_methods[] = {
  { "is_valid", ltpy_iter_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return True if this LineTable iterator is valid, False if not." },
  {NULL}  /* Sentinel */
};

PyTypeObject ltpy_iterator_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.LineTableIterator",		  /*tp_name*/
  sizeof (ltpy_iterator_object),  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  ltpy_iterator_dealloc,	  /*tp_dealloc*/
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
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB line table iterator object",	      /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  ltpy_iterator,                  /*tp_iter */
  ltpy_iternext,	          /*tp_iternext */
  ltpy_iterator_methods           /*tp_methods */
};


static gdb_PyGetSetDef linetable_entry_object_getset[] = {
  { "line", ltpy_entry_get_line, NULL,
    "The line number in the source file.", NULL },
  { "pc", ltpy_entry_get_pc, NULL,
    "The memory address for this line number.", NULL },
  { NULL }  /* Sentinel */
};

PyTypeObject linetable_entry_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.LineTableEntry",	          /*tp_name*/
  sizeof (linetable_entry_object), /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,                              /*tp_dealloc*/
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
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,             /*tp_flags*/
  "GDB line table entry object",  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,			          /* tp_iter */
  0,				  /* tp_iternext */
  0,                              /* tp_methods */
  0,				  /* tp_members */
  linetable_entry_object_getset,  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,	                          /* tp_init */
  0,				  /* tp_alloc */
};

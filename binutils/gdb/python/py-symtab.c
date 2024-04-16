/* Python interface to symbol tables.

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
#include "charset.h"
#include "symtab.h"
#include "source.h"
#include "python-internal.h"
#include "objfiles.h"
#include "block.h"

struct symtab_object {
  PyObject_HEAD
  /* The GDB Symbol table structure.  */
  struct symtab *symtab;
  /* A symtab object is associated with an objfile, so keep track with
     a doubly-linked list, rooted in the objfile.  This allows
     invalidation of the underlying struct symtab when the objfile is
     deleted.  */
  symtab_object *prev;
  symtab_object *next;
};

/* This function is called when an objfile is about to be freed.
   Invalidate the symbol table as further actions on the symbol table
   would result in bad data.  All access to obj->symtab should be
   gated by STPY_REQUIRE_VALID which will raise an exception on
   invalid symbol tables.  */
struct stpy_deleter
{
  void operator() (symtab_object *obj)
  {
    while (obj)
      {
	symtab_object *next = obj->next;

	obj->symtab = NULL;
	obj->next = NULL;
	obj->prev = NULL;
	obj = next;
      }
  }
};

extern PyTypeObject symtab_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("symtab_object");
static const registry<objfile>::key<symtab_object, stpy_deleter>
     stpy_objfile_data_key;

/* Require a valid symbol table.  All access to symtab_object->symtab
   should be gated by this call.  */
#define STPY_REQUIRE_VALID(symtab_obj, symtab)		 \
  do {							 \
    symtab = symtab_object_to_symtab (symtab_obj);	 \
    if (symtab == NULL)					 \
      {							 \
	PyErr_SetString (PyExc_RuntimeError,		 \
			 _("Symbol Table is invalid.")); \
	return NULL;					 \
      }							 \
  } while (0)

struct sal_object {
  PyObject_HEAD
  /* The GDB Symbol table structure.  */
  PyObject *symtab;
  /* The GDB Symbol table and line structure.  */
  struct symtab_and_line *sal;
  /* A Symtab and line object is associated with an objfile, so keep
     track with a doubly-linked list, rooted in the objfile.  This
     allows invalidation of the underlying struct symtab_and_line
     when the objfile is deleted.  */
  sal_object *prev;
  sal_object *next;
};

/* This is called when an objfile is about to be freed.  Invalidate
   the sal object as further actions on the sal would result in bad
   data.  All access to obj->sal should be gated by
   SALPY_REQUIRE_VALID which will raise an exception on invalid symbol
   table and line objects.  */
struct salpy_deleter
{
  void operator() (sal_object *obj)
  {
    gdbpy_enter enter_py;

    while (obj)
      {
	sal_object *next = obj->next;

	gdbpy_ref<> tmp (obj->symtab);
	obj->symtab = Py_None;
	Py_INCREF (Py_None);

	obj->next = NULL;
	obj->prev = NULL;
	xfree (obj->sal);
	obj->sal = NULL;

	obj = next;
      }
  }
};

extern PyTypeObject sal_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("sal_object");
static const registry<objfile>::key<sal_object, salpy_deleter>
     salpy_objfile_data_key;

/* Require a valid symbol table and line object.  All access to
   sal_object->sal should be gated by this call.  */
#define SALPY_REQUIRE_VALID(sal_obj, sal)				\
  do {									\
    sal = sal_object_to_symtab_and_line (sal_obj);			\
    if (sal == NULL)							\
      {									\
	  PyErr_SetString (PyExc_RuntimeError,				\
			   _("Symbol Table and Line is invalid."));	\
	  return NULL;							\
	}								\
  } while (0)

static PyObject *
stpy_str (PyObject *self)
{
  PyObject *result;
  struct symtab *symtab = NULL;

  STPY_REQUIRE_VALID (self, symtab);

  result = PyUnicode_FromString (symtab_to_filename_for_display (symtab));

  return result;
}

static PyObject *
stpy_get_filename (PyObject *self, void *closure)
{
  PyObject *str_obj;
  struct symtab *symtab = NULL;
  const char *filename;

  STPY_REQUIRE_VALID (self, symtab);
  filename = symtab_to_filename_for_display (symtab);

  str_obj = host_string_to_python_string (filename).release ();
  return str_obj;
}

static PyObject *
stpy_get_objfile (PyObject *self, void *closure)
{
  struct symtab *symtab = NULL;

  STPY_REQUIRE_VALID (self, symtab);

  return objfile_to_objfile_object (symtab->compunit ()->objfile ()).release ();
}

/* Getter function for symtab.producer.  */

static PyObject *
stpy_get_producer (PyObject *self, void *closure)
{
  struct symtab *symtab = NULL;
  struct compunit_symtab *cust;

  STPY_REQUIRE_VALID (self, symtab);
  cust = symtab->compunit ();
  if (cust->producer () != nullptr)
    {
      const char *producer = cust->producer ();

      return host_string_to_python_string (producer).release ();
    }

  Py_RETURN_NONE;
}

static PyObject *
stpy_fullname (PyObject *self, PyObject *args)
{
  const char *fullname;
  struct symtab *symtab = NULL;

  STPY_REQUIRE_VALID (self, symtab);

  fullname = symtab_to_fullname (symtab);

  return host_string_to_python_string (fullname).release ();
}

/* Implementation of gdb.Symtab.is_valid (self) -> Boolean.
   Returns True if this Symbol table still exists in GDB.  */

static PyObject *
stpy_is_valid (PyObject *self, PyObject *args)
{
  struct symtab *symtab = NULL;

  symtab = symtab_object_to_symtab (self);
  if (symtab == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Return the GLOBAL_BLOCK of the underlying symtab.  */

static PyObject *
stpy_global_block (PyObject *self, PyObject *args)
{
  struct symtab *symtab = NULL;
  const struct blockvector *blockvector;

  STPY_REQUIRE_VALID (self, symtab);

  blockvector = symtab->compunit ()->blockvector ();
  const struct block *block = blockvector->global_block ();

  return block_to_block_object (block, symtab->compunit ()->objfile ());
}

/* Return the STATIC_BLOCK of the underlying symtab.  */

static PyObject *
stpy_static_block (PyObject *self, PyObject *args)
{
  struct symtab *symtab = NULL;
  const struct blockvector *blockvector;

  STPY_REQUIRE_VALID (self, symtab);

  blockvector = symtab->compunit ()->blockvector ();
  const struct block *block = blockvector->static_block ();

  return block_to_block_object (block, symtab->compunit ()->objfile ());
}

/* Implementation of gdb.Symtab.linetable (self) -> gdb.LineTable.
   Returns a gdb.LineTable object corresponding to this symbol
   table.  */

static PyObject *
stpy_get_linetable (PyObject *self, PyObject *args)
{
  struct symtab *symtab = NULL;

  STPY_REQUIRE_VALID (self, symtab);

  return symtab_to_linetable_object (self);
}

static PyObject *
salpy_str (PyObject *self)
{
  const char *filename;
  sal_object *sal_obj;
  struct symtab_and_line *sal = NULL;

  SALPY_REQUIRE_VALID (self, sal);

  sal_obj = (sal_object *) self;
  if (sal_obj->symtab == Py_None)
    filename = "<unknown>";
  else
    {
      symtab *symtab = symtab_object_to_symtab (sal_obj->symtab);
      filename = symtab_to_filename_for_display (symtab);
    }

  return PyUnicode_FromFormat ("symbol and line for %s, line %d", filename,
			       sal->line);
}

static void
stpy_dealloc (PyObject *obj)
{
  symtab_object *symtab = (symtab_object *) obj;

  if (symtab->prev)
    symtab->prev->next = symtab->next;
  else if (symtab->symtab)
    stpy_objfile_data_key.set (symtab->symtab->compunit ()->objfile (),
			       symtab->next);
  if (symtab->next)
    symtab->next->prev = symtab->prev;
  symtab->symtab = NULL;
  Py_TYPE (obj)->tp_free (obj);
}


static PyObject *
salpy_get_pc (PyObject *self, void *closure)
{
  struct symtab_and_line *sal = NULL;

  SALPY_REQUIRE_VALID (self, sal);

  return gdb_py_object_from_ulongest (sal->pc).release ();
}

/* Implementation of the get method for the 'last' attribute of
   gdb.Symtab_and_line.  */

static PyObject *
salpy_get_last (PyObject *self, void *closure)
{
  struct symtab_and_line *sal = NULL;

  SALPY_REQUIRE_VALID (self, sal);

  if (sal->end > 0)
    return gdb_py_object_from_ulongest (sal->end - 1).release ();
  else
    Py_RETURN_NONE;
}

static PyObject *
salpy_get_line (PyObject *self, void *closure)
{
  struct symtab_and_line *sal = NULL;

  SALPY_REQUIRE_VALID (self, sal);

  return gdb_py_object_from_longest (sal->line).release ();
}

static PyObject *
salpy_get_symtab (PyObject *self, void *closure)
{
  struct symtab_and_line *sal;
  sal_object *self_sal = (sal_object *) self;

  SALPY_REQUIRE_VALID (self, sal);

  Py_INCREF (self_sal->symtab);

  return (PyObject *) self_sal->symtab;
}

/* Implementation of gdb.Symtab_and_line.is_valid (self) -> Boolean.
   Returns True if this Symbol table and line object still exists GDB.  */

static PyObject *
salpy_is_valid (PyObject *self, PyObject *args)
{
  struct symtab_and_line *sal;

  sal = sal_object_to_symtab_and_line (self);
  if (sal == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

static void
salpy_dealloc (PyObject *self)
{
  sal_object *self_sal = (sal_object *) self;

  if (self_sal->prev)
    self_sal->prev->next = self_sal->next;
  else if (self_sal->symtab != Py_None)
    salpy_objfile_data_key.set
      (symtab_object_to_symtab (self_sal->symtab)->compunit ()->objfile (),
       self_sal->next);

  if (self_sal->next)
    self_sal->next->prev = self_sal->prev;

  Py_DECREF (self_sal->symtab);
  xfree (self_sal->sal);
  Py_TYPE (self)->tp_free (self);
}

/* Given a sal, and a sal_object that has previously been allocated
   and initialized, populate the sal_object with the struct sal data.
   Also, register the sal_object life-cycle with the life-cycle of the
   object file associated with this sal, if needed.  If a failure
   occurs during the sal population, this function will return -1.  */
static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
set_sal (sal_object *sal_obj, struct symtab_and_line sal)
{
  PyObject *symtab_obj;

  if (sal.symtab)
    {
      symtab_obj = symtab_to_symtab_object  (sal.symtab);
      /* If a symtab existed in the sal, but it cannot be duplicated,
	 we exit.  */
      if (symtab_obj == NULL)
	return -1;
    }
  else
    {
      symtab_obj = Py_None;
      Py_INCREF (Py_None);
    }

  sal_obj->sal = ((struct symtab_and_line *)
		  xmemdup (&sal, sizeof (struct symtab_and_line),
			   sizeof (struct symtab_and_line)));
  sal_obj->symtab = symtab_obj;
  sal_obj->prev = NULL;

  /* If the SAL does not have a symtab, we do not add it to the
     objfile cleanup observer linked list.  */
  if (sal_obj->symtab != Py_None)
    {
      symtab *symtab = symtab_object_to_symtab (sal_obj->symtab);

      sal_obj->next
	= salpy_objfile_data_key.get (symtab->compunit ()->objfile ());
      if (sal_obj->next)
	sal_obj->next->prev = sal_obj;

      salpy_objfile_data_key.set (symtab->compunit ()->objfile (), sal_obj);
    }
  else
    sal_obj->next = NULL;

  return 0;
}

/* Given a symtab, and a symtab_object that has previously been
   allocated and initialized, populate the symtab_object with the
   struct symtab data.  Also, register the symtab_object life-cycle
   with the life-cycle of the object file associated with this
   symtab, if needed.  */
static void
set_symtab (symtab_object *obj, struct symtab *symtab)
{
  obj->symtab = symtab;
  obj->prev = NULL;
  if (symtab)
    {
      obj->next = stpy_objfile_data_key.get (symtab->compunit ()->objfile ());
      if (obj->next)
	obj->next->prev = obj;
      stpy_objfile_data_key.set (symtab->compunit ()->objfile (), obj);
    }
  else
    obj->next = NULL;
}

/* Create a new symbol table (gdb.Symtab) object that encapsulates the
   symtab structure from GDB.  */
PyObject *
symtab_to_symtab_object (struct symtab *symtab)
{
  symtab_object *symtab_obj;

  symtab_obj = PyObject_New (symtab_object, &symtab_object_type);
  if (symtab_obj)
    set_symtab (symtab_obj, symtab);

  return (PyObject *) symtab_obj;
}

/* Create a new symtab and line (gdb.Symtab_and_line) object
   that encapsulates the symtab_and_line structure from GDB.  */
PyObject *
symtab_and_line_to_sal_object (struct symtab_and_line sal)
{
  gdbpy_ref<sal_object> sal_obj (PyObject_New (sal_object, &sal_object_type));
  if (sal_obj != NULL)
    {
      if (set_sal (sal_obj.get (), sal) < 0)
	return NULL;
    }

  return (PyObject *) sal_obj.release ();
}

/* Return struct symtab_and_line reference that is wrapped by this
   object.  */
struct symtab_and_line *
sal_object_to_symtab_and_line (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &sal_object_type))
    return NULL;
  return ((sal_object *) obj)->sal;
}

/* Return struct symtab reference that is wrapped by this object.  */
struct symtab *
symtab_object_to_symtab (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &symtab_object_type))
    return NULL;
  return ((symtab_object *) obj)->symtab;
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_symtabs (void)
{
  symtab_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&symtab_object_type) < 0)
    return -1;

  sal_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&sal_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "Symtab",
			      (PyObject *) &symtab_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Symtab_and_line",
				 (PyObject *) &sal_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_symtabs);



static gdb_PyGetSetDef symtab_object_getset[] = {
  { "filename", stpy_get_filename, NULL,
    "The symbol table's source filename.", NULL },
  { "objfile", stpy_get_objfile, NULL, "The symtab's objfile.",
    NULL },
  { "producer", stpy_get_producer, NULL,
    "The name/version of the program that compiled this symtab.", NULL },
  {NULL}  /* Sentinel */
};

static PyMethodDef symtab_object_methods[] = {
  { "is_valid", stpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this symbol table is valid, false if not." },
  { "fullname", stpy_fullname, METH_NOARGS,
    "fullname () -> String.\n\
Return the symtab's full source filename." },
  { "global_block", stpy_global_block, METH_NOARGS,
    "global_block () -> gdb.Block.\n\
Return the global block of the symbol table." },
  { "static_block", stpy_static_block, METH_NOARGS,
    "static_block () -> gdb.Block.\n\
Return the static block of the symbol table." },
    { "linetable", stpy_get_linetable, METH_NOARGS,
    "linetable () -> gdb.LineTable.\n\
Return the LineTable associated with this symbol table" },
  {NULL}  /* Sentinel */
};

PyTypeObject symtab_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Symtab",			  /*tp_name*/
  sizeof (symtab_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  stpy_dealloc,			  /*tp_dealloc*/
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
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB symtab object",		  /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  0,				  /*tp_iter */
  0,				  /*tp_iternext */
  symtab_object_methods,	  /*tp_methods */
  0,				  /*tp_members */
  symtab_object_getset		  /*tp_getset */
};

static gdb_PyGetSetDef sal_object_getset[] = {
  { "symtab", salpy_get_symtab, NULL, "Symtab object.", NULL },
  { "pc", salpy_get_pc, NULL, "Return the symtab_and_line's pc.", NULL },
  { "last", salpy_get_last, NULL,
    "Return the symtab_and_line's last address.", NULL },
  { "line", salpy_get_line, NULL,
    "Return the symtab_and_line's line.", NULL },
  {NULL}  /* Sentinel */
};

static PyMethodDef sal_object_methods[] = {
  { "is_valid", salpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this symbol table and line is valid, false if not." },
  {NULL}  /* Sentinel */
};

PyTypeObject sal_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Symtab_and_line",	  /*tp_name*/
  sizeof (sal_object),		  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  salpy_dealloc,		  /*tp_dealloc*/
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
  salpy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB symtab_and_line object",	  /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  0,				  /*tp_iter */
  0,				  /*tp_iternext */
  sal_object_methods,		  /*tp_methods */
  0,				  /*tp_members */
  sal_object_getset		  /*tp_getset */
};

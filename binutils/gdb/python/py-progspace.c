/* Python interface to program spaces.

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
#include "progspace.h"
#include "objfiles.h"
#include "language.h"
#include "arch-utils.h"
#include "solib.h"
#include "block.h"
#include "py-event.h"
#include "observable.h"
#include "inferior.h"

struct pspace_object
{
  PyObject_HEAD

  /* The corresponding pspace.  */
  struct program_space *pspace;

  /* Dictionary holding user-added attributes.
     This is the __dict__ attribute of the object.  */
  PyObject *dict;

  /* The pretty-printer list of functions.  */
  PyObject *printers;

  /* The frame filter list of functions.  */
  PyObject *frame_filters;

  /* The frame unwinder list.  */
  PyObject *frame_unwinders;

  /* The type-printer list.  */
  PyObject *type_printers;

  /* The debug method list.  */
  PyObject *xmethods;

  /* The missing debug handler list.  */
  PyObject *missing_debug_handlers;
};

extern PyTypeObject pspace_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("pspace_object");

/* Clear the PSPACE pointer in a Pspace object and remove the reference.  */
struct pspace_deleter
{
  void operator() (pspace_object *obj)
  {
    /* This is a fiction, but we're in a nasty spot: The pspace is in the
       process of being deleted, we can't rely on anything in it.  Plus
       this is one time when the current program space and current inferior
       are not in sync: All inferiors that use PSPACE may no longer exist.
       We don't need to do much here, and since "there is always an inferior"
       using the current inferior's arch suffices.
       Note: We cannot call get_current_arch because it may try to access
       the target, which may involve accessing data in the pspace currently
       being deleted.  */
    gdbarch *arch = current_inferior ()->arch ();

    gdbpy_enter enter_py (arch);
    gdbpy_ref<pspace_object> object (obj);
    object->pspace = NULL;
  }
};

static const registry<program_space>::key<pspace_object, pspace_deleter>
     pspy_pspace_data_key;

/* Require that PSPACE_OBJ be a valid program space ID.  */
#define PSPY_REQUIRE_VALID(pspace_obj)				\
  do {								\
    if (pspace_obj->pspace == nullptr)				\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Program space no longer exists."));	\
	return NULL;						\
      }								\
  } while (0)

/* An Objfile method which returns the objfile's file name, or None.  */

static PyObject *
pspy_get_filename (PyObject *self, void *closure)
{
  pspace_object *obj = (pspace_object *) self;

  if (obj->pspace)
    {
      struct objfile *objfile = obj->pspace->symfile_object_file;

      if (objfile)
	return (host_string_to_python_string (objfile_name (objfile))
		.release ());
    }
  Py_RETURN_NONE;
}

/* Implement the gdb.Progspace.symbol_file attribute.  Retun the
   gdb.Objfile corresponding to the currently loaded symbol-file, or None
   if no symbol-file is loaded.  If the Progspace is invalid then raise an
   exception.  */

static PyObject *
pspy_get_symbol_file (PyObject *self, void *closure)
{
  pspace_object *obj = (pspace_object *) self;

  PSPY_REQUIRE_VALID (obj);

  struct objfile *objfile = obj->pspace->symfile_object_file;

  if (objfile != nullptr)
    return objfile_to_objfile_object (objfile).release ();

  Py_RETURN_NONE;
}

/* Implement the gdb.Progspace.executable_filename attribute.  Retun a
   string containing the name of the current executable, or None if no
   executable is currently set.  If the Progspace is invalid then raise an
   exception.  */

static PyObject *
pspy_get_exec_file (PyObject *self, void *closure)
{
  pspace_object *obj = (pspace_object *) self;

  PSPY_REQUIRE_VALID (obj);

  const char *filename = obj->pspace->exec_filename.get ();
  if (filename != nullptr)
    return host_string_to_python_string (filename).release ();

  Py_RETURN_NONE;
}

static void
pspy_dealloc (PyObject *self)
{
  pspace_object *ps_self = (pspace_object *) self;

  Py_XDECREF (ps_self->dict);
  Py_XDECREF (ps_self->printers);
  Py_XDECREF (ps_self->frame_filters);
  Py_XDECREF (ps_self->frame_unwinders);
  Py_XDECREF (ps_self->type_printers);
  Py_XDECREF (ps_self->xmethods);
  Py_XDECREF (ps_self->missing_debug_handlers);
  Py_TYPE (self)->tp_free (self);
}

/* Initialize a pspace_object.
   The result is a boolean indicating success.  */

static int
pspy_initialize (pspace_object *self)
{
  self->pspace = NULL;

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

  self->missing_debug_handlers = PyList_New (0);
  if (self->missing_debug_handlers == nullptr)
    return 0;

  return 1;
}

PyObject *
pspy_get_printers (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->printers);
  return self->printers;
}

static int
pspy_set_printers (PyObject *o, PyObject *value, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the pretty_printers attribute");
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the pretty_printers attribute must be a list");
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->printers);
  Py_INCREF (value);
  self->printers = value;

  return 0;
}

/* Return the Python dictionary attribute containing frame filters for
   this program space.  */
PyObject *
pspy_get_frame_filters (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->frame_filters);
  return self->frame_filters;
}

/* Set this object file's frame filters dictionary to FILTERS.  */
static int
pspy_set_frame_filters (PyObject *o, PyObject *frame, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  if (! frame)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the frame filter attribute");
      return -1;
    }

  if (! PyDict_Check (frame))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the frame filter attribute must be a dictionary");
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->frame_filters);
  Py_INCREF (frame);
  self->frame_filters = frame;

  return 0;
}

/* Return the list of the frame unwinders for this program space.  */

PyObject *
pspy_get_frame_unwinders (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->frame_unwinders);
  return self->frame_unwinders;
}

/* Set this program space's list of the unwinders to UNWINDERS.  */

static int
pspy_set_frame_unwinders (PyObject *o, PyObject *unwinders, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  if (!unwinders)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the frame unwinders list");
      return -1;
    }

  if (!PyList_Check (unwinders))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the frame unwinders attribute must be a list");
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
pspy_get_type_printers (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->type_printers);
  return self->type_printers;
}

/* Get the 'xmethods' attribute.  */

PyObject *
pspy_get_xmethods (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->xmethods);
  return self->xmethods;
}

/* Return the list of missing debug handlers for this program space.  */

static PyObject *
pspy_get_missing_debug_handlers (PyObject *o, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  Py_INCREF (self->missing_debug_handlers);
  return self->missing_debug_handlers;
}

/* Set this program space's list of missing debug handlers to HANDLERS.  */

static int
pspy_set_missing_debug_handlers (PyObject *o, PyObject *handlers,
				 void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  if (handlers == nullptr)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the missing debug handlers list");
      return -1;
    }

  if (!PyList_Check (handlers))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the missing debug handlers attribute must be a list");
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->missing_debug_handlers);
  Py_INCREF (handlers);
  self->missing_debug_handlers = handlers;

  return 0;
}

/* Set the 'type_printers' attribute.  */

static int
pspy_set_type_printers (PyObject *o, PyObject *value, void *ignore)
{
  pspace_object *self = (pspace_object *) o;

  if (! value)
    {
      PyErr_SetString (PyExc_TypeError,
		       "cannot delete the type_printers attribute");
      return -1;
    }

  if (! PyList_Check (value))
    {
      PyErr_SetString (PyExc_TypeError,
		       "the type_printers attribute must be a list");
      return -1;
    }

  /* Take care in case the LHS and RHS are related somehow.  */
  gdbpy_ref<> tmp (self->type_printers);
  Py_INCREF (value);
  self->type_printers = value;

  return 0;
}

/* Implement the objfiles method.  */

static PyObject *
pspy_get_objfiles (PyObject *self_, PyObject *args)
{
  pspace_object *self = (pspace_object *) self_;

  PSPY_REQUIRE_VALID (self);

  gdbpy_ref<> list (PyList_New (0));
  if (list == NULL)
    return NULL;

  if (self->pspace != NULL)
    {
      for (objfile *objf : self->pspace->objfiles ())
	{
	  gdbpy_ref<> item = objfile_to_objfile_object (objf);

	  if (item == nullptr
	      || PyList_Append (list.get (), item.get ()) == -1)
	    return NULL;
	}
    }

  return list.release ();
}

/* Implementation of solib_name (Long) -> String.
   Returns the name of the shared library holding a given address, or None.  */

static PyObject *
pspy_solib_name (PyObject *o, PyObject *args)
{
  CORE_ADDR pc;
  PyObject *pc_obj;

  pspace_object *self = (pspace_object *) o;

  PSPY_REQUIRE_VALID (self);

  if (!PyArg_ParseTuple (args, "O", &pc_obj))
    return NULL;
  if (get_addr_from_python (pc_obj, &pc) < 0)
    return nullptr;

  const char *soname = solib_name_from_address (self->pspace, pc);
  if (soname == nullptr)
    Py_RETURN_NONE;
  return host_string_to_python_string (soname).release ();
}

/* Implement objfile_for_address.  */

static PyObject *
pspy_objfile_for_address (PyObject *o, PyObject *args)
{
  CORE_ADDR addr;
  PyObject *addr_obj;

  pspace_object *self = (pspace_object *) o;

  PSPY_REQUIRE_VALID (self);

  if (!PyArg_ParseTuple (args, "O", &addr_obj))
    return nullptr;
  if (get_addr_from_python (addr_obj, &addr) < 0)
    return nullptr;

  struct objfile *objf = self->pspace->objfile_for_address (addr);
  if (objf == nullptr)
    Py_RETURN_NONE;

  return objfile_to_objfile_object (objf).release ();
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  */
static PyObject *
pspy_block_for_pc (PyObject *o, PyObject *args)
{
  pspace_object *self = (pspace_object *) o;
  CORE_ADDR pc;
  PyObject *pc_obj;
  const struct block *block = NULL;
  struct compunit_symtab *cust = NULL;

  PSPY_REQUIRE_VALID (self);

  if (!PyArg_ParseTuple (args, "O", &pc_obj))
    return NULL;
  if (get_addr_from_python (pc_obj, &pc) < 0)
    return nullptr;

  try
    {
      scoped_restore_current_program_space saver;

      set_current_program_space (self->pspace);
      cust = find_pc_compunit_symtab (pc);

      if (cust != NULL && cust->objfile () != NULL)
	block = block_for_pc (pc);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (cust == NULL || cust->objfile () == NULL)
    Py_RETURN_NONE;

  if (block)
    return block_to_block_object (block, cust->objfile ());

  Py_RETURN_NONE;
}

/* Implementation of the find_pc_line function.
   Returns the gdb.Symtab_and_line object corresponding to a PC value.  */

static PyObject *
pspy_find_pc_line (PyObject *o, PyObject *args)
{
  CORE_ADDR pc;
  PyObject *result = NULL; /* init for gcc -Wall */
  PyObject *pc_obj;
  pspace_object *self = (pspace_object *) o;

  PSPY_REQUIRE_VALID (self);

  if (!PyArg_ParseTuple (args, "O", &pc_obj))
    return NULL;
  if (get_addr_from_python (pc_obj, &pc) < 0)
    return nullptr;

  try
    {
      struct symtab_and_line sal;
      scoped_restore_current_program_space saver;

      set_current_program_space (self->pspace);

      sal = find_pc_line (pc, 0);
      result = symtab_and_line_to_sal_object (sal);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Implementation of is_valid (self) -> Boolean.
   Returns True if this program space still exists in GDB.  */

static PyObject *
pspy_is_valid (PyObject *o, PyObject *args)
{
  pspace_object *self = (pspace_object *) o;

  if (self->pspace == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}



/* Return a new reference to the Python object of type Pspace
   representing PSPACE.  If the object has already been created,
   return it.  Otherwise, create it.  Return NULL and set the Python
   error on failure.  */

gdbpy_ref<>
pspace_to_pspace_object (struct program_space *pspace)
{
  PyObject *result = (PyObject *) pspy_pspace_data_key.get (pspace);
  if (result == NULL)
    {
      gdbpy_ref<pspace_object> object
	((pspace_object *) PyObject_New (pspace_object, &pspace_object_type));
      if (object == NULL)
	return NULL;
      if (!pspy_initialize (object.get ()))
	return NULL;

      object->pspace = pspace;
      pspy_pspace_data_key.set (pspace, object.get ());
      result = (PyObject *) object.release ();
    }

  return gdbpy_ref<>::new_reference (result);
}

/* See python-internal.h.  */

struct program_space *
progspace_object_to_program_space (PyObject *obj)
{
  gdb_assert (gdbpy_is_progspace (obj));
  return ((pspace_object *) obj)->pspace;
}

/* See python-internal.h.  */

bool
gdbpy_is_progspace (PyObject *obj)
{
  return PyObject_TypeCheck (obj, &pspace_object_type);
}

/* Emit an ExecutableChangedEvent event to REGISTRY.  Return 0 on success,
   or a negative value on error.  PSPACE is the program_space in which the
   current executable has changed, and RELOAD_P is true if the executable
   path stayed the same, but the file on disk changed, or false if the
   executable path actually changed.  */

static int
emit_executable_changed_event (eventregistry_object *registry,
			       struct program_space *pspace, bool reload_p)
{
  gdbpy_ref<> event_obj
    = create_event_object (&executable_changed_event_object_type);
  if (event_obj == nullptr)
    return -1;

  gdbpy_ref<> py_pspace = pspace_to_pspace_object (pspace);
  if (py_pspace == nullptr
      || evpy_add_attribute (event_obj.get (), "progspace",
			     py_pspace.get ()) < 0)
    return -1;

  gdbpy_ref<> py_reload_p (PyBool_FromLong (reload_p ? 1 : 0));
  if (py_reload_p == nullptr
      || evpy_add_attribute (event_obj.get (), "reload",
			     py_reload_p.get ()) < 0)
    return -1;

  return evpy_emit_event (event_obj.get (), registry);
}

/* Listener for the executable_changed observable, this is called when the
   current executable within PSPACE changes.  RELOAD_P is true if the
   executable path stayed the same but the file changed on disk.  RELOAD_P
   is false if the executable path was changed.  */

static void
gdbpy_executable_changed (struct program_space *pspace, bool reload_p)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  if (!evregpy_no_listeners_p (gdb_py_events.executable_changed))
    if (emit_executable_changed_event (gdb_py_events.executable_changed,
				       pspace, reload_p) < 0)
      gdbpy_print_stack ();
}

/* Helper function to emit NewProgspaceEvent (when ADDING_P is true) or
   FreeProgspaceEvent events (when ADDING_P is false).  */

static void
gdbpy_program_space_event (program_space *pspace, bool adding_p)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  eventregistry_object *registry;
  PyTypeObject *event_type;
  if (adding_p)
    {
      registry = gdb_py_events.new_progspace;
      event_type = &new_progspace_event_object_type;
    }
  else
    {
      registry = gdb_py_events.free_progspace;
      event_type = &free_progspace_event_object_type;
    }

  if (evregpy_no_listeners_p (registry))
    return;

  gdbpy_ref<> pspace_obj = pspace_to_pspace_object (pspace);
  if (pspace_obj == nullptr)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> event = create_event_object (event_type);
  if (event == nullptr
      || evpy_add_attribute (event.get (), "progspace",
			     pspace_obj.get ()) < 0
      || evpy_emit_event (event.get (), registry) < 0)
    gdbpy_print_stack ();
}

/* Emit a NewProgspaceEvent to indicate PSPACE has been created.  */

static void
gdbpy_new_program_space_event (program_space *pspace)
{
  gdbpy_program_space_event (pspace, true);
}

/* Emit a FreeProgspaceEvent to indicate PSPACE is just about to be removed
   from GDB.  */

static void
gdbpy_free_program_space_event (program_space *pspace)
{
  gdbpy_program_space_event (pspace, false);
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_pspace (void)
{
  gdb::observers::executable_changed.attach (gdbpy_executable_changed,
					     "py-progspace");
  gdb::observers::new_program_space.attach (gdbpy_new_program_space_event,
					    "py-progspace");
  gdb::observers::free_program_space.attach (gdbpy_free_program_space_event,
					     "py-progspace");

  if (PyType_Ready (&pspace_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Progspace",
				 (PyObject *) &pspace_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_pspace);



static gdb_PyGetSetDef pspace_getset[] =
{
  { "__dict__", gdb_py_generic_dict, NULL,
    "The __dict__ for this progspace.", &pspace_object_type },
  { "filename", pspy_get_filename, NULL,
    "The filename of the progspace's main symbol file, or None.", nullptr },
  { "symbol_file", pspy_get_symbol_file, nullptr,
    "The gdb.Objfile for the progspace's main symbol file, or None.",
    nullptr},
  { "executable_filename", pspy_get_exec_file, nullptr,
    "The filename for the progspace's executable, or None.", nullptr},
  { "pretty_printers", pspy_get_printers, pspy_set_printers,
    "Pretty printers.", NULL },
  { "frame_filters", pspy_get_frame_filters, pspy_set_frame_filters,
    "Frame filters.", NULL },
  { "frame_unwinders", pspy_get_frame_unwinders, pspy_set_frame_unwinders,
    "Frame unwinders.", NULL },
  { "type_printers", pspy_get_type_printers, pspy_set_type_printers,
    "Type printers.", NULL },
  { "xmethods", pspy_get_xmethods, NULL,
    "Debug methods.", NULL },
  { "missing_debug_handlers", pspy_get_missing_debug_handlers,
    pspy_set_missing_debug_handlers, "Missing debug handlers.", NULL },
  { NULL }
};

static PyMethodDef progspace_object_methods[] =
{
  { "objfiles", pspy_get_objfiles, METH_NOARGS,
    "Return a sequence of objfiles associated to this program space." },
  { "solib_name", pspy_solib_name, METH_VARARGS,
    "solib_name (Long) -> String.\n\
Return the name of the shared library holding a given address, or None." },
  { "objfile_for_address", pspy_objfile_for_address, METH_VARARGS,
    "objfile_for_address (int) -> gdb.Objfile\n\
Return the objfile containing the given address, or None." },
  { "block_for_pc", pspy_block_for_pc, METH_VARARGS,
    "Return the block containing the given pc value, or None." },
  { "find_pc_line", pspy_find_pc_line, METH_VARARGS,
    "find_pc_line (pc) -> Symtab_and_line.\n\
Return the gdb.Symtab_and_line object corresponding to the pc value." },
  { "is_valid", pspy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this program space is valid, false if not." },
  { NULL }
};

PyTypeObject pspace_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Progspace",		  /*tp_name*/
  sizeof (pspace_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  pspy_dealloc,			  /*tp_dealloc*/
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
  "GDB progspace object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  progspace_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  pspace_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  offsetof (pspace_object, dict), /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  0,				  /* tp_new */
};

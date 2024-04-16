/* Python interface to inferior threads.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "gdbthread.h"
#include "inferior.h"
#include "python-internal.h"

extern PyTypeObject thread_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("thread_object");

/* Require that INFERIOR be a valid inferior ID.  */
#define THPY_REQUIRE_VALID(Thread)				\
  do {								\
    if (!Thread->thread)					\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Thread no longer exists."));	\
	return NULL;						\
      }								\
  } while (0)

gdbpy_ref<thread_object>
create_thread_object (struct thread_info *tp)
{
  gdbpy_ref<thread_object> thread_obj;

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (tp->inf);
  if (inf_obj == NULL)
    return NULL;

  thread_obj.reset (PyObject_New (thread_object, &thread_object_type));
  if (thread_obj == NULL)
    return NULL;

  thread_obj->thread = tp;
  thread_obj->inf_obj = (PyObject *) inf_obj.release ();
  thread_obj->dict = PyDict_New ();
  if (thread_obj->dict == nullptr)
    return nullptr;

  return thread_obj;
}

static void
thpy_dealloc (PyObject *self)
{
  thread_object *thr_obj = (thread_object *) self;

  gdb_assert (thr_obj->inf_obj != nullptr);

  Py_DECREF (thr_obj->inf_obj);
  Py_XDECREF (thr_obj->dict);

  Py_TYPE (self)->tp_free (self);
}

static PyObject *
thpy_get_name (PyObject *self, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  const char *name = thread_name (thread_obj->thread);
  if (name == NULL)
    Py_RETURN_NONE;

  return PyUnicode_FromString (name);
}

/* Return a string containing target specific additional information about
   the state of the thread, or None, if there is no such additional
   information.  */

static PyObject *
thpy_get_details (PyObject *self, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  /* GCC can't tell that extra_info will always be assigned after the
     'catch', so initialize it.  */
  const char *extra_info = nullptr;
  try
    {
      extra_info = target_extra_thread_info (thread_obj->thread);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  if (extra_info == nullptr)
    Py_RETURN_NONE;

  return PyUnicode_FromString (extra_info);
}

static int
thpy_set_name (PyObject *self, PyObject *newvalue, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;
  gdb::unique_xmalloc_ptr<char> name;

  if (! thread_obj->thread)
    {
      PyErr_SetString (PyExc_RuntimeError, _("Thread no longer exists."));
      return -1;
    }

  if (newvalue == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete `name' attribute."));
      return -1;
    }
  else if (newvalue == Py_None)
    {
      /* Nothing.  */
    }
  else if (! gdbpy_is_string (newvalue))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value of `name' must be a string."));
      return -1;
    }
  else
    {
      name = python_string_to_host_string (newvalue);
      if (! name)
	return -1;
    }

  thread_obj->thread->set_name (std::move (name));

  return 0;
}

/* Getter for InferiorThread.num.  */

static PyObject *
thpy_get_num (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  gdbpy_ref<> result
    = gdb_py_object_from_longest (thread_obj->thread->per_inf_num);
  return result.release ();
}

/* Getter for InferiorThread.global_num.  */

static PyObject *
thpy_get_global_num (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  gdbpy_ref<> result
    = gdb_py_object_from_longest (thread_obj->thread->global_num);
  return result.release ();
}

/* Getter for InferiorThread.ptid  -> (pid, lwp, tid).
   Returns a tuple with the thread's ptid components.  */

static PyObject *
thpy_get_ptid (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  return gdbpy_create_ptid_object (thread_obj->thread->ptid);
}

/* Implement gdb.InferiorThread.ptid_string attribute.  */

static PyObject *
thpy_get_ptid_string (PyObject *self, void *closure)
{
  thread_object *thread_obj = (thread_object *) self;
  THPY_REQUIRE_VALID (thread_obj);
  ptid_t ptid = thread_obj->thread->ptid;

  try
    {
      /* Select the correct inferior before calling a target_* function.  */
      scoped_restore_current_thread restore_thread;
      switch_to_inferior_no_thread (thread_obj->thread->inf);
      std::string ptid_str = target_pid_to_str (ptid);
      return PyUnicode_FromString (ptid_str.c_str ());
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
      return nullptr;
    }
}

/* Getter for InferiorThread.inferior -> Inferior.  */

static PyObject *
thpy_get_inferior (PyObject *self, void *ignore)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);
  Py_INCREF (thread_obj->inf_obj);

  return thread_obj->inf_obj;
}

/* Implementation of InferiorThread.switch ().
   Makes this the GDB selected thread.  */

static PyObject *
thpy_switch (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  try
    {
      switch_to_thread (thread_obj->thread);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Implementation of InferiorThread.is_stopped () -> Boolean.
   Return whether the thread is stopped.  */

static PyObject *
thpy_is_stopped (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  if (thread_obj->thread->state == THREAD_STOPPED)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implementation of InferiorThread.is_running () -> Boolean.
   Return whether the thread is running.  */

static PyObject *
thpy_is_running (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  if (thread_obj->thread->state == THREAD_RUNNING)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implementation of InferiorThread.is_exited () -> Boolean.
   Return whether the thread is exited.  */

static PyObject *
thpy_is_exited (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  THPY_REQUIRE_VALID (thread_obj);

  if (thread_obj->thread->state == THREAD_EXITED)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implementation of gdb.InfThread.is_valid (self) -> Boolean.
   Returns True if this inferior Thread object still exists
   in GDB.  */

static PyObject *
thpy_is_valid (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;

  if (! thread_obj->thread)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.InferiorThread.handle (self) -> handle. */

static PyObject *
thpy_thread_handle (PyObject *self, PyObject *args)
{
  thread_object *thread_obj = (thread_object *) self;
  THPY_REQUIRE_VALID (thread_obj);

  gdb::array_view<const gdb_byte> hv;

  try
    {
      hv = target_thread_info_to_thread_handle (thread_obj->thread);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (hv.size () == 0)
    {
      PyErr_SetString (PyExc_RuntimeError, _("Thread handle not found."));
      return NULL;
    }

  PyObject *object = PyBytes_FromStringAndSize ((const char *) hv.data (),
						hv.size());
  return object;
}

/* Implement repr() for gdb.InferiorThread.  */

static PyObject *
thpy_repr (PyObject *self)
{
  thread_object *thread_obj = (thread_object *) self;

  if (thread_obj->thread == nullptr)
    return gdb_py_invalid_object_repr (self);

  thread_info *thr = thread_obj->thread;
  return PyUnicode_FromFormat ("<%s id=%s target-id=\"%s\">",
			       Py_TYPE (self)->tp_name,
			       print_full_thread_id (thr),
			       target_pid_to_str (thr->ptid).c_str ());
}

/* Return a reference to a new Python object representing a ptid_t.
   The object is a tuple containing (pid, lwp, tid). */
PyObject *
gdbpy_create_ptid_object (ptid_t ptid)
{
  int pid;
  long lwp;
  ULONGEST tid;
  PyObject *ret;

  ret = PyTuple_New (3);
  if (!ret)
    return NULL;

  pid = ptid.pid ();
  lwp = ptid.lwp ();
  tid = ptid.tid ();

  gdbpy_ref<> pid_obj = gdb_py_object_from_longest (pid);
  if (pid_obj == nullptr)
    return nullptr;
  gdbpy_ref<> lwp_obj = gdb_py_object_from_longest (lwp);
  if (lwp_obj == nullptr)
    return nullptr;
  gdbpy_ref<> tid_obj = gdb_py_object_from_ulongest (tid);
  if (tid_obj == nullptr)
    return nullptr;

  /* Note that these steal references, hence the use of 'release'.  */
  PyTuple_SET_ITEM (ret, 0, pid_obj.release ());
  PyTuple_SET_ITEM (ret, 1, lwp_obj.release ());
  PyTuple_SET_ITEM (ret, 2, tid_obj.release ());

  return ret;
}

/* Implementation of gdb.selected_thread () -> gdb.InferiorThread.
   Returns the selected thread object.  */

PyObject *
gdbpy_selected_thread (PyObject *self, PyObject *args)
{
  if (inferior_ptid != null_ptid)
    return thread_to_thread_object (inferior_thread ()).release ();

  Py_RETURN_NONE;
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_thread (void)
{
  if (PyType_Ready (&thread_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "InferiorThread",
				 (PyObject *) &thread_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_thread);



static gdb_PyGetSetDef thread_object_getset[] =
{
  { "__dict__", gdb_py_generic_dict, nullptr,
    "The __dict__ for this thread.", &thread_object_type },
  { "name", thpy_get_name, thpy_set_name,
    "The name of the thread, as set by the user or the OS.", NULL },
  { "details", thpy_get_details, NULL,
    "A target specific string containing extra thread state details.",
    NULL },
  { "num", thpy_get_num, NULL,
    "Per-inferior number of the thread, as assigned by GDB.", NULL },
  { "global_num", thpy_get_global_num, NULL,
    "Global number of the thread, as assigned by GDB.", NULL },
  { "ptid", thpy_get_ptid, NULL, "ID of the thread, as assigned by the OS.",
    NULL },
  { "ptid_string", thpy_get_ptid_string, nullptr,
    "A string representing ptid, as used by, for example, 'info threads'.",
    nullptr },
  { "inferior", thpy_get_inferior, NULL,
    "The Inferior object this thread belongs to.", NULL },

  { NULL }
};

static PyMethodDef thread_object_methods[] =
{
  { "is_valid", thpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this inferior thread is valid, false if not." },
  { "switch", thpy_switch, METH_NOARGS,
    "switch ()\n\
Makes this the GDB selected thread." },
  { "is_stopped", thpy_is_stopped, METH_NOARGS,
    "is_stopped () -> Boolean\n\
Return whether the thread is stopped." },
  { "is_running", thpy_is_running, METH_NOARGS,
    "is_running () -> Boolean\n\
Return whether the thread is running." },
  { "is_exited", thpy_is_exited, METH_NOARGS,
    "is_exited () -> Boolean\n\
Return whether the thread is exited." },
  { "handle", thpy_thread_handle, METH_NOARGS,
    "handle  () -> handle\n\
Return thread library specific handle for thread." },

  { NULL }
};

PyTypeObject thread_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.InferiorThread",		  /*tp_name*/
  sizeof (thread_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  thpy_dealloc,			  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  thpy_repr,			  /*tp_repr*/
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
  "GDB thread object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  thread_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  thread_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  offsetof (thread_object, dict), /* tp_dictoffset */
  0,				  /* tp_init */
  0				  /* tp_alloc */
};

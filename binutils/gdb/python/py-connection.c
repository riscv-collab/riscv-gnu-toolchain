/* Python interface to inferiors.

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
#include "python-internal.h"
#include "process-stratum-target.h"
#include "inferior.h"
#include "observable.h"
#include "target-connection.h"
#include "py-events.h"
#include "py-event.h"
#include "arch-utils.h"
#include "remote.h"
#include "charset.h"

#include <map>

/* The Python object that represents a connection.  */

struct connection_object
{
  PyObject_HEAD

  /* The process target that represents this connection.   When a
     connection_object is created this field will always point at a valid
     target.  Later, if GDB stops using this target (the target is popped
     from all target stacks) then this field is set to nullptr, which
     indicates that this Python object is now in the invalid state (see
     the is_valid() method below).  */
  struct process_stratum_target *target;
};

extern PyTypeObject connection_object_type
  CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("connection_object");

extern PyTypeObject remote_connection_object_type
  CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("remote_connection_object");

/* Require that CONNECTION be valid.  */
#define CONNPY_REQUIRE_VALID(connection)			\
  do {								\
    if (connection->target == nullptr)				\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Connection no longer exists."));	\
	return nullptr;						\
      }								\
  } while (0)

/* A map between process_stratum targets and the Python object representing
   them.  We actually hold a gdbpy_ref around the Python object so that
   reference counts are handled correctly when entries are deleted.  */
static std::map<process_stratum_target *,
		gdbpy_ref<connection_object>> all_connection_objects;

/* Return a reference to a gdb.TargetConnection object for TARGET.  If
   TARGET is nullptr then a reference to None is returned.

   Previously created gdb.TargetConnection objects are cached, and
   additional references to the same connection object can be returned with
   later calls to this function.  */

gdbpy_ref<>
target_to_connection_object (process_stratum_target *target)
{
  if (target == nullptr)
    return gdbpy_ref<>::new_reference (Py_None);

  gdbpy_ref <connection_object> conn_obj;
  auto conn_obj_iter = all_connection_objects.find (target);
  if (conn_obj_iter == all_connection_objects.end ())
    {
      PyTypeObject *type;

      if (is_remote_target (target))
	type = &remote_connection_object_type;
      else
	type = &connection_object_type;

      conn_obj.reset (PyObject_New (connection_object, type));
      if (conn_obj == nullptr)
	return nullptr;
      conn_obj->target = target;
      all_connection_objects.emplace (target, conn_obj);
    }
  else
    conn_obj = conn_obj_iter->second;

  gdb_assert (conn_obj != nullptr);

  /* Repackage the result as a PyObject reference.  */
  return gdbpy_ref<> ((PyObject *) conn_obj.release ());
}

/* Return a list of gdb.TargetConnection objects, one for each currently
   active connection.  The returned list is in no particular order.  */

PyObject *
gdbpy_connections (PyObject *self, PyObject *args)
{
  gdbpy_ref<> list (PyList_New (0));
  if (list == nullptr)
    return nullptr;

  for (process_stratum_target *target : all_non_exited_process_targets ())
    {
      gdb_assert (target != nullptr);

      gdbpy_ref<> conn = target_to_connection_object (target);
      if (conn == nullptr)
	return nullptr;
      gdb_assert (conn.get () != Py_None);

      if (PyList_Append (list.get (), conn.get ()) < 0)
	return nullptr;
    }

  return list.release ();
}

/* Emit a connection event for TARGET to REGISTRY.  Return 0 on success, or
   a negative value on error.  */

static int
emit_connection_event (process_stratum_target *target,
		       eventregistry_object *registry)
{
  gdbpy_ref<> event_obj
    = create_event_object (&connection_event_object_type);
  if (event_obj == nullptr)
    return -1;

  gdbpy_ref<> conn = target_to_connection_object (target);
  if (evpy_add_attribute (event_obj.get (), "connection", conn.get ()) < 0)
    return -1;

  return evpy_emit_event (event_obj.get (), registry);
}

/* Callback for the connection_removed observer.  */

static void
connpy_connection_removed (process_stratum_target *target)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  if (!evregpy_no_listeners_p (gdb_py_events.connection_removed))
    if (emit_connection_event (target, gdb_py_events.connection_removed) < 0)
      gdbpy_print_stack ();

  auto conn_obj_iter = all_connection_objects.find (target);
  if (conn_obj_iter != all_connection_objects.end ())
    {
      gdbpy_ref <connection_object> conn_obj = conn_obj_iter->second;
      conn_obj->target = nullptr;
      all_connection_objects.erase (target);
    }
}

/* Called when a gdb.TargetConnection object is deallocated.  */

static void
connpy_connection_dealloc (PyObject *obj)
{
  connection_object *conn_obj = (connection_object *) obj;

  /* As the all_connection_objects map holds a reference to each connection
     object we can only enter the dealloc function when the reference in
     all_connection_objects has been erased.

     As we always set the target pointer back to nullptr before we erase
     items from all_connection_objects then, when we get here, the target
     pointer must be nullptr.  */
  gdb_assert (conn_obj->target == nullptr);

  Py_TYPE (obj)->tp_free (obj);
}

/* Implement repr() for gdb.TargetConnection.  */

static PyObject *
connpy_repr (PyObject *obj)
{
  connection_object *self = (connection_object *) obj;
  process_stratum_target *target = self->target;

  if (target == nullptr)
    return gdb_py_invalid_object_repr (obj);

  return PyUnicode_FromFormat ("<%s num=%d, what=\"%s\">",
			       Py_TYPE (obj)->tp_name,
			       target->connection_number,
			       make_target_connection_string (target).c_str ());
}

/* Implementation of gdb.TargetConnection.is_valid() -> Boolean.  Returns
   True if this connection object is still associated with a
   process_stratum_target, otherwise, returns False.  */

static PyObject *
connpy_is_valid (PyObject *self, PyObject *args)
{
  connection_object *conn = (connection_object *) self;

  if (conn->target == nullptr)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Return the id number of this connection.  */

static PyObject *
connpy_get_connection_num (PyObject *self, void *closure)
{
  connection_object *conn = (connection_object *) self;

  CONNPY_REQUIRE_VALID (conn);

  auto num = conn->target->connection_number;
  return gdb_py_object_from_longest (num).release ();
}

/* Return a string that gives the short name for this connection type.  */

static PyObject *
connpy_get_connection_type (PyObject *self, void *closure)
{
  connection_object *conn = (connection_object *) self;

  CONNPY_REQUIRE_VALID (conn);

  const char *shortname = conn->target->shortname ();
  return host_string_to_python_string (shortname).release ();
}

/* Return a string that gives a longer description of this connection type.  */

static PyObject *
connpy_get_description (PyObject *self, void *closure)
{
  connection_object *conn = (connection_object *) self;

  CONNPY_REQUIRE_VALID (conn);

  const char *longname = conn->target->longname ();
  return host_string_to_python_string (longname).release ();
}

/* Return a string that gives additional details about this connection, or
   None, if there are no additional details for this connection type.  */

static PyObject *
connpy_get_connection_details (PyObject *self, void *closure)
{
  connection_object *conn = (connection_object *) self;

  CONNPY_REQUIRE_VALID (conn);

  const char *details = conn->target->connection_string ();
  if (details != nullptr)
    return host_string_to_python_string (details).release ();
  else
    Py_RETURN_NONE;
}

/* Python specific initialization for this file.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_connection (void)
{
  if (PyType_Ready (&connection_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "TargetConnection",
			      (PyObject *) &connection_object_type) < 0)
    return -1;

  if (PyType_Ready (&remote_connection_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "RemoteTargetConnection",
			      (PyObject *) &remote_connection_object_type) < 0)
    return -1;

  return 0;
}

/* Set of callbacks used to implement gdb.send_packet.  */

struct py_send_packet_callbacks : public send_remote_packet_callbacks
{
  /* Constructor, initialise the result to nullptr.  It is invalid to try
     and read the result before sending a packet and processing the
     reply.  */

  py_send_packet_callbacks ()
    : m_result (nullptr)
  { /* Nothing.  */ }

  /* There's nothing to do when the packet is sent.  */

  void sending (gdb::array_view<const char> &buf) override
  { /* Nothing.  */ }

  /* When the result is returned create a Python object and assign this
     into M_RESULT.  If for any reason we can't create a Python object to
     represent the result then M_RESULT is set to nullptr, and Python's
     internal error flags will be set.  If the result we got back from the
     remote is empty then set the result to None.  */

  void received (gdb::array_view<const char> &buf) override
  {
    if (buf.size () > 0 && buf.data ()[0] != '\0')
      m_result.reset (PyBytes_FromStringAndSize (buf.data (), buf.size ()));
    else
      {
	/* We didn't get back any result data; set the result to None.  */
	Py_INCREF (Py_None);
	m_result.reset (Py_None);
      }
  }

  /* Get a reference to the result as a Python object.  It is invalid to
     call this before sending a packet to the remote and processing the
     reply.

     The result value is setup in the RECEIVED call above.  If the RECEIVED
     call causes an error then the result value will be set to nullptr,
     and the error reason is left stored in Python's global error state.

     It is important that the result is inspected immediately after sending
     a packet to the remote, and any error fetched,  calling any other
     Python functions that might clear the error state, or rely on an error
     not being set will cause undefined behaviour.  */

  gdbpy_ref<> result () const
  {
    return m_result;
  }

private:

  /* A reference to the result value.  */

  gdbpy_ref<> m_result;
};

/* Implement RemoteTargetConnection.send_packet function.  Send a packet to
   the target identified by SELF.  The connection must still be valid, and
   the packet to be sent must be non-empty, otherwise an exception will be
   thrown.  */

static PyObject *
connpy_send_packet (PyObject *self, PyObject *args, PyObject *kw)
{
  connection_object *conn = (connection_object *) self;

  CONNPY_REQUIRE_VALID (conn);

  static const char *keywords[] = {"packet", nullptr};
  PyObject *packet_obj;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords,
					&packet_obj))
    return nullptr;

  /* If the packet is a unicode string then convert it to a bytes object.  */
  if (PyUnicode_Check (packet_obj))
    {
      /* We encode the string to bytes using the ascii codec, if this fails
	 then a suitable error will have been set.  */
      packet_obj = PyUnicode_AsASCIIString (packet_obj);
      if (packet_obj == nullptr)
	return nullptr;
    }

  /* Check the packet is now a bytes object.  */
  if (!PyBytes_Check (packet_obj))
    {
      PyErr_SetString (PyExc_TypeError, _("Packet is not a bytes object"));
      return nullptr;
    }

  Py_ssize_t packet_len = 0;
  char *packet_str_nonconst = nullptr;
  if (PyBytes_AsStringAndSize (packet_obj, &packet_str_nonconst,
			       &packet_len) < 0)
    return nullptr;
  const char *packet_str = packet_str_nonconst;
  gdb_assert (packet_str != nullptr);

  if (packet_len == 0)
    {
      PyErr_SetString (PyExc_ValueError, _("Packet must not be empty"));
      return nullptr;
    }

  try
    {
      scoped_restore_current_thread restore_thread;
      switch_to_target_no_thread (conn->target);

      gdb::array_view<const char> view (packet_str, packet_len);
      py_send_packet_callbacks callbacks;
      send_remote_packet (view, &callbacks);
      PyObject *result = callbacks.result ().release ();
      /* If we encountered an error converting the reply to a Python
	 object, then the result here can be nullptr.  In that case, Python
	 should be aware that an error occurred.  */
      gdb_assert ((result == nullptr) == (PyErr_Occurred () != nullptr));
      return result;
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return nullptr;
    }
}

/* Global initialization for this file.  */

void _initialize_py_connection ();
void
_initialize_py_connection ()
{
  gdb::observers::connection_removed.attach (connpy_connection_removed,
					     "py-connection");
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_connection);

/* Methods for the gdb.TargetConnection object type.  */

static PyMethodDef connection_object_methods[] =
{
  { "is_valid", connpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this TargetConnection is valid, false if not." },
  { NULL }
};

/* Methods for the gdb.RemoteTargetConnection object type.  */

static PyMethodDef remote_connection_object_methods[] =
{
  { "send_packet", (PyCFunction) connpy_send_packet,
    METH_VARARGS | METH_KEYWORDS,
    "send_packet (PACKET) -> Bytes\n\
Send PACKET to a remote target, return the reply as a bytes array." },
  { NULL }
};

/* Attributes for the gdb.TargetConnection object type.  */

static gdb_PyGetSetDef connection_object_getset[] =
{
  { "num", connpy_get_connection_num, NULL,
    "ID number of this connection, as assigned by GDB.", NULL },
  { "type", connpy_get_connection_type, NULL,
    "A short string that is the name for this connection type.", NULL },
  { "description", connpy_get_description, NULL,
    "A longer string describing this connection type.", NULL },
  { "details", connpy_get_connection_details, NULL,
    "A string containing additional connection details.", NULL },
  { NULL }
};

/* Define the gdb.TargetConnection object type.  */

PyTypeObject connection_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.TargetConnection",	  /* tp_name */
  sizeof (connection_object),	  /* tp_basicsize */
  0,				  /* tp_itemsize */
  connpy_connection_dealloc,	  /* tp_dealloc */
  0,				  /* tp_print */
  0,				  /* tp_getattr */
  0,				  /* tp_setattr */
  0,				  /* tp_compare */
  connpy_repr,			  /* tp_repr */
  0,				  /* tp_as_number */
  0,				  /* tp_as_sequence */
  0,				  /* tp_as_mapping */
  0,				  /* tp_hash  */
  0,				  /* tp_call */
  0,				  /* tp_str */
  0,				  /* tp_getattro */
  0,				  /* tp_setattro */
  0,				  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/* tp_flags */
  "GDB target connection object", /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  connection_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  connection_object_getset,	  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0				  /* tp_alloc */
};

/* Define the gdb.RemoteTargetConnection object type.  */

PyTypeObject remote_connection_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.RemoteTargetConnection",	  /* tp_name */
  sizeof (connection_object),	  /* tp_basicsize */
  0,				  /* tp_itemsize */
  connpy_connection_dealloc,	  /* tp_dealloc */
  0,				  /* tp_print */
  0,				  /* tp_getattr */
  0,				  /* tp_setattr */
  0,				  /* tp_compare */
  connpy_repr,			  /* tp_repr */
  0,				  /* tp_as_number */
  0,				  /* tp_as_sequence */
  0,				  /* tp_as_mapping */
  0,				  /* tp_hash  */
  0,				  /* tp_call */
  0,				  /* tp_str */
  0,				  /* tp_getattro */
  0,				  /* tp_setattro */
  0,				  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,		  /* tp_flags */
  "GDB remote target connection object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  remote_connection_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  &connection_object_type,	  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0				  /* tp_alloc */
};

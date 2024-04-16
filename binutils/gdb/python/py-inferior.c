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
#include "auto-load.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "inferior.h"
#include "objfiles.h"
#include "observable.h"
#include "python-internal.h"
#include "arch-utils.h"
#include "language.h"
#include "gdbsupport/gdb_signals.h"
#include "py-event.h"
#include "py-stopevent.h"
#include "progspace-and-thread.h"
#include <unordered_map>

using thread_map_t
  = std::unordered_map<thread_info *, gdbpy_ref<thread_object>>;

struct inferior_object
{
  PyObject_HEAD

  /* The inferior we represent.  */
  struct inferior *inferior;

  /* thread_object instances under this inferior.  This owns a
     reference to each object it contains.  */
  thread_map_t *threads;

  /* Dictionary holding user-added attributes.
     This is the __dict__ attribute of the object.  */
  PyObject *dict;
};

extern PyTypeObject inferior_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("inferior_object");

/* Deleter to clean up when an inferior is removed.  */
struct infpy_deleter
{
  void operator() (inferior_object *obj)
  {
    if (!gdb_python_initialized)
      return;

    gdbpy_enter enter_py;
    gdbpy_ref<inferior_object> inf_obj (obj);

    inf_obj->inferior = NULL;

    delete inf_obj->threads;
  }
};

static const registry<inferior>::key<inferior_object, infpy_deleter>
     infpy_inf_data_key;

/* Require that INFERIOR be a valid inferior ID.  */
#define INFPY_REQUIRE_VALID(Inferior)				\
  do {								\
    if (!Inferior->inferior)					\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Inferior no longer exists."));	\
	return NULL;						\
      }								\
  } while (0)

static void
python_on_normal_stop (struct bpstat *bs, int print_frame)
{
  enum gdb_signal stop_signal;

  if (!gdb_python_initialized)
    return;

  if (inferior_ptid == null_ptid)
    return;

  stop_signal = inferior_thread ()->stop_signal ();

  gdbpy_enter enter_py;

  if (emit_stop_event (bs, stop_signal) < 0)
    gdbpy_print_stack ();
}

static void
python_on_resume (ptid_t ptid)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (emit_continue_event (ptid) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when an inferior function call is about to be made. */

static void
python_on_inferior_call_pre (ptid_t thread, CORE_ADDR address)
{
  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (emit_inferior_call_event (INFERIOR_CALL_PRE, thread, address) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when an inferior function call has completed. */

static void
python_on_inferior_call_post (ptid_t thread, CORE_ADDR address)
{
  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (emit_inferior_call_event (INFERIOR_CALL_POST, thread, address) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when a part of memory has been modified by user action (eg via a
   'set' command). */

static void
python_on_memory_change (struct inferior *inferior, CORE_ADDR addr, ssize_t len, const bfd_byte *data)
{
  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (emit_memory_changed_event (addr, len) < 0)
    gdbpy_print_stack ();
}

/* Callback, registered as an observer, that notifies Python listeners
   when a register has been modified by user action (eg via a 'set'
   command). */

static void
python_on_register_change (frame_info_ptr frame, int regnum)
{
  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (emit_register_changed_event (frame, regnum) < 0)
    gdbpy_print_stack ();
}

static void
python_inferior_exit (struct inferior *inf)
{
  const LONGEST *exit_code = NULL;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (inf->has_exit_code)
    exit_code = &inf->exit_code;

  if (emit_exited_event (exit_code, inf) < 0)
    gdbpy_print_stack ();
}

/* Callback used to notify Python listeners about new objfiles loaded in the
   inferior.  OBJFILE may be NULL which means that the objfile list has been
   cleared (emptied).  */

static void
python_new_objfile (struct objfile *objfile)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (objfile->arch ());

  if (emit_new_objfile_event (objfile) < 0)
    gdbpy_print_stack ();
}

static void
python_all_objfiles_removed (program_space *pspace)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (current_inferior ()->arch ());

  if (emit_clear_objfiles_event (pspace) < 0)
    gdbpy_print_stack ();
}

/* Emit a Python event when an objfile is about to be removed.  */

static void
python_free_objfile (struct objfile *objfile)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (objfile->arch ());

  if (emit_free_objfile_event (objfile) < 0)
    gdbpy_print_stack ();
}

/* Return a reference to the Python object of type Inferior
   representing INFERIOR.  If the object has already been created,
   return it and increment the reference count,  otherwise, create it.
   Return NULL on failure.  */

gdbpy_ref<inferior_object>
inferior_to_inferior_object (struct inferior *inferior)
{
  inferior_object *inf_obj;

  inf_obj = infpy_inf_data_key.get (inferior);
  if (!inf_obj)
    {
      inf_obj = PyObject_New (inferior_object, &inferior_object_type);
      if (!inf_obj)
	return NULL;

      inf_obj->inferior = inferior;
      inf_obj->threads = new thread_map_t ();
      inf_obj->dict = PyDict_New ();
      if (inf_obj->dict == nullptr)
	return nullptr;

      /* PyObject_New initializes the new object with a refcount of 1.  This
	 counts for the reference we are keeping in the inferior data.  */
      infpy_inf_data_key.set (inferior, inf_obj);
    }

  /* We are returning a new reference.  */
  gdb_assert (inf_obj != nullptr);
  return gdbpy_ref<inferior_object>::new_reference (inf_obj);
}

/* Called when a new inferior is created.  Notifies any Python event
   listeners.  */
static void
python_new_inferior (struct inferior *inf)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  if (evregpy_no_listeners_p (gdb_py_events.new_inferior))
    return;

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (inf);
  if (inf_obj == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> event = create_event_object (&new_inferior_event_object_type);
  if (event == NULL
      || evpy_add_attribute (event.get (), "inferior",
			     (PyObject *) inf_obj.get ()) < 0
      || evpy_emit_event (event.get (), gdb_py_events.new_inferior) < 0)
    gdbpy_print_stack ();
}

/* Called when an inferior is removed.  Notifies any Python event
   listeners.  */
static void
python_inferior_deleted (struct inferior *inf)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  if (evregpy_no_listeners_p (gdb_py_events.inferior_deleted))
    return;

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (inf);
  if (inf_obj == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> event = create_event_object (&inferior_deleted_event_object_type);
  if (event == NULL
      || evpy_add_attribute (event.get (), "inferior",
			     (PyObject *) inf_obj.get ()) < 0
      || evpy_emit_event (event.get (), gdb_py_events.inferior_deleted) < 0)
    gdbpy_print_stack ();
}

gdbpy_ref<>
thread_to_thread_object (thread_info *thr)
{
  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (thr->inf);
  if (inf_obj == NULL)
    return NULL;

  auto thread_it = inf_obj->threads->find (thr);
  if (thread_it != inf_obj->threads->end ())
    return gdbpy_ref<>::new_reference
      ((PyObject *) (thread_it->second.get ()));

  PyErr_SetString (PyExc_SystemError,
		   _("could not find gdb thread object"));
  return NULL;
}

static void
add_thread_object (struct thread_info *tp)
{
  inferior_object *inf_obj;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  gdbpy_ref<thread_object> thread_obj = create_thread_object (tp);
  if (thread_obj == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  inf_obj = (inferior_object *) thread_obj->inf_obj;

  auto ins_result = inf_obj->threads->emplace
    (thread_map_t::value_type (tp, std::move (thread_obj)));

  if (!ins_result.second)
    return;

  if (evregpy_no_listeners_p (gdb_py_events.new_thread))
    return;

  gdbpy_ref<> event = create_thread_event_object
    (&new_thread_event_object_type,
     (PyObject *) ins_result.first->second.get ());

  if (event == NULL
      || evpy_emit_event (event.get (), gdb_py_events.new_thread) < 0)
    gdbpy_print_stack ();
}

static void
delete_thread_object (thread_info *tp,
		      std::optional<ULONGEST> /* exit_code */,
		      bool /* silent */)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  gdbpy_ref<inferior_object> inf_obj = inferior_to_inferior_object (tp->inf);
  if (inf_obj == NULL)
    return;

  if (emit_thread_exit_event (tp) < 0)
    gdbpy_print_stack ();

  auto it = inf_obj->threads->find (tp);
  if (it != inf_obj->threads->end ())
    {
      /* Some python code can still hold a reference to the thread_object
	 instance.   Make sure to remove the link to the associated
	 thread_info object as it will be freed soon.  This makes the python
	 object invalid (i.e. gdb.InfThread.is_valid returns False).  */
      it->second->thread = nullptr;
      inf_obj->threads->erase (it);
    }
}

static PyObject *
infpy_threads (PyObject *self, PyObject *args)
{
  int i = 0;
  inferior_object *inf_obj = (inferior_object *) self;
  PyObject *tuple;

  INFPY_REQUIRE_VALID (inf_obj);

  try
    {
      update_thread_list ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  tuple = PyTuple_New (inf_obj->threads->size ());
  if (!tuple)
    return NULL;

  for (const thread_map_t::value_type &entry : *inf_obj->threads)
    {
      PyObject *thr = (PyObject *) entry.second.get ();
      Py_INCREF (thr);
      PyTuple_SET_ITEM (tuple, i, thr);
      i = i + 1;
    }

  return tuple;
}

static PyObject *
infpy_get_num (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  return gdb_py_object_from_longest (inf->inferior->num).release ();
}

/* Return the gdb.TargetConnection object for this inferior, or None if a
   connection does not exist.  */

static PyObject *
infpy_get_connection (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  process_stratum_target *target = inf->inferior->process_target ();
  return target_to_connection_object (target).release ();
}

/* Return the connection number of the given inferior, or None if a
   connection does not exist.  */

static PyObject *
infpy_get_connection_num (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  process_stratum_target *target = inf->inferior->process_target ();
  if (target == nullptr)
    Py_RETURN_NONE;

  return gdb_py_object_from_longest (target->connection_number).release ();
}

static PyObject *
infpy_get_pid (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  return gdb_py_object_from_longest (inf->inferior->pid).release ();
}

static PyObject *
infpy_get_was_attached (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);
  if (inf->inferior->attach_flag)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Getter of gdb.Inferior.progspace.  */

static PyObject *
infpy_get_progspace (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  program_space *pspace = inf->inferior->pspace;
  gdb_assert (pspace != nullptr);

  return pspace_to_pspace_object (pspace).release ();
}

/* Implementation of gdb.inferiors () -> (gdb.Inferior, ...).
   Returns a tuple of all inferiors.  */
PyObject *
gdbpy_inferiors (PyObject *unused, PyObject *unused2)
{
  gdbpy_ref<> list (PyList_New (0));
  if (list == NULL)
    return NULL;

  for (inferior *inf : all_inferiors ())
    {
      gdbpy_ref<inferior_object> inferior = inferior_to_inferior_object (inf);

      if (inferior == NULL)
	continue;

      if (PyList_Append (list.get (), (PyObject *) inferior.get ()) != 0)
	return NULL;
    }

  return PyList_AsTuple (list.get ());
}

/* Membuf and memory manipulation.  */

/* Implementation of Inferior.read_memory (address, length).
   Returns a Python buffer object with LENGTH bytes of the inferior's
   memory at ADDRESS.  Both arguments are integers.  Returns NULL on error,
   with a python exception set.  */
static PyObject *
infpy_read_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  inferior_object *inf = (inferior_object *) self;
  CORE_ADDR addr, length;
  gdb::unique_xmalloc_ptr<gdb_byte> buffer;
  PyObject *addr_obj, *length_obj;
  static const char *keywords[] = { "address", "length", NULL };

  INFPY_REQUIRE_VALID (inf);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "OO", keywords,
					&addr_obj, &length_obj))
    return NULL;

  if (get_addr_from_python (addr_obj, &addr) < 0
      || get_addr_from_python (length_obj, &length) < 0)
    return NULL;

  try
    {
      /* Use this scoped-restore because we want to be able to read
	 memory from an unwinder.  */
      scoped_restore_current_inferior_for_memory restore_inferior
	(inf->inferior);

      buffer.reset ((gdb_byte *) xmalloc (length));

      read_memory (addr, buffer.get (), length);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }


  return gdbpy_buffer_to_membuf (std::move (buffer), addr, length);
}

/* Implementation of Inferior.write_memory (address, buffer [, length]).
   Writes the contents of BUFFER (a Python object supporting the read
   buffer protocol) at ADDRESS in the inferior's memory.  Write LENGTH
   bytes from BUFFER, or its entire contents if the argument is not
   provided.  The function returns nothing.  Returns NULL on error, with
   a python exception set.  */
static PyObject *
infpy_write_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  inferior_object *inf = (inferior_object *) self;
  struct gdb_exception except;
  Py_ssize_t buf_len;
  const gdb_byte *buffer;
  CORE_ADDR addr, length;
  PyObject *addr_obj, *length_obj = NULL;
  static const char *keywords[] = { "address", "buffer", "length", NULL };
  Py_buffer pybuf;

  INFPY_REQUIRE_VALID (inf);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "Os*|O", keywords,
					&addr_obj, &pybuf, &length_obj))
    return NULL;

  Py_buffer_up buffer_up (&pybuf);
  buffer = (const gdb_byte *) pybuf.buf;
  buf_len = pybuf.len;

  if (get_addr_from_python (addr_obj, &addr) < 0)
    return nullptr;

  if (!length_obj)
    length = buf_len;
  else if (get_addr_from_python (length_obj, &length) < 0)
    return nullptr;

  try
    {
      /* It's probably not too important to avoid invalidating the
	 frame cache when writing memory, but this scoped-restore is
	 still used here, just to keep the code similar to other code
	 in this file.  */
      scoped_restore_current_inferior_for_memory restore_inferior
	(inf->inferior);

      write_memory_with_notification (addr, buffer, length);
    }
  catch (gdb_exception &ex)
    {
      except = std::move (ex);
    }

  GDB_PY_HANDLE_EXCEPTION (except);

  Py_RETURN_NONE;
}

/* Implementation of
   Inferior.search_memory (address, length, pattern).  ADDRESS is the
   address to start the search.  LENGTH specifies the scope of the
   search from ADDRESS.  PATTERN is the pattern to search for (and
   must be a Python object supporting the buffer protocol).
   Returns a Python Long object holding the address where the pattern
   was located, or if the pattern was not found, returns None.  Returns NULL
   on error, with a python exception set.  */
static PyObject *
infpy_search_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  inferior_object *inf = (inferior_object *) self;
  struct gdb_exception except;
  CORE_ADDR start_addr, length;
  static const char *keywords[] = { "address", "length", "pattern", NULL };
  PyObject *start_addr_obj, *length_obj;
  Py_ssize_t pattern_size;
  const gdb_byte *buffer;
  CORE_ADDR found_addr;
  int found = 0;
  Py_buffer pybuf;

  INFPY_REQUIRE_VALID (inf);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "OOs*", keywords,
					&start_addr_obj, &length_obj,
					&pybuf))
    return NULL;

  Py_buffer_up buffer_up (&pybuf);
  buffer = (const gdb_byte *) pybuf.buf;
  pattern_size = pybuf.len;

  if (get_addr_from_python (start_addr_obj, &start_addr) < 0)
    return nullptr;

  if (get_addr_from_python (length_obj, &length) < 0)
    return nullptr;

  if (!length)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Search range is empty."));
      return nullptr;
    }
  /* Watch for overflows.  */
  else if (length > CORE_ADDR_MAX
	   || (start_addr + length - 1) < start_addr)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("The search range is too large."));
      return nullptr;
    }

  try
    {
      /* It's probably not too important to avoid invalidating the
	 frame cache when searching memory, but this scoped-restore is
	 still used here, just to keep the code similar to other code
	 in this file.  */
      scoped_restore_current_inferior_for_memory restore_inferior
	(inf->inferior);

      found = target_search_memory (start_addr, length,
				    buffer, pattern_size,
				    &found_addr);
    }
  catch (gdb_exception &ex)
    {
      except = std::move (ex);
    }

  GDB_PY_HANDLE_EXCEPTION (except);

  if (found)
    return gdb_py_object_from_ulongest (found_addr).release ();
  else
    Py_RETURN_NONE;
}

/* Implementation of gdb.Inferior.is_valid (self) -> Boolean.
   Returns True if this inferior object still exists in GDB.  */

static PyObject *
infpy_is_valid (PyObject *self, PyObject *args)
{
  inferior_object *inf = (inferior_object *) self;

  if (! inf->inferior)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Inferior.thread_from_handle (self, handle)
			->  gdb.InferiorThread.  */

static PyObject *
infpy_thread_from_thread_handle (PyObject *self, PyObject *args, PyObject *kw)
{
  PyObject *handle_obj;
  inferior_object *inf_obj = (inferior_object *) self;
  static const char *keywords[] = { "handle", NULL };

  INFPY_REQUIRE_VALID (inf_obj);

  if (! gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords, &handle_obj))
    return NULL;

  const gdb_byte *bytes;
  size_t bytes_len;
  Py_buffer_up buffer_up;
  Py_buffer py_buf;

  if (PyObject_CheckBuffer (handle_obj)
      && PyObject_GetBuffer (handle_obj, &py_buf, PyBUF_SIMPLE) == 0)
    {
      buffer_up.reset (&py_buf);
      bytes = (const gdb_byte *) py_buf.buf;
      bytes_len = py_buf.len;
    }
  else if (gdbpy_is_value_object (handle_obj))
    {
      struct value *val = value_object_to_value (handle_obj);
      bytes = val->contents_all ().data ();
      bytes_len = val->type ()->length ();
    }
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Argument 'handle' must be a thread handle object."));

      return NULL;
    }

  try
    {
      struct thread_info *thread_info;

      thread_info = find_thread_by_handle
	(gdb::array_view<const gdb_byte> (bytes, bytes_len),
	 inf_obj->inferior);
      if (thread_info != NULL)
	return thread_to_thread_object (thread_info).release ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Implementation of gdb.Inferior.architecture.  */

static PyObject *
infpy_architecture (PyObject *self, PyObject *args)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  return gdbarch_to_arch_object (inf->inferior->arch ());
}

/* Implement repr() for gdb.Inferior.  */

static PyObject *
infpy_repr (PyObject *obj)
{
  inferior_object *self = (inferior_object *) obj;
  inferior *inf = self->inferior;

  if (inf == nullptr)
    return gdb_py_invalid_object_repr (obj);

  return PyUnicode_FromFormat ("<gdb.Inferior num=%d, pid=%d>",
			       inf->num, inf->pid);
}

/* Implement clear_env.  */

static PyObject *
infpy_clear_env (PyObject *obj)
{
  inferior_object *self = (inferior_object *) obj;

  INFPY_REQUIRE_VALID (self);

  self->inferior->environment.clear ();
  Py_RETURN_NONE;
}

/* Implement set_env.  */

static PyObject *
infpy_set_env (PyObject *obj, PyObject *args, PyObject *kw)
{
  inferior_object *self = (inferior_object *) obj;
  INFPY_REQUIRE_VALID (self);

  const char *name, *val;
  static const char *keywords[] = { "name", "value", nullptr };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "ss", keywords,
					&name, &val))
    return nullptr;

  self->inferior->environment.set (name, val);
  Py_RETURN_NONE;
}

/* Implement unset_env.  */

static PyObject *
infpy_unset_env (PyObject *obj, PyObject *args, PyObject *kw)
{
  inferior_object *self = (inferior_object *) obj;
  INFPY_REQUIRE_VALID (self);

  const char *name;
  static const char *keywords[] = { "name", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s", keywords, &name))
    return nullptr;

  self->inferior->environment.unset (name);
  Py_RETURN_NONE;
}

/* Getter for "arguments".  */

static PyObject *
infpy_get_args (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  const std::string &args = inf->inferior->args ();
  if (args.empty ())
    Py_RETURN_NONE;

  return host_string_to_python_string (args.c_str ()).release ();
}

/* Setter for "arguments".  */

static int
infpy_set_args (PyObject *self, PyObject *value, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  if (!inf->inferior)
    {
      PyErr_SetString (PyExc_RuntimeError, _("Inferior no longer exists."));
      return -1;
    }

  if (value == nullptr)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Cannot delete 'arguments' attribute."));
      return -1;
    }

  if (gdbpy_is_string (value))
    {
      gdb::unique_xmalloc_ptr<char> str = python_string_to_host_string (value);
      if (str == nullptr)
	return -1;
      inf->inferior->set_args (std::string (str.get ()));
    }
  else if (PySequence_Check (value))
    {
      std::vector<gdb::unique_xmalloc_ptr<char>> args;
      Py_ssize_t len = PySequence_Size (value);
      if (len == -1)
	return -1;
      for (Py_ssize_t i = 0; i < len; ++i)
	{
	  gdbpy_ref<> item (PySequence_ITEM (value, i));
	  if (item == nullptr)
	    return -1;
	  gdb::unique_xmalloc_ptr<char> str
	    = python_string_to_host_string (item.get ());
	  if (str == nullptr)
	    return -1;
	  args.push_back (std::move (str));
	}
      std::vector<char *> argvec;
      for (const auto &arg : args)
	argvec.push_back (arg.get ());
      gdb::array_view<char * const> view (argvec.data (), argvec.size ());
      inf->inferior->set_args (view);
    }
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("string or sequence required for 'arguments'"));
      return -1;
    }
  return 0;
}

/* Getter for "main_name".  */

static PyObject *
infpy_get_main_name (PyObject *self, void *closure)
{
  inferior_object *inf = (inferior_object *) self;

  INFPY_REQUIRE_VALID (inf);

  const char *name = nullptr;
  try
    {
      /* This is unfortunate but the implementation of main_name can
	 reach into memory.  It's probably not too important to avoid
	 invalidating the frame cache here, but this scoped-restore is
	 still used, just to keep the code similar to other code in
	 this file.  */
      scoped_restore_current_inferior_for_memory restore_inferior
	(inf->inferior);

      name = main_name ();
    }
  catch (const gdb_exception &except)
    {
      /* We can just ignore this.  */
    }

  if (name == nullptr)
    Py_RETURN_NONE;

  return host_string_to_python_string (name).release ();
}

static void
infpy_dealloc (PyObject *obj)
{
  inferior_object *inf_obj = (inferior_object *) obj;

  /* The inferior itself holds a reference to this Python object, which
     will keep the reference count of this object above zero until GDB
     deletes the inferior and py_free_inferior is called.

     Once py_free_inferior has been called then the link between this
     Python object and the inferior is set to nullptr, and then the
     reference count on this Python object is decremented.

     The result of all this is that the link between this Python object and
     the inferior should always have been set to nullptr before this
     function is called.  */
  gdb_assert (inf_obj->inferior == nullptr);

  Py_XDECREF (inf_obj->dict);

  Py_TYPE (obj)->tp_free (obj);
}

/* Implementation of gdb.selected_inferior() -> gdb.Inferior.
   Returns the current inferior object.  */

PyObject *
gdbpy_selected_inferior (PyObject *self, PyObject *args)
{
  return ((PyObject *)
	  inferior_to_inferior_object (current_inferior ()).release ());
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_inferior (void)
{
  if (PyType_Ready (&inferior_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "Inferior",
			      (PyObject *) &inferior_object_type) < 0)
    return -1;

  gdb::observers::new_thread.attach (add_thread_object, "py-inferior");
  gdb::observers::thread_exit.attach (delete_thread_object, "py-inferior");
  gdb::observers::normal_stop.attach (python_on_normal_stop, "py-inferior");
  gdb::observers::target_resumed.attach (python_on_resume, "py-inferior");
  gdb::observers::inferior_call_pre.attach (python_on_inferior_call_pre,
					    "py-inferior");
  gdb::observers::inferior_call_post.attach (python_on_inferior_call_post,
					     "py-inferior");
  gdb::observers::memory_changed.attach (python_on_memory_change,
					 "py-inferior");
  gdb::observers::register_changed.attach (python_on_register_change,
					   "py-inferior");
  gdb::observers::inferior_exit.attach (python_inferior_exit, "py-inferior");
  /* Need to run after auto-load's new_objfile observer, so that
     auto-loaded pretty-printers are available.  */
  gdb::observers::new_objfile.attach
    (python_new_objfile, "py-inferior",
     { &auto_load_new_objfile_observer_token });
  gdb::observers::all_objfiles_removed.attach (python_all_objfiles_removed,
					       "py-inferior");
  gdb::observers::free_objfile.attach (python_free_objfile, "py-inferior");
  gdb::observers::inferior_added.attach (python_new_inferior, "py-inferior");
  gdb::observers::inferior_removed.attach (python_inferior_deleted,
					   "py-inferior");

  return 0;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_inferior);



static gdb_PyGetSetDef inferior_object_getset[] =
{
  { "__dict__", gdb_py_generic_dict, nullptr,
    "The __dict__ for this inferior.", &inferior_object_type },
  { "arguments", infpy_get_args, infpy_set_args,
    "Arguments to this program.", nullptr },
  { "num", infpy_get_num, NULL, "ID of inferior, as assigned by GDB.", NULL },
  { "connection", infpy_get_connection, NULL,
    "The gdb.TargetConnection for this inferior.", NULL },
  { "connection_num", infpy_get_connection_num, NULL,
    "ID of inferior's connection, as assigned by GDB.", NULL },
  { "pid", infpy_get_pid, NULL, "PID of inferior, as assigned by the OS.",
    NULL },
  { "was_attached", infpy_get_was_attached, NULL,
    "True if the inferior was created using 'attach'.", NULL },
  { "progspace", infpy_get_progspace, NULL, "Program space of this inferior" },
  { "main_name", infpy_get_main_name, nullptr,
    "Name of 'main' function, if known.", nullptr },
  { NULL }
};

static PyMethodDef inferior_object_methods[] =
{
  { "is_valid", infpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this inferior is valid, false if not." },
  { "threads", infpy_threads, METH_NOARGS,
    "Return all the threads of this inferior." },
  { "read_memory", (PyCFunction) infpy_read_memory,
    METH_VARARGS | METH_KEYWORDS,
    "read_memory (address, length) -> buffer\n\
Return a buffer object for reading from the inferior's memory." },
  { "write_memory", (PyCFunction) infpy_write_memory,
    METH_VARARGS | METH_KEYWORDS,
    "write_memory (address, buffer [, length])\n\
Write the given buffer object to the inferior's memory." },
  { "search_memory", (PyCFunction) infpy_search_memory,
    METH_VARARGS | METH_KEYWORDS,
    "search_memory (address, length, pattern) -> long\n\
Return a long with the address of a match, or None." },
  /* thread_from_thread_handle is deprecated.  */
  { "thread_from_thread_handle", (PyCFunction) infpy_thread_from_thread_handle,
    METH_VARARGS | METH_KEYWORDS,
    "thread_from_thread_handle (handle) -> gdb.InferiorThread.\n\
Return thread object corresponding to thread handle.\n\
This method is deprecated - use thread_from_handle instead." },
  { "thread_from_handle", (PyCFunction) infpy_thread_from_thread_handle,
    METH_VARARGS | METH_KEYWORDS,
    "thread_from_handle (handle) -> gdb.InferiorThread.\n\
Return thread object corresponding to thread handle." },
  { "architecture", (PyCFunction) infpy_architecture, METH_NOARGS,
    "architecture () -> gdb.Architecture\n\
Return architecture of this inferior." },
  { "clear_env", (PyCFunction) infpy_clear_env, METH_NOARGS,
    "clear_env () -> None\n\
Clear environment of this inferior." },
  { "set_env", (PyCFunction) infpy_set_env, METH_VARARGS | METH_KEYWORDS,
    "set_env (name, value) -> None\n\
Set an environment variable of this inferior." },
  { "unset_env", (PyCFunction) infpy_unset_env, METH_VARARGS | METH_KEYWORDS,
    "unset_env (name) -> None\n\
Unset an environment of this inferior." },
  { NULL }
};

PyTypeObject inferior_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Inferior",		  /* tp_name */
  sizeof (inferior_object),	  /* tp_basicsize */
  0,				  /* tp_itemsize */
  infpy_dealloc,		  /* tp_dealloc */
  0,				  /* tp_print */
  0,				  /* tp_getattr */
  0,				  /* tp_setattr */
  0,				  /* tp_compare */
  infpy_repr,			  /* tp_repr */
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
  "GDB inferior object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  inferior_object_methods,	  /* tp_methods */
  0,				  /* tp_members */
  inferior_object_getset,	  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  offsetof (inferior_object, dict), /* tp_dictoffset */
  0,				  /* tp_init */
  0				  /* tp_alloc */
};

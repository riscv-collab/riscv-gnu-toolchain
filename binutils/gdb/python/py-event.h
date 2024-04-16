/* Python interface to inferior events.

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

#ifndef PYTHON_PY_EVENT_H
#define PYTHON_PY_EVENT_H

#include "py-events.h"
#include "command.h"
#include "python-internal.h"
#include "inferior.h"

/* Declare all event types.  */
#define GDB_PY_DEFINE_EVENT_TYPE(name, py_name, doc, base) \
  extern PyTypeObject name##_event_object_type		    \
	CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
#include "py-event-types.def"
#undef GDB_PY_DEFINE_EVENT_TYPE

struct event_object
{
  PyObject_HEAD

  PyObject *dict;
};

extern int emit_continue_event (ptid_t ptid);
extern int emit_exited_event (const LONGEST *exit_code, struct inferior *inf);

/* For inferior function call events, discriminate whether event is
   before or after the call. */

enum inferior_call_kind
{
  /* Before the call */
  INFERIOR_CALL_PRE,
  /* after the call */
  INFERIOR_CALL_POST,
};

extern int emit_inferior_call_event (inferior_call_kind kind,
				     ptid_t thread, CORE_ADDR addr);
extern int emit_register_changed_event (frame_info_ptr frame,
					int regnum);
extern int emit_memory_changed_event (CORE_ADDR addr, ssize_t len);
extern int evpy_emit_event (PyObject *event,
			    eventregistry_object *registry);

/* Emits a thread exit event for THREAD */
extern int emit_thread_exit_event (thread_info * thread);

extern gdbpy_ref<> create_event_object (PyTypeObject *py_type);

/* thread events can either be thread specific or process wide.  If gdb is
   running in non-stop mode then the event is thread specific, otherwise
   it is process wide.
   This function returns the currently stopped thread in non-stop mode and
   Py_None otherwise.  */
extern gdbpy_ref<> py_get_event_thread (ptid_t ptid);

extern gdbpy_ref<> create_thread_event_object (PyTypeObject *py_type,
					       PyObject *thread);

extern int emit_new_objfile_event (struct objfile *objfile);
extern int emit_free_objfile_event (struct objfile *objfile);
extern int emit_clear_objfiles_event (program_space *pspace);

extern void evpy_dealloc (PyObject *self);
extern int evpy_add_attribute (PyObject *event,
			       const char *name, PyObject *attr)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;
int gdbpy_initialize_event_generic (PyTypeObject *type, const char *name)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;

#endif /* PYTHON_PY_EVENT_H */

/* Python interface to inferior stop events.

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
#include "py-stopevent.h"
#include "py-uiout.h"

gdbpy_ref<>
create_stop_event_object (PyTypeObject *py_type, const gdbpy_ref<> &dict)
{
  gdbpy_ref<> thread = py_get_event_thread (inferior_ptid);
  if (thread == nullptr)
    return nullptr;

  gdbpy_ref<> result = create_thread_event_object (py_type, thread.get ());
  if (result == nullptr)
    return nullptr;

  if (evpy_add_attribute (result.get (), "details", dict.get ()) < 0)
    return nullptr;

  return result;
}

/* Print BPSTAT to a new Python dictionary.  Returns the dictionary,
   or null if a Python exception occurred.  */

static gdbpy_ref<>
py_print_bpstat (bpstat *bs, enum gdb_signal stop_signal)
{
  py_ui_out uiout;

  try
    {
      scoped_restore save_uiout = make_scoped_restore (&current_uiout, &uiout);

      thread_info *tp = inferior_thread ();
      if (tp->thread_fsm () != nullptr && tp->thread_fsm ()->finished_p ())
	{
	  async_reply_reason reason = tp->thread_fsm ()->async_reply_reason ();
	  uiout.field_string ("reason", async_reason_lookup (reason));
	}

      if (stop_signal != GDB_SIGNAL_0 && stop_signal != GDB_SIGNAL_TRAP)
	print_signal_received_reason (&uiout, stop_signal);
      else
	{
	  struct target_waitstatus last;
	  get_last_target_status (nullptr, nullptr, &last);

	  bpstat_print (bs, last.kind ());
	}
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return nullptr;
    }

  return uiout.result ();
}

/* Callback observers when a stop event occurs.  This function will create a
   new Python stop event object.  If only a specific thread is stopped the
   thread object of the event will be set to that thread.  Otherwise, if all
   threads are stopped thread object will be set to None.
   return 0 if the event was created and emitted successfully otherwise
   returns -1.  */

int
emit_stop_event (struct bpstat *bs, enum gdb_signal stop_signal)
{
  gdbpy_ref<> stop_event_obj;
  gdbpy_ref<> list;
  PyObject *first_bp = NULL;
  struct bpstat *current_bs;

  if (evregpy_no_listeners_p (gdb_py_events.stop))
    return 0;

  gdbpy_ref<> dict = py_print_bpstat (bs, stop_signal);
  if (dict == nullptr)
    return -1;

  /* Add any breakpoint set at this location to the list.  */
  for (current_bs = bs; current_bs != NULL; current_bs = current_bs->next)
    {
      if (current_bs->breakpoint_at
	  && current_bs->breakpoint_at->py_bp_object)
	{
	  PyObject *current_py_bp =
	      (PyObject *) current_bs->breakpoint_at->py_bp_object;

	  if (list == NULL)
	    {
	      list.reset (PyList_New (0));
	      if (list == NULL)
		return -1;
	    }

	  if (PyList_Append (list.get (), current_py_bp))
	    return -1;

	  if (first_bp == NULL)
	    first_bp = current_py_bp;
	}
    }

  if (list != NULL)
    {
      stop_event_obj = create_breakpoint_event_object (dict,
						       list.get (),
						       first_bp);
      if (stop_event_obj == NULL)
	return -1;
    }

  /* Check if the signal is "Signal 0" or "Trace/breakpoint trap".  */
  if (stop_signal != GDB_SIGNAL_0
      && stop_signal != GDB_SIGNAL_TRAP)
    {
      stop_event_obj = create_signal_event_object (dict, stop_signal);
      if (stop_event_obj == NULL)
	return -1;
    }

  /* If all fails emit an unknown stop event.  All event types should
     be known and this should eventually be unused.  */
  if (stop_event_obj == NULL)
    {
      stop_event_obj = create_stop_event_object (&stop_event_object_type,
						 dict);
      if (stop_event_obj == NULL)
	return -1;
    }

  return evpy_emit_event (stop_event_obj.get (), gdb_py_events.stop);
}

/* Python interface to record targets.

   Copyright 2016-2024 Free Software Foundation, Inc.

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
#include "py-instruction.h"
#include "py-record.h"
#include "py-record-btrace.h"
#include "py-record-full.h"
#include "target.h"
#include "gdbthread.h"

/* Python Record type.  */

static PyTypeObject recpy_record_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
};

/* Python RecordInstruction type.  */

PyTypeObject recpy_insn_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
};

/* Python RecordFunctionSegment type.  */

PyTypeObject recpy_func_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
};

/* Python RecordGap type.  */

static PyTypeObject recpy_gap_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
};

/* Python RecordGap object.  */
struct recpy_gap_object
{
  PyObject_HEAD

  /* Reason code.  */
  int reason_code;

  /* Reason message.  */
  const char *reason_string;

  /* Element number.  */
  Py_ssize_t number;
};

/* Implementation of record.method.  */

static PyObject *
recpy_method (PyObject *self, void* closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_FULL)
    return recpy_full_method (self, closure);

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_method (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.format.  */

static PyObject *
recpy_format (PyObject *self, void* closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_FULL)
    return recpy_full_format (self, closure);

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_format (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.goto (instruction) -> None.  */

static PyObject *
recpy_goto (PyObject *self, PyObject *value)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_goto (self, value);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.replay_position [instruction]  */

static PyObject *
recpy_replay_position (PyObject *self, void *closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_replay_position (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.instruction_history [list].  */

static PyObject *
recpy_instruction_history (PyObject *self, void* closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_instruction_history (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.function_call_history [list].  */

static PyObject *
recpy_function_call_history (PyObject *self, void* closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_function_call_history (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.begin [instruction].  */

static PyObject *
recpy_begin (PyObject *self, void* closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_begin (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of record.end [instruction].  */

static PyObject *
recpy_end (PyObject *self, void* closure)
{
  const recpy_record_object * const obj = (recpy_record_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_end (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Create a new gdb.RecordInstruction object.  */

PyObject *
recpy_insn_new (thread_info *thread, enum record_method method,
		Py_ssize_t number)
{
  recpy_element_object * const obj = PyObject_New (recpy_element_object,
						   &recpy_insn_type);

  if (obj == NULL)
   return NULL;

  obj->thread = thread;
  obj->method = method;
  obj->number = number;

  return (PyObject *) obj;
}

/* Implementation of RecordInstruction.sal [gdb.Symtab_and_line].  */

static PyObject *
recpy_insn_sal (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_insn_sal (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordInstruction.pc [int].  */

static PyObject *
recpy_insn_pc (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_insn_pc (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordInstruction.data [buffer].  */

static PyObject *
recpy_insn_data (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_insn_data (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordInstruction.decoded [str].  */

static PyObject *
recpy_insn_decoded (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_insn_decoded (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordInstruction.size [int].  */

static PyObject *
recpy_insn_size (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_insn_size (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordInstruction.is_speculative [bool].  */

static PyObject *
recpy_insn_is_speculative (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_insn_is_speculative (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Create a new gdb.RecordFunctionSegment object.  */

PyObject *
recpy_func_new (thread_info *thread, enum record_method method,
		Py_ssize_t number)
{
  recpy_element_object * const obj = PyObject_New (recpy_element_object,
						   &recpy_func_type);

  if (obj == NULL)
   return NULL;

  obj->thread = thread;
  obj->method = method;
  obj->number = number;

  return (PyObject *) obj;
}

/* Implementation of RecordFunctionSegment.level [int].  */

static PyObject *
recpy_func_level (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_func_level (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordFunctionSegment.symbol [gdb.Symbol].  */

static PyObject *
recpy_func_symbol (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_func_symbol (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordFunctionSegment.instructions [list].  */

static PyObject *
recpy_func_instructions (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_func_instructions (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordFunctionSegment.up [RecordFunctionSegment].  */

static PyObject *
recpy_func_up (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_func_up (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordFunctionSegment.prev [RecordFunctionSegment].  */

static PyObject *
recpy_func_prev (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_func_prev (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordFunctionSegment.next [RecordFunctionSegment].  */

static PyObject *
recpy_func_next (PyObject *self, void *closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  if (obj->method == RECORD_METHOD_BTRACE)
    return recpy_bt_func_next (self, closure);

  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Implementation of RecordInstruction.number [int] and
   RecordFunctionSegment.number [int].  */

static PyObject *
recpy_element_number (PyObject *self, void* closure)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  return gdb_py_object_from_longest (obj->number).release ();
}

/* Implementation of RecordInstruction.__hash__ [int] and
   RecordFunctionSegment.__hash__ [int].  */

static Py_hash_t
recpy_element_hash (PyObject *self)
{
  const recpy_element_object * const obj = (recpy_element_object *) self;

  return obj->number;
}

/* Implementation of operator == and != of RecordInstruction and
   RecordFunctionSegment.  */

static PyObject *
recpy_element_richcompare (PyObject *self, PyObject *other, int op)
{
  const recpy_element_object * const obj1 = (recpy_element_object *) self;
  const recpy_element_object * const obj2 = (recpy_element_object *) other;

  if (Py_TYPE (self) != Py_TYPE (other))
    {
      Py_INCREF (Py_NotImplemented);
      return Py_NotImplemented;
    }

  switch (op)
  {
    case Py_EQ:
      if (obj1->thread == obj2->thread
	  && obj1->method == obj2->method
	  && obj1->number == obj2->number)
	Py_RETURN_TRUE;
      else
	Py_RETURN_FALSE;

    case Py_NE:
      if (obj1->thread != obj2->thread
	  || obj1->method != obj2->method
	  || obj1->number != obj2->number)
	Py_RETURN_TRUE;
      else
	Py_RETURN_FALSE;

    default:
      break;
  }

  Py_INCREF (Py_NotImplemented);
  return Py_NotImplemented;
}

/* Create a new gdb.RecordGap object.  */

PyObject *
recpy_gap_new (int reason_code, const char *reason_string, Py_ssize_t number)
{
  recpy_gap_object * const obj = PyObject_New (recpy_gap_object,
					       &recpy_gap_type);

  if (obj == NULL)
   return NULL;

  obj->reason_code = reason_code;
  obj->reason_string = reason_string;
  obj->number = number;

  return (PyObject *) obj;
}

/* Implementation of RecordGap.number [int].  */

static PyObject *
recpy_gap_number (PyObject *self, void *closure)
{
  const recpy_gap_object * const obj = (const recpy_gap_object *) self;

  return gdb_py_object_from_longest (obj->number).release ();
}

/* Implementation of RecordGap.error_code [int].  */

static PyObject *
recpy_gap_reason_code (PyObject *self, void *closure)
{
  const recpy_gap_object * const obj = (const recpy_gap_object *) self;

  return gdb_py_object_from_longest (obj->reason_code).release ();
}

/* Implementation of RecordGap.error_string [str].  */

static PyObject *
recpy_gap_reason_string (PyObject *self, void *closure)
{
  const recpy_gap_object * const obj = (const recpy_gap_object *) self;

  return PyUnicode_FromString (obj->reason_string);
}

/* Record method list.  */

static PyMethodDef recpy_record_methods[] = {
  { "goto", recpy_goto, METH_VARARGS,
    "goto (instruction|function_call) -> None.\n\
Rewind to given location."},
  { NULL }
};

/* Record member list.  */

static gdb_PyGetSetDef recpy_record_getset[] = {
  { "method", recpy_method, NULL, "Current recording method.", NULL },
  { "format", recpy_format, NULL, "Current recording format.", NULL },
  { "replay_position", recpy_replay_position, NULL, "Current replay position.",
    NULL },
  { "instruction_history", recpy_instruction_history, NULL,
    "List of instructions in current recording.", NULL },
  { "function_call_history", recpy_function_call_history, NULL,
    "List of function calls in current recording.", NULL },
  { "begin", recpy_begin, NULL,
    "First instruction in current recording.", NULL },
  { "end", recpy_end, NULL,
    "One past the last instruction in current recording.  This is typically \
the current instruction and is used for e.g. record.goto (record.end).", NULL },
  { NULL }
};

/* RecordInstruction member list.  */

static gdb_PyGetSetDef recpy_insn_getset[] = {
  { "number", recpy_element_number, NULL, "instruction number", NULL},
  { "sal", recpy_insn_sal, NULL, "associated symbol and line", NULL},
  { "pc", recpy_insn_pc, NULL, "instruction address", NULL},
  { "data", recpy_insn_data, NULL, "raw instruction data", NULL},
  { "decoded", recpy_insn_decoded, NULL, "decoded instruction", NULL},
  { "size", recpy_insn_size, NULL, "instruction size in byte", NULL},
  { "is_speculative", recpy_insn_is_speculative, NULL, "if the instruction was \
  executed speculatively", NULL},
  { NULL }
};

/* RecordFunctionSegment member list.  */

static gdb_PyGetSetDef recpy_func_getset[] = {
  { "number", recpy_element_number, NULL, "function segment number", NULL},
  { "level", recpy_func_level, NULL, "call stack level", NULL},
  { "symbol", recpy_func_symbol, NULL, "associated line and symbol", NULL},
  { "instructions", recpy_func_instructions, NULL, "list of instructions in \
this function segment", NULL},
  { "up", recpy_func_up, NULL, "caller or returned-to function segment", NULL},
  { "prev", recpy_func_prev, NULL, "previous segment of this function", NULL},
  { "next", recpy_func_next, NULL, "next segment of this function", NULL},
  { NULL }
};

/* RecordGap member list.  */

static gdb_PyGetSetDef recpy_gap_getset[] = {
  { "number", recpy_gap_number, NULL, "element number", NULL},
  { "reason_code", recpy_gap_reason_code, NULL, "reason code", NULL},
  { "reason_string", recpy_gap_reason_string, NULL, "reason string", NULL},
  { NULL }
};

/* Sets up the record API in the gdb module.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_record (void)
{
  recpy_record_type.tp_new = PyType_GenericNew;
  recpy_record_type.tp_flags = Py_TPFLAGS_DEFAULT;
  recpy_record_type.tp_basicsize = sizeof (recpy_record_object);
  recpy_record_type.tp_name = "gdb.Record";
  recpy_record_type.tp_doc = "GDB record object";
  recpy_record_type.tp_methods = recpy_record_methods;
  recpy_record_type.tp_getset = recpy_record_getset;

  recpy_insn_type.tp_new = PyType_GenericNew;
  recpy_insn_type.tp_flags = Py_TPFLAGS_DEFAULT;
  recpy_insn_type.tp_basicsize = sizeof (recpy_element_object);
  recpy_insn_type.tp_name = "gdb.RecordInstruction";
  recpy_insn_type.tp_doc = "GDB recorded instruction object";
  recpy_insn_type.tp_getset = recpy_insn_getset;
  recpy_insn_type.tp_richcompare = recpy_element_richcompare;
  recpy_insn_type.tp_hash = recpy_element_hash;
  recpy_insn_type.tp_base = py_insn_get_insn_type ();

  recpy_func_type.tp_new = PyType_GenericNew;
  recpy_func_type.tp_flags = Py_TPFLAGS_DEFAULT;
  recpy_func_type.tp_basicsize = sizeof (recpy_element_object);
  recpy_func_type.tp_name = "gdb.RecordFunctionSegment";
  recpy_func_type.tp_doc = "GDB record function segment object";
  recpy_func_type.tp_getset = recpy_func_getset;
  recpy_func_type.tp_richcompare = recpy_element_richcompare;
  recpy_func_type.tp_hash = recpy_element_hash;

  recpy_gap_type.tp_new = PyType_GenericNew;
  recpy_gap_type.tp_flags = Py_TPFLAGS_DEFAULT;
  recpy_gap_type.tp_basicsize = sizeof (recpy_gap_object);
  recpy_gap_type.tp_name = "gdb.RecordGap";
  recpy_gap_type.tp_doc = "GDB recorded gap object";
  recpy_gap_type.tp_getset = recpy_gap_getset;

  if (PyType_Ready (&recpy_record_type) < 0
      || PyType_Ready (&recpy_insn_type) < 0
      || PyType_Ready (&recpy_func_type) < 0
      || PyType_Ready (&recpy_gap_type) < 0)
    return -1;
  else
    return 0;
}

/* Implementation of gdb.start_recording (method) -> gdb.Record.  */

PyObject *
gdbpy_start_recording (PyObject *self, PyObject *args)
{
  const char *method = NULL;
  const char *format = NULL;
  PyObject *ret = NULL;

  if (!PyArg_ParseTuple (args, "|ss", &method, &format))
    return NULL;

  try
    {
      record_start (method, format, 0);
      ret = gdbpy_current_recording (self, args);
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
    }

  return ret;
}

/* Implementation of gdb.current_recording (self) -> gdb.Record.  */

PyObject *
gdbpy_current_recording (PyObject *self, PyObject *args)
{
  recpy_record_object *ret = NULL;

  if (find_record_target () == NULL)
    Py_RETURN_NONE;

  ret = PyObject_New (recpy_record_object, &recpy_record_type);
  ret->thread = inferior_thread ();
  ret->method = target_record_method (ret->thread->ptid);

  return (PyObject *) ret;
}

/* Implementation of gdb.stop_recording (self) -> None.  */

PyObject *
gdbpy_stop_recording (PyObject *self, PyObject *args)
{
  try
    {
      record_stop (0);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_record);

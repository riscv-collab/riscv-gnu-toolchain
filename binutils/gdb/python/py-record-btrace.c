/* Python interface to btrace instruction history.

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
#include "gdbcore.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "btrace.h"
#include "py-record.h"
#include "py-record-btrace.h"
#include "record-btrace.h"
#include "disasm.h"
#include "gdbarch.h"

/* Python object for btrace record lists.  */

struct btpy_list_object {
  PyObject_HEAD

  /* The thread this list belongs to.  */
  thread_info *thread;

  /* The first index being part of this list.  */
  Py_ssize_t first;

  /* The last index begin part of this list.  */
  Py_ssize_t last;

  /* Stride size.  */
  Py_ssize_t step;

  /* Either &BTPY_CALL_TYPE or &RECPY_INSN_TYPE.  */
  PyTypeObject* element_type;
};

/* Python type for btrace lists.  */

static PyTypeObject btpy_list_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
};

/* Returns either a btrace_insn for the given Python gdb.RecordInstruction
   object or sets an appropriate Python exception and returns NULL.  */

static const btrace_insn *
btrace_insn_from_recpy_insn (const PyObject * const pyobject)
{
  const btrace_insn *insn;
  const recpy_element_object *obj;
  thread_info *tinfo;
  btrace_insn_iterator iter;

  if (Py_TYPE (pyobject) != &recpy_insn_type)
    {
      PyErr_Format (gdbpy_gdb_error, _("Must be gdb.RecordInstruction"));
      return NULL;
    }

  obj = (const recpy_element_object *) pyobject;
  tinfo = obj->thread;

  if (tinfo == NULL || btrace_is_empty (tinfo))
    {
      PyErr_Format (gdbpy_gdb_error, _("No such instruction."));
      return NULL;
    }

  if (btrace_find_insn_by_number (&iter, &tinfo->btrace, obj->number) == 0)
    {
      PyErr_Format (gdbpy_gdb_error, _("No such instruction."));
      return NULL;
    }

  insn = btrace_insn_get (&iter);
  if (insn == NULL)
    {
      PyErr_Format (gdbpy_gdb_error, _("Not a valid instruction."));
      return NULL;
    }

  return insn;
}

/* Returns either a btrace_function for the given Python
   gdb.RecordFunctionSegment object or sets an appropriate Python exception and
   returns NULL.  */

static const btrace_function *
btrace_func_from_recpy_func (const PyObject * const pyobject)
{
  const btrace_function *func;
  const recpy_element_object *obj;
  thread_info *tinfo;
  btrace_call_iterator iter;

  if (Py_TYPE (pyobject) != &recpy_func_type)
    {
      PyErr_Format (gdbpy_gdb_error, _("Must be gdb.RecordFunctionSegment"));
      return NULL;
    }

  obj = (const recpy_element_object *) pyobject;
  tinfo = obj->thread;

  if (tinfo == NULL || btrace_is_empty (tinfo))
    {
      PyErr_Format (gdbpy_gdb_error, _("No such function segment."));
      return NULL;
    }

  if (btrace_find_call_by_number (&iter, &tinfo->btrace, obj->number) == 0)
    {
      PyErr_Format (gdbpy_gdb_error, _("No such function segment."));
      return NULL;
    }

  func = btrace_call_get (&iter);
  if (func == NULL)
    {
      PyErr_Format (gdbpy_gdb_error, _("Not a valid function segment."));
      return NULL;
    }

  return func;
}

/* Looks at the recorded item with the number NUMBER and create a
   gdb.RecordInstruction or gdb.RecordGap object for it accordingly.  */

static PyObject *
btpy_insn_or_gap_new (thread_info *tinfo, Py_ssize_t number)
{
  btrace_insn_iterator iter;
  int err_code;

  btrace_find_insn_by_number (&iter, &tinfo->btrace, number);
  err_code = btrace_insn_get_error (&iter);

  if (err_code != 0)
    {
      const btrace_config *config;
      const char *err_string;

      config = btrace_conf (&tinfo->btrace);
      err_string = btrace_decode_error (config->format, err_code);

      return recpy_gap_new (err_code, err_string, number);
    }

  return recpy_insn_new (tinfo, RECORD_METHOD_BTRACE, number);
}

/* Create a new gdb.BtraceList object.  */

static PyObject *
btpy_list_new (thread_info *thread, Py_ssize_t first, Py_ssize_t last,
	       Py_ssize_t step, PyTypeObject *element_type)
{
  btpy_list_object * const obj = PyObject_New (btpy_list_object,
					       &btpy_list_type);

  if (obj == NULL)
    return NULL;

  obj->thread = thread;
  obj->first = first;
  obj->last = last;
  obj->step = step;
  obj->element_type = element_type;

  return (PyObject *) obj;
}

/* Implementation of RecordInstruction.sal [gdb.Symtab_and_line] for btrace.
   Returns the SAL associated with this instruction.  */

PyObject *
recpy_bt_insn_sal (PyObject *self, void *closure)
{
  const btrace_insn * const insn = btrace_insn_from_recpy_insn (self);
  PyObject *result = NULL;

  if (insn == NULL)
    return NULL;

  try
    {
      result = symtab_and_line_to_sal_object (find_pc_line (insn->pc, 0));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Implementation of RecordInstruction.pc [int] for btrace.
   Returns the instruction address.  */

PyObject *
recpy_bt_insn_pc (PyObject *self, void *closure)
{
  const btrace_insn * const insn = btrace_insn_from_recpy_insn (self);

  if (insn == NULL)
    return NULL;

  return gdb_py_object_from_ulongest (insn->pc).release ();
}

/* Implementation of RecordInstruction.size [int] for btrace.
   Returns the instruction size.  */

PyObject *
recpy_bt_insn_size (PyObject *self, void *closure)
{
  const btrace_insn * const insn = btrace_insn_from_recpy_insn (self);

  if (insn == NULL)
    return NULL;

  return gdb_py_object_from_longest (insn->size).release ();
}

/* Implementation of RecordInstruction.is_speculative [bool] for btrace.
   Returns if this instruction was executed speculatively.  */

PyObject *
recpy_bt_insn_is_speculative (PyObject *self, void *closure)
{
  const btrace_insn * const insn = btrace_insn_from_recpy_insn (self);

  if (insn == NULL)
    return NULL;

  if (insn->flags & BTRACE_INSN_FLAG_SPECULATIVE)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

/* Implementation of RecordInstruction.data [buffer] for btrace.
   Returns raw instruction data.  */

PyObject *
recpy_bt_insn_data (PyObject *self, void *closure)
{
  const btrace_insn * const insn = btrace_insn_from_recpy_insn (self);
  gdb::byte_vector buffer;
  PyObject *object;

  if (insn == NULL)
    return NULL;

  try
    {
      buffer.resize (insn->size);
      read_memory (insn->pc, buffer.data (), insn->size);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  object = PyBytes_FromStringAndSize ((const char *) buffer.data (),
				      insn->size);

  if (object == NULL)
    return NULL;

  return PyMemoryView_FromObject (object);
}

/* Implementation of RecordInstruction.decoded [str] for btrace.
   Returns the instruction as human readable string.  */

PyObject *
recpy_bt_insn_decoded (PyObject *self, void *closure)
{
  const btrace_insn * const insn = btrace_insn_from_recpy_insn (self);
  string_file strfile;

  if (insn == NULL)
    return NULL;

  try
    {
      gdb_print_insn (current_inferior ()->arch (), insn->pc, &strfile, NULL);
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return NULL;
    }

  return PyBytes_FromString (strfile.string ().c_str ());
}

/* Implementation of RecordFunctionSegment.level [int] for btrace.
   Returns the call level.  */

PyObject *
recpy_bt_func_level (PyObject *self, void *closure)
{
  const btrace_function * const func = btrace_func_from_recpy_func (self);
  thread_info *tinfo;

  if (func == NULL)
    return NULL;

  tinfo = ((recpy_element_object *) self)->thread;
  return gdb_py_object_from_longest (tinfo->btrace.level
				     + func->level).release ();
}

/* Implementation of RecordFunctionSegment.symbol [gdb.Symbol] for btrace.
   Returns the symbol associated with this function call.  */

PyObject *
recpy_bt_func_symbol (PyObject *self, void *closure)
{
  const btrace_function * const func = btrace_func_from_recpy_func (self);

  if (func == NULL)
    return NULL;

  if (func->sym == NULL)
    Py_RETURN_NONE;

  return symbol_to_symbol_object (func->sym);
}

/* Implementation of RecordFunctionSegment.instructions [list] for btrace.
   Returns the list of instructions that belong to this function call.  */

PyObject *
recpy_bt_func_instructions (PyObject *self, void *closure)
{
  const btrace_function * const func = btrace_func_from_recpy_func (self);
  unsigned int len;

  if (func == NULL)
    return NULL;

  len = func->insn.size ();

  /* Gaps count as one instruction.  */
  if (len == 0)
    len = 1;

  return btpy_list_new (((recpy_element_object *) self)->thread,
			func->insn_offset, func->insn_offset + len, 1,
			&recpy_insn_type);
}

/* Implementation of RecordFunctionSegment.up [RecordFunctionSegment] for
   btrace.  Returns the caller / returnee of this function.  */

PyObject *
recpy_bt_func_up (PyObject *self, void *closure)
{
  const btrace_function * const func = btrace_func_from_recpy_func (self);

  if (func == NULL)
    return NULL;

  if (func->up == 0)
    Py_RETURN_NONE;

  return recpy_func_new (((recpy_element_object *) self)->thread,
			 RECORD_METHOD_BTRACE, func->up);
}

/* Implementation of RecordFunctionSegment.prev [RecordFunctionSegment] for
   btrace.  Returns a previous segment of this function.  */

PyObject *
recpy_bt_func_prev (PyObject *self, void *closure)
{
  const btrace_function * const func = btrace_func_from_recpy_func (self);

  if (func == NULL)
    return NULL;

  if (func->prev == 0)
    Py_RETURN_NONE;

  return recpy_func_new (((recpy_element_object *) self)->thread,
			 RECORD_METHOD_BTRACE, func->prev);
}

/* Implementation of RecordFunctionSegment.next [RecordFunctionSegment] for
   btrace.  Returns a following segment of this function.  */

PyObject *
recpy_bt_func_next (PyObject *self, void *closure)
{
  const btrace_function * const func = btrace_func_from_recpy_func (self);

  if (func == NULL)
    return NULL;

  if (func->next == 0)
    Py_RETURN_NONE;

  return recpy_func_new (((recpy_element_object *) self)->thread,
			 RECORD_METHOD_BTRACE, func->next);
}

/* Implementation of BtraceList.__len__ (self) -> int.  */

static Py_ssize_t
btpy_list_length (PyObject *self)
{
  const btpy_list_object * const obj = (btpy_list_object *) self;
  const Py_ssize_t distance = obj->last - obj->first;
  const Py_ssize_t result = distance / obj->step;

  if ((distance % obj->step) == 0)
    return result;

  return result + 1;
}

/* Implementation of
   BtraceList.__getitem__ (self, key) -> BtraceInstruction and
   BtraceList.__getitem__ (self, key) -> BtraceFunctionCall.  */

static PyObject *
btpy_list_item (PyObject *self, Py_ssize_t index)
{
  const btpy_list_object * const obj = (btpy_list_object *) self;
  Py_ssize_t number;

  if (index < 0 || index >= btpy_list_length (self))
    return PyErr_Format (PyExc_IndexError, _("Index out of range: %zd."),
			 index);

  number = obj->first + (obj->step * index);

  if (obj->element_type == &recpy_insn_type)
    return recpy_insn_new (obj->thread, RECORD_METHOD_BTRACE, number);
  else
    return recpy_func_new (obj->thread, RECORD_METHOD_BTRACE, number);
}

/* Implementation of BtraceList.__getitem__ (self, slice) -> BtraceList.  */

static PyObject *
btpy_list_slice (PyObject *self, PyObject *value)
{
  const btpy_list_object * const obj = (btpy_list_object *) self;
  const Py_ssize_t length = btpy_list_length (self);
  Py_ssize_t start, stop, step, slicelength;

  if (PyLong_Check (value))
    {
      Py_ssize_t index = PyLong_AsSsize_t (value);

      /* Emulate Python behavior for negative indices.  */
      if (index < 0)
	index += length;

      return btpy_list_item (self, index);
    }

  if (!PySlice_Check (value))
    return PyErr_Format (PyExc_TypeError, _("Index must be int or slice."));

  if (0 != PySlice_GetIndicesEx (value, length, &start, &stop,
				 &step, &slicelength))
    return NULL;

  return btpy_list_new (obj->thread, obj->first + obj->step * start,
			obj->first + obj->step * stop, obj->step * step,
			obj->element_type);
}

/* Helper function that returns the position of an element in a BtraceList
   or -1 if the element is not in the list.  */

static LONGEST
btpy_list_position (PyObject *self, PyObject *value)
{
  const btpy_list_object * const list_obj = (btpy_list_object *) self;
  const recpy_element_object * const obj = (const recpy_element_object *) value;
  Py_ssize_t index = obj->number;

  if (list_obj->element_type != Py_TYPE (value))
    return -1;

  if (list_obj->thread != obj->thread)
    return -1;

  if (index < list_obj->first || index > list_obj->last)
    return -1;

  index -= list_obj->first;

  if (index % list_obj->step != 0)
    return -1;

  return index / list_obj->step;
}

/* Implementation of "in" operator for BtraceLists.  */

static int
btpy_list_contains (PyObject *self, PyObject *value)
{
  if (btpy_list_position (self, value) < 0)
    return 0;

  return 1;
}

/* Implementation of BtraceLists.index (self, value) -> int.  */

static PyObject *
btpy_list_index (PyObject *self, PyObject *value)
{
  const LONGEST index = btpy_list_position (self, value);

  if (index < 0)
    return PyErr_Format (PyExc_ValueError, _("Not in list."));

  return gdb_py_object_from_longest (index).release ();
}

/* Implementation of BtraceList.count (self, value) -> int.  */

static PyObject *
btpy_list_count (PyObject *self, PyObject *value)
{
  /* We know that if an element is in the list, it is so exactly one time,
     enabling us to reuse the "is element of" check.  */
  return gdb_py_object_from_longest (btpy_list_contains (self,
							 value)).release ();
}

/* Python rich compare function to allow for equality and inequality checks
   in Python.  */

static PyObject *
btpy_list_richcompare (PyObject *self, PyObject *other, int op)
{
  const btpy_list_object * const obj1 = (btpy_list_object *) self;
  const btpy_list_object * const obj2 = (btpy_list_object *) other;

  if (Py_TYPE (self) != Py_TYPE (other))
    {
      Py_INCREF (Py_NotImplemented);
      return Py_NotImplemented;
    }

  switch (op)
  {
    case Py_EQ:
      if (obj1->thread == obj2->thread
	  && obj1->element_type == obj2->element_type
	  && obj1->first == obj2->first
	  && obj1->last == obj2->last
	  && obj1->step == obj2->step)
	Py_RETURN_TRUE;
      else
	Py_RETURN_FALSE;

    case Py_NE:
      if (obj1->thread != obj2->thread
	  || obj1->element_type != obj2->element_type
	  || obj1->first != obj2->first
	  || obj1->last != obj2->last
	  || obj1->step != obj2->step)
	Py_RETURN_TRUE;
      else
	Py_RETURN_FALSE;

    default:
      break;
  }

  Py_INCREF (Py_NotImplemented);
  return Py_NotImplemented;
}

/* Implementation of
   BtraceRecord.method [str].  */

PyObject *
recpy_bt_method (PyObject *self, void *closure)
{
  return PyUnicode_FromString ("btrace");
}

/* Implementation of
   BtraceRecord.format [str].  */

PyObject *
recpy_bt_format (PyObject *self, void *closure)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  const struct thread_info * const tinfo = record->thread;
  const struct btrace_config * config;

  if (tinfo == NULL)
    Py_RETURN_NONE;

  config = btrace_conf (&tinfo->btrace);

  if (config == NULL)
    Py_RETURN_NONE;

  return PyUnicode_FromString (btrace_format_short_string (config->format));
}

/* Implementation of
   BtraceRecord.replay_position [BtraceInstruction].  */

PyObject *
recpy_bt_replay_position (PyObject *self, void *closure)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  thread_info * tinfo = record->thread;

  if (tinfo == NULL)
    Py_RETURN_NONE;

  if (tinfo->btrace.replay == NULL)
    Py_RETURN_NONE;

  return btpy_insn_or_gap_new (tinfo,
			       btrace_insn_number (tinfo->btrace.replay));
}

/* Implementation of
   BtraceRecord.begin [BtraceInstruction].  */

PyObject *
recpy_bt_begin (PyObject *self, void *closure)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  thread_info *const tinfo = record->thread;
  struct btrace_insn_iterator iterator;

  if (tinfo == NULL)
    Py_RETURN_NONE;

  btrace_fetch (tinfo, record_btrace_get_cpu ());

  if (btrace_is_empty (tinfo))
    Py_RETURN_NONE;

  btrace_insn_begin (&iterator, &tinfo->btrace);
  return btpy_insn_or_gap_new (tinfo, btrace_insn_number (&iterator));
}

/* Implementation of
   BtraceRecord.end [BtraceInstruction].  */

PyObject *
recpy_bt_end (PyObject *self, void *closure)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  thread_info *const tinfo = record->thread;
  struct btrace_insn_iterator iterator;

  if (tinfo == NULL)
    Py_RETURN_NONE;

  btrace_fetch (tinfo, record_btrace_get_cpu ());

  if (btrace_is_empty (tinfo))
    Py_RETURN_NONE;

  btrace_insn_end (&iterator, &tinfo->btrace);
  return btpy_insn_or_gap_new (tinfo, btrace_insn_number (&iterator));
}

/* Implementation of
   BtraceRecord.instruction_history [list].  */

PyObject *
recpy_bt_instruction_history (PyObject *self, void *closure)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  thread_info *const tinfo = record->thread;
  struct btrace_insn_iterator iterator;
  unsigned long first = 0;
  unsigned long last = 0;

   if (tinfo == NULL)
     Py_RETURN_NONE;

   btrace_fetch (tinfo, record_btrace_get_cpu ());

   if (btrace_is_empty (tinfo))
     Py_RETURN_NONE;

   btrace_insn_begin (&iterator, &tinfo->btrace);
   first = btrace_insn_number (&iterator);

   btrace_insn_end (&iterator, &tinfo->btrace);
   last = btrace_insn_number (&iterator);

   return btpy_list_new (tinfo, first, last, 1, &recpy_insn_type);
}

/* Implementation of
   BtraceRecord.function_call_history [list].  */

PyObject *
recpy_bt_function_call_history (PyObject *self, void *closure)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  thread_info *const tinfo = record->thread;
  struct btrace_call_iterator iterator;
  unsigned long first = 0;
  unsigned long last = 0;

  if (tinfo == NULL)
    Py_RETURN_NONE;

  btrace_fetch (tinfo, record_btrace_get_cpu ());

  if (btrace_is_empty (tinfo))
    Py_RETURN_NONE;

  btrace_call_begin (&iterator, &tinfo->btrace);
  first = btrace_call_number (&iterator);

  btrace_call_end (&iterator, &tinfo->btrace);
  last = btrace_call_number (&iterator);

  return btpy_list_new (tinfo, first, last, 1, &recpy_func_type);
}

/* Implementation of BtraceRecord.goto (self, BtraceInstruction) -> None.  */

PyObject *
recpy_bt_goto (PyObject *self, PyObject *args)
{
  const recpy_record_object * const record = (recpy_record_object *) self;
  thread_info *const tinfo = record->thread;
  const recpy_element_object *obj;
  PyObject *parse_obj;

  if (tinfo == NULL || btrace_is_empty (tinfo))
	return PyErr_Format (gdbpy_gdb_error, _("Empty branch trace."));

  if (!PyArg_ParseTuple (args, "O", &parse_obj))
    return NULL;

  if (Py_TYPE (parse_obj) != &recpy_insn_type)
    return PyErr_Format (PyExc_TypeError, _("Argument must be instruction."));
  obj = (const recpy_element_object *) parse_obj;

  try
    {
      struct btrace_insn_iterator iter;

      btrace_insn_end (&iter, &tinfo->btrace);

      if (btrace_insn_number (&iter) == obj->number)
	target_goto_record_end ();
      else
	target_goto_record (obj->number);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* BtraceList methods.  */

static PyMethodDef btpy_list_methods[] =
{
  { "count", btpy_list_count, METH_O, "count number of occurrences"},
  { "index", btpy_list_index, METH_O, "index of entry"},
  {NULL}
};

/* BtraceList sequence methods.  */

static PySequenceMethods btpy_list_sequence_methods =
{
  NULL
};

/* BtraceList mapping methods.  Necessary for slicing.  */

static PyMappingMethods btpy_list_mapping_methods =
{
  NULL
};

/* Sets up the btrace record API.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_btrace (void)
{
  btpy_list_type.tp_new = PyType_GenericNew;
  btpy_list_type.tp_flags = Py_TPFLAGS_DEFAULT;
  btpy_list_type.tp_basicsize = sizeof (btpy_list_object);
  btpy_list_type.tp_name = "gdb.BtraceObjectList";
  btpy_list_type.tp_doc = "GDB btrace list object";
  btpy_list_type.tp_methods = btpy_list_methods;
  btpy_list_type.tp_as_sequence = &btpy_list_sequence_methods;
  btpy_list_type.tp_as_mapping = &btpy_list_mapping_methods;
  btpy_list_type.tp_richcompare = btpy_list_richcompare;

  btpy_list_sequence_methods.sq_item = btpy_list_item;
  btpy_list_sequence_methods.sq_length = btpy_list_length;
  btpy_list_sequence_methods.sq_contains = btpy_list_contains;

  btpy_list_mapping_methods.mp_subscript = btpy_list_slice;

  return PyType_Ready (&btpy_list_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_btrace);

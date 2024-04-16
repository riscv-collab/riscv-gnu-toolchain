/* Python interface to instruction objects.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

/* Python type object for the abstract gdb.Instruction class.  This class
   contains getters for four elements: "pc" (int), "data" (buffer), "decode"
   (str) and "size" (int) that must be overridden by sub classes.  */

PyTypeObject py_insn_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
};

/* Python instruction object.  */

struct py_insn_obj {
  PyObject_HEAD
};

/* Getter function for gdb.Instruction attributes.  */

static PyObject *
py_insn_getter (PyObject *self, void *closure)
{
  return PyErr_Format (PyExc_NotImplementedError, _("Not implemented."));
}

/* Instruction members.  */

static gdb_PyGetSetDef py_insn_getset[] =
{
  { "pc", py_insn_getter, NULL, "instruction address", NULL},
  { "data", py_insn_getter, NULL, "instruction memory", NULL},
  { "decoded", py_insn_getter, NULL, "decoded instruction", NULL},
  { "size", py_insn_getter, NULL, "instruction size in bytes", NULL},
  {NULL}
};

/* See py-instruction.h.  */

PyTypeObject *
py_insn_get_insn_type ()
{
  if (py_insn_type.tp_new == nullptr)
    {
      py_insn_type.tp_new = PyType_GenericNew;
      py_insn_type.tp_flags = Py_TPFLAGS_DEFAULT;
      py_insn_type.tp_basicsize = sizeof (py_insn_obj);
      py_insn_type.tp_name = "gdb.Instruction";
      py_insn_type.tp_doc = "GDB instruction object";
      py_insn_type.tp_getset = py_insn_getset;

      if (PyType_Ready (&py_insn_type) < 0)
	{
	  /* Reset the tp_new field so any subsequent calls to this
	     function will retry to make the type ready.  */
	  py_insn_type.tp_new = nullptr;
	  return nullptr;
	}
    }

  return &py_insn_type;
}

/* Sets up the gdb.Instruction type.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_instruction (void)
{
  if (py_insn_get_insn_type () == nullptr)
    return -1;
  return 0;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_instruction);

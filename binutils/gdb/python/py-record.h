/* Python interface to record targets.

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

#ifndef PYTHON_PY_RECORD_H
#define PYTHON_PY_RECORD_H

#include "inferior.h"
#include "python-internal.h"
#include "record.h"

/* Python Record object.  */
struct recpy_record_object
{
  PyObject_HEAD

  /* The thread this object refers to.  */
  thread_info *thread;

  /* The current recording method.  */
  enum record_method method;
};

/* Python recorded element object.  This is generic enough to represent
   recorded instructions as well as recorded function call segments, hence the
   generic name.  */
struct recpy_element_object
{
  PyObject_HEAD

  /* The thread this object refers to.  */
  thread_info *thread;

  /* The current recording method.  */
  enum record_method method;

  /* Element number.  */
  Py_ssize_t number;
};

/* Python RecordInstruction type.  */
extern PyTypeObject recpy_insn_type;

/* Python RecordFunctionSegment type.  */
extern PyTypeObject recpy_func_type;

/* Create a new gdb.RecordInstruction object.  */
extern PyObject *recpy_insn_new (thread_info *thread, enum record_method method,
				 Py_ssize_t number);

/* Create a new gdb.RecordFunctionSegment object.  */
extern PyObject *recpy_func_new (thread_info *thread, enum record_method method,
				 Py_ssize_t number);

/* Create a new gdb.RecordGap object.  */
extern PyObject *recpy_gap_new (int reason_code, const char *reason_string,
				Py_ssize_t number);

#endif /* PYTHON_PY_RECORD_H */

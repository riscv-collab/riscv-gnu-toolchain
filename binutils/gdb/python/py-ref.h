/* Python reference-holding class

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef PYTHON_PY_REF_H
#define PYTHON_PY_REF_H

#include "gdbsupport/gdb_ref_ptr.h"

/* A policy class for gdb::ref_ptr for Python reference counting.  */
template<typename T>
struct gdbpy_ref_policy
{
  static void incref (T *ptr)
  {
    Py_INCREF (ptr);
  }

  static void decref (T *ptr)
  {
    Py_DECREF (ptr);
  }
};

/* A gdb::ref_ptr that has been specialized for Python objects or
   their "subclasses".  */
template<typename T = PyObject> using gdbpy_ref
  = gdb::ref_ptr<T, gdbpy_ref_policy<T>>;

#endif /* PYTHON_PY_REF_H */

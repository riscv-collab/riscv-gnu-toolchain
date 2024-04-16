/* Python interface to full record targets.

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

#ifndef PYTHON_PY_RECORD_FULL_H
#define PYTHON_PY_RECORD_FULL_H

#include "python-internal.h"

/* Implementation of record.method [str].  */
extern PyObject *recpy_full_method (PyObject *self, void *value);

/* Implementation of record.format [str].  */
extern PyObject *recpy_full_format (PyObject *self, void *value);

#endif /* PYTHON_PY_RECORD_FULL_H */

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

#ifndef PYTHON_PY_INSTRUCTION_H
#define PYTHON_PY_INSTRUCTION_H

#include "python-internal.h"

/* Return a pointer to the py_insn_type object (see py-instruction.c), but
   ensure that PyType_Ready has been called for the type first.  If the
   PyType_Ready call is successful then subsequent calls to this function
   will not call PyType_Ready, the type pointer will just be returned.

   If the PyType_Ready call is not successful then nullptr is returned and
   subsequent calls to this function will call PyType_Ready again.  */

extern PyTypeObject *py_insn_get_insn_type ();

#endif /* PYTHON_PY_INSTRUCTION_H */

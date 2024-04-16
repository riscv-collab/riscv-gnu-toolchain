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

#ifndef PYTHON_PY_STOPEVENT_H
#define PYTHON_PY_STOPEVENT_H

#include "py-event.h"

extern gdbpy_ref<> create_stop_event_object (PyTypeObject *py_type,
					     const gdbpy_ref<> &dict);

extern int emit_stop_event (struct bpstat *bs,
			    enum gdb_signal stop_signal);

extern gdbpy_ref<> create_breakpoint_event_object (const gdbpy_ref<> &dict,
						   PyObject *breakpoint_list,
						   PyObject *first_bp);

extern gdbpy_ref<> create_signal_event_object (const gdbpy_ref<> &dict,
					       enum gdb_signal stop_signal);

#endif /* PYTHON_PY_STOPEVENT_H */

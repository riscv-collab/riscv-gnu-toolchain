/* Python interface to inferior signal stop events.

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

gdbpy_ref<>
create_signal_event_object (const gdbpy_ref<> &dict,
			    enum gdb_signal stop_signal)
{
  gdbpy_ref<> signal_event_obj
    = create_stop_event_object (&signal_event_object_type, dict);

  if (signal_event_obj == NULL)
    return NULL;

  const char *signal_name = gdb_signal_to_name (stop_signal);

  gdbpy_ref<> signal_name_obj (PyUnicode_FromString (signal_name));
  if (signal_name_obj == NULL)
    return NULL;
  if (evpy_add_attribute (signal_event_obj.get (),
			  "stop_signal",
			  signal_name_obj.get ()) < 0)
    return NULL;

  return signal_event_obj;
}

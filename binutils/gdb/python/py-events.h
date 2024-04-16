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

#ifndef PYTHON_PY_EVENTS_H
#define PYTHON_PY_EVENTS_H

#include "command.h"
#include "python-internal.h"
#include "inferior.h"

/* Stores a list of objects to be notified when the event for which this
   registry tracks occurs.  */

struct eventregistry_object
{
  PyObject_HEAD

  PyObject *callbacks;
};

/* Struct holding references to event registries both in python and c.
   This is meant to be a singleton.  */

struct events_object
{
#define GDB_PY_DEFINE_EVENT(name)		\
  eventregistry_object *name;
#include "py-all-events.def"
#undef GDB_PY_DEFINE_EVENT

  PyObject *module;

};

/* Python events singleton.  */
extern events_object gdb_py_events;

extern eventregistry_object *create_eventregistry_object (void);
extern bool evregpy_no_listeners_p (eventregistry_object *registry);

#endif /* PYTHON_PY_EVENTS_H */

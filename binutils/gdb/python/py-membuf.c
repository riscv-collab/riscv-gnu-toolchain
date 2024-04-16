/* Python memory buffer interface for reading inferior memory.

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
#include "python-internal.h"

struct membuf_object {
  PyObject_HEAD

  /* Pointer to the raw data, and array of gdb_bytes.  */
  void *buffer;

  /* The address from where the data was read, held for mbpy_str.  */
  CORE_ADDR addr;

  /* The number of octets in BUFFER.  */
  CORE_ADDR length;
};

extern PyTypeObject membuf_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("membuf_object");

/* Wrap BUFFER, ADDRESS, and LENGTH into a gdb.Membuf object.  ADDRESS is
   the address within the inferior that the contents of BUFFER were read,
   and LENGTH is the number of octets in BUFFER.  */

PyObject *
gdbpy_buffer_to_membuf (gdb::unique_xmalloc_ptr<gdb_byte> buffer,
			CORE_ADDR address,
			ULONGEST length)
{
  gdbpy_ref<membuf_object> membuf_obj (PyObject_New (membuf_object,
						     &membuf_object_type));
  if (membuf_obj == nullptr)
    return nullptr;

  membuf_obj->buffer = buffer.release ();
  membuf_obj->addr = address;
  membuf_obj->length = length;

  return PyMemoryView_FromObject ((PyObject *) membuf_obj.get ());
}

/* Destructor for gdb.Membuf objects.  */

static void
mbpy_dealloc (PyObject *self)
{
  xfree (((membuf_object *) self)->buffer);
  Py_TYPE (self)->tp_free (self);
}

/* Return a description of the gdb.Membuf object.  */

static PyObject *
mbpy_str (PyObject *self)
{
  membuf_object *membuf_obj = (membuf_object *) self;

  return PyUnicode_FromFormat (_("Memory buffer for address %s, \
which is %s bytes long."),
			       paddress (gdbpy_enter::get_gdbarch (),
					 membuf_obj->addr),
			       pulongest (membuf_obj->length));
}

static int
get_buffer (PyObject *self, Py_buffer *buf, int flags)
{
  membuf_object *membuf_obj = (membuf_object *) self;
  int ret;

  ret = PyBuffer_FillInfo (buf, self, membuf_obj->buffer,
			   membuf_obj->length, 0,
			   PyBUF_CONTIG);

  /* Despite the documentation saying this field is a "const char *",
     in Python 3.4 at least, it's really a "char *".  */
  buf->format = (char *) "c";

  return ret;
}

/* General Python initialization callback.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_membuf (void)
{
  membuf_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&membuf_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Membuf",
				 (PyObject *) &membuf_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_membuf);



static PyBufferProcs buffer_procs =
{
  get_buffer
};

PyTypeObject membuf_object_type = {
  PyVarObject_HEAD_INIT (nullptr, 0)
  "gdb.Membuf",			  /*tp_name*/
  sizeof (membuf_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  mbpy_dealloc,			  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  mbpy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  &buffer_procs,		  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB memory buffer object", 	  /*tp_doc*/
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  0,				  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
};

/* Python interface to architecture

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
#include "gdbarch.h"
#include "arch-utils.h"
#include "disasm.h"
#include "python-internal.h"

struct arch_object {
  PyObject_HEAD
  struct gdbarch *gdbarch;
};

static const registry<gdbarch>::key<PyObject, gdb::noop_deleter<PyObject>>
     arch_object_data;

/* Require a valid Architecture.  */
#define ARCHPY_REQUIRE_VALID(arch_obj, arch)			\
  do {								\
    arch = arch_object_to_gdbarch (arch_obj);			\
    if (arch == NULL)						\
      {								\
	PyErr_SetString (PyExc_RuntimeError,			\
			 _("Architecture is invalid."));	\
	return NULL;						\
      }								\
  } while (0)

extern PyTypeObject arch_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("arch_object");

/* Associates an arch_object with GDBARCH as gdbarch_data via the gdbarch
   post init registration mechanism (gdbarch_data_register_post_init).  */

static PyObject *
arch_object_data_init (struct gdbarch *gdbarch)
{
  arch_object *arch_obj = PyObject_New (arch_object, &arch_object_type);

  if (arch_obj == NULL)
    return NULL;

  arch_obj->gdbarch = gdbarch;

  return (PyObject *) arch_obj;
}

/* Returns the struct gdbarch value corresponding to the given Python
   architecture object OBJ, which must be a gdb.Architecture object.  */

struct gdbarch *
arch_object_to_gdbarch (PyObject *obj)
{
  gdb_assert (gdbpy_is_architecture (obj));

  arch_object *py_arch = (arch_object *) obj;
  return py_arch->gdbarch;
}

/* See python-internal.h.  */

bool
gdbpy_is_architecture (PyObject *obj)
{
  return PyObject_TypeCheck (obj, &arch_object_type);
}

/* Returns the Python architecture object corresponding to GDBARCH.
   Returns a new reference to the arch_object associated as data with
   GDBARCH.  */

PyObject *
gdbarch_to_arch_object (struct gdbarch *gdbarch)
{
  PyObject *new_ref = arch_object_data.get (gdbarch);
  if (new_ref == nullptr)
    {
      new_ref = arch_object_data_init (gdbarch);
      arch_object_data.set (gdbarch, new_ref);
    }

  /* new_ref could be NULL if creation failed.  */
  Py_XINCREF (new_ref);

  return new_ref;
}

/* Implementation of gdb.Architecture.name (self) -> String.
   Returns the name of the architecture as a string value.  */

static PyObject *
archpy_name (PyObject *self, PyObject *args)
{
  struct gdbarch *gdbarch = NULL;
  const char *name;

  ARCHPY_REQUIRE_VALID (self, gdbarch);

  name = (gdbarch_bfd_arch_info (gdbarch))->printable_name;
  return PyUnicode_FromString (name);
}

/* Implementation of
   gdb.Architecture.disassemble (self, start_pc [, end_pc [,count]]) -> List.
   Returns a list of instructions in a memory address range.  Each instruction
   in the list is a Python dict object.
*/

static PyObject *
archpy_disassemble (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "start_pc", "end_pc", "count", NULL };
  CORE_ADDR start = 0, end = 0;
  CORE_ADDR pc;
  long count = 0, i;
  PyObject *start_obj = nullptr, *end_obj = nullptr, *count_obj = nullptr;
  struct gdbarch *gdbarch = NULL;

  ARCHPY_REQUIRE_VALID (self, gdbarch);

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O|OO",
					keywords, &start_obj, &end_obj,
					&count_obj))
    return NULL;

  if (get_addr_from_python (start_obj, &start) < 0)
    return nullptr;

  if (end_obj != nullptr)
    {
      if (get_addr_from_python (end_obj, &end) < 0)
	return nullptr;

      if (end < start)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("Argument 'end_pc' should be greater than or "
			     "equal to the argument 'start_pc'."));

	  return NULL;
	}
    }
  if (count_obj)
    {
      count = PyLong_AsLong (count_obj);
      if (PyErr_Occurred () || count < 0)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("Argument 'count' should be an non-negative "
			     "integer."));

	  return NULL;
	}
    }

  gdbpy_ref<> result_list (PyList_New (0));
  if (result_list == NULL)
    return NULL;

  for (pc = start, i = 0;
       /* All args are specified.  */
       (end_obj && count_obj && pc <= end && i < count)
       /* end_pc is specified, but no count.  */
       || (end_obj && count_obj == NULL && pc <= end)
       /* end_pc is not specified, but a count is.  */
       || (end_obj == NULL && count_obj && i < count)
       /* Both end_pc and count are not specified.  */
       || (end_obj == NULL && count_obj == NULL && pc == start);)
    {
      int insn_len = 0;
      gdbpy_ref<> insn_dict (PyDict_New ());

      if (insn_dict == NULL)
	return NULL;
      if (PyList_Append (result_list.get (), insn_dict.get ()))
	return NULL;  /* PyList_Append Sets the exception.  */

      string_file stb;

      try
	{
	  insn_len = gdb_print_insn (gdbarch, pc, &stb, NULL);
	}
      catch (const gdb_exception &except)
	{
	  gdbpy_convert_exception (except);
	  return NULL;
	}

      gdbpy_ref<> pc_obj = gdb_py_object_from_ulongest (pc);
      if (pc_obj == nullptr)
	return nullptr;

      gdbpy_ref<> asm_obj
	(PyUnicode_FromString (!stb.empty () ? stb.c_str () : "<unknown>"));
      if (asm_obj == nullptr)
	return nullptr;

      gdbpy_ref<> len_obj = gdb_py_object_from_longest (insn_len);
      if (len_obj == nullptr)
	return nullptr;

      if (PyDict_SetItemString (insn_dict.get (), "addr", pc_obj.get ())
	  || PyDict_SetItemString (insn_dict.get (), "asm", asm_obj.get ())
	  || PyDict_SetItemString (insn_dict.get (), "length", len_obj.get ()))
	return NULL;

      pc += insn_len;
      i++;
    }

  return result_list.release ();
}

/* Implementation of gdb.Architecture.registers (self, reggroup) -> Iterator.
   Returns an iterator over register descriptors for registers in GROUP
   within the architecture SELF.  */

static PyObject *
archpy_registers (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "reggroup", NULL };
  struct gdbarch *gdbarch = NULL;
  const char *group_name = NULL;

  /* Parse method arguments.  */
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "|s", keywords,
					&group_name))
    return NULL;

  /* Extract the gdbarch from the self object.  */
  ARCHPY_REQUIRE_VALID (self, gdbarch);

  return gdbpy_new_register_descriptor_iterator (gdbarch, group_name);
}

/* Implementation of gdb.Architecture.register_groups (self) -> Iterator.
   Returns an iterator that will give up all valid register groups in the
   architecture SELF.  */

static PyObject *
archpy_register_groups (PyObject *self, PyObject *args)
{
  struct gdbarch *gdbarch = NULL;

  /* Extract the gdbarch from the self object.  */
  ARCHPY_REQUIRE_VALID (self, gdbarch);
  return gdbpy_new_reggroup_iterator (gdbarch);
}

/* Implementation of gdb.integer_type.  */
static PyObject *
archpy_integer_type (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "size", "signed", NULL };
  int size;
  PyObject *is_signed_obj = nullptr;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "i|O", keywords,
					&size, &is_signed_obj))
    return nullptr;

  /* Assume signed by default.  */
  bool is_signed = (is_signed_obj == nullptr
		    || PyObject_IsTrue (is_signed_obj));

  struct gdbarch *gdbarch;
  ARCHPY_REQUIRE_VALID (self, gdbarch);

  const struct builtin_type *builtins = builtin_type (gdbarch);
  struct type *type = nullptr;
  switch (size)
    {
    case 0:
      type = builtins->builtin_int0;
      break;
    case 8:
      type = is_signed ? builtins->builtin_int8 : builtins->builtin_uint8;
      break;
    case 16:
      type = is_signed ? builtins->builtin_int16 : builtins->builtin_uint16;
      break;
    case 24:
      type = is_signed ? builtins->builtin_int24 : builtins->builtin_uint24;
      break;
    case 32:
      type = is_signed ? builtins->builtin_int32 : builtins->builtin_uint32;
      break;
    case 64:
      type = is_signed ? builtins->builtin_int64 : builtins->builtin_uint64;
      break;
    case 128:
      type = is_signed ? builtins->builtin_int128 : builtins->builtin_uint128;
      break;

    default:
      PyErr_SetString (PyExc_ValueError,
		       _("no integer type of that size is available"));
      return nullptr;
    }

  return type_to_type_object (type);
}

/* __repr__ implementation for gdb.Architecture.  */

static PyObject *
archpy_repr (PyObject *self)
{
  const auto gdbarch = arch_object_to_gdbarch (self);
  if (gdbarch == nullptr)
    return gdb_py_invalid_object_repr (self);

  auto arch_info = gdbarch_bfd_arch_info (gdbarch);
  return PyUnicode_FromFormat ("<%s arch_name=%s printable_name=%s>",
			       Py_TYPE (self)->tp_name, arch_info->arch_name,
			       arch_info->printable_name);
}

/* Implementation of gdb.architecture_names().  Return a list of all the
   BFD architecture names that GDB understands.  */

PyObject *
gdbpy_all_architecture_names (PyObject *self, PyObject *args)
{
  gdbpy_ref<> list (PyList_New (0));
  if (list == nullptr)
    return nullptr;

  std::vector<const char *> name_list = gdbarch_printable_names ();
  for (const char *name : name_list)
    {
      gdbpy_ref <> py_name (PyUnicode_FromString (name));
      if (py_name == nullptr)
	return nullptr;
      if (PyList_Append (list.get (), py_name.get ()) < 0)
	return nullptr;
    }

 return list.release ();
}

/* Initializes the Architecture class in the gdb module.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_arch (void)
{
  arch_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&arch_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Architecture",
				 (PyObject *) &arch_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_arch);



static PyMethodDef arch_object_methods [] = {
  { "name", archpy_name, METH_NOARGS,
    "name () -> String.\n\
Return the name of the architecture as a string value." },
  { "disassemble", (PyCFunction) archpy_disassemble,
    METH_VARARGS | METH_KEYWORDS,
    "disassemble (start_pc [, end_pc [, count]]) -> List.\n\
Return a list of at most COUNT disassembled instructions from START_PC to\n\
END_PC." },
  { "integer_type", (PyCFunction) archpy_integer_type,
    METH_VARARGS | METH_KEYWORDS,
    "integer_type (size [, signed]) -> type\n\
Return an integer Type corresponding to the given bitsize and signed-ness.\n\
If not specified, the type defaults to signed." },
  { "registers", (PyCFunction) archpy_registers,
    METH_VARARGS | METH_KEYWORDS,
    "registers ([ group-name ]) -> Iterator.\n\
Return an iterator of register descriptors for the registers in register\n\
group GROUP-NAME." },
  { "register_groups", archpy_register_groups,
    METH_NOARGS,
    "register_groups () -> Iterator.\n\
Return an iterator over all of the register groups in this architecture." },
  {NULL}  /* Sentinel */
};

PyTypeObject arch_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Architecture",                 /* tp_name */
  sizeof (arch_object),               /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  archpy_repr,                        /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash  */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  "GDB architecture object",          /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  arch_object_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
};

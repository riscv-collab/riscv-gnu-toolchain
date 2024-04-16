/* Python interface to stack frames

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "language.h"
#include "charset.h"
#include "block.h"
#include "frame.h"
#include "symtab.h"
#include "stack.h"
#include "value.h"
#include "python-internal.h"
#include "symfile.h"
#include "objfiles.h"

struct frame_object {
  PyObject_HEAD
  struct frame_id frame_id;
  struct gdbarch *gdbarch;

  /* Marks that the FRAME_ID member actually holds the ID of the frame next
     to this, and not this frames' ID itself.  This is a hack to permit Python
     frame objects which represent invalid frames (i.e., the last frame_info
     in a corrupt stack).  The problem arises from the fact that this code
     relies on FRAME_ID to uniquely identify a frame, which is not always true
     for the last "frame" in a corrupt stack (it can have a null ID, or the same
     ID as the  previous frame).  Whenever get_prev_frame returns NULL, we
     record the frame_id of the next frame and set FRAME_ID_IS_NEXT to 1.  */
  int frame_id_is_next;
};

/* Require a valid frame.  This must be called inside a TRY_CATCH, or
   another context in which a gdb exception is allowed.  */
#define FRAPY_REQUIRE_VALID(frame_obj, frame)		\
    do {						\
      frame = frame_object_to_frame_info (frame_obj);	\
      if (frame == NULL)				\
	error (_("Frame is invalid."));			\
    } while (0)

/* Returns the frame_info object corresponding to the given Python Frame
   object.  If the frame doesn't exist anymore (the frame id doesn't
   correspond to any frame in the inferior), returns NULL.  */

frame_info_ptr
frame_object_to_frame_info (PyObject *obj)
{
  frame_object *frame_obj = (frame_object *) obj;
  frame_info_ptr frame;

  frame = frame_find_by_id (frame_obj->frame_id);
  if (frame == NULL)
    return NULL;

  if (frame_obj->frame_id_is_next)
    frame = get_prev_frame (frame);

  return frame;
}

/* Called by the Python interpreter to obtain string representation
   of the object.  */

static PyObject *
frapy_str (PyObject *self)
{
  const frame_id &fid = ((frame_object *) self)->frame_id;
  return PyUnicode_FromString (fid.to_string ().c_str ());
}

/* Implement repr() for gdb.Frame.  */

static PyObject *
frapy_repr (PyObject *self)
{
  frame_object *frame_obj = (frame_object *) self;
  frame_info_ptr f_info = frame_find_by_id (frame_obj->frame_id);
  if (f_info == nullptr)
    return gdb_py_invalid_object_repr (self);

  const frame_id &fid = frame_obj->frame_id;
  return PyUnicode_FromFormat ("<%s level=%d frame-id=%s>",
			       Py_TYPE (self)->tp_name,
			       frame_relative_level (f_info),
			       fid.to_string ().c_str ());
}

/* Implementation of gdb.Frame.is_valid (self) -> Boolean.
   Returns True if the frame corresponding to the frame_id of this
   object still exists in the inferior.  */

static PyObject *
frapy_is_valid (PyObject *self, PyObject *args)
{
  frame_info_ptr frame = NULL;

  try
    {
      frame = frame_object_to_frame_info (self);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (frame == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Frame.name (self) -> String.
   Returns the name of the function corresponding to this frame.  */

static PyObject *
frapy_name (PyObject *self, PyObject *args)
{
  frame_info_ptr frame;
  gdb::unique_xmalloc_ptr<char> name;
  enum language lang;
  PyObject *result;

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      name = find_frame_funname (frame, &lang, NULL);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (name)
    {
      result = PyUnicode_Decode (name.get (), strlen (name.get ()),
				 host_charset (), NULL);
    }
  else
    {
      result = Py_None;
      Py_INCREF (Py_None);
    }

  return result;
}

/* Implementation of gdb.Frame.type (self) -> Integer.
   Returns the frame type, namely one of the gdb.*_FRAME constants.  */

static PyObject *
frapy_type (PyObject *self, PyObject *args)
{
  frame_info_ptr frame;
  enum frame_type type = NORMAL_FRAME;/* Initialize to appease gcc warning.  */

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      type = get_frame_type (frame);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return gdb_py_object_from_longest (type).release ();
}

/* Implementation of gdb.Frame.architecture (self) -> gdb.Architecture.
   Returns the frame's architecture as a gdb.Architecture object.  */

static PyObject *
frapy_arch (PyObject *self, PyObject *args)
{
  frame_info_ptr frame = NULL;    /* Initialize to appease gcc warning.  */
  frame_object *obj = (frame_object *) self;

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return gdbarch_to_arch_object (obj->gdbarch);
}

/* Implementation of gdb.Frame.unwind_stop_reason (self) -> Integer.
   Returns one of the gdb.FRAME_UNWIND_* constants.  */

static PyObject *
frapy_unwind_stop_reason (PyObject *self, PyObject *args)
{
  frame_info_ptr frame = NULL;    /* Initialize to appease gcc warning.  */
  enum unwind_stop_reason stop_reason;

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  stop_reason = get_frame_unwind_stop_reason (frame);

  return gdb_py_object_from_longest (stop_reason).release ();
}

/* Implementation of gdb.Frame.pc (self) -> Long.
   Returns the frame's resume address.  */

static PyObject *
frapy_pc (PyObject *self, PyObject *args)
{
  CORE_ADDR pc = 0;	      /* Initialize to appease gcc warning.  */
  frame_info_ptr frame;

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      pc = get_frame_pc (frame);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return gdb_py_object_from_ulongest (pc).release ();
}

/* Implementation of gdb.Frame.read_register (self, register) -> gdb.Value.
   Returns the value of a register in this frame.  */

static PyObject *
frapy_read_register (PyObject *self, PyObject *args, PyObject *kw)
{
  PyObject *pyo_reg_id;
  PyObject *result = nullptr;

  static const char *keywords[] = { "register", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords, &pyo_reg_id))
    return nullptr;

  try
    {
      scoped_value_mark free_values;
      frame_info_ptr frame;
      int regnum;

      FRAPY_REQUIRE_VALID (self, frame);

      if (!gdbpy_parse_register_id (get_frame_arch (frame), pyo_reg_id,
				    &regnum))
	return nullptr;

      gdb_assert (regnum >= 0);
      value *val
	= value_of_register (regnum, get_next_frame_sentinel_okay (frame));

      if (val == NULL)
	PyErr_SetString (PyExc_ValueError, _("Can't read register."));
      else
	result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Implementation of gdb.Frame.block (self) -> gdb.Block.
   Returns the frame's code block.  */

static PyObject *
frapy_block (PyObject *self, PyObject *args)
{
  frame_info_ptr frame;
  const struct block *block = NULL, *fn_block;

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);
      block = get_frame_block (frame, NULL);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  for (fn_block = block;
       fn_block != NULL && fn_block->function () == NULL;
       fn_block = fn_block->superblock ())
    ;

  if (block == NULL || fn_block == NULL || fn_block->function () == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Cannot locate block for frame."));
      return NULL;
    }

  return block_to_block_object (block, fn_block->function ()->objfile ());
}


/* Implementation of gdb.Frame.function (self) -> gdb.Symbol.
   Returns the symbol for the function corresponding to this frame.  */

static PyObject *
frapy_function (PyObject *self, PyObject *args)
{
  struct symbol *sym = NULL;
  frame_info_ptr frame;

  try
    {
      enum language funlang;

      FRAPY_REQUIRE_VALID (self, frame);

      gdb::unique_xmalloc_ptr<char> funname
	= find_frame_funname (frame, &funlang, &sym);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (sym)
    return symbol_to_symbol_object (sym);

  Py_RETURN_NONE;
}

/* Convert a frame_info struct to a Python Frame object.
   Sets a Python exception and returns NULL on error.  */

PyObject *
frame_info_to_frame_object (frame_info_ptr frame)
{
  gdbpy_ref<frame_object> frame_obj (PyObject_New (frame_object,
						   &frame_object_type));
  if (frame_obj == NULL)
    return NULL;

  try
    {

      /* Try to get the previous frame, to determine if this is the last frame
	 in a corrupt stack.  If so, we need to store the frame_id of the next
	 frame and not of this one (which is possibly invalid).  */
      if (get_prev_frame (frame) == NULL
	  && get_frame_unwind_stop_reason (frame) != UNWIND_NO_REASON
	  && get_next_frame (frame) != NULL)
	{
	  frame_obj->frame_id = get_frame_id (get_next_frame (frame));
	  frame_obj->frame_id_is_next = 1;
	}
      else
	{
	  frame_obj->frame_id = get_frame_id (frame);
	  frame_obj->frame_id_is_next = 0;
	}
      frame_obj->gdbarch = get_frame_arch (frame);
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return NULL;
    }

  return (PyObject *) frame_obj.release ();
}

/* Implementation of gdb.Frame.older (self) -> gdb.Frame.
   Returns the frame immediately older (outer) to this frame, or None if
   there isn't one.  */

static PyObject *
frapy_older (PyObject *self, PyObject *args)
{
  frame_info_ptr frame, prev = NULL;
  PyObject *prev_obj = NULL;   /* Initialize to appease gcc warning.  */

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      prev = get_prev_frame (frame);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (prev)
    prev_obj = frame_info_to_frame_object (prev);
  else
    {
      Py_INCREF (Py_None);
      prev_obj = Py_None;
    }

  return prev_obj;
}

/* Implementation of gdb.Frame.newer (self) -> gdb.Frame.
   Returns the frame immediately newer (inner) to this frame, or None if
   there isn't one.  */

static PyObject *
frapy_newer (PyObject *self, PyObject *args)
{
  frame_info_ptr frame, next = NULL;
  PyObject *next_obj = NULL;   /* Initialize to appease gcc warning.  */

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      next = get_next_frame (frame);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (next)
    next_obj = frame_info_to_frame_object (next);
  else
    {
      Py_INCREF (Py_None);
      next_obj = Py_None;
    }

  return next_obj;
}

/* Implementation of gdb.Frame.find_sal (self) -> gdb.Symtab_and_line.
   Returns the frame's symtab and line.  */

static PyObject *
frapy_find_sal (PyObject *self, PyObject *args)
{
  frame_info_ptr frame;
  PyObject *sal_obj = NULL;   /* Initialize to appease gcc warning.  */

  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      symtab_and_line sal = find_frame_sal (frame);
      sal_obj = symtab_and_line_to_sal_object (sal);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return sal_obj;
}

/* Implementation of gdb.Frame.read_var_value (self, variable,
   [block]) -> gdb.Value.  If the optional block argument is provided
   start the search from that block, otherwise search from the frame's
   current block (determined by examining the resume address of the
   frame).  The variable argument must be a string or an instance of a
   gdb.Symbol.  The block argument must be an instance of gdb.Block.  Returns
   NULL on error, with a python exception set.  */
static PyObject *
frapy_read_var (PyObject *self, PyObject *args, PyObject *kw)
{
  frame_info_ptr frame;
  PyObject *sym_obj, *block_obj = NULL;
  struct symbol *var = NULL;	/* gcc-4.3.2 false warning.  */
  const struct block *block = NULL;

  static const char *keywords[] = { "variable", "block", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O|O!", keywords,
					&sym_obj, &block_object_type,
					&block_obj))
    return nullptr;

  if (PyObject_TypeCheck (sym_obj, &symbol_object_type))
    var = symbol_object_to_symbol (sym_obj);
  else if (gdbpy_is_string (sym_obj))
    {
      gdb::unique_xmalloc_ptr<char>
	var_name (python_string_to_target_string (sym_obj));

      if (!var_name)
	return NULL;

      if (block_obj != nullptr)
	{
	  /* This call should only fail if the type of BLOCK_OBJ is wrong,
	     and we ensure the type is correct when we parse the arguments,
	     so we can just assert the return value is not nullptr.  */
	  block = block_object_to_block (block_obj);
	  gdb_assert (block != nullptr);
	}

      try
	{
	  struct block_symbol lookup_sym;
	  FRAPY_REQUIRE_VALID (self, frame);

	  if (!block)
	    block = get_frame_block (frame, NULL);
	  lookup_sym = lookup_symbol (var_name.get (), block, VAR_DOMAIN, NULL);
	  var = lookup_sym.symbol;
	  block = lookup_sym.block;
	}
      catch (const gdb_exception &except)
	{
	  gdbpy_convert_exception (except);
	  return NULL;
	}

      if (!var)
	{
	  PyErr_Format (PyExc_ValueError,
			_("Variable '%s' not found."), var_name.get ());

	  return NULL;
	}
    }
  else
    {
      PyErr_Format (PyExc_TypeError,
		    _("argument 1 must be gdb.Symbol or str, not %s"),
		    Py_TYPE (sym_obj)->tp_name);
      return NULL;
    }

  PyObject *result = nullptr;
  try
    {
      FRAPY_REQUIRE_VALID (self, frame);

      scoped_value_mark free_values;
      struct value *val = read_var_value (var, block, frame);
      result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Select this frame.  */

static PyObject *
frapy_select (PyObject *self, PyObject *args)
{
  frame_info_ptr fi;

  try
    {
      FRAPY_REQUIRE_VALID (self, fi);

      select_frame (fi);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* The stack frame level for this frame.  */

static PyObject *
frapy_level (PyObject *self, PyObject *args)
{
  frame_info_ptr fi;

  try
    {
      FRAPY_REQUIRE_VALID (self, fi);

      return gdb_py_object_from_longest (frame_relative_level (fi)).release ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* The language for this frame.  */

static PyObject *
frapy_language (PyObject *self, PyObject *args)
{
  try
    {
      frame_info_ptr fi;
      FRAPY_REQUIRE_VALID (self, fi);

      enum language lang = get_frame_language (fi);
      const language_defn *lang_def = language_def (lang);

      return host_string_to_python_string (lang_def->name ()).release ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* The static link for this frame.  */

static PyObject *
frapy_static_link (PyObject *self, PyObject *args)
{
  frame_info_ptr link;

  try
    {
      FRAPY_REQUIRE_VALID (self, link);

      link = frame_follow_static_link (link);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (link == nullptr)
    Py_RETURN_NONE;

  return frame_info_to_frame_object (link);
}

/* Implementation of gdb.newest_frame () -> gdb.Frame.
   Returns the newest frame object.  */

PyObject *
gdbpy_newest_frame (PyObject *self, PyObject *args)
{
  frame_info_ptr frame = NULL;

  try
    {
      frame = get_current_frame ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return frame_info_to_frame_object (frame);
}

/* Implementation of gdb.selected_frame () -> gdb.Frame.
   Returns the selected frame object.  */

PyObject *
gdbpy_selected_frame (PyObject *self, PyObject *args)
{
  frame_info_ptr frame = NULL;

  try
    {
      frame = get_selected_frame ("No frame is currently selected.");
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return frame_info_to_frame_object (frame);
}

/* Implementation of gdb.stop_reason_string (Integer) -> String.
   Return a string explaining the unwind stop reason.  */

PyObject *
gdbpy_frame_stop_reason_string (PyObject *self, PyObject *args)
{
  int reason;
  const char *str;

  if (!PyArg_ParseTuple (args, "i", &reason))
    return NULL;

  if (reason < UNWIND_FIRST || reason > UNWIND_LAST)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Invalid frame stop reason."));
      return NULL;
    }

  str = unwind_stop_reason_to_string ((enum unwind_stop_reason) reason);
  return PyUnicode_Decode (str, strlen (str), host_charset (), NULL);
}

/* Implements the equality comparison for Frame objects.
   All other comparison operators will throw a TypeError Python exception,
   as they aren't valid for frames.  */

static PyObject *
frapy_richcompare (PyObject *self, PyObject *other, int op)
{
  int result;

  if (!PyObject_TypeCheck (other, &frame_object_type)
      || (op != Py_EQ && op != Py_NE))
    {
      Py_INCREF (Py_NotImplemented);
      return Py_NotImplemented;
    }

  frame_object *self_frame = (frame_object *) self;
  frame_object *other_frame = (frame_object *) other;

  if (self_frame->frame_id_is_next == other_frame->frame_id_is_next
      && self_frame->frame_id == other_frame->frame_id)
    result = Py_EQ;
  else
    result = Py_NE;

  if (op == result)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Sets up the Frame API in the gdb module.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_frames (void)
{
  frame_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&frame_object_type) < 0)
    return -1;

  /* Note: These would probably be best exposed as class attributes of
     Frame, but I don't know how to do it except by messing with the
     type's dictionary.  That seems too messy.  */
  if (PyModule_AddIntConstant (gdb_module, "NORMAL_FRAME", NORMAL_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "DUMMY_FRAME", DUMMY_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "INLINE_FRAME", INLINE_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "TAILCALL_FRAME",
				  TAILCALL_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "SIGTRAMP_FRAME",
				  SIGTRAMP_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "ARCH_FRAME", ARCH_FRAME) < 0
      || PyModule_AddIntConstant (gdb_module, "SENTINEL_FRAME",
				  SENTINEL_FRAME) < 0)
    return -1;

#define SET(name, description) \
  if (PyModule_AddIntConstant (gdb_module, "FRAME_"#name, name) < 0) \
    return -1;
#include "unwind_stop_reasons.def"
#undef SET

  return gdb_pymodule_addobject (gdb_module, "Frame",
				 (PyObject *) &frame_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_frames);



static PyMethodDef frame_object_methods[] = {
  { "is_valid", frapy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this frame is valid, false if not." },
  { "name", frapy_name, METH_NOARGS,
    "name () -> String.\n\
Return the function name of the frame, or None if it can't be determined." },
  { "type", frapy_type, METH_NOARGS,
    "type () -> Integer.\n\
Return the type of the frame." },
  { "architecture", frapy_arch, METH_NOARGS,
    "architecture () -> gdb.Architecture.\n\
Return the architecture of the frame." },
  { "unwind_stop_reason", frapy_unwind_stop_reason, METH_NOARGS,
    "unwind_stop_reason () -> Integer.\n\
Return the reason why it's not possible to find frames older than this." },
  { "pc", frapy_pc, METH_NOARGS,
    "pc () -> Long.\n\
Return the frame's resume address." },
  { "read_register", (PyCFunction) frapy_read_register,
    METH_VARARGS | METH_KEYWORDS,
    "read_register (register_name) -> gdb.Value\n\
Return the value of the register in the frame." },
  { "block", frapy_block, METH_NOARGS,
    "block () -> gdb.Block.\n\
Return the frame's code block." },
  { "function", frapy_function, METH_NOARGS,
    "function () -> gdb.Symbol.\n\
Returns the symbol for the function corresponding to this frame." },
  { "older", frapy_older, METH_NOARGS,
    "older () -> gdb.Frame.\n\
Return the frame that called this frame." },
  { "newer", frapy_newer, METH_NOARGS,
    "newer () -> gdb.Frame.\n\
Return the frame called by this frame." },
  { "find_sal", frapy_find_sal, METH_NOARGS,
    "find_sal () -> gdb.Symtab_and_line.\n\
Return the frame's symtab and line." },
  { "read_var", (PyCFunction) frapy_read_var, METH_VARARGS | METH_KEYWORDS,
    "read_var (variable) -> gdb.Value.\n\
Return the value of the variable in this frame." },
  { "select", frapy_select, METH_NOARGS,
    "Select this frame as the user's current frame." },
  { "level", frapy_level, METH_NOARGS,
    "The stack level of this frame." },
  { "language", frapy_language, METH_NOARGS,
    "The language of this frame." },
  { "static_link", frapy_static_link, METH_NOARGS,
    "The static link of this frame, or None." },
  {NULL}  /* Sentinel */
};

PyTypeObject frame_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Frame",			  /* tp_name */
  sizeof (frame_object),	  /* tp_basicsize */
  0,				  /* tp_itemsize */
  0,				  /* tp_dealloc */
  0,				  /* tp_print */
  0,				  /* tp_getattr */
  0,				  /* tp_setattr */
  0,				  /* tp_compare */
  frapy_repr,			  /* tp_repr */
  0,				  /* tp_as_number */
  0,				  /* tp_as_sequence */
  0,				  /* tp_as_mapping */
  0,				  /* tp_hash  */
  0,				  /* tp_call */
  frapy_str,			  /* tp_str */
  0,				  /* tp_getattro */
  0,				  /* tp_setattro */
  0,				  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,		  /* tp_flags */
  "GDB frame object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  frapy_richcompare,		  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  frame_object_methods,		  /* tp_methods */
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

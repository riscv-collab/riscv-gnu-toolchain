/* Python frame unwinder interface.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "frame-unwind.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbcmd.h"
#include "language.h"
#include "observable.h"
#include "python-internal.h"
#include "regcache.h"
#include "valprint.h"
#include "user-regs.h"
#include "stack.h"
#include "charset.h"
#include "block.h"


/* Debugging of Python unwinders.  */

static bool pyuw_debug;

/* Implementation of "show debug py-unwind".  */

static void
show_pyuw_debug (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Python unwinder debugging is %s.\n"), value);
}

/* Print a "py-unwind" debug statement.  */

#define pyuw_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (pyuw_debug, "py-unwind", fmt, ##__VA_ARGS__)

/* Print "py-unwind" enter/exit debug statements.  */

#define PYUW_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (pyuw_debug, "py-unwind")

/* Require a valid pending frame.  */
#define PENDING_FRAMEPY_REQUIRE_VALID(pending_frame)	     \
  do {							     \
    if ((pending_frame)->frame_info == nullptr)		     \
      {							     \
	PyErr_SetString (PyExc_ValueError,		     \
			 _("gdb.PendingFrame is invalid.")); \
	return nullptr;					     \
      }							     \
  } while (0)

struct pending_frame_object
{
  PyObject_HEAD

  /* Frame we are unwinding.  */
  frame_info_ptr frame_info;

  /* Its architecture, passed by the sniffer caller.  */
  struct gdbarch *gdbarch;
};

/* Saved registers array item.  */

struct saved_reg
{
  saved_reg (int n, gdbpy_ref<> &&v)
    : number (n),
      value (std::move (v))
  {
  }

  int number;
  gdbpy_ref<> value;
};

/* The data we keep for the PyUnwindInfo: pending_frame, saved registers
   and frame ID.  */

struct unwind_info_object
{
  PyObject_HEAD

  /* gdb.PendingFrame for the frame we are unwinding.  */
  PyObject *pending_frame;

  /* Its ID.  */
  struct frame_id frame_id;

  /* Saved registers array.  */
  std::vector<saved_reg> *saved_regs;
};

/* The data we keep for a frame we can unwind: frame ID and an array of
   (register_number, register_value) pairs.  */

struct cached_frame_info
{
  /* Frame ID.  */
  struct frame_id frame_id;

  /* GDB Architecture.  */
  struct gdbarch *gdbarch;

  /* Length of the `reg' array below.  */
  int reg_count;

  /* Flexible array member.  Note: use a zero-sized array rather than
     an actual C99-style flexible array member (unsized array),
     because the latter would cause an error with Clang:

       error: flexible array member 'reg' of type 'cached_reg_t[]' with non-trivial destruction

     Note we manually call the destructor of each array element in
     pyuw_dealloc_cache.  */
  cached_reg_t reg[0];
};

extern PyTypeObject pending_frame_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("pending_frame_object");

extern PyTypeObject unwind_info_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("unwind_info_object");

/* An enum returned by pyuw_object_attribute_to_pointer, a function which
   is used to extract an attribute from a Python object.  */

enum class pyuw_get_attr_code
{
  /* The attribute was present, and its value was successfully extracted.  */
  ATTR_OK,

  /* The attribute was not present, or was present and its value was None.
     No Python error has been set.  */
  ATTR_MISSING,

  /* The attribute was present, but there was some error while trying to
     get the value from the attribute.  A Python error will be set when
     this is returned.  */
  ATTR_ERROR,
};

/* Get the attribute named ATTR_NAME from the object PYO and convert it to
   an inferior pointer value, placing the pointer in *ADDR.

   Return pyuw_get_attr_code::ATTR_OK if the attribute was present and its
   value was successfully written into *ADDR.  For any other return value
   the contents of *ADDR are undefined.

   Return pyuw_get_attr_code::ATTR_MISSING if the attribute was not
   present, or it was present but its value was None.  The contents of
   *ADDR are undefined in this case.  No Python error will be set in this
   case.

   Return pyuw_get_attr_code::ATTR_ERROR if the attribute was present, but
   there was some error while extracting the attribute's value.  A Python
   error will be set in this case.  The contents of *ADDR are undefined.  */

static pyuw_get_attr_code
pyuw_object_attribute_to_pointer (PyObject *pyo, const char *attr_name,
				  CORE_ADDR *addr)
{
  if (!PyObject_HasAttrString (pyo, attr_name))
    return pyuw_get_attr_code::ATTR_MISSING;

  gdbpy_ref<> pyo_value (PyObject_GetAttrString (pyo, attr_name));
  if (pyo_value == nullptr)
    {
      gdb_assert (PyErr_Occurred ());
      return pyuw_get_attr_code::ATTR_ERROR;
    }
  if (pyo_value == Py_None)
    return pyuw_get_attr_code::ATTR_MISSING;

  if (get_addr_from_python (pyo_value.get (), addr) < 0)
    {
      gdb_assert (PyErr_Occurred ());
      return pyuw_get_attr_code::ATTR_ERROR;
    }

  return pyuw_get_attr_code::ATTR_OK;
}

/* Called by the Python interpreter to obtain string representation
   of the UnwindInfo object.  */

static PyObject *
unwind_infopy_str (PyObject *self)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;
  string_file stb;

  stb.printf ("Frame ID: %s", unwind_info->frame_id.to_string ().c_str ());
  {
    const char *sep = "";
    struct value_print_options opts;

    get_user_print_options (&opts);
    stb.printf ("\nSaved registers: (");
    for (const saved_reg &reg : *unwind_info->saved_regs)
      {
	struct value *value = value_object_to_value (reg.value.get ());

	stb.printf ("%s(%d, ", sep, reg.number);
	if (value != NULL)
	  {
	    try
	      {
		value_print (value, &stb, &opts);
		stb.puts (")");
	      }
	    catch (const gdb_exception &except)
	      {
		GDB_PY_HANDLE_EXCEPTION (except);
	      }
	  }
	else
	  stb.puts ("<BAD>)");
	sep = ", ";
      }
    stb.puts (")");
  }

  return PyUnicode_FromString (stb.c_str ());
}

/* Implement UnwindInfo.__repr__().  */

static PyObject *
unwind_infopy_repr (PyObject *self)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;
  pending_frame_object *pending_frame
    = (pending_frame_object *) (unwind_info->pending_frame);
  frame_info_ptr frame = pending_frame->frame_info;

  if (frame == nullptr)
    return PyUnicode_FromFormat ("<%s for an invalid frame>",
				 Py_TYPE (self)->tp_name);

  std::string saved_reg_names;
  struct gdbarch *gdbarch = pending_frame->gdbarch;

  for (const saved_reg &reg : *unwind_info->saved_regs)
    {
      const char *name = gdbarch_register_name (gdbarch, reg.number);
      if (saved_reg_names.empty ())
	saved_reg_names = name;
      else
	saved_reg_names = (saved_reg_names + ", ") + name;
    }

  return PyUnicode_FromFormat ("<%s frame #%d, saved_regs=(%s)>",
			       Py_TYPE (self)->tp_name,
			       frame_relative_level (frame),
			       saved_reg_names.c_str ());
}

/* Create UnwindInfo instance for given PendingFrame and frame ID.
   Sets Python error and returns NULL on error.

   The PYO_PENDING_FRAME object must be valid.  */

static PyObject *
pyuw_create_unwind_info (PyObject *pyo_pending_frame,
			 struct frame_id frame_id)
{
  gdb_assert (((pending_frame_object *) pyo_pending_frame)->frame_info
	      != nullptr);

  unwind_info_object *unwind_info
    = PyObject_New (unwind_info_object, &unwind_info_object_type);

  unwind_info->frame_id = frame_id;
  Py_INCREF (pyo_pending_frame);
  unwind_info->pending_frame = pyo_pending_frame;
  unwind_info->saved_regs = new std::vector<saved_reg>;
  return (PyObject *) unwind_info;
}

/* The implementation of
   gdb.UnwindInfo.add_saved_register (REG, VALUE) -> None.  */

static PyObject *
unwind_infopy_add_saved_register (PyObject *self, PyObject *args, PyObject *kw)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;
  pending_frame_object *pending_frame
      = (pending_frame_object *) (unwind_info->pending_frame);
  PyObject *pyo_reg_id;
  PyObject *pyo_reg_value;
  int regnum;

  if (pending_frame->frame_info == NULL)
    {
      PyErr_SetString (PyExc_ValueError,
		       "UnwindInfo instance refers to a stale PendingFrame");
      return nullptr;
    }

  static const char *keywords[] = { "register", "value", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "OO!", keywords,
					&pyo_reg_id, &value_object_type,
					&pyo_reg_value))
    return nullptr;

  if (!gdbpy_parse_register_id (pending_frame->gdbarch, pyo_reg_id, &regnum))
    return nullptr;

  /* If REGNUM identifies a user register then *maybe* we can convert this
     to a real (i.e. non-user) register.  The maybe qualifier is because we
     don't know what user registers each target might add, however, the
     following logic should work for the usual style of user registers,
     where the read function just forwards the register read on to some
     other register with no adjusting the value.  */
  if (regnum >= gdbarch_num_cooked_regs (pending_frame->gdbarch))
    {
      struct value *user_reg_value
	= value_of_user_reg (regnum, pending_frame->frame_info);
      if (user_reg_value->lval () == lval_register)
	regnum = user_reg_value->regnum ();
      if (regnum >= gdbarch_num_cooked_regs (pending_frame->gdbarch))
	{
	  PyErr_SetString (PyExc_ValueError, "Bad register");
	  return NULL;
	}
    }

  /* The argument parsing above guarantees that PYO_REG_VALUE will be a
     gdb.Value object, as a result the value_object_to_value call should
     succeed.  */
  gdb_assert (pyo_reg_value != nullptr);
  struct value *value = value_object_to_value (pyo_reg_value);
  gdb_assert (value != nullptr);

  ULONGEST reg_size = register_size (pending_frame->gdbarch, regnum);
  if (reg_size != value->type ()->length ())
    {
      PyErr_Format (PyExc_ValueError,
		    "The value of the register returned by the Python "
		    "sniffer has unexpected size: %s instead of %s.",
		    pulongest (value->type ()->length ()),
		    pulongest (reg_size));
      return nullptr;
    }

  gdbpy_ref<> new_value = gdbpy_ref<>::new_reference (pyo_reg_value);
  bool found = false;
  for (saved_reg &reg : *unwind_info->saved_regs)
    {
      if (regnum == reg.number)
	{
	  found = true;
	  reg.value = std::move (new_value);
	  break;
	}
    }
  if (!found)
    unwind_info->saved_regs->emplace_back (regnum, std::move (new_value));

  Py_RETURN_NONE;
}

/* UnwindInfo cleanup.  */

static void
unwind_infopy_dealloc (PyObject *self)
{
  unwind_info_object *unwind_info = (unwind_info_object *) self;

  Py_XDECREF (unwind_info->pending_frame);
  delete unwind_info->saved_regs;
  Py_TYPE (self)->tp_free (self);
}

/* Called by the Python interpreter to obtain string representation
   of the PendingFrame object.  */

static PyObject *
pending_framepy_str (PyObject *self)
{
  frame_info_ptr frame = ((pending_frame_object *) self)->frame_info;
  const char *sp_str = NULL;
  const char *pc_str = NULL;

  if (frame == NULL)
    return PyUnicode_FromString ("Stale PendingFrame instance");
  try
    {
      sp_str = core_addr_to_string_nz (get_frame_sp (frame));
      pc_str = core_addr_to_string_nz (get_frame_pc (frame));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyUnicode_FromFormat ("SP=%s,PC=%s", sp_str, pc_str);
}

/* Implement PendingFrame.__repr__().  */

static PyObject *
pending_framepy_repr (PyObject *self)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;
  frame_info_ptr frame = pending_frame->frame_info;

  if (frame == nullptr)
    return gdb_py_invalid_object_repr (self);

  const char *sp_str = nullptr;
  const char *pc_str = nullptr;

  try
    {
      sp_str = core_addr_to_string_nz (get_frame_sp (frame));
      pc_str = core_addr_to_string_nz (get_frame_pc (frame));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyUnicode_FromFormat ("<%s level=%d, sp=%s, pc=%s>",
			       Py_TYPE (self)->tp_name,
			       frame_relative_level (frame),
			       sp_str,
			       pc_str);
}

/* Implementation of gdb.PendingFrame.read_register (self, reg) -> gdb.Value.
   Returns the value of register REG as gdb.Value instance.  */

static PyObject *
pending_framepy_read_register (PyObject *self, PyObject *args, PyObject *kw)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;
  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  PyObject *pyo_reg_id;
  static const char *keywords[] = { "register", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords, &pyo_reg_id))
    return nullptr;

  int regnum;
  if (!gdbpy_parse_register_id (pending_frame->gdbarch, pyo_reg_id, &regnum))
    return nullptr;

  PyObject *result = nullptr;
  try
    {
      scoped_value_mark free_values;

      /* Fetch the value associated with a register, whether it's
	 a real register or a so called "user" register, like "pc",
	 which maps to a real register.  In the past,
	 get_frame_register_value() was used here, which did not
	 handle the user register case.  */
      value *val = value_of_register
        (regnum, get_next_frame_sentinel_okay (pending_frame->frame_info));
      if (val == NULL)
	PyErr_Format (PyExc_ValueError,
		      "Cannot read register %d from frame.",
		      regnum);
      else
	result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Implement PendingFrame.is_valid().  Return True if this pending frame
   object is still valid.  */

static PyObject *
pending_framepy_is_valid (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  if (pending_frame->frame_info == nullptr)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implement PendingFrame.name().  Return a string that is the name of the
   function for this frame, or None if the name can't be found.  */

static PyObject *
pending_framepy_name (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  gdb::unique_xmalloc_ptr<char> name;

  try
    {
      enum language lang;
      frame_info_ptr frame = pending_frame->frame_info;

      name = find_frame_funname (frame, &lang, nullptr);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (name != nullptr)
    return PyUnicode_Decode (name.get (), strlen (name.get ()),
			     host_charset (), nullptr);

  Py_RETURN_NONE;
}

/* Implement gdb.PendingFrame.pc().  Returns an integer containing the
   frame's current $pc value.  */

static PyObject *
pending_framepy_pc (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  CORE_ADDR pc = 0;

  try
    {
      pc = get_frame_pc (pending_frame->frame_info);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return gdb_py_object_from_ulongest (pc).release ();
}

/* Implement gdb.PendingFrame.language().  Return the name of the language
   for this frame.  */

static PyObject *
pending_framepy_language (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  try
    {
      frame_info_ptr fi = pending_frame->frame_info;

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

/* Implement PendingFrame.find_sal().  Return the PendingFrame's symtab and
   line.  */

static PyObject *
pending_framepy_find_sal (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  PyObject *sal_obj = nullptr;

  try
    {
      frame_info_ptr frame = pending_frame->frame_info;

      symtab_and_line sal = find_frame_sal (frame);
      sal_obj = symtab_and_line_to_sal_object (sal);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return sal_obj;
}

/* Implement PendingFrame.block().  Return a gdb.Block for the pending
   frame's code, or raise  RuntimeError if the block can't be found.  */

static PyObject *
pending_framepy_block (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  frame_info_ptr frame = pending_frame->frame_info;
  const struct block *block = nullptr, *fn_block;

  try
    {
      block = get_frame_block (frame, nullptr);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  for (fn_block = block;
       fn_block != nullptr && fn_block->function () == nullptr;
       fn_block = fn_block->superblock ())
    ;

  if (block == nullptr
      || fn_block == nullptr
      || fn_block->function () == nullptr)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Cannot locate block for frame."));
      return nullptr;
    }

  return block_to_block_object (block, fn_block->function ()->objfile ());
}

/* Implement gdb.PendingFrame.function().  Return a gdb.Symbol
   representing the function of this frame, or None if no suitable symbol
   can be found.  */

static PyObject *
pending_framepy_function (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  struct symbol *sym = nullptr;

  try
    {
      enum language funlang;
      frame_info_ptr frame = pending_frame->frame_info;

      gdb::unique_xmalloc_ptr<char> funname
	= find_frame_funname (frame, &funlang, &sym);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (sym != nullptr)
    return symbol_to_symbol_object (sym);

  Py_RETURN_NONE;
}

/* Implementation of
   PendingFrame.create_unwind_info (self, frameId) -> UnwindInfo.  */

static PyObject *
pending_framepy_create_unwind_info (PyObject *self, PyObject *args,
				    PyObject *kw)
{
  PyObject *pyo_frame_id;
  CORE_ADDR sp;
  CORE_ADDR pc;
  CORE_ADDR special;

  PENDING_FRAMEPY_REQUIRE_VALID ((pending_frame_object *) self);

  static const char *keywords[] = { "frame_id", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords,
					&pyo_frame_id))
    return nullptr;

  pyuw_get_attr_code code
    = pyuw_object_attribute_to_pointer (pyo_frame_id, "sp", &sp);
  if (code == pyuw_get_attr_code::ATTR_MISSING)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("frame_id should have 'sp' attribute."));
      return nullptr;
    }
  else if (code == pyuw_get_attr_code::ATTR_ERROR)
    return nullptr;

  /* The logic of building frame_id depending on the attributes of
     the frame_id object:
     Has     Has    Has           Function to call
     'sp'?   'pc'?  'special'?
     ------|------|--------------|-------------------------
     Y       N      *             frame_id_build_wild (sp)
     Y       Y      N             frame_id_build (sp, pc)
     Y       Y      Y             frame_id_build_special (sp, pc, special)
  */
  code = pyuw_object_attribute_to_pointer (pyo_frame_id, "pc", &pc);
  if (code == pyuw_get_attr_code::ATTR_ERROR)
    return nullptr;
  else if (code == pyuw_get_attr_code::ATTR_MISSING)
    return pyuw_create_unwind_info (self, frame_id_build_wild (sp));

  code = pyuw_object_attribute_to_pointer (pyo_frame_id, "special", &special);
  if (code == pyuw_get_attr_code::ATTR_ERROR)
    return nullptr;
  else if (code == pyuw_get_attr_code::ATTR_MISSING)
    return pyuw_create_unwind_info (self, frame_id_build (sp, pc));

  return pyuw_create_unwind_info (self,
				  frame_id_build_special (sp, pc, special));
}

/* Implementation of PendingFrame.architecture (self) -> gdb.Architecture.  */

static PyObject *
pending_framepy_architecture (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  return gdbarch_to_arch_object (pending_frame->gdbarch);
}

/* Implementation of PendingFrame.level (self) -> Integer.  */

static PyObject *
pending_framepy_level (PyObject *self, PyObject *args)
{
  pending_frame_object *pending_frame = (pending_frame_object *) self;

  PENDING_FRAMEPY_REQUIRE_VALID (pending_frame);

  int level = frame_relative_level (pending_frame->frame_info);
  return gdb_py_object_from_longest (level).release ();
}

/* frame_unwind.this_id method.  */

static void
pyuw_this_id (frame_info_ptr this_frame, void **cache_ptr,
	      struct frame_id *this_id)
{
  *this_id = ((cached_frame_info *) *cache_ptr)->frame_id;
  pyuw_debug_printf ("frame_id: %s", this_id->to_string ().c_str ());
}

/* frame_unwind.prev_register.  */

static struct value *
pyuw_prev_register (frame_info_ptr this_frame, void **cache_ptr,
		    int regnum)
{
  PYUW_SCOPED_DEBUG_ENTER_EXIT;

  cached_frame_info *cached_frame = (cached_frame_info *) *cache_ptr;
  cached_reg_t *reg_info = cached_frame->reg;
  cached_reg_t *reg_info_end = reg_info + cached_frame->reg_count;

  pyuw_debug_printf ("frame=%d, reg=%d",
		     frame_relative_level (this_frame), regnum);
  for (; reg_info < reg_info_end; ++reg_info)
    {
      if (regnum == reg_info->num)
	return frame_unwind_got_bytes (this_frame, regnum, reg_info->data.get ());
    }

  return frame_unwind_got_optimized (this_frame, regnum);
}

/* Frame sniffer dispatch.  */

static int
pyuw_sniffer (const struct frame_unwind *self, frame_info_ptr this_frame,
	      void **cache_ptr)
{
  PYUW_SCOPED_DEBUG_ENTER_EXIT;

  struct gdbarch *gdbarch = (struct gdbarch *) (self->unwind_data);
  cached_frame_info *cached_frame;

  gdbpy_enter enter_py (gdbarch);

  pyuw_debug_printf ("frame=%d, sp=%s, pc=%s",
		     frame_relative_level (this_frame),
		     paddress (gdbarch, get_frame_sp (this_frame)),
		     paddress (gdbarch, get_frame_pc (this_frame)));

  /* Create PendingFrame instance to pass to sniffers.  */
  pending_frame_object *pfo = PyObject_New (pending_frame_object,
					    &pending_frame_object_type);
  gdbpy_ref<> pyo_pending_frame ((PyObject *) pfo);
  if (pyo_pending_frame == NULL)
    {
      gdbpy_print_stack ();
      return 0;
    }
  pfo->gdbarch = gdbarch;
  pfo->frame_info = nullptr;
  scoped_restore invalidate_frame = make_scoped_restore (&pfo->frame_info,
							 this_frame);

  /* Run unwinders.  */
  if (gdb_python_module == NULL
      || ! PyObject_HasAttrString (gdb_python_module, "_execute_unwinders"))
    {
      PyErr_SetString (PyExc_NameError,
		       "Installation error: gdb._execute_unwinders function "
		       "is missing");
      gdbpy_print_stack ();
      return 0;
    }
  gdbpy_ref<> pyo_execute (PyObject_GetAttrString (gdb_python_module,
						   "_execute_unwinders"));
  if (pyo_execute == nullptr)
    {
      gdbpy_print_stack ();
      return 0;
    }

  /* A (gdb.UnwindInfo, str) tuple, or None.  */
  gdbpy_ref<> pyo_execute_ret
    (PyObject_CallFunctionObjArgs (pyo_execute.get (),
				   pyo_pending_frame.get (), NULL));
  if (pyo_execute_ret == nullptr)
    {
      /* If the unwinder is cancelled due to a Ctrl-C, then propagate
	 the Ctrl-C as a GDB exception instead of swallowing it.  */
      gdbpy_print_stack_or_quit ();
      return 0;
    }
  if (pyo_execute_ret == Py_None)
    return 0;

  /* Verify the return value of _execute_unwinders is a tuple of size 2.  */
  gdb_assert (PyTuple_Check (pyo_execute_ret.get ()));
  gdb_assert (PyTuple_GET_SIZE (pyo_execute_ret.get ()) == 2);

  if (pyuw_debug)
    {
      PyObject *pyo_unwinder_name = PyTuple_GET_ITEM (pyo_execute_ret.get (), 1);
      gdb::unique_xmalloc_ptr<char> name
	= python_string_to_host_string (pyo_unwinder_name);

      /* This could happen if the user passed something else than a string
	 as the unwinder's name.  */
      if (name == nullptr)
	{
	  gdbpy_print_stack ();
	  name = make_unique_xstrdup ("<failed to get unwinder name>");
	}

      pyuw_debug_printf ("frame claimed by unwinder %s", name.get ());
    }

  /* Received UnwindInfo, cache data.  */
  PyObject *pyo_unwind_info = PyTuple_GET_ITEM (pyo_execute_ret.get (), 0);
  if (PyObject_IsInstance (pyo_unwind_info,
			   (PyObject *) &unwind_info_object_type) <= 0)
    error (_("A Unwinder should return gdb.UnwindInfo instance."));

  {
    unwind_info_object *unwind_info =
      (unwind_info_object *) pyo_unwind_info;
    int reg_count = unwind_info->saved_regs->size ();

    cached_frame
      = ((cached_frame_info *)
	 xmalloc (sizeof (*cached_frame)
		  + reg_count * sizeof (cached_frame->reg[0])));
    cached_frame->gdbarch = gdbarch;
    cached_frame->frame_id = unwind_info->frame_id;
    cached_frame->reg_count = reg_count;

    /* Populate registers array.  */
    for (int i = 0; i < unwind_info->saved_regs->size (); ++i)
      {
	saved_reg *reg = &(*unwind_info->saved_regs)[i];

	struct value *value = value_object_to_value (reg->value.get ());
	size_t data_size = register_size (gdbarch, reg->number);

	/* `value' validation was done before, just assert.  */
	gdb_assert (value != NULL);
	gdb_assert (data_size == value->type ()->length ());

	cached_reg_t *cached = new (&cached_frame->reg[i]) cached_reg_t ();
	cached->num = reg->number;
	cached->data.reset ((gdb_byte *) xmalloc (data_size));
	memcpy (cached->data.get (), value->contents ().data (), data_size);
      }
  }

  *cache_ptr = cached_frame;
  return 1;
}

/* Frame cache release shim.  */

static void
pyuw_dealloc_cache (frame_info *this_frame, void *cache)
{
  PYUW_SCOPED_DEBUG_ENTER_EXIT;
  cached_frame_info *cached_frame = (cached_frame_info *) cache;

  for (int i = 0; i < cached_frame->reg_count; i++)
    cached_frame->reg[i].~cached_reg_t ();

  xfree (cache);
}

struct pyuw_gdbarch_data_type
{
  /* Has the unwinder shim been prepended? */
  int unwinder_registered = 0;
};

static const registry<gdbarch>::key<pyuw_gdbarch_data_type> pyuw_gdbarch_data;

/* New inferior architecture callback: register the Python unwinders
   intermediary.  */

static void
pyuw_on_new_gdbarch (gdbarch *newarch)
{
  struct pyuw_gdbarch_data_type *data = pyuw_gdbarch_data.get (newarch);
  if (data == nullptr)
    data= pyuw_gdbarch_data.emplace (newarch);

  if (!data->unwinder_registered)
    {
      struct frame_unwind *unwinder
	  = GDBARCH_OBSTACK_ZALLOC (newarch, struct frame_unwind);

      unwinder->name = "python";
      unwinder->type = NORMAL_FRAME;
      unwinder->stop_reason = default_frame_unwind_stop_reason;
      unwinder->this_id = pyuw_this_id;
      unwinder->prev_register = pyuw_prev_register;
      unwinder->unwind_data = (const struct frame_data *) newarch;
      unwinder->sniffer = pyuw_sniffer;
      unwinder->dealloc_cache = pyuw_dealloc_cache;
      frame_unwind_prepend_unwinder (newarch, unwinder);
      data->unwinder_registered = 1;
    }
}

/* Initialize unwind machinery.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_unwind (void)
{
  gdb::observers::new_architecture.attach (pyuw_on_new_gdbarch, "py-unwind");

  if (PyType_Ready (&pending_frame_object_type) < 0)
    return -1;
  int rc = gdb_pymodule_addobject (gdb_module, "PendingFrame",
				   (PyObject *) &pending_frame_object_type);
  if (rc != 0)
    return rc;

  if (PyType_Ready (&unwind_info_object_type) < 0)
    return -1;
  return gdb_pymodule_addobject (gdb_module, "UnwindInfo",
      (PyObject *) &unwind_info_object_type);
}

void _initialize_py_unwind ();
void
_initialize_py_unwind ()
{
  add_setshow_boolean_cmd
      ("py-unwind", class_maintenance, &pyuw_debug,
	_("Set Python unwinder debugging."),
	_("Show Python unwinder debugging."),
	_("When on, Python unwinder debugging is enabled."),
	NULL,
	show_pyuw_debug,
	&setdebuglist, &showdebuglist);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_unwind);



static PyMethodDef pending_frame_object_methods[] =
{
  { "read_register", (PyCFunction) pending_framepy_read_register,
    METH_VARARGS | METH_KEYWORDS,
    "read_register (REG) -> gdb.Value\n"
    "Return the value of the REG in the frame." },
  { "create_unwind_info", (PyCFunction) pending_framepy_create_unwind_info,
    METH_VARARGS | METH_KEYWORDS,
    "create_unwind_info (FRAME_ID) -> gdb.UnwindInfo\n"
    "Construct UnwindInfo for this PendingFrame, using FRAME_ID\n"
    "to identify it." },
  { "architecture",
    pending_framepy_architecture, METH_NOARGS,
    "architecture () -> gdb.Architecture\n"
    "The architecture for this PendingFrame." },
  { "name",
    pending_framepy_name, METH_NOARGS,
    "name() -> String.\n\
Return the function name of the frame, or None if it can't be determined." },
  { "is_valid",
    pending_framepy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this PendingFrame is valid, false if not." },
  { "pc",
    pending_framepy_pc, METH_NOARGS,
    "pc () -> Long.\n\
Return the frame's resume address." },
  { "language", pending_framepy_language, METH_NOARGS,
    "The language of this frame." },
  { "find_sal", pending_framepy_find_sal, METH_NOARGS,
    "find_sal () -> gdb.Symtab_and_line.\n\
Return the frame's symtab and line." },
  { "block", pending_framepy_block, METH_NOARGS,
    "block () -> gdb.Block.\n\
Return the frame's code block." },
  { "function", pending_framepy_function, METH_NOARGS,
    "function () -> gdb.Symbol.\n\
Returns the symbol for the function corresponding to this frame." },
  { "level", pending_framepy_level, METH_NOARGS,
    "The stack level of this frame." },
  {NULL}  /* Sentinel */
};

PyTypeObject pending_frame_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.PendingFrame",             /* tp_name */
  sizeof (pending_frame_object),  /* tp_basicsize */
  0,                              /* tp_itemsize */
  0,                              /* tp_dealloc */
  0,                              /* tp_print */
  0,                              /* tp_getattr */
  0,                              /* tp_setattr */
  0,                              /* tp_compare */
  pending_framepy_repr,           /* tp_repr */
  0,                              /* tp_as_number */
  0,                              /* tp_as_sequence */
  0,                              /* tp_as_mapping */
  0,                              /* tp_hash  */
  0,                              /* tp_call */
  pending_framepy_str,            /* tp_str */
  0,                              /* tp_getattro */
  0,                              /* tp_setattro */
  0,                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,             /* tp_flags */
  "GDB PendingFrame object",      /* tp_doc */
  0,                              /* tp_traverse */
  0,                              /* tp_clear */
  0,                              /* tp_richcompare */
  0,                              /* tp_weaklistoffset */
  0,                              /* tp_iter */
  0,                              /* tp_iternext */
  pending_frame_object_methods,   /* tp_methods */
  0,                              /* tp_members */
  0,                              /* tp_getset */
  0,                              /* tp_base */
  0,                              /* tp_dict */
  0,                              /* tp_descr_get */
  0,                              /* tp_descr_set */
  0,                              /* tp_dictoffset */
  0,                              /* tp_init */
  0,                              /* tp_alloc */
};

static PyMethodDef unwind_info_object_methods[] =
{
  { "add_saved_register",
    (PyCFunction) unwind_infopy_add_saved_register,
    METH_VARARGS | METH_KEYWORDS,
    "add_saved_register (REG, VALUE) -> None\n"
    "Set the value of the REG in the previous frame to VALUE." },
  { NULL }  /* Sentinel */
};

PyTypeObject unwind_info_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.UnwindInfo",               /* tp_name */
  sizeof (unwind_info_object),    /* tp_basicsize */
  0,                              /* tp_itemsize */
  unwind_infopy_dealloc,          /* tp_dealloc */
  0,                              /* tp_print */
  0,                              /* tp_getattr */
  0,                              /* tp_setattr */
  0,                              /* tp_compare */
  unwind_infopy_repr,             /* tp_repr */
  0,                              /* tp_as_number */
  0,                              /* tp_as_sequence */
  0,                              /* tp_as_mapping */
  0,                              /* tp_hash  */
  0,                              /* tp_call */
  unwind_infopy_str,              /* tp_str */
  0,                              /* tp_getattro */
  0,                              /* tp_setattro */
  0,                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,             /* tp_flags */
  "GDB UnwindInfo object",        /* tp_doc */
  0,                              /* tp_traverse */
  0,                              /* tp_clear */
  0,                              /* tp_richcompare */
  0,                              /* tp_weaklistoffset */
  0,                              /* tp_iter */
  0,                              /* tp_iternext */
  unwind_info_object_methods,     /* tp_methods */
  0,                              /* tp_members */
  0,                              /* tp_getset */
  0,                              /* tp_base */
  0,                              /* tp_dict */
  0,                              /* tp_descr_get */
  0,                              /* tp_descr_set */
  0,                              /* tp_dictoffset */
  0,                              /* tp_init */
  0,                              /* tp_alloc */
};

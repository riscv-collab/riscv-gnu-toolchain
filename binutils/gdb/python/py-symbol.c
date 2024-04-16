/* Python interface to symbols.

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
#include "top.h"
#include "block.h"
#include "frame.h"
#include "symtab.h"
#include "python-internal.h"
#include "objfiles.h"
#include "symfile.h"

struct symbol_object {
  PyObject_HEAD
  /* The GDB symbol structure this object is wrapping.  */
  struct symbol *symbol;
  /* A symbol object is associated with an objfile, so keep track with
     doubly-linked list, rooted in the objfile.  This lets us
     invalidate the underlying struct symbol when the objfile is
     deleted.  */
  symbol_object *prev;
  symbol_object *next;
};

/* Require a valid symbol.  All access to symbol_object->symbol should be
   gated by this call.  */
#define SYMPY_REQUIRE_VALID(symbol_obj, symbol)		\
  do {							\
    symbol = symbol_object_to_symbol (symbol_obj);	\
    if (symbol == NULL)					\
      {							\
	PyErr_SetString (PyExc_RuntimeError,		\
			 _("Symbol is invalid."));	\
	return NULL;					\
      }							\
  } while (0)

/* A deleter that is used when an objfile is about to be freed.  */
struct symbol_object_deleter
{
  void operator() (symbol_object *obj)
  {
    while (obj)
      {
	symbol_object *next = obj->next;

	obj->symbol = NULL;
	obj->next = NULL;
	obj->prev = NULL;

	obj = next;
      }
  }
};

static const registry<objfile>::key<symbol_object, symbol_object_deleter>
     sympy_objfile_data_key;

static PyObject *
sympy_str (PyObject *self)
{
  PyObject *result;
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  result = PyUnicode_FromString (symbol->print_name ());

  return result;
}

static PyObject *
sympy_get_type (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  if (symbol->type () == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return type_to_type_object (symbol->type ());
}

static PyObject *
sympy_get_symtab (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  if (!symbol->is_objfile_owned ())
    Py_RETURN_NONE;

  return symtab_to_symtab_object (symbol->symtab ());
}

static PyObject *
sympy_get_name (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  return PyUnicode_FromString (symbol->natural_name ());
}

static PyObject *
sympy_get_linkage_name (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  return PyUnicode_FromString (symbol->linkage_name ());
}

static PyObject *
sympy_get_print_name (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  return sympy_str (self);
}

static PyObject *
sympy_get_addr_class (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  return gdb_py_object_from_longest (symbol->aclass ()).release ();
}

static PyObject *
sympy_is_argument (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  return PyBool_FromLong (symbol->is_argument ());
}

static PyObject *
sympy_is_constant (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;
  enum address_class theclass;

  SYMPY_REQUIRE_VALID (self, symbol);

  theclass = symbol->aclass ();

  return PyBool_FromLong (theclass == LOC_CONST || theclass == LOC_CONST_BYTES);
}

static PyObject *
sympy_is_function (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;
  enum address_class theclass;

  SYMPY_REQUIRE_VALID (self, symbol);

  theclass = symbol->aclass ();

  return PyBool_FromLong (theclass == LOC_BLOCK);
}

static PyObject *
sympy_is_variable (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;
  enum address_class theclass;

  SYMPY_REQUIRE_VALID (self, symbol);

  theclass = symbol->aclass ();

  return PyBool_FromLong (!symbol->is_argument ()
			  && (theclass == LOC_LOCAL || theclass == LOC_REGISTER
			      || theclass == LOC_STATIC || theclass == LOC_COMPUTED
			      || theclass == LOC_OPTIMIZED_OUT));
}

/* Implementation of gdb.Symbol.needs_frame -> Boolean.
   Returns true iff the symbol needs a frame for evaluation.  */

static PyObject *
sympy_needs_frame (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;
  int result = 0;

  SYMPY_REQUIRE_VALID (self, symbol);

  try
    {
      result = symbol_read_needs_frame (symbol);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (result)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Implementation of gdb.Symbol.line -> int.
   Returns the line number at which the symbol was defined.  */

static PyObject *
sympy_line (PyObject *self, void *closure)
{
  struct symbol *symbol = NULL;

  SYMPY_REQUIRE_VALID (self, symbol);

  return gdb_py_object_from_longest (symbol->line ()).release ();
}

/* Implementation of gdb.Symbol.is_valid (self) -> Boolean.
   Returns True if this Symbol still exists in GDB.  */

static PyObject *
sympy_is_valid (PyObject *self, PyObject *args)
{
  struct symbol *symbol = NULL;

  symbol = symbol_object_to_symbol (self);
  if (symbol == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Symbol.value (self[, frame]) -> gdb.Value.  Returns
   the value of the symbol, or an error in various circumstances.  */

static PyObject *
sympy_value (PyObject *self, PyObject *args)
{
  struct symbol *symbol = NULL;
  frame_info_ptr frame_info = NULL;
  PyObject *frame_obj = NULL;

  if (!PyArg_ParseTuple (args, "|O", &frame_obj))
    return NULL;

  if (frame_obj != NULL && !PyObject_TypeCheck (frame_obj, &frame_object_type))
    {
      PyErr_SetString (PyExc_TypeError, "argument is not a frame");
      return NULL;
    }

  SYMPY_REQUIRE_VALID (self, symbol);
  if (symbol->aclass () == LOC_TYPEDEF)
    {
      PyErr_SetString (PyExc_TypeError, "cannot get the value of a typedef");
      return NULL;
    }

  PyObject *result = nullptr;
  try
    {
      if (frame_obj != NULL)
	{
	  frame_info = frame_object_to_frame_info (frame_obj);
	  if (frame_info == NULL)
	    error (_("invalid frame"));
	}

      if (symbol_read_needs_frame (symbol) && frame_info == NULL)
	error (_("symbol requires a frame to compute its value"));

      /* TODO: currently, we have no way to recover the block in which SYMBOL
	 was found, so we have no block to pass to read_var_value.  This will
	 yield an incorrect value when symbol is not local to FRAME_INFO (this
	 can happen with nested functions).  */
      scoped_value_mark free_values;
      struct value *value = read_var_value (symbol, NULL, frame_info);
      result = value_to_value_object (value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Given a symbol, and a symbol_object that has previously been
   allocated and initialized, populate the symbol_object with the
   struct symbol data.  Also, register the symbol_object life-cycle
   with the life-cycle of the object file associated with this
   symbol, if needed.  */
static void
set_symbol (symbol_object *obj, struct symbol *symbol)
{
  obj->symbol = symbol;
  obj->prev = NULL;
  if (symbol->is_objfile_owned ()
      && symbol->symtab () != NULL)
    {
      struct objfile *objfile = symbol->objfile ();

      obj->next = sympy_objfile_data_key.get (objfile);
      if (obj->next)
	obj->next->prev = obj;
      sympy_objfile_data_key.set (objfile, obj);
    }
  else
    obj->next = NULL;
}

/* Create a new symbol object (gdb.Symbol) that encapsulates the struct
   symbol object from GDB.  */
PyObject *
symbol_to_symbol_object (struct symbol *sym)
{
  symbol_object *sym_obj;

  sym_obj = PyObject_New (symbol_object, &symbol_object_type);
  if (sym_obj)
    set_symbol (sym_obj, sym);

  return (PyObject *) sym_obj;
}

/* Return the symbol that is wrapped by this symbol object.  */
struct symbol *
symbol_object_to_symbol (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &symbol_object_type))
    return NULL;
  return ((symbol_object *) obj)->symbol;
}

static void
sympy_dealloc (PyObject *obj)
{
  symbol_object *sym_obj = (symbol_object *) obj;

  if (sym_obj->prev)
    sym_obj->prev->next = sym_obj->next;
  else if (sym_obj->symbol != NULL
	   && sym_obj->symbol->is_objfile_owned ()
	   && sym_obj->symbol->symtab () != NULL)
    sympy_objfile_data_key.set (sym_obj->symbol->objfile (), sym_obj->next);
  if (sym_obj->next)
    sym_obj->next->prev = sym_obj->prev;
  sym_obj->symbol = NULL;
  Py_TYPE (obj)->tp_free (obj);
}

/* __repr__ implementation for gdb.Symbol.  */

static PyObject *
sympy_repr (PyObject *self)
{
  const auto symbol = symbol_object_to_symbol (self);
  if (symbol == nullptr)
    return gdb_py_invalid_object_repr (self);

  return PyUnicode_FromFormat ("<%s print_name=%s>", Py_TYPE (self)->tp_name,
			       symbol->print_name ());
}

/* Implementation of
   gdb.lookup_symbol (name [, block] [, domain]) -> (symbol, is_field_of_this)
   A tuple with 2 elements is always returned.  The first is the symbol
   object or None, the second is a boolean with the value of
   is_a_field_of_this (see comment in lookup_symbol_in_language).  */

PyObject *
gdbpy_lookup_symbol (PyObject *self, PyObject *args, PyObject *kw)
{
  int domain = VAR_DOMAIN;
  struct field_of_this_result is_a_field_of_this;
  const char *name;
  static const char *keywords[] = { "name", "block", "domain", NULL };
  struct symbol *symbol = NULL;
  PyObject *block_obj = NULL, *sym_obj, *bool_obj;
  const struct block *block = NULL;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O!i", keywords, &name,
					&block_object_type, &block_obj,
					&domain))
    return NULL;

  if (block_obj)
    block = block_object_to_block (block_obj);
  else
    {
      frame_info_ptr selected_frame;

      try
	{
	  selected_frame = get_selected_frame (_("No frame selected."));
	  block = get_frame_block (selected_frame, NULL);
	}
      catch (const gdb_exception &except)
	{
	  GDB_PY_HANDLE_EXCEPTION (except);
	}
    }

  try
    {
      symbol = lookup_symbol (name, block, (domain_enum) domain,
			      &is_a_field_of_this).symbol;
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  gdbpy_ref<> ret_tuple (PyTuple_New (2));
  if (ret_tuple == NULL)
    return NULL;

  if (symbol)
    {
      sym_obj = symbol_to_symbol_object (symbol);
      if (!sym_obj)
	return NULL;
    }
  else
    {
      sym_obj = Py_None;
      Py_INCREF (Py_None);
    }
  PyTuple_SET_ITEM (ret_tuple.get (), 0, sym_obj);

  bool_obj = PyBool_FromLong (is_a_field_of_this.type != NULL);
  PyTuple_SET_ITEM (ret_tuple.get (), 1, bool_obj);

  return ret_tuple.release ();
}

/* Implementation of
   gdb.lookup_global_symbol (name [, domain]) -> symbol or None.  */

PyObject *
gdbpy_lookup_global_symbol (PyObject *self, PyObject *args, PyObject *kw)
{
  int domain = VAR_DOMAIN;
  const char *name;
  static const char *keywords[] = { "name", "domain", NULL };
  struct symbol *symbol = NULL;
  PyObject *sym_obj;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &name,
					&domain))
    return NULL;

  try
    {
      symbol = lookup_global_symbol (name, NULL, (domain_enum) domain).symbol;
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (symbol)
    {
      sym_obj = symbol_to_symbol_object (symbol);
      if (!sym_obj)
	return NULL;
    }
  else
    {
      sym_obj = Py_None;
      Py_INCREF (Py_None);
    }

  return sym_obj;
}

/* Implementation of
   gdb.lookup_static_symbol (name [, domain]) -> symbol or None.  */

PyObject *
gdbpy_lookup_static_symbol (PyObject *self, PyObject *args, PyObject *kw)
{
  const char *name;
  int domain = VAR_DOMAIN;
  static const char *keywords[] = { "name", "domain", NULL };
  struct symbol *symbol = NULL;
  PyObject *sym_obj;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &name,
					&domain))
    return NULL;

  /* In order to find static symbols associated with the "current" object
     file ahead of those from other object files, we first need to see if
     we can acquire a current block.  If this fails however, then we still
     want to search all static symbols, so don't throw an exception just
     yet.  */
  const struct block *block = NULL;
  try
    {
      frame_info_ptr selected_frame
	= get_selected_frame (_("No frame selected."));
      block = get_frame_block (selected_frame, NULL);
    }
  catch (const gdb_exception_forced_quit &e)
    {
      quit_force (NULL, 0);
    }
  catch (const gdb_exception &except)
    {
      /* Nothing.  */
    }

  try
    {
      if (block != nullptr)
	symbol
	  = lookup_symbol_in_static_block (name, block,
					   (domain_enum) domain).symbol;

      if (symbol == nullptr)
	symbol = lookup_static_symbol (name, (domain_enum) domain).symbol;
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (symbol)
    {
      sym_obj = symbol_to_symbol_object (symbol);
      if (!sym_obj)
	return NULL;
    }
  else
    {
      sym_obj = Py_None;
      Py_INCREF (Py_None);
    }

  return sym_obj;
}

/* Implementation of
   gdb.lookup_static_symbols (name [, domain]) -> symbol list.

   Returns a list of all static symbols matching NAME in DOMAIN.  */

PyObject *
gdbpy_lookup_static_symbols (PyObject *self, PyObject *args, PyObject *kw)
{
  const char *name;
  int domain = VAR_DOMAIN;
  static const char *keywords[] = { "name", "domain", NULL };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &name,
					&domain))
    return NULL;

  gdbpy_ref<> return_list (PyList_New (0));
  if (return_list == NULL)
    return NULL;

  try
    {
      /* Expand any symtabs that contain potentially matching symbols.  */
      lookup_name_info lookup_name (name, symbol_name_match_type::FULL);
      expand_symtabs_matching (NULL, lookup_name, NULL, NULL,
			       SEARCH_GLOBAL_BLOCK | SEARCH_STATIC_BLOCK,
			       ALL_DOMAIN);

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (compunit_symtab *cust : objfile->compunits ())
	    {
	      /* Skip included compunits to prevent including compunits from
		 being searched twice.  */
	      if (cust->user != nullptr)
		continue;

	      const struct blockvector *bv = cust->blockvector ();
	      const struct block *block = bv->static_block ();

	      if (block != nullptr)
		{
		  symbol *symbol = lookup_symbol_in_static_block
		    (name, block, (domain_enum) domain).symbol;

		  if (symbol != nullptr)
		    {
		      PyObject *sym_obj
			= symbol_to_symbol_object (symbol);
		      if (PyList_Append (return_list.get (), sym_obj) == -1)
			return NULL;
		    }
		}
	    }
	}
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return return_list.release ();
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_symbols (void)
{
  if (PyType_Ready (&symbol_object_type) < 0)
    return -1;

  if (PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_UNDEF", LOC_UNDEF) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_CONST",
				  LOC_CONST) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_STATIC",
				  LOC_STATIC) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_REGISTER",
				  LOC_REGISTER) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_ARG",
				  LOC_ARG) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_REF_ARG",
				  LOC_REF_ARG) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_LOCAL",
				  LOC_LOCAL) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_TYPEDEF",
				  LOC_TYPEDEF) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_LABEL",
				  LOC_LABEL) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_BLOCK",
				  LOC_BLOCK) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_CONST_BYTES",
				  LOC_CONST_BYTES) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_UNRESOLVED",
				  LOC_UNRESOLVED) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_OPTIMIZED_OUT",
				  LOC_OPTIMIZED_OUT) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_COMPUTED",
				  LOC_COMPUTED) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_COMMON_BLOCK",
				  LOC_COMMON_BLOCK) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LOC_REGPARM_ADDR",
				  LOC_REGPARM_ADDR) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_UNDEF_DOMAIN",
				  UNDEF_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_VAR_DOMAIN",
				  VAR_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_STRUCT_DOMAIN",
				  STRUCT_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_LABEL_DOMAIN",
				  LABEL_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_MODULE_DOMAIN",
				  MODULE_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_COMMON_BLOCK_DOMAIN",
				  COMMON_BLOCK_DOMAIN) < 0)
    return -1;

  /* These remain defined for compatibility, but as they were never
     correct, they are no longer documented.  Eventually we can remove
     them.  These exist because at one time, enum search_domain and
     enum domain_enum_tag were combined -- but different values were
     used differently.  Here we try to give them values that will make
     sense if they are passed to gdb.lookup_symbol.  */
  if (PyModule_AddIntConstant (gdb_module, "SYMBOL_VARIABLES_DOMAIN",
			       VAR_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_FUNCTIONS_DOMAIN",
				  VAR_DOMAIN) < 0
      || PyModule_AddIntConstant (gdb_module, "SYMBOL_TYPES_DOMAIN",
				  VAR_DOMAIN) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Symbol",
				 (PyObject *) &symbol_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_symbols);



static gdb_PyGetSetDef symbol_object_getset[] = {
  { "type", sympy_get_type, NULL,
    "Type of the symbol.", NULL },
  { "symtab", sympy_get_symtab, NULL,
    "Symbol table in which the symbol appears.", NULL },
  { "name", sympy_get_name, NULL,
    "Name of the symbol, as it appears in the source code.", NULL },
  { "linkage_name", sympy_get_linkage_name, NULL,
    "Name of the symbol, as used by the linker (i.e., may be mangled).",
    NULL },
  { "print_name", sympy_get_print_name, NULL,
    "Name of the symbol in a form suitable for output.\n\
This is either name or linkage_name, depending on whether the user asked GDB\n\
to display demangled or mangled names.", NULL },
  { "addr_class", sympy_get_addr_class, NULL, "Address class of the symbol." },
  { "is_argument", sympy_is_argument, NULL,
    "True if the symbol is an argument of a function." },
  { "is_constant", sympy_is_constant, NULL,
    "True if the symbol is a constant." },
  { "is_function", sympy_is_function, NULL,
    "True if the symbol is a function or method." },
  { "is_variable", sympy_is_variable, NULL,
    "True if the symbol is a variable." },
  { "needs_frame", sympy_needs_frame, NULL,
    "True if the symbol requires a frame for evaluation." },
  { "line", sympy_line, NULL,
    "The source line number at which the symbol was defined." },
  { NULL }  /* Sentinel */
};

static PyMethodDef symbol_object_methods[] = {
  { "is_valid", sympy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this symbol is valid, false if not." },
  { "value", sympy_value, METH_VARARGS,
    "value ([frame]) -> gdb.Value\n\
Return the value of the symbol." },
  {NULL}  /* Sentinel */
};

PyTypeObject symbol_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Symbol",			  /*tp_name*/
  sizeof (symbol_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  sympy_dealloc,		  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  sympy_repr,                    /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  sympy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB symbol object",		  /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  0,				  /*tp_iter */
  0,				  /*tp_iternext */
  symbol_object_methods,	  /*tp_methods */
  0,				  /*tp_members */
  symbol_object_getset		  /*tp_getset */
};

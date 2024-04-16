/* Python interface to blocks.

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
#include "block.h"
#include "dictionary.h"
#include "symtab.h"
#include "python-internal.h"
#include "objfiles.h"

struct block_object {
  PyObject_HEAD
  /* The GDB block structure that represents a frame's code block.  */
  const struct block *block;
  /* The backing object file.  There is no direct relationship in GDB
     between a block and an object file.  When a block is created also
     store a pointer to the object file for later use.  */
  struct objfile *objfile;
  /* Keep track of all blocks with a doubly-linked list.  Needed for
     block invalidation if the source object file has been freed.  */
  block_object *prev;
  block_object *next;
};

struct block_syms_iterator_object {
  PyObject_HEAD
  /* The block.  */
  const struct block *block;
  /* The iterator for that block.  */
  struct block_iterator iter;
  /* Has the iterator been initialized flag.  */
  int initialized_p;
  /* Pointer back to the original source block object.  Needed to
     check if the block is still valid, and has not been invalidated
     when an object file has been freed.  */
  block_object *source;
};

/* Require a valid block.  All access to block_object->block should be
   gated by this call.  */
#define BLPY_REQUIRE_VALID(block_obj, block)		\
  do {							\
    block = block_object_to_block (block_obj);		\
    if (block == NULL)					\
      {							\
	PyErr_SetString (PyExc_RuntimeError,		\
			 _("Block is invalid."));	\
	return NULL;					\
      }							\
  } while (0)

/* Require a valid block.  This macro is called during block iterator
   creation, and at each next call.  */
#define BLPY_ITER_REQUIRE_VALID(block_obj)				\
  do {									\
    if (block_obj->block == NULL)					\
      {									\
	PyErr_SetString (PyExc_RuntimeError,				\
			 _("Source block for iterator is invalid."));	\
	return NULL;							\
      }									\
  } while (0)

/* This is called when an objfile is about to be freed.
   Invalidate the block as further actions on the block would result
   in bad data.  All access to obj->symbol should be gated by
   BLPY_REQUIRE_VALID which will raise an exception on invalid
   blocks.  */
struct blpy_deleter
{
  void operator() (block_object *obj)
  {
    while (obj)
      {
	block_object *next = obj->next;

	obj->block = NULL;
	obj->objfile = NULL;
	obj->next = NULL;
	obj->prev = NULL;

	obj = next;
      }
  }
};

extern PyTypeObject block_syms_iterator_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("block_syms_iterator_object");
static const registry<objfile>::key<block_object, blpy_deleter>
     blpy_objfile_data_key;

static PyObject *
blpy_iter (PyObject *self)
{
  block_syms_iterator_object *block_iter_obj;
  const struct block *block = NULL;

  BLPY_REQUIRE_VALID (self, block);

  block_iter_obj = PyObject_New (block_syms_iterator_object,
				 &block_syms_iterator_object_type);
  if (block_iter_obj == NULL)
      return NULL;

  block_iter_obj->block = block;
  block_iter_obj->initialized_p = 0;
  Py_INCREF (self);
  block_iter_obj->source = (block_object *) self;

  return (PyObject *) block_iter_obj;
}

static PyObject *
blpy_get_start (PyObject *self, void *closure)
{
  const struct block *block = NULL;

  BLPY_REQUIRE_VALID (self, block);

  return gdb_py_object_from_ulongest (block->start ()).release ();
}

static PyObject *
blpy_get_end (PyObject *self, void *closure)
{
  const struct block *block = NULL;

  BLPY_REQUIRE_VALID (self, block);

  return gdb_py_object_from_ulongest (block->end ()).release ();
}

static PyObject *
blpy_get_function (PyObject *self, void *closure)
{
  struct symbol *sym;
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  sym = block->function ();
  if (sym)
    return symbol_to_symbol_object (sym);

  Py_RETURN_NONE;
}

static PyObject *
blpy_get_superblock (PyObject *self, void *closure)
{
  const struct block *block;
  const struct block *super_block;
  block_object *self_obj  = (block_object *) self;

  BLPY_REQUIRE_VALID (self, block);

  super_block = block->superblock ();
  if (super_block)
    return block_to_block_object (super_block, self_obj->objfile);

  Py_RETURN_NONE;
}

/* Return the global block associated to this block.  */

static PyObject *
blpy_get_global_block (PyObject *self, void *closure)
{
  const struct block *block;
  const struct block *global_block;
  block_object *self_obj  = (block_object *) self;

  BLPY_REQUIRE_VALID (self, block);

  global_block = block->global_block ();

  return block_to_block_object (global_block,
				self_obj->objfile);

}

/* Return the static block associated to this block.  Return None
   if we cannot get the static block (this is the global block).  */

static PyObject *
blpy_get_static_block (PyObject *self, void *closure)
{
  const struct block *block;
  const struct block *static_block;
  block_object *self_obj  = (block_object *) self;

  BLPY_REQUIRE_VALID (self, block);

  if (block->superblock () == NULL)
    Py_RETURN_NONE;

  static_block = block->static_block ();

  return block_to_block_object (static_block, self_obj->objfile);
}

/* Implementation of gdb.Block.is_global (self) -> Boolean.
   Returns True if this block object is a global block.  */

static PyObject *
blpy_is_global (PyObject *self, void *closure)
{
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  if (block->superblock ())
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Block.is_static (self) -> Boolean.
   Returns True if this block object is a static block.  */

static PyObject *
blpy_is_static (PyObject *self, void *closure)
{
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  if (block->superblock () != NULL
     && block->superblock ()->superblock () == NULL)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Given a string, returns the gdb.Symbol representing that symbol in this
   block.  If such a symbol does not exist, returns NULL with a Python
   exception.  */

static PyObject *
blpy_getitem (PyObject *self, PyObject *key)
{
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  gdb::unique_xmalloc_ptr<char> name = python_string_to_host_string (key);
  if (name == nullptr)
    return nullptr;

  lookup_name_info lookup_name (name.get(), symbol_name_match_type::FULL);

  /* We use an iterator instead of block_lookup_symbol so that we can
     look up symbols irrespective of the domain, matching the
     iterator. It would be confusing if the iterator returns symbols
     you can't find via getitem.  */
  for (struct symbol *sym : block_iterator_range (block, &lookup_name))
    {
      /* Just stop at the first match */
      return symbol_to_symbol_object (sym);
    }

  PyErr_SetObject (PyExc_KeyError, key);
  return nullptr;
}

static void
blpy_dealloc (PyObject *obj)
{
  block_object *block = (block_object *) obj;

  if (block->prev)
    block->prev->next = block->next;
  else if (block->objfile)
    blpy_objfile_data_key.set (block->objfile, block->next);
  if (block->next)
    block->next->prev = block->prev;
  block->block = NULL;
  Py_TYPE (obj)->tp_free (obj);
}

/* Given a block, and a block_object that has previously been
   allocated and initialized, populate the block_object with the
   struct block data.  Also, register the block_object life-cycle
   with the life-cycle of the object file associated with this
   block, if needed.  */
static void
set_block (block_object *obj, const struct block *block,
	   struct objfile *objfile)
{
  obj->block = block;
  obj->prev = NULL;
  if (objfile)
    {
      obj->objfile = objfile;
      obj->next = blpy_objfile_data_key.get (objfile);
      if (obj->next)
	obj->next->prev = obj;
      blpy_objfile_data_key.set (objfile, obj);
    }
  else
    obj->next = NULL;
}

/* Create a new block object (gdb.Block) that encapsulates the struct
   block object from GDB.  */
PyObject *
block_to_block_object (const struct block *block, struct objfile *objfile)
{
  block_object *block_obj;

  block_obj = PyObject_New (block_object, &block_object_type);
  if (block_obj)
    set_block (block_obj, block, objfile);

  return (PyObject *) block_obj;
}

/* Return struct block reference that is wrapped by this object.  */
const struct block *
block_object_to_block (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &block_object_type))
    return NULL;
  return ((block_object *) obj)->block;
}

/* Return a reference to the block iterator.  */
static PyObject *
blpy_block_syms_iter (PyObject *self)
{
  block_syms_iterator_object *iter_obj = (block_syms_iterator_object *) self;

  BLPY_ITER_REQUIRE_VALID (iter_obj->source);

  Py_INCREF (self);
  return self;
}

/* Return the next symbol in the iteration through the block's
   dictionary.  */
static PyObject *
blpy_block_syms_iternext (PyObject *self)
{
  block_syms_iterator_object *iter_obj = (block_syms_iterator_object *) self;
  struct symbol *sym;

  BLPY_ITER_REQUIRE_VALID (iter_obj->source);

  if (!iter_obj->initialized_p)
    {
      sym = block_iterator_first (iter_obj->block,  &(iter_obj->iter));
      iter_obj->initialized_p = 1;
    }
  else
    sym = block_iterator_next (&(iter_obj->iter));

  if (sym == NULL)
    {
      PyErr_SetString (PyExc_StopIteration, _("Symbol is null."));
      return NULL;
    }

  return symbol_to_symbol_object (sym);
}

static void
blpy_block_syms_dealloc (PyObject *obj)
{
  block_syms_iterator_object *iter_obj = (block_syms_iterator_object *) obj;

  Py_XDECREF (iter_obj->source);
  Py_TYPE (obj)->tp_free (obj);
}

/* Implementation of gdb.Block.is_valid (self) -> Boolean.
   Returns True if this block object still exists in GDB.  */

static PyObject *
blpy_is_valid (PyObject *self, PyObject *args)
{
  const struct block *block;

  block = block_object_to_block (self);
  if (block == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.BlockIterator.is_valid (self) -> Boolean.
   Returns True if this block iterator object still exists in GDB  */

static PyObject *
blpy_iter_is_valid (PyObject *self, PyObject *args)
{
  block_syms_iterator_object *iter_obj =
    (block_syms_iterator_object *) self;

  if (iter_obj->source->block == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* __repr__ implementation for gdb.Block.  */

static PyObject *
blpy_repr (PyObject *self)
{
  const auto block = block_object_to_block (self);
  if (block == nullptr)
    return gdb_py_invalid_object_repr (self);

  const auto name = block->function () ?
    block->function ()->print_name () : "<anonymous>";

  std::string str;
  unsigned int written_symbols = 0;
  const int len = mdict_size (block->multidict ());
  static constexpr int SYMBOLS_TO_SHOW = 5;
  for (struct symbol *symbol : block_iterator_range (block))
    {
      if (written_symbols == SYMBOLS_TO_SHOW)
	{
	  const int remaining = len - SYMBOLS_TO_SHOW;
	  if (remaining == 1)
	    str += string_printf ("... (%d more symbol)", remaining);
	  else
	    str += string_printf ("... (%d more symbols)", remaining);
	  break;
	}
      str += symbol->print_name ();
      if (++written_symbols < len)
	str += ", ";
    }
  return PyUnicode_FromFormat ("<%s %s {%s}>", Py_TYPE (self)->tp_name,
			       name, str.c_str ());
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_blocks (void)
{
  block_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&block_object_type) < 0)
    return -1;

  block_syms_iterator_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&block_syms_iterator_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "Block",
			      (PyObject *) &block_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "BlockIterator",
				 (PyObject *) &block_syms_iterator_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_blocks);



static PyMethodDef block_object_methods[] = {
  { "is_valid", blpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this block is valid, false if not." },
  {NULL}  /* Sentinel */
};

static gdb_PyGetSetDef block_object_getset[] = {
  { "start", blpy_get_start, NULL, "Start address of the block.", NULL },
  { "end", blpy_get_end, NULL, "End address of the block.", NULL },
  { "function", blpy_get_function, NULL,
    "Symbol that names the block, or None.", NULL },
  { "superblock", blpy_get_superblock, NULL,
    "Block containing the block, or None.", NULL },
  { "global_block", blpy_get_global_block, NULL,
    "Block containing the global block.", NULL },
  { "static_block", blpy_get_static_block, NULL,
    "Block containing the static block.", NULL },
  { "is_static", blpy_is_static, NULL,
    "Whether this block is a static block.", NULL },
  { "is_global", blpy_is_global, NULL,
    "Whether this block is a global block.", NULL },
  { NULL }  /* Sentinel */
};

static PyMappingMethods block_object_as_mapping = {
  NULL,
  blpy_getitem,
  NULL
};

PyTypeObject block_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Block",			  /*tp_name*/
  sizeof (block_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  blpy_dealloc,                   /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  blpy_repr,                      /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  &block_object_as_mapping,	  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB block object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  blpy_iter,			  /* tp_iter */
  0,				  /* tp_iternext */
  block_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  block_object_getset		  /* tp_getset */
};

static PyMethodDef block_iterator_object_methods[] = {
  { "is_valid", blpy_iter_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this block iterator is valid, false if not." },
  {NULL}  /* Sentinel */
};

PyTypeObject block_syms_iterator_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.BlockIterator",		  /*tp_name*/
  sizeof (block_syms_iterator_object),	      /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  blpy_block_syms_dealloc,	  /*tp_dealloc*/
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
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB block syms iterator object",	      /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  blpy_block_syms_iter,           /*tp_iter */
  blpy_block_syms_iternext,	  /*tp_iternext */
  block_iterator_object_methods   /*tp_methods */
};

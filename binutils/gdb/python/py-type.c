/* Python interface to types.

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
#include "value.h"
#include "python-internal.h"
#include "charset.h"
#include "gdbtypes.h"
#include "cp-support.h"
#include "demangle.h"
#include "objfiles.h"
#include "language.h"
#include "typeprint.h"
#include "ada-lang.h"

struct type_object
{
  PyObject_HEAD
  struct type *type;

  /* If a Type object is associated with an objfile, it is kept on a
     doubly-linked list, rooted in the objfile.  This lets us copy the
     underlying struct type when the objfile is deleted.  */
  struct type_object *prev;
  struct type_object *next;
};

extern PyTypeObject type_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("type_object");

/* A Field object.  */
struct field_object
{
  PyObject_HEAD

  /* Dictionary holding our attributes.  */
  PyObject *dict;
};

extern PyTypeObject field_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("field_object");

/* A type iterator object.  */
struct typy_iterator_object {
  PyObject_HEAD
  /* The current field index.  */
  int field;
  /* What to return.  */
  enum gdbpy_iter_kind kind;
  /* Pointer back to the original source type object.  */
  type_object *source;
};

extern PyTypeObject type_iterator_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("typy_iterator_object");

/* This is used to initialize various gdb.TYPE_ constants.  */
struct pyty_code
{
  /* The code.  */
  int code;
  /* The name.  */
  const char *name;
};

/* Forward declarations.  */
static PyObject *typy_make_iter (PyObject *self, enum gdbpy_iter_kind kind);

static struct pyty_code pyty_codes[] =
{
  /* This is kept for backward compatibility.  */
  { -1, "TYPE_CODE_BITSTRING" },

#define OP(X) { X, #X },
#include "type-codes.def"
#undef OP
};



static void
field_dealloc (PyObject *obj)
{
  field_object *f = (field_object *) obj;

  Py_XDECREF (f->dict);
  Py_TYPE (obj)->tp_free (obj);
}

static PyObject *
field_new (void)
{
  gdbpy_ref<field_object> result (PyObject_New (field_object,
						&field_object_type));

  if (result != NULL)
    {
      result->dict = PyDict_New ();
      if (!result->dict)
	return NULL;
    }
  return (PyObject *) result.release ();
}



/* Return true if OBJ is of type gdb.Field, false otherwise.  */

int
gdbpy_is_field (PyObject *obj)
{
  return PyObject_TypeCheck (obj, &field_object_type);
}

/* Return the code for this type.  */
static PyObject *
typy_get_code (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  return gdb_py_object_from_longest (type->code ()).release ();
}

/* Helper function for typy_fields which converts a single field to a
   gdb.Field object.  Returns NULL on error.  */

static gdbpy_ref<>
convert_field (struct type *type, int field)
{
  gdbpy_ref<> result (field_new ());

  if (result == NULL)
    return NULL;

  gdbpy_ref<> arg (type_to_type_object (type));
  if (arg == NULL)
    return NULL;
  if (PyObject_SetAttrString (result.get (), "parent_type", arg.get ()) < 0)
    return NULL;

  if (!type->field (field).is_static ())
    {
      const char *attrstring;

      if (type->code () == TYPE_CODE_ENUM)
	{
	  arg = gdb_py_object_from_longest (type->field (field).loc_enumval ());
	  attrstring = "enumval";
	}
      else
	{
	  if (type->field (field).loc_kind () == FIELD_LOC_KIND_DWARF_BLOCK)
	    arg = gdbpy_ref<>::new_reference (Py_None);
	  else
	    arg = gdb_py_object_from_longest (type->field (field).loc_bitpos ());
	  attrstring = "bitpos";
	}

      if (arg == NULL)
	return NULL;

      if (PyObject_SetAttrString (result.get (), attrstring, arg.get ()) < 0)
	return NULL;
    }

  arg.reset (NULL);
  if (type->field (field).name ())
    {
      const char *field_name = type->field (field).name ();

      if (field_name[0] != '\0')
	{
	  arg.reset (PyUnicode_FromString (type->field (field).name ()));
	  if (arg == NULL)
	    return NULL;
	}
    }
  if (arg == NULL)
    arg = gdbpy_ref<>::new_reference (Py_None);

  if (PyObject_SetAttrString (result.get (), "name", arg.get ()) < 0)
    return NULL;

  arg.reset (PyBool_FromLong (type->field (field).is_artificial ()));
  if (PyObject_SetAttrString (result.get (), "artificial", arg.get ()) < 0)
    return NULL;

  if (type->code () == TYPE_CODE_STRUCT)
    arg.reset (PyBool_FromLong (field < TYPE_N_BASECLASSES (type)));
  else
    arg = gdbpy_ref<>::new_reference (Py_False);
  if (PyObject_SetAttrString (result.get (), "is_base_class", arg.get ()) < 0)
    return NULL;

  arg = gdb_py_object_from_longest (type->field (field).bitsize ());
  if (arg == NULL)
    return NULL;
  if (PyObject_SetAttrString (result.get (), "bitsize", arg.get ()) < 0)
    return NULL;

  /* A field can have a NULL type in some situations.  */
  if (type->field (field).type () == NULL)
    arg = gdbpy_ref<>::new_reference (Py_None);
  else
    arg.reset (type_to_type_object (type->field (field).type ()));
  if (arg == NULL)
    return NULL;
  if (PyObject_SetAttrString (result.get (), "type", arg.get ()) < 0)
    return NULL;

  return result;
}

/* Helper function to return the name of a field, as a gdb.Field object.
   If the field doesn't have a name, None is returned.  */

static gdbpy_ref<>
field_name (struct type *type, int field)
{
  gdbpy_ref<> result;

  if (type->field (field).name ())
    result.reset (PyUnicode_FromString (type->field (field).name ()));
  else
    result = gdbpy_ref<>::new_reference (Py_None);

  return result;
}

/* Helper function for Type standard mapping methods.  Returns a
   Python object for field i of the type.  "kind" specifies what to
   return: the name of the field, a gdb.Field object corresponding to
   the field, or a tuple consisting of field name and gdb.Field
   object.  */

static gdbpy_ref<>
make_fielditem (struct type *type, int i, enum gdbpy_iter_kind kind)
{
  switch (kind)
    {
    case iter_items:
      {
	gdbpy_ref<> key (field_name (type, i));
	if (key == NULL)
	  return NULL;
	gdbpy_ref<> value = convert_field (type, i);
	if (value == NULL)
	  return NULL;
	gdbpy_ref<> item (PyTuple_New (2));
	if (item == NULL)
	  return NULL;
	PyTuple_SET_ITEM (item.get (), 0, key.release ());
	PyTuple_SET_ITEM (item.get (), 1, value.release ());
	return item;
      }
    case iter_keys:
      return field_name (type, i);
    case iter_values:
      return convert_field (type, i);
    }
  gdb_assert_not_reached ("invalid gdbpy_iter_kind");
}

/* Return a sequence of all field names, fields, or (name, field) pairs.
   Each field is a gdb.Field object.  */

static PyObject *
typy_fields_items (PyObject *self, enum gdbpy_iter_kind kind)
{
  PyObject *py_type = self;
  struct type *type = ((type_object *) py_type)->type;
  struct type *checked_type = type;

  try
    {
      checked_type = check_typedef (checked_type);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  gdbpy_ref<> type_holder;
  if (checked_type != type)
    {
      type_holder.reset (type_to_type_object (checked_type));
      if (type_holder == nullptr)
	return nullptr;
      py_type = type_holder.get ();
    }
  gdbpy_ref<> iter (typy_make_iter (py_type, kind));
  if (iter == nullptr)
    return nullptr;

  return PySequence_List (iter.get ());
}

/* Return a sequence of all fields.  Each field is a gdb.Field object.  */

static PyObject *
typy_values (PyObject *self, PyObject *args)
{
  return typy_fields_items (self, iter_values);
}

/* Return a sequence of all fields.  Each field is a gdb.Field object.
   This method is similar to typy_values, except where the supplied
   gdb.Type is an array, in which case it returns a list of one entry
   which is a gdb.Field object for a range (the array bounds).  */

static PyObject *
typy_fields (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  if (type->code () != TYPE_CODE_ARRAY)
    return typy_fields_items (self, iter_values);

  /* Array type.  Handle this as a special case because the common
     machinery wants struct or union or enum types.  Build a list of
     one entry which is the range for the array.  */
  gdbpy_ref<> r = convert_field (type, 0);
  if (r == NULL)
    return NULL;

  return Py_BuildValue ("[O]", r.get ());
}

/* Return a sequence of all field names.  Each field is a gdb.Field object.  */

static PyObject *
typy_field_names (PyObject *self, PyObject *args)
{
  return typy_fields_items (self, iter_keys);
}

/* Return a sequence of all (name, fields) pairs.  Each field is a
   gdb.Field object.  */

static PyObject *
typy_items (PyObject *self, PyObject *args)
{
  return typy_fields_items (self, iter_items);
}

/* Return the type's name, or None.  */

static PyObject *
typy_get_name (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  if (type->name () == NULL)
    Py_RETURN_NONE;
  /* Ada type names are encoded, but it is better for users to see the
     decoded form.  */
  if (ADA_TYPE_P (type))
    {
      std::string name = ada_decode (type->name (), false);
      if (!name.empty ())
	return PyUnicode_FromString (name.c_str ());
    }
  return PyUnicode_FromString (type->name ());
}

/* Return the type's tag, or None.  */
static PyObject *
typy_get_tag (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;
  const char *tagname = nullptr;

  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_ENUM)
    tagname = type->name ();

  if (tagname == nullptr)
    Py_RETURN_NONE;
  return PyUnicode_FromString (tagname);
}

/* Return the type's objfile, or None.  */
static PyObject *
typy_get_objfile (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;
  struct objfile *objfile = type->objfile_owner ();

  if (objfile == nullptr)
    Py_RETURN_NONE;
  return objfile_to_objfile_object (objfile).release ();
}

/* Return true if this is a scalar type, otherwise, returns false.  */

static PyObject *
typy_is_scalar (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  if (is_scalar_type (type))
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

/* Return true if this type is signed.  Raises a ValueError if this type
   is not a scalar type.  */

static PyObject *
typy_is_signed (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  if (!is_scalar_type (type))
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Type must be a scalar type"));
      return nullptr;
    }

  if (type->is_unsigned ())
    Py_RETURN_FALSE;
  else
    Py_RETURN_TRUE;
}

/* Return true if this type is array-like.  */

static PyObject *
typy_is_array_like (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;
  bool result = false;

  try
    {
      type = check_typedef (type);
      result = type->is_array_like ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (result)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

/* Return true if this type is string-like.  */

static PyObject *
typy_is_string_like (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;
  bool result = false;

  try
    {
      type = check_typedef (type);
      result = type->is_string_like ();
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (result)
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

/* Return the type, stripped of typedefs. */
static PyObject *
typy_strip_typedefs (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  try
    {
      type = check_typedef (type);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (type);
}

/* Strip typedefs and pointers/reference from a type.  Then check that
   it is a struct, union, or enum type.  If not, raise TypeError.  */

static struct type *
typy_get_composite (struct type *type)
{

  for (;;)
    {
      try
	{
	  type = check_typedef (type);
	}
      catch (const gdb_exception &except)
	{
	  GDB_PY_HANDLE_EXCEPTION (except);
	}

      if (!type->is_pointer_or_reference ())
	break;
      type = type->target_type ();
    }

  /* If this is not a struct, union, or enum type, raise TypeError
     exception.  */
  if (type->code () != TYPE_CODE_STRUCT
      && type->code () != TYPE_CODE_UNION
      && type->code () != TYPE_CODE_ENUM
      && type->code () != TYPE_CODE_METHOD
      && type->code () != TYPE_CODE_FUNC)
    {
      PyErr_SetString (PyExc_TypeError,
		       "Type is not a structure, union, enum, or function type.");
      return NULL;
    }

  return type;
}

/* Helper for typy_array and typy_vector.  */

static PyObject *
typy_array_1 (PyObject *self, PyObject *args, int is_vector)
{
  long n1, n2;
  PyObject *n2_obj = NULL;
  struct type *array = NULL;
  struct type *type = ((type_object *) self)->type;

  if (! PyArg_ParseTuple (args, "l|O", &n1, &n2_obj))
    return NULL;

  if (n2_obj)
    {
      if (!PyLong_Check (n2_obj))
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Array bound must be an integer"));
	  return NULL;
	}

      if (! gdb_py_int_as_long (n2_obj, &n2))
	return NULL;
    }
  else
    {
      n2 = n1;
      n1 = 0;
    }

  if (n2 < n1 - 1) /* Note: An empty array has n2 == n1 - 1.  */
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Array length must not be negative"));
      return NULL;
    }

  try
    {
      array = lookup_array_range_type (type, n1, n2);
      if (is_vector)
	make_vector_type (array);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (array);
}

/* Return an array type.  */

static PyObject *
typy_array (PyObject *self, PyObject *args)
{
  return typy_array_1 (self, args, 0);
}

/* Return a vector type.  */

static PyObject *
typy_vector (PyObject *self, PyObject *args)
{
  return typy_array_1 (self, args, 1);
}

/* Return a Type object which represents a pointer to SELF.  */
static PyObject *
typy_pointer (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  try
    {
      type = lookup_pointer_type (type);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (type);
}

/* Return the range of a type represented by SELF.  The return type is
   a tuple.  The first element of the tuple contains the low bound,
   while the second element of the tuple contains the high bound.  */
static PyObject *
typy_range (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;
  /* Initialize these to appease GCC warnings.  */
  LONGEST low = 0, high = 0;

  if (type->code () != TYPE_CODE_ARRAY
      && type->code () != TYPE_CODE_STRING
      && type->code () != TYPE_CODE_RANGE)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("This type does not have a range."));
      return NULL;
    }

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRING:
    case TYPE_CODE_RANGE:
      if (type->bounds ()->low.is_constant ())
	low = type->bounds ()->low.const_val ();
      else
	low = 0;

      if (type->bounds ()->high.is_constant ())
	high = type->bounds ()->high.const_val ();
      else
	high = 0;
      break;
    }

  gdbpy_ref<> low_bound = gdb_py_object_from_longest (low);
  if (low_bound == NULL)
    return NULL;

  gdbpy_ref<> high_bound = gdb_py_object_from_longest (high);
  if (high_bound == NULL)
    return NULL;

  gdbpy_ref<> result (PyTuple_New (2));
  if (result == NULL)
    return NULL;

  if (PyTuple_SetItem (result.get (), 0, low_bound.release ()) != 0
      || PyTuple_SetItem (result.get (), 1, high_bound.release ()) != 0)
    return NULL;
  return result.release ();
}

/* Return a Type object which represents a reference to SELF.  */
static PyObject *
typy_reference (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  try
    {
      type = lookup_lvalue_reference_type (type);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (type);
}

/* Return a Type object which represents the target type of SELF.  */
static PyObject *
typy_target (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  if (!type->target_type ())
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Type does not have a target."));
      return NULL;
    }

  return type_to_type_object (type->target_type ());
}

/* Return a const-qualified type variant.  */
static PyObject *
typy_const (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  try
    {
      type = make_cv_type (1, 0, type, NULL);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (type);
}

/* Return a volatile-qualified type variant.  */
static PyObject *
typy_volatile (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  try
    {
      type = make_cv_type (0, 1, type, NULL);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (type);
}

/* Return an unqualified type variant.  */
static PyObject *
typy_unqualified (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  try
    {
      type = make_cv_type (0, 0, type, NULL);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type_to_type_object (type);
}

/* Return the size of the type represented by SELF, in bytes.  */
static PyObject *
typy_get_sizeof (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  bool size_varies = false;
  try
    {
      check_typedef (type);

      size_varies = TYPE_HAS_DYNAMIC_LENGTH (type);
    }
  catch (const gdb_exception &except)
    {
    }

  /* Ignore exceptions.  */

  if (size_varies)
    Py_RETURN_NONE;
  return gdb_py_object_from_longest (type->length ()).release ();
}

/* Return the alignment of the type represented by SELF, in bytes.  */
static PyObject *
typy_get_alignof (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  ULONGEST align = 0;
  try
    {
      align = type_align (type);
    }
  catch (const gdb_exception &except)
    {
      align = 0;
    }

  /* Ignore exceptions.  */

  return gdb_py_object_from_ulongest (align).release ();
}

/* Return whether or not the type is dynamic.  */
static PyObject *
typy_get_dynamic (PyObject *self, void *closure)
{
  struct type *type = ((type_object *) self)->type;

  bool result = false;
  try
    {
      result = is_dynamic_type (type);
    }
  catch (const gdb_exception &except)
    {
      /* Ignore exceptions.  */
    }

  if (result)
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static struct type *
typy_lookup_typename (const char *type_name, const struct block *block)
{
  struct type *type = NULL;

  try
    {
      if (startswith (type_name, "struct "))
	type = lookup_struct (type_name + 7, NULL);
      else if (startswith (type_name, "union "))
	type = lookup_union (type_name + 6, NULL);
      else if (startswith (type_name, "enum "))
	type = lookup_enum (type_name + 5, NULL);
      else
	type = lookup_typename (current_language,
				type_name, block, 0);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return type;
}

static struct type *
typy_lookup_type (struct demangle_component *demangled,
		  const struct block *block)
{
  struct type *type, *rtype = NULL;
  enum demangle_component_type demangled_type;

  /* Save the type: typy_lookup_type() may (indirectly) overwrite
     memory pointed by demangled.  */
  demangled_type = demangled->type;

  if (demangled_type == DEMANGLE_COMPONENT_POINTER
      || demangled_type == DEMANGLE_COMPONENT_REFERENCE
      || demangled_type == DEMANGLE_COMPONENT_RVALUE_REFERENCE
      || demangled_type == DEMANGLE_COMPONENT_CONST
      || demangled_type == DEMANGLE_COMPONENT_VOLATILE)
    {
      type = typy_lookup_type (demangled->u.s_binary.left, block);
      if (! type)
	return NULL;

      try
	{
	  /* If the demangled_type matches with one of the types
	     below, run the corresponding function and save the type
	     to return later.  We cannot just return here as we are in
	     an exception handler.  */
	  switch (demangled_type)
	    {
	    case DEMANGLE_COMPONENT_REFERENCE:
	      rtype = lookup_lvalue_reference_type (type);
	      break;
	    case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
	      rtype = lookup_rvalue_reference_type (type);
	      break;
	    case DEMANGLE_COMPONENT_POINTER:
	      rtype = lookup_pointer_type (type);
	      break;
	    case DEMANGLE_COMPONENT_CONST:
	      rtype = make_cv_type (1, 0, type, NULL);
	      break;
	    case DEMANGLE_COMPONENT_VOLATILE:
	      rtype = make_cv_type (0, 1, type, NULL);
	      break;
	    }
	}
      catch (const gdb_exception &except)
	{
	  GDB_PY_HANDLE_EXCEPTION (except);
	}
    }

  /* If we have a type from the switch statement above, just return
     that.  */
  if (rtype)
    return rtype;

  /* We don't have a type, so lookup the type.  */
  gdb::unique_xmalloc_ptr<char> type_name = cp_comp_to_string (demangled, 10);
  return typy_lookup_typename (type_name.get (), block);
}

/* This is a helper function for typy_template_argument that is used
   when the type does not have template symbols attached.  It works by
   parsing the type name.  This happens with compilers, like older
   versions of GCC, that do not emit DW_TAG_template_*.  */

static PyObject *
typy_legacy_template_argument (struct type *type, const struct block *block,
			       int argno)
{
  int i;
  struct demangle_component *demangled;
  std::unique_ptr<demangle_parse_info> info;
  std::string err;
  struct type *argtype;

  if (type->name () == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError, _("Null type name."));
      return NULL;
    }

  try
    {
      /* Note -- this is not thread-safe.  */
      info = cp_demangled_name_to_comp (type->name (), &err);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (! info)
    {
      PyErr_SetString (PyExc_RuntimeError, err.c_str ());
      return NULL;
    }
  demangled = info->tree;

  /* Strip off component names.  */
  while (demangled->type == DEMANGLE_COMPONENT_QUAL_NAME
	 || demangled->type == DEMANGLE_COMPONENT_LOCAL_NAME)
    demangled = demangled->u.s_binary.right;

  if (demangled->type != DEMANGLE_COMPONENT_TEMPLATE)
    {
      PyErr_SetString (PyExc_RuntimeError, _("Type is not a template."));
      return NULL;
    }

  /* Skip from the template to the arguments.  */
  demangled = demangled->u.s_binary.right;

  for (i = 0; demangled && i < argno; ++i)
    demangled = demangled->u.s_binary.right;

  if (! demangled)
    {
      PyErr_Format (PyExc_RuntimeError, _("No argument %d in template."),
		    argno);
      return NULL;
    }

  argtype = typy_lookup_type (demangled->u.s_binary.left, block);
  if (! argtype)
    return NULL;

  return type_to_type_object (argtype);
}

static PyObject *
typy_template_argument (PyObject *self, PyObject *args)
{
  int argno;
  struct type *type = ((type_object *) self)->type;
  const struct block *block = NULL;
  PyObject *block_obj = NULL;
  struct symbol *sym;

  if (! PyArg_ParseTuple (args, "i|O", &argno, &block_obj))
    return NULL;

  if (argno < 0)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Template argument number must be non-negative"));
      return NULL;
    }

  if (block_obj)
    {
      block = block_object_to_block (block_obj);
      if (! block)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Second argument must be block."));
	  return NULL;
	}
    }

  try
    {
      type = check_typedef (type);
      if (TYPE_IS_REFERENCE (type))
	type = check_typedef (type->target_type ());
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  /* We might not have DW_TAG_template_*, so try to parse the type's
     name.  This is inefficient if we do not have a template type --
     but that is going to wind up as an error anyhow.  */
  if (! TYPE_N_TEMPLATE_ARGUMENTS (type))
    return typy_legacy_template_argument (type, block, argno);

  if (argno >= TYPE_N_TEMPLATE_ARGUMENTS (type))
    {
      PyErr_Format (PyExc_RuntimeError, _("No argument %d in template."),
		    argno);
      return NULL;
    }

  sym = TYPE_TEMPLATE_ARGUMENT (type, argno);
  if (sym->aclass () == LOC_TYPEDEF)
    return type_to_type_object (sym->type ());
  else if (sym->aclass () == LOC_OPTIMIZED_OUT)
    {
      PyErr_Format (PyExc_RuntimeError,
		    _("Template argument is optimized out"));
      return NULL;
    }

  PyObject *result = nullptr;
  try
    {
      scoped_value_mark free_values;
      struct value *val = value_of_variable (sym, block);
      result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* __repr__ implementation for gdb.Type.  */

static PyObject *
typy_repr (PyObject *self)
{
  const auto type = type_object_to_type (self);
  if (type == nullptr)
    return gdb_py_invalid_object_repr (self);

  const char *code = pyty_codes[type->code ()].name;
  string_file type_name;
  try
    {
      current_language->print_type (type, "", &type_name, -1, 0,
				    &type_print_raw_options);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }
  auto py_typename = PyUnicode_Decode (type_name.c_str (), type_name.size (),
				       host_charset (), NULL);

  return PyUnicode_FromFormat ("<%s code=%s name=%U>", Py_TYPE (self)->tp_name,
			       code, py_typename);
}

static PyObject *
typy_str (PyObject *self)
{
  string_file thetype;

  try
    {
      current_language->print_type (type_object_to_type (self), "",
				    &thetype, -1, 0,
				    &type_print_raw_options);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyUnicode_Decode (thetype.c_str (), thetype.size (),
			   host_charset (), NULL);
}

/* Implement the richcompare method.  */

static PyObject *
typy_richcompare (PyObject *self, PyObject *other, int op)
{
  bool result = false;
  struct type *type1 = type_object_to_type (self);
  struct type *type2 = type_object_to_type (other);

  /* We can only compare ourselves to another Type object, and only
     for equality or inequality.  */
  if (type2 == NULL || (op != Py_EQ && op != Py_NE))
    {
      Py_INCREF (Py_NotImplemented);
      return Py_NotImplemented;
    }

  if (type1 == type2)
    result = true;
  else
    {
      try
	{
	  result = types_deeply_equal (type1, type2);
	}
      catch (const gdb_exception &except)
	{
	  /* If there is a GDB exception, a comparison is not capable
	     (or trusted), so exit.  */
	  GDB_PY_HANDLE_EXCEPTION (except);
	}
    }

  if (op == (result ? Py_EQ : Py_NE))
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}



/* Deleter that saves types when an objfile is being destroyed.  */
struct typy_deleter
{
  void operator() (type_object *obj)
  {
    if (!gdb_python_initialized)
      return;

    /* This prevents another thread from freeing the objects we're
       operating on.  */
    gdbpy_enter enter_py;

    htab_up copied_types = create_copied_types_hash ();

    while (obj)
      {
	type_object *next = obj->next;

	htab_empty (copied_types.get ());

	obj->type = copy_type_recursive (obj->type, copied_types.get ());

	obj->next = NULL;
	obj->prev = NULL;

	obj = next;
      }
  }
};

static const registry<objfile>::key<type_object, typy_deleter>
     typy_objfile_data_key;

static void
set_type (type_object *obj, struct type *type)
{
  obj->type = type;
  obj->prev = NULL;
  if (type != nullptr && type->objfile_owner () != nullptr)
    {
      struct objfile *objfile = type->objfile_owner ();

      obj->next = typy_objfile_data_key.get (objfile);
      if (obj->next)
	obj->next->prev = obj;
      typy_objfile_data_key.set (objfile, obj);
    }
  else
    obj->next = NULL;
}

static void
typy_dealloc (PyObject *obj)
{
  type_object *type = (type_object *) obj;

  if (type->prev)
    type->prev->next = type->next;
  else if (type->type != nullptr && type->type->objfile_owner () != nullptr)
    {
      /* Must reset head of list.  */
      struct objfile *objfile = type->type->objfile_owner ();

      if (objfile)
	typy_objfile_data_key.set (objfile, type->next);
    }
  if (type->next)
    type->next->prev = type->prev;

  Py_TYPE (type)->tp_free (type);
}

/* Return number of fields ("length" of the field dictionary).  */

static Py_ssize_t
typy_length (PyObject *self)
{
  struct type *type = ((type_object *) self)->type;

  type = typy_get_composite (type);
  if (type == NULL)
    return -1;

  return type->num_fields ();
}

/* Implements boolean evaluation of gdb.Type.  Handle this like other
   Python objects that don't have a meaningful truth value -- all
   values are true.  */

static int
typy_nonzero (PyObject *self)
{
  return 1;
}

/* Return optimized out value of this type.  */

static PyObject *
typy_optimized_out (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;

  scoped_value_mark free_values;
  return value_to_value_object (value::allocate_optimized_out (type));
}

/* Return a gdb.Field object for the field named by the argument.  */

static PyObject *
typy_getitem (PyObject *self, PyObject *key)
{
  struct type *type = ((type_object *) self)->type;
  int i;

  gdb::unique_xmalloc_ptr<char> field = python_string_to_host_string (key);
  if (field == NULL)
    return NULL;

  /* We want just fields of this type, not of base types, so instead of
     using lookup_struct_elt_type, portions of that function are
     copied here.  */

  type = typy_get_composite (type);
  if (type == NULL)
    return NULL;

  for (i = 0; i < type->num_fields (); i++)
    {
      const char *t_field_name = type->field (i).name ();

      if (t_field_name && (strcmp_iw (t_field_name, field.get ()) == 0))
	return convert_field (type, i).release ();
    }
  PyErr_SetObject (PyExc_KeyError, key);
  return NULL;
}

/* Implement the "get" method on the type object.  This is the
   same as getitem if the key is present, but returns the supplied
   default value or None if the key is not found.  */

static PyObject *
typy_get (PyObject *self, PyObject *args)
{
  PyObject *key, *defval = Py_None, *result;

  if (!PyArg_UnpackTuple (args, "get", 1, 2, &key, &defval))
    return NULL;

  result = typy_getitem (self, key);
  if (result != NULL)
    return result;

  /* typy_getitem returned error status.  If the exception is
     KeyError, clear the exception status and return the defval
     instead.  Otherwise return the exception unchanged.  */
  if (!PyErr_ExceptionMatches (PyExc_KeyError))
    return NULL;

  PyErr_Clear ();
  Py_INCREF (defval);
  return defval;
}

/* Implement the "has_key" method on the type object.  */

static PyObject *
typy_has_key (PyObject *self, PyObject *args)
{
  struct type *type = ((type_object *) self)->type;
  const char *field;
  int i;

  if (!PyArg_ParseTuple (args, "s", &field))
    return NULL;

  /* We want just fields of this type, not of base types, so instead of
     using lookup_struct_elt_type, portions of that function are
     copied here.  */

  type = typy_get_composite (type);
  if (type == NULL)
    return NULL;

  for (i = 0; i < type->num_fields (); i++)
    {
      const char *t_field_name = type->field (i).name ();

      if (t_field_name && (strcmp_iw (t_field_name, field) == 0))
	Py_RETURN_TRUE;
    }
  Py_RETURN_FALSE;
}

/* Make an iterator object to iterate over keys, values, or items.  */

static PyObject *
typy_make_iter (PyObject *self, enum gdbpy_iter_kind kind)
{
  typy_iterator_object *typy_iter_obj;

  /* Check that "self" is a structure or union type.  */
  if (typy_get_composite (((type_object *) self)->type) == NULL)
    return NULL;

  typy_iter_obj = PyObject_New (typy_iterator_object,
				&type_iterator_object_type);
  if (typy_iter_obj == NULL)
      return NULL;

  typy_iter_obj->field = 0;
  typy_iter_obj->kind = kind;
  Py_INCREF (self);
  typy_iter_obj->source = (type_object *) self;

  return (PyObject *) typy_iter_obj;
}

/* iteritems() method.  */

static PyObject *
typy_iteritems (PyObject *self, PyObject *args)
{
  return typy_make_iter (self, iter_items);
}

/* iterkeys() method.  */

static PyObject *
typy_iterkeys (PyObject *self, PyObject *args)
{
  return typy_make_iter (self, iter_keys);
}

/* Iterating over the class, same as iterkeys except for the function
   signature.  */

static PyObject *
typy_iter (PyObject *self)
{
  return typy_make_iter (self, iter_keys);
}

/* itervalues() method.  */

static PyObject *
typy_itervalues (PyObject *self, PyObject *args)
{
  return typy_make_iter (self, iter_values);
}

/* Return a reference to the type iterator.  */

static PyObject *
typy_iterator_iter (PyObject *self)
{
  Py_INCREF (self);
  return self;
}

/* Return the next field in the iteration through the list of fields
   of the type.  */

static PyObject *
typy_iterator_iternext (PyObject *self)
{
  typy_iterator_object *iter_obj = (typy_iterator_object *) self;
  struct type *type = iter_obj->source->type;

  if (iter_obj->field < type->num_fields ())
    {
      gdbpy_ref<> result = make_fielditem (type, iter_obj->field,
					   iter_obj->kind);
      if (result != NULL)
	iter_obj->field++;
      return result.release ();
    }

  return NULL;
}

static void
typy_iterator_dealloc (PyObject *obj)
{
  typy_iterator_object *iter_obj = (typy_iterator_object *) obj;

  Py_DECREF (iter_obj->source);
  Py_TYPE (obj)->tp_free (obj);
}

/* Create a new Type referring to TYPE.  */
PyObject *
type_to_type_object (struct type *type)
{
  type_object *type_obj;

  try
    {
      /* Try not to let stub types leak out to Python.  */
      if (type->is_stub ())
	type = check_typedef (type);
    }
  catch (...)
    {
      /* Just ignore failures in check_typedef.  */
    }

  type_obj = PyObject_New (type_object, &type_object_type);
  if (type_obj)
    set_type (type_obj, type);

  return (PyObject *) type_obj;
}

struct type *
type_object_to_type (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &type_object_type))
    return NULL;
  return ((type_object *) obj)->type;
}



/* Implementation of gdb.lookup_type.  */
PyObject *
gdbpy_lookup_type (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", "block", NULL };
  const char *type_name = NULL;
  struct type *type = NULL;
  PyObject *block_obj = NULL;
  const struct block *block = NULL;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O", keywords,
					&type_name, &block_obj))
    return NULL;

  if (block_obj)
    {
      block = block_object_to_block (block_obj);
      if (! block)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("'block' argument must be a Block."));
	  return NULL;
	}
    }

  type = typy_lookup_typename (type_name, block);
  if (! type)
    return NULL;

  return type_to_type_object (type);
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_types (void)
{
  if (PyType_Ready (&type_object_type) < 0)
    return -1;
  if (PyType_Ready (&field_object_type) < 0)
    return -1;
  if (PyType_Ready (&type_iterator_object_type) < 0)
    return -1;

  for (const auto &item : pyty_codes)
    {
      if (PyModule_AddIntConstant (gdb_module, item.name, item.code) < 0)
	return -1;
    }

  if (gdb_pymodule_addobject (gdb_module, "Type",
			      (PyObject *) &type_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_module, "TypeIterator",
			      (PyObject *) &type_iterator_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Field",
				 (PyObject *) &field_object_type);
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_types);



static gdb_PyGetSetDef type_object_getset[] =
{
  { "alignof", typy_get_alignof, NULL,
    "The alignment of this type, in bytes.", NULL },
  { "code", typy_get_code, NULL,
    "The code for this type.", NULL },
  { "dynamic", typy_get_dynamic, NULL,
    "Whether this type is dynamic.", NULL },
  { "name", typy_get_name, NULL,
    "The name for this type, or None.", NULL },
  { "sizeof", typy_get_sizeof, NULL,
    "The size of this type, in bytes.", NULL },
  { "tag", typy_get_tag, NULL,
    "The tag name for this type, or None.", NULL },
  { "objfile", typy_get_objfile, NULL,
    "The objfile this type was defined in, or None.", NULL },
  { "is_scalar", typy_is_scalar, nullptr,
    "Is this a scalar type?", nullptr },
  { "is_signed", typy_is_signed, nullptr,
    "Is this a signed type?", nullptr },
  { "is_array_like", typy_is_array_like, nullptr,
    "Is this an array-like type?", nullptr },
  { "is_string_like", typy_is_string_like, nullptr,
    "Is this a string-like type?", nullptr },
  { NULL }
};

static PyMethodDef type_object_methods[] =
{
  { "array", typy_array, METH_VARARGS,
    "array ([LOW_BOUND,] HIGH_BOUND) -> Type\n\
Return a type which represents an array of objects of this type.\n\
The bounds of the array are [LOW_BOUND, HIGH_BOUND] inclusive.\n\
If LOW_BOUND is omitted, a value of zero is used." },
  { "vector", typy_vector, METH_VARARGS,
    "vector ([LOW_BOUND,] HIGH_BOUND) -> Type\n\
Return a type which represents a vector of objects of this type.\n\
The bounds of the array are [LOW_BOUND, HIGH_BOUND] inclusive.\n\
If LOW_BOUND is omitted, a value of zero is used.\n\
Vectors differ from arrays in that if the current language has C-style\n\
arrays, vectors don't decay to a pointer to the first element.\n\
They are first class values." },
   { "__contains__", typy_has_key, METH_VARARGS,
     "T.__contains__(k) -> True if T has a field named k, else False" },
  { "const", typy_const, METH_NOARGS,
    "const () -> Type\n\
Return a const variant of this type." },
  { "optimized_out", typy_optimized_out, METH_NOARGS,
    "optimized_out() -> Value\n\
Return optimized out value of this type." },
  { "fields", typy_fields, METH_NOARGS,
    "fields () -> list\n\
Return a list holding all the fields of this type.\n\
Each field is a gdb.Field object." },
  { "get", typy_get, METH_VARARGS,
    "T.get(k[,default]) -> returns field named k in T, if it exists;\n\
otherwise returns default, if supplied, or None if not." },
  { "has_key", typy_has_key, METH_VARARGS,
    "T.has_key(k) -> True if T has a field named k, else False" },
  { "items", typy_items, METH_NOARGS,
    "items () -> list\n\
Return a list of (name, field) pairs of this type.\n\
Each field is a gdb.Field object." },
  { "iteritems", typy_iteritems, METH_NOARGS,
    "iteritems () -> an iterator over the (name, field)\n\
pairs of this type.  Each field is a gdb.Field object." },
  { "iterkeys", typy_iterkeys, METH_NOARGS,
    "iterkeys () -> an iterator over the field names of this type." },
  { "itervalues", typy_itervalues, METH_NOARGS,
    "itervalues () -> an iterator over the fields of this type.\n\
Each field is a gdb.Field object." },
  { "keys", typy_field_names, METH_NOARGS,
    "keys () -> list\n\
Return a list holding all the fields names of this type." },
  { "pointer", typy_pointer, METH_NOARGS,
    "pointer () -> Type\n\
Return a type of pointer to this type." },
  { "range", typy_range, METH_NOARGS,
    "range () -> tuple\n\
Return a tuple containing the lower and upper range for this type."},
  { "reference", typy_reference, METH_NOARGS,
    "reference () -> Type\n\
Return a type of reference to this type." },
  { "strip_typedefs", typy_strip_typedefs, METH_NOARGS,
    "strip_typedefs () -> Type\n\
Return a type formed by stripping this type of all typedefs."},
  { "target", typy_target, METH_NOARGS,
    "target () -> Type\n\
Return the target type of this type." },
  { "template_argument", typy_template_argument, METH_VARARGS,
    "template_argument (arg, [block]) -> Type\n\
Return the type of a template argument." },
  { "unqualified", typy_unqualified, METH_NOARGS,
    "unqualified () -> Type\n\
Return a variant of this type without const or volatile attributes." },
  { "values", typy_values, METH_NOARGS,
    "values () -> list\n\
Return a list holding all the fields of this type.\n\
Each field is a gdb.Field object." },
  { "volatile", typy_volatile, METH_NOARGS,
    "volatile () -> Type\n\
Return a volatile variant of this type" },
  { NULL }
};

static PyNumberMethods type_object_as_number = {
  NULL,			      /* nb_add */
  NULL,			      /* nb_subtract */
  NULL,			      /* nb_multiply */
  NULL,			      /* nb_remainder */
  NULL,			      /* nb_divmod */
  NULL,			      /* nb_power */
  NULL,			      /* nb_negative */
  NULL,			      /* nb_positive */
  NULL,			      /* nb_absolute */
  typy_nonzero,		      /* nb_nonzero */
  NULL,			      /* nb_invert */
  NULL,			      /* nb_lshift */
  NULL,			      /* nb_rshift */
  NULL,			      /* nb_and */
  NULL,			      /* nb_xor */
  NULL,			      /* nb_or */
  NULL,			      /* nb_int */
  NULL,			      /* reserved */
  NULL,			      /* nb_float */
};

static PyMappingMethods typy_mapping = {
  typy_length,
  typy_getitem,
  NULL				  /* no "set" method */
};

PyTypeObject type_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Type",			  /*tp_name*/
  sizeof (type_object),		  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  typy_dealloc,			  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  typy_repr,                     /*tp_repr*/
  &type_object_as_number,	  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  &typy_mapping,		  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  typy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB type object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  typy_richcompare,		  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  typy_iter,			  /* tp_iter */
  0,				  /* tp_iternext */
  type_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  type_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  0,				  /* tp_new */
};

static gdb_PyGetSetDef field_object_getset[] =
{
  { "__dict__", gdb_py_generic_dict, NULL,
    "The __dict__ for this field.", &field_object_type },
  { NULL }
};

PyTypeObject field_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Field",			  /*tp_name*/
  sizeof (field_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  field_dealloc,		  /*tp_dealloc*/
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
  "GDB field object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  0,				  /* tp_methods */
  0,				  /* tp_members */
  field_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  offsetof (field_object, dict),  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  0,				  /* tp_new */
};

PyTypeObject type_iterator_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.TypeIterator",		  /*tp_name*/
  sizeof (typy_iterator_object),  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  typy_iterator_dealloc,	  /*tp_dealloc*/
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
  "GDB type iterator object",	  /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  typy_iterator_iter,             /*tp_iter */
  typy_iterator_iternext,	  /*tp_iternext */
  0				  /*tp_methods */
};

/* Support for debug methods in Python.

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
#include "arch-utils.h"
#include "extension-priv.h"
#include "objfiles.h"
#include "value.h"
#include "language.h"

#include "python.h"
#include "python-internal.h"

static const char enabled_field_name[] = "enabled";
static const char match_method_name[] = "match";
static const char get_arg_types_method_name[] = "get_arg_types";
static const char get_result_type_method_name[] = "get_result_type";
static const char matchers_attr_str[] = "xmethods";

static PyObject *py_match_method_name = NULL;
static PyObject *py_get_arg_types_method_name = NULL;

struct python_xmethod_worker : xmethod_worker
{
  python_xmethod_worker (PyObject *worker, PyObject *this_type);
  ~python_xmethod_worker ();

  DISABLE_COPY_AND_ASSIGN (python_xmethod_worker);

  /* Implementation of xmethod_worker::invoke for Python.  */

  value *invoke (value *obj, gdb::array_view<value *> args) override;

  /* Implementation of xmethod_worker::do_get_arg_types for Python.  */

  ext_lang_rc do_get_arg_types (std::vector<type *> *type_args) override;

  /* Implementation of xmethod_worker::do_get_result_type for Python.

     For backward compatibility with 7.9, which did not support getting the
     result type, if the get_result_type operation is not provided by WORKER
     then EXT_LANG_RC_OK is returned and NULL is returned in *RESULT_TYPE.  */

  ext_lang_rc do_get_result_type (value *obj, gdb::array_view<value *> args,
				  type **result_type_ptr) override;

private:

  PyObject *m_py_worker;
  PyObject *m_this_type;
};

python_xmethod_worker::~python_xmethod_worker ()
{
  /* We don't do much here, but we still need the GIL.  */
  gdbpy_enter enter_py;

  Py_DECREF (m_py_worker);
  Py_DECREF (m_this_type);
}

/* Invoke the "match" method of the MATCHER and return a new reference
   to the result.  Returns NULL on error.  */

static PyObject *
invoke_match_method (PyObject *matcher, PyObject *py_obj_type,
		     const char *xmethod_name)
{
  int enabled;

  gdbpy_ref<> enabled_field (PyObject_GetAttrString (matcher,
						     enabled_field_name));
  if (enabled_field == NULL)
    return NULL;

  enabled = PyObject_IsTrue (enabled_field.get ());
  if (enabled == -1)
    return NULL;
  if (enabled == 0)
    {
      /* Return 'None' if the matcher is not enabled.  */
      Py_RETURN_NONE;
    }

  gdbpy_ref<> match_method (PyObject_GetAttrString (matcher,
						    match_method_name));
  if (match_method == NULL)
    return NULL;

  gdbpy_ref<> py_xmethod_name (PyUnicode_FromString (xmethod_name));
  if (py_xmethod_name == NULL)
    return NULL;

  return PyObject_CallMethodObjArgs (matcher, py_match_method_name,
				     py_obj_type, py_xmethod_name.get (),
				     NULL);
}

/* Implementation of get_matching_xmethod_workers for Python.  */

enum ext_lang_rc
gdbpy_get_matching_xmethod_workers
  (const struct extension_language_defn *extlang,
   struct type *obj_type, const char *method_name,
   std::vector<xmethod_worker_up> *dm_vec)
{
  gdb_assert (obj_type != NULL && method_name != NULL);

  gdbpy_enter enter_py;

  gdbpy_ref<> py_type (type_to_type_object (obj_type));
  if (py_type == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  /* Create an empty list of debug methods.  */
  gdbpy_ref<> py_xmethod_matcher_list (PyList_New (0));
  if (py_xmethod_matcher_list == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  /* Gather debug method matchers registered with the object files.
     This could be done differently by iterating over each objfile's matcher
     list individually, but there's no data yet to show it's needed.  */
  for (objfile *objfile : current_program_space->objfiles ())
    {
      gdbpy_ref<> py_objfile = objfile_to_objfile_object (objfile);

      if (py_objfile == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}

      gdbpy_ref<> objfile_matchers (objfpy_get_xmethods (py_objfile.get (),
							 NULL));
      gdbpy_ref<> temp (PySequence_Concat (py_xmethod_matcher_list.get (),
					   objfile_matchers.get ()));
      if (temp == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}

      py_xmethod_matcher_list = std::move (temp);
    }

  /* Gather debug methods matchers registered with the current program
     space.  */
  gdbpy_ref<> py_progspace = pspace_to_pspace_object (current_program_space);
  if (py_progspace != NULL)
    {
      gdbpy_ref<> pspace_matchers (pspy_get_xmethods (py_progspace.get (),
						      NULL));

      gdbpy_ref<> temp (PySequence_Concat (py_xmethod_matcher_list.get (),
					   pspace_matchers.get ()));
      if (temp == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}

      py_xmethod_matcher_list = std::move (temp);
    }
  else
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  /* Gather debug method matchers registered globally.  */
  if (gdb_python_module != NULL
      && PyObject_HasAttrString (gdb_python_module, matchers_attr_str))
    {
      gdbpy_ref<> gdb_matchers (PyObject_GetAttrString (gdb_python_module,
							matchers_attr_str));
      if (gdb_matchers != NULL)
	{
	  gdbpy_ref<> temp (PySequence_Concat (py_xmethod_matcher_list.get (),
					       gdb_matchers.get ()));
	  if (temp == NULL)
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  py_xmethod_matcher_list = std::move (temp);
	}
      else
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}
    }

  gdbpy_ref<> list_iter (PyObject_GetIter (py_xmethod_matcher_list.get ()));
  if (list_iter == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }
  while (true)
    {
      gdbpy_ref<> matcher (PyIter_Next (list_iter.get ()));
      if (matcher == NULL)
	{
	  if (PyErr_Occurred ())
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }
	  break;
	}

      gdbpy_ref<> match_result (invoke_match_method (matcher.get (),
						     py_type.get (),
						     method_name));

      if (match_result == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}
      if (match_result == Py_None)
	; /* This means there was no match.  */
      else if (PySequence_Check (match_result.get ()))
	{
	  gdbpy_ref<> iter (PyObject_GetIter (match_result.get ()));

	  if (iter == NULL)
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }
	  while (true)
	    {
	      struct xmethod_worker *worker;

	      gdbpy_ref<> py_worker (PyIter_Next (iter.get ()));
	      if (py_worker == NULL)
		{
		  if (PyErr_Occurred ())
		    {
		      gdbpy_print_stack ();
		      return EXT_LANG_RC_ERROR;
		    }
		  break;
		}

	      worker = new python_xmethod_worker (py_worker.get (),
						  py_type.get ());

	      dm_vec->emplace_back (worker);
	    }
	}
      else
	{
	  struct xmethod_worker *worker;

	  worker = new python_xmethod_worker (match_result.get (),
					      py_type.get ());
	  dm_vec->emplace_back (worker);
	}
    }

  return EXT_LANG_RC_OK;
}

/* See declaration.  */

ext_lang_rc
python_xmethod_worker::do_get_arg_types (std::vector<type *> *arg_types)
{
  /* The gdbpy_enter object needs to be placed first, so that it's the last to
     be destroyed.  */
  gdbpy_enter enter_py;
  struct type *obj_type;
  int i = 1, arg_count;
  gdbpy_ref<> list_iter;

  gdbpy_ref<> get_arg_types_method
    (PyObject_GetAttrString (m_py_worker, get_arg_types_method_name));
  if (get_arg_types_method == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> py_argtype_list
    (PyObject_CallMethodObjArgs (m_py_worker, py_get_arg_types_method_name,
				 NULL));
  if (py_argtype_list == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  if (py_argtype_list == Py_None)
    arg_count = 0;
  else if (PySequence_Check (py_argtype_list.get ()))
    {
      arg_count = PySequence_Size (py_argtype_list.get ());
      if (arg_count == -1)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}

      list_iter.reset (PyObject_GetIter (py_argtype_list.get ()));
      if (list_iter == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}
    }
  else
    arg_count = 1;

  /* Include the 'this' argument in the size.  */
  arg_types->resize (arg_count + 1);
  i = 1;
  if (list_iter != NULL)
    {
      while (true)
	{
	  gdbpy_ref<> item (PyIter_Next (list_iter.get ()));
	  if (item == NULL)
	    {
	      if (PyErr_Occurred ())
		{
		  gdbpy_print_stack ();
		  return EXT_LANG_RC_ERROR;
		}
	      break;
	    }

	  struct type *arg_type = type_object_to_type (item.get ());
	  if (arg_type == NULL)
	    {
	      PyErr_SetString (PyExc_TypeError,
			       _("Arg type returned by the get_arg_types "
				 "method of a debug method worker object is "
				 "not a gdb.Type object."));
	      return EXT_LANG_RC_ERROR;
	    }

	  (*arg_types)[i] = arg_type;
	  i++;
	}
    }
  else if (arg_count == 1)
    {
      /* py_argtype_list is not actually a list but a single gdb.Type
	 object.  */
      struct type *arg_type = type_object_to_type (py_argtype_list.get ());

      if (arg_type == NULL)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("Arg type returned by the get_arg_types method "
			     "of an xmethod worker object is not a gdb.Type "
			     "object."));
	  return EXT_LANG_RC_ERROR;
	}
      else
	{
	  (*arg_types)[i] = arg_type;
	  i++;
	}
    }

  /* Add the type of 'this' as the first argument.  The 'this' pointer should
     be a 'const' value.  Hence, create a 'const' variant of the 'this' pointer
     type.  */
  obj_type = type_object_to_type (m_this_type);
  (*arg_types)[0] = make_cv_type (1, 0, lookup_pointer_type (obj_type),
				  NULL);

  return EXT_LANG_RC_OK;
}

/* See declaration.  */

ext_lang_rc
python_xmethod_worker::do_get_result_type (value *obj,
					   gdb::array_view<value *> args,
					   type **result_type_ptr)
{
  struct type *obj_type, *this_type;
  int i;

  gdbpy_enter enter_py;

  /* First see if there is a get_result_type method.
     If not this could be an old xmethod (pre 7.9.1).  */
  gdbpy_ref<> get_result_type_method
    (PyObject_GetAttrString (m_py_worker, get_result_type_method_name));
  if (get_result_type_method == NULL)
    {
      PyErr_Clear ();
      *result_type_ptr = NULL;
      return EXT_LANG_RC_OK;
    }

  scoped_value_mark free_values;
  obj_type = check_typedef (obj->type ());
  this_type = check_typedef (type_object_to_type (m_this_type));
  if (obj_type->code () == TYPE_CODE_PTR)
    {
      struct type *this_ptr = lookup_pointer_type (this_type);

      if (!types_equal (obj_type, this_ptr))
	obj = value_cast (this_ptr, obj);
    }
  else if (TYPE_IS_REFERENCE (obj_type))
    {
      struct type *this_ref
	= lookup_reference_type (this_type, obj_type->code ());

      if (!types_equal (obj_type, this_ref))
	obj = value_cast (this_ref, obj);
    }
  else
    {
      if (!types_equal (obj_type, this_type))
	obj = value_cast (this_type, obj);
    }
  gdbpy_ref<> py_value_obj (value_to_value_object (obj));
  if (py_value_obj == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> py_arg_tuple (PyTuple_New (args.size () + 1));
  if (py_arg_tuple == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  /* PyTuple_SET_ITEM steals the reference of the element, hence the
     release.  */
  PyTuple_SET_ITEM (py_arg_tuple.get (), 0, py_value_obj.release ());

  for (i = 0; i < args.size (); i++)
    {
      PyObject *py_value_arg = value_to_value_object (args[i]);

      if (py_value_arg == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}
      PyTuple_SET_ITEM (py_arg_tuple.get (), i + 1, py_value_arg);
    }

  gdbpy_ref<> py_result_type
    (PyObject_CallObject (get_result_type_method.get (), py_arg_tuple.get ()));
  if (py_result_type == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  *result_type_ptr = type_object_to_type (py_result_type.get ());
  if (*result_type_ptr == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Type returned by the get_result_type method of an"
			 " xmethod worker object is not a gdb.Type object."));
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  return EXT_LANG_RC_OK;
}

/* See declaration.  */

struct value *
python_xmethod_worker::invoke (struct value *obj,
			       gdb::array_view<value *> args)
{
  gdbpy_enter enter_py;

  int i;
  struct type *obj_type, *this_type;
  struct value *res = NULL;

  obj_type = check_typedef (obj->type ());
  this_type = check_typedef (type_object_to_type (m_this_type));
  if (obj_type->code () == TYPE_CODE_PTR)
    {
      struct type *this_ptr = lookup_pointer_type (this_type);

      if (!types_equal (obj_type, this_ptr))
	obj = value_cast (this_ptr, obj);
    }
  else if (TYPE_IS_REFERENCE (obj_type))
    {
      struct type *this_ref
	= lookup_reference_type (this_type, obj_type->code ());

      if (!types_equal (obj_type, this_ref))
	obj = value_cast (this_ref, obj);
    }
  else
    {
      if (!types_equal (obj_type, this_type))
	obj = value_cast (this_type, obj);
    }
  gdbpy_ref<> py_value_obj (value_to_value_object (obj));
  if (py_value_obj == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }

  gdbpy_ref<> py_arg_tuple (PyTuple_New (args.size () + 1));
  if (py_arg_tuple == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }

  /* PyTuple_SET_ITEM steals the reference of the element, hence the
     release.  */
  PyTuple_SET_ITEM (py_arg_tuple.get (), 0, py_value_obj.release ());

  for (i = 0; i < args.size (); i++)
    {
      PyObject *py_value_arg = value_to_value_object (args[i]);

      if (py_value_arg == NULL)
	{
	  gdbpy_print_stack ();
	  error (_("Error while executing Python code."));
	}

      PyTuple_SET_ITEM (py_arg_tuple.get (), i + 1, py_value_arg);
    }

  gdbpy_ref<> py_result (PyObject_CallObject (m_py_worker,
					      py_arg_tuple.get ()));
  if (py_result == NULL)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }

  if (py_result != Py_None)
    {
      res = convert_value_from_python (py_result.get ());
      if (res == NULL)
	{
	  gdbpy_print_stack ();
	  error (_("Error while executing Python code."));
	}
    }
  else
    {
      res = value::allocate (lookup_typename (current_language,
					     "void", NULL, 0));
    }

  return res;
}

python_xmethod_worker::python_xmethod_worker (PyObject *py_worker,
					       PyObject *this_type)
: xmethod_worker (&extension_language_python),
  m_py_worker (py_worker), m_this_type (this_type)
{
  gdb_assert (m_py_worker != NULL && m_this_type != NULL);

  Py_INCREF (py_worker);
  Py_INCREF (this_type);
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_xmethods (void)
{
  py_match_method_name = PyUnicode_FromString (match_method_name);
  if (py_match_method_name == NULL)
    return -1;

  py_get_arg_types_method_name
    = PyUnicode_FromString (get_arg_types_method_name);
  if (py_get_arg_types_method_name == NULL)
    return -1;

  return 1;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_xmethods);

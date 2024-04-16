/* Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
#include "varobj.h"
#include "varobj-iter.h"
#include "valprint.h"

/* A dynamic varobj iterator "class" for python pretty-printed
   varobjs.  This inherits struct varobj_iter.  */

struct py_varobj_iter : public varobj_iter
{
  py_varobj_iter (struct varobj *var, gdbpy_ref<> &&pyiter,
		  const value_print_options *opts);
  ~py_varobj_iter () override;

  std::unique_ptr<varobj_item> next () override;

private:

  /* The varobj this iterator is listing children for.  */
  struct varobj *m_var;

  /* The next raw index we will try to check is available.  If it is
     equal to number_of_children, then we've already iterated the
     whole set.  */
  int m_next_raw_index = 0;

  /* The python iterator returned by the printer's 'children' method,
     or NULL if not available.  */
  PyObject *m_iter;

  /* The print options to use.  */
  value_print_options m_opts;
};

/* Implementation of the 'dtor' method of pretty-printed varobj
   iterators.  */

py_varobj_iter::~py_varobj_iter ()
{
  gdbpy_enter_varobj enter_py (m_var);
  Py_XDECREF (m_iter);
}

/* Implementation of the 'next' method of pretty-printed varobj
   iterators.  */

std::unique_ptr<varobj_item>
py_varobj_iter::next ()
{
  PyObject *py_v;
  varobj_item *vitem;
  const char *name = NULL;

  if (!gdb_python_initialized)
    return NULL;

  gdbpy_enter_varobj enter_py (m_var);

  scoped_restore set_options = make_scoped_restore (&gdbpy_current_print_options,
						    &m_opts);

  gdbpy_ref<> item (PyIter_Next (m_iter));

  if (item == NULL)
    {
      /* Normal end of iteration.  */
      if (!PyErr_Occurred ())
	return NULL;

      /* If we got a memory error, just use the text as the item.  */
      if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
	{
	  gdbpy_err_fetch fetched_error;
	  gdb::unique_xmalloc_ptr<char> value_str = fetched_error.to_string ();
	  if (value_str == NULL)
	    {
	      gdbpy_print_stack ();
	      return NULL;
	    }

	  std::string name_str = string_printf ("<error at %d>",
						m_next_raw_index++);
	  item.reset (Py_BuildValue ("(ss)", name_str.c_str (),
				     value_str.get ()));
	  if (item == NULL)
	    {
	      gdbpy_print_stack ();
	      return NULL;
	    }
	}
      else
	{
	  /* Any other kind of error.  */
	  gdbpy_print_stack ();
	  return NULL;
	}
    }

  if (!PyArg_ParseTuple (item.get (), "sO", &name, &py_v))
    {
      gdbpy_print_stack ();
      error (_("Invalid item from the child list"));
    }

  vitem = new varobj_item ();
  vitem->value = release_value (convert_value_from_python (py_v));
  if (vitem->value == NULL)
    gdbpy_print_stack ();
  vitem->name = name;

  m_next_raw_index++;
  return std::unique_ptr<varobj_item> (vitem);
}

/* Constructor of pretty-printed varobj iterators.  VAR is the varobj
   whose children the iterator will be iterating over.  PYITER is the
   python iterator actually responsible for the iteration.  */

py_varobj_iter::py_varobj_iter (struct varobj *var, gdbpy_ref<> &&pyiter,
				const value_print_options *opts)
  : m_var (var),
    m_iter (pyiter.release ()),
    m_opts (*opts)
{
}

/* Return a new pretty-printed varobj iterator suitable to iterate
   over VAR's children.  */

std::unique_ptr<varobj_iter>
py_varobj_get_iterator (struct varobj *var, PyObject *printer,
			const value_print_options *opts)
{
  gdbpy_enter_varobj enter_py (var);

  if (!PyObject_HasAttr (printer, gdbpy_children_cst))
    return NULL;

  scoped_restore set_options = make_scoped_restore (&gdbpy_current_print_options,
						    opts);

  gdbpy_ref<> children (PyObject_CallMethodObjArgs (printer, gdbpy_children_cst,
						    NULL));
  if (children == NULL)
    {
      gdbpy_print_stack ();
      error (_("Null value returned for children"));
    }

  gdbpy_ref<> iter (PyObject_GetIter (children.get ()));
  if (iter == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not get children iterator"));
    }

  return std::make_unique<py_varobj_iter> (var, std::move (iter), opts);
}

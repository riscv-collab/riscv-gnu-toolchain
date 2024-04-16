/* Python interface to MI commands

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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
#include "py-uiout.h"
#include "utils.h"
#include "ui.h"
#include "interps.h"
#include "target.h"
#include "mi/mi-parse.h"
#include "mi/mi-console.h"
#include "mi/mi-interp.h"

void
py_ui_out::add_field (const char *name, const gdbpy_ref<> &obj)
{
  if (obj == nullptr)
    {
      m_error.emplace ();
      return;
    }

  object_desc &desc = current ();
  if (desc.type == ui_out_type_list)
    {
      if (PyList_Append (desc.obj.get (), obj.get ()) < 0)
	m_error.emplace ();
    }
  else
    {
      if (PyDict_SetItemString (desc.obj.get (), name, obj.get ()) < 0)
	m_error.emplace ();
    }
}

void
py_ui_out::do_begin (ui_out_type type, const char *id)
{
  if (m_error.has_value ())
    return;

  gdbpy_ref<> new_obj (type == ui_out_type_list
		       ? PyList_New (0)
		       : PyDict_New ());
  if (new_obj == nullptr)
    {
      m_error.emplace ();
      return;
    }

  object_desc new_desc;
  if (id != nullptr)
    new_desc.field_name = id;
  new_desc.obj = std::move (new_obj);
  new_desc.type = type;

  m_objects.push_back (std::move (new_desc));
}

void
py_ui_out::do_end (ui_out_type type)
{
  if (m_error.has_value ())
    return;

  object_desc new_obj = std::move (current ());
  m_objects.pop_back ();
  add_field (new_obj.field_name.c_str (), new_obj.obj);
}

void
py_ui_out::do_field_signed (int fldno, int width, ui_align align,
			    const char *fldname, LONGEST value)
{
  if (m_error.has_value ())
    return;

  gdbpy_ref<> val = gdb_py_object_from_longest (value);
  add_field (fldname, val);
}

void
py_ui_out::do_field_unsigned (int fldno, int width, ui_align align,
			    const char *fldname, ULONGEST value)
{
  if (m_error.has_value ())
    return;

  gdbpy_ref<> val = gdb_py_object_from_ulongest (value);
  add_field (fldname, val);
}

void
py_ui_out::do_field_string (int fldno, int width, ui_align align,
			    const char *fldname, const char *string,
			    const ui_file_style &style)
{
  if (m_error.has_value ())
    return;

  gdbpy_ref<> val = host_string_to_python_string (string);
  add_field (fldname, val);
}

void
py_ui_out::do_field_fmt (int fldno, int width, ui_align align,
			 const char *fldname, const ui_file_style &style,
			 const char *format, va_list args)
{
  if (m_error.has_value ())
    return;

  std::string str = string_vprintf (format, args);
  do_field_string (fldno, width, align, fldname, str.c_str (), style);
}

/* Implementation of the gdb.execute_mi command.  */

PyObject *
gdbpy_execute_mi_command (PyObject *self, PyObject *args, PyObject *kw)
{
  gdb::unique_xmalloc_ptr<char> mi_command;
  std::vector<gdb::unique_xmalloc_ptr<char>> arg_strings;

  Py_ssize_t n_args = PyTuple_Size (args);
  if (n_args < 0)
    return nullptr;

  for (Py_ssize_t i = 0; i < n_args; ++i)
    {
      /* Note this returns a borrowed reference.  */
      PyObject *arg = PyTuple_GetItem (args, i);
      if (arg == nullptr)
	return nullptr;
      gdb::unique_xmalloc_ptr<char> str = python_string_to_host_string (arg);
      if (str == nullptr)
	return nullptr;
      if (i == 0)
	mi_command = std::move (str);
      else
	arg_strings.push_back (std::move (str));
    }

  py_ui_out uiout;

  try
    {
      scoped_restore save_uiout = make_scoped_restore (&current_uiout, &uiout);
      auto parser = std::make_unique<mi_parse> (std::move (mi_command),
						std::move (arg_strings));
      mi_execute_command (parser.get ());
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return nullptr;
    }

  return uiout.result ().release ();
}

/* Convert KEY_OBJ into a string that can be used as a field name in MI
   output.  KEY_OBJ must be a Python string object, and must only contain
   characters suitable for use as an MI field name.

   If KEY_OBJ is not a string, or if KEY_OBJ contains invalid characters,
   then an error is thrown.  Otherwise, KEY_OBJ is converted to a string
   and returned.  */

static gdb::unique_xmalloc_ptr<char>
py_object_to_mi_key (PyObject *key_obj)
{
  /* The key must be a string.  */
  if (!PyUnicode_Check (key_obj))
    {
      gdbpy_ref<> key_repr (PyObject_Repr (key_obj));
      gdb::unique_xmalloc_ptr<char> key_repr_string;
      if (key_repr != nullptr)
	key_repr_string = python_string_to_target_string (key_repr.get ());
      if (key_repr_string == nullptr)
	gdbpy_handle_exception ();

      gdbpy_error (_("non-string object used as key: %s"),
		   key_repr_string.get ());
    }

  gdb::unique_xmalloc_ptr<char> key_string
    = python_string_to_target_string (key_obj);
  if (key_string == nullptr)
    gdbpy_handle_exception ();

  /* Predicate function, returns true if NAME is a valid field name for use
     in MI result output, otherwise, returns false.  */
  auto is_valid_key_name = [] (const char *name) -> bool
  {
    gdb_assert (name != nullptr);

    if (*name == '\0' || !isalpha (*name))
      return false;

    for (; *name != '\0'; ++name)
      if (!isalnum (*name) && *name != '_' && *name != '-')
	return false;

    return true;
  };

  if (!is_valid_key_name (key_string.get ()))
    {
      if (*key_string.get () == '\0')
	gdbpy_error (_("Invalid empty key in MI result"));
      else
	gdbpy_error (_("Invalid key in MI result: %s"), key_string.get ());
    }

  return key_string;
}

/* Serialize RESULT and print it in MI format to the current_uiout.
   FIELD_NAME is used as the name of this result field.

   RESULT can be a dictionary, a sequence, an iterator, or an object that
   can be converted to a string, these are converted to the matching MI
   output format (dictionaries as tuples, sequences and iterators as lists,
   and strings as named fields).

   If anything goes wrong while formatting the output then an error is
   thrown.

   This function is the recursive inner core of serialize_mi_result, and
   should only be called from that function.  */

static void
serialize_mi_result_1 (PyObject *result, const char *field_name)
{
  struct ui_out *uiout = current_uiout;

  if (PyDict_Check (result))
    {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      ui_out_emit_tuple tuple_emitter (uiout, field_name);
      while (PyDict_Next (result, &pos, &key, &value))
	{
	  gdb::unique_xmalloc_ptr<char> key_string
	    (py_object_to_mi_key (key));
	  serialize_mi_result_1 (value, key_string.get ());
	}
    }
  else if (PySequence_Check (result) && !PyUnicode_Check (result))
    {
      ui_out_emit_list list_emitter (uiout, field_name);
      Py_ssize_t len = PySequence_Size (result);
      if (len == -1)
	gdbpy_handle_exception ();
      for (Py_ssize_t i = 0; i < len; ++i)
	{
	  gdbpy_ref<> item (PySequence_ITEM (result, i));
	  if (item == nullptr)
	    gdbpy_handle_exception ();
	  serialize_mi_result_1 (item.get (), nullptr);
	}
    }
  else if (PyIter_Check (result))
    {
      gdbpy_ref<> item;
      ui_out_emit_list list_emitter (uiout, field_name);
      while (true)
	{
	  item.reset (PyIter_Next (result));
	  if (item == nullptr)
	    {
	      if (PyErr_Occurred () != nullptr)
		gdbpy_handle_exception ();
	      break;
	    }
	  serialize_mi_result_1 (item.get (), nullptr);
	}
    }
  else
    {
      if (PyLong_Check (result))
	{
	  int overflow = 0;
	  gdb_py_longest val = gdb_py_long_as_long_and_overflow (result,
								 &overflow);
	  if (PyErr_Occurred () != nullptr)
	    gdbpy_handle_exception ();
	  if (overflow == 0)
	    {
	      uiout->field_signed (field_name, val);
	      return;
	    }
	  /* Fall through to the string case on overflow.  */
	}

      gdb::unique_xmalloc_ptr<char> string (gdbpy_obj_to_string (result));
      if (string == nullptr)
	gdbpy_handle_exception ();
      uiout->field_string (field_name, string.get ());
    }
}

/* See python-internal.h.  */

void
serialize_mi_results (PyObject *results)
{
  gdb_assert (PyDict_Check (results));

  PyObject *key, *value;
  Py_ssize_t pos = 0;
  while (PyDict_Next (results, &pos, &key, &value))
    {
      gdb::unique_xmalloc_ptr<char> key_string
	(py_object_to_mi_key (key));
      serialize_mi_result_1 (value, key_string.get ());
    }
}

/* See python-internal.h.  */

PyObject *
gdbpy_notify_mi (PyObject *self, PyObject *args, PyObject *kwargs)
{
  static const char *keywords[] = { "name", "data", nullptr };
  char *name = nullptr;
  PyObject *data = Py_None;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "s|O", keywords,
					&name, &data))
    return nullptr;

  /* Validate notification name.  */
  const int name_len = strlen (name);
  if (name_len == 0)
    {
      PyErr_SetString (PyExc_ValueError, _("MI notification name is empty."));
      return nullptr;
    }
  for (int i = 0; i < name_len; i++)
    {
      if (!isalnum (name[i]) && name[i] != '-')
	{
	  PyErr_Format
	    (PyExc_ValueError,
	     _("MI notification name contains invalid character: %c."),
	     name[i]);
	  return nullptr;
	}
    }

  /* Validate additional data.  */
  if (!(data == Py_None || PyDict_Check (data)))
    {
      PyErr_Format
	(PyExc_ValueError,
	 _("MI notification data must be either None or a dictionary, not %s"),
	 Py_TYPE (data)->tp_name);
      return nullptr;
    }

  SWITCH_THRU_ALL_UIS ()
    {
      struct mi_interp *mi = as_mi_interp (top_level_interpreter ());

      if (mi == nullptr)
	continue;

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      gdb_printf (mi->event_channel, "%s", name);
      if (data != Py_None)
	{
	  ui_out *mi_uiout = mi->interp_ui_out ();
	  ui_out_redirect_pop redir (mi_uiout, mi->event_channel);
	  scoped_restore restore_uiout
	    = make_scoped_restore (&current_uiout, mi_uiout);

	  serialize_mi_results (data);
	}
      gdb_flush (mi->event_channel);
    }

  Py_RETURN_NONE;
}

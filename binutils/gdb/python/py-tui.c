/* TUI windows implemented in Python

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "python-internal.h"
#include "gdbsupport/intrusive_list.h"

#ifdef TUI

/* Note that Python's public headers may define HAVE_NCURSES_H, so if
   we unconditionally include this (outside the #ifdef above), then we
   can get a compile error when ncurses is not in fact installed.  See
   PR tui/25597; or the upstream Python bug
   https://bugs.python.org/issue20768.  */
#include "gdb_curses.h"

#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-layout.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-winsource.h"

class tui_py_window;

/* A PyObject representing a TUI window.  */

struct gdbpy_tui_window
{
  PyObject_HEAD

  /* The TUI window, or nullptr if the window has been deleted.  */
  tui_py_window *window;

  /* Return true if this object is valid.  */
  bool is_valid () const;
};

extern PyTypeObject gdbpy_tui_window_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("gdbpy_tui_window");

/* A TUI window written in Python.  */

class tui_py_window : public tui_win_info
{
public:

  tui_py_window (const char *name, gdbpy_ref<gdbpy_tui_window> wrapper)
    : m_name (name),
      m_wrapper (std::move (wrapper))
  {
    m_wrapper->window = this;
  }

  ~tui_py_window ();

  DISABLE_COPY_AND_ASSIGN (tui_py_window);

  /* Set the "user window" to the indicated reference.  The user
     window is the object returned the by user-defined window
     constructor.  */
  void set_user_window (gdbpy_ref<> &&user_window)
  {
    m_window = std::move (user_window);
  }

  const char *name () const override
  {
    return m_name.c_str ();
  }

  void rerender () override;
  void do_scroll_vertical (int num_to_scroll) override;
  void do_scroll_horizontal (int num_to_scroll) override;

  void refresh_window () override
  {
    if (m_inner_window != nullptr)
      {
	wnoutrefresh (handle.get ());
	touchwin (m_inner_window.get ());
	tui_wrefresh (m_inner_window.get ());
      }
    else
      tui_win_info::refresh_window ();
  }

  void resize (int height, int width, int origin_x, int origin_y) override;

  void click (int mouse_x, int mouse_y, int mouse_button) override;

  /* Erase and re-box the window.  */
  void erase ()
  {
    if (is_visible () && m_inner_window != nullptr)
      {
	werase (m_inner_window.get ());
	check_and_display_highlight_if_needed ();
      }
  }

  /* Write STR to the window.  FULL_WINDOW is true to erase the window
     contents beforehand.  */
  void output (const char *str, bool full_window);

  /* A helper function to compute the viewport width.  */
  int viewport_width () const
  {
    return std::max (0, width - 2);
  }

  /* A helper function to compute the viewport height.  */
  int viewport_height () const
  {
    return std::max (0, height - 2);
  }

private:

  /* The name of this window.  */
  std::string m_name;

  /* We make our own inner window, so that it is easy to print without
     overwriting the border.  */
  std::unique_ptr<WINDOW, curses_deleter> m_inner_window;

  /* The underlying Python window object.  */
  gdbpy_ref<> m_window;

  /* The Python wrapper for this object.  */
  gdbpy_ref<gdbpy_tui_window> m_wrapper;
};

/* See gdbpy_tui_window declaration above.  */

bool
gdbpy_tui_window::is_valid () const
{
  return window != nullptr && tui_active;
}

tui_py_window::~tui_py_window ()
{
  gdbpy_enter enter_py;

  /* This can be null if the user-provided Python construction
     function failed.  */
  if (m_window != nullptr
      && PyObject_HasAttrString (m_window.get (), "close"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "close",
					       nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }

  /* Unlink.  */
  m_wrapper->window = nullptr;
  /* Explicitly free the Python references.  We have to do this
     manually because we need to hold the GIL while doing so.  */
  m_wrapper.reset (nullptr);
  m_window.reset (nullptr);
}

void
tui_py_window::rerender ()
{
  tui_win_info::rerender ();

  gdbpy_enter enter_py;

  int h = viewport_height ();
  int w = viewport_width ();
  if (h == 0 || w == 0)
    {
      /* The window would be too small, so just remove the
	 contents.  */
      m_inner_window.reset (nullptr);
      return;
    }
  m_inner_window.reset (newwin (h, w, y + 1, x + 1));

  if (PyObject_HasAttrString (m_window.get (), "render"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "render",
					       nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::do_scroll_horizontal (int num_to_scroll)
{
  gdbpy_enter enter_py;

  if (PyObject_HasAttrString (m_window.get (), "hscroll"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get(), "hscroll",
					       "i", num_to_scroll, nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::do_scroll_vertical (int num_to_scroll)
{
  gdbpy_enter enter_py;

  if (PyObject_HasAttrString (m_window.get (), "vscroll"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "vscroll",
					       "i", num_to_scroll, nullptr));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::resize (int height_, int width_, int origin_x_, int origin_y_)
{
  m_inner_window.reset (nullptr);

  tui_win_info::resize (height_, width_, origin_x_, origin_y_);
}

void
tui_py_window::click (int mouse_x, int mouse_y, int mouse_button)
{
  gdbpy_enter enter_py;

  if (PyObject_HasAttrString (m_window.get (), "click"))
    {
      gdbpy_ref<> result (PyObject_CallMethod (m_window.get (), "click",
					       "iii", mouse_x, mouse_y,
					       mouse_button));
      if (result == nullptr)
	gdbpy_print_stack ();
    }
}

void
tui_py_window::output (const char *text, bool full_window)
{
  if (m_inner_window != nullptr)
    {
      if (full_window)
	werase (m_inner_window.get ());

      tui_puts (text, m_inner_window.get ());
      if (full_window)
	check_and_display_highlight_if_needed ();
      else
	tui_wrefresh (m_inner_window.get ());
    }
}



/* A callable that is used to create a TUI window.  It wraps the
   user-supplied window constructor.  */

class gdbpy_tui_window_maker
  : public intrusive_list_node<gdbpy_tui_window_maker>
{
public:

  explicit gdbpy_tui_window_maker (gdbpy_ref<> &&constr)
    : m_constr (std::move (constr))
  {
    m_window_maker_list.push_back (*this);
  }

  ~gdbpy_tui_window_maker ();

  gdbpy_tui_window_maker (gdbpy_tui_window_maker &&other) noexcept
    : m_constr (std::move (other.m_constr))
  {
    m_window_maker_list.push_back (*this);
  }

  gdbpy_tui_window_maker (const gdbpy_tui_window_maker &other)
  {
    gdbpy_enter enter_py;
    m_constr = other.m_constr;
    m_window_maker_list.push_back (*this);
  }

  gdbpy_tui_window_maker &operator= (gdbpy_tui_window_maker &&other)
  {
    m_constr = std::move (other.m_constr);
    return *this;
  }

  gdbpy_tui_window_maker &operator= (const gdbpy_tui_window_maker &other)
  {
    gdbpy_enter enter_py;
    m_constr = other.m_constr;
    return *this;
  }

  tui_win_info *operator() (const char *name);

  /* Reset the m_constr field of all gdbpy_tui_window_maker objects back to
     nullptr, this will allow the Python object referenced to be
     deallocated.  This function is intended to be called when GDB is
     shutting down the Python interpreter to allow all Python objects to be
     deallocated and cleaned up.  */
  static void
  invalidate_all ()
  {
    gdbpy_enter enter_py;
    for (gdbpy_tui_window_maker &f : m_window_maker_list)
      f.m_constr.reset (nullptr);
  }

private:

  /* A constructor that is called to make a TUI window.  */
  gdbpy_ref<> m_constr;

  /* A global list of all gdbpy_tui_window_maker objects.  */
  static intrusive_list<gdbpy_tui_window_maker> m_window_maker_list;
};

/* See comment in class declaration above.  */

intrusive_list<gdbpy_tui_window_maker>
  gdbpy_tui_window_maker::m_window_maker_list;

gdbpy_tui_window_maker::~gdbpy_tui_window_maker ()
{
  /* Remove this gdbpy_tui_window_maker from the global list.  */
  if (is_linked ())
    m_window_maker_list.erase (m_window_maker_list.iterator_to (*this));

  if (m_constr != nullptr)
    {
      gdbpy_enter enter_py;
      m_constr.reset (nullptr);
    }
}

tui_win_info *
gdbpy_tui_window_maker::operator() (const char *win_name)
{
  gdbpy_enter enter_py;

  gdbpy_ref<gdbpy_tui_window> wrapper
    (PyObject_New (gdbpy_tui_window, &gdbpy_tui_window_object_type));
  if (wrapper == nullptr)
    {
      gdbpy_print_stack ();
      return nullptr;
    }

  std::unique_ptr<tui_py_window> window
    (new tui_py_window (win_name, wrapper));

  /* There's only two ways that m_constr can be reset back to nullptr,
     first when the parent gdbpy_tui_window_maker object is deleted, in
     which case it should be impossible to call this method, or second, as
     a result of a gdbpy_tui_window_maker::invalidate_all call, but this is
     only called when GDB's Python interpreter is being shut down, after
     which, this method should not be called.  */
  gdb_assert (m_constr != nullptr);

  gdbpy_ref<> user_window
    (PyObject_CallFunctionObjArgs (m_constr.get (),
				   (PyObject *) wrapper.get (),
				   nullptr));
  if (user_window == nullptr)
    {
      gdbpy_print_stack ();
      return nullptr;
    }

  window->set_user_window (std::move (user_window));
  /* Window is now owned by the TUI.  */
  return window.release ();
}

/* Implement "gdb.register_window_type".  */

PyObject *
gdbpy_register_tui_window (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", "constructor", nullptr };

  const char *name;
  PyObject *cons_obj;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "sO", keywords,
					&name, &cons_obj))
    return nullptr;

  try
    {
      gdbpy_tui_window_maker constr (gdbpy_ref<>::new_reference (cons_obj));
      tui_register_window (name, constr);
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return nullptr;
    }

  Py_RETURN_NONE;
}



/* Require that "Window" be a valid window.  */

#define REQUIRE_WINDOW(Window)					\
    do {							\
      if (!(Window)->is_valid ())				\
	return PyErr_Format (PyExc_RuntimeError,		\
			     _("TUI window is invalid."));	\
    } while (0)

/* Require that "Window" be a valid window.  */

#define REQUIRE_WINDOW_FOR_SETTER(Window)			\
    do {							\
      if (!(Window)->is_valid ())				\
	{							\
	  PyErr_Format (PyExc_RuntimeError,			\
			_("TUI window is invalid."));		\
	  return -1;						\
	}							\
    } while (0)

/* Python function which checks the validity of a TUI window
   object.  */
static PyObject *
gdbpy_tui_is_valid (PyObject *self, PyObject *args)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;

  if (win->is_valid ())
    Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

/* Python function that erases the TUI window.  */
static PyObject *
gdbpy_tui_erase (PyObject *self, PyObject *args)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;

  REQUIRE_WINDOW (win);

  win->window->erase ();

  Py_RETURN_NONE;
}

/* Python function that writes some text to a TUI window.  */
static PyObject *
gdbpy_tui_write (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "string", "full_window", nullptr };

  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  const char *text;
  int full_window = 0;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords,
					&text, &full_window))
    return nullptr;

  REQUIRE_WINDOW (win);

  win->window->output (text, full_window);

  Py_RETURN_NONE;
}

/* Return the width of the TUI window.  */
static PyObject *
gdbpy_tui_width (PyObject *self, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  REQUIRE_WINDOW (win);
  gdbpy_ref<> result
    = gdb_py_object_from_longest (win->window->viewport_width ());
  return result.release ();
}

/* Return the height of the TUI window.  */
static PyObject *
gdbpy_tui_height (PyObject *self, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  REQUIRE_WINDOW (win);
  gdbpy_ref<> result
    = gdb_py_object_from_longest (win->window->viewport_height ());
  return result.release ();
}

/* Return the title of the TUI window.  */
static PyObject *
gdbpy_tui_title (PyObject *self, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;
  REQUIRE_WINDOW (win);
  return host_string_to_python_string (win->window->title ().c_str ()).release ();
}

/* Set the title of the TUI window.  */
static int
gdbpy_tui_set_title (PyObject *self, PyObject *newvalue, void *closure)
{
  gdbpy_tui_window *win = (gdbpy_tui_window *) self;

  REQUIRE_WINDOW_FOR_SETTER (win);

  if (newvalue == nullptr)
    {
      PyErr_Format (PyExc_TypeError, _("Cannot delete \"title\" attribute."));
      return -1;
    }

  gdb::unique_xmalloc_ptr<char> value
    = python_string_to_host_string (newvalue);
  if (value == nullptr)
    return -1;

  win->window->set_title (value.get ());
  return 0;
}

static gdb_PyGetSetDef tui_object_getset[] =
{
  { "width", gdbpy_tui_width, NULL, "Width of the window.", NULL },
  { "height", gdbpy_tui_height, NULL, "Height of the window.", NULL },
  { "title", gdbpy_tui_title, gdbpy_tui_set_title, "Title of the window.",
    NULL },
  { NULL }  /* Sentinel */
};

static PyMethodDef tui_object_methods[] =
{
  { "is_valid", gdbpy_tui_is_valid, METH_NOARGS,
    "is_valid () -> Boolean\n\
Return true if this TUI window is valid, false if not." },
  { "erase", gdbpy_tui_erase, METH_NOARGS,
    "Erase the TUI window." },
  { "write", (PyCFunction) gdbpy_tui_write, METH_VARARGS | METH_KEYWORDS,
    "Append a string to the TUI window." },
  { NULL } /* Sentinel.  */
};

PyTypeObject gdbpy_tui_window_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.TuiWindow",		  /*tp_name*/
  sizeof (gdbpy_tui_window),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
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
  0,				  /*tp_setattro */
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
  "GDB TUI window object",	  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  tui_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  tui_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
};

#endif /* TUI */

/* Initialize this module.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_tui ()
{
#ifdef TUI
  gdbpy_tui_window_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&gdbpy_tui_window_object_type) < 0)
    return -1;
#endif	/* TUI */

  return 0;
}

/* Finalize this module.  */

static void
gdbpy_finalize_tui ()
{
#ifdef TUI
  gdbpy_tui_window_maker::invalidate_all ();
#endif	/* TUI */
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_tui, gdbpy_finalize_tui);

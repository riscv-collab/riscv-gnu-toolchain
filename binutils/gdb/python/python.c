/* General python/gdb code

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
#include "arch-utils.h"
#include "command.h"
#include "ui-out.h"
#include "cli/cli-script.h"
#include "gdbcmd.h"
#include "progspace.h"
#include "objfiles.h"
#include "value.h"
#include "language.h"
#include "gdbsupport/event-loop.h"
#include "readline/tilde.h"
#include "python.h"
#include "extension-priv.h"
#include "cli/cli-utils.h"
#include <ctype.h>
#include "location.h"
#include "run-on-main-thread.h"
#include "observable.h"

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"
#endif

/* Declared constants and enum for python stack printing.  */
static const char python_excp_none[] = "none";
static const char python_excp_full[] = "full";
static const char python_excp_message[] = "message";

/* "set python print-stack" choices.  */
static const char *const python_excp_enums[] =
  {
    python_excp_none,
    python_excp_full,
    python_excp_message,
    NULL
  };

/* The exception printing variable.  'full' if we want to print the
   error message and stack, 'none' if we want to print nothing, and
   'message' if we only want to print the error message.  'message' is
   the default.  */
static const char *gdbpy_should_print_stack = python_excp_message;


#ifdef HAVE_PYTHON

#include "cli/cli-decode.h"
#include "charset.h"
#include "top.h"
#include "ui.h"
#include "python-internal.h"
#include "linespec.h"
#include "source.h"
#include "gdbsupport/version.h"
#include "target.h"
#include "gdbthread.h"
#include "interps.h"
#include "event-top.h"
#include "py-event.h"

/* True if Python has been successfully initialized, false
   otherwise.  */

int gdb_python_initialized;

extern PyMethodDef python_GdbMethods[];

PyObject *gdb_module;
PyObject *gdb_python_module;

/* Some string constants we may wish to use.  */
PyObject *gdbpy_to_string_cst;
PyObject *gdbpy_children_cst;
PyObject *gdbpy_display_hint_cst;
PyObject *gdbpy_doc_cst;
PyObject *gdbpy_enabled_cst;
PyObject *gdbpy_value_cst;

/* The GdbError exception.  */
PyObject *gdbpy_gdberror_exc;

/* The `gdb.error' base class.  */
PyObject *gdbpy_gdb_error;

/* The `gdb.MemoryError' exception.  */
PyObject *gdbpy_gdb_memory_error;

static script_sourcer_func gdbpy_source_script;
static objfile_script_sourcer_func gdbpy_source_objfile_script;
static objfile_script_executor_func gdbpy_execute_objfile_script;
static void gdbpy_initialize (const struct extension_language_defn *);
static int gdbpy_initialized (const struct extension_language_defn *);
static void gdbpy_eval_from_control_command
  (const struct extension_language_defn *, struct command_line *cmd);
static void gdbpy_start_type_printers (const struct extension_language_defn *,
				       struct ext_lang_type_printers *);
static enum ext_lang_rc gdbpy_apply_type_printers
  (const struct extension_language_defn *,
   const struct ext_lang_type_printers *, struct type *,
   gdb::unique_xmalloc_ptr<char> *);
static void gdbpy_free_type_printers (const struct extension_language_defn *,
				      struct ext_lang_type_printers *);
static void gdbpy_set_quit_flag (const struct extension_language_defn *);
static int gdbpy_check_quit_flag (const struct extension_language_defn *);
static enum ext_lang_rc gdbpy_before_prompt_hook
  (const struct extension_language_defn *, const char *current_gdb_prompt);
static std::optional<std::string> gdbpy_colorize
  (const std::string &filename, const std::string &contents);
static std::optional<std::string> gdbpy_colorize_disasm
(const std::string &content, gdbarch *gdbarch);
static ext_lang_missing_debuginfo_result gdbpy_handle_missing_debuginfo
  (const struct extension_language_defn *extlang, struct objfile *objfile);

/* The interface between gdb proper and loading of python scripts.  */

static const struct extension_language_script_ops python_extension_script_ops =
{
  gdbpy_source_script,
  gdbpy_source_objfile_script,
  gdbpy_execute_objfile_script,
  gdbpy_auto_load_enabled
};

/* The interface between gdb proper and python extensions.  */

static const struct extension_language_ops python_extension_ops =
{
  gdbpy_initialize,
  gdbpy_initialized,

  gdbpy_eval_from_control_command,

  gdbpy_start_type_printers,
  gdbpy_apply_type_printers,
  gdbpy_free_type_printers,

  gdbpy_apply_val_pretty_printer,

  gdbpy_apply_frame_filter,

  gdbpy_preserve_values,

  gdbpy_breakpoint_has_cond,
  gdbpy_breakpoint_cond_says_stop,

  gdbpy_set_quit_flag,
  gdbpy_check_quit_flag,

  gdbpy_before_prompt_hook,

  gdbpy_get_matching_xmethod_workers,

  gdbpy_colorize,

  gdbpy_colorize_disasm,

  gdbpy_print_insn,

  gdbpy_handle_missing_debuginfo
};

#endif /* HAVE_PYTHON */

/* The main struct describing GDB's interface to the Python
   extension language.  */
const struct extension_language_defn extension_language_python =
{
  EXT_LANG_PYTHON,
  "python",
  "Python",

  ".py",
  "-gdb.py",

  python_control,

#ifdef HAVE_PYTHON
  &python_extension_script_ops,
  &python_extension_ops
#else
  NULL,
  NULL
#endif
};

#ifdef HAVE_PYTHON

/* Architecture and language to be used in callbacks from
   the Python interpreter.  */
struct gdbarch *gdbpy_enter::python_gdbarch;

gdbpy_enter::gdbpy_enter  (struct gdbarch *gdbarch,
			   const struct language_defn *language)
: m_gdbarch (python_gdbarch),
  m_language (language == nullptr ? nullptr : current_language)
{
  /* We should not ever enter Python unless initialized.  */
  if (!gdb_python_initialized)
    error (_("Python not initialized"));

  m_previous_active = set_active_ext_lang (&extension_language_python);

  m_state = PyGILState_Ensure ();

  python_gdbarch = gdbarch;
  if (language != nullptr)
    set_language (language->la_language);

  /* Save it and ensure ! PyErr_Occurred () afterwards.  */
  m_error.emplace ();
}

gdbpy_enter::~gdbpy_enter ()
{
  /* Leftover Python error is forbidden by Python Exception Handling.  */
  if (PyErr_Occurred ())
    {
      /* This order is similar to the one calling error afterwards. */
      gdbpy_print_stack ();
      warning (_("internal error: Unhandled Python exception"));
    }

  m_error->restore ();

  python_gdbarch = m_gdbarch;
  if (m_language != nullptr)
    set_language (m_language->la_language);

  restore_active_ext_lang (m_previous_active);
  PyGILState_Release (m_state);
}

struct gdbarch *
gdbpy_enter::get_gdbarch ()
{
  if (python_gdbarch != nullptr)
    return python_gdbarch;
  return get_current_arch ();
}

void
gdbpy_enter::finalize ()
{
  python_gdbarch = current_inferior ()->arch ();
}

/* Set the quit flag.  */

static void
gdbpy_set_quit_flag (const struct extension_language_defn *extlang)
{
  PyErr_SetInterrupt ();
}

/* Return true if the quit flag has been set, false otherwise.  */

static int
gdbpy_check_quit_flag (const struct extension_language_defn *extlang)
{
  if (!gdb_python_initialized)
    return 0;

  gdbpy_gil gil;
  return PyOS_InterruptOccurred ();
}

/* Evaluate a Python command like PyRun_SimpleString, but uses
   Py_single_input which prints the result of expressions, and does
   not automatically print the stack on errors.  */

static int
eval_python_command (const char *command)
{
  PyObject *m, *d;

  m = PyImport_AddModule ("__main__");
  if (m == NULL)
    return -1;

  d = PyModule_GetDict (m);
  if (d == NULL)
    return -1;
  gdbpy_ref<> v (PyRun_StringFlags (command, Py_single_input, d, d, NULL));
  if (v == NULL)
    return -1;

  return 0;
}

/* Implementation of the gdb "python-interactive" command.  */

static void
python_interactive_command (const char *arg, int from_tty)
{
  struct ui *ui = current_ui;
  int err;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  arg = skip_spaces (arg);

  gdbpy_enter enter_py;

  if (arg && *arg)
    {
      std::string script = std::string (arg) + "\n";
      err = eval_python_command (script.c_str ());
    }
  else
    {
      err = PyRun_InteractiveLoop (ui->instream, "<stdin>");
      dont_repeat ();
    }

  if (err)
    {
      gdbpy_print_stack ();
      error (_("Error while executing Python code."));
    }
}

/* A wrapper around PyRun_SimpleFile.  FILE is the Python script to run
   named FILENAME.

   On Windows hosts few users would build Python themselves (this is no
   trivial task on this platform), and thus use binaries built by
   someone else instead.  There may happen situation where the Python
   library and GDB are using two different versions of the C runtime
   library.  Python, being built with VC, would use one version of the
   msvcr DLL (Eg. msvcr100.dll), while MinGW uses msvcrt.dll.
   A FILE * from one runtime does not necessarily operate correctly in
   the other runtime.

   To work around this potential issue, we run code in Python to load
   the script.  */

static void
python_run_simple_file (FILE *file, const char *filename)
{
#ifndef _WIN32

  PyRun_SimpleFile (file, filename);

#else /* _WIN32 */

  /* Because we have a string for a filename, and are using Python to
     open the file, we need to expand any tilde in the path first.  */
  gdb::unique_xmalloc_ptr<char> full_path (tilde_expand (filename));

  if (gdb_python_module == nullptr
      || ! PyObject_HasAttrString (gdb_python_module, "_execute_file"))
    error (_("Installation error: gdb._execute_file function is missing"));

  gdbpy_ref<> return_value
    (PyObject_CallMethod (gdb_python_module, "_execute_file", "s",
			  full_path.get ()));
  if (return_value == nullptr)
    {
      /* Use PyErr_PrintEx instead of gdbpy_print_stack to better match the
	 behavior of the non-Windows codepath.  */
      PyErr_PrintEx(0);
    }

#endif /* _WIN32 */
}

/* Given a command_line, return a command string suitable for passing
   to Python.  Lines in the string are separated by newlines.  */

static std::string
compute_python_string (struct command_line *l)
{
  struct command_line *iter;
  std::string script;

  for (iter = l; iter; iter = iter->next)
    {
      script += iter->line;
      script += '\n';
    }
  return script;
}

/* Take a command line structure representing a 'python' command, and
   evaluate its body using the Python interpreter.  */

static void
gdbpy_eval_from_control_command (const struct extension_language_defn *extlang,
				 struct command_line *cmd)
{
  int ret;

  if (cmd->body_list_1 != nullptr)
    error (_("Invalid \"python\" block structure."));

  gdbpy_enter enter_py;

  std::string script = compute_python_string (cmd->body_list_0.get ());
  ret = PyRun_SimpleString (script.c_str ());
  if (ret)
    error (_("Error while executing Python code."));
}

/* Implementation of the gdb "python" command.  */

static void
python_command (const char *arg, int from_tty)
{
  gdbpy_enter enter_py;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  arg = skip_spaces (arg);
  if (arg && *arg)
    {
      if (PyRun_SimpleString (arg))
	error (_("Error while executing Python code."));
    }
  else
    {
      counted_command_line l = get_command_line (python_control, "");

      execute_control_command_untraced (l.get ());
    }
}



/* Transform a gdb parameters's value into a Python value.  May return
   NULL (and set a Python exception) on error.  Helper function for
   get_parameter.  */
PyObject *
gdbpy_parameter_value (const setting &var)
{
  switch (var.type ())
    {
    case var_string:
    case var_string_noescape:
    case var_optional_filename:
    case var_filename:
    case var_enum:
      {
	const char *str;
	if (var.type () == var_enum)
	  str = var.get<const char *> ();
	else
	  str = var.get<std::string> ().c_str ();

	return host_string_to_python_string (str).release ();
      }

    case var_boolean:
      {
	if (var.get<bool> ())
	  Py_RETURN_TRUE;
	else
	  Py_RETURN_FALSE;
      }

    case var_auto_boolean:
      {
	enum auto_boolean ab = var.get<enum auto_boolean> ();

	if (ab == AUTO_BOOLEAN_TRUE)
	  Py_RETURN_TRUE;
	else if (ab == AUTO_BOOLEAN_FALSE)
	  Py_RETURN_FALSE;
	else
	  Py_RETURN_NONE;
      }

    case var_uinteger:
    case var_integer:
    case var_pinteger:
      {
	LONGEST value
	  = (var.type () == var_uinteger
	     ? static_cast<LONGEST> (var.get<unsigned int> ())
	     : static_cast<LONGEST> (var.get<int> ()));

	if (var.extra_literals () != nullptr)
	  for (const literal_def *l = var.extra_literals ();
	       l->literal != nullptr;
	       l++)
	    if (value == l->use)
	      {
		if (strcmp (l->literal, "unlimited") == 0)
		  {
		    /* Compatibility hack for API brokenness.  */
		    if (var.type () == var_pinteger
			&& l->val.has_value ()
			&& *l->val == -1)
		      value = -1;
		    else
		      Py_RETURN_NONE;
		  }
		else if (l->val.has_value ())
		  value = *l->val;
		else
		  return host_string_to_python_string (l->literal).release ();
	      }

	if (var.type () == var_uinteger)
	  return
	    gdb_py_object_from_ulongest
	      (static_cast<unsigned int> (value)).release ();
	else
	  return
	    gdb_py_object_from_longest
	      (static_cast<int> (value)).release ();
      }
    }

  return PyErr_Format (PyExc_RuntimeError,
		       _("Programmer error: unhandled type."));
}

/* A Python function which returns a gdb parameter's value as a Python
   value.  */

static PyObject *
gdbpy_parameter (PyObject *self, PyObject *args)
{
  struct cmd_list_element *alias, *prefix, *cmd;
  const char *arg;
  int found = -1;

  if (! PyArg_ParseTuple (args, "s", &arg))
    return NULL;

  std::string newarg = std::string ("show ") + arg;

  try
    {
      found = lookup_cmd_composition (newarg.c_str (), &alias, &prefix, &cmd);
    }
  catch (const gdb_exception &ex)
    {
      GDB_PY_HANDLE_EXCEPTION (ex);
    }

  if (!found)
    return PyErr_Format (PyExc_RuntimeError,
			 _("Could not find parameter `%s'."), arg);

  if (!cmd->var.has_value ())
    return PyErr_Format (PyExc_RuntimeError,
			 _("`%s' is not a parameter."), arg);

  return gdbpy_parameter_value (*cmd->var);
}

/* Wrapper for target_charset.  */

static PyObject *
gdbpy_target_charset (PyObject *self, PyObject *args)
{
  const char *cset = target_charset (gdbpy_enter::get_gdbarch ());

  return PyUnicode_Decode (cset, strlen (cset), host_charset (), NULL);
}

/* Wrapper for target_wide_charset.  */

static PyObject *
gdbpy_target_wide_charset (PyObject *self, PyObject *args)
{
  const char *cset = target_wide_charset (gdbpy_enter::get_gdbarch ());

  return PyUnicode_Decode (cset, strlen (cset), host_charset (), NULL);
}

/* Implement gdb.host_charset().  */

static PyObject *
gdbpy_host_charset (PyObject *self, PyObject *args)
{
  const char *cset = host_charset ();

  return PyUnicode_Decode (cset, strlen (cset), host_charset (), NULL);
}

/* A Python function which evaluates a string using the gdb CLI.  */

static PyObject *
execute_gdb_command (PyObject *self, PyObject *args, PyObject *kw)
{
  const char *arg;
  PyObject *from_tty_obj = nullptr;
  PyObject *to_string_obj = nullptr;
  static const char *keywords[] = { "command", "from_tty", "to_string",
				    nullptr };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O!O!", keywords, &arg,
					&PyBool_Type, &from_tty_obj,
					&PyBool_Type, &to_string_obj))
    return nullptr;

  bool from_tty = false;
  if (from_tty_obj != nullptr)
    {
      int cmp = PyObject_IsTrue (from_tty_obj);
      if (cmp < 0)
	return nullptr;
      from_tty = (cmp != 0);
    }

  bool to_string = false;
  if (to_string_obj != nullptr)
    {
      int cmp = PyObject_IsTrue (to_string_obj);
      if (cmp < 0)
	return nullptr;
      to_string = (cmp != 0);
    }

  std::string to_string_res;

  scoped_restore preventer = prevent_dont_repeat ();

  try
    {
      gdbpy_allow_threads allow_threads;

      struct interp *interp;

      std::string arg_copy = arg;
      bool first = true;
      char *save_ptr = nullptr;
      auto reader
	= [&] (std::string &buffer)
	  {
	    const char *result = strtok_r (first ? &arg_copy[0] : nullptr,
					   "\n", &save_ptr);
	    first = false;
	    return result;
	  };

      counted_command_line lines = read_command_lines_1 (reader, 1, nullptr);

      {
	scoped_restore save_async = make_scoped_restore (&current_ui->async,
							 0);

	scoped_restore save_uiout = make_scoped_restore (&current_uiout);

	/* Use the console interpreter uiout to have the same print format
	   for console or MI.  */
	interp = interp_lookup (current_ui, "console");
	current_uiout = interp->interp_ui_out ();

	if (to_string)
	  to_string_res = execute_control_commands_to_string (lines.get (),
							      from_tty);
	else
	  execute_control_commands (lines.get (), from_tty);
      }

      /* Do any commands attached to breakpoint we stopped at.  */
      bpstat_do_actions ();
    }
  catch (const gdb_exception &except)
    {
      /* If an exception occurred then we won't hit normal_stop (), or have
	 an exception reach the top level of the event loop, which are the
	 two usual places in which stdin would be re-enabled. So, before we
	 convert the exception and continue back in Python, we should
	 re-enable stdin here.  */
      async_enable_stdin ();
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (to_string)
    return PyUnicode_FromString (to_string_res.c_str ());
  Py_RETURN_NONE;
}

/* Implementation of Python rbreak command.  Take a REGEX and
   optionally a MINSYMS, THROTTLE and SYMTABS keyword and return a
   Python list that contains newly set breakpoints that match that
   criteria.  REGEX refers to a GDB format standard regex pattern of
   symbols names to search; MINSYMS is an optional boolean (default
   False) that indicates if the function should search GDB's minimal
   symbols; THROTTLE is an optional integer (default unlimited) that
   indicates the maximum amount of breakpoints allowable before the
   function exits (note, if the throttle bound is passed, no
   breakpoints will be set and a runtime error returned); SYMTABS is
   an optional Python iterable that contains a set of gdb.Symtabs to
   constrain the search within.  */

static PyObject *
gdbpy_rbreak (PyObject *self, PyObject *args, PyObject *kw)
{
  char *regex = NULL;
  std::vector<symbol_search> symbols;
  unsigned long count = 0;
  PyObject *symtab_list = NULL;
  PyObject *minsyms_p_obj = NULL;
  int minsyms_p = 0;
  unsigned int throttle = 0;
  static const char *keywords[] = {"regex","minsyms", "throttle",
				   "symtabs", NULL};

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O!IO", keywords,
					&regex, &PyBool_Type,
					&minsyms_p_obj, &throttle,
					&symtab_list))
    return NULL;

  /* Parse minsyms keyword.  */
  if (minsyms_p_obj != NULL)
    {
      int cmp = PyObject_IsTrue (minsyms_p_obj);
      if (cmp < 0)
	return NULL;
      minsyms_p = cmp;
    }

  global_symbol_searcher spec (FUNCTIONS_DOMAIN, regex);
  SCOPE_EXIT {
    for (const char *elem : spec.filenames)
      xfree ((void *) elem);
  };

  /* The "symtabs" keyword is any Python iterable object that returns
     a gdb.Symtab on each iteration.  If specified, iterate through
     the provided gdb.Symtabs and extract their full path.  As
     python_string_to_target_string returns a
     gdb::unique_xmalloc_ptr<char> and a vector containing these types
     cannot be coerced to a const char **p[] via the vector.data call,
     release the value from the unique_xmalloc_ptr and place it in a
     simple type symtab_list_type (which holds the vector and a
     destructor that frees the contents of the allocated strings.  */
  if (symtab_list != NULL)
    {
      gdbpy_ref<> iter (PyObject_GetIter (symtab_list));

      if (iter == NULL)
	return NULL;

      while (true)
	{
	  gdbpy_ref<> next (PyIter_Next (iter.get ()));

	  if (next == NULL)
	    {
	      if (PyErr_Occurred ())
		return NULL;
	      break;
	    }

	  gdbpy_ref<> obj_name (PyObject_GetAttrString (next.get (),
							"filename"));

	  if (obj_name == NULL)
	    return NULL;

	  /* Is the object file still valid?  */
	  if (obj_name == Py_None)
	    continue;

	  gdb::unique_xmalloc_ptr<char> filename =
	    python_string_to_target_string (obj_name.get ());

	  if (filename == NULL)
	    return NULL;

	  /* Make sure there is a definite place to store the value of
	     filename before it is released.  */
	  spec.filenames.push_back (nullptr);
	  spec.filenames.back () = filename.release ();
	}
    }

  /* The search spec.  */
  symbols = spec.search ();

  /* Count the number of symbols (both symbols and optionally minimal
     symbols) so we can correctly check the throttle limit.  */
  for (const symbol_search &p : symbols)
    {
      /* Minimal symbols included?  */
      if (minsyms_p)
	{
	  if (p.msymbol.minsym != NULL)
	    count++;
	}

      if (p.symbol != NULL)
	count++;
    }

  /* Check throttle bounds and exit if in excess.  */
  if (throttle != 0 && count > throttle)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Number of breakpoints exceeds throttled maximum."));
      return NULL;
    }

  gdbpy_ref<> return_list (PyList_New (0));

  if (return_list == NULL)
    return NULL;

  /* Construct full path names for symbols and call the Python
     breakpoint constructor on the resulting names.  Be tolerant of
     individual breakpoint failures.  */
  for (const symbol_search &p : symbols)
    {
      std::string symbol_name;

      /* Skipping minimal symbols?  */
      if (minsyms_p == 0)
	if (p.msymbol.minsym != NULL)
	  continue;

      if (p.msymbol.minsym == NULL)
	{
	  struct symtab *symtab = p.symbol->symtab ();
	  const char *fullname = symtab_to_fullname (symtab);

	  symbol_name = fullname;
	  symbol_name  += ":";
	  symbol_name  += p.symbol->linkage_name ();
	}
      else
	symbol_name = p.msymbol.minsym->linkage_name ();

      gdbpy_ref<> argList (Py_BuildValue("(s)", symbol_name.c_str ()));
      gdbpy_ref<> obj (PyObject_CallObject ((PyObject *)
					    &breakpoint_object_type,
					    argList.get ()));

      /* Tolerate individual breakpoint failures.  */
      if (obj == NULL)
	gdbpy_print_stack ();
      else
	{
	  if (PyList_Append (return_list.get (), obj.get ()) == -1)
	    return NULL;
	}
    }
  return return_list.release ();
}

/* A Python function which is a wrapper for decode_line_1.  */

static PyObject *
gdbpy_decode_line (PyObject *self, PyObject *args)
{
  const char *arg = NULL;
  gdbpy_ref<> result;
  gdbpy_ref<> unparsed;
  location_spec_up locspec;

  if (! PyArg_ParseTuple (args, "|s", &arg))
    return NULL;

  /* Treat a string consisting of just whitespace the same as
     NULL.  */
  if (arg != NULL)
    {
      arg = skip_spaces (arg);
      if (*arg == '\0')
	arg = NULL;
    }

  if (arg != NULL)
    locspec = string_to_location_spec_basic (&arg, current_language,
					     symbol_name_match_type::WILD);

  std::vector<symtab_and_line> decoded_sals;
  symtab_and_line def_sal;
  gdb::array_view<symtab_and_line> sals;
  try
    {
      if (locspec != NULL)
	{
	  decoded_sals = decode_line_1 (locspec.get (), 0, NULL, NULL, 0);
	  sals = decoded_sals;
	}
      else
	{
	  set_default_source_symtab_and_line ();
	  def_sal = get_current_source_symtab_and_line ();
	  sals = def_sal;
	}
    }
  catch (const gdb_exception &ex)
    {
      /* We know this will always throw.  */
      gdbpy_convert_exception (ex);
      return NULL;
    }

  if (!sals.empty ())
    {
      result.reset (PyTuple_New (sals.size ()));
      if (result == NULL)
	return NULL;
      for (size_t i = 0; i < sals.size (); ++i)
	{
	  PyObject *obj = symtab_and_line_to_sal_object (sals[i]);
	  if (obj == NULL)
	    return NULL;

	  PyTuple_SetItem (result.get (), i, obj);
	}
    }
  else
    result = gdbpy_ref<>::new_reference (Py_None);

  gdbpy_ref<> return_result (PyTuple_New (2));
  if (return_result == NULL)
    return NULL;

  if (arg != NULL && strlen (arg) > 0)
    {
      unparsed.reset (PyUnicode_FromString (arg));
      if (unparsed == NULL)
	return NULL;
    }
  else
    unparsed = gdbpy_ref<>::new_reference (Py_None);

  PyTuple_SetItem (return_result.get (), 0, unparsed.release ());
  PyTuple_SetItem (return_result.get (), 1, result.release ());

  return return_result.release ();
}

/* Parse a string and evaluate it as an expression.  */
static PyObject *
gdbpy_parse_and_eval (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "expression", "global_context", nullptr };

  const char *expr_str;
  PyObject *global_context_obj = nullptr;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|O!", keywords,
					&expr_str,
					&PyBool_Type, &global_context_obj))
    return nullptr;

  parser_flags flags = 0;
  if (global_context_obj != NULL)
    {
      int cmp = PyObject_IsTrue (global_context_obj);
      if (cmp < 0)
	return nullptr;
      if (cmp)
	flags |= PARSER_LEAVE_BLOCK_ALONE;
    }

  PyObject *result = nullptr;
  try
    {
      scoped_value_mark free_values;
      struct value *val;
      {
	/* Allow other Python threads to run while we're evaluating
	   the expression.  This is important because the expression
	   could involve inferior calls or otherwise be a lengthy
	   calculation.  We take care here to re-acquire the GIL here
	   before continuing with Python work.  */
	gdbpy_allow_threads allow_threads;
	val = parse_and_eval (expr_str, flags);
      }
      result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Implementation of gdb.invalidate_cached_frames.  */

static PyObject *
gdbpy_invalidate_cached_frames (PyObject *self, PyObject *args)
{
  reinit_frame_cache ();
  Py_RETURN_NONE;
}

/* Read a file as Python code.
   This is the extension_language_script_ops.script_sourcer "method".
   FILE is the file to load.  FILENAME is name of the file FILE.
   This does not throw any errors.  If an exception occurs python will print
   the traceback and clear the error indicator.  */

static void
gdbpy_source_script (const struct extension_language_defn *extlang,
		     FILE *file, const char *filename)
{
  gdbpy_enter enter_py;
  python_run_simple_file (file, filename);
}



/* Posting and handling events.  */

/* A single event.  */
struct gdbpy_event
{
  gdbpy_event (gdbpy_ref<> &&func)
    : m_func (func.release ())
  {
  }

  gdbpy_event (gdbpy_event &&other) noexcept
    : m_func (other.m_func)
  {
    other.m_func = nullptr;
  }

  gdbpy_event (const gdbpy_event &other)
    : m_func (other.m_func)
  {
    gdbpy_gil gil;
    Py_XINCREF (m_func);
  }

  ~gdbpy_event ()
  {
    gdbpy_gil gil;
    Py_XDECREF (m_func);
  }

  gdbpy_event &operator= (const gdbpy_event &other) = delete;

  void operator() ()
  {
    gdbpy_enter enter_py;

    gdbpy_ref<> call_result (PyObject_CallObject (m_func, NULL));
    if (call_result == NULL)
      gdbpy_print_stack ();
  }

private:

  /* The Python event.  This is just a callable object.  Note that
     this is not a gdbpy_ref<>, because we have to take particular
     care to only destroy the reference when holding the GIL. */
  PyObject *m_func;
};

/* Submit an event to the gdb thread.  */
static PyObject *
gdbpy_post_event (PyObject *self, PyObject *args)
{
  PyObject *func;

  if (!PyArg_ParseTuple (args, "O", &func))
    return NULL;

  if (!PyCallable_Check (func))
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Posted event is not callable"));
      return NULL;
    }

  gdbpy_ref<> func_ref = gdbpy_ref<>::new_reference (func);
  gdbpy_event event (std::move (func_ref));
  run_on_main_thread (event);

  Py_RETURN_NONE;
}

/* Interrupt the current operation on the main thread.  */
static PyObject *
gdbpy_interrupt (PyObject *self, PyObject *args)
{
  {
    /* Make sure the interrupt isn't delivered immediately somehow.
       This probably is not truly needed, but at the same time it
       seems more clear to be explicit about the intent.  */
    gdbpy_allow_threads temporarily_exit_python;
    scoped_disable_cooperative_sigint_handling no_python_sigint;

    set_quit_flag ();
  }

  Py_RETURN_NONE;
}



/* This is the extension_language_ops.before_prompt "method".  */

static enum ext_lang_rc
gdbpy_before_prompt_hook (const struct extension_language_defn *extlang,
			  const char *current_gdb_prompt)
{
  if (!gdb_python_initialized)
    return EXT_LANG_RC_NOP;

  gdbpy_enter enter_py;

  if (!evregpy_no_listeners_p (gdb_py_events.before_prompt)
      && evpy_emit_event (NULL, gdb_py_events.before_prompt) < 0)
    return EXT_LANG_RC_ERROR;

  if (gdb_python_module
      && PyObject_HasAttrString (gdb_python_module, "prompt_hook"))
    {
      gdbpy_ref<> hook (PyObject_GetAttrString (gdb_python_module,
						"prompt_hook"));
      if (hook == NULL)
	{
	  gdbpy_print_stack ();
	  return EXT_LANG_RC_ERROR;
	}

      if (PyCallable_Check (hook.get ()))
	{
	  gdbpy_ref<> current_prompt (PyUnicode_FromString (current_gdb_prompt));
	  if (current_prompt == NULL)
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  gdbpy_ref<> result
	    (PyObject_CallFunctionObjArgs (hook.get (), current_prompt.get (),
					   NULL));
	  if (result == NULL)
	    {
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  /* Return type should be None, or a String.  If it is None,
	     fall through, we will not set a prompt.  If it is a
	     string, set  PROMPT.  Anything else, set an exception.  */
	  if (result != Py_None && !PyUnicode_Check (result.get ()))
	    {
	      PyErr_Format (PyExc_RuntimeError,
			    _("Return from prompt_hook must " \
			      "be either a Python string, or None"));
	      gdbpy_print_stack ();
	      return EXT_LANG_RC_ERROR;
	    }

	  if (result != Py_None)
	    {
	      gdb::unique_xmalloc_ptr<char>
		prompt (python_string_to_host_string (result.get ()));

	      if (prompt == NULL)
		{
		  gdbpy_print_stack ();
		  return EXT_LANG_RC_ERROR;
		}

	      set_prompt (prompt.get ());
	      return EXT_LANG_RC_OK;
	    }
	}
    }

  return EXT_LANG_RC_NOP;
}

/* This is the extension_language_ops.colorize "method".  */

static std::optional<std::string>
gdbpy_colorize (const std::string &filename, const std::string &contents)
{
  if (!gdb_python_initialized)
    return {};

  gdbpy_enter enter_py;

  gdbpy_ref<> module (PyImport_ImportModule ("gdb.styling"));
  if (module == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  if (!PyObject_HasAttrString (module.get (), "colorize"))
    return {};

  gdbpy_ref<> hook (PyObject_GetAttrString (module.get (), "colorize"));
  if (hook == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  if (!PyCallable_Check (hook.get ()))
    return {};

  gdbpy_ref<> fname_arg (PyUnicode_FromString (filename.c_str ()));
  if (fname_arg == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  /* The pygments library, which is what we currently use for applying
     styling, is happy to take input as a bytes object, and to figure out
     the encoding for itself.  This removes the need for us to figure out
     (guess?) at how the content is encoded, which is probably a good
     thing.  */
  gdbpy_ref<> contents_arg (PyBytes_FromStringAndSize (contents.c_str (),
						       contents.size ()));
  if (contents_arg == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  /* Calling gdb.colorize passing in the filename (a string), and the file
     contents (a bytes object).  This function should return either a bytes
     object, the same contents with styling applied, or None to indicate
     that no styling should be performed.  */
  gdbpy_ref<> result (PyObject_CallFunctionObjArgs (hook.get (),
						    fname_arg.get (),
						    contents_arg.get (),
						    nullptr));
  if (result == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  if (result == Py_None)
    return {};
  else if (!PyBytes_Check (result.get ()))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Return value from gdb.colorize should be a bytes object or None."));
      gdbpy_print_stack ();
      return {};
    }

  return std::string (PyBytes_AsString (result.get ()));
}

/* This is the extension_language_ops.colorize_disasm "method".  */

static std::optional<std::string>
gdbpy_colorize_disasm (const std::string &content, gdbarch *gdbarch)
{
  if (!gdb_python_initialized)
    return {};

  gdbpy_enter enter_py;

  gdbpy_ref<> module (PyImport_ImportModule ("gdb.styling"));
  if (module == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  if (!PyObject_HasAttrString (module.get (), "colorize_disasm"))
    return {};

  gdbpy_ref<> hook (PyObject_GetAttrString (module.get (),
					    "colorize_disasm"));
  if (hook == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  if (!PyCallable_Check (hook.get ()))
    return {};

  gdbpy_ref<> content_arg (PyBytes_FromString (content.c_str ()));
  if (content_arg == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  gdbpy_ref<> gdbarch_arg (gdbarch_to_arch_object (gdbarch));
  if (gdbarch_arg == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  gdbpy_ref<> result (PyObject_CallFunctionObjArgs (hook.get (),
						    content_arg.get (),
						    gdbarch_arg.get (),
						    nullptr));
  if (result == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  if (result == Py_None)
    return {};

  if (!PyBytes_Check (result.get ()))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Return value from gdb.colorize_disasm should be a bytes object or None."));
      gdbpy_print_stack ();
      return {};
    }

  return std::string (PyBytes_AsString (result.get ()));
}



/* Implement gdb.format_address(ADDR,P_SPACE,ARCH).  Provide access to
   GDB's print_address function from Python.  The returned address will
   have the format '0x..... <symbol+offset>'.  */

static PyObject *
gdbpy_format_address (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] =
    {
      "address", "progspace", "architecture", nullptr
    };
  PyObject *addr_obj = nullptr, *pspace_obj = nullptr, *arch_obj = nullptr;
  CORE_ADDR addr;
  struct gdbarch *gdbarch = nullptr;
  struct program_space *pspace = nullptr;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O|OO", keywords,
					&addr_obj, &pspace_obj, &arch_obj))
    return nullptr;

  if (get_addr_from_python (addr_obj, &addr) < 0)
    return nullptr;

  /* If the user passed None for progspace or architecture, then we
     consider this to mean "the default".  Here we replace references to
     None with nullptr, this means that in the following code we only have
     to handle the nullptr case.  These are only borrowed references, so
     no decref is required here.  */
  if (pspace_obj == Py_None)
    pspace_obj = nullptr;
  if (arch_obj == Py_None)
    arch_obj = nullptr;

  if (pspace_obj == nullptr && arch_obj == nullptr)
    {
      /* Grab both of these from the current inferior, and its associated
	 default architecture.  */
      pspace = current_inferior ()->pspace;
      gdbarch = current_inferior ()->arch ();
    }
  else if (arch_obj == nullptr || pspace_obj == nullptr)
    {
      /* If the user has only given one of program space or architecture,
	 then don't use the default for the other.  Sure we could use the
	 default, but it feels like there's too much scope of mistakes in
	 this case, so better to require the user to provide both
	 arguments.  */
      PyErr_SetString (PyExc_ValueError,
		       _("The architecture and progspace arguments must both be supplied"));
      return nullptr;
    }
  else
    {
      /* The user provided an address, program space, and architecture.
	 Just check that these objects are valid.  */
      if (!gdbpy_is_progspace (pspace_obj))
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("The progspace argument is not a gdb.Progspace object"));
	  return nullptr;
	}

      pspace = progspace_object_to_program_space (pspace_obj);
      if (pspace == nullptr)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("The progspace argument is not valid"));
	  return nullptr;
	}

      if (!gdbpy_is_architecture (arch_obj))
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("The architecture argument is not a gdb.Architecture object"));
	  return nullptr;
	}

      /* Architectures are never deleted once created, so gdbarch should
	 never come back as nullptr.  */
      gdbarch = arch_object_to_gdbarch (arch_obj);
      gdb_assert (gdbarch != nullptr);
    }

  /* By this point we should know the program space and architecture we are
     going to use.  */
  gdb_assert (pspace != nullptr);
  gdb_assert (gdbarch != nullptr);

  /* Unfortunately print_address relies on the current program space for
     its symbol lookup.  Temporarily switch now.  */
  scoped_restore_current_program_space restore_progspace;
  set_current_program_space (pspace);

  /* Format the address, and return it as a string.  */
  string_file buf;
  print_address (gdbarch, addr, &buf);
  return PyUnicode_FromString (buf.c_str ());
}



/* Printing.  */

/* A python function to write a single string using gdb's filtered
   output stream .  The optional keyword STREAM can be used to write
   to a particular stream.  The default stream is to gdb_stdout.  */

static PyObject *
gdbpy_write (PyObject *self, PyObject *args, PyObject *kw)
{
  const char *arg;
  static const char *keywords[] = { "text", "stream", NULL };
  int stream_type = 0;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s|i", keywords, &arg,
					&stream_type))
    return NULL;

  try
    {
      switch (stream_type)
	{
	case 1:
	  {
	    gdb_printf (gdb_stderr, "%s", arg);
	    break;
	  }
	case 2:
	  {
	    gdb_printf (gdb_stdlog, "%s", arg);
	    break;
	  }
	default:
	  gdb_printf (gdb_stdout, "%s", arg);
	}
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* A python function to flush a gdb stream.  The optional keyword
   STREAM can be used to flush a particular stream.  The default stream
   is gdb_stdout.  */

static PyObject *
gdbpy_flush (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "stream", NULL };
  int stream_type = 0;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "|i", keywords,
					&stream_type))
    return NULL;

  switch (stream_type)
    {
    case 1:
      {
	gdb_flush (gdb_stderr);
	break;
      }
    case 2:
      {
	gdb_flush (gdb_stdlog);
	break;
      }
    default:
      gdb_flush (gdb_stdout);
    }

  Py_RETURN_NONE;
}

/* Return non-zero if print-stack is not "none".  */

int
gdbpy_print_python_errors_p (void)
{
  return gdbpy_should_print_stack != python_excp_none;
}

/* Print a python exception trace, print just a message, or print
   nothing and clear the python exception, depending on
   gdbpy_should_print_stack.  Only call this if a python exception is
   set.  */
void
gdbpy_print_stack (void)
{

  /* Print "none", just clear exception.  */
  if (gdbpy_should_print_stack == python_excp_none)
    {
      PyErr_Clear ();
    }
  /* Print "full" message and backtrace.  */
  else if (gdbpy_should_print_stack == python_excp_full)
    {
      PyErr_Print ();
      /* PyErr_Print doesn't necessarily end output with a newline.
	 This works because Python's stdout/stderr is fed through
	 gdb_printf.  */
      try
	{
	  begin_line ();
	}
      catch (const gdb_exception &except)
	{
	}
    }
  /* Print "message", just error print message.  */
  else
    {
      gdbpy_err_fetch fetched_error;

      gdb::unique_xmalloc_ptr<char> msg = fetched_error.to_string ();
      gdb::unique_xmalloc_ptr<char> type;
      /* Don't compute TYPE if MSG already indicates that there is an
	 error.  */
      if (msg != NULL)
	type = fetched_error.type_to_string ();

      try
	{
	  if (msg == NULL || type == NULL)
	    {
	      /* An error occurred computing the string representation of the
		 error message.  */
	      gdb_printf (gdb_stderr,
			  _("Error occurred computing Python error" \
			    "message.\n"));
	      PyErr_Clear ();
	    }
	  else
	    gdb_printf (gdb_stderr, "Python Exception %s: %s\n",
			type.get (), msg.get ());
	}
      catch (const gdb_exception &except)
	{
	}
    }
}

/* Like gdbpy_print_stack, but if the exception is a
   KeyboardException, throw a gdb "quit" instead.  */

void
gdbpy_print_stack_or_quit ()
{
  if (PyErr_ExceptionMatches (PyExc_KeyboardInterrupt))
    {
      PyErr_Clear ();
      throw_quit ("Quit");
    }
  gdbpy_print_stack ();
}



/* Return a sequence holding all the Progspaces.  */

static PyObject *
gdbpy_progspaces (PyObject *unused1, PyObject *unused2)
{
  gdbpy_ref<> list (PyList_New (0));
  if (list == NULL)
    return NULL;

  for (struct program_space *ps : program_spaces)
    {
      gdbpy_ref<> item = pspace_to_pspace_object (ps);

      if (item == NULL || PyList_Append (list.get (), item.get ()) == -1)
	return NULL;
    }

  return list.release ();
}

/* Return the name of the current language.  */

static PyObject *
gdbpy_current_language (PyObject *unused1, PyObject *unused2)
{
  return host_string_to_python_string (current_language->name ()).release ();
}



/* See python.h.  */
struct objfile *gdbpy_current_objfile;

/* Set the current objfile to OBJFILE and then read FILE named FILENAME
   as Python code.  This does not throw any errors.  If an exception
   occurs python will print the traceback and clear the error indicator.
   This is the extension_language_script_ops.objfile_script_sourcer
   "method".  */

static void
gdbpy_source_objfile_script (const struct extension_language_defn *extlang,
			     struct objfile *objfile, FILE *file,
			     const char *filename)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (objfile->arch ());
  scoped_restore restire_current_objfile
    = make_scoped_restore (&gdbpy_current_objfile, objfile);

  python_run_simple_file (file, filename);
}

/* Set the current objfile to OBJFILE and then execute SCRIPT
   as Python code.  This does not throw any errors.  If an exception
   occurs python will print the traceback and clear the error indicator.
   This is the extension_language_script_ops.objfile_script_executor
   "method".  */

static void
gdbpy_execute_objfile_script (const struct extension_language_defn *extlang,
			      struct objfile *objfile, const char *name,
			      const char *script)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py (objfile->arch ());
  scoped_restore restire_current_objfile
    = make_scoped_restore (&gdbpy_current_objfile, objfile);

  PyRun_SimpleString (script);
}

/* Return the current Objfile, or None if there isn't one.  */

static PyObject *
gdbpy_get_current_objfile (PyObject *unused1, PyObject *unused2)
{
  if (! gdbpy_current_objfile)
    Py_RETURN_NONE;

  return objfile_to_objfile_object (gdbpy_current_objfile).release ();
}

/* Implement the 'handle_missing_debuginfo' hook for Python.  GDB has
   failed to find any debug information for OBJFILE.  The extension has a
   chance to record this, or even install the required debug information.
   See the description of ext_lang_missing_debuginfo_result in
   extension-priv.h for details of the return value.  */

static ext_lang_missing_debuginfo_result
gdbpy_handle_missing_debuginfo (const struct extension_language_defn *extlang,
				struct objfile *objfile)
{
  /* Early exit if Python is not initialised.  */
  if (!gdb_python_initialized || gdb_python_module == nullptr)
    return {};

  struct gdbarch *gdbarch = objfile->arch ();

  gdbpy_enter enter_py (gdbarch);

  /* Convert OBJFILE into the corresponding Python object.  */
  gdbpy_ref<> pyo_objfile = objfile_to_objfile_object (objfile);
  if (pyo_objfile == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  /* Lookup the helper function within the GDB module.  */
  gdbpy_ref<> pyo_handler
    (PyObject_GetAttrString (gdb_python_module, "_handle_missing_debuginfo"));
  if (pyo_handler == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  /* Call the function, passing in the Python objfile object.  */
  gdbpy_ref<> pyo_execute_ret
    (PyObject_CallFunctionObjArgs (pyo_handler.get (), pyo_objfile.get (),
				   nullptr));
  if (pyo_execute_ret == nullptr)
    {
      /* If the handler is cancelled due to a Ctrl-C, then propagate
	 the Ctrl-C as a GDB exception instead of swallowing it.  */
      gdbpy_print_stack_or_quit ();
      return {};
    }

  /* Parse the result, and convert it back to the C++ object.  */
  if (pyo_execute_ret == Py_None)
    return {};

  if (PyBool_Check (pyo_execute_ret.get ()))
    {
      bool try_again = PyObject_IsTrue (pyo_execute_ret.get ());
      return ext_lang_missing_debuginfo_result (try_again);
    }

  if (!gdbpy_is_string (pyo_execute_ret.get ()))
    {
      PyErr_SetString (PyExc_ValueError,
		       "return value from _handle_missing_debuginfo should "
		       "be None, a Bool, or a String");
      gdbpy_print_stack ();
      return {};
    }

  gdb::unique_xmalloc_ptr<char> filename
    = python_string_to_host_string (pyo_execute_ret.get ());
  if (filename == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  return ext_lang_missing_debuginfo_result (std::string (filename.get ()));
}

/* Compute the list of active python type printers and store them in
   EXT_PRINTERS->py_type_printers.  The product of this function is used by
   gdbpy_apply_type_printers, and freed by gdbpy_free_type_printers.
   This is the extension_language_ops.start_type_printers "method".  */

static void
gdbpy_start_type_printers (const struct extension_language_defn *extlang,
			   struct ext_lang_type_printers *ext_printers)
{
  PyObject *printers_obj = NULL;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  gdbpy_ref<> type_module (PyImport_ImportModule ("gdb.types"));
  if (type_module == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  gdbpy_ref<> func (PyObject_GetAttrString (type_module.get (),
					    "get_type_recognizers"));
  if (func == NULL)
    {
      gdbpy_print_stack ();
      return;
    }

  printers_obj = PyObject_CallFunctionObjArgs (func.get (), (char *) NULL);
  if (printers_obj == NULL)
    gdbpy_print_stack ();
  else
    ext_printers->py_type_printers = printers_obj;
}

/* If TYPE is recognized by some type printer, store in *PRETTIED_TYPE
   a newly allocated string holding the type's replacement name, and return
   EXT_LANG_RC_OK.
   If there's a Python error return EXT_LANG_RC_ERROR.
   Otherwise, return EXT_LANG_RC_NOP.
   This is the extension_language_ops.apply_type_printers "method".  */

static enum ext_lang_rc
gdbpy_apply_type_printers (const struct extension_language_defn *extlang,
			   const struct ext_lang_type_printers *ext_printers,
			   struct type *type,
			   gdb::unique_xmalloc_ptr<char> *prettied_type)
{
  PyObject *printers_obj = (PyObject *) ext_printers->py_type_printers;
  gdb::unique_xmalloc_ptr<char> result;

  if (printers_obj == NULL)
    return EXT_LANG_RC_NOP;

  if (!gdb_python_initialized)
    return EXT_LANG_RC_NOP;

  gdbpy_enter enter_py;

  gdbpy_ref<> type_obj (type_to_type_object (type));
  if (type_obj == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> type_module (PyImport_ImportModule ("gdb.types"));
  if (type_module == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> func (PyObject_GetAttrString (type_module.get (),
					    "apply_type_recognizers"));
  if (func == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  gdbpy_ref<> result_obj (PyObject_CallFunctionObjArgs (func.get (),
							printers_obj,
							type_obj.get (),
							(char *) NULL));
  if (result_obj == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  if (result_obj == Py_None)
    return EXT_LANG_RC_NOP;

  result = python_string_to_host_string (result_obj.get ());
  if (result == NULL)
    {
      gdbpy_print_stack ();
      return EXT_LANG_RC_ERROR;
    }

  *prettied_type = std::move (result);
  return EXT_LANG_RC_OK;
}

/* Free the result of start_type_printers.
   This is the extension_language_ops.free_type_printers "method".  */

static void
gdbpy_free_type_printers (const struct extension_language_defn *extlang,
			  struct ext_lang_type_printers *ext_printers)
{
  PyObject *printers = (PyObject *) ext_printers->py_type_printers;

  if (printers == NULL)
    return;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;
  Py_DECREF (printers);
}

#else /* HAVE_PYTHON */

/* Dummy implementation of the gdb "python-interactive" and "python"
   command. */

static void
python_interactive_command (const char *arg, int from_tty)
{
  arg = skip_spaces (arg);
  if (arg && *arg)
    error (_("Python scripting is not supported in this copy of GDB."));
  else
    {
      counted_command_line l = get_command_line (python_control, "");

      execute_control_command_untraced (l.get ());
    }
}

static void
python_command (const char *arg, int from_tty)
{
  python_interactive_command (arg, from_tty);
}

#endif /* HAVE_PYTHON */

/* When this is turned on before Python is initialised then Python will
   ignore any environment variables related to Python.  This is equivalent
   to passing `-E' to the python program.  */
static bool python_ignore_environment = false;

/* Implement 'show python ignore-environment'.  */

static void
show_python_ignore_environment (struct ui_file *file, int from_tty,
				struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Python's ignore-environment setting is %s.\n"),
	      value);
}

/* Implement 'set python ignore-environment'.  This sets Python's internal
   flag no matter when the command is issued, however, if this is used
   after Py_Initialize has been called then most of the environment will
   already have been read.  */

static void
set_python_ignore_environment (const char *args, int from_tty,
			       struct cmd_list_element *c)
{
#ifdef HAVE_PYTHON
  /* Py_IgnoreEnvironmentFlag is deprecated in Python 3.12.  Disable
     its usage in Python 3.10 and above since the PyConfig mechanism
     is now (also) used in 3.10 and higher.  See do_start_initialization()
     in this file.  */
#if PY_VERSION_HEX < 0x030a0000
  Py_IgnoreEnvironmentFlag = python_ignore_environment ? 1 : 0;
#endif
#endif
}

/* When this is turned on before Python is initialised then Python will
   not write `.pyc' files on import of a module.  */
static enum auto_boolean python_dont_write_bytecode = AUTO_BOOLEAN_AUTO;

/* Implement 'show python dont-write-bytecode'.  */

static void
show_python_dont_write_bytecode (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c, const char *value)
{
  if (python_dont_write_bytecode == AUTO_BOOLEAN_AUTO)
    {
      const char *auto_string
	= (python_ignore_environment
	   || getenv ("PYTHONDONTWRITEBYTECODE") == nullptr) ? "off" : "on";

      gdb_printf (file,
		  _("Python's dont-write-bytecode setting is %s (currently %s).\n"),
		  value, auto_string);
    }
  else
    gdb_printf (file, _("Python's dont-write-bytecode setting is %s.\n"),
		value);
}

#ifdef HAVE_PYTHON
/* Return value to assign to PyConfig.write_bytecode or, when
   negated (via !), Py_DontWriteBytecodeFlag.  Py_DontWriteBytecodeFlag
   is deprecated in Python 3.12.  */

static int
python_write_bytecode ()
{
  int wbc = 0;

  if (python_dont_write_bytecode == AUTO_BOOLEAN_AUTO)
    {
      if (python_ignore_environment)
	wbc = 1;
      else
	{
	  const char *pdwbc = getenv ("PYTHONDONTWRITEBYTECODE");
	  wbc = (pdwbc == nullptr || pdwbc[0] == '\0') ? 1 : 0;
	}
    }
  else
    wbc = python_dont_write_bytecode == AUTO_BOOLEAN_TRUE ? 0 : 1;

  return wbc;
}
#endif /* HAVE_PYTHON */

/* Implement 'set python dont-write-bytecode'.  This sets Python's internal
   flag no matter when the command is issued, however, if this is used
   after Py_Initialize has been called then many modules could already
   have been imported and their byte code written out.  */

static void
set_python_dont_write_bytecode (const char *args, int from_tty,
				struct cmd_list_element *c)
{
#ifdef HAVE_PYTHON
  /* Py_DontWriteBytecodeFlag is deprecated in Python 3.12.  Disable
     its usage in Python 3.10 and above since the PyConfig mechanism
     is now (also) used in 3.10 and higher.  See do_start_initialization()
     in this file.  */
#if PY_VERSION_HEX < 0x030a0000
  Py_DontWriteBytecodeFlag = !python_write_bytecode ();
#endif
#endif /* HAVE_PYTHON */
}



/* Lists for 'set python' commands.  */

static struct cmd_list_element *user_set_python_list;
static struct cmd_list_element *user_show_python_list;

/* Initialize the Python code.  */

#ifdef HAVE_PYTHON

/* This is installed as a final cleanup and cleans up the
   interpreter.  This lets Python's 'atexit' work.  */

static void
finalize_python (void *ignore)
{
  struct active_ext_lang_state *previous_active;

  /* We don't use ensure_python_env here because if we ever ran the
     cleanup, gdb would crash -- because the cleanup calls into the
     Python interpreter, which we are about to destroy.  It seems
     clearer to make the needed calls explicitly here than to create a
     cleanup and then mysteriously discard it.  */

  /* This is only called as a final cleanup so we can assume the active
     SIGINT handler is gdb's.  We still need to tell it to notify Python.  */
  previous_active = set_active_ext_lang (&extension_language_python);

  (void) PyGILState_Ensure ();
  gdbpy_enter::finalize ();

  /* Call the gdbpy_finalize_* functions from every *.c file.  */
  gdbpy_initialize_file::finalize_all ();

  Py_Finalize ();

  gdb_python_initialized = false;
  restore_active_ext_lang (previous_active);
}

static struct PyModuleDef python_GdbModuleDef =
{
  PyModuleDef_HEAD_INIT,
  "_gdb",
  NULL,
  -1,
  python_GdbMethods,
  NULL,
  NULL,
  NULL,
  NULL
};

/* This is called via the PyImport_AppendInittab mechanism called
   during initialization, to make the built-in _gdb module known to
   Python.  */
PyMODINIT_FUNC init__gdb_module (void);
PyMODINIT_FUNC
init__gdb_module (void)
{
  return PyModule_Create (&python_GdbModuleDef);
}

/* Emit a gdb.GdbExitingEvent, return a negative value if there are any
   errors, otherwise, return 0.  */

static int
emit_exiting_event (int exit_code)
{
  if (evregpy_no_listeners_p (gdb_py_events.gdb_exiting))
    return 0;

  gdbpy_ref<> event_obj = create_event_object (&gdb_exiting_event_object_type);
  if (event_obj == nullptr)
    return -1;

  gdbpy_ref<> code = gdb_py_object_from_longest (exit_code);
  if (evpy_add_attribute (event_obj.get (), "exit_code", code.get ()) < 0)
    return -1;

  return evpy_emit_event (event_obj.get (), gdb_py_events.gdb_exiting);
}

/* Callback for the gdb_exiting observable.  EXIT_CODE is the value GDB
   will exit with.  */

static void
gdbpy_gdb_exiting (int exit_code)
{
  if (!gdb_python_initialized)
    return;

  gdbpy_enter enter_py;

  if (emit_exiting_event (exit_code) < 0)
    gdbpy_print_stack ();
}

static bool
do_start_initialization ()
{
  /* Define all internal modules.  These are all imported (and thus
     created) during initialization.  */
  struct _inittab mods[] =
  {
    { "_gdb", init__gdb_module },
    { "_gdbevents", gdbpy_events_mod_func },
    { nullptr, nullptr }
  };

  if (PyImport_ExtendInittab (mods) < 0)
    return false;

#ifdef WITH_PYTHON_PATH
  /* Work around problem where python gets confused about where it is,
     and then can't find its libraries, etc.
     NOTE: Python assumes the following layout:
     /foo/bin/python
     /foo/lib/pythonX.Y/...
     This must be done before calling Py_Initialize.  */
  gdb::unique_xmalloc_ptr<char> progname
    (concat (ldirname (python_libdir.c_str ()).c_str (), SLASH_STRING, "bin",
	      SLASH_STRING, "python", (char *) NULL));
  /* Python documentation indicates that the memory given
     to Py_SetProgramName cannot be freed.  However, it seems that
     at least Python 3.7.4 Py_SetProgramName takes a copy of the
     given program_name.  Making progname_copy static and not release
     the memory avoids a leak report for Python versions that duplicate
     program_name, and respect the requirement of Py_SetProgramName
     for Python versions that do not duplicate program_name.  */
  static wchar_t *progname_copy;

  std::string oldloc = setlocale (LC_ALL, NULL);
  setlocale (LC_ALL, "");
  size_t progsize = strlen (progname.get ());
  progname_copy = XNEWVEC (wchar_t, progsize + 1);
  size_t count = mbstowcs (progname_copy, progname.get (), progsize + 1);
  if (count == (size_t) -1)
    {
      fprintf (stderr, "Could not convert python path to string\n");
      return false;
    }
  setlocale (LC_ALL, oldloc.c_str ());

  /* Py_SetProgramName was deprecated in Python 3.11.  Use PyConfig
     mechanisms for Python 3.10 and newer.  */
#if PY_VERSION_HEX < 0x030a0000
  /* Note that Py_SetProgramName expects the string it is passed to
     remain alive for the duration of the program's execution, so
     it is not freed after this call.  */
  Py_SetProgramName (progname_copy);
  Py_Initialize ();
#else
  PyConfig config;

  PyConfig_InitPythonConfig (&config);
  PyStatus status = PyConfig_SetString (&config, &config.program_name,
					progname_copy);
  if (PyStatus_Exception (status))
    goto init_done;

  config.write_bytecode = python_write_bytecode ();
  config.use_environment = !python_ignore_environment;

  status = PyConfig_Read (&config);
  if (PyStatus_Exception (status))
    goto init_done;

  status = Py_InitializeFromConfig (&config);

init_done:
  PyConfig_Clear (&config);
  if (PyStatus_Exception (status))
    return false;
#endif
#else
  Py_Initialize ();
#endif

#if PY_VERSION_HEX < 0x03090000
  /* PyEval_InitThreads became deprecated in Python 3.9 and will
     be removed in Python 3.11.  Prior to Python 3.7, this call was
     required to initialize the GIL.  */
  PyEval_InitThreads ();
#endif

  gdb_module = PyImport_ImportModule ("_gdb");
  if (gdb_module == NULL)
    return false;

  if (PyModule_AddStringConstant (gdb_module, "VERSION", version) < 0
      || PyModule_AddStringConstant (gdb_module, "HOST_CONFIG", host_name) < 0
      || PyModule_AddStringConstant (gdb_module, "TARGET_CONFIG",
				     target_name) < 0)
    return false;

  /* Add stream constants.  */
  if (PyModule_AddIntConstant (gdb_module, "STDOUT", 0) < 0
      || PyModule_AddIntConstant (gdb_module, "STDERR", 1) < 0
      || PyModule_AddIntConstant (gdb_module, "STDLOG", 2) < 0)
    return false;

  gdbpy_gdb_error = PyErr_NewException ("gdb.error", PyExc_RuntimeError, NULL);
  if (gdbpy_gdb_error == NULL
      || gdb_pymodule_addobject (gdb_module, "error", gdbpy_gdb_error) < 0)
    return false;

  gdbpy_gdb_memory_error = PyErr_NewException ("gdb.MemoryError",
					       gdbpy_gdb_error, NULL);
  if (gdbpy_gdb_memory_error == NULL
      || gdb_pymodule_addobject (gdb_module, "MemoryError",
				 gdbpy_gdb_memory_error) < 0)
    return false;

  gdbpy_gdberror_exc = PyErr_NewException ("gdb.GdbError", NULL, NULL);
  if (gdbpy_gdberror_exc == NULL
      || gdb_pymodule_addobject (gdb_module, "GdbError",
				 gdbpy_gdberror_exc) < 0)
    return false;

  /* Call the gdbpy_initialize_* functions from every *.c file.  */
  if (!gdbpy_initialize_file::initialize_all ())
    return false;

#define GDB_PY_DEFINE_EVENT_TYPE(name, py_name, doc, base)	\
  if (gdbpy_initialize_event_generic (&name##_event_object_type, py_name) < 0) \
    return false;
#include "py-event-types.def"
#undef GDB_PY_DEFINE_EVENT_TYPE

  gdbpy_to_string_cst = PyUnicode_FromString ("to_string");
  if (gdbpy_to_string_cst == NULL)
    return false;
  gdbpy_children_cst = PyUnicode_FromString ("children");
  if (gdbpy_children_cst == NULL)
    return false;
  gdbpy_display_hint_cst = PyUnicode_FromString ("display_hint");
  if (gdbpy_display_hint_cst == NULL)
    return false;
  gdbpy_doc_cst = PyUnicode_FromString ("__doc__");
  if (gdbpy_doc_cst == NULL)
    return false;
  gdbpy_enabled_cst = PyUnicode_FromString ("enabled");
  if (gdbpy_enabled_cst == NULL)
    return false;
  gdbpy_value_cst = PyUnicode_FromString ("value");
  if (gdbpy_value_cst == NULL)
    return false;

  gdb::observers::gdb_exiting.attach (gdbpy_gdb_exiting, "python");

  /* Release the GIL while gdb runs.  */
  PyEval_SaveThread ();

  make_final_cleanup (finalize_python, NULL);

  /* Only set this when initialization has succeeded.  */
  gdb_python_initialized = 1;
  return true;
}

#if GDB_SELF_TEST
namespace selftests {

/* Entry point for python unit tests.  */

static void
test_python ()
{
#define CMD(S) execute_command_to_string (S, "python print(5)", 0, true)

  std::string output;

  CMD (output);
  SELF_CHECK (output == "5\n");
  output.clear ();

  bool saw_exception = false;
  {
    scoped_restore reset_gdb_python_initialized
      = make_scoped_restore (&gdb_python_initialized, 0);
    try
      {
	CMD (output);
      }
    catch (const gdb_exception &e)
      {
	saw_exception = true;
	SELF_CHECK (e.reason == RETURN_ERROR);
	SELF_CHECK (e.error == GENERIC_ERROR);
	SELF_CHECK (*e.message == "Python not initialized");
      }
    SELF_CHECK (saw_exception);
    SELF_CHECK (output.empty ());
  }

  saw_exception = false;
  {
    scoped_restore save_hook
      = make_scoped_restore (&hook_set_active_ext_lang,
			     []() { raise (SIGINT); });
    try
      {
	CMD (output);
      }
    catch (const gdb_exception &e)
      {
	saw_exception = true;
	SELF_CHECK (e.reason == RETURN_ERROR);
	SELF_CHECK (e.error == GENERIC_ERROR);
	SELF_CHECK (*e.message == "Error while executing Python code.");
      }
    SELF_CHECK (saw_exception);
    std::string ref_output_0 ("Traceback (most recent call last):\n"
			      "  File \"<string>\", line 0, in <module>\n"
			      "KeyboardInterrupt\n");
    std::string ref_output_1 ("Traceback (most recent call last):\n"
			      "  File \"<string>\", line 1, in <module>\n"
			      "KeyboardInterrupt\n");
    SELF_CHECK (output == ref_output_0 || output == ref_output_1);
  }

#undef CMD
}

#undef CHECK_OUTPUT

} // namespace selftests
#endif /* GDB_SELF_TEST */

#endif /* HAVE_PYTHON */

/* See python.h.  */
cmd_list_element *python_cmd_element = nullptr;

void _initialize_python ();
void
_initialize_python ()
{
  cmd_list_element *python_interactive_cmd
    =	add_com ("python-interactive", class_obscure,
		 python_interactive_command,
#ifdef HAVE_PYTHON
	   _("\
Start an interactive Python prompt.\n\
\n\
To return to GDB, type the EOF character (e.g., Ctrl-D on an empty\n\
prompt).\n\
\n\
Alternatively, a single-line Python command can be given as an\n\
argument, and if the command is an expression, the result will be\n\
printed.  For example:\n\
\n\
    (gdb) python-interactive 2 + 3\n\
    5")
#else /* HAVE_PYTHON */
	   _("\
Start a Python interactive prompt.\n\
\n\
Python scripting is not supported in this copy of GDB.\n\
This command is only a placeholder.")
#endif /* HAVE_PYTHON */
	   );
  add_com_alias ("pi", python_interactive_cmd, class_obscure, 1);

  python_cmd_element = add_com ("python", class_obscure, python_command,
#ifdef HAVE_PYTHON
	   _("\
Evaluate a Python command.\n\
\n\
The command can be given as an argument, for instance:\n\
\n\
    python print (23)\n\
\n\
If no argument is given, the following lines are read and used\n\
as the Python commands.  Type a line containing \"end\" to indicate\n\
the end of the command.")
#else /* HAVE_PYTHON */
	   _("\
Evaluate a Python command.\n\
\n\
Python scripting is not supported in this copy of GDB.\n\
This command is only a placeholder.")
#endif /* HAVE_PYTHON */
	   );
  add_com_alias ("py", python_cmd_element, class_obscure, 1);

  /* Add set/show python print-stack.  */
  add_setshow_prefix_cmd ("python", no_class,
			  _("Prefix command for python preference settings."),
			  _("Prefix command for python preference settings."),
			  &user_set_python_list, &user_show_python_list,
			  &setlist, &showlist);

  add_setshow_enum_cmd ("print-stack", no_class, python_excp_enums,
			&gdbpy_should_print_stack, _("\
Set mode for Python stack dump on error."), _("\
Show the mode of Python stack printing on error."), _("\
none  == no stack or message will be printed.\n\
full == a message and a stack will be printed.\n\
message == an error message without a stack will be printed."),
			NULL, NULL,
			&user_set_python_list,
			&user_show_python_list);

  add_setshow_boolean_cmd ("ignore-environment", no_class,
			   &python_ignore_environment, _("\
Set whether the Python interpreter should ignore environment variables."), _(" \
Show whether the Python interpreter showlist ignore environment variables."), _(" \
When enabled GDB's Python interpreter will ignore any Python related\n	\
flags in the environment.  This is equivalent to passing `-E' to a\n	\
python executable."),
			   set_python_ignore_environment,
			   show_python_ignore_environment,
			   &user_set_python_list,
			   &user_show_python_list);

  add_setshow_auto_boolean_cmd ("dont-write-bytecode", no_class,
				&python_dont_write_bytecode, _("\
Set whether the Python interpreter should avoid byte-compiling python modules."), _("\
Show whether the Python interpreter should avoid byte-compiling python modules."), _("\
When enabled, GDB's embedded Python interpreter won't byte-compile python\n\
modules.  In order to take effect, this setting must be enabled in an early\n\
initialization file, i.e. those run via the --early-init-eval-command or\n\
-eix command line options.  A 'set python dont-write-bytecode on' command\n\
can also be issued directly from the GDB command line via the\n\
--early-init-eval-command or -eiex command line options.\n\
\n\
This setting defaults to 'auto'.  In this mode, provided the 'python\n\
ignore-environment' setting is 'off', the environment variable\n\
PYTHONDONTWRITEBYTECODE is examined to determine whether or not to\n\
byte-compile python modules.  PYTHONDONTWRITEBYTECODE is considered to be\n\
off/disabled either when set to the empty string or when the\n\
environment variable doesn't exist.  All other settings, including those\n\
which don't seem to make sense, indicate that it's on/enabled."),
				set_python_dont_write_bytecode,
				show_python_dont_write_bytecode,
				&user_set_python_list,
				&user_show_python_list);

#ifdef HAVE_PYTHON
#if GDB_SELF_TEST
  selftests::register_test ("python", selftests::test_python);
#endif /* GDB_SELF_TEST */
#endif /* HAVE_PYTHON */
}

#ifdef HAVE_PYTHON

/* Helper function for gdbpy_initialize.  This does the work and then
   returns false if an error has occurred and must be displayed, or true on
   success.  */

static bool
do_initialize (const struct extension_language_defn *extlang)
{
  PyObject *m;
  PyObject *sys_path;

  /* Add the initial data-directory to sys.path.  */

  std::string gdb_pythondir = (std::string (gdb_datadir) + SLASH_STRING
			       + "python");

  sys_path = PySys_GetObject ("path");

  /* PySys_SetPath was deprecated in Python 3.11.  Disable this
     deprecated code for Python 3.10 and newer.  Also note that this
     ifdef eliminates potential initialization of sys.path via
     PySys_SetPath.  My (kevinb's) understanding of PEP 587 suggests
     that it's not necessary due to module_search_paths being
     initialized to an empty list following any of the PyConfig
     initialization functions.  If it does turn out that some kind of
     initialization is still needed, it should be added to the
     PyConfig-based initialization in do_start_initialize().  */
#if PY_VERSION_HEX < 0x030a0000
  /* If sys.path is not defined yet, define it first.  */
  if (!(sys_path && PyList_Check (sys_path)))
    {
      PySys_SetPath (L"");
      sys_path = PySys_GetObject ("path");
    }
#endif
  if (sys_path && PyList_Check (sys_path))
    {
      gdbpy_ref<> pythondir (PyUnicode_FromString (gdb_pythondir.c_str ()));
      if (pythondir == NULL || PyList_Insert (sys_path, 0, pythondir.get ()))
	return false;
    }
  else
    return false;

  /* Import the gdb module to finish the initialization, and
     add it to __main__ for convenience.  */
  m = PyImport_AddModule ("__main__");
  if (m == NULL)
    return false;

  /* Keep the reference to gdb_python_module since it is in a global
     variable.  */
  gdb_python_module = PyImport_ImportModule ("gdb");
  if (gdb_python_module == NULL)
    {
      gdbpy_print_stack ();
      /* This is passed in one call to warning so that blank lines aren't
	 inserted between each line of text.  */
      warning (_("\n"
		 "Could not load the Python gdb module from `%s'.\n"
		 "Limited Python support is available from the _gdb module.\n"
		 "Suggest passing --data-directory=/path/to/gdb/data-directory."),
	       gdb_pythondir.c_str ());
      /* We return "success" here as we've already emitted the
	 warning.  */
      return true;
    }

  return gdb_pymodule_addobject (m, "gdb", gdb_python_module) >= 0;
}

/* Perform Python initialization.  This will be called after GDB has
   performed all of its own initialization.  This is the
   extension_language_ops.initialize "method".  */

static void
gdbpy_initialize (const struct extension_language_defn *extlang)
{
  if (!do_start_initialization () && PyErr_Occurred ())
    gdbpy_print_stack ();

  gdbpy_enter enter_py;

  if (!do_initialize (extlang))
    {
      gdbpy_print_stack ();
      warning (_("internal error: Unhandled Python exception"));
    }
}

/* Return non-zero if Python has successfully initialized.
   This is the extension_languages_ops.initialized "method".  */

static int
gdbpy_initialized (const struct extension_language_defn *extlang)
{
  return gdb_python_initialized;
}

PyMethodDef python_GdbMethods[] =
{
  { "history", gdbpy_history, METH_VARARGS,
    "Get a value from history" },
  { "add_history", gdbpy_add_history, METH_VARARGS,
    "Add a value to the value history list" },
  { "history_count", gdbpy_history_count, METH_NOARGS,
    "Return an integer, the number of values in GDB's value history" },
  { "execute", (PyCFunction) execute_gdb_command, METH_VARARGS | METH_KEYWORDS,
    "execute (command [, from_tty] [, to_string]) -> [String]\n\
Evaluate command, a string, as a gdb CLI command.  Optionally returns\n\
a Python String containing the output of the command if to_string is\n\
set to True." },
  { "execute_mi", (PyCFunction) gdbpy_execute_mi_command,
    METH_VARARGS | METH_KEYWORDS,
    "execute_mi (command, arg...) -> dictionary\n\
Evaluate command, a string, as a gdb MI command.\n\
Arguments (also strings) are passed to the command." },
  { "parameter", gdbpy_parameter, METH_VARARGS,
    "Return a gdb parameter's value" },

  { "breakpoints", gdbpy_breakpoints, METH_NOARGS,
    "Return a tuple of all breakpoint objects" },

  { "default_visualizer", gdbpy_default_visualizer, METH_VARARGS,
    "Find the default visualizer for a Value." },

  { "progspaces", gdbpy_progspaces, METH_NOARGS,
    "Return a sequence of all progspaces." },

  { "current_objfile", gdbpy_get_current_objfile, METH_NOARGS,
    "Return the current Objfile being loaded, or None." },

  { "newest_frame", gdbpy_newest_frame, METH_NOARGS,
    "newest_frame () -> gdb.Frame.\n\
Return the newest frame object." },
  { "selected_frame", gdbpy_selected_frame, METH_NOARGS,
    "selected_frame () -> gdb.Frame.\n\
Return the selected frame object." },
  { "frame_stop_reason_string", gdbpy_frame_stop_reason_string, METH_VARARGS,
    "stop_reason_string (Integer) -> String.\n\
Return a string explaining unwind stop reason." },

  { "start_recording", gdbpy_start_recording, METH_VARARGS,
    "start_recording ([method] [, format]) -> gdb.Record.\n\
Start recording with the given method.  If no method is given, will fall back\n\
to the system default method.  If no format is given, will fall back to the\n\
default format for the given method."},
  { "current_recording", gdbpy_current_recording, METH_NOARGS,
    "current_recording () -> gdb.Record.\n\
Return current recording object." },
  { "stop_recording", gdbpy_stop_recording, METH_NOARGS,
    "stop_recording () -> None.\n\
Stop current recording." },

  { "lookup_type", (PyCFunction) gdbpy_lookup_type,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_type (name [, block]) -> type\n\
Return a Type corresponding to the given name." },
  { "lookup_symbol", (PyCFunction) gdbpy_lookup_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_symbol (name [, block] [, domain]) -> (symbol, is_field_of_this)\n\
Return a tuple with the symbol corresponding to the given name (or None) and\n\
a boolean indicating if name is a field of the current implied argument\n\
`this' (when the current language is object-oriented)." },
  { "lookup_global_symbol", (PyCFunction) gdbpy_lookup_global_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_global_symbol (name [, domain]) -> symbol\n\
Return the symbol corresponding to the given name (or None)." },
  { "lookup_static_symbol", (PyCFunction) gdbpy_lookup_static_symbol,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_static_symbol (name [, domain]) -> symbol\n\
Return the static-linkage symbol corresponding to the given name (or None)." },
  { "lookup_static_symbols", (PyCFunction) gdbpy_lookup_static_symbols,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_static_symbols (name [, domain]) -> symbol\n\
Return a list of all static-linkage symbols corresponding to the given name." },

  { "lookup_objfile", (PyCFunction) gdbpy_lookup_objfile,
    METH_VARARGS | METH_KEYWORDS,
    "lookup_objfile (name, [by_build_id]) -> objfile\n\
Look up the specified objfile.\n\
If by_build_id is True, the objfile is looked up by using name\n\
as its build id." },

  { "decode_line", gdbpy_decode_line, METH_VARARGS,
    "decode_line (String) -> Tuple.  Decode a string argument the way\n\
that 'break' or 'edit' does.  Return a tuple containing two elements.\n\
The first element contains any unparsed portion of the String parameter\n\
(or None if the string was fully parsed).  The second element contains\n\
a tuple that contains all the locations that match, represented as\n\
gdb.Symtab_and_line objects (or None)."},
  { "parse_and_eval", (PyCFunction) gdbpy_parse_and_eval,
    METH_VARARGS | METH_KEYWORDS,
    "parse_and_eval (String, [Boolean]) -> Value.\n\
Parse String as an expression, evaluate it, and return the result as a Value."
  },

  { "post_event", gdbpy_post_event, METH_VARARGS,
    "Post an event into gdb's event loop." },
  { "interrupt", gdbpy_interrupt, METH_NOARGS,
    "Interrupt gdb's current operation." },

  { "target_charset", gdbpy_target_charset, METH_NOARGS,
    "target_charset () -> string.\n\
Return the name of the current target charset." },
  { "target_wide_charset", gdbpy_target_wide_charset, METH_NOARGS,
    "target_wide_charset () -> string.\n\
Return the name of the current target wide charset." },
  { "host_charset", gdbpy_host_charset, METH_NOARGS,
    "host_charset () -> string.\n\
Return the name of the current host charset." },
  { "rbreak", (PyCFunction) gdbpy_rbreak, METH_VARARGS | METH_KEYWORDS,
    "rbreak (Regex) -> List.\n\
Return a Tuple containing gdb.Breakpoint objects that match the given Regex." },
  { "string_to_argv", gdbpy_string_to_argv, METH_VARARGS,
    "string_to_argv (String) -> Array.\n\
Parse String and return an argv-like array.\n\
Arguments are separate by spaces and may be quoted."
  },
  { "write", (PyCFunction)gdbpy_write, METH_VARARGS | METH_KEYWORDS,
    "Write a string using gdb's filtered stream." },
  { "flush", (PyCFunction)gdbpy_flush, METH_VARARGS | METH_KEYWORDS,
    "Flush gdb's filtered stdout stream." },
  { "selected_thread", gdbpy_selected_thread, METH_NOARGS,
    "selected_thread () -> gdb.InferiorThread.\n\
Return the selected thread object." },
  { "selected_inferior", gdbpy_selected_inferior, METH_NOARGS,
    "selected_inferior () -> gdb.Inferior.\n\
Return the selected inferior object." },
  { "inferiors", gdbpy_inferiors, METH_NOARGS,
    "inferiors () -> (gdb.Inferior, ...).\n\
Return a tuple containing all inferiors." },

  { "invalidate_cached_frames", gdbpy_invalidate_cached_frames, METH_NOARGS,
    "invalidate_cached_frames () -> None.\n\
Invalidate any cached frame objects in gdb.\n\
Intended for internal use only." },

  { "convenience_variable", gdbpy_convenience_variable, METH_VARARGS,
    "convenience_variable (NAME) -> value.\n\
Return the value of the convenience variable $NAME,\n\
or None if not set." },
  { "set_convenience_variable", gdbpy_set_convenience_variable, METH_VARARGS,
    "convenience_variable (NAME, VALUE) -> None.\n\
Set the value of the convenience variable $NAME." },

#ifdef TUI
  { "register_window_type", (PyCFunction) gdbpy_register_tui_window,
    METH_VARARGS | METH_KEYWORDS,
    "register_window_type (NAME, CONSTRUCTOR) -> None\n\
Register a TUI window constructor." },
#endif	/* TUI */

  { "architecture_names", gdbpy_all_architecture_names, METH_NOARGS,
    "architecture_names () -> List.\n\
Return a list of all the architecture names GDB understands." },

  { "connections", gdbpy_connections, METH_NOARGS,
    "connections () -> List.\n\
Return a list of gdb.TargetConnection objects." },

  { "format_address", (PyCFunction) gdbpy_format_address,
    METH_VARARGS | METH_KEYWORDS,
    "format_address (ADDRESS, PROG_SPACE, ARCH) -> String.\n\
Format ADDRESS, an address within PROG_SPACE, a gdb.Progspace, using\n\
ARCH, a gdb.Architecture to determine the address size.  The format of\n\
the returned string is 'ADDRESS <SYMBOL+OFFSET>' without the quotes." },

  { "current_language", gdbpy_current_language, METH_NOARGS,
    "current_language () -> string\n\
Return the name of the currently selected language." },

  { "print_options", gdbpy_print_options, METH_NOARGS,
    "print_options () -> dict\n\
Return the current print options." },

  { "notify_mi", (PyCFunction) gdbpy_notify_mi,
    METH_VARARGS | METH_KEYWORDS,
    "notify_mi (name, data) -> None\n\
Output async record to MI channels if any." },
  {NULL, NULL, 0, NULL}
};

/* Define all the event objects.  */
#define GDB_PY_DEFINE_EVENT_TYPE(name, py_name, doc, base) \
  PyTypeObject name##_event_object_type		    \
	CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object") \
    = { \
      PyVarObject_HEAD_INIT (NULL, 0)				\
      "gdb." py_name,                             /* tp_name */ \
      sizeof (event_object),                      /* tp_basicsize */ \
      0,                                          /* tp_itemsize */ \
      evpy_dealloc,                               /* tp_dealloc */ \
      0,                                          /* tp_print */ \
      0,                                          /* tp_getattr */ \
      0,                                          /* tp_setattr */ \
      0,                                          /* tp_compare */ \
      0,                                          /* tp_repr */ \
      0,                                          /* tp_as_number */ \
      0,                                          /* tp_as_sequence */ \
      0,                                          /* tp_as_mapping */ \
      0,                                          /* tp_hash  */ \
      0,                                          /* tp_call */ \
      0,                                          /* tp_str */ \
      0,                                          /* tp_getattro */ \
      0,                                          /* tp_setattro */ \
      0,                                          /* tp_as_buffer */ \
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */ \
      doc,                                        /* tp_doc */ \
      0,                                          /* tp_traverse */ \
      0,                                          /* tp_clear */ \
      0,                                          /* tp_richcompare */ \
      0,                                          /* tp_weaklistoffset */ \
      0,                                          /* tp_iter */ \
      0,                                          /* tp_iternext */ \
      0,                                          /* tp_methods */ \
      0,                                          /* tp_members */ \
      0,                                          /* tp_getset */ \
      &base,                                      /* tp_base */ \
      0,                                          /* tp_dict */ \
      0,                                          /* tp_descr_get */ \
      0,                                          /* tp_descr_set */ \
      0,                                          /* tp_dictoffset */ \
      0,                                          /* tp_init */ \
      0                                           /* tp_alloc */ \
    };
#include "py-event-types.def"
#undef GDB_PY_DEFINE_EVENT_TYPE

#endif /* HAVE_PYTHON */

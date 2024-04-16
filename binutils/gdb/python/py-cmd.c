/* gdb commands implemented in Python

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
#include "value.h"
#include "python-internal.h"
#include "charset.h"
#include "gdbcmd.h"
#include "cli/cli-decode.h"
#include "completer.h"
#include "language.h"

/* Struct representing built-in completion types.  */
struct cmdpy_completer
{
  /* Python symbol name.  */
  const char *name;
  /* Completion function.  */
  completer_ftype *completer;
};

static const struct cmdpy_completer completers[] =
{
  { "COMPLETE_NONE", noop_completer },
  { "COMPLETE_FILENAME", filename_completer },
  { "COMPLETE_LOCATION", location_completer },
  { "COMPLETE_COMMAND", command_completer },
  { "COMPLETE_SYMBOL", symbol_completer },
  { "COMPLETE_EXPRESSION", expression_completer },
};

#define N_COMPLETERS (sizeof (completers) / sizeof (completers[0]))

/* A gdb command.  For the time being only ordinary commands (not
   set/show commands) are allowed.  */
struct cmdpy_object
{
  PyObject_HEAD

  /* The corresponding gdb command object, or NULL if the command is
     no longer installed.  */
  struct cmd_list_element *command;

  /* A prefix command requires storage for a list of its sub-commands.
     A pointer to this is passed to add_prefix_command, and to add_cmd
     for sub-commands of that prefix.  If this Command is not a prefix
     command, then this field is unused.  */
  struct cmd_list_element *sub_list;
};

extern PyTypeObject cmdpy_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("cmdpy_object");

/* Constants used by this module.  */
static PyObject *invoke_cst;
static PyObject *complete_cst;



/* Python function which wraps dont_repeat.  */
static PyObject *
cmdpy_dont_repeat (PyObject *self, PyObject *args)
{
  dont_repeat ();
  Py_RETURN_NONE;
}



/* Called if the gdb cmd_list_element is destroyed.  */

static void
cmdpy_destroyer (struct cmd_list_element *self, void *context)
{
  gdbpy_enter enter_py;

  /* Release our hold on the command object.  */
  gdbpy_ref<cmdpy_object> cmd ((cmdpy_object *) context);
  cmd->command = NULL;
}

/* Called by gdb to invoke the command.  */

static void
cmdpy_function (const char *args, int from_tty, cmd_list_element *command)
{
  cmdpy_object *obj = (cmdpy_object *) command->context ();

  gdbpy_enter enter_py;

  if (! obj)
    error (_("Invalid invocation of Python command object."));
  if (! PyObject_HasAttr ((PyObject *) obj, invoke_cst))
    {
      if (obj->command->is_prefix ())
	{
	  /* A prefix command does not need an invoke method.  */
	  return;
	}
      error (_("Python command object missing 'invoke' method."));
    }

  if (! args)
    args = "";
  gdbpy_ref<> argobj (PyUnicode_Decode (args, strlen (args), host_charset (),
					NULL));
  if (argobj == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not convert arguments to Python string."));
    }

  gdbpy_ref<> ttyobj (PyBool_FromLong (from_tty));
  gdbpy_ref<> result (PyObject_CallMethodObjArgs ((PyObject *) obj, invoke_cst,
						  argobj.get (), ttyobj.get (),
						  NULL));

  if (result == NULL)
    gdbpy_handle_exception ();
}

/* Helper function for the Python command completers (both "pure"
   completer and brkchar handler).  This function takes COMMAND, TEXT
   and WORD and tries to call the Python method for completion with
   these arguments.

   This function is usually called twice: once when we are figuring out
   the break characters to be used, and another to perform the real
   completion itself.  The reason for this two step dance is that we
   need to know the set of "brkchars" to use early on, before we
   actually try to perform the completion.  But if a Python command
   supplies a "complete" method then we have to call that method
   first: it may return as its result the kind of completion to
   perform and that will in turn specify which brkchars to use.  IOW,
   we need the result of the "complete" method before we actually
   perform the completion.  The only situation when this function is
   not called twice is when the user uses the "complete" command: in
   this scenario, there is no call to determine the "brkchars".

   Ideally, it would be nice to cache the result of the first call (to
   determine the "brkchars") and return this value directly in the
   second call (to perform the actual completion).  However, due to
   the peculiarity of the "complete" command mentioned above, it is
   possible to put GDB in a bad state if you perform a TAB-completion
   and then a "complete"-completion sequentially.  Therefore, we just
   recalculate everything twice for TAB-completions.

   This function returns a reference to the PyObject representing the
   Python method call.  */

static gdbpy_ref<>
cmdpy_completer_helper (struct cmd_list_element *command,
			const char *text, const char *word)
{
  cmdpy_object *obj = (cmdpy_object *) command->context ();

  if (obj == NULL)
    error (_("Invalid invocation of Python command object."));
  if (!PyObject_HasAttr ((PyObject *) obj, complete_cst))
    {
      /* If there is no complete method, don't error.  */
      return NULL;
    }

  gdbpy_ref<> textobj (PyUnicode_Decode (text, strlen (text), host_charset (),
					 NULL));
  if (textobj == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not convert argument to Python string."));
    }

  gdbpy_ref<> wordobj;
  if (word == NULL)
    {
      /* "brkchars" phase.  */
      wordobj = gdbpy_ref<>::new_reference (Py_None);
    }
  else
    {
      wordobj.reset (PyUnicode_Decode (word, strlen (word), host_charset (),
				       NULL));
      if (wordobj == NULL)
	{
	  gdbpy_print_stack ();
	  error (_("Could not convert argument to Python string."));
	}
    }

  gdbpy_ref<> resultobj (PyObject_CallMethodObjArgs ((PyObject *) obj,
						     complete_cst,
						     textobj.get (),
						     wordobj.get (), NULL));

  /* Check if an exception was raised by the Command.complete method.  */
  if (resultobj == nullptr)
    {
      gdbpy_print_stack_or_quit ();
      error (_("exception raised during Command.complete method"));
    }

  return resultobj;
}

/* Python function called to determine the break characters of a
   certain completer.  We are only interested in knowing if the
   completer registered by the user will return one of the integer
   codes (see COMPLETER_* symbols).  */

static void
cmdpy_completer_handle_brkchars (struct cmd_list_element *command,
				 completion_tracker &tracker,
				 const char *text, const char *word)
{
  gdbpy_enter enter_py;

  /* Calling our helper to obtain a reference to the PyObject of the Python
     function.  */
  gdbpy_ref<> resultobj = cmdpy_completer_helper (command, text, word);

  /* Check if there was an error.  */
  if (resultobj == NULL)
    return;

  if (PyLong_Check (resultobj.get ()))
    {
      /* User code may also return one of the completion constants,
	 thus requesting that sort of completion.  We are only
	 interested in this kind of return.  */
      long value;

      if (!gdb_py_int_as_long (resultobj.get (), &value))
	gdbpy_print_stack ();
      else if (value >= 0 && value < (long) N_COMPLETERS)
	{
	  completer_handle_brkchars_ftype *brkchars_fn;

	  /* This is the core of this function.  Depending on which
	     completer type the Python function returns, we have to
	     adjust the break characters accordingly.  */
	  brkchars_fn = (completer_handle_brkchars_func_for_completer
			 (completers[value].completer));
	  brkchars_fn (command, tracker, text, word);
	}
    }
}

/* Called by gdb for command completion.  */

static void
cmdpy_completer (struct cmd_list_element *command,
		 completion_tracker &tracker,
		 const char *text, const char *word)
{
  gdbpy_enter enter_py;

  /* Calling our helper to obtain a reference to the PyObject of the Python
     function.  */
  gdbpy_ref<> resultobj = cmdpy_completer_helper (command, text, word);

  /* If the result object of calling the Python function is NULL, it
     means that there was an error.  In this case, just give up.  */
  if (resultobj == NULL)
    return;

  if (PyLong_Check (resultobj.get ()))
    {
      /* User code may also return one of the completion constants,
	 thus requesting that sort of completion.  */
      long value;

      if (! gdb_py_int_as_long (resultobj.get (), &value))
	gdbpy_print_stack ();
      else if (value >= 0 && value < (long) N_COMPLETERS)
	completers[value].completer (command, tracker, text, word);
    }
  else if (PySequence_Check (resultobj.get ()))
    {
      gdbpy_ref<> iter (PyObject_GetIter (resultobj.get ()));

      if (iter == NULL)
	{
	  gdbpy_print_stack ();
	  return;
	}

      while (true)
	{
	  gdbpy_ref<> elt (PyIter_Next (iter.get ()));
	  if (elt == NULL)
	    {
	      if (PyErr_Occurred() != nullptr)
		gdbpy_print_stack ();
	      break;
	    }

	  if (! gdbpy_is_string (elt.get ()))
	    {
	      /* Skip problem elements.  */
	      continue;
	    }

	  gdb::unique_xmalloc_ptr<char>
	    item (python_string_to_host_string (elt.get ()));
	  if (item == NULL)
	    {
	      gdbpy_print_stack ();
	      continue;
	    }
	  tracker.add_completion (std::move (item));
	}
    }
}

/* Helper for cmdpy_init which locates the command list to use and
   pulls out the command name.

   NAME is the command name list.  The final word in the list is the
   name of the new command.  All earlier words must be existing prefix
   commands.

   *BASE_LIST is set to the final prefix command's list of
   *sub-commands.

   START_LIST is the list in which the search starts.

   This function returns the name of the new command.  On error sets the Python
   error and returns NULL.  */

gdb::unique_xmalloc_ptr<char>
gdbpy_parse_command_name (const char *name,
			  struct cmd_list_element ***base_list,
			  struct cmd_list_element **start_list)
{
  struct cmd_list_element *elt;
  int len = strlen (name);
  int i, lastchar;
  const char *prefix_text2;

  /* Skip trailing whitespace.  */
  for (i = len - 1; i >= 0 && (name[i] == ' ' || name[i] == '\t'); --i)
    ;
  if (i < 0)
    {
      PyErr_SetString (PyExc_RuntimeError, _("No command name found."));
      return NULL;
    }
  lastchar = i;

  /* Find first character of the final word.  */
  for (; i > 0 && valid_cmd_char_p (name[i - 1]); --i)
    ;

  gdb::unique_xmalloc_ptr<char> result ((char *) xmalloc (lastchar - i + 2));
  memcpy (result.get (), &name[i], lastchar - i + 1);
  result.get ()[lastchar - i + 1] = '\0';

  /* Skip whitespace again.  */
  for (--i; i >= 0 && (name[i] == ' ' || name[i] == '\t'); --i)
    ;
  if (i < 0)
    {
      *base_list = start_list;
      return result;
    }

  std::string prefix_text (name, i + 1);

  prefix_text2 = prefix_text.c_str ();
  elt = lookup_cmd_1 (&prefix_text2, *start_list, NULL, NULL, 1);
  if (elt == NULL || elt == CMD_LIST_AMBIGUOUS)
    {
      PyErr_Format (PyExc_RuntimeError, _("Could not find command prefix %s."),
		    prefix_text.c_str ());
      return NULL;
    }

  if (elt->is_prefix ())
    {
      *base_list = elt->subcommands;
      return result;
    }

  PyErr_Format (PyExc_RuntimeError, _("'%s' is not a prefix command."),
		prefix_text.c_str ());
  return NULL;
}

/* Object initializer; sets up gdb-side structures for command.

   Use: __init__(NAME, COMMAND_CLASS [, COMPLETER_CLASS][, PREFIX]]).

   NAME is the name of the command.  It may consist of multiple words,
   in which case the final word is the name of the new command, and
   earlier words must be prefix commands.

   COMMAND_CLASS is the kind of command.  It should be one of the COMMAND_*
   constants defined in the gdb module.

   COMPLETER_CLASS is the kind of completer.  If not given, the
   "complete" method will be used.  Otherwise, it should be one of the
   COMPLETE_* constants defined in the gdb module.

   If PREFIX is True, then this command is a prefix command.

   The documentation for the command is taken from the doc string for
   the python class.  */

static int
cmdpy_init (PyObject *self, PyObject *args, PyObject *kw)
{
  cmdpy_object *obj = (cmdpy_object *) self;
  const char *name;
  int cmdtype;
  int completetype = -1;
  struct cmd_list_element **cmd_list;
  static const char *keywords[] = { "name", "command_class", "completer_class",
				    "prefix", NULL };
  PyObject *is_prefix_obj = NULL;
  bool is_prefix = false;

  if (obj->command)
    {
      /* Note: this is apparently not documented in Python.  We return
	 0 for success, -1 for failure.  */
      PyErr_Format (PyExc_RuntimeError,
		    _("Command object already initialized."));
      return -1;
    }

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "si|iO",
					keywords, &name, &cmdtype,
					&completetype, &is_prefix_obj))
    return -1;

  if (cmdtype != no_class && cmdtype != class_run
      && cmdtype != class_vars && cmdtype != class_stack
      && cmdtype != class_files && cmdtype != class_support
      && cmdtype != class_info && cmdtype != class_breakpoint
      && cmdtype != class_trace && cmdtype != class_obscure
      && cmdtype != class_maintenance && cmdtype != class_user
      && cmdtype != class_tui)
    {
      PyErr_Format (PyExc_RuntimeError, _("Invalid command class argument."));
      return -1;
    }

  if (completetype < -1 || completetype >= (int) N_COMPLETERS)
    {
      PyErr_Format (PyExc_RuntimeError,
		    _("Invalid completion type argument."));
      return -1;
    }

  gdb::unique_xmalloc_ptr<char> cmd_name
    = gdbpy_parse_command_name (name, &cmd_list, &cmdlist);
  if (cmd_name == nullptr)
    return -1;

  if (is_prefix_obj != NULL)
    {
      int cmp = PyObject_IsTrue (is_prefix_obj);
      if (cmp < 0)
	return -1;

      is_prefix = cmp > 0;
    }

  gdb::unique_xmalloc_ptr<char> docstring = nullptr;
  if (PyObject_HasAttr (self, gdbpy_doc_cst))
    {
      gdbpy_ref<> ds_obj (PyObject_GetAttr (self, gdbpy_doc_cst));

      if (ds_obj != NULL && gdbpy_is_string (ds_obj.get ()))
	{
	  docstring = python_string_to_host_string (ds_obj.get ());
	  if (docstring == nullptr)
	    return -1;
	  docstring = gdbpy_fix_doc_string_indentation (std::move (docstring));
	}
    }
  if (docstring == nullptr)
    docstring = make_unique_xstrdup (_("This command is not documented."));

  gdbpy_ref<> self_ref = gdbpy_ref<>::new_reference (self);

  try
    {
      struct cmd_list_element *cmd;

      if (is_prefix)
	{
	  int allow_unknown;

	  /* If we have our own "invoke" method, then allow unknown
	     sub-commands.  */
	  allow_unknown = PyObject_HasAttr (self, invoke_cst);
	  cmd = add_prefix_cmd (cmd_name.get (),
				(enum command_class) cmdtype,
				NULL, docstring.release (), &obj->sub_list,
				allow_unknown, cmd_list);
	}
      else
	cmd = add_cmd (cmd_name.get (), (enum command_class) cmdtype,
		       docstring.release (), cmd_list);

      /* If successful, the above takes ownership of the name, since we set
	 name_allocated, so release it.  */
      cmd_name.release ();

      /* There appears to be no API to set this.  */
      cmd->func = cmdpy_function;
      cmd->destroyer = cmdpy_destroyer;
      cmd->doc_allocated = 1;
      cmd->name_allocated = 1;

      obj->command = cmd;
      cmd->set_context (self_ref.release ());
      set_cmd_completer (cmd, ((completetype == -1) ? cmdpy_completer
			       : completers[completetype].completer));
      if (completetype == -1)
	set_cmd_completer_handle_brkchars (cmd,
					   cmdpy_completer_handle_brkchars);
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return -1;
    }

  return 0;
}



/* Initialize the 'commands' code.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_commands (void)
{
  int i;

  cmdpy_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&cmdpy_object_type) < 0)
    return -1;

  /* Note: alias and user are special.  */
  if (PyModule_AddIntConstant (gdb_module, "COMMAND_NONE", no_class) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_RUNNING", class_run) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_DATA", class_vars) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_STACK", class_stack) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_FILES", class_files) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_SUPPORT",
				  class_support) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_STATUS", class_info) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_BREAKPOINTS",
				  class_breakpoint) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_TRACEPOINTS",
				  class_trace) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_OBSCURE",
				  class_obscure) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_MAINTENANCE",
				  class_maintenance) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_USER", class_user) < 0
      || PyModule_AddIntConstant (gdb_module, "COMMAND_TUI", class_tui) < 0)
    return -1;

  for (i = 0; i < N_COMPLETERS; ++i)
    {
      if (PyModule_AddIntConstant (gdb_module, completers[i].name, i) < 0)
	return -1;
    }

  if (gdb_pymodule_addobject (gdb_module, "Command",
			      (PyObject *) &cmdpy_object_type) < 0)
    return -1;

  invoke_cst = PyUnicode_FromString ("invoke");
  if (invoke_cst == NULL)
    return -1;
  complete_cst = PyUnicode_FromString ("complete");
  if (complete_cst == NULL)
    return -1;

  return 0;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_commands);



static PyMethodDef cmdpy_object_methods[] =
{
  { "dont_repeat", cmdpy_dont_repeat, METH_NOARGS,
    "Prevent command repetition when user enters empty line." },

  { 0 }
};

PyTypeObject cmdpy_object_type =
{
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Command",		  /*tp_name*/
  sizeof (cmdpy_object),	  /*tp_basicsize*/
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
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "GDB command object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  cmdpy_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  0,				  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  cmdpy_init,			  /* tp_init */
  0,				  /* tp_alloc */
};



/* Utility to build a buildargv-like result from ARGS.
   This intentionally parses arguments the way libiberty/argv.c:buildargv
   does.  It splits up arguments in a reasonable way, and we want a standard
   way of parsing arguments.  Several gdb commands use buildargv to parse their
   arguments.  Plus we want to be able to write compatible python
   implementations of gdb commands.  */

PyObject *
gdbpy_string_to_argv (PyObject *self, PyObject *args)
{
  const char *input;

  if (!PyArg_ParseTuple (args, "s", &input))
    return NULL;

  gdbpy_ref<> py_argv (PyList_New (0));
  if (py_argv == NULL)
    return NULL;

  /* buildargv uses NULL to represent an empty argument list, but we can't use
     that in Python.  Instead, if ARGS is "" then return an empty list.
     This undoes the NULL -> "" conversion that cmdpy_function does.  */

  if (*input != '\0')
    {
      gdb_argv c_argv (input);

      for (char *arg : c_argv)
	{
	  gdbpy_ref<> argp (PyUnicode_FromString (arg));

	  if (argp == NULL
	      || PyList_Append (py_argv.get (), argp.get ()) < 0)
	    return NULL;
	}
    }

  return py_argv.release ();
}

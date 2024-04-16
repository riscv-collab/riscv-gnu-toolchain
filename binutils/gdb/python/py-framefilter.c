/* Python frame filters

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
#include "objfiles.h"
#include "symtab.h"
#include "language.h"
#include "arch-utils.h"
#include "python.h"
#include "ui-out.h"
#include "valprint.h"
#include "stack.h"
#include "source.h"
#include "annotate.h"
#include "hashtab.h"
#include "demangle.h"
#include "mi/mi-cmds.h"
#include "python-internal.h"
#include <optional>
#include "cli/cli-style.h"

enum mi_print_types
{
  MI_PRINT_ARGS,
  MI_PRINT_LOCALS
};

/* Helper  function  to  extract  a  symbol, a  name  and  a  language
   definition from a Python object that conforms to the "Symbol Value"
   interface.  OBJ  is the Python  object to extract the  values from.
   NAME is a  pass-through argument where the name of  the symbol will
   be written.  NAME is allocated in  this function, but the caller is
   responsible for clean up.  SYM is a pass-through argument where the
   symbol will be written and  SYM_BLOCK is a pass-through argument to
   write  the block where the symbol lies in.  In the case of the  API
   returning a  string,  this will be set to NULL.  LANGUAGE is also a
   pass-through  argument  denoting  the  language  attributed  to the
   Symbol.  In the case of SYM being  NULL, this  will be  set to  the
   current  language.  Returns  EXT_LANG_BT_ERROR  on  error  with the
   appropriate Python exception set, and EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
extract_sym (PyObject *obj, gdb::unique_xmalloc_ptr<char> *name,
	     struct symbol **sym, const struct block **sym_block,
	     const struct language_defn **language)
{
  gdbpy_ref<> result (PyObject_CallMethod (obj, "symbol", NULL));

  if (result == NULL)
    return EXT_LANG_BT_ERROR;

  /* For 'symbol' callback, the function can return a symbol or a
     string.  */
  if (gdbpy_is_string (result.get ()))
    {
      *name = python_string_to_host_string (result.get ());

      if (*name == NULL)
	return EXT_LANG_BT_ERROR;
      /* If the API returns a string (and not a symbol), then there is
	 no symbol derived language available and the frame filter has
	 either overridden the symbol with a string, or supplied a
	 entirely synthetic symbol/value pairing.  In that case, use
	 the current language.  */
      *language = current_language;
      *sym = NULL;
      *sym_block = NULL;
    }
  else
    {
      /* This type checks 'result' during the conversion so we
	 just call it unconditionally and check the return.  */
      *sym = symbol_object_to_symbol (result.get ());
      /* TODO: currently, we have no way to recover the block in which SYMBOL
	 was found, so we have no block to return.  Trying to evaluate SYMBOL
	 will yield an incorrect value when it's located in a FRAME and
	 evaluated from another frame (as permitted in nested functions).  */
      *sym_block = NULL;

      if (*sym == NULL)
	{
	  PyErr_SetString (PyExc_RuntimeError,
			   _("Unexpected value.  Expecting a "
			     "gdb.Symbol or a Python string."));
	  return EXT_LANG_BT_ERROR;
	}

      /* Duplicate the symbol name, so the caller has consistency
	 in garbage collection.  */
      name->reset (xstrdup ((*sym)->print_name ()));

      /* If a symbol is specified attempt to determine the language
	 from the symbol.  If mode is not "auto", then the language
	 has been explicitly set, use that.  */
      if (language_mode == language_mode_auto)
	*language = language_def ((*sym)->language ());
      else
	*language = current_language;
    }

  return EXT_LANG_BT_OK;
}

/* Helper function to extract a value from an object that conforms to
   the "Symbol Value" interface.  OBJ is the Python object to extract
   the value from.  VALUE is a pass-through argument where the value
   will be written.  If the object does not have the value attribute,
   or provides the Python None for a value, VALUE will be set to NULL
   and this function will return as successful.  Returns EXT_LANG_BT_ERROR
   on error with the appropriate Python exception set, and EXT_LANG_BT_OK on
   success.  */

static enum ext_lang_bt_status
extract_value (PyObject *obj, struct value **value)
{
  if (PyObject_HasAttrString (obj, "value"))
    {
      gdbpy_ref<> vresult (PyObject_CallMethod (obj, "value", NULL));

      if (vresult == NULL)
	return EXT_LANG_BT_ERROR;

      /* The Python code has returned 'None' for a value, so we set
	 value to NULL.  This flags that GDB should read the
	 value.  */
      if (vresult == Py_None)
	{
	  *value = NULL;
	  return EXT_LANG_BT_OK;
	}
      else
	{
	  *value = convert_value_from_python (vresult.get ());

	  if (*value == NULL)
	    return EXT_LANG_BT_ERROR;

	  return EXT_LANG_BT_OK;
	}
    }
  else
    *value = NULL;

  return EXT_LANG_BT_OK;
}

/* MI prints only certain values according to the type of symbol and
   also what the user has specified.  SYM is the symbol to check, and
   MI_PRINT_TYPES is an enum specifying what the user wants emitted
   for the MI command in question.  */
static int
mi_should_print (struct symbol *sym, enum mi_print_types type)
{
  int print_me = 0;

  switch (sym->aclass ())
    {
    default:
    case LOC_UNDEF:	/* catches errors        */
    case LOC_CONST:	/* constant              */
    case LOC_TYPEDEF:	/* local typedef         */
    case LOC_LABEL:	/* local label           */
    case LOC_BLOCK:	/* local function        */
    case LOC_CONST_BYTES:	/* loc. byte seq.        */
    case LOC_UNRESOLVED:	/* unresolved static     */
    case LOC_OPTIMIZED_OUT:	/* optimized out         */
      print_me = 0;
      break;

    case LOC_ARG:	/* argument              */
    case LOC_REF_ARG:	/* reference arg         */
    case LOC_REGPARM_ADDR:	/* indirect register arg */
    case LOC_LOCAL:	/* stack local           */
    case LOC_STATIC:	/* static                */
    case LOC_REGISTER:	/* register              */
    case LOC_COMPUTED:	/* computed location     */
      if (type == MI_PRINT_LOCALS)
	print_me = ! sym->is_argument ();
      else
	print_me = sym->is_argument ();
    }
  return print_me;
}

/* Helper function which outputs a type name extracted from VAL to a
   "type" field in the output stream OUT.  OUT is the ui-out structure
   the type name will be output too, and VAL is the value that the
   type will be extracted from.  */

static void
py_print_type (struct ui_out *out, struct value *val)
{
  check_typedef (val->type ());

  string_file stb;
  type_print (val->type (), "", &stb, -1);
  out->field_stream ("type", stb);
}

/* Helper function which outputs a value to an output field in a
   stream.  OUT is the ui-out structure the value will be output to,
   VAL is the value that will be printed, OPTS contains the value
   printing options, ARGS_TYPE is an enumerator describing the
   argument format, and LANGUAGE is the language_defn that the value
   will be printed with.  */

static void
py_print_value (struct ui_out *out, struct value *val,
		const struct value_print_options *opts,
		int indent,
		enum ext_lang_frame_args args_type,
		const struct language_defn *language)
{
  int should_print = 0;

  /* MI does not print certain values, differentiated by type,
     depending on what ARGS_TYPE indicates.  Test type against option.
     For CLI print all values.  */
  if (args_type == MI_PRINT_SIMPLE_VALUES
      || args_type == MI_PRINT_ALL_VALUES)
    {
      if (args_type == MI_PRINT_ALL_VALUES)
	should_print = 1;
      else if (args_type == MI_PRINT_SIMPLE_VALUES
	       && mi_simple_type_p (val->type ()))
	should_print = 1;
    }
  else if (args_type != NO_VALUES)
    should_print = 1;

  if (should_print)
    {
      string_file stb;

      common_val_print (val, &stb, indent, opts, language);
      out->field_stream ("value", stb);
    }
}

/* Helper function to call a Python method and extract an iterator
   from the result.  If the function returns anything but an iterator
   the exception is preserved and NULL is returned.  FILTER is the
   Python object to call, and FUNC is the name of the method.  Returns
   a PyObject, or NULL on error with the appropriate exception set.
   This function can return an iterator, or NULL.  */

static PyObject *
get_py_iter_from_func (PyObject *filter, const char *func)
{
  if (PyObject_HasAttrString (filter, func))
    {
      gdbpy_ref<> result (PyObject_CallMethod (filter, func, NULL));

      if (result != NULL)
	{
	  if (result == Py_None)
	    {
	      return result.release ();
	    }
	  else
	    {
	      return PyObject_GetIter (result.get ());
	    }
	}
    }
  else
    Py_RETURN_NONE;

  return NULL;
}

/*  Helper function to output a single frame argument and value to an
    output stream.  This function will account for entry values if the
    FV parameter is populated, the frame argument has entry values
    associated with them, and the appropriate "set entry-value"
    options are set.  Will output in CLI or MI like format depending
    on the type of output stream detected.  OUT is the output stream,
    SYM_NAME is the name of the symbol.  If SYM_NAME is populated then
    it must have an accompanying value in the parameter FV.  FA is a
    frame argument structure.  If FA is populated, both SYM_NAME and
    FV are ignored.  OPTS contains the value printing options,
    ARGS_TYPE is an enumerator describing the argument format,
    PRINT_ARGS_FIELD is a flag which indicates if we output "ARGS=1"
    in MI output in commands where both arguments and locals are
    printed.  */

static void
py_print_single_arg (struct ui_out *out,
		     const char *sym_name,
		     struct frame_arg *fa,
		     struct value *fv,
		     const struct value_print_options *opts,
		     enum ext_lang_frame_args args_type,
		     int print_args_field,
		     const struct language_defn *language)
{
  struct value *val;

  if (fa != NULL)
    {
      if (fa->val == NULL && fa->error == NULL)
	return;
      language = language_def (fa->sym->language ());
      val = fa->val;
    }
  else
    val = fv;

  std::optional<ui_out_emit_tuple> maybe_tuple;

  /*  MI has varying rules for tuples, but generally if there is only
      one element in each item in the list, do not start a tuple.  The
      exception is -stack-list-variables which emits an ARGS="1" field
      if the value is a frame argument.  This is denoted in this
      function with PRINT_ARGS_FIELD which is flag from the caller to
      emit the ARGS field.  */
  if (out->is_mi_like_p ())
    {
      if (print_args_field || args_type != NO_VALUES)
	maybe_tuple.emplace (out, nullptr);
    }

  annotate_arg_begin ();

  /* If frame argument is populated, check for entry-values and the
     entry value options.  */
  if (fa != NULL)
    {
      string_file stb;

      gdb_puts (fa->sym->print_name (), &stb);
      if (fa->entry_kind == print_entry_values_compact)
	{
	  stb.puts ("=");

	  gdb_puts (fa->sym->print_name (), &stb);
	}
      if (fa->entry_kind == print_entry_values_only
	  || fa->entry_kind == print_entry_values_compact)
	stb.puts ("@entry");
      out->field_stream ("name", stb);
    }
  else
    /* Otherwise, just output the name.  */
    out->field_string ("name", sym_name);

  annotate_arg_name_end ();

  out->text ("=");

  if (print_args_field)
    out->field_signed ("arg", 1);

  /* For MI print the type, but only for simple values.  This seems
     weird, but this is how MI choose to format the various output
     types.  */
  if (args_type == MI_PRINT_SIMPLE_VALUES && val != NULL)
    py_print_type (out, val);

  if (val != NULL)
    annotate_arg_value (val->type ());

  /* If the output is to the CLI, and the user option "set print
     frame-arguments" is set to none, just output "...".  */
  if (! out->is_mi_like_p () && args_type == NO_VALUES)
    out->field_string ("value", "...");
  else
    {
      /* Otherwise, print the value for both MI and the CLI, except
	 for the case of MI_PRINT_NO_VALUES.  */
      if (args_type != NO_VALUES)
	{
	  if (val == NULL)
	    {
	      gdb_assert (fa != NULL && fa->error != NULL);
	      out->field_fmt ("value", metadata_style.style (),
			      _("<error reading variable: %s>"),
			      fa->error.get ());
	    }
	  else
	    py_print_value (out, val, opts, 0, args_type, language);
	}
    }
}

/* Helper function to loop over frame arguments provided by the
   "frame_arguments" Python API.  Elements in the iterator must
   conform to the "Symbol Value" interface.  ITER is the Python
   iterable object, OUT is the output stream, ARGS_TYPE is an
   enumerator describing the argument format, PRINT_ARGS_FIELD is a
   flag which indicates if we output "ARGS=1" in MI output in commands
   where both arguments and locals are printed, and FRAME is the
   backing frame.  Returns EXT_LANG_BT_ERROR on error, with any GDB
   exceptions converted to a Python exception, or EXT_LANG_BT_OK on
   success.  */

static enum ext_lang_bt_status
enumerate_args (PyObject *iter,
		struct ui_out *out,
		enum ext_lang_frame_args args_type,
		int print_args_field,
		frame_info_ptr frame)
{
  struct value_print_options opts;

  get_user_print_options (&opts);

  if (args_type == CLI_SCALAR_VALUES)
    {
      /* True in "summary" mode, false otherwise.  */
      opts.summary = true;
    }

  opts.deref_ref = true;

  annotate_frame_args ();

  /*  Collect the first argument outside of the loop, so output of
      commas in the argument output is correct.  At the end of the
      loop block collect another item from the iterator, and, if it is
      not null emit a comma.  */
  gdbpy_ref<> item (PyIter_Next (iter));
  if (item == NULL && PyErr_Occurred ())
    return EXT_LANG_BT_ERROR;

  while (item != NULL)
    {
      const struct language_defn *language;
      gdb::unique_xmalloc_ptr<char> sym_name;
      struct symbol *sym;
      const struct block *sym_block;
      struct value *val;
      enum ext_lang_bt_status success = EXT_LANG_BT_ERROR;

      success = extract_sym (item.get (), &sym_name, &sym, &sym_block,
			     &language);
      if (success == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;

      success = extract_value (item.get (), &val);
      if (success == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;

      if (sym && out->is_mi_like_p ()
	  && ! mi_should_print (sym, MI_PRINT_ARGS))
	continue;

      /* If the object did not provide a value, read it using
	 read_frame_args and account for entry values, if any.  */
      if (val == NULL)
	{
	  struct frame_arg arg, entryarg;

	  /* If there is no value, and also no symbol, set error and
	     exit.  */
	  if (sym == NULL)
	    {
	      PyErr_SetString (PyExc_RuntimeError,
			       _("No symbol or value provided."));
	      return EXT_LANG_BT_ERROR;
	    }

	  read_frame_arg (user_frame_print_options,
			  sym, frame, &arg, &entryarg);

	  /* The object has not provided a value, so this is a frame
	     argument to be read by GDB.  In this case we have to
	     account for entry-values.  */

	  if (arg.entry_kind != print_entry_values_only)
	    {
	      py_print_single_arg (out, NULL, &arg,
				   NULL, &opts,
				   args_type,
				   print_args_field,
				   NULL);
	    }

	  if (entryarg.entry_kind != print_entry_values_no)
	    {
	      if (arg.entry_kind != print_entry_values_only)
		{
		  out->text (", ");
		  out->wrap_hint (4);
		}

	      py_print_single_arg (out, NULL, &entryarg, NULL, &opts,
				   args_type, print_args_field, NULL);
	    }
	}
      else
	{
	  /* If the object has provided a value, we just print that.  */
	  if (val != NULL)
	    py_print_single_arg (out, sym_name.get (), NULL, val, &opts,
				 args_type, print_args_field,
				 language);
	}

      /* Collect the next item from the iterator.  If
	 this is the last item, do not print the
	 comma.  */
      item.reset (PyIter_Next (iter));
      if (item != NULL)
	out->text (", ");
      else if (PyErr_Occurred ())
	return EXT_LANG_BT_ERROR;

      annotate_arg_end ();
    }

  return EXT_LANG_BT_OK;
}


/* Helper function to loop over variables provided by the
   "frame_locals" Python API.  Elements in the iterable must conform
   to the "Symbol Value" interface.  ITER is the Python iterable
   object, OUT is the output stream, INDENT is whether we should
   indent the output (for CLI), ARGS_TYPE is an enumerator describing
   the argument format, PRINT_ARGS_FIELD is flag which indicates
   whether to output the ARGS field in the case of
   -stack-list-variables and FRAME is the backing frame.  Returns
   EXT_LANG_BT_ERROR on error, with any GDB exceptions converted to a Python
   exception, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
enumerate_locals (PyObject *iter,
		  struct ui_out *out,
		  int indent,
		  enum ext_lang_frame_args args_type,
		  int print_args_field,
		  frame_info_ptr frame)
{
  struct value_print_options opts;

  get_user_print_options (&opts);
  opts.deref_ref = true;

  while (true)
    {
      const struct language_defn *language;
      gdb::unique_xmalloc_ptr<char> sym_name;
      struct value *val;
      enum ext_lang_bt_status success = EXT_LANG_BT_ERROR;
      struct symbol *sym;
      const struct block *sym_block;
      int local_indent = 8 + (8 * indent);
      std::optional<ui_out_emit_tuple> tuple;

      gdbpy_ref<> item (PyIter_Next (iter));
      if (item == NULL)
	break;

      success = extract_sym (item.get (), &sym_name, &sym, &sym_block,
			     &language);
      if (success == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;

      success = extract_value (item.get (), &val);
      if (success == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;

      if (sym != NULL && out->is_mi_like_p ()
	  && ! mi_should_print (sym, MI_PRINT_LOCALS))
	continue;

      /* If the object did not provide a value, read it.  */
      if (val == NULL)
	val = read_var_value (sym, sym_block, frame);

      /* With PRINT_NO_VALUES, MI does not emit a tuple normally as
	 each output contains only one field.  The exception is
	 -stack-list-variables, which always provides a tuple.  */
      if (out->is_mi_like_p ())
	{
	  if (print_args_field || args_type != NO_VALUES)
	    tuple.emplace (out, nullptr);
	}

      /* If the output is not MI we indent locals.  */
      out->spaces (local_indent);
      out->field_string ("name", sym_name.get ());
      out->text (" = ");

      if (args_type == MI_PRINT_SIMPLE_VALUES)
	py_print_type (out, val);

      /* CLI always prints values for locals.  MI uses the
	 simple/no/all system.  */
      if (! out->is_mi_like_p ())
	{
	  int val_indent = (indent + 1) * 4;

	  py_print_value (out, val, &opts, val_indent, args_type,
			  language);
	}
      else
	{
	  if (args_type != NO_VALUES)
	    py_print_value (out, val, &opts, 0, args_type,
			    language);
	}

      out->text ("\n");
    }

  if (!PyErr_Occurred ())
    return EXT_LANG_BT_OK;

  return EXT_LANG_BT_ERROR;
}

/*  Helper function for -stack-list-variables.  Returns EXT_LANG_BT_ERROR on
    error, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_mi_print_variables (PyObject *filter, struct ui_out *out,
		       struct value_print_options *opts,
		       enum ext_lang_frame_args args_type,
		       frame_info_ptr frame)
{
  gdbpy_ref<> args_iter (get_py_iter_from_func (filter, "frame_args"));
  if (args_iter == NULL)
    return EXT_LANG_BT_ERROR;

  gdbpy_ref<> locals_iter (get_py_iter_from_func (filter, "frame_locals"));
  if (locals_iter == NULL)
    return EXT_LANG_BT_ERROR;

  ui_out_emit_list list_emitter (out, "variables");

  if (args_iter != Py_None
      && (enumerate_args (args_iter.get (), out, args_type, 1, frame)
	  == EXT_LANG_BT_ERROR))
    return EXT_LANG_BT_ERROR;

  if (locals_iter != Py_None
      && (enumerate_locals (locals_iter.get (), out, 1, args_type, 1, frame)
	  == EXT_LANG_BT_ERROR))
    return EXT_LANG_BT_ERROR;

  return EXT_LANG_BT_OK;
}

/* Helper function for printing locals.  This function largely just
   creates the wrapping tuple, and calls enumerate_locals.  Returns
   EXT_LANG_BT_ERROR on error, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_print_locals (PyObject *filter,
		 struct ui_out *out,
		 enum ext_lang_frame_args args_type,
		 int indent,
		 frame_info_ptr frame)
{
  gdbpy_ref<> locals_iter (get_py_iter_from_func (filter, "frame_locals"));
  if (locals_iter == NULL)
    return EXT_LANG_BT_ERROR;

  ui_out_emit_list list_emitter (out, "locals");

  if (locals_iter != Py_None
      && (enumerate_locals (locals_iter.get (), out, indent, args_type,
			    0, frame) == EXT_LANG_BT_ERROR))
    return EXT_LANG_BT_ERROR;

  return EXT_LANG_BT_OK;
}

/* Helper function for printing frame arguments.  This function
   largely just creates the wrapping tuple, and calls enumerate_args.
   Returns EXT_LANG_BT_ERROR on error, with any GDB exceptions converted to
   a Python exception, or EXT_LANG_BT_OK on success.  */

static enum ext_lang_bt_status
py_print_args (PyObject *filter,
	       struct ui_out *out,
	       enum ext_lang_frame_args args_type,
	       frame_info_ptr frame)
{
  gdbpy_ref<> args_iter (get_py_iter_from_func (filter, "frame_args"));
  if (args_iter == NULL)
    return EXT_LANG_BT_ERROR;

  ui_out_emit_list list_emitter (out, "args");

  out->wrap_hint (3);
  annotate_frame_args ();
  out->text (" (");

  if (args_type == CLI_PRESENCE)
    {
      if (args_iter != Py_None)
	{
	  gdbpy_ref<> item (PyIter_Next (args_iter.get ()));

	  if (item != NULL)
	    out->text ("...");
	  else if (PyErr_Occurred ())
	    return EXT_LANG_BT_ERROR;
	}
    }
  else if (args_iter != Py_None
	   && (enumerate_args (args_iter.get (), out, args_type, 0, frame)
	       == EXT_LANG_BT_ERROR))
    return EXT_LANG_BT_ERROR;

  out->text (")");

  return EXT_LANG_BT_OK;
}

/*  Print a single frame to the designated output stream, detecting
    whether the output is MI or console, and formatting the output
    according to the conventions of that protocol.  FILTER is the
    frame-filter associated with this frame.  FLAGS is an integer
    describing the various print options.  The FLAGS variables is
    described in "apply_frame_filter" function.  ARGS_TYPE is an
    enumerator describing the argument format.  OUT is the output
    stream to print, INDENT is the level of indention for this frame
    (in the case of elided frames), and LEVELS_PRINTED is a hash-table
    containing all the frames level that have already been printed.
    If a frame level has been printed, do not print it again (in the
    case of elided frames).  Returns EXT_LANG_BT_ERROR on error, with any
    GDB exceptions converted to a Python exception, or EXT_LANG_BT_OK
    on success.  It can also throw an exception RETURN_QUIT.  */

static enum ext_lang_bt_status
py_print_frame (PyObject *filter, frame_filter_flags flags,
		enum ext_lang_frame_args args_type,
		struct ui_out *out, int indent, htab_t levels_printed)
{
  int has_addr = 0;
  CORE_ADDR address = 0;
  struct gdbarch *gdbarch = NULL;
  frame_info_ptr frame = NULL;
  struct value_print_options opts;

  int print_level, print_frame_info, print_args, print_locals;
  /* Note that the below default in non-mi mode is the same as the
     default value for the backtrace command (see the call to print_frame_info
     in backtrace_command_1).
     Having the same default ensures that 'bt' and 'bt no-filters'
     have the same behaviour when some filters exist but do not apply
     to a frame.  */
  enum print_what print_what
    = out->is_mi_like_p () ? LOC_AND_ADDRESS : LOCATION;
  gdb::unique_xmalloc_ptr<char> function_to_free;

  /* Extract print settings from FLAGS.  */
  print_level = (flags & PRINT_LEVEL) ? 1 : 0;
  print_frame_info = (flags & PRINT_FRAME_INFO) ? 1 : 0;
  print_args = (flags & PRINT_ARGS) ? 1 : 0;
  print_locals = (flags & PRINT_LOCALS) ? 1 : 0;

  get_user_print_options (&opts);
  if (print_frame_info)
    {
      std::optional<enum print_what> user_frame_info_print_what;

      get_user_print_what_frame_info (&user_frame_info_print_what);
      if (!out->is_mi_like_p () && user_frame_info_print_what.has_value ())
	{
	  /* Use the specific frame information desired by the user.  */
	  print_what = *user_frame_info_print_what;
	}
    }

  /* Get the underlying frame.  This is needed to determine GDB
  architecture, and also, in the cases of frame variables/arguments to
  read them if they returned filter object requires us to do so.  */
  gdbpy_ref<> py_inf_frame (PyObject_CallMethod (filter, "inferior_frame",
						 NULL));
  if (py_inf_frame == NULL)
    return EXT_LANG_BT_ERROR;

  frame = frame_object_to_frame_info (py_inf_frame.get ());
  if (frame == NULL)
    return EXT_LANG_BT_ERROR;

  symtab_and_line sal = find_frame_sal (frame);

  gdbarch = get_frame_arch (frame);

  /* stack-list-variables.  */
  if (print_locals && print_args && ! print_frame_info)
    {
      if (py_mi_print_variables (filter, out, &opts,
				 args_type, frame) == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;
      return EXT_LANG_BT_OK;
    }

  std::optional<ui_out_emit_tuple> tuple;

  /* -stack-list-locals does not require a
     wrapping frame attribute.  */
  if (print_frame_info || (print_args && ! print_locals))
    tuple.emplace (out, "frame");

  if (print_frame_info)
    {
      /* Elided frames are also printed with this function (recursively)
	 and are printed with indention.  */
      if (indent > 0)
	out->spaces (indent * 4);

      /* The address is required for frame annotations, and also for
	 address printing.  */
      if (PyObject_HasAttrString (filter, "address"))
	{
	  gdbpy_ref<> paddr (PyObject_CallMethod (filter, "address", NULL));

	  if (paddr == NULL)
	    return EXT_LANG_BT_ERROR;

	  if (paddr != Py_None)
	    {
	      if (get_addr_from_python (paddr.get (), &address) < 0)
		return EXT_LANG_BT_ERROR;

	      has_addr = 1;
	    }
	}
    }

  /* For MI, each piece is controlled individually.  */
  bool location_print = (print_frame_info
			 && !out->is_mi_like_p ()
			 && (print_what == LOCATION
			     || print_what == SRC_AND_LOC
			     || print_what == LOC_AND_ADDRESS
			     || print_what == SHORT_LOCATION));

  /* Print frame level.  MI does not require the level if
     locals/variables only are being printed.  */
  if (print_level
      && (location_print
	  || (out->is_mi_like_p () && (print_frame_info || print_args))))
    {
      struct frame_info **slot;
      int level;

      slot = (frame_info **) htab_find_slot (levels_printed,
						   frame.get(), INSERT);

      level = frame_relative_level (frame);

      /* Check if this frame has already been printed (there are cases
	 where elided synthetic dummy-frames have to 'borrow' the frame
	 architecture from the eliding frame.  If that is the case, do
	 not print 'level', but print spaces.  */
      if (*slot == frame)
	out->field_skip ("level");
      else
	{
	  *slot = frame.get ();
	  annotate_frame_begin (print_level ? level : 0,
				gdbarch, address);
	  out->text ("#");
	  out->field_fmt_signed (2, ui_left, "level", level);
	}
    }

  if (location_print || (out->is_mi_like_p () && print_frame_info))
    {
      /* Print address to the address field.  If an address is not provided,
	 print nothing.  */
      if (opts.addressprint && has_addr)
	{
	  if (!sal.symtab
	      || frame_show_address (frame, sal)
	      || print_what == LOC_AND_ADDRESS)
	    {
	      annotate_frame_address ();
	      out->field_core_addr ("addr", gdbarch, address);
	      if (get_frame_pc_masked (frame))
		out->field_string ("pac", " [PAC]");
	      annotate_frame_address_end ();
	      out->text (" in ");
	    }
	}

      /* Print frame function name.  */
      if (PyObject_HasAttrString (filter, "function"))
	{
	  gdbpy_ref<> py_func (PyObject_CallMethod (filter, "function", NULL));
	  const char *function = NULL;

	  if (py_func == NULL)
	    return EXT_LANG_BT_ERROR;

	  if (gdbpy_is_string (py_func.get ()))
	    {
	      function_to_free = python_string_to_host_string (py_func.get ());

	      if (function_to_free == NULL)
		return EXT_LANG_BT_ERROR;

	      function = function_to_free.get ();
	    }
	  else if (PyLong_Check (py_func.get ()))
	    {
	      CORE_ADDR addr;
	      struct bound_minimal_symbol msymbol;

	      if (get_addr_from_python (py_func.get (), &addr) < 0)
		return EXT_LANG_BT_ERROR;

	      msymbol = lookup_minimal_symbol_by_pc (addr);
	      if (msymbol.minsym != NULL)
		function = msymbol.minsym->print_name ();
	    }
	  else if (py_func != Py_None)
	    {
	      PyErr_SetString (PyExc_RuntimeError,
			       _("FrameDecorator.function: expecting a " \
				 "String, integer or None."));
	      return EXT_LANG_BT_ERROR;
	    }

	  annotate_frame_function_name ();
	  if (function == NULL)
	    out->field_skip ("func");
	  else
	    out->field_string ("func", function, function_name_style.style ());
	}
    }


  /* Frame arguments.  Check the result, and error if something went
     wrong.  */
  if (print_args && (location_print || out->is_mi_like_p ()))
    {
      if (py_print_args (filter, out, args_type, frame) == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;
    }

  /* File name/source/line number information.  */
  bool print_location_source
    = ((location_print && print_what != SHORT_LOCATION)
       || (out->is_mi_like_p () && print_frame_info));
  if (print_location_source)
    {
      annotate_frame_source_begin ();

      if (PyObject_HasAttrString (filter, "filename"))
	{
	  gdbpy_ref<> py_fn (PyObject_CallMethod (filter, "filename", NULL));

	  if (py_fn == NULL)
	    return EXT_LANG_BT_ERROR;

	  if (py_fn != Py_None)
	    {
	      gdb::unique_xmalloc_ptr<char>
		filename (python_string_to_host_string (py_fn.get ()));

	      if (filename == NULL)
		return EXT_LANG_BT_ERROR;

	      out->wrap_hint (3);
	      out->text (" at ");
	      annotate_frame_source_file ();
	      out->field_string ("file", filename.get (),
				 file_name_style.style ());
	      annotate_frame_source_file_end ();
	    }
	}

      if (PyObject_HasAttrString (filter, "line"))
	{
	  gdbpy_ref<> py_line (PyObject_CallMethod (filter, "line", NULL));
	  int line;

	  if (py_line == NULL)
	    return EXT_LANG_BT_ERROR;

	  if (py_line != Py_None)
	    {
	      line = PyLong_AsLong (py_line.get ());
	      if (PyErr_Occurred ())
		return EXT_LANG_BT_ERROR;

	      out->text (":");
	      annotate_frame_source_line ();
	      out->field_signed ("line", line);
	    }
	}
      if (out->is_mi_like_p ())
	out->field_string ("arch",
			   (gdbarch_bfd_arch_info (gdbarch))->printable_name);
    }

  bool source_print
    = (! out->is_mi_like_p ()
       && (print_what == SRC_LINE || print_what == SRC_AND_LOC));
  if (source_print)
    {
      if (print_location_source)
	out->text ("\n"); /* Newline after the location source.  */
      print_source_lines (sal.symtab, sal.line, sal.line + 1, 0);
    }

  /* For MI we need to deal with the "children" list population of
     elided frames, so if MI output detected do not send newline.  */
  if (! out->is_mi_like_p ())
    {
      annotate_frame_end ();
      /* print_source_lines has already printed a newline.  */
      if (!source_print)
	out->text ("\n");
    }

  if (print_locals)
    {
      if (py_print_locals (filter, out, args_type, indent,
			   frame) == EXT_LANG_BT_ERROR)
	return EXT_LANG_BT_ERROR;
    }

  if ((flags & PRINT_HIDE) == 0)
    {
      /* Finally recursively print elided frames, if any.  */
      gdbpy_ref<> elided (get_py_iter_from_func (filter, "elided"));
      if (elided == NULL)
	return EXT_LANG_BT_ERROR;

      if (elided != Py_None)
	{
	  PyObject *item;

	  ui_out_emit_list inner_list_emiter (out, "children");

	  indent++;

	  while ((item = PyIter_Next (elided.get ())))
	    {
	      gdbpy_ref<> item_ref (item);

	      enum ext_lang_bt_status success
		= py_print_frame (item, flags, args_type, out, indent,
				  levels_printed);

	      if (success == EXT_LANG_BT_ERROR)
		return EXT_LANG_BT_ERROR;
	    }
	  if (item == NULL && PyErr_Occurred ())
	    return EXT_LANG_BT_ERROR;
	}
    }

  return EXT_LANG_BT_OK;
}

/* Helper function to initiate frame filter invocation at starting
   frame FRAME.  */

static PyObject *
bootstrap_python_frame_filters (frame_info_ptr frame,
				int frame_low, int frame_high)
{
  gdbpy_ref<> frame_obj (frame_info_to_frame_object (frame));
  if (frame_obj == NULL)
    return NULL;

  gdbpy_ref<> module (PyImport_ImportModule ("gdb.frames"));
  if (module == NULL)
    return NULL;

  gdbpy_ref<> sort_func (PyObject_GetAttrString (module.get (),
						 "execute_frame_filters"));
  if (sort_func == NULL)
    return NULL;

  gdbpy_ref<> py_frame_low = gdb_py_object_from_longest (frame_low);
  if (py_frame_low == NULL)
    return NULL;

  gdbpy_ref<> py_frame_high = gdb_py_object_from_longest (frame_high);
  if (py_frame_high == NULL)
    return NULL;

  gdbpy_ref<> iterable (PyObject_CallFunctionObjArgs (sort_func.get (),
						      frame_obj.get (),
						      py_frame_low.get (),
						      py_frame_high.get (),
						      NULL));
  if (iterable == NULL)
    return NULL;

  if (iterable != Py_None)
    return PyObject_GetIter (iterable.get ());
  else
    return iterable.release ();
}

/*  This is the only publicly exported function in this file.  FRAME
    is the source frame to start frame-filter invocation.  FLAGS is an
    integer holding the flags for printing.  The following elements of
    the FRAME_FILTER_FLAGS enum denotes the make-up of FLAGS:
    PRINT_LEVEL is a flag indicating whether to print the frame's
    relative level in the output.  PRINT_FRAME_INFO is a flag that
    indicates whether this function should print the frame
    information, PRINT_ARGS is a flag that indicates whether to print
    frame arguments, and PRINT_LOCALS, likewise, with frame local
    variables.  ARGS_TYPE is an enumerator describing the argument
    format, OUT is the output stream to print.  FRAME_LOW is the
    beginning of the slice of frames to print, and FRAME_HIGH is the
    upper limit of the frames to count.  Returns EXT_LANG_BT_ERROR on error,
    or EXT_LANG_BT_OK on success.  */

enum ext_lang_bt_status
gdbpy_apply_frame_filter (const struct extension_language_defn *extlang,
			  frame_info_ptr frame, frame_filter_flags flags,
			  enum ext_lang_frame_args args_type,
			  struct ui_out *out, int frame_low, int frame_high)
{
  struct gdbarch *gdbarch = NULL;
  enum ext_lang_bt_status success = EXT_LANG_BT_ERROR;

  if (!gdb_python_initialized)
    return EXT_LANG_BT_NO_FILTERS;

  try
    {
      gdbarch = get_frame_arch (frame);
    }
  catch (const gdb_exception_error &except)
    {
      /* Let gdb try to print the stack trace.  */
      return EXT_LANG_BT_NO_FILTERS;
    }

  gdbpy_enter enter_py (gdbarch);

  /* When we're limiting the number of frames, be careful to request
     one extra frame, so that we can print a message if there are more
     frames.  */
  int frame_countdown = -1;
  if ((flags & PRINT_MORE_FRAMES) != 0 && frame_low >= 0 && frame_high >= 0)
    {
      ++frame_high;
      /* This has an extra +1 because it is checked before a frame is
	 printed.  */
      frame_countdown = frame_high - frame_low + 1;
    }

  gdbpy_ref<> iterable (bootstrap_python_frame_filters (frame, frame_low,
							frame_high));

  if (iterable == NULL)
    {
      /* Normally if there is an error GDB prints the exception,
	 abandons the backtrace and exits.  The user can then call "bt
	 no-filters", and get a default backtrace (it would be
	 confusing to automatically start a standard backtrace halfway
	 through a Python filtered backtrace).  However in the case
	 where GDB cannot initialize the frame filters (most likely
	 due to incorrect auto-load paths), GDB has printed nothing.
	 In this case it is OK to print the default backtrace after
	 printing the error message.  GDB returns EXT_LANG_BT_NO_FILTERS
	 here to signify there are no filters after printing the
	 initialization error.  This return code will trigger a
	 default backtrace.  */

      gdbpy_print_stack_or_quit ();
      return EXT_LANG_BT_NO_FILTERS;
    }

  /* If iterable is None, then there are no frame filters registered.
     If this is the case, defer to default GDB printing routines in MI
     and CLI.  */
  if (iterable == Py_None)
    return EXT_LANG_BT_NO_FILTERS;

  htab_up levels_printed (htab_create (20,
				       htab_hash_pointer,
				       htab_eq_pointer,
				       NULL));

  while (true)
    {
      gdbpy_ref<> item (PyIter_Next (iterable.get ()));

      if (item == NULL)
	{
	  if (PyErr_Occurred ())
	    {
	      gdbpy_print_stack_or_quit ();
	      return EXT_LANG_BT_ERROR;
	    }
	  break;
	}

      if (frame_countdown != -1)
	{
	  gdb_assert ((flags & PRINT_MORE_FRAMES) != 0);
	  --frame_countdown;
	  if (frame_countdown == 0)
	    {
	      /* We've printed all the frames we were asked to
		 print, but more frames existed.  */
	      gdb_printf (_("(More stack frames follow...)\n"));
	      break;
	    }
	}

      try
	{
	  success = py_print_frame (item.get (), flags, args_type, out, 0,
				    levels_printed.get ());
	}
      catch (const gdb_exception_error &except)
	{
	  gdbpy_convert_exception (except);
	  success = EXT_LANG_BT_ERROR;
	}

      /* Do not exit on error printing a single frame.  Print the
	 error and continue with other frames.  */
      if (success == EXT_LANG_BT_ERROR)
	gdbpy_print_stack_or_quit ();
    }

  return success;
}

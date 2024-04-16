/* MI Command Set - varobj commands.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "mi-cmds.h"
#include "mi-main.h"
#include "ui-out.h"
#include "mi-out.h"
#include "varobj.h"
#include "language.h"
#include "value.h"
#include <ctype.h>
#include "mi-getopt.h"
#include "gdbthread.h"
#include "mi-parse.h"
#include <optional>
#include "inferior.h"

static void varobj_update_one (struct varobj *var,
			       enum print_values print_values,
			       bool is_explicit);

static int mi_print_value_p (struct varobj *var,
			     enum print_values print_values);

/* Print variable object VAR.  The PRINT_VALUES parameter controls
   if the value should be printed.  The PRINT_EXPRESSION parameter
   controls if the expression should be printed.  */

static void 
print_varobj (struct varobj *var, enum print_values print_values,
	      int print_expression)
{
  struct ui_out *uiout = current_uiout;
  int thread_id;

  uiout->field_string ("name", varobj_get_objname (var));
  if (print_expression)
    {
      std::string exp = varobj_get_expression (var);

      uiout->field_string ("exp", exp);
    }
  uiout->field_signed ("numchild", varobj_get_num_children (var));
  
  if (mi_print_value_p (var, print_values))
    {
      std::string val = varobj_get_value (var);

      uiout->field_string ("value", val);
    }

  std::string type = varobj_get_type (var);
  if (!type.empty ())
    uiout->field_string ("type", type);

  thread_id = varobj_get_thread_id (var);
  if (thread_id > 0)
    uiout->field_signed ("thread-id", thread_id);

  if (varobj_get_frozen (var))
    uiout->field_signed ("frozen", 1);

  gdb::unique_xmalloc_ptr<char> display_hint = varobj_get_display_hint (var);
  if (display_hint)
    uiout->field_string ("displayhint", display_hint.get ());

  if (varobj_is_dynamic_p (var))
    uiout->field_signed ("dynamic", 1);
}

/* VAROBJ operations */

void
mi_cmd_var_create (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  CORE_ADDR frameaddr = 0;
  struct varobj *var;
  const char *frame;
  const char *expr;
  enum varobj_type var_type;

  if (argc != 3)
    error (_("-var-create: Usage: NAME FRAME EXPRESSION."));

  frame = argv[1];
  expr = argv[2];

  const char *name = argv[0];
  std::string gen_name;
  if (strcmp (name, "-") == 0)
    {
      gen_name = varobj_gen_name ();
      name = gen_name.c_str ();
    }
  else if (!isalpha (name[0]))
    error (_("-var-create: name of object must begin with a letter"));

  if (strcmp (frame, "*") == 0)
    var_type = USE_CURRENT_FRAME;
  else if (strcmp (frame, "@") == 0)
    var_type = USE_SELECTED_FRAME;  
  else
    {
      var_type = USE_SPECIFIED_FRAME;
      frameaddr = string_to_core_addr (frame);
    }

  if (varobjdebug)
    gdb_printf (gdb_stdlog,
		"Name=\"%s\", Frame=\"%s\" (%s), Expression=\"%s\"\n",
		name, frame, hex_string (frameaddr), expr);

  var = varobj_create (name, expr, frameaddr, var_type);

  if (var == NULL)
    error (_("-var-create: unable to create variable object"));

  print_varobj (var, PRINT_ALL_VALUES, 0 /* don't print expression */);

  uiout->field_signed ("has_more", varobj_has_more (var, 0));
}

void
mi_cmd_var_delete (const char *command, const char *const *argv, int argc)
{
  const char *name;
  struct varobj *var;
  int numdel;
  int children_only_p = 0;
  struct ui_out *uiout = current_uiout;

  if (argc < 1 || argc > 2)
    error (_("-var-delete: Usage: [-c] EXPRESSION."));

  name = argv[0];

  /* If we have one single argument it cannot be '-c' or any string
     starting with '-'.  */
  if (argc == 1)
    {
      if (strcmp (name, "-c") == 0)
	error (_("-var-delete: Missing required "
		 "argument after '-c': variable object name"));
      if (*name == '-')
	error (_("-var-delete: Illegal variable object name"));
    }

  /* If we have 2 arguments they must be '-c' followed by a string
     which would be the variable name.  */
  if (argc == 2)
    {
      if (strcmp (name, "-c") != 0)
	error (_("-var-delete: Invalid option."));
      children_only_p = 1;
      name = argv[1];
    }

  /* If we didn't error out, now NAME contains the name of the
     variable.  */

  var = varobj_get_handle (name);

  numdel = varobj_delete (var, children_only_p);

  uiout->field_signed ("ndeleted", numdel);
}

/* Parse a string argument into a format value.  */

static enum varobj_display_formats
mi_parse_format (const char *arg)
{
  if (arg != NULL)
    {
      int len;

      len = strlen (arg);

      if (strncmp (arg, "natural", len) == 0)
	return FORMAT_NATURAL;
      else if (strncmp (arg, "binary", len) == 0)
	return FORMAT_BINARY;
      else if (strncmp (arg, "decimal", len) == 0)
	return FORMAT_DECIMAL;
      else if (strncmp (arg, "hexadecimal", len) == 0)
	return FORMAT_HEXADECIMAL;
      else if (strncmp (arg, "octal", len) == 0)
	return FORMAT_OCTAL;
      else if (strncmp (arg, "zero-hexadecimal", len) == 0)
	return FORMAT_ZHEXADECIMAL;
    }

  error (_("Must specify the format as: \"natural\", "
	   "\"binary\", \"decimal\", \"hexadecimal\", \"octal\" or \"zero-hexadecimal\""));
}

void
mi_cmd_var_set_format (const char *command, const char *const *argv, int argc)
{
  enum varobj_display_formats format;
  struct varobj *var;
  struct ui_out *uiout = current_uiout;

  if (argc != 2)
    error (_("-var-set-format: Usage: NAME FORMAT."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);

  format = mi_parse_format (argv[1]);
  
  /* Set the format of VAR to the given format.  */
  varobj_set_display_format (var, format);

  /* Report the new current format.  */
  uiout->field_string ("format", varobj_format_string[(int) format]);
 
  /* Report the value in the new format.  */
  std::string val = varobj_get_value (var);
  uiout->field_string ("value", val);
}

void
mi_cmd_var_set_visualizer (const char *command, const char *const *argv,
			   int argc)
{
  struct varobj *var;

  if (argc != 2)
    error (_("-var-set-visualizer: Usage: NAME VISUALIZER_FUNCTION."));

  var = varobj_get_handle (argv[0]);

  if (var == NULL)
    error (_("Variable object not found"));

  varobj_set_visualizer (var, argv[1]);
}

void
mi_cmd_var_set_frozen (const char *command, const char *const *argv, int argc)
{
  struct varobj *var;
  bool frozen;

  if (argc != 2)
    error (_("-var-set-frozen: Usage: NAME FROZEN_FLAG."));

  var = varobj_get_handle (argv[0]);

  if (strcmp (argv[1], "0") == 0)
    frozen = false;
  else if (strcmp (argv[1], "1") == 0)
    frozen = true;
  else
    error (_("Invalid flag value"));

  varobj_set_frozen (var, frozen);

  /* We don't automatically return the new value, or what varobjs got
     new values during unfreezing.  If this information is required,
     client should call -var-update explicitly.  */
}

void
mi_cmd_var_show_format (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  enum varobj_display_formats format;
  struct varobj *var;

  if (argc != 1)
    error (_("-var-show-format: Usage: NAME."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);

  format = varobj_get_display_format (var);

  /* Report the current format.  */
  uiout->field_string ("format", varobj_format_string[(int) format]);
}

void
mi_cmd_var_info_num_children (const char *command, const char *const *argv,
			      int argc)
{
  struct ui_out *uiout = current_uiout;
  struct varobj *var;

  if (argc != 1)
    error (_("-var-info-num-children: Usage: NAME."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);

  uiout->field_signed ("numchild", varobj_get_num_children (var));
}

/* Return 1 if given the argument PRINT_VALUES we should display
   the varobj VAR.  */

static int
mi_print_value_p (struct varobj *var, enum print_values print_values)
{
  struct type *type;

  if (print_values == PRINT_NO_VALUES)
    return 0;

  if (print_values == PRINT_ALL_VALUES)
    return 1;

  if (varobj_is_dynamic_p (var))
    return 1;

  type = varobj_get_gdb_type (var);
  if (type == NULL)
    return 1;
  else
    return mi_simple_type_p (type);
}

/* See mi-cmds.h.  */

bool
mi_simple_type_p (struct type *type)
{
  type = check_typedef (type);

  if (TYPE_IS_REFERENCE (type))
    type = check_typedef (type->target_type ());

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return false;
    default:
      return true;
    }
}

void
mi_cmd_var_list_children (const char *command, const char *const *argv,
			  int argc)
{
  struct ui_out *uiout = current_uiout;
  struct varobj *var;  
  enum print_values print_values;
  int from, to;

  if (argc < 1 || argc > 4)
    error (_("-var-list-children: Usage: "
	     "[PRINT_VALUES] NAME [FROM TO]"));

  /* Get varobj handle, if a valid var obj name was specified.  */
  if (argc == 1 || argc == 3)
    var = varobj_get_handle (argv[0]);
  else
    var = varobj_get_handle (argv[1]);

  if (argc > 2)
    {
      from = atoi (argv[argc - 2]);
      to = atoi (argv[argc - 1]);
    }
  else
    {
      from = -1;
      to = -1;
    }

  const std::vector<varobj *> &children
    = varobj_list_children (var, &from, &to);

  uiout->field_signed ("numchild", to - from);
  if (argc == 2 || argc == 4)
    print_values = mi_parse_print_values (argv[0]);
  else
    print_values = PRINT_NO_VALUES;

  gdb::unique_xmalloc_ptr<char> display_hint = varobj_get_display_hint (var);
  if (display_hint)
    uiout->field_string ("displayhint", display_hint.get ());

  if (from < to)
    {
      ui_out_emit_list list_emitter (uiout, "children");
      for (int ix = from; ix < to && ix < children.size (); ix++)
	{
	  ui_out_emit_tuple child_emitter (uiout, "child");

	  print_varobj (children[ix], print_values, 1 /* print expression */);
	}
    }

  uiout->field_signed ("has_more", varobj_has_more (var, to));
}

void
mi_cmd_var_info_type (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  struct varobj *var;

  if (argc != 1)
    error (_("-var-info-type: Usage: NAME."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);

  std::string type_name = varobj_get_type (var);
  uiout->field_string ("type", type_name);
}

void
mi_cmd_var_info_path_expression (const char *command, const char *const *argv,
				 int argc)
{
  struct ui_out *uiout = current_uiout;
  struct varobj *var;

  if (argc != 1)
    error (_("-var-info-path-expression: Usage: NAME."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);
  
  const char *path_expr = varobj_get_path_expr (var);

  uiout->field_string ("path_expr", path_expr);
}

void
mi_cmd_var_info_expression (const char *command, const char *const *argv,
			    int argc)
{
  struct ui_out *uiout = current_uiout;
  const struct language_defn *lang;
  struct varobj *var;

  if (argc != 1)
    error (_("-var-info-expression: Usage: NAME."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);

  lang = varobj_get_language (var);

  uiout->field_string ("lang", lang->natural_name ());

  std::string exp = varobj_get_expression (var);
  uiout->field_string ("exp", exp);
}

void
mi_cmd_var_show_attributes (const char *command, const char *const *argv,
			    int argc)
{
  struct ui_out *uiout = current_uiout;
  int attr;
  const char *attstr;
  struct varobj *var;

  if (argc != 1)
    error (_("-var-show-attributes: Usage: NAME."));

  /* Get varobj handle, if a valid var obj name was specified */
  var = varobj_get_handle (argv[0]);

  attr = varobj_get_attributes (var);
  /* FIXME: define masks for attributes */
  if (attr & 0x00000001)
    attstr = "editable";
  else
    attstr = "noneditable";

  uiout->field_string ("attr", attstr);
}

void
mi_cmd_var_evaluate_expression (const char *command, const char *const *argv,
				int argc)
{
  struct ui_out *uiout = current_uiout;
  struct varobj *var;

  enum varobj_display_formats format;
  int formatFound;
  int oind;
  const char *oarg;
    
  enum opt
  {
    OP_FORMAT
  };
  static const struct mi_opt opts[] =
    {
      {"f", OP_FORMAT, 1},
      { 0, 0, 0 }
    };

  /* Parse arguments.  */
  format = FORMAT_NATURAL;
  formatFound = 0;
  oind = 0;
  while (1)
    {
      int opt = mi_getopt ("-var-evaluate-expression", argc, argv,
			   opts, &oind, &oarg);

      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case OP_FORMAT:
	  if (formatFound)
	    error (_("Cannot specify format more than once"));
   
	  format = mi_parse_format (oarg);
	  formatFound = 1;
	  break;
	}
    }

  if (oind >= argc)
    error (_("Usage: [-f FORMAT] NAME"));
   
  if (oind < argc - 1)
    error (_("Garbage at end of command"));
 
  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[oind]);
   
  if (formatFound)
    {
      std::string val = varobj_get_formatted_value (var, format);

      uiout->field_string ("value", val);
    }
  else
    {
      std::string val = varobj_get_value (var);

      uiout->field_string ("value", val);
    }
}

void
mi_cmd_var_assign (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  struct varobj *var;

  if (argc != 2)
    error (_("-var-assign: Usage: NAME EXPRESSION."));

  /* Get varobj handle, if a valid var obj name was specified.  */
  var = varobj_get_handle (argv[0]);

  if (!varobj_editable_p (var))
    error (_("-var-assign: Variable object is not editable"));

  const char *expression = argv[1];

  /* MI command '-var-assign' may write memory, so suppress memory
     changed notification if it does.  */
  scoped_restore save_suppress
    = make_scoped_restore (&mi_suppress_notification.memory, 1);

  if (!varobj_set_value (var, expression))
    error (_("-var-assign: Could not assign "
	     "expression to variable object"));

  std::string val = varobj_get_value (var);
  uiout->field_string ("value", val);
}

/* Helper for mi_cmd_var_update - update each VAR.  */

static void
mi_cmd_var_update_iter (struct varobj *var, bool only_floating,
			enum print_values print_values)
{
  bool thread_stopped;

  int thread_id = varobj_get_thread_id (var);

  if (thread_id == -1)
    {
      thread_stopped = (inferior_ptid == null_ptid
			|| inferior_thread ()->state == THREAD_STOPPED);
    }
  else
    {
      thread_info *tp = find_thread_global_id (thread_id);

      thread_stopped = (tp == NULL
			|| tp->state == THREAD_STOPPED);
    }

  if (thread_stopped
      && (!only_floating || varobj_floating_p (var)))
    varobj_update_one (var, print_values, false /* implicit */);
}

void
mi_cmd_var_update (const char *command, const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  const char *name;
  enum print_values print_values;

  if (argc != 1 && argc != 2)
    error (_("-var-update: Usage: [PRINT_VALUES] NAME."));

  if (argc == 1)
    name = argv[0];
  else
    name = argv[1];

  if (argc == 2)
    print_values = mi_parse_print_values (argv[0]);
  else
    print_values = PRINT_NO_VALUES;

  ui_out_emit_list list_emitter (uiout, "changelist");

  /* Check if the parameter is a "*", which means that we want to
     update all variables.  */

  if ((*name == '*' || *name == '@') && (*(name + 1) == '\0'))
    {
      /* varobj_update_one automatically updates all the children of
	 VAROBJ.  Therefore update each VAROBJ only once by iterating
	 only the root VAROBJs.  */

      all_root_varobjs ([=] (varobj *var)
	{ mi_cmd_var_update_iter (var, *name == '0', print_values); });
    }
  else
    {
      /* Get varobj handle, if a valid var obj name was specified.  */
      struct varobj *var = varobj_get_handle (name);

      varobj_update_one (var, print_values, true /* explicit */);
    }
}

/* Helper for mi_cmd_var_update().  */

static void
varobj_update_one (struct varobj *var, enum print_values print_values,
		   bool is_explicit)
{
  struct ui_out *uiout = current_uiout;

  std::vector<varobj_update_result> changes = varobj_update (&var, is_explicit);
  
  for (const varobj_update_result &r : changes)
    {
      int from, to;

      ui_out_emit_tuple tuple_emitter (uiout, nullptr);
      uiout->field_string ("name", varobj_get_objname (r.varobj));

      switch (r.status)
	{
	case VAROBJ_IN_SCOPE:
	  if (mi_print_value_p (r.varobj, print_values))
	    {
	      std::string val = varobj_get_value (r.varobj);

	      uiout->field_string ("value", val);
	    }
	  uiout->field_string ("in_scope", "true");
	  break;
	case VAROBJ_NOT_IN_SCOPE:
	  uiout->field_string ("in_scope", "false");
	  break;
	case VAROBJ_INVALID:
	  uiout->field_string ("in_scope", "invalid");
	  break;
	}

      if (r.status != VAROBJ_INVALID)
	{
	  if (r.type_changed)
	    uiout->field_string ("type_changed", "true");
	  else
	    uiout->field_string ("type_changed", "false");
	}

      if (r.type_changed)
	{
	  std::string type_name = varobj_get_type (r.varobj);

	  uiout->field_string ("new_type", type_name);
	}

      if (r.type_changed || r.children_changed)
	uiout->field_signed ("new_num_children",
			     varobj_get_num_children (r.varobj));

      gdb::unique_xmalloc_ptr<char> display_hint
	= varobj_get_display_hint (r.varobj);
      if (display_hint)
	uiout->field_string ("displayhint", display_hint.get ());

      if (varobj_is_dynamic_p (r.varobj))
	uiout->field_signed ("dynamic", 1);

      varobj_get_child_range (r.varobj, &from, &to);
      uiout->field_signed ("has_more", varobj_has_more (r.varobj, to));

      if (!r.newobj.empty ())
	{
	  ui_out_emit_list list_emitter (uiout, "new_children");

	  for (varobj *child : r.newobj)
	    {
	      ui_out_emit_tuple inner_tuple_emitter (uiout, NULL);
	      print_varobj (child, print_values, 1 /* print_expression */);
	    }
	}
    }
}

void
mi_cmd_enable_pretty_printing (const char *command, const char *const *argv,
			       int argc)
{
  if (argc != 0)
    error (_("-enable-pretty-printing: no arguments allowed"));

  varobj_enable_pretty_printing ();
}

void
mi_cmd_var_set_update_range (const char *command, const char *const *argv,
			     int argc)
{
  struct varobj *var;
  int from, to;

  if (argc != 3)
    error (_("-var-set-update-range: Usage: VAROBJ FROM TO"));
  
  var = varobj_get_handle (argv[0]);
  from = atoi (argv[1]);
  to = atoi (argv[2]);

  varobj_set_child_range (var, from, to);
}

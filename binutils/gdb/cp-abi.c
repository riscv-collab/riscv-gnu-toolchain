/* Generic code for supporting multiple C++ ABI's

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "language.h"
#include "value.h"
#include "cp-abi.h"
#include "command.h"
#include "gdbcmd.h"
#include "ui-out.h"
static struct cp_abi_ops *find_cp_abi (const char *short_name);

static struct cp_abi_ops current_cp_abi = { "", NULL };
static struct cp_abi_ops auto_cp_abi = { "auto", NULL };

#define CP_ABI_MAX 8
static struct cp_abi_ops *cp_abis[CP_ABI_MAX];
static int num_cp_abis = 0;

enum ctor_kinds
is_constructor_name (const char *name)
{
  if ((current_cp_abi.is_constructor_name) == NULL)
    error (_("ABI doesn't define required function is_constructor_name"));
  return (*current_cp_abi.is_constructor_name) (name);
}

enum dtor_kinds
is_destructor_name (const char *name)
{
  if ((current_cp_abi.is_destructor_name) == NULL)
    error (_("ABI doesn't define required function is_destructor_name"));
  return (*current_cp_abi.is_destructor_name) (name);
}

int
is_vtable_name (const char *name)
{
  if ((current_cp_abi.is_vtable_name) == NULL)
    error (_("ABI doesn't define required function is_vtable_name"));
  return (*current_cp_abi.is_vtable_name) (name);
}

int
is_operator_name (const char *name)
{
  if ((current_cp_abi.is_operator_name) == NULL)
    error (_("ABI doesn't define required function is_operator_name"));
  return (*current_cp_abi.is_operator_name) (name);
}

int
baseclass_offset (struct type *type, int index, const gdb_byte *valaddr,
		  LONGEST embedded_offset, CORE_ADDR address,
		  const struct value *val)
{
  int res = 0;

  gdb_assert (current_cp_abi.baseclass_offset != NULL);

  try
    {
      res = (*current_cp_abi.baseclass_offset) (type, index, valaddr,
						embedded_offset,
						address, val);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;

      throw_error (NOT_AVAILABLE_ERROR,
		   _("Cannot determine virtual baseclass offset "
		     "of incomplete object"));
    }

  return res;
}

struct value *
value_virtual_fn_field (struct value **arg1p,
			struct fn_field *f, int j,
			struct type *type, int offset)
{
  if ((current_cp_abi.virtual_fn_field) == NULL)
    return NULL;
  return (*current_cp_abi.virtual_fn_field) (arg1p, f, j,
					     type, offset);
}

struct type *
value_rtti_type (struct value *v, int *full,
		 LONGEST *top, int *using_enc)
{
  struct type *ret = NULL;

  if ((current_cp_abi.rtti_type) == NULL
      || !HAVE_CPLUS_STRUCT (check_typedef (v->type ())))
    return NULL;
  try
    {
      ret = (*current_cp_abi.rtti_type) (v, full, top, using_enc);
    }
  catch (const gdb_exception_error &e)
    {
      return NULL;
    }

  return ret;
}

void
cplus_print_method_ptr (const gdb_byte *contents,
			struct type *type,
			struct ui_file *stream)
{
  if (current_cp_abi.print_method_ptr == NULL)
    error (_("GDB does not support pointers to methods on this target"));
  (*current_cp_abi.print_method_ptr) (contents, type, stream);
}

int
cplus_method_ptr_size (struct type *to_type)
{
  if (current_cp_abi.method_ptr_size == NULL)
    error (_("GDB does not support pointers to methods on this target"));
  return (*current_cp_abi.method_ptr_size) (to_type);
}

void
cplus_make_method_ptr (struct type *type, gdb_byte *contents,
		       CORE_ADDR value, int is_virtual)
{
  if (current_cp_abi.make_method_ptr == NULL)
    error (_("GDB does not support pointers to methods on this target"));
  (*current_cp_abi.make_method_ptr) (type, contents, value, is_virtual);
}

CORE_ADDR
cplus_skip_trampoline (frame_info_ptr frame,
		       CORE_ADDR stop_pc)
{
  if (current_cp_abi.skip_trampoline == NULL)
    return 0;
  return (*current_cp_abi.skip_trampoline) (frame, stop_pc);
}

struct value *
cplus_method_ptr_to_value (struct value **this_p,
			   struct value *method_ptr)
{
  if (current_cp_abi.method_ptr_to_value == NULL)
    error (_("GDB does not support pointers to methods on this target"));
  return (*current_cp_abi.method_ptr_to_value) (this_p, method_ptr);
}

/* See cp-abi.h.  */

void
cplus_print_vtable (struct value *value)
{
  if (current_cp_abi.print_vtable == NULL)
    error (_("GDB cannot print the vtable on this target"));
  (*current_cp_abi.print_vtable) (value);
}

/* See cp-abi.h.  */

struct value *
cplus_typeid (struct value *value)
{
  if (current_cp_abi.get_typeid == NULL)
    error (_("GDB cannot find the typeid on this target"));
  return (*current_cp_abi.get_typeid) (value);
}

/* See cp-abi.h.  */

struct type *
cplus_typeid_type (struct gdbarch *gdbarch)
{
  if (current_cp_abi.get_typeid_type == NULL)
    error (_("GDB cannot find the type for 'typeid' on this target"));
  return (*current_cp_abi.get_typeid_type) (gdbarch);
}

/* See cp-abi.h.  */

struct type *
cplus_type_from_type_info (struct value *value)
{
  if (current_cp_abi.get_type_from_type_info == NULL)
    error (_("GDB cannot find the type from a std::type_info on this target"));
  return (*current_cp_abi.get_type_from_type_info) (value);
}

/* See cp-abi.h.  */

std::string
cplus_typename_from_type_info (struct value *value)
{
  if (current_cp_abi.get_typename_from_type_info == NULL)
    error (_("GDB cannot find the type name "
	     "from a std::type_info on this target"));
  return (*current_cp_abi.get_typename_from_type_info) (value);
}

/* See cp-abi.h.  */

struct language_pass_by_ref_info
cp_pass_by_reference (struct type *type)
{
  if ((current_cp_abi.pass_by_reference) == NULL)
    return {};
  return (*current_cp_abi.pass_by_reference) (type);
}

/* Set the current C++ ABI to SHORT_NAME.  */

static int
switch_to_cp_abi (const char *short_name)
{
  struct cp_abi_ops *abi;

  abi = find_cp_abi (short_name);
  if (abi == NULL)
    return 0;

  current_cp_abi = *abi;
  return 1;
}

/* Add ABI to the list of supported C++ ABI's.  */

int
register_cp_abi (struct cp_abi_ops *abi)
{
  if (num_cp_abis == CP_ABI_MAX)
    internal_error (_("Too many C++ ABIs, please increase "
		      "CP_ABI_MAX in cp-abi.c"));

  cp_abis[num_cp_abis++] = abi;

  return 1;
}

/* Set the ABI to use in "auto" mode to SHORT_NAME.  */

void
set_cp_abi_as_auto_default (const char *short_name)
{
  struct cp_abi_ops *abi = find_cp_abi (short_name);

  if (abi == NULL)
    internal_error (_("Cannot find C++ ABI \"%s\" to set it as auto default."),
		    short_name);

  xfree ((char *) auto_cp_abi.longname);
  xfree ((char *) auto_cp_abi.doc);

  auto_cp_abi = *abi;

  auto_cp_abi.shortname = "auto";
  auto_cp_abi.longname = xstrprintf ("currently \"%s\"",
				     abi->shortname).release ();
  auto_cp_abi.doc = xstrprintf ("Automatically selected; currently \"%s\"",
				abi->shortname).release ();

  /* Since we copy the current ABI into current_cp_abi instead of
     using a pointer, if auto is currently the default, we need to
     reset it.  */
  if (strcmp (current_cp_abi.shortname, "auto") == 0)
    switch_to_cp_abi ("auto");
}

/* Return the ABI operations associated with SHORT_NAME.  */

static struct cp_abi_ops *
find_cp_abi (const char *short_name)
{
  int i;

  for (i = 0; i < num_cp_abis; i++)
    if (strcmp (cp_abis[i]->shortname, short_name) == 0)
      return cp_abis[i];

  return NULL;
}

/* Display the list of registered C++ ABIs.  */

static void
list_cp_abis (int from_tty)
{
  struct ui_out *uiout = current_uiout;
  int i;

  uiout->text ("The available C++ ABIs are:\n");
  ui_out_emit_tuple tuple_emitter (uiout, "cp-abi-list");
  for (i = 0; i < num_cp_abis; i++)
    {
      char pad[14];
      int padcount;

      uiout->text ("  ");
      uiout->field_string ("cp-abi", cp_abis[i]->shortname);

      padcount = 16 - 2 - strlen (cp_abis[i]->shortname);
      pad[padcount] = 0;
      while (padcount > 0)
	pad[--padcount] = ' ';
      uiout->text (pad);

      uiout->field_string ("doc", cp_abis[i]->doc);
      uiout->text ("\n");
    }
}

/* Set the current C++ ABI, or display the list of options if no
   argument is given.  */

static void
set_cp_abi_cmd (const char *args, int from_tty)
{
  if (args == NULL)
    {
      list_cp_abis (from_tty);
      return;
    }

  if (!switch_to_cp_abi (args))
    error (_("Could not find \"%s\" in ABI list"), args);
}

/* A completion function for "set cp-abi".  */

static void
cp_abi_completer (struct cmd_list_element *ignore,
		  completion_tracker &tracker,
		  const char *text, const char *word)
{
  static const char **cp_abi_names;

  if (cp_abi_names == NULL)
    {
      int i;

      cp_abi_names = XNEWVEC (const char *, num_cp_abis + 1);
      for (i = 0; i < num_cp_abis; ++i)
	cp_abi_names[i] = cp_abis[i]->shortname;
      cp_abi_names[i] = NULL;
    }

  complete_on_enum (tracker, cp_abi_names, text, word);
}

/* Show the currently selected C++ ABI.  */

static void
show_cp_abi_cmd (const char *args, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  uiout->text ("The currently selected C++ ABI is \"");

  uiout->field_string ("cp-abi", current_cp_abi.shortname);
  uiout->text ("\" (");
  uiout->field_string ("longname", current_cp_abi.longname);
  uiout->text (").\n");
}

void _initialize_cp_abi ();
void
_initialize_cp_abi ()
{
  struct cmd_list_element *c;

  register_cp_abi (&auto_cp_abi);
  switch_to_cp_abi ("auto");

  c = add_cmd ("cp-abi", class_obscure, set_cp_abi_cmd, _("\
Set the ABI used for inspecting C++ objects.\n\
\"set cp-abi\" with no arguments will list the available ABIs."),
	       &setlist);
  set_cmd_completer (c, cp_abi_completer);

  add_cmd ("cp-abi", class_obscure, show_cp_abi_cmd,
	   _("Show the ABI used for inspecting C++ objects."),
	   &showlist);
}

/* Implementation of the GDB variable objects API.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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
#include "expression.h"
#include "frame.h"
#include "language.h"
#include "gdbcmd.h"
#include "block.h"
#include "valprint.h"
#include "gdbsupport/gdb_regex.h"

#include "varobj.h"
#include "gdbthread.h"
#include "inferior.h"
#include "varobj-iter.h"
#include "parser-defs.h"
#include "gdbarch.h"
#include <algorithm>
#include "observable.h"

#if HAVE_PYTHON
#include "python/python.h"
#include "python/python-internal.h"
#else
typedef int PyObject;
#endif

/* See varobj.h.  */

unsigned int varobjdebug = 0;
static void
show_varobjdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Varobj debugging is %s.\n"), value);
}

/* String representations of gdb's format codes.  */
const char *varobj_format_string[] =
  { "natural", "binary", "decimal", "hexadecimal", "octal", "zero-hexadecimal" };

/* True if we want to allow Python-based pretty-printing.  */
static bool pretty_printing = false;

void
varobj_enable_pretty_printing (void)
{
  pretty_printing = true;
}

/* Data structures */

/* Every root variable has one of these structures saved in its
   varobj.  */
struct varobj_root
{
  /* The expression for this parent.  */
  expression_up exp;

  /* Cached arch from exp, for use in case exp gets invalidated.  */
  struct gdbarch *gdbarch = nullptr;

  /* Cached language from exp, for use in case exp gets invalidated.  */
  const struct language_defn *language_defn = nullptr;

  /* Block for which this expression is valid.  */
  const struct block *valid_block = NULL;

  /* The frame for this expression.  This field is set iff valid_block is
     not NULL.  */
  struct frame_id frame = null_frame_id;

  /* The global thread ID that this varobj_root belongs to.  This field
     is only valid if valid_block is not NULL.
     When not 0, indicates which thread 'frame' belongs to.
     When 0, indicates that the thread list was empty when the varobj_root
     was created.  */
  int thread_id = 0;

  /* If true, the -var-update always recomputes the value in the
     current thread and frame.  Otherwise, variable object is
     always updated in the specific scope/thread/frame.  */
  bool floating = false;

  /* Flag that indicates validity: set to false when this varobj_root refers
     to symbols that do not exist anymore.  */
  bool is_valid = true;

  /* Set to true if the varobj was created as tracking a global.  */
  bool global = false;

  /* Language-related operations for this variable and its
     children.  */
  const struct lang_varobj_ops *lang_ops = NULL;

  /* The varobj for this root node.  */
  struct varobj *rootvar = NULL;
};

/* Dynamic part of varobj.  */

struct varobj_dynamic
{
  /* Whether the children of this varobj were requested.  This field is
     used to decide if dynamic varobj should recompute their children.
     In the event that the frontend never asked for the children, we
     can avoid that.  */
  bool children_requested = false;

  /* The pretty-printer constructor.  If NULL, then the default
     pretty-printer will be looked up.  If None, then no
     pretty-printer will be installed.  */
  PyObject *constructor = NULL;

  /* The pretty-printer that has been constructed.  If NULL, then a
     new printer object is needed, and one will be constructed.  */
  PyObject *pretty_printer = NULL;

  /* The iterator returned by the printer's 'children' method, or NULL
     if not available.  */
  std::unique_ptr<varobj_iter> child_iter;

  /* We request one extra item from the iterator, so that we can
     report to the caller whether there are more items than we have
     already reported.  However, we don't want to install this value
     when we read it, because that will mess up future updates.  So,
     we stash it here instead.  */
  std::unique_ptr<varobj_item> saved_item;
};

/* Private function prototypes */

/* Helper functions for the above subcommands.  */

static int delete_variable (struct varobj *, bool);

static void delete_variable_1 (int *, struct varobj *, bool, bool);

static void install_variable (struct varobj *);

static void uninstall_variable (struct varobj *);

static struct varobj *create_child (struct varobj *, int, std::string &);

static struct varobj *
create_child_with_value (struct varobj *parent, int index,
			 struct varobj_item *item);

/* Utility routines */

static bool update_type_if_necessary (struct varobj *var,
				      struct value *new_value);

static bool install_new_value (struct varobj *var, struct value *value,
			       bool initial);

/* Language-specific routines.  */

static int number_of_children (const struct varobj *);

static std::string name_of_variable (const struct varobj *);

static std::string name_of_child (struct varobj *, int);

static struct value *value_of_root (struct varobj **var_handle, bool *);

static struct value *value_of_child (const struct varobj *parent, int index);

static std::string my_value_of_variable (struct varobj *var,
					 enum varobj_display_formats format);

static bool is_root_p (const struct varobj *var);

static struct varobj *varobj_add_child (struct varobj *var,
					struct varobj_item *item);

/* Private data */

/* Mappings of varobj_display_formats enums to gdb's format codes.  */
static int format_code[] = { 0, 't', 'd', 'x', 'o', 'z' };

/* List of root variable objects.  */
static std::list<struct varobj_root *> rootlist;

/* Pointer to the varobj hash table (built at run time).  */
static htab_t varobj_table;



/* API Implementation */
static bool
is_root_p (const struct varobj *var)
{
  return (var->root->rootvar == var);
}

#ifdef HAVE_PYTHON

/* See python-internal.h.  */
gdbpy_enter_varobj::gdbpy_enter_varobj (const struct varobj *var)
: gdbpy_enter (var->root->gdbarch, var->root->language_defn)
{
}

#endif

/* Return the full FRAME which corresponds to the given CORE_ADDR
   or NULL if no FRAME on the chain corresponds to CORE_ADDR.  */

static frame_info_ptr
find_frame_addr_in_frame_chain (CORE_ADDR frame_addr)
{
  frame_info_ptr frame = NULL;

  if (frame_addr == (CORE_ADDR) 0)
    return NULL;

  for (frame = get_current_frame ();
       frame != NULL;
       frame = get_prev_frame (frame))
    {
      /* The CORE_ADDR we get as argument was parsed from a string GDB
	 output as $fp.  This output got truncated to gdbarch_addr_bit.
	 Truncate the frame base address in the same manner before
	 comparing it against our argument.  */
      CORE_ADDR frame_base = get_frame_base_address (frame);
      int addr_bit = gdbarch_addr_bit (get_frame_arch (frame));

      if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
	frame_base &= ((CORE_ADDR) 1 << addr_bit) - 1;

      if (frame_base == frame_addr)
	return frame;
    }

  return NULL;
}

/* Creates a varobj (not its children).  */

struct varobj *
varobj_create (const char *objname,
	       const char *expression, CORE_ADDR frame, enum varobj_type type)
{
  /* Fill out a varobj structure for the (root) variable being constructed.  */
  auto var = std::make_unique<varobj> (new varobj_root);

  if (expression != NULL)
    {
      frame_info_ptr fi;
      struct frame_id old_id = null_frame_id;
      const struct block *block;
      const char *p;
      struct value *value = NULL;
      CORE_ADDR pc;

      /* Parse and evaluate the expression, filling in as much of the
	 variable's data as possible.  */

      if (has_stack_frames ())
	{
	  /* Allow creator to specify context of variable.  */
	  if ((type == USE_CURRENT_FRAME) || (type == USE_SELECTED_FRAME))
	    fi = get_selected_frame (NULL);
	  else
	    /* FIXME: cagney/2002-11-23: This code should be doing a
	       lookup using the frame ID and not just the frame's
	       ``address''.  This, of course, means an interface
	       change.  However, with out that interface change ISAs,
	       such as the ia64 with its two stacks, won't work.
	       Similar goes for the case where there is a frameless
	       function.  */
	    fi = find_frame_addr_in_frame_chain (frame);
	}
      else
	fi = NULL;

      if (type == USE_SELECTED_FRAME)
	var->root->floating = true;

      pc = 0;
      block = NULL;
      if (fi != NULL)
	{
	  block = get_frame_block (fi, 0);
	  pc = get_frame_pc (fi);
	}

      p = expression;

      innermost_block_tracker tracker (INNERMOST_BLOCK_FOR_SYMBOLS
				       | INNERMOST_BLOCK_FOR_REGISTERS);
      /* Wrap the call to parse expression, so we can 
	 return a sensible error.  */
      try
	{
	  var->root->exp = parse_exp_1 (&p, pc, block, 0, &tracker);

	  /* Cache gdbarch and language_defn as they might be used even
	     after var is invalidated and var->root->exp cleared.  */
	  var->root->gdbarch = var->root->exp->gdbarch;
	  var->root->language_defn = var->root->exp->language_defn;
	}

      catch (const gdb_exception_error &except)
	{
	  return NULL;
	}

      /* Don't allow variables to be created for types.  */
      enum exp_opcode opcode = var->root->exp->first_opcode ();
      if (opcode == OP_TYPE
	  || opcode == OP_TYPEOF
	  || opcode == OP_DECLTYPE)
	{
	  gdb_printf (gdb_stderr, "Attempt to use a type name"
		      " as an expression.\n");
	  return NULL;
	}

      var->format = FORMAT_NATURAL;
      var->root->valid_block =
	var->root->floating ? NULL : tracker.block ();
      var->root->global
	= var->root->floating ? false : var->root->valid_block == nullptr;
      var->name = expression;
      /* For a root var, the name and the expr are the same.  */
      var->path_expr = expression;

      /* When the frame is different from the current frame, 
	 we must select the appropriate frame before parsing
	 the expression, otherwise the value will not be current.
	 Since select_frame is so benign, just call it for all cases.  */
      if (var->root->valid_block)
	{
	  /* User could specify explicit FRAME-ADDR which was not found but
	     EXPRESSION is frame specific and we would not be able to evaluate
	     it correctly next time.  With VALID_BLOCK set we must also set
	     FRAME and THREAD_ID.  */
	  if (fi == NULL)
	    error (_("Failed to find the specified frame"));

	  var->root->frame = get_frame_id (fi);
	  var->root->thread_id = inferior_thread ()->global_num;
	  old_id = get_frame_id (get_selected_frame (NULL));
	  select_frame (fi);	 
	}

      /* We definitely need to catch errors here.  If evaluation of
	 the expression succeeds, we got the value we wanted.  But if
	 it fails, we still go on with a call to evaluate_type().  */
      try
	{
	  value = var->root->exp->evaluate ();
	}
      catch (const gdb_exception_error &except)
	{
	  /* Error getting the value.  Try to at least get the
	     right type.  */
	  struct value *type_only_value = var->root->exp->evaluate_type ();

	  var->type = type_only_value->type ();
	}

      if (value != NULL)
	{
	  int real_type_found = 0;

	  var->type = value_actual_type (value, 0, &real_type_found);
	  if (real_type_found)
	    value = value_cast (var->type, value);
	}

      /* Set language info */
      var->root->lang_ops = var->root->exp->language_defn->varobj_ops ();

      install_new_value (var.get (), value, 1 /* Initial assignment */);

      /* Set ourselves as our root.  */
      var->root->rootvar = var.get ();

      /* Reset the selected frame.  */
      if (frame_id_p (old_id))
	select_frame (frame_find_by_id (old_id));
    }

  /* If the variable object name is null, that means this
     is a temporary variable, so don't install it.  */

  if ((var != NULL) && (objname != NULL))
    {
      var->obj_name = objname;
      install_variable (var.get ());
    }

  return var.release ();
}

/* Generates an unique name that can be used for a varobj.  */

std::string
varobj_gen_name (void)
{
  static int id = 0;

  /* Generate a name for this object.  */
  id++;
  return string_printf ("var%d", id);
}

/* Given an OBJNAME, returns the pointer to the corresponding varobj.  Call
   error if OBJNAME cannot be found.  */

struct varobj *
varobj_get_handle (const char *objname)
{
  varobj *var = (varobj *) htab_find_with_hash (varobj_table, objname,
						htab_hash_string (objname));

  if (var == NULL)
    error (_("Variable object not found"));

  return var;
}

/* Given the handle, return the name of the object.  */

const char *
varobj_get_objname (const struct varobj *var)
{
  return var->obj_name.c_str ();
}

/* Given the handle, return the expression represented by the
   object.  */

std::string
varobj_get_expression (const struct varobj *var)
{
  return name_of_variable (var);
}

/* See varobj.h.  */

int
varobj_delete (struct varobj *var, bool only_children)
{
  return delete_variable (var, only_children);
}

#if HAVE_PYTHON

/* Convenience function for varobj_set_visualizer.  Instantiate a
   pretty-printer for a given value.  */
static PyObject *
instantiate_pretty_printer (PyObject *constructor, struct value *value)
{
  gdbpy_ref<> val_obj (value_to_value_object (value));
  if (val_obj == nullptr)
    return NULL;

  return PyObject_CallFunctionObjArgs (constructor, val_obj.get (), NULL);
}

#endif

/* Set/Get variable object display format.  */

enum varobj_display_formats
varobj_set_display_format (struct varobj *var,
			   enum varobj_display_formats format)
{
  var->format = format;

  if (varobj_value_is_changeable_p (var) 
      && var->value != nullptr && !var->value->lazy ())
    {
      var->print_value = varobj_value_get_print_value (var->value.get (),
						       var->format, var);
    }

  return var->format;
}

enum varobj_display_formats
varobj_get_display_format (const struct varobj *var)
{
  return var->format;
}

gdb::unique_xmalloc_ptr<char>
varobj_get_display_hint (const struct varobj *var)
{
  gdb::unique_xmalloc_ptr<char> result;

#if HAVE_PYTHON
  if (!gdb_python_initialized)
    return NULL;

  gdbpy_enter_varobj enter_py (var);

  if (var->dynamic->pretty_printer != NULL)
    result = gdbpy_get_display_hint (var->dynamic->pretty_printer);
#endif

  return result;
}

/* Return true if the varobj has items after TO, false otherwise.  */

bool
varobj_has_more (const struct varobj *var, int to)
{
  if (var->children.size () > to)
    return true;

  return ((to == -1 || var->children.size () == to)
	  && (var->dynamic->saved_item != NULL));
}

/* If the variable object is bound to a specific thread, that
   is its evaluation can always be done in context of a frame
   inside that thread, returns GDB id of the thread -- which
   is always positive.  Otherwise, returns -1.  */
int
varobj_get_thread_id (const struct varobj *var)
{
  if (var->root->valid_block && var->root->thread_id > 0)
    return var->root->thread_id;
  else
    return -1;
}

void
varobj_set_frozen (struct varobj *var, bool frozen)
{
  /* When a variable is unfrozen, we don't fetch its value.
     The 'not_fetched' flag remains set, so next -var-update
     won't complain.

     We don't fetch the value, because for structures the client
     should do -var-update anyway.  It would be bad to have different
     client-size logic for structure and other types.  */
  var->frozen = frozen;
}

bool
varobj_get_frozen (const struct varobj *var)
{
  return var->frozen;
}

/* A helper function that updates the contents of FROM and TO based on the
   size of the vector CHILDREN.  If the contents of either FROM or TO are
   negative the entire range is used.  */

void
varobj_restrict_range (const std::vector<varobj *> &children,
		       int *from, int *to)
{
  int len = children.size ();

  if (*from < 0 || *to < 0)
    {
      *from = 0;
      *to = len;
    }
  else
    {
      if (*from > len)
	*from = len;
      if (*to > len)
	*to = len;
      if (*from > *to)
	*from = *to;
    }
}

/* A helper for update_dynamic_varobj_children that installs a new
   child when needed.  */

static void
install_dynamic_child (struct varobj *var,
		       std::vector<varobj *> *changed,
		       std::vector<varobj *> *type_changed,
		       std::vector<varobj *> *newobj,
		       std::vector<varobj *> *unchanged,
		       bool *cchanged,
		       int index,
		       struct varobj_item *item)
{
  if (var->children.size () < index + 1)
    {
      /* There's no child yet.  */
      struct varobj *child = varobj_add_child (var, item);

      if (newobj != NULL)
	{
	  newobj->push_back (child);
	  *cchanged = true;
	}
    }
  else
    {
      varobj *existing = var->children[index];
      bool type_updated = update_type_if_necessary (existing,
						    item->value.get ());

      if (type_updated)
	{
	  if (type_changed != NULL)
	    type_changed->push_back (existing);
	}
      if (install_new_value (existing, item->value.get (), 0))
	{
	  if (!type_updated && changed != NULL)
	    changed->push_back (existing);
	}
      else if (!type_updated && unchanged != NULL)
	unchanged->push_back (existing);
    }
}

/* A factory for creating dynamic varobj's iterators.  Returns an
   iterator object suitable for iterating over VAR's children.  */

static std::unique_ptr<varobj_iter>
varobj_get_iterator (struct varobj *var)
{
#if HAVE_PYTHON
  if (var->dynamic->pretty_printer)
    {
      value_print_options opts;
      varobj_formatted_print_options (&opts, var->format);
      return py_varobj_get_iterator (var, var->dynamic->pretty_printer, &opts);
    }
#endif

  gdb_assert_not_reached ("requested an iterator from a non-dynamic varobj");
}

static bool
update_dynamic_varobj_children (struct varobj *var,
				std::vector<varobj *> *changed,
				std::vector<varobj *> *type_changed,
				std::vector<varobj *> *newobj,
				std::vector<varobj *> *unchanged,
				bool *cchanged,
				bool update_children,
				int from,
				int to)
{
  int i;

  *cchanged = false;

  if (update_children || var->dynamic->child_iter == NULL)
    {
      var->dynamic->child_iter = varobj_get_iterator (var);
      var->dynamic->saved_item.reset (nullptr);

      i = 0;

      if (var->dynamic->child_iter == NULL)
	return false;
    }
  else
    i = var->children.size ();

  /* We ask for one extra child, so that MI can report whether there
     are more children.  */
  for (; to < 0 || i < to + 1; ++i)
    {
      std::unique_ptr<varobj_item> item;

      /* See if there was a leftover from last time.  */
      if (var->dynamic->saved_item != NULL)
	item = std::move (var->dynamic->saved_item);
      else
	item = var->dynamic->child_iter->next ();

      if (item == NULL)
	{
	  /* Iteration is done.  Remove iterator from VAR.  */
	  var->dynamic->child_iter.reset (nullptr);
	  break;
	}
      /* We don't want to push the extra child on any report list.  */
      if (to < 0 || i < to)
	{
	  bool can_mention = from < 0 || i >= from;

	  install_dynamic_child (var, can_mention ? changed : NULL,
				 can_mention ? type_changed : NULL,
				 can_mention ? newobj : NULL,
				 can_mention ? unchanged : NULL,
				 can_mention ? cchanged : NULL, i,
				 item.get ());
	}
      else
	{
	  var->dynamic->saved_item = std::move (item);

	  /* We want to truncate the child list just before this
	     element.  */
	  break;
	}
    }

  if (i < var->children.size ())
    {
      *cchanged = true;
      for (int j = i; j < var->children.size (); ++j)
	varobj_delete (var->children[j], 0);

      var->children.resize (i);
    }

  /* If there are fewer children than requested, note that the list of
     children changed.  */
  if (to >= 0 && var->children.size () < to)
    *cchanged = true;

  var->num_children = var->children.size ();

  return true;
}

int
varobj_get_num_children (struct varobj *var)
{
  if (var->num_children == -1)
    {
      if (varobj_is_dynamic_p (var))
	{
	  bool dummy;

	  /* If we have a dynamic varobj, don't report -1 children.
	     So, try to fetch some children first.  */
	  update_dynamic_varobj_children (var, NULL, NULL, NULL, NULL, &dummy,
					  false, 0, 0);
	}
      else
	var->num_children = number_of_children (var);
    }

  return var->num_children >= 0 ? var->num_children : 0;
}

/* Creates a list of the immediate children of a variable object;
   the return code is the number of such children or -1 on error.  */

const std::vector<varobj *> &
varobj_list_children (struct varobj *var, int *from, int *to)
{
  var->dynamic->children_requested = true;

  if (varobj_is_dynamic_p (var))
    {
      bool children_changed;

      /* This, in theory, can result in the number of children changing without
	 frontend noticing.  But well, calling -var-list-children on the same
	 varobj twice is not something a sane frontend would do.  */
      update_dynamic_varobj_children (var, NULL, NULL, NULL, NULL,
				      &children_changed, false, 0, *to);
      varobj_restrict_range (var->children, from, to);
      return var->children;
    }

  if (var->num_children == -1)
    var->num_children = number_of_children (var);

  /* If that failed, give up.  */
  if (var->num_children == -1)
    return var->children;

  /* If we're called when the list of children is not yet initialized,
     allocate enough elements in it.  */
  while (var->children.size () < var->num_children)
    var->children.push_back (NULL);

  for (int i = 0; i < var->num_children; i++)
    {
      if (var->children[i] == NULL)
	{
	  /* Either it's the first call to varobj_list_children for
	     this variable object, and the child was never created,
	     or it was explicitly deleted by the client.  */
	  std::string name = name_of_child (var, i);
	  var->children[i] = create_child (var, i, name);
	}
    }

  varobj_restrict_range (var->children, from, to);
  return var->children;
}

static struct varobj *
varobj_add_child (struct varobj *var, struct varobj_item *item)
{
  varobj *v = create_child_with_value (var, var->children.size (), item);

  var->children.push_back (v);

  return v;
}

/* Obtain the type of an object Variable as a string similar to the one gdb
   prints on the console.  The caller is responsible for freeing the string.
   */

std::string
varobj_get_type (struct varobj *var)
{
  /* For the "fake" variables, do not return a type.  (Its type is
     NULL, too.)
     Do not return a type for invalid variables as well.  */
  if (CPLUS_FAKE_CHILD (var) || !var->root->is_valid)
    return std::string ();

  return type_to_string (var->type);
}

/* Obtain the type of an object variable.  */

struct type *
varobj_get_gdb_type (const struct varobj *var)
{
  return var->type;
}

/* Is VAR a path expression parent, i.e., can it be used to construct
   a valid path expression?  */

static bool
is_path_expr_parent (const struct varobj *var)
{
  gdb_assert (var->root->lang_ops->is_path_expr_parent != NULL);
  return var->root->lang_ops->is_path_expr_parent (var);
}

/* Is VAR a path expression parent, i.e., can it be used to construct
   a valid path expression?  By default we assume any VAR can be a path
   parent.  */

bool
varobj_default_is_path_expr_parent (const struct varobj *var)
{
  return true;
}

/* Return the path expression parent for VAR.  */

const struct varobj *
varobj_get_path_expr_parent (const struct varobj *var)
{
  const struct varobj *parent = var;

  while (!is_root_p (parent) && !is_path_expr_parent (parent))
    parent = parent->parent;

  /* Computation of full rooted expression for children of dynamic
     varobjs is not supported.  */
  if (varobj_is_dynamic_p (parent))
    error (_("Invalid variable object (child of a dynamic varobj)"));

  return parent;
}

/* Return a pointer to the full rooted expression of varobj VAR.
   If it has not been computed yet, compute it.  */

const char *
varobj_get_path_expr (const struct varobj *var)
{
  if (var->path_expr.empty ())
    {
      /* For root varobjs, we initialize path_expr
	 when creating varobj, so here it should be
	 child varobj.  */
      struct varobj *mutable_var = (struct varobj *) var;
      gdb_assert (!is_root_p (var));

      mutable_var->path_expr = (*var->root->lang_ops->path_expr_of_child) (var);
    }

  return var->path_expr.c_str ();
}

const struct language_defn *
varobj_get_language (const struct varobj *var)
{
  return var->root->exp->language_defn;
}

int
varobj_get_attributes (const struct varobj *var)
{
  int attributes = 0;

  if (varobj_editable_p (var))
    /* FIXME: define masks for attributes.  */
    attributes |= 0x00000001;	/* Editable */

  return attributes;
}

/* Return true if VAR is a dynamic varobj.  */

bool
varobj_is_dynamic_p (const struct varobj *var)
{
  return var->dynamic->pretty_printer != NULL;
}

std::string
varobj_get_formatted_value (struct varobj *var,
			    enum varobj_display_formats format)
{
  return my_value_of_variable (var, format);
}

std::string
varobj_get_value (struct varobj *var)
{
  return my_value_of_variable (var, var->format);
}

/* Set the value of an object variable (if it is editable) to the
   value of the given expression.  */
/* Note: Invokes functions that can call error().  */

bool
varobj_set_value (struct varobj *var, const char *expression)
{
  struct value *val = NULL; /* Initialize to keep gcc happy.  */
  /* The argument "expression" contains the variable's new value.
     We need to first construct a legal expression for this -- ugh!  */
  /* Does this cover all the bases?  */
  struct value *value = NULL; /* Initialize to keep gcc happy.  */
  const char *s = expression;

  gdb_assert (varobj_editable_p (var));

  /* ALWAYS reset to decimal temporarily.  */
  auto save_input_radix = make_scoped_restore (&input_radix, 10);
  expression_up exp = parse_exp_1 (&s, 0, 0, 0);
  try
    {
      value = exp->evaluate ();
    }

  catch (const gdb_exception_error &except)
    {
      /* We cannot proceed without a valid expression.  */
      return false;
    }

  /* All types that are editable must also be changeable.  */
  gdb_assert (varobj_value_is_changeable_p (var));

  /* The value of a changeable variable object must not be lazy.  */
  gdb_assert (!var->value->lazy ());

  /* Need to coerce the input.  We want to check if the
     value of the variable object will be different
     after assignment, and the first thing value_assign
     does is coerce the input.
     For example, if we are assigning an array to a pointer variable we
     should compare the pointer with the array's address, not with the
     array's content.  */
  value = coerce_array (value);

  /* The new value may be lazy.  value_assign, or
     rather value_contents, will take care of this.  */
  try
    {
      val = value_assign (var->value.get (), value);
    }

  catch (const gdb_exception_error &except)
    {
      return false;
    }

  /* If the value has changed, record it, so that next -var-update can
     report this change.  If a variable had a value of '1', we've set it
     to '333' and then set again to '1', when -var-update will report this
     variable as changed -- because the first assignment has set the
     'updated' flag.  There's no need to optimize that, because return value
     of -var-update should be considered an approximation.  */
  var->updated = install_new_value (var, val, false /* Compare values.  */);
  return true;
}

#if HAVE_PYTHON

/* A helper function to install a constructor function and visualizer
   in a varobj_dynamic.  */

static void
install_visualizer (struct varobj_dynamic *var, PyObject *constructor,
		    PyObject *visualizer)
{
  Py_XDECREF (var->constructor);
  var->constructor = constructor;

  Py_XDECREF (var->pretty_printer);
  var->pretty_printer = visualizer;

  var->child_iter.reset (nullptr);
}

/* Install the default visualizer for VAR.  */

static void
install_default_visualizer (struct varobj *var)
{
  /* Do not install a visualizer on a CPLUS_FAKE_CHILD.  */
  if (CPLUS_FAKE_CHILD (var))
    return;

  if (pretty_printing)
    {
      gdbpy_ref<> pretty_printer;

      if (var->value != nullptr)
	{
	  pretty_printer = gdbpy_get_varobj_pretty_printer (var->value.get ());
	  if (pretty_printer == nullptr)
	    {
	      gdbpy_print_stack ();
	      error (_("Cannot instantiate printer for default visualizer"));
	    }
	}

      if (pretty_printer == Py_None)
	pretty_printer.reset (nullptr);
  
      install_visualizer (var->dynamic, NULL, pretty_printer.release ());
    }
}

/* Instantiate and install a visualizer for VAR using CONSTRUCTOR to
   make a new object.  */

static void
construct_visualizer (struct varobj *var, PyObject *constructor)
{
  PyObject *pretty_printer;

  /* Do not install a visualizer on a CPLUS_FAKE_CHILD.  */
  if (CPLUS_FAKE_CHILD (var))
    return;

  Py_INCREF (constructor);
  if (constructor == Py_None)
    pretty_printer = NULL;
  else
    {
      pretty_printer = instantiate_pretty_printer (constructor,
						   var->value.get ());
      if (! pretty_printer)
	{
	  gdbpy_print_stack ();
	  Py_DECREF (constructor);
	  constructor = Py_None;
	  Py_INCREF (constructor);
	}

      if (pretty_printer == Py_None)
	{
	  Py_DECREF (pretty_printer);
	  pretty_printer = NULL;
	}
    }

  install_visualizer (var->dynamic, constructor, pretty_printer);
}

#endif /* HAVE_PYTHON */

/* A helper function for install_new_value.  This creates and installs
   a visualizer for VAR, if appropriate.  */

static void
install_new_value_visualizer (struct varobj *var)
{
#if HAVE_PYTHON
  /* If the constructor is None, then we want the raw value.  If VAR
     does not have a value, just skip this.  */
  if (!gdb_python_initialized)
    return;

  if (var->dynamic->constructor != Py_None && var->value != NULL)
    {
      gdbpy_enter_varobj enter_py (var);

      if (var->dynamic->constructor == NULL)
	install_default_visualizer (var);
      else
	construct_visualizer (var, var->dynamic->constructor);
    }
#else
  /* Do nothing.  */
#endif
}

/* When using RTTI to determine variable type it may be changed in runtime when
   the variable value is changed.  This function checks whether type of varobj
   VAR will change when a new value NEW_VALUE is assigned and if it is so
   updates the type of VAR.  */

static bool
update_type_if_necessary (struct varobj *var, struct value *new_value)
{
  if (new_value)
    {
      struct value_print_options opts;

      get_user_print_options (&opts);
      if (opts.objectprint)
	{
	  struct type *new_type = value_actual_type (new_value, 0, 0);
	  std::string new_type_str = type_to_string (new_type);
	  std::string curr_type_str = varobj_get_type (var);

	  /* Did the type name change?  */
	  if (curr_type_str != new_type_str)
	    {
	      var->type = new_type;

	      /* This information may be not valid for a new type.  */
	      varobj_delete (var, 1);
	      var->children.clear ();
	      var->num_children = -1;
	      return true;
	    }
	}
    }

  return false;
}

/* Assign a new value to a variable object.  If INITIAL is true,
   this is the first assignment after the variable object was just
   created, or changed type.  In that case, just assign the value 
   and return false.
   Otherwise, assign the new value, and return true if the value is
   different from the current one, false otherwise.  The comparison is
   done on textual representation of value.  Therefore, some types
   need not be compared.  E.g.  for structures the reported value is
   always "{...}", so no comparison is necessary here.  If the old
   value was NULL and new one is not, or vice versa, we always return true.

   The VALUE parameter should not be released -- the function will
   take care of releasing it when needed.  */
static bool
install_new_value (struct varobj *var, struct value *value, bool initial)
{ 
  bool changeable;
  bool need_to_fetch;
  bool changed = false;
  bool intentionally_not_fetched = false;

  /* We need to know the varobj's type to decide if the value should
     be fetched or not.  C++ fake children (public/protected/private)
     don't have a type.  */
  gdb_assert (var->type || CPLUS_FAKE_CHILD (var));
  changeable = varobj_value_is_changeable_p (var);

  /* If the type has custom visualizer, we consider it to be always
     changeable.  FIXME: need to make sure this behaviour will not
     mess up read-sensitive values.  */
  if (var->dynamic->pretty_printer != NULL)
    changeable = true;

  need_to_fetch = changeable;

  /* We are not interested in the address of references, and given
     that in C++ a reference is not rebindable, it cannot
     meaningfully change.  So, get hold of the real value.  */
  if (value)
    value = coerce_ref (value);

  if (var->type && var->type->code () == TYPE_CODE_UNION)
    /* For unions, we need to fetch the value implicitly because
       of implementation of union member fetch.  When gdb
       creates a value for a field and the value of the enclosing
       structure is not lazy,  it immediately copies the necessary
       bytes from the enclosing values.  If the enclosing value is
       lazy, the call to value_fetch_lazy on the field will read
       the data from memory.  For unions, that means we'll read the
       same memory more than once, which is not desirable.  So
       fetch now.  */
    need_to_fetch = true;

  /* The new value might be lazy.  If the type is changeable,
     that is we'll be comparing values of this type, fetch the
     value now.  Otherwise, on the next update the old value
     will be lazy, which means we've lost that old value.  */
  if (need_to_fetch && value && value->lazy ())
    {
      const struct varobj *parent = var->parent;
      bool frozen = var->frozen;

      for (; !frozen && parent; parent = parent->parent)
	frozen |= parent->frozen;

      if (frozen && initial)
	{
	  /* For variables that are frozen, or are children of frozen
	     variables, we don't do fetch on initial assignment.
	     For non-initial assignment we do the fetch, since it means we're
	     explicitly asked to compare the new value with the old one.  */
	  intentionally_not_fetched = true;
	}
      else
	{

	  try
	    {
	      value->fetch_lazy ();
	    }

	  catch (const gdb_exception_error &except)
	    {
	      /* Set the value to NULL, so that for the next -var-update,
		 we don't try to compare the new value with this value,
		 that we couldn't even read.  */
	      value = NULL;
	    }
	}
    }

  /* Get a reference now, before possibly passing it to any Python
     code that might release it.  */
  value_ref_ptr value_holder;
  if (value != NULL)
    value_holder = value_ref_ptr::new_reference (value);

  /* Below, we'll be comparing string rendering of old and new
     values.  Don't get string rendering if the value is
     lazy -- if it is, the code above has decided that the value
     should not be fetched.  */
  std::string print_value;
  if (value != NULL && !value->lazy ()
      && var->dynamic->pretty_printer == NULL)
    print_value = varobj_value_get_print_value (value, var->format, var);

  /* If the type is changeable, compare the old and the new values.
     If this is the initial assignment, we don't have any old value
     to compare with.  */
  if (!initial && changeable)
    {
      /* If the value of the varobj was changed by -var-set-value,
	 then the value in the varobj and in the target is the same.
	 However, that value is different from the value that the
	 varobj had after the previous -var-update.  So need to the
	 varobj as changed.  */
      if (var->updated)
	changed = true;
      else if (var->dynamic->pretty_printer == NULL)
	{
	  /* Try to compare the values.  That requires that both
	     values are non-lazy.  */
	  if (var->not_fetched && var->value->lazy ())
	    {
	      /* This is a frozen varobj and the value was never read.
		 Presumably, UI shows some "never read" indicator.
		 Now that we've fetched the real value, we need to report
		 this varobj as changed so that UI can show the real
		 value.  */
	      changed = true;
	    }
	  else  if (var->value == NULL && value == NULL)
	    /* Equal.  */
	    ;
	  else if (var->value == NULL || value == NULL)
	    {
	      changed = true;
	    }
	  else
	    {
	      gdb_assert (!var->value->lazy ());
	      gdb_assert (!value->lazy ());

	      gdb_assert (!var->print_value.empty () && !print_value.empty ());
	      if (var->print_value != print_value)
		changed = true;
	    }
	}
    }

  if (!initial && !changeable)
    {
      /* For values that are not changeable, we don't compare the values.
	 However, we want to notice if a value was not NULL and now is NULL,
	 or vise versa, so that we report when top-level varobjs come in scope
	 and leave the scope.  */
      changed = (var->value != NULL) != (value != NULL);
    }

  /* We must always keep the new value, since children depend on it.  */
  var->value = value_holder;
  if (value && value->lazy () && intentionally_not_fetched)
    var->not_fetched = true;
  else
    var->not_fetched = false;
  var->updated = false;

  install_new_value_visualizer (var);

  /* If we installed a pretty-printer, re-compare the printed version
     to see if the variable changed.  */
  if (var->dynamic->pretty_printer != NULL)
    {
      print_value = varobj_value_get_print_value (var->value.get (),
						  var->format, var);
      if (var->print_value != print_value)
	changed = true;
    }
  var->print_value = print_value;

  gdb_assert (var->value == nullptr || var->value->type ());

  return changed;
}

/* Return the requested range for a varobj.  VAR is the varobj.  FROM
   and TO are out parameters; *FROM and *TO will be set to the
   selected sub-range of VAR.  If no range was selected using
   -var-set-update-range, then both will be -1.  */
void
varobj_get_child_range (const struct varobj *var, int *from, int *to)
{
  *from = var->from;
  *to = var->to;
}

/* Set the selected sub-range of children of VAR to start at index
   FROM and end at index TO.  If either FROM or TO is less than zero,
   this is interpreted as a request for all children.  */
void
varobj_set_child_range (struct varobj *var, int from, int to)
{
  var->from = from;
  var->to = to;
}

void 
varobj_set_visualizer (struct varobj *var, const char *visualizer)
{
#if HAVE_PYTHON
  PyObject *mainmod;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter_varobj enter_py (var);

  mainmod = PyImport_AddModule ("__main__");
  gdbpy_ref<> globals
    = gdbpy_ref<>::new_reference (PyModule_GetDict (mainmod));
  gdbpy_ref<> constructor (PyRun_String (visualizer, Py_eval_input,
					 globals.get (), globals.get ()));

  if (constructor == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not evaluate visualizer expression: %s"), visualizer);
    }

  construct_visualizer (var, constructor.get ());

  /* If there are any children now, wipe them.  */
  varobj_delete (var, 1 /* children only */);
  var->num_children = -1;

  /* Also be sure to reset the print value.  */
  varobj_set_display_format (var, var->format);
#else
  error (_("Python support required"));
#endif
}

/* If NEW_VALUE is the new value of the given varobj (var), return
   true if var has mutated.  In other words, if the type of
   the new value is different from the type of the varobj's old
   value.

   NEW_VALUE may be NULL, if the varobj is now out of scope.  */

static bool
varobj_value_has_mutated (const struct varobj *var, struct value *new_value,
			  struct type *new_type)
{
  /* If we haven't previously computed the number of children in var,
     it does not matter from the front-end's perspective whether
     the type has mutated or not.  For all intents and purposes,
     it has not mutated.  */
  if (var->num_children < 0)
    return false;

  if (var->root->lang_ops->value_has_mutated != NULL)
    {
      /* The varobj module, when installing new values, explicitly strips
	 references, saying that we're not interested in those addresses.
	 But detection of mutation happens before installing the new
	 value, so our value may be a reference that we need to strip
	 in order to remain consistent.  */
      if (new_value != NULL)
	new_value = coerce_ref (new_value);
      return var->root->lang_ops->value_has_mutated (var, new_value, new_type);
    }
  else
    return false;
}

/* Update the values for a variable and its children.  This is a
   two-pronged attack.  First, re-parse the value for the root's
   expression to see if it's changed.  Then go all the way
   through its children, reconstructing them and noting if they've
   changed.

   The IS_EXPLICIT parameter specifies if this call is result
   of MI request to update this specific variable, or 
   result of implicit -var-update *.  For implicit request, we don't
   update frozen variables.

   NOTE: This function may delete the caller's varobj.  If it
   returns TYPE_CHANGED, then it has done this and VARP will be modified
   to point to the new varobj.  */

std::vector<varobj_update_result>
varobj_update (struct varobj **varp, bool is_explicit)
{
  bool type_changed = false;
  struct value *newobj;
  std::vector<varobj_update_result> stack;
  std::vector<varobj_update_result> result;

  /* Frozen means frozen -- we don't check for any change in
     this varobj, including its going out of scope, or
     changing type.  One use case for frozen varobjs is
     retaining previously evaluated expressions, and we don't
     want them to be reevaluated at all.  */
  if (!is_explicit && (*varp)->frozen)
    return result;

  if (!(*varp)->root->is_valid)
    {
      result.emplace_back (*varp, VAROBJ_INVALID);
      return result;
    }

  if ((*varp)->root->rootvar == *varp)
    {
      varobj_update_result r (*varp);

      /* Update the root variable.  value_of_root can return NULL
	 if the variable is no longer around, i.e. we stepped out of
	 the frame in which a local existed.  We are letting the 
	 value_of_root variable dispose of the varobj if the type
	 has changed.  */
      newobj = value_of_root (varp, &type_changed);
      if (update_type_if_necessary (*varp, newobj))
	  type_changed = true;
      r.varobj = *varp;
      r.type_changed = type_changed;
      if (install_new_value ((*varp), newobj, type_changed))
	r.changed = true;
      
      if (newobj == NULL)
	r.status = VAROBJ_NOT_IN_SCOPE;
      r.value_installed = true;

      if (r.status == VAROBJ_NOT_IN_SCOPE)
	{
	  if (r.type_changed || r.changed)
	    result.push_back (std::move (r));

	  return result;
	}

      stack.push_back (std::move (r));
    }
  else
    stack.emplace_back (*varp);

  /* Walk through the children, reconstructing them all.  */
  while (!stack.empty ())
    {
      varobj_update_result r = std::move (stack.back ());
      stack.pop_back ();
      struct varobj *v = r.varobj;

      /* Update this variable, unless it's a root, which is already
	 updated.  */
      if (!r.value_installed)
	{
	  struct type *new_type;

	  newobj = value_of_child (v->parent, v->index);
	  if (update_type_if_necessary (v, newobj))
	    r.type_changed = true;
	  if (newobj)
	    new_type = newobj->type ();
	  else
	    new_type = v->root->lang_ops->type_of_child (v->parent, v->index);

	  if (varobj_value_has_mutated (v, newobj, new_type))
	    {
	      /* The children are no longer valid; delete them now.
		 Report the fact that its type changed as well.  */
	      varobj_delete (v, 1 /* only_children */);
	      v->num_children = -1;
	      v->to = -1;
	      v->from = -1;
	      v->type = new_type;
	      r.type_changed = true;
	    }

	  if (install_new_value (v, newobj, r.type_changed))
	    {
	      r.changed = true;
	      v->updated = false;
	    }
	}

      /* We probably should not get children of a dynamic varobj, but
	 for which -var-list-children was never invoked.  */
      if (varobj_is_dynamic_p (v))
	{
	  std::vector<varobj *> changed, type_changed_vec, unchanged, newobj_vec;
	  bool children_changed = false;

	  if (v->frozen)
	    continue;

	  if (!v->dynamic->children_requested)
	    {
	      bool dummy;

	      /* If we initially did not have potential children, but
		 now we do, consider the varobj as changed.
		 Otherwise, if children were never requested, consider
		 it as unchanged -- presumably, such varobj is not yet
		 expanded in the UI, so we need not bother getting
		 it.  */
	      if (!varobj_has_more (v, 0))
		{
		  update_dynamic_varobj_children (v, NULL, NULL, NULL, NULL,
						  &dummy, false, 0, 0);
		  if (varobj_has_more (v, 0))
		    r.changed = true;
		}

	      if (r.changed)
		result.push_back (std::move (r));

	      continue;
	    }

	  /* If update_dynamic_varobj_children returns false, then we have
	     a non-conforming pretty-printer, so we skip it.  */
	  if (update_dynamic_varobj_children (v, &changed, &type_changed_vec,
					      &newobj_vec,
					      &unchanged, &children_changed,
					      true, v->from, v->to))
	    {
	      if (children_changed || !newobj_vec.empty ())
		{
		  r.children_changed = true;
		  r.newobj = std::move (newobj_vec);
		}
	      /* Push in reverse order so that the first child is
		 popped from the work stack first, and so will be
		 added to result first.  This does not affect
		 correctness, just "nicer".  */
	      for (int i = type_changed_vec.size () - 1; i >= 0; --i)
		{
		  varobj_update_result item (type_changed_vec[i]);

		  /* Type may change only if value was changed.  */
		  item.changed = true;
		  item.type_changed = true;
		  item.value_installed = true;

		  stack.push_back (std::move (item));
		}
	      for (int i = changed.size () - 1; i >= 0; --i)
		{
		  varobj_update_result item (changed[i]);

		  item.changed = true;
		  item.value_installed = true;

		  stack.push_back (std::move (item));
		}
	      for (int i = unchanged.size () - 1; i >= 0; --i)
		{
		  if (!unchanged[i]->frozen)
		    {
		      varobj_update_result item (unchanged[i]);

		      item.value_installed = true;

		      stack.push_back (std::move (item));
		    }
		}
	      if (r.changed || r.children_changed)
		result.push_back (std::move (r));

	      continue;
	    }
	}

      /* Push any children.  Use reverse order so that the first
	 child is popped from the work stack first, and so
	 will be added to result first.  This does not
	 affect correctness, just "nicer".  */
      for (int i = v->children.size () - 1; i >= 0; --i)
	{
	  varobj *c = v->children[i];

	  /* Child may be NULL if explicitly deleted by -var-delete.  */
	  if (c != NULL && !c->frozen)
	    stack.emplace_back (c);
	}

      if (r.changed || r.type_changed)
	result.push_back (std::move (r));
    }

  return result;
}

/* Helper functions */

/*
 * Variable object construction/destruction
 */

static int
delete_variable (struct varobj *var, bool only_children_p)
{
  int delcount = 0;

  delete_variable_1 (&delcount, var, only_children_p,
		     true /* remove_from_parent_p */ );

  return delcount;
}

/* Delete the variable object VAR and its children.  */
/* IMPORTANT NOTE: If we delete a variable which is a child
   and the parent is not removed we dump core.  It must be always
   initially called with remove_from_parent_p set.  */
static void
delete_variable_1 (int *delcountp, struct varobj *var, bool only_children_p,
		   bool remove_from_parent_p)
{
  /* Delete any children of this variable, too.  */
  for (varobj *child : var->children)
    {   
      if (!child)
	continue;

      if (!remove_from_parent_p)
	child->parent = NULL;

      delete_variable_1 (delcountp, child, false, only_children_p);
    }
  var->children.clear ();

  /* if we were called to delete only the children we are done here.  */
  if (only_children_p)
    return;

  /* Otherwise, add it to the list of deleted ones and proceed to do so.  */
  /* If the name is empty, this is a temporary variable, that has not
     yet been installed, don't report it, it belongs to the caller...  */
  if (!var->obj_name.empty ())
    {
      *delcountp = *delcountp + 1;
    }

  /* If this variable has a parent, remove it from its parent's list.  */
  /* OPTIMIZATION: if the parent of this variable is also being deleted, 
     (as indicated by remove_from_parent_p) we don't bother doing an
     expensive list search to find the element to remove when we are
     discarding the list afterwards.  */
  if ((remove_from_parent_p) && (var->parent != NULL))
    var->parent->children[var->index] = NULL;

  if (!var->obj_name.empty ())
    uninstall_variable (var);

  /* Free memory associated with this variable.  */
  delete var;
}

/* Install the given variable VAR with the object name VAR->OBJ_NAME.  */
static void
install_variable (struct varobj *var)
{
  hashval_t hash = htab_hash_string (var->obj_name.c_str ());
  void **slot = htab_find_slot_with_hash (varobj_table,
					  var->obj_name.c_str (),
					  hash, INSERT);
  if (*slot != nullptr)
    error (_("Duplicate variable object name"));

  /* Add varobj to hash table.  */
  *slot = var;

  /* If root, add varobj to root list.  */
  if (is_root_p (var))
    rootlist.push_front (var->root);
}

/* Uninstall the object VAR.  */
static void
uninstall_variable (struct varobj *var)
{
  hashval_t hash = htab_hash_string (var->obj_name.c_str ());
  htab_remove_elt_with_hash (varobj_table, var->obj_name.c_str (), hash);

  if (varobjdebug)
    gdb_printf (gdb_stdlog, "Deleting %s\n", var->obj_name.c_str ());

  /* If root, remove varobj from root list.  */
  if (is_root_p (var))
    {
      auto iter = std::find (rootlist.begin (), rootlist.end (), var->root);
      rootlist.erase (iter);
    }
}

/* Create and install a child of the parent of the given name.

   The created VAROBJ takes ownership of the allocated NAME.  */

static struct varobj *
create_child (struct varobj *parent, int index, std::string &name)
{
  struct varobj_item item;

  std::swap (item.name, name);
  item.value = release_value (value_of_child (parent, index));

  return create_child_with_value (parent, index, &item);
}

static struct varobj *
create_child_with_value (struct varobj *parent, int index,
			 struct varobj_item *item)
{
  varobj *child = new varobj (parent->root);

  /* NAME is allocated by caller.  */
  std::swap (child->name, item->name);
  child->index = index;
  child->parent = parent;

  if (varobj_is_anonymous_child (child))
    child->obj_name = string_printf ("%s.%d_anonymous",
				     parent->obj_name.c_str (), index);
  else
    child->obj_name = string_printf ("%s.%s",
				     parent->obj_name.c_str (),
				     child->name.c_str ());

  install_variable (child);

  /* Compute the type of the child.  Must do this before
     calling install_new_value.  */
  if (item->value != NULL)
    /* If the child had no evaluation errors, var->value
       will be non-NULL and contain a valid type.  */
    child->type = value_actual_type (item->value.get (), 0, NULL);
  else
    /* Otherwise, we must compute the type.  */
    child->type = (*child->root->lang_ops->type_of_child) (child->parent,
							   child->index);
  install_new_value (child, item->value.get (), 1);

  return child;
}


/*
 * Miscellaneous utility functions.
 */

/* Allocate memory and initialize a new variable.  */
varobj::varobj (varobj_root *root_)
: root (root_), dynamic (new varobj_dynamic)
{
}

/* Free any allocated memory associated with VAR.  */

varobj::~varobj ()
{
  varobj *var = this;

#if HAVE_PYTHON
  if (var->dynamic->pretty_printer != NULL)
    {
      gdbpy_enter_varobj enter_py (var);

      Py_XDECREF (var->dynamic->constructor);
      Py_XDECREF (var->dynamic->pretty_printer);
    }
#endif

  /* This must be deleted before the root object, because Python-based
     destructors need access to some components.  */
  delete var->dynamic;

  if (is_root_p (var))
    delete var->root;
}

/* Return the type of the value that's stored in VAR,
   or that would have being stored there if the
   value were accessible.

   This differs from VAR->type in that VAR->type is always
   the true type of the expression in the source language.
   The return value of this function is the type we're
   actually storing in varobj, and using for displaying
   the values and for comparing previous and new values.

   For example, top-level references are always stripped.  */
struct type *
varobj_get_value_type (const struct varobj *var)
{
  struct type *type;

  if (var->value != nullptr)
    type = var->value->type ();
  else
    type = var->type;

  type = check_typedef (type);

  if (TYPE_IS_REFERENCE (type))
    type = get_target_type (type);

  type = check_typedef (type);

  return type;
}

/*
 * Language-dependencies
 */

/* Common entry points */

/* Return the number of children for a given variable.
   The result of this function is defined by the language
   implementation.  The number of children returned by this function
   is the number of children that the user will see in the variable
   display.  */
static int
number_of_children (const struct varobj *var)
{
  return (*var->root->lang_ops->number_of_children) (var);
}

/* What is the expression for the root varobj VAR? */

static std::string
name_of_variable (const struct varobj *var)
{
  return (*var->root->lang_ops->name_of_variable) (var);
}

/* What is the name of the INDEX'th child of VAR?  */

static std::string
name_of_child (struct varobj *var, int index)
{
  return (*var->root->lang_ops->name_of_child) (var, index);
}

/* If frame associated with VAR can be found, switch
   to it and return true.  Otherwise, return false.  */

static bool
check_scope (const struct varobj *var)
{
  frame_info_ptr fi;
  bool scope;

  fi = frame_find_by_id (var->root->frame);
  scope = fi != NULL;

  if (fi)
    {
      CORE_ADDR pc = get_frame_pc (fi);

      if (pc <  var->root->valid_block->start () ||
	  pc >= var->root->valid_block->end ())
	scope = false;
      else
	select_frame (fi);
    }
  return scope;
}

/* Helper function to value_of_root.  */

static struct value *
value_of_root_1 (struct varobj **var_handle)
{
  struct value *new_val = NULL;
  struct varobj *var = *var_handle;
  bool within_scope = false;
								 
  /*  Only root variables can be updated...  */
  if (!is_root_p (var))
    /* Not a root var.  */
    return NULL;

  scoped_restore_current_thread restore_thread;

  /* Determine whether the variable is still around.  */
  if (var->root->valid_block == NULL || var->root->floating)
    within_scope = true;
  else if (var->root->thread_id == 0)
    {
      /* The program was single-threaded when the variable object was
	 created.  Technically, it's possible that the program became
	 multi-threaded since then, but we don't support such
	 scenario yet.  */
      within_scope = check_scope (var);	  
    }
  else
    {
      thread_info *thread = find_thread_global_id (var->root->thread_id);

      if (thread != NULL)
	{
	  switch_to_thread (thread);
	  within_scope = check_scope (var);
	}
    }

  if (within_scope)
    {

      /* We need to catch errors here, because if evaluate
	 expression fails we want to just return NULL.  */
      try
	{
	  new_val = var->root->exp->evaluate ();
	}
      catch (const gdb_exception_error &except)
	{
	}
    }

  return new_val;
}

/* What is the ``struct value *'' of the root variable VAR?
   For floating variable object, evaluation can get us a value
   of different type from what is stored in varobj already.  In
   that case:
   - *type_changed will be set to 1
   - old varobj will be freed, and new one will be
   created, with the same name.
   - *var_handle will be set to the new varobj 
   Otherwise, *type_changed will be set to 0.  */
static struct value *
value_of_root (struct varobj **var_handle, bool *type_changed)
{
  struct varobj *var;

  if (var_handle == NULL)
    return NULL;

  var = *var_handle;

  /* This should really be an exception, since this should
     only get called with a root variable.  */

  if (!is_root_p (var))
    return NULL;

  if (var->root->floating)
    {
      struct varobj *tmp_var;

      tmp_var = varobj_create (NULL, var->name.c_str (), (CORE_ADDR) 0,
			       USE_SELECTED_FRAME);
      if (tmp_var == NULL)
	{
	  return NULL;
	}
      std::string old_type = varobj_get_type (var);
      std::string new_type = varobj_get_type (tmp_var);
      if (old_type == new_type)
	{
	  /* The expression presently stored inside var->root->exp
	     remembers the locations of local variables relatively to
	     the frame where the expression was created (in DWARF location
	     button, for example).  Naturally, those locations are not
	     correct in other frames, so update the expression.  */

	  std::swap (var->root->exp, tmp_var->root->exp);

	  varobj_delete (tmp_var, 0);
	  *type_changed = 0;
	}
      else
	{
	  tmp_var->obj_name = var->obj_name;
	  tmp_var->from = var->from;
	  tmp_var->to = var->to;
	  varobj_delete (var, 0);

	  install_variable (tmp_var);
	  *var_handle = tmp_var;
	  var = *var_handle;
	  *type_changed = true;
	}
    }
  else
    {
      *type_changed = 0;
    }

  {
    struct value *value;

    value = value_of_root_1 (var_handle);
    if (var->value == NULL || value == NULL)
      {
	/* For root varobj-s, a NULL value indicates a scoping issue.
	   So, nothing to do in terms of checking for mutations.  */
      }
    else if (varobj_value_has_mutated (var, value, value->type ()))
      {
	/* The type has mutated, so the children are no longer valid.
	   Just delete them, and tell our caller that the type has
	   changed.  */
	varobj_delete (var, 1 /* only_children */);
	var->num_children = -1;
	var->to = -1;
	var->from = -1;
	*type_changed = true;
      }
    return value;
  }
}

/* What is the ``struct value *'' for the INDEX'th child of PARENT?  */
static struct value *
value_of_child (const struct varobj *parent, int index)
{
  struct value *value;

  value = (*parent->root->lang_ops->value_of_child) (parent, index);

  return value;
}

/* GDB already has a command called "value_of_variable".  Sigh.  */
static std::string
my_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  if (var->root->is_valid)
    {
      if (var->dynamic->pretty_printer != NULL)
	return varobj_value_get_print_value (var->value.get (), var->format,
					     var);
      else if (var->parent != nullptr && varobj_is_dynamic_p (var->parent))
	return var->print_value;

      return (*var->root->lang_ops->value_of_variable) (var, format);
    }
  else
    return std::string ();
}

void
varobj_formatted_print_options (struct value_print_options *opts,
				enum varobj_display_formats format)
{
  get_formatted_print_options (opts, format_code[(int) format]);
  opts->deref_ref = false;
  opts->raw = !pretty_printing;
}

std::string
varobj_value_get_print_value (struct value *value,
			      enum varobj_display_formats format,
			      const struct varobj *var)
{
  struct value_print_options opts;
  struct type *type = NULL;
  long len = 0;
  gdb::unique_xmalloc_ptr<char> encoding;
  /* Initialize it just to avoid a GCC false warning.  */
  CORE_ADDR str_addr = 0;
  bool string_print = false;

  if (value == NULL)
    return std::string ();

  string_file stb;
  std::string thevalue;

  varobj_formatted_print_options (&opts, format);

#if HAVE_PYTHON
  if (gdb_python_initialized)
    {
      PyObject *value_formatter =  var->dynamic->pretty_printer;

      gdbpy_enter_varobj enter_py (var);

      if (value_formatter)
	{
	  if (PyObject_HasAttr (value_formatter, gdbpy_to_string_cst))
	    {
	      struct value *replacement;

	      gdbpy_ref<> output = apply_varobj_pretty_printer (value_formatter,
								&replacement,
								&stb,
								&opts);

	      /* If we have string like output ...  */
	      if (output != nullptr && output != Py_None)
		{
		  /* If this is a lazy string, extract it.  For lazy
		     strings we always print as a string, so set
		     string_print.  */
		  if (gdbpy_is_lazy_string (output.get ()))
		    {
		      gdbpy_extract_lazy_string (output.get (), &str_addr,
						 &type, &len, &encoding);
		      string_print = true;
		    }
		  else
		    {
		      /* If it is a regular (non-lazy) string, extract
			 it and copy the contents into THEVALUE.  If the
			 hint says to print it as a string, set
			 string_print.  Otherwise just return the extracted
			 string as a value.  */

		      gdb::unique_xmalloc_ptr<char> s
			= python_string_to_target_string (output.get ());

		      if (s)
			{
			  struct gdbarch *gdbarch;

			  gdb::unique_xmalloc_ptr<char> hint
			    = gdbpy_get_display_hint (value_formatter);
			  if (hint)
			    {
			      if (!strcmp (hint.get (), "string"))
				string_print = true;
			    }

			  thevalue = std::string (s.get ());
			  len = thevalue.size ();
			  gdbarch = value->type ()->arch ();
			  type = builtin_type (gdbarch)->builtin_char;

			  if (!string_print)
			    return thevalue;
			}
		      else
			gdbpy_print_stack ();
		    }
		}
	      /* If the printer returned a replacement value, set VALUE
		 to REPLACEMENT.  If there is not a replacement value,
		 just use the value passed to this function.  */
	      if (replacement)
		value = replacement;
	    }
	  else
	    {
	      /* No to_string method, so if there is a 'children'
		 method, return the default.  */
	      if (PyObject_HasAttr (value_formatter, gdbpy_children_cst))
		return "{...}";
	    }
	}
      else
	{
	  /* If we've made it here, we don't want a pretty-printer --
	     if we had one, it would already have been used.  */
	  opts.raw = true;
	}
    }
#endif

  /* If the THEVALUE has contents, it is a regular string.  */
  if (!thevalue.empty ())
    current_language->printstr (&stb, type, (gdb_byte *) thevalue.c_str (),
				len, encoding.get (), 0, &opts);
  else if (string_print)
    /* Otherwise, if string_print is set, and it is not a regular
       string, it is a lazy string.  */
    val_print_string (type, encoding.get (), str_addr, len, &stb, &opts);
  else
    /* All other cases.  */
    common_val_print (value, &stb, 0, &opts, current_language);

  return stb.release ();
}

bool
varobj_editable_p (const struct varobj *var)
{
  struct type *type;

  if (!(var->root->is_valid && var->value != nullptr
	&& var->value->lval ()))
    return false;

  type = varobj_get_value_type (var);

  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      return false;
      break;

    default:
      return true;
      break;
    }
}

/* Call VAR's value_is_changeable_p language-specific callback.  */

bool
varobj_value_is_changeable_p (const struct varobj *var)
{
  return var->root->lang_ops->value_is_changeable_p (var);
}

/* Return true if that varobj is floating, that is is always evaluated in the
   selected frame, and not bound to thread/frame.  Such variable objects
   are created using '@' as frame specifier to -var-create.  */
bool
varobj_floating_p (const struct varobj *var)
{
  return var->root->floating;
}

/* Implement the "value_is_changeable_p" varobj callback for most
   languages.  */

bool
varobj_default_value_is_changeable_p (const struct varobj *var)
{
  bool r;
  struct type *type;

  if (CPLUS_FAKE_CHILD (var))
    return false;

  type = varobj_get_value_type (var);

  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ARRAY:
      r = false;
      break;

    default:
      r = true;
    }

  return r;
}

/* Iterate all the existing _root_ VAROBJs and call the FUNC callback
   for each one.  */

void
all_root_varobjs (gdb::function_view<void (struct varobj *var)> func)
{
  /* Iterate "safely" - handle if the callee deletes its passed VAROBJ.  */
  auto iter = rootlist.begin ();
  auto end = rootlist.end ();
  while (iter != end)
    {
      auto self = iter++;
      func ((*self)->rootvar);
    }
}

/* Try to recreate the varobj VAR if it is a global or floating.  This is a
   helper function for varobj_re_set.  */

static void
varobj_re_set_iter (struct varobj *var)
{
  /* Invalidated global varobjs must be re-evaluated.  */
  if (!var->root->is_valid && var->root->global)
    {
      struct varobj *tmp_var;

      /* Try to create a varobj with same expression.  If we succeed
	 and have a global replace the old varobj.  */
      tmp_var = varobj_create (nullptr, var->name.c_str (), (CORE_ADDR) 0,
			       USE_CURRENT_FRAME);
      if (tmp_var != nullptr && tmp_var->root->global)
	{
	  tmp_var->obj_name = var->obj_name;
	  varobj_delete (var, 0);
	  install_variable (tmp_var);
	}
    }
}

/* See varobj.h.  */

void 
varobj_re_set (void)
{
  all_root_varobjs (varobj_re_set_iter);
}

/* Ensure that no varobj keep references to OBJFILE.  */

static void
varobj_invalidate_if_uses_objfile (struct objfile *objfile)
{
  if (objfile->separate_debug_objfile_backlink != nullptr)
    objfile = objfile->separate_debug_objfile_backlink;

  all_root_varobjs ([objfile] (struct varobj *var)
    {
      if (var->root->valid_block != nullptr)
	{
	  struct objfile *bl_objfile = var->root->valid_block->objfile ();
	  if (bl_objfile->separate_debug_objfile_backlink != nullptr)
	    bl_objfile = bl_objfile->separate_debug_objfile_backlink;

	  if (bl_objfile == objfile)
	    {
	      /* The varobj is tied to a block which is going away.  There is
		 no way to reconstruct something later, so invalidate the
		 varobj completely and drop the reference to the block which is
		 being freed.  */
	      var->root->is_valid = false;
	      var->root->valid_block = nullptr;
	    }
	}

      if (var->root->exp != nullptr && var->root->exp->uses_objfile (objfile))
	{
	  /* The varobj's current expression references the objfile.  For
	     globals and floating, it is possible that when we try to
	     re-evaluate the expression later it is still valid with
	     whatever is in scope at that moment.  Just invalidate the
	     expression for now.  */
	  var->root->exp.reset ();

	  /* It only makes sense to keep a floating varobj around.  */
	  if (!var->root->floating)
	    var->root->is_valid = false;
	}

      /* var->value->type and var->type might also reference the objfile.
	 This is taken care of in value.c:preserve_values which deals with
	 making sure that objfile-owned types are replaced with
	 gdbarch-owned equivalents.  */
    });
}

/* A hash function for a varobj.  */

static hashval_t
hash_varobj (const void *a)
{
  const varobj *obj = (const varobj *) a;
  return htab_hash_string (obj->obj_name.c_str ());
}

/* A hash table equality function for varobjs.  */

static int
eq_varobj_and_string (const void *a, const void *b)
{
  const varobj *obj = (const varobj *) a;
  const char *name = (const char *) b;

  return obj->obj_name == name;
}

void _initialize_varobj ();
void
_initialize_varobj ()
{
  varobj_table = htab_create_alloc (5, hash_varobj, eq_varobj_and_string,
				    nullptr, xcalloc, xfree);

  add_setshow_zuinteger_cmd ("varobj", class_maintenance,
			     &varobjdebug,
			     _("Set varobj debugging."),
			     _("Show varobj debugging."),
			     _("When non-zero, varobj debugging is enabled."),
			     NULL, show_varobjdebug,
			     &setdebuglist, &showdebuglist);

  gdb::observers::free_objfile.attach (varobj_invalidate_if_uses_objfile,
				       "varobj");
}

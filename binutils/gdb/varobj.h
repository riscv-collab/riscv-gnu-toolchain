/* GDB variable objects API.
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

#ifndef VAROBJ_H
#define VAROBJ_H 1

#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"

/* Enumeration for the format types */
enum varobj_display_formats
  {
    FORMAT_NATURAL,		/* What gdb actually calls 'natural' */
    FORMAT_BINARY,		/* Binary display                    */
    FORMAT_DECIMAL,		/* Decimal display                   */
    FORMAT_HEXADECIMAL,		/* Hex display                       */
    FORMAT_OCTAL,		/* Octal display                     */
    FORMAT_ZHEXADECIMAL		/* Zero padded hexadecimal	     */
  };

enum varobj_type
  {
    USE_SPECIFIED_FRAME,        /* Use the frame passed to varobj_create.  */
    USE_CURRENT_FRAME,          /* Use the current frame.  */
    USE_SELECTED_FRAME          /* Always reevaluate in selected frame.  */
  };

/* Enumerator describing if a variable object is in scope.  */
enum varobj_scope_status
  {
    VAROBJ_IN_SCOPE = 0,        /* Varobj is scope, value available.  */
    VAROBJ_NOT_IN_SCOPE = 1,    /* Varobj is not in scope, value not
				   available, but varobj can become in
				   scope later.  */
    VAROBJ_INVALID = 2,         /* Varobj no longer has any value, and never
				   will.  */
  };

/* String representations of gdb's format codes (defined in varobj.c).  */
extern const char *varobj_format_string[];

/* Struct that describes a variable object instance.  */

struct varobj;

struct varobj_update_result
{
  varobj_update_result (struct varobj *varobj_,
			varobj_scope_status status_ = VAROBJ_IN_SCOPE)
  : varobj (varobj_), status (status_)
  {}

  varobj_update_result (varobj_update_result &&other) = default;

  DISABLE_COPY_AND_ASSIGN (varobj_update_result);

  struct varobj *varobj;
  bool type_changed = false;
  bool children_changed = false;
  bool changed = false;
  enum varobj_scope_status status;
  /* This variable is used internally by varobj_update to indicate if the
     new value of varobj is already computed and installed, or has to
     be yet installed.  Don't use this outside varobj.c.  */
  bool value_installed = false;

  /* This will be non-NULL when new children were added to the varobj.
     It lists the new children (which must necessarily come at the end
     of the child list) added during an update.  The caller is
     responsible for freeing this vector.  */
  std::vector<struct varobj *> newobj;
};

struct varobj_root;
struct varobj_dynamic;

/* Every variable in the system has a structure of this type defined
   for it.  This structure holds all information necessary to manipulate
   a particular object variable.  */
struct varobj
{
  explicit varobj (varobj_root *root_);
  ~varobj ();

  /* Name of the variable for this object.  If this variable is a
     child, then this name will be the child's source name.
     (bar, not foo.bar).  */
  /* NOTE: This is the "expression".  */
  std::string name;

  /* Expression for this child.  Can be used to create a root variable
     corresponding to this child.  */
  std::string path_expr;

  /* The name for this variable's object.  This is here for
     convenience when constructing this object's children.  */
  std::string obj_name;

  /* Index of this variable in its parent or -1.  */
  int index = -1;

  /* The type of this variable.  This can be NULL
     for artificial variable objects -- currently, the "accessibility"
     variable objects in C++.  */
  struct type *type = NULL;

  /* The value of this expression or subexpression.  A NULL value
     indicates there was an error getting this value.
     Invariant: if varobj_value_is_changeable_p (this) is non-zero, 
     the value is either NULL, or not lazy.  */
  value_ref_ptr value;

  /* The number of (immediate) children this variable has.  */
  int num_children = -1;

  /* If this object is a child, this points to its immediate parent.  */
  struct varobj *parent = NULL;

  /* Children of this object.  */
  std::vector<varobj *> children;

  /* Description of the root variable.  Points to root variable for
     children.  */
  struct varobj_root *root;

  /* The format of the output for this object.  */
  enum varobj_display_formats format = FORMAT_NATURAL;

  /* Was this variable updated via a varobj_set_value operation.  */
  bool updated = false;

  /* Last print value.  */
  std::string print_value;

  /* Is this variable frozen.  Frozen variables are never implicitly
     updated by -var-update * 
     or -var-update <direct-or-indirect-parent>.  */
  bool frozen = false;

  /* Is the value of this variable intentionally not fetched?  It is
     not fetched if either the variable is frozen, or any parents is
     frozen.  */
  bool not_fetched = false;

  /* Sub-range of children which the MI consumer has requested.  If
     FROM < 0 or TO < 0, means that all children have been
     requested.  */
  int from = -1;
  int to = -1;

  /* Dynamic part of varobj.  */
  struct varobj_dynamic *dynamic;
};

/* Is the variable X one of our "fake" children?  */
#define CPLUS_FAKE_CHILD(x) \
((x) != NULL && (x)->type == NULL && (x)->value == NULL)

/* The language specific vector */

struct lang_varobj_ops
{
  /* The number of children of PARENT.  */
  int (*number_of_children) (const struct varobj *parent);

  /* The name (expression) of a root varobj.  */
  std::string (*name_of_variable) (const struct varobj *parent);

  /* The name of the INDEX'th child of PARENT.  */
  std::string (*name_of_child) (const struct varobj *parent, int index);

  /* Returns the rooted expression of CHILD, which is a variable
     obtain that has some parent.  */
  std::string (*path_expr_of_child) (const struct varobj *child);

  /* The ``struct value *'' of the INDEX'th child of PARENT.  */
  struct value *(*value_of_child) (const struct varobj *parent, int index);

  /* The type of the INDEX'th child of PARENT.  */
  struct type *(*type_of_child) (const struct varobj *parent, int index);

  /* The current value of VAR.  */
  std::string (*value_of_variable) (const struct varobj *var,
				    enum varobj_display_formats format);

  /* Return true if changes in value of VAR must be detected and
     reported by -var-update.  Return false if -var-update should never
     report changes of such values.  This makes sense for structures
     (since the changes in children values will be reported separately),
     or for artificial objects (like 'public' pseudo-field in C++).

     Return value of false means that gdb need not call value_fetch_lazy
     for the value of this variable object.  */
  bool (*value_is_changeable_p) (const struct varobj *var);

  /* Return true if the type of VAR has mutated.

     VAR's value is still the varobj's previous value, while NEW_VALUE
     is VAR's new value and NEW_TYPE is the var's new type.  NEW_VALUE
     may be NULL indicating that there is no value available (the varobj
     may be out of scope, of may be the child of a null pointer, for
     instance).  NEW_TYPE, on the other hand, must never be NULL.

     This function should also be able to assume that var's number of
     children is set (not < 0).

     Languages where types do not mutate can set this to NULL.  */
  bool (*value_has_mutated) (const struct varobj *var, struct value *new_value,
			     struct type *new_type);

  /* Return true if VAR is a suitable path expression parent.

     For C like languages with anonymous structures and unions an anonymous
     structure or union is not a suitable parent.  */
  bool (*is_path_expr_parent) (const struct varobj *var);
};

extern const struct lang_varobj_ops c_varobj_ops;
extern const struct lang_varobj_ops cplus_varobj_ops;
extern const struct lang_varobj_ops ada_varobj_ops;

/* Non-zero if we want to see trace of varobj level stuff.  */

extern unsigned int varobjdebug;

/* API functions */

extern struct varobj *varobj_create (const char *objname,
				     const char *expression, CORE_ADDR frame,
				     enum varobj_type type);

extern std::string varobj_gen_name (void);

extern struct varobj *varobj_get_handle (const char *name);

extern const char *varobj_get_objname (const struct varobj *var);

extern std::string varobj_get_expression (const struct varobj *var);

/* Delete a varobj and all its children if only_children is false, otherwise
   delete only the children.  Return the number of deleted variables.  */

extern int varobj_delete (struct varobj *var, bool only_children);

extern enum varobj_display_formats varobj_set_display_format (
							 struct varobj *var,
					enum varobj_display_formats format);

extern enum varobj_display_formats varobj_get_display_format (
						const struct varobj *var);

extern int varobj_get_thread_id (const struct varobj *var);

extern void varobj_set_frozen (struct varobj *var, bool frozen);

extern bool varobj_get_frozen (const struct varobj *var);

extern void varobj_get_child_range (const struct varobj *var, int *from,
				    int *to);

extern void varobj_set_child_range (struct varobj *var, int from, int to);

extern gdb::unique_xmalloc_ptr<char>
     varobj_get_display_hint (const struct varobj *var);

extern int varobj_get_num_children (struct varobj *var);

/* Return the list of children of VAR.  The returned vector should not
   be modified in any way.  FROM and TO are in/out parameters
   indicating the range of children to return.  If either *FROM or *TO
   is less than zero on entry, then all children will be returned.  On
   return, *FROM and *TO will be updated to indicate the real range
   that was returned.  The resulting vector will contain at least the
   children from *FROM to just before *TO; it might contain more
   children, depending on whether any more were available.  */
extern const std::vector<varobj *> &
  varobj_list_children (struct varobj *var, int *from, int *to);

extern std::string varobj_get_type (struct varobj *var);

extern struct type *varobj_get_gdb_type (const struct varobj *var);

extern const char *varobj_get_path_expr (const struct varobj *var);

extern const struct language_defn *
  varobj_get_language (const struct varobj *var);

extern int varobj_get_attributes (const struct varobj *var);

extern std::string
  varobj_get_formatted_value (struct varobj *var,
			      enum varobj_display_formats format);

extern std::string varobj_get_value (struct varobj *var);

extern bool varobj_set_value (struct varobj *var, const char *expression);

extern void all_root_varobjs (gdb::function_view<void (struct varobj *var)>);

extern std::vector<varobj_update_result>
  varobj_update (struct varobj **varp, bool is_explicit);

/* Try to recreate any global or floating varobj.  This is called after
   changing symbol files.  */

extern void varobj_re_set (void);

extern bool varobj_editable_p (const struct varobj *var);

extern bool varobj_floating_p (const struct varobj *var);

extern void varobj_set_visualizer (struct varobj *var,
				   const char *visualizer);

extern void varobj_enable_pretty_printing (void);

extern bool varobj_has_more (const struct varobj *var, int to);

extern bool varobj_is_dynamic_p (const struct varobj *var);

extern bool varobj_default_value_is_changeable_p (const struct varobj *var);
extern bool varobj_value_is_changeable_p (const struct varobj *var);

extern struct type *varobj_get_value_type (const struct varobj *var);

extern bool varobj_is_anonymous_child (const struct varobj *child);

extern const struct varobj *
  varobj_get_path_expr_parent (const struct varobj *var);

extern std::string
  varobj_value_get_print_value (struct value *value,
				enum varobj_display_formats format,
				const struct varobj *var);

extern void varobj_formatted_print_options (struct value_print_options *opts,
					    enum varobj_display_formats format);

extern void varobj_restrict_range (const std::vector<varobj *> &children,
				   int *from, int *to);

extern bool varobj_default_is_path_expr_parent (const struct varobj *var);

#endif /* VAROBJ_H */

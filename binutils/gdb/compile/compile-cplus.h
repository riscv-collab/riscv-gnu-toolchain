/* Header file for GDB compile C++ language support.
   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef COMPILE_COMPILE_CPLUS_H
#define COMPILE_COMPILE_CPLUS_H

#include "compile/compile.h"
#include "gdbsupport/enum-flags.h"
#include "gcc-cp-plugin.h"
#include "symtab.h"

struct type;
struct block;

/* enum-flags wrapper  */
DEF_ENUM_FLAGS_TYPE (enum gcc_cp_qualifiers, gcc_cp_qualifiers_flags);
DEF_ENUM_FLAGS_TYPE (enum gcc_cp_ref_qualifiers, gcc_cp_ref_qualifiers_flags);
DEF_ENUM_FLAGS_TYPE (enum gcc_cp_symbol_kind, gcc_cp_symbol_kind_flags);

class compile_cplus_instance;

/* A single component of a type's scope.  Type names are broken into
   "components," a series of unqualified names comprising the type name,
   e.g., "namespace1", "namespace2", "myclass".  */

struct scope_component
{
  /* The unqualified name of this scope.  */
  std::string name;

  /* The block symbol for this type/scope.  */
  struct block_symbol bsymbol;
};

/* Comparison operators for scope_components.  */

bool operator== (const scope_component &lhs, const scope_component &rhs);
bool operator!= (const scope_component &lhs, const scope_component &rhs);


/* A single compiler scope used to define a type.

   A compile_scope is a list of scope_components, where all leading
   scope_components are namespaces, followed by a single non-namespace
   type component (the actual type we are converting).  */

class compile_scope : private std::vector<scope_component>
{
public:

  using std::vector<scope_component>::push_back;
  using std::vector<scope_component>::pop_back;
  using std::vector<scope_component>::back;
  using std::vector<scope_component>::empty;
  using std::vector<scope_component>::size;
  using std::vector<scope_component>::begin;
  using std::vector<scope_component>::end;
  using std::vector<scope_component>::operator[];

  compile_scope ()
    : m_nested_type (GCC_TYPE_NONE), m_pushed (false)
  {
  }

  /* Return the gcc_type of the type if it is a nested definition.
     Returns GCC_TYPE_NONE if this type was not nested.  */
  gcc_type nested_type ()
  {
    return m_nested_type;
  }

private:

  /* compile_cplus_instance is a friend class so that it can set the
     following private members when compile_scopes are created.  */
  friend compile_cplus_instance;

  /* If the type was actually a nested type, this will hold that nested
     type after the scope is pushed.  */
  gcc_type m_nested_type;

  /* If true, this scope was pushed to the compiler and all namespaces
     must be popped when leaving the scope.  */
  bool m_pushed;
};

/* Comparison operators for compile_scopes.  */

bool operator== (const compile_scope &lhs, const compile_scope &rhs);
bool operator!= (const compile_scope &lhs, const compile_scope &rhs);

/* Convert TYPENAME into a vector of namespace and top-most/super
   composite scopes.

   For example, for the input "Namespace::classB::classInner", the
   resultant vector will contain the tokens "Namespace" and
   "classB".  */

compile_scope type_name_to_scope (const char *type_name,
				  const struct block *block);

/* A callback suitable for use as the GCC C++ symbol oracle.  */

extern gcc_cp_oracle_function gcc_cplus_convert_symbol;

/* A callback suitable for use as the GCC C++ address oracle.  */

extern gcc_cp_symbol_address_function gcc_cplus_symbol_address;

/* A subclass of compile_instance that is specific to the C++ front
   end.  */

class compile_cplus_instance : public compile_instance
{
public:

  explicit compile_cplus_instance (struct gcc_cp_context *gcc_cp)
    : compile_instance (&gcc_cp->base, m_default_cflags),
      m_plugin (gcc_cp)
  {
    m_plugin.set_callbacks (gcc_cplus_convert_symbol,
			    gcc_cplus_symbol_address,
			    gcc_cplus_enter_scope, gcc_cplus_leave_scope,
			    this);
  }

  /* Convert a gdb type, TYPE, to a GCC type.

     If this type was defined in another type, NESTED_ACCESS should indicate
     the accessibility of this type (or GCC_CP_ACCESS_NONE if not a nested
     type).  GCC_CP_ACCESS_NONE is the default nested access.

     The new GCC type is returned.  */
  gcc_type convert_type
    (struct type *type,
     enum gcc_cp_symbol_kind nested_access = GCC_CP_ACCESS_NONE);

  /* Return a handle for the GCC plug-in.  */
  gcc_cp_plugin &plugin () { return m_plugin; }

  /* Factory method to create a new scope based on TYPE with name TYPE_NAME.
     [TYPE_NAME could be TYPE_NAME or SYMBOL_NATURAL_NAME.]

     If TYPE is a nested or local definition, nested_type () will return
     the gcc_type of the conversion.

     Otherwise, nested_type () is GCC_TYPE_NONE.  */
  compile_scope new_scope (const char *type_name, struct type *type);

  /* Enter the given NEW_SCOPE.  */
  void enter_scope (compile_scope &&scope);

  /* Leave the current scope.  */
  void leave_scope ();

  /* Add the qualifiers given by QUALS to BASE.  */
  gcc_type convert_qualified_base (gcc_type base,
				   gcc_cp_qualifiers_flags quals);

  /* Convert TARGET into a pointer type.  */
  gcc_type convert_pointer_base (gcc_type target);

  /* Convert BASE into a reference type.  RQUALS describes the reference.  */
  gcc_type convert_reference_base (gcc_type base,
				   enum gcc_cp_ref_qualifiers rquals);

  /* Return the declaration name of the symbol named NATURAL.
     This returns a name with no function arguments or template parameters,
     suitable for passing to the compiler plug-in.  */
  static gdb::unique_xmalloc_ptr<char> decl_name (const char *natural);

private:

  /* Callbacks suitable for use as the GCC C++ enter/leave scope requests.  */
  static gcc_cp_enter_leave_user_expr_scope_function gcc_cplus_enter_scope;
  static gcc_cp_enter_leave_user_expr_scope_function gcc_cplus_leave_scope;

  /* Default compiler flags for C++.  */
  static const char *m_default_cflags;

  /* The GCC plug-in.  */
  gcc_cp_plugin m_plugin;

  /* A list of scopes we are processing.  */
  std::vector<compile_scope> m_scopes;
};

/* Get the access flag for the NUM'th method of TYPE's FNI'th
   fieldlist.  */

enum gcc_cp_symbol_kind get_method_access_flag (const struct type *type,
						int fni, int num);

#endif /* COMPILE_COMPILE_CPLUS_H */

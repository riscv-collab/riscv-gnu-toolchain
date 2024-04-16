/* Helper routines for C++ support in GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by MontaVista Software.
   Namespace support contributed by David Carlton.

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

#ifndef CP_SUPPORT_H
#define CP_SUPPORT_H

#include "symtab.h"
#include "gdbsupport/gdb_vecs.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbsupport/array-view.h"
#include <vector>

/* Opaque declarations.  */

struct symbol;
struct block;
struct buildsym_compunit;
struct objfile;
struct type;
struct demangle_component;
struct using_direct;

/* A string representing the name of the anonymous namespace used in GDB.  */

#define CP_ANONYMOUS_NAMESPACE_STR "(anonymous namespace)"

/* The length of the string representing the anonymous namespace.  */

#define CP_ANONYMOUS_NAMESPACE_LEN 21

/* A string representing the start of an operator name.  */

#define CP_OPERATOR_STR "operator"

/* The length of CP_OPERATOR_STR.  */

#define CP_OPERATOR_LEN 8

/* The result of parsing a name.  */

struct demangle_parse_info
{
  demangle_parse_info ();

  ~demangle_parse_info ();

  /* The memory used during the parse.  */
  struct demangle_info *info;

  /* The result of the parse.  */
  struct demangle_component *tree;

  /* Any temporary memory used during typedef replacement.  */
  struct obstack obstack;
};


/* Functions from cp-support.c.  */

extern gdb::unique_xmalloc_ptr<char> cp_canonicalize_string
  (const char *string);

extern gdb::unique_xmalloc_ptr<char> cp_canonicalize_string_no_typedefs
  (const char *string);

typedef const char *(canonicalization_ftype) (struct type *, void *);

extern gdb::unique_xmalloc_ptr<char> cp_canonicalize_string_full
  (const char *string, canonicalization_ftype *finder, void *data);

extern char *cp_class_name_from_physname (const char *physname);

extern char *method_name_from_physname (const char *physname);

extern unsigned int cp_find_first_component (const char *name);

extern unsigned int cp_entire_prefix_len (const char *name);

extern gdb::unique_xmalloc_ptr<char> cp_func_name (const char *full_name);

extern gdb::unique_xmalloc_ptr<char> cp_remove_params
  (const char *demangled_name);

/* DEMANGLED_NAME is the name of a function, (optionally) including
   parameters and (optionally) a return type.  Return the name of the
   function without parameters or return type, or NULL if we can not
   parse the name.  If COMPLETION_MODE is true, then tolerate a
   non-existing or unbalanced parameter list.  */
extern gdb::unique_xmalloc_ptr<char> cp_remove_params_if_any
  (const char *demangled_name, bool completion_mode);

extern std::vector<symbol *> make_symbol_overload_list (const char *,
							const char *);

extern void add_symbol_overload_list_adl
  (gdb::array_view<type *> arg_types,
   const char *func_name,
   std::vector<symbol *> *overload_list);

extern struct type *cp_lookup_rtti_type (const char *name,
					 const struct block *block);

/* Produce an unsigned hash value from SEARCH_NAME that is compatible
   with cp_symbol_name_matches.  Only the last component in
   "foo::bar::function()" is considered for hashing purposes (i.e.,
   the entire prefix is skipped), so that later on looking up for
   "function" or "bar::function" in all namespaces is possible.  */
extern unsigned int cp_search_name_hash (const char *search_name);

/* Implement the "get_symbol_name_matcher" language_defn method for C++.  */
extern symbol_name_matcher_ftype *cp_get_symbol_name_matcher
  (const lookup_name_info &lookup_name);

/* Functions/variables from cp-namespace.c.  */

extern int cp_is_in_anonymous (const char *symbol_name);

extern void cp_scan_for_anonymous_namespaces (struct buildsym_compunit *,
					      const struct symbol *symbol,
					      struct objfile *objfile);

extern struct block_symbol cp_lookup_symbol_nonlocal
     (const struct language_defn *langdef,
      const char *name,
      const struct block *block,
      const domain_enum domain);

extern struct block_symbol
  cp_lookup_symbol_namespace (const char *the_namespace,
			      const char *name,
			      const struct block *block,
			      const domain_enum domain);

extern struct block_symbol cp_lookup_symbol_imports_or_template
     (const char *scope,
      const char *name,
      const struct block *block,
      const domain_enum domain);

extern struct block_symbol
  cp_lookup_nested_symbol (struct type *parent_type,
			   const char *nested_name,
			   const struct block *block,
			   const domain_enum domain);

struct type *cp_lookup_transparent_type (const char *name);

/* See description in cp-namespace.c.  */

struct type *cp_find_type_baseclass_by_name (struct type *parent_type,
					     const char *name);

/* Functions from cp-name-parser.y.  */

extern std::unique_ptr<demangle_parse_info> cp_demangled_name_to_comp
     (const char *demangled_name, std::string *errmsg);

/* Convert RESULT to a string.  ESTIMATED_LEN is used only as a guide
   to the length of the result.  */

extern gdb::unique_xmalloc_ptr<char> cp_comp_to_string
  (struct demangle_component *result, int estimated_len);

extern void cp_merge_demangle_parse_infos (struct demangle_parse_info *,
					   struct demangle_component *,
					   struct demangle_parse_info *);

/* The list of "maint cplus" commands.  */

extern struct cmd_list_element *maint_cplus_cmd_list;

/* Wrappers for bfd and libiberty demangling entry points.  Note they
   all force DMGL_VERBOSE so that callers don't need to.  This is so
   that GDB consistently uses DMGL_VERBOSE throughout -- we want
   libiberty's demangler to expand standard substitutions to their
   full template name.  */

/* A wrapper for bfd_demangle.  */

gdb::unique_xmalloc_ptr<char> gdb_demangle (const char *name, int options);

/* A wrapper for cplus_demangle_print.  */

extern char *gdb_cplus_demangle_print (int options,
				       struct demangle_component *tree,
				       int estimated_length,
				       size_t *p_allocated_size);

/* Find an instance of the character C in the string S that is outside
   of all parenthesis pairs, single-quoted strings, and double-quoted
   strings.  Also, ignore the char within a template name, like a ','
   within foo<int, int>.  */

extern const char *find_toplevel_char (const char *s, char c);

#endif /* CP_SUPPORT_H */

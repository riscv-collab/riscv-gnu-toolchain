/* Helper routines for C++ support in GDB.
   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Contributed by David Carlton and by Kealia, Inc.

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
#include "cp-support.h"
#include "gdbsupport/gdb_obstack.h"
#include "symtab.h"
#include "symfile.h"
#include "block.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "dictionary.h"
#include "command.h"
#include "frame.h"
#include "buildsym.h"
#include "language.h"
#include "namespace.h"
#include "inferior.h"
#include <map>
#include <string>
#include <string.h>

static struct block_symbol
  cp_lookup_nested_symbol_1 (struct type *container_type,
			     const char *nested_name,
			     const char *concatenated_name,
			     const struct block *block,
			     const domain_enum domain,
			     int basic_lookup, int is_in_anonymous);

static struct type *cp_lookup_transparent_type_loop (const char *name,
						     const char *scope,
						     int scope_len);

/* Check to see if SYMBOL refers to an object contained within an
   anonymous namespace; if so, add an appropriate using directive.  */

void
cp_scan_for_anonymous_namespaces (struct buildsym_compunit *compunit,
				  const struct symbol *const symbol,
				  struct objfile *const objfile)
{
  if (symbol->demangled_name () != NULL)
    {
      const char *name = symbol->demangled_name ();
      unsigned int previous_component;
      unsigned int next_component;

      /* Start with a quick-and-dirty check for mention of "(anonymous
	 namespace)".  */

      if (!cp_is_in_anonymous (name))
	return;

      previous_component = 0;
      next_component = cp_find_first_component (name + previous_component);

      while (name[next_component] == ':')
	{
	  if (((next_component - previous_component)
	       == CP_ANONYMOUS_NAMESPACE_LEN)
	      && strncmp (name + previous_component,
			  CP_ANONYMOUS_NAMESPACE_STR,
			  CP_ANONYMOUS_NAMESPACE_LEN) == 0)
	    {
	      int dest_len = (previous_component == 0
			      ? 0 : previous_component - 2);
	      int src_len = next_component;

	      char *dest = (char *) alloca (dest_len + 1);
	      char *src = (char *) alloca (src_len + 1);

	      memcpy (dest, name, dest_len);
	      memcpy (src, name, src_len);

	      dest[dest_len] = '\0';
	      src[src_len] = '\0';

	      /* We've found a component of the name that's an
		 anonymous namespace.  So add symbols in it to the
		 namespace given by the previous component if there is
		 one, or to the global namespace if there isn't.
		 The declared line of this using directive can be set
		 to 0, this way it is always considered valid.  */
	      std::vector<const char *> excludes;
	      add_using_directive (compunit->get_local_using_directives (),
				   dest, src, NULL, NULL, excludes, 0,
				   1, &objfile->objfile_obstack);
	    }
	  /* The "+ 2" is for the "::".  */
	  previous_component = next_component + 2;
	  next_component = (previous_component
			    + cp_find_first_component (name
						       + previous_component));
	}
    }
}

/* Test whether or not NAMESPACE looks like it mentions an anonymous
   namespace; return nonzero if so.  */

int
cp_is_in_anonymous (const char *symbol_name)
{
  return (strstr (symbol_name, CP_ANONYMOUS_NAMESPACE_STR)
	  != NULL);
}

/* Look up NAME in DOMAIN in BLOCK's static block and in global blocks.
   If IS_IN_ANONYMOUS is nonzero, the symbol in question is located
   within an anonymous namespace.  */

static struct block_symbol
cp_basic_lookup_symbol (const char *name, const struct block *block,
			const domain_enum domain, int is_in_anonymous)
{
  struct block_symbol sym;

  sym = lookup_symbol_in_static_block (name, block, domain);
  if (sym.symbol != NULL)
    return sym;

  if (is_in_anonymous)
    {
      /* Symbols defined in anonymous namespaces have external linkage
	 but should be treated as local to a single file nonetheless.
	 So we only search the current file's global block.  */

      const struct block *global_block = block->global_block ();

      if (global_block != NULL)
	{
	  sym.symbol = lookup_symbol_in_block (name,
					       symbol_name_match_type::FULL,
					       global_block, domain);
	  sym.block = global_block;
	}
    }
  else
    sym = lookup_global_symbol (name, block, domain);

  return sym;
}

/* Search bare symbol NAME in DOMAIN in BLOCK.
   NAME is guaranteed to not have any scope (no "::") in its name, though
   if for example NAME is a template spec then "::" may appear in the
   argument list.
   If LANGDEF is non-NULL then try to lookup NAME as a primitive type in
   that language.  Normally we wouldn't need LANGDEF but fortran also uses
   this code.
   If SEARCH is non-zero then see if we can determine "this" from BLOCK, and
   if so then also search for NAME in that class.  */

static struct block_symbol
cp_lookup_bare_symbol (const struct language_defn *langdef,
		       const char *name, const struct block *block,
		       const domain_enum domain, int search)
{
  struct block_symbol sym;

  /* Note: We can't do a simple assert for ':' not being in NAME because
     ':' may be in the args of a template spec.  This isn't intended to be
     a complete test, just cheap and documentary.  */
  gdb_assert (strpbrk ("<>()", name) != nullptr
	      || strstr (name, "::") == nullptr);

  sym = lookup_symbol_in_static_block (name, block, domain);
  if (sym.symbol != NULL)
    return sym;

  /* If we didn't find a definition for a builtin type in the static block,
     search for it now.  This is actually the right thing to do and can be
     a massive performance win.  E.g., when debugging a program with lots of
     shared libraries we could search all of them only to find out the
     builtin type isn't defined in any of them.  This is common for types
     like "void".  */
  if (langdef != NULL && domain == VAR_DOMAIN)
    {
      struct gdbarch *gdbarch;

      if (block == NULL)
	gdbarch = current_inferior ()->arch ();
      else
	gdbarch = block->gdbarch ();
      sym.symbol
	= language_lookup_primitive_type_as_symbol (langdef, gdbarch, name);
      sym.block = NULL;
      if (sym.symbol != NULL)
	return sym;
    }

  sym = lookup_global_symbol (name, block, domain);
  if (sym.symbol != NULL)
    return sym;

  if (search)
    {
      struct block_symbol lang_this;
      struct type *type;

      lang_this.symbol = NULL;

      if (langdef != NULL)
	lang_this = lookup_language_this (langdef, block);

      if (lang_this.symbol == NULL)
	return {};


      type = check_typedef (lang_this.symbol->type ()->target_type ());
      /* If TYPE_NAME is NULL, abandon trying to find this symbol.
	 This can happen for lambda functions compiled with clang++,
	 which outputs no name for the container class.  */
      if (type->name () == NULL)
	return {};

      /* Look for symbol NAME in this class.  */
      sym = cp_lookup_nested_symbol (type, name, block, domain);
    }

  return sym;
}

/* Search NAME in DOMAIN in all static blocks, and then in all baseclasses.
   BLOCK specifies the context in which to perform the search.
   NAME is guaranteed to have scope (contain "::") and PREFIX_LEN specifies
   the length of the entire scope of NAME (up to, but not including, the last
   "::".

   Note: At least in the case of Fortran, which also uses this code, there
   may be no text after the last "::".  */

static struct block_symbol
cp_search_static_and_baseclasses (const char *name,
				  const struct block *block,
				  const domain_enum domain,
				  unsigned int prefix_len,
				  int is_in_anonymous)
{
  /* Check for malformed input.  */
  if (prefix_len + 2 > strlen (name) || name[prefix_len + 1] != ':')
    return {};

  /* The class, namespace or function name is everything up to and
     including PREFIX_LEN.  */
  std::string scope (name, prefix_len);

  /* The rest of the name is everything else past the initial scope
     operator.  */
  const char *nested = name + prefix_len + 2;

  /* Lookup the scope symbol.  If none is found, there is nothing more
     that can be done.  SCOPE could be a namespace, so always look in
     VAR_DOMAIN.  This works for classes too because of
     symbol_matches_domain (which should be replaced with something
     else, but it's what we have today).  */
  block_symbol scope_sym = lookup_symbol_in_static_block (scope.c_str (),
							  block, VAR_DOMAIN);
  if (scope_sym.symbol == NULL)
    scope_sym = lookup_global_symbol (scope.c_str (), block, VAR_DOMAIN);
  if (scope_sym.symbol == NULL)
    return {};

  struct type *scope_type = scope_sym.symbol->type ();

  /* If the scope is a function/method, then look up NESTED as a local
     static variable.  E.g., "print 'function()::static_var'".  */
  if ((scope_type->code () == TYPE_CODE_FUNC
       || scope_type->code () == TYPE_CODE_METHOD)
      && domain == VAR_DOMAIN)
    return lookup_symbol (nested, scope_sym.symbol->value_block (),
			  VAR_DOMAIN, NULL);

  /* Look for a symbol named NESTED in this class/namespace.
     The caller is assumed to have already have done a basic lookup of NAME.
     So we pass zero for BASIC_LOOKUP to cp_lookup_nested_symbol_1 here.  */
  return cp_lookup_nested_symbol_1 (scope_type, nested, name,
				    block, domain, 0, is_in_anonymous);
}

/* Look up NAME in the C++ namespace NAMESPACE.  Other arguments are
   as in cp_lookup_symbol_nonlocal.  If SEARCH is non-zero, search
   through base classes for a matching symbol.

   Note: Part of the complexity is because NAME may itself specify scope.
   Part of the complexity is also because this handles the case where
   there is no scoping in which case we also try looking in the class of
   "this" if we can compute it.  */

static struct block_symbol
cp_lookup_symbol_in_namespace (const char *the_namespace, const char *name,
			       const struct block *block,
			       const domain_enum domain, int search)
{
  char *concatenated_name = NULL;
  int is_in_anonymous;
  unsigned int prefix_len;
  struct block_symbol sym;

  if (the_namespace[0] != '\0')
    {
      concatenated_name
	= (char *) alloca (strlen (the_namespace) + 2 + strlen (name) + 1);
      strcpy (concatenated_name, the_namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, name);
      name = concatenated_name;
    }

  prefix_len = cp_entire_prefix_len (name);
  if (prefix_len == 0)
    return cp_lookup_bare_symbol (NULL, name, block, domain, search);

  /* This would be simpler if we just called cp_lookup_nested_symbol
     at this point.  But that would require first looking up the containing
     class/namespace.  Since we're only searching static and global blocks
     there's often no need to first do that lookup.  */

  is_in_anonymous
    = the_namespace[0] != '\0' && cp_is_in_anonymous (the_namespace);
  sym = cp_basic_lookup_symbol (name, block, domain, is_in_anonymous);
  if (sym.symbol != NULL)
    return sym;

  if (search)
    sym = cp_search_static_and_baseclasses (name, block, domain, prefix_len,
					    is_in_anonymous);

  return sym;
}

/* This version of the function is internal, use the wrapper unless
   the list of ambiguous symbols is needed.

   Search for NAME by applying all import statements belonging to
   BLOCK which are applicable in SCOPE.  If DECLARATION_ONLY the
   search is restricted to using declarations.
   Example:

     namespace A {
       int x;
     }
     using A::x;

   If SEARCH_PARENTS the search will include imports which are
   applicable in parents of SCOPE.
   Example:

     namespace A {
       using namespace X;
       namespace B {
	 using namespace Y;
       }
     }

   If SCOPE is "A::B" and SEARCH_PARENTS is true the imports of
   namespaces X and Y will be considered.  If SEARCH_PARENTS is false
   only the import of Y is considered.

   SEARCH_SCOPE_FIRST is an internal implementation detail: Callers must
   pass 0 for it.  Internally we pass 1 when recursing.  */

static void
cp_lookup_symbol_via_imports (const char *scope,
			      const char *name,
			      const struct block *block,
			      const domain_enum domain,
			      const int search_scope_first,
			      const int declaration_only,
			      const int search_parents,
			      std::map<std::string,
				       struct block_symbol>& found_symbols)
{
  struct using_direct *current;
  struct block_symbol sym = {};
  int len;
  int directive_match;

  /* All the symbols we found will be kept in this relational map between
     the mangled name and the block_symbol found.  We do this so that GDB
     won't incorrectly report an ambiguous symbol for finding the same
     thing twice.  */

  /* First, try to find the symbol in the given namespace if requested.  */
  if (search_scope_first)
    {
      sym = cp_lookup_symbol_in_namespace (scope, name,
					   block, domain, 1);
      if (sym.symbol != nullptr)
	found_symbols[sym.symbol->m_name] = sym;
    }

  /* Due to a GCC bug, we need to know the boundaries of the current block
     to know if a certain using directive is valid.  */
  symtab_and_line boundary_sal = find_pc_line (block->end () - 1, 0);

  /* Go through the using directives.  If any of them add new names to
     the namespace we're searching in, see if we can find a match by
     applying them.  */
  for (current = block->get_using ();
       current != NULL;
       current = current->next)
    {
      const char **excludep;

      /* If the using directive was below the place we are stopped at,
	 do not use this directive.  */
      if (!current->valid_line (boundary_sal.line))
	continue;
      len = strlen (current->import_dest);
      directive_match = (search_parents
			 ? (startswith (scope, current->import_dest)
			    && (len == 0
				|| scope[len] == ':'
				|| scope[len] == '\0'))
			 : strcmp (scope, current->import_dest) == 0);

      /* If the import destination is the current scope or one of its
	 ancestors then it is applicable.  */
      if (directive_match && !current->searched)
	{
	  /* Mark this import as searched so that the recursive call
	     does not search it again.  */
	  scoped_restore reset_directive_searched
	    = make_scoped_restore (&current->searched, 1);

	  /* If there is an import of a single declaration, compare the
	     imported declaration (after optional renaming by its alias)
	     with the sought out name.  If there is a match pass
	     current->import_src as NAMESPACE to direct the search
	     towards the imported namespace.  */
	  if (current->declaration
	      && strcmp (name, current->alias
			 ? current->alias : current->declaration) == 0)
	    sym = cp_lookup_symbol_in_namespace (current->import_src,
						 current->declaration,
						 block, domain, 1);

	  /* If this is a DECLARATION_ONLY search or a symbol was found
	     or this import statement was an import declaration, the
	     search of this import is complete.  */
	  if (declaration_only || sym.symbol != NULL || current->declaration)
	    {
	      if (sym.symbol != NULL)
		found_symbols[sym.symbol->m_name] = sym;

	      continue;
	    }

	  /* Do not follow CURRENT if NAME matches its EXCLUDES.  */
	  for (excludep = current->excludes; *excludep; excludep++)
	    if (strcmp (name, *excludep) == 0)
	      break;
	  if (*excludep)
	    continue;

	  if (current->alias != NULL
	      && strcmp (name, current->alias) == 0)
	    /* If the import is creating an alias and the alias matches
	       the sought name.  Pass current->import_src as the NAME to
	       direct the search towards the aliased namespace.  */
	    {
	      sym = cp_lookup_symbol_in_namespace (scope,
						   current->import_src,
						   block, domain, 1);
	      found_symbols[sym.symbol->m_name] = sym;
	    }
	  else if (current->alias == NULL)
	    {
	      /* If this import statement creates no alias, pass
		 current->inner as NAMESPACE to direct the search
		 towards the imported namespace.  */
	      cp_lookup_symbol_via_imports (current->import_src, name,
					    block, domain, 1, 0, 0,
					    found_symbols);
	    }

	}
    }
}

/* Wrapper for the actual cp_lookup_symbol_via_imports.  This wrapper sets
   search_scope_first correctly and handles errors if needed.  */
static struct block_symbol
cp_lookup_symbol_via_imports (const char *scope,
			      const char *name,
			      const struct block *block,
			      const domain_enum domain,
			      const int declaration_only,
			      const int search_parents)
{
  std::map<std::string, struct block_symbol> found_symbols;

  cp_lookup_symbol_via_imports(scope, name, block, domain, 0,
			       declaration_only, search_parents,
			       found_symbols);

  if (found_symbols.size () > 1)
    {
      auto itr = found_symbols.cbegin ();
      std::string error_str = "Reference to \"";
      error_str += name;
      error_str += "\" is ambiguous, possibilities are: ";
      error_str += itr->second.symbol->print_name ();
      for (itr++; itr != found_symbols.end (); itr++)
	{
	  error_str += " and ";
	  error_str += itr->second.symbol->print_name ();
	}
      error (_("%s"), error_str.c_str ());
    }

  if (found_symbols.size() == 1)
    return found_symbols.cbegin ()->second;
  else
    return {};
}

/* Helper function that searches an array of symbols for one named NAME.  */

static struct symbol *
search_symbol_list (const char *name, int num,
		    struct symbol **syms)
{
  int i;

  /* Maybe we should store a dictionary in here instead.  */
  for (i = 0; i < num; ++i)
    {
      if (strcmp (name, syms[i]->natural_name ()) == 0)
	return syms[i];
    }
  return NULL;
}

/* Search for symbols whose name match NAME in the given SCOPE.
   if BLOCK is a function, we'll search first through the template
   parameters and function type. Afterwards (or if BLOCK is not a function)
   search through imported directives using cp_lookup_symbol_via_imports.  */

struct block_symbol
cp_lookup_symbol_imports_or_template (const char *scope,
				      const char *name,
				      const struct block *block,
				      const domain_enum domain)
{
  struct symbol *function = block->function ();

  symbol_lookup_debug_printf
    ("cp_lookup_symbol_imports_or_template (%s, %s, %s, %s)",
     scope, name, host_address_to_string (block), domain_name (domain));

  if (function != NULL && function->language () == language_cplus)
    {
      /* Search the function's template parameters.  */
      if (function->is_cplus_template_function ())
	{
	  struct template_symbol *templ
	    = (struct template_symbol *) function;
	  struct symbol *sym = search_symbol_list (name,
						   templ->n_template_arguments,
						   templ->template_arguments);

	  if (sym != NULL)
	    {
	      symbol_lookup_debug_printf
		("cp_lookup_symbol_imports_or_template (...) = %s",
		 host_address_to_string (sym));
	      return (struct block_symbol) {sym, block};
	    }
	}

      /* Search the template parameters of the function's defining
	 context.  */
      if (function->natural_name ())
	{
	  struct type *context;
	  std::string name_copy (function->natural_name ());
	  const struct language_defn *lang = language_def (language_cplus);
	  const struct block *parent = block->superblock ();
	  struct symbol *sym;

	  while (1)
	    {
	      unsigned int prefix_len
		= cp_entire_prefix_len (name_copy.c_str ());

	      if (prefix_len == 0)
		context = NULL;
	      else
		{
		  name_copy.erase (prefix_len);
		  context = lookup_typename (lang,
					     name_copy.c_str (),
					     parent, 1);
		}

	      if (context == NULL)
		break;

	      sym
		= search_symbol_list (name,
				      TYPE_N_TEMPLATE_ARGUMENTS (context),
				      TYPE_TEMPLATE_ARGUMENTS (context));
	      if (sym != NULL)
		{
		  symbol_lookup_debug_printf
		    ("cp_lookup_symbol_imports_or_template (...) = %s",
		     host_address_to_string (sym));
		  return (struct block_symbol) {sym, parent};
		}
	    }
	}
    }

  struct block_symbol result
    = cp_lookup_symbol_via_imports (scope, name, block, domain, 1, 1);
  symbol_lookup_debug_printf ("cp_lookup_symbol_imports_or_template (...) = %s\n",
		  result.symbol != nullptr
		  ? host_address_to_string (result.symbol) : "NULL");
  return result;
}

/* Search for NAME by applying relevant import statements belonging to BLOCK
   and its parents.  SCOPE is the namespace scope of the context in which the
   search is being evaluated.  */

static struct block_symbol
cp_lookup_symbol_via_all_imports (const char *scope, const char *name,
				  const struct block *block,
				  const domain_enum domain)
{
  struct block_symbol sym;

  while (block != NULL)
    {
      sym = cp_lookup_symbol_via_imports (scope, name, block, domain, 0, 1);
      if (sym.symbol != nullptr)
	return sym;

      block = block->superblock ();
    }

  return {};
}

/* Searches for NAME in the current namespace, and by applying
   relevant import statements belonging to BLOCK and its parents.
   SCOPE is the namespace scope of the context in which the search is
   being evaluated.  */

struct block_symbol
cp_lookup_symbol_namespace (const char *scope,
			    const char *name,
			    const struct block *block,
			    const domain_enum domain)
{
  struct block_symbol sym;

  symbol_lookup_debug_printf ("cp_lookup_symbol_namespace (%s, %s, %s, %s)",
			      scope, name, host_address_to_string (block),
			      domain_name (domain));

  /* First, try to find the symbol in the given namespace.  */
  sym = cp_lookup_symbol_in_namespace (scope, name, block, domain, 1);

  /* Search for name in namespaces imported to this and parent blocks.  */
  if (sym.symbol == NULL)
    sym = cp_lookup_symbol_via_all_imports (scope, name, block, domain);

  symbol_lookup_debug_printf ("cp_lookup_symbol_namespace (...) = %s",
			      sym.symbol != NULL
			      ? host_address_to_string (sym.symbol) : "NULL");
  return sym;
}

/* Lookup NAME at namespace scope (or, in C terms, in static and
   global variables).  SCOPE is the namespace that the current
   function is defined within; only consider namespaces whose length
   is at least SCOPE_LEN.  Other arguments are as in
   cp_lookup_symbol_nonlocal.

   For example, if we're within a function A::B::f and looking for a
   symbol x, this will get called with NAME = "x", SCOPE = "A::B", and
   SCOPE_LEN = 0.  It then calls itself with NAME and SCOPE the same,
   but with SCOPE_LEN = 1.  And then it calls itself with NAME and
   SCOPE the same, but with SCOPE_LEN = 4.  This third call looks for
   "A::B::x"; if it doesn't find it, then the second call looks for
   "A::x", and if that call fails, then the first call looks for
   "x".  */

static struct block_symbol
lookup_namespace_scope (const struct language_defn *langdef,
			const char *name,
			const struct block *block,
			const domain_enum domain,
			const char *scope,
			int scope_len)
{
  char *the_namespace;

  if (scope[scope_len] != '\0')
    {
      /* Recursively search for names in child namespaces first.  */

      struct block_symbol sym;
      int new_scope_len = scope_len;

      /* If the current scope is followed by "::", skip past that.  */
      if (new_scope_len != 0)
	{
	  gdb_assert (scope[new_scope_len] == ':');
	  new_scope_len += 2;
	}
      new_scope_len += cp_find_first_component (scope + new_scope_len);
      sym = lookup_namespace_scope (langdef, name, block, domain,
				    scope, new_scope_len);
      if (sym.symbol != NULL)
	return sym;
    }

  /* Okay, we didn't find a match in our children, so look for the
     name in the current namespace.

     If we there is no scope and we know we have a bare symbol, then short
     circuit everything and call cp_lookup_bare_symbol directly.
     This isn't an optimization, rather it allows us to pass LANGDEF which
     is needed for primitive type lookup.  The test doesn't have to be
     perfect: if NAME is a bare symbol that our test doesn't catch (e.g., a
     template symbol with "::" in the argument list) then
     cp_lookup_symbol_in_namespace will catch it.  */

  if (scope_len == 0 && strchr (name, ':') == NULL)
    return cp_lookup_bare_symbol (langdef, name, block, domain, 1);

  the_namespace = (char *) alloca (scope_len + 1);
  strncpy (the_namespace, scope, scope_len);
  the_namespace[scope_len] = '\0';
  return cp_lookup_symbol_in_namespace (the_namespace, name,
					block, domain, 1);
}

/* The C++-specific version of name lookup for static and global
   names.  This makes sure that names get looked for in all namespaces
   that are in scope.  NAME is the natural name of the symbol that
   we're looking for, BLOCK is the block that we're searching within,
   DOMAIN says what kind of symbols we're looking for.  */

struct block_symbol
cp_lookup_symbol_nonlocal (const struct language_defn *langdef,
			   const char *name,
			   const struct block *block,
			   const domain_enum domain)
{
  struct block_symbol sym;
  const char *scope = block == nullptr ? "" : block->scope ();

  symbol_lookup_debug_printf
    ("cp_lookup_symbol_non_local (%s, %s (scope %s), %s)",
     name, host_address_to_string (block), scope, domain_name (domain));

  /* First, try to find the symbol in the given namespace, and all
     containing namespaces.  */
  sym = lookup_namespace_scope (langdef, name, block, domain, scope, 0);

  /* Search for name in namespaces imported to this and parent blocks.  */
  if (sym.symbol == NULL)
    sym = cp_lookup_symbol_via_all_imports (scope, name, block, domain);

  symbol_lookup_debug_printf ("cp_lookup_symbol_nonlocal (...) = %s",
			      (sym.symbol != NULL
			       ? host_address_to_string (sym.symbol)
			       : "NULL"));
  return sym;
}

/* Search through the base classes of PARENT_TYPE for a base class
   named NAME and return its type.  If not found, return NULL.  */

struct type *
cp_find_type_baseclass_by_name (struct type *parent_type, const char *name)
{
  int i;

  parent_type = check_typedef (parent_type);
  for (i = 0; i < TYPE_N_BASECLASSES (parent_type); ++i)
    {
      struct type *type = check_typedef (TYPE_BASECLASS (parent_type, i));
      const char *tdef_name = TYPE_BASECLASS_NAME (parent_type, i);
      const char *base_name = type->name ();

      if (base_name == NULL)
	continue;

      if (streq (tdef_name, name) || streq (base_name, name))
	return type;

      type = cp_find_type_baseclass_by_name (type, name);
      if (type != NULL)
	return type;
    }

  return NULL;
}

/* Search through the base classes of PARENT_TYPE for a symbol named
   NAME in block BLOCK.  */

static struct block_symbol
find_symbol_in_baseclass (struct type *parent_type, const char *name,
			  const struct block *block, const domain_enum domain,
			  int is_in_anonymous)
{
  int i;
  struct block_symbol sym = {};

  for (i = 0; i < TYPE_N_BASECLASSES (parent_type); ++i)
    {
      struct type *base_type = TYPE_BASECLASS (parent_type, i);
      const char *base_name = TYPE_BASECLASS_NAME (parent_type, i);

      if (base_name == NULL)
	continue;

      std::string concatenated_name = std::string (base_name) + "::" + name;

      sym = cp_lookup_nested_symbol_1 (base_type, name,
				       concatenated_name.c_str (),
				       block, domain, 1, is_in_anonymous);
      if (sym.symbol != NULL)
	break;
    }

  return sym;
}

/* Helper function to look up NESTED_NAME in CONTAINER_TYPE and in DOMAIN
   and within the context of BLOCK.
   NESTED_NAME may have scope ("::").
   CONTAINER_TYPE needn't have been "check_typedef'd" yet.
   CONCATENATED_NAME is the fully scoped spelling of NESTED_NAME, it is
   passed as an argument so that callers can control how space for it is
   allocated.
   If BASIC_LOOKUP is non-zero then perform a basic lookup of
   CONCATENATED_NAME.  See cp_basic_lookup_symbol for details.
   If IS_IN_ANONYMOUS is non-zero then CONCATENATED_NAME is in an anonymous
   namespace.  */

static struct block_symbol
cp_lookup_nested_symbol_1 (struct type *container_type,
			   const char *nested_name,
			   const char *concatenated_name,
			   const struct block *block,
			   const domain_enum domain,
			   int basic_lookup, int is_in_anonymous)
{
  struct block_symbol sym;

  /* NOTE: carlton/2003-11-10: We don't treat C++ class members
     of classes like, say, data or function members.  Instead,
     they're just represented by symbols whose names are
     qualified by the name of the surrounding class.  This is
     just like members of namespaces; in particular,
     cp_basic_lookup_symbol works when looking them up.  */

  if (basic_lookup)
    {
      sym = cp_basic_lookup_symbol (concatenated_name, block, domain,
				    is_in_anonymous);
      if (sym.symbol != NULL)
	return sym;
    }

  /* Now search all static file-level symbols.  We have to do this for things
     like typedefs in the class.  We do not try to guess any imported
     namespace as even the fully specified namespace search is already not
     C++ compliant and more assumptions could make it too magic.  */

  /* First search in this symtab, what we want is possibly there.  */
  sym = lookup_symbol_in_static_block (concatenated_name, block, domain);
  if (sym.symbol != NULL)
    return sym;

  /* Nope.  We now have to search all static blocks in all objfiles,
     even if block != NULL, because there's no guarantees as to which
     symtab the symbol we want is in.  Except for symbols defined in
     anonymous namespaces should be treated as local to a single file,
     which we just searched.  */
  if (!is_in_anonymous)
    {
      sym = lookup_static_symbol (concatenated_name, domain);
      if (sym.symbol != NULL)
	return sym;
    }

  /* If this is a class with baseclasses, search them next.  */
  container_type = check_typedef (container_type);
  if (TYPE_N_BASECLASSES (container_type) > 0)
    {
      sym = find_symbol_in_baseclass (container_type, nested_name, block,
				      domain, is_in_anonymous);
      if (sym.symbol != NULL)
	return sym;
    }

  return {};
}

/* Look up a symbol named NESTED_NAME that is nested inside the C++
   class or namespace given by PARENT_TYPE, from within the context
   given by BLOCK, and in DOMAIN.
   Return NULL if there is no such nested symbol.  */

struct block_symbol
cp_lookup_nested_symbol (struct type *parent_type,
			 const char *nested_name,
			 const struct block *block,
			 const domain_enum domain)
{
  /* type_name_or_error provides better error reporting using the
     original type.  */
  struct type *saved_parent_type = parent_type;

  parent_type = check_typedef (parent_type);

  if (symbol_lookup_debug)
    {
      const char *type_name = saved_parent_type->name ();

      symbol_lookup_debug_printf ("cp_lookup_nested_symbol (%s, %s, %s, %s)",
				  type_name != NULL ? type_name : "unnamed",
				  nested_name, host_address_to_string (block),
				  domain_name (domain));
    }

  switch (parent_type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_NAMESPACE:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ENUM:
    /* NOTE: Handle modules here as well, because Fortran is re-using the C++
       specific code to lookup nested symbols in modules, by calling the
       method lookup_symbol_nonlocal, which ends up here.  */
    case TYPE_CODE_MODULE:
      {
	int size;
	const char *parent_name = type_name_or_error (saved_parent_type);
	struct block_symbol sym;
	char *concatenated_name;
	int is_in_anonymous;

	size = strlen (parent_name) + 2 + strlen (nested_name) + 1;
	concatenated_name = (char *) alloca (size);
	xsnprintf (concatenated_name, size, "%s::%s",
		   parent_name, nested_name);
	is_in_anonymous = cp_is_in_anonymous (concatenated_name);

	sym = cp_lookup_nested_symbol_1 (parent_type, nested_name,
					 concatenated_name, block, domain,
					 1, is_in_anonymous);

	symbol_lookup_debug_printf ("cp_lookup_nested_symbol (...) = %s",
				    (sym.symbol != NULL
				     ? host_address_to_string (sym.symbol)
				     : "NULL"));
	return sym;
      }

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      symbol_lookup_debug_printf
	("cp_lookup_nested_symbol (...) = NULL (func/method)");
      return {};

    default:
      internal_error (_("cp_lookup_nested_symbol called "
			"on a non-aggregate type."));
    }
}

/* The C++-version of lookup_transparent_type.  */

/* FIXME: carlton/2004-01-16: The problem that this is trying to
   address is that, unfortunately, sometimes NAME is wrong: it may not
   include the name of namespaces enclosing the type in question.
   lookup_transparent_type gets called when the type in question
   is a declaration, and we're trying to find its definition; but, for
   declarations, our type name deduction mechanism doesn't work.
   There's nothing we can do to fix this in general, I think, in the
   absence of debug information about namespaces (I've filed PR
   gdb/1511 about this); until such debug information becomes more
   prevalent, one heuristic which sometimes looks is to search for the
   definition in namespaces containing the current namespace.

   We should delete this functions once the appropriate debug
   information becomes more widespread.  (GCC 3.4 will be the first
   released version of GCC with such information.)  */

struct type *
cp_lookup_transparent_type (const char *name)
{
  /* First, try the honest way of looking up the definition.  */
  struct type *t = basic_lookup_transparent_type (name);
  const char *scope;

  if (t != NULL)
    return t;

  /* If that doesn't work and we're within a namespace, look there
     instead.  */
  const block *block = get_selected_block (0);
  if (block == nullptr)
    return nullptr;

  scope = block->scope ();

  if (scope[0] == '\0')
    return NULL;

  return cp_lookup_transparent_type_loop (name, scope, 0);
}

/* Lookup the type definition associated to NAME in namespaces/classes
   containing SCOPE whose name is strictly longer than LENGTH.  LENGTH
   must be the index of the start of a component of SCOPE.  */

static struct type *
cp_lookup_transparent_type_loop (const char *name,
				 const char *scope,
				 int length)
{
  int scope_length = length + cp_find_first_component (scope + length);
  char *full_name;

  /* If the current scope is followed by "::", look in the next
     component.  */
  if (scope[scope_length] == ':')
    {
      struct type *retval
	= cp_lookup_transparent_type_loop (name, scope,
					   scope_length + 2);

      if (retval != NULL)
	return retval;
    }

  full_name = (char *) alloca (scope_length + 2 + strlen (name) + 1);
  strncpy (full_name, scope, scope_length);
  memcpy (full_name + scope_length, "::", 2);
  strcpy (full_name + scope_length + 2, name);

  return basic_lookup_transparent_type (full_name);
}

/* This used to do something but was removed when it became
   obsolete.  */

static void
maintenance_cplus_namespace (const char *args, int from_tty)
{
  gdb_printf (_("The `maint namespace' command was removed.\n"));
}

void _initialize_cp_namespace ();
void
_initialize_cp_namespace ()
{
  struct cmd_list_element *cmd;

  cmd = add_cmd ("namespace", class_maintenance,
		 maintenance_cplus_namespace,
		 _("Deprecated placeholder for removed functionality."),
		 &maint_cplus_cmd_list);
  deprecate_cmd (cmd, NULL);
}

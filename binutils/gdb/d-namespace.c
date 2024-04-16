/* Helper routines for D support in GDB.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "block.h"
#include "language.h"
#include "namespace.h"
#include "d-lang.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbarch.h"
#include "inferior.h"

/* This returns the length of first component of NAME, which should be
   the demangled name of a D variable/function/method/etc.
   Specifically, it returns the index of the first dot forming the
   boundary of the first component: so, given 'A.foo' or 'A.B.foo'
   it returns the 1, and given 'foo', it returns 0.  */

/* The character in NAME indexed by the return value is guaranteed to
   always be either '.' or '\0'.  */

static unsigned int
d_find_first_component (const char *name)
{
  unsigned int index = 0;

  for (;; ++index)
    {
      if (name[index] == '.' || name[index] == '\0')
	return index;
    }
}

/* If NAME is the fully-qualified name of a D function/variable/method,
   this returns the length of its entire prefix: all of the modules and
   classes that make up its name.  Given 'A.foo', it returns 1, given
   'A.B.foo', it returns 4, given 'foo', it returns 0.  */

static unsigned int
d_entire_prefix_len (const char *name)
{
  unsigned int current_len = d_find_first_component (name);
  unsigned int previous_len = 0;

  while (name[current_len] != '\0')
    {
      gdb_assert (name[current_len] == '.');
      previous_len = current_len;
      /* Skip the '.'  */
      current_len++;
      current_len += d_find_first_component (name + current_len);
    }

  return previous_len;
}

/* Look up NAME in BLOCK's static block and in global blocks.
   If SEARCH is non-zero, search through base classes for a matching
   symbol.  Other arguments are as in d_lookup_symbol_nonlocal.  */

static struct block_symbol
d_lookup_symbol (const struct language_defn *langdef,
		 const char *name, const struct block *block,
		 const domain_enum domain, int search)
{
  struct block_symbol sym;

  sym = lookup_symbol_in_static_block (name, block, domain);
  if (sym.symbol != NULL)
    return sym;

  /* If we didn't find a definition for a builtin type in the static block,
     such as "ucent" which is a specialist type, search for it now.  */
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
      std::string classname, nested;
      unsigned int prefix_len;
      struct block_symbol class_sym;

      /* A simple lookup failed.  Check if the symbol was defined in
	 a base class.  */

      /* Find the name of the class and the name of the method,
	 variable, etc.  */
      prefix_len = d_entire_prefix_len (name);

      /* If no prefix was found, search "this".  */
      if (prefix_len == 0)
	{
	  struct type *type;
	  struct block_symbol lang_this;

	  lang_this = lookup_language_this (language_def (language_d), block);
	  if (lang_this.symbol == NULL)
	    return {};

	  type = check_typedef (lang_this.symbol->type ()->target_type ());
	  classname = type->name ();
	  nested = name;
	}
      else
	{
	  /* The class name is everything up to and including PREFIX_LEN.  */
	  classname = std::string (name, prefix_len);

	  /* The rest of the name is everything else past the initial scope
	     operator.  */
	  nested = std::string (name + prefix_len + 1);
	}

      /* Lookup a class named CLASSNAME.  If none is found, there is nothing
	 more that can be done.  */
      class_sym = lookup_global_symbol (classname.c_str (), block, domain);
      if (class_sym.symbol == NULL)
	return {};

      /* Look for a symbol named NESTED in this class.  */
      sym = d_lookup_nested_symbol (class_sym.symbol->type (),
				    nested.c_str (), block);
    }

  return sym;
}

/* Look up NAME in the D module MODULE.  Other arguments are as in
   d_lookup_symbol_nonlocal.  If SEARCH is non-zero, search through
   base classes for a matching symbol.  */

static struct block_symbol
d_lookup_symbol_in_module (const char *module, const char *name,
			   const struct block *block,
			   const domain_enum domain, int search)
{
  char *concatenated_name = NULL;

  if (module[0] != '\0')
    {
      concatenated_name
	= (char *) alloca (strlen (module) + strlen (name) + 2);
      strcpy (concatenated_name, module);
      strcat (concatenated_name, ".");
      strcat (concatenated_name, name);
      name = concatenated_name;
    }

  return d_lookup_symbol (NULL, name, block, domain, search);
}

/* Lookup NAME at module scope.  SCOPE is the module that the current
   function is defined within; only consider modules whose length is at
   least SCOPE_LEN.  Other arguments are as in d_lookup_symbol_nonlocal.

   For example, if we're within a function A.B.f and looking for a
   symbol x, this will get called with NAME = "x", SCOPE = "A.B", and
   SCOPE_LEN = 0.  It then calls itself with NAME and SCOPE the same,
   but with SCOPE_LEN = 1.  And then it calls itself with NAME and
   SCOPE the same, but with SCOPE_LEN = 4.  This third call looks for
   "A.B.x"; if it doesn't find it, then the second call looks for "A.x",
   and if that call fails, then the first call looks for "x".  */

static struct block_symbol
lookup_module_scope (const struct language_defn *langdef,
		     const char *name, const struct block *block,
		     const domain_enum domain, const char *scope,
		     int scope_len)
{
  char *module;

  if (scope[scope_len] != '\0')
    {
      /* Recursively search for names in child modules first.  */

      struct block_symbol sym;
      int new_scope_len = scope_len;

      /* If the current scope is followed by ".", skip past that.  */
      if (new_scope_len != 0)
	{
	  gdb_assert (scope[new_scope_len] == '.');
	  new_scope_len++;
	}
      new_scope_len += d_find_first_component (scope + new_scope_len);
      sym = lookup_module_scope (langdef, name, block, domain,
				 scope, new_scope_len);
      if (sym.symbol != NULL)
	return sym;
    }

  /* Okay, we didn't find a match in our children, so look for the
     name in the current module.

     If we there is no scope and we know we have a bare symbol, then short
     circuit everything and call d_lookup_symbol directly.
     This isn't an optimization, rather it allows us to pass LANGDEF which
     is needed for primitive type lookup.  */

  if (scope_len == 0 && strchr (name, '.') == NULL)
    return d_lookup_symbol (langdef, name, block, domain, 1);

  module = (char *) alloca (scope_len + 1);
  strncpy (module, scope, scope_len);
  module[scope_len] = '\0';
  return d_lookup_symbol_in_module (module, name,
				    block, domain, 1);
}

/* Search through the base classes of PARENT_TYPE for a symbol named
   NAME in block BLOCK.  */

static struct block_symbol
find_symbol_in_baseclass (struct type *parent_type, const char *name,
			  const struct block *block)
{
  struct block_symbol sym = {};
  int i;

  for (i = 0; i < TYPE_N_BASECLASSES (parent_type); ++i)
    {
      struct type *base_type = TYPE_BASECLASS (parent_type, i);
      const char *base_name = TYPE_BASECLASS_NAME (parent_type, i);

      if (base_name == NULL)
	continue;

      /* Search this particular base class.  */
      sym = d_lookup_symbol_in_module (base_name, name, block,
				       VAR_DOMAIN, 0);
      if (sym.symbol != NULL)
	break;

      /* Now search all static file-level symbols.  We have to do this for
	 things like typedefs in the class.  First search in this symtab,
	 what we want is possibly there.  */
      std::string concatenated_name = std::string (base_name) + "." + name;
      sym = lookup_symbol_in_static_block (concatenated_name.c_str (), block,
					   VAR_DOMAIN);
      if (sym.symbol != NULL)
	break;

      /* Nope.  We now have to search all static blocks in all objfiles,
	 even if block != NULL, because there's no guarantees as to which
	 symtab the symbol we want is in.  */
      sym = lookup_static_symbol (concatenated_name.c_str (), VAR_DOMAIN);
      if (sym.symbol != NULL)
	break;

      /* If this class has base classes, search them next.  */
      base_type = check_typedef (base_type);
      if (TYPE_N_BASECLASSES (base_type) > 0)
	{
	  sym = find_symbol_in_baseclass (base_type, name, block);
	  if (sym.symbol != NULL)
	    break;
	}
    }

  return sym;
}

/* Look up a symbol named NESTED_NAME that is nested inside the D
   class or module given by PARENT_TYPE, from within the context
   given by BLOCK.  Return NULL if there is no such nested type.  */

struct block_symbol
d_lookup_nested_symbol (struct type *parent_type,
			const char *nested_name,
			const struct block *block)
{
  /* type_name_no_tag_required provides better error reporting using the
     original type.  */
  struct type *saved_parent_type = parent_type;

  parent_type = check_typedef (parent_type);

  switch (parent_type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_MODULE:
	{
	  int size;
	  const char *parent_name = type_name_or_error (saved_parent_type);
	  struct block_symbol sym
	    = d_lookup_symbol_in_module (parent_name, nested_name,
					 block, VAR_DOMAIN, 0);
	  char *concatenated_name;

	  if (sym.symbol != NULL)
	    return sym;

	  /* Now search all static file-level symbols.  We have to do this
	     for things like typedefs in the class.  We do not try to
	     guess any imported module as even the fully specified
	     module search is already not D compliant and more assumptions
	     could make it too magic.  */
	  size = strlen (parent_name) + strlen (nested_name) + 2;
	  concatenated_name = (char *) alloca (size);

	  xsnprintf (concatenated_name, size, "%s.%s",
		     parent_name, nested_name);

	  sym = lookup_static_symbol (concatenated_name, VAR_DOMAIN);
	  if (sym.symbol != NULL)
	    return sym;

	  /* If no matching symbols were found, try searching any
	     base classes.  */
	  return find_symbol_in_baseclass (parent_type, nested_name, block);
	}

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      return {};

    default:
      gdb_assert_not_reached ("called with non-aggregate type.");
    }
}

/* Search for NAME by applying all import statements belonging to
   BLOCK which are applicable in SCOPE.  */

static struct block_symbol
d_lookup_symbol_imports (const char *scope, const char *name,
			 const struct block *block,
			 const domain_enum domain)
{
  struct using_direct *current;
  struct block_symbol sym;

  /* First, try to find the symbol in the given module.  */
  sym = d_lookup_symbol_in_module (scope, name, block, domain, 1);

  if (sym.symbol != NULL)
    return sym;

  /* Go through the using directives.  If any of them add new names to
     the module we're searching in, see if we can find a match by
     applying them.  */

  for (current = block->get_using ();
       current != NULL;
       current = current->next)
    {
      const char **excludep;

      /* If the import destination is the current scope then search it.  */
      if (!current->searched && strcmp (scope, current->import_dest) == 0)
	{
	  /* Mark this import as searched so that the recursive call
	     does not search it again.  */
	  scoped_restore restore_searched
	    = make_scoped_restore (&current->searched, 1);

	  /* If there is an import of a single declaration, compare the
	     imported declaration (after optional renaming by its alias)
	     with the sought out name.  If there is a match pass
	     current->import_src as MODULE to direct the search towards
	     the imported module.  */
	  if (current->declaration
	      && strcmp (name, current->alias
			 ? current->alias : current->declaration) == 0)
	    sym = d_lookup_symbol_in_module (current->import_src,
					     current->declaration,
					     block, domain, 1);

	  /* If a symbol was found or this import statement was an import
	     declaration, the search of this import is complete.  */
	  if (sym.symbol != NULL || current->declaration)
	    {
	      if (sym.symbol != NULL)
		return sym;

	      continue;
	    }

	  /* Do not follow CURRENT if NAME matches its EXCLUDES.  */
	  for (excludep = current->excludes; *excludep; excludep++)
	    if (strcmp (name, *excludep) == 0)
	      break;
	  if (*excludep)
	    continue;

	  /* If the import statement is creating an alias.  */
	  if (current->alias != NULL)
	    {
	      if (strcmp (name, current->alias) == 0)
		{
		  /* If the alias matches the sought name.  Pass
		     current->import_src as the NAME to direct the
		     search towards the aliased module.  */
		  sym = lookup_module_scope (NULL, current->import_src, block,
					     domain, scope, 0);
		}
	      else
		{
		  /* If the alias matches the first component of the
		     sought name, pass current->import_src as MODULE
		     to direct the search, skipping over the aliased
		     component in NAME.  */
		  int name_scope = d_find_first_component (name);

		  if (name[name_scope] != '\0'
		      && strncmp (name, current->alias, name_scope) == 0)
		    {
		      /* Skip the '.'  */
		      name_scope++;
		      sym = d_lookup_symbol_in_module (current->import_src,
						       name + name_scope,
						       block, domain, 1);
		    }
		}
	    }
	  else
	    {
	      /* If this import statement creates no alias, pass
		 current->import_src as MODULE to direct the search
		 towards the imported module.  */
	      sym = d_lookup_symbol_in_module (current->import_src,
					       name, block, domain, 1);
	    }

	  if (sym.symbol != NULL)
	    return sym;
	}
    }

  return {};
}

/* Searches for NAME in the current module, and by applying relevant
   import statements belonging to BLOCK and its parents.  SCOPE is the
   module scope of the context in which the search is being evaluated.  */

static struct block_symbol
d_lookup_symbol_module (const char *scope, const char *name,
			const struct block *block,
			const domain_enum domain)
{
  struct block_symbol sym;

  /* First, try to find the symbol in the given module.  */
  sym = d_lookup_symbol_in_module (scope, name,
				   block, domain, 1);
  if (sym.symbol != NULL)
    return sym;

  /* Search for name in modules imported to this and parent
     blocks.  */
  while (block != NULL)
    {
      sym = d_lookup_symbol_imports (scope, name, block, domain);

      if (sym.symbol != NULL)
	return sym;

      block = block->superblock ();
    }

  return {};
}

/* The D-specific version of name lookup for static and global names
   This makes sure that names get looked for in all modules that are
   in scope.  NAME is the natural name of the symbol that we're looking
   looking for, BLOCK is the block that we're searching within, DOMAIN
   says what kind of symbols we're looking for, and if SYMTAB is non-NULL,
   we should store the symtab where we found the symbol in it.  */

struct block_symbol
d_lookup_symbol_nonlocal (const struct language_defn *langdef,
			  const char *name,
			  const struct block *block,
			  const domain_enum domain)
{
  struct block_symbol sym;
  const char *scope = block == nullptr ? "" : block->scope ();

  sym = lookup_module_scope (langdef, name, block, domain, scope, 0);
  if (sym.symbol != NULL)
    return sym;

  return d_lookup_symbol_module (scope, name, block, domain);
}


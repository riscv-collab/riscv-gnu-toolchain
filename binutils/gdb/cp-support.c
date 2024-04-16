/* Helper routines for C++ support in GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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
#include "language.h"
#include "demangle.h"
#include "gdbcmd.h"
#include "dictionary.h"
#include "objfiles.h"
#include "frame.h"
#include "symtab.h"
#include "block.h"
#include "complaints.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "cp-abi.h"
#include "namespace.h"
#include <signal.h>
#include "gdbsupport/gdb_setjmp.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/gdb-sigmask.h"
#include <atomic>
#include "event-top.h"
#include "run-on-main-thread.h"
#include "typeprint.h"
#include "inferior.h"

#define d_left(dc) (dc)->u.s_binary.left
#define d_right(dc) (dc)->u.s_binary.right

/* Functions related to demangled name parsing.  */

static unsigned int cp_find_first_component_aux (const char *name,
						 int permissive);

static void demangled_name_complaint (const char *name);

/* Functions related to overload resolution.  */

static void overload_list_add_symbol (struct symbol *sym,
				      const char *oload_name,
				      std::vector<symbol *> *overload_list);

static void add_symbol_overload_list_using
  (const char *func_name, const char *the_namespace,
   std::vector<symbol *> *overload_list);

static void add_symbol_overload_list_qualified
  (const char *func_name,
   std::vector<symbol *> *overload_list);

/* The list of "maint cplus" commands.  */

struct cmd_list_element *maint_cplus_cmd_list = NULL;

static void
  replace_typedefs (struct demangle_parse_info *info,
		    struct demangle_component *ret_comp,
		    canonicalization_ftype *finder,
		    void *data);

static struct demangle_component *
  gdb_cplus_demangle_v3_components (const char *mangled,
				    int options, void **mem);

/* A convenience function to copy STRING into OBSTACK, returning a pointer
   to the newly allocated string and saving the number of bytes saved in LEN.

   It does not copy the terminating '\0' byte!  */

static char *
copy_string_to_obstack (struct obstack *obstack, const char *string,
			long *len)
{
  *len = strlen (string);
  return (char *) obstack_copy (obstack, string, *len);
}

/* Return 1 if STRING is clearly already in canonical form.  This
   function is conservative; things which it does not recognize are
   assumed to be non-canonical, and the parser will sort them out
   afterwards.  This speeds up the critical path for alphanumeric
   identifiers.  */

static int
cp_already_canonical (const char *string)
{
  /* Identifier start character [a-zA-Z_].  */
  if (!ISIDST (string[0]))
    return 0;

  /* These are the only two identifiers which canonicalize to other
     than themselves or an error: unsigned -> unsigned int and
     signed -> int.  */
  if (string[0] == 'u' && strcmp (&string[1], "nsigned") == 0)
    return 0;
  else if (string[0] == 's' && strcmp (&string[1], "igned") == 0)
    return 0;

  /* Identifier character [a-zA-Z0-9_].  */
  while (ISIDNUM (string[1]))
    string++;

  if (string[1] == '\0')
    return 1;
  else
    return 0;
}

/* Inspect the given RET_COMP for its type.  If it is a typedef,
   replace the node with the typedef's tree.

   Returns 1 if any typedef substitutions were made, 0 otherwise.  */

static int
inspect_type (struct demangle_parse_info *info,
	      struct demangle_component *ret_comp,
	      canonicalization_ftype *finder,
	      void *data)
{
  char *name;
  struct symbol *sym;

  /* Copy the symbol's name from RET_COMP and look it up
     in the symbol table.  */
  name = (char *) alloca (ret_comp->u.s_name.len + 1);
  memcpy (name, ret_comp->u.s_name.s, ret_comp->u.s_name.len);
  name[ret_comp->u.s_name.len] = '\0';

  sym = NULL;

  try
    {
      sym = lookup_symbol (name, 0, VAR_DOMAIN, 0).symbol;
    }
  catch (const gdb_exception &except)
    {
      return 0;
    }

  if (sym != NULL)
    {
      struct type *otype = sym->type ();

      if (finder != NULL)
	{
	  const char *new_name = (*finder) (otype, data);

	  if (new_name != NULL)
	    {
	      ret_comp->u.s_name.s = new_name;
	      ret_comp->u.s_name.len = strlen (new_name);
	      return 1;
	    }

	  return 0;
	}

      /* If the type is a typedef or namespace alias, replace it.  */
      if (otype->code () == TYPE_CODE_TYPEDEF
	  || otype->code () == TYPE_CODE_NAMESPACE)
	{
	  long len;
	  int is_anon;
	  struct type *type;
	  std::unique_ptr<demangle_parse_info> i;

	  /* Get the real type of the typedef.  */
	  type = check_typedef (otype);

	  /* If the symbol name is the same as the original type name,
	     don't substitute.  That would cause infinite recursion in
	     symbol lookups, as the typedef symbol is often the first
	     found symbol in the symbol table.

	     However, this can happen in a number of situations, such as:

	     If the symbol is a namespace and its type name is no different
	     than the name we looked up, this symbol is not a namespace
	     alias and does not need to be substituted.

	     If the symbol is typedef and its type name is the same
	     as the symbol's name, e.g., "typedef struct foo foo;".  */
	  if (type->name () != nullptr
	      && strcmp (type->name (), name) == 0)
	    return 0;

	  is_anon = (type->name () == NULL
		     && (type->code () == TYPE_CODE_ENUM
			 || type->code () == TYPE_CODE_STRUCT
			 || type->code () == TYPE_CODE_UNION));
	  if (is_anon)
	    {
	      struct type *last = otype;

	      /* Find the last typedef for the type.  */
	      while (last->target_type () != NULL
		     && (last->target_type ()->code ()
			 == TYPE_CODE_TYPEDEF))
		last = last->target_type ();

	      /* If there is only one typedef for this anonymous type,
		 do not substitute it.  */
	      if (type == otype)
		return 0;
	      else
		/* Use the last typedef seen as the type for this
		   anonymous type.  */
		type = last;
	    }

	  string_file buf;
	  try
	    {
	      /* Avoid using the current language.  If the language is
		 C, and TYPE is a struct/class, the printed type is
		 prefixed with "struct " or "class ", which we don't
		 want when we're expanding a C++ typedef.  Print using
		 the type symbol's language to expand a C++ typedef
		 the C++ way even if the current language is C.  */
	      const language_defn *lang = language_def (sym->language ());
	      lang->print_type (type, "", &buf, -1, 0, &type_print_raw_options);
	    }
	  /* If type_print threw an exception, there is little point
	     in continuing, so just bow out gracefully.  */
	  catch (const gdb_exception_error &except)
	    {
	      return 0;
	    }

	  len = buf.size ();
	  name = obstack_strdup (&info->obstack, buf.string ());

	  /* Turn the result into a new tree.  Note that this
	     tree will contain pointers into NAME, so NAME cannot
	     be free'd until all typedef conversion is done and
	     the final result is converted into a string.  */
	  i = cp_demangled_name_to_comp (name, NULL);
	  if (i != NULL)
	    {
	      /* Merge the two trees.  */
	      cp_merge_demangle_parse_infos (info, ret_comp, i.get ());

	      /* Replace any newly introduced typedefs -- but not
		 if the type is anonymous (that would lead to infinite
		 looping).  */
	      if (!is_anon)
		replace_typedefs (info, ret_comp, finder, data);
	    }
	  else
	    {
	      /* This shouldn't happen unless the type printer has
		 output something that the name parser cannot grok.
		 Nonetheless, an ounce of prevention...

		 Canonicalize the name again, and store it in the
		 current node (RET_COMP).  */
	      gdb::unique_xmalloc_ptr<char> canon
		= cp_canonicalize_string_no_typedefs (name);

	      if (canon != nullptr)
		{
		  /* Copy the canonicalization into the obstack.  */
		  name = copy_string_to_obstack (&info->obstack, canon.get (), &len);
		}

	      ret_comp->u.s_name.s = name;
	      ret_comp->u.s_name.len = len;
	    }

	  return 1;
	}
    }

  return 0;
}

/* Helper for replace_typedefs_qualified_name to handle
   DEMANGLE_COMPONENT_TEMPLATE.  TMPL is the template node.  BUF is
   the buffer that holds the qualified name being built by
   replace_typedefs_qualified_name.  REPL is the node that will be
   rewritten as a DEMANGLE_COMPONENT_NAME node holding the 'template
   plus template arguments' name with typedefs replaced.  */

static bool
replace_typedefs_template (struct demangle_parse_info *info,
			   string_file &buf,
			   struct demangle_component *tmpl,
			   struct demangle_component *repl,
			   canonicalization_ftype *finder,
			   void *data)
{
  demangle_component *tmpl_arglist = d_right (tmpl);

  /* Replace typedefs in the template argument list.  */
  replace_typedefs (info, tmpl_arglist, finder, data);

  /* Convert 'template + replaced template argument list' to a string
     and replace the REPL node.  */
  gdb::unique_xmalloc_ptr<char> tmpl_str = cp_comp_to_string (tmpl, 100);
  if (tmpl_str == nullptr)
    {
      /* If something went astray, abort typedef substitutions.  */
      return false;
    }
  buf.puts (tmpl_str.get ());

  repl->type = DEMANGLE_COMPONENT_NAME;
  repl->u.s_name.s = obstack_strdup (&info->obstack, buf.string ());
  repl->u.s_name.len = buf.size ();
  return true;
}

/* Replace any typedefs appearing in the qualified name
   (DEMANGLE_COMPONENT_QUAL_NAME) represented in RET_COMP for the name parse
   given in INFO.  */

static void
replace_typedefs_qualified_name (struct demangle_parse_info *info,
				 struct demangle_component *ret_comp,
				 canonicalization_ftype *finder,
				 void *data)
{
  string_file buf;
  struct demangle_component *comp = ret_comp;

  /* Walk each node of the qualified name, reconstructing the name of
     this element.  With every node, check for any typedef substitutions.
     If a substitution has occurred, replace the qualified name node
     with a DEMANGLE_COMPONENT_NAME node representing the new, typedef-
     substituted name.  */
  while (comp->type == DEMANGLE_COMPONENT_QUAL_NAME)
    {
      if (d_left (comp)->type == DEMANGLE_COMPONENT_TEMPLATE)
	{
	  /* Convert 'template + replaced template argument list' to a
	     string and replace the top DEMANGLE_COMPONENT_QUAL_NAME
	     node.  */
	  if (!replace_typedefs_template (info, buf,
					  d_left (comp), d_left (ret_comp),
					  finder, data))
	    return;

	  buf.clear ();
	  d_right (ret_comp) = d_right (comp);
	  comp = ret_comp;

	  /* Fallback to DEMANGLE_COMPONENT_NAME processing.  We want
	     to call inspect_type for this template, in case we have a
	     template alias, like:
	       template<typename T> using alias = base<int, t>;
	     in which case we want inspect_type to do a replacement like:
	       alias<int> -> base<int, int>
	  */
	}

      if (d_left (comp)->type == DEMANGLE_COMPONENT_NAME)
	{
	  struct demangle_component newobj;

	  buf.write (d_left (comp)->u.s_name.s, d_left (comp)->u.s_name.len);
	  newobj.type = DEMANGLE_COMPONENT_NAME;
	  newobj.u.s_name.s = obstack_strdup (&info->obstack, buf.string ());
	  newobj.u.s_name.len = buf.size ();
	  if (inspect_type (info, &newobj, finder, data))
	    {
	      char *s;
	      long slen;

	      /* A typedef was substituted in NEW.  Convert it to a
		 string and replace the top DEMANGLE_COMPONENT_QUAL_NAME
		 node.  */

	      buf.clear ();
	      gdb::unique_xmalloc_ptr<char> n
		= cp_comp_to_string (&newobj, 100);
	      if (n == NULL)
		{
		  /* If something went astray, abort typedef substitutions.  */
		  return;
		}

	      s = copy_string_to_obstack (&info->obstack, n.get (), &slen);

	      d_left (ret_comp)->type = DEMANGLE_COMPONENT_NAME;
	      d_left (ret_comp)->u.s_name.s = s;
	      d_left (ret_comp)->u.s_name.len = slen;
	      d_right (ret_comp) = d_right (comp);
	      comp = ret_comp;
	      continue;
	    }
	}
      else
	{
	  /* The current node is not a name, so simply replace any
	     typedefs in it.  Then print it to the stream to continue
	     checking for more typedefs in the tree.  */
	  replace_typedefs (info, d_left (comp), finder, data);
	  gdb::unique_xmalloc_ptr<char> name
	    = cp_comp_to_string (d_left (comp), 100);
	  if (name == NULL)
	    {
	      /* If something went astray, abort typedef substitutions.  */
	      return;
	    }
	  buf.puts (name.get ());
	}

      buf.write ("::", 2);
      comp = d_right (comp);
    }

  /* If the next component is DEMANGLE_COMPONENT_TEMPLATE or
     DEMANGLE_COMPONENT_NAME, save the qualified name assembled above
     and append the name given by COMP.  Then use this reassembled
     name to check for a typedef.  */

  if (comp->type == DEMANGLE_COMPONENT_TEMPLATE)
    {
      /* Replace the top (DEMANGLE_COMPONENT_QUAL_NAME) node with a
	 DEMANGLE_COMPONENT_NAME node containing the whole name.  */
      if (!replace_typedefs_template (info, buf, comp, ret_comp, finder, data))
	return;
      inspect_type (info, ret_comp, finder, data);
    }
  else if (comp->type == DEMANGLE_COMPONENT_NAME)
    {
      buf.write (comp->u.s_name.s, comp->u.s_name.len);

      /* Replace the top (DEMANGLE_COMPONENT_QUAL_NAME) node
	 with a DEMANGLE_COMPONENT_NAME node containing the whole
	 name.  */
      ret_comp->type = DEMANGLE_COMPONENT_NAME;
      ret_comp->u.s_name.s = obstack_strdup (&info->obstack, buf.string ());
      ret_comp->u.s_name.len = buf.size ();
      inspect_type (info, ret_comp, finder, data);
    }
  else
    replace_typedefs (info, comp, finder, data);
}


/* A function to check const and volatile qualifiers for argument types.

   "Parameter declarations that differ only in the presence
   or absence of `const' and/or `volatile' are equivalent."
   C++ Standard N3290, clause 13.1.3 #4.  */

static void
check_cv_qualifiers (struct demangle_component *ret_comp)
{
  while (d_left (ret_comp) != NULL
	 && (d_left (ret_comp)->type == DEMANGLE_COMPONENT_CONST
	     || d_left (ret_comp)->type == DEMANGLE_COMPONENT_VOLATILE))
    {
      d_left (ret_comp) = d_left (d_left (ret_comp));
    }
}

/* Walk the parse tree given by RET_COMP, replacing any typedefs with
   their basic types.  */

static void
replace_typedefs (struct demangle_parse_info *info,
		  struct demangle_component *ret_comp,
		  canonicalization_ftype *finder,
		  void *data)
{
  if (ret_comp)
    {
      if (finder != NULL
	  && (ret_comp->type == DEMANGLE_COMPONENT_NAME
	      || ret_comp->type == DEMANGLE_COMPONENT_QUAL_NAME
	      || ret_comp->type == DEMANGLE_COMPONENT_TEMPLATE
	      || ret_comp->type == DEMANGLE_COMPONENT_BUILTIN_TYPE))
	{
	  gdb::unique_xmalloc_ptr<char> local_name
	    = cp_comp_to_string (ret_comp, 10);

	  if (local_name != NULL)
	    {
	      struct symbol *sym = NULL;

	      sym = NULL;
	      try
		{
		  sym = lookup_symbol (local_name.get (), 0,
				       VAR_DOMAIN, 0).symbol;
		}
	      catch (const gdb_exception &except)
		{
		}

	      if (sym != NULL)
		{
		  struct type *otype = sym->type ();
		  const char *new_name = (*finder) (otype, data);

		  if (new_name != NULL)
		    {
		      ret_comp->type = DEMANGLE_COMPONENT_NAME;
		      ret_comp->u.s_name.s = new_name;
		      ret_comp->u.s_name.len = strlen (new_name);
		      return;
		    }
		}
	    }
	}

      switch (ret_comp->type)
	{
	case DEMANGLE_COMPONENT_ARGLIST:
	  check_cv_qualifiers (ret_comp);
	  [[fallthrough]];

	case DEMANGLE_COMPONENT_FUNCTION_TYPE:
	case DEMANGLE_COMPONENT_TEMPLATE:
	case DEMANGLE_COMPONENT_TEMPLATE_ARGLIST:
	case DEMANGLE_COMPONENT_TYPED_NAME:
	  replace_typedefs (info, d_left (ret_comp), finder, data);
	  replace_typedefs (info, d_right (ret_comp), finder, data);
	  break;

	case DEMANGLE_COMPONENT_NAME:
	  inspect_type (info, ret_comp, finder, data);
	  break;

	case DEMANGLE_COMPONENT_QUAL_NAME:
	  replace_typedefs_qualified_name (info, ret_comp, finder, data);
	  break;

	case DEMANGLE_COMPONENT_LOCAL_NAME:
	case DEMANGLE_COMPONENT_CTOR:
	case DEMANGLE_COMPONENT_ARRAY_TYPE:
	case DEMANGLE_COMPONENT_PTRMEM_TYPE:
	  replace_typedefs (info, d_right (ret_comp), finder, data);
	  break;

	case DEMANGLE_COMPONENT_CONST:
	case DEMANGLE_COMPONENT_RESTRICT:
	case DEMANGLE_COMPONENT_VOLATILE:
	case DEMANGLE_COMPONENT_VOLATILE_THIS:
	case DEMANGLE_COMPONENT_CONST_THIS:
	case DEMANGLE_COMPONENT_RESTRICT_THIS:
	case DEMANGLE_COMPONENT_POINTER:
	case DEMANGLE_COMPONENT_REFERENCE:
	case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
	  replace_typedefs (info, d_left (ret_comp), finder, data);
	  break;

	default:
	  break;
	}
    }
}

/* Parse STRING and convert it to canonical form, resolving any
   typedefs.  If parsing fails, or if STRING is already canonical,
   return nullptr.  Otherwise return the canonical form.  If
   FINDER is not NULL, then type components are passed to FINDER to be
   looked up.  DATA is passed verbatim to FINDER.  */

gdb::unique_xmalloc_ptr<char>
cp_canonicalize_string_full (const char *string,
			     canonicalization_ftype *finder,
			     void *data)
{
  unsigned int estimated_len;
  std::unique_ptr<demangle_parse_info> info;

  estimated_len = strlen (string) * 2;
  info = cp_demangled_name_to_comp (string, NULL);
  if (info != NULL)
    {
      /* Replace all the typedefs in the tree.  */
      replace_typedefs (info.get (), info->tree, finder, data);

      /* Convert the tree back into a string.  */
      gdb::unique_xmalloc_ptr<char> us = cp_comp_to_string (info->tree,
							    estimated_len);
      gdb_assert (us);

      /* Finally, compare the original string with the computed
	 name, returning NULL if they are the same.  */
      if (strcmp (us.get (), string) == 0)
	return nullptr;

      return us;
    }

  return nullptr;
}

/* Like cp_canonicalize_string_full, but always passes NULL for
   FINDER.  */

gdb::unique_xmalloc_ptr<char>
cp_canonicalize_string_no_typedefs (const char *string)
{
  return cp_canonicalize_string_full (string, NULL, NULL);
}

/* Parse STRING and convert it to canonical form.  If parsing fails,
   or if STRING is already canonical, return nullptr.
   Otherwise return the canonical form.  */

gdb::unique_xmalloc_ptr<char>
cp_canonicalize_string (const char *string)
{
  std::unique_ptr<demangle_parse_info> info;
  unsigned int estimated_len;

  if (cp_already_canonical (string))
    return nullptr;

  info = cp_demangled_name_to_comp (string, NULL);
  if (info == NULL)
    return nullptr;

  estimated_len = strlen (string) * 2;
  gdb::unique_xmalloc_ptr<char> us (cp_comp_to_string (info->tree,
						       estimated_len));

  if (!us)
    {
      warning (_("internal error: string \"%s\" failed to be canonicalized"),
	       string);
      return nullptr;
    }

  if (strcmp (us.get (), string) == 0)
    return nullptr;

  return us;
}

/* Convert a mangled name to a demangle_component tree.  *MEMORY is
   set to the block of used memory that should be freed when finished
   with the tree.  DEMANGLED_P is set to the char * that should be
   freed when finished with the tree, or NULL if none was needed.
   OPTIONS will be passed to the demangler.  */

static std::unique_ptr<demangle_parse_info>
mangled_name_to_comp (const char *mangled_name, int options,
		      void **memory,
		      gdb::unique_xmalloc_ptr<char> *demangled_p)
{
  /* If it looks like a v3 mangled name, then try to go directly
     to trees.  */
  if (mangled_name[0] == '_' && mangled_name[1] == 'Z')
    {
      struct demangle_component *ret;

      ret = gdb_cplus_demangle_v3_components (mangled_name,
					      options, memory);
      if (ret)
	{
	  auto info = std::make_unique<demangle_parse_info> ();
	  info->tree = ret;
	  *demangled_p = NULL;
	  return info;
	}
    }

  /* If it doesn't, or if that failed, then try to demangle the
     name.  */
  gdb::unique_xmalloc_ptr<char> demangled_name = gdb_demangle (mangled_name,
							       options);
  if (demangled_name == NULL)
   return NULL;
  
  /* If we could demangle the name, parse it to build the component
     tree.  */
  std::unique_ptr<demangle_parse_info> info
    = cp_demangled_name_to_comp (demangled_name.get (), NULL);

  if (info == NULL)
    return NULL;

  *demangled_p = std::move (demangled_name);
  return info;
}

/* Return the name of the class containing method PHYSNAME.  */

char *
cp_class_name_from_physname (const char *physname)
{
  void *storage = NULL;
  gdb::unique_xmalloc_ptr<char> demangled_name;
  gdb::unique_xmalloc_ptr<char> ret;
  struct demangle_component *ret_comp, *prev_comp, *cur_comp;
  std::unique_ptr<demangle_parse_info> info;
  int done;

  info = mangled_name_to_comp (physname, DMGL_ANSI,
			       &storage, &demangled_name);
  if (info == NULL)
    return NULL;

  done = 0;
  ret_comp = info->tree;

  /* First strip off any qualifiers, if we have a function or
     method.  */
  while (!done)
    switch (ret_comp->type)
      {
      case DEMANGLE_COMPONENT_CONST:
      case DEMANGLE_COMPONENT_RESTRICT:
      case DEMANGLE_COMPONENT_VOLATILE:
      case DEMANGLE_COMPONENT_CONST_THIS:
      case DEMANGLE_COMPONENT_RESTRICT_THIS:
      case DEMANGLE_COMPONENT_VOLATILE_THIS:
      case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
	ret_comp = d_left (ret_comp);
	break;
      default:
	done = 1;
	break;
      }

  /* If what we have now is a function, discard the argument list.  */
  if (ret_comp->type == DEMANGLE_COMPONENT_TYPED_NAME)
    ret_comp = d_left (ret_comp);

  /* If what we have now is a template, strip off the template
     arguments.  The left subtree may be a qualified name.  */
  if (ret_comp->type == DEMANGLE_COMPONENT_TEMPLATE)
    ret_comp = d_left (ret_comp);

  /* What we have now should be a name, possibly qualified.
     Additional qualifiers could live in the left subtree or the right
     subtree.  Find the last piece.  */
  done = 0;
  prev_comp = NULL;
  cur_comp = ret_comp;
  while (!done)
    switch (cur_comp->type)
      {
      case DEMANGLE_COMPONENT_QUAL_NAME:
      case DEMANGLE_COMPONENT_LOCAL_NAME:
	prev_comp = cur_comp;
	cur_comp = d_right (cur_comp);
	break;
      case DEMANGLE_COMPONENT_TEMPLATE:
      case DEMANGLE_COMPONENT_NAME:
      case DEMANGLE_COMPONENT_CTOR:
      case DEMANGLE_COMPONENT_DTOR:
      case DEMANGLE_COMPONENT_OPERATOR:
      case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
	done = 1;
	break;
      default:
	done = 1;
	cur_comp = NULL;
	break;
      }

  if (cur_comp != NULL && prev_comp != NULL)
    {
      /* We want to discard the rightmost child of PREV_COMP.  */
      *prev_comp = *d_left (prev_comp);
      /* The ten is completely arbitrary; we don't have a good
	 estimate.  */
      ret = cp_comp_to_string (ret_comp, 10);
    }

  xfree (storage);
  return ret.release ();
}

/* Return the child of COMP which is the basename of a method,
   variable, et cetera.  All scope qualifiers are discarded, but
   template arguments will be included.  The component tree may be
   modified.  */

static struct demangle_component *
unqualified_name_from_comp (struct demangle_component *comp)
{
  struct demangle_component *ret_comp = comp, *last_template;
  int done;

  done = 0;
  last_template = NULL;
  while (!done)
    switch (ret_comp->type)
      {
      case DEMANGLE_COMPONENT_QUAL_NAME:
      case DEMANGLE_COMPONENT_LOCAL_NAME:
	ret_comp = d_right (ret_comp);
	break;
      case DEMANGLE_COMPONENT_TYPED_NAME:
	ret_comp = d_left (ret_comp);
	break;
      case DEMANGLE_COMPONENT_TEMPLATE:
	gdb_assert (last_template == NULL);
	last_template = ret_comp;
	ret_comp = d_left (ret_comp);
	break;
      case DEMANGLE_COMPONENT_CONST:
      case DEMANGLE_COMPONENT_RESTRICT:
      case DEMANGLE_COMPONENT_VOLATILE:
      case DEMANGLE_COMPONENT_CONST_THIS:
      case DEMANGLE_COMPONENT_RESTRICT_THIS:
      case DEMANGLE_COMPONENT_VOLATILE_THIS:
      case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
	ret_comp = d_left (ret_comp);
	break;
      case DEMANGLE_COMPONENT_NAME:
      case DEMANGLE_COMPONENT_CTOR:
      case DEMANGLE_COMPONENT_DTOR:
      case DEMANGLE_COMPONENT_OPERATOR:
      case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
	done = 1;
	break;
      default:
	return NULL;
	break;
      }

  if (last_template)
    {
      d_left (last_template) = ret_comp;
      return last_template;
    }

  return ret_comp;
}

/* Return the name of the method whose linkage name is PHYSNAME.  */

char *
method_name_from_physname (const char *physname)
{
  void *storage = NULL;
  gdb::unique_xmalloc_ptr<char> demangled_name;
  gdb::unique_xmalloc_ptr<char> ret;
  struct demangle_component *ret_comp;
  std::unique_ptr<demangle_parse_info> info;

  info = mangled_name_to_comp (physname, DMGL_ANSI,
			       &storage, &demangled_name);
  if (info == NULL)
    return NULL;

  ret_comp = unqualified_name_from_comp (info->tree);

  if (ret_comp != NULL)
    /* The ten is completely arbitrary; we don't have a good
       estimate.  */
    ret = cp_comp_to_string (ret_comp, 10);

  xfree (storage);
  return ret.release ();
}

/* If FULL_NAME is the demangled name of a C++ function (including an
   arg list, possibly including namespace/class qualifications),
   return a new string containing only the function name (without the
   arg list/class qualifications).  Otherwise, return NULL.  */

gdb::unique_xmalloc_ptr<char>
cp_func_name (const char *full_name)
{
  gdb::unique_xmalloc_ptr<char> ret;
  struct demangle_component *ret_comp;
  std::unique_ptr<demangle_parse_info> info;

  info = cp_demangled_name_to_comp (full_name, NULL);
  if (!info)
    return nullptr;

  ret_comp = unqualified_name_from_comp (info->tree);

  if (ret_comp != NULL)
    ret = cp_comp_to_string (ret_comp, 10);

  return ret;
}

/* Helper for cp_remove_params.  DEMANGLED_NAME is the name of a
   function, including parameters and (optionally) a return type.
   Return the name of the function without parameters or return type,
   or NULL if we can not parse the name.  If REQUIRE_PARAMS is false,
   then tolerate a non-existing or unbalanced parameter list.  */

static gdb::unique_xmalloc_ptr<char>
cp_remove_params_1 (const char *demangled_name, bool require_params)
{
  bool done = false;
  struct demangle_component *ret_comp;
  std::unique_ptr<demangle_parse_info> info;
  gdb::unique_xmalloc_ptr<char> ret;

  if (demangled_name == NULL)
    return NULL;

  info = cp_demangled_name_to_comp (demangled_name, NULL);
  if (info == NULL)
    return NULL;

  /* First strip off any qualifiers, if we have a function or method.  */
  ret_comp = info->tree;
  while (!done)
    switch (ret_comp->type)
      {
      case DEMANGLE_COMPONENT_CONST:
      case DEMANGLE_COMPONENT_RESTRICT:
      case DEMANGLE_COMPONENT_VOLATILE:
      case DEMANGLE_COMPONENT_CONST_THIS:
      case DEMANGLE_COMPONENT_RESTRICT_THIS:
      case DEMANGLE_COMPONENT_VOLATILE_THIS:
      case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
	ret_comp = d_left (ret_comp);
	break;
      default:
	done = true;
	break;
      }

  /* What we have now should be a function.  Return its name.  */
  if (ret_comp->type == DEMANGLE_COMPONENT_TYPED_NAME)
    ret = cp_comp_to_string (d_left (ret_comp), 10);
  else if (!require_params
	   && (ret_comp->type == DEMANGLE_COMPONENT_NAME
	       || ret_comp->type == DEMANGLE_COMPONENT_QUAL_NAME
	       || ret_comp->type == DEMANGLE_COMPONENT_TEMPLATE))
    ret = cp_comp_to_string (ret_comp, 10);

  return ret;
}

/* DEMANGLED_NAME is the name of a function, including parameters and
   (optionally) a return type.  Return the name of the function
   without parameters or return type, or NULL if we can not parse the
   name.  */

gdb::unique_xmalloc_ptr<char>
cp_remove_params (const char *demangled_name)
{
  return cp_remove_params_1 (demangled_name, true);
}

/* See cp-support.h.  */

gdb::unique_xmalloc_ptr<char>
cp_remove_params_if_any (const char *demangled_name, bool completion_mode)
{
  /* Trying to remove parameters from the empty string fails.  If
     we're completing / matching everything, avoid returning NULL
     which would make callers interpret the result as an error.  */
  if (demangled_name[0] == '\0' && completion_mode)
    return make_unique_xstrdup ("");

  gdb::unique_xmalloc_ptr<char> without_params
    = cp_remove_params_1 (demangled_name, false);

  if (without_params == NULL && completion_mode)
    {
      std::string copy = demangled_name;

      while (!copy.empty ())
	{
	  copy.pop_back ();
	  without_params = cp_remove_params_1 (copy.c_str (), false);
	  if (without_params != NULL)
	    break;
	}
    }

  return without_params;
}

/* Here are some random pieces of trivia to keep in mind while trying
   to take apart demangled names:

   - Names can contain function arguments or templates, so the process
     has to be, to some extent recursive: maybe keep track of your
     depth based on encountering <> and ().

   - Parentheses don't just have to happen at the end of a name: they
     can occur even if the name in question isn't a function, because
     a template argument might be a type that's a function.

   - Conversely, even if you're trying to deal with a function, its
     demangled name might not end with ')': it could be a const or
     volatile class method, in which case it ends with "const" or
     "volatile".

   - Parentheses are also used in anonymous namespaces: a variable
     'foo' in an anonymous namespace gets demangled as "(anonymous
     namespace)::foo".

   - And operator names can contain parentheses or angle brackets.  */

/* FIXME: carlton/2003-03-13: We have several functions here with
   overlapping functionality; can we combine them?  Also, do they
   handle all the above considerations correctly?  */


/* This returns the length of first component of NAME, which should be
   the demangled name of a C++ variable/function/method/etc.
   Specifically, it returns the index of the first colon forming the
   boundary of the first component: so, given 'A::foo' or 'A::B::foo'
   it returns the 1, and given 'foo', it returns 0.  */

/* The character in NAME indexed by the return value is guaranteed to
   always be either ':' or '\0'.  */

/* NOTE: carlton/2003-03-13: This function is currently only intended
   for internal use: it's probably not entirely safe when called on
   user-generated input, because some of the 'index += 2' lines in
   cp_find_first_component_aux might go past the end of malformed
   input.  */

unsigned int
cp_find_first_component (const char *name)
{
  return cp_find_first_component_aux (name, 0);
}

/* Helper function for cp_find_first_component.  Like that function,
   it returns the length of the first component of NAME, but to make
   the recursion easier, it also stops if it reaches an unexpected ')'
   or '>' if the value of PERMISSIVE is nonzero.  */

static unsigned int
cp_find_first_component_aux (const char *name, int permissive)
{
  unsigned int index = 0;
  /* Operator names can show up in unexpected places.  Since these can
     contain parentheses or angle brackets, they can screw up the
     recursion.  But not every string 'operator' is part of an
     operator name: e.g. you could have a variable 'cooperator'.  So
     this variable tells us whether or not we should treat the string
     'operator' as starting an operator.  */
  int operator_possible = 1;

  for (;; ++index)
    {
      switch (name[index])
	{
	case '<':
	  /* Template; eat it up.  The calls to cp_first_component
	     should only return (I hope!) when they reach the '>'
	     terminating the component or a '::' between two
	     components.  (Hence the '+ 2'.)  */
	  index += 1;
	  for (index += cp_find_first_component_aux (name + index, 1);
	       name[index] != '>';
	       index += cp_find_first_component_aux (name + index, 1))
	    {
	      if (name[index] != ':')
		{
		  demangled_name_complaint (name);
		  return strlen (name);
		}
	      index += 2;
	    }
	  operator_possible = 1;
	  break;
	case '(':
	  /* Similar comment as to '<'.  */
	  index += 1;
	  for (index += cp_find_first_component_aux (name + index, 1);
	       name[index] != ')';
	       index += cp_find_first_component_aux (name + index, 1))
	    {
	      if (name[index] != ':')
		{
		  demangled_name_complaint (name);
		  return strlen (name);
		}
	      index += 2;
	    }
	  operator_possible = 1;
	  break;
	case '>':
	case ')':
	  if (permissive)
	    return index;
	  else
	    {
	      demangled_name_complaint (name);
	      return strlen (name);
	    }
	case '\0':
	  return index;
	case ':':
	  /* ':' marks a component iff the next character is also a ':'.
	     Otherwise it is probably malformed input.  */
	  if (name[index + 1] == ':')
	    return index;
	  break;
	case 'o':
	  /* Operator names can screw up the recursion.  */
	  if (operator_possible
	      && startswith (name + index, CP_OPERATOR_STR))
	    {
	      index += CP_OPERATOR_LEN;
	      while (ISSPACE(name[index]))
		++index;
	      switch (name[index])
		{
		case '\0':
		  return index;
		  /* Skip over one less than the appropriate number of
		     characters: the for loop will skip over the last
		     one.  */
		case '<':
		  if (name[index + 1] == '<')
		    index += 1;
		  else
		    index += 0;
		  break;
		case '>':
		case '-':
		  if (name[index + 1] == '>')
		    index += 1;
		  else
		    index += 0;
		  break;
		case '(':
		  index += 1;
		  break;
		default:
		  index += 0;
		  break;
		}
	    }
	  operator_possible = 0;
	  break;
	case ' ':
	case ',':
	case '.':
	case '&':
	case '*':
	  /* NOTE: carlton/2003-04-18: I'm not sure what the precise
	     set of relevant characters are here: it's necessary to
	     include any character that can show up before 'operator'
	     in a demangled name, and it's safe to include any
	     character that can't be part of an identifier's name.  */
	  operator_possible = 1;
	  break;
	default:
	  operator_possible = 0;
	  break;
	}
    }
}

/* Complain about a demangled name that we don't know how to parse.
   NAME is the demangled name in question.  */

static void
demangled_name_complaint (const char *name)
{
  complaint ("unexpected demangled name '%s'", name);
}

/* If NAME is the fully-qualified name of a C++
   function/variable/method/etc., this returns the length of its
   entire prefix: all of the namespaces and classes that make up its
   name.  Given 'A::foo', it returns 1, given 'A::B::foo', it returns
   4, given 'foo', it returns 0.  */

unsigned int
cp_entire_prefix_len (const char *name)
{
  unsigned int current_len = cp_find_first_component (name);
  unsigned int previous_len = 0;

  while (name[current_len] != '\0')
    {
      gdb_assert (name[current_len] == ':');
      previous_len = current_len;
      /* Skip the '::'.  */
      current_len += 2;
      current_len += cp_find_first_component (name + current_len);
    }

  return previous_len;
}

/* Overload resolution functions.  */

/* Test to see if SYM is a symbol that we haven't seen corresponding
   to a function named OLOAD_NAME.  If so, add it to
   OVERLOAD_LIST.  */

static void
overload_list_add_symbol (struct symbol *sym,
			  const char *oload_name,
			  std::vector<symbol *> *overload_list)
{
  /* If there is no type information, we can't do anything, so
     skip.  */
  if (sym->type () == NULL)
    return;

  /* skip any symbols that we've already considered.  */
  for (symbol *listed_sym : *overload_list)
    if (strcmp (sym->linkage_name (), listed_sym->linkage_name ()) == 0)
      return;

  /* Get the demangled name without parameters */
  gdb::unique_xmalloc_ptr<char> sym_name
    = cp_remove_params (sym->natural_name ());
  if (!sym_name)
    return;

  /* skip symbols that cannot match */
  if (strcmp (sym_name.get (), oload_name) != 0)
    return;

  overload_list->push_back (sym);
}

/* Return a null-terminated list of pointers to function symbols that
   are named FUNC_NAME and are visible within NAMESPACE.  */

struct std::vector<symbol *>
make_symbol_overload_list (const char *func_name,
			   const char *the_namespace)
{
  const char *name;
  std::vector<symbol *> overload_list;

  overload_list.reserve (100);

  add_symbol_overload_list_using (func_name, the_namespace, &overload_list);

  if (the_namespace[0] == '\0')
    name = func_name;
  else
    {
      char *concatenated_name
	= (char *) alloca (strlen (the_namespace) + 2 + strlen (func_name) + 1);
      strcpy (concatenated_name, the_namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, func_name);
      name = concatenated_name;
    }

  add_symbol_overload_list_qualified (name, &overload_list);
  return overload_list;
}

/* Add all symbols with a name matching NAME in BLOCK to the overload
   list.  */

static void
add_symbol_overload_list_block (const char *name,
				const struct block *block,
				std::vector<symbol *> *overload_list)
{
  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  for (struct symbol *sym : block_iterator_range (block, &lookup_name))
    overload_list_add_symbol (sym, name, overload_list);
}

/* Adds the function FUNC_NAME from NAMESPACE to the overload set.  */

static void
add_symbol_overload_list_namespace (const char *func_name,
				    const char *the_namespace,
				    std::vector<symbol *> *overload_list)
{
  const char *name;
  const struct block *block = NULL;

  if (the_namespace[0] == '\0')
    name = func_name;
  else
    {
      char *concatenated_name
	= (char *) alloca (strlen (the_namespace) + 2 + strlen (func_name) + 1);

      strcpy (concatenated_name, the_namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, func_name);
      name = concatenated_name;
    }

  /* Look in the static block.  */
  block = get_selected_block (0);
  block = block == nullptr ? nullptr : block->static_block ();
  if (block != nullptr)
    {
      add_symbol_overload_list_block (name, block, overload_list);

      /* Look in the global block.  */
      block = block->global_block ();
      if (block)
	add_symbol_overload_list_block (name, block, overload_list);
    }
}

/* Search the namespace of the given type and namespace of and public
   base types.  */

static void
add_symbol_overload_list_adl_namespace (struct type *type,
					const char *func_name,
					std::vector<symbol *> *overload_list)
{
  char *the_namespace;
  const char *type_name;
  int i, prefix_len;

  while (type->is_pointer_or_reference ()
	 || type->code () == TYPE_CODE_ARRAY
	 || type->code () == TYPE_CODE_TYPEDEF)
    {
      if (type->code () == TYPE_CODE_TYPEDEF)
	type = check_typedef (type);
      else
	type = type->target_type ();
    }

  type_name = type->name ();

  if (type_name == NULL)
    return;

  prefix_len = cp_entire_prefix_len (type_name);

  if (prefix_len != 0)
    {
      the_namespace = (char *) alloca (prefix_len + 1);
      strncpy (the_namespace, type_name, prefix_len);
      the_namespace[prefix_len] = '\0';

      add_symbol_overload_list_namespace (func_name, the_namespace,
					  overload_list);
    }

  /* Check public base type */
  if (type->code () == TYPE_CODE_STRUCT)
    for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
      {
	if (BASETYPE_VIA_PUBLIC (type, i))
	  add_symbol_overload_list_adl_namespace (TYPE_BASECLASS (type, i),
						  func_name,
						  overload_list);
      }
}

/* Adds to OVERLOAD_LIST the overload list overload candidates for
   FUNC_NAME found through argument dependent lookup.  */

void
add_symbol_overload_list_adl (gdb::array_view<type *> arg_types,
			      const char *func_name,
			      std::vector<symbol *> *overload_list)
{
  for (type *arg_type : arg_types)
    add_symbol_overload_list_adl_namespace (arg_type, func_name,
					    overload_list);
}

/* This applies the using directives to add namespaces to search in,
   and then searches for overloads in all of those namespaces.  It
   adds the symbols found to sym_return_val.  Arguments are as in
   make_symbol_overload_list.  */

static void
add_symbol_overload_list_using (const char *func_name,
				const char *the_namespace,
				std::vector<symbol *> *overload_list)
{
  struct using_direct *current;
  const struct block *block;

  /* First, go through the using directives.  If any of them apply,
     look in the appropriate namespaces for new functions to match
     on.  */

  for (block = get_selected_block (0);
       block != NULL;
       block = block->superblock ())
    for (current = block->get_using ();
	current != NULL;
	current = current->next)
      {
	/* Prevent recursive calls.  */
	if (current->searched)
	  continue;

	/* If this is a namespace alias or imported declaration ignore
	   it.  */
	if (current->alias != NULL || current->declaration != NULL)
	  continue;

	if (strcmp (the_namespace, current->import_dest) == 0)
	  {
	    /* Mark this import as searched so that the recursive call
	       does not search it again.  */
	    scoped_restore reset_directive_searched
	      = make_scoped_restore (&current->searched, 1);

	    add_symbol_overload_list_using (func_name,
					    current->import_src,
					    overload_list);
	  }
      }

  /* Now, add names for this namespace.  */
  add_symbol_overload_list_namespace (func_name, the_namespace,
				      overload_list);
}

/* This does the bulk of the work of finding overloaded symbols.
   FUNC_NAME is the name of the overloaded function we're looking for
   (possibly including namespace info).  */

static void
add_symbol_overload_list_qualified (const char *func_name,
				    std::vector<symbol *> *overload_list)
{
  const struct block *surrounding_static_block = 0;

  /* Look through the partial symtabs for all symbols which begin by
     matching FUNC_NAME.  Make sure we read that symbol table in.  */

  for (objfile *objf : current_program_space->objfiles ())
    objf->expand_symtabs_for_function (func_name);

  /* Search upwards from currently selected frame (so that we can
     complete on local vars.  */

  for (const block *b = get_selected_block (0);
       b != nullptr;
       b = b->superblock ())
    add_symbol_overload_list_block (func_name, b, overload_list);

  surrounding_static_block = get_selected_block (0);
  surrounding_static_block = (surrounding_static_block == nullptr
			      ? nullptr
			      : surrounding_static_block->static_block ());

  /* Go through the symtabs and check the externs and statics for
     symbols which match.  */

  const block *block = get_selected_block (0);
  struct objfile *current_objfile = block ? block->objfile () : nullptr;

  gdbarch_iterate_over_objfiles_in_search_order
    (current_objfile ? current_objfile->arch () : current_inferior ()->arch (),
     [func_name, surrounding_static_block, &overload_list]
     (struct objfile *obj)
       {
	 for (compunit_symtab *cust : obj->compunits ())
	   {
	     QUIT;
	     const struct block *b = cust->blockvector ()->global_block ();
	     add_symbol_overload_list_block (func_name, b, overload_list);

	     b = cust->blockvector ()->static_block ();
	     /* Don't do this block twice.  */
	     if (b == surrounding_static_block)
	       continue;

	     add_symbol_overload_list_block (func_name, b, overload_list);
	   }

	 return 0;
       }, current_objfile);
}

/* Lookup the rtti type for a class name.  */

struct type *
cp_lookup_rtti_type (const char *name, const struct block *block)
{
  struct symbol * rtti_sym;
  struct type * rtti_type;

  /* Use VAR_DOMAIN here as NAME may be a typedef.  PR 18141, 18417.
     Classes "live" in both STRUCT_DOMAIN and VAR_DOMAIN.  */
  rtti_sym = lookup_symbol (name, block, VAR_DOMAIN, NULL).symbol;

  if (rtti_sym == NULL)
    {
      warning (_("RTTI symbol not found for class '%s'"), name);
      return NULL;
    }

  if (rtti_sym->aclass () != LOC_TYPEDEF)
    {
      warning (_("RTTI symbol for class '%s' is not a type"), name);
      return NULL;
    }

  rtti_type = check_typedef (rtti_sym->type ());

  switch (rtti_type->code ())
    {
    case TYPE_CODE_STRUCT:
      break;
    case TYPE_CODE_NAMESPACE:
      /* chastain/2003-11-26: the symbol tables often contain fake
	 symbols for namespaces with the same name as the struct.
	 This warning is an indication of a bug in the lookup order
	 or a bug in the way that the symbol tables are populated.  */
      warning (_("RTTI symbol for class '%s' is a namespace"), name);
      return NULL;
    default:
      warning (_("RTTI symbol for class '%s' has bad type"), name);
      return NULL;
    }

  return rtti_type;
}

#ifdef HAVE_WORKING_FORK

/* If true, attempt to catch crashes in the demangler and print
   useful debugging information.  */

static bool catch_demangler_crashes = true;

/* Stack context and environment for demangler crash recovery.  */

static thread_local SIGJMP_BUF *gdb_demangle_jmp_buf;

/* If true, attempt to dump core from the signal handler.  */

static std::atomic<bool> gdb_demangle_attempt_core_dump;

/* Signal handler for gdb_demangle.  */

static void
gdb_demangle_signal_handler (int signo)
{
  if (gdb_demangle_attempt_core_dump)
    {
      if (fork () == 0)
	dump_core ();

      gdb_demangle_attempt_core_dump = false;
    }

  SIGLONGJMP (*gdb_demangle_jmp_buf, signo);
}

/* A helper for gdb_demangle that reports a demangling failure.  */

static void
report_failed_demangle (const char *name, bool core_dump_allowed,
			int crash_signal)
{
  static bool error_reported = false;

  if (!error_reported)
    {
      std::string short_msg
	= string_printf (_("unable to demangle '%s' "
			   "(demangler failed with signal %d)"),
			 name, crash_signal);

      std::string long_msg
	= string_printf ("%s:%d: %s: %s", __FILE__, __LINE__,
			 "demangler-warning", short_msg.c_str ());

      target_terminal::scoped_restore_terminal_state term_state;
      target_terminal::ours_for_output ();

      begin_line ();
      if (core_dump_allowed)
	gdb_printf (gdb_stderr,
		    _("%s\nAttempting to dump core.\n"),
		    long_msg.c_str ());
      else
	warn_cant_dump_core (long_msg.c_str ());

      demangler_warning (__FILE__, __LINE__, "%s", short_msg.c_str ());

      error_reported = true;
    }
}

#endif

/* A wrapper for bfd_demangle.  */

gdb::unique_xmalloc_ptr<char>
gdb_demangle (const char *name, int options)
{
  gdb::unique_xmalloc_ptr<char> result;
  int crash_signal = 0;

#ifdef HAVE_WORKING_FORK
  scoped_segv_handler_restore restore_segv
    (catch_demangler_crashes
     ? gdb_demangle_signal_handler
     : nullptr);

  bool core_dump_allowed = gdb_demangle_attempt_core_dump;
  SIGJMP_BUF jmp_buf;
  scoped_restore restore_jmp_buf
    = make_scoped_restore (&gdb_demangle_jmp_buf, &jmp_buf);
  if (catch_demangler_crashes)
    {
      /* The signal handler may keep the signal blocked when we longjmp out
	 of it.  If we have sigprocmask, we can use it to unblock the signal
	 afterwards and we can avoid the performance overhead of saving the
	 signal mask just in case the signal gets triggered.  Otherwise, just
	 tell sigsetjmp to save the mask.  */
#ifdef HAVE_SIGPROCMASK
      crash_signal = SIGSETJMP (*gdb_demangle_jmp_buf, 0);
#else
      crash_signal = SIGSETJMP (*gdb_demangle_jmp_buf, 1);
#endif
    }
#endif

  if (crash_signal == 0)
    result.reset (bfd_demangle (NULL, name, options | DMGL_VERBOSE));

#ifdef HAVE_WORKING_FORK
  if (catch_demangler_crashes)
    {
      if (crash_signal != 0)
	{
#ifdef HAVE_SIGPROCMASK
	  /* If we got the signal, SIGSEGV may still be blocked; restore it.  */
	  sigset_t segv_sig_set;
	  sigemptyset (&segv_sig_set);
	  sigaddset (&segv_sig_set, SIGSEGV);
	  gdb_sigmask (SIG_UNBLOCK, &segv_sig_set, NULL);
#endif

	  /* If there was a failure, we can't report it here, because
	     we might be in a background thread.  Instead, arrange for
	     the reporting to happen on the main thread.  */
	  std::string copy = name;
	  run_on_main_thread ([=, copy = std::move (copy)] ()
	    {
	      report_failed_demangle (copy.c_str (), core_dump_allowed,
				      crash_signal);
	    });

	  result = NULL;
	}
    }
#endif

  return result;
}

/* See cp-support.h.  */

char *
gdb_cplus_demangle_print (int options,
			  struct demangle_component *tree,
			  int estimated_length,
			  size_t *p_allocated_size)
{
  return cplus_demangle_print (options | DMGL_VERBOSE, tree,
			       estimated_length, p_allocated_size);
}

/* A wrapper for cplus_demangle_v3_components that forces
   DMGL_VERBOSE.  */

static struct demangle_component *
gdb_cplus_demangle_v3_components (const char *mangled,
				  int options, void **mem)
{
  return cplus_demangle_v3_components (mangled, options | DMGL_VERBOSE, mem);
}

/* See cp-support.h.  */

unsigned int
cp_search_name_hash (const char *search_name)
{
  /* cp_entire_prefix_len assumes a fully-qualified name with no
     leading "::".  */
  if (startswith (search_name, "::"))
    search_name += 2;

  unsigned int prefix_len = cp_entire_prefix_len (search_name);
  if (prefix_len != 0)
    search_name += prefix_len + 2;

  unsigned int hash = 0;
  for (const char *string = search_name; *string != '\0'; ++string)
    {
      string = skip_spaces (string);

      if (*string == '(')
	break;

      /* Ignore ABI tags such as "[abi:cxx11].  */
      if (*string == '['
	  && startswith (string + 1, "abi:")
	  && string[5] != ':')
	break;

      /* Ignore template parameter lists.  */
      if (string[0] == '<'
	  && string[1] != '(' && string[1] != '<' && string[1] != '='
	  && string[1] != ' ' && string[1] != '\0')
	break;

      hash = SYMBOL_HASH_NEXT (hash, *string);
    }
  return hash;
}

/* Helper for cp_symbol_name_matches (i.e., symbol_name_matcher_ftype
   implementation for symbol_name_match_type::WILD matching).  Split
   to a separate function for unit-testing convenience.

   If SYMBOL_SEARCH_NAME has more scopes than LOOKUP_NAME, we try to
   match ignoring the extra leading scopes of SYMBOL_SEARCH_NAME.
   This allows conveniently setting breakpoints on functions/methods
   inside any namespace/class without specifying the fully-qualified
   name.

   E.g., these match:

    [symbol search name]   [lookup name]
    foo::bar::func         foo::bar::func
    foo::bar::func         bar::func
    foo::bar::func         func

   While these don't:

    [symbol search name]   [lookup name]
    foo::zbar::func        bar::func
    foo::bar::func         foo::func

   See more examples in the test_cp_symbol_name_matches selftest
   function below.

   See symbol_name_matcher_ftype for description of SYMBOL_SEARCH_NAME
   and COMP_MATCH_RES.

   LOOKUP_NAME/LOOKUP_NAME_LEN is the name we're looking up.

   See strncmp_iw_with_mode for description of MODE.
*/

static bool
cp_symbol_name_matches_1 (const char *symbol_search_name,
			  const char *lookup_name,
			  size_t lookup_name_len,
			  strncmp_iw_mode mode,
			  completion_match_result *comp_match_res)
{
  const char *sname = symbol_search_name;
  completion_match_for_lcd *match_for_lcd
    = (comp_match_res != NULL ? &comp_match_res->match_for_lcd : NULL);

  gdb_assert (match_for_lcd == nullptr || match_for_lcd->empty ());

  while (true)
    {
      if (strncmp_iw_with_mode (sname, lookup_name, lookup_name_len,
				mode, language_cplus, match_for_lcd, true) == 0)
	{
	  if (comp_match_res != NULL)
	    {
	      /* Note here we set different MATCH and MATCH_FOR_LCD
		 strings.  This is because with

		  (gdb) b push_bac[TAB]

		 we want the completion matches to list

		  std::vector<int>::push_back(...)
		  std::vector<char>::push_back(...)

		 etc., which are SYMBOL_SEARCH_NAMEs, while we want
		 the input line to auto-complete to

		  (gdb) push_back(...)

		 which is SNAME, not to

		  (gdb) std::vector<

		 which would be the regular common prefix between all
		 the matches otherwise.  */
	      comp_match_res->set_match (symbol_search_name, sname);
	    }
	  return true;
	}

      /* Clear match_for_lcd so the next strncmp_iw_with_mode call starts
	 from scratch.  */
      if (match_for_lcd != nullptr)
	match_for_lcd->clear ();

      unsigned int len = cp_find_first_component (sname);

      if (sname[len] == '\0')
	return false;

      gdb_assert (sname[len] == ':');
      /* Skip the '::'.  */
      sname += len + 2;
    }
}

/* C++ symbol_name_matcher_ftype implementation.  */

static bool
cp_fq_symbol_name_matches (const char *symbol_search_name,
			   const lookup_name_info &lookup_name,
			   completion_match_result *comp_match_res)
{
  /* Get the demangled name.  */
  const std::string &name = lookup_name.cplus ().lookup_name ();
  completion_match_for_lcd *match_for_lcd
    = (comp_match_res != NULL ? &comp_match_res->match_for_lcd : NULL);
  strncmp_iw_mode mode = (lookup_name.completion_mode ()
			  ? strncmp_iw_mode::NORMAL
			  : strncmp_iw_mode::MATCH_PARAMS);

  if (strncmp_iw_with_mode (symbol_search_name,
			    name.c_str (), name.size (),
			    mode, language_cplus, match_for_lcd) == 0)
    {
      if (comp_match_res != NULL)
	comp_match_res->set_match (symbol_search_name);
      return true;
    }

  return false;
}

/* C++ symbol_name_matcher_ftype implementation for wild matches.
   Defers work to cp_symbol_name_matches_1.  */

static bool
cp_symbol_name_matches (const char *symbol_search_name,
			const lookup_name_info &lookup_name,
			completion_match_result *comp_match_res)
{
  /* Get the demangled name.  */
  const std::string &name = lookup_name.cplus ().lookup_name ();

  strncmp_iw_mode mode = (lookup_name.completion_mode ()
			  ? strncmp_iw_mode::NORMAL
			  : strncmp_iw_mode::MATCH_PARAMS);

  return cp_symbol_name_matches_1 (symbol_search_name,
				   name.c_str (), name.size (),
				   mode, comp_match_res);
}

/* See cp-support.h.  */

symbol_name_matcher_ftype *
cp_get_symbol_name_matcher (const lookup_name_info &lookup_name)
{
  switch (lookup_name.match_type ())
    {
    case symbol_name_match_type::FULL:
    case symbol_name_match_type::EXPRESSION:
    case symbol_name_match_type::SEARCH_NAME:
      return cp_fq_symbol_name_matches;
    case symbol_name_match_type::WILD:
      return cp_symbol_name_matches;
    }

  gdb_assert_not_reached ("");
}

#if GDB_SELF_TEST

namespace selftests {

static void
test_cp_symbol_name_matches ()
{
#define CHECK_MATCH(SYMBOL, INPUT)					\
  SELF_CHECK (cp_symbol_name_matches_1 (SYMBOL,				\
					INPUT, sizeof (INPUT) - 1,	\
					strncmp_iw_mode::MATCH_PARAMS,	\
					NULL))

#define CHECK_NOT_MATCH(SYMBOL, INPUT)					\
  SELF_CHECK (!cp_symbol_name_matches_1 (SYMBOL,			\
					 INPUT, sizeof (INPUT) - 1,	\
					 strncmp_iw_mode::MATCH_PARAMS,	\
					 NULL))

  /* Like CHECK_MATCH, and also check that INPUT (and all substrings
     that start at index 0) completes to SYMBOL.  */
#define CHECK_MATCH_C(SYMBOL, INPUT)					\
  do									\
    {									\
      CHECK_MATCH (SYMBOL, INPUT);					\
      for (size_t i = 0; i < sizeof (INPUT) - 1; i++)			\
	SELF_CHECK (cp_symbol_name_matches_1 (SYMBOL, INPUT, i,		\
					      strncmp_iw_mode::NORMAL,	\
					      NULL));			\
    } while (0)

  /* Like CHECK_NOT_MATCH, and also check that INPUT does NOT complete
     to SYMBOL.  */
#define CHECK_NOT_MATCH_C(SYMBOL, INPUT)				\
  do									\
    { 									\
      CHECK_NOT_MATCH (SYMBOL, INPUT);					\
      SELF_CHECK (!cp_symbol_name_matches_1 (SYMBOL, INPUT,		\
					     sizeof (INPUT) - 1,	\
					     strncmp_iw_mode::NORMAL,	\
					     NULL));			\
    } while (0)

  /* Lookup name without parens matches all overloads.  */
  CHECK_MATCH_C ("function()", "function");
  CHECK_MATCH_C ("function(int)", "function");

  /* Check whitespace around parameters is ignored.  */
  CHECK_MATCH_C ("function()", "function ()");
  CHECK_MATCH_C ("function ( )", "function()");
  CHECK_MATCH_C ("function ()", "function( )");
  CHECK_MATCH_C ("func(int)", "func( int )");
  CHECK_MATCH_C ("func(int)", "func ( int ) ");
  CHECK_MATCH_C ("func ( int )", "func( int )");
  CHECK_MATCH_C ("func ( int )", "func ( int ) ");

  /* Check symbol name prefixes aren't incorrectly matched.  */
  CHECK_NOT_MATCH ("func", "function");
  CHECK_NOT_MATCH ("function", "func");
  CHECK_NOT_MATCH ("function()", "func");

  /* Check that if the lookup name includes parameters, only the right
     overload matches.  */
  CHECK_MATCH_C ("function(int)", "function(int)");
  CHECK_NOT_MATCH_C ("function(int)", "function()");

  /* Check that whitespace within symbol names is not ignored.  */
  CHECK_NOT_MATCH_C ("function", "func tion");
  CHECK_NOT_MATCH_C ("func__tion", "func_ _tion");
  CHECK_NOT_MATCH_C ("func11tion", "func1 1tion");

  /* Check the converse, which can happen with template function,
     where the return type is part of the demangled name.  */
  CHECK_NOT_MATCH_C ("func tion", "function");
  CHECK_NOT_MATCH_C ("func1 1tion", "func11tion");
  CHECK_NOT_MATCH_C ("func_ _tion", "func__tion");

  /* Within parameters too.  */
  CHECK_NOT_MATCH_C ("func(param)", "func(par am)");

  /* Check handling of whitespace around C++ operators.  */
  CHECK_NOT_MATCH_C ("operator<<", "opera tor<<");
  CHECK_NOT_MATCH_C ("operator<<", "operator< <");
  CHECK_NOT_MATCH_C ("operator<<", "operator < <");
  CHECK_NOT_MATCH_C ("operator==", "operator= =");
  CHECK_NOT_MATCH_C ("operator==", "operator = =");
  CHECK_MATCH_C ("operator<<", "operator <<");
  CHECK_MATCH_C ("operator<<()", "operator <<");
  CHECK_NOT_MATCH_C ("operator<<()", "operator<<(int)");
  CHECK_NOT_MATCH_C ("operator<<(int)", "operator<<()");
  CHECK_MATCH_C ("operator==", "operator ==");
  CHECK_MATCH_C ("operator==()", "operator ==");
  CHECK_MATCH_C ("operator <<", "operator<<");
  CHECK_MATCH_C ("operator ==", "operator==");
  CHECK_MATCH_C ("operator bool", "operator  bool");
  CHECK_MATCH_C ("operator bool ()", "operator  bool");
  CHECK_MATCH_C ("operatorX<<", "operatorX < <");
  CHECK_MATCH_C ("Xoperator<<", "Xoperator < <");

  CHECK_MATCH_C ("operator()(int)", "operator()(int)");
  CHECK_MATCH_C ("operator()(int)", "operator ( ) ( int )");
  CHECK_MATCH_C ("operator()<long>(int)", "operator ( ) < long > ( int )");
  /* The first "()" is not the parameter list.  */
  CHECK_NOT_MATCH ("operator()(int)", "operator");

  /* Misc user-defined operator tests.  */

  CHECK_NOT_MATCH_C ("operator/=()", "operator ^=");
  /* Same length at end of input.  */
  CHECK_NOT_MATCH_C ("operator>>", "operator[]");
  /* Same length but not at end of input.  */
  CHECK_NOT_MATCH_C ("operator>>()", "operator[]()");

  CHECK_MATCH_C ("base::operator char*()", "base::operator char*()");
  CHECK_MATCH_C ("base::operator char*()", "base::operator char * ()");
  CHECK_MATCH_C ("base::operator char**()", "base::operator char * * ()");
  CHECK_MATCH ("base::operator char**()", "base::operator char * *");
  CHECK_MATCH_C ("base::operator*()", "base::operator*()");
  CHECK_NOT_MATCH_C ("base::operator char*()", "base::operatorc");
  CHECK_NOT_MATCH ("base::operator char*()", "base::operator char");
  CHECK_NOT_MATCH ("base::operator char*()", "base::operat");

  /* Check handling of whitespace around C++ scope operators.  */
  CHECK_NOT_MATCH_C ("foo::bar", "foo: :bar");
  CHECK_MATCH_C ("foo::bar", "foo :: bar");
  CHECK_MATCH_C ("foo :: bar", "foo::bar");

  CHECK_MATCH_C ("abc::def::ghi()", "abc::def::ghi()");
  CHECK_MATCH_C ("abc::def::ghi ( )", "abc::def::ghi()");
  CHECK_MATCH_C ("abc::def::ghi()", "abc::def::ghi ( )");
  CHECK_MATCH_C ("function()", "function()");
  CHECK_MATCH_C ("bar::function()", "bar::function()");

  /* Wild matching tests follow.  */

  /* Tests matching symbols in some scope.  */
  CHECK_MATCH_C ("foo::function()", "function");
  CHECK_MATCH_C ("foo::function(int)", "function");
  CHECK_MATCH_C ("foo::bar::function()", "function");
  CHECK_MATCH_C ("bar::function()", "bar::function");
  CHECK_MATCH_C ("foo::bar::function()", "bar::function");
  CHECK_MATCH_C ("foo::bar::function(int)", "bar::function");

  /* Same, with parameters in the lookup name.  */
  CHECK_MATCH_C ("foo::function()", "function()");
  CHECK_MATCH_C ("foo::bar::function()", "function()");
  CHECK_MATCH_C ("foo::function(int)", "function(int)");
  CHECK_MATCH_C ("foo::function()", "foo::function()");
  CHECK_MATCH_C ("foo::bar::function()", "bar::function()");
  CHECK_MATCH_C ("foo::bar::function(int)", "bar::function(int)");
  CHECK_MATCH_C ("bar::function()", "bar::function()");

  CHECK_NOT_MATCH_C ("foo::bar::function(int)", "bar::function()");

  CHECK_MATCH_C ("(anonymous namespace)::bar::function(int)",
		 "bar::function(int)");
  CHECK_MATCH_C ("foo::(anonymous namespace)::bar::function(int)",
		 "function(int)");

  /* Lookup scope wider than symbol scope, should not match.  */
  CHECK_NOT_MATCH_C ("function()", "bar::function");
  CHECK_NOT_MATCH_C ("function()", "bar::function()");

  /* Explicit global scope doesn't match.  */
  CHECK_NOT_MATCH_C ("foo::function()", "::function");
  CHECK_NOT_MATCH_C ("foo::function()", "::function()");
  CHECK_NOT_MATCH_C ("foo::function(int)", "::function()");
  CHECK_NOT_MATCH_C ("foo::function(int)", "::function(int)");

  /* Test ABI tag matching/ignoring.  */

  /* If the symbol name has an ABI tag, but the lookup name doesn't,
     then the ABI tag in the symbol name is ignored.  */
  CHECK_MATCH_C ("function[abi:foo]()", "function");
  CHECK_MATCH_C ("function[abi:foo](int)", "function");
  CHECK_MATCH_C ("function[abi:foo]()", "function ()");
  CHECK_NOT_MATCH_C ("function[abi:foo]()", "function (int)");

  CHECK_MATCH_C ("function[abi:foo]()", "function[abi:foo]");
  CHECK_MATCH_C ("function[abi:foo](int)", "function[abi:foo]");
  CHECK_MATCH_C ("function[abi:foo]()", "function[abi:foo] ()");
  CHECK_MATCH_C ("function[abi:foo][abi:bar]()", "function");
  CHECK_MATCH_C ("function[abi:foo][abi:bar](int)", "function");
  CHECK_MATCH_C ("function[abi:foo][abi:bar]()", "function[abi:foo]");
  CHECK_MATCH_C ("function[abi:foo][abi:bar](int)", "function[abi:foo]");
  CHECK_MATCH_C ("function[abi:foo][abi:bar]()", "function[abi:foo] ()");
  CHECK_NOT_MATCH_C ("function[abi:foo][abi:bar]()", "function[abi:foo] (int)");

  CHECK_MATCH_C ("function  [abi:foo][abi:bar] ( )", "function [abi:foo]");

  /* If the symbol name does not have an ABI tag, while the lookup
     name has one, then there's no match.  */
  CHECK_NOT_MATCH_C ("function()", "function[abi:foo]()");
  CHECK_NOT_MATCH_C ("function()", "function[abi:foo]");
}

/* If non-NULL, return STR wrapped in quotes.  Otherwise, return a
   "<null>" string (with no quotes).  */

static std::string
quote (const char *str)
{
  if (str != NULL)
    return std::string (1, '"') + str + '"';
  else
    return "<null>";
}

/* Check that removing parameter info out of NAME produces EXPECTED.
   COMPLETION_MODE indicates whether we're testing normal and
   completion mode.  FILE and LINE are used to provide better test
   location information in case the check fails.  */

static void
check_remove_params (const char *file, int line,
		      const char *name, const char *expected,
		      bool completion_mode)
{
  gdb::unique_xmalloc_ptr<char> result
    = cp_remove_params_if_any (name, completion_mode);

  if ((expected == NULL) != (result == NULL)
      || (expected != NULL
	  && strcmp (result.get (), expected) != 0))
    {
      error (_("%s:%d: make-paramless self-test failed: (completion=%d) "
	       "\"%s\" -> %s, expected %s"),
	     file, line, completion_mode, name,
	     quote (result.get ()).c_str (), quote (expected).c_str ());
    }
}

/* Entry point for cp_remove_params unit tests.  */

static void
test_cp_remove_params ()
{
  /* Check that removing parameter info out of NAME produces EXPECTED.
     Checks both normal and completion modes.  */
#define CHECK(NAME, EXPECTED)						\
  do									\
    {									\
      check_remove_params (__FILE__, __LINE__, NAME, EXPECTED, false);	\
      check_remove_params (__FILE__, __LINE__, NAME, EXPECTED, true);	\
    }									\
  while (0)

  /* Similar, but used when NAME is incomplete -- i.e., is has
     unbalanced parentheses.  In this case, looking for the exact name
     should fail / return empty.  */
#define CHECK_INCOMPL(NAME, EXPECTED)					\
  do									\
    {									\
      check_remove_params (__FILE__, __LINE__, NAME, NULL, false);	\
      check_remove_params (__FILE__, __LINE__, NAME, EXPECTED, true);	\
    }									\
  while (0)

  CHECK ("function()", "function");
  CHECK_INCOMPL ("function(", "function");
  CHECK ("function() const", "function");

  CHECK ("(anonymous namespace)::A::B::C",
	 "(anonymous namespace)::A::B::C");

  CHECK ("A::(anonymous namespace)",
	 "A::(anonymous namespace)");

  CHECK_INCOMPL ("A::(anonymou", "A");

  CHECK ("A::foo<int>()",
	 "A::foo<int>");

  CHECK_INCOMPL ("A::foo<int>(",
		 "A::foo<int>");

  CHECK ("A::foo<(anonymous namespace)::B>::func(int)",
	 "A::foo<(anonymous namespace)::B>::func");

  CHECK_INCOMPL ("A::foo<(anonymous namespace)::B>::func(in",
		 "A::foo<(anonymous namespace)::B>::func");

  CHECK_INCOMPL ("A::foo<(anonymous namespace)::B>::",
		 "A::foo<(anonymous namespace)::B>");

  CHECK_INCOMPL ("A::foo<(anonymous namespace)::B>:",
		 "A::foo<(anonymous namespace)::B>");

  CHECK ("A::foo<(anonymous namespace)::B>",
	 "A::foo<(anonymous namespace)::B>");

  CHECK_INCOMPL ("A::foo<(anonymous namespace)::B",
		 "A::foo");

  /* Shouldn't this parse?  Looks like a bug in
     cp_demangled_name_to_comp.  See PR c++/22411.  */
#if 0
  CHECK ("A::foo<void(int)>::func(int)",
	 "A::foo<void(int)>::func");
#else
  CHECK_INCOMPL ("A::foo<void(int)>::func(int)",
		 "A::foo");
#endif

  CHECK_INCOMPL ("A::foo<void(int",
		 "A::foo");

#undef CHECK
#undef CHECK_INCOMPL
}

} // namespace selftests

#endif /* GDB_SELF_CHECK */

/* This is a front end for cp_find_first_component, for unit testing.
   Be careful when using it: see the NOTE above
   cp_find_first_component.  */

static void
first_component_command (const char *arg, int from_tty)
{
  int len;  
  char *prefix; 

  if (!arg)
    return;

  len = cp_find_first_component (arg);
  prefix = (char *) alloca (len + 1);

  memcpy (prefix, arg, len);
  prefix[len] = '\0';

  gdb_printf ("%s\n", prefix);
}

/* Implement "info vtbl".  */

static void
info_vtbl_command (const char *arg, int from_tty)
{
  struct value *value;

  value = parse_and_eval (arg);
  cplus_print_vtable (value);
}

/* See description in cp-support.h.  */

const char *
find_toplevel_char (const char *s, char c)
{
  int quoted = 0;		/* zero if we're not in quotes;
				   '"' if we're in a double-quoted string;
				   '\'' if we're in a single-quoted string.  */
  int depth = 0;		/* Number of unclosed parens we've seen.  */
  const char *scan;

  for (scan = s; *scan; scan++)
    {
      if (quoted)
	{
	  if (*scan == quoted)
	    quoted = 0;
	  else if (*scan == '\\' && *(scan + 1))
	    scan++;
	}
      else if (*scan == c && ! quoted && depth == 0)
	return scan;
      else if (*scan == '"' || *scan == '\'')
	quoted = *scan;
      else if (*scan == '(' || *scan == '<')
	depth++;
      else if ((*scan == ')' || *scan == '>') && depth > 0)
	depth--;
      else if (*scan == 'o' && !quoted && depth == 0)
	{
	  /* Handle C++ operator names.  */
	  if (strncmp (scan, CP_OPERATOR_STR, CP_OPERATOR_LEN) == 0)
	    {
	      scan += CP_OPERATOR_LEN;
	      if (*scan == c)
		return scan;
	      while (ISSPACE (*scan))
		{
		  ++scan;
		  if (*scan == c)
		    return scan;
		}
	      if (*scan == '\0')
		break;

	      switch (*scan)
		{
		  /* Skip over one less than the appropriate number of
		     characters: the for loop will skip over the last
		     one.  */
		case '<':
		  if (scan[1] == '<')
		    {
		      scan++;
		      if (*scan == c)
			return scan;
		    }
		  break;
		case '>':
		  if (scan[1] == '>')
		    {
		      scan++;
		      if (*scan == c)
			return scan;
		    }
		  break;
		}
	    }
	}
    }

  return 0;
}

void _initialize_cp_support ();
void
_initialize_cp_support ()
{
  cmd_list_element *maintenance_cplus
    = add_basic_prefix_cmd ("cplus", class_maintenance,
			    _("C++ maintenance commands."),
			    &maint_cplus_cmd_list,
			    0, &maintenancelist);
  add_alias_cmd ("cp", maintenance_cplus, class_maintenance, 1,
		 &maintenancelist);

  add_cmd ("first_component",
	   class_maintenance,
	   first_component_command,
	   _("Print the first class/namespace component of NAME."),
	   &maint_cplus_cmd_list);

  add_info ("vtbl", info_vtbl_command,
	    _("Show the virtual function table for a C++ object.\n\
Usage: info vtbl EXPRESSION\n\
Evaluate EXPRESSION and display the virtual function table for the\n\
resulting object."));

#ifdef HAVE_WORKING_FORK
  add_setshow_boolean_cmd ("catch-demangler-crashes", class_maintenance,
			   &catch_demangler_crashes, _("\
Set whether to attempt to catch demangler crashes."), _("\
Show whether to attempt to catch demangler crashes."), _("\
If enabled GDB will attempt to catch demangler crashes and\n\
display the offending symbol."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  gdb_demangle_attempt_core_dump = can_dump_core (LIMIT_CUR);
#endif

#if GDB_SELF_TEST
  selftests::register_test ("cp_symbol_name_matches",
			    selftests::test_cp_symbol_name_matches);
  selftests::register_test ("cp_remove_params",
			    selftests::test_cp_remove_params);
#endif
}

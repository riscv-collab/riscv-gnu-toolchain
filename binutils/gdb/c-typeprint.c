/* Support for printing C and C++ types for GDB, the GNU debugger.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/gdb_obstack.h"
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "target.h"
#include "language.h"
#include "demangle.h"
#include "c-lang.h"
#include "cli/cli-style.h"
#include "typeprint.h"
#include "cp-abi.h"
#include "cp-support.h"

static void c_type_print_varspec_suffix (struct type *, struct ui_file *, int,
					 int, int,
					 enum language,
					 const struct type_print_options *);

static void c_type_print_varspec_prefix (struct type *,
					 struct ui_file *,
					 int, int, int,
					 enum language,
					 const struct type_print_options *,
					 struct print_offset_data *);

/* Print "const", "volatile", or address space modifiers.  */
static void c_type_print_modifier (struct type *,
				   struct ui_file *,
				   int, int, enum language);

static void c_type_print_base_1 (struct type *type, struct ui_file *stream,
				 int show, int level, enum language language,
				 const struct type_print_options *flags,
				 struct print_offset_data *podata);


/* A callback function for cp_canonicalize_string_full that uses
   typedef_hash_table::find_typedef.  */

static const char *
find_typedef_for_canonicalize (struct type *t, void *data)
{
  return typedef_hash_table::find_typedef
    ((const struct type_print_options *) data, t);
}

/* Print NAME on STREAM.  If the 'raw' field of FLAGS is not set,
   canonicalize NAME using the local typedefs first.  */

static void
print_name_maybe_canonical (const char *name,
			    const struct type_print_options *flags,
			    struct ui_file *stream)
{
  gdb::unique_xmalloc_ptr<char> s;

  if (!flags->raw)
    s = cp_canonicalize_string_full (name,
				     find_typedef_for_canonicalize,
				     (void *) flags);

  gdb_puts (s != nullptr ? s.get () : name, stream);
}



/* Helper function for c_print_type.  */

static void
c_print_type_1 (struct type *type,
		const char *varstring,
		struct ui_file *stream,
		int show, int level,
		enum language language,
		const struct type_print_options *flags,
		struct print_offset_data *podata)
{
  enum type_code code;
  int demangled_args;
  int need_post_space;
  const char *local_name;

  if (show > 0)
    type = check_typedef (type);

  local_name = typedef_hash_table::find_typedef (flags, type);
  code = type->code ();
  if (local_name != NULL)
    {
      c_type_print_modifier (type, stream, 0, 1, language);
      gdb_puts (local_name, stream);
      if (varstring != NULL && *varstring != '\0')
	gdb_puts (" ", stream);
    }
  else
    {
      c_type_print_base_1 (type, stream, show, level, language, flags, podata);
      if ((varstring != NULL && *varstring != '\0')
	  /* Need a space if going to print stars or brackets;
	     but not if we will print just a type name.  */
	  || ((show > 0 || type->name () == 0)
	      && (code == TYPE_CODE_PTR || code == TYPE_CODE_FUNC
		  || code == TYPE_CODE_METHOD
		  || (code == TYPE_CODE_ARRAY
		      && !type->is_vector ())
		  || code == TYPE_CODE_MEMBERPTR
		  || code == TYPE_CODE_METHODPTR
		  || TYPE_IS_REFERENCE (type))))
	gdb_puts (" ", stream);
      need_post_space = (varstring != NULL && strcmp (varstring, "") != 0);
      c_type_print_varspec_prefix (type, stream, show, 0, need_post_space,
				   language, flags, podata);
    }

  if (varstring != NULL)
    {
      if (code == TYPE_CODE_FUNC || code == TYPE_CODE_METHOD)
	fputs_styled (varstring, function_name_style.style (), stream);
      else
	fputs_styled (varstring, variable_name_style.style (), stream);

      /* For demangled function names, we have the arglist as part of
	 the name, so don't print an additional pair of ()'s.  */
      if (local_name == NULL)
	{
	  demangled_args = strchr (varstring, '(') != NULL;
	  c_type_print_varspec_suffix (type, stream, show,
				       0, demangled_args,
				       language, flags);
	}
    }
}

/* See c-lang.h.  */

void
c_print_type (struct type *type,
	      const char *varstring,
	      struct ui_file *stream,
	      int show, int level,
	      enum language language,
	      const struct type_print_options *flags)
{
  struct print_offset_data podata (flags);

  c_print_type_1 (type, varstring, stream, show, level, language, flags,
		  &podata);
}

/* Print a typedef using C syntax.  TYPE is the underlying type.
   NEW_SYMBOL is the symbol naming the type.  STREAM is the stream on
   which to print.  */

void
c_print_typedef (struct type *type,
		 struct symbol *new_symbol,
		 struct ui_file *stream)
{
  type = check_typedef (type);
  gdb_printf (stream, "typedef ");
  type_print (type, "", stream, -1);
  if ((new_symbol->type ())->name () == 0
      || strcmp ((new_symbol->type ())->name (),
		 new_symbol->linkage_name ()) != 0
      || new_symbol->type ()->code () == TYPE_CODE_TYPEDEF)
    gdb_printf (stream, " %s", new_symbol->print_name ());
  gdb_printf (stream, ";");
}

/* If TYPE is a derived type, then print out derivation information.
   Print only the actual base classes of this type, not the base
   classes of the base classes.  I.e. for the derivation hierarchy:

   class A { int a; };
   class B : public A {int b; };
   class C : public B {int c; };

   Print the type of class C as:

   class C : public B {
   int c;
   }

   Not as the following (like gdb used to), which is not legal C++
   syntax for derived types and may be confused with the multiple
   inheritance form:

   class C : public B : public A {
   int c;
   }

   In general, gdb should try to print the types as closely as
   possible to the form that they appear in the source code.  */

static void
cp_type_print_derivation_info (struct ui_file *stream,
			       struct type *type,
			       const struct type_print_options *flags)
{
  const char *name;
  int i;

  for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
    {
      stream->wrap_here (8);
      gdb_puts (i == 0 ? ": " : ", ", stream);
      gdb_printf (stream, "%s%s ",
		  BASETYPE_VIA_PUBLIC (type, i)
		  ? "public" : (type->field (i).is_protected ()
				? "protected" : "private"),
		  BASETYPE_VIA_VIRTUAL (type, i) ? " virtual" : "");
      name = TYPE_BASECLASS (type, i)->name ();
      if (name)
	print_name_maybe_canonical (name, flags, stream);
      else
	gdb_printf (stream, "(null)");
    }
  if (i > 0)
    {
      gdb_puts (" ", stream);
    }
}

/* Print the C++ method arguments ARGS to the file STREAM.  */

static void
cp_type_print_method_args (struct type *mtype, const char *prefix,
			   const char *varstring, int staticp,
			   struct ui_file *stream,
			   enum language language,
			   const struct type_print_options *flags)
{
  struct field *args = mtype->fields ();
  int nargs = mtype->num_fields ();
  int varargs = mtype->has_varargs ();
  int i;

  fprintf_symbol (stream, prefix,
		  language_cplus, DMGL_ANSI);
  fprintf_symbol (stream, varstring,
		  language_cplus, DMGL_ANSI);
  gdb_puts ("(", stream);

  int printed_args = 0;
  for (i = 0; i < nargs; ++i)
    {
      if (i == 0 && !staticp)
	{
	  /* Skip the class variable.  We keep this here to accommodate older
	     compilers and debug formats which may not support artificial
	     parameters.  */
	  continue;
	}

      struct field arg = args[i];
      /* Skip any artificial arguments.  */
      if (arg.is_artificial ())
	continue;

      if (printed_args > 0)
	{
	  gdb_printf (stream, ", ");
	  stream->wrap_here (8);
	}

      c_print_type (arg.type (), "", stream, 0, 0, language, flags);
      printed_args++;
    }

  if (varargs)
    {
      if (printed_args == 0)
	gdb_printf (stream, "...");
      else
	gdb_printf (stream, ", ...");
    }
  else if (printed_args == 0)
    {
      if (language == language_cplus)
	gdb_printf (stream, "void");
    }

  gdb_printf (stream, ")");

  /* For non-static methods, read qualifiers from the type of
     THIS.  */
  if (!staticp)
    {
      struct type *domain;

      gdb_assert (nargs > 0);
      gdb_assert (args[0].type ()->code () == TYPE_CODE_PTR);
      domain = args[0].type ()->target_type ();

      if (TYPE_CONST (domain))
	gdb_printf (stream, " const");

      if (TYPE_VOLATILE (domain))
	gdb_printf (stream, " volatile");

      if (TYPE_RESTRICT (domain))
	gdb_printf (stream, (language == language_cplus
			     ? " __restrict__"
			     : " restrict"));

      if (TYPE_ATOMIC (domain))
	gdb_printf (stream, " _Atomic");
    }
}


/* Print any asterisks or open-parentheses needed before the
   variable name (to describe its type).

   On outermost call, pass 0 for PASSED_A_PTR.
   On outermost call, SHOW > 0 means should ignore
   any typename for TYPE and show its details.
   SHOW is always zero on recursive calls.
   
   NEED_POST_SPACE is non-zero when a space will be be needed
   between a trailing qualifier and a field, variable, or function
   name.  */

static void
c_type_print_varspec_prefix (struct type *type,
			     struct ui_file *stream,
			     int show, int passed_a_ptr,
			     int need_post_space,
			     enum language language,
			     const struct type_print_options *flags,
			     struct print_offset_data *podata)
{
  const char *name;

  if (type == 0)
    return;

  if (type->name () && show <= 0)
    return;

  QUIT;

  switch (type->code ())
    {
    case TYPE_CODE_PTR:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, 1, 1, language, flags,
				   podata);
      gdb_printf (stream, "*");
      c_type_print_modifier (type, stream, 1, need_post_space, language);
      break;

    case TYPE_CODE_MEMBERPTR:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, 0, 0, language, flags, podata);
      name = TYPE_SELF_TYPE (type)->name ();
      if (name)
	print_name_maybe_canonical (name, flags, stream);
      else
	c_type_print_base_1 (TYPE_SELF_TYPE (type),
			     stream, -1, passed_a_ptr, language, flags,
			     podata);
      gdb_printf (stream, "::*");
      break;

    case TYPE_CODE_METHODPTR:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, 0, 0, language, flags,
				   podata);
      gdb_printf (stream, "(");
      name = TYPE_SELF_TYPE (type)->name ();
      if (name)
	print_name_maybe_canonical (name, flags, stream);
      else
	c_type_print_base_1 (TYPE_SELF_TYPE (type),
			     stream, -1, passed_a_ptr, language, flags,
			     podata);
      gdb_printf (stream, "::*");
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, 1, 0, language, flags,
				   podata);
      gdb_printf (stream, type->code () == TYPE_CODE_REF ? "&" : "&&");
      c_type_print_modifier (type, stream, 1, need_post_space, language);
      break;

    case TYPE_CODE_METHOD:
    case TYPE_CODE_FUNC:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, 0, 0, language, flags,
				   podata);
      if (passed_a_ptr)
	gdb_printf (stream, "(");
      break;

    case TYPE_CODE_ARRAY:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, 0, need_post_space,
				   language, flags, podata);
      if (passed_a_ptr)
	gdb_printf (stream, "(");
      break;

    case TYPE_CODE_TYPEDEF:
      c_type_print_varspec_prefix (type->target_type (),
				   stream, show, passed_a_ptr, 0,
				   language, flags, podata);
      break;
    }
}

/* Print out "const" and "volatile" attributes,
   and address space id if present.
   TYPE is a pointer to the type being printed out.
   STREAM is the output destination.
   NEED_PRE_SPACE = 1 indicates an initial white space is needed.
   NEED_POST_SPACE = 1 indicates a final white space is needed.  */

static void
c_type_print_modifier (struct type *type, struct ui_file *stream,
		       int need_pre_space, int need_post_space,
		       enum language language)
{
  int did_print_modifier = 0;
  const char *address_space_id;

  /* We don't print `const' qualifiers for references --- since all
     operators affect the thing referenced, not the reference itself,
     every reference is `const'.  */
  if (TYPE_CONST (type) && !TYPE_IS_REFERENCE (type))
    {
      if (need_pre_space)
	gdb_printf (stream, " ");
      gdb_printf (stream, "const");
      did_print_modifier = 1;
    }

  if (TYPE_VOLATILE (type))
    {
      if (did_print_modifier || need_pre_space)
	gdb_printf (stream, " ");
      gdb_printf (stream, "volatile");
      did_print_modifier = 1;
    }

  if (TYPE_RESTRICT (type))
    {
      if (did_print_modifier || need_pre_space)
	gdb_printf (stream, " ");
      gdb_printf (stream, (language == language_cplus
			   ? "__restrict__"
			   : "restrict"));
      did_print_modifier = 1;
    }

  if (TYPE_ATOMIC (type))
    {
      if (did_print_modifier || need_pre_space)
	gdb_printf (stream, " ");
      gdb_printf (stream, "_Atomic");
      did_print_modifier = 1;
    }

  address_space_id
    = address_space_type_instance_flags_to_name (type->arch (),
						 type->instance_flags ());
  if (address_space_id)
    {
      if (did_print_modifier || need_pre_space)
	gdb_printf (stream, " ");
      gdb_printf (stream, "@%s", address_space_id);
      did_print_modifier = 1;
    }

  if (did_print_modifier && need_post_space)
    gdb_printf (stream, " ");
}


/* Print out the arguments of TYPE, which should have TYPE_CODE_METHOD
   or TYPE_CODE_FUNC, to STREAM.  Artificial arguments, such as "this"
   in non-static methods, are displayed if LINKAGE_NAME is zero.  If
   LINKAGE_NAME is non-zero and LANGUAGE is language_cplus the topmost
   parameter types get removed their possible const and volatile qualifiers to
   match demangled linkage name parameters part of such function type.
   LANGUAGE is the language in which TYPE was defined.  This is a necessary
   evil since this code is used by the C and C++.  */

void
c_type_print_args (struct type *type, struct ui_file *stream,
		   int linkage_name, enum language language,
		   const struct type_print_options *flags)
{
  int i;
  int printed_any = 0;

  gdb_printf (stream, "(");

  for (i = 0; i < type->num_fields (); i++)
    {
      struct type *param_type;

      if (type->field (i).is_artificial () && linkage_name)
	continue;

      if (printed_any)
	{
	  gdb_printf (stream, ", ");
	  stream->wrap_here (4);
	}

      param_type = type->field (i).type ();

      if (language == language_cplus && linkage_name)
	{
	  /* C++ standard, 13.1 Overloadable declarations, point 3, item:
	     - Parameter declarations that differ only in the presence or
	       absence of const and/or volatile are equivalent.

	     And the const/volatile qualifiers are not present in the mangled
	     names as produced by GCC.  */

	  param_type = make_cv_type (0, 0, param_type, NULL);
	}

      c_print_type (param_type, "", stream, -1, 0, language, flags);
      printed_any = 1;
    }

  if (printed_any && type->has_varargs ())
    {
      /* Print out a trailing ellipsis for varargs functions.  Ignore
	 TYPE_VARARGS if the function has no named arguments; that
	 represents unprototyped (K&R style) C functions.  */
      if (printed_any && type->has_varargs ())
	{
	  gdb_printf (stream, ", ");
	  stream->wrap_here (4);
	  gdb_printf (stream, "...");
	}
    }
  else if (!printed_any
	   && (type->is_prototyped () || language == language_cplus))
    gdb_printf (stream, "void");

  gdb_printf (stream, ")");
}

/* Return true iff the j'th overloading of the i'th method of TYPE
   is a type conversion operator, like `operator int () { ... }'.
   When listing a class's methods, we don't print the return type of
   such operators.  */

static int
is_type_conversion_operator (struct type *type, int i, int j)
{
  /* I think the whole idea of recognizing type conversion operators
     by their name is pretty terrible.  But I don't think our present
     data structure gives us any other way to tell.  If you know of
     some other way, feel free to rewrite this function.  */
  const char *name = TYPE_FN_FIELDLIST_NAME (type, i);

  if (!startswith (name, CP_OPERATOR_STR))
    return 0;

  name += 8;
  if (! strchr (" \t\f\n\r", *name))
    return 0;

  while (strchr (" \t\f\n\r", *name))
    name++;

  if (!('a' <= *name && *name <= 'z')
      && !('A' <= *name && *name <= 'Z')
      && *name != '_')
    /* If this doesn't look like the start of an identifier, then it
       isn't a type conversion operator.  */
    return 0;
  else if (startswith (name, "new"))
    name += 3;
  else if (startswith (name, "delete"))
    name += 6;
  else
    /* If it doesn't look like new or delete, it's a type conversion
       operator.  */
    return 1;

  /* Is that really the end of the name?  */
  if (('a' <= *name && *name <= 'z')
      || ('A' <= *name && *name <= 'Z')
      || ('0' <= *name && *name <= '9')
      || *name == '_')
    /* No, so the identifier following "operator" must be a type name,
       and this is a type conversion operator.  */
    return 1;

  /* That was indeed the end of the name, so it was `operator new' or
     `operator delete', neither of which are type conversion
     operators.  */
  return 0;
}

/* Given a C++ qualified identifier QID, strip off the qualifiers,
   yielding the unqualified name.  The return value is a pointer into
   the original string.

   It's a pity we don't have this information in some more structured
   form.  Even the author of this function feels that writing little
   parsers like this everywhere is stupid.  */

static const char *
remove_qualifiers (const char *qid)
{
  int quoted = 0;	/* Zero if we're not in quotes;
			   '"' if we're in a double-quoted string;
			   '\'' if we're in a single-quoted string.  */
  int depth = 0;	/* Number of unclosed parens we've seen.  */
  char *parenstack = (char *) alloca (strlen (qid));
  const char *scan;
  const char *last = 0;	/* The character after the rightmost
			   `::' token we've seen so far.  */

  for (scan = qid; *scan; scan++)
    {
      if (quoted)
	{
	  if (*scan == quoted)
	    quoted = 0;
	  else if (*scan == '\\' && *(scan + 1))
	    scan++;
	}
      else if (scan[0] == ':' && scan[1] == ':')
	{
	  /* If we're inside parenthesis (i.e., an argument list) or
	     angle brackets (i.e., a list of template arguments), then
	     we don't record the position of this :: token, since it's
	     not relevant to the top-level structure we're trying to
	     operate on.  */
	  if (depth == 0)
	    {
	      last = scan + 2;
	      scan++;
	    }
	}
      else if (*scan == '"' || *scan == '\'')
	quoted = *scan;
      else if (*scan == '(')
	parenstack[depth++] = ')';
      else if (*scan == '[')
	parenstack[depth++] = ']';
      /* We're going to treat <> as a pair of matching characters,
	 since we're more likely to see those in template id's than
	 real less-than characters.  What a crock.  */
      else if (*scan == '<')
	parenstack[depth++] = '>';
      else if (*scan == ')' || *scan == ']' || *scan == '>')
	{
	  if (depth > 0 && parenstack[depth - 1] == *scan)
	    depth--;
	  else
	    {
	      /* We're going to do a little error recovery here.  If
		 we don't find a match for *scan on the paren stack,
		 but there is something lower on the stack that does
		 match, we pop the stack to that point.  */
	      int i;

	      for (i = depth - 1; i >= 0; i--)
		if (parenstack[i] == *scan)
		  {
		    depth = i;
		    break;
		  }
	    }
	}
    }

  if (last)
    return last;
  else
    /* We didn't find any :: tokens at the top level, so declare the
       whole thing an unqualified identifier.  */
    return qid;
}

/* Print any array sizes, function arguments or close parentheses
   needed after the variable name (to describe its type).
   Args work like c_type_print_varspec_prefix.  */

static void
c_type_print_varspec_suffix (struct type *type,
			     struct ui_file *stream,
			     int show, int passed_a_ptr,
			     int demangled_args,
			     enum language language,
			     const struct type_print_options *flags)
{
  if (type == 0)
    return;

  if (type->name () && show <= 0)
    return;

  QUIT;

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;
	int is_vector = type->is_vector ();

	if (passed_a_ptr)
	  gdb_printf (stream, ")");

	gdb_printf (stream, (is_vector ?
			     " __attribute__ ((vector_size(" : "["));
	/* Bounds are not yet resolved, print a bounds placeholder instead.  */
	if (type->bounds ()->high.kind () == PROP_LOCEXPR
	    || type->bounds ()->high.kind () == PROP_LOCLIST)
	  gdb_printf (stream, "variable length");
	else if (get_array_bounds (type, &low_bound, &high_bound))
	  gdb_printf (stream, "%s", 
		      plongest (high_bound - low_bound + 1));
	gdb_printf (stream, (is_vector ? ")))" : "]"));

	c_type_print_varspec_suffix (type->target_type (), stream,
				     show, 0, 0, language, flags);
      }
      break;

    case TYPE_CODE_MEMBERPTR:
      c_type_print_varspec_suffix (type->target_type (), stream,
				   show, 0, 0, language, flags);
      break;

    case TYPE_CODE_METHODPTR:
      gdb_printf (stream, ")");
      c_type_print_varspec_suffix (type->target_type (), stream,
				   show, 0, 0, language, flags);
      break;

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      c_type_print_varspec_suffix (type->target_type (), stream,
				   show, 1, 0, language, flags);
      break;

    case TYPE_CODE_METHOD:
    case TYPE_CODE_FUNC:
      if (passed_a_ptr)
	gdb_printf (stream, ")");
      if (!demangled_args)
	c_type_print_args (type, stream, 0, language, flags);
      c_type_print_varspec_suffix (type->target_type (), stream,
				   show, passed_a_ptr, 0, language, flags);
      break;

    case TYPE_CODE_TYPEDEF:
      c_type_print_varspec_suffix (type->target_type (), stream,
				   show, passed_a_ptr, 0, language, flags);
      break;
    }
}

/* A helper for c_type_print_base that displays template
   parameters and their bindings, if needed.

   TABLE is the local bindings table to use.  If NULL, no printing is
   done.  Note that, at this point, TABLE won't have any useful
   information in it -- but it is also used as a flag to
   print_name_maybe_canonical to activate searching the global typedef
   table.

   TYPE is the type whose template arguments are being displayed.

   STREAM is the stream on which to print.  */

static void
c_type_print_template_args (const struct type_print_options *flags,
			    struct type *type, struct ui_file *stream,
			    enum language language)
{
  int first = 1, i;

  if (flags->raw)
    return;

  for (i = 0; i < TYPE_N_TEMPLATE_ARGUMENTS (type); ++i)
    {
      struct symbol *sym = TYPE_TEMPLATE_ARGUMENT (type, i);

      if (sym->aclass () != LOC_TYPEDEF)
	continue;

      if (first)
	{
	  stream->wrap_here (4);
	  gdb_printf (stream, _("[with %s = "), sym->linkage_name ());
	  first = 0;
	}
      else
	{
	  gdb_puts (", ", stream);
	  stream->wrap_here (9);
	  gdb_printf (stream, "%s = ", sym->linkage_name ());
	}

      c_print_type (sym->type (), "", stream, -1, 0, language, flags);
    }

  if (!first)
    gdb_puts (_("] "), stream);
}

/* Use 'print_spaces', but take into consideration the
   type_print_options FLAGS in order to determine how many whitespaces
   will be printed.  */

static void
print_spaces_filtered_with_print_options
  (int level, struct ui_file *stream, const struct type_print_options *flags)
{
  if (!flags->print_offsets)
    print_spaces (level, stream);
  else
    print_spaces (level + print_offset_data::indentation, stream);
}

/* Output an access specifier to STREAM, if needed.  LAST_ACCESS is the
   last access specifier output (typically returned by this function).  */

static accessibility
output_access_specifier (struct ui_file *stream,
			 accessibility last_access,
			 int level, accessibility new_access,
			 const struct type_print_options *flags)
{
  if (last_access == new_access)
    return new_access;

  if (new_access == accessibility::PROTECTED)
    {
      print_spaces_filtered_with_print_options (level + 2, stream, flags);
      gdb_printf (stream, "protected:\n");
    }
  else if (new_access == accessibility::PRIVATE)
    {
      print_spaces_filtered_with_print_options (level + 2, stream, flags);
      gdb_printf (stream, "private:\n");
    }
  else
    {
      print_spaces_filtered_with_print_options (level + 2, stream, flags);
      gdb_printf (stream, "public:\n");
    }

  return new_access;
}

/* Helper function that temporarily disables FLAGS->PRINT_OFFSETS,
   calls 'c_print_type_1', and then reenables FLAGS->PRINT_OFFSETS if
   applicable.  */

static void
c_print_type_no_offsets (struct type *type,
			 const char *varstring,
			 struct ui_file *stream,
			 int show, int level,
			 enum language language,
			 struct type_print_options *flags,
			 struct print_offset_data *podata)
{
  unsigned int old_po = flags->print_offsets;

  /* Temporarily disable print_offsets, because it would mess with
     indentation.  */
  flags->print_offsets = 0;
  c_print_type_1 (type, varstring, stream, show, level, language, flags,
		  podata);
  flags->print_offsets = old_po;
}

/* Helper for 'c_type_print_base' that handles structs and unions.
   For a description of the arguments, see 'c_type_print_base'.  */

static void
c_type_print_base_struct_union (struct type *type, struct ui_file *stream,
				int show, int level,
				enum language language,
				const struct type_print_options *flags,
				struct print_offset_data *podata)
{
  struct type_print_options local_flags = *flags;
  local_flags.local_typedefs = NULL;

  std::unique_ptr<typedef_hash_table> hash_holder;
  if (!flags->raw)
    {
      if (flags->local_typedefs)
	local_flags.local_typedefs
	  = new typedef_hash_table (*flags->local_typedefs);
      else
	local_flags.local_typedefs = new typedef_hash_table ();

      hash_holder.reset (local_flags.local_typedefs);
    }

  c_type_print_modifier (type, stream, 0, 1, language);
  if (type->code () == TYPE_CODE_UNION)
    gdb_printf (stream, "union ");
  else if (type->is_declared_class ())
    gdb_printf (stream, "class ");
  else
    gdb_printf (stream, "struct ");

  /* Print the tag if it exists.  The HP aCC compiler emits a
     spurious "{unnamed struct}"/"{unnamed union}"/"{unnamed
     enum}" tag for unnamed struct/union/enum's, which we don't
     want to print.  */
  if (type->name () != NULL
      && !startswith (type->name (), "{unnamed"))
    {
      /* When printing the tag name, we are still effectively
	 printing in the outer context, hence the use of FLAGS
	 here.  */
      print_name_maybe_canonical (type->name (), flags, stream);
      if (show > 0)
	gdb_puts (" ", stream);
    }

  if (show < 0)
    {
      /* If we just printed a tag name, no need to print anything
	 else.  */
      if (type->name () == NULL)
	gdb_printf (stream, "{...}");
    }
  else if (show > 0 || type->name () == NULL)
    {
      struct type *basetype;
      int vptr_fieldno;

      c_type_print_template_args (&local_flags, type, stream, language);

      /* Add in template parameters when printing derivation info.  */
      if (local_flags.local_typedefs != NULL)
	local_flags.local_typedefs->add_template_parameters (type);
      cp_type_print_derivation_info (stream, type, &local_flags);

      /* This holds just the global typedefs and the template
	 parameters.  */
      struct type_print_options semi_local_flags = *flags;
      semi_local_flags.local_typedefs = NULL;

      std::unique_ptr<typedef_hash_table> semi_holder;
      if (local_flags.local_typedefs != nullptr)
	{
	  semi_local_flags.local_typedefs
	    = new typedef_hash_table (*local_flags.local_typedefs);
	  semi_holder.reset (semi_local_flags.local_typedefs);

	  /* Now add in the local typedefs.  */
	  local_flags.local_typedefs->recursively_update (type);
	}

      gdb_printf (stream, "{\n");

      if (type->num_fields () == 0 && TYPE_NFN_FIELDS (type) == 0
	  && TYPE_TYPEDEF_FIELD_COUNT (type) == 0)
	{
	  print_spaces_filtered_with_print_options (level + 4, stream, flags);
	  if (type->is_stub ())
	    gdb_printf (stream, _("%p[<incomplete type>%p]\n"),
			metadata_style.style ().ptr (), nullptr);
	  else
	    gdb_printf (stream, _("%p[<no data fields>%p]\n"),
			metadata_style.style ().ptr (), nullptr);
	}

      accessibility section_type = (type->is_declared_class ()
				    ? accessibility::PRIVATE
				    : accessibility::PUBLIC);

      /* If there is a base class for this type,
	 do not print the field that it occupies.  */

      int len = type->num_fields ();
      vptr_fieldno = get_vptr_fieldno (type, &basetype);

      struct print_offset_data local_podata (flags);

      for (int i = TYPE_N_BASECLASSES (type); i < len; i++)
	{
	  QUIT;

	  /* If we have a virtual table pointer, omit it.  Even if
	     virtual table pointers are not specifically marked in
	     the debug info, they should be artificial.  */
	  if ((i == vptr_fieldno && type == basetype)
	      || type->field (i).is_artificial ())
	    continue;

	  section_type
	    = output_access_specifier (stream, section_type, level,
				       type->field (i).accessibility (),
				       flags);

	  bool is_static = type->field (i).is_static ();

	  if (flags->print_offsets)
	    podata->update (type, i, stream);

	  print_spaces (level + 4, stream);
	  if (is_static)
	    gdb_printf (stream, "static ");

	  int newshow = show - 1;

	  if (!is_static && flags->print_offsets
	      && (type->field (i).type ()->code () == TYPE_CODE_STRUCT
		  || type->field (i).type ()->code () == TYPE_CODE_UNION))
	    {
	      /* If we're printing offsets and this field's type is
		 either a struct or an union, then we're interested in
		 expanding it.  */
	      ++newshow;

	      /* Make sure we carry our offset when we expand the
		 struct/union.  */
	      local_podata.offset_bitpos
		= podata->offset_bitpos + type->field (i).loc_bitpos ();
	      /* We're entering a struct/union.  Right now,
		 PODATA->END_BITPOS points right *after* the
		 struct/union.  However, when printing the first field
		 of this inner struct/union, the end_bitpos we're
		 expecting is exactly at the beginning of the
		 struct/union.  Therefore, we subtract the length of
		 the whole struct/union.  */
	      local_podata.end_bitpos
		= podata->end_bitpos
		  - type->field (i).type ()->length () * TARGET_CHAR_BIT;
	    }

	  c_print_type_1 (type->field (i).type (),
			  type->field (i).name (),
			  stream, newshow, level + 4,
			  language, &local_flags, &local_podata);

	  if (!is_static && type->field (i).is_packed ())
	    {
	      /* It is a bitfield.  This code does not attempt
		 to look at the bitpos and reconstruct filler,
		 unnamed fields.  This would lead to misleading
		 results if the compiler does not put out fields
		 for such things (I don't know what it does).  */
	      gdb_printf (stream, " : %d", type->field (i).bitsize ());
	    }
	  gdb_printf (stream, ";\n");
	}

      /* If there are both fields and methods, put a blank line
	 between them.  Make sure to count only method that we
	 will display; artificial methods will be hidden.  */
      len = TYPE_NFN_FIELDS (type);
      if (!flags->print_methods)
	len = 0;
      int real_len = 0;
      for (int i = 0; i < len; i++)
	{
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);
	  int len2 = TYPE_FN_FIELDLIST_LENGTH (type, i);
	  int j;

	  for (j = 0; j < len2; j++)
	    if (!TYPE_FN_FIELD_ARTIFICIAL (f, j))
	      real_len++;
	}
      if (real_len > 0)
	gdb_printf (stream, "\n");

      /* C++: print out the methods.  */
      for (int i = 0; i < len; i++)
	{
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);
	  int j, len2 = TYPE_FN_FIELDLIST_LENGTH (type, i);
	  const char *method_name = TYPE_FN_FIELDLIST_NAME (type, i);
	  const char *name = type->name ();
	  int is_constructor = name && strcmp (method_name,
					       name) == 0;

	  for (j = 0; j < len2; j++)
	    {
	      const char *mangled_name;
	      gdb::unique_xmalloc_ptr<char> mangled_name_holder;
	      const char *physname = TYPE_FN_FIELD_PHYSNAME (f, j);
	      int is_full_physname_constructor =
		TYPE_FN_FIELD_CONSTRUCTOR (f, j)
		|| is_constructor_name (physname)
		|| is_destructor_name (physname)
		|| method_name[0] == '~';

	      /* Do not print out artificial methods.  */
	      if (TYPE_FN_FIELD_ARTIFICIAL (f, j))
		continue;

	      QUIT;
	      section_type = output_access_specifier
		(stream, section_type, level,
		 TYPE_FN_FIELD (f, j).accessibility,
		 flags);

	      print_spaces_filtered_with_print_options (level + 4, stream,
							flags);
	      if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		gdb_printf (stream, "virtual ");
	      else if (TYPE_FN_FIELD_STATIC_P (f, j))
		gdb_printf (stream, "static ");
	      if (TYPE_FN_FIELD_TYPE (f, j)->target_type () == 0)
		{
		  /* Keep GDB from crashing here.  */
		  gdb_printf (stream,
			      _("%p[<undefined type>%p] %s;\n"),
			      metadata_style.style ().ptr (), nullptr,
			      TYPE_FN_FIELD_PHYSNAME (f, j));
		  break;
		}
	      else if (!is_constructor	/* Constructors don't
					   have declared
					   types.  */
		       && !is_full_physname_constructor  /* " " */
		       && !is_type_conversion_operator (type, i, j))
		{
		  c_print_type_no_offsets
		    (TYPE_FN_FIELD_TYPE (f, j)->target_type (),
		     "", stream, -1, 0, language, &local_flags, podata);

		  gdb_puts (" ", stream);
		}
	      if (TYPE_FN_FIELD_STUB (f, j))
		{
		  /* Build something we can demangle.  */
		  mangled_name_holder.reset (gdb_mangle_name (type, i, j));
		  mangled_name = mangled_name_holder.get ();
		}
	      else
		mangled_name = TYPE_FN_FIELD_PHYSNAME (f, j);

	      gdb::unique_xmalloc_ptr<char> demangled_name
		= gdb_demangle (mangled_name,
				DMGL_ANSI | DMGL_PARAMS);
	      if (demangled_name == NULL)
		{
		  /* In some cases (for instance with the HP
		     demangling), if a function has more than 10
		     arguments, the demangling will fail.
		     Let's try to reconstruct the function
		     signature from the symbol information.  */
		  if (!TYPE_FN_FIELD_STUB (f, j))
		    {
		      int staticp = TYPE_FN_FIELD_STATIC_P (f, j);
		      struct type *mtype = TYPE_FN_FIELD_TYPE (f, j);

		      cp_type_print_method_args (mtype,
						 "",
						 method_name,
						 staticp,
						 stream, language,
						 &local_flags);
		    }
		  else
		    fprintf_styled (stream, metadata_style.style (),
				    _("<badly mangled name '%s'>"),
				    mangled_name);
		}
	      else
		{
		  const char *p;
		  const char *demangled_no_class
		    = remove_qualifiers (demangled_name.get ());

		  /* Get rid of the `static' appended by the
		     demangler.  */
		  p = strstr (demangled_no_class, " static");
		  if (p != NULL)
		    {
		      int length = p - demangled_no_class;
		      std::string demangled_no_static (demangled_no_class,
						       length);
		      gdb_puts (demangled_no_static.c_str (), stream);
		    }
		  else
		    gdb_puts (demangled_no_class, stream);
		}

	      gdb_printf (stream, ";\n");
	    }
	}

      /* Print out nested types.  */
      if (TYPE_NESTED_TYPES_COUNT (type) != 0
	  && semi_local_flags.print_nested_type_limit != 0)
	{
	  if (semi_local_flags.print_nested_type_limit > 0)
	    --semi_local_flags.print_nested_type_limit;

	  if (type->num_fields () != 0 || TYPE_NFN_FIELDS (type) != 0)
	    gdb_printf (stream, "\n");

	  for (int i = 0; i < TYPE_NESTED_TYPES_COUNT (type); ++i)
	    {
	      print_spaces_filtered_with_print_options (level + 4, stream,
							flags);
	      c_print_type_no_offsets (TYPE_NESTED_TYPES_FIELD_TYPE (type, i),
				       "", stream, show, level + 4,
				       language, &semi_local_flags, podata);
	      gdb_printf (stream, ";\n");
	    }
	}

      /* Print typedefs defined in this class.  */

      if (TYPE_TYPEDEF_FIELD_COUNT (type) != 0 && flags->print_typedefs)
	{
	  if (type->num_fields () != 0 || TYPE_NFN_FIELDS (type) != 0
	      || TYPE_NESTED_TYPES_COUNT (type) != 0)
	    gdb_printf (stream, "\n");

	  for (int i = 0; i < TYPE_TYPEDEF_FIELD_COUNT (type); i++)
	    {
	      struct type *target = TYPE_TYPEDEF_FIELD_TYPE (type, i);

	      /* Dereference the typedef declaration itself.  */
	      gdb_assert (target->code () == TYPE_CODE_TYPEDEF);
	      target = target->target_type ();

	      section_type = (output_access_specifier
			      (stream, section_type, level,
			       TYPE_TYPEDEF_FIELD (type, i).accessibility,
			       flags));

	      print_spaces_filtered_with_print_options (level + 4, stream,
							flags);
	      gdb_printf (stream, "typedef ");

	      /* We want to print typedefs with substitutions
		 from the template parameters or globally-known
		 typedefs but not local typedefs.  */
	      c_print_type_no_offsets (target,
				       TYPE_TYPEDEF_FIELD_NAME (type, i),
				       stream, show - 1, level + 4,
				       language, &semi_local_flags, podata);
	      gdb_printf (stream, ";\n");
	    }
	}

      if (flags->print_offsets)
	{
	  if (show > 0)
	    podata->finish (type, level, stream);

	  print_spaces (print_offset_data::indentation, stream);
	  if (level == 0)
	    print_spaces (2, stream);
	}

      gdb_printf (stream, "%*s}", level, "");
    }
}

/* Print the name of the type (or the ultimate pointer target,
   function value or array element), or the description of a structure
   or union.

   SHOW positive means print details about the type (e.g. enum
   values), and print structure elements passing SHOW - 1 for show.

   SHOW negative means just print the type name or struct tag if there
   is one.  If there is no name, print something sensible but concise
   like "struct {...}".

   SHOW zero means just print the type name or struct tag if there is
   one.  If there is no name, print something sensible but not as
   concise like "struct {int x; int y;}".

   LEVEL is the number of spaces to indent by.
   We increase it for some recursive calls.  */

static void
c_type_print_base_1 (struct type *type, struct ui_file *stream,
		     int show, int level,
		     enum language language,
		     const struct type_print_options *flags,
		     struct print_offset_data *podata)
{
  int i;
  int len;

  QUIT;

  if (type == NULL)
    {
      fputs_styled (_("<type unknown>"), metadata_style.style (), stream);
      return;
    }

  /* When SHOW is zero or less, and there is a valid type name, then
     always just print the type name directly from the type.  */

  if (show <= 0
      && type->name () != NULL)
    {
      c_type_print_modifier (type, stream, 0, 1, language);

      /* If we have "typedef struct foo {. . .} bar;" do we want to
	 print it as "struct foo" or as "bar"?  Pick the latter for
	 C++, because C++ folk tend to expect things like "class5
	 *foo" rather than "struct class5 *foo".  We rather
	 arbitrarily choose to make language_minimal work in a C-like
	 way. */
      if (language == language_c || language == language_minimal)
	{
	  if (type->code () == TYPE_CODE_UNION)
	    gdb_printf (stream, "union ");
	  else if (type->code () == TYPE_CODE_STRUCT)
	    {
	      if (type->is_declared_class ())
		gdb_printf (stream, "class ");
	      else
		gdb_printf (stream, "struct ");
	    }
	  else if (type->code () == TYPE_CODE_ENUM)
	    gdb_printf (stream, "enum ");
	}

      print_name_maybe_canonical (type->name (), flags, stream);
      return;
    }

  type = check_typedef (type);

  switch (type->code ())
    {
    case TYPE_CODE_TYPEDEF:
      /* If we get here, the typedef doesn't have a name, and we
	 couldn't resolve type::target_type.  Not much we can do.  */
      gdb_assert (type->name () == NULL);
      gdb_assert (type->target_type () == NULL);
      fprintf_styled (stream, metadata_style.style (),
		      _("<unnamed typedef>"));
      break;

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      if (type->target_type () == NULL)
	type_print_unknown_return_type (stream);
      else
	c_type_print_base_1 (type->target_type (),
			     stream, show, level, language, flags, podata);
      break;
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_PTR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_METHODPTR:
      c_type_print_base_1 (type->target_type (),
			   stream, show, level, language, flags, podata);
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      c_type_print_base_struct_union (type, stream, show, level,
				      language, flags, podata);
      break;

    case TYPE_CODE_ENUM:
      c_type_print_modifier (type, stream, 0, 1, language);
      gdb_printf (stream, "enum ");
      if (type->is_declared_class ())
	gdb_printf (stream, "class ");
      /* Print the tag name if it exists.
	 The aCC compiler emits a spurious 
	 "{unnamed struct}"/"{unnamed union}"/"{unnamed enum}"
	 tag for unnamed struct/union/enum's, which we don't
	 want to print.  */
      if (type->name () != NULL
	  && !startswith (type->name (), "{unnamed"))
	{
	  print_name_maybe_canonical (type->name (), flags, stream);
	  if (show > 0)
	    gdb_puts (" ", stream);
	}

      stream->wrap_here (4);
      if (show < 0)
	{
	  /* If we just printed a tag name, no need to print anything
	     else.  */
	  if (type->name () == NULL)
	    gdb_printf (stream, "{...}");
	}
      else if (show > 0 || type->name () == NULL)
	{
	  LONGEST lastval = 0;

	  /* We can't handle this case perfectly, as DWARF does not
	     tell us whether or not the underlying type was specified
	     in the source (and other debug formats don't provide this
	     at all).  We choose to print the underlying type, if it
	     has a name, when in C++ on the theory that it's better to
	     print too much than too little; but conversely not to
	     print something egregiously outside the current
	     language's syntax.  */
	  if (language == language_cplus && type->target_type () != NULL)
	    {
	      struct type *underlying = check_typedef (type->target_type ());

	      if (underlying->name () != NULL)
		gdb_printf (stream, ": %s ", underlying->name ());
	    }

	  gdb_printf (stream, "{");
	  len = type->num_fields ();
	  for (i = 0; i < len; i++)
	    {
	      QUIT;
	      if (i)
		gdb_printf (stream, ", ");
	      stream->wrap_here (4);
	      fputs_styled (type->field (i).name (),
			    variable_name_style.style (), stream);
	      if (lastval != type->field (i).loc_enumval ())
		{
		  gdb_printf (stream, " = %s",
			      plongest (type->field (i).loc_enumval ()));
		  lastval = type->field (i).loc_enumval ();
		}
	      lastval++;
	    }
	  gdb_printf (stream, "}");
	}
      break;

    case TYPE_CODE_FLAGS:
      {
	struct type_print_options local_flags = *flags;

	local_flags.local_typedefs = NULL;

	c_type_print_modifier (type, stream, 0, 1, language);
	gdb_printf (stream, "flag ");
	print_name_maybe_canonical (type->name (), flags, stream);
	if (show > 0)
	  {
	    gdb_puts (" ", stream);
	    gdb_printf (stream, "{\n");
	    if (type->num_fields () == 0)
	      {
		if (type->is_stub ())
		  gdb_printf (stream,
			      _("%*s%p[<incomplete type>%p]\n"),
			      level + 4, "",
			      metadata_style.style ().ptr (), nullptr);
		else
		  gdb_printf (stream,
			      _("%*s%p[<no data fields>%p]\n"),
			      level + 4, "",
			      metadata_style.style ().ptr (), nullptr);
	      }
	    len = type->num_fields ();
	    for (i = 0; i < len; i++)
	      {
		QUIT;
		print_spaces (level + 4, stream);
		/* We pass "show" here and not "show - 1" to get enum types
		   printed.  There's no other way to see them.  */
		c_print_type_1 (type->field (i).type (),
				type->field (i).name (),
				stream, show, level + 4,
				language, &local_flags, podata);
		gdb_printf (stream, " @%s",
			    plongest (type->field (i).loc_bitpos ()));
		if (type->field (i).bitsize () > 1)
		  {
		    gdb_printf (stream, "-%s",
				plongest (type->field (i).loc_bitpos ()
					  + type->field (i).bitsize ()
					  - 1));
		  }
		gdb_printf (stream, ";\n");
	      }
	    gdb_printf (stream, "%*s}", level, "");
	  }
      }
      break;

    case TYPE_CODE_VOID:
      gdb_printf (stream, "void");
      break;

    case TYPE_CODE_UNDEF:
      gdb_printf (stream, _("struct <unknown>"));
      break;

    case TYPE_CODE_ERROR:
      gdb_printf (stream, "%s", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_RANGE:
      /* This should not occur.  */
      fprintf_styled (stream, metadata_style.style (), _("<range type>"));
      break;

    case TYPE_CODE_FIXED_POINT:
      print_type_fixed_point (type, stream);
      break;

    case TYPE_CODE_NAMESPACE:
      gdb_puts ("namespace ", stream);
      gdb_puts (type->name (), stream);
      break;

    default:
      /* Handle types not explicitly handled by the other cases, such
	 as fundamental types.  For these, just print whatever the
	 type name is, as recorded in the type itself.  If there is no
	 type name, then complain.  */
      if (type->name () != NULL)
	{
	  c_type_print_modifier (type, stream, 0, 1, language);
	  print_name_maybe_canonical (type->name (), flags, stream);
	}
      else
	{
	  /* At least for dump_symtab, it is important that this not
	     be an error ().  */
	  fprintf_styled (stream, metadata_style.style (),
			  _("<invalid type code %d>"), type->code ());
	}
      break;
    }
}

/* See c_type_print_base_1.  */

void
c_type_print_base (struct type *type, struct ui_file *stream,
		   int show, int level,
		   const struct type_print_options *flags)
{
  struct print_offset_data podata (flags);

  c_type_print_base_1 (type, stream, show, level,
		       current_language->la_language, flags, &podata);
}

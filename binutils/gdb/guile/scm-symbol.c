/* Scheme interface to symbols.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "block.h"
#include "frame.h"
#include "symtab.h"
#include "objfiles.h"
#include "value.h"
#include "guile-internal.h"

/* The <gdb:symbol> smob.  */

struct symbol_smob
{
  /* This always appears first.  */
  eqable_gdb_smob base;

  /* The GDB symbol structure this smob is wrapping.  */
  struct symbol *symbol;
};

static const char symbol_smob_name[] = "gdb:symbol";

/* The tag Guile knows the symbol smob by.  */
static scm_t_bits symbol_smob_tag;

/* Keywords used in argument passing.  */
static SCM block_keyword;
static SCM domain_keyword;
static SCM frame_keyword;

/* This is called when an objfile is about to be freed.
   Invalidate the symbol as further actions on the symbol would result
   in bad data.  All access to s_smob->symbol should be gated by
   syscm_get_valid_symbol_smob_arg_unsafe which will raise an exception on
   invalid symbols.  */
struct syscm_deleter
{
  /* Helper function for syscm_del_objfile_symbols to mark the symbol
     as invalid.  */

  static int
  syscm_mark_symbol_invalid (void **slot, void *info)
  {
    symbol_smob *s_smob = (symbol_smob *) *slot;

    s_smob->symbol = NULL;
    return 1;
  }

  void operator() (htab_t htab)
  {
    gdb_assert (htab != nullptr);
    htab_traverse_noresize (htab, syscm_mark_symbol_invalid, NULL);
    htab_delete (htab);
  }
};

static const registry<objfile>::key<htab, syscm_deleter>
     syscm_objfile_data_key;

struct syscm_gdbarch_data
{
  /* Hash table to implement eqable gdbarch symbols.  */
  htab_t htab;
};

static const registry<gdbarch>::key<syscm_gdbarch_data> syscm_gdbarch_data_key;

/* Administrivia for symbol smobs.  */

/* Helper function to hash a symbol_smob.  */

static hashval_t
syscm_hash_symbol_smob (const void *p)
{
  const symbol_smob *s_smob = (const symbol_smob *) p;

  return htab_hash_pointer (s_smob->symbol);
}

/* Helper function to compute equality of symbol_smobs.  */

static int
syscm_eq_symbol_smob (const void *ap, const void *bp)
{
  const symbol_smob *a = (const symbol_smob *) ap;
  const symbol_smob *b = (const symbol_smob *) bp;

  return (a->symbol == b->symbol
	  && a->symbol != NULL);
}

/* Return the struct symbol pointer -> SCM mapping table.
   It is created if necessary.  */

static htab_t
syscm_get_symbol_map (struct symbol *symbol)
{
  htab_t htab;

  if (symbol->is_objfile_owned ())
    {
      struct objfile *objfile = symbol->objfile ();

      htab = syscm_objfile_data_key.get (objfile);
      if (htab == NULL)
	{
	  htab = gdbscm_create_eqable_gsmob_ptr_map (syscm_hash_symbol_smob,
						     syscm_eq_symbol_smob);
	  syscm_objfile_data_key.set (objfile, htab);
	}
    }
  else
    {
      struct gdbarch *gdbarch = symbol->arch ();
      struct syscm_gdbarch_data *data = syscm_gdbarch_data_key.get (gdbarch);
      if (data == nullptr)
	{
	  data = syscm_gdbarch_data_key.emplace (gdbarch);
	  data->htab
	    = gdbscm_create_eqable_gsmob_ptr_map (syscm_hash_symbol_smob,
						  syscm_eq_symbol_smob);
	}

      htab = data->htab;
    }

  return htab;
}

/* The smob "free" function for <gdb:symbol>.  */

static size_t
syscm_free_symbol_smob (SCM self)
{
  symbol_smob *s_smob = (symbol_smob *) SCM_SMOB_DATA (self);

  if (s_smob->symbol != NULL)
    {
      htab_t htab = syscm_get_symbol_map (s_smob->symbol);

      gdbscm_clear_eqable_gsmob_ptr_slot (htab, &s_smob->base);
    }

  /* Not necessary, done to catch bugs.  */
  s_smob->symbol = NULL;

  return 0;
}

/* The smob "print" function for <gdb:symbol>.  */

static int
syscm_print_symbol_smob (SCM self, SCM port, scm_print_state *pstate)
{
  symbol_smob *s_smob = (symbol_smob *) SCM_SMOB_DATA (self);

  if (pstate->writingp)
    gdbscm_printf (port, "#<%s ", symbol_smob_name);
  gdbscm_printf (port, "%s",
		 s_smob->symbol != NULL
		 ? s_smob->symbol->print_name ()
		 : "<invalid>");
  if (pstate->writingp)
    scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:symbol> object.  */

static SCM
syscm_make_symbol_smob (void)
{
  symbol_smob *s_smob = (symbol_smob *)
    scm_gc_malloc (sizeof (symbol_smob), symbol_smob_name);
  SCM s_scm;

  s_smob->symbol = NULL;
  s_scm = scm_new_smob (symbol_smob_tag, (scm_t_bits) s_smob);
  gdbscm_init_eqable_gsmob (&s_smob->base, s_scm);

  return s_scm;
}

/* Return non-zero if SCM is a symbol smob.  */

int
syscm_is_symbol (SCM scm)
{
  return SCM_SMOB_PREDICATE (symbol_smob_tag, scm);
}

/* (symbol? object) -> boolean */

static SCM
gdbscm_symbol_p (SCM scm)
{
  return scm_from_bool (syscm_is_symbol (scm));
}

/* Return the existing object that encapsulates SYMBOL, or create a new
   <gdb:symbol> object.  */

SCM
syscm_scm_from_symbol (struct symbol *symbol)
{
  htab_t htab;
  eqable_gdb_smob **slot;
  symbol_smob *s_smob, s_smob_for_lookup;
  SCM s_scm;

  /* If we've already created a gsmob for this symbol, return it.
     This makes symbols eq?-able.  */
  htab = syscm_get_symbol_map (symbol);
  s_smob_for_lookup.symbol = symbol;
  slot = gdbscm_find_eqable_gsmob_ptr_slot (htab, &s_smob_for_lookup.base);
  if (*slot != NULL)
    return (*slot)->containing_scm;

  s_scm = syscm_make_symbol_smob ();
  s_smob = (symbol_smob *) SCM_SMOB_DATA (s_scm);
  s_smob->symbol = symbol;
  gdbscm_fill_eqable_gsmob_ptr_slot (slot, &s_smob->base);

  return s_scm;
}

/* Returns the <gdb:symbol> object in SELF.
   Throws an exception if SELF is not a <gdb:symbol> object.  */

static SCM
syscm_get_symbol_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (syscm_is_symbol (self), self, arg_pos, func_name,
		   symbol_smob_name);

  return self;
}

/* Returns a pointer to the symbol smob of SELF.
   Throws an exception if SELF is not a <gdb:symbol> object.  */

static symbol_smob *
syscm_get_symbol_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM s_scm = syscm_get_symbol_arg_unsafe (self, arg_pos, func_name);
  symbol_smob *s_smob = (symbol_smob *) SCM_SMOB_DATA (s_scm);

  return s_smob;
}

/* Return non-zero if symbol S_SMOB is valid.  */

static int
syscm_is_valid (symbol_smob *s_smob)
{
  return s_smob->symbol != NULL;
}

/* Throw a Scheme error if SELF is not a valid symbol smob.
   Otherwise return a pointer to the symbol smob.  */

static symbol_smob *
syscm_get_valid_symbol_smob_arg_unsafe (SCM self, int arg_pos,
					const char *func_name)
{
  symbol_smob *s_smob
    = syscm_get_symbol_smob_arg_unsafe (self, arg_pos, func_name);

  if (!syscm_is_valid (s_smob))
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("<gdb:symbol>"));
    }

  return s_smob;
}

/* Throw a Scheme error if SELF is not a valid symbol smob.
   Otherwise return a pointer to the symbol struct.  */

struct symbol *
syscm_get_valid_symbol_arg_unsafe (SCM self, int arg_pos,
				   const char *func_name)
{
  symbol_smob *s_smob = syscm_get_valid_symbol_smob_arg_unsafe (self, arg_pos,
								func_name);

  return s_smob->symbol;
}


/* Symbol methods.  */

/* (symbol-valid? <gdb:symbol>) -> boolean
   Returns #t if SELF still exists in GDB.  */

static SCM
gdbscm_symbol_valid_p (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (syscm_is_valid (s_smob));
}

/* (symbol-type <gdb:symbol>) -> <gdb:type>
   Return the type of SELF, or #f if SELF has no type.  */

static SCM
gdbscm_symbol_type (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  if (symbol->type () == NULL)
    return SCM_BOOL_F;

  return tyscm_scm_from_type (symbol->type ());
}

/* (symbol-symtab <gdb:symbol>) -> <gdb:symtab> | #f
   Return the symbol table of SELF.
   If SELF does not have a symtab (it is arch-owned) return #f.  */

static SCM
gdbscm_symbol_symtab (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  if (!symbol->is_objfile_owned ())
    return SCM_BOOL_F;
  return stscm_scm_from_symtab (symbol->symtab ());
}

/* (symbol-name <gdb:symbol>) -> string */

static SCM
gdbscm_symbol_name (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  return gdbscm_scm_from_c_string (symbol->natural_name ());
}

/* (symbol-linkage-name <gdb:symbol>) -> string */

static SCM
gdbscm_symbol_linkage_name (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  return gdbscm_scm_from_c_string (symbol->linkage_name ());
}

/* (symbol-print-name <gdb:symbol>) -> string */

static SCM
gdbscm_symbol_print_name (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  return gdbscm_scm_from_c_string (symbol->print_name ());
}

/* (symbol-addr-class <gdb:symbol>) -> integer */

static SCM
gdbscm_symbol_addr_class (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  return scm_from_int (symbol->aclass ());
}

/* (symbol-argument? <gdb:symbol>) -> boolean */

static SCM
gdbscm_symbol_argument_p (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  return scm_from_bool (symbol->is_argument ());
}

/* (symbol-constant? <gdb:symbol>) -> boolean */

static SCM
gdbscm_symbol_constant_p (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;
  enum address_class theclass;

  theclass = symbol->aclass ();

  return scm_from_bool (theclass == LOC_CONST || theclass == LOC_CONST_BYTES);
}

/* (symbol-function? <gdb:symbol>) -> boolean */

static SCM
gdbscm_symbol_function_p (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;
  enum address_class theclass;

  theclass = symbol->aclass ();

  return scm_from_bool (theclass == LOC_BLOCK);
}

/* (symbol-variable? <gdb:symbol>) -> boolean */

static SCM
gdbscm_symbol_variable_p (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;
  enum address_class theclass;

  theclass = symbol->aclass ();

  return scm_from_bool (!symbol->is_argument ()
			&& (theclass == LOC_LOCAL || theclass == LOC_REGISTER
			    || theclass == LOC_STATIC || theclass == LOC_COMPUTED
			    || theclass == LOC_OPTIMIZED_OUT));
}

/* (symbol-needs-frame? <gdb:symbol>) -> boolean
   Return #t if the symbol needs a frame for evaluation.  */

static SCM
gdbscm_symbol_needs_frame_p (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct symbol *symbol = s_smob->symbol;
  int result = 0;

  gdbscm_gdb_exception exc {};
  try
    {
      result = symbol_read_needs_frame (symbol);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return scm_from_bool (result);
}

/* (symbol-line <gdb:symbol>) -> integer
   Return the line number at which the symbol was defined.  */

static SCM
gdbscm_symbol_line (SCM self)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct symbol *symbol = s_smob->symbol;

  return scm_from_int (symbol->line ());
}

/* (symbol-value <gdb:symbol> [#:frame <gdb:frame>]) -> <gdb:value>
   Return the value of the symbol, or an error in various circumstances.  */

static SCM
gdbscm_symbol_value (SCM self, SCM rest)
{
  symbol_smob *s_smob
    = syscm_get_valid_symbol_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct symbol *symbol = s_smob->symbol;
  SCM keywords[] = { frame_keyword, SCM_BOOL_F };
  int frame_pos = -1;
  SCM frame_scm = SCM_BOOL_F;
  frame_smob *f_smob = NULL;
  struct value *value = NULL;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#O",
			      rest, &frame_pos, &frame_scm);
  if (!gdbscm_is_false (frame_scm))
    f_smob = frscm_get_frame_smob_arg_unsafe (frame_scm, frame_pos, FUNC_NAME);

  if (symbol->aclass () == LOC_TYPEDEF)
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
				 _("cannot get the value of a typedef"));
    }

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame_info;

      if (f_smob != NULL)
	{
	  frame_info = frame_info_ptr (frscm_frame_smob_to_frame (f_smob));
	  if (frame_info == NULL)
	    error (_("Invalid frame"));
	}
      
      if (symbol_read_needs_frame (symbol) && frame_info == NULL)
	error (_("Symbol requires a frame to compute its value"));

      /* TODO: currently, we have no way to recover the block in which SYMBOL
	 was found, so we have no block to pass to read_var_value.  This will
	 yield an incorrect value when symbol is not local to FRAME_INFO (this
	 can happen with nested functions).  */
      value = read_var_value (symbol, NULL, frame_info);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return vlscm_scm_from_value (value);
}

/* (lookup-symbol name [#:block <gdb:block>] [#:domain domain])
     -> (<gdb:symbol> field-of-this?)
   The result is #f if the symbol is not found.
   See comment in lookup_symbol_in_language for field-of-this?.  */

static SCM
gdbscm_lookup_symbol (SCM name_scm, SCM rest)
{
  char *name;
  SCM keywords[] = { block_keyword, domain_keyword, SCM_BOOL_F };
  const struct block *block = NULL;
  SCM block_scm = SCM_BOOL_F;
  int domain = VAR_DOMAIN;
  int block_arg_pos = -1, domain_arg_pos = -1;
  struct field_of_this_result is_a_field_of_this;
  struct symbol *symbol = NULL;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#Oi",
			      name_scm, &name, rest,
			      &block_arg_pos, &block_scm,
			      &domain_arg_pos, &domain);

  if (block_arg_pos >= 0)
    {
      SCM except_scm;

      block = bkscm_scm_to_block (block_scm, block_arg_pos, FUNC_NAME,
				  &except_scm);
      if (block == NULL)
	{
	  xfree (name);
	  gdbscm_throw (except_scm);
	}
    }
  else
    {
      gdbscm_gdb_exception exc {};
      try
	{
	  frame_info_ptr selected_frame
	    = get_selected_frame (_("no frame selected"));
	  block = get_frame_block (selected_frame, NULL);
	}
      catch (const gdb_exception &ex)
	{
	  xfree (name);
	  exc = unpack (ex);
	}
      GDBSCM_HANDLE_GDB_EXCEPTION (exc);
    }

  gdbscm_gdb_exception except {};
  try
    {
      symbol = lookup_symbol (name, block, (domain_enum) domain,
			      &is_a_field_of_this).symbol;
    }
  catch (const gdb_exception &ex)
    {
      except = unpack (ex);
    }

  xfree (name);
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (symbol == NULL)
    return SCM_BOOL_F;

  return scm_list_2 (syscm_scm_from_symbol (symbol),
		     scm_from_bool (is_a_field_of_this.type != NULL));
}

/* (lookup-global-symbol name [#:domain domain]) -> <gdb:symbol>
   The result is #f if the symbol is not found.  */

static SCM
gdbscm_lookup_global_symbol (SCM name_scm, SCM rest)
{
  char *name;
  SCM keywords[] = { domain_keyword, SCM_BOOL_F };
  int domain_arg_pos = -1;
  int domain = VAR_DOMAIN;
  struct symbol *symbol = NULL;
  gdbscm_gdb_exception except {};

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#i",
			      name_scm, &name, rest,
			      &domain_arg_pos, &domain);

  try
    {
      symbol = lookup_global_symbol (name, NULL, (domain_enum) domain).symbol;
    }
  catch (const gdb_exception &ex)
    {
      except = unpack (ex);
    }

  xfree (name);
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (symbol == NULL)
    return SCM_BOOL_F;

  return syscm_scm_from_symbol (symbol);
}

/* Initialize the Scheme symbol support.  */

/* Note: The SYMBOL_ prefix on the integer constants here is present for
   compatibility with the Python support.  */

static const scheme_integer_constant symbol_integer_constants[] =
{
#define X(SYM) { "SYMBOL_" #SYM, SYM }
  X (LOC_UNDEF),
  X (LOC_CONST),
  X (LOC_STATIC),
  X (LOC_REGISTER),
  X (LOC_ARG),
  X (LOC_REF_ARG),
  X (LOC_LOCAL),
  X (LOC_TYPEDEF),
  X (LOC_LABEL),
  X (LOC_BLOCK),
  X (LOC_CONST_BYTES),
  X (LOC_UNRESOLVED),
  X (LOC_OPTIMIZED_OUT),
  X (LOC_COMPUTED),
  X (LOC_REGPARM_ADDR),

  X (UNDEF_DOMAIN),
  X (VAR_DOMAIN),
  X (STRUCT_DOMAIN),
  X (LABEL_DOMAIN),
  X (VARIABLES_DOMAIN),
  X (FUNCTIONS_DOMAIN),
  X (TYPES_DOMAIN),
#undef X

  END_INTEGER_CONSTANTS
};

static const scheme_function symbol_functions[] =
{
  { "symbol?", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_p),
    "\
Return #t if the object is a <gdb:symbol> object." },

  { "symbol-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_valid_p),
    "\
Return #t if object is a valid <gdb:symbol> object.\n\
A valid symbol is a symbol that has not been freed.\n\
Symbols are freed when the objfile they come from is freed." },

  { "symbol-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_type),
    "\
Return the type of symbol." },

  { "symbol-symtab", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_symtab),
    "\
Return the symbol table (<gdb:symtab>) containing symbol." },

  { "symbol-line", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_line),
    "\
Return the line number at which the symbol was defined." },

  { "symbol-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_name),
    "\
Return the name of the symbol as a string." },

  { "symbol-linkage-name", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_symbol_linkage_name),
    "\
Return the linkage name of the symbol as a string." },

  { "symbol-print-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_print_name),
    "\
Return the print name of the symbol as a string.\n\
This is either name or linkage-name, depending on whether the user\n\
asked GDB to display demangled or mangled names." },

  { "symbol-addr-class", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_addr_class),
    "\
Return the address class of the symbol." },

  { "symbol-needs-frame?", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_symbol_needs_frame_p),
    "\
Return #t if the symbol needs a frame to compute its value." },

  { "symbol-argument?", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_argument_p),
    "\
Return #t if the symbol is a function argument." },

  { "symbol-constant?", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_constant_p),
    "\
Return #t if the symbol is a constant." },

  { "symbol-function?", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_function_p),
    "\
Return #t if the symbol is a function." },

  { "symbol-variable?", 1, 0, 0, as_a_scm_t_subr (gdbscm_symbol_variable_p),
    "\
Return #t if the symbol is a variable." },

  { "symbol-value", 1, 0, 1, as_a_scm_t_subr (gdbscm_symbol_value),
    "\
Return the value of the symbol.\n\
\n\
  Arguments: <gdb:symbol> [#:frame frame]" },

  { "lookup-symbol", 1, 0, 1, as_a_scm_t_subr (gdbscm_lookup_symbol),
    "\
Return (<gdb:symbol> field-of-this?) if found, otherwise #f.\n\
\n\
  Arguments: name [#:block block] [#:domain domain]\n\
    name:   a string containing the name of the symbol to lookup\n\
    block:  a <gdb:block> object\n\
    domain: a SYMBOL_*_DOMAIN value" },

  { "lookup-global-symbol", 1, 0, 1,
    as_a_scm_t_subr (gdbscm_lookup_global_symbol),
    "\
Return <gdb:symbol> if found, otherwise #f.\n\
\n\
  Arguments: name [#:domain domain]\n\
    name:   a string containing the name of the symbol to lookup\n\
    domain: a SYMBOL_*_DOMAIN value" },

  END_FUNCTIONS
};

void
gdbscm_initialize_symbols (void)
{
  symbol_smob_tag
    = gdbscm_make_smob_type (symbol_smob_name, sizeof (symbol_smob));
  scm_set_smob_free (symbol_smob_tag, syscm_free_symbol_smob);
  scm_set_smob_print (symbol_smob_tag, syscm_print_symbol_smob);

  gdbscm_define_integer_constants (symbol_integer_constants, 1);
  gdbscm_define_functions (symbol_functions, 1);

  block_keyword = scm_from_latin1_keyword ("block");
  domain_keyword = scm_from_latin1_keyword ("domain");
  frame_keyword = scm_from_latin1_keyword ("frame");
}

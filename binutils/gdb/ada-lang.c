/* Ada language support routines for GDB, the GNU debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "gdbsupport/gdb_regex.h"
#include "frame.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "inferior.h"
#include "symfile.h"
#include "objfiles.h"
#include "breakpoint.h"
#include "gdbcore.h"
#include "hashtab.h"
#include "gdbsupport/gdb_obstack.h"
#include "ada-lang.h"
#include "completer.h"
#include "ui-out.h"
#include "block.h"
#include "infcall.h"
#include "annotate.h"
#include "valprint.h"
#include "source.h"
#include "observable.h"
#include "stack.h"
#include "typeprint.h"
#include "namespace.h"
#include "cli/cli-style.h"
#include "cli/cli-decode.h"

#include "value.h"
#include "mi/mi-common.h"
#include "arch-utils.h"
#include "cli/cli-utils.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/selftest.h"
#include <algorithm>
#include "ada-exp.h"
#include "charset.h"
#include "ax-gdb.h"

static struct type *desc_base_type (struct type *);

static struct type *desc_bounds_type (struct type *);

static struct value *desc_bounds (struct value *);

static int fat_pntr_bounds_bitpos (struct type *);

static int fat_pntr_bounds_bitsize (struct type *);

static struct type *desc_data_target_type (struct type *);

static struct value *desc_data (struct value *);

static int fat_pntr_data_bitpos (struct type *);

static int fat_pntr_data_bitsize (struct type *);

static struct value *desc_one_bound (struct value *, int, int);

static int desc_bound_bitpos (struct type *, int, int);

static int desc_bound_bitsize (struct type *, int, int);

static struct type *desc_index_type (struct type *, int);

static int desc_arity (struct type *);

static int ada_args_match (struct symbol *, struct value **, int);

static struct value *make_array_descriptor (struct type *, struct value *);

static void ada_add_block_symbols (std::vector<struct block_symbol> &,
				   const struct block *,
				   const lookup_name_info &lookup_name,
				   domain_enum, struct objfile *);

static void ada_add_all_symbols (std::vector<struct block_symbol> &,
				 const struct block *,
				 const lookup_name_info &lookup_name,
				 domain_enum, int, int *);

static int is_nonfunction (const std::vector<struct block_symbol> &);

static void add_defn_to_vec (std::vector<struct block_symbol> &,
			     struct symbol *,
			     const struct block *);

static int possible_user_operator_p (enum exp_opcode, struct value **);

static const char *ada_decoded_op_name (enum exp_opcode);

static int numeric_type_p (struct type *);

static int integer_type_p (struct type *);

static int scalar_type_p (struct type *);

static int discrete_type_p (struct type *);

static struct type *ada_lookup_struct_elt_type (struct type *, const char *,
						int, int);

static struct type *ada_find_parallel_type_with_name (struct type *,
						      const char *);

static int is_dynamic_field (struct type *, int);

static struct type *to_fixed_variant_branch_type (struct type *,
						  const gdb_byte *,
						  CORE_ADDR, struct value *);

static struct type *to_fixed_array_type (struct type *, struct value *, int);

static struct type *to_fixed_range_type (struct type *, struct value *);

static struct type *to_static_fixed_type (struct type *);
static struct type *static_unwrap_type (struct type *type);

static struct value *unwrap_value (struct value *);

static struct type *constrained_packed_array_type (struct type *, long *);

static struct type *decode_constrained_packed_array_type (struct type *);

static long decode_packed_array_bitsize (struct type *);

static struct value *decode_constrained_packed_array (struct value *);

static int ada_is_unconstrained_packed_array_type (struct type *);

static struct value *value_subscript_packed (struct value *, int,
					     struct value **);

static struct value *coerce_unspec_val_to_type (struct value *,
						struct type *);

static int lesseq_defined_than (struct symbol *, struct symbol *);

static int equiv_types (struct type *, struct type *);

static int is_name_suffix (const char *);

static int advance_wild_match (const char **, const char *, char);

static bool wild_match (const char *name, const char *patn);

static struct value *ada_coerce_ref (struct value *);

static LONGEST pos_atr (struct value *);

static struct value *val_atr (struct type *, LONGEST);

static struct symbol *standard_lookup (const char *, const struct block *,
				       domain_enum);

static struct value *ada_search_struct_field (const char *, struct value *, int,
					      struct type *);

static int find_struct_field (const char *, struct type *, int,
			      struct type **, int *, int *, int *, int *);

static int ada_resolve_function (std::vector<struct block_symbol> &,
				 struct value **, int, const char *,
				 struct type *, bool);

static int ada_is_direct_array_type (struct type *);

static struct value *ada_index_struct_field (int, struct value *, int,
					     struct type *);

static void add_component_interval (LONGEST, LONGEST, std::vector<LONGEST> &);


static struct type *ada_find_any_type (const char *name);

static symbol_name_matcher_ftype *ada_get_symbol_name_matcher
  (const lookup_name_info &lookup_name);

static int symbols_are_identical_enums
  (const std::vector<struct block_symbol> &syms);

static int ada_identical_enum_types_p (struct type *type1, struct type *type2);


/* The character set used for source files.  */
static const char *ada_source_charset;

/* The string "UTF-8".  This is here so we can check for the UTF-8
   charset using == rather than strcmp.  */
static const char ada_utf8[] = "UTF-8";

/* Each entry in the UTF-32 case-folding table is of this form.  */
struct utf8_entry
{
  /* The start and end, inclusive, of this range of codepoints.  */
  uint32_t start, end;
  /* The delta to apply to get the upper-case form.  0 if this is
     already upper-case.  */
  int upper_delta;
  /* The delta to apply to get the lower-case form.  0 if this is
     already lower-case.  */
  int lower_delta;

  bool operator< (uint32_t val) const
  {
    return end < val;
  }
};

static const utf8_entry ada_case_fold[] =
{
#include "ada-casefold.h"
};



static const char ada_completer_word_break_characters[] =
#ifdef VMS
  " \t\n!@#%^&*()+=|~`}{[]\";:?/,-";
#else
  " \t\n!@#$%^&*()+=|~`}{[]\";:?/,-";
#endif

/* The name of the symbol to use to get the name of the main subprogram.  */
static const char ADA_MAIN_PROGRAM_SYMBOL_NAME[]
  = "__gnat_ada_main_program_name";

/* Limit on the number of warnings to raise per expression evaluation.  */
static int warning_limit = 2;

/* Number of warning messages issued; reset to 0 by cleanups after
   expression evaluation.  */
static int warnings_issued = 0;

static const char * const known_runtime_file_name_patterns[] = {
  ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS NULL
};

static const char * const known_auxiliary_function_name_patterns[] = {
  ADA_KNOWN_AUXILIARY_FUNCTION_NAME_PATTERNS NULL
};

/* Maintenance-related settings for this module.  */

static struct cmd_list_element *maint_set_ada_cmdlist;
static struct cmd_list_element *maint_show_ada_cmdlist;

/* The "maintenance ada set/show ignore-descriptive-type" value.  */

static bool ada_ignore_descriptive_types_p = false;

			/* Inferior-specific data.  */

/* Per-inferior data for this module.  */

struct ada_inferior_data
{
  /* The ada__tags__type_specific_data type, which is used when decoding
     tagged types.  With older versions of GNAT, this type was directly
     accessible through a component ("tsd") in the object tag.  But this
     is no longer the case, so we cache it for each inferior.  */
  struct type *tsd_type = nullptr;

  /* The exception_support_info data.  This data is used to determine
     how to implement support for Ada exception catchpoints in a given
     inferior.  */
  const struct exception_support_info *exception_info = nullptr;
};

/* Our key to this module's inferior data.  */
static const registry<inferior>::key<ada_inferior_data> ada_inferior_data;

/* Return our inferior data for the given inferior (INF).

   This function always returns a valid pointer to an allocated
   ada_inferior_data structure.  If INF's inferior data has not
   been previously set, this functions creates a new one with all
   fields set to zero, sets INF's inferior to it, and then returns
   a pointer to that newly allocated ada_inferior_data.  */

static struct ada_inferior_data *
get_ada_inferior_data (struct inferior *inf)
{
  struct ada_inferior_data *data;

  data = ada_inferior_data.get (inf);
  if (data == NULL)
    data = ada_inferior_data.emplace (inf);

  return data;
}

/* Perform all necessary cleanups regarding our module's inferior data
   that is required after the inferior INF just exited.  */

static void
ada_inferior_exit (struct inferior *inf)
{
  ada_inferior_data.clear (inf);
}


			/* program-space-specific data.  */

/* The result of a symbol lookup to be stored in our symbol cache.  */

struct cache_entry
{
  /* The name used to perform the lookup.  */
  std::string name;
  /* The namespace used during the lookup.  */
  domain_enum domain = UNDEF_DOMAIN;
  /* The symbol returned by the lookup, or NULL if no matching symbol
     was found.  */
  struct symbol *sym = nullptr;
  /* The block where the symbol was found, or NULL if no matching
     symbol was found.  */
  const struct block *block = nullptr;
};

/* The symbol cache uses this type when searching.  */

struct cache_entry_search
{
  const char *name;
  domain_enum domain;

  hashval_t hash () const
  {
    /* This must agree with hash_cache_entry, below.  */
    return htab_hash_string (name);
  }
};

/* Hash function for cache_entry.  */

static hashval_t
hash_cache_entry (const void *v)
{
  const cache_entry *entry = (const cache_entry *) v;
  return htab_hash_string (entry->name.c_str ());
}

/* Equality function for cache_entry.  */

static int
eq_cache_entry (const void *a, const void *b)
{
  const cache_entry *entrya = (const cache_entry *) a;
  const cache_entry_search *entryb = (const cache_entry_search *) b;

  return entrya->domain == entryb->domain && entrya->name == entryb->name;
}

/* Key to our per-program-space data.  */
static const registry<program_space>::key<htab, htab_deleter>
  ada_pspace_data_handle;

/* Return this module's data for the given program space (PSPACE).
   If not is found, add a zero'ed one now.

   This function always returns a valid object.  */

static htab_t
get_ada_pspace_data (struct program_space *pspace)
{
  htab_t data = ada_pspace_data_handle.get (pspace);
  if (data == nullptr)
    {
      data = htab_create_alloc (10, hash_cache_entry, eq_cache_entry,
				htab_delete_entry<cache_entry>,
				xcalloc, xfree);
      ada_pspace_data_handle.set (pspace, data);
    }

  return data;
}

			/* Utilities */

/* If TYPE is a TYPE_CODE_TYPEDEF type, return the target type after
   all typedef layers have been peeled.  Otherwise, return TYPE.

   Normally, we really expect a typedef type to only have 1 typedef layer.
   In other words, we really expect the target type of a typedef type to be
   a non-typedef type.  This is particularly true for Ada units, because
   the language does not have a typedef vs not-typedef distinction.
   In that respect, the Ada compiler has been trying to eliminate as many
   typedef definitions in the debugging information, since they generally
   do not bring any extra information (we still use typedef under certain
   circumstances related mostly to the GNAT encoding).

   Unfortunately, we have seen situations where the debugging information
   generated by the compiler leads to such multiple typedef layers.  For
   instance, consider the following example with stabs:

     .stabs  "pck__float_array___XUP:Tt(0,46)=s16P_ARRAY:(0,47)=[...]"[...]
     .stabs  "pck__float_array___XUP:t(0,36)=(0,46)",128,0,6,0

   This is an error in the debugging information which causes type
   pck__float_array___XUP to be defined twice, and the second time,
   it is defined as a typedef of a typedef.

   This is on the fringe of legality as far as debugging information is
   concerned, and certainly unexpected.  But it is easy to handle these
   situations correctly, so we can afford to be lenient in this case.  */

static struct type *
ada_typedef_target_type (struct type *type)
{
  while (type->code () == TYPE_CODE_TYPEDEF)
    type = type->target_type ();
  return type;
}

/* Given DECODED_NAME a string holding a symbol name in its
   decoded form (ie using the Ada dotted notation), returns
   its unqualified name.  */

static const char *
ada_unqualified_name (const char *decoded_name)
{
  const char *result;
  
  /* If the decoded name starts with '<', it means that the encoded
     name does not follow standard naming conventions, and thus that
     it is not your typical Ada symbol name.  Trying to unqualify it
     is therefore pointless and possibly erroneous.  */
  if (decoded_name[0] == '<')
    return decoded_name;

  result = strrchr (decoded_name, '.');
  if (result != NULL)
    result++;                   /* Skip the dot...  */
  else
    result = decoded_name;

  return result;
}

/* Return a string starting with '<', followed by STR, and '>'.  */

static std::string
add_angle_brackets (const char *str)
{
  return string_printf ("<%s>", str);
}

/* True (non-zero) iff TARGET matches FIELD_NAME up to any trailing
   suffix of FIELD_NAME beginning "___".  */

static int
field_name_match (const char *field_name, const char *target)
{
  int len = strlen (target);

  return
    (strncmp (field_name, target, len) == 0
     && (field_name[len] == '\0'
	 || (startswith (field_name + len, "___")
	     && strcmp (field_name + strlen (field_name) - 6,
			"___XVN") != 0)));
}


/* Assuming TYPE is a TYPE_CODE_STRUCT or a TYPE_CODE_TYPDEF to
   a TYPE_CODE_STRUCT, find the field whose name matches FIELD_NAME,
   and return its index.  This function also handles fields whose name
   have ___ suffixes because the compiler sometimes alters their name
   by adding such a suffix to represent fields with certain constraints.
   If the field could not be found, return a negative number if
   MAYBE_MISSING is set.  Otherwise raise an error.  */

int
ada_get_field_index (const struct type *type, const char *field_name,
		     int maybe_missing)
{
  int fieldno;
  struct type *struct_type = check_typedef ((struct type *) type);

  for (fieldno = 0; fieldno < struct_type->num_fields (); fieldno++)
    if (field_name_match (struct_type->field (fieldno).name (), field_name))
      return fieldno;

  if (!maybe_missing)
    error (_("Unable to find field %s in struct %s.  Aborting"),
	   field_name, struct_type->name ());

  return -1;
}

/* The length of the prefix of NAME prior to any "___" suffix.  */

int
ada_name_prefix_len (const char *name)
{
  if (name == NULL)
    return 0;
  else
    {
      const char *p = strstr (name, "___");

      if (p == NULL)
	return strlen (name);
      else
	return p - name;
    }
}

/* Return non-zero if SUFFIX is a suffix of STR.
   Return zero if STR is null.  */

static int
is_suffix (const char *str, const char *suffix)
{
  int len1, len2;

  if (str == NULL)
    return 0;
  len1 = strlen (str);
  len2 = strlen (suffix);
  return (len1 >= len2 && strcmp (str + len1 - len2, suffix) == 0);
}

/* The contents of value VAL, treated as a value of type TYPE.  The
   result is an lval in memory if VAL is.  */

static struct value *
coerce_unspec_val_to_type (struct value *val, struct type *type)
{
  type = ada_check_typedef (type);
  if (val->type () == type)
    return val;
  else
    {
      struct value *result;

      if (val->optimized_out ())
	result = value::allocate_optimized_out (type);
      else if (val->lazy ()
	       /* Be careful not to make a lazy not_lval value.  */
	       || (val->lval () != not_lval
		   && type->length () > val->type ()->length ()))
	result = value::allocate_lazy (type);
      else
	{
	  result = value::allocate (type);
	  val->contents_copy (result, 0, 0, type->length ());
	}
      result->set_component_location (val);
      result->set_bitsize (val->bitsize ());
      result->set_bitpos (val->bitpos ());
      if (result->lval () == lval_memory)
	result->set_address (val->address ());
      return result;
    }
}

static const gdb_byte *
cond_offset_host (const gdb_byte *valaddr, long offset)
{
  if (valaddr == NULL)
    return NULL;
  else
    return valaddr + offset;
}

static CORE_ADDR
cond_offset_target (CORE_ADDR address, long offset)
{
  if (address == 0)
    return 0;
  else
    return address + offset;
}

/* Issue a warning (as for the definition of warning in utils.c, but
   with exactly one argument rather than ...), unless the limit on the
   number of warnings has passed during the evaluation of the current
   expression.  */

/* FIXME: cagney/2004-10-10: This function is mimicking the behavior
   provided by "complaint".  */
static void lim_warning (const char *format, ...) ATTRIBUTE_PRINTF (1, 2);

static void
lim_warning (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  warnings_issued += 1;
  if (warnings_issued <= warning_limit)
    vwarning (format, args);

  va_end (args);
}

/* Maximum value of a SIZE-byte signed integer type.  */
static LONGEST
max_of_size (int size)
{
  LONGEST top_bit = (LONGEST) 1 << (size * 8 - 2);

  return top_bit | (top_bit - 1);
}

/* Minimum value of a SIZE-byte signed integer type.  */
static LONGEST
min_of_size (int size)
{
  return -max_of_size (size) - 1;
}

/* Maximum value of a SIZE-byte unsigned integer type.  */
static ULONGEST
umax_of_size (int size)
{
  ULONGEST top_bit = (ULONGEST) 1 << (size * 8 - 1);

  return top_bit | (top_bit - 1);
}

/* Maximum value of integral type T, as a signed quantity.  */
static LONGEST
max_of_type (struct type *t)
{
  if (t->is_unsigned ())
    return (LONGEST) umax_of_size (t->length ());
  else
    return max_of_size (t->length ());
}

/* Minimum value of integral type T, as a signed quantity.  */
static LONGEST
min_of_type (struct type *t)
{
  if (t->is_unsigned ())
    return 0;
  else
    return min_of_size (t->length ());
}

/* The largest value in the domain of TYPE, a discrete type, as an integer.  */
LONGEST
ada_discrete_type_high_bound (struct type *type)
{
  type = resolve_dynamic_type (type, {}, 0);
  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      {
	const dynamic_prop &high = type->bounds ()->high;

	if (high.is_constant ())
	  return high.const_val ();
	else
	  {
	    gdb_assert (high.kind () == PROP_UNDEFINED);

	    /* This happens when trying to evaluate a type's dynamic bound
	       without a live target.  There is nothing relevant for us to
	       return here, so return 0.  */
	    return 0;
	  }
      }
    case TYPE_CODE_ENUM:
      return type->field (type->num_fields () - 1).loc_enumval ();
    case TYPE_CODE_BOOL:
      return 1;
    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      return max_of_type (type);
    default:
      error (_("Unexpected type in ada_discrete_type_high_bound."));
    }
}

/* The smallest value in the domain of TYPE, a discrete type, as an integer.  */
LONGEST
ada_discrete_type_low_bound (struct type *type)
{
  type = resolve_dynamic_type (type, {}, 0);
  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      {
	const dynamic_prop &low = type->bounds ()->low;

	if (low.is_constant ())
	  return low.const_val ();
	else
	  {
	    gdb_assert (low.kind () == PROP_UNDEFINED);

	    /* This happens when trying to evaluate a type's dynamic bound
	       without a live target.  There is nothing relevant for us to
	       return here, so return 0.  */
	    return 0;
	  }
      }
    case TYPE_CODE_ENUM:
      return type->field (0).loc_enumval ();
    case TYPE_CODE_BOOL:
      return 0;
    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      return min_of_type (type);
    default:
      error (_("Unexpected type in ada_discrete_type_low_bound."));
    }
}

/* The identity on non-range types.  For range types, the underlying
   non-range scalar type.  */

static struct type *
get_base_type (struct type *type)
{
  while (type != NULL && type->code () == TYPE_CODE_RANGE)
    {
      if (type == type->target_type () || type->target_type () == NULL)
	return type;
      type = type->target_type ();
    }
  return type;
}

/* Return a decoded version of the given VALUE.  This means returning
   a value whose type is obtained by applying all the GNAT-specific
   encodings, making the resulting type a static but standard description
   of the initial type.  */

struct value *
ada_get_decoded_value (struct value *value)
{
  struct type *type = ada_check_typedef (value->type ());

  if (ada_is_array_descriptor_type (type)
      || (ada_is_constrained_packed_array_type (type)
	  && type->code () != TYPE_CODE_PTR))
    {
      if (type->code () == TYPE_CODE_TYPEDEF)  /* array access type.  */
	value = ada_coerce_to_simple_array_ptr (value);
      else
	value = ada_coerce_to_simple_array (value);
    }
  else
    value = ada_to_fixed_value (value);

  return value;
}

/* Same as ada_get_decoded_value, but with the given TYPE.
   Because there is no associated actual value for this type,
   the resulting type might be a best-effort approximation in
   the case of dynamic types.  */

struct type *
ada_get_decoded_type (struct type *type)
{
  type = to_static_fixed_type (type);
  if (ada_is_constrained_packed_array_type (type))
    type = ada_coerce_to_simple_array_type (type);
  return type;
}



				/* Language Selection */

/* If the main procedure is written in Ada, then return its name.
   The result is good until the next call.  Return NULL if the main
   procedure doesn't appear to be in Ada.  */

const char *
ada_main_name ()
{
  struct bound_minimal_symbol msym;
  static gdb::unique_xmalloc_ptr<char> main_program_name;

  /* For Ada, the name of the main procedure is stored in a specific
     string constant, generated by the binder.  Look for that symbol,
     extract its address, and then read that string.  If we didn't find
     that string, then most probably the main procedure is not written
     in Ada.  */
  msym = lookup_minimal_symbol (ADA_MAIN_PROGRAM_SYMBOL_NAME, NULL, NULL);

  if (msym.minsym != NULL)
    {
      CORE_ADDR main_program_name_addr = msym.value_address ();
      if (main_program_name_addr == 0)
	error (_("Invalid address for Ada main program name."));

      /* Force trust_readonly, because we always want to fetch this
	 string from the executable, not from inferior memory.  If the
	 user changes the exec-file and invokes "start", we want to
	 pick the "main" from the new executable, not one that may
	 come from the still-live inferior.  */
      scoped_restore save_trust_readonly
	= make_scoped_restore (&trust_readonly, true);
      main_program_name = target_read_string (main_program_name_addr, 1024);
      return main_program_name.get ();
    }

  /* The main procedure doesn't seem to be in Ada.  */
  return NULL;
}

				/* Symbols */

/* Table of Ada operators and their GNAT-encoded names.  Last entry is pair
   of NULLs.  */

const struct ada_opname_map ada_opname_table[] = {
  {"Oadd", "\"+\"", BINOP_ADD},
  {"Osubtract", "\"-\"", BINOP_SUB},
  {"Omultiply", "\"*\"", BINOP_MUL},
  {"Odivide", "\"/\"", BINOP_DIV},
  {"Omod", "\"mod\"", BINOP_MOD},
  {"Orem", "\"rem\"", BINOP_REM},
  {"Oexpon", "\"**\"", BINOP_EXP},
  {"Olt", "\"<\"", BINOP_LESS},
  {"Ole", "\"<=\"", BINOP_LEQ},
  {"Ogt", "\">\"", BINOP_GTR},
  {"Oge", "\">=\"", BINOP_GEQ},
  {"Oeq", "\"=\"", BINOP_EQUAL},
  {"One", "\"/=\"", BINOP_NOTEQUAL},
  {"Oand", "\"and\"", BINOP_BITWISE_AND},
  {"Oor", "\"or\"", BINOP_BITWISE_IOR},
  {"Oxor", "\"xor\"", BINOP_BITWISE_XOR},
  {"Oconcat", "\"&\"", BINOP_CONCAT},
  {"Oabs", "\"abs\"", UNOP_ABS},
  {"Onot", "\"not\"", UNOP_LOGICAL_NOT},
  {"Oadd", "\"+\"", UNOP_PLUS},
  {"Osubtract", "\"-\"", UNOP_NEG},
  {NULL, NULL}
};

/* If STR is a decoded version of a compiler-provided suffix (like the
   "[cold]" in "symbol[cold]"), return true.  Otherwise, return
   false.  */

static bool
is_compiler_suffix (const char *str)
{
  gdb_assert (*str == '[');
  ++str;
  while (*str != '\0' && isalpha (*str))
    ++str;
  /* We accept a missing "]" in order to support completion.  */
  return *str == '\0' || (str[0] == ']' && str[1] == '\0');
}

/* Append a non-ASCII character to RESULT.  */
static void
append_hex_encoded (std::string &result, uint32_t one_char)
{
  if (one_char <= 0xff)
    {
      result.append ("U");
      result.append (phex (one_char, 1));
    }
  else if (one_char <= 0xffff)
    {
      result.append ("W");
      result.append (phex (one_char, 2));
    }
  else
    {
      result.append ("WW");
      result.append (phex (one_char, 4));
    }
}

/* Return a string that is a copy of the data in STORAGE, with
   non-ASCII characters replaced by the appropriate hex encoding.  A
   template is used because, for UTF-8, we actually want to work with
   UTF-32 codepoints.  */
template<typename T>
std::string
copy_and_hex_encode (struct obstack *storage)
{
  const T *chars = (T *) obstack_base (storage);
  int num_chars = obstack_object_size (storage) / sizeof (T);
  std::string result;
  for (int i = 0; i < num_chars; ++i)
    {
      if (chars[i] <= 0x7f)
	{
	  /* The host character set has to be a superset of ASCII, as
	     are all the other character sets we can use.  */
	  result.push_back (chars[i]);
	}
      else
	append_hex_encoded (result, chars[i]);
    }
  return result;
}

/* The "encoded" form of DECODED, according to GNAT conventions.  If
   THROW_ERRORS, throw an error if invalid operator name is found.
   Otherwise, return the empty string in that case.  */

static std::string
ada_encode_1 (const char *decoded, bool throw_errors)
{
  if (decoded == NULL)
    return {};

  std::string encoding_buffer;
  bool saw_non_ascii = false;
  for (const char *p = decoded; *p != '\0'; p += 1)
    {
      if ((*p & 0x80) != 0)
	saw_non_ascii = true;

      if (*p == '.')
	encoding_buffer.append ("__");
      else if (*p == '[' && is_compiler_suffix (p))
	{
	  encoding_buffer = encoding_buffer + "." + (p + 1);
	  if (encoding_buffer.back () == ']')
	    encoding_buffer.pop_back ();
	  break;
	}
      else if (*p == '"')
	{
	  const struct ada_opname_map *mapping;

	  for (mapping = ada_opname_table;
	       mapping->encoded != NULL
	       && !startswith (p, mapping->decoded); mapping += 1)
	    ;
	  if (mapping->encoded == NULL)
	    {
	      if (throw_errors)
		error (_("invalid Ada operator name: %s"), p);
	      else
		return {};
	    }
	  encoding_buffer.append (mapping->encoded);
	  break;
	}
      else
	encoding_buffer.push_back (*p);
    }

  /* If a non-ASCII character is seen, we must convert it to the
     appropriate hex form.  As this is more expensive, we keep track
     of whether it is even necessary.  */
  if (saw_non_ascii)
    {
      auto_obstack storage;
      bool is_utf8 = ada_source_charset == ada_utf8;
      try
	{
	  convert_between_encodings
	    (host_charset (),
	     is_utf8 ? HOST_UTF32 : ada_source_charset,
	     (const gdb_byte *) encoding_buffer.c_str (),
	     encoding_buffer.length (), 1,
	     &storage, translit_none);
	}
      catch (const gdb_exception &)
	{
	  static bool warned = false;

	  /* Converting to UTF-32 shouldn't fail, so if it doesn't, we
	     might like to know why.  */
	  if (!warned)
	    {
	      warned = true;
	      warning (_("charset conversion failure for '%s'.\n"
			 "You may have the wrong value for 'set ada source-charset'."),
		       encoding_buffer.c_str ());
	    }

	  /* We don't try to recover from errors.  */
	  return encoding_buffer;
	}

      if (is_utf8)
	return copy_and_hex_encode<uint32_t> (&storage);
      return copy_and_hex_encode<gdb_byte> (&storage);
    }

  return encoding_buffer;
}

/* Find the entry for C in the case-folding table.  Return nullptr if
   the entry does not cover C.  */
static const utf8_entry *
find_case_fold_entry (uint32_t c)
{
  auto iter = std::lower_bound (std::begin (ada_case_fold),
				std::end (ada_case_fold),
				c);
  if (iter == std::end (ada_case_fold)
      || c < iter->start
      || c > iter->end)
    return nullptr;
  return &*iter;
}

/* Return NAME folded to lower case, or, if surrounded by single
   quotes, unfolded, but with the quotes stripped away.  If
   THROW_ON_ERROR is true, encoding failures will throw an exception
   rather than emitting a warning.  Result good to next call.  */

static const char *
ada_fold_name (std::string_view name, bool throw_on_error = false)
{
  static std::string fold_storage;

  if (!name.empty () && name[0] == '\'')
    fold_storage = name.substr (1, name.size () - 2);
  else
    {
      /* Why convert to UTF-32 and implement our own case-folding,
	 rather than convert to wchar_t and use the platform's
	 functions?  I'm glad you asked.

	 The main problem is that GNAT implements an unusual rule for
	 case folding.  For ASCII letters, letters in single-byte
	 encodings (such as ISO-8859-*), and Unicode letters that fit
	 in a single byte (i.e., code point is <= 0xff), the letter is
	 folded to lower case.  Other Unicode letters are folded to
	 upper case.

	 This rule means that the code must be able to examine the
	 value of the character.  And, some hosts do not use Unicode
	 for wchar_t, so examining the value of such characters is
	 forbidden.  */
      auto_obstack storage;
      try
	{
	  convert_between_encodings
	    (host_charset (), HOST_UTF32,
	     (const gdb_byte *) name.data (),
	     name.length (), 1,
	     &storage, translit_none);
	}
      catch (const gdb_exception &)
	{
	  if (throw_on_error)
	    throw;

	  static bool warned = false;

	  /* Converting to UTF-32 shouldn't fail, so if it doesn't, we
	     might like to know why.  */
	  if (!warned)
	    {
	      warned = true;
	      warning (_("could not convert '%s' from the host encoding (%s) to UTF-32.\n"
			 "This normally should not happen, please file a bug report."),
		       std::string (name).c_str (), host_charset ());
	    }

	  /* We don't try to recover from errors; just return the
	     original string.  */
	  fold_storage = name;
	  return fold_storage.c_str ();
	}

      bool is_utf8 = ada_source_charset == ada_utf8;
      uint32_t *chars = (uint32_t *) obstack_base (&storage);
      int num_chars = obstack_object_size (&storage) / sizeof (uint32_t);
      for (int i = 0; i < num_chars; ++i)
	{
	  const struct utf8_entry *entry = find_case_fold_entry (chars[i]);
	  if (entry != nullptr)
	    {
	      uint32_t low = chars[i] + entry->lower_delta;
	      if (!is_utf8 || low <= 0xff)
		chars[i] = low;
	      else
		chars[i] = chars[i] + entry->upper_delta;
	    }
	}

      /* Now convert back to ordinary characters.  */
      auto_obstack reconverted;
      try
	{
	  convert_between_encodings (HOST_UTF32,
				     host_charset (),
				     (const gdb_byte *) chars,
				     num_chars * sizeof (uint32_t),
				     sizeof (uint32_t),
				     &reconverted,
				     translit_none);
	  obstack_1grow (&reconverted, '\0');
	  fold_storage = std::string ((const char *) obstack_base (&reconverted));
	}
      catch (const gdb_exception &)
	{
	  if (throw_on_error)
	    throw;

	  static bool warned = false;

	  /* Converting back from UTF-32 shouldn't normally fail, but
	     there are some host encodings without upper/lower
	     equivalence.  */
	  if (!warned)
	    {
	      warned = true;
	      warning (_("could not convert the lower-cased variant of '%s'\n"
			 "from UTF-32 to the host encoding (%s)."),
		       std::string (name).c_str (), host_charset ());
	    }

	  /* We don't try to recover from errors; just return the
	     original string.  */
	  fold_storage = name;
	}
    }

  return fold_storage.c_str ();
}

/* The "encoded" form of DECODED, according to GNAT conventions.  If
   FOLD is true (the default), case-fold any ordinary symbol.  Symbols
   with <...> quoting are not folded in any case.  */

std::string
ada_encode (const char *decoded, bool fold)
{
  if (fold && decoded[0] != '<')
    decoded = ada_fold_name (decoded);
  return ada_encode_1 (decoded, true);
}

/* Return nonzero if C is either a digit or a lowercase alphabet character.  */

static int
is_lower_alphanum (const char c)
{
  return (isdigit (c) || (isalpha (c) && islower (c)));
}

/* ENCODED is the linkage name of a symbol and LEN contains its length.
   This function saves in LEN the length of that same symbol name but
   without either of these suffixes:
     . .{DIGIT}+
     . ${DIGIT}+
     . ___{DIGIT}+
     . __{DIGIT}+.

   These are suffixes introduced by the compiler for entities such as
   nested subprogram for instance, in order to avoid name clashes.
   They do not serve any purpose for the debugger.  */

static void
ada_remove_trailing_digits (const char *encoded, int *len)
{
  if (*len > 1 && isdigit (encoded[*len - 1]))
    {
      int i = *len - 2;

      while (i > 0 && isdigit (encoded[i]))
	i--;
      if (i >= 0 && encoded[i] == '.')
	*len = i;
      else if (i >= 0 && encoded[i] == '$')
	*len = i;
      else if (i >= 2 && startswith (encoded + i - 2, "___"))
	*len = i - 2;
      else if (i >= 1 && startswith (encoded + i - 1, "__"))
	*len = i - 1;
    }
}

/* Remove the suffix introduced by the compiler for protected object
   subprograms.  */

static void
ada_remove_po_subprogram_suffix (const char *encoded, int *len)
{
  /* Remove trailing N.  */

  /* Protected entry subprograms are broken into two
     separate subprograms: The first one is unprotected, and has
     a 'N' suffix; the second is the protected version, and has
     the 'P' suffix.  The second calls the first one after handling
     the protection.  Since the P subprograms are internally generated,
     we leave these names undecoded, giving the user a clue that this
     entity is internal.  */

  if (*len > 1
      && encoded[*len - 1] == 'N'
      && (isdigit (encoded[*len - 2]) || islower (encoded[*len - 2])))
    *len = *len - 1;
}

/* If ENCODED ends with a compiler-provided suffix (like ".cold"),
   then update *LEN to remove the suffix and return the offset of the
   character just past the ".".  Otherwise, return -1.  */

static int
remove_compiler_suffix (const char *encoded, int *len)
{
  int offset = *len - 1;
  while (offset > 0 && isalpha (encoded[offset]))
    --offset;
  if (offset > 0 && encoded[offset] == '.')
    {
      *len = offset;
      return offset + 1;
    }
  return -1;
}

/* Convert an ASCII hex string to a number.  Reads exactly N
   characters from STR.  Returns true on success, false if one of the
   digits was not a hex digit.  */
static bool
convert_hex (const char *str, int n, uint32_t *out)
{
  uint32_t result = 0;

  for (int i = 0; i < n; ++i)
    {
      if (!isxdigit (str[i]))
	return false;
      result <<= 4;
      result |= fromhex (str[i]);
    }

  *out = result;
  return true;
}

/* Convert a wide character from its ASCII hex representation in STR
   (consisting of exactly N characters) to the host encoding,
   appending the resulting bytes to OUT.  If N==2 and the Ada source
   charset is not UTF-8, then hex refers to an encoding in the
   ADA_SOURCE_CHARSET; otherwise, use UTF-32.  Return true on success.
   Return false and do not modify OUT on conversion failure.  */
static bool
convert_from_hex_encoded (std::string &out, const char *str, int n)
{
  uint32_t value;

  if (!convert_hex (str, n, &value))
    return false;
  try
    {
      auto_obstack bytes;
      /* In the 'U' case, the hex digits encode the character in the
	 Ada source charset.  However, if the source charset is UTF-8,
	 this really means it is a single-byte UTF-32 character.  */
      if (n == 2 && ada_source_charset != ada_utf8)
	{
	  gdb_byte one_char = (gdb_byte) value;

	  convert_between_encodings (ada_source_charset, host_charset (),
				     &one_char,
				     sizeof (one_char), sizeof (one_char),
				     &bytes, translit_none);
	}
      else
	convert_between_encodings (HOST_UTF32, host_charset (),
				   (const gdb_byte *) &value,
				   sizeof (value), sizeof (value),
				   &bytes, translit_none);
      obstack_1grow (&bytes, '\0');
      out.append ((const char *) obstack_base (&bytes));
    }
  catch (const gdb_exception &)
    {
      /* On failure, the caller will just let the encoded form
	 through, which seems basically reasonable.  */
      return false;
    }

  return true;
}

/* See ada-lang.h.  */

std::string
ada_decode (const char *encoded, bool wrap, bool operators, bool wide)
{
  int i;
  int len0;
  const char *p;
  int at_start_name;
  std::string decoded;
  int suffix = -1;

  /* With function descriptors on PPC64, the value of a symbol named
     ".FN", if it exists, is the entry point of the function "FN".  */
  if (encoded[0] == '.')
    encoded += 1;

  /* The name of the Ada main procedure starts with "_ada_".
     This prefix is not part of the decoded name, so skip this part
     if we see this prefix.  */
  if (startswith (encoded, "_ada_"))
    encoded += 5;
  /* The "___ghost_" prefix is used for ghost entities.  Normally
     these aren't preserved but when they are, it's useful to see
     them.  */
  if (startswith (encoded, "___ghost_"))
    encoded += 9;

  /* If the name starts with '_', then it is not a properly encoded
     name, so do not attempt to decode it.  Similarly, if the name
     starts with '<', the name should not be decoded.  */
  if (encoded[0] == '_' || encoded[0] == '<')
    goto Suppress;

  len0 = strlen (encoded);

  suffix = remove_compiler_suffix (encoded, &len0);

  ada_remove_trailing_digits (encoded, &len0);
  ada_remove_po_subprogram_suffix (encoded, &len0);

  /* Remove the ___X.* suffix if present.  Do not forget to verify that
     the suffix is located before the current "end" of ENCODED.  We want
     to avoid re-matching parts of ENCODED that have previously been
     marked as discarded (by decrementing LEN0).  */
  p = strstr (encoded, "___");
  if (p != NULL && p - encoded < len0 - 3)
    {
      if (p[3] == 'X')
	len0 = p - encoded;
      else
	goto Suppress;
    }

  /* Remove any trailing TKB suffix.  It tells us that this symbol
     is for the body of a task, but that information does not actually
     appear in the decoded name.  */

  if (len0 > 3 && startswith (encoded + len0 - 3, "TKB"))
    len0 -= 3;

  /* Remove any trailing TB suffix.  The TB suffix is slightly different
     from the TKB suffix because it is used for non-anonymous task
     bodies.  */

  if (len0 > 2 && startswith (encoded + len0 - 2, "TB"))
    len0 -= 2;

  /* Remove trailing "B" suffixes.  */
  /* FIXME: brobecker/2006-04-19: Not sure what this are used for...  */

  if (len0 > 1 && startswith (encoded + len0 - 1, "B"))
    len0 -= 1;

  /* Remove trailing __{digit}+ or trailing ${digit}+.  */

  if (len0 > 1 && isdigit (encoded[len0 - 1]))
    {
      i = len0 - 2;
      while ((i >= 0 && isdigit (encoded[i]))
	     || (i >= 1 && encoded[i] == '_' && isdigit (encoded[i - 1])))
	i -= 1;
      if (i > 1 && encoded[i] == '_' && encoded[i - 1] == '_')
	len0 = i - 1;
      else if (i >= 0 && encoded[i] == '$')
	len0 = i;
    }

  /* The first few characters that are not alphabetic are not part
     of any encoding we use, so we can copy them over verbatim.  */

  for (i = 0; i < len0 && !isalpha (encoded[i]); i += 1)
    decoded.push_back (encoded[i]);

  at_start_name = 1;
  while (i < len0)
    {
      /* Is this a symbol function?  */
      if (operators && at_start_name && encoded[i] == 'O')
	{
	  int k;

	  for (k = 0; ada_opname_table[k].encoded != NULL; k += 1)
	    {
	      int op_len = strlen (ada_opname_table[k].encoded);
	      if ((strncmp (ada_opname_table[k].encoded + 1, encoded + i + 1,
			    op_len - 1) == 0)
		  && !isalnum (encoded[i + op_len]))
		{
		  decoded.append (ada_opname_table[k].decoded);
		  at_start_name = 0;
		  i += op_len;
		  break;
		}
	    }
	  if (ada_opname_table[k].encoded != NULL)
	    continue;
	}
      at_start_name = 0;

      /* Replace "TK__" with "__", which will eventually be translated
	 into "." (just below).  */

      if (i < len0 - 4 && startswith (encoded + i, "TK__"))
	i += 2;

      /* Replace "__B_{DIGITS}+__" sequences by "__", which will eventually
	 be translated into "." (just below).  These are internal names
	 generated for anonymous blocks inside which our symbol is nested.  */

      if (len0 - i > 5 && encoded [i] == '_' && encoded [i+1] == '_'
	  && encoded [i+2] == 'B' && encoded [i+3] == '_'
	  && isdigit (encoded [i+4]))
	{
	  int k = i + 5;
	  
	  while (k < len0 && isdigit (encoded[k]))
	    k++;  /* Skip any extra digit.  */

	  /* Double-check that the "__B_{DIGITS}+" sequence we found
	     is indeed followed by "__".  */
	  if (len0 - k > 2 && encoded [k] == '_' && encoded [k+1] == '_')
	    i = k;
	}

      /* Remove _E{DIGITS}+[sb] */

      /* Just as for protected object subprograms, there are 2 categories
	 of subprograms created by the compiler for each entry.  The first
	 one implements the actual entry code, and has a suffix following
	 the convention above; the second one implements the barrier and
	 uses the same convention as above, except that the 'E' is replaced
	 by a 'B'.

	 Just as above, we do not decode the name of barrier functions
	 to give the user a clue that the code he is debugging has been
	 internally generated.  */

      if (len0 - i > 3 && encoded [i] == '_' && encoded[i+1] == 'E'
	  && isdigit (encoded[i+2]))
	{
	  int k = i + 3;

	  while (k < len0 && isdigit (encoded[k]))
	    k++;

	  if (k < len0
	      && (encoded[k] == 'b' || encoded[k] == 's'))
	    {
	      k++;
	      /* Just as an extra precaution, make sure that if this
		 suffix is followed by anything else, it is a '_'.
		 Otherwise, we matched this sequence by accident.  */
	      if (k == len0
		  || (k < len0 && encoded[k] == '_'))
		i = k;
	    }
	}

      /* Remove trailing "N" in [a-z0-9]+N__.  The N is added by
	 the GNAT front-end in protected object subprograms.  */

      if (i < len0 + 3
	  && encoded[i] == 'N' && encoded[i+1] == '_' && encoded[i+2] == '_')
	{
	  /* Backtrack a bit up until we reach either the begining of
	     the encoded name, or "__".  Make sure that we only find
	     digits or lowercase characters.  */
	  const char *ptr = encoded + i - 1;

	  while (ptr >= encoded && is_lower_alphanum (ptr[0]))
	    ptr--;
	  if (ptr < encoded
	      || (ptr > encoded && ptr[0] == '_' && ptr[-1] == '_'))
	    i++;
	}

      if (wide && i < len0 + 3 && encoded[i] == 'U' && isxdigit (encoded[i + 1]))
	{
	  if (convert_from_hex_encoded (decoded, &encoded[i + 1], 2))
	    {
	      i += 3;
	      continue;
	    }
	}
      else if (wide && i < len0 + 5 && encoded[i] == 'W' && isxdigit (encoded[i + 1]))
	{
	  if (convert_from_hex_encoded (decoded, &encoded[i + 1], 4))
	    {
	      i += 5;
	      continue;
	    }
	}
      else if (wide && i < len0 + 10 && encoded[i] == 'W' && encoded[i + 1] == 'W'
	       && isxdigit (encoded[i + 2]))
	{
	  if (convert_from_hex_encoded (decoded, &encoded[i + 2], 8))
	    {
	      i += 10;
	      continue;
	    }
	}

      if (encoded[i] == 'X' && i != 0 && isalnum (encoded[i - 1]))
	{
	  /* This is a X[bn]* sequence not separated from the previous
	     part of the name with a non-alpha-numeric character (in other
	     words, immediately following an alpha-numeric character), then
	     verify that it is placed at the end of the encoded name.  If
	     not, then the encoding is not valid and we should abort the
	     decoding.  Otherwise, just skip it, it is used in body-nested
	     package names.  */
	  do
	    i += 1;
	  while (i < len0 && (encoded[i] == 'b' || encoded[i] == 'n'));
	  if (i < len0)
	    goto Suppress;
	}
      else if (i < len0 - 2 && encoded[i] == '_' && encoded[i + 1] == '_')
	{
	 /* Replace '__' by '.'.  */
	  decoded.push_back ('.');
	  at_start_name = 1;
	  i += 2;
	}
      else
	{
	  /* It's a character part of the decoded name, so just copy it
	     over.  */
	  decoded.push_back (encoded[i]);
	  i += 1;
	}
    }

  /* Decoded names should never contain any uppercase character.
     Double-check this, and abort the decoding if we find one.  */

  if (operators)
    {
      for (i = 0; i < decoded.length(); ++i)
	if (isupper (decoded[i]) || decoded[i] == ' ')
	  goto Suppress;
    }

  /* If the compiler added a suffix, append it now.  */
  if (suffix >= 0)
    decoded = decoded + "[" + &encoded[suffix] + "]";

  return decoded;

Suppress:
  if (!wrap)
    return {};

  if (encoded[0] == '<')
    decoded = encoded;
  else
    decoded = '<' + std::string(encoded) + '>';
  return decoded;
}

#ifdef GDB_SELF_TEST

static void
ada_decode_tests ()
{
  /* This isn't valid, but used to cause a crash.  PR gdb/30639.  The
     result does not really matter very much.  */
  SELF_CHECK (ada_decode ("44") == "44");
}

#endif

/* Table for keeping permanent unique copies of decoded names.  Once
   allocated, names in this table are never released.  While this is a
   storage leak, it should not be significant unless there are massive
   changes in the set of decoded names in successive versions of a 
   symbol table loaded during a single session.  */
static struct htab *decoded_names_store;

/* Returns the decoded name of GSYMBOL, as for ada_decode, caching it
   in the language-specific part of GSYMBOL, if it has not been
   previously computed.  Tries to save the decoded name in the same
   obstack as GSYMBOL, if possible, and otherwise on the heap (so that,
   in any case, the decoded symbol has a lifetime at least that of
   GSYMBOL).
   The GSYMBOL parameter is "mutable" in the C++ sense: logically
   const, but nevertheless modified to a semantically equivalent form
   when a decoded name is cached in it.  */

const char *
ada_decode_symbol (const struct general_symbol_info *arg)
{
  struct general_symbol_info *gsymbol = (struct general_symbol_info *) arg;
  const char **resultp =
    &gsymbol->language_specific.demangled_name;

  if (!gsymbol->ada_mangled)
    {
      std::string decoded = ada_decode (gsymbol->linkage_name ());
      struct obstack *obstack = gsymbol->language_specific.obstack;

      gsymbol->ada_mangled = 1;

      if (obstack != NULL)
	*resultp = obstack_strdup (obstack, decoded.c_str ());
      else
	{
	  /* Sometimes, we can't find a corresponding objfile, in
	     which case, we put the result on the heap.  Since we only
	     decode when needed, we hope this usually does not cause a
	     significant memory leak (FIXME).  */

	  char **slot = (char **) htab_find_slot (decoded_names_store,
						  decoded.c_str (), INSERT);

	  if (*slot == NULL)
	    *slot = xstrdup (decoded.c_str ());
	  *resultp = *slot;
	}
    }

  return *resultp;
}



				/* Arrays */

/* Assuming that INDEX_DESC_TYPE is an ___XA structure, a structure
   generated by the GNAT compiler to describe the index type used
   for each dimension of an array, check whether it follows the latest
   known encoding.  If not, fix it up to conform to the latest encoding.
   Otherwise, do nothing.  This function also does nothing if
   INDEX_DESC_TYPE is NULL.

   The GNAT encoding used to describe the array index type evolved a bit.
   Initially, the information would be provided through the name of each
   field of the structure type only, while the type of these fields was
   described as unspecified and irrelevant.  The debugger was then expected
   to perform a global type lookup using the name of that field in order
   to get access to the full index type description.  Because these global
   lookups can be very expensive, the encoding was later enhanced to make
   the global lookup unnecessary by defining the field type as being
   the full index type description.

   The purpose of this routine is to allow us to support older versions
   of the compiler by detecting the use of the older encoding, and by
   fixing up the INDEX_DESC_TYPE to follow the new one (at this point,
   we essentially replace each field's meaningless type by the associated
   index subtype).  */

void
ada_fixup_array_indexes_type (struct type *index_desc_type)
{
  int i;

  if (index_desc_type == NULL)
    return;
  gdb_assert (index_desc_type->num_fields () > 0);

  /* Check if INDEX_DESC_TYPE follows the older encoding (it is sufficient
     to check one field only, no need to check them all).  If not, return
     now.

     If our INDEX_DESC_TYPE was generated using the older encoding,
     the field type should be a meaningless integer type whose name
     is not equal to the field name.  */
  if (index_desc_type->field (0).type ()->name () != NULL
      && strcmp (index_desc_type->field (0).type ()->name (),
		 index_desc_type->field (0).name ()) == 0)
    return;

  /* Fixup each field of INDEX_DESC_TYPE.  */
  for (i = 0; i < index_desc_type->num_fields (); i++)
   {
     const char *name = index_desc_type->field (i).name ();
     struct type *raw_type = ada_check_typedef (ada_find_any_type (name));

     if (raw_type)
       index_desc_type->field (i).set_type (raw_type);
   }
}

/* The desc_* routines return primitive portions of array descriptors
   (fat pointers).  */

/* The descriptor or array type, if any, indicated by TYPE; removes
   level of indirection, if needed.  */

static struct type *
desc_base_type (struct type *type)
{
  if (type == NULL)
    return NULL;
  type = ada_check_typedef (type);
  if (type->code () == TYPE_CODE_TYPEDEF)
    type = ada_typedef_target_type (type);

  if (type != NULL
      && (type->code () == TYPE_CODE_PTR
	  || type->code () == TYPE_CODE_REF))
    return ada_check_typedef (type->target_type ());
  else
    return type;
}

/* True iff TYPE indicates a "thin" array pointer type.  */

static int
is_thin_pntr (struct type *type)
{
  return
    is_suffix (ada_type_name (desc_base_type (type)), "___XUT")
    || is_suffix (ada_type_name (desc_base_type (type)), "___XUT___XVE");
}

/* The descriptor type for thin pointer type TYPE.  */

static struct type *
thin_descriptor_type (struct type *type)
{
  struct type *base_type = desc_base_type (type);

  if (base_type == NULL)
    return NULL;
  if (is_suffix (ada_type_name (base_type), "___XVE"))
    return base_type;
  else
    {
      struct type *alt_type = ada_find_parallel_type (base_type, "___XVE");

      if (alt_type == NULL)
	return base_type;
      else
	return alt_type;
    }
}

/* A pointer to the array data for thin-pointer value VAL.  */

static struct value *
thin_data_pntr (struct value *val)
{
  struct type *type = ada_check_typedef (val->type ());
  struct type *data_type = desc_data_target_type (thin_descriptor_type (type));

  data_type = lookup_pointer_type (data_type);

  if (type->code () == TYPE_CODE_PTR)
    return value_cast (data_type, val->copy ());
  else
    return value_from_longest (data_type, val->address ());
}

/* True iff TYPE indicates a "thick" array pointer type.  */

static int
is_thick_pntr (struct type *type)
{
  type = desc_base_type (type);
  return (type != NULL && type->code () == TYPE_CODE_STRUCT
	  && lookup_struct_elt_type (type, "P_BOUNDS", 1) != NULL);
}

/* If TYPE is the type of an array descriptor (fat or thin pointer) or a
   pointer to one, the type of its bounds data; otherwise, NULL.  */

static struct type *
desc_bounds_type (struct type *type)
{
  struct type *r;

  type = desc_base_type (type);

  if (type == NULL)
    return NULL;
  else if (is_thin_pntr (type))
    {
      type = thin_descriptor_type (type);
      if (type == NULL)
	return NULL;
      r = lookup_struct_elt_type (type, "BOUNDS", 1);
      if (r != NULL)
	return ada_check_typedef (r);
    }
  else if (type->code () == TYPE_CODE_STRUCT)
    {
      r = lookup_struct_elt_type (type, "P_BOUNDS", 1);
      if (r != NULL)
	return ada_check_typedef (ada_check_typedef (r)->target_type ());
    }
  return NULL;
}

/* If ARR is an array descriptor (fat or thin pointer), or pointer to
   one, a pointer to its bounds data.   Otherwise NULL.  */

static struct value *
desc_bounds (struct value *arr)
{
  struct type *type = ada_check_typedef (arr->type ());

  if (is_thin_pntr (type))
    {
      struct type *bounds_type =
	desc_bounds_type (thin_descriptor_type (type));
      LONGEST addr;

      if (bounds_type == NULL)
	error (_("Bad GNAT array descriptor"));

      /* NOTE: The following calculation is not really kosher, but
	 since desc_type is an XVE-encoded type (and shouldn't be),
	 the correct calculation is a real pain.  FIXME (and fix GCC).  */
      if (type->code () == TYPE_CODE_PTR)
	addr = value_as_long (arr);
      else
	addr = arr->address ();

      return
	value_from_longest (lookup_pointer_type (bounds_type),
			    addr - bounds_type->length ());
    }

  else if (is_thick_pntr (type))
    {
      struct value *p_bounds = value_struct_elt (&arr, {}, "P_BOUNDS", NULL,
					       _("Bad GNAT array descriptor"));
      struct type *p_bounds_type = p_bounds->type ();

      if (p_bounds_type
	  && p_bounds_type->code () == TYPE_CODE_PTR)
	{
	  struct type *target_type = p_bounds_type->target_type ();

	  if (target_type->is_stub ())
	    p_bounds = value_cast (lookup_pointer_type
				   (ada_check_typedef (target_type)),
				   p_bounds);
	}
      else
	error (_("Bad GNAT array descriptor"));

      return p_bounds;
    }
  else
    return NULL;
}

/* If TYPE is the type of an array-descriptor (fat pointer),  the bit
   position of the field containing the address of the bounds data.  */

static int
fat_pntr_bounds_bitpos (struct type *type)
{
  return desc_base_type (type)->field (1).loc_bitpos ();
}

/* If TYPE is the type of an array-descriptor (fat pointer), the bit
   size of the field containing the address of the bounds data.  */

static int
fat_pntr_bounds_bitsize (struct type *type)
{
  type = desc_base_type (type);

  if (type->field (1).bitsize () > 0)
    return type->field (1).bitsize ();
  else
    return 8 * ada_check_typedef (type->field (1).type ())->length ();
}

/* If TYPE is the type of an array descriptor (fat or thin pointer) or a
   pointer to one, the type of its array data (a array-with-no-bounds type);
   otherwise, NULL.  Use ada_type_of_array to get an array type with bounds
   data.  */

static struct type *
desc_data_target_type (struct type *type)
{
  type = desc_base_type (type);

  /* NOTE: The following is bogus; see comment in desc_bounds.  */
  if (is_thin_pntr (type))
    return desc_base_type (thin_descriptor_type (type)->field (1).type ());
  else if (is_thick_pntr (type))
    {
      struct type *data_type = lookup_struct_elt_type (type, "P_ARRAY", 1);

      if (data_type
	  && ada_check_typedef (data_type)->code () == TYPE_CODE_PTR)
	return ada_check_typedef (data_type->target_type ());
    }

  return NULL;
}

/* If ARR is an array descriptor (fat or thin pointer), a pointer to
   its array data.  */

static struct value *
desc_data (struct value *arr)
{
  struct type *type = arr->type ();

  if (is_thin_pntr (type))
    return thin_data_pntr (arr);
  else if (is_thick_pntr (type))
    return value_struct_elt (&arr, {}, "P_ARRAY", NULL,
			     _("Bad GNAT array descriptor"));
  else
    return NULL;
}


/* If TYPE is the type of an array-descriptor (fat pointer), the bit
   position of the field containing the address of the data.  */

static int
fat_pntr_data_bitpos (struct type *type)
{
  return desc_base_type (type)->field (0).loc_bitpos ();
}

/* If TYPE is the type of an array-descriptor (fat pointer), the bit
   size of the field containing the address of the data.  */

static int
fat_pntr_data_bitsize (struct type *type)
{
  type = desc_base_type (type);

  if (type->field (0).bitsize () > 0)
    return type->field (0).bitsize ();
  else
    return TARGET_CHAR_BIT * type->field (0).type ()->length ();
}

/* If BOUNDS is an array-bounds structure (or pointer to one), return
   the Ith lower bound stored in it, if WHICH is 0, and the Ith upper
   bound, if WHICH is 1.  The first bound is I=1.  */

static struct value *
desc_one_bound (struct value *bounds, int i, int which)
{
  char bound_name[20];
  xsnprintf (bound_name, sizeof (bound_name), "%cB%d",
	     which ? 'U' : 'L', i - 1);
  return value_struct_elt (&bounds, {}, bound_name, NULL,
			   _("Bad GNAT array descriptor bounds"));
}

/* If BOUNDS is an array-bounds structure type, return the bit position
   of the Ith lower bound stored in it, if WHICH is 0, and the Ith upper
   bound, if WHICH is 1.  The first bound is I=1.  */

static int
desc_bound_bitpos (struct type *type, int i, int which)
{
  return desc_base_type (type)->field (2 * i + which - 2).loc_bitpos ();
}

/* If BOUNDS is an array-bounds structure type, return the bit field size
   of the Ith lower bound stored in it, if WHICH is 0, and the Ith upper
   bound, if WHICH is 1.  The first bound is I=1.  */

static int
desc_bound_bitsize (struct type *type, int i, int which)
{
  type = desc_base_type (type);

  if (type->field (2 * i + which - 2).bitsize () > 0)
    return type->field (2 * i + which - 2).bitsize ();
  else
    return 8 * type->field (2 * i + which - 2).type ()->length ();
}

/* If TYPE is the type of an array-bounds structure, the type of its
   Ith bound (numbering from 1).  Otherwise, NULL.  */

static struct type *
desc_index_type (struct type *type, int i)
{
  type = desc_base_type (type);

  if (type->code () == TYPE_CODE_STRUCT)
    {
      char bound_name[20];
      xsnprintf (bound_name, sizeof (bound_name), "LB%d", i - 1);
      return lookup_struct_elt_type (type, bound_name, 1);
    }
  else
    return NULL;
}

/* The number of index positions in the array-bounds type TYPE.
   Return 0 if TYPE is NULL.  */

static int
desc_arity (struct type *type)
{
  type = desc_base_type (type);

  if (type != NULL)
    return type->num_fields () / 2;
  return 0;
}

/* Non-zero iff TYPE is a simple array type (not a pointer to one) or 
   an array descriptor type (representing an unconstrained array
   type).  */

static int
ada_is_direct_array_type (struct type *type)
{
  if (type == NULL)
    return 0;
  type = ada_check_typedef (type);
  return (type->code () == TYPE_CODE_ARRAY
	  || ada_is_array_descriptor_type (type));
}

/* Non-zero iff TYPE represents any kind of array in Ada, or a pointer
 * to one.  */

static int
ada_is_array_type (struct type *type)
{
  while (type != NULL
	 && (type->code () == TYPE_CODE_PTR
	     || type->code () == TYPE_CODE_REF))
    type = type->target_type ();
  return ada_is_direct_array_type (type);
}

/* Non-zero iff TYPE is a simple array type or pointer to one.  */

int
ada_is_simple_array_type (struct type *type)
{
  if (type == NULL)
    return 0;
  type = ada_check_typedef (type);
  return (type->code () == TYPE_CODE_ARRAY
	  || (type->code () == TYPE_CODE_PTR
	      && (ada_check_typedef (type->target_type ())->code ()
		  == TYPE_CODE_ARRAY)));
}

/* Non-zero iff TYPE belongs to a GNAT array descriptor.  */

int
ada_is_array_descriptor_type (struct type *type)
{
  struct type *data_type = desc_data_target_type (type);

  if (type == NULL)
    return 0;
  type = ada_check_typedef (type);
  return (data_type != NULL
	  && data_type->code () == TYPE_CODE_ARRAY
	  && desc_arity (desc_bounds_type (type)) > 0);
}

/* If ARR has a record type in the form of a standard GNAT array descriptor,
   (fat pointer) returns the type of the array data described---specifically,
   a pointer-to-array type.  If BOUNDS is non-zero, the bounds data are filled
   in from the descriptor; otherwise, they are left unspecified.  If
   the ARR denotes a null array descriptor and BOUNDS is non-zero,
   returns NULL.  The result is simply the type of ARR if ARR is not
   a descriptor.  */

static struct type *
ada_type_of_array (struct value *arr, int bounds)
{
  if (ada_is_constrained_packed_array_type (arr->type ()))
    return decode_constrained_packed_array_type (arr->type ());

  if (!ada_is_array_descriptor_type (arr->type ()))
    return arr->type ();

  if (!bounds)
    {
      struct type *array_type =
	ada_check_typedef (desc_data_target_type (arr->type ()));

      if (ada_is_unconstrained_packed_array_type (arr->type ()))
	array_type->field (0).set_bitsize
	  (decode_packed_array_bitsize (arr->type ()));

      return array_type;
    }
  else
    {
      struct type *elt_type;
      int arity;
      struct value *descriptor;

      elt_type = ada_array_element_type (arr->type (), -1);
      arity = ada_array_arity (arr->type ());

      if (elt_type == NULL || arity == 0)
	return ada_check_typedef (arr->type ());

      descriptor = desc_bounds (arr);
      if (value_as_long (descriptor) == 0)
	return NULL;
      while (arity > 0)
	{
	  type_allocator alloc (arr->type ());
	  struct value *low = desc_one_bound (descriptor, arity, 0);
	  struct value *high = desc_one_bound (descriptor, arity, 1);

	  arity -= 1;
	  struct type *range_type
	    = create_static_range_type (alloc, low->type (),
					longest_to_int (value_as_long (low)),
					longest_to_int (value_as_long (high)));
	  elt_type = create_array_type (alloc, elt_type, range_type);
	  INIT_GNAT_SPECIFIC (elt_type);

	  if (ada_is_unconstrained_packed_array_type (arr->type ()))
	    {
	      /* We need to store the element packed bitsize, as well as
		 recompute the array size, because it was previously
		 computed based on the unpacked element size.  */
	      LONGEST lo = value_as_long (low);
	      LONGEST hi = value_as_long (high);

	      elt_type->field (0).set_bitsize
		(decode_packed_array_bitsize (arr->type ()));

	      /* If the array has no element, then the size is already
		 zero, and does not need to be recomputed.  */
	      if (lo < hi)
		{
		  int array_bitsize =
			(hi - lo + 1) * elt_type->field (0).bitsize ();

		  elt_type->set_length ((array_bitsize + 7) / 8);
		}
	    }
	}

      return lookup_pointer_type (elt_type);
    }
}

/* If ARR does not represent an array, returns ARR unchanged.
   Otherwise, returns either a standard GDB array with bounds set
   appropriately or, if ARR is a non-null fat pointer, a pointer to a standard
   GDB array.  Returns NULL if ARR is a null fat pointer.  */

struct value *
ada_coerce_to_simple_array_ptr (struct value *arr)
{
  if (ada_is_array_descriptor_type (arr->type ()))
    {
      struct type *arrType = ada_type_of_array (arr, 1);

      if (arrType == NULL)
	return NULL;
      return value_cast (arrType, desc_data (arr)->copy ());
    }
  else if (ada_is_constrained_packed_array_type (arr->type ()))
    return decode_constrained_packed_array (arr);
  else
    return arr;
}

/* If ARR does not represent an array, returns ARR unchanged.
   Otherwise, returns a standard GDB array describing ARR (which may
   be ARR itself if it already is in the proper form).  */

struct value *
ada_coerce_to_simple_array (struct value *arr)
{
  if (ada_is_array_descriptor_type (arr->type ()))
    {
      struct value *arrVal = ada_coerce_to_simple_array_ptr (arr);

      if (arrVal == NULL)
	error (_("Bounds unavailable for null array pointer."));
      return value_ind (arrVal);
    }
  else if (ada_is_constrained_packed_array_type (arr->type ()))
    return decode_constrained_packed_array (arr);
  else
    return arr;
}

/* If TYPE represents a GNAT array type, return it translated to an
   ordinary GDB array type (possibly with BITSIZE fields indicating
   packing).  For other types, is the identity.  */

struct type *
ada_coerce_to_simple_array_type (struct type *type)
{
  if (ada_is_constrained_packed_array_type (type))
    return decode_constrained_packed_array_type (type);

  if (ada_is_array_descriptor_type (type))
    return ada_check_typedef (desc_data_target_type (type));

  return type;
}

/* Non-zero iff TYPE represents a standard GNAT packed-array type.  */

static int
ada_is_gnat_encoded_packed_array_type  (struct type *type)
{
  if (type == NULL)
    return 0;
  type = desc_base_type (type);
  type = ada_check_typedef (type);
  return
    ada_type_name (type) != NULL
    && strstr (ada_type_name (type), "___XP") != NULL;
}

/* Non-zero iff TYPE represents a standard GNAT constrained
   packed-array type.  */

int
ada_is_constrained_packed_array_type (struct type *type)
{
  return ada_is_gnat_encoded_packed_array_type (type)
    && !ada_is_array_descriptor_type (type);
}

/* Non-zero iff TYPE represents an array descriptor for a
   unconstrained packed-array type.  */

static int
ada_is_unconstrained_packed_array_type (struct type *type)
{
  if (!ada_is_array_descriptor_type (type))
    return 0;

  if (ada_is_gnat_encoded_packed_array_type (type))
    return 1;

  /* If we saw GNAT encodings, then the above code is sufficient.
     However, with minimal encodings, we will just have a thick
     pointer instead.  */
  if (is_thick_pntr (type))
    {
      type = desc_base_type (type);
      /* The structure's first field is a pointer to an array, so this
	 fetches the array type.  */
      type = type->field (0).type ()->target_type ();
      if (type->code () == TYPE_CODE_TYPEDEF)
	type = ada_typedef_target_type (type);
      /* Now we can see if the array elements are packed.  */
      return type->field (0).bitsize () > 0;
    }

  return 0;
}

/* Return true if TYPE is a (Gnat-encoded) constrained packed array
   type, or if it is an ordinary (non-Gnat-encoded) packed array.  */

static bool
ada_is_any_packed_array_type (struct type *type)
{
  return (ada_is_constrained_packed_array_type (type)
	  || (type->code () == TYPE_CODE_ARRAY
	      && type->field (0).bitsize () % 8 != 0));
}

/* Given that TYPE encodes a packed array type (constrained or unconstrained),
   return the size of its elements in bits.  */

static long
decode_packed_array_bitsize (struct type *type)
{
  const char *raw_name;
  const char *tail;
  long bits;

  /* Access to arrays implemented as fat pointers are encoded as a typedef
     of the fat pointer type.  We need the name of the fat pointer type
     to do the decoding, so strip the typedef layer.  */
  if (type->code () == TYPE_CODE_TYPEDEF)
    type = ada_typedef_target_type (type);

  raw_name = ada_type_name (ada_check_typedef (type));
  if (!raw_name)
    raw_name = ada_type_name (desc_base_type (type));

  if (!raw_name)
    return 0;

  tail = strstr (raw_name, "___XP");
  if (tail == nullptr)
    {
      gdb_assert (is_thick_pntr (type));
      /* The structure's first field is a pointer to an array, so this
	 fetches the array type.  */
      type = type->field (0).type ()->target_type ();
      /* Now we can see if the array elements are packed.  */
      return type->field (0).bitsize ();
    }

  if (sscanf (tail + sizeof ("___XP") - 1, "%ld", &bits) != 1)
    {
      lim_warning
	(_("could not understand bit size information on packed array"));
      return 0;
    }

  return bits;
}

/* Given that TYPE is a standard GDB array type with all bounds filled
   in, and that the element size of its ultimate scalar constituents
   (that is, either its elements, or, if it is an array of arrays, its
   elements' elements, etc.) is *ELT_BITS, return an identical type,
   but with the bit sizes of its elements (and those of any
   constituent arrays) recorded in the BITSIZE components of its
   TYPE_FIELD_BITSIZE values, and with *ELT_BITS set to its total size
   in bits.

   Note that, for arrays whose index type has an XA encoding where
   a bound references a record discriminant, getting that discriminant,
   and therefore the actual value of that bound, is not possible
   because none of the given parameters gives us access to the record.
   This function assumes that it is OK in the context where it is being
   used to return an array whose bounds are still dynamic and where
   the length is arbitrary.  */

static struct type *
constrained_packed_array_type (struct type *type, long *elt_bits)
{
  struct type *new_elt_type;
  struct type *new_type;
  struct type *index_type_desc;
  struct type *index_type;
  LONGEST low_bound, high_bound;

  type = ada_check_typedef (type);
  if (type->code () != TYPE_CODE_ARRAY)
    return type;

  index_type_desc = ada_find_parallel_type (type, "___XA");
  if (index_type_desc)
    index_type = to_fixed_range_type (index_type_desc->field (0).type (),
				      NULL);
  else
    index_type = type->index_type ();

  type_allocator alloc (type);
  new_elt_type =
    constrained_packed_array_type (ada_check_typedef (type->target_type ()),
				   elt_bits);
  new_type = create_array_type (alloc, new_elt_type, index_type);
  new_type->field (0).set_bitsize (*elt_bits);
  new_type->set_name (ada_type_name (type));

  if ((check_typedef (index_type)->code () == TYPE_CODE_RANGE
       && is_dynamic_type (check_typedef (index_type)))
      || !get_discrete_bounds (index_type, &low_bound, &high_bound))
    low_bound = high_bound = 0;
  if (high_bound < low_bound)
    {
      *elt_bits = 0;
      new_type->set_length (0);
    }
  else
    {
      *elt_bits *= (high_bound - low_bound + 1);
      new_type->set_length ((*elt_bits + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT);
    }

  new_type->set_is_fixed_instance (true);
  return new_type;
}

/* The array type encoded by TYPE, where
   ada_is_constrained_packed_array_type (TYPE).  */

static struct type *
decode_constrained_packed_array_type (struct type *type)
{
  const char *raw_name = ada_type_name (ada_check_typedef (type));
  char *name;
  const char *tail;
  struct type *shadow_type;
  long bits;

  if (!raw_name)
    raw_name = ada_type_name (desc_base_type (type));

  if (!raw_name)
    return NULL;

  name = (char *) alloca (strlen (raw_name) + 1);
  tail = strstr (raw_name, "___XP");
  type = desc_base_type (type);

  memcpy (name, raw_name, tail - raw_name);
  name[tail - raw_name] = '\000';

  shadow_type = ada_find_parallel_type_with_name (type, name);

  if (shadow_type == NULL)
    {
      lim_warning (_("could not find bounds information on packed array"));
      return NULL;
    }
  shadow_type = check_typedef (shadow_type);

  if (shadow_type->code () != TYPE_CODE_ARRAY)
    {
      lim_warning (_("could not understand bounds "
		     "information on packed array"));
      return NULL;
    }

  bits = decode_packed_array_bitsize (type);
  return constrained_packed_array_type (shadow_type, &bits);
}

/* Helper function for decode_constrained_packed_array.  Set the field
   bitsize on a series of packed arrays.  Returns the number of
   elements in TYPE.  */

static LONGEST
recursively_update_array_bitsize (struct type *type)
{
  gdb_assert (type->code () == TYPE_CODE_ARRAY);

  LONGEST low, high;
  if (!get_discrete_bounds (type->index_type (), &low, &high)
      || low > high)
    return 0;
  LONGEST our_len = high - low + 1;

  struct type *elt_type = type->target_type ();
  if (elt_type->code () == TYPE_CODE_ARRAY)
    {
      LONGEST elt_len = recursively_update_array_bitsize (elt_type);
      LONGEST elt_bitsize = elt_len * elt_type->field (0).bitsize ();
      type->field (0).set_bitsize (elt_bitsize);

      type->set_length (((our_len * elt_bitsize + HOST_CHAR_BIT - 1)
			 / HOST_CHAR_BIT));
    }

  return our_len;
}

/* Given that ARR is a struct value *indicating a GNAT constrained packed
   array, returns a simple array that denotes that array.  Its type is a
   standard GDB array type except that the BITSIZEs of the array
   target types are set to the number of bits in each element, and the
   type length is set appropriately.  */

static struct value *
decode_constrained_packed_array (struct value *arr)
{
  struct type *type;

  /* If our value is a pointer, then dereference it. Likewise if
     the value is a reference.  Make sure that this operation does not
     cause the target type to be fixed, as this would indirectly cause
     this array to be decoded.  The rest of the routine assumes that
     the array hasn't been decoded yet, so we use the basic "coerce_ref"
     and "value_ind" routines to perform the dereferencing, as opposed
     to using "ada_coerce_ref" or "ada_value_ind".  */
  arr = coerce_ref (arr);
  if (ada_check_typedef (arr->type ())->code () == TYPE_CODE_PTR)
    arr = value_ind (arr);

  type = decode_constrained_packed_array_type (arr->type ());
  if (type == NULL)
    {
      error (_("can't unpack array"));
      return NULL;
    }

  /* Decoding the packed array type could not correctly set the field
     bitsizes for any dimension except the innermost, because the
     bounds may be variable and were not passed to that function.  So,
     we further resolve the array bounds here and then update the
     sizes.  */
  const gdb_byte *valaddr = arr->contents_for_printing ().data ();
  CORE_ADDR address = arr->address ();
  gdb::array_view<const gdb_byte> view
    = gdb::make_array_view (valaddr, type->length ());
  type = resolve_dynamic_type (type, view, address);
  recursively_update_array_bitsize (type);

  if (type_byte_order (arr->type ()) == BFD_ENDIAN_BIG
      && ada_is_modular_type (arr->type ()))
    {
       /* This is a (right-justified) modular type representing a packed
	  array with no wrapper.  In order to interpret the value through
	  the (left-justified) packed array type we just built, we must
	  first left-justify it.  */
      int bit_size, bit_pos;
      ULONGEST mod;

      mod = ada_modulus (arr->type ()) - 1;
      bit_size = 0;
      while (mod > 0)
	{
	  bit_size += 1;
	  mod >>= 1;
	}
      bit_pos = HOST_CHAR_BIT * arr->type ()->length () - bit_size;
      arr = ada_value_primitive_packed_val (arr, NULL,
					    bit_pos / HOST_CHAR_BIT,
					    bit_pos % HOST_CHAR_BIT,
					    bit_size,
					    type);
    }

  return coerce_unspec_val_to_type (arr, type);
}


/* The value of the element of packed array ARR at the ARITY indices
   given in IND.   ARR must be a simple array.  */

static struct value *
value_subscript_packed (struct value *arr, int arity, struct value **ind)
{
  int i;
  int bits, elt_off, bit_off;
  long elt_total_bit_offset;
  struct type *elt_type;
  struct value *v;

  bits = 0;
  elt_total_bit_offset = 0;
  elt_type = ada_check_typedef (arr->type ());
  for (i = 0; i < arity; i += 1)
    {
      if (elt_type->code () != TYPE_CODE_ARRAY
	  || elt_type->field (0).bitsize () == 0)
	error
	  (_("attempt to do packed indexing of "
	     "something other than a packed array"));
      else
	{
	  struct type *range_type = elt_type->index_type ();
	  LONGEST lowerbound, upperbound;
	  LONGEST idx;

	  if (!get_discrete_bounds (range_type, &lowerbound, &upperbound))
	    {
	      lim_warning (_("don't know bounds of array"));
	      lowerbound = upperbound = 0;
	    }

	  idx = pos_atr (ind[i]);
	  if (idx < lowerbound || idx > upperbound)
	    lim_warning (_("packed array index %ld out of bounds"),
			 (long) idx);
	  bits = elt_type->field (0).bitsize ();
	  elt_total_bit_offset += (idx - lowerbound) * bits;
	  elt_type = ada_check_typedef (elt_type->target_type ());
	}
    }
  elt_off = elt_total_bit_offset / HOST_CHAR_BIT;
  bit_off = elt_total_bit_offset % HOST_CHAR_BIT;

  v = ada_value_primitive_packed_val (arr, NULL, elt_off, bit_off,
				      bits, elt_type);
  return v;
}

/* Non-zero iff TYPE includes negative integer values.  */

static int
has_negatives (struct type *type)
{
  switch (type->code ())
    {
    default:
      return 0;
    case TYPE_CODE_INT:
      return !type->is_unsigned ();
    case TYPE_CODE_RANGE:
      return type->bounds ()->low.const_val () - type->bounds ()->bias < 0;
    }
}

/* With SRC being a buffer containing BIT_SIZE bits of data at BIT_OFFSET,
   unpack that data into UNPACKED.  UNPACKED_LEN is the size in bytes of
   the unpacked buffer.

   The size of the unpacked buffer (UNPACKED_LEN) is expected to be large
   enough to contain at least BIT_OFFSET bits.  If not, an error is raised.

   IS_BIG_ENDIAN is nonzero if the data is stored in big endian mode,
   zero otherwise.

   IS_SIGNED_TYPE is nonzero if the data corresponds to a signed type.

   IS_SCALAR is nonzero if the data corresponds to a signed type.  */

static void
ada_unpack_from_contents (const gdb_byte *src, int bit_offset, int bit_size,
			  gdb_byte *unpacked, int unpacked_len,
			  int is_big_endian, int is_signed_type,
			  int is_scalar)
{
  int src_len = (bit_size + bit_offset + HOST_CHAR_BIT - 1) / 8;
  int src_idx;                  /* Index into the source area */
  int src_bytes_left;           /* Number of source bytes left to process.  */
  int srcBitsLeft;              /* Number of source bits left to move */
  int unusedLS;                 /* Number of bits in next significant
				   byte of source that are unused */

  int unpacked_idx;             /* Index into the unpacked buffer */
  int unpacked_bytes_left;      /* Number of bytes left to set in unpacked.  */

  unsigned long accum;          /* Staging area for bits being transferred */
  int accumSize;                /* Number of meaningful bits in accum */
  unsigned char sign;

  /* Transmit bytes from least to most significant; delta is the direction
     the indices move.  */
  int delta = is_big_endian ? -1 : 1;

  /* Make sure that unpacked is large enough to receive the BIT_SIZE
     bits from SRC.  .*/
  if ((bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT > unpacked_len)
    error (_("Cannot unpack %d bits into buffer of %d bytes"),
	   bit_size, unpacked_len);

  srcBitsLeft = bit_size;
  src_bytes_left = src_len;
  unpacked_bytes_left = unpacked_len;
  sign = 0;

  if (is_big_endian)
    {
      src_idx = src_len - 1;
      if (is_signed_type
	  && ((src[0] << bit_offset) & (1 << (HOST_CHAR_BIT - 1))))
	sign = ~0;

      unusedLS =
	(HOST_CHAR_BIT - (bit_size + bit_offset) % HOST_CHAR_BIT)
	% HOST_CHAR_BIT;

      if (is_scalar)
	{
	  accumSize = 0;
	  unpacked_idx = unpacked_len - 1;
	}
      else
	{
	  /* Non-scalar values must be aligned at a byte boundary...  */
	  accumSize =
	    (HOST_CHAR_BIT - bit_size % HOST_CHAR_BIT) % HOST_CHAR_BIT;
	  /* ... And are placed at the beginning (most-significant) bytes
	     of the target.  */
	  unpacked_idx = (bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT - 1;
	  unpacked_bytes_left = unpacked_idx + 1;
	}
    }
  else
    {
      int sign_bit_offset = (bit_size + bit_offset - 1) % 8;

      src_idx = unpacked_idx = 0;
      unusedLS = bit_offset;
      accumSize = 0;

      if (is_signed_type && (src[src_len - 1] & (1 << sign_bit_offset)))
	sign = ~0;
    }

  accum = 0;
  while (src_bytes_left > 0)
    {
      /* Mask for removing bits of the next source byte that are not
	 part of the value.  */
      unsigned int unusedMSMask =
	(1 << (srcBitsLeft >= HOST_CHAR_BIT ? HOST_CHAR_BIT : srcBitsLeft)) -
	1;
      /* Sign-extend bits for this byte.  */
      unsigned int signMask = sign & ~unusedMSMask;

      accum |=
	(((src[src_idx] >> unusedLS) & unusedMSMask) | signMask) << accumSize;
      accumSize += HOST_CHAR_BIT - unusedLS;
      if (accumSize >= HOST_CHAR_BIT)
	{
	  unpacked[unpacked_idx] = accum & ~(~0UL << HOST_CHAR_BIT);
	  accumSize -= HOST_CHAR_BIT;
	  accum >>= HOST_CHAR_BIT;
	  unpacked_bytes_left -= 1;
	  unpacked_idx += delta;
	}
      srcBitsLeft -= HOST_CHAR_BIT - unusedLS;
      unusedLS = 0;
      src_bytes_left -= 1;
      src_idx += delta;
    }
  while (unpacked_bytes_left > 0)
    {
      accum |= sign << accumSize;
      unpacked[unpacked_idx] = accum & ~(~0UL << HOST_CHAR_BIT);
      accumSize -= HOST_CHAR_BIT;
      if (accumSize < 0)
	accumSize = 0;
      accum >>= HOST_CHAR_BIT;
      unpacked_bytes_left -= 1;
      unpacked_idx += delta;
    }
}

/* Create a new value of type TYPE from the contents of OBJ starting
   at byte OFFSET, and bit offset BIT_OFFSET within that byte,
   proceeding for BIT_SIZE bits.  If OBJ is an lval in memory, then
   assigning through the result will set the field fetched from.
   VALADDR is ignored unless OBJ is NULL, in which case,
   VALADDR+OFFSET must address the start of storage containing the 
   packed value.  The value returned  in this case is never an lval.
   Assumes 0 <= BIT_OFFSET < HOST_CHAR_BIT.  */

struct value *
ada_value_primitive_packed_val (struct value *obj, const gdb_byte *valaddr,
				long offset, int bit_offset, int bit_size,
				struct type *type)
{
  struct value *v;
  const gdb_byte *src;                /* First byte containing data to unpack */
  gdb_byte *unpacked;
  const int is_scalar = is_scalar_type (type);
  const int is_big_endian = type_byte_order (type) == BFD_ENDIAN_BIG;
  gdb::byte_vector staging;

  type = ada_check_typedef (type);

  if (obj == NULL)
    src = valaddr + offset;
  else
    src = obj->contents ().data () + offset;

  if (is_dynamic_type (type))
    {
      /* The length of TYPE might by dynamic, so we need to resolve
	 TYPE in order to know its actual size, which we then use
	 to create the contents buffer of the value we return.
	 The difficulty is that the data containing our object is
	 packed, and therefore maybe not at a byte boundary.  So, what
	 we do, is unpack the data into a byte-aligned buffer, and then
	 use that buffer as our object's value for resolving the type.  */
      int staging_len = (bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
      staging.resize (staging_len);

      ada_unpack_from_contents (src, bit_offset, bit_size,
				staging.data (), staging.size (),
				is_big_endian, has_negatives (type),
				is_scalar);
      type = resolve_dynamic_type (type, staging, 0);
      if (type->length () < (bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT)
	{
	  /* This happens when the length of the object is dynamic,
	     and is actually smaller than the space reserved for it.
	     For instance, in an array of variant records, the bit_size
	     we're given is the array stride, which is constant and
	     normally equal to the maximum size of its element.
	     But, in reality, each element only actually spans a portion
	     of that stride.  */
	  bit_size = type->length () * HOST_CHAR_BIT;
	}
    }

  if (obj == NULL)
    {
      v = value::allocate (type);
      src = valaddr + offset;
    }
  else if (obj->lval () == lval_memory && obj->lazy ())
    {
      int src_len = (bit_size + bit_offset + HOST_CHAR_BIT - 1) / 8;
      gdb_byte *buf;

      v = value_at (type, obj->address () + offset);
      buf = (gdb_byte *) alloca (src_len);
      read_memory (v->address (), buf, src_len);
      src = buf;
    }
  else
    {
      v = value::allocate (type);
      src = obj->contents ().data () + offset;
    }

  if (obj != NULL)
    {
      long new_offset = offset;

      v->set_component_location (obj);
      v->set_bitpos (bit_offset + obj->bitpos ());
      v->set_bitsize (bit_size);
      if (v->bitpos () >= HOST_CHAR_BIT)
	{
	  ++new_offset;
	  v->set_bitpos (v->bitpos () - HOST_CHAR_BIT);
	}
      v->set_offset (new_offset);

      /* Also set the parent value.  This is needed when trying to
	 assign a new value (in inferior memory).  */
      v->set_parent (obj);
    }
  else
    v->set_bitsize (bit_size);
  unpacked = v->contents_writeable ().data ();

  if (bit_size == 0)
    {
      memset (unpacked, 0, type->length ());
      return v;
    }

  if (staging.size () == type->length ())
    {
      /* Small short-cut: If we've unpacked the data into a buffer
	 of the same size as TYPE's length, then we can reuse that,
	 instead of doing the unpacking again.  */
      memcpy (unpacked, staging.data (), staging.size ());
    }
  else
    ada_unpack_from_contents (src, bit_offset, bit_size,
			      unpacked, type->length (),
			      is_big_endian, has_negatives (type), is_scalar);

  return v;
}

/* Store the contents of FROMVAL into the location of TOVAL.
   Return a new value with the location of TOVAL and contents of
   FROMVAL.   Handles assignment into packed fields that have
   floating-point or non-scalar types.  */

static struct value *
ada_value_assign (struct value *toval, struct value *fromval)
{
  struct type *type = toval->type ();
  int bits = toval->bitsize ();

  toval = ada_coerce_ref (toval);
  fromval = ada_coerce_ref (fromval);

  if (ada_is_direct_array_type (toval->type ()))
    toval = ada_coerce_to_simple_array (toval);
  if (ada_is_direct_array_type (fromval->type ()))
    fromval = ada_coerce_to_simple_array (fromval);

  if (!toval->deprecated_modifiable ())
    error (_("Left operand of assignment is not a modifiable lvalue."));

  if (toval->lval () == lval_memory
      && bits > 0
      && (type->code () == TYPE_CODE_FLT
	  || type->code () == TYPE_CODE_STRUCT))
    {
      int len = (toval->bitpos ()
		 + bits + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
      int from_size;
      gdb_byte *buffer = (gdb_byte *) alloca (len);
      struct value *val;
      CORE_ADDR to_addr = toval->address ();

      if (type->code () == TYPE_CODE_FLT)
	fromval = value_cast (type, fromval);

      read_memory (to_addr, buffer, len);
      from_size = fromval->bitsize ();
      if (from_size == 0)
	from_size = fromval->type ()->length () * TARGET_CHAR_BIT;

      const int is_big_endian = type_byte_order (type) == BFD_ENDIAN_BIG;
      ULONGEST from_offset = 0;
      if (is_big_endian && is_scalar_type (fromval->type ()))
	from_offset = from_size - bits;
      copy_bitwise (buffer, toval->bitpos (),
		    fromval->contents ().data (), from_offset,
		    bits, is_big_endian);
      write_memory_with_notification (to_addr, buffer, len);

      val = toval->copy ();
      memcpy (val->contents_raw ().data (),
	      fromval->contents ().data (),
	      type->length ());
      val->deprecated_set_type (type);

      return val;
    }

  return value_assign (toval, fromval);
}


/* Given that COMPONENT is a memory lvalue that is part of the lvalue
   CONTAINER, assign the contents of VAL to COMPONENTS's place in
   CONTAINER.  Modifies the VALUE_CONTENTS of CONTAINER only, not
   COMPONENT, and not the inferior's memory.  The current contents
   of COMPONENT are ignored.

   Although not part of the initial design, this function also works
   when CONTAINER and COMPONENT are not_lval's: it works as if CONTAINER
   had a null address, and COMPONENT had an address which is equal to
   its offset inside CONTAINER.  */

static void
value_assign_to_component (struct value *container, struct value *component,
			   struct value *val)
{
  LONGEST offset_in_container =
    (LONGEST)  (component->address () - container->address ());
  int bit_offset_in_container =
    component->bitpos () - container->bitpos ();
  int bits;

  val = value_cast (component->type (), val);

  if (component->bitsize () == 0)
    bits = TARGET_CHAR_BIT * component->type ()->length ();
  else
    bits = component->bitsize ();

  if (type_byte_order (container->type ()) == BFD_ENDIAN_BIG)
    {
      int src_offset;

      if (is_scalar_type (check_typedef (component->type ())))
	src_offset
	  = component->type ()->length () * TARGET_CHAR_BIT - bits;
      else
	src_offset = 0;
      copy_bitwise ((container->contents_writeable ().data ()
		     + offset_in_container),
		    container->bitpos () + bit_offset_in_container,
		    val->contents ().data (), src_offset, bits, 1);
    }
  else
    copy_bitwise ((container->contents_writeable ().data ()
		   + offset_in_container),
		  container->bitpos () + bit_offset_in_container,
		  val->contents ().data (), 0, bits, 0);
}

/* Determine if TYPE is an access to an unconstrained array.  */

bool
ada_is_access_to_unconstrained_array (struct type *type)
{
  return (type->code () == TYPE_CODE_TYPEDEF
	  && is_thick_pntr (ada_typedef_target_type (type)));
}

/* The value of the element of array ARR at the ARITY indices given in IND.
   ARR may be either a simple array, GNAT array descriptor, or pointer
   thereto.  */

struct value *
ada_value_subscript (struct value *arr, int arity, struct value **ind)
{
  int k;
  struct value *elt;
  struct type *elt_type;

  elt = ada_coerce_to_simple_array (arr);

  elt_type = ada_check_typedef (elt->type ());
  if (elt_type->code () == TYPE_CODE_ARRAY
      && elt_type->field (0).bitsize () > 0)
    return value_subscript_packed (elt, arity, ind);

  for (k = 0; k < arity; k += 1)
    {
      struct type *saved_elt_type = elt_type->target_type ();

      if (elt_type->code () != TYPE_CODE_ARRAY)
	error (_("too many subscripts (%d expected)"), k);

      elt = value_subscript (elt, pos_atr (ind[k]));

      if (ada_is_access_to_unconstrained_array (saved_elt_type)
	  && elt->type ()->code () != TYPE_CODE_TYPEDEF)
	{
	  /* The element is a typedef to an unconstrained array,
	     except that the value_subscript call stripped the
	     typedef layer.  The typedef layer is GNAT's way to
	     specify that the element is, at the source level, an
	     access to the unconstrained array, rather than the
	     unconstrained array.  So, we need to restore that
	     typedef layer, which we can do by forcing the element's
	     type back to its original type. Otherwise, the returned
	     value is going to be printed as the array, rather
	     than as an access.  Another symptom of the same issue
	     would be that an expression trying to dereference the
	     element would also be improperly rejected.  */
	  elt->deprecated_set_type (saved_elt_type);
	}

      elt_type = ada_check_typedef (elt->type ());
    }

  return elt;
}

/* Assuming ARR is a pointer to a GDB array, the value of the element
   of *ARR at the ARITY indices given in IND.
   Does not read the entire array into memory.

   Note: Unlike what one would expect, this function is used instead of
   ada_value_subscript for basically all non-packed array types.  The reason
   for this is that a side effect of doing our own pointer arithmetics instead
   of relying on value_subscript is that there is no implicit typedef peeling.
   This is important for arrays of array accesses, where it allows us to
   preserve the fact that the array's element is an array access, where the
   access part os encoded in a typedef layer.  */

static struct value *
ada_value_ptr_subscript (struct value *arr, int arity, struct value **ind)
{
  int k;
  struct value *array_ind = ada_value_ind (arr);
  struct type *type
    = check_typedef (array_ind->enclosing_type ());

  if (type->code () == TYPE_CODE_ARRAY
      && type->field (0).bitsize () > 0)
    return value_subscript_packed (array_ind, arity, ind);

  for (k = 0; k < arity; k += 1)
    {
      LONGEST lwb, upb;

      if (type->code () != TYPE_CODE_ARRAY)
	error (_("too many subscripts (%d expected)"), k);
      arr = value_cast (lookup_pointer_type (type->target_type ()),
			arr->copy ());
      get_discrete_bounds (type->index_type (), &lwb, &upb);
      arr = value_ptradd (arr, pos_atr (ind[k]) - lwb);
      type = type->target_type ();
    }

  return value_ind (arr);
}

/* Given that ARRAY_PTR is a pointer or reference to an array of type TYPE (the
   actual type of ARRAY_PTR is ignored), returns the Ada slice of
   HIGH'Pos-LOW'Pos+1 elements starting at index LOW.  The lower bound of
   this array is LOW, as per Ada rules.  */
static struct value *
ada_value_slice_from_ptr (struct value *array_ptr, struct type *type,
			  int low, int high)
{
  struct type *type0 = ada_check_typedef (type);
  struct type *base_index_type = type0->index_type ()->target_type ();
  type_allocator alloc (base_index_type);
  struct type *index_type
    = create_static_range_type (alloc, base_index_type, low, high);
  struct type *slice_type = create_array_type_with_stride
			      (alloc, type0->target_type (), index_type,
			       type0->dyn_prop (DYN_PROP_BYTE_STRIDE),
			       type0->field (0).bitsize ());
  int base_low =  ada_discrete_type_low_bound (type0->index_type ());
  std::optional<LONGEST> base_low_pos, low_pos;
  CORE_ADDR base;

  low_pos = discrete_position (base_index_type, low);
  base_low_pos = discrete_position (base_index_type, base_low);

  if (!low_pos.has_value () || !base_low_pos.has_value ())
    {
      warning (_("unable to get positions in slice, use bounds instead"));
      low_pos = low;
      base_low_pos = base_low;
    }

  ULONGEST stride = slice_type->field (0).bitsize () / 8;
  if (stride == 0)
    stride = type0->target_type ()->length ();

  base = value_as_address (array_ptr) + (*low_pos - *base_low_pos) * stride;
  return value_at_lazy (slice_type, base);
}


static struct value *
ada_value_slice (struct value *array, int low, int high)
{
  struct type *type = ada_check_typedef (array->type ());
  struct type *base_index_type = type->index_type ()->target_type ();
  type_allocator alloc (type->index_type ());
  struct type *index_type
    = create_static_range_type (alloc, type->index_type (), low, high);
  struct type *slice_type = create_array_type_with_stride
			      (alloc, type->target_type (), index_type,
			       type->dyn_prop (DYN_PROP_BYTE_STRIDE),
			       type->field (0).bitsize ());
  std::optional<LONGEST> low_pos, high_pos;


  low_pos = discrete_position (base_index_type, low);
  high_pos = discrete_position (base_index_type, high);

  if (!low_pos.has_value () || !high_pos.has_value ())
    {
      warning (_("unable to get positions in slice, use bounds instead"));
      low_pos = low;
      high_pos = high;
    }

  return value_cast (slice_type,
		     value_slice (array, low, *high_pos - *low_pos + 1));
}

/* If type is a record type in the form of a standard GNAT array
   descriptor, returns the number of dimensions for type.  If arr is a
   simple array, returns the number of "array of"s that prefix its
   type designation.  Otherwise, returns 0.  */

int
ada_array_arity (struct type *type)
{
  int arity;

  if (type == NULL)
    return 0;

  type = desc_base_type (type);

  arity = 0;
  if (type->code () == TYPE_CODE_STRUCT)
    return desc_arity (desc_bounds_type (type));
  else
    while (type->code () == TYPE_CODE_ARRAY)
      {
	arity += 1;
	type = ada_check_typedef (type->target_type ());
      }

  return arity;
}

/* If TYPE is a record type in the form of a standard GNAT array
   descriptor or a simple array type, returns the element type for
   TYPE after indexing by NINDICES indices, or by all indices if
   NINDICES is -1.  Otherwise, returns NULL.  */

struct type *
ada_array_element_type (struct type *type, int nindices)
{
  type = desc_base_type (type);

  if (type->code () == TYPE_CODE_STRUCT)
    {
      int k;
      struct type *p_array_type;

      p_array_type = desc_data_target_type (type);

      k = ada_array_arity (type);
      if (k == 0)
	return NULL;

      /* Initially p_array_type = elt_type(*)[]...(k times)...[].  */
      if (nindices >= 0 && k > nindices)
	k = nindices;
      while (k > 0 && p_array_type != NULL)
	{
	  p_array_type = ada_check_typedef (p_array_type->target_type ());
	  k -= 1;
	}
      return p_array_type;
    }
  else if (type->code () == TYPE_CODE_ARRAY)
    {
      while (nindices != 0 && type->code () == TYPE_CODE_ARRAY)
	{
	  type = type->target_type ();
	  /* A multi-dimensional array is represented using a sequence
	     of array types.  If one of these types has a name, then
	     it is not another dimension of the outer array, but
	     rather the element type of the outermost array.  */
	  if (type->name () != nullptr)
	    break;
	  nindices -= 1;
	}
      return type;
    }

  return NULL;
}

/* See ada-lang.h.  */

struct type *
ada_index_type (struct type *type, int n, const char *name)
{
  struct type *result_type;

  type = desc_base_type (type);

  if (n < 0 || n > ada_array_arity (type))
    error (_("invalid dimension number to '%s"), name);

  if (ada_is_simple_array_type (type))
    {
      int i;

      for (i = 1; i < n; i += 1)
	{
	  type = ada_check_typedef (type);
	  type = type->target_type ();
	}
      result_type = ada_check_typedef (type)->index_type ()->target_type ();
      /* FIXME: The stabs type r(0,0);bound;bound in an array type
	 has a target type of TYPE_CODE_UNDEF.  We compensate here, but
	 perhaps stabsread.c would make more sense.  */
      if (result_type && result_type->code () == TYPE_CODE_UNDEF)
	result_type = NULL;
    }
  else
    {
      result_type = desc_index_type (desc_bounds_type (type), n);
      if (result_type == NULL)
	error (_("attempt to take bound of something that is not an array"));
    }

  return result_type;
}

/* Given that arr is an array type, returns the lower bound of the
   Nth index (numbering from 1) if WHICH is 0, and the upper bound if
   WHICH is 1.  This returns bounds 0 .. -1 if ARR_TYPE is an
   array-descriptor type.  It works for other arrays with bounds supplied
   by run-time quantities other than discriminants.  */

static LONGEST
ada_array_bound_from_type (struct type *arr_type, int n, int which)
{
  struct type *type, *index_type_desc, *index_type;
  int i;

  gdb_assert (which == 0 || which == 1);

  if (ada_is_constrained_packed_array_type (arr_type))
    arr_type = decode_constrained_packed_array_type (arr_type);

  if (arr_type == NULL || !ada_is_simple_array_type (arr_type))
    return - which;

  if (arr_type->code () == TYPE_CODE_PTR)
    type = arr_type->target_type ();
  else
    type = arr_type;

  if (type->is_fixed_instance ())
    {
      /* The array has already been fixed, so we do not need to
	 check the parallel ___XA type again.  That encoding has
	 already been applied, so ignore it now.  */
      index_type_desc = NULL;
    }
  else
    {
      index_type_desc = ada_find_parallel_type (type, "___XA");
      ada_fixup_array_indexes_type (index_type_desc);
    }

  if (index_type_desc != NULL)
    index_type = to_fixed_range_type (index_type_desc->field (n - 1).type (),
				      NULL);
  else
    {
      struct type *elt_type = check_typedef (type);

      for (i = 1; i < n; i++)
	elt_type = check_typedef (elt_type->target_type ());

      index_type = elt_type->index_type ();
    }

  return (which == 0
	  ? ada_discrete_type_low_bound (index_type)
	  : ada_discrete_type_high_bound (index_type));
}

/* Given that arr is an array value, returns the lower bound of the
   nth index (numbering from 1) if WHICH is 0, and the upper bound if
   WHICH is 1.  This routine will also work for arrays with bounds
   supplied by run-time quantities other than discriminants.  */

static LONGEST
ada_array_bound (struct value *arr, int n, int which)
{
  struct type *arr_type;

  if (check_typedef (arr->type ())->code () == TYPE_CODE_PTR)
    arr = value_ind (arr);
  arr_type = arr->enclosing_type ();

  if (ada_is_constrained_packed_array_type (arr_type))
    return ada_array_bound (decode_constrained_packed_array (arr), n, which);
  else if (ada_is_simple_array_type (arr_type))
    return ada_array_bound_from_type (arr_type, n, which);
  else
    return value_as_long (desc_one_bound (desc_bounds (arr), n, which));
}

/* Given that arr is an array value, returns the length of the
   nth index.  This routine will also work for arrays with bounds
   supplied by run-time quantities other than discriminants.
   Does not work for arrays indexed by enumeration types with representation
   clauses at the moment.  */

static LONGEST
ada_array_length (struct value *arr, int n)
{
  struct type *arr_type, *index_type;
  int low, high;

  if (check_typedef (arr->type ())->code () == TYPE_CODE_PTR)
    arr = value_ind (arr);
  arr_type = arr->enclosing_type ();

  if (ada_is_constrained_packed_array_type (arr_type))
    return ada_array_length (decode_constrained_packed_array (arr), n);

  if (ada_is_simple_array_type (arr_type))
    {
      low = ada_array_bound_from_type (arr_type, n, 0);
      high = ada_array_bound_from_type (arr_type, n, 1);
    }
  else
    {
      low = value_as_long (desc_one_bound (desc_bounds (arr), n, 0));
      high = value_as_long (desc_one_bound (desc_bounds (arr), n, 1));
    }

  arr_type = check_typedef (arr_type);
  index_type = ada_index_type (arr_type, n, "length");
  if (index_type != NULL)
    {
      struct type *base_type;
      if (index_type->code () == TYPE_CODE_RANGE)
	base_type = index_type->target_type ();
      else
	base_type = index_type;

      low = pos_atr (value_from_longest (base_type, low));
      high = pos_atr (value_from_longest (base_type, high));
    }
  return high - low + 1;
}

/* An array whose type is that of ARR_TYPE (an array type), with
   bounds LOW to HIGH, but whose contents are unimportant.  If HIGH is
   less than LOW, then LOW-1 is used.  */

static struct value *
empty_array (struct type *arr_type, int low, int high)
{
  struct type *arr_type0 = ada_check_typedef (arr_type);
  type_allocator alloc (arr_type0->index_type ()->target_type ());
  struct type *index_type
    = create_static_range_type
	(alloc, arr_type0->index_type ()->target_type (), low,
	 high < low ? low - 1 : high);
  struct type *elt_type = ada_array_element_type (arr_type0, 1);

  return value::allocate (create_array_type (alloc, elt_type, index_type));
}


				/* Name resolution */

/* The "decoded" name for the user-definable Ada operator corresponding
   to OP.  */

static const char *
ada_decoded_op_name (enum exp_opcode op)
{
  int i;

  for (i = 0; ada_opname_table[i].encoded != NULL; i += 1)
    {
      if (ada_opname_table[i].op == op)
	return ada_opname_table[i].decoded;
    }
  error (_("Could not find operator name for opcode"));
}

/* Returns true (non-zero) iff decoded name N0 should appear before N1
   in a listing of choices during disambiguation (see sort_choices, below).
   The idea is that overloadings of a subprogram name from the
   same package should sort in their source order.  We settle for ordering
   such symbols by their trailing number (__N  or $N).  */

static int
encoded_ordered_before (const char *N0, const char *N1)
{
  if (N1 == NULL)
    return 0;
  else if (N0 == NULL)
    return 1;
  else
    {
      int k0, k1;

      for (k0 = strlen (N0) - 1; k0 > 0 && isdigit (N0[k0]); k0 -= 1)
	;
      for (k1 = strlen (N1) - 1; k1 > 0 && isdigit (N1[k1]); k1 -= 1)
	;
      if ((N0[k0] == '_' || N0[k0] == '$') && N0[k0 + 1] != '\000'
	  && (N1[k1] == '_' || N1[k1] == '$') && N1[k1 + 1] != '\000')
	{
	  int n0, n1;

	  n0 = k0;
	  while (N0[n0] == '_' && n0 > 0 && N0[n0 - 1] == '_')
	    n0 -= 1;
	  n1 = k1;
	  while (N1[n1] == '_' && n1 > 0 && N1[n1 - 1] == '_')
	    n1 -= 1;
	  if (n0 == n1 && strncmp (N0, N1, n0) == 0)
	    return (atoi (N0 + k0 + 1) < atoi (N1 + k1 + 1));
	}
      return (strcmp (N0, N1) < 0);
    }
}

/* Sort SYMS[0..NSYMS-1] to put the choices in a canonical order by the
   encoded names.  */

static void
sort_choices (struct block_symbol syms[], int nsyms)
{
  int i;

  for (i = 1; i < nsyms; i += 1)
    {
      struct block_symbol sym = syms[i];
      int j;

      for (j = i - 1; j >= 0; j -= 1)
	{
	  if (encoded_ordered_before (syms[j].symbol->linkage_name (),
				      sym.symbol->linkage_name ()))
	    break;
	  syms[j + 1] = syms[j];
	}
      syms[j + 1] = sym;
    }
}

/* Whether GDB should display formals and return types for functions in the
   overloads selection menu.  */
static bool print_signatures = true;

/* Print the signature for SYM on STREAM according to the FLAGS options.  For
   all but functions, the signature is just the name of the symbol.  For
   functions, this is the name of the function, the list of types for formals
   and the return type (if any).  */

static void
ada_print_symbol_signature (struct ui_file *stream, struct symbol *sym,
			    const struct type_print_options *flags)
{
  struct type *type = sym->type ();

  gdb_printf (stream, "%s", sym->print_name ());
  if (!print_signatures
      || type == NULL
      || type->code () != TYPE_CODE_FUNC)
    return;

  if (type->num_fields () > 0)
    {
      int i;

      gdb_printf (stream, " (");
      for (i = 0; i < type->num_fields (); ++i)
	{
	  if (i > 0)
	    gdb_printf (stream, "; ");
	  ada_print_type (type->field (i).type (), NULL, stream, -1, 0,
			  flags);
	}
      gdb_printf (stream, ")");
    }
  if (type->target_type () != NULL
      && type->target_type ()->code () != TYPE_CODE_VOID)
    {
      gdb_printf (stream, " return ");
      ada_print_type (type->target_type (), NULL, stream, -1, 0, flags);
    }
}

/* Read and validate a set of numeric choices from the user in the
   range 0 .. N_CHOICES-1.  Place the results in increasing
   order in CHOICES[0 .. N-1], and return N.

   The user types choices as a sequence of numbers on one line
   separated by blanks, encoding them as follows:

     + A choice of 0 means to cancel the selection, throwing an error.
     + If IS_ALL_CHOICE, a choice of 1 selects the entire set 0 .. N_CHOICES-1.
     + The user chooses k by typing k+IS_ALL_CHOICE+1.

   The user is not allowed to choose more than MAX_RESULTS values.

   ANNOTATION_SUFFIX, if present, is used to annotate the input
   prompts (for use with the -f switch).  */

static int
get_selections (int *choices, int n_choices, int max_results,
		int is_all_choice, const char *annotation_suffix)
{
  const char *args;
  const char *prompt;
  int n_chosen;
  int first_choice = is_all_choice ? 2 : 1;

  prompt = getenv ("PS2");
  if (prompt == NULL)
    prompt = "> ";

  std::string buffer;
  args = command_line_input (buffer, prompt, annotation_suffix);

  if (args == NULL)
    error_no_arg (_("one or more choice numbers"));

  n_chosen = 0;

  /* Set choices[0 .. n_chosen-1] to the users' choices in ascending
     order, as given in args.  Choices are validated.  */
  while (1)
    {
      char *args2;
      int choice, j;

      args = skip_spaces (args);
      if (*args == '\0' && n_chosen == 0)
	error_no_arg (_("one or more choice numbers"));
      else if (*args == '\0')
	break;

      choice = strtol (args, &args2, 10);
      if (args == args2 || choice < 0
	  || choice > n_choices + first_choice - 1)
	error (_("Argument must be choice number"));
      args = args2;

      if (choice == 0)
	error (_("cancelled"));

      if (choice < first_choice)
	{
	  n_chosen = n_choices;
	  for (j = 0; j < n_choices; j += 1)
	    choices[j] = j;
	  break;
	}
      choice -= first_choice;

      for (j = n_chosen - 1; j >= 0 && choice < choices[j]; j -= 1)
	{
	}

      if (j < 0 || choice != choices[j])
	{
	  int k;

	  for (k = n_chosen - 1; k > j; k -= 1)
	    choices[k + 1] = choices[k];
	  choices[j + 1] = choice;
	  n_chosen += 1;
	}
    }

  if (n_chosen > max_results)
    error (_("Select no more than %d of the above"), max_results);

  return n_chosen;
}

/* Given a list of NSYMS symbols in SYMS, select up to MAX_RESULTS>0
   by asking the user (if necessary), returning the number selected,
   and setting the first elements of SYMS items.  Error if no symbols
   selected.  */

/* NOTE: Adapted from decode_line_2 in symtab.c, with which it ought
   to be re-integrated one of these days.  */

static int
user_select_syms (struct block_symbol *syms, int nsyms, int max_results)
{
  int i;
  int *chosen = XALLOCAVEC (int , nsyms);
  int n_chosen;
  int first_choice = (max_results == 1) ? 1 : 2;
  const char *select_mode = multiple_symbols_select_mode ();

  if (max_results < 1)
    error (_("Request to select 0 symbols!"));
  if (nsyms <= 1)
    return nsyms;

  if (select_mode == multiple_symbols_cancel)
    error (_("\
canceled because the command is ambiguous\n\
See set/show multiple-symbol."));

  /* If select_mode is "all", then return all possible symbols.
     Only do that if more than one symbol can be selected, of course.
     Otherwise, display the menu as usual.  */
  if (select_mode == multiple_symbols_all && max_results > 1)
    return nsyms;

  gdb_printf (_("[0] cancel\n"));
  if (max_results > 1)
    gdb_printf (_("[1] all\n"));

  sort_choices (syms, nsyms);

  for (i = 0; i < nsyms; i += 1)
    {
      if (syms[i].symbol == NULL)
	continue;

      if (syms[i].symbol->aclass () == LOC_BLOCK)
	{
	  struct symtab_and_line sal =
	    find_function_start_sal (syms[i].symbol, 1);

	  gdb_printf ("[%d] ", i + first_choice);
	  ada_print_symbol_signature (gdb_stdout, syms[i].symbol,
				      &type_print_raw_options);
	  if (sal.symtab == NULL)
	    gdb_printf (_(" at %p[<no source file available>%p]:%d\n"),
			metadata_style.style ().ptr (), nullptr, sal.line);
	  else
	    gdb_printf
	      (_(" at %ps:%d\n"),
	       styled_string (file_name_style.style (),
			      symtab_to_filename_for_display (sal.symtab)),
	       sal.line);
	  continue;
	}
      else
	{
	  int is_enumeral =
	    (syms[i].symbol->aclass () == LOC_CONST
	     && syms[i].symbol->type () != NULL
	     && syms[i].symbol->type ()->code () == TYPE_CODE_ENUM);
	  struct symtab *symtab = NULL;

	  if (syms[i].symbol->is_objfile_owned ())
	    symtab = syms[i].symbol->symtab ();

	  if (syms[i].symbol->line () != 0 && symtab != NULL)
	    {
	      gdb_printf ("[%d] ", i + first_choice);
	      ada_print_symbol_signature (gdb_stdout, syms[i].symbol,
					  &type_print_raw_options);
	      gdb_printf (_(" at %s:%d\n"),
			  symtab_to_filename_for_display (symtab),
			  syms[i].symbol->line ());
	    }
	  else if (is_enumeral
		   && syms[i].symbol->type ()->name () != NULL)
	    {
	      gdb_printf (("[%d] "), i + first_choice);
	      ada_print_type (syms[i].symbol->type (), NULL,
			      gdb_stdout, -1, 0, &type_print_raw_options);
	      gdb_printf (_("'(%s) (enumeral)\n"),
			  syms[i].symbol->print_name ());
	    }
	  else
	    {
	      gdb_printf ("[%d] ", i + first_choice);
	      ada_print_symbol_signature (gdb_stdout, syms[i].symbol,
					  &type_print_raw_options);

	      if (symtab != NULL)
		gdb_printf (is_enumeral
			    ? _(" in %s (enumeral)\n")
			    : _(" at %s:?\n"),
			    symtab_to_filename_for_display (symtab));
	      else
		gdb_printf (is_enumeral
			    ? _(" (enumeral)\n")
			    : _(" at ?\n"));
	    }
	}
    }

  n_chosen = get_selections (chosen, nsyms, max_results, max_results > 1,
			     "overload-choice");

  for (i = 0; i < n_chosen; i += 1)
    syms[i] = syms[chosen[i]];

  return n_chosen;
}

/* See ada-lang.h.  */

block_symbol
ada_find_operator_symbol (enum exp_opcode op, bool parse_completion,
			  int nargs, value *argvec[])
{
  if (possible_user_operator_p (op, argvec))
    {
      std::vector<struct block_symbol> candidates
	= ada_lookup_symbol_list (ada_decoded_op_name (op),
				  NULL, VAR_DOMAIN);

      int i = ada_resolve_function (candidates, argvec,
				    nargs, ada_decoded_op_name (op), NULL,
				    parse_completion);
      if (i >= 0)
	return candidates[i];
    }
  return {};
}

/* See ada-lang.h.  */

block_symbol
ada_resolve_funcall (struct symbol *sym, const struct block *block,
		     struct type *context_type,
		     bool parse_completion,
		     int nargs, value *argvec[],
		     innermost_block_tracker *tracker)
{
  std::vector<struct block_symbol> candidates
    = ada_lookup_symbol_list (sym->linkage_name (), block, VAR_DOMAIN);

  int i;
  if (candidates.size () == 1)
    i = 0;
  else
    {
      i = ada_resolve_function
	(candidates,
	 argvec, nargs,
	 sym->linkage_name (),
	 context_type, parse_completion);
      if (i < 0)
	error (_("Could not find a match for %s"), sym->print_name ());
    }

  tracker->update (candidates[i]);
  return candidates[i];
}

/* Resolve a mention of a name where the context type is an
   enumeration type.  */

static int
ada_resolve_enum (std::vector<struct block_symbol> &syms,
		  const char *name, struct type *context_type,
		  bool parse_completion)
{
  gdb_assert (context_type->code () == TYPE_CODE_ENUM);
  context_type = ada_check_typedef (context_type);

  /* We already know the name matches, so we're just looking for
     an element of the correct enum type.  */
  struct type *type1 = context_type;
  for (int i = 0; i < syms.size (); ++i)
    {
      struct type *type2 = ada_check_typedef (syms[i].symbol->type ());
      if (type1 == type2)
	return i;
    }

  for (int i = 0; i < syms.size (); ++i)
    {
      struct type *type2 = ada_check_typedef (syms[i].symbol->type ());
      if (type1->num_fields () != type2->num_fields ())
	continue;
      if (strcmp (type1->name (), type2->name ()) != 0)
	continue;
      if (ada_identical_enum_types_p (type1, type2))
	return i;
    }

  error (_("No name '%s' in enumeration type '%s'"), name,
	 ada_type_name (context_type));
}

/* See ada-lang.h.  */

block_symbol
ada_resolve_variable (struct symbol *sym, const struct block *block,
		      struct type *context_type,
		      bool parse_completion,
		      int deprocedure_p,
		      innermost_block_tracker *tracker)
{
  std::vector<struct block_symbol> candidates
    = ada_lookup_symbol_list (sym->linkage_name (), block, VAR_DOMAIN);

  if (std::any_of (candidates.begin (),
		   candidates.end (),
		   [] (block_symbol &bsym)
		   {
		     switch (bsym.symbol->aclass ())
		       {
		       case LOC_REGISTER:
		       case LOC_ARG:
		       case LOC_REF_ARG:
		       case LOC_REGPARM_ADDR:
		       case LOC_LOCAL:
		       case LOC_COMPUTED:
			 return true;
		       default:
			 return false;
		       }
		   }))
    {
      /* Types tend to get re-introduced locally, so if there
	 are any local symbols that are not types, first filter
	 out all types.  */
      candidates.erase
	(std::remove_if
	 (candidates.begin (),
	  candidates.end (),
	  [] (block_symbol &bsym)
	  {
	    return bsym.symbol->aclass () == LOC_TYPEDEF;
	  }),
	 candidates.end ());
    }

  /* Filter out artificial symbols.  */
  candidates.erase
    (std::remove_if
     (candidates.begin (),
      candidates.end (),
      [] (block_symbol &bsym)
      {
	return bsym.symbol->is_artificial ();
      }),
     candidates.end ());

  int i;
  if (candidates.empty ())
    error (_("No definition found for %s"), sym->print_name ());
  else if (candidates.size () == 1)
    i = 0;
  else if (context_type != nullptr
	   && context_type->code () == TYPE_CODE_ENUM)
    i = ada_resolve_enum (candidates, sym->linkage_name (), context_type,
			  parse_completion);
  else if (context_type == nullptr
	   && symbols_are_identical_enums (candidates))
    {
      /* If all the remaining symbols are identical enumerals, then
	 just keep the first one and discard the rest.

	 Unlike what we did previously, we do not discard any entry
	 unless they are ALL identical.  This is because the symbol
	 comparison is not a strict comparison, but rather a practical
	 comparison.  If all symbols are considered identical, then
	 we can just go ahead and use the first one and discard the rest.
	 But if we cannot reduce the list to a single element, we have
	 to ask the user to disambiguate anyways.  And if we have to
	 present a multiple-choice menu, it's less confusing if the list
	 isn't missing some choices that were identical and yet distinct.  */
      candidates.resize (1);
      i = 0;
    }
  else if (deprocedure_p && !is_nonfunction (candidates))
    {
      i = ada_resolve_function
	(candidates, NULL, 0,
	 sym->linkage_name (),
	 context_type, parse_completion);
      if (i < 0)
	error (_("Could not find a match for %s"), sym->print_name ());
    }
  else
    {
      gdb_printf (_("Multiple matches for %s\n"), sym->print_name ());
      user_select_syms (candidates.data (), candidates.size (), 1);
      i = 0;
    }

  tracker->update (candidates[i]);
  return candidates[i];
}

static bool ada_type_match (struct type *ftype, struct type *atype);

/* Helper for ada_type_match that checks that two array types are
   compatible.  As with that function, FTYPE is the formal type and
   ATYPE is the actual type.  */

static bool
ada_type_match_arrays (struct type *ftype, struct type *atype)
{
  if (ftype->code () != TYPE_CODE_ARRAY
      && !ada_is_array_descriptor_type (ftype))
    return false;
  if (atype->code () != TYPE_CODE_ARRAY
      && !ada_is_array_descriptor_type (atype))
    return false;

  if (ada_array_arity (ftype) != ada_array_arity (atype))
    return false;

  struct type *f_elt_type = ada_array_element_type (ftype, -1);
  struct type *a_elt_type = ada_array_element_type (atype, -1);
  return ada_type_match (f_elt_type, a_elt_type);
}

/* Return non-zero if formal type FTYPE matches actual type ATYPE.
   The term "match" here is rather loose.  The match is heuristic and
   liberal -- while it tries to reject matches that are obviously
   incorrect, it may still let through some that do not strictly
   correspond to Ada rules.  */

static bool
ada_type_match (struct type *ftype, struct type *atype)
{
  ftype = ada_check_typedef (ftype);
  atype = ada_check_typedef (atype);

  if (ftype->code () == TYPE_CODE_REF)
    ftype = ftype->target_type ();
  if (atype->code () == TYPE_CODE_REF)
    atype = atype->target_type ();

  switch (ftype->code ())
    {
    default:
      return ftype->code () == atype->code ();
    case TYPE_CODE_PTR:
      if (atype->code () != TYPE_CODE_PTR)
	return false;
      atype = atype->target_type ();
      /* This can only happen if the actual argument is 'null'.  */
      if (atype->code () == TYPE_CODE_INT && atype->length () == 0)
	return true;
      return ada_type_match (ftype->target_type (), atype);
    case TYPE_CODE_INT:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_RANGE:
      switch (atype->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_RANGE:
	  return true;
	default:
	  return false;
	}

    case TYPE_CODE_STRUCT:
      if (!ada_is_array_descriptor_type (ftype))
	return (atype->code () == TYPE_CODE_STRUCT
		&& !ada_is_array_descriptor_type (atype));

      [[fallthrough]];
    case TYPE_CODE_ARRAY:
      return ada_type_match_arrays (ftype, atype);

    case TYPE_CODE_UNION:
    case TYPE_CODE_FLT:
      return (atype->code () == ftype->code ());
    }
}

/* Return non-zero if the formals of FUNC "sufficiently match" the
   vector of actual argument types ACTUALS of size N_ACTUALS.  FUNC
   may also be an enumeral, in which case it is treated as a 0-
   argument function.  */

static int
ada_args_match (struct symbol *func, struct value **actuals, int n_actuals)
{
  int i;
  struct type *func_type = func->type ();

  if (func->aclass () == LOC_CONST
      && func_type->code () == TYPE_CODE_ENUM)
    return (n_actuals == 0);
  else if (func_type == NULL || func_type->code () != TYPE_CODE_FUNC)
    return 0;

  if (func_type->num_fields () != n_actuals)
    return 0;

  for (i = 0; i < n_actuals; i += 1)
    {
      if (actuals[i] == NULL)
	return 0;
      else
	{
	  struct type *ftype = ada_check_typedef (func_type->field (i).type ());
	  struct type *atype = ada_check_typedef (actuals[i]->type ());

	  if (!ada_type_match (ftype, atype))
	    return 0;
	}
    }
  return 1;
}

/* False iff function type FUNC_TYPE definitely does not produce a value
   compatible with type CONTEXT_TYPE.  Conservatively returns 1 if
   FUNC_TYPE is not a valid function type with a non-null return type
   or an enumerated type.  A null CONTEXT_TYPE indicates any non-void type.  */

static int
return_match (struct type *func_type, struct type *context_type)
{
  struct type *return_type;

  if (func_type == NULL)
    return 1;

  if (func_type->code () == TYPE_CODE_FUNC)
    return_type = get_base_type (func_type->target_type ());
  else
    return_type = get_base_type (func_type);
  if (return_type == NULL)
    return 1;

  context_type = get_base_type (context_type);

  if (return_type->code () == TYPE_CODE_ENUM)
    return context_type == NULL || return_type == context_type;
  else if (context_type == NULL)
    return return_type->code () != TYPE_CODE_VOID;
  else
    return return_type->code () == context_type->code ();
}


/* Returns the index in SYMS that contains the symbol for the
   function (if any) that matches the types of the NARGS arguments in
   ARGS.  If CONTEXT_TYPE is non-null and there is at least one match
   that returns that type, then eliminate matches that don't.  If
   CONTEXT_TYPE is void and there is at least one match that does not
   return void, eliminate all matches that do.

   Asks the user if there is more than one match remaining.  Returns -1
   if there is no such symbol or none is selected.  NAME is used
   solely for messages.  May re-arrange and modify SYMS in
   the process; the index returned is for the modified vector.  */

static int
ada_resolve_function (std::vector<struct block_symbol> &syms,
		      struct value **args, int nargs,
		      const char *name, struct type *context_type,
		      bool parse_completion)
{
  int fallback;
  int k;
  int m;                        /* Number of hits */

  m = 0;
  /* In the first pass of the loop, we only accept functions matching
     context_type.  If none are found, we add a second pass of the loop
     where every function is accepted.  */
  for (fallback = 0; m == 0 && fallback < 2; fallback++)
    {
      for (k = 0; k < syms.size (); k += 1)
	{
	  struct type *type = ada_check_typedef (syms[k].symbol->type ());

	  if (ada_args_match (syms[k].symbol, args, nargs)
	      && (fallback || return_match (type, context_type)))
	    {
	      syms[m] = syms[k];
	      m += 1;
	    }
	}
    }

  /* If we got multiple matches, ask the user which one to use.  Don't do this
     interactive thing during completion, though, as the purpose of the
     completion is providing a list of all possible matches.  Prompting the
     user to filter it down would be completely unexpected in this case.  */
  if (m == 0)
    return -1;
  else if (m > 1 && !parse_completion)
    {
      gdb_printf (_("Multiple matches for %s\n"), name);
      user_select_syms (syms.data (), m, 1);
      return 0;
    }
  return 0;
}

/* Type-class predicates */

/* True iff TYPE is numeric (i.e., an INT, RANGE (of numeric type),
   or FLOAT).  */

static int
numeric_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (type->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_FLT:
	case TYPE_CODE_FIXED_POINT:
	  return 1;
	case TYPE_CODE_RANGE:
	  return (type == type->target_type ()
		  || numeric_type_p (type->target_type ()));
	default:
	  return 0;
	}
    }
}

/* True iff TYPE is integral (an INT or RANGE of INTs).  */

static int
integer_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (type->code ())
	{
	case TYPE_CODE_INT:
	  return 1;
	case TYPE_CODE_RANGE:
	  return (type == type->target_type ()
		  || integer_type_p (type->target_type ()));
	default:
	  return 0;
	}
    }
}

/* True iff TYPE is scalar (INT, RANGE, FLOAT, ENUM).  */

static int
scalar_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (type->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_FLT:
	case TYPE_CODE_FIXED_POINT:
	  return 1;
	default:
	  return 0;
	}
    }
}

/* True iff TYPE is discrete, as defined in the Ada Reference Manual.
   This essentially means one of (INT, RANGE, ENUM) -- but note that
   "enum" includes character and boolean as well.  */

static int
discrete_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (type->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	  return 1;
	default:
	  return 0;
	}
    }
}

/* Returns non-zero if OP with operands in the vector ARGS could be
   a user-defined function.  Errs on the side of pre-defined operators
   (i.e., result 0).  */

static int
possible_user_operator_p (enum exp_opcode op, struct value *args[])
{
  struct type *type0 =
    (args[0] == NULL) ? NULL : ada_check_typedef (args[0]->type ());
  struct type *type1 =
    (args[1] == NULL) ? NULL : ada_check_typedef (args[1]->type ());

  if (type0 == NULL)
    return 0;

  switch (op)
    {
    default:
      return 0;

    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
      return (!(numeric_type_p (type0) && numeric_type_p (type1)));

    case BINOP_REM:
    case BINOP_MOD:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
      return (!(integer_type_p (type0) && integer_type_p (type1)));

    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_LEQ:
    case BINOP_GEQ:
      return (!(scalar_type_p (type0) && scalar_type_p (type1)));

    case BINOP_CONCAT:
      return !ada_is_array_type (type0) || !ada_is_array_type (type1);

    case BINOP_EXP:
      return (!(numeric_type_p (type0) && integer_type_p (type1)));

    case UNOP_NEG:
    case UNOP_PLUS:
    case UNOP_LOGICAL_NOT:
    case UNOP_ABS:
      return (!numeric_type_p (type0));

    }
}

				/* Renaming */

/* NOTES: 

   1. In the following, we assume that a renaming type's name may
      have an ___XD suffix.  It would be nice if this went away at some
      point.
   2. We handle both the (old) purely type-based representation of 
      renamings and the (new) variable-based encoding.  At some point,
      it is devoutly to be hoped that the former goes away 
      (FIXME: hilfinger-2007-07-09).
   3. Subprogram renamings are not implemented, although the XRS
      suffix is recognized (FIXME: hilfinger-2007-07-09).  */

/* If SYM encodes a renaming, 

       <renaming> renames <renamed entity>,

   sets *LEN to the length of the renamed entity's name,
   *RENAMED_ENTITY to that name (not null-terminated), and *RENAMING_EXPR to
   the string describing the subcomponent selected from the renamed
   entity.  Returns ADA_NOT_RENAMING if SYM does not encode a renaming
   (in which case, the values of *RENAMED_ENTITY, *LEN, and *RENAMING_EXPR
   are undefined).  Otherwise, returns a value indicating the category
   of entity renamed: an object (ADA_OBJECT_RENAMING), exception
   (ADA_EXCEPTION_RENAMING), package (ADA_PACKAGE_RENAMING), or
   subprogram (ADA_SUBPROGRAM_RENAMING).  Does no allocation; the
   strings returned in *RENAMED_ENTITY and *RENAMING_EXPR should not be
   deallocated.  The values of RENAMED_ENTITY, LEN, or RENAMING_EXPR
   may be NULL, in which case they are not assigned.

   [Currently, however, GCC does not generate subprogram renamings.]  */

enum ada_renaming_category
ada_parse_renaming (struct symbol *sym,
		    const char **renamed_entity, int *len, 
		    const char **renaming_expr)
{
  enum ada_renaming_category kind;
  const char *info;
  const char *suffix;

  if (sym == NULL)
    return ADA_NOT_RENAMING;
  switch (sym->aclass ()) 
    {
    default:
      return ADA_NOT_RENAMING;
    case LOC_LOCAL:
    case LOC_STATIC:
    case LOC_COMPUTED:
    case LOC_OPTIMIZED_OUT:
      info = strstr (sym->linkage_name (), "___XR");
      if (info == NULL)
	return ADA_NOT_RENAMING;
      switch (info[5])
	{
	case '_':
	  kind = ADA_OBJECT_RENAMING;
	  info += 6;
	  break;
	case 'E':
	  kind = ADA_EXCEPTION_RENAMING;
	  info += 7;
	  break;
	case 'P':
	  kind = ADA_PACKAGE_RENAMING;
	  info += 7;
	  break;
	case 'S':
	  kind = ADA_SUBPROGRAM_RENAMING;
	  info += 7;
	  break;
	default:
	  return ADA_NOT_RENAMING;
	}
    }

  if (renamed_entity != NULL)
    *renamed_entity = info;
  suffix = strstr (info, "___XE");
  if (suffix == NULL || suffix == info)
    return ADA_NOT_RENAMING;
  if (len != NULL)
    *len = strlen (info) - strlen (suffix);
  suffix += 5;
  if (renaming_expr != NULL)
    *renaming_expr = suffix;
  return kind;
}

/* Compute the value of the given RENAMING_SYM, which is expected to
   be a symbol encoding a renaming expression.  BLOCK is the block
   used to evaluate the renaming.  */

static struct value *
ada_read_renaming_var_value (struct symbol *renaming_sym,
			     const struct block *block)
{
  const char *sym_name;

  sym_name = renaming_sym->linkage_name ();
  expression_up expr = parse_exp_1 (&sym_name, 0, block, 0);
  return expr->evaluate ();
}


				/* Evaluation: Function Calls */

/* Return an lvalue containing the value VAL.  This is the identity on
   lvalues, and otherwise has the side-effect of allocating memory
   in the inferior where a copy of the value contents is copied.  */

static struct value *
ensure_lval (struct value *val)
{
  if (val->lval () == not_lval
      || val->lval () == lval_internalvar)
    {
      int len = ada_check_typedef (val->type ())->length ();
      const CORE_ADDR addr =
	value_as_long (value_allocate_space_in_inferior (len));

      val->set_lval (lval_memory);
      val->set_address (addr);
      write_memory (addr, val->contents ().data (), len);
    }

  return val;
}

/* Given ARG, a value of type (pointer or reference to a)*
   structure/union, extract the component named NAME from the ultimate
   target structure/union and return it as a value with its
   appropriate type.

   The routine searches for NAME among all members of the structure itself
   and (recursively) among all members of any wrapper members
   (e.g., '_parent').

   If NO_ERR, then simply return NULL in case of error, rather than
   calling error.  */

static struct value *
ada_value_struct_elt (struct value *arg, const char *name, int no_err)
{
  struct type *t, *t1;
  struct value *v;
  int check_tag;

  v = NULL;
  t1 = t = ada_check_typedef (arg->type ());
  if (t->code () == TYPE_CODE_REF)
    {
      t1 = t->target_type ();
      if (t1 == NULL)
	goto BadValue;
      t1 = ada_check_typedef (t1);
      if (t1->code () == TYPE_CODE_PTR)
	{
	  arg = coerce_ref (arg);
	  t = t1;
	}
    }

  while (t->code () == TYPE_CODE_PTR)
    {
      t1 = t->target_type ();
      if (t1 == NULL)
	goto BadValue;
      t1 = ada_check_typedef (t1);
      if (t1->code () == TYPE_CODE_PTR)
	{
	  arg = value_ind (arg);
	  t = t1;
	}
      else
	break;
    }

  if (t1->code () != TYPE_CODE_STRUCT && t1->code () != TYPE_CODE_UNION)
    goto BadValue;

  if (t1 == t)
    v = ada_search_struct_field (name, arg, 0, t);
  else
    {
      int bit_offset, bit_size, byte_offset;
      struct type *field_type;
      CORE_ADDR address;

      if (t->code () == TYPE_CODE_PTR)
	address = ada_value_ind (arg)->address ();
      else
	address = ada_coerce_ref (arg)->address ();

      /* Check to see if this is a tagged type.  We also need to handle
	 the case where the type is a reference to a tagged type, but
	 we have to be careful to exclude pointers to tagged types.
	 The latter should be shown as usual (as a pointer), whereas
	 a reference should mostly be transparent to the user.  */

      if (ada_is_tagged_type (t1, 0)
	  || (t1->code () == TYPE_CODE_REF
	      && ada_is_tagged_type (t1->target_type (), 0)))
	{
	  /* We first try to find the searched field in the current type.
	     If not found then let's look in the fixed type.  */

	  if (!find_struct_field (name, t1, 0,
				  nullptr, nullptr, nullptr,
				  nullptr, nullptr))
	    check_tag = 1;
	  else
	    check_tag = 0;
	}
      else
	check_tag = 0;

      /* Convert to fixed type in all cases, so that we have proper
	 offsets to each field in unconstrained record types.  */
      t1 = ada_to_fixed_type (ada_get_base_type (t1), NULL,
			      address, NULL, check_tag);

      /* Resolve the dynamic type as well.  */
      arg = value_from_contents_and_address (t1, nullptr, address);
      t1 = arg->type ();

      if (find_struct_field (name, t1, 0,
			     &field_type, &byte_offset, &bit_offset,
			     &bit_size, NULL))
	{
	  if (bit_size != 0)
	    {
	      if (t->code () == TYPE_CODE_REF)
		arg = ada_coerce_ref (arg);
	      else
		arg = ada_value_ind (arg);
	      v = ada_value_primitive_packed_val (arg, NULL, byte_offset,
						  bit_offset, bit_size,
						  field_type);
	    }
	  else
	    v = value_at_lazy (field_type, address + byte_offset);
	}
    }

  if (v != NULL || no_err)
    return v;
  else
    error (_("There is no member named %s."), name);

 BadValue:
  if (no_err)
    return NULL;
  else
    error (_("Attempt to extract a component of "
	     "a value that is not a record."));
}

/* Return the value ACTUAL, converted to be an appropriate value for a
   formal of type FORMAL_TYPE.  Use *SP as a stack pointer for
   allocating any necessary descriptors (fat pointers), or copies of
   values not residing in memory, updating it as needed.  */

struct value *
ada_convert_actual (struct value *actual, struct type *formal_type0)
{
  struct type *actual_type = ada_check_typedef (actual->type ());
  struct type *formal_type = ada_check_typedef (formal_type0);
  struct type *formal_target =
    formal_type->code () == TYPE_CODE_PTR
    ? ada_check_typedef (formal_type->target_type ()) : formal_type;
  struct type *actual_target =
    actual_type->code () == TYPE_CODE_PTR
    ? ada_check_typedef (actual_type->target_type ()) : actual_type;

  if (ada_is_array_descriptor_type (formal_target)
      && actual_target->code () == TYPE_CODE_ARRAY)
    return make_array_descriptor (formal_type, actual);
  else if (formal_type->code () == TYPE_CODE_PTR
	   || formal_type->code () == TYPE_CODE_REF)
    {
      struct value *result;

      if (formal_target->code () == TYPE_CODE_ARRAY
	  && ada_is_array_descriptor_type (actual_target))
	result = desc_data (actual);
      else if (formal_type->code () != TYPE_CODE_PTR)
	{
	  if (actual->lval () != lval_memory)
	    {
	      struct value *val;

	      actual_type = ada_check_typedef (actual->type ());
	      val = value::allocate (actual_type);
	      copy (actual->contents (), val->contents_raw ());
	      actual = ensure_lval (val);
	    }
	  result = value_addr (actual);
	}
      else
	return actual;
      return value_cast_pointers (formal_type, result, 0);
    }
  else if (actual_type->code () == TYPE_CODE_PTR)
    return ada_value_ind (actual);
  else if (ada_is_aligner_type (formal_type))
    {
      /* We need to turn this parameter into an aligner type
	 as well.  */
      struct value *aligner = value::allocate (formal_type);
      struct value *component = ada_value_struct_elt (aligner, "F", 0);

      value_assign_to_component (aligner, component, actual);
      return aligner;
    }

  return actual;
}

/* Convert VALUE (which must be an address) to a CORE_ADDR that is a pointer of
   type TYPE.  This is usually an inefficient no-op except on some targets
   (such as AVR) where the representation of a pointer and an address
   differs.  */

static CORE_ADDR
value_pointer (struct value *value, struct type *type)
{
  unsigned len = type->length ();
  gdb_byte *buf = (gdb_byte *) alloca (len);
  CORE_ADDR addr;

  addr = value->address ();
  gdbarch_address_to_pointer (type->arch (), type, buf, addr);
  addr = extract_unsigned_integer (buf, len, type_byte_order (type));
  return addr;
}


/* Push a descriptor of type TYPE for array value ARR on the stack at
   *SP, updating *SP to reflect the new descriptor.  Return either
   an lvalue representing the new descriptor, or (if TYPE is a pointer-
   to-descriptor type rather than a descriptor type), a struct value *
   representing a pointer to this descriptor.  */

static struct value *
make_array_descriptor (struct type *type, struct value *arr)
{
  struct type *bounds_type = desc_bounds_type (type);
  struct type *desc_type = desc_base_type (type);
  struct value *descriptor = value::allocate (desc_type);
  struct value *bounds = value::allocate (bounds_type);
  int i;

  for (i = ada_array_arity (ada_check_typedef (arr->type ()));
       i > 0; i -= 1)
    {
      modify_field (bounds->type (),
		    bounds->contents_writeable ().data (),
		    ada_array_bound (arr, i, 0),
		    desc_bound_bitpos (bounds_type, i, 0),
		    desc_bound_bitsize (bounds_type, i, 0));
      modify_field (bounds->type (),
		    bounds->contents_writeable ().data (),
		    ada_array_bound (arr, i, 1),
		    desc_bound_bitpos (bounds_type, i, 1),
		    desc_bound_bitsize (bounds_type, i, 1));
    }

  bounds = ensure_lval (bounds);

  modify_field (descriptor->type (),
		descriptor->contents_writeable ().data (),
		value_pointer (ensure_lval (arr),
			       desc_type->field (0).type ()),
		fat_pntr_data_bitpos (desc_type),
		fat_pntr_data_bitsize (desc_type));

  modify_field (descriptor->type (),
		descriptor->contents_writeable ().data (),
		value_pointer (bounds,
			       desc_type->field (1).type ()),
		fat_pntr_bounds_bitpos (desc_type),
		fat_pntr_bounds_bitsize (desc_type));

  descriptor = ensure_lval (descriptor);

  if (type->code () == TYPE_CODE_PTR)
    return value_addr (descriptor);
  else
    return descriptor;
}

				/* Symbol Cache Module */

/* Performance measurements made as of 2010-01-15 indicate that
   this cache does bring some noticeable improvements.  Depending
   on the type of entity being printed, the cache can make it as much
   as an order of magnitude faster than without it.

   The descriptive type DWARF extension has significantly reduced
   the need for this cache, at least when DWARF is being used.  However,
   even in this case, some expensive name-based symbol searches are still
   sometimes necessary - to find an XVZ variable, mostly.  */

/* Clear all entries from the symbol cache.  */

static void
ada_clear_symbol_cache (program_space *pspace)
{
  ada_pspace_data_handle.clear (pspace);
}

/* Search the symbol cache for an entry matching NAME and DOMAIN.
   Return 1 if found, 0 otherwise.

   If an entry was found and SYM is not NULL, set *SYM to the entry's
   SYM.  Same principle for BLOCK if not NULL.  */

static int
lookup_cached_symbol (const char *name, domain_enum domain,
		      struct symbol **sym, const struct block **block)
{
  htab_t tab = get_ada_pspace_data (current_program_space);
  cache_entry_search search;
  search.name = name;
  search.domain = domain;

  cache_entry *e = (cache_entry *) htab_find_with_hash (tab, &search,
							search.hash ());
  if (e == nullptr)
    return 0;
  if (sym != nullptr)
    *sym = e->sym;
  if (block != nullptr)
    *block = e->block;
  return 1;
}

/* Assuming that (SYM, BLOCK) is the result of the lookup of NAME
   in domain DOMAIN, save this result in our symbol cache.  */

static void
cache_symbol (const char *name, domain_enum domain, struct symbol *sym,
	      const struct block *block)
{
  /* Symbols for builtin types don't have a block.
     For now don't cache such symbols.  */
  if (sym != NULL && !sym->is_objfile_owned ())
    return;

  /* If the symbol is a local symbol, then do not cache it, as a search
     for that symbol depends on the context.  To determine whether
     the symbol is local or not, we check the block where we found it
     against the global and static blocks of its associated symtab.  */
  if (sym != nullptr)
    {
      const blockvector &bv = *sym->symtab ()->compunit ()->blockvector ();

      if (bv.global_block () != block && bv.static_block () != block)
	return;
    }

  htab_t tab = get_ada_pspace_data (current_program_space);
  cache_entry_search search;
  search.name = name;
  search.domain = domain;

  void **slot = htab_find_slot_with_hash (tab, &search,
					  search.hash (), INSERT);

  cache_entry *e = new cache_entry;
  e->name = name;
  e->domain = domain;
  e->sym = sym;
  e->block = block;

  *slot = e;
}

				/* Symbol Lookup */

/* Return the symbol name match type that should be used used when
   searching for all symbols matching LOOKUP_NAME.

   LOOKUP_NAME is expected to be a symbol name after transformation
   for Ada lookups.  */

static symbol_name_match_type
name_match_type_from_name (const char *lookup_name)
{
  return (strstr (lookup_name, "__") == NULL
	  ? symbol_name_match_type::WILD
	  : symbol_name_match_type::FULL);
}

/* Return the result of a standard (literal, C-like) lookup of NAME in
   given DOMAIN, visible from lexical block BLOCK.  */

static struct symbol *
standard_lookup (const char *name, const struct block *block,
		 domain_enum domain)
{
  /* Initialize it just to avoid a GCC false warning.  */
  struct block_symbol sym = {};

  if (lookup_cached_symbol (name, domain, &sym.symbol, NULL))
    return sym.symbol;
  ada_lookup_encoded_symbol (name, block, domain, &sym);
  cache_symbol (name, domain, sym.symbol, sym.block);
  return sym.symbol;
}


/* Non-zero iff there is at least one non-function/non-enumeral symbol
   in the symbol fields of SYMS.  We treat enumerals as functions, 
   since they contend in overloading in the same way.  */
static int
is_nonfunction (const std::vector<struct block_symbol> &syms)
{
  for (const block_symbol &sym : syms)
    if (sym.symbol->type ()->code () != TYPE_CODE_FUNC
	&& (sym.symbol->type ()->code () != TYPE_CODE_ENUM
	    || sym.symbol->aclass () != LOC_CONST))
      return 1;

  return 0;
}

/* If true (non-zero), then TYPE0 and TYPE1 represent equivalent
   struct types.  Otherwise, they may not.  */

static int
equiv_types (struct type *type0, struct type *type1)
{
  if (type0 == type1)
    return 1;
  if (type0 == NULL || type1 == NULL
      || type0->code () != type1->code ())
    return 0;
  if ((type0->code () == TYPE_CODE_STRUCT
       || type0->code () == TYPE_CODE_ENUM)
      && ada_type_name (type0) != NULL && ada_type_name (type1) != NULL
      && strcmp (ada_type_name (type0), ada_type_name (type1)) == 0)
    return 1;

  return 0;
}

/* True iff SYM0 represents the same entity as SYM1, or one that is
   no more defined than that of SYM1.  */

static int
lesseq_defined_than (struct symbol *sym0, struct symbol *sym1)
{
  if (sym0 == sym1)
    return 1;
  if (sym0->domain () != sym1->domain ()
      || sym0->aclass () != sym1->aclass ())
    return 0;

  switch (sym0->aclass ())
    {
    case LOC_UNDEF:
      return 1;
    case LOC_TYPEDEF:
      {
	struct type *type0 = sym0->type ();
	struct type *type1 = sym1->type ();
	const char *name0 = sym0->linkage_name ();
	const char *name1 = sym1->linkage_name ();
	int len0 = strlen (name0);

	return
	  type0->code () == type1->code ()
	  && (equiv_types (type0, type1)
	      || (len0 < strlen (name1) && strncmp (name0, name1, len0) == 0
		  && startswith (name1 + len0, "___XV")));
      }
    case LOC_CONST:
      return sym0->value_longest () == sym1->value_longest ()
	&& equiv_types (sym0->type (), sym1->type ());

    case LOC_STATIC:
      {
	const char *name0 = sym0->linkage_name ();
	const char *name1 = sym1->linkage_name ();
	return (strcmp (name0, name1) == 0
		&& sym0->value_address () == sym1->value_address ());
      }

    default:
      return 0;
    }
}

/* Append (SYM,BLOCK) to the end of the array of struct block_symbol
   records in RESULT.  Do nothing if SYM is a duplicate.  */

static void
add_defn_to_vec (std::vector<struct block_symbol> &result,
		 struct symbol *sym,
		 const struct block *block)
{
  /* Do not try to complete stub types, as the debugger is probably
     already scanning all symbols matching a certain name at the
     time when this function is called.  Trying to replace the stub
     type by its associated full type will cause us to restart a scan
     which may lead to an infinite recursion.  Instead, the client
     collecting the matching symbols will end up collecting several
     matches, with at least one of them complete.  It can then filter
     out the stub ones if needed.  */

  for (int i = result.size () - 1; i >= 0; i -= 1)
    {
      if (lesseq_defined_than (sym, result[i].symbol))
	return;
      else if (lesseq_defined_than (result[i].symbol, sym))
	{
	  result[i].symbol = sym;
	  result[i].block = block;
	  return;
	}
    }

  struct block_symbol info;
  info.symbol = sym;
  info.block = block;
  result.push_back (info);
}

/* Return a bound minimal symbol matching NAME according to Ada
   decoding rules.  Returns an invalid symbol if there is no such
   minimal symbol.  Names prefixed with "standard__" are handled
   specially: "standard__" is first stripped off, and only static and
   global symbols are searched.  */

struct bound_minimal_symbol
ada_lookup_simple_minsym (const char *name, struct objfile *objfile)
{
  struct bound_minimal_symbol result;

  symbol_name_match_type match_type = name_match_type_from_name (name);
  lookup_name_info lookup_name (name, match_type);

  symbol_name_matcher_ftype *match_name
    = ada_get_symbol_name_matcher (lookup_name);

  gdbarch_iterate_over_objfiles_in_search_order
    (objfile != NULL ? objfile->arch () : current_inferior ()->arch (),
     [&result, lookup_name, match_name] (struct objfile *obj)
       {
	 for (minimal_symbol *msymbol : obj->msymbols ())
	   {
	     if (match_name (msymbol->linkage_name (), lookup_name, nullptr)
		 && msymbol->type () != mst_solib_trampoline)
	       {
		 result.minsym = msymbol;
		 result.objfile = obj;
		 return 1;
	       }
	   }

	 return 0;
       }, objfile);

  return result;
}

/* True if TYPE is definitely an artificial type supplied to a symbol
   for which no debugging information was given in the symbol file.  */

static int
is_nondebugging_type (struct type *type)
{
  const char *name = ada_type_name (type);

  return (name != NULL && strcmp (name, "<variable, no debug info>") == 0);
}

/* Return nonzero if TYPE1 and TYPE2 are two enumeration types
   that are deemed "identical" for practical purposes.

   This function assumes that TYPE1 and TYPE2 are both TYPE_CODE_ENUM
   types and that their number of enumerals is identical (in other
   words, type1->num_fields () == type2->num_fields ()).  */

static int
ada_identical_enum_types_p (struct type *type1, struct type *type2)
{
  int i;

  /* The heuristic we use here is fairly conservative.  We consider
     that 2 enumerate types are identical if they have the same
     number of enumerals and that all enumerals have the same
     underlying value and name.  */

  /* All enums in the type should have an identical underlying value.  */
  for (i = 0; i < type1->num_fields (); i++)
    if (type1->field (i).loc_enumval () != type2->field (i).loc_enumval ())
      return 0;

  /* All enumerals should also have the same name (modulo any numerical
     suffix).  */
  for (i = 0; i < type1->num_fields (); i++)
    {
      const char *name_1 = type1->field (i).name ();
      const char *name_2 = type2->field (i).name ();
      int len_1 = strlen (name_1);
      int len_2 = strlen (name_2);

      ada_remove_trailing_digits (type1->field (i).name (), &len_1);
      ada_remove_trailing_digits (type2->field (i).name (), &len_2);
      if (len_1 != len_2
	  || strncmp (type1->field (i).name (),
		      type2->field (i).name (),
		      len_1) != 0)
	return 0;
    }

  return 1;
}

/* Return nonzero if all the symbols in SYMS are all enumeral symbols
   that are deemed "identical" for practical purposes.  Sometimes,
   enumerals are not strictly identical, but their types are so similar
   that they can be considered identical.

   For instance, consider the following code:

      type Color is (Black, Red, Green, Blue, White);
      type RGB_Color is new Color range Red .. Blue;

   Type RGB_Color is a subrange of an implicit type which is a copy
   of type Color. If we call that implicit type RGB_ColorB ("B" is
   for "Base Type"), then type RGB_ColorB is a copy of type Color.
   As a result, when an expression references any of the enumeral
   by name (Eg. "print green"), the expression is technically
   ambiguous and the user should be asked to disambiguate. But
   doing so would only hinder the user, since it wouldn't matter
   what choice he makes, the outcome would always be the same.
   So, for practical purposes, we consider them as the same.  */

static int
symbols_are_identical_enums (const std::vector<struct block_symbol> &syms)
{
  int i;

  /* Before performing a thorough comparison check of each type,
     we perform a series of inexpensive checks.  We expect that these
     checks will quickly fail in the vast majority of cases, and thus
     help prevent the unnecessary use of a more expensive comparison.
     Said comparison also expects us to make some of these checks
     (see ada_identical_enum_types_p).  */

  /* Quick check: All symbols should have an enum type.  */
  for (i = 0; i < syms.size (); i++)
    if (syms[i].symbol->type ()->code () != TYPE_CODE_ENUM)
      return 0;

  /* Quick check: They should all have the same value.  */
  for (i = 1; i < syms.size (); i++)
    if (syms[i].symbol->value_longest () != syms[0].symbol->value_longest ())
      return 0;

  /* Quick check: They should all have the same number of enumerals.  */
  for (i = 1; i < syms.size (); i++)
    if (syms[i].symbol->type ()->num_fields ()
	!= syms[0].symbol->type ()->num_fields ())
      return 0;

  /* All the sanity checks passed, so we might have a set of
     identical enumeration types.  Perform a more complete
     comparison of the type of each symbol.  */
  for (i = 1; i < syms.size (); i++)
    if (!ada_identical_enum_types_p (syms[i].symbol->type (),
				     syms[0].symbol->type ()))
      return 0;

  return 1;
}

/* Remove any non-debugging symbols in SYMS that definitely
   duplicate other symbols in the list (The only case I know of where
   this happens is when object files containing stabs-in-ecoff are
   linked with files containing ordinary ecoff debugging symbols (or no
   debugging symbols)).  Modifies SYMS to squeeze out deleted entries.  */

static void
remove_extra_symbols (std::vector<struct block_symbol> &syms)
{
  int i, j;

  /* We should never be called with less than 2 symbols, as there
     cannot be any extra symbol in that case.  But it's easy to
     handle, since we have nothing to do in that case.  */
  if (syms.size () < 2)
    return;

  i = 0;
  while (i < syms.size ())
    {
      bool remove_p = false;

      /* If two symbols have the same name and one of them is a stub type,
	 the get rid of the stub.  */

      if (syms[i].symbol->type ()->is_stub ()
	  && syms[i].symbol->linkage_name () != NULL)
	{
	  for (j = 0; !remove_p && j < syms.size (); j++)
	    {
	      if (j != i
		  && !syms[j].symbol->type ()->is_stub ()
		  && syms[j].symbol->linkage_name () != NULL
		  && strcmp (syms[i].symbol->linkage_name (),
			     syms[j].symbol->linkage_name ()) == 0)
		remove_p = true;
	    }
	}

      /* Two symbols with the same name, same class and same address
	 should be identical.  */

      else if (syms[i].symbol->linkage_name () != NULL
	  && syms[i].symbol->aclass () == LOC_STATIC
	  && is_nondebugging_type (syms[i].symbol->type ()))
	{
	  for (j = 0; !remove_p && j < syms.size (); j += 1)
	    {
	      if (i != j
		  && syms[j].symbol->linkage_name () != NULL
		  && strcmp (syms[i].symbol->linkage_name (),
			     syms[j].symbol->linkage_name ()) == 0
		  && (syms[i].symbol->aclass ()
		      == syms[j].symbol->aclass ())
		  && syms[i].symbol->value_address ()
		  == syms[j].symbol->value_address ())
		remove_p = true;
	    }
	}
      
      /* Two functions with the same block are identical.  */

      else if (syms[i].symbol->aclass () == LOC_BLOCK)
	{
	  for (j = 0; !remove_p && j < syms.size (); j += 1)
	    {
	      if (i != j
		  && syms[j].symbol->aclass () == LOC_BLOCK
		  && (syms[i].symbol->value_block ()
		      == syms[j].symbol->value_block ()))
		remove_p = true;
	    }
	}

      if (remove_p)
	syms.erase (syms.begin () + i);
      else
	i += 1;
    }
}

/* Given a type that corresponds to a renaming entity, use the type name
   to extract the scope (package name or function name, fully qualified,
   and following the GNAT encoding convention) where this renaming has been
   defined.  */

static std::string
xget_renaming_scope (struct type *renaming_type)
{
  /* The renaming types adhere to the following convention:
     <scope>__<rename>___<XR extension>.
     So, to extract the scope, we search for the "___XR" extension,
     and then backtrack until we find the first "__".  */

  const char *name = renaming_type->name ();
  const char *suffix = strstr (name, "___XR");
  const char *last;

  /* Now, backtrack a bit until we find the first "__".  Start looking
     at suffix - 3, as the <rename> part is at least one character long.  */

  for (last = suffix - 3; last > name; last--)
    if (last[0] == '_' && last[1] == '_')
      break;

  /* Make a copy of scope and return it.  */
  return std::string (name, last);
}

/* Return nonzero if NAME corresponds to a package name.  */

static int
is_package_name (const char *name)
{
  /* Here, We take advantage of the fact that no symbols are generated
     for packages, while symbols are generated for each function.
     So the condition for NAME represent a package becomes equivalent
     to NAME not existing in our list of symbols.  There is only one
     small complication with library-level functions (see below).  */

  /* If it is a function that has not been defined at library level,
     then we should be able to look it up in the symbols.  */
  if (standard_lookup (name, NULL, VAR_DOMAIN) != NULL)
    return 0;

  /* Library-level function names start with "_ada_".  See if function
     "_ada_" followed by NAME can be found.  */

  /* Do a quick check that NAME does not contain "__", since library-level
     functions names cannot contain "__" in them.  */
  if (strstr (name, "__") != NULL)
    return 0;

  std::string fun_name = string_printf ("_ada_%s", name);

  return (standard_lookup (fun_name.c_str (), NULL, VAR_DOMAIN) == NULL);
}

/* Return nonzero if SYM corresponds to a renaming entity that is
   not visible from FUNCTION_NAME.  */

static int
old_renaming_is_invisible (const struct symbol *sym, const char *function_name)
{
  if (sym->aclass () != LOC_TYPEDEF)
    return 0;

  std::string scope = xget_renaming_scope (sym->type ());

  /* If the rename has been defined in a package, then it is visible.  */
  if (is_package_name (scope.c_str ()))
    return 0;

  /* Check that the rename is in the current function scope by checking
     that its name starts with SCOPE.  */

  /* If the function name starts with "_ada_", it means that it is
     a library-level function.  Strip this prefix before doing the
     comparison, as the encoding for the renaming does not contain
     this prefix.  */
  if (startswith (function_name, "_ada_"))
    function_name += 5;

  return !startswith (function_name, scope.c_str ());
}

/* Remove entries from SYMS that corresponds to a renaming entity that
   is not visible from the function associated with CURRENT_BLOCK or
   that is superfluous due to the presence of more specific renaming
   information.  Places surviving symbols in the initial entries of
   SYMS.

   Rationale:
   First, in cases where an object renaming is implemented as a
   reference variable, GNAT may produce both the actual reference
   variable and the renaming encoding.  In this case, we discard the
   latter.

   Second, GNAT emits a type following a specified encoding for each renaming
   entity.  Unfortunately, STABS currently does not support the definition
   of types that are local to a given lexical block, so all renamings types
   are emitted at library level.  As a consequence, if an application
   contains two renaming entities using the same name, and a user tries to
   print the value of one of these entities, the result of the ada symbol
   lookup will also contain the wrong renaming type.

   This function partially covers for this limitation by attempting to
   remove from the SYMS list renaming symbols that should be visible
   from CURRENT_BLOCK.  However, there does not seem be a 100% reliable
   method with the current information available.  The implementation
   below has a couple of limitations (FIXME: brobecker-2003-05-12):  
   
      - When the user tries to print a rename in a function while there
	is another rename entity defined in a package:  Normally, the
	rename in the function has precedence over the rename in the
	package, so the latter should be removed from the list.  This is
	currently not the case.
	
      - This function will incorrectly remove valid renames if
	the CURRENT_BLOCK corresponds to a function which symbol name
	has been changed by an "Export" pragma.  As a consequence,
	the user will be unable to print such rename entities.  */

static void
remove_irrelevant_renamings (std::vector<struct block_symbol> *syms,
			     const struct block *current_block)
{
  struct symbol *current_function;
  const char *current_function_name;
  int i;
  int is_new_style_renaming;

  /* If there is both a renaming foo___XR... encoded as a variable and
     a simple variable foo in the same block, discard the latter.
     First, zero out such symbols, then compress.  */
  is_new_style_renaming = 0;
  for (i = 0; i < syms->size (); i += 1)
    {
      struct symbol *sym = (*syms)[i].symbol;
      const struct block *block = (*syms)[i].block;
      const char *name;
      const char *suffix;

      if (sym == NULL || sym->aclass () == LOC_TYPEDEF)
	continue;
      name = sym->linkage_name ();
      suffix = strstr (name, "___XR");

      if (suffix != NULL)
	{
	  int name_len = suffix - name;
	  int j;

	  is_new_style_renaming = 1;
	  for (j = 0; j < syms->size (); j += 1)
	    if (i != j && (*syms)[j].symbol != NULL
		&& strncmp (name, (*syms)[j].symbol->linkage_name (),
			    name_len) == 0
		&& block == (*syms)[j].block)
	      (*syms)[j].symbol = NULL;
	}
    }
  if (is_new_style_renaming)
    {
      int j, k;

      for (j = k = 0; j < syms->size (); j += 1)
	if ((*syms)[j].symbol != NULL)
	    {
	      (*syms)[k] = (*syms)[j];
	      k += 1;
	    }
      syms->resize (k);
      return;
    }

  /* Extract the function name associated to CURRENT_BLOCK.
     Abort if unable to do so.  */

  if (current_block == NULL)
    return;

  current_function = current_block->linkage_function ();
  if (current_function == NULL)
    return;

  current_function_name = current_function->linkage_name ();
  if (current_function_name == NULL)
    return;

  /* Check each of the symbols, and remove it from the list if it is
     a type corresponding to a renaming that is out of the scope of
     the current block.  */

  i = 0;
  while (i < syms->size ())
    {
      if (ada_parse_renaming ((*syms)[i].symbol, NULL, NULL, NULL)
	  == ADA_OBJECT_RENAMING
	  && old_renaming_is_invisible ((*syms)[i].symbol,
					current_function_name))
	syms->erase (syms->begin () + i);
      else
	i += 1;
    }
}

/* Add to RESULT all symbols from BLOCK (and its super-blocks)
   whose name and domain match LOOKUP_NAME and DOMAIN respectively.

   Note: This function assumes that RESULT is empty.  */

static void
ada_add_local_symbols (std::vector<struct block_symbol> &result,
		       const lookup_name_info &lookup_name,
		       const struct block *block, domain_enum domain)
{
  while (block != NULL)
    {
      ada_add_block_symbols (result, block, lookup_name, domain, NULL);

      /* If we found a non-function match, assume that's the one.  We
	 only check this when finding a function boundary, so that we
	 can accumulate all results from intervening blocks first.  */
      if (block->function () != nullptr && is_nonfunction (result))
	return;

      block = block->superblock ();
    }
}

/* An object of this type is used as the callback argument when
   calling the map_matching_symbols method.  */

struct match_data
{
  explicit match_data (std::vector<struct block_symbol> *rp)
    : resultp (rp)
  {
  }
  DISABLE_COPY_AND_ASSIGN (match_data);

  bool operator() (struct block_symbol *bsym);

  struct objfile *objfile = nullptr;
  std::vector<struct block_symbol> *resultp;
  struct symbol *arg_sym = nullptr;
  bool found_sym = false;
};

/* A callback for add_nonlocal_symbols that adds symbol, found in
   BSYM, to a list of symbols.  */

bool
match_data::operator() (struct block_symbol *bsym)
{
  const struct block *block = bsym->block;
  struct symbol *sym = bsym->symbol;

  if (sym == NULL)
    {
      if (!found_sym && arg_sym != NULL)
	add_defn_to_vec (*resultp, arg_sym, block);
      found_sym = false;
      arg_sym = NULL;
    }
  else 
    {
      if (sym->aclass () == LOC_UNRESOLVED)
	return true;
      else if (sym->is_argument ())
	arg_sym = sym;
      else
	{
	  found_sym = true;
	  add_defn_to_vec (*resultp, sym, block);
	}
    }
  return true;
}

/* Helper for add_nonlocal_symbols.  Find symbols in DOMAIN which are
   targeted by renamings matching LOOKUP_NAME in BLOCK.  Add these
   symbols to RESULT.  Return whether we found such symbols.  */

static int
ada_add_block_renamings (std::vector<struct block_symbol> &result,
			 const struct block *block,
			 const lookup_name_info &lookup_name,
			 domain_enum domain)
{
  struct using_direct *renaming;
  int defns_mark = result.size ();

  symbol_name_matcher_ftype *name_match
    = ada_get_symbol_name_matcher (lookup_name);

  for (renaming = block->get_using ();
       renaming != NULL;
       renaming = renaming->next)
    {
      const char *r_name;

      /* Avoid infinite recursions: skip this renaming if we are actually
	 already traversing it.

	 Currently, symbol lookup in Ada don't use the namespace machinery from
	 C++/Fortran support: skip namespace imports that use them.  */
      if (renaming->searched
	  || (renaming->import_src != NULL
	      && renaming->import_src[0] != '\0')
	  || (renaming->import_dest != NULL
	      && renaming->import_dest[0] != '\0'))
	continue;
      renaming->searched = 1;

      /* TODO: here, we perform another name-based symbol lookup, which can
	 pull its own multiple overloads.  In theory, we should be able to do
	 better in this case since, in DWARF, DW_AT_import is a DIE reference,
	 not a simple name.  But in order to do this, we would need to enhance
	 the DWARF reader to associate a symbol to this renaming, instead of a
	 name.  So, for now, we do something simpler: re-use the C++/Fortran
	 namespace machinery.  */
      r_name = (renaming->alias != NULL
		? renaming->alias
		: renaming->declaration);
      if (name_match (r_name, lookup_name, NULL))
	{
	  lookup_name_info decl_lookup_name (renaming->declaration,
					     lookup_name.match_type ());
	  ada_add_all_symbols (result, block, decl_lookup_name, domain,
			       1, NULL);
	}
      renaming->searched = 0;
    }
  return result.size () != defns_mark;
}

/* Convenience function to get at the Ada encoded lookup name for
   LOOKUP_NAME, as a C string.  */

static const char *
ada_lookup_name (const lookup_name_info &lookup_name)
{
  return lookup_name.ada ().lookup_name ().c_str ();
}

/* A helper for add_nonlocal_symbols.  Expand all necessary symtabs
   for OBJFILE, then walk the objfile's symtabs and update the
   results.  */

static void
map_matching_symbols (struct objfile *objfile,
		      const lookup_name_info &lookup_name,
		      domain_enum domain,
		      int global,
		      match_data &data)
{
  data.objfile = objfile;
  objfile->expand_symtabs_matching (nullptr, &lookup_name,
				    nullptr, nullptr,
				    global
				    ? SEARCH_GLOBAL_BLOCK
				    : SEARCH_STATIC_BLOCK,
				    domain, ALL_DOMAIN);

  const int block_kind = global ? GLOBAL_BLOCK : STATIC_BLOCK;
  for (compunit_symtab *symtab : objfile->compunits ())
    {
      const struct block *block
	= symtab->blockvector ()->block (block_kind);
      if (!iterate_over_symbols_terminated (block, lookup_name,
					    domain, data))
	break;
    }
}

/* Add to RESULT all non-local symbols whose name and domain match
   LOOKUP_NAME and DOMAIN respectively.  The search is performed on
   GLOBAL_BLOCK symbols if GLOBAL is non-zero, or on STATIC_BLOCK
   symbols otherwise.  */

static void
add_nonlocal_symbols (std::vector<struct block_symbol> &result,
		      const lookup_name_info &lookup_name,
		      domain_enum domain, int global)
{
  struct match_data data (&result);

  bool is_wild_match = lookup_name.ada ().wild_match_p ();

  for (objfile *objfile : current_program_space->objfiles ())
    {
      map_matching_symbols (objfile, lookup_name, domain, global, data);

      for (compunit_symtab *cu : objfile->compunits ())
	{
	  const struct block *global_block
	    = cu->blockvector ()->global_block ();

	  if (ada_add_block_renamings (result, global_block, lookup_name,
				       domain))
	    data.found_sym = true;
	}
    }

  if (result.empty () && global && !is_wild_match)
    {
      const char *name = ada_lookup_name (lookup_name);
      std::string bracket_name = std::string ("<_ada_") + name + '>';
      lookup_name_info name1 (bracket_name, symbol_name_match_type::FULL);

      for (objfile *objfile : current_program_space->objfiles ())
	map_matching_symbols (objfile, name1, domain, global, data);
    }
}

/* Find symbols in DOMAIN matching LOOKUP_NAME, in BLOCK and, if
   FULL_SEARCH is non-zero, enclosing scope and in global scopes,
   returning the number of matches.  Add these to RESULT.

   When FULL_SEARCH is non-zero, any non-function/non-enumeral
   symbol match within the nest of blocks whose innermost member is BLOCK,
   is the one match returned (no other matches in that or
   enclosing blocks is returned).  If there are any matches in or
   surrounding BLOCK, then these alone are returned.

   Names prefixed with "standard__" are handled specially:
   "standard__" is first stripped off (by the lookup_name
   constructor), and only static and global symbols are searched.

   If MADE_GLOBAL_LOOKUP_P is non-null, set it before return to whether we had
   to lookup global symbols.  */

static void
ada_add_all_symbols (std::vector<struct block_symbol> &result,
		     const struct block *block,
		     const lookup_name_info &lookup_name,
		     domain_enum domain,
		     int full_search,
		     int *made_global_lookup_p)
{
  struct symbol *sym;

  if (made_global_lookup_p)
    *made_global_lookup_p = 0;

  /* Special case: If the user specifies a symbol name inside package
     Standard, do a non-wild matching of the symbol name without
     the "standard__" prefix.  This was primarily introduced in order
     to allow the user to specifically access the standard exceptions
     using, for instance, Standard.Constraint_Error when Constraint_Error
     is ambiguous (due to the user defining its own Constraint_Error
     entity inside its program).  */
  if (lookup_name.ada ().standard_p ())
    block = NULL;

  /* Check the non-global symbols.  If we have ANY match, then we're done.  */

  if (block != NULL)
    {
      if (full_search)
	ada_add_local_symbols (result, lookup_name, block, domain);
      else
	{
	  /* In the !full_search case we're are being called by
	     iterate_over_symbols, and we don't want to search
	     superblocks.  */
	  ada_add_block_symbols (result, block, lookup_name, domain, NULL);
	}
      if (!result.empty () || !full_search)
	return;
    }

  /* No non-global symbols found.  Check our cache to see if we have
     already performed this search before.  If we have, then return
     the same result.  */

  if (lookup_cached_symbol (ada_lookup_name (lookup_name),
			    domain, &sym, &block))
    {
      if (sym != NULL)
	add_defn_to_vec (result, sym, block);
      return;
    }

  if (made_global_lookup_p)
    *made_global_lookup_p = 1;

  /* Search symbols from all global blocks.  */
 
  add_nonlocal_symbols (result, lookup_name, domain, 1);

  /* Now add symbols from all per-file blocks if we've gotten no hits
     (not strictly correct, but perhaps better than an error).  */

  if (result.empty ())
    add_nonlocal_symbols (result, lookup_name, domain, 0);
}

/* Find symbols in DOMAIN matching LOOKUP_NAME, in BLOCK and, if FULL_SEARCH
   is non-zero, enclosing scope and in global scopes.

   Returns (SYM,BLOCK) tuples, indicating the symbols found and the
   blocks and symbol tables (if any) in which they were found.

   When full_search is non-zero, any non-function/non-enumeral
   symbol match within the nest of blocks whose innermost member is BLOCK,
   is the one match returned (no other matches in that or
   enclosing blocks is returned).  If there are any matches in or
   surrounding BLOCK, then these alone are returned.

   Names prefixed with "standard__" are handled specially: "standard__"
   is first stripped off, and only static and global symbols are searched.  */

static std::vector<struct block_symbol>
ada_lookup_symbol_list_worker (const lookup_name_info &lookup_name,
			       const struct block *block,
			       domain_enum domain,
			       int full_search)
{
  int syms_from_global_search;
  std::vector<struct block_symbol> results;

  ada_add_all_symbols (results, block, lookup_name,
		       domain, full_search, &syms_from_global_search);

  remove_extra_symbols (results);

  if (results.empty () && full_search && syms_from_global_search)
    cache_symbol (ada_lookup_name (lookup_name), domain, NULL, NULL);

  if (results.size () == 1 && full_search && syms_from_global_search)
    cache_symbol (ada_lookup_name (lookup_name), domain,
		  results[0].symbol, results[0].block);

  remove_irrelevant_renamings (&results, block);
  return results;
}

/* Find symbols in DOMAIN matching NAME, in BLOCK and enclosing scope and
   in global scopes, returning (SYM,BLOCK) tuples.

   See ada_lookup_symbol_list_worker for further details.  */

std::vector<struct block_symbol>
ada_lookup_symbol_list (const char *name, const struct block *block,
			domain_enum domain)
{
  symbol_name_match_type name_match_type = name_match_type_from_name (name);
  lookup_name_info lookup_name (name, name_match_type);

  return ada_lookup_symbol_list_worker (lookup_name, block, domain, 1);
}

/* The result is as for ada_lookup_symbol_list with FULL_SEARCH set
   to 1, but choosing the first symbol found if there are multiple
   choices.

   The result is stored in *INFO, which must be non-NULL.
   If no match is found, INFO->SYM is set to NULL.  */

void
ada_lookup_encoded_symbol (const char *name, const struct block *block,
			   domain_enum domain,
			   struct block_symbol *info)
{
  /* Since we already have an encoded name, wrap it in '<>' to force a
     verbatim match.  Otherwise, if the name happens to not look like
     an encoded name (because it doesn't include a "__"),
     ada_lookup_name_info would re-encode/fold it again, and that
     would e.g., incorrectly lowercase object renaming names like
     "R28b" -> "r28b".  */
  std::string verbatim = add_angle_brackets (name);

  gdb_assert (info != NULL);
  *info = ada_lookup_symbol (verbatim.c_str (), block, domain);
}

/* Return a symbol in DOMAIN matching NAME, in BLOCK0 and enclosing
   scope and in global scopes, or NULL if none.  NAME is folded and
   encoded first.  Otherwise, the result is as for ada_lookup_symbol_list,
   choosing the first symbol if there are multiple choices.  */

struct block_symbol
ada_lookup_symbol (const char *name, const struct block *block0,
		   domain_enum domain)
{
  std::vector<struct block_symbol> candidates
    = ada_lookup_symbol_list (name, block0, domain);

  if (candidates.empty ())
    return {};

  return candidates[0];
}


/* True iff STR is a possible encoded suffix of a normal Ada name
   that is to be ignored for matching purposes.  Suffixes of parallel
   names (e.g., XVE) are not included here.  Currently, the possible suffixes
   are given by any of the regular expressions:

   [.$][0-9]+       [nested subprogram suffix, on platforms such as GNU/Linux]
   ___[0-9]+        [nested subprogram suffix, on platforms such as HP/UX]
   TKB              [subprogram suffix for task bodies]
   _E[0-9]+[bs]$    [protected object entry suffixes]
   (X[nb]*)?((\$|__)[0-9](_?[0-9]+)|___(JM|LJM|X([FDBUP].*|R[^T]?)))?$

   Also, any leading "__[0-9]+" sequence is skipped before the suffix
   match is performed.  This sequence is used to differentiate homonyms,
   is an optional part of a valid name suffix.  */

static int
is_name_suffix (const char *str)
{
  int k;
  const char *matching;
  const int len = strlen (str);

  /* Skip optional leading __[0-9]+.  */

  if (len > 3 && str[0] == '_' && str[1] == '_' && isdigit (str[2]))
    {
      str += 3;
      while (isdigit (str[0]))
	str += 1;
    }
  
  /* [.$][0-9]+ */

  if (str[0] == '.' || str[0] == '$')
    {
      matching = str + 1;
      while (isdigit (matching[0]))
	matching += 1;
      if (matching[0] == '\0')
	return 1;
    }

  /* ___[0-9]+ */

  if (len > 3 && str[0] == '_' && str[1] == '_' && str[2] == '_')
    {
      matching = str + 3;
      while (isdigit (matching[0]))
	matching += 1;
      if (matching[0] == '\0')
	return 1;
    }

  /* "TKB" suffixes are used for subprograms implementing task bodies.  */

  if (strcmp (str, "TKB") == 0)
    return 1;

#if 0
  /* FIXME: brobecker/2005-09-23: Protected Object subprograms end
     with a N at the end.  Unfortunately, the compiler uses the same
     convention for other internal types it creates.  So treating
     all entity names that end with an "N" as a name suffix causes
     some regressions.  For instance, consider the case of an enumerated
     type.  To support the 'Image attribute, it creates an array whose
     name ends with N.
     Having a single character like this as a suffix carrying some
     information is a bit risky.  Perhaps we should change the encoding
     to be something like "_N" instead.  In the meantime, do not do
     the following check.  */
  /* Protected Object Subprograms */
  if (len == 1 && str [0] == 'N')
    return 1;
#endif

  /* _E[0-9]+[bs]$ */
  if (len > 3 && str[0] == '_' && str [1] == 'E' && isdigit (str[2]))
    {
      matching = str + 3;
      while (isdigit (matching[0]))
	matching += 1;
      if ((matching[0] == 'b' || matching[0] == 's')
	  && matching [1] == '\0')
	return 1;
    }

  /* ??? We should not modify STR directly, as we are doing below.  This
     is fine in this case, but may become problematic later if we find
     that this alternative did not work, and want to try matching
     another one from the begining of STR.  Since we modified it, we
     won't be able to find the begining of the string anymore!  */
  if (str[0] == 'X')
    {
      str += 1;
      while (str[0] != '_' && str[0] != '\0')
	{
	  if (str[0] != 'n' && str[0] != 'b')
	    return 0;
	  str += 1;
	}
    }

  if (str[0] == '\000')
    return 1;

  if (str[0] == '_')
    {
      if (str[1] != '_' || str[2] == '\000')
	return 0;
      if (str[2] == '_')
	{
	  if (strcmp (str + 3, "JM") == 0)
	    return 1;
	  /* FIXME: brobecker/2004-09-30: GNAT will soon stop using
	     the LJM suffix in favor of the JM one.  But we will
	     still accept LJM as a valid suffix for a reasonable
	     amount of time, just to allow ourselves to debug programs
	     compiled using an older version of GNAT.  */
	  if (strcmp (str + 3, "LJM") == 0)
	    return 1;
	  if (str[3] != 'X')
	    return 0;
	  if (str[4] == 'F' || str[4] == 'D' || str[4] == 'B'
	      || str[4] == 'U' || str[4] == 'P')
	    return 1;
	  if (str[4] == 'R' && str[5] != 'T')
	    return 1;
	  return 0;
	}
      if (!isdigit (str[2]))
	return 0;
      for (k = 3; str[k] != '\0'; k += 1)
	if (!isdigit (str[k]) && str[k] != '_')
	  return 0;
      return 1;
    }
  if (str[0] == '$' && isdigit (str[1]))
    {
      for (k = 2; str[k] != '\0'; k += 1)
	if (!isdigit (str[k]) && str[k] != '_')
	  return 0;
      return 1;
    }
  return 0;
}

/* Return non-zero if the string starting at NAME and ending before
   NAME_END contains no capital letters.  */

static int
is_valid_name_for_wild_match (const char *name0)
{
  std::string decoded_name = ada_decode (name0);
  int i;

  /* If the decoded name starts with an angle bracket, it means that
     NAME0 does not follow the GNAT encoding format.  It should then
     not be allowed as a possible wild match.  */
  if (decoded_name[0] == '<')
    return 0;

  for (i=0; decoded_name[i] != '\0'; i++)
    if (isalpha (decoded_name[i]) && !islower (decoded_name[i]))
      return 0;

  return 1;
}

/* Advance *NAMEP to next occurrence in the string NAME0 of the TARGET0
   character which could start a simple name.  Assumes that *NAMEP points
   somewhere inside the string beginning at NAME0.  */

static int
advance_wild_match (const char **namep, const char *name0, char target0)
{
  const char *name = *namep;

  while (1)
    {
      char t0, t1;

      t0 = *name;
      if (t0 == '_')
	{
	  t1 = name[1];
	  if ((t1 >= 'a' && t1 <= 'z') || (t1 >= '0' && t1 <= '9'))
	    {
	      name += 1;
	      if (name == name0 + 5 && startswith (name0, "_ada"))
		break;
	      else
		name += 1;
	    }
	  else if (t1 == '_' && ((name[2] >= 'a' && name[2] <= 'z')
				 || name[2] == target0))
	    {
	      name += 2;
	      break;
	    }
	  else if (t1 == '_' && name[2] == 'B' && name[3] == '_')
	    {
	      /* Names like "pkg__B_N__name", where N is a number, are
		 block-local.  We can handle these by simply skipping
		 the "B_" here.  */
	      name += 4;
	    }
	  else
	    return 0;
	}
      else if ((t0 >= 'a' && t0 <= 'z') || (t0 >= '0' && t0 <= '9'))
	name += 1;
      else
	return 0;
    }

  *namep = name;
  return 1;
}

/* Return true iff NAME encodes a name of the form prefix.PATN.
   Ignores any informational suffixes of NAME (i.e., for which
   is_name_suffix is true).  Assumes that PATN is a lower-cased Ada
   simple name.  */

static bool
wild_match (const char *name, const char *patn)
{
  const char *p;
  const char *name0 = name;

  if (startswith (name, "___ghost_"))
    name += 9;

  while (1)
    {
      const char *match = name;

      if (*name == *patn)
	{
	  for (name += 1, p = patn + 1; *p != '\0'; name += 1, p += 1)
	    if (*p != *name)
	      break;
	  if (*p == '\0' && is_name_suffix (name))
	    return match == name0 || is_valid_name_for_wild_match (name0);

	  if (name[-1] == '_')
	    name -= 1;
	}
      if (!advance_wild_match (&name, name0, *patn))
	return false;
    }
}

/* Add symbols from BLOCK matching LOOKUP_NAME in DOMAIN to RESULT (if
   necessary).  OBJFILE is the section containing BLOCK.  */

static void
ada_add_block_symbols (std::vector<struct block_symbol> &result,
		       const struct block *block,
		       const lookup_name_info &lookup_name,
		       domain_enum domain, struct objfile *objfile)
{
  /* A matching argument symbol, if any.  */
  struct symbol *arg_sym;
  /* Set true when we find a matching non-argument symbol.  */
  bool found_sym;

  arg_sym = NULL;
  found_sym = false;
  for (struct symbol *sym : block_iterator_range (block, &lookup_name))
    {
      if (sym->matches (domain))
	{
	  if (sym->aclass () != LOC_UNRESOLVED)
	    {
	      if (sym->is_argument ())
		arg_sym = sym;
	      else
		{
		  found_sym = true;
		  add_defn_to_vec (result, sym, block);
		}
	    }
	}
    }

  /* Handle renamings.  */

  if (ada_add_block_renamings (result, block, lookup_name, domain))
    found_sym = true;

  if (!found_sym && arg_sym != NULL)
    {
      add_defn_to_vec (result, arg_sym, block);
    }

  if (!lookup_name.ada ().wild_match_p ())
    {
      arg_sym = NULL;
      found_sym = false;
      const std::string &ada_lookup_name = lookup_name.ada ().lookup_name ();
      const char *name = ada_lookup_name.c_str ();
      size_t name_len = ada_lookup_name.size ();

      for (struct symbol *sym : block_iterator_range (block))
      {
	if (sym->matches (domain))
	  {
	    int cmp;

	    cmp = (int) '_' - (int) sym->linkage_name ()[0];
	    if (cmp == 0)
	      {
		cmp = !startswith (sym->linkage_name (), "_ada_");
		if (cmp == 0)
		  cmp = strncmp (name, sym->linkage_name () + 5,
				 name_len);
	      }

	    if (cmp == 0
		&& is_name_suffix (sym->linkage_name () + name_len + 5))
	      {
		if (sym->aclass () != LOC_UNRESOLVED)
		  {
		    if (sym->is_argument ())
		      arg_sym = sym;
		    else
		      {
			found_sym = true;
			add_defn_to_vec (result, sym, block);
		      }
		  }
	      }
	  }
      }

      /* NOTE: This really shouldn't be needed for _ada_ symbols.
	 They aren't parameters, right?  */
      if (!found_sym && arg_sym != NULL)
	{
	  add_defn_to_vec (result, arg_sym, block);
	}
    }
}


				/* Symbol Completion */

/* See symtab.h.  */

bool
ada_lookup_name_info::matches
  (const char *sym_name,
   symbol_name_match_type match_type,
   completion_match_result *comp_match_res) const
{
  bool match = false;
  const char *text = m_encoded_name.c_str ();
  size_t text_len = m_encoded_name.size ();

  /* First, test against the fully qualified name of the symbol.  */

  if (strncmp (sym_name, text, text_len) == 0)
    match = true;

  std::string decoded_name = ada_decode (sym_name);
  if (match && !m_encoded_p)
    {
      /* One needed check before declaring a positive match is to verify
	 that iff we are doing a verbatim match, the decoded version
	 of the symbol name starts with '<'.  Otherwise, this symbol name
	 is not a suitable completion.  */

      bool has_angle_bracket = (decoded_name[0] == '<');
      match = (has_angle_bracket == m_verbatim_p);
    }

  if (match && !m_verbatim_p)
    {
      /* When doing non-verbatim match, another check that needs to
	 be done is to verify that the potentially matching symbol name
	 does not include capital letters, because the ada-mode would
	 not be able to understand these symbol names without the
	 angle bracket notation.  */
      const char *tmp;

      for (tmp = sym_name; *tmp != '\0' && !isupper (*tmp); tmp++);
      if (*tmp != '\0')
	match = false;
    }

  /* Second: Try wild matching...  */

  if (!match && m_wild_match_p)
    {
      /* Since we are doing wild matching, this means that TEXT
	 may represent an unqualified symbol name.  We therefore must
	 also compare TEXT against the unqualified name of the symbol.  */
      sym_name = ada_unqualified_name (decoded_name.c_str ());

      if (strncmp (sym_name, text, text_len) == 0)
	match = true;
    }

  /* Finally: If we found a match, prepare the result to return.  */

  if (!match)
    return false;

  if (comp_match_res != NULL)
    {
      std::string &match_str = comp_match_res->match.storage ();

      if (!m_encoded_p)
	match_str = ada_decode (sym_name);
      else
	{
	  if (m_verbatim_p)
	    match_str = add_angle_brackets (sym_name);
	  else
	    match_str = sym_name;

	}

      comp_match_res->set_match (match_str.c_str ());
    }

  return true;
}

				/* Field Access */

/* Return non-zero if TYPE is a pointer to the GNAT dispatch table used
   for tagged types.  */

static int
ada_is_dispatch_table_ptr_type (struct type *type)
{
  const char *name;

  if (type->code () != TYPE_CODE_PTR)
    return 0;

  name = type->target_type ()->name ();
  if (name == NULL)
    return 0;

  return (strcmp (name, "ada__tags__dispatch_table") == 0);
}

/* Return non-zero if TYPE is an interface tag.  */

static int
ada_is_interface_tag (struct type *type)
{
  const char *name = type->name ();

  if (name == NULL)
    return 0;

  return (strcmp (name, "ada__tags__interface_tag") == 0);
}

/* True if field number FIELD_NUM in struct or union type TYPE is supposed
   to be invisible to users.  */

int
ada_is_ignored_field (struct type *type, int field_num)
{
  if (field_num < 0 || field_num > type->num_fields ())
    return 1;

  /* Check the name of that field.  */
  {
    const char *name = type->field (field_num).name ();

    /* Anonymous field names should not be printed.
       brobecker/2007-02-20: I don't think this can actually happen
       but we don't want to print the value of anonymous fields anyway.  */
    if (name == NULL)
      return 1;

    /* Normally, fields whose name start with an underscore ("_")
       are fields that have been internally generated by the compiler,
       and thus should not be printed.  The "_parent" field is special,
       however: This is a field internally generated by the compiler
       for tagged types, and it contains the components inherited from
       the parent type.  This field should not be printed as is, but
       should not be ignored either.  */
    if (name[0] == '_' && !startswith (name, "_parent"))
      return 1;

    /* The compiler doesn't document this, but sometimes it emits
       a field whose name starts with a capital letter, like 'V148s'.
       These aren't marked as artificial in any way, but we know they
       should be ignored.  However, wrapper fields should not be
       ignored.  */
    if (name[0] == 'S' || name[0] == 'R' || name[0] == 'O')
      {
	/* Wrapper field.  */
      }
    else if (isupper (name[0]))
      return 1;
  }

  /* If this is the dispatch table of a tagged type or an interface tag,
     then ignore.  */
  if (ada_is_tagged_type (type, 1)
      && (ada_is_dispatch_table_ptr_type (type->field (field_num).type ())
	  || ada_is_interface_tag (type->field (field_num).type ())))
    return 1;

  /* Not a special field, so it should not be ignored.  */
  return 0;
}

/* True iff TYPE has a tag field.  If REFOK, then TYPE may also be a
   pointer or reference type whose ultimate target has a tag field.  */

int
ada_is_tagged_type (struct type *type, int refok)
{
  return (ada_lookup_struct_elt_type (type, "_tag", refok, 1) != NULL);
}

/* True iff TYPE represents the type of X'Tag */

int
ada_is_tag_type (struct type *type)
{
  type = ada_check_typedef (type);

  if (type == NULL || type->code () != TYPE_CODE_PTR)
    return 0;
  else
    {
      const char *name = ada_type_name (type->target_type ());

      return (name != NULL
	      && strcmp (name, "ada__tags__dispatch_table") == 0);
    }
}

/* The type of the tag on VAL.  */

static struct type *
ada_tag_type (struct value *val)
{
  return ada_lookup_struct_elt_type (val->type (), "_tag", 1, 0);
}

/* Return 1 if TAG follows the old scheme for Ada tags (used for Ada 95,
   retired at Ada 05).  */

static int
is_ada95_tag (struct value *tag)
{
  return ada_value_struct_elt (tag, "tsd", 1) != NULL;
}

/* The value of the tag on VAL.  */

static struct value *
ada_value_tag (struct value *val)
{
  return ada_value_struct_elt (val, "_tag", 0);
}

/* The value of the tag on the object of type TYPE whose contents are
   saved at VALADDR, if it is non-null, or is at memory address
   ADDRESS.  */

static struct value *
value_tag_from_contents_and_address (struct type *type,
				     const gdb_byte *valaddr,
				     CORE_ADDR address)
{
  int tag_byte_offset;
  struct type *tag_type;

  gdb::array_view<const gdb_byte> contents;
  if (valaddr != nullptr)
    contents = gdb::make_array_view (valaddr, type->length ());
  struct type *resolved_type = resolve_dynamic_type (type, contents, address);
  if (find_struct_field ("_tag", resolved_type, 0, &tag_type, &tag_byte_offset,
			 NULL, NULL, NULL))
    {
      const gdb_byte *valaddr1 = ((valaddr == NULL)
				  ? NULL
				  : valaddr + tag_byte_offset);
      CORE_ADDR address1 = (address == 0) ? 0 : address + tag_byte_offset;

      return value_from_contents_and_address (tag_type, valaddr1, address1);
    }
  return NULL;
}

static struct type *
type_from_tag (struct value *tag)
{
  gdb::unique_xmalloc_ptr<char> type_name = ada_tag_name (tag);

  if (type_name != NULL)
    return ada_find_any_type (ada_encode (type_name.get ()).c_str ());
  return NULL;
}

/* Given a value OBJ of a tagged type, return a value of this
   type at the base address of the object.  The base address, as
   defined in Ada.Tags, it is the address of the primary tag of
   the object, and therefore where the field values of its full
   view can be fetched.  */

struct value *
ada_tag_value_at_base_address (struct value *obj)
{
  struct value *val;
  LONGEST offset_to_top = 0;
  struct type *ptr_type, *obj_type;
  struct value *tag;
  CORE_ADDR base_address;

  obj_type = obj->type ();

  /* It is the responsibility of the caller to deref pointers.  */

  if (obj_type->code () == TYPE_CODE_PTR || obj_type->code () == TYPE_CODE_REF)
    return obj;

  tag = ada_value_tag (obj);
  if (!tag)
    return obj;

  /* Base addresses only appeared with Ada 05 and multiple inheritance.  */

  if (is_ada95_tag (tag))
    return obj;

  struct type *offset_type
    = language_lookup_primitive_type (language_def (language_ada),
				      current_inferior ()->arch (),
				      "storage_offset");
  ptr_type = lookup_pointer_type (offset_type);
  val = value_cast (ptr_type, tag);
  if (!val)
    return obj;

  /* It is perfectly possible that an exception be raised while
     trying to determine the base address, just like for the tag;
     see ada_tag_name for more details.  We do not print the error
     message for the same reason.  */

  try
    {
      offset_to_top = value_as_long (value_ind (value_ptradd (val, -2)));
    }

  catch (const gdb_exception_error &e)
    {
      return obj;
    }

  /* If offset is null, nothing to do.  */

  if (offset_to_top == 0)
    return obj;

  /* -1 is a special case in Ada.Tags; however, what should be done
     is not quite clear from the documentation.  So do nothing for
     now.  */

  if (offset_to_top == -1)
    return obj;

  /* Storage_Offset'Last is used to indicate that a dynamic offset to
     top is used.  In this situation the offset is stored just after
     the tag, in the object itself.  */
  ULONGEST last = (((ULONGEST) 1) << (8 * offset_type->length () - 1)) - 1;
  if (offset_to_top == last)
    {
      struct value *tem = value_addr (tag);
      tem = value_ptradd (tem, 1);
      tem = value_cast (ptr_type, tem);
      offset_to_top = value_as_long (value_ind (tem));
    }

  if (offset_to_top > 0)
    {
      /* OFFSET_TO_TOP used to be a positive value to be subtracted
	 from the base address.  This was however incompatible with
	 C++ dispatch table: C++ uses a *negative* value to *add*
	 to the base address.  Ada's convention has therefore been
	 changed in GNAT 19.0w 20171023: since then, C++ and Ada
	 use the same convention.  Here, we support both cases by
	 checking the sign of OFFSET_TO_TOP.  */
      offset_to_top = -offset_to_top;
    }

  base_address = obj->address () + offset_to_top;
  tag = value_tag_from_contents_and_address (obj_type, NULL, base_address);

  /* Make sure that we have a proper tag at the new address.
     Otherwise, offset_to_top is bogus (which can happen when
     the object is not initialized yet).  */

  if (!tag)
    return obj;

  obj_type = type_from_tag (tag);

  if (!obj_type)
    return obj;

  return value_from_contents_and_address (obj_type, NULL, base_address);
}

/* Return the "ada__tags__type_specific_data" type.  */

static struct type *
ada_get_tsd_type (struct inferior *inf)
{
  struct ada_inferior_data *data = get_ada_inferior_data (inf);

  if (data->tsd_type == 0)
    data->tsd_type = ada_find_any_type ("ada__tags__type_specific_data");
  return data->tsd_type;
}

/* Return the TSD (type-specific data) associated to the given TAG.
   TAG is assumed to be the tag of a tagged-type entity.

   May return NULL if we are unable to get the TSD.  */

static struct value *
ada_get_tsd_from_tag (struct value *tag)
{
  struct value *val;
  struct type *type;

  /* First option: The TSD is simply stored as a field of our TAG.
     Only older versions of GNAT would use this format, but we have
     to test it first, because there are no visible markers for
     the current approach except the absence of that field.  */

  val = ada_value_struct_elt (tag, "tsd", 1);
  if (val)
    return val;

  /* Try the second representation for the dispatch table (in which
     there is no explicit 'tsd' field in the referent of the tag pointer,
     and instead the tsd pointer is stored just before the dispatch
     table.  */

  type = ada_get_tsd_type (current_inferior());
  if (type == NULL)
    return NULL;
  type = lookup_pointer_type (lookup_pointer_type (type));
  val = value_cast (type, tag);
  if (val == NULL)
    return NULL;
  return value_ind (value_ptradd (val, -1));
}

/* Given the TSD of a tag (type-specific data), return a string
   containing the name of the associated type.

   May return NULL if we are unable to determine the tag name.  */

static gdb::unique_xmalloc_ptr<char>
ada_tag_name_from_tsd (struct value *tsd)
{
  struct value *val;

  val = ada_value_struct_elt (tsd, "expanded_name", 1);
  if (val == NULL)
    return NULL;
  gdb::unique_xmalloc_ptr<char> buffer
    = target_read_string (value_as_address (val), INT_MAX);
  if (buffer == nullptr)
    return nullptr;

  try
    {
      /* Let this throw an exception on error.  If the data is
	 uninitialized, we'd rather not have the user see a
	 warning.  */
      const char *folded = ada_fold_name (buffer.get (), true);
      return make_unique_xstrdup (folded);
    }
  catch (const gdb_exception &)
    {
      return nullptr;
    }
}

/* The type name of the dynamic type denoted by the 'tag value TAG, as
   a C string.

   Return NULL if the TAG is not an Ada tag, or if we were unable to
   determine the name of that tag.  */

gdb::unique_xmalloc_ptr<char>
ada_tag_name (struct value *tag)
{
  gdb::unique_xmalloc_ptr<char> name;

  if (!ada_is_tag_type (tag->type ()))
    return NULL;

  /* It is perfectly possible that an exception be raised while trying
     to determine the TAG's name, even under normal circumstances:
     The associated variable may be uninitialized or corrupted, for
     instance. We do not let any exception propagate past this point.
     instead we return NULL.

     We also do not print the error message either (which often is very
     low-level (Eg: "Cannot read memory at 0x[...]"), but instead let
     the caller print a more meaningful message if necessary.  */
  try
    {
      struct value *tsd = ada_get_tsd_from_tag (tag);

      if (tsd != NULL)
	name = ada_tag_name_from_tsd (tsd);
    }
  catch (const gdb_exception_error &e)
    {
    }

  return name;
}

/* The parent type of TYPE, or NULL if none.  */

struct type *
ada_parent_type (struct type *type)
{
  int i;

  type = ada_check_typedef (type);

  if (type == NULL || type->code () != TYPE_CODE_STRUCT)
    return NULL;

  for (i = 0; i < type->num_fields (); i += 1)
    if (ada_is_parent_field (type, i))
      {
	struct type *parent_type = type->field (i).type ();

	/* If the _parent field is a pointer, then dereference it.  */
	if (parent_type->code () == TYPE_CODE_PTR)
	  parent_type = parent_type->target_type ();
	/* If there is a parallel XVS type, get the actual base type.  */
	parent_type = ada_get_base_type (parent_type);

	return ada_check_typedef (parent_type);
      }

  return NULL;
}

/* True iff field number FIELD_NUM of structure type TYPE contains the
   parent-type (inherited) fields of a derived type.  Assumes TYPE is
   a structure type with at least FIELD_NUM+1 fields.  */

int
ada_is_parent_field (struct type *type, int field_num)
{
  const char *name = ada_check_typedef (type)->field (field_num).name ();

  return (name != NULL
	  && (startswith (name, "PARENT")
	      || startswith (name, "_parent")));
}

/* True iff field number FIELD_NUM of structure type TYPE is a
   transparent wrapper field (which should be silently traversed when doing
   field selection and flattened when printing).  Assumes TYPE is a
   structure type with at least FIELD_NUM+1 fields.  Such fields are always
   structures.  */

int
ada_is_wrapper_field (struct type *type, int field_num)
{
  const char *name = type->field (field_num).name ();

  if (name != NULL && strcmp (name, "RETVAL") == 0)
    {
      /* This happens in functions with "out" or "in out" parameters
	 which are passed by copy.  For such functions, GNAT describes
	 the function's return type as being a struct where the return
	 value is in a field called RETVAL, and where the other "out"
	 or "in out" parameters are fields of that struct.  This is not
	 a wrapper.  */
      return 0;
    }

  return (name != NULL
	  && (startswith (name, "PARENT")
	      || strcmp (name, "REP") == 0
	      || startswith (name, "_parent")
	      || name[0] == 'S' || name[0] == 'R' || name[0] == 'O'));
}

/* True iff field number FIELD_NUM of structure or union type TYPE
   is a variant wrapper.  Assumes TYPE is a structure type with at least
   FIELD_NUM+1 fields.  */

int
ada_is_variant_part (struct type *type, int field_num)
{
  /* Only Ada types are eligible.  */
  if (!ADA_TYPE_P (type))
    return 0;

  struct type *field_type = type->field (field_num).type ();

  return (field_type->code () == TYPE_CODE_UNION
	  || (is_dynamic_field (type, field_num)
	      && (field_type->target_type ()->code ()
		  == TYPE_CODE_UNION)));
}

/* Assuming that VAR_TYPE is a variant wrapper (type of the variant part)
   whose discriminants are contained in the record type OUTER_TYPE,
   returns the type of the controlling discriminant for the variant.
   May return NULL if the type could not be found.  */

struct type *
ada_variant_discrim_type (struct type *var_type, struct type *outer_type)
{
  const char *name = ada_variant_discrim_name (var_type);

  return ada_lookup_struct_elt_type (outer_type, name, 1, 1);
}

/* Assuming that TYPE is the type of a variant wrapper, and FIELD_NUM is a
   valid field number within it, returns 1 iff field FIELD_NUM of TYPE
   represents a 'when others' clause; otherwise 0.  */

static int
ada_is_others_clause (struct type *type, int field_num)
{
  const char *name = type->field (field_num).name ();

  return (name != NULL && name[0] == 'O');
}

/* Assuming that TYPE0 is the type of the variant part of a record,
   returns the name of the discriminant controlling the variant.
   The value is valid until the next call to ada_variant_discrim_name.  */

const char *
ada_variant_discrim_name (struct type *type0)
{
  static std::string result;
  struct type *type;
  const char *name;
  const char *discrim_end;
  const char *discrim_start;

  if (type0->code () == TYPE_CODE_PTR)
    type = type0->target_type ();
  else
    type = type0;

  name = ada_type_name (type);

  if (name == NULL || name[0] == '\000')
    return "";

  for (discrim_end = name + strlen (name) - 6; discrim_end != name;
       discrim_end -= 1)
    {
      if (startswith (discrim_end, "___XVN"))
	break;
    }
  if (discrim_end == name)
    return "";

  for (discrim_start = discrim_end; discrim_start != name + 3;
       discrim_start -= 1)
    {
      if (discrim_start == name + 1)
	return "";
      if ((discrim_start > name + 3
	   && startswith (discrim_start - 3, "___"))
	  || discrim_start[-1] == '.')
	break;
    }

  result = std::string (discrim_start, discrim_end - discrim_start);
  return result.c_str ();
}

/* Scan STR for a subtype-encoded number, beginning at position K.
   Put the position of the character just past the number scanned in
   *NEW_K, if NEW_K!=NULL.  Put the scanned number in *R, if R!=NULL.
   Return 1 if there was a valid number at the given position, and 0
   otherwise.  A "subtype-encoded" number consists of the absolute value
   in decimal, followed by the letter 'm' to indicate a negative number.
   Assumes 0m does not occur.  */

int
ada_scan_number (const char str[], int k, LONGEST * R, int *new_k)
{
  ULONGEST RU;

  if (!isdigit (str[k]))
    return 0;

  /* Do it the hard way so as not to make any assumption about
     the relationship of unsigned long (%lu scan format code) and
     LONGEST.  */
  RU = 0;
  while (isdigit (str[k]))
    {
      RU = RU * 10 + (str[k] - '0');
      k += 1;
    }

  if (str[k] == 'm')
    {
      if (R != NULL)
	*R = (-(LONGEST) (RU - 1)) - 1;
      k += 1;
    }
  else if (R != NULL)
    *R = (LONGEST) RU;

  /* NOTE on the above: Technically, C does not say what the results of
     - (LONGEST) RU or (LONGEST) -RU are for RU == largest positive
     number representable as a LONGEST (although either would probably work
     in most implementations).  When RU>0, the locution in the then branch
     above is always equivalent to the negative of RU.  */

  if (new_k != NULL)
    *new_k = k;
  return 1;
}

/* Assuming that TYPE is a variant part wrapper type (a VARIANTS field),
   and FIELD_NUM is a valid field number within it, returns 1 iff VAL is
   in the range encoded by field FIELD_NUM of TYPE; otherwise 0.  */

static int
ada_in_variant (LONGEST val, struct type *type, int field_num)
{
  const char *name = type->field (field_num).name ();
  int p;

  p = 0;
  while (1)
    {
      switch (name[p])
	{
	case '\0':
	  return 0;
	case 'S':
	  {
	    LONGEST W;

	    if (!ada_scan_number (name, p + 1, &W, &p))
	      return 0;
	    if (val == W)
	      return 1;
	    break;
	  }
	case 'R':
	  {
	    LONGEST L, U;

	    if (!ada_scan_number (name, p + 1, &L, &p)
		|| name[p] != 'T' || !ada_scan_number (name, p + 1, &U, &p))
	      return 0;
	    if (val >= L && val <= U)
	      return 1;
	    break;
	  }
	case 'O':
	  return 1;
	default:
	  return 0;
	}
    }
}

/* FIXME: Lots of redundancy below.  Try to consolidate.  */

/* Given a value ARG1 (offset by OFFSET bytes) of a struct or union type
   ARG_TYPE, extract and return the value of one of its (non-static)
   fields.  FIELDNO says which field.   Differs from value_primitive_field
   only in that it can handle packed values of arbitrary type.  */

struct value *
ada_value_primitive_field (struct value *arg1, int offset, int fieldno,
			   struct type *arg_type)
{
  struct type *type;

  arg_type = ada_check_typedef (arg_type);
  type = arg_type->field (fieldno).type ();

  /* Handle packed fields.  It might be that the field is not packed
     relative to its containing structure, but the structure itself is
     packed; in this case we must take the bit-field path.  */
  if (arg_type->field (fieldno).bitsize () != 0 || arg1->bitpos () != 0)
    {
      int bit_pos = arg_type->field (fieldno).loc_bitpos ();
      int bit_size = arg_type->field (fieldno).bitsize ();

      return ada_value_primitive_packed_val (arg1,
					     arg1->contents ().data (),
					     offset + bit_pos / 8,
					     bit_pos % 8, bit_size, type);
    }
  else
    return arg1->primitive_field (offset, fieldno, arg_type);
}

/* Find field with name NAME in object of type TYPE.  If found, 
   set the following for each argument that is non-null:
    - *FIELD_TYPE_P to the field's type; 
    - *BYTE_OFFSET_P to OFFSET + the byte offset of the field within 
      an object of that type;
    - *BIT_OFFSET_P to the bit offset modulo byte size of the field; 
    - *BIT_SIZE_P to its size in bits if the field is packed, and 
      0 otherwise;
   If INDEX_P is non-null, increment *INDEX_P by the number of source-visible
   fields up to but not including the desired field, or by the total
   number of fields if not found.   A NULL value of NAME never
   matches; the function just counts visible fields in this case.
   
   Notice that we need to handle when a tagged record hierarchy
   has some components with the same name, like in this scenario:

      type Top_T is tagged record
	 N : Integer := 1;
	 U : Integer := 974;
	 A : Integer := 48;
      end record;

      type Middle_T is new Top.Top_T with record
	 N : Character := 'a';
	 C : Integer := 3;
      end record;

     type Bottom_T is new Middle.Middle_T with record
	N : Float := 4.0;
	C : Character := '5';
	X : Integer := 6;
	A : Character := 'J';
     end record;

   Let's say we now have a variable declared and initialized as follow:

     TC : Top_A := new Bottom_T;

   And then we use this variable to call this function

     procedure Assign (Obj: in out Top_T; TV : Integer);

   as follow:

      Assign (Top_T (B), 12);

   Now, we're in the debugger, and we're inside that procedure
   then and we want to print the value of obj.c:

   Usually, the tagged record or one of the parent type owns the
   component to print and there's no issue but in this particular
   case, what does it mean to ask for Obj.C? Since the actual
   type for object is type Bottom_T, it could mean two things: type
   component C from the Middle_T view, but also component C from
   Bottom_T.  So in that "undefined" case, when the component is
   not found in the non-resolved type (which includes all the
   components of the parent type), then resolve it and see if we
   get better luck once expanded.

   In the case of homonyms in the derived tagged type, we don't
   guaranty anything, and pick the one that's easiest for us
   to program.

   Returns 1 if found, 0 otherwise.  */

static int
find_struct_field (const char *name, struct type *type, int offset,
		   struct type **field_type_p,
		   int *byte_offset_p, int *bit_offset_p, int *bit_size_p,
		   int *index_p)
{
  int i;
  int parent_offset = -1;

  type = ada_check_typedef (type);

  if (field_type_p != NULL)
    *field_type_p = NULL;
  if (byte_offset_p != NULL)
    *byte_offset_p = 0;
  if (bit_offset_p != NULL)
    *bit_offset_p = 0;
  if (bit_size_p != NULL)
    *bit_size_p = 0;

  for (i = 0; i < type->num_fields (); i += 1)
    {
      /* These can't be computed using TYPE_FIELD_BITPOS for a dynamic
	 type.  However, we only need the values to be correct when
	 the caller asks for them.  */
      int bit_pos = 0, fld_offset = 0;
      if (byte_offset_p != nullptr || bit_offset_p != nullptr)
	{
	  bit_pos = type->field (i).loc_bitpos ();
	  fld_offset = offset + bit_pos / 8;
	}

      const char *t_field_name = type->field (i).name ();

      if (t_field_name == NULL)
	continue;

      else if (ada_is_parent_field (type, i))
	{
	  /* This is a field pointing us to the parent type of a tagged
	     type.  As hinted in this function's documentation, we give
	     preference to fields in the current record first, so what
	     we do here is just record the index of this field before
	     we skip it.  If it turns out we couldn't find our field
	     in the current record, then we'll get back to it and search
	     inside it whether the field might exist in the parent.  */

	  parent_offset = i;
	  continue;
	}

      else if (name != NULL && field_name_match (t_field_name, name))
	{
	  int bit_size = type->field (i).bitsize ();

	  if (field_type_p != NULL)
	    *field_type_p = type->field (i).type ();
	  if (byte_offset_p != NULL)
	    *byte_offset_p = fld_offset;
	  if (bit_offset_p != NULL)
	    *bit_offset_p = bit_pos % 8;
	  if (bit_size_p != NULL)
	    *bit_size_p = bit_size;
	  return 1;
	}
      else if (ada_is_wrapper_field (type, i))
	{
	  if (find_struct_field (name, type->field (i).type (), fld_offset,
				 field_type_p, byte_offset_p, bit_offset_p,
				 bit_size_p, index_p))
	    return 1;
	}
      else if (ada_is_variant_part (type, i))
	{
	  /* PNH: Wait.  Do we ever execute this section, or is ARG always of 
	     fixed type?? */
	  int j;
	  struct type *field_type
	    = ada_check_typedef (type->field (i).type ());

	  for (j = 0; j < field_type->num_fields (); j += 1)
	    {
	      if (find_struct_field (name, field_type->field (j).type (),
				     fld_offset
				     + field_type->field (j).loc_bitpos () / 8,
				     field_type_p, byte_offset_p,
				     bit_offset_p, bit_size_p, index_p))
		return 1;
	    }
	}
      else if (index_p != NULL)
	*index_p += 1;
    }

  /* Field not found so far.  If this is a tagged type which
     has a parent, try finding that field in the parent now.  */

  if (parent_offset != -1)
    {
      /* As above, only compute the offset when truly needed.  */
      int fld_offset = offset;
      if (byte_offset_p != nullptr || bit_offset_p != nullptr)
	{
	  int bit_pos = type->field (parent_offset).loc_bitpos ();
	  fld_offset += bit_pos / 8;
	}

      if (find_struct_field (name, type->field (parent_offset).type (),
			     fld_offset, field_type_p, byte_offset_p,
			     bit_offset_p, bit_size_p, index_p))
	return 1;
    }

  return 0;
}

/* Number of user-visible fields in record type TYPE.  */

static int
num_visible_fields (struct type *type)
{
  int n;

  n = 0;
  find_struct_field (NULL, type, 0, NULL, NULL, NULL, NULL, &n);
  return n;
}

/* Look for a field NAME in ARG.  Adjust the address of ARG by OFFSET bytes,
   and search in it assuming it has (class) type TYPE.
   If found, return value, else return NULL.

   Searches recursively through wrapper fields (e.g., '_parent').

   In the case of homonyms in the tagged types, please refer to the
   long explanation in find_struct_field's function documentation.  */

static struct value *
ada_search_struct_field (const char *name, struct value *arg, int offset,
			 struct type *type)
{
  int i;
  int parent_offset = -1;

  type = ada_check_typedef (type);
  for (i = 0; i < type->num_fields (); i += 1)
    {
      const char *t_field_name = type->field (i).name ();

      if (t_field_name == NULL)
	continue;

      else if (ada_is_parent_field (type, i))
	{
	  /* This is a field pointing us to the parent type of a tagged
	     type.  As hinted in this function's documentation, we give
	     preference to fields in the current record first, so what
	     we do here is just record the index of this field before
	     we skip it.  If it turns out we couldn't find our field
	     in the current record, then we'll get back to it and search
	     inside it whether the field might exist in the parent.  */

	  parent_offset = i;
	  continue;
	}

      else if (field_name_match (t_field_name, name))
	return ada_value_primitive_field (arg, offset, i, type);

      else if (ada_is_wrapper_field (type, i))
	{
	  struct value *v =     /* Do not let indent join lines here.  */
	    ada_search_struct_field (name, arg,
				     offset + type->field (i).loc_bitpos () / 8,
				     type->field (i).type ());

	  if (v != NULL)
	    return v;
	}

      else if (ada_is_variant_part (type, i))
	{
	  /* PNH: Do we ever get here?  See find_struct_field.  */
	  int j;
	  struct type *field_type = ada_check_typedef (type->field (i).type ());
	  int var_offset = offset + type->field (i).loc_bitpos () / 8;

	  for (j = 0; j < field_type->num_fields (); j += 1)
	    {
	      struct value *v = ada_search_struct_field /* Force line
							   break.  */
		(name, arg,
		 var_offset + field_type->field (j).loc_bitpos () / 8,
		 field_type->field (j).type ());

	      if (v != NULL)
		return v;
	    }
	}
    }

  /* Field not found so far.  If this is a tagged type which
     has a parent, try finding that field in the parent now.  */

  if (parent_offset != -1)
    {
      struct value *v = ada_search_struct_field (
	name, arg, offset + type->field (parent_offset).loc_bitpos () / 8,
	type->field (parent_offset).type ());

      if (v != NULL)
	return v;
    }

  return NULL;
}

static struct value *ada_index_struct_field_1 (int *, struct value *,
					       int, struct type *);


/* Return field #INDEX in ARG, where the index is that returned by
 * find_struct_field through its INDEX_P argument.  Adjust the address
 * of ARG by OFFSET bytes, and search in it assuming it has (class) type TYPE.
 * If found, return value, else return NULL.  */

static struct value *
ada_index_struct_field (int index, struct value *arg, int offset,
			struct type *type)
{
  return ada_index_struct_field_1 (&index, arg, offset, type);
}


/* Auxiliary function for ada_index_struct_field.  Like
 * ada_index_struct_field, but takes index from *INDEX_P and modifies
 * *INDEX_P.  */

static struct value *
ada_index_struct_field_1 (int *index_p, struct value *arg, int offset,
			  struct type *type)
{
  int i;
  type = ada_check_typedef (type);

  for (i = 0; i < type->num_fields (); i += 1)
    {
      if (type->field (i).name () == NULL)
	continue;
      else if (ada_is_wrapper_field (type, i))
	{
	  struct value *v =     /* Do not let indent join lines here.  */
	    ada_index_struct_field_1 (index_p, arg,
				      offset + type->field (i).loc_bitpos () / 8,
				      type->field (i).type ());

	  if (v != NULL)
	    return v;
	}

      else if (ada_is_variant_part (type, i))
	{
	  /* PNH: Do we ever get here?  See ada_search_struct_field,
	     find_struct_field.  */
	  error (_("Cannot assign this kind of variant record"));
	}
      else if (*index_p == 0)
	return ada_value_primitive_field (arg, offset, i, type);
      else
	*index_p -= 1;
    }
  return NULL;
}

/* Return a string representation of type TYPE.  */

static std::string
type_as_string (struct type *type)
{
  string_file tmp_stream;

  type_print (type, "", &tmp_stream, -1);

  return tmp_stream.release ();
}

/* Given a type TYPE, look up the type of the component of type named NAME.

   Matches any field whose name has NAME as a prefix, possibly
   followed by "___".

   TYPE can be either a struct or union.  If REFOK, TYPE may also 
   be a (pointer or reference)+ to a struct or union, and the
   ultimate target type will be searched.

   Looks recursively into variant clauses and parent types.

   In the case of homonyms in the tagged types, please refer to the
   long explanation in find_struct_field's function documentation.

   If NOERR is nonzero, return NULL if NAME is not suitably defined or
   TYPE is not a type of the right kind.  */

static struct type *
ada_lookup_struct_elt_type (struct type *type, const char *name, int refok,
			    int noerr)
{
  if (name == NULL)
    goto BadName;

  if (refok && type != NULL)
    while (1)
      {
	type = ada_check_typedef (type);
	if (type->code () != TYPE_CODE_PTR && type->code () != TYPE_CODE_REF)
	  break;
	type = type->target_type ();
      }

  if (type == NULL
      || (type->code () != TYPE_CODE_STRUCT
	  && type->code () != TYPE_CODE_UNION))
    {
      if (noerr)
	return NULL;

      error (_("Type %s is not a structure or union type"),
	     type != NULL ? type_as_string (type).c_str () : _("(null)"));
    }

  type = to_static_fixed_type (type);

  struct type *result;
  find_struct_field (name, type, 0, &result, nullptr, nullptr, nullptr,
		     nullptr);
  if (result != nullptr)
    return result;

BadName:
  if (!noerr)
    {
      const char *name_str = name != NULL ? name : _("<null>");

      error (_("Type %s has no component named %s"),
	     type_as_string (type).c_str (), name_str);
    }

  return NULL;
}

/* Assuming that VAR_TYPE is the type of a variant part of a record (a union),
   within a value of type OUTER_TYPE, return true iff VAR_TYPE
   represents an unchecked union (that is, the variant part of a
   record that is named in an Unchecked_Union pragma).  */

static int
is_unchecked_variant (struct type *var_type, struct type *outer_type)
{
  const char *discrim_name = ada_variant_discrim_name (var_type);

  return (ada_lookup_struct_elt_type (outer_type, discrim_name, 0, 1) == NULL);
}


/* Assuming that VAR_TYPE is the type of a variant part of a record (a union),
   within OUTER, determine which variant clause (field number in VAR_TYPE,
   numbering from 0) is applicable.  Returns -1 if none are.  */

int
ada_which_variant_applies (struct type *var_type, struct value *outer)
{
  int others_clause;
  int i;
  const char *discrim_name = ada_variant_discrim_name (var_type);
  struct value *discrim;
  LONGEST discrim_val;

  /* Using plain value_from_contents_and_address here causes problems
     because we will end up trying to resolve a type that is currently
     being constructed.  */
  discrim = ada_value_struct_elt (outer, discrim_name, 1);
  if (discrim == NULL)
    return -1;
  discrim_val = value_as_long (discrim);

  others_clause = -1;
  for (i = 0; i < var_type->num_fields (); i += 1)
    {
      if (ada_is_others_clause (var_type, i))
	others_clause = i;
      else if (ada_in_variant (discrim_val, var_type, i))
	return i;
    }

  return others_clause;
}



				/* Dynamic-Sized Records */

/* Strategy: The type ostensibly attached to a value with dynamic size
   (i.e., a size that is not statically recorded in the debugging
   data) does not accurately reflect the size or layout of the value.
   Our strategy is to convert these values to values with accurate,
   conventional types that are constructed on the fly.  */

/* There is a subtle and tricky problem here.  In general, we cannot
   determine the size of dynamic records without its data.  However,
   the 'struct value' data structure, which GDB uses to represent
   quantities in the inferior process (the target), requires the size
   of the type at the time of its allocation in order to reserve space
   for GDB's internal copy of the data.  That's why the
   'to_fixed_xxx_type' routines take (target) addresses as parameters,
   rather than struct value*s.

   However, GDB's internal history variables ($1, $2, etc.) are
   struct value*s containing internal copies of the data that are not, in
   general, the same as the data at their corresponding addresses in
   the target.  Fortunately, the types we give to these values are all
   conventional, fixed-size types (as per the strategy described
   above), so that we don't usually have to perform the
   'to_fixed_xxx_type' conversions to look at their values.
   Unfortunately, there is one exception: if one of the internal
   history variables is an array whose elements are unconstrained
   records, then we will need to create distinct fixed types for each
   element selected.  */

/* The upshot of all of this is that many routines take a (type, host
   address, target address) triple as arguments to represent a value.
   The host address, if non-null, is supposed to contain an internal
   copy of the relevant data; otherwise, the program is to consult the
   target at the target address.  */

/* Assuming that VAL0 represents a pointer value, the result of
   dereferencing it.  Differs from value_ind in its treatment of
   dynamic-sized types.  */

struct value *
ada_value_ind (struct value *val0)
{
  struct value *val = value_ind (val0);

  if (ada_is_tagged_type (val->type (), 0))
    val = ada_tag_value_at_base_address (val);

  return ada_to_fixed_value (val);
}

/* The value resulting from dereferencing any "reference to"
   qualifiers on VAL0.  */

static struct value *
ada_coerce_ref (struct value *val0)
{
  if (val0->type ()->code () == TYPE_CODE_REF)
    {
      struct value *val = val0;

      val = coerce_ref (val);

      if (ada_is_tagged_type (val->type (), 0))
	val = ada_tag_value_at_base_address (val);

      return ada_to_fixed_value (val);
    }
  else
    return val0;
}

/* Return the bit alignment required for field #F of template type TYPE.  */

static unsigned int
field_alignment (struct type *type, int f)
{
  const char *name = type->field (f).name ();
  int len;
  int align_offset;

  /* The field name should never be null, unless the debugging information
     is somehow malformed.  In this case, we assume the field does not
     require any alignment.  */
  if (name == NULL)
    return 1;

  len = strlen (name);

  if (!isdigit (name[len - 1]))
    return 1;

  if (isdigit (name[len - 2]))
    align_offset = len - 2;
  else
    align_offset = len - 1;

  if (align_offset < 7 || !startswith (name + align_offset - 6, "___XV"))
    return TARGET_CHAR_BIT;

  return atoi (name + align_offset) * TARGET_CHAR_BIT;
}

/* Find a typedef or tag symbol named NAME.  Ignores ambiguity.  */

static struct symbol *
ada_find_any_type_symbol (const char *name)
{
  struct symbol *sym;

  sym = standard_lookup (name, get_selected_block (NULL), VAR_DOMAIN);
  if (sym != NULL && sym->aclass () == LOC_TYPEDEF)
    return sym;

  sym = standard_lookup (name, NULL, STRUCT_DOMAIN);
  return sym;
}

/* Find a type named NAME.  Ignores ambiguity.  This routine will look
   solely for types defined by debug info, it will not search the GDB
   primitive types.  */

static struct type *
ada_find_any_type (const char *name)
{
  struct symbol *sym = ada_find_any_type_symbol (name);

  if (sym != NULL)
    return sym->type ();

  return NULL;
}

/* Given NAME_SYM and an associated BLOCK, find a "renaming" symbol
   associated with NAME_SYM's name.  NAME_SYM may itself be a renaming
   symbol, in which case it is returned.  Otherwise, this looks for
   symbols whose name is that of NAME_SYM suffixed with  "___XR".
   Return symbol if found, and NULL otherwise.  */

static bool
ada_is_renaming_symbol (struct symbol *name_sym)
{
  const char *name = name_sym->linkage_name ();
  return strstr (name, "___XR") != NULL;
}

/* Because of GNAT encoding conventions, several GDB symbols may match a
   given type name.  If the type denoted by TYPE0 is to be preferred to
   that of TYPE1 for purposes of type printing, return non-zero;
   otherwise return 0.  */

int
ada_prefer_type (struct type *type0, struct type *type1)
{
  if (type1 == NULL)
    return 1;
  else if (type0 == NULL)
    return 0;
  else if (type1->code () == TYPE_CODE_VOID)
    return 1;
  else if (type0->code () == TYPE_CODE_VOID)
    return 0;
  else if (type1->name () == NULL && type0->name () != NULL)
    return 1;
  else if (ada_is_constrained_packed_array_type (type0))
    return 1;
  else if (ada_is_array_descriptor_type (type0)
	   && !ada_is_array_descriptor_type (type1))
    return 1;
  else
    {
      const char *type0_name = type0->name ();
      const char *type1_name = type1->name ();

      if (type0_name != NULL && strstr (type0_name, "___XR") != NULL
	  && (type1_name == NULL || strstr (type1_name, "___XR") == NULL))
	return 1;
    }
  return 0;
}

/* The name of TYPE, which is its TYPE_NAME.  Null if TYPE is
   null.  */

const char *
ada_type_name (struct type *type)
{
  if (type == NULL)
    return NULL;
  return type->name ();
}

/* Search the list of "descriptive" types associated to TYPE for a type
   whose name is NAME.  */

static struct type *
find_parallel_type_by_descriptive_type (struct type *type, const char *name)
{
  struct type *result, *tmp;

  if (ada_ignore_descriptive_types_p)
    return NULL;

  /* If there no descriptive-type info, then there is no parallel type
     to be found.  */
  if (!HAVE_GNAT_AUX_INFO (type))
    return NULL;

  result = TYPE_DESCRIPTIVE_TYPE (type);
  while (result != NULL)
    {
      const char *result_name = ada_type_name (result);

      if (result_name == NULL)
	{
	  warning (_("unexpected null name on descriptive type"));
	  return NULL;
	}

      /* If the names match, stop.  */
      if (strcmp (result_name, name) == 0)
	break;

      /* Otherwise, look at the next item on the list, if any.  */
      if (HAVE_GNAT_AUX_INFO (result))
	tmp = TYPE_DESCRIPTIVE_TYPE (result);
      else
	tmp = NULL;

      /* If not found either, try after having resolved the typedef.  */
      if (tmp != NULL)
	result = tmp;
      else
	{
	  result = check_typedef (result);
	  if (HAVE_GNAT_AUX_INFO (result))
	    result = TYPE_DESCRIPTIVE_TYPE (result);
	  else
	    result = NULL;
	}
    }

  /* If we didn't find a match, see whether this is a packed array.  With
     older compilers, the descriptive type information is either absent or
     irrelevant when it comes to packed arrays so the above lookup fails.
     Fall back to using a parallel lookup by name in this case.  */
  if (result == NULL && ada_is_constrained_packed_array_type (type))
    return ada_find_any_type (name);

  return result;
}

/* Find a parallel type to TYPE with the specified NAME, using the
   descriptive type taken from the debugging information, if available,
   and otherwise using the (slower) name-based method.  */

static struct type *
ada_find_parallel_type_with_name (struct type *type, const char *name)
{
  struct type *result = NULL;

  if (HAVE_GNAT_AUX_INFO (type))
    result = find_parallel_type_by_descriptive_type (type, name);
  else
    result = ada_find_any_type (name);

  return result;
}

/* Same as above, but specify the name of the parallel type by appending
   SUFFIX to the name of TYPE.  */

struct type *
ada_find_parallel_type (struct type *type, const char *suffix)
{
  char *name;
  const char *type_name = ada_type_name (type);
  int len;

  if (type_name == NULL)
    return NULL;

  len = strlen (type_name);

  name = (char *) alloca (len + strlen (suffix) + 1);

  strcpy (name, type_name);
  strcpy (name + len, suffix);

  return ada_find_parallel_type_with_name (type, name);
}

/* If TYPE is a variable-size record type, return the corresponding template
   type describing its fields.  Otherwise, return NULL.  */

static struct type *
dynamic_template_type (struct type *type)
{
  type = ada_check_typedef (type);

  if (type == NULL || type->code () != TYPE_CODE_STRUCT
      || ada_type_name (type) == NULL)
    return NULL;
  else
    {
      int len = strlen (ada_type_name (type));

      if (len > 6 && strcmp (ada_type_name (type) + len - 6, "___XVE") == 0)
	return type;
      else
	return ada_find_parallel_type (type, "___XVE");
    }
}

/* Assuming that TEMPL_TYPE is a union or struct type, returns
   non-zero iff field FIELD_NUM of TEMPL_TYPE has dynamic size.  */

static int
is_dynamic_field (struct type *templ_type, int field_num)
{
  const char *name = templ_type->field (field_num).name ();

  return name != NULL
    && templ_type->field (field_num).type ()->code () == TYPE_CODE_PTR
    && strstr (name, "___XVL") != NULL;
}

/* The index of the variant field of TYPE, or -1 if TYPE does not
   represent a variant record type.  */

static int
variant_field_index (struct type *type)
{
  int f;

  if (type == NULL || type->code () != TYPE_CODE_STRUCT)
    return -1;

  for (f = 0; f < type->num_fields (); f += 1)
    {
      if (ada_is_variant_part (type, f))
	return f;
    }
  return -1;
}

/* A record type with no fields.  */

static struct type *
empty_record (struct type *templ)
{
  struct type *type = type_allocator (templ).new_type ();

  type->set_code (TYPE_CODE_STRUCT);
  INIT_NONE_SPECIFIC (type);
  type->set_name ("<empty>");
  type->set_length (0);
  return type;
}

/* An ordinary record type (with fixed-length fields) that describes
   the value of type TYPE at VALADDR or ADDRESS (see comments at
   the beginning of this section) VAL according to GNAT conventions.
   DVAL0 should describe the (portion of a) record that contains any
   necessary discriminants.  It should be NULL if VAL->type () is
   an outer-level type (i.e., as opposed to a branch of a variant.)  A
   variant field (unless unchecked) is replaced by a particular branch
   of the variant.

   If not KEEP_DYNAMIC_FIELDS, then all fields whose position or
   length are not statically known are discarded.  As a consequence,
   VALADDR, ADDRESS and DVAL0 are ignored.

   NOTE: Limitations: For now, we assume that dynamic fields and
   variants occupy whole numbers of bytes.  However, they need not be
   byte-aligned.  */

struct type *
ada_template_to_fixed_record_type_1 (struct type *type,
				     const gdb_byte *valaddr,
				     CORE_ADDR address, struct value *dval0,
				     int keep_dynamic_fields)
{
  struct value *dval;
  struct type *rtype;
  int nfields, bit_len;
  int variant_field;
  long off;
  int fld_bit_len;
  int f;

  scoped_value_mark mark;

  /* Compute the number of fields in this record type that are going
     to be processed: unless keep_dynamic_fields, this includes only
     fields whose position and length are static will be processed.  */
  if (keep_dynamic_fields)
    nfields = type->num_fields ();
  else
    {
      nfields = 0;
      while (nfields < type->num_fields ()
	     && !ada_is_variant_part (type, nfields)
	     && !is_dynamic_field (type, nfields))
	nfields++;
    }

  rtype = type_allocator (type).new_type ();
  rtype->set_code (TYPE_CODE_STRUCT);
  INIT_NONE_SPECIFIC (rtype);
  rtype->alloc_fields (nfields);
  rtype->set_name (ada_type_name (type));
  rtype->set_is_fixed_instance (true);

  off = 0;
  bit_len = 0;
  variant_field = -1;

  for (f = 0; f < nfields; f += 1)
    {
      off = align_up (off, field_alignment (type, f))
	+ type->field (f).loc_bitpos ();
      rtype->field (f).set_loc_bitpos (off);
      rtype->field (f).set_bitsize (0);

      if (ada_is_variant_part (type, f))
	{
	  variant_field = f;
	  fld_bit_len = 0;
	}
      else if (is_dynamic_field (type, f))
	{
	  const gdb_byte *field_valaddr = valaddr;
	  CORE_ADDR field_address = address;
	  struct type *field_type = type->field (f).type ()->target_type ();

	  if (dval0 == NULL)
	    {
	      /* Using plain value_from_contents_and_address here
		 causes problems because we will end up trying to
		 resolve a type that is currently being
		 constructed.  */
	      dval = value_from_contents_and_address_unresolved (rtype,
								 valaddr,
								 address);
	      rtype = dval->type ();
	    }
	  else
	    dval = dval0;

	  /* If the type referenced by this field is an aligner type, we need
	     to unwrap that aligner type, because its size might not be set.
	     Keeping the aligner type would cause us to compute the wrong
	     size for this field, impacting the offset of the all the fields
	     that follow this one.  */
	  if (ada_is_aligner_type (field_type))
	    {
	      long field_offset = type->field (f).loc_bitpos ();

	      field_valaddr = cond_offset_host (field_valaddr, field_offset);
	      field_address = cond_offset_target (field_address, field_offset);
	      field_type = ada_aligned_type (field_type);
	    }

	  field_valaddr = cond_offset_host (field_valaddr,
					    off / TARGET_CHAR_BIT);
	  field_address = cond_offset_target (field_address,
					      off / TARGET_CHAR_BIT);

	  /* Get the fixed type of the field.  Note that, in this case,
	     we do not want to get the real type out of the tag: if
	     the current field is the parent part of a tagged record,
	     we will get the tag of the object.  Clearly wrong: the real
	     type of the parent is not the real type of the child.  We
	     would end up in an infinite loop.	*/
	  field_type = ada_get_base_type (field_type);
	  field_type = ada_to_fixed_type (field_type, field_valaddr,
					  field_address, dval, 0);

	  rtype->field (f).set_type (field_type);
	  rtype->field (f).set_name (type->field (f).name ());
	  /* The multiplication can potentially overflow.  But because
	     the field length has been size-checked just above, and
	     assuming that the maximum size is a reasonable value,
	     an overflow should not happen in practice.  So rather than
	     adding overflow recovery code to this already complex code,
	     we just assume that it's not going to happen.  */
	  fld_bit_len = rtype->field (f).type ()->length () * TARGET_CHAR_BIT;
	}
      else
	{
	  /* Note: If this field's type is a typedef, it is important
	     to preserve the typedef layer.

	     Otherwise, we might be transforming a typedef to a fat
	     pointer (encoding a pointer to an unconstrained array),
	     into a basic fat pointer (encoding an unconstrained
	     array).  As both types are implemented using the same
	     structure, the typedef is the only clue which allows us
	     to distinguish between the two options.  Stripping it
	     would prevent us from printing this field appropriately.  */
	  rtype->field (f).set_type (type->field (f).type ());
	  rtype->field (f).set_name (type->field (f).name ());
	  if (type->field (f).bitsize () > 0)
	    {
	      fld_bit_len = type->field (f).bitsize ();
	      rtype->field (f).set_bitsize (fld_bit_len);
	    }
	  else
	    {
	      struct type *field_type = type->field (f).type ();

	      /* We need to be careful of typedefs when computing
		 the length of our field.  If this is a typedef,
		 get the length of the target type, not the length
		 of the typedef.  */
	      if (field_type->code () == TYPE_CODE_TYPEDEF)
		field_type = ada_typedef_target_type (field_type);

	      fld_bit_len =
		ada_check_typedef (field_type)->length () * TARGET_CHAR_BIT;
	    }
	}
      if (off + fld_bit_len > bit_len)
	bit_len = off + fld_bit_len;
      off += fld_bit_len;
      rtype->set_length (align_up (bit_len, TARGET_CHAR_BIT) / TARGET_CHAR_BIT);
    }

  /* We handle the variant part, if any, at the end because of certain
     odd cases in which it is re-ordered so as NOT to be the last field of
     the record.  This can happen in the presence of representation
     clauses.  */
  if (variant_field >= 0)
    {
      struct type *branch_type;

      off = rtype->field (variant_field).loc_bitpos ();

      if (dval0 == NULL)
	{
	  /* Using plain value_from_contents_and_address here causes
	     problems because we will end up trying to resolve a type
	     that is currently being constructed.  */
	  dval = value_from_contents_and_address_unresolved (rtype, valaddr,
							     address);
	  rtype = dval->type ();
	}
      else
	dval = dval0;

      branch_type =
	to_fixed_variant_branch_type
	(type->field (variant_field).type (),
	 cond_offset_host (valaddr, off / TARGET_CHAR_BIT),
	 cond_offset_target (address, off / TARGET_CHAR_BIT), dval);
      if (branch_type == NULL)
	{
	  for (f = variant_field + 1; f < rtype->num_fields (); f += 1)
	    rtype->field (f - 1) = rtype->field (f);
	  rtype->set_num_fields (rtype->num_fields () - 1);
	}
      else
	{
	  rtype->field (variant_field).set_type (branch_type);
	  rtype->field (variant_field).set_name ("S");
	  fld_bit_len =
	    rtype->field (variant_field).type ()->length () * TARGET_CHAR_BIT;
	  if (off + fld_bit_len > bit_len)
	    bit_len = off + fld_bit_len;

	  rtype->set_length
	    (align_up (bit_len, TARGET_CHAR_BIT) / TARGET_CHAR_BIT);
	}
    }

  /* According to exp_dbug.ads, the size of TYPE for variable-size records
     should contain the alignment of that record, which should be a strictly
     positive value.  If null or negative, then something is wrong, most
     probably in the debug info.  In that case, we don't round up the size
     of the resulting type.  If this record is not part of another structure,
     the current RTYPE length might be good enough for our purposes.  */
  if (type->length () <= 0)
    {
      if (rtype->name ())
	warning (_("Invalid type size for `%s' detected: %s."),
		 rtype->name (), pulongest (type->length ()));
      else
	warning (_("Invalid type size for <unnamed> detected: %s."),
		 pulongest (type->length ()));
    }
  else
    rtype->set_length (align_up (rtype->length (), type->length ()));

  return rtype;
}

/* As for ada_template_to_fixed_record_type_1 with KEEP_DYNAMIC_FIELDS
   of 1.  */

static struct type *
template_to_fixed_record_type (struct type *type, const gdb_byte *valaddr,
			       CORE_ADDR address, struct value *dval0)
{
  return ada_template_to_fixed_record_type_1 (type, valaddr,
					      address, dval0, 1);
}

/* An ordinary record type in which ___XVL-convention fields and
   ___XVU- and ___XVN-convention field types in TYPE0 are replaced with
   static approximations, containing all possible fields.  Uses
   no runtime values.  Useless for use in values, but that's OK,
   since the results are used only for type determinations.   Works on both
   structs and unions.  Representation note: to save space, we memorize
   the result of this function in the type::target_type of the
   template type.  */

static struct type *
template_to_static_fixed_type (struct type *type0)
{
  struct type *type;
  int nfields;
  int f;

  /* No need no do anything if the input type is already fixed.  */
  if (type0->is_fixed_instance ())
    return type0;

  /* Likewise if we already have computed the static approximation.  */
  if (type0->target_type () != NULL)
    return type0->target_type ();

  /* Don't clone TYPE0 until we are sure we are going to need a copy.  */
  type = type0;
  nfields = type0->num_fields ();

  /* Whether or not we cloned TYPE0, cache the result so that we don't do
     recompute all over next time.  */
  type0->set_target_type (type);

  for (f = 0; f < nfields; f += 1)
    {
      struct type *field_type = type0->field (f).type ();
      struct type *new_type;

      if (is_dynamic_field (type0, f))
	{
	  field_type = ada_check_typedef (field_type);
	  new_type = to_static_fixed_type (field_type->target_type ());
	}
      else
	new_type = static_unwrap_type (field_type);

      if (new_type != field_type)
	{
	  /* Clone TYPE0 only the first time we get a new field type.  */
	  if (type == type0)
	    {
	      type = type_allocator (type0).new_type ();
	      type0->set_target_type (type);
	      type->set_code (type0->code ());
	      INIT_NONE_SPECIFIC (type);

	      type->copy_fields (type0);

	      type->set_name (ada_type_name (type0));
	      type->set_is_fixed_instance (true);
	      type->set_length (0);
	    }
	  type->field (f).set_type (new_type);
	  type->field (f).set_name (type0->field (f).name ());
	}
    }

  return type;
}

/* Given an object of type TYPE whose contents are at VALADDR and
   whose address in memory is ADDRESS, returns a revision of TYPE,
   which should be a non-dynamic-sized record, in which the variant
   part, if any, is replaced with the appropriate branch.  Looks
   for discriminant values in DVAL0, which can be NULL if the record
   contains the necessary discriminant values.  */

static struct type *
to_record_with_fixed_variant_part (struct type *type, const gdb_byte *valaddr,
				   CORE_ADDR address, struct value *dval0)
{
  struct value *dval;
  struct type *rtype;
  struct type *branch_type;
  int nfields = type->num_fields ();
  int variant_field = variant_field_index (type);

  if (variant_field == -1)
    return type;

  scoped_value_mark mark;
  if (dval0 == NULL)
    {
      dval = value_from_contents_and_address (type, valaddr, address);
      type = dval->type ();
    }
  else
    dval = dval0;

  rtype = type_allocator (type).new_type ();
  rtype->set_code (TYPE_CODE_STRUCT);
  INIT_NONE_SPECIFIC (rtype);
  rtype->copy_fields (type);

  rtype->set_name (ada_type_name (type));
  rtype->set_is_fixed_instance (true);
  rtype->set_length (type->length ());

  branch_type = to_fixed_variant_branch_type
    (type->field (variant_field).type (),
     cond_offset_host (valaddr,
		       type->field (variant_field).loc_bitpos ()
		       / TARGET_CHAR_BIT),
     cond_offset_target (address,
			 type->field (variant_field).loc_bitpos ()
			 / TARGET_CHAR_BIT), dval);
  if (branch_type == NULL)
    {
      int f;

      for (f = variant_field + 1; f < nfields; f += 1)
	rtype->field (f - 1) = rtype->field (f);
      rtype->set_num_fields (rtype->num_fields () - 1);
    }
  else
    {
      rtype->field (variant_field).set_type (branch_type);
      rtype->field (variant_field).set_name ("S");
      rtype->field (variant_field).set_bitsize (0);
      rtype->set_length (rtype->length () + branch_type->length ());
    }

  rtype->set_length (rtype->length ()
		     - type->field (variant_field).type ()->length ());

  return rtype;
}

/* An ordinary record type (with fixed-length fields) that describes
   the value at (TYPE0, VALADDR, ADDRESS) [see explanation at
   beginning of this section].   Any necessary discriminants' values
   should be in DVAL, a record value; it may be NULL if the object
   at ADDR itself contains any necessary discriminant values.
   Additionally, VALADDR and ADDRESS may also be NULL if no discriminant
   values from the record are needed.  Except in the case that DVAL,
   VALADDR, and ADDRESS are all 0 or NULL, a variant field (unless
   unchecked) is replaced by a particular branch of the variant.

   NOTE: the case in which DVAL and VALADDR are NULL and ADDRESS is 0
   is questionable and may be removed.  It can arise during the
   processing of an unconstrained-array-of-record type where all the
   variant branches have exactly the same size.  This is because in
   such cases, the compiler does not bother to use the XVS convention
   when encoding the record.  I am currently dubious of this
   shortcut and suspect the compiler should be altered.  FIXME.  */

static struct type *
to_fixed_record_type (struct type *type0, const gdb_byte *valaddr,
		      CORE_ADDR address, struct value *dval)
{
  struct type *templ_type;

  if (type0->is_fixed_instance ())
    return type0;

  templ_type = dynamic_template_type (type0);

  if (templ_type != NULL)
    return template_to_fixed_record_type (templ_type, valaddr, address, dval);
  else if (variant_field_index (type0) >= 0)
    {
      if (dval == NULL && valaddr == NULL && address == 0)
	return type0;
      return to_record_with_fixed_variant_part (type0, valaddr, address,
						dval);
    }
  else
    {
      type0->set_is_fixed_instance (true);
      return type0;
    }

}

/* An ordinary record type (with fixed-length fields) that describes
   the value at (VAR_TYPE0, VALADDR, ADDRESS), where VAR_TYPE0 is a
   union type.  Any necessary discriminants' values should be in DVAL,
   a record value.  That is, this routine selects the appropriate
   branch of the union at ADDR according to the discriminant value
   indicated in the union's type name.  Returns VAR_TYPE0 itself if
   it represents a variant subject to a pragma Unchecked_Union.  */

static struct type *
to_fixed_variant_branch_type (struct type *var_type0, const gdb_byte *valaddr,
			      CORE_ADDR address, struct value *dval)
{
  int which;
  struct type *templ_type;
  struct type *var_type;

  if (var_type0->code () == TYPE_CODE_PTR)
    var_type = var_type0->target_type ();
  else
    var_type = var_type0;

  templ_type = ada_find_parallel_type (var_type, "___XVU");

  if (templ_type != NULL)
    var_type = templ_type;

  if (is_unchecked_variant (var_type, dval->type ()))
      return var_type0;
  which = ada_which_variant_applies (var_type, dval);

  if (which < 0)
    return empty_record (var_type);
  else if (is_dynamic_field (var_type, which))
    return to_fixed_record_type
      (var_type->field (which).type ()->target_type(), valaddr, address, dval);
  else if (variant_field_index (var_type->field (which).type ()) >= 0)
    return
      to_fixed_record_type
      (var_type->field (which).type (), valaddr, address, dval);
  else
    return var_type->field (which).type ();
}

/* Assuming RANGE_TYPE is a TYPE_CODE_RANGE, return nonzero if
   ENCODING_TYPE, a type following the GNAT conventions for discrete
   type encodings, only carries redundant information.  */

static int
ada_is_redundant_range_encoding (struct type *range_type,
				 struct type *encoding_type)
{
  const char *bounds_str;
  int n;
  LONGEST lo, hi;

  gdb_assert (range_type->code () == TYPE_CODE_RANGE);

  if (get_base_type (range_type)->code ()
      != get_base_type (encoding_type)->code ())
    {
      /* The compiler probably used a simple base type to describe
	 the range type instead of the range's actual base type,
	 expecting us to get the real base type from the encoding
	 anyway.  In this situation, the encoding cannot be ignored
	 as redundant.  */
      return 0;
    }

  if (is_dynamic_type (range_type))
    return 0;

  if (encoding_type->name () == NULL)
    return 0;

  bounds_str = strstr (encoding_type->name (), "___XDLU_");
  if (bounds_str == NULL)
    return 0;

  n = 8; /* Skip "___XDLU_".  */
  if (!ada_scan_number (bounds_str, n, &lo, &n))
    return 0;
  if (range_type->bounds ()->low.const_val () != lo)
    return 0;

  n += 2; /* Skip the "__" separator between the two bounds.  */
  if (!ada_scan_number (bounds_str, n, &hi, &n))
    return 0;
  if (range_type->bounds ()->high.const_val () != hi)
    return 0;

  return 1;
}

/* Given the array type ARRAY_TYPE, return nonzero if DESC_TYPE,
   a type following the GNAT encoding for describing array type
   indices, only carries redundant information.  */

static int
ada_is_redundant_index_type_desc (struct type *array_type,
				  struct type *desc_type)
{
  struct type *this_layer = check_typedef (array_type);
  int i;

  for (i = 0; i < desc_type->num_fields (); i++)
    {
      if (!ada_is_redundant_range_encoding (this_layer->index_type (),
					    desc_type->field (i).type ()))
	return 0;
      this_layer = check_typedef (this_layer->target_type ());
    }

  return 1;
}

/* Assuming that TYPE0 is an array type describing the type of a value
   at ADDR, and that DVAL describes a record containing any
   discriminants used in TYPE0, returns a type for the value that
   contains no dynamic components (that is, no components whose sizes
   are determined by run-time quantities).  Unless IGNORE_TOO_BIG is
   true, gives an error message if the resulting type's size is over
   varsize_limit.  */

static struct type *
to_fixed_array_type (struct type *type0, struct value *dval,
		     int ignore_too_big)
{
  struct type *index_type_desc;
  struct type *result;
  int constrained_packed_array_p;
  static const char *xa_suffix = "___XA";

  type0 = ada_check_typedef (type0);
  if (type0->is_fixed_instance ())
    return type0;

  constrained_packed_array_p = ada_is_constrained_packed_array_type (type0);
  if (constrained_packed_array_p)
    {
      type0 = decode_constrained_packed_array_type (type0);
      if (type0 == nullptr)
	error (_("could not decode constrained packed array type"));
    }

  index_type_desc = ada_find_parallel_type (type0, xa_suffix);

  /* As mentioned in exp_dbug.ads, for non bit-packed arrays an
     encoding suffixed with 'P' may still be generated.  If so,
     it should be used to find the XA type.  */

  if (index_type_desc == NULL)
    {
      const char *type_name = ada_type_name (type0);

      if (type_name != NULL)
	{
	  const int len = strlen (type_name);
	  char *name = (char *) alloca (len + strlen (xa_suffix));

	  if (type_name[len - 1] == 'P')
	    {
	      strcpy (name, type_name);
	      strcpy (name + len - 1, xa_suffix);
	      index_type_desc = ada_find_parallel_type_with_name (type0, name);
	    }
	}
    }

  ada_fixup_array_indexes_type (index_type_desc);
  if (index_type_desc != NULL
      && ada_is_redundant_index_type_desc (type0, index_type_desc))
    {
      /* Ignore this ___XA parallel type, as it does not bring any
	 useful information.  This allows us to avoid creating fixed
	 versions of the array's index types, which would be identical
	 to the original ones.  This, in turn, can also help avoid
	 the creation of fixed versions of the array itself.  */
      index_type_desc = NULL;
    }

  if (index_type_desc == NULL)
    {
      struct type *elt_type0 = ada_check_typedef (type0->target_type ());

      /* NOTE: elt_type---the fixed version of elt_type0---should never
	 depend on the contents of the array in properly constructed
	 debugging data.  */
      /* Create a fixed version of the array element type.
	 We're not providing the address of an element here,
	 and thus the actual object value cannot be inspected to do
	 the conversion.  This should not be a problem, since arrays of
	 unconstrained objects are not allowed.  In particular, all
	 the elements of an array of a tagged type should all be of
	 the same type specified in the debugging info.  No need to
	 consult the object tag.  */
      struct type *elt_type = ada_to_fixed_type (elt_type0, 0, 0, dval, 1);

      /* Make sure we always create a new array type when dealing with
	 packed array types, since we're going to fix-up the array
	 type length and element bitsize a little further down.  */
      if (elt_type0 == elt_type && !constrained_packed_array_p)
	result = type0;
      else
	{
	  type_allocator alloc (type0);
	  result = create_array_type (alloc, elt_type, type0->index_type ());
	}
    }
  else
    {
      int i;
      struct type *elt_type0;

      elt_type0 = type0;
      for (i = index_type_desc->num_fields (); i > 0; i -= 1)
	elt_type0 = elt_type0->target_type ();

      /* NOTE: result---the fixed version of elt_type0---should never
	 depend on the contents of the array in properly constructed
	 debugging data.  */
      /* Create a fixed version of the array element type.
	 We're not providing the address of an element here,
	 and thus the actual object value cannot be inspected to do
	 the conversion.  This should not be a problem, since arrays of
	 unconstrained objects are not allowed.  In particular, all
	 the elements of an array of a tagged type should all be of
	 the same type specified in the debugging info.  No need to
	 consult the object tag.  */
      result =
	ada_to_fixed_type (ada_check_typedef (elt_type0), 0, 0, dval, 1);

      elt_type0 = type0;
      for (i = index_type_desc->num_fields () - 1; i >= 0; i -= 1)
	{
	  struct type *range_type =
	    to_fixed_range_type (index_type_desc->field (i).type (), dval);

	  type_allocator alloc (elt_type0);
	  result = create_array_type (alloc, result, range_type);
	  elt_type0 = elt_type0->target_type ();
	}
    }

  /* We want to preserve the type name.  This can be useful when
     trying to get the type name of a value that has already been
     printed (for instance, if the user did "print VAR; whatis $".  */
  result->set_name (type0->name ());

  if (constrained_packed_array_p)
    {
      /* So far, the resulting type has been created as if the original
	 type was a regular (non-packed) array type.  As a result, the
	 bitsize of the array elements needs to be set again, and the array
	 length needs to be recomputed based on that bitsize.  */
      int len = result->length () / result->target_type ()->length ();
      int elt_bitsize = type0->field (0).bitsize ();

      result->field (0).set_bitsize (elt_bitsize);
      result->set_length (len * elt_bitsize / HOST_CHAR_BIT);
      if (result->length () * HOST_CHAR_BIT < len * elt_bitsize)
	result->set_length (result->length () + 1);
    }

  result->set_is_fixed_instance (true);
  return result;
}


/* A standard type (containing no dynamically sized components)
   corresponding to TYPE for the value (TYPE, VALADDR, ADDRESS)
   DVAL describes a record containing any discriminants used in TYPE0,
   and may be NULL if there are none, or if the object of type TYPE at
   ADDRESS or in VALADDR contains these discriminants.
   
   If CHECK_TAG is not null, in the case of tagged types, this function
   attempts to locate the object's tag and use it to compute the actual
   type.  However, when ADDRESS is null, we cannot use it to determine the
   location of the tag, and therefore compute the tagged type's actual type.
   So we return the tagged type without consulting the tag.  */
   
static struct type *
ada_to_fixed_type_1 (struct type *type, const gdb_byte *valaddr,
		   CORE_ADDR address, struct value *dval, int check_tag)
{
  type = ada_check_typedef (type);

  /* Only un-fixed types need to be handled here.  */
  if (!HAVE_GNAT_AUX_INFO (type))
    return type;

  switch (type->code ())
    {
    default:
      return type;
    case TYPE_CODE_STRUCT:
      {
	struct type *static_type = to_static_fixed_type (type);
	struct type *fixed_record_type =
	  to_fixed_record_type (type, valaddr, address, NULL);

	/* If STATIC_TYPE is a tagged type and we know the object's address,
	   then we can determine its tag, and compute the object's actual
	   type from there.  Note that we have to use the fixed record
	   type (the parent part of the record may have dynamic fields
	   and the way the location of _tag is expressed may depend on
	   them).  */

	if (check_tag && address != 0 && ada_is_tagged_type (static_type, 0))
	  {
	    struct value *tag =
	      value_tag_from_contents_and_address
	      (fixed_record_type,
	       valaddr,
	       address);
	    struct type *real_type = type_from_tag (tag);
	    struct value *obj =
	      value_from_contents_and_address (fixed_record_type,
					       valaddr,
					       address);
	    fixed_record_type = obj->type ();
	    if (real_type != NULL)
	      return to_fixed_record_type
		(real_type, NULL,
		 ada_tag_value_at_base_address (obj)->address (), NULL);
	  }

	/* Check to see if there is a parallel ___XVZ variable.
	   If there is, then it provides the actual size of our type.  */
	else if (ada_type_name (fixed_record_type) != NULL)
	  {
	    const char *name = ada_type_name (fixed_record_type);
	    char *xvz_name
	      = (char *) alloca (strlen (name) + 7 /* "___XVZ\0" */);
	    bool xvz_found = false;
	    LONGEST size;

	    xsnprintf (xvz_name, strlen (name) + 7, "%s___XVZ", name);
	    try
	      {
		xvz_found = get_int_var_value (xvz_name, size);
	      }
	    catch (const gdb_exception_error &except)
	      {
		/* We found the variable, but somehow failed to read
		   its value.  Rethrow the same error, but with a little
		   bit more information, to help the user understand
		   what went wrong (Eg: the variable might have been
		   optimized out).  */
		throw_error (except.error,
			     _("unable to read value of %s (%s)"),
			     xvz_name, except.what ());
	      }

	    if (xvz_found && fixed_record_type->length () != size)
	      {
		fixed_record_type = copy_type (fixed_record_type);
		fixed_record_type->set_length (size);

		/* The FIXED_RECORD_TYPE may have be a stub.  We have
		   observed this when the debugging info is STABS, and
		   apparently it is something that is hard to fix.

		   In practice, we don't need the actual type definition
		   at all, because the presence of the XVZ variable allows us
		   to assume that there must be a XVS type as well, which we
		   should be able to use later, when we need the actual type
		   definition.

		   In the meantime, pretend that the "fixed" type we are
		   returning is NOT a stub, because this can cause trouble
		   when using this type to create new types targeting it.
		   Indeed, the associated creation routines often check
		   whether the target type is a stub and will try to replace
		   it, thus using a type with the wrong size.  This, in turn,
		   might cause the new type to have the wrong size too.
		   Consider the case of an array, for instance, where the size
		   of the array is computed from the number of elements in
		   our array multiplied by the size of its element.  */
		fixed_record_type->set_is_stub (false);
	      }
	  }
	return fixed_record_type;
      }
    case TYPE_CODE_ARRAY:
      return to_fixed_array_type (type, dval, 1);
    case TYPE_CODE_UNION:
      if (dval == NULL)
	return type;
      else
	return to_fixed_variant_branch_type (type, valaddr, address, dval);
    }
}

/* The same as ada_to_fixed_type_1, except that it preserves the type
   if it is a TYPE_CODE_TYPEDEF of a type that is already fixed.

   The typedef layer needs be preserved in order to differentiate between
   arrays and array pointers when both types are implemented using the same
   fat pointer.  In the array pointer case, the pointer is encoded as
   a typedef of the pointer type.  For instance, considering:

	  type String_Access is access String;
	  S1 : String_Access := null;

   To the debugger, S1 is defined as a typedef of type String.  But
   to the user, it is a pointer.  So if the user tries to print S1,
   we should not dereference the array, but print the array address
   instead.

   If we didn't preserve the typedef layer, we would lose the fact that
   the type is to be presented as a pointer (needs de-reference before
   being printed).  And we would also use the source-level type name.  */

struct type *
ada_to_fixed_type (struct type *type, const gdb_byte *valaddr,
		   CORE_ADDR address, struct value *dval, int check_tag)

{
  struct type *fixed_type =
    ada_to_fixed_type_1 (type, valaddr, address, dval, check_tag);

  /*  If TYPE is a typedef and its target type is the same as the FIXED_TYPE,
      then preserve the typedef layer.

      Implementation note: We can only check the main-type portion of
      the TYPE and FIXED_TYPE, because eliminating the typedef layer
      from TYPE now returns a type that has the same instance flags
      as TYPE.  For instance, if TYPE is a "typedef const", and its
      target type is a "struct", then the typedef elimination will return
      a "const" version of the target type.  See check_typedef for more
      details about how the typedef layer elimination is done.

      brobecker/2010-11-19: It seems to me that the only case where it is
      useful to preserve the typedef layer is when dealing with fat pointers.
      Perhaps, we could add a check for that and preserve the typedef layer
      only in that situation.  But this seems unnecessary so far, probably
      because we call check_typedef/ada_check_typedef pretty much everywhere.
      */
  if (type->code () == TYPE_CODE_TYPEDEF
      && (TYPE_MAIN_TYPE (ada_typedef_target_type (type))
	  == TYPE_MAIN_TYPE (fixed_type)))
    return type;

  return fixed_type;
}

/* A standard (static-sized) type corresponding as well as possible to
   TYPE0, but based on no runtime data.  */

static struct type *
to_static_fixed_type (struct type *type0)
{
  struct type *type;

  if (type0 == NULL)
    return NULL;

  if (type0->is_fixed_instance ())
    return type0;

  type0 = ada_check_typedef (type0);

  switch (type0->code ())
    {
    default:
      return type0;
    case TYPE_CODE_STRUCT:
      type = dynamic_template_type (type0);
      if (type != NULL)
	return template_to_static_fixed_type (type);
      else
	return template_to_static_fixed_type (type0);
    case TYPE_CODE_UNION:
      type = ada_find_parallel_type (type0, "___XVU");
      if (type != NULL)
	return template_to_static_fixed_type (type);
      else
	return template_to_static_fixed_type (type0);
    }
}

/* A static approximation of TYPE with all type wrappers removed.  */

static struct type *
static_unwrap_type (struct type *type)
{
  if (ada_is_aligner_type (type))
    {
      struct type *type1 = ada_check_typedef (type)->field (0).type ();
      if (ada_type_name (type1) == NULL)
	type1->set_name (ada_type_name (type));

      return static_unwrap_type (type1);
    }
  else
    {
      struct type *raw_real_type = ada_get_base_type (type);

      if (raw_real_type == type)
	return type;
      else
	return to_static_fixed_type (raw_real_type);
    }
}

/* In some cases, incomplete and private types require
   cross-references that are not resolved as records (for example,
      type Foo;
      type FooP is access Foo;
      V: FooP;
      type Foo is array ...;
   ).  In these cases, since there is no mechanism for producing
   cross-references to such types, we instead substitute for FooP a
   stub enumeration type that is nowhere resolved, and whose tag is
   the name of the actual type.  Call these types "non-record stubs".  */

/* A type equivalent to TYPE that is not a non-record stub, if one
   exists, otherwise TYPE.  */

struct type *
ada_check_typedef (struct type *type)
{
  if (type == NULL)
    return NULL;

  /* If our type is an access to an unconstrained array, which is encoded
     as a TYPE_CODE_TYPEDEF of a fat pointer, then we're done.
     We don't want to strip the TYPE_CODE_TYPDEF layer, because this is
     what allows us to distinguish between fat pointers that represent
     array types, and fat pointers that represent array access types
     (in both cases, the compiler implements them as fat pointers).  */
  if (ada_is_access_to_unconstrained_array (type))
    return type;

  type = check_typedef (type);
  if (type == NULL || type->code () != TYPE_CODE_ENUM
      || !type->is_stub ()
      || type->name () == NULL)
    return type;
  else
    {
      const char *name = type->name ();
      struct type *type1 = ada_find_any_type (name);

      if (type1 == NULL)
	return type;

      /* TYPE1 might itself be a TYPE_CODE_TYPEDEF (this can happen with
	 stubs pointing to arrays, as we don't create symbols for array
	 types, only for the typedef-to-array types).  If that's the case,
	 strip the typedef layer.  */
      if (type1->code () == TYPE_CODE_TYPEDEF)
	type1 = ada_check_typedef (type1);

      return type1;
    }
}

/* A value representing the data at VALADDR/ADDRESS as described by
   type TYPE0, but with a standard (static-sized) type that correctly
   describes it.  If VAL0 is not NULL and TYPE0 already is a standard
   type, then return VAL0 [this feature is simply to avoid redundant
   creation of struct values].  */

static struct value *
ada_to_fixed_value_create (struct type *type0, CORE_ADDR address,
			   struct value *val0)
{
  struct type *type = ada_to_fixed_type (type0, 0, address, NULL, 1);

  if (type == type0 && val0 != NULL)
    return val0;

  if (val0->lval () != lval_memory)
    {
      /* Our value does not live in memory; it could be a convenience
	 variable, for instance.  Create a not_lval value using val0's
	 contents.  */
      return value_from_contents (type, val0->contents ().data ());
    }

  return value_from_contents_and_address (type, 0, address);
}

/* A value representing VAL, but with a standard (static-sized) type
   that correctly describes it.  Does not necessarily create a new
   value.  */

struct value *
ada_to_fixed_value (struct value *val)
{
  val = unwrap_value (val);
  val = ada_to_fixed_value_create (val->type (), val->address (), val);
  return val;
}


/* Attributes */

/* Evaluate the 'POS attribute applied to ARG.  */

static LONGEST
pos_atr (struct value *arg)
{
  struct value *val = coerce_ref (arg);
  struct type *type = val->type ();

  if (!discrete_type_p (type))
    error (_("'POS only defined on discrete types"));

  std::optional<LONGEST> result = discrete_position (type, value_as_long (val));
  if (!result.has_value ())
    error (_("enumeration value is invalid: can't find 'POS"));

  return *result;
}

struct value *
ada_pos_atr (struct type *expect_type,
	     struct expression *exp,
	     enum noside noside, enum exp_opcode op,
	     struct value *arg)
{
  struct type *type = builtin_type (exp->gdbarch)->builtin_int;
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (type, not_lval);
  return value_from_longest (type, pos_atr (arg));
}

/* Evaluate the TYPE'VAL attribute applied to ARG.  */

static struct value *
val_atr (struct type *type, LONGEST val)
{
  gdb_assert (discrete_type_p (type));
  if (type->code () == TYPE_CODE_RANGE)
    type = type->target_type ();
  if (type->code () == TYPE_CODE_ENUM)
    {
      if (val < 0 || val >= type->num_fields ())
	error (_("argument to 'VAL out of range"));
      val = type->field (val).loc_enumval ();
    }
  return value_from_longest (type, val);
}

struct value *
ada_val_atr (struct expression *exp, enum noside noside, struct type *type,
	     struct value *arg)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (type, not_lval);

  if (!discrete_type_p (type))
    error (_("'VAL only defined on discrete types"));
  if (!integer_type_p (arg->type ()))
    error (_("'VAL requires integral argument"));

  return val_atr (type, value_as_long (arg));
}

/* Implementation of the enum_rep attribute.  */
struct value *
ada_atr_enum_rep (struct expression *exp, enum noside noside, struct type *type,
		  struct value *arg)
{
  struct type *inttype = builtin_type (exp->gdbarch)->builtin_int;
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (inttype, not_lval);

  if (type->code () == TYPE_CODE_RANGE)
    type = type->target_type ();
  if (type->code () != TYPE_CODE_ENUM)
    error (_("'Enum_Rep only defined on enum types"));
  if (!types_equal (type, arg->type ()))
    error (_("'Enum_Rep requires argument to have same type as enum"));

  return value_cast (inttype, arg);
}

/* Implementation of the enum_val attribute.  */
struct value *
ada_atr_enum_val (struct expression *exp, enum noside noside, struct type *type,
		  struct value *arg)
{
  struct type *original_type = type;
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (original_type, not_lval);

  if (type->code () == TYPE_CODE_RANGE)
    type = type->target_type ();
  if (type->code () != TYPE_CODE_ENUM)
    error (_("'Enum_Val only defined on enum types"));
  if (!integer_type_p (arg->type ()))
    error (_("'Enum_Val requires integral argument"));

  LONGEST value = value_as_long (arg);
  for (int i = 0; i < type->num_fields (); ++i)
    {
      if (type->field (i).loc_enumval () == value)
	return value_from_longest (original_type, value);
    }

  error (_("value %s not found in enum"), plongest (value));
}



				/* Evaluation */

/* True if TYPE appears to be an Ada character type.
   [At the moment, this is true only for Character and Wide_Character;
   It is a heuristic test that could stand improvement].  */

bool
ada_is_character_type (struct type *type)
{
  const char *name;

  /* If the type code says it's a character, then assume it really is,
     and don't check any further.  */
  if (type->code () == TYPE_CODE_CHAR)
    return true;
  
  /* Otherwise, assume it's a character type iff it is a discrete type
     with a known character type name.  */
  name = ada_type_name (type);
  return (name != NULL
	  && (type->code () == TYPE_CODE_INT
	      || type->code () == TYPE_CODE_RANGE)
	  && (strcmp (name, "character") == 0
	      || strcmp (name, "wide_character") == 0
	      || strcmp (name, "wide_wide_character") == 0
	      || strcmp (name, "unsigned char") == 0));
}

/* True if TYPE appears to be an Ada string type.  */

bool
ada_is_string_type (struct type *type)
{
  type = ada_check_typedef (type);
  if (type != NULL
      && type->code () != TYPE_CODE_PTR
      && (ada_is_simple_array_type (type)
	  || ada_is_array_descriptor_type (type))
      && ada_array_arity (type) == 1)
    {
      struct type *elttype = ada_array_element_type (type, 1);

      return ada_is_character_type (elttype);
    }
  else
    return false;
}

/* The compiler sometimes provides a parallel XVS type for a given
   PAD type.  Normally, it is safe to follow the PAD type directly,
   but older versions of the compiler have a bug that causes the offset
   of its "F" field to be wrong.  Following that field in that case
   would lead to incorrect results, but this can be worked around
   by ignoring the PAD type and using the associated XVS type instead.

   Set to True if the debugger should trust the contents of PAD types.
   Otherwise, ignore the PAD type if there is a parallel XVS type.  */
static bool trust_pad_over_xvs = true;

/* True if TYPE is a struct type introduced by the compiler to force the
   alignment of a value.  Such types have a single field with a
   distinctive name.  */

int
ada_is_aligner_type (struct type *type)
{
  type = ada_check_typedef (type);

  if (!trust_pad_over_xvs && ada_find_parallel_type (type, "___XVS") != NULL)
    return 0;

  return (type->code () == TYPE_CODE_STRUCT
	  && type->num_fields () == 1
	  && strcmp (type->field (0).name (), "F") == 0);
}

/* If there is an ___XVS-convention type parallel to SUBTYPE, return
   the parallel type.  */

struct type *
ada_get_base_type (struct type *raw_type)
{
  struct type *real_type_namer;
  struct type *raw_real_type;

  if (raw_type == NULL || raw_type->code () != TYPE_CODE_STRUCT)
    return raw_type;

  if (ada_is_aligner_type (raw_type))
    /* The encoding specifies that we should always use the aligner type.
       So, even if this aligner type has an associated XVS type, we should
       simply ignore it.

       According to the compiler gurus, an XVS type parallel to an aligner
       type may exist because of a stabs limitation.  In stabs, aligner
       types are empty because the field has a variable-sized type, and
       thus cannot actually be used as an aligner type.  As a result,
       we need the associated parallel XVS type to decode the type.
       Since the policy in the compiler is to not change the internal
       representation based on the debugging info format, we sometimes
       end up having a redundant XVS type parallel to the aligner type.  */
    return raw_type;

  real_type_namer = ada_find_parallel_type (raw_type, "___XVS");
  if (real_type_namer == NULL
      || real_type_namer->code () != TYPE_CODE_STRUCT
      || real_type_namer->num_fields () != 1)
    return raw_type;

  if (real_type_namer->field (0).type ()->code () != TYPE_CODE_REF)
    {
      /* This is an older encoding form where the base type needs to be
	 looked up by name.  We prefer the newer encoding because it is
	 more efficient.  */
      raw_real_type = ada_find_any_type (real_type_namer->field (0).name ());
      if (raw_real_type == NULL)
	return raw_type;
      else
	return raw_real_type;
    }

  /* The field in our XVS type is a reference to the base type.  */
  return real_type_namer->field (0).type ()->target_type ();
}

/* The type of value designated by TYPE, with all aligners removed.  */

struct type *
ada_aligned_type (struct type *type)
{
  if (ada_is_aligner_type (type))
    return ada_aligned_type (type->field (0).type ());
  else
    return ada_get_base_type (type);
}


/* The address of the aligned value in an object at address VALADDR
   having type TYPE.  Assumes ada_is_aligner_type (TYPE).  */

const gdb_byte *
ada_aligned_value_addr (struct type *type, const gdb_byte *valaddr)
{
  if (ada_is_aligner_type (type))
    return ada_aligned_value_addr
      (type->field (0).type (),
       valaddr + type->field (0).loc_bitpos () / TARGET_CHAR_BIT);
  else
    return valaddr;
}



/* The printed representation of an enumeration literal with encoded
   name NAME.  The value is good to the next call of ada_enum_name.  */
const char *
ada_enum_name (const char *name)
{
  static std::string storage;
  const char *tmp;

  /* First, unqualify the enumeration name:
     1. Search for the last '.' character.  If we find one, then skip
     all the preceding characters, the unqualified name starts
     right after that dot.
     2. Otherwise, we may be debugging on a target where the compiler
     translates dots into "__".  Search forward for double underscores,
     but stop searching when we hit an overloading suffix, which is
     of the form "__" followed by digits.  */

  tmp = strrchr (name, '.');
  if (tmp != NULL)
    name = tmp + 1;
  else
    {
      while ((tmp = strstr (name, "__")) != NULL)
	{
	  if (isdigit (tmp[2]))
	    break;
	  else
	    name = tmp + 2;
	}
    }

  if (name[0] == 'Q')
    {
      int v;

      if (name[1] == 'U' || name[1] == 'W')
	{
	  int offset = 2;
	  if (name[1] == 'W' && name[2] == 'W')
	    {
	      /* Also handle the QWW case.  */
	      ++offset;
	    }
	  if (sscanf (name + offset, "%x", &v) != 1)
	    return name;
	}
      else if (((name[1] >= '0' && name[1] <= '9')
		|| (name[1] >= 'a' && name[1] <= 'z'))
	       && name[2] == '\0')
	{
	  storage = string_printf ("'%c'", name[1]);
	  return storage.c_str ();
	}
      else
	return name;

      if (isascii (v) && isprint (v))
	storage = string_printf ("'%c'", v);
      else if (name[1] == 'U')
	storage = string_printf ("'[\"%02x\"]'", v);
      else if (name[2] != 'W')
	storage = string_printf ("'[\"%04x\"]'", v);
      else
	storage = string_printf ("'[\"%06x\"]'", v);

      return storage.c_str ();
    }
  else
    {
      tmp = strstr (name, "__");
      if (tmp == NULL)
	tmp = strstr (name, "$");
      if (tmp != NULL)
	{
	  storage = std::string (name, tmp - name);
	  return storage.c_str ();
	}

      return name;
    }
}

/* If TYPE is a dynamic type, return the base type.  Otherwise, if
   there is no parallel type, return nullptr.  */

static struct type *
find_base_type (struct type *type)
{
  struct type *raw_real_type
    = ada_check_typedef (ada_get_base_type (type));

  /* No parallel XVS or XVE type.  */
  if (type == raw_real_type
      && ada_find_parallel_type (type, "___XVE") == nullptr)
    return nullptr;

  return raw_real_type;
}

/* If VAL is wrapped in an aligner or subtype wrapper, return the
   value it wraps.  */

static struct value *
unwrap_value (struct value *val)
{
  struct type *type = ada_check_typedef (val->type ());

  if (ada_is_aligner_type (type))
    {
      struct value *v = ada_value_struct_elt (val, "F", 0);
      struct type *val_type = ada_check_typedef (v->type ());

      if (ada_type_name (val_type) == NULL)
	val_type->set_name (ada_type_name (type));

      return unwrap_value (v);
    }
  else
    {
      struct type *raw_real_type = find_base_type (type);
      if (raw_real_type == nullptr)
	return val;

      return
	coerce_unspec_val_to_type
	(val, ada_to_fixed_type (raw_real_type, 0,
				 val->address (),
				 NULL, 1));
    }
}

/* Given two array types T1 and T2, return nonzero iff both arrays
   contain the same number of elements.  */

static int
ada_same_array_size_p (struct type *t1, struct type *t2)
{
  LONGEST lo1, hi1, lo2, hi2;

  /* Get the array bounds in order to verify that the size of
     the two arrays match.  */
  if (!get_array_bounds (t1, &lo1, &hi1)
      || !get_array_bounds (t2, &lo2, &hi2))
    error (_("unable to determine array bounds"));

  /* To make things easier for size comparison, normalize a bit
     the case of empty arrays by making sure that the difference
     between upper bound and lower bound is always -1.  */
  if (lo1 > hi1)
    hi1 = lo1 - 1;
  if (lo2 > hi2)
    hi2 = lo2 - 1;

  return (hi1 - lo1 == hi2 - lo2);
}

/* Assuming that VAL is an array of integrals, and TYPE represents
   an array with the same number of elements, but with wider integral
   elements, return an array "casted" to TYPE.  In practice, this
   means that the returned array is built by casting each element
   of the original array into TYPE's (wider) element type.  */

static struct value *
ada_promote_array_of_integrals (struct type *type, struct value *val)
{
  struct type *elt_type = type->target_type ();
  LONGEST lo, hi;
  LONGEST i;

  /* Verify that both val and type are arrays of scalars, and
     that the size of val's elements is smaller than the size
     of type's element.  */
  gdb_assert (type->code () == TYPE_CODE_ARRAY);
  gdb_assert (is_integral_type (type->target_type ()));
  gdb_assert (val->type ()->code () == TYPE_CODE_ARRAY);
  gdb_assert (is_integral_type (val->type ()->target_type ()));
  gdb_assert (type->target_type ()->length ()
	      > val->type ()->target_type ()->length ());

  if (!get_array_bounds (type, &lo, &hi))
    error (_("unable to determine array bounds"));

  value *res = value::allocate (type);
  gdb::array_view<gdb_byte> res_contents = res->contents_writeable ();

  /* Promote each array element.  */
  for (i = 0; i < hi - lo + 1; i++)
    {
      struct value *elt = value_cast (elt_type, value_subscript (val, lo + i));
      int elt_len = elt_type->length ();

      copy (elt->contents_all (), res_contents.slice (elt_len * i, elt_len));
    }

  return res;
}

/* Coerce VAL as necessary for assignment to an lval of type TYPE, and
   return the converted value.  */

static struct value *
coerce_for_assign (struct type *type, struct value *val)
{
  struct type *type2 = val->type ();

  if (type == type2)
    return val;

  type2 = ada_check_typedef (type2);
  type = ada_check_typedef (type);

  if (type2->code () == TYPE_CODE_PTR
      && type->code () == TYPE_CODE_ARRAY)
    {
      val = ada_value_ind (val);
      type2 = val->type ();
    }

  if (type2->code () == TYPE_CODE_ARRAY
      && type->code () == TYPE_CODE_ARRAY)
    {
      if (!ada_same_array_size_p (type, type2))
	error (_("cannot assign arrays of different length"));

      if (is_integral_type (type->target_type ())
	  && is_integral_type (type2->target_type ())
	  && type2->target_type ()->length () < type->target_type ()->length ())
	{
	  /* Allow implicit promotion of the array elements to
	     a wider type.  */
	  return ada_promote_array_of_integrals (type, val);
	}

      if (type2->target_type ()->length () != type->target_type ()->length ())
	error (_("Incompatible types in assignment"));
      val->deprecated_set_type (type);
    }
  return val;
}

static struct value *
ada_value_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct type *type1, *type2;

  arg1 = coerce_ref (arg1);
  arg2 = coerce_ref (arg2);
  type1 = get_base_type (ada_check_typedef (arg1->type ()));
  type2 = get_base_type (ada_check_typedef (arg2->type ()));

  if (type1->code () != TYPE_CODE_INT
      || type2->code () != TYPE_CODE_INT)
    return value_binop (arg1, arg2, op);

  switch (op)
    {
    case BINOP_MOD:
    case BINOP_DIV:
    case BINOP_REM:
      break;
    default:
      return value_binop (arg1, arg2, op);
    }

  gdb_mpz v2 = value_as_mpz (arg2);
  if (v2.sgn () == 0)
    {
      const char *name;
      if (op == BINOP_MOD)
	name = "mod";
      else if (op == BINOP_DIV)
	name = "/";
      else
	{
	  gdb_assert (op == BINOP_REM);
	  name = "rem";
	}

      error (_("second operand of %s must not be zero."), name);
    }

  if (type1->is_unsigned () || op == BINOP_MOD)
    return value_binop (arg1, arg2, op);

  gdb_mpz v1 = value_as_mpz (arg1);
  gdb_mpz v;
  switch (op)
    {
    case BINOP_DIV:
      v = v1 / v2;
      break;
    case BINOP_REM:
      v = v1 % v2;
      if (v * v1 < 0)
	v -= v2;
      break;
    default:
      /* Should not reach this point.  */
      gdb_assert_not_reached ("invalid operator");
    }

  return value_from_mpz (type1, v);
}

static int
ada_value_equal (struct value *arg1, struct value *arg2)
{
  if (ada_is_direct_array_type (arg1->type ())
      || ada_is_direct_array_type (arg2->type ()))
    {
      struct type *arg1_type, *arg2_type;

      /* Automatically dereference any array reference before
	 we attempt to perform the comparison.  */
      arg1 = ada_coerce_ref (arg1);
      arg2 = ada_coerce_ref (arg2);

      arg1 = ada_coerce_to_simple_array (arg1);
      arg2 = ada_coerce_to_simple_array (arg2);

      arg1_type = ada_check_typedef (arg1->type ());
      arg2_type = ada_check_typedef (arg2->type ());

      if (arg1_type->code () != TYPE_CODE_ARRAY
	  || arg2_type->code () != TYPE_CODE_ARRAY)
	error (_("Attempt to compare array with non-array"));
      /* FIXME: The following works only for types whose
	 representations use all bits (no padding or undefined bits)
	 and do not have user-defined equality.  */
      return (arg1_type->length () == arg2_type->length ()
	      && memcmp (arg1->contents ().data (),
			 arg2->contents ().data (),
			 arg1_type->length ()) == 0);
    }
  return value_equal (arg1, arg2);
}

namespace expr
{

bool
check_objfile (const std::unique_ptr<ada_component> &comp,
	       struct objfile *objfile)
{
  return comp->uses_objfile (objfile);
}

/* Assign the result of evaluating ARG starting at *POS to the INDEXth
   component of LHS (a simple array or a record).  Does not modify the
   inferior's memory, nor does it modify LHS (unless LHS ==
   CONTAINER).  */

static void
assign_component (struct value *container, struct value *lhs, LONGEST index,
		  struct expression *exp, operation_up &arg)
{
  scoped_value_mark mark;

  struct value *elt;
  struct type *lhs_type = check_typedef (lhs->type ());

  if (lhs_type->code () == TYPE_CODE_ARRAY)
    {
      struct type *index_type = builtin_type (exp->gdbarch)->builtin_int;
      struct value *index_val = value_from_longest (index_type, index);

      elt = unwrap_value (ada_value_subscript (lhs, 1, &index_val));
    }
  else
    {
      elt = ada_index_struct_field (index, lhs, 0, lhs->type ());
      elt = ada_to_fixed_value (elt);
    }

  ada_aggregate_operation *ag_op
    = dynamic_cast<ada_aggregate_operation *> (arg.get ());
  if (ag_op != nullptr)
    ag_op->assign_aggregate (container, elt, exp);
  else
    value_assign_to_component (container, elt,
			       arg->evaluate (nullptr, exp,
					      EVAL_NORMAL));
}

bool
ada_aggregate_component::uses_objfile (struct objfile *objfile)
{
  for (const auto &item : m_components)
    if (item->uses_objfile (objfile))
      return true;
  return false;
}

void
ada_aggregate_component::dump (ui_file *stream, int depth)
{
  gdb_printf (stream, _("%*sAggregate\n"), depth, "");
  for (const auto &item : m_components)
    item->dump (stream, depth + 1);
}

void
ada_aggregate_component::assign (struct value *container,
				 struct value *lhs, struct expression *exp,
				 std::vector<LONGEST> &indices,
				 LONGEST low, LONGEST high)
{
  for (auto &item : m_components)
    item->assign (container, lhs, exp, indices, low, high);
}

/* See ada-exp.h.  */

value *
ada_aggregate_operation::assign_aggregate (struct value *container,
					   struct value *lhs,
					   struct expression *exp)
{
  struct type *lhs_type;
  LONGEST low_index, high_index;

  container = ada_coerce_ref (container);
  if (ada_is_direct_array_type (container->type ()))
    container = ada_coerce_to_simple_array (container);
  lhs = ada_coerce_ref (lhs);
  if (!lhs->deprecated_modifiable ())
    error (_("Left operand of assignment is not a modifiable lvalue."));

  lhs_type = check_typedef (lhs->type ());
  if (ada_is_direct_array_type (lhs_type))
    {
      lhs = ada_coerce_to_simple_array (lhs);
      lhs_type = check_typedef (lhs->type ());
      low_index = lhs_type->bounds ()->low.const_val ();
      high_index = lhs_type->bounds ()->high.const_val ();
    }
  else if (lhs_type->code () == TYPE_CODE_STRUCT)
    {
      low_index = 0;
      high_index = num_visible_fields (lhs_type) - 1;
    }
  else
    error (_("Left-hand side must be array or record."));

  std::vector<LONGEST> indices (4);
  indices[0] = indices[1] = low_index - 1;
  indices[2] = indices[3] = high_index + 1;

  std::get<0> (m_storage)->assign (container, lhs, exp, indices,
				   low_index, high_index);

  return container;
}

bool
ada_positional_component::uses_objfile (struct objfile *objfile)
{
  return m_op->uses_objfile (objfile);
}

void
ada_positional_component::dump (ui_file *stream, int depth)
{
  gdb_printf (stream, _("%*sPositional, index = %d\n"),
	      depth, "", m_index);
  m_op->dump (stream, depth + 1);
}

/* Assign into the component of LHS indexed by the OP_POSITIONAL
   construct, given that the positions are relative to lower bound
   LOW, where HIGH is the upper bound.  Record the position in
   INDICES.  CONTAINER is as for assign_aggregate.  */
void
ada_positional_component::assign (struct value *container,
				  struct value *lhs, struct expression *exp,
				  std::vector<LONGEST> &indices,
				  LONGEST low, LONGEST high)
{
  LONGEST ind = m_index + low;

  if (ind - 1 == high)
    warning (_("Extra components in aggregate ignored."));
  if (ind <= high)
    {
      add_component_interval (ind, ind, indices);
      assign_component (container, lhs, ind, exp, m_op);
    }
}

bool
ada_discrete_range_association::uses_objfile (struct objfile *objfile)
{
  return m_low->uses_objfile (objfile) || m_high->uses_objfile (objfile);
}

void
ada_discrete_range_association::dump (ui_file *stream, int depth)
{
  gdb_printf (stream, _("%*sDiscrete range:\n"), depth, "");
  m_low->dump (stream, depth + 1);
  m_high->dump (stream, depth + 1);
}

void
ada_discrete_range_association::assign (struct value *container,
					struct value *lhs,
					struct expression *exp,
					std::vector<LONGEST> &indices,
					LONGEST low, LONGEST high,
					operation_up &op)
{
  LONGEST lower = value_as_long (m_low->evaluate (nullptr, exp, EVAL_NORMAL));
  LONGEST upper = value_as_long (m_high->evaluate (nullptr, exp, EVAL_NORMAL));

  if (lower <= upper && (lower < low || upper > high))
    error (_("Index in component association out of bounds."));

  add_component_interval (lower, upper, indices);
  while (lower <= upper)
    {
      assign_component (container, lhs, lower, exp, op);
      lower += 1;
    }
}

bool
ada_name_association::uses_objfile (struct objfile *objfile)
{
  return m_val->uses_objfile (objfile);
}

void
ada_name_association::dump (ui_file *stream, int depth)
{
  gdb_printf (stream, _("%*sName:\n"), depth, "");
  m_val->dump (stream, depth + 1);
}

void
ada_name_association::assign (struct value *container,
			      struct value *lhs,
			      struct expression *exp,
			      std::vector<LONGEST> &indices,
			      LONGEST low, LONGEST high,
			      operation_up &op)
{
  int index;

  if (ada_is_direct_array_type (lhs->type ()))
    index = longest_to_int (value_as_long (m_val->evaluate (nullptr, exp,
							    EVAL_NORMAL)));
  else
    {
      ada_string_operation *strop
	= dynamic_cast<ada_string_operation *> (m_val.get ());

      const char *name;
      if (strop != nullptr)
	name = strop->get_name ();
      else
	{
	  ada_var_value_operation *vvo
	    = dynamic_cast<ada_var_value_operation *> (m_val.get ());
	  if (vvo == nullptr)
	    error (_("Invalid record component association."));
	  name = vvo->get_symbol ()->natural_name ();
	  /* In this scenario, the user wrote (name => expr), but
	     write_name_assoc found some fully-qualified name and
	     substituted it.  This happens because, at parse time, the
	     meaning of the expression isn't known; but here we know
	     that just the base name was supplied and it refers to the
	     name of a field.  */
	  name = ada_unqualified_name (name);
	}

      index = 0;
      if (! find_struct_field (name, lhs->type (), 0,
			       NULL, NULL, NULL, NULL, &index))
	error (_("Unknown component name: %s."), name);
    }

  add_component_interval (index, index, indices);
  assign_component (container, lhs, index, exp, op);
}

bool
ada_choices_component::uses_objfile (struct objfile *objfile)
{
  if (m_op->uses_objfile (objfile))
    return true;
  for (const auto &item : m_assocs)
    if (item->uses_objfile (objfile))
      return true;
  return false;
}

void
ada_choices_component::dump (ui_file *stream, int depth)
{
  gdb_printf (stream, _("%*sChoices:\n"), depth, "");
  m_op->dump (stream, depth + 1);
  for (const auto &item : m_assocs)
    item->dump (stream, depth + 1);
}

/* Assign into the components of LHS indexed by the OP_CHOICES
   construct at *POS, updating *POS past the construct, given that
   the allowable indices are LOW..HIGH.  Record the indices assigned
   to in INDICES.  CONTAINER is as for assign_aggregate.  */
void
ada_choices_component::assign (struct value *container,
			       struct value *lhs, struct expression *exp,
			       std::vector<LONGEST> &indices,
			       LONGEST low, LONGEST high)
{
  for (auto &item : m_assocs)
    item->assign (container, lhs, exp, indices, low, high, m_op);
}

bool
ada_others_component::uses_objfile (struct objfile *objfile)
{
  return m_op->uses_objfile (objfile);
}

void
ada_others_component::dump (ui_file *stream, int depth)
{
  gdb_printf (stream, _("%*sOthers:\n"), depth, "");
  m_op->dump (stream, depth + 1);
}

/* Assign the value of the expression in the OP_OTHERS construct in
   EXP at *POS into the components of LHS indexed from LOW .. HIGH that
   have not been previously assigned.  The index intervals already assigned
   are in INDICES.  CONTAINER is as for assign_aggregate.  */
void
ada_others_component::assign (struct value *container,
			      struct value *lhs, struct expression *exp,
			      std::vector<LONGEST> &indices,
			      LONGEST low, LONGEST high)
{
  int num_indices = indices.size ();
  for (int i = 0; i < num_indices - 2; i += 2)
    {
      for (LONGEST ind = indices[i + 1] + 1; ind < indices[i + 2]; ind += 1)
	assign_component (container, lhs, ind, exp, m_op);
    }
}

struct value *
ada_assign_operation::evaluate (struct type *expect_type,
				struct expression *exp,
				enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  scoped_restore save_lhs = make_scoped_restore (&m_current, arg1);

  ada_aggregate_operation *ag_op
    = dynamic_cast<ada_aggregate_operation *> (std::get<1> (m_storage).get ());
  if (ag_op != nullptr)
    {
      if (noside != EVAL_NORMAL)
	return arg1;

      arg1 = ag_op->assign_aggregate (arg1, arg1, exp);
      return ada_value_assign (arg1, arg1);
    }
  /* Force the evaluation of the rhs ARG2 to the type of the lhs ARG1,
     except if the lhs of our assignment is a convenience variable.
     In the case of assigning to a convenience variable, the lhs
     should be exactly the result of the evaluation of the rhs.  */
  struct type *type = arg1->type ();
  if (arg1->lval () == lval_internalvar)
    type = NULL;
  value *arg2 = std::get<1> (m_storage)->evaluate (type, exp, noside);
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  if (arg1->lval () == lval_internalvar)
    {
      /* Nothing.  */
    }
  else
    arg2 = coerce_for_assign (arg1->type (), arg2);
  return ada_value_assign (arg1, arg2);
}

} /* namespace expr */

/* Add the interval [LOW .. HIGH] to the sorted set of intervals
   [ INDICES[0] .. INDICES[1] ],...  The resulting intervals do not
   overlap.  */
static void
add_component_interval (LONGEST low, LONGEST high, 
			std::vector<LONGEST> &indices)
{
  int i, j;

  int size = indices.size ();
  for (i = 0; i < size; i += 2) {
    if (high >= indices[i] && low <= indices[i + 1])
      {
	int kh;

	for (kh = i + 2; kh < size; kh += 2)
	  if (high < indices[kh])
	    break;
	if (low < indices[i])
	  indices[i] = low;
	indices[i + 1] = indices[kh - 1];
	if (high > indices[i + 1])
	  indices[i + 1] = high;
	memcpy (indices.data () + i + 2, indices.data () + kh, size - kh);
	indices.resize (kh - i - 2);
	return;
      }
    else if (high < indices[i])
      break;
  }
	
  indices.resize (indices.size () + 2);
  for (j = indices.size () - 1; j >= i + 2; j -= 1)
    indices[j] = indices[j - 2];
  indices[i] = low;
  indices[i + 1] = high;
}

/* Perform and Ada cast of ARG2 to type TYPE if the type of ARG2
   is different.  */

static struct value *
ada_value_cast (struct type *type, struct value *arg2)
{
  if (type == ada_check_typedef (arg2->type ()))
    return arg2;

  return value_cast (type, arg2);
}

/*  Evaluating Ada expressions, and printing their result.
    ------------------------------------------------------

    1. Introduction:
    ----------------

    We usually evaluate an Ada expression in order to print its value.
    We also evaluate an expression in order to print its type, which
    happens during the EVAL_AVOID_SIDE_EFFECTS phase of the evaluation,
    but we'll focus mostly on the EVAL_NORMAL phase.  In practice, the
    EVAL_AVOID_SIDE_EFFECTS phase allows us to simplify certain aspects of
    the evaluation compared to the EVAL_NORMAL, but is otherwise very
    similar.

    Evaluating expressions is a little more complicated for Ada entities
    than it is for entities in languages such as C.  The main reason for
    this is that Ada provides types whose definition might be dynamic.
    One example of such types is variant records.  Or another example
    would be an array whose bounds can only be known at run time.

    The following description is a general guide as to what should be
    done (and what should NOT be done) in order to evaluate an expression
    involving such types, and when.  This does not cover how the semantic
    information is encoded by GNAT as this is covered separatly.  For the
    document used as the reference for the GNAT encoding, see exp_dbug.ads
    in the GNAT sources.

    Ideally, we should embed each part of this description next to its
    associated code.  Unfortunately, the amount of code is so vast right
    now that it's hard to see whether the code handling a particular
    situation might be duplicated or not.  One day, when the code is
    cleaned up, this guide might become redundant with the comments
    inserted in the code, and we might want to remove it.

    2. ``Fixing'' an Entity, the Simple Case:
    -----------------------------------------

    When evaluating Ada expressions, the tricky issue is that they may
    reference entities whose type contents and size are not statically
    known.  Consider for instance a variant record:

       type Rec (Empty : Boolean := True) is record
	  case Empty is
	     when True => null;
	     when False => Value : Integer;
	  end case;
       end record;
       Yes : Rec := (Empty => False, Value => 1);
       No  : Rec := (empty => True);

    The size and contents of that record depends on the value of the
    discriminant (Rec.Empty).  At this point, neither the debugging
    information nor the associated type structure in GDB are able to
    express such dynamic types.  So what the debugger does is to create
    "fixed" versions of the type that applies to the specific object.
    We also informally refer to this operation as "fixing" an object,
    which means creating its associated fixed type.

    Example: when printing the value of variable "Yes" above, its fixed
    type would look like this:

       type Rec is record
	  Empty : Boolean;
	  Value : Integer;
       end record;

    On the other hand, if we printed the value of "No", its fixed type
    would become:

       type Rec is record
	  Empty : Boolean;
       end record;

    Things become a little more complicated when trying to fix an entity
    with a dynamic type that directly contains another dynamic type,
    such as an array of variant records, for instance.  There are
    two possible cases: Arrays, and records.

    3. ``Fixing'' Arrays:
    ---------------------

    The type structure in GDB describes an array in terms of its bounds,
    and the type of its elements.  By design, all elements in the array
    have the same type and we cannot represent an array of variant elements
    using the current type structure in GDB.  When fixing an array,
    we cannot fix the array element, as we would potentially need one
    fixed type per element of the array.  As a result, the best we can do
    when fixing an array is to produce an array whose bounds and size
    are correct (allowing us to read it from memory), but without having
    touched its element type.  Fixing each element will be done later,
    when (if) necessary.

    Arrays are a little simpler to handle than records, because the same
    amount of memory is allocated for each element of the array, even if
    the amount of space actually used by each element differs from element
    to element.  Consider for instance the following array of type Rec:

       type Rec_Array is array (1 .. 2) of Rec;

    The actual amount of memory occupied by each element might be different
    from element to element, depending on the value of their discriminant.
    But the amount of space reserved for each element in the array remains
    fixed regardless.  So we simply need to compute that size using
    the debugging information available, from which we can then determine
    the array size (we multiply the number of elements of the array by
    the size of each element).

    The simplest case is when we have an array of a constrained element
    type. For instance, consider the following type declarations:

	type Bounded_String (Max_Size : Integer) is
	   Length : Integer;
	   Buffer : String (1 .. Max_Size);
	end record;
	type Bounded_String_Array is array (1 ..2) of Bounded_String (80);

    In this case, the compiler describes the array as an array of
    variable-size elements (identified by its XVS suffix) for which
    the size can be read in the parallel XVZ variable.

    In the case of an array of an unconstrained element type, the compiler
    wraps the array element inside a private PAD type.  This type should not
    be shown to the user, and must be "unwrap"'ed before printing.  Note
    that we also use the adjective "aligner" in our code to designate
    these wrapper types.

    In some cases, the size allocated for each element is statically
    known.  In that case, the PAD type already has the correct size,
    and the array element should remain unfixed.

    But there are cases when this size is not statically known.
    For instance, assuming that "Five" is an integer variable:

	type Dynamic is array (1 .. Five) of Integer;
	type Wrapper (Has_Length : Boolean := False) is record
	   Data : Dynamic;
	   case Has_Length is
	      when True => Length : Integer;
	      when False => null;
	   end case;
	end record;
	type Wrapper_Array is array (1 .. 2) of Wrapper;

	Hello : Wrapper_Array := (others => (Has_Length => True,
					     Data => (others => 17),
					     Length => 1));


    The debugging info would describe variable Hello as being an
    array of a PAD type.  The size of that PAD type is not statically
    known, but can be determined using a parallel XVZ variable.
    In that case, a copy of the PAD type with the correct size should
    be used for the fixed array.

    3. ``Fixing'' record type objects:
    ----------------------------------

    Things are slightly different from arrays in the case of dynamic
    record types.  In this case, in order to compute the associated
    fixed type, we need to determine the size and offset of each of
    its components.  This, in turn, requires us to compute the fixed
    type of each of these components.

    Consider for instance the example:

	type Bounded_String (Max_Size : Natural) is record
	   Str : String (1 .. Max_Size);
	   Length : Natural;
	end record;
	My_String : Bounded_String (Max_Size => 10);

    In that case, the position of field "Length" depends on the size
    of field Str, which itself depends on the value of the Max_Size
    discriminant.  In order to fix the type of variable My_String,
    we need to fix the type of field Str.  Therefore, fixing a variant
    record requires us to fix each of its components.

    However, if a component does not have a dynamic size, the component
    should not be fixed.  In particular, fields that use a PAD type
    should not fixed.  Here is an example where this might happen
    (assuming type Rec above):

       type Container (Big : Boolean) is record
	  First : Rec;
	  After : Integer;
	  case Big is
	     when True => Another : Integer;
	     when False => null;
	  end case;
       end record;
       My_Container : Container := (Big => False,
				    First => (Empty => True),
				    After => 42);

    In that example, the compiler creates a PAD type for component First,
    whose size is constant, and then positions the component After just
    right after it.  The offset of component After is therefore constant
    in this case.

    The debugger computes the position of each field based on an algorithm
    that uses, among other things, the actual position and size of the field
    preceding it.  Let's now imagine that the user is trying to print
    the value of My_Container.  If the type fixing was recursive, we would
    end up computing the offset of field After based on the size of the
    fixed version of field First.  And since in our example First has
    only one actual field, the size of the fixed type is actually smaller
    than the amount of space allocated to that field, and thus we would
    compute the wrong offset of field After.

    To make things more complicated, we need to watch out for dynamic
    components of variant records (identified by the ___XVL suffix in
    the component name).  Even if the target type is a PAD type, the size
    of that type might not be statically known.  So the PAD type needs
    to be unwrapped and the resulting type needs to be fixed.  Otherwise,
    we might end up with the wrong size for our component.  This can be
    observed with the following type declarations:

	type Octal is new Integer range 0 .. 7;
	type Octal_Array is array (Positive range <>) of Octal;
	pragma Pack (Octal_Array);

	type Octal_Buffer (Size : Positive) is record
	   Buffer : Octal_Array (1 .. Size);
	   Length : Integer;
	end record;

    In that case, Buffer is a PAD type whose size is unset and needs
    to be computed by fixing the unwrapped type.

    4. When to ``Fix'' un-``Fixed'' sub-elements of an entity:
    ----------------------------------------------------------

    Lastly, when should the sub-elements of an entity that remained unfixed
    thus far, be actually fixed?

    The answer is: Only when referencing that element.  For instance
    when selecting one component of a record, this specific component
    should be fixed at that point in time.  Or when printing the value
    of a record, each component should be fixed before its value gets
    printed.  Similarly for arrays, the element of the array should be
    fixed when printing each element of the array, or when extracting
    one element out of that array.  On the other hand, fixing should
    not be performed on the elements when taking a slice of an array!

    Note that one of the side effects of miscomputing the offset and
    size of each field is that we end up also miscomputing the size
    of the containing type.  This can have adverse results when computing
    the value of an entity.  GDB fetches the value of an entity based
    on the size of its type, and thus a wrong size causes GDB to fetch
    the wrong amount of memory.  In the case where the computed size is
    too small, GDB fetches too little data to print the value of our
    entity.  Results in this case are unpredictable, as we usually read
    past the buffer containing the data =:-o.  */

/* A helper function for TERNOP_IN_RANGE.  */

static value *
eval_ternop_in_range (struct type *expect_type, struct expression *exp,
		      enum noside noside,
		      value *arg1, value *arg2, value *arg3)
{
  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg3);
  struct type *type = language_bool_type (exp->language_defn, exp->gdbarch);
  return
    value_from_longest (type,
			(value_less (arg1, arg3)
			 || value_equal (arg1, arg3))
			&& (value_less (arg2, arg1)
			    || value_equal (arg2, arg1)));
}

/* A helper function for UNOP_NEG.  */

value *
ada_unop_neg (struct type *expect_type,
	      struct expression *exp,
	      enum noside noside, enum exp_opcode op,
	      struct value *arg1)
{
  unop_promote (exp->language_defn, exp->gdbarch, &arg1);
  return value_neg (arg1);
}

/* A helper function for UNOP_IN_RANGE.  */

value *
ada_unop_in_range (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside, enum exp_opcode op,
		   struct value *arg1, struct type *type)
{
  struct value *arg2, *arg3;
  switch (type->code ())
    {
    default:
      lim_warning (_("Membership test incompletely implemented; "
		     "always returns true"));
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return value_from_longest (type, 1);

    case TYPE_CODE_RANGE:
      arg2 = value_from_longest (type,
				 type->bounds ()->low.const_val ());
      arg3 = value_from_longest (type,
				 type->bounds ()->high.const_val ());
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg3);
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return
	value_from_longest (type,
			    (value_less (arg1, arg3)
			     || value_equal (arg1, arg3))
			    && (value_less (arg2, arg1)
				|| value_equal (arg2, arg1)));
    }
}

/* A helper function for OP_ATR_TAG.  */

value *
ada_atr_tag (struct type *expect_type,
	     struct expression *exp,
	     enum noside noside, enum exp_opcode op,
	     struct value *arg1)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (ada_tag_type (arg1), not_lval);

  return ada_value_tag (arg1);
}

/* A helper function for OP_ATR_SIZE.  */

value *
ada_atr_size (struct type *expect_type,
	      struct expression *exp,
	      enum noside noside, enum exp_opcode op,
	      struct value *arg1)
{
  struct type *type = arg1->type ();

  /* If the argument is a reference, then dereference its type, since
     the user is really asking for the size of the actual object,
     not the size of the pointer.  */
  if (type->code () == TYPE_CODE_REF)
    type = type->target_type ();

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (builtin_type (exp->gdbarch)->builtin_int, not_lval);
  else
    return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
			       TARGET_CHAR_BIT * type->length ());
}

/* A helper function for UNOP_ABS.  */

value *
ada_abs (struct type *expect_type,
	 struct expression *exp,
	 enum noside noside, enum exp_opcode op,
	 struct value *arg1)
{
  unop_promote (exp->language_defn, exp->gdbarch, &arg1);
  if (value_less (arg1, value::zero (arg1->type (), not_lval)))
    return value_neg (arg1);
  else
    return arg1;
}

/* A helper function for BINOP_MUL.  */

value *
ada_mult_binop (struct type *expect_type,
		struct expression *exp,
		enum noside noside, enum exp_opcode op,
		struct value *arg1, struct value *arg2)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return value::zero (arg1->type (), not_lval);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return ada_value_binop (arg1, arg2, op);
    }
}

/* A helper function for BINOP_EQUAL and BINOP_NOTEQUAL.  */

value *
ada_equal_binop (struct type *expect_type,
		 struct expression *exp,
		 enum noside noside, enum exp_opcode op,
		 struct value *arg1, struct value *arg2)
{
  int tem;
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    tem = 0;
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      tem = ada_value_equal (arg1, arg2);
    }
  if (op == BINOP_NOTEQUAL)
    tem = !tem;
  struct type *type = language_bool_type (exp->language_defn, exp->gdbarch);
  return value_from_longest (type, tem);
}

/* A helper function for TERNOP_SLICE.  */

value *
ada_ternop_slice (struct expression *exp,
		  enum noside noside,
		  struct value *array, struct value *low_bound_val,
		  struct value *high_bound_val)
{
  LONGEST low_bound;
  LONGEST high_bound;

  low_bound_val = coerce_ref (low_bound_val);
  high_bound_val = coerce_ref (high_bound_val);
  low_bound = value_as_long (low_bound_val);
  high_bound = value_as_long (high_bound_val);

  /* If this is a reference to an aligner type, then remove all
     the aligners.  */
  if (array->type ()->code () == TYPE_CODE_REF
      && ada_is_aligner_type (array->type ()->target_type ()))
    array->type ()->set_target_type
      (ada_aligned_type (array->type ()->target_type ()));

  if (ada_is_any_packed_array_type (array->type ()))
    error (_("cannot slice a packed array"));

  /* If this is a reference to an array or an array lvalue,
     convert to a pointer.  */
  if (array->type ()->code () == TYPE_CODE_REF
      || (array->type ()->code () == TYPE_CODE_ARRAY
	  && array->lval () == lval_memory))
    array = value_addr (array);

  if (noside == EVAL_AVOID_SIDE_EFFECTS
      && ada_is_array_descriptor_type (ada_check_typedef
				       (array->type ())))
    return empty_array (ada_type_of_array (array, 0), low_bound,
			high_bound);

  array = ada_coerce_to_simple_array_ptr (array);

  /* If we have more than one level of pointer indirection,
     dereference the value until we get only one level.  */
  while (array->type ()->code () == TYPE_CODE_PTR
	 && (array->type ()->target_type ()->code ()
	     == TYPE_CODE_PTR))
    array = value_ind (array);

  /* Make sure we really do have an array type before going further,
     to avoid a SEGV when trying to get the index type or the target
     type later down the road if the debug info generated by
     the compiler is incorrect or incomplete.  */
  if (!ada_is_simple_array_type (array->type ()))
    error (_("cannot take slice of non-array"));

  if (ada_check_typedef (array->type ())->code ()
      == TYPE_CODE_PTR)
    {
      struct type *type0 = ada_check_typedef (array->type ());

      if (high_bound < low_bound || noside == EVAL_AVOID_SIDE_EFFECTS)
	return empty_array (type0->target_type (), low_bound, high_bound);
      else
	{
	  struct type *arr_type0 =
	    to_fixed_array_type (type0->target_type (), NULL, 1);

	  return ada_value_slice_from_ptr (array, arr_type0,
					   longest_to_int (low_bound),
					   longest_to_int (high_bound));
	}
    }
  else if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return array;
  else if (high_bound < low_bound)
    return empty_array (array->type (), low_bound, high_bound);
  else
    return ada_value_slice (array, longest_to_int (low_bound),
			    longest_to_int (high_bound));
}

/* A helper function for BINOP_IN_BOUNDS.  */

value *
ada_binop_in_bounds (struct expression *exp, enum noside noside,
		     struct value *arg1, struct value *arg2, int n)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value::zero (type, not_lval);
    }

  struct type *type = ada_index_type (arg2->type (), n, "range");
  if (!type)
    type = arg1->type ();

  value *arg3 = value_from_longest (type, ada_array_bound (arg2, n, 1));
  arg2 = value_from_longest (type, ada_array_bound (arg2, n, 0));

  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg3);
  type = language_bool_type (exp->language_defn, exp->gdbarch);
  return value_from_longest (type,
			     (value_less (arg1, arg3)
			      || value_equal (arg1, arg3))
			     && (value_less (arg2, arg1)
				 || value_equal (arg2, arg1)));
}

/* A helper function for some attribute operations.  */

static value *
ada_unop_atr (struct expression *exp, enum noside noside, enum exp_opcode op,
	      struct value *arg1, struct type *type_arg, int tem)
{
  const char *attr_name = nullptr;
  if (op == OP_ATR_FIRST)
    attr_name = "first";
  else if (op == OP_ATR_LAST)
    attr_name = "last";

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      if (type_arg == NULL)
	type_arg = arg1->type ();

      if (ada_is_constrained_packed_array_type (type_arg))
	type_arg = decode_constrained_packed_array_type (type_arg);

      if (!discrete_type_p (type_arg))
	{
	  switch (op)
	    {
	    default:          /* Should never happen.  */
	      error (_("unexpected attribute encountered"));
	    case OP_ATR_FIRST:
	    case OP_ATR_LAST:
	      type_arg = ada_index_type (type_arg, tem,
					 attr_name);
	      break;
	    case OP_ATR_LENGTH:
	      type_arg = builtin_type (exp->gdbarch)->builtin_int;
	      break;
	    }
	}

      return value::zero (type_arg, not_lval);
    }
  else if (type_arg == NULL)
    {
      arg1 = ada_coerce_ref (arg1);

      if (ada_is_constrained_packed_array_type (arg1->type ()))
	arg1 = ada_coerce_to_simple_array (arg1);

      struct type *type;
      if (op == OP_ATR_LENGTH)
	type = builtin_type (exp->gdbarch)->builtin_int;
      else
	{
	  type = ada_index_type (arg1->type (), tem,
				 attr_name);
	  if (type == NULL)
	    type = builtin_type (exp->gdbarch)->builtin_int;
	}

      switch (op)
	{
	default:          /* Should never happen.  */
	  error (_("unexpected attribute encountered"));
	case OP_ATR_FIRST:
	  return value_from_longest
	    (type, ada_array_bound (arg1, tem, 0));
	case OP_ATR_LAST:
	  return value_from_longest
	    (type, ada_array_bound (arg1, tem, 1));
	case OP_ATR_LENGTH:
	  return value_from_longest
	    (type, ada_array_length (arg1, tem));
	}
    }
  else if (discrete_type_p (type_arg))
    {
      struct type *range_type;
      const char *name = ada_type_name (type_arg);

      range_type = NULL;
      if (name != NULL && type_arg->code () != TYPE_CODE_ENUM)
	range_type = to_fixed_range_type (type_arg, NULL);
      if (range_type == NULL)
	range_type = type_arg;
      switch (op)
	{
	default:
	  error (_("unexpected attribute encountered"));
	case OP_ATR_FIRST:
	  return value_from_longest 
	    (range_type, ada_discrete_type_low_bound (range_type));
	case OP_ATR_LAST:
	  return value_from_longest
	    (range_type, ada_discrete_type_high_bound (range_type));
	case OP_ATR_LENGTH:
	  error (_("the 'length attribute applies only to array types"));
	}
    }
  else if (type_arg->code () == TYPE_CODE_FLT)
    error (_("unimplemented type attribute"));
  else
    {
      LONGEST low, high;

      if (ada_is_constrained_packed_array_type (type_arg))
	type_arg = decode_constrained_packed_array_type (type_arg);

      struct type *type;
      if (op == OP_ATR_LENGTH)
	type = builtin_type (exp->gdbarch)->builtin_int;
      else
	{
	  type = ada_index_type (type_arg, tem, attr_name);
	  if (type == NULL)
	    type = builtin_type (exp->gdbarch)->builtin_int;
	}

      switch (op)
	{
	default:
	  error (_("unexpected attribute encountered"));
	case OP_ATR_FIRST:
	  low = ada_array_bound_from_type (type_arg, tem, 0);
	  return value_from_longest (type, low);
	case OP_ATR_LAST:
	  high = ada_array_bound_from_type (type_arg, tem, 1);
	  return value_from_longest (type, high);
	case OP_ATR_LENGTH:
	  low = ada_array_bound_from_type (type_arg, tem, 0);
	  high = ada_array_bound_from_type (type_arg, tem, 1);
	  return value_from_longest (type, high - low + 1);
	}
    }
}

/* A helper function for OP_ATR_MIN and OP_ATR_MAX.  */

struct value *
ada_binop_minmax (struct type *expect_type,
		  struct expression *exp,
		  enum noside noside, enum exp_opcode op,
		  struct value *arg1, struct value *arg2)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (arg1->type (), not_lval);
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return value_binop (arg1, arg2, op);
    }
}

/* A helper function for BINOP_EXP.  */

struct value *
ada_binop_exp (struct type *expect_type,
	       struct expression *exp,
	       enum noside noside, enum exp_opcode op,
	       struct value *arg1, struct value *arg2)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (arg1->type (), not_lval);
  else
    {
      /* For integer exponentiation operations,
	 only promote the first argument.  */
      if (is_integral_type (arg2->type ()))
	unop_promote (exp->language_defn, exp->gdbarch, &arg1);
      else
	binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);

      return value_binop (arg1, arg2, op);
    }
}

namespace expr
{

/* See ada-exp.h.  */

operation_up
ada_resolvable::replace (operation_up &&owner,
			 struct expression *exp,
			 bool deprocedure_p,
			 bool parse_completion,
			 innermost_block_tracker *tracker,
			 struct type *context_type)
{
  if (resolve (exp, deprocedure_p, parse_completion, tracker, context_type))
    return (make_operation<ada_funcall_operation>
	    (std::move (owner),
	     std::vector<operation_up> ()));
  return std::move (owner);
}

/* Convert the character literal whose value would be VAL to the
   appropriate value of type TYPE, if there is a translation.
   Otherwise return VAL.  Hence, in an enumeration type ('A', 'B'),
   the literal 'A' (VAL == 65), returns 0.  */

static LONGEST
convert_char_literal (struct type *type, LONGEST val)
{
  char name[12];
  int f;

  if (type == NULL)
    return val;
  type = check_typedef (type);
  if (type->code () != TYPE_CODE_ENUM)
    return val;

  if ((val >= 'a' && val <= 'z') || (val >= '0' && val <= '9'))
    xsnprintf (name, sizeof (name), "Q%c", (int) val);
  else if (val >= 0 && val < 256)
    xsnprintf (name, sizeof (name), "QU%02x", (unsigned) val);
  else if (val >= 0 && val < 0x10000)
    xsnprintf (name, sizeof (name), "QW%04x", (unsigned) val);
  else
    xsnprintf (name, sizeof (name), "QWW%08lx", (unsigned long) val);
  size_t len = strlen (name);
  for (f = 0; f < type->num_fields (); f += 1)
    {
      /* Check the suffix because an enum constant in a package will
	 have a name like "pkg__QUxx".  This is safe enough because we
	 already have the correct type, and because mangling means
	 there can't be clashes.  */
      const char *ename = type->field (f).name ();
      size_t elen = strlen (ename);

      if (elen >= len && strcmp (name, ename + elen - len) == 0)
	return type->field (f).loc_enumval ();
    }
  return val;
}

value *
ada_char_operation::evaluate (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  value *result = long_const_operation::evaluate (expect_type, exp, noside);
  if (expect_type != nullptr)
    result = ada_value_cast (expect_type, result);
  return result;
}

/* See ada-exp.h.  */

operation_up
ada_char_operation::replace (operation_up &&owner,
			     struct expression *exp,
			     bool deprocedure_p,
			     bool parse_completion,
			     innermost_block_tracker *tracker,
			     struct type *context_type)
{
  operation_up result = std::move (owner);

  if (context_type != nullptr && context_type->code () == TYPE_CODE_ENUM)
    {
      LONGEST val = as_longest ();
      gdb_assert (result.get () == this);
      std::get<0> (m_storage) = context_type;
      std::get<1> (m_storage) = convert_char_literal (context_type, val);
    }

  return result;
}

value *
ada_wrapped_operation::evaluate (struct type *expect_type,
				 struct expression *exp,
				 enum noside noside)
{
  value *result = std::get<0> (m_storage)->evaluate (expect_type, exp, noside);
  if (noside == EVAL_NORMAL)
    result = unwrap_value (result);

  /* If evaluating an OP_FLOAT and an EXPECT_TYPE was provided,
     then we need to perform the conversion manually, because
     evaluate_subexp_standard doesn't do it.  This conversion is
     necessary in Ada because the different kinds of float/fixed
     types in Ada have different representations.

     Similarly, we need to perform the conversion from OP_LONG
     ourselves.  */
  if ((opcode () == OP_FLOAT || opcode () == OP_LONG) && expect_type != NULL)
    result = ada_value_cast (expect_type, result);

  return result;
}

void
ada_wrapped_operation::do_generate_ax (struct expression *exp,
				       struct agent_expr *ax,
				       struct axs_value *value,
				       struct type *cast_type)
{
  std::get<0> (m_storage)->generate_ax (exp, ax, value, cast_type);

  struct type *type = value->type;
  if (ada_is_aligner_type (type))
    error (_("Aligner types cannot be handled in agent expressions"));
  else if (find_base_type (type) != nullptr)
    error (_("Dynamic types cannot be handled in agent expressions"));
}

value *
ada_string_operation::evaluate (struct type *expect_type,
				struct expression *exp,
				enum noside noside)
{
  struct type *char_type;
  if (expect_type != nullptr && ada_is_string_type (expect_type))
    char_type = ada_array_element_type (expect_type, 1);
  else
    char_type = language_string_char_type (exp->language_defn, exp->gdbarch);

  const std::string &str = std::get<0> (m_storage);
  const char *encoding;
  switch (char_type->length ())
    {
    case 1:
      {
	/* Simply copy over the data -- this isn't perhaps strictly
	   correct according to the encodings, but it is gdb's
	   historical behavior.  */
	struct type *stringtype
	  = lookup_array_range_type (char_type, 1, str.length ());
	struct value *val = value::allocate (stringtype);
	memcpy (val->contents_raw ().data (), str.c_str (),
		str.length ());
	return val;
      }

    case 2:
      if (gdbarch_byte_order (exp->gdbarch) == BFD_ENDIAN_BIG)
	encoding = "UTF-16BE";
      else
	encoding = "UTF-16LE";
      break;

    case 4:
      if (gdbarch_byte_order (exp->gdbarch) == BFD_ENDIAN_BIG)
	encoding = "UTF-32BE";
      else
	encoding = "UTF-32LE";
      break;

    default:
      error (_("unexpected character type size %s"),
	     pulongest (char_type->length ()));
    }

  auto_obstack converted;
  convert_between_encodings (host_charset (), encoding,
			     (const gdb_byte *) str.c_str (),
			     str.length (), 1,
			     &converted, translit_none);

  struct type *stringtype
    = lookup_array_range_type (char_type, 1,
			       obstack_object_size (&converted)
			       / char_type->length ());
  struct value *val = value::allocate (stringtype);
  memcpy (val->contents_raw ().data (),
	  obstack_base (&converted),
	  obstack_object_size (&converted));
  return val;
}

value *
ada_concat_operation::evaluate (struct type *expect_type,
				struct expression *exp,
				enum noside noside)
{
  /* If one side is a literal, evaluate the other side first so that
     the expected type can be set properly.  */
  const operation_up &lhs_expr = std::get<0> (m_storage);
  const operation_up &rhs_expr = std::get<1> (m_storage);

  value *lhs, *rhs;
  if (dynamic_cast<ada_string_operation *> (lhs_expr.get ()) != nullptr)
    {
      rhs = rhs_expr->evaluate (nullptr, exp, noside);
      lhs = lhs_expr->evaluate (rhs->type (), exp, noside);
    }
  else if (dynamic_cast<ada_char_operation *> (lhs_expr.get ()) != nullptr)
    {
      rhs = rhs_expr->evaluate (nullptr, exp, noside);
      struct type *rhs_type = check_typedef (rhs->type ());
      struct type *elt_type = nullptr;
      if (rhs_type->code () == TYPE_CODE_ARRAY)
	elt_type = rhs_type->target_type ();
      lhs = lhs_expr->evaluate (elt_type, exp, noside);
    }
  else if (dynamic_cast<ada_string_operation *> (rhs_expr.get ()) != nullptr)
    {
      lhs = lhs_expr->evaluate (nullptr, exp, noside);
      rhs = rhs_expr->evaluate (lhs->type (), exp, noside);
    }
  else if (dynamic_cast<ada_char_operation *> (rhs_expr.get ()) != nullptr)
    {
      lhs = lhs_expr->evaluate (nullptr, exp, noside);
      struct type *lhs_type = check_typedef (lhs->type ());
      struct type *elt_type = nullptr;
      if (lhs_type->code () == TYPE_CODE_ARRAY)
	elt_type = lhs_type->target_type ();
      rhs = rhs_expr->evaluate (elt_type, exp, noside);
    }
  else
    return concat_operation::evaluate (expect_type, exp, noside);

  return value_concat (lhs, rhs);
}

value *
ada_qual_operation::evaluate (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  struct type *type = std::get<1> (m_storage);
  return std::get<0> (m_storage)->evaluate (type, exp, noside);
}

value *
ada_ternop_range_operation::evaluate (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside)
{
  value *arg0 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  value *arg1 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  value *arg2 = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
  return eval_ternop_in_range (expect_type, exp, noside, arg0, arg1, arg2);
}

value *
ada_binop_addsub_operation::evaluate (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside)
{
  value *arg1 = std::get<1> (m_storage)->evaluate_with_coercion (exp, noside);
  value *arg2 = std::get<2> (m_storage)->evaluate_with_coercion (exp, noside);

  auto do_op = [this] (LONGEST x, LONGEST y)
    {
      if (std::get<0> (m_storage) == BINOP_ADD)
	return x + y;
      return x - y;
    };

  if (arg1->type ()->code () == TYPE_CODE_PTR)
    return (value_from_longest
	    (arg1->type (),
	     do_op (value_as_long (arg1), value_as_long (arg2))));
  if (arg2->type ()->code () == TYPE_CODE_PTR)
    return (value_from_longest
	    (arg2->type (),
	     do_op (value_as_long (arg1), value_as_long (arg2))));
  /* Preserve the original type for use by the range case below.
     We cannot cast the result to a reference type, so if ARG1 is
     a reference type, find its underlying type.  */
  struct type *type = arg1->type ();
  while (type->code () == TYPE_CODE_REF)
    type = type->target_type ();
  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
  arg1 = value_binop (arg1, arg2, std::get<0> (m_storage));
  /* We need to special-case the result with a range.
     This is done for the benefit of "ptype".  gdb's Ada support
     historically used the LHS to set the result type here, so
     preserve this behavior.  */
  if (type->code () == TYPE_CODE_RANGE)
    arg1 = value_cast (type, arg1);
  return arg1;
}

value *
ada_unop_atr_operation::evaluate (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside)
{
  struct type *type_arg = nullptr;
  value *val = nullptr;

  if (std::get<0> (m_storage)->opcode () == OP_TYPE)
    {
      value *tem = std::get<0> (m_storage)->evaluate (nullptr, exp,
						      EVAL_AVOID_SIDE_EFFECTS);
      type_arg = tem->type ();
    }
  else
    val = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);

  return ada_unop_atr (exp, noside, std::get<1> (m_storage),
		       val, type_arg, std::get<2> (m_storage));
}

value *
ada_var_msym_value_operation::evaluate_for_cast (struct type *expect_type,
						 struct expression *exp,
						 enum noside noside)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (expect_type, not_lval);

  const bound_minimal_symbol &b = std::get<0> (m_storage);
  value *val = evaluate_var_msym_value (noside, b.objfile, b.minsym);

  val = ada_value_cast (expect_type, val);

  /* Follow the Ada language semantics that do not allow taking
     an address of the result of a cast (view conversion in Ada).  */
  if (val->lval () == lval_memory)
    {
      if (val->lazy ())
	val->fetch_lazy ();
      val->set_lval (not_lval);
    }
  return val;
}

value *
ada_var_value_operation::evaluate_for_cast (struct type *expect_type,
					    struct expression *exp,
					    enum noside noside)
{
  value *val = evaluate_var_value (noside,
				   std::get<0> (m_storage).block,
				   std::get<0> (m_storage).symbol);

  val = ada_value_cast (expect_type, val);

  /* Follow the Ada language semantics that do not allow taking
     an address of the result of a cast (view conversion in Ada).  */
  if (val->lval () == lval_memory)
    {
      if (val->lazy ())
	val->fetch_lazy ();
      val->set_lval (not_lval);
    }
  return val;
}

value *
ada_var_value_operation::evaluate (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside)
{
  symbol *sym = std::get<0> (m_storage).symbol;

  if (sym->domain () == UNDEF_DOMAIN)
    /* Only encountered when an unresolved symbol occurs in a
       context other than a function call, in which case, it is
       invalid.  */
    error (_("Unexpected unresolved symbol, %s, during evaluation"),
	   sym->print_name ());

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = static_unwrap_type (sym->type ());
      /* Check to see if this is a tagged type.  We also need to handle
	 the case where the type is a reference to a tagged type, but
	 we have to be careful to exclude pointers to tagged types.
	 The latter should be shown as usual (as a pointer), whereas
	 a reference should mostly be transparent to the user.  */
      if (ada_is_tagged_type (type, 0)
	  || (type->code () == TYPE_CODE_REF
	      && ada_is_tagged_type (type->target_type (), 0)))
	{
	  /* Tagged types are a little special in the fact that the real
	     type is dynamic and can only be determined by inspecting the
	     object's tag.  This means that we need to get the object's
	     value first (EVAL_NORMAL) and then extract the actual object
	     type from its tag.

	     Note that we cannot skip the final step where we extract
	     the object type from its tag, because the EVAL_NORMAL phase
	     results in dynamic components being resolved into fixed ones.
	     This can cause problems when trying to print the type
	     description of tagged types whose parent has a dynamic size:
	     We use the type name of the "_parent" component in order
	     to print the name of the ancestor type in the type description.
	     If that component had a dynamic size, the resolution into
	     a fixed type would result in the loss of that type name,
	     thus preventing us from printing the name of the ancestor
	     type in the type description.  */
	  value *arg1 = evaluate (nullptr, exp, EVAL_NORMAL);

	  if (type->code () != TYPE_CODE_REF)
	    {
	      struct type *actual_type;

	      actual_type = type_from_tag (ada_value_tag (arg1));
	      if (actual_type == NULL)
		/* If, for some reason, we were unable to determine
		   the actual type from the tag, then use the static
		   approximation that we just computed as a fallback.
		   This can happen if the debugging information is
		   incomplete, for instance.  */
		actual_type = type;
	      return value::zero (actual_type, not_lval);
	    }
	  else
	    {
	      /* In the case of a ref, ada_coerce_ref takes care
		 of determining the actual type.  But the evaluation
		 should return a ref as it should be valid to ask
		 for its address; so rebuild a ref after coerce.  */
	      arg1 = ada_coerce_ref (arg1);
	      return value_ref (arg1, TYPE_CODE_REF);
	    }
	}

      /* Records and unions for which GNAT encodings have been
	 generated need to be statically fixed as well.
	 Otherwise, non-static fixing produces a type where
	 all dynamic properties are removed, which prevents "ptype"
	 from being able to completely describe the type.
	 For instance, a case statement in a variant record would be
	 replaced by the relevant components based on the actual
	 value of the discriminants.  */
      if ((type->code () == TYPE_CODE_STRUCT
	   && dynamic_template_type (type) != NULL)
	  || (type->code () == TYPE_CODE_UNION
	      && ada_find_parallel_type (type, "___XVU") != NULL))
	return value::zero (to_static_fixed_type (type), not_lval);
    }

  value *arg1 = var_value_operation::evaluate (expect_type, exp, noside);
  return ada_to_fixed_value (arg1);
}

bool
ada_var_value_operation::resolve (struct expression *exp,
				  bool deprocedure_p,
				  bool parse_completion,
				  innermost_block_tracker *tracker,
				  struct type *context_type)
{
  symbol *sym = std::get<0> (m_storage).symbol;
  if (sym->domain () == UNDEF_DOMAIN)
    {
      block_symbol resolved
	= ada_resolve_variable (sym, std::get<0> (m_storage).block,
				context_type, parse_completion,
				deprocedure_p, tracker);
      std::get<0> (m_storage) = resolved;
    }

  if (deprocedure_p
      && (std::get<0> (m_storage).symbol->type ()->code ()
	  == TYPE_CODE_FUNC))
    return true;

  return false;
}

void
ada_var_value_operation::do_generate_ax (struct expression *exp,
					 struct agent_expr *ax,
					 struct axs_value *value,
					 struct type *cast_type)
{
  symbol *sym = std::get<0> (m_storage).symbol;

  if (sym->domain () == UNDEF_DOMAIN)
    error (_("Unexpected unresolved symbol, %s, during evaluation"),
	   sym->print_name ());

  struct type *type = static_unwrap_type (sym->type ());
  if (ada_is_tagged_type (type, 0)
      || (type->code () == TYPE_CODE_REF
	  && ada_is_tagged_type (type->target_type (), 0)))
    error (_("Tagged types cannot be handled in agent expressions"));

  if ((type->code () == TYPE_CODE_STRUCT
       && dynamic_template_type (type) != NULL)
      || (type->code () == TYPE_CODE_UNION
	  && ada_find_parallel_type (type, "___XVU") != NULL))
    error (_("Dynamic types cannot be handled in agent expressions"));

  var_value_operation::do_generate_ax (exp, ax, value, cast_type);
}

value *
ada_unop_ind_operation::evaluate (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (expect_type, exp, noside);

  struct type *type = ada_check_typedef (arg1->type ());
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      if (ada_is_array_descriptor_type (type))
	/* GDB allows dereferencing GNAT array descriptors.  */
	{
	  struct type *arrType = ada_type_of_array (arg1, 0);

	  if (arrType == NULL)
	    error (_("Attempt to dereference null array pointer."));
	  return value_at_lazy (arrType, 0);
	}
      else if (type->code () == TYPE_CODE_PTR
	       || type->code () == TYPE_CODE_REF
	       /* In C you can dereference an array to get the 1st elt.  */
	       || type->code () == TYPE_CODE_ARRAY)
	{
	  /* As mentioned in the OP_VAR_VALUE case, tagged types can
	     only be determined by inspecting the object's tag.
	     This means that we need to evaluate completely the
	     expression in order to get its type.  */

	  if ((type->code () == TYPE_CODE_REF
	       || type->code () == TYPE_CODE_PTR)
	      && ada_is_tagged_type (type->target_type (), 0))
	    {
	      arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp,
							EVAL_NORMAL);
	      type = ada_value_ind (arg1)->type ();
	    }
	  else
	    {
	      type = to_static_fixed_type
		(ada_aligned_type
		 (ada_check_typedef (type->target_type ())));
	    }
	  return value::zero (type, lval_memory);
	}
      else if (type->code () == TYPE_CODE_INT)
	{
	  /* GDB allows dereferencing an int.  */
	  if (expect_type == NULL)
	    return value::zero (builtin_type (exp->gdbarch)->builtin_int,
			       lval_memory);
	  else
	    {
	      expect_type =
		to_static_fixed_type (ada_aligned_type (expect_type));
	      return value::zero (expect_type, lval_memory);
	    }
	}
      else
	error (_("Attempt to take contents of a non-pointer value."));
    }
  arg1 = ada_coerce_ref (arg1);     /* FIXME: What is this for??  */
  type = ada_check_typedef (arg1->type ());

  if (type->code () == TYPE_CODE_INT)
    /* GDB allows dereferencing an int.  If we were given
       the expect_type, then use that as the target type.
       Otherwise, assume that the target type is an int.  */
    {
      if (expect_type != NULL)
	return ada_value_ind (value_cast (lookup_pointer_type (expect_type),
					  arg1));
      else
	return value_at_lazy (builtin_type (exp->gdbarch)->builtin_int,
			      (CORE_ADDR) value_as_address (arg1));
    }

  if (ada_is_array_descriptor_type (type))
    /* GDB allows dereferencing GNAT array descriptors.  */
    return ada_coerce_to_simple_array (arg1);
  else
    return ada_value_ind (arg1);
}

value *
ada_structop_operation::evaluate (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  const char *str = std::get<1> (m_storage).c_str ();
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type;
      struct type *type1 = arg1->type ();

      if (ada_is_tagged_type (type1, 1))
	{
	  type = ada_lookup_struct_elt_type (type1, str, 1, 1);

	  /* If the field is not found, check if it exists in the
	     extension of this object's type. This means that we
	     need to evaluate completely the expression.  */

	  if (type == NULL)
	    {
	      arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp,
							EVAL_NORMAL);
	      arg1 = ada_value_struct_elt (arg1, str, 0);
	      arg1 = unwrap_value (arg1);
	      type = ada_to_fixed_value (arg1)->type ();
	    }
	}
      else
	type = ada_lookup_struct_elt_type (type1, str, 1, 0);

      return value::zero (ada_aligned_type (type), lval_memory);
    }
  else
    {
      arg1 = ada_value_struct_elt (arg1, str, 0);
      arg1 = unwrap_value (arg1);
      return ada_to_fixed_value (arg1);
    }
}

value *
ada_funcall_operation::evaluate (struct type *expect_type,
				 struct expression *exp,
				 enum noside noside)
{
  const std::vector<operation_up> &args_up = std::get<1> (m_storage);
  int nargs = args_up.size ();
  std::vector<value *> argvec (nargs);
  operation_up &callee_op = std::get<0> (m_storage);

  ada_var_value_operation *avv
    = dynamic_cast<ada_var_value_operation *> (callee_op.get ());
  if (avv != nullptr
      && avv->get_symbol ()->domain () == UNDEF_DOMAIN)
    error (_("Unexpected unresolved symbol, %s, during evaluation"),
	   avv->get_symbol ()->print_name ());

  value *callee = callee_op->evaluate (nullptr, exp, noside);
  for (int i = 0; i < args_up.size (); ++i)
    argvec[i] = args_up[i]->evaluate (nullptr, exp, noside);

  if (ada_is_constrained_packed_array_type
      (desc_base_type (callee->type ())))
    callee = ada_coerce_to_simple_array (callee);
  else if (callee->type ()->code () == TYPE_CODE_ARRAY
	   && callee->type ()->field (0).bitsize () != 0)
    /* This is a packed array that has already been fixed, and
       therefore already coerced to a simple array.  Nothing further
       to do.  */
    ;
  else if (callee->type ()->code () == TYPE_CODE_REF)
    {
      /* Make sure we dereference references so that all the code below
	 feels like it's really handling the referenced value.  Wrapping
	 types (for alignment) may be there, so make sure we strip them as
	 well.  */
      callee = ada_to_fixed_value (coerce_ref (callee));
    }
  else if (callee->type ()->code () == TYPE_CODE_ARRAY
	   && callee->lval () == lval_memory)
    callee = value_addr (callee);

  struct type *type = ada_check_typedef (callee->type ());

  /* Ada allows us to implicitly dereference arrays when subscripting
     them.  So, if this is an array typedef (encoding use for array
     access types encoded as fat pointers), strip it now.  */
  if (type->code () == TYPE_CODE_TYPEDEF)
    type = ada_typedef_target_type (type);

  if (type->code () == TYPE_CODE_PTR)
    {
      switch (ada_check_typedef (type->target_type ())->code ())
	{
	case TYPE_CODE_FUNC:
	  type = ada_check_typedef (type->target_type ());
	  break;
	case TYPE_CODE_ARRAY:
	  break;
	case TYPE_CODE_STRUCT:
	  if (noside != EVAL_AVOID_SIDE_EFFECTS)
	    callee = ada_value_ind (callee);
	  type = ada_check_typedef (type->target_type ());
	  break;
	default:
	  error (_("cannot subscript or call something of type `%s'"),
		 ada_type_name (callee->type ()));
	  break;
	}
    }

  switch (type->code ())
    {
    case TYPE_CODE_FUNC:
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  if (type->target_type () == NULL)
	    error_call_unknown_return_type (NULL);
	  return value::allocate (type->target_type ());
	}
      return call_function_by_hand (callee, expect_type, argvec);
    case TYPE_CODE_INTERNAL_FUNCTION:
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	/* We don't know anything about what the internal
	   function might return, but we have to return
	   something.  */
	return value::zero (builtin_type (exp->gdbarch)->builtin_int,
			   not_lval);
      else
	return call_internal_function (exp->gdbarch, exp->language_defn,
				       callee, nargs,
				       argvec.data ());

    case TYPE_CODE_STRUCT:
      {
	int arity;

	arity = ada_array_arity (type);
	type = ada_array_element_type (type, nargs);
	if (type == NULL)
	  error (_("cannot subscript or call a record"));
	if (arity != nargs)
	  error (_("wrong number of subscripts; expecting %d"), arity);
	if (noside == EVAL_AVOID_SIDE_EFFECTS)
	  return value::zero (ada_aligned_type (type), lval_memory);
	return
	  unwrap_value (ada_value_subscript
			(callee, nargs, argvec.data ()));
      }
    case TYPE_CODE_ARRAY:
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  type = ada_array_element_type (type, nargs);
	  if (type == NULL)
	    error (_("element type of array unknown"));
	  else
	    return value::zero (ada_aligned_type (type), lval_memory);
	}
      return
	unwrap_value (ada_value_subscript
		      (ada_coerce_to_simple_array (callee),
		       nargs, argvec.data ()));
    case TYPE_CODE_PTR:     /* Pointer to array */
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  type = to_fixed_array_type (type->target_type (), NULL, 1);
	  type = ada_array_element_type (type, nargs);
	  if (type == NULL)
	    error (_("element type of array unknown"));
	  else
	    return value::zero (ada_aligned_type (type), lval_memory);
	}
      return
	unwrap_value (ada_value_ptr_subscript (callee, nargs,
					       argvec.data ()));

    default:
      error (_("Attempt to index or call something other than an "
	       "array or function"));
    }
}

bool
ada_funcall_operation::resolve (struct expression *exp,
				bool deprocedure_p,
				bool parse_completion,
				innermost_block_tracker *tracker,
				struct type *context_type)
{
  operation_up &callee_op = std::get<0> (m_storage);

  ada_var_value_operation *avv
    = dynamic_cast<ada_var_value_operation *> (callee_op.get ());
  if (avv == nullptr)
    return false;

  symbol *sym = avv->get_symbol ();
  if (sym->domain () != UNDEF_DOMAIN)
    return false;

  const std::vector<operation_up> &args_up = std::get<1> (m_storage);
  int nargs = args_up.size ();
  std::vector<value *> argvec (nargs);

  for (int i = 0; i < args_up.size (); ++i)
    argvec[i] = args_up[i]->evaluate (nullptr, exp, EVAL_AVOID_SIDE_EFFECTS);

  const block *block = avv->get_block ();
  block_symbol resolved
    = ada_resolve_funcall (sym, block,
			   context_type, parse_completion,
			   nargs, argvec.data (),
			   tracker);

  std::get<0> (m_storage)
    = make_operation<ada_var_value_operation> (resolved);
  return false;
}

bool
ada_ternop_slice_operation::resolve (struct expression *exp,
				     bool deprocedure_p,
				     bool parse_completion,
				     innermost_block_tracker *tracker,
				     struct type *context_type)
{
  /* Historically this check was done during resolution, so we
     continue that here.  */
  value *v = std::get<0> (m_storage)->evaluate (context_type, exp,
						EVAL_AVOID_SIDE_EFFECTS);
  if (ada_is_any_packed_array_type (v->type ()))
    error (_("cannot slice a packed array"));
  return false;
}

}



/* Return non-zero iff TYPE represents a System.Address type.  */

int
ada_is_system_address_type (struct type *type)
{
  return (type->name () && strcmp (type->name (), "system__address") == 0);
}



				/* Range types */

/* Scan STR beginning at position K for a discriminant name, and
   return the value of that discriminant field of DVAL in *PX.  If
   PNEW_K is not null, put the position of the character beyond the
   name scanned in *PNEW_K.  Return 1 if successful; return 0 and do
   not alter *PX and *PNEW_K if unsuccessful.  */

static int
scan_discrim_bound (const char *str, int k, struct value *dval, LONGEST * px,
		    int *pnew_k)
{
  static std::string storage;
  const char *pstart, *pend, *bound;
  struct value *bound_val;

  if (dval == NULL || str == NULL || str[k] == '\0')
    return 0;

  pstart = str + k;
  pend = strstr (pstart, "__");
  if (pend == NULL)
    {
      bound = pstart;
      k += strlen (bound);
    }
  else
    {
      int len = pend - pstart;

      /* Strip __ and beyond.  */
      storage = std::string (pstart, len);
      bound = storage.c_str ();
      k = pend - str;
    }

  bound_val = ada_search_struct_field (bound, dval, 0, dval->type ());
  if (bound_val == NULL)
    return 0;

  *px = value_as_long (bound_val);
  if (pnew_k != NULL)
    *pnew_k = k;
  return 1;
}

/* Value of variable named NAME.  Only exact matches are considered.
   If no such variable found, then if ERR_MSG is null, returns 0, and
   otherwise causes an error with message ERR_MSG.  */

static struct value *
get_var_value (const char *name, const char *err_msg)
{
  std::string quoted_name = add_angle_brackets (name);

  lookup_name_info lookup_name (quoted_name, symbol_name_match_type::FULL);

  std::vector<struct block_symbol> syms
    = ada_lookup_symbol_list_worker (lookup_name,
				     get_selected_block (0),
				     VAR_DOMAIN, 1);

  if (syms.size () != 1)
    {
      if (err_msg == NULL)
	return 0;
      else
	error (("%s"), err_msg);
    }

  return value_of_variable (syms[0].symbol, syms[0].block);
}

/* Value of integer variable named NAME in the current environment.
   If no such variable is found, returns false.  Otherwise, sets VALUE
   to the variable's value and returns true.  */

bool
get_int_var_value (const char *name, LONGEST &value)
{
  struct value *var_val = get_var_value (name, 0);

  if (var_val == 0)
    return false;

  value = value_as_long (var_val);
  return true;
}


/* Return a range type whose base type is that of the range type named
   NAME in the current environment, and whose bounds are calculated
   from NAME according to the GNAT range encoding conventions.
   Extract discriminant values, if needed, from DVAL.  ORIG_TYPE is the
   corresponding range type from debug information; fall back to using it
   if symbol lookup fails.  If a new type must be created, allocate it
   like ORIG_TYPE was.  The bounds information, in general, is encoded
   in NAME, the base type given in the named range type.  */

static struct type *
to_fixed_range_type (struct type *raw_type, struct value *dval)
{
  const char *name;
  struct type *base_type;
  const char *subtype_info;

  gdb_assert (raw_type != NULL);
  gdb_assert (raw_type->name () != NULL);

  if (raw_type->code () == TYPE_CODE_RANGE)
    base_type = raw_type->target_type ();
  else
    base_type = raw_type;

  name = raw_type->name ();
  subtype_info = strstr (name, "___XD");
  if (subtype_info == NULL)
    {
      LONGEST L = ada_discrete_type_low_bound (raw_type);
      LONGEST U = ada_discrete_type_high_bound (raw_type);

      if (L < INT_MIN || U > INT_MAX)
	return raw_type;
      else
	{
	  type_allocator alloc (raw_type);
	  return create_static_range_type (alloc, raw_type, L, U);
	}
    }
  else
    {
      int prefix_len = subtype_info - name;
      LONGEST L, U;
      struct type *type;
      const char *bounds_str;
      int n;

      subtype_info += 5;
      bounds_str = strchr (subtype_info, '_');
      n = 1;

      if (*subtype_info == 'L')
	{
	  if (!ada_scan_number (bounds_str, n, &L, &n)
	      && !scan_discrim_bound (bounds_str, n, dval, &L, &n))
	    return raw_type;
	  if (bounds_str[n] == '_')
	    n += 2;
	  else if (bounds_str[n] == '.')     /* FIXME? SGI Workshop kludge.  */
	    n += 1;
	  subtype_info += 1;
	}
      else
	{
	  std::string name_buf = std::string (name, prefix_len) + "___L";
	  if (!get_int_var_value (name_buf.c_str (), L))
	    {
	      lim_warning (_("Unknown lower bound, using 1."));
	      L = 1;
	    }
	}

      if (*subtype_info == 'U')
	{
	  if (!ada_scan_number (bounds_str, n, &U, &n)
	      && !scan_discrim_bound (bounds_str, n, dval, &U, &n))
	    return raw_type;
	}
      else
	{
	  std::string name_buf = std::string (name, prefix_len) + "___U";
	  if (!get_int_var_value (name_buf.c_str (), U))
	    {
	      lim_warning (_("Unknown upper bound, using %ld."), (long) L);
	      U = L;
	    }
	}

      type_allocator alloc (raw_type);
      type = create_static_range_type (alloc, base_type, L, U);
      /* create_static_range_type alters the resulting type's length
	 to match the size of the base_type, which is not what we want.
	 Set it back to the original range type's length.  */
      type->set_length (raw_type->length ());
      type->set_name (name);
      return type;
    }
}

/* True iff NAME is the name of a range type.  */

int
ada_is_range_type_name (const char *name)
{
  return (name != NULL && strstr (name, "___XD"));
}


				/* Modular types */

/* True iff TYPE is an Ada modular type.  */

int
ada_is_modular_type (struct type *type)
{
  struct type *subranged_type = get_base_type (type);

  return (subranged_type != NULL && type->code () == TYPE_CODE_RANGE
	  && subranged_type->code () == TYPE_CODE_INT
	  && subranged_type->is_unsigned ());
}

/* Assuming ada_is_modular_type (TYPE), the modulus of TYPE.  */

ULONGEST
ada_modulus (struct type *type)
{
  const dynamic_prop &high = type->bounds ()->high;

  if (high.is_constant ())
    return (ULONGEST) high.const_val () + 1;

  /* If TYPE is unresolved, the high bound might be a location list.  Return
     0, for lack of a better value to return.  */
  return 0;
}


/* Ada exception catchpoint support:
   ---------------------------------

   We support 3 kinds of exception catchpoints:
     . catchpoints on Ada exceptions
     . catchpoints on unhandled Ada exceptions
     . catchpoints on failed assertions

   Exceptions raised during failed assertions, or unhandled exceptions
   could perfectly be caught with the general catchpoint on Ada exceptions.
   However, we can easily differentiate these two special cases, and having
   the option to distinguish these two cases from the rest can be useful
   to zero-in on certain situations.

   Exception catchpoints are a specialized form of breakpoint,
   since they rely on inserting breakpoints inside known routines
   of the GNAT runtime.  The implementation therefore uses a standard
   breakpoint structure of the BP_BREAKPOINT type, but with its own set
   of breakpoint_ops.

   Support in the runtime for exception catchpoints have been changed
   a few times already, and these changes affect the implementation
   of these catchpoints.  In order to be able to support several
   variants of the runtime, we use a sniffer that will determine
   the runtime variant used by the program being debugged.  */

/* Ada's standard exceptions.

   The Ada 83 standard also defined Numeric_Error.  But there so many
   situations where it was unclear from the Ada 83 Reference Manual
   (RM) whether Constraint_Error or Numeric_Error should be raised,
   that the ARG (Ada Rapporteur Group) eventually issued a Binding
   Interpretation saying that anytime the RM says that Numeric_Error
   should be raised, the implementation may raise Constraint_Error.
   Ada 95 went one step further and pretty much removed Numeric_Error
   from the list of standard exceptions (it made it a renaming of
   Constraint_Error, to help preserve compatibility when compiling
   an Ada83 compiler). As such, we do not include Numeric_Error from
   this list of standard exceptions.  */

static const char * const standard_exc[] = {
  "constraint_error",
  "program_error",
  "storage_error",
  "tasking_error"
};

typedef CORE_ADDR (ada_unhandled_exception_name_addr_ftype) (void);

/* A structure that describes how to support exception catchpoints
   for a given executable.  */

struct exception_support_info
{
   /* The name of the symbol to break on in order to insert
      a catchpoint on exceptions.  */
   const char *catch_exception_sym;

   /* The name of the symbol to break on in order to insert
      a catchpoint on unhandled exceptions.  */
   const char *catch_exception_unhandled_sym;

   /* The name of the symbol to break on in order to insert
      a catchpoint on failed assertions.  */
   const char *catch_assert_sym;

   /* The name of the symbol to break on in order to insert
      a catchpoint on exception handling.  */
   const char *catch_handlers_sym;

   /* Assuming that the inferior just triggered an unhandled exception
      catchpoint, this function is responsible for returning the address
      in inferior memory where the name of that exception is stored.
      Return zero if the address could not be computed.  */
   ada_unhandled_exception_name_addr_ftype *unhandled_exception_name_addr;
};

static CORE_ADDR ada_unhandled_exception_name_addr (void);
static CORE_ADDR ada_unhandled_exception_name_addr_from_raise (void);

/* The following exception support info structure describes how to
   implement exception catchpoints with the latest version of the
   Ada runtime (as of 2019-08-??).  */

static const struct exception_support_info default_exception_support_info =
{
  "__gnat_debug_raise_exception", /* catch_exception_sym */
  "__gnat_unhandled_exception", /* catch_exception_unhandled_sym */
  "__gnat_debug_raise_assert_failure", /* catch_assert_sym */
  "__gnat_begin_handler_v1", /* catch_handlers_sym */
  ada_unhandled_exception_name_addr
};

/* The following exception support info structure describes how to
   implement exception catchpoints with an earlier version of the
   Ada runtime (as of 2007-03-06) using v0 of the EH ABI.  */

static const struct exception_support_info exception_support_info_v0 =
{
  "__gnat_debug_raise_exception", /* catch_exception_sym */
  "__gnat_unhandled_exception", /* catch_exception_unhandled_sym */
  "__gnat_debug_raise_assert_failure", /* catch_assert_sym */
  "__gnat_begin_handler", /* catch_handlers_sym */
  ada_unhandled_exception_name_addr
};

/* The following exception support info structure describes how to
   implement exception catchpoints with a slightly older version
   of the Ada runtime.  */

static const struct exception_support_info exception_support_info_fallback =
{
  "__gnat_raise_nodefer_with_msg", /* catch_exception_sym */
  "__gnat_unhandled_exception", /* catch_exception_unhandled_sym */
  "system__assertions__raise_assert_failure",  /* catch_assert_sym */
  "__gnat_begin_handler", /* catch_handlers_sym */
  ada_unhandled_exception_name_addr_from_raise
};

/* Return nonzero if we can detect the exception support routines
   described in EINFO.

   This function errors out if an abnormal situation is detected
   (for instance, if we find the exception support routines, but
   that support is found to be incomplete).  */

static int
ada_has_this_exception_support (const struct exception_support_info *einfo)
{
  struct symbol *sym;

  /* The symbol we're looking up is provided by a unit in the GNAT runtime
     that should be compiled with debugging information.  As a result, we
     expect to find that symbol in the symtabs.  */

  sym = standard_lookup (einfo->catch_exception_sym, NULL, VAR_DOMAIN);
  if (sym == NULL)
    {
      /* Perhaps we did not find our symbol because the Ada runtime was
	 compiled without debugging info, or simply stripped of it.
	 It happens on some GNU/Linux distributions for instance, where
	 users have to install a separate debug package in order to get
	 the runtime's debugging info.  In that situation, let the user
	 know why we cannot insert an Ada exception catchpoint.

	 Note: Just for the purpose of inserting our Ada exception
	 catchpoint, we could rely purely on the associated minimal symbol.
	 But we would be operating in degraded mode anyway, since we are
	 still lacking the debugging info needed later on to extract
	 the name of the exception being raised (this name is printed in
	 the catchpoint message, and is also used when trying to catch
	 a specific exception).  We do not handle this case for now.  */
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol (einfo->catch_exception_sym, NULL, NULL);

      if (msym.minsym && msym.minsym->type () != mst_solib_trampoline)
	error (_("Your Ada runtime appears to be missing some debugging "
		 "information.\nCannot insert Ada exception catchpoint "
		 "in this configuration."));

      return 0;
    }

  /* Make sure that the symbol we found corresponds to a function.  */

  if (sym->aclass () != LOC_BLOCK)
    error (_("Symbol \"%s\" is not a function (class = %d)"),
	   sym->linkage_name (), sym->aclass ());

  sym = standard_lookup (einfo->catch_handlers_sym, NULL, VAR_DOMAIN);
  if (sym == NULL)
    {
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol (einfo->catch_handlers_sym, NULL, NULL);

      if (msym.minsym && msym.minsym->type () != mst_solib_trampoline)
	error (_("Your Ada runtime appears to be missing some debugging "
		 "information.\nCannot insert Ada exception catchpoint "
		 "in this configuration."));

      return 0;
    }

  /* Make sure that the symbol we found corresponds to a function.  */

  if (sym->aclass () != LOC_BLOCK)
    error (_("Symbol \"%s\" is not a function (class = %d)"),
	   sym->linkage_name (), sym->aclass ());

  return 1;
}

/* Inspect the Ada runtime and determine which exception info structure
   should be used to provide support for exception catchpoints.

   This function will always set the per-inferior exception_info,
   or raise an error.  */

static void
ada_exception_support_info_sniffer (void)
{
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  /* If the exception info is already known, then no need to recompute it.  */
  if (data->exception_info != NULL)
    return;

  /* Check the latest (default) exception support info.  */
  if (ada_has_this_exception_support (&default_exception_support_info))
    {
      data->exception_info = &default_exception_support_info;
      return;
    }

  /* Try the v0 exception suport info.  */
  if (ada_has_this_exception_support (&exception_support_info_v0))
    {
      data->exception_info = &exception_support_info_v0;
      return;
    }

  /* Try our fallback exception suport info.  */
  if (ada_has_this_exception_support (&exception_support_info_fallback))
    {
      data->exception_info = &exception_support_info_fallback;
      return;
    }

  throw_error (NOT_FOUND_ERROR,
	       _("Could not find Ada runtime exception support"));
}

/* True iff FRAME is very likely to be that of a function that is
   part of the runtime system.  This is all very heuristic, but is
   intended to be used as advice as to what frames are uninteresting
   to most users.  */

static int
is_known_support_routine (frame_info_ptr frame)
{
  enum language func_lang;
  int i;
  const char *fullname;

  /* If this code does not have any debugging information (no symtab),
     This cannot be any user code.  */

  symtab_and_line sal = find_frame_sal (frame);
  if (sal.symtab == NULL)
    return 1;

  /* If there is a symtab, but the associated source file cannot be
     located, then assume this is not user code:  Selecting a frame
     for which we cannot display the code would not be very helpful
     for the user.  This should also take care of case such as VxWorks
     where the kernel has some debugging info provided for a few units.  */

  fullname = symtab_to_fullname (sal.symtab);
  if (access (fullname, R_OK) != 0)
    return 1;

  /* Check the unit filename against the Ada runtime file naming.
     We also check the name of the objfile against the name of some
     known system libraries that sometimes come with debugging info
     too.  */

  for (i = 0; known_runtime_file_name_patterns[i] != NULL; i += 1)
    {
      re_comp (known_runtime_file_name_patterns[i]);
      if (re_exec (lbasename (sal.symtab->filename)))
	return 1;
      if (sal.symtab->compunit ()->objfile () != NULL
	  && re_exec (objfile_name (sal.symtab->compunit ()->objfile ())))
	return 1;
    }

  /* Check whether the function is a GNAT-generated entity.  */

  gdb::unique_xmalloc_ptr<char> func_name
    = find_frame_funname (frame, &func_lang, NULL);
  if (func_name == NULL)
    return 1;

  for (i = 0; known_auxiliary_function_name_patterns[i] != NULL; i += 1)
    {
      re_comp (known_auxiliary_function_name_patterns[i]);
      if (re_exec (func_name.get ()))
	return 1;
    }

  return 0;
}

/* Find the first frame that contains debugging information and that is not
   part of the Ada run-time, starting from FI and moving upward.  */

void
ada_find_printable_frame (frame_info_ptr fi)
{
  for (; fi != NULL; fi = get_prev_frame (fi))
    {
      if (!is_known_support_routine (fi))
	{
	  select_frame (fi);
	  break;
	}
    }

}

/* Assuming that the inferior just triggered an unhandled exception
   catchpoint, return the address in inferior memory where the name
   of the exception is stored.
   
   Return zero if the address could not be computed.  */

static CORE_ADDR
ada_unhandled_exception_name_addr (void)
{
  return parse_and_eval_address ("e.full_name");
}

/* Same as ada_unhandled_exception_name_addr, except that this function
   should be used when the inferior uses an older version of the runtime,
   where the exception name needs to be extracted from a specific frame
   several frames up in the callstack.  */

static CORE_ADDR
ada_unhandled_exception_name_addr_from_raise (void)
{
  int frame_level;
  frame_info_ptr fi;
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  /* To determine the name of this exception, we need to select
     the frame corresponding to RAISE_SYM_NAME.  This frame is
     at least 3 levels up, so we simply skip the first 3 frames
     without checking the name of their associated function.  */
  fi = get_current_frame ();
  for (frame_level = 0; frame_level < 3; frame_level += 1)
    if (fi != NULL)
      fi = get_prev_frame (fi); 

  while (fi != NULL)
    {
      enum language func_lang;

      gdb::unique_xmalloc_ptr<char> func_name
	= find_frame_funname (fi, &func_lang, NULL);
      if (func_name != NULL)
	{
	  if (strcmp (func_name.get (),
		      data->exception_info->catch_exception_sym) == 0)
	    break; /* We found the frame we were looking for...  */
	}
      fi = get_prev_frame (fi);
    }

  if (fi == NULL)
    return 0;

  select_frame (fi);
  return parse_and_eval_address ("id.full_name");
}

/* Assuming the inferior just triggered an Ada exception catchpoint
   (of any type), return the address in inferior memory where the name
   of the exception is stored, if applicable.

   Assumes the selected frame is the current frame.

   Return zero if the address could not be computed, or if not relevant.  */

static CORE_ADDR
ada_exception_name_addr_1 (enum ada_exception_catchpoint_kind ex)
{
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  switch (ex)
    {
      case ada_catch_exception:
	return (parse_and_eval_address ("e.full_name"));
	break;

      case ada_catch_exception_unhandled:
	return data->exception_info->unhandled_exception_name_addr ();
	break;

      case ada_catch_handlers:
	return 0;  /* The runtimes does not provide access to the exception
		      name.  */
	break;

      case ada_catch_assert:
	return 0;  /* Exception name is not relevant in this case.  */
	break;

      default:
	internal_error (_("unexpected catchpoint type"));
	break;
    }

  return 0; /* Should never be reached.  */
}

/* Assuming the inferior is stopped at an exception catchpoint,
   return the message which was associated to the exception, if
   available.  Return NULL if the message could not be retrieved.

   Note: The exception message can be associated to an exception
   either through the use of the Raise_Exception function, or
   more simply (Ada 2005 and later), via:

       raise Exception_Name with "exception message";

   */

static gdb::unique_xmalloc_ptr<char>
ada_exception_message_1 (void)
{
  struct value *e_msg_val;
  int e_msg_len;

  /* For runtimes that support this feature, the exception message
     is passed as an unbounded string argument called "message".  */
  e_msg_val = parse_and_eval ("message");
  if (e_msg_val == NULL)
    return NULL; /* Exception message not supported.  */

  e_msg_val = ada_coerce_to_simple_array (e_msg_val);
  gdb_assert (e_msg_val != NULL);
  e_msg_len = e_msg_val->type ()->length ();

  /* If the message string is empty, then treat it as if there was
     no exception message.  */
  if (e_msg_len <= 0)
    return NULL;

  gdb::unique_xmalloc_ptr<char> e_msg ((char *) xmalloc (e_msg_len + 1));
  read_memory (e_msg_val->address (), (gdb_byte *) e_msg.get (),
	       e_msg_len);
  e_msg.get ()[e_msg_len] = '\0';

  return e_msg;
}

/* Same as ada_exception_message_1, except that all exceptions are
   contained here (returning NULL instead).  */

static gdb::unique_xmalloc_ptr<char>
ada_exception_message (void)
{
  gdb::unique_xmalloc_ptr<char> e_msg;

  try
    {
      e_msg = ada_exception_message_1 ();
    }
  catch (const gdb_exception_error &e)
    {
      e_msg.reset (nullptr);
    }

  return e_msg;
}

/* Same as ada_exception_name_addr_1, except that it intercepts and contains
   any error that ada_exception_name_addr_1 might cause to be thrown.
   When an error is intercepted, a warning with the error message is printed,
   and zero is returned.  */

static CORE_ADDR
ada_exception_name_addr (enum ada_exception_catchpoint_kind ex)
{
  CORE_ADDR result = 0;

  try
    {
      result = ada_exception_name_addr_1 (ex);
    }

  catch (const gdb_exception_error &e)
    {
      warning (_("failed to get exception name: %s"), e.what ());
      return 0;
    }

  return result;
}

static std::string ada_exception_catchpoint_cond_string
  (const char *excep_string,
   enum ada_exception_catchpoint_kind ex);

/* Ada catchpoints.

   In the case of catchpoints on Ada exceptions, the catchpoint will
   stop the target on every exception the program throws.  When a user
   specifies the name of a specific exception, we translate this
   request into a condition expression (in text form), and then parse
   it into an expression stored in each of the catchpoint's locations.
   We then use this condition to check whether the exception that was
   raised is the one the user is interested in.  If not, then the
   target is resumed again.  We store the name of the requested
   exception, in order to be able to re-set the condition expression
   when symbols change.  */

/* An instance of this type is used to represent an Ada catchpoint.  */

struct ada_catchpoint : public code_breakpoint
{
  ada_catchpoint (struct gdbarch *gdbarch_,
		  enum ada_exception_catchpoint_kind kind,
		  const char *cond_string,
		  bool tempflag,
		  bool enabled,
		  bool from_tty,
		  std::string &&excep_string_)
    : code_breakpoint (gdbarch_, bp_catchpoint, tempflag, cond_string),
      m_excep_string (std::move (excep_string_)),
      m_kind (kind)
  {
    /* Unlike most code_breakpoint types, Ada catchpoints are
       pspace-specific.  */
    pspace = current_program_space;
    enable_state = enabled ? bp_enabled : bp_disabled;
    language = language_ada;

    re_set ();
  }

  struct bp_location *allocate_location () override;
  void re_set () override;
  void check_status (struct bpstat *bs) override;
  enum print_stop_action print_it (const bpstat *bs) const override;
  bool print_one (const bp_location **) const override;
  void print_mention () const override;
  void print_recreate (struct ui_file *fp) const override;

private:

  /* A helper function for check_status.  Returns true if we should
     stop for this breakpoint hit.  If the user specified a specific
     exception, we only want to cause a stop if the program thrown
     that exception.  */
  bool should_stop_exception (const struct bp_location *bl) const;

  /* The name of the specific exception the user specified.  */
  std::string m_excep_string;

  /* What kind of catchpoint this is.  */
  enum ada_exception_catchpoint_kind m_kind;
};

/* An instance of this type is used to represent an Ada catchpoint
   breakpoint location.  */

class ada_catchpoint_location : public bp_location
{
public:
  explicit ada_catchpoint_location (ada_catchpoint *owner)
    : bp_location (owner, bp_loc_software_breakpoint)
  {}

  /* The condition that checks whether the exception that was raised
     is the specific exception the user specified on catchpoint
     creation.  */
  expression_up excep_cond_expr;
};

static struct symtab_and_line ada_exception_sal
     (enum ada_exception_catchpoint_kind ex);

/* Implement the RE_SET method in the structure for all exception
   catchpoint kinds.  */

void
ada_catchpoint::re_set ()
{
  std::vector<symtab_and_line> sals;
  try
    {
      struct symtab_and_line sal = ada_exception_sal (m_kind);
      sals.push_back (sal);
    }
  catch (const gdb_exception_error &ex)
    {
      /* For NOT_FOUND_ERROR, the breakpoint will be pending.  */
      if (ex.error != NOT_FOUND_ERROR)
	throw;
    }

  update_breakpoint_locations (this, pspace, sals, {});

  /* Reparse the exception conditional expressions.  One for each
     location.  */

  /* Nothing to do if there's no specific exception to catch.  */
  if (m_excep_string.empty ())
    return;

  /* Same if there are no locations... */
  if (!has_locations ())
    return;

  /* Compute the condition expression in text form, from the specific
     exception we want to catch.  */
  std::string cond_string
    = ada_exception_catchpoint_cond_string (m_excep_string.c_str (), m_kind);

  /* Iterate over all the catchpoint's locations, and parse an
     expression for each.  */
  for (bp_location &bl : locations ())
    {
      ada_catchpoint_location &ada_loc
	= static_cast<ada_catchpoint_location &> (bl);
      expression_up exp;

      if (!bl.shlib_disabled)
	{
	  const char *s;

	  s = cond_string.c_str ();
	  try
	    {
	      exp = parse_exp_1 (&s, bl.address, block_for_pc (bl.address), 0);
	    }
	  catch (const gdb_exception_error &e)
	    {
	      warning (_("failed to reevaluate internal exception condition "
			 "for catchpoint %d: %s"),
		       number, e.what ());
	    }
	}

      ada_loc.excep_cond_expr = std::move (exp);
    }
}

/* Implement the ALLOCATE_LOCATION method in the structure for all
   exception catchpoint kinds.  */

struct bp_location *
ada_catchpoint::allocate_location ()
{
  return new ada_catchpoint_location (this);
}

/* See declaration.  */

bool
ada_catchpoint::should_stop_exception (const struct bp_location *bl) const
{
  ada_catchpoint *c = gdb::checked_static_cast<ada_catchpoint *> (bl->owner);
  const struct ada_catchpoint_location *ada_loc
    = (const struct ada_catchpoint_location *) bl;
  bool stop;

  struct internalvar *var = lookup_internalvar ("_ada_exception");
  if (c->m_kind == ada_catch_assert)
    clear_internalvar (var);
  else
    {
      try
	{
	  const char *expr;

	  if (c->m_kind == ada_catch_handlers)
	    expr = ("GNAT_GCC_exception_Access(gcc_exception)"
		    ".all.occurrence.id");
	  else
	    expr = "e";

	  struct value *exc = parse_and_eval (expr);
	  set_internalvar (var, exc);
	}
      catch (const gdb_exception_error &ex)
	{
	  clear_internalvar (var);
	}
    }

  /* With no specific exception, should always stop.  */
  if (c->m_excep_string.empty ())
    return true;

  if (ada_loc->excep_cond_expr == NULL)
    {
      /* We will have a NULL expression if back when we were creating
	 the expressions, this location's had failed to parse.  */
      return true;
    }

  stop = true;
  try
    {
      scoped_value_mark mark;
      stop = value_true (ada_loc->excep_cond_expr->evaluate ());
    }
  catch (const gdb_exception_error &ex)
    {
      exception_fprintf (gdb_stderr, ex,
			 _("Error in testing exception condition:\n"));
    }

  return stop;
}

/* Implement the CHECK_STATUS method in the structure for all
   exception catchpoint kinds.  */

void
ada_catchpoint::check_status (bpstat *bs)
{
  bs->stop = should_stop_exception (bs->bp_location_at.get ());
}

/* Implement the PRINT_IT method in the structure for all exception
   catchpoint kinds.  */

enum print_stop_action
ada_catchpoint::print_it (const bpstat *bs) const
{
  struct ui_out *uiout = current_uiout;

  annotate_catchpoint (number);

  if (uiout->is_mi_like_p ())
    {
      uiout->field_string ("reason",
			   async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      uiout->field_string ("disp", bpdisp_text (disposition));
    }

  uiout->text (disposition == disp_del
	       ? "\nTemporary catchpoint " : "\nCatchpoint ");
  print_num_locno (bs, uiout);
  uiout->text (", ");

  /* ada_exception_name_addr relies on the selected frame being the
     current frame.  Need to do this here because this function may be
     called more than once when printing a stop, and below, we'll
     select the first frame past the Ada run-time (see
     ada_find_printable_frame).  */
  select_frame (get_current_frame ());

  switch (m_kind)
    {
      case ada_catch_exception:
      case ada_catch_exception_unhandled:
      case ada_catch_handlers:
	{
	  const CORE_ADDR addr = ada_exception_name_addr (m_kind);
	  char exception_name[256];

	  if (addr != 0)
	    {
	      read_memory (addr, (gdb_byte *) exception_name,
			   sizeof (exception_name) - 1);
	      exception_name [sizeof (exception_name) - 1] = '\0';
	    }
	  else
	    {
	      /* For some reason, we were unable to read the exception
		 name.  This could happen if the Runtime was compiled
		 without debugging info, for instance.  In that case,
		 just replace the exception name by the generic string
		 "exception" - it will read as "an exception" in the
		 notification we are about to print.  */
	      memcpy (exception_name, "exception", sizeof ("exception"));
	    }
	  /* In the case of unhandled exception breakpoints, we print
	     the exception name as "unhandled EXCEPTION_NAME", to make
	     it clearer to the user which kind of catchpoint just got
	     hit.  We used ui_out_text to make sure that this extra
	     info does not pollute the exception name in the MI case.  */
	  if (m_kind == ada_catch_exception_unhandled)
	    uiout->text ("unhandled ");
	  uiout->field_string ("exception-name", exception_name);
	}
	break;
      case ada_catch_assert:
	/* In this case, the name of the exception is not really
	   important.  Just print "failed assertion" to make it clearer
	   that his program just hit an assertion-failure catchpoint.
	   We used ui_out_text because this info does not belong in
	   the MI output.  */
	uiout->text ("failed assertion");
	break;
    }

  gdb::unique_xmalloc_ptr<char> exception_message = ada_exception_message ();
  if (exception_message != NULL)
    {
      uiout->text (" (");
      uiout->field_string ("exception-message", exception_message.get ());
      uiout->text (")");
    }

  uiout->text (" at ");
  ada_find_printable_frame (get_current_frame ());

  return PRINT_SRC_AND_LOC;
}

/* Implement the PRINT_ONE method in the structure for all exception
   catchpoint kinds.  */

bool
ada_catchpoint::print_one (const bp_location **last_loc) const
{ 
  struct ui_out *uiout = current_uiout;
  struct value_print_options opts;

  get_user_print_options (&opts);

  if (opts.addressprint)
    uiout->field_skip ("addr");

  annotate_field (5);
  switch (m_kind)
    {
      case ada_catch_exception:
	if (!m_excep_string.empty ())
	  {
	    std::string msg = string_printf (_("`%s' Ada exception"),
					     m_excep_string.c_str ());

	    uiout->field_string ("what", msg);
	  }
	else
	  uiout->field_string ("what", "all Ada exceptions");
	
	break;

      case ada_catch_exception_unhandled:
	uiout->field_string ("what", "unhandled Ada exceptions");
	break;
      
      case ada_catch_handlers:
	if (!m_excep_string.empty ())
	  {
	    uiout->field_fmt ("what",
			      _("`%s' Ada exception handlers"),
			      m_excep_string.c_str ());
	  }
	else
	  uiout->field_string ("what", "all Ada exceptions handlers");
	break;

      case ada_catch_assert:
	uiout->field_string ("what", "failed Ada assertions");
	break;

      default:
	internal_error (_("unexpected catchpoint type"));
	break;
    }

  return true;
}

/* Implement the PRINT_MENTION method in the breakpoint_ops structure
   for all exception catchpoint kinds.  */

void
ada_catchpoint::print_mention () const
{
  struct ui_out *uiout = current_uiout;

  uiout->text (disposition == disp_del ? _("Temporary catchpoint ")
						 : _("Catchpoint "));
  uiout->field_signed ("bkptno", number);
  uiout->text (": ");

  switch (m_kind)
    {
      case ada_catch_exception:
	if (!m_excep_string.empty ())
	  {
	    std::string info = string_printf (_("`%s' Ada exception"),
					      m_excep_string.c_str ());
	    uiout->text (info);
	  }
	else
	  uiout->text (_("all Ada exceptions"));
	break;

      case ada_catch_exception_unhandled:
	uiout->text (_("unhandled Ada exceptions"));
	break;

      case ada_catch_handlers:
	if (!m_excep_string.empty ())
	  {
	    std::string info
	      = string_printf (_("`%s' Ada exception handlers"),
			       m_excep_string.c_str ());
	    uiout->text (info);
	  }
	else
	  uiout->text (_("all Ada exceptions handlers"));
	break;

      case ada_catch_assert:
	uiout->text (_("failed Ada assertions"));
	break;

      default:
	internal_error (_("unexpected catchpoint type"));
	break;
    }
}

/* Implement the PRINT_RECREATE method in the structure for all
   exception catchpoint kinds.  */

void
ada_catchpoint::print_recreate (struct ui_file *fp) const
{
  switch (m_kind)
    {
      case ada_catch_exception:
	gdb_printf (fp, "catch exception");
	if (!m_excep_string.empty ())
	  gdb_printf (fp, " %s", m_excep_string.c_str ());
	break;

      case ada_catch_exception_unhandled:
	gdb_printf (fp, "catch exception unhandled");
	break;

      case ada_catch_handlers:
	gdb_printf (fp, "catch handlers");
	break;

      case ada_catch_assert:
	gdb_printf (fp, "catch assert");
	break;

      default:
	internal_error (_("unexpected catchpoint type"));
    }
  print_recreate_thread (fp);
}

/* See ada-lang.h.  */

bool
is_ada_exception_catchpoint (breakpoint *bp)
{
  return dynamic_cast<ada_catchpoint *> (bp) != nullptr;
}

/* Split the arguments specified in a "catch exception" command.  
   Set EX to the appropriate catchpoint type.
   Set EXCEP_STRING to the name of the specific exception if
   specified by the user.
   IS_CATCH_HANDLERS_CMD: True if the arguments are for a
   "catch handlers" command.  False otherwise.
   If a condition is found at the end of the arguments, the condition
   expression is stored in COND_STRING (memory must be deallocated
   after use).  Otherwise COND_STRING is set to NULL.  */

static void
catch_ada_exception_command_split (const char *args,
				   bool is_catch_handlers_cmd,
				   enum ada_exception_catchpoint_kind *ex,
				   std::string *excep_string,
				   std::string *cond_string)
{
  std::string exception_name;

  exception_name = extract_arg (&args);
  if (exception_name == "if")
    {
      /* This is not an exception name; this is the start of a condition
	 expression for a catchpoint on all exceptions.  So, "un-get"
	 this token, and set exception_name to NULL.  */
      exception_name.clear ();
      args -= 2;
    }

  /* Check to see if we have a condition.  */

  args = skip_spaces (args);
  if (startswith (args, "if")
      && (isspace (args[2]) || args[2] == '\0'))
    {
      args += 2;
      args = skip_spaces (args);

      if (args[0] == '\0')
	error (_("Condition missing after `if' keyword"));
      *cond_string = args;

      args += strlen (args);
    }

  /* Check that we do not have any more arguments.  Anything else
     is unexpected.  */

  if (args[0] != '\0')
    error (_("Junk at end of expression"));

  if (is_catch_handlers_cmd)
    {
      /* Catch handling of exceptions.  */
      *ex = ada_catch_handlers;
      *excep_string = exception_name;
    }
  else if (exception_name.empty ())
    {
      /* Catch all exceptions.  */
      *ex = ada_catch_exception;
      excep_string->clear ();
    }
  else if (exception_name == "unhandled")
    {
      /* Catch unhandled exceptions.  */
      *ex = ada_catch_exception_unhandled;
      excep_string->clear ();
    }
  else
    {
      /* Catch a specific exception.  */
      *ex = ada_catch_exception;
      *excep_string = exception_name;
    }
}

/* Return the name of the symbol on which we should break in order to
   implement a catchpoint of the EX kind.  */

static const char *
ada_exception_sym_name (enum ada_exception_catchpoint_kind ex)
{
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  gdb_assert (data->exception_info != NULL);

  switch (ex)
    {
      case ada_catch_exception:
	return (data->exception_info->catch_exception_sym);
	break;
      case ada_catch_exception_unhandled:
	return (data->exception_info->catch_exception_unhandled_sym);
	break;
      case ada_catch_assert:
	return (data->exception_info->catch_assert_sym);
	break;
      case ada_catch_handlers:
	return (data->exception_info->catch_handlers_sym);
	break;
      default:
	internal_error (_("unexpected catchpoint kind (%d)"), ex);
    }
}

/* Return the condition that will be used to match the current exception
   being raised with the exception that the user wants to catch.  This
   assumes that this condition is used when the inferior just triggered
   an exception catchpoint.
   EX: the type of catchpoints used for catching Ada exceptions.  */

static std::string
ada_exception_catchpoint_cond_string (const char *excep_string,
				      enum ada_exception_catchpoint_kind ex)
{
  bool is_standard_exc = false;
  std::string result;

  if (ex == ada_catch_handlers)
    {
      /* For exception handlers catchpoints, the condition string does
	 not use the same parameter as for the other exceptions.  */
      result = ("long_integer (GNAT_GCC_exception_Access"
		"(gcc_exception).all.occurrence.id)");
    }
  else
    result = "long_integer (e)";

  /* The standard exceptions are a special case.  They are defined in
     runtime units that have been compiled without debugging info; if
     EXCEP_STRING is the not-fully-qualified name of a standard
     exception (e.g. "constraint_error") then, during the evaluation
     of the condition expression, the symbol lookup on this name would
     *not* return this standard exception.  The catchpoint condition
     may then be set only on user-defined exceptions which have the
     same not-fully-qualified name (e.g. my_package.constraint_error).

     To avoid this unexcepted behavior, these standard exceptions are
     systematically prefixed by "standard".  This means that "catch
     exception constraint_error" is rewritten into "catch exception
     standard.constraint_error".

     If an exception named constraint_error is defined in another package of
     the inferior program, then the only way to specify this exception as a
     breakpoint condition is to use its fully-qualified named:
     e.g. my_package.constraint_error.  */

  for (const char *name : standard_exc)
    {
      if (strcmp (name, excep_string) == 0)
	{
	  is_standard_exc = true;
	  break;
	}
    }

  result += " = ";

  if (is_standard_exc)
    string_appendf (result, "long_integer (&standard.%s)", excep_string);
  else
    string_appendf (result, "long_integer (&%s)", excep_string);

  return result;
}

/* Return the symtab_and_line that should be used to insert an
   exception catchpoint of the TYPE kind.  */

static struct symtab_and_line
ada_exception_sal (enum ada_exception_catchpoint_kind ex)
{
  const char *sym_name;
  struct symbol *sym;

  /* First, find out which exception support info to use.  */
  ada_exception_support_info_sniffer ();

  /* Then lookup the function on which we will break in order to catch
     the Ada exceptions requested by the user.  */
  sym_name = ada_exception_sym_name (ex);
  sym = standard_lookup (sym_name, NULL, VAR_DOMAIN);

  if (sym == NULL)
    throw_error (NOT_FOUND_ERROR, _("Catchpoint symbol not found: %s"),
		 sym_name);

  if (sym->aclass () != LOC_BLOCK)
    error (_("Unable to insert catchpoint. %s is not a function."), sym_name);

  return find_function_start_sal (sym, 1);
}

/* Create an Ada exception catchpoint.

   EX_KIND is the kind of exception catchpoint to be created.

   If EXCEPT_STRING is empty, this catchpoint is expected to trigger
   for all exceptions.  Otherwise, EXCEPT_STRING indicates the name
   of the exception to which this catchpoint applies.

   COND_STRING, if not empty, is the catchpoint condition.

   TEMPFLAG, if nonzero, means that the underlying breakpoint
   should be temporary.

   FROM_TTY is the usual argument passed to all commands implementations.  */

void
create_ada_exception_catchpoint (struct gdbarch *gdbarch,
				 enum ada_exception_catchpoint_kind ex_kind,
				 std::string &&excep_string,
				 const std::string &cond_string,
				 int tempflag,
				 int enabled,
				 int from_tty)
{
  std::unique_ptr<ada_catchpoint> c
    (new ada_catchpoint (gdbarch, ex_kind,
			 cond_string.empty () ? nullptr : cond_string.c_str (),
			 tempflag, enabled, from_tty,
			 std::move (excep_string)));
  install_breakpoint (0, std::move (c), 1);
}

/* Implement the "catch exception" command.  */

static void
catch_ada_exception_command (const char *arg_entry, int from_tty,
			     struct cmd_list_element *command)
{
  const char *arg = arg_entry;
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  enum ada_exception_catchpoint_kind ex_kind;
  std::string excep_string;
  std::string cond_string;

  tempflag = command->context () == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  catch_ada_exception_command_split (arg, false, &ex_kind, &excep_string,
				     &cond_string);
  create_ada_exception_catchpoint (gdbarch, ex_kind,
				   std::move (excep_string), cond_string,
				   tempflag, 1 /* enabled */,
				   from_tty);
}

/* Implement the "catch handlers" command.  */

static void
catch_ada_handlers_command (const char *arg_entry, int from_tty,
			    struct cmd_list_element *command)
{
  const char *arg = arg_entry;
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  enum ada_exception_catchpoint_kind ex_kind;
  std::string excep_string;
  std::string cond_string;

  tempflag = command->context () == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  catch_ada_exception_command_split (arg, true, &ex_kind, &excep_string,
				     &cond_string);
  create_ada_exception_catchpoint (gdbarch, ex_kind,
				   std::move (excep_string), cond_string,
				   tempflag, 1 /* enabled */,
				   from_tty);
}

/* Completion function for the Ada "catch" commands.  */

static void
catch_ada_completer (struct cmd_list_element *cmd, completion_tracker &tracker,
		     const char *text, const char *word)
{
  std::vector<ada_exc_info> exceptions = ada_exceptions_list (NULL);

  for (const ada_exc_info &info : exceptions)
    {
      if (startswith (info.name, word))
	tracker.add_completion (make_unique_xstrdup (info.name));
    }
}

/* Split the arguments specified in a "catch assert" command.

   ARGS contains the command's arguments (or the empty string if
   no arguments were passed).

   If ARGS contains a condition, set COND_STRING to that condition
   (the memory needs to be deallocated after use).  */

static void
catch_ada_assert_command_split (const char *args, std::string &cond_string)
{
  args = skip_spaces (args);

  /* Check whether a condition was provided.  */
  if (startswith (args, "if")
      && (isspace (args[2]) || args[2] == '\0'))
    {
      args += 2;
      args = skip_spaces (args);
      if (args[0] == '\0')
	error (_("condition missing after `if' keyword"));
      cond_string.assign (args);
    }

  /* Otherwise, there should be no other argument at the end of
     the command.  */
  else if (args[0] != '\0')
    error (_("Junk at end of arguments."));
}

/* Implement the "catch assert" command.  */

static void
catch_assert_command (const char *arg_entry, int from_tty,
		      struct cmd_list_element *command)
{
  const char *arg = arg_entry;
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  std::string cond_string;

  tempflag = command->context () == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  catch_ada_assert_command_split (arg, cond_string);
  create_ada_exception_catchpoint (gdbarch, ada_catch_assert,
				   {}, cond_string,
				   tempflag, 1 /* enabled */,
				   from_tty);
}

/* Return non-zero if the symbol SYM is an Ada exception object.  */

static int
ada_is_exception_sym (struct symbol *sym)
{
  const char *type_name = sym->type ()->name ();

  return (sym->aclass () != LOC_TYPEDEF
	  && sym->aclass () != LOC_BLOCK
	  && sym->aclass () != LOC_CONST
	  && sym->aclass () != LOC_UNRESOLVED
	  && type_name != NULL && strcmp (type_name, "exception") == 0);
}

/* Given a global symbol SYM, return non-zero iff SYM is a non-standard
   Ada exception object.  This matches all exceptions except the ones
   defined by the Ada language.  */

static int
ada_is_non_standard_exception_sym (struct symbol *sym)
{
  if (!ada_is_exception_sym (sym))
    return 0;

  for (const char *name : standard_exc)
    if (strcmp (sym->linkage_name (), name) == 0)
      return 0;  /* A standard exception.  */

  /* Numeric_Error is also a standard exception, so exclude it.
     See the STANDARD_EXC description for more details as to why
     this exception is not listed in that array.  */
  if (strcmp (sym->linkage_name (), "numeric_error") == 0)
    return 0;

  return 1;
}

/* A helper function for std::sort, comparing two struct ada_exc_info
   objects.

   The comparison is determined first by exception name, and then
   by exception address.  */

bool
ada_exc_info::operator< (const ada_exc_info &other) const
{
  int result;

  result = strcmp (name, other.name);
  if (result < 0)
    return true;
  if (result == 0 && addr < other.addr)
    return true;
  return false;
}

bool
ada_exc_info::operator== (const ada_exc_info &other) const
{
  return addr == other.addr && strcmp (name, other.name) == 0;
}

/* Sort EXCEPTIONS using compare_ada_exception_info as the comparison
   routine, but keeping the first SKIP elements untouched.

   All duplicates are also removed.  */

static void
sort_remove_dups_ada_exceptions_list (std::vector<ada_exc_info> *exceptions,
				      int skip)
{
  std::sort (exceptions->begin () + skip, exceptions->end ());
  exceptions->erase (std::unique (exceptions->begin () + skip, exceptions->end ()),
		     exceptions->end ());
}

/* Add all exceptions defined by the Ada standard whose name match
   a regular expression.

   If PREG is not NULL, then this regexp_t object is used to
   perform the symbol name matching.  Otherwise, no name-based
   filtering is performed.

   EXCEPTIONS is a vector of exceptions to which matching exceptions
   gets pushed.  */

static void
ada_add_standard_exceptions (compiled_regex *preg,
			     std::vector<ada_exc_info> *exceptions)
{
  for (const char *name : standard_exc)
    {
      if (preg == NULL || preg->exec (name, 0, NULL, 0) == 0)
	{
	  symbol_name_match_type match_type = name_match_type_from_name (name);
	  lookup_name_info lookup_name (name, match_type);

	  symbol_name_matcher_ftype *match_name
	    = ada_get_symbol_name_matcher (lookup_name);

	  /* Iterate over all objfiles irrespective of scope or linker
	     namespaces so we get all exceptions anywhere in the
	     progspace.  */
	  for (objfile *objfile : current_program_space->objfiles ())
	    {
	      for (minimal_symbol *msymbol : objfile->msymbols ())
		{
		  if (match_name (msymbol->linkage_name (), lookup_name,
				  nullptr)
		      && msymbol->type () != mst_solib_trampoline)
		    {
		      ada_exc_info info
			= {name, msymbol->value_address (objfile)};

		      exceptions->push_back (info);
		    }
		}
	    }
	}
    }
}

/* Add all Ada exceptions defined locally and accessible from the given
   FRAME.

   If PREG is not NULL, then this regexp_t object is used to
   perform the symbol name matching.  Otherwise, no name-based
   filtering is performed.

   EXCEPTIONS is a vector of exceptions to which matching exceptions
   gets pushed.  */

static void
ada_add_exceptions_from_frame (compiled_regex *preg,
			       frame_info_ptr frame,
			       std::vector<ada_exc_info> *exceptions)
{
  const struct block *block = get_frame_block (frame, 0);

  while (block != 0)
    {
      for (struct symbol *sym : block_iterator_range (block))
	{
	  switch (sym->aclass ())
	    {
	    case LOC_TYPEDEF:
	    case LOC_BLOCK:
	    case LOC_CONST:
	      break;
	    default:
	      if (ada_is_exception_sym (sym))
		{
		  struct ada_exc_info info = {sym->print_name (),
					      sym->value_address ()};

		  exceptions->push_back (info);
		}
	    }
	}
      if (block->function () != NULL)
	break;
      block = block->superblock ();
    }
}

/* Return true if NAME matches PREG or if PREG is NULL.  */

static bool
name_matches_regex (const char *name, compiled_regex *preg)
{
  return (preg == NULL
	  || preg->exec (ada_decode (name).c_str (), 0, NULL, 0) == 0);
}

/* Add all exceptions defined globally whose name name match
   a regular expression, excluding standard exceptions.

   The reason we exclude standard exceptions is that they need
   to be handled separately: Standard exceptions are defined inside
   a runtime unit which is normally not compiled with debugging info,
   and thus usually do not show up in our symbol search.  However,
   if the unit was in fact built with debugging info, we need to
   exclude them because they would duplicate the entry we found
   during the special loop that specifically searches for those
   standard exceptions.

   If PREG is not NULL, then this regexp_t object is used to
   perform the symbol name matching.  Otherwise, no name-based
   filtering is performed.

   EXCEPTIONS is a vector of exceptions to which matching exceptions
   gets pushed.  */

static void
ada_add_global_exceptions (compiled_regex *preg,
			   std::vector<ada_exc_info> *exceptions)
{
  /* In Ada, the symbol "search name" is a linkage name, whereas the
     regular expression used to do the matching refers to the natural
     name.  So match against the decoded name.  */
  expand_symtabs_matching (NULL,
			   lookup_name_info::match_any (),
			   [&] (const char *search_name)
			   {
			     std::string decoded = ada_decode (search_name);
			     return name_matches_regex (decoded.c_str (), preg);
			   },
			   NULL,
			   SEARCH_GLOBAL_BLOCK | SEARCH_STATIC_BLOCK,
			   VARIABLES_DOMAIN);

  /* Iterate over all objfiles irrespective of scope or linker namespaces
     so we get all exceptions anywhere in the progspace.  */
  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *s : objfile->compunits ())
	{
	  const struct blockvector *bv = s->blockvector ();
	  int i;

	  for (i = GLOBAL_BLOCK; i <= STATIC_BLOCK; i++)
	    {
	      const struct block *b = bv->block (i);

	      for (struct symbol *sym : block_iterator_range (b))
		if (ada_is_non_standard_exception_sym (sym)
		    && name_matches_regex (sym->natural_name (), preg))
		  {
		    struct ada_exc_info info
		      = {sym->print_name (), sym->value_address ()};

		    exceptions->push_back (info);
		  }
	    }
	}
    }
}

/* Implements ada_exceptions_list with the regular expression passed
   as a regex_t, rather than a string.

   If not NULL, PREG is used to filter out exceptions whose names
   do not match.  Otherwise, all exceptions are listed.  */

static std::vector<ada_exc_info>
ada_exceptions_list_1 (compiled_regex *preg)
{
  std::vector<ada_exc_info> result;
  int prev_len;

  /* First, list the known standard exceptions.  These exceptions
     need to be handled separately, as they are usually defined in
     runtime units that have been compiled without debugging info.  */

  ada_add_standard_exceptions (preg, &result);

  /* Next, find all exceptions whose scope is local and accessible
     from the currently selected frame.  */

  if (has_stack_frames ())
    {
      prev_len = result.size ();
      ada_add_exceptions_from_frame (preg, get_selected_frame (NULL),
				     &result);
      if (result.size () > prev_len)
	sort_remove_dups_ada_exceptions_list (&result, prev_len);
    }

  /* Add all exceptions whose scope is global.  */

  prev_len = result.size ();
  ada_add_global_exceptions (preg, &result);
  if (result.size () > prev_len)
    sort_remove_dups_ada_exceptions_list (&result, prev_len);

  return result;
}

/* Return a vector of ada_exc_info.

   If REGEXP is NULL, all exceptions are included in the result.
   Otherwise, it should contain a valid regular expression,
   and only the exceptions whose names match that regular expression
   are included in the result.

   The exceptions are sorted in the following order:
     - Standard exceptions (defined by the Ada language), in
       alphabetical order;
     - Exceptions only visible from the current frame, in
       alphabetical order;
     - Exceptions whose scope is global, in alphabetical order.  */

std::vector<ada_exc_info>
ada_exceptions_list (const char *regexp)
{
  if (regexp == NULL)
    return ada_exceptions_list_1 (NULL);

  compiled_regex reg (regexp, REG_NOSUB, _("invalid regular expression"));
  return ada_exceptions_list_1 (&reg);
}

/* Implement the "info exceptions" command.  */

static void
info_exceptions_command (const char *regexp, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  std::vector<ada_exc_info> exceptions = ada_exceptions_list (regexp);

  if (regexp != NULL)
    gdb_printf
      (_("All Ada exceptions matching regular expression \"%s\":\n"), regexp);
  else
    gdb_printf (_("All defined Ada exceptions:\n"));

  for (const ada_exc_info &info : exceptions)
    gdb_printf ("%s: %s\n", info.name, paddress (gdbarch, info.addr));
}


				/* Language vector */

/* symbol_name_matcher_ftype adapter for wild_match.  */

static bool
do_wild_match (const char *symbol_search_name,
	       const lookup_name_info &lookup_name,
	       completion_match_result *comp_match_res)
{
  return wild_match (symbol_search_name, ada_lookup_name (lookup_name));
}

/* symbol_name_matcher_ftype adapter for full_match.  */

static bool
do_full_match (const char *symbol_search_name,
	       const lookup_name_info &lookup_name,
	       completion_match_result *comp_match_res)
{
  const char *lname = lookup_name.ada ().lookup_name ().c_str ();

  /* If both symbols start with "_ada_", just let the loop below
     handle the comparison.  However, if only the symbol name starts
     with "_ada_", skip the prefix and let the match proceed as
     usual.  */
  if (startswith (symbol_search_name, "_ada_")
      && !startswith (lname, "_ada"))
    symbol_search_name += 5;
  /* Likewise for ghost entities.  */
  if (startswith (symbol_search_name, "___ghost_")
      && !startswith (lname, "___ghost_"))
    symbol_search_name += 9;

  int uscore_count = 0;
  while (*lname != '\0')
    {
      if (*symbol_search_name != *lname)
	{
	  if (*symbol_search_name == 'B' && uscore_count == 2
	      && symbol_search_name[1] == '_')
	    {
	      symbol_search_name += 2;
	      while (isdigit (*symbol_search_name))
		++symbol_search_name;
	      if (symbol_search_name[0] == '_'
		  && symbol_search_name[1] == '_')
		{
		  symbol_search_name += 2;
		  continue;
		}
	    }
	  return false;
	}

      if (*symbol_search_name == '_')
	++uscore_count;
      else
	uscore_count = 0;

      ++symbol_search_name;
      ++lname;
    }

  return is_name_suffix (symbol_search_name);
}

/* symbol_name_matcher_ftype for exact (verbatim) matches.  */

static bool
do_exact_match (const char *symbol_search_name,
		const lookup_name_info &lookup_name,
		completion_match_result *comp_match_res)
{
  return strcmp (symbol_search_name, ada_lookup_name (lookup_name)) == 0;
}

/* Build the Ada lookup name for LOOKUP_NAME.  */

ada_lookup_name_info::ada_lookup_name_info (const lookup_name_info &lookup_name)
{
  std::string_view user_name = lookup_name.name ();

  if (!user_name.empty () && user_name[0] == '<')
    {
      if (user_name.back () == '>')
	m_encoded_name = user_name.substr (1, user_name.size () - 2);
      else
	m_encoded_name = user_name.substr (1, user_name.size () - 1);
      m_encoded_p = true;
      m_verbatim_p = true;
      m_wild_match_p = false;
      m_standard_p = false;
    }
  else
    {
      m_verbatim_p = false;

      m_encoded_p = user_name.find ("__") != std::string_view::npos;

      if (!m_encoded_p)
	{
	  const char *folded = ada_fold_name (user_name);
	  m_encoded_name = ada_encode_1 (folded, false);
	  if (m_encoded_name.empty ())
	    m_encoded_name = user_name;
	}
      else
	m_encoded_name = user_name;

      /* Handle the 'package Standard' special case.  See description
	 of m_standard_p.  */
      if (startswith (m_encoded_name.c_str (), "standard__"))
	{
	  m_encoded_name = m_encoded_name.substr (sizeof ("standard__") - 1);
	  m_standard_p = true;
	}
      else
	m_standard_p = false;

      m_decoded_name = ada_decode (m_encoded_name.c_str (), true, false, false);

      /* If the name contains a ".", then the user is entering a fully
	 qualified entity name, and the match must not be done in wild
	 mode.  Similarly, if the user wants to complete what looks
	 like an encoded name, the match must not be done in wild
	 mode.  Also, in the standard__ special case always do
	 non-wild matching.  */
      m_wild_match_p
	= (lookup_name.match_type () != symbol_name_match_type::FULL
	   && !m_encoded_p
	   && !m_standard_p
	   && user_name.find ('.') == std::string::npos);
    }
}

/* symbol_name_matcher_ftype method for Ada.  This only handles
   completion mode.  */

static bool
ada_symbol_name_matches (const char *symbol_search_name,
			 const lookup_name_info &lookup_name,
			 completion_match_result *comp_match_res)
{
  return lookup_name.ada ().matches (symbol_search_name,
				     lookup_name.match_type (),
				     comp_match_res);
}

/* A name matcher that matches the symbol name exactly, with
   strcmp.  */

static bool
literal_symbol_name_matcher (const char *symbol_search_name,
			     const lookup_name_info &lookup_name,
			     completion_match_result *comp_match_res)
{
  std::string_view name_view = lookup_name.name ();

  if (lookup_name.completion_mode ()
      ? (strncmp (symbol_search_name, name_view.data (),
		  name_view.size ()) == 0)
      : symbol_search_name == name_view)
    {
      if (comp_match_res != NULL)
	comp_match_res->set_match (symbol_search_name);
      return true;
    }
  else
    return false;
}

/* Implement the "get_symbol_name_matcher" language_defn method for
   Ada.  */

static symbol_name_matcher_ftype *
ada_get_symbol_name_matcher (const lookup_name_info &lookup_name)
{
  if (lookup_name.match_type () == symbol_name_match_type::SEARCH_NAME)
    return literal_symbol_name_matcher;

  if (lookup_name.completion_mode ())
    return ada_symbol_name_matches;
  else
    {
      if (lookup_name.ada ().wild_match_p ())
	return do_wild_match;
      else if (lookup_name.ada ().verbatim_p ())
	return do_exact_match;
      else
	return do_full_match;
    }
}

/* Class representing the Ada language.  */

class ada_language : public language_defn
{
public:
  ada_language ()
    : language_defn (language_ada)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "ada"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Ada"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions
      = { ".adb", ".ads", ".a", ".ada", ".dg" };
    return extensions;
  }

  /* Print an array element index using the Ada syntax.  */

  void print_array_index (struct type *index_type,
			  LONGEST index,
			  struct ui_file *stream,
			  const value_print_options *options) const override
  {
    struct value *index_value = val_atr (index_type, index);

    value_print (index_value, stream, options);
    gdb_printf (stream, " => ");
  }

  /* Implement the "read_var_value" language_defn method for Ada.  */

  struct value *read_var_value (struct symbol *var,
				const struct block *var_block,
				frame_info_ptr frame) const override
  {
    /* The only case where default_read_var_value is not sufficient
       is when VAR is a renaming...  */
    if (frame != nullptr)
      {
	const struct block *frame_block = get_frame_block (frame, NULL);
	if (frame_block != nullptr && ada_is_renaming_symbol (var))
	  return ada_read_renaming_var_value (var, frame_block);
      }

    /* This is a typical case where we expect the default_read_var_value
       function to work.  */
    return language_defn::read_var_value (var, var_block, frame);
  }

  /* See language.h.  */
  bool symbol_printing_suppressed (struct symbol *symbol) const override
  {
    return symbol->is_artificial ();
  }

  /* See language.h.  */
  struct value *value_string (struct gdbarch *gdbarch,
			      const char *ptr, ssize_t len) const override
  {
    struct type *type = language_string_char_type (this, gdbarch);
    value *val = ::value_string (ptr, len, type);
    /* VAL will be a TYPE_CODE_STRING, but Ada only knows how to print
       strings that are arrays of characters, so fix the type now.  */
    gdb_assert (val->type ()->code () == TYPE_CODE_STRING);
    val->type ()->set_code (TYPE_CODE_ARRAY);
    return val;
  }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    const struct builtin_type *builtin = builtin_type (gdbarch);

    /* Helper function to allow shorter lines below.  */
    auto add = [&] (struct type *t)
    {
      lai->add_primitive_type (t);
    };

    type_allocator alloc (gdbarch);
    add (init_integer_type (alloc, gdbarch_int_bit (gdbarch),
			    0, "integer"));
    add (init_integer_type (alloc, gdbarch_long_bit (gdbarch),
			    0, "long_integer"));
    add (init_integer_type (alloc, gdbarch_short_bit (gdbarch),
			    0, "short_integer"));
    struct type *char_type = init_character_type (alloc, TARGET_CHAR_BIT,
						  1, "character");
    lai->set_string_char_type (char_type);
    add (char_type);
    add (init_character_type (alloc, 16, 1, "wide_character"));
    add (init_character_type (alloc, 32, 1, "wide_wide_character"));
    add (init_float_type (alloc, gdbarch_float_bit (gdbarch),
			  "float", gdbarch_float_format (gdbarch)));
    add (init_float_type (alloc, gdbarch_double_bit (gdbarch),
			  "long_float", gdbarch_double_format (gdbarch)));
    add (init_integer_type (alloc, gdbarch_long_long_bit (gdbarch),
			    0, "long_long_integer"));
    add (init_integer_type (alloc, 128, 0, "long_long_long_integer"));
    add (init_integer_type (alloc, 128, 1, "unsigned_long_long_long_integer"));
    add (init_float_type (alloc, gdbarch_long_double_bit (gdbarch),
			  "long_long_float",
			  gdbarch_long_double_format (gdbarch)));
    add (init_integer_type (alloc, gdbarch_int_bit (gdbarch),
			    0, "natural"));
    add (init_integer_type (alloc, gdbarch_int_bit (gdbarch),
			    0, "positive"));
    add (builtin->builtin_void);

    struct type *system_addr_ptr
      = lookup_pointer_type (alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT,
					     "void"));
    system_addr_ptr->set_name ("system__address");
    add (system_addr_ptr);

    /* Create the equivalent of the System.Storage_Elements.Storage_Offset
       type.  This is a signed integral type whose size is the same as
       the size of addresses.  */
    unsigned int addr_length = system_addr_ptr->length ();
    add (init_integer_type (alloc, addr_length * HOST_CHAR_BIT, 0,
			    "storage_offset"));

    lai->set_bool_type (builtin->builtin_bool);
  }

  /* See language.h.  */

  bool iterate_over_symbols
	(const struct block *block, const lookup_name_info &name,
	 domain_enum domain,
	 gdb::function_view<symbol_found_callback_ftype> callback) const override
  {
    std::vector<struct block_symbol> results
      = ada_lookup_symbol_list_worker (name, block, domain, 0);
    for (block_symbol &sym : results)
      {
	if (!callback (&sym))
	  return false;
      }

    return true;
  }

  /* See language.h.  */
  bool sniff_from_mangled_name
       (const char *mangled,
	gdb::unique_xmalloc_ptr<char> *out) const override
  {
    std::string demangled = ada_decode (mangled);

    *out = NULL;

    if (demangled != mangled && demangled[0] != '<')
      {
	/* Set the gsymbol language to Ada, but still return 0.
	   Two reasons for that:

	   1. For Ada, we prefer computing the symbol's decoded name
	   on the fly rather than pre-compute it, in order to save
	   memory (Ada projects are typically very large).

	   2. There are some areas in the definition of the GNAT
	   encoding where, with a bit of bad luck, we might be able
	   to decode a non-Ada symbol, generating an incorrect
	   demangled name (Eg: names ending with "TB" for instance
	   are identified as task bodies and so stripped from
	   the decoded name returned).

	   Returning true, here, but not setting *DEMANGLED, helps us get
	   a little bit of the best of both worlds.  Because we're last,
	   we should not affect any of the other languages that were
	   able to demangle the symbol before us; we get to correctly
	   tag Ada symbols as such; and even if we incorrectly tagged a
	   non-Ada symbol, which should be rare, any routing through the
	   Ada language should be transparent (Ada tries to behave much
	   like C/C++ with non-Ada symbols).  */
	return true;
      }

    return false;
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
    return make_unique_xstrdup (ada_decode (mangled).c_str ());
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    ada_print_type (type, varstring, stream, show, level, flags);
  }

  /* See language.h.  */

  const char *word_break_characters (void) const override
  {
    return ada_completer_word_break_characters;
  }

  /* See language.h.  */

  void collect_symbol_completion_matches (completion_tracker &tracker,
					  complete_symbol_mode mode,
					  symbol_name_match_type name_match_type,
					  const char *text, const char *word,
					  enum type_code code) const override
  {
    const struct block *b, *surrounding_static_block = 0;

    gdb_assert (code == TYPE_CODE_UNDEF);

    lookup_name_info lookup_name (text, name_match_type, true);

    /* First, look at the partial symtab symbols.  */
    expand_symtabs_matching (NULL,
			     lookup_name,
			     NULL,
			     NULL,
			     SEARCH_GLOBAL_BLOCK | SEARCH_STATIC_BLOCK,
			     ALL_DOMAIN);

    /* At this point scan through the misc symbol vectors and add each
       symbol you find to the list.  Eventually we want to ignore
       anything that isn't a text symbol (everything else will be
       handled by the psymtab code above).  */

    for (objfile *objfile : current_program_space->objfiles ())
      {
	for (minimal_symbol *msymbol : objfile->msymbols ())
	  {
	    QUIT;

	    if (completion_skip_symbol (mode, msymbol))
	      continue;

	    language symbol_language = msymbol->language ();

	    /* Ada minimal symbols won't have their language set to Ada.  If
	       we let completion_list_add_name compare using the
	       default/C-like matcher, then when completing e.g., symbols in a
	       package named "pck", we'd match internal Ada symbols like
	       "pckS", which are invalid in an Ada expression, unless you wrap
	       them in '<' '>' to request a verbatim match.

	       Unfortunately, some Ada encoded names successfully demangle as
	       C++ symbols (using an old mangling scheme), such as "name__2Xn"
	       -> "Xn::name(void)" and thus some Ada minimal symbols end up
	       with the wrong language set.  Paper over that issue here.  */
	    if (symbol_language == language_unknown
		|| symbol_language == language_cplus)
	      symbol_language = language_ada;

	    completion_list_add_name (tracker,
				      symbol_language,
				      msymbol->linkage_name (),
				      lookup_name, text, word);
	  }
      }

    /* Search upwards from currently selected frame (so that we can
       complete on local vars.  */

    for (b = get_selected_block (0); b != NULL; b = b->superblock ())
      {
	if (!b->superblock ())
	  surrounding_static_block = b;   /* For elmin of dups */

	for (struct symbol *sym : block_iterator_range (b))
	  {
	    if (completion_skip_symbol (mode, sym))
	      continue;

	    completion_list_add_name (tracker,
				      sym->language (),
				      sym->linkage_name (),
				      lookup_name, text, word);
	  }
      }

    /* Go through the symtabs and check the externs and statics for
       symbols which match.  */

    for (objfile *objfile : current_program_space->objfiles ())
      {
	for (compunit_symtab *s : objfile->compunits ())
	  {
	    QUIT;
	    b = s->blockvector ()->global_block ();
	    for (struct symbol *sym : block_iterator_range (b))
	      {
		if (completion_skip_symbol (mode, sym))
		  continue;

		completion_list_add_name (tracker,
					  sym->language (),
					  sym->linkage_name (),
					  lookup_name, text, word);
	      }
	  }
      }

    for (objfile *objfile : current_program_space->objfiles ())
      {
	for (compunit_symtab *s : objfile->compunits ())
	  {
	    QUIT;
	    b = s->blockvector ()->static_block ();
	    /* Don't do this block twice.  */
	    if (b == surrounding_static_block)
	      continue;
	    for (struct symbol *sym : block_iterator_range (b))
	      {
		if (completion_skip_symbol (mode, sym))
		  continue;

		completion_list_add_name (tracker,
					  sym->language (),
					  sym->linkage_name (),
					  lookup_name, text, word);
	      }
	  }
      }
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> watch_location_expression
	(struct type *type, CORE_ADDR addr) const override
  {
    type = check_typedef (check_typedef (type)->target_type ());
    std::string name = type_to_string (type);
    return xstrprintf ("{%s} %s", name.c_str (), core_addr_to_string (addr));
  }

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override
  {
    return ada_value_print (val, stream, options);
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    return ada_value_print_inner (val, stream, recurse, options);
  }

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override
  {
    struct block_symbol sym;

    sym = ada_lookup_symbol (name,
			     (block == nullptr
			      ? nullptr
			      : block->static_block ()),
			     domain);
    if (sym.symbol != NULL)
      return sym;

    /* If we haven't found a match at this point, try the primitive
       types.  In other languages, this search is performed before
       searching for global symbols in order to short-circuit that
       global-symbol search if it happens that the name corresponds
       to a primitive type.  But we cannot do the same in Ada, because
       it is perfectly legitimate for a program to declare a type which
       has the same name as a standard type.  If looking up a type in
       that situation, we have traditionally ignored the primitive type
       in favor of user-defined types.  This is why, unlike most other
       languages, we search the primitive types this late and only after
       having searched the global symbols without success.  */

    if (domain == VAR_DOMAIN)
      {
	struct gdbarch *gdbarch;

	if (block == NULL)
	  gdbarch = current_inferior ()->arch ();
	else
	  gdbarch = block->gdbarch ();
	sym.symbol
	  = language_lookup_primitive_type_as_symbol (this, gdbarch, name);
	if (sym.symbol != NULL)
	  return sym;
      }

    return {};
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    warnings_issued = 0;
    return ada_parse (ps);
  }

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    ada_emit_char (ch, chtype, stream, quoter, 1);
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    ada_printchar (ch, chtype, stream);
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override
  {
    ada_printstr (stream, elttype, string, length, encoding,
		  force_ellipses, options);
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    ada_print_typedef (type, new_symbol, stream);
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    return ada_is_string_type (type);
  }

  /* See language.h.  */

  bool is_array_like (struct type *type) const override
  {
    return (ada_is_constrained_packed_array_type (type)
	    || ada_is_array_descriptor_type (type));
  }

  /* See language.h.  */

  struct value *to_array (struct value *val) const override
  { return ada_coerce_to_simple_array (val); }

  /* See language.h.  */

  const char *struct_too_deep_ellipsis () const override
  { return "(...)"; }

  /* See language.h.  */

  bool c_style_arrays_p () const override
  { return false; }

  /* See language.h.  */

  bool store_sym_names_in_linkage_form_p () const override
  { return true; }

  /* See language.h.  */

  const struct lang_varobj_ops *varobj_ops () const override
  { return &ada_varobj_ops; }

protected:
  /* See language.h.  */

  symbol_name_matcher_ftype *get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const override
  {
    return ada_get_symbol_name_matcher (lookup_name);
  }
};

/* Single instance of the Ada language class.  */

static ada_language ada_language_defn;

/* Command-list for the "set/show ada" prefix command.  */
static struct cmd_list_element *set_ada_list;
static struct cmd_list_element *show_ada_list;

/* This module's 'new_objfile' observer.  */

static void
ada_new_objfile_observer (struct objfile *objfile)
{
  ada_clear_symbol_cache (objfile->pspace);
}

/* This module's 'free_objfile' observer.  */

static void
ada_free_objfile_observer (struct objfile *objfile)
{
  ada_clear_symbol_cache (objfile->pspace);
}

/* Charsets known to GNAT.  */
static const char * const gnat_source_charsets[] =
{
  /* Note that code below assumes that the default comes first.
     Latin-1 is the default here, because that is also GNAT's
     default.  */
  "ISO-8859-1",
  "ISO-8859-2",
  "ISO-8859-3",
  "ISO-8859-4",
  "ISO-8859-5",
  "ISO-8859-15",
  "CP437",
  "CP850",
  /* Note that this value is special-cased in the encoder and
     decoder.  */
  ada_utf8,
  nullptr
};

void _initialize_ada_language ();
void
_initialize_ada_language ()
{
  add_setshow_prefix_cmd
    ("ada", no_class,
     _("Prefix command for changing Ada-specific settings."),
     _("Generic command for showing Ada-specific settings."),
     &set_ada_list, &show_ada_list,
     &setlist, &showlist);

  add_setshow_boolean_cmd ("trust-PAD-over-XVS", class_obscure,
			   &trust_pad_over_xvs, _("\
Enable or disable an optimization trusting PAD types over XVS types."), _("\
Show whether an optimization trusting PAD types over XVS types is activated."),
			   _("\
This is related to the encoding used by the GNAT compiler.  The debugger\n\
should normally trust the contents of PAD types, but certain older versions\n\
of GNAT have a bug that sometimes causes the information in the PAD type\n\
to be incorrect.  Turning this setting \"off\" allows the debugger to\n\
work around this bug.  It is always safe to turn this option \"off\", but\n\
this incurs a slight performance penalty, so it is recommended to NOT change\n\
this option to \"off\" unless necessary."),
			    NULL, NULL, &set_ada_list, &show_ada_list);

  add_setshow_boolean_cmd ("print-signatures", class_vars,
			   &print_signatures, _("\
Enable or disable the output of formal and return types for functions in the \
overloads selection menu."), _("\
Show whether the output of formal and return types for functions in the \
overloads selection menu is activated."),
			   NULL, NULL, NULL, &set_ada_list, &show_ada_list);

  ada_source_charset = gnat_source_charsets[0];
  add_setshow_enum_cmd ("source-charset", class_files,
			gnat_source_charsets,
			&ada_source_charset,  _("\
Set the Ada source character set."), _("\
Show the Ada source character set."), _("\
The character set used for Ada source files.\n\
This must correspond to the '-gnati' or '-gnatW' option passed to GNAT."),
			nullptr, nullptr,
			&set_ada_list, &show_ada_list);

  add_catch_command ("exception", _("\
Catch Ada exceptions, when raised.\n\
Usage: catch exception [ARG] [if CONDITION]\n\
Without any argument, stop when any Ada exception is raised.\n\
If ARG is \"unhandled\" (without the quotes), only stop when the exception\n\
being raised does not have a handler (and will therefore lead to the task's\n\
termination).\n\
Otherwise, the catchpoint only stops when the name of the exception being\n\
raised is the same as ARG.\n\
CONDITION is a boolean expression that is evaluated to see whether the\n\
exception should cause a stop."),
		     catch_ada_exception_command,
		     catch_ada_completer,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);

  add_catch_command ("handlers", _("\
Catch Ada exceptions, when handled.\n\
Usage: catch handlers [ARG] [if CONDITION]\n\
Without any argument, stop when any Ada exception is handled.\n\
With an argument, catch only exceptions with the given name.\n\
CONDITION is a boolean expression that is evaluated to see whether the\n\
exception should cause a stop."),
		     catch_ada_handlers_command,
		     catch_ada_completer,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("assert", _("\
Catch failed Ada assertions, when raised.\n\
Usage: catch assert [if CONDITION]\n\
CONDITION is a boolean expression that is evaluated to see whether the\n\
exception should cause a stop."),
		     catch_assert_command,
		     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);

  add_info ("exceptions", info_exceptions_command,
	    _("\
List all Ada exception names.\n\
Usage: info exceptions [REGEXP]\n\
If a regular expression is passed as an argument, only those matching\n\
the regular expression are listed."));

  add_setshow_prefix_cmd ("ada", class_maintenance,
			  _("Set Ada maintenance-related variables."),
			  _("Show Ada maintenance-related variables."),
			  &maint_set_ada_cmdlist, &maint_show_ada_cmdlist,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  add_setshow_boolean_cmd
    ("ignore-descriptive-types", class_maintenance,
     &ada_ignore_descriptive_types_p,
     _("Set whether descriptive types generated by GNAT should be ignored."),
     _("Show whether descriptive types generated by GNAT should be ignored."),
     _("\
When enabled, the debugger will stop using the DW_AT_GNAT_descriptive_type\n\
DWARF attribute."),
     NULL, NULL, &maint_set_ada_cmdlist, &maint_show_ada_cmdlist);

  decoded_names_store = htab_create_alloc (256, htab_hash_string,
					   htab_eq_string,
					   NULL, xcalloc, xfree);

  /* The ada-lang observers.  */
  gdb::observers::new_objfile.attach (ada_new_objfile_observer, "ada-lang");
  gdb::observers::all_objfiles_removed.attach (ada_clear_symbol_cache,
					       "ada-lang");
  gdb::observers::free_objfile.attach (ada_free_objfile_observer, "ada-lang");
  gdb::observers::inferior_exit.attach (ada_inferior_exit, "ada-lang");

#ifdef GDB_SELF_TEST
  selftests::register_test ("ada-decode", ada_decode_tests);
#endif
}

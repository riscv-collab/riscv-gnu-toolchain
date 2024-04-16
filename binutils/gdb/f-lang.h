/* Fortran language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C definitions by Farooq Butt
   (fmbutt@engage.sps.mot.com).

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

#ifndef F_LANG_H
#define F_LANG_H

#include "language.h"
#include "valprint.h"

struct type_print_options;
struct parser_state;

/* Class representing the Fortran language.  */

class f_language : public language_defn
{
public:
  f_language ()
    : language_defn (language_fortran)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "fortran"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Fortran"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions = {
      ".f", ".F", ".for", ".FOR", ".ftn", ".FTN", ".fpp", ".FPP",
      ".f90", ".F90", ".f95", ".F95", ".f03", ".F03", ".f08", ".F08"
    };
    return extensions;
  }

  /* See language.h.  */
  void print_array_index (struct type *index_type,
			  LONGEST index,
			  struct ui_file *stream,
			  const value_print_options *options) const override;

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override;

  /* See language.h.  */
  unsigned int search_name_hash (const char *name) const override;

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
      /* We could support demangling here to provide module namespaces
	 also for inferiors with only minimal symbol table (ELF symbols).
	 Just the mangling standard is not standardized across compilers
	 and there is no DW_AT_producer available for inferiors with only
	 the ELF symbols to check the mangling kind.  */
    return nullptr;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override;

  /* See language.h.  This just returns default set of word break
     characters but with the modules separator `::' removed.  */

  const char *word_break_characters (void) const override
  {
    static char *retval;

    if (!retval)
      {
	char *s;

	retval = xstrdup (language_defn::word_break_characters ());
	s = strchr (retval, ':');
	if (s)
	  {
	    char *last_char = &s[strlen (s) - 1];

	    *s = *last_char;
	    *last_char = 0;
	  }
      }
    return retval;
  }


  /* See language.h.  */

  void collect_symbol_completion_matches (completion_tracker &tracker,
					  complete_symbol_mode mode,
					  symbol_name_match_type name_match_type,
					  const char *text, const char *word,
					  enum type_code code) const override
  {
    /* Consider the modules separator :: as a valid symbol name character
       class.  */
    default_collect_symbol_completion_matches_break_on (tracker, mode,
							name_match_type,
							text, word, ":",
							code);
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override;

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override;

  /* See language.h.  */

  int parser (struct parser_state *ps) const override;

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    const char *encoding = get_encoding (chtype);
    generic_emit_char (ch, chtype, stream, quoter, encoding);
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    gdb_puts ("'", stream);
    emitchar (ch, chtype, stream, '\'');
    gdb_puts ("'", stream);
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override
  {
    const char *type_encoding = get_encoding (elttype);

    if (elttype->length () == 4)
      gdb_puts ("4_", stream);

    if (!encoding || !*encoding)
      encoding = type_encoding;

    generic_printstr (stream, elttype, string, length, encoding,
		      force_ellipses, '\'', 0, options);
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override;

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    type = check_typedef (type);
    return (type->code () == TYPE_CODE_STRING
	    || (type->code () == TYPE_CODE_ARRAY
		&& type->target_type ()->code () == TYPE_CODE_CHAR));
  }

  /* See language.h.  */

  struct value *value_string (struct gdbarch *gdbarch,
			      const char *ptr, ssize_t len) const override;

  /* See language.h.  */

  const char *struct_too_deep_ellipsis () const override
  { return "(...)"; }

  /* See language.h.  */

  bool c_style_arrays_p () const override
  { return false; }

  /* See language.h.  */

  bool range_checking_on_by_default () const override
  { return true; }

  /* See language.h.  */

  enum case_sensitivity case_sensitivity () const override
  { return case_sensitive_off; }

  /* See language.h.  */

  enum array_ordering array_ordering () const override
  { return array_column_major; }

protected:

  /* See language.h.  */

  symbol_name_matcher_ftype *get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const override;

private:
  /* Return the encoding that should be used for the character type
     TYPE.  */

  static const char *get_encoding (struct type *type);

  /* Print any asterisks or open-parentheses needed before the variable
     name (to describe its type).

     On outermost call, pass 0 for PASSED_A_PTR.
     On outermost call, SHOW > 0 means should ignore
     any typename for TYPE and show its details.
     SHOW is always zero on recursive calls.  */

  void f_type_print_varspec_prefix (struct type *type,
				    struct ui_file * stream,
				    int show, int passed_a_ptr) const;

  /* Print any array sizes, function arguments or close parentheses needed
     after the variable name (to describe its type).  Args work like
     c_type_print_varspec_prefix.

     PRINT_RANK_ONLY is true when TYPE is an array which should be printed
     without the upper and lower bounds being specified, this will occur
     when the array is not allocated or not associated and so there are no
     known upper or lower bounds.  */

  void f_type_print_varspec_suffix (struct type *type,
				    struct ui_file *stream,
				    int show, int passed_a_ptr,
				    int demangled_args,
				    int arrayprint_recurse_level,
				    bool print_rank_only) const;

  /* If TYPE is an extended type, then print out derivation information.

     A typical output could look like this:
     "Type, extends(point) :: waypoint"
     "    Type point :: point"
     "    real(kind=4) :: angle"
     "End Type waypoint".  */

  void f_type_print_derivation_info (struct type *type,
				     struct ui_file *stream) const;

  /* Print the name of the type (or the ultimate pointer target, function
     value or array element), or the description of a structure or union.

     SHOW nonzero means don't print this type as just its name;
     show its real definition even if it has a name.
     SHOW zero means print just typename or struct tag if there is one
     SHOW negative means abbreviate structure elements.
     SHOW is decremented for printing of structure elements.

     LEVEL is the depth to indent by.  We increase it for some recursive
     calls.  */

  void f_type_print_base (struct type *type, struct ui_file *stream, int show,
			  int level) const;
};

/* Language-specific data structures */

/* A common block.  */

struct common_block
{
  /* The number of entries in the block.  */
  size_t n_entries;

  /* The contents of the block, allocated using the struct hack.  All
     pointers in the array are non-NULL.  */
  struct symbol *contents[1];
};

extern LONGEST f77_get_upperbound (struct type *);

extern LONGEST f77_get_lowerbound (struct type *);

extern int calc_f77_array_dims (struct type *);

/* Fortran (F77) types */

struct builtin_f_type
{
  struct type *builtin_character = nullptr;
  struct type *builtin_integer_s1 = nullptr;
  struct type *builtin_integer_s2 = nullptr;
  struct type *builtin_integer = nullptr;
  struct type *builtin_integer_s8 = nullptr;
  struct type *builtin_logical_s1 = nullptr;
  struct type *builtin_logical_s2 = nullptr;
  struct type *builtin_logical = nullptr;
  struct type *builtin_logical_s8 = nullptr;
  struct type *builtin_real = nullptr;
  struct type *builtin_real_s8 = nullptr;
  struct type *builtin_real_s16 = nullptr;
  struct type *builtin_complex = nullptr;
  struct type *builtin_complex_s8 = nullptr;
  struct type *builtin_complex_s16 = nullptr;
  struct type *builtin_void = nullptr;
};

/* Return the Fortran type table for the specified architecture.  */
extern const struct builtin_f_type *builtin_f_type (struct gdbarch *gdbarch);

/* Ensures that function argument TYPE is appropriate to inform the debugger
   that ARG should be passed as a pointer.  Returns the potentially updated
   argument type.

   If ARG is of type pointer then the type of ARG is returned, otherwise
   TYPE is returned untouched.

   This function exists to augment the types of Fortran function call
   parameters to be pointers to the reported value, when the corresponding ARG
   has also been wrapped in a pointer (by fortran_argument_convert).  This
   informs the debugger that these arguments should be passed as a pointer
   rather than as the pointed to type.  */

extern struct type *fortran_preserve_arg_pointer (struct value *arg,
						  struct type *type);

/* Fortran arrays can have a negative stride.  When this happens it is
   often the case that the base address for an object is not the lowest
   address occupied by that object.  For example, an array slice (10:1:-1)
   will be encoded with lower bound 1, upper bound 10, a stride of
   -ELEMENT_SIZE, and have a base address pointer that points at the
   element with the highest address in memory.

   This really doesn't play well with our current model of value contents,
   but could easily require a significant update in order to be supported
   "correctly".

   For now, we manually force the base address to be the lowest addressed
   element here.  Yes, this will break some things, but it fixes other
   things.  The hope is that it fixes more than it breaks.  */

extern CORE_ADDR fortran_adjust_dynamic_array_base_address_hack
	(struct type *type, CORE_ADDR address);

#endif /* F_LANG_H */

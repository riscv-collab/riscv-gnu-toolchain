/* Declarations for value printing routines for GDB, the GNU debugger.

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

#ifndef VALPRINT_H
#define VALPRINT_H

#include "cli/cli-option.h"

/* Possibilities for prettyformat parameters to routines which print
   things.  */

enum val_prettyformat
  {
    Val_no_prettyformat = 0,
    Val_prettyformat,
    /* * Use the default setting which the user has specified.  */
    Val_prettyformat_default
  };

/* This is used to pass formatting options to various value-printing
   functions.  */
struct value_print_options
{
  /* Pretty-formatting control.  */
  enum val_prettyformat prettyformat;

  /* Controls pretty formatting of arrays.  */
  bool prettyformat_arrays;

  /* Controls pretty formatting of structures.  */
  bool prettyformat_structs;

  /* Controls printing of virtual tables.  */
  bool vtblprint;

  /* Controls printing of nested unions.  */
  bool unionprint;

  /* Controls printing of addresses.  */
  bool addressprint;

  /* Controls printing of nibbles.  */
  bool nibblesprint;

  /* Controls looking up an object's derived type using what we find
     in its vtables.  */
  bool objectprint;

  /* Maximum number of elements to print for vector contents, or UINT_MAX
     for no limit.  Note that "set print elements 0" stores UINT_MAX in
     print_max, which displays in a show command as "unlimited".  */
  unsigned int print_max;

  /* Maximum number of string chars to print for a string pointer value,
     zero if to follow the value of print_max, or UINT_MAX for no limit.  */
  unsigned int print_max_chars;

  /* Print repeat counts if there are more than this many repetitions
     of an element in an array.  */
  unsigned int repeat_count_threshold;

  /* The global output format letter.  */
  int output_format;

  /* The current format letter.  This is set locally for a given call,
     e.g. when the user passes a format to "print".  */
  int format;

  /* Print memory tag violations for pointers.  */
  bool memory_tag_violations;

  /* Stop printing at null character?  */
  bool stop_print_at_null;

  /* True if we should print the index of each element when printing
     an array.  */
  bool print_array_indexes;

  /* If true, then dereference references, otherwise just print
     them like pointers.  */
  bool deref_ref;

  /* If true, print static fields.  */
  bool static_field_print;

  /* If true, print static fields for Pascal.  FIXME: C++ has a
     flag, why not share with Pascal too?  */
  bool pascal_static_field_print;

  /* If true, don't do Python pretty-printing.  */
  bool raw;

  /* If true, print the value in "summary" form.
     If raw and summary are both true, don't print non-scalar values
     ("..." is printed instead).  */
  bool summary;

  /* If true, when printing a pointer, print the symbol to which it
     points, if any.  */
  bool symbol_print;

  /* Maximum print depth when printing nested aggregates.  */
  int max_depth;
};

/* The value to use for `print_max_chars' to follow `print_max'.  */
#define PRINT_MAX_CHARS_ELEMENTS 0

/* The value to use for `print_max_chars' for no limit.  */
#define PRINT_MAX_CHARS_UNLIMITED UINT_MAX

/* Return the character count limit for printing strings.  */

static inline unsigned int
get_print_max_chars (const struct value_print_options *options)
{
  return (options->print_max_chars != PRINT_MAX_CHARS_ELEMENTS
	  ? options->print_max_chars : options->print_max);
}

/* Create an option_def_group for the value_print options, with OPTS
   as context.  */
extern gdb::option::option_def_group make_value_print_options_def_group
  (value_print_options *opts);

/* The global print options set by the user.  In general this should
   not be directly accessed, except by set/show commands.  Ordinary
   code should call get_user_print_options instead.  */
extern struct value_print_options user_print_options;

/* Initialize *OPTS to be a copy of the user print options.  */
extern void get_user_print_options (struct value_print_options *opts);

/* Initialize *OPTS to be a copy of the user print options, but with
   pretty-formatting disabled.  */
extern void get_no_prettyformat_print_options (struct value_print_options *);

/* Initialize *OPTS to be a copy of the user print options, but using
   FORMAT as the formatting option.  */
extern void get_formatted_print_options (struct value_print_options *opts,
					 char format);

extern void maybe_print_array_index (struct type *index_type, LONGEST index,
				     struct ui_file *stream,
				     const struct value_print_options *);


/* Print elements of an array.  */

extern void value_print_array_elements (struct value *, struct ui_file *, int,
					const struct value_print_options *,
					unsigned int);

/* Print a scalar according to OPTIONS and SIZE on STREAM.  Format 'i'
   is not supported at this level.

   This is how the elements of an array or structure are printed
   with a format.  */

extern void value_print_scalar_formatted
  (struct value *val, const struct value_print_options *options,
   int size, struct ui_file *stream);

extern void print_binary_chars (struct ui_file *, const gdb_byte *,
				unsigned int, enum bfd_endian, bool,
				const struct value_print_options *options);

extern void print_octal_chars (struct ui_file *, const gdb_byte *,
			       unsigned int, enum bfd_endian);

extern void print_decimal_chars (struct ui_file *, const gdb_byte *,
				 unsigned int, bool, enum bfd_endian);

extern void print_hex_chars (struct ui_file *, const gdb_byte *,
			     unsigned int, enum bfd_endian, bool);

extern void print_function_pointer_address (const struct value_print_options *options,
					    struct gdbarch *gdbarch,
					    CORE_ADDR address,
					    struct ui_file *stream);

/* Helper function to check the validity of some bits of a value.

   If TYPE represents some aggregate type (e.g., a structure), return 1.

   Otherwise, any of the bytes starting at OFFSET and extending for
   TYPE->length () bytes are invalid, print a message to STREAM and
   return 0.  The checking is done using FUNCS.

   Otherwise, return 1.  */

extern int valprint_check_validity (struct ui_file *stream, struct type *type,
				    LONGEST embedded_offset,
				    const struct value *val);

extern void val_print_optimized_out (const struct value *val,
				     struct ui_file *stream);

/* Prints "<not saved>" to STREAM.  */
extern void val_print_not_saved (struct ui_file *stream);

extern void val_print_unavailable (struct ui_file *stream);

extern void val_print_invalid_address (struct ui_file *stream);

/* An instance of this is passed to generic_val_print and describes
   some language-specific ways to print things.  */

struct generic_val_print_decorations
{
  /* Printing complex numbers: what to print before, between the
     elements, and after.  */

  const char *complex_prefix;
  const char *complex_infix;
  const char *complex_suffix;

  /* Boolean true and false.  */

  const char *true_name;
  const char *false_name;

  /* What to print when we see TYPE_CODE_VOID.  */

  const char *void_name;

  /* Array start and end strings.  */
  const char *array_start;
  const char *array_end;
};


/* Print a value in a generic way.  VAL is the value, STREAM is where
   to print it, RECURSE is the recursion depth, OPTIONS describe how
   the printing should be done, and D is the language-specific
   decorations object.  Note that structs and unions cannot be printed
   by this function.  */

extern void generic_value_print (struct value *val, struct ui_file *stream,
				 int recurse,
				 const struct value_print_options *options,
				 const struct generic_val_print_decorations *d);

extern void generic_emit_char (int c, struct type *type, struct ui_file *stream,
			       int quoter, const char *encoding);

extern void generic_printstr (struct ui_file *stream, struct type *type, 
			      const gdb_byte *string, unsigned int length, 
			      const char *encoding, int force_ellipses,
			      int quote_char, int c_style_terminator,
			      const struct value_print_options *options);

/* Run the "output" command.  ARGS and FROM_TTY are the usual
   arguments passed to all command implementations, except ARGS is
   const.  */

extern void output_command (const char *args, int from_tty);

extern int val_print_scalar_type_p (struct type *type);

struct format_data
  {
    int count;
    char format;
    char size;
    bool print_tags;

    /* True if the value should be printed raw -- that is, bypassing
       python-based formatters.  */
    unsigned char raw;
  };

extern void print_command_parse_format (const char **expp, const char *cmdname,
					value_print_options *opts);

/* Print VAL to console according to OPTS, including recording it to
   the history.  */
extern void print_value (value *val, const value_print_options &opts);

/* Completer for the "print", "call", and "compile print"
   commands.  */
extern void print_command_completer (struct cmd_list_element *ignore,
				     completion_tracker &tracker,
				     const char *text, const char *word);

/* Given an address ADDR return all the elements needed to print the
   address in a symbolic form.  NAME can be mangled or not depending
   on DO_DEMANGLE (and also on the asm_demangle global variable,
   manipulated via ''set print asm-demangle'').  When
   PREFER_SYM_OVER_MINSYM is true, names (and offsets) from minimal
   symbols won't be used except in instances where no symbol was
   found; otherwise, a minsym might be used in some instances (mostly
   involving function with non-contiguous address ranges).  Return
   0 in case of success, when all the info in the OUT parameters is
   valid.  Return 1 otherwise.  */

extern int build_address_symbolic (struct gdbarch *,
				   CORE_ADDR addr,
				   bool do_demangle,
				   bool prefer_sym_over_minsym,
				   std::string *name,
				   int *offset,
				   std::string *filename,
				   int *line,
				   int *unmapped);

/* Check to see if RECURSE is greater than or equal to the allowed
   printing max-depth (see 'set print max-depth').  If it is then print an
   ellipsis expression to STREAM and return true, otherwise return false.
   LANGUAGE determines what type of ellipsis expression is printed.  */

extern bool val_print_check_max_depth (struct ui_file *stream, int recurse,
				       const struct value_print_options *opts,
				       const struct language_defn *language);

/* Like common_val_print, but call value_check_printable first.  */

extern void common_val_print_checked
  (struct value *val,
   struct ui_file *stream, int recurse,
   const struct value_print_options *options,
   const struct language_defn *language);

#endif

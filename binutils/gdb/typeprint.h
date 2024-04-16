/* Language independent support for printing types for GDB, the GNU debugger.
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

#ifndef TYPEPRINT_H
#define TYPEPRINT_H

#include "gdbsupport/gdb_obstack.h"

enum language;
struct ui_file;
struct typedef_hash_table;
struct ext_lang_type_printers;

struct type_print_options
{
  /* True means that no special printing flags should apply.  */
  unsigned int raw : 1;

  /* True means print methods in a class.  */
  unsigned int print_methods : 1;

  /* True means print typedefs in a class.  */
  unsigned int print_typedefs : 1;

  /* True means to print offsets, a la 'pahole'.  */
  unsigned int print_offsets : 1;

  /* True means to print offsets in hex, otherwise use decimal.  */
  unsigned int print_in_hex : 1;

  /* The number of nested type definitions to print.  -1 == all.  */
  int print_nested_type_limit;

  /* If not NULL, a local typedef hash table used when printing a
     type.  */
  typedef_hash_table *local_typedefs;

  /* If not NULL, a global typedef hash table used when printing a
     type.  */
  typedef_hash_table *global_typedefs;

  /* The list of type printers associated with the global typedef
     table.  This is intentionally opaque.  */
  struct ext_lang_type_printers *global_printers;
};

struct print_offset_data
{
  /* Indicate if the offset an d size fields should be printed in decimal
     (default) or hexadecimal.  */
  bool print_in_hex  = false;

  /* The offset to be applied to bitpos when PRINT_OFFSETS is true.
     This is needed for when we are printing nested structs and want
     to make sure that the printed offset for each field carries over
     the offset of the outer struct.  */
  unsigned int offset_bitpos = 0;

  /* END_BITPOS is the one-past-the-end bit position of the previous
     field (where we expect the current field to be if there is no
     hole).  */
  unsigned int end_bitpos = 0;

  /* Print information about field at index FIELD_IDX of the struct type
     TYPE and update this object.

     If the field is static, it simply prints the correct number of
     spaces.

     The output is strongly based on pahole(1).  */
  void update (struct type *type, unsigned int field_idx,
	       struct ui_file *stream);

  /* Call when all fields have been printed.  This will print
     information about any padding that may exist.  LEVEL is the
     desired indentation level.  */
  void finish (struct type *type, int level, struct ui_file *stream);

  /* When printing the offsets of a struct and its fields (i.e.,
     'ptype /o'; type_print_options::print_offsets), we use this many
     characters when printing the offset information at the beginning
     of the line.  This is needed in order to generate the correct
     amount of whitespaces when no offset info should be printed for a
     certain field.  */
  static const int indentation;

  explicit print_offset_data (const struct type_print_options *flags);

private:

  /* Helper function for ptype/o implementation that prints
     information about a hole, if necessary.  STREAM is where to
     print.  BITPOS is the bitpos of the current field.  FOR_WHAT is a
     string describing the purpose of the hole.  */

  void maybe_print_hole (struct ui_file *stream, unsigned int bitpos,
			 const char *for_what);
};

extern const struct type_print_options type_print_raw_options;

/* A hash table holding typedef_field objects.  This is more
   complicated than an ordinary hash because it must also track the
   lifetime of some -- but not all -- of the contained objects.  */

class typedef_hash_table
{
public:

  /* Create a new typedef-lookup hash table.  */
  typedef_hash_table ();

  /* Copy a typedef hash.  */
  typedef_hash_table (const typedef_hash_table &);

  typedef_hash_table &operator= (const typedef_hash_table &) = delete;

  /* Add typedefs from T to the hash table TABLE.  */
  void recursively_update (struct type *);

  /* Add template parameters from T to the typedef hash TABLE.  */
  void add_template_parameters (struct type *t);

  /* Look up the type T in the typedef hash tables contained in FLAGS.
     The local table is searched first, then the global table (either
     table can be NULL, in which case it is skipped).  If T is in a
     table, return its short (class-relative) typedef name.  Otherwise
     return NULL.  */
  static const char *find_typedef (const struct type_print_options *flags,
				   struct type *t);

private:

  static const char *find_global_typedef (const struct type_print_options *flags,
					  struct type *t);


  /* The actual hash table.  */
  htab_up m_table;

  /* Storage for typedef_field objects that must be synthesized.  */
  auto_obstack m_storage;
};


void print_type_scalar (struct type * type, LONGEST, struct ui_file *);

/* Assuming the TYPE is a fixed point type, print its type description
   on STREAM.  */

void print_type_fixed_point (struct type *type, struct ui_file *stream);

void c_type_print_args (struct type *, struct ui_file *, int, enum language,
			const struct type_print_options *);

/* Print <unknown return type> to stream STREAM.  */

void type_print_unknown_return_type (struct ui_file *stream);

/* Throw an error indicating that the user tried to use a symbol that
   has unknown type.  SYM_PRINT_NAME is the name of the symbol, to be
   included in the error message.  */
extern void error_unknown_type (const char *sym_print_name);

extern void val_print_not_allocated (struct ui_file *stream);

extern void val_print_not_associated (struct ui_file *stream);

#endif

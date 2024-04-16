/* Pascal language support definitions for GDB, the GNU debugger.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef P_LANG_H
#define P_LANG_H

/* This file is derived from c-lang.h */

struct value;
struct parser_state;

/* Determines if type TYPE is a pascal string type.  Returns a positive
   value if the type is a known pascal string type.  This function is used
   by p-valprint.c code to allow better string display.  If it is a pascal
   string type, then it also sets info needed to get the length and the
   data of the string length_pos, length_size and string_pos are given in
   bytes.  char_size gives the element size in bytes.  FIXME: if the
   position or the size of these fields are not multiple of TARGET_CHAR_BIT
   then the results are wrong but this does not happen for Free Pascal nor
   for GPC.  */

extern int pascal_is_string_type (struct type *type,int *length_pos,
				  int *length_size, int *string_pos,
				  struct type **char_type,
				  const char **arrayname);

/* Defined in p-lang.c */

extern const char *pascal_main_name (void);

/* These are in p-lang.c: */

extern int is_pascal_string_type (struct type *, int *, int *, int *,
				  struct type **, const char **);

extern int pascal_object_is_vtbl_ptr_type (struct type *);

extern int pascal_object_is_vtbl_member (struct type *);

/* Class representing the Pascal language.  */

class pascal_language : public language_defn
{
public:
  pascal_language ()
    : language_defn (language_pascal)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "pascal"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Pascal"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions
      = { ".pas", ".p", ".pp" };
    return extensions;
  }

  /* See language.h.  */

  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override;

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override;

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override;

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override;

  /* See language.h.  */

  int parser (struct parser_state *ps) const override;

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    int in_quotes = 0;

    print_one_char (ch, stream, &in_quotes);
    if (in_quotes)
      gdb_puts ("'", stream);
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override;

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override;

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override;

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    return pascal_is_string_type(type, nullptr, nullptr, nullptr,
				 nullptr, nullptr) > 0;
  }

  /* See language.h.  */

  const char *name_of_this () const override
  { return "this"; }

  /* See language.h.  */

  bool range_checking_on_by_default () const override
  { return true; }

private:

  /* Print the character C on STREAM as part of the contents of a literal
     string.  IN_QUOTES is reset to 0 if a char is written with #4 notation.  */

  void print_one_char (int c, struct ui_file *stream, int *in_quotes) const;

  /* Print the name of the type (or the ultimate pointer target,
     function value or array element), or the description of a
     structure or union.

     SHOW positive means print details about the type (e.g. enum values),
     and print structure elements passing SHOW - 1 for show.  SHOW negative
     means just print the type name or struct tag if there is one.  If
     there is no name, print something sensible but concise like "struct
     {...}".
     SHOW zero means just print the type name or struct tag if there is one.
     If there is no name, print something sensible but not as concise like
     "struct {int x; int y;}".

     LEVEL is the number of spaces to indent by.
     We increase it for some recursive calls.  */

  void type_print_base (struct type *type, struct ui_file *stream, int show,
			int level,
			const struct type_print_options *flags) const;


  /* Print any array sizes, function arguments or close parentheses
     needed after the variable name (to describe its type).
     Args work like pascal_type_print_varspec_prefix.  */

  void type_print_varspec_suffix (struct type *type, struct ui_file *stream,
				  int show, int passed_a_ptr,
				  int demangled_args,
				  const struct type_print_options *flags) const;

  /* Helper for pascal_language::type_print_varspec_suffix to print the
     suffix of a function or method.  */

  void type_print_func_varspec_suffix
	(struct type *type, struct ui_file *stream, int show,
	 int passed_a_ptr, int demangled_args,
	 const struct type_print_options *flags) const;

  /* Print any asterisks or open-parentheses needed before the
     variable name (to describe its type).

     On outermost call, pass 0 for PASSED_A_PTR.
     On outermost call, SHOW > 0 means should ignore
     any typename for TYPE and show its details.
     SHOW is always zero on recursive calls.  */

  void type_print_varspec_prefix
	(struct type *type, struct ui_file *stream, int show,
	 int passed_a_ptr, const struct type_print_options *flags) const;

  /* Print the function args from TYPE (a TYPE_CODE_FUNC) to STREAM taking
     FLAGS into account where appropriate.  */

  void  print_func_args (struct type *type, struct ui_file *stream,
			 const struct type_print_options *flags) const;

  /* Print the Pascal method arguments for PHYSNAME and METHODNAME to the
     file STREAM.  */

  void type_print_method_args (const char *physname, const char *methodname,
			       struct ui_file *stream) const;

  /* If TYPE is a derived type, then print out derivation information.
     Print only the actual base classes of this type, not the base classes
     of the base classes.  I.e. for the derivation hierarchy:

     class A { int a; };
     class B : public A {int b; };
     class C : public B {int c; };

     Print the type of class C as:

     class C : public B {
     int c;
     }

     Not as the following (like gdb used to), which is not legal C++ syntax
     for derived types and may be confused with the multiple inheritance
     form:

     class C : public B : public A {
     int c;
     }

     In general, gdb should try to print the types as closely as possible
     to the form that they appear in the source code.  */

  void type_print_derivation_info (struct ui_file *stream,
				   struct type *type) const;
};

#endif /* P_LANG_H */

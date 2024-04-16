/* C language support routines for GDB, the GNU debugger.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "c-lang.h"
#include "c-support.h"
#include "valprint.h"
#include "macroscope.h"
#include "charset.h"
#include "demangle.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "gdbsupport/gdb_obstack.h"
#include <ctype.h>
#include "gdbcore.h"
#include "gdbarch.h"
#include "c-exp.h"

/* Given a C string type, STR_TYPE, return the corresponding target
   character set name.  */

static const char *
charset_for_string_type (c_string_type str_type, struct gdbarch *gdbarch)
{
  switch (str_type & ~C_CHAR)
    {
    case C_STRING:
      return target_charset (gdbarch);
    case C_WIDE_STRING:
      return target_wide_charset (gdbarch);
    case C_STRING_16:
      /* FIXME: UTF-16 is not always correct.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	return "UTF-16BE";
      else
	return "UTF-16LE";
    case C_STRING_32:
      /* FIXME: UTF-32 is not always correct.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	return "UTF-32BE";
      else
	return "UTF-32LE";
    }
  internal_error (_("unhandled c_string_type"));
}

/* Classify ELTTYPE according to what kind of character it is.  Return
   the enum constant representing the character type.  Also set
   *ENCODING to the name of the character set to use when converting
   characters of this type in target BYTE_ORDER to the host character
   set.  */

static c_string_type
classify_type (struct type *elttype, struct gdbarch *gdbarch,
	       const char **encoding)
{
  c_string_type result;

  /* We loop because ELTTYPE may be a typedef, and we want to
     successively peel each typedef until we reach a type we
     understand.  We don't use CHECK_TYPEDEF because that will strip
     all typedefs at once -- but in C, wchar_t is itself a typedef, so
     that would do the wrong thing.  */
  while (elttype)
    {
      const char *name = elttype->name ();

      if (name == nullptr)
	{
	  result = C_CHAR;
	  goto done;
	}

      if (!strcmp (name, "wchar_t"))
	{
	  result = C_WIDE_CHAR;
	  goto done;
	}

      if (!strcmp (name, "char16_t"))
	{
	  result = C_CHAR_16;
	  goto done;
	}

      if (!strcmp (name, "char32_t"))
	{
	  result = C_CHAR_32;
	  goto done;
	}

      if (elttype->code () != TYPE_CODE_TYPEDEF)
	break;

      /* Call for side effects.  */
      check_typedef (elttype);

      if (elttype->target_type ())
	elttype = elttype->target_type ();
      else
	{
	  /* Perhaps check_typedef did not update the target type.  In
	     this case, force the lookup again and hope it works out.
	     It never will for C, but it might for C++.  */
	  elttype = check_typedef (elttype);
	}
    }

  /* Punt.  */
  result = C_CHAR;

 done:
  if (encoding)
    *encoding = charset_for_string_type (result, gdbarch);

  return result;
}

/* Print the character C on STREAM as part of the contents of a
   literal string whose delimiter is QUOTER.  Note that that format
   for printing characters and strings is language specific.  */

void
language_defn::emitchar (int c, struct type *type,
			 struct ui_file *stream, int quoter) const
{
  const char *encoding;

  classify_type (type, type->arch (), &encoding);
  generic_emit_char (c, type, stream, quoter, encoding);
}

/* See language.h.  */

void
language_defn::printchar (int c, struct type *type,
			  struct ui_file * stream) const
{
  c_string_type str_type;

  str_type = classify_type (type, type->arch (), NULL);
  switch (str_type)
    {
    case C_CHAR:
      break;
    case C_WIDE_CHAR:
      gdb_putc ('L', stream);
      break;
    case C_CHAR_16:
      gdb_putc ('u', stream);
      break;
    case C_CHAR_32:
      gdb_putc ('U', stream);
      break;
    }

  gdb_putc ('\'', stream);
  emitchar (c, type, stream, '\'');
  gdb_putc ('\'', stream);
}

/* Print the character string STRING, printing at most LENGTH
   characters.  LENGTH is -1 if the string is nul terminated.  Each
   character is WIDTH bytes long.  Printing stops early if the number
   hits print_max_chars; repeat counts are printed as appropriate.
   Print ellipses at the end if we had to stop before printing LENGTH
   characters, or if FORCE_ELLIPSES.  */

void
language_defn::printstr (struct ui_file *stream, struct type *type,
			 const gdb_byte *string, unsigned int length,
			 const char *user_encoding, int force_ellipses,
			 const struct value_print_options *options) const
{
  c_string_type str_type;
  const char *type_encoding;
  const char *encoding;

  str_type = (classify_type (type, type->arch (), &type_encoding)
	      & ~C_CHAR);
  switch (str_type)
    {
    case C_STRING:
      break;
    case C_WIDE_STRING:
      gdb_puts ("L", stream);
      break;
    case C_STRING_16:
      gdb_puts ("u", stream);
      break;
    case C_STRING_32:
      gdb_puts ("U", stream);
      break;
    }

  encoding = (user_encoding && *user_encoding) ? user_encoding : type_encoding;

  generic_printstr (stream, type, string, length, encoding, force_ellipses,
		    '"', 1, options);
}

/* Obtain a C string from the inferior storing it in a newly allocated
   buffer in BUFFER, which should be freed by the caller.  If the in-
   and out-parameter *LENGTH is specified at -1, the string is read
   until a null character of the appropriate width is found, otherwise
   the string is read to the length of characters specified.  The size
   of a character is determined by the length of the target type of
   the pointer or array.

   If VALUE is an array with a known length, and *LENGTH is -1,
   the function will not read past the end of the array.  However, any
   declared size of the array is ignored if *LENGTH > 0.

   On completion, *LENGTH will be set to the size of the string read in
   characters.  (If a length of -1 is specified, the length returned
   will not include the null character).  CHARSET is always set to the
   target charset.  */

void
c_get_string (struct value *value, gdb::unique_xmalloc_ptr<gdb_byte> *buffer,
	      int *length, struct type **char_type,
	      const char **charset)
{
  int err, width;
  unsigned int fetchlimit;
  struct type *type = check_typedef (value->type ());
  struct type *element_type = type->target_type ();
  int req_length = *length;
  enum bfd_endian byte_order
    = type_byte_order (type);

  if (element_type == NULL)
    goto error;

  if (type->code () == TYPE_CODE_ARRAY)
    {
      /* If we know the size of the array, we can use it as a limit on
	 the number of characters to be fetched.  */
      if (type->num_fields () == 1
	  && type->field (0).type ()->code () == TYPE_CODE_RANGE)
	{
	  LONGEST low_bound, high_bound;

	  get_discrete_bounds (type->field (0).type (),
			       &low_bound, &high_bound);
	  fetchlimit = high_bound - low_bound + 1;
	}
      else
	fetchlimit = UINT_MAX;
    }
  else if (type->code () == TYPE_CODE_PTR)
    fetchlimit = UINT_MAX;
  else
    /* We work only with arrays and pointers.  */
    goto error;

  if (! c_textual_element_type (element_type, 0))
    goto error;
  classify_type (element_type, element_type->arch (), charset);
  width = element_type->length ();

  /* If the string lives in GDB's memory instead of the inferior's,
     then we just need to copy it to BUFFER.  Also, since such strings
     are arrays with known size, FETCHLIMIT will hold the size of the
     array.

     An array is assumed to live in GDB's memory, so we take this path
     here.

     However, it's possible for the caller to request more array
     elements than apparently exist -- this can happen when using the
     C struct hack.  So, only do this if either no length was
     specified, or the length is within the existing bounds.  This
     avoids running off the end of the value's contents.  */
  if ((value->lval () == not_lval
       || value->lval () == lval_internalvar
       || type->code () == TYPE_CODE_ARRAY)
      && fetchlimit != UINT_MAX
      && (*length < 0 || *length <= fetchlimit))
    {
      int i;
      const gdb_byte *contents = value->contents ().data ();

      /* If a length is specified, use that.  */
      if (*length >= 0)
	i  = *length;
      else
	/* Otherwise, look for a null character.  */
	for (i = 0; i < fetchlimit; i++)
	  if (extract_unsigned_integer (contents + i * width,
					width, byte_order) == 0)
	    break;
  
      /* I is now either a user-defined length, the number of non-null
	 characters, or FETCHLIMIT.  */
      *length = i * width;
      buffer->reset ((gdb_byte *) xmalloc (*length));
      memcpy (buffer->get (), contents, *length);
      err = 0;
    }
  else
    {
      /* value_as_address does not return an address for an array when
	 c_style_arrays is false, so we handle that specially
	 here.  */
      CORE_ADDR addr;
      if (type->code () == TYPE_CODE_ARRAY)
	{
	  if (value->lval () != lval_memory)
	    error (_("Attempt to take address of value "
		     "not located in memory."));
	  addr = value->address ();
	}
      else
	addr = value_as_address (value);

      /* Prior to the fix for PR 16196 read_string would ignore fetchlimit
	 if length > 0.  The old "broken" behaviour is the behaviour we want:
	 The caller may want to fetch 100 bytes from a variable length array
	 implemented using the common idiom of having an array of length 1 at
	 the end of a struct.  In this case we want to ignore the declared
	 size of the array.  However, it's counterintuitive to implement that
	 behaviour in read_string: what does fetchlimit otherwise mean if
	 length > 0.  Therefore we implement the behaviour we want here:
	 If *length > 0, don't specify a fetchlimit.  This preserves the
	 previous behaviour.  We could move this check above where we know
	 whether the array is declared with a fixed size, but we only want
	 to apply this behaviour when calling read_string.  PR 16286.  */
      if (*length > 0)
	fetchlimit = UINT_MAX;

      err = target_read_string (addr, *length, width, fetchlimit,
				buffer, length);
      if (err != 0)
	memory_error (TARGET_XFER_E_IO, addr);
    }

  /* If the LENGTH is specified at -1, we want to return the string
     length up to the terminating null character.  If an actual length
     was specified, we want to return the length of exactly what was
     read.  */
  if (req_length == -1)
    /* If the last character is null, subtract it from LENGTH.  */
    if (*length > 0
	&& extract_unsigned_integer (buffer->get () + *length - width,
				     width, byte_order) == 0)
      *length -= width;
  
  /* The read_string function will return the number of bytes read.
     If length returned from read_string was > 0, return the number of
     characters read by dividing the number of bytes by width.  */
  if (*length != 0)
     *length = *length / width;

  *char_type = element_type;

  return;

 error:
  {
    std::string type_str = type_to_string (type);
    if (!type_str.empty ())
      {
	error (_("Trying to read string with inappropriate type `%s'."),
	       type_str.c_str ());
      }
    else
      error (_("Trying to read string with inappropriate type."));
  }
}


/* Evaluating C and C++ expressions.  */

/* Convert a UCN.  The digits of the UCN start at P and extend no
   farther than LIMIT.  DEST_CHARSET is the name of the character set
   into which the UCN should be converted.  The results are written to
   OUTPUT.  LENGTH is the maximum length of the UCN, either 4 or 8.
   Returns a pointer to just after the final digit of the UCN.  */

static const char *
convert_ucn (const char *p, const char *limit, const char *dest_charset,
	     struct obstack *output, int length)
{
  unsigned long result = 0;
  gdb_byte data[4];
  int i;

  for (i = 0; i < length && p < limit && ISXDIGIT (*p); ++i, ++p)
    result = (result << 4) + fromhex (*p);

  for (i = 3; i >= 0; --i)
    {
      data[i] = result & 0xff;
      result >>= 8;
    }

  convert_between_encodings ("UTF-32BE", dest_charset, data,
			     4, 4, output, translit_none);

  return p;
}

/* Emit a character, VALUE, which was specified numerically, to
   OUTPUT.  TYPE is the target character type.  */

static void
emit_numeric_character (struct type *type, unsigned long value,
			struct obstack *output)
{
  gdb_byte *buffer;

  buffer = (gdb_byte *) alloca (type->length ());
  pack_long (buffer, type, value);
  obstack_grow (output, buffer, type->length ());
}

/* Convert an octal escape sequence.  TYPE is the target character
   type.  The digits of the escape sequence begin at P and extend no
   farther than LIMIT.  The result is written to OUTPUT.  Returns a
   pointer to just after the final digit of the escape sequence.  */

static const char *
convert_octal (struct type *type, const char *p,
	       const char *limit, struct obstack *output)
{
  int i;
  unsigned long value = 0;

  for (i = 0;
       i < 3 && p < limit && ISDIGIT (*p) && *p != '8' && *p != '9';
       ++i)
    {
      value = 8 * value + fromhex (*p);
      ++p;
    }

  emit_numeric_character (type, value, output);

  return p;
}

/* Convert a hex escape sequence.  TYPE is the target character type.
   The digits of the escape sequence begin at P and extend no farther
   than LIMIT.  The result is written to OUTPUT.  Returns a pointer to
   just after the final digit of the escape sequence.  */

static const char *
convert_hex (struct type *type, const char *p,
	     const char *limit, struct obstack *output)
{
  unsigned long value = 0;

  while (p < limit && ISXDIGIT (*p))
    {
      value = 16 * value + fromhex (*p);
      ++p;
    }

  emit_numeric_character (type, value, output);

  return p;
}

#define ADVANCE					\
  do {						\
    ++p;					\
    if (p == limit)				\
      error (_("Malformed escape sequence"));	\
  } while (0)

/* Convert an escape sequence to a target format.  TYPE is the target
   character type to use, and DEST_CHARSET is the name of the target
   character set.  The backslash of the escape sequence is at *P, and
   the escape sequence will not extend past LIMIT.  The results are
   written to OUTPUT.  Returns a pointer to just past the final
   character of the escape sequence.  */

static const char *
convert_escape (struct type *type, const char *dest_charset,
		const char *p, const char *limit, struct obstack *output)
{
  /* Skip the backslash.  */
  ADVANCE;

  switch (*p)
    {
    case '\\':
      obstack_1grow (output, '\\');
      ++p;
      break;

    case 'x':
      ADVANCE;
      if (!ISXDIGIT (*p))
	error (_("\\x used with no following hex digits."));
      p = convert_hex (type, p, limit, output);
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      p = convert_octal (type, p, limit, output);
      break;

    case 'u':
    case 'U':
      {
	int length = *p == 'u' ? 4 : 8;

	ADVANCE;
	if (!ISXDIGIT (*p))
	  error (_("\\u used with no following hex digits"));
	p = convert_ucn (p, limit, dest_charset, output, length);
      }
    }

  return p;
}

/* Given a single string from a (C-specific) OP_STRING list, convert
   it to a target string, handling escape sequences specially.  The
   output is written to OUTPUT.  DATA is the input string, which has
   length LEN.  DEST_CHARSET is the name of the target character set,
   and TYPE is the type of target character to use.  */

static void
parse_one_string (struct obstack *output, const char *data, int len,
		  const char *dest_charset, struct type *type)
{
  const char *limit;

  limit = data + len;

  while (data < limit)
    {
      const char *p = data;

      /* Look for next escape, or the end of the input.  */
      while (p < limit && *p != '\\')
	++p;
      /* If we saw a run of characters, convert them all.  */
      if (p > data)
	convert_between_encodings (host_charset (), dest_charset,
				   (const gdb_byte *) data, p - data, 1,
				   output, translit_none);
      /* If we saw an escape, convert it.  */
      if (p < limit)
	p = convert_escape (type, dest_charset, p, limit, output);
      data = p;
    }
}

namespace expr
{

value *
c_string_operation::evaluate (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  struct type *type;
  struct value *result;
  c_string_type dest_type;
  const char *dest_charset;
  int satisfy_expected = 0;

  auto_obstack output;

  dest_type = std::get<0> (m_storage);

  switch (dest_type & ~C_CHAR)
    {
    case C_STRING:
      type = language_string_char_type (exp->language_defn,
					exp->gdbarch);
      break;
    case C_WIDE_STRING:
      type = lookup_typename (exp->language_defn, "wchar_t", NULL, 0);
      break;
    case C_STRING_16:
      type = lookup_typename (exp->language_defn, "char16_t", NULL, 0);
      break;
    case C_STRING_32:
      type = lookup_typename (exp->language_defn, "char32_t", NULL, 0);
      break;
    default:
      internal_error (_("unhandled c_string_type"));
    }

  /* If the caller expects an array of some integral type,
     satisfy them.  If something odder is expected, rely on the
     caller to cast.  */
  if (expect_type && expect_type->code () == TYPE_CODE_ARRAY)
    {
      struct type *element_type
	= check_typedef (expect_type->target_type ());

      if (element_type->code () == TYPE_CODE_INT
	  || element_type->code () == TYPE_CODE_CHAR)
	{
	  type = element_type;
	  satisfy_expected = 1;
	}
    }

  dest_charset = charset_for_string_type (dest_type, exp->gdbarch);

  for (const std::string &item : std::get<1> (m_storage))
    parse_one_string (&output, item.c_str (), item.size (),
		      dest_charset, type);

  if ((dest_type & C_CHAR) != 0)
    {
      LONGEST value;

      if (obstack_object_size (&output) != type->length ())
	error (_("Could not convert character "
		 "constant to target character set"));
      value = unpack_long (type, (gdb_byte *) obstack_base (&output));
      result = value_from_longest (type, value);
    }
  else
    {
      int element_size = type->length ();

      if (satisfy_expected)
	{
	  LONGEST low_bound, high_bound;

	  if (!get_discrete_bounds (expect_type->index_type (),
				    &low_bound, &high_bound))
	    {
	      low_bound = 0;
	      high_bound = (expect_type->length () / element_size) - 1;
	    }
	  if (obstack_object_size (&output) / element_size
	      > (high_bound - low_bound + 1))
	    error (_("Too many array elements"));

	  result = value::allocate (expect_type);
	  memcpy (result->contents_raw ().data (), obstack_base (&output),
		  obstack_object_size (&output));
	  /* Write the terminating character.  */
	  memset (result->contents_raw ().data () + obstack_object_size (&output),
		  0, element_size);
	}
      else
	result = value_cstring ((const gdb_byte *) obstack_base (&output),
				obstack_object_size (&output) / element_size,
				type);
    }
  return result;
}

} /* namespace expr */


/* See c-lang.h.  */

bool
c_is_string_type_p (struct type *type)
{
  type = check_typedef (type);
  while (type->code () == TYPE_CODE_REF)
    {
      type = type->target_type ();
      type = check_typedef (type);
    }

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      {
	/* See if target type looks like a string.  */
	struct type *array_target_type = type->target_type ();
	return (type->length () > 0
		&& array_target_type->length () > 0
		&& c_textual_element_type (array_target_type, 0));
      }
    case TYPE_CODE_STRING:
      return true;
    case TYPE_CODE_PTR:
      {
	struct type *element_type = type->target_type ();
	return c_textual_element_type (element_type, 0);
      }
    default:
      break;
    }

  return false;
}



/* See c-lang.h.  */

gdb::unique_xmalloc_ptr<char>
c_canonicalize_name (const char *name)
{
  if (strchr (name, ' ') != nullptr
      || streq (name, "signed")
      || streq (name, "unsigned"))
    return cp_canonicalize_string (name);
  return nullptr;
}



void
c_language_arch_info (struct gdbarch *gdbarch,
		      struct language_arch_info *lai)
{
  const struct builtin_type *builtin = builtin_type (gdbarch);

  /* Helper function to allow shorter lines below.  */
  auto add  = [&] (struct type * t)
  {
    lai->add_primitive_type (t);
  };

  add (builtin->builtin_int);
  add (builtin->builtin_long);
  add (builtin->builtin_short);
  add (builtin->builtin_char);
  add (builtin->builtin_float);
  add (builtin->builtin_double);
  add (builtin->builtin_void);
  add (builtin->builtin_long_long);
  add (builtin->builtin_signed_char);
  add (builtin->builtin_unsigned_char);
  add (builtin->builtin_unsigned_short);
  add (builtin->builtin_unsigned_int);
  add (builtin->builtin_unsigned_long);
  add (builtin->builtin_unsigned_long_long);
  add (builtin->builtin_long_double);
  add (builtin->builtin_complex);
  add (builtin->builtin_double_complex);
  add (builtin->builtin_decfloat);
  add (builtin->builtin_decdouble);
  add (builtin->builtin_declong);

  lai->set_string_char_type (builtin->builtin_char);
  lai->set_bool_type (builtin->builtin_int);
}

/* Class representing the C language.  */

class c_language : public language_defn
{
public:
  c_language ()
    : language_defn (language_c)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "c"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "C"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions = { ".c" };
    return extensions;
  }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    c_language_arch_info (gdbarch, lai);
  }

  /* See language.h.  */
  std::unique_ptr<compile_instance> get_compile_instance () const override
  {
    return c_get_compile_context ();
  }

  /* See language.h.  */
  std::string compute_program (compile_instance *inst,
			       const char *input,
			       struct gdbarch *gdbarch,
			       const struct block *expr_block,
			       CORE_ADDR expr_pc) const override
  {
    return c_compute_program (inst, input, gdbarch, expr_block, expr_pc);
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  bool store_sym_names_in_linkage_form_p () const override
  { return true; }

  /* See language.h.  */

  enum macro_expansion macro_expansion () const override
  { return macro_expansion_c; }
};

/* Single instance of the C language class.  */

static c_language c_language_defn;

/* A class for the C++ language.  */

class cplus_language : public language_defn
{
public:
  cplus_language ()
    : language_defn (language_cplus)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "c++"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "C++"; }

  /* See language.h  */
  const char *get_digit_separator () const override
  { return "\'"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions
      = { ".C", ".cc", ".cp", ".cpp", ".cxx", ".c++" };
    return extensions;
  }

  /* See language.h.  */

  struct language_pass_by_ref_info pass_by_reference_info
	(struct type *type) const override
  {
    return cp_pass_by_reference (type);
  }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    const struct builtin_type *builtin = builtin_type (gdbarch);

    /* Helper function to allow shorter lines below.  */
    auto add  = [&] (struct type * t)
    {
      lai->add_primitive_type (t);
    };

    add (builtin->builtin_int);
    add (builtin->builtin_long);
    add (builtin->builtin_short);
    add (builtin->builtin_char);
    add (builtin->builtin_float);
    add (builtin->builtin_double);
    add (builtin->builtin_void);
    add (builtin->builtin_long_long);
    add (builtin->builtin_signed_char);
    add (builtin->builtin_unsigned_char);
    add (builtin->builtin_unsigned_short);
    add (builtin->builtin_unsigned_int);
    add (builtin->builtin_unsigned_long);
    add (builtin->builtin_unsigned_long_long);
    add (builtin->builtin_long_double);
    add (builtin->builtin_complex);
    add (builtin->builtin_double_complex);
    add (builtin->builtin_bool);
    add (builtin->builtin_decfloat);
    add (builtin->builtin_decdouble);
    add (builtin->builtin_declong);
    add (builtin->builtin_char16);
    add (builtin->builtin_char32);
    add (builtin->builtin_wchar);

    lai->set_string_char_type (builtin->builtin_char);
    lai->set_bool_type (builtin->builtin_bool, "bool");
  }

  /* See language.h.  */
  struct type *lookup_transparent_type (const char *name) const override
  {
    return cp_lookup_transparent_type (name);
  }

  /* See language.h.  */
  std::unique_ptr<compile_instance> get_compile_instance () const override
  {
    return cplus_get_compile_context ();
  }

  /* See language.h.  */
  std::string compute_program (compile_instance *inst,
			       const char *input,
			       struct gdbarch *gdbarch,
			       const struct block *expr_block,
			       CORE_ADDR expr_pc) const override
  {
    return cplus_compute_program (inst, input, gdbarch, expr_block, expr_pc);
  }

  /* See language.h.  */
  unsigned int search_name_hash (const char *name) const override
  {
    return cp_search_name_hash (name);
  }

  /* See language.h.  */
  bool sniff_from_mangled_name
       (const char *mangled,
	gdb::unique_xmalloc_ptr<char> *demangled) const override
  {
    *demangled = gdb_demangle (mangled, DMGL_PARAMS | DMGL_ANSI);
    return *demangled != NULL;
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
    return gdb_demangle (mangled, options);
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  CORE_ADDR skip_trampoline (const frame_info_ptr &fi,
			     CORE_ADDR pc) const override
  {
    return cplus_skip_trampoline (fi, pc);
  }

  /* See language.h.  */

  char *class_name_from_physname (const char *physname) const override
  {
    return cp_class_name_from_physname (physname);
  }

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override
  {
    return cp_lookup_symbol_nonlocal (this, name, block, domain);
  }

  /* See language.h.  */

  const char *name_of_this () const override
  { return "this"; }

  /* See language.h.  */

  enum macro_expansion macro_expansion () const override
  { return macro_expansion_c; }

  /* See language.h.  */

  const struct lang_varobj_ops *varobj_ops () const override
  { return &cplus_varobj_ops; }

protected:

  /* See language.h.  */

  symbol_name_matcher_ftype *get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const override
  {
    return cp_get_symbol_name_matcher (lookup_name);
  }
};

/* The single instance of the C++ language class.  */

static cplus_language cplus_language_defn;

/* A class for the ASM language.  */

class asm_language : public language_defn
{
public:
  asm_language ()
    : language_defn (language_asm)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "asm"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Assembly"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions
      = { ".s", ".sx", ".S" };
    return extensions;
  }

  /* See language.h.

     FIXME: Should this have its own arch info method?  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    c_language_arch_info (gdbarch, lai);
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  bool store_sym_names_in_linkage_form_p () const override
  { return true; }

  /* See language.h.  */

  enum macro_expansion macro_expansion () const override
  { return macro_expansion_c; }
};

/* The single instance of the ASM language class.  */
static asm_language asm_language_defn;

/* A class for the minimal language.  This does not represent a real
   language.  It just provides a minimal support a-la-C that should allow
   users to do some simple operations when debugging applications that use
   a language currently not supported by GDB.  */

class minimal_language : public language_defn
{
public:
  minimal_language ()
    : language_defn (language_minimal)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "minimal"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Minimal"; }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    c_language_arch_info (gdbarch, lai);
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  bool store_sym_names_in_linkage_form_p () const override
  { return true; }

  /* See language.h.  */

  enum macro_expansion macro_expansion () const override
  { return macro_expansion_c; }
};

/* The single instance of the minimal language class.  */
static minimal_language minimal_language_defn;

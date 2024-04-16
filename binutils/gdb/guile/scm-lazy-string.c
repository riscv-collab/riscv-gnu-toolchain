/* Scheme interface to lazy strings.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "charset.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "guile-internal.h"

/* The <gdb:lazy-string> smob.  */

struct lazy_string_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /*  Holds the address of the lazy string.  */
  CORE_ADDR address;

  /*  Holds the encoding that will be applied to the string when the string
      is printed by GDB.  If the encoding is set to NULL then GDB will select
      the most appropriate encoding when the sting is printed.
      Space for this is malloc'd and will be freed when the object is
      freed.  */
  char *encoding;

  /* If TYPE is an array: If the length is known, then this value is the
     array's length, otherwise it is -1.
     If TYPE is not an array: Then this value represents the string's length.
     In either case, if the value is -1 then the string will be fetched and
     encoded up to the first null of appropriate width.  */
  int length;

  /* The type of the string.
     For example if the lazy string was created from a C "char*" then TYPE
     represents a C "char*".  To get the type of the character in the string
     call lsscm_elt_type which handles the different kinds of values for TYPE.
     This is recorded as an SCM object so that we take advantage of support for
     preserving the type should its owning objfile go away.  */
  SCM type;
};

static const char lazy_string_smob_name[] = "gdb:lazy-string";

/* The tag Guile knows the lazy string smob by.  */
static scm_t_bits lazy_string_smob_tag;

/* Administrivia for lazy string smobs.  */

/* The smob "free" function for <gdb:lazy-string>.  */

static size_t
lsscm_free_lazy_string_smob (SCM self)
{
  lazy_string_smob *v_smob = (lazy_string_smob *) SCM_SMOB_DATA (self);

  xfree (v_smob->encoding);

  return 0;
}

/* The smob "print" function for <gdb:lazy-string>.  */

static int
lsscm_print_lazy_string_smob (SCM self, SCM port, scm_print_state *pstate)
{
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s", lazy_string_smob_name);
  gdbscm_printf (port, " @%s", hex_string (ls_smob->address));
  if (ls_smob->length >= 0)
    gdbscm_printf (port, " length %d", ls_smob->length);
  if (ls_smob->encoding != NULL)
    gdbscm_printf (port, " encoding %s", ls_smob->encoding);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:lazy-string> object.
   The caller must verify:
   - length >= -1
   - !(address == 0 && length != 0)
   - type != NULL */

static SCM
lsscm_make_lazy_string_smob (CORE_ADDR address, int length,
			     const char *encoding, struct type *type)
{
  lazy_string_smob *ls_smob = (lazy_string_smob *)
    scm_gc_malloc (sizeof (lazy_string_smob), lazy_string_smob_name);
  SCM ls_scm;

  gdb_assert (length >= -1);
  gdb_assert (!(address == 0 && length != 0));
  gdb_assert (type != NULL);

  ls_smob->address = address;
  ls_smob->length = length;
  if (encoding == NULL || strcmp (encoding, "") == 0)
    ls_smob->encoding = NULL;
  else
    ls_smob->encoding = xstrdup (encoding);
  ls_smob->type = tyscm_scm_from_type (type);

  ls_scm = scm_new_smob (lazy_string_smob_tag, (scm_t_bits) ls_smob);
  gdbscm_init_gsmob (&ls_smob->base);

  return ls_scm;
}

/* Return non-zero if SCM is a <gdb:lazy-string> object.  */

int
lsscm_is_lazy_string (SCM scm)
{
  return SCM_SMOB_PREDICATE (lazy_string_smob_tag, scm);
}

/* (lazy-string? object) -> boolean */

static SCM
gdbscm_lazy_string_p (SCM scm)
{
  return scm_from_bool (lsscm_is_lazy_string (scm));
}

/* Main entry point to create a <gdb:lazy-string> object.
   If there's an error a <gdb:exception> object is returned.  */

SCM
lsscm_make_lazy_string (CORE_ADDR address, int length,
			const char *encoding, struct type *type)
{
  if (length < -1)
    {
      return gdbscm_make_out_of_range_error (NULL, 0,
					     scm_from_int (length),
					     _("invalid length"));
    }

  if (address == 0 && length != 0)
    {
      return gdbscm_make_out_of_range_error
	(NULL, 0, scm_from_int (length),
	 _("cannot create a lazy string with address 0x0,"
	   " and a non-zero length"));
    }

  if (type == NULL)
    {
      return gdbscm_make_out_of_range_error
	(NULL, 0, scm_from_int (0), _("a lazy string's type cannot be NULL"));
    }

  return lsscm_make_lazy_string_smob (address, length, encoding, type);
}

/* Returns the <gdb:lazy-string> smob in SELF.
   Throws an exception if SELF is not a <gdb:lazy-string> object.  */

static SCM
lsscm_get_lazy_string_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (lsscm_is_lazy_string (self), self, arg_pos, func_name,
		   lazy_string_smob_name);

  return self;
}

/* Return the type of a character in lazy string LS_SMOB.  */

static struct type *
lsscm_elt_type (lazy_string_smob *ls_smob)
{
  struct type *type = tyscm_scm_to_type (ls_smob->type);
  struct type *realtype;

  realtype = check_typedef (type);

  switch (realtype->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
      return realtype->target_type ();
    default:
      /* This is done to preserve existing behaviour.  PR 20769.
	 E.g., gdb.parse_and_eval("my_int_variable").lazy_string().type.  */
      return realtype;
    }
}

/* Lazy string methods.  */

/* (lazy-string-address <gdb:lazy-string>) -> address */

static SCM
gdbscm_lazy_string_address (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  return gdbscm_scm_from_ulongest (ls_smob->address);
}

/* (lazy-string-length <gdb:lazy-string>) -> integer */

static SCM
gdbscm_lazy_string_length (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  return scm_from_int (ls_smob->length);
}

/* (lazy-string-encoding <gdb:lazy-string>) -> string */

static SCM
gdbscm_lazy_string_encoding (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  /* An encoding can be set to NULL by the user, so check first.
     If NULL return #f.  */
  if (ls_smob != NULL)
    return gdbscm_scm_from_c_string (ls_smob->encoding);
  return SCM_BOOL_F;
}

/* (lazy-string-type <gdb:lazy-string>) -> <gdb:type> */

static SCM
gdbscm_lazy_string_type (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  return ls_smob->type;
}

/* (lazy-string->value <gdb:lazy-string>) -> <gdb:value> */

static SCM
gdbscm_lazy_string_to_value (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  SCM except_scm;
  struct value *value;

  value = lsscm_safe_lazy_string_to_value (ls_scm, SCM_ARG1, FUNC_NAME,
					   &except_scm);
  if (value == NULL)
    gdbscm_throw (except_scm);
  return vlscm_scm_from_value (value);
}

/* A "safe" version of gdbscm_lazy_string_to_value for use by
   vlscm_convert_typed_value_from_scheme.
   The result, upon success, is the value of <gdb:lazy-string> STRING.
   ARG_POS is the argument position of STRING in the original Scheme
   function call, used in exception text.
   If there's an error, NULL is returned and a <gdb:exception> object
   is stored in *except_scmp.

   Note: The result is still "lazy".  The caller must call value_fetch_lazy
   to actually fetch the value.  */

struct value *
lsscm_safe_lazy_string_to_value (SCM string, int arg_pos,
				 const char *func_name, SCM *except_scmp)
{
  lazy_string_smob *ls_smob;
  struct value *value = NULL;

  gdb_assert (lsscm_is_lazy_string (string));

  ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (string);

  if (ls_smob->address == 0)
    {
      *except_scmp
	= gdbscm_make_out_of_range_error (func_name, arg_pos, string,
					 _("cannot create a value from NULL"));
      return NULL;
    }

  try
    {
      struct type *type = tyscm_scm_to_type (ls_smob->type);
      struct type *realtype = check_typedef (type);

      switch (realtype->code ())
	{
	case TYPE_CODE_PTR:
	  /* If a length is specified we need to convert this to an array
	     of the specified size.  */
	  if (ls_smob->length != -1)
	    {
	      /* PR 20786: There's no way to specify an array of length zero.
		 Record a length of [0,-1] which is how Ada does it.  Anything
		 we do is broken, but this one possible solution.  */
	      type = lookup_array_range_type (realtype->target_type (),
					      0, ls_smob->length - 1);
	      value = value_at_lazy (type, ls_smob->address);
	    }
	  else
	    value = value_from_pointer (type, ls_smob->address);
	  break;
	default:
	  value = value_at_lazy (type, ls_smob->address);
	  break;
	}
    }
  catch (const gdb_exception &except)
    {
      *except_scmp = gdbscm_scm_from_gdb_exception (unpack (except));
      return NULL;
    }

  return value;
}

/* Print a lazy string to STREAM using val_print_string.
   STRING must be a <gdb:lazy-string> object.  */

void
lsscm_val_print_lazy_string (SCM string, struct ui_file *stream,
			     const struct value_print_options *options)
{
  lazy_string_smob *ls_smob;
  struct type *elt_type;

  gdb_assert (lsscm_is_lazy_string (string));

  ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (string);
  elt_type = lsscm_elt_type (ls_smob);

  val_print_string (elt_type, ls_smob->encoding,
		    ls_smob->address, ls_smob->length,
		    stream, options);
}

/* Initialize the Scheme lazy-strings code.  */

static const scheme_function lazy_string_functions[] =
{
  { "lazy-string?", 1, 0, 0, as_a_scm_t_subr (gdbscm_lazy_string_p),
    "\
Return #t if the object is a <gdb:lazy-string> object." },

  { "lazy-string-address", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_lazy_string_address),
    "\
Return the address of the lazy-string." },

  { "lazy-string-length", 1, 0, 0, as_a_scm_t_subr (gdbscm_lazy_string_length),
    "\
Return the length of the lazy-string.\n\
If the length is -1 then the length is determined by the first null\n\
of appropriate width." },

  { "lazy-string-encoding", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_lazy_string_encoding),
    "\
Return the encoding of the lazy-string." },

  { "lazy-string-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_lazy_string_type),
    "\
Return the <gdb:type> of the lazy-string." },

  { "lazy-string->value", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_lazy_string_to_value),
    "\
Return the <gdb:value> representation of the lazy-string." },

  END_FUNCTIONS
};

void
gdbscm_initialize_lazy_strings (void)
{
  lazy_string_smob_tag = gdbscm_make_smob_type (lazy_string_smob_name,
						sizeof (lazy_string_smob));
  scm_set_smob_free (lazy_string_smob_tag, lsscm_free_lazy_string_smob);
  scm_set_smob_print (lazy_string_smob_tag, lsscm_print_lazy_string_smob);

  gdbscm_define_functions (lazy_string_functions, 1);
}

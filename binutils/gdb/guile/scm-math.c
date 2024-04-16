/* GDB/Scheme support for math operations on values.

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
#include "arch-utils.h"
#include "charset.h"
#include "cp-abi.h"
#include "target-float.h"
#include "symtab.h"
#include "language.h"
#include "valprint.h"
#include "value.h"
#include "guile-internal.h"

/* Note: Use target types here to remain consistent with the values system in
   GDB (which uses target arithmetic).  */

enum valscm_unary_opcode
{
  VALSCM_NOT,
  VALSCM_NEG,
  VALSCM_NOP,
  VALSCM_ABS,
  /* Note: This is Scheme's "logical not", not GDB's.
     GDB calls this UNOP_COMPLEMENT.  */
  VALSCM_LOGNOT
};

enum valscm_binary_opcode
{
  VALSCM_ADD,
  VALSCM_SUB,
  VALSCM_MUL,
  VALSCM_DIV,
  VALSCM_REM,
  VALSCM_MOD,
  VALSCM_POW,
  VALSCM_LSH,
  VALSCM_RSH,
  VALSCM_MIN,
  VALSCM_MAX,
  VALSCM_BITAND,
  VALSCM_BITOR,
  VALSCM_BITXOR
};

/* If TYPE is a reference, return the target; otherwise return TYPE.  */
#define STRIP_REFERENCE(TYPE) \
  ((TYPE->code () == TYPE_CODE_REF) ? ((TYPE)->target_type ()) : (TYPE))

/* Helper for vlscm_unop.  Contains all the code that may throw a GDB
   exception.  */

static SCM
vlscm_unop_gdbthrow (enum valscm_unary_opcode opcode, SCM x,
		     const char *func_name)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const struct language_defn *language = current_language;

  scoped_value_mark free_values;

  SCM except_scm;
  value *arg1 = vlscm_convert_value_from_scheme (func_name, SCM_ARG1, x,
						 &except_scm, gdbarch,
						 language);
  if (arg1 == NULL)
    return except_scm;

  struct value *res_val = NULL;

  switch (opcode)
    {
    case VALSCM_NOT:
      /* Alas gdb and guile use the opposite meaning for "logical
	 not".  */
      {
	struct type *type = language_bool_type (language, gdbarch);
	res_val
	  = value_from_longest (type,
				(LONGEST) value_logical_not (arg1));
      }
      break;
    case VALSCM_NEG:
      res_val = value_neg (arg1);
      break;
    case VALSCM_NOP:
      /* Seemingly a no-op, but if X was a Scheme value it is now a
	 <gdb:value> object.  */
      res_val = arg1;
      break;
    case VALSCM_ABS:
      if (value_less (arg1, value::zero (arg1->type (), not_lval)))
	res_val = value_neg (arg1);
      else
	res_val = arg1;
      break;
    case VALSCM_LOGNOT:
      res_val = value_complement (arg1);
      break;
    default:
      gdb_assert_not_reached ("unsupported operation");
    }

  gdb_assert (res_val != NULL);
  return vlscm_scm_from_value (res_val);
}

static SCM
vlscm_unop (enum valscm_unary_opcode opcode, SCM x, const char *func_name)
{
  return gdbscm_wrap (vlscm_unop_gdbthrow, opcode, x, func_name);
}

/* Helper for vlscm_binop.  Contains all the code that may throw a GDB
   exception.  */

static SCM
vlscm_binop_gdbthrow (enum valscm_binary_opcode opcode, SCM x, SCM y,
		      const char *func_name)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const struct language_defn *language = current_language;
  struct value *arg1, *arg2;
  struct value *res_val = NULL;
  SCM except_scm;

  scoped_value_mark free_values;

  arg1 = vlscm_convert_value_from_scheme (func_name, SCM_ARG1, x,
					  &except_scm, gdbarch, language);
  if (arg1 == NULL)
    return except_scm;

  arg2 = vlscm_convert_value_from_scheme (func_name, SCM_ARG2, y,
					  &except_scm, gdbarch, language);
  if (arg2 == NULL)
    return except_scm;

  switch (opcode)
    {
    case VALSCM_ADD:
      {
	struct type *ltype = arg1->type ();
	struct type *rtype = arg2->type ();

	ltype = check_typedef (ltype);
	ltype = STRIP_REFERENCE (ltype);
	rtype = check_typedef (rtype);
	rtype = STRIP_REFERENCE (rtype);

	if (ltype->code () == TYPE_CODE_PTR
	    && is_integral_type (rtype))
	  res_val = value_ptradd (arg1, value_as_long (arg2));
	else if (rtype->code () == TYPE_CODE_PTR
		 && is_integral_type (ltype))
	  res_val = value_ptradd (arg2, value_as_long (arg1));
	else
	  res_val = value_binop (arg1, arg2, BINOP_ADD);
      }
      break;
    case VALSCM_SUB:
      {
	struct type *ltype = arg1->type ();
	struct type *rtype = arg2->type ();

	ltype = check_typedef (ltype);
	ltype = STRIP_REFERENCE (ltype);
	rtype = check_typedef (rtype);
	rtype = STRIP_REFERENCE (rtype);

	if (ltype->code () == TYPE_CODE_PTR
	    && rtype->code () == TYPE_CODE_PTR)
	  {
	    /* A ptrdiff_t for the target would be preferable here.  */
	    res_val
	      = value_from_longest (builtin_type (gdbarch)->builtin_long,
				    value_ptrdiff (arg1, arg2));
	  }
	else if (ltype->code () == TYPE_CODE_PTR
		 && is_integral_type (rtype))
	  res_val = value_ptradd (arg1, - value_as_long (arg2));
	else
	  res_val = value_binop (arg1, arg2, BINOP_SUB);
      }
      break;
    case VALSCM_MUL:
      res_val = value_binop (arg1, arg2, BINOP_MUL);
      break;
    case VALSCM_DIV:
      res_val = value_binop (arg1, arg2, BINOP_DIV);
      break;
    case VALSCM_REM:
      res_val = value_binop (arg1, arg2, BINOP_REM);
      break;
    case VALSCM_MOD:
      res_val = value_binop (arg1, arg2, BINOP_MOD);
      break;
    case VALSCM_POW:
      res_val = value_binop (arg1, arg2, BINOP_EXP);
      break;
    case VALSCM_LSH:
      res_val = value_binop (arg1, arg2, BINOP_LSH);
      break;
    case VALSCM_RSH:
      res_val = value_binop (arg1, arg2, BINOP_RSH);
      break;
    case VALSCM_MIN:
      res_val = value_binop (arg1, arg2, BINOP_MIN);
      break;
    case VALSCM_MAX:
      res_val = value_binop (arg1, arg2, BINOP_MAX);
      break;
    case VALSCM_BITAND:
      res_val = value_binop (arg1, arg2, BINOP_BITWISE_AND);
      break;
    case VALSCM_BITOR:
      res_val = value_binop (arg1, arg2, BINOP_BITWISE_IOR);
      break;
    case VALSCM_BITXOR:
      res_val = value_binop (arg1, arg2, BINOP_BITWISE_XOR);
      break;
    default:
      gdb_assert_not_reached ("unsupported operation");
    }

  gdb_assert (res_val != NULL);
  return vlscm_scm_from_value (res_val);
}

/* Returns a value object which is the result of applying the operation
   specified by OPCODE to the given arguments.
   If there's an error a Scheme exception is thrown.  */

static SCM
vlscm_binop (enum valscm_binary_opcode opcode, SCM x, SCM y,
	     const char *func_name)
{
  return gdbscm_wrap (vlscm_binop_gdbthrow, opcode, x, y, func_name);
}

/* (value-add x y) -> <gdb:value> */

static SCM
gdbscm_value_add (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_ADD, x, y, FUNC_NAME);
}

/* (value-sub x y) -> <gdb:value> */

static SCM
gdbscm_value_sub (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_SUB, x, y, FUNC_NAME);
}

/* (value-mul x y) -> <gdb:value> */

static SCM
gdbscm_value_mul (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_MUL, x, y, FUNC_NAME);
}

/* (value-div x y) -> <gdb:value> */

static SCM
gdbscm_value_div (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_DIV, x, y, FUNC_NAME);
}

/* (value-rem x y) -> <gdb:value> */

static SCM
gdbscm_value_rem (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_REM, x, y, FUNC_NAME);
}

/* (value-mod x y) -> <gdb:value> */

static SCM
gdbscm_value_mod (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_MOD, x, y, FUNC_NAME);
}

/* (value-pow x y) -> <gdb:value> */

static SCM
gdbscm_value_pow (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_POW, x, y, FUNC_NAME);
}

/* (value-neg x) -> <gdb:value> */

static SCM
gdbscm_value_neg (SCM x)
{
  return vlscm_unop (VALSCM_NEG, x, FUNC_NAME);
}

/* (value-pos x) -> <gdb:value> */

static SCM
gdbscm_value_pos (SCM x)
{
  return vlscm_unop (VALSCM_NOP, x, FUNC_NAME);
}

/* (value-abs x) -> <gdb:value> */

static SCM
gdbscm_value_abs (SCM x)
{
  return vlscm_unop (VALSCM_ABS, x, FUNC_NAME);
}

/* (value-lsh x y) -> <gdb:value> */

static SCM
gdbscm_value_lsh (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_LSH, x, y, FUNC_NAME);
}

/* (value-rsh x y) -> <gdb:value> */

static SCM
gdbscm_value_rsh (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_RSH, x, y, FUNC_NAME);
}

/* (value-min x y) -> <gdb:value> */

static SCM
gdbscm_value_min (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_MIN, x, y, FUNC_NAME);
}

/* (value-max x y) -> <gdb:value> */

static SCM
gdbscm_value_max (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_MAX, x, y, FUNC_NAME);
}

/* (value-not x) -> <gdb:value> */

static SCM
gdbscm_value_not (SCM x)
{
  return vlscm_unop (VALSCM_NOT, x, FUNC_NAME);
}

/* (value-lognot x) -> <gdb:value> */

static SCM
gdbscm_value_lognot (SCM x)
{
  return vlscm_unop (VALSCM_LOGNOT, x, FUNC_NAME);
}

/* (value-logand x y) -> <gdb:value> */

static SCM
gdbscm_value_logand (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_BITAND, x, y, FUNC_NAME);
}

/* (value-logior x y) -> <gdb:value> */

static SCM
gdbscm_value_logior (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_BITOR, x, y, FUNC_NAME);
}

/* (value-logxor x y) -> <gdb:value> */

static SCM
gdbscm_value_logxor (SCM x, SCM y)
{
  return vlscm_binop (VALSCM_BITXOR, x, y, FUNC_NAME);
}

/* Utility to perform all value comparisons.
   If there's an error a Scheme exception is thrown.  */

static SCM
vlscm_rich_compare (int op, SCM x, SCM y, const char *func_name)
{
  return gdbscm_wrap ([=]
    {
      struct gdbarch *gdbarch = get_current_arch ();
      const struct language_defn *language = current_language;
      SCM except_scm;

      scoped_value_mark free_values;

      value *v1
	= vlscm_convert_value_from_scheme (func_name, SCM_ARG1, x,
					   &except_scm, gdbarch, language);
      if (v1 == NULL)
	return except_scm;

      value *v2
	= vlscm_convert_value_from_scheme (func_name, SCM_ARG2, y,
					   &except_scm, gdbarch, language);
      if (v2 == NULL)
	return except_scm;

      int result;
      switch (op)
	{
	case BINOP_LESS:
	  result = value_less (v1, v2);
	  break;
	case BINOP_LEQ:
	  result = (value_less (v1, v2)
		    || value_equal (v1, v2));
	  break;
	case BINOP_EQUAL:
	  result = value_equal (v1, v2);
	  break;
	case BINOP_NOTEQUAL:
	  gdb_assert_not_reached ("not-equal not implemented");
	case BINOP_GTR:
	  result = value_less (v2, v1);
	  break;
	case BINOP_GEQ:
	  result = (value_less (v2, v1)
		    || value_equal (v1, v2));
	  break;
	default:
	  gdb_assert_not_reached ("invalid <gdb:value> comparison");
	}
      return scm_from_bool (result);
    });
}

/* (value=? x y) -> boolean
   There is no "not-equal?" function (value!= ?) on purpose.
   We're following string=?, etc. as our Guide here.  */

static SCM
gdbscm_value_eq_p (SCM x, SCM y)
{
  return vlscm_rich_compare (BINOP_EQUAL, x, y, FUNC_NAME);
}

/* (value<? x y) -> boolean */

static SCM
gdbscm_value_lt_p (SCM x, SCM y)
{
  return vlscm_rich_compare (BINOP_LESS, x, y, FUNC_NAME);
}

/* (value<=? x y) -> boolean */

static SCM
gdbscm_value_le_p (SCM x, SCM y)
{
  return vlscm_rich_compare (BINOP_LEQ, x, y, FUNC_NAME);
}

/* (value>? x y) -> boolean */

static SCM
gdbscm_value_gt_p (SCM x, SCM y)
{
  return vlscm_rich_compare (BINOP_GTR, x, y, FUNC_NAME);
}

/* (value>=? x y) -> boolean */

static SCM
gdbscm_value_ge_p (SCM x, SCM y)
{
  return vlscm_rich_compare (BINOP_GEQ, x, y, FUNC_NAME);
}

/* Subroutine of vlscm_convert_typed_value_from_scheme to simplify it.
   Convert OBJ, a Scheme number, to a <gdb:value> object.
   OBJ_ARG_POS is its position in the argument list, used in exception text.

   TYPE is the result type.  TYPE_ARG_POS is its position in
   the argument list, used in exception text.
   TYPE_SCM is Scheme object wrapping TYPE, used in exception text.

   If the number isn't representable, e.g. it's too big, a <gdb:exception>
   object is stored in *EXCEPT_SCMP and NULL is returned.
   The conversion may throw a gdb error, e.g., if TYPE is invalid.  */

static struct value *
vlscm_convert_typed_number (const char *func_name, int obj_arg_pos, SCM obj,
			    int type_arg_pos, SCM type_scm, struct type *type,
			    struct gdbarch *gdbarch, SCM *except_scmp)
{
  if (is_integral_type (type))
    {
      if (type->is_unsigned ())
	{
	  ULONGEST max = get_unsigned_type_max (type);
	  if (!scm_is_unsigned_integer (obj, 0, max))
	    {
	      *except_scmp
		= gdbscm_make_out_of_range_error
		    (func_name, obj_arg_pos, obj,
		     _("value out of range for type"));
	      return NULL;
	    }
	  return value_from_longest (type, gdbscm_scm_to_ulongest (obj));
	}
      else
	{
	  LONGEST min, max;

	  get_signed_type_minmax (type, &min, &max);
	  if (!scm_is_signed_integer (obj, min, max))
	    {
	      *except_scmp
		= gdbscm_make_out_of_range_error
		    (func_name, obj_arg_pos, obj,
		     _("value out of range for type"));
	      return NULL;
	    }
	  return value_from_longest (type, gdbscm_scm_to_longest (obj));
	}
    }
  else if (type->code () == TYPE_CODE_PTR)
    {
      CORE_ADDR max = get_pointer_type_max (type);
      if (!scm_is_unsigned_integer (obj, 0, max))
	{
	  *except_scmp
	    = gdbscm_make_out_of_range_error
		(func_name, obj_arg_pos, obj,
		 _("value out of range for type"));
	  return NULL;
	}
      return value_from_pointer (type, gdbscm_scm_to_ulongest (obj));
    }
  else if (type->code () == TYPE_CODE_FLT)
    return value_from_host_double (type, scm_to_double (obj));
  else
    {
      *except_scmp = gdbscm_make_type_error (func_name, obj_arg_pos, obj,
					     NULL);
      return NULL;
    }
}

/* Return non-zero if OBJ, an integer, fits in TYPE.  */

static int
vlscm_integer_fits_p (SCM obj, struct type *type)
{
  if (type->is_unsigned ())
    {
      /* If scm_is_unsigned_integer can't work with this type, just punt.  */
      if (type->length () > sizeof (uintmax_t))
	return 0;

      ULONGEST max = get_unsigned_type_max (type);
      return scm_is_unsigned_integer (obj, 0, max);
    }
  else
    {
      LONGEST min, max;

      /* If scm_is_signed_integer can't work with this type, just punt.  */
      if (type->length () > sizeof (intmax_t))
	return 0;
      get_signed_type_minmax (type, &min, &max);
      return scm_is_signed_integer (obj, min, max);
    }
}

/* Subroutine of vlscm_convert_typed_value_from_scheme to simplify it.
   Convert OBJ, a Scheme number, to a <gdb:value> object.
   OBJ_ARG_POS is its position in the argument list, used in exception text.

   If OBJ is an integer, then the smallest int that will hold the value in
   the following progression is chosen:
   int, unsigned int, long, unsigned long, long long, unsigned long long.
   Otherwise, if OBJ is a real number, then it is converted to a double.
   Otherwise an exception is thrown.

   If the number isn't representable, e.g. it's too big, a <gdb:exception>
   object is stored in *EXCEPT_SCMP and NULL is returned.  */

static struct value *
vlscm_convert_number (const char *func_name, int obj_arg_pos, SCM obj,
		      struct gdbarch *gdbarch, SCM *except_scmp)
{
  const struct builtin_type *bt = builtin_type (gdbarch);

  /* One thing to keep in mind here is that we are interested in the
     target's representation of OBJ, not the host's.  */

  if (scm_is_exact (obj) && scm_is_integer (obj))
    {
      if (vlscm_integer_fits_p (obj, bt->builtin_int))
	return value_from_longest (bt->builtin_int,
				   gdbscm_scm_to_longest (obj));
      if (vlscm_integer_fits_p (obj, bt->builtin_unsigned_int))
	return value_from_longest (bt->builtin_unsigned_int,
				   gdbscm_scm_to_ulongest (obj));
      if (vlscm_integer_fits_p (obj, bt->builtin_long))
	return value_from_longest (bt->builtin_long,
				   gdbscm_scm_to_longest (obj));
      if (vlscm_integer_fits_p (obj, bt->builtin_unsigned_long))
	return value_from_longest (bt->builtin_unsigned_long,
				   gdbscm_scm_to_ulongest (obj));
      if (vlscm_integer_fits_p (obj, bt->builtin_long_long))
	return value_from_longest (bt->builtin_long_long,
				   gdbscm_scm_to_longest (obj));
      if (vlscm_integer_fits_p (obj, bt->builtin_unsigned_long_long))
	return value_from_longest (bt->builtin_unsigned_long_long,
				   gdbscm_scm_to_ulongest (obj));
    }
  else if (scm_is_real (obj))
    return value_from_host_double (bt->builtin_double, scm_to_double (obj));

  *except_scmp = gdbscm_make_out_of_range_error (func_name, obj_arg_pos, obj,
			_("value not a number representable on the target"));
  return NULL;
}

/* Subroutine of vlscm_convert_typed_value_from_scheme to simplify it.
   Convert BV, a Scheme bytevector, to a <gdb:value> object.

   TYPE, if non-NULL, is the result type.  Otherwise, a vector of type
   uint8_t is used.
   TYPE_SCM is Scheme object wrapping TYPE, used in exception text,
   or #f if TYPE is NULL.

   If the bytevector isn't the same size as the type, then a <gdb:exception>
   object is stored in *EXCEPT_SCMP, and NULL is returned.  */

static struct value *
vlscm_convert_bytevector (SCM bv, struct type *type, SCM type_scm,
			  int arg_pos, const char *func_name,
			  SCM *except_scmp, struct gdbarch *gdbarch)
{
  LONGEST length = SCM_BYTEVECTOR_LENGTH (bv);
  struct value *value;

  if (type == NULL)
    {
      type = builtin_type (gdbarch)->builtin_uint8;
      type = lookup_array_range_type (type, 0, length);
      make_vector_type (type);
    }
  type = check_typedef (type);
  if (type->length () != length)
    {
      *except_scmp = gdbscm_make_out_of_range_error (func_name, arg_pos,
						     type_scm,
			_("size of type does not match size of bytevector"));
      return NULL;
    }

  value = value_from_contents (type,
			       (gdb_byte *) SCM_BYTEVECTOR_CONTENTS (bv));
  return value;
}

/* Convert OBJ, a Scheme value, to a <gdb:value> object.
   OBJ_ARG_POS is its position in the argument list, used in exception text.

   TYPE, if non-NULL, is the result type which must be compatible with
   the value being converted.
   If TYPE is NULL then a suitable default type is chosen.
   TYPE_SCM is Scheme object wrapping TYPE, used in exception text,
   or SCM_UNDEFINED if TYPE is NULL.
   TYPE_ARG_POS is its position in the argument list, used in exception text,
   or -1 if TYPE is NULL.

   OBJ may also be a <gdb:value> object, in which case a copy is returned
   and TYPE must be NULL.

   If the value cannot be converted, NULL is returned and a gdb:exception
   object is stored in *EXCEPT_SCMP.
   Otherwise the new value is returned, added to the all_values chain.  */

struct value *
vlscm_convert_typed_value_from_scheme (const char *func_name,
				       int obj_arg_pos, SCM obj,
				       int type_arg_pos, SCM type_scm,
				       struct type *type,
				       SCM *except_scmp,
				       struct gdbarch *gdbarch,
				       const struct language_defn *language)
{
  struct value *value = NULL;
  SCM except_scm = SCM_BOOL_F;

  if (type == NULL)
    {
      gdb_assert (type_arg_pos == -1);
      gdb_assert (SCM_UNBNDP (type_scm));
    }

  *except_scmp = SCM_BOOL_F;

  try
    {
      if (vlscm_is_value (obj))
	{
	  if (type != NULL)
	    {
	      except_scm = gdbscm_make_misc_error (func_name, type_arg_pos,
						   type_scm,
						   _("No type allowed"));
	      value = NULL;
	    }
	  else
	    value = vlscm_scm_to_value (obj)->copy ();
	}
      else if (gdbscm_is_true (scm_bytevector_p (obj)))
	{
	  value = vlscm_convert_bytevector (obj, type, type_scm,
					    obj_arg_pos, func_name,
					    &except_scm, gdbarch);
	}
      else if (gdbscm_is_bool (obj)) 
	{
	  if (type != NULL
	      && !is_integral_type (type))
	    {
	      except_scm = gdbscm_make_type_error (func_name, type_arg_pos,
						   type_scm, NULL);
	    }
	  else
	    {
	      value = value_from_longest (type
					  ? type
					  : language_bool_type (language,
								gdbarch),
					  gdbscm_is_true (obj));
	    }
	}
      else if (scm_is_number (obj))
	{
	  if (type != NULL)
	    {
	      value = vlscm_convert_typed_number (func_name, obj_arg_pos, obj,
						  type_arg_pos, type_scm, type,
						  gdbarch, &except_scm);
	    }
	  else
	    {
	      value = vlscm_convert_number (func_name, obj_arg_pos, obj,
					    gdbarch, &except_scm);
	    }
	}
      else if (scm_is_string (obj))
	{
	  size_t len;

	  if (type != NULL)
	    {
	      except_scm = gdbscm_make_misc_error (func_name, type_arg_pos,
						   type_scm,
						   _("No type allowed"));
	      value = NULL;
	    }
	  else
	    {
	      /* TODO: Provide option to specify conversion strategy.  */
	      gdb::unique_xmalloc_ptr<char> s
		= gdbscm_scm_to_string (obj, &len,
					target_charset (gdbarch),
					0 /*non-strict*/,
					&except_scm);
	      if (s != NULL)
		value = language->value_string (gdbarch, s.get (), len);
	      else
		value = NULL;
	    }
	}
      else if (lsscm_is_lazy_string (obj))
	{
	  if (type != NULL)
	    {
	      except_scm = gdbscm_make_misc_error (func_name, type_arg_pos,
						   type_scm,
						   _("No type allowed"));
	      value = NULL;
	    }
	  else
	    {
	      value = lsscm_safe_lazy_string_to_value (obj, obj_arg_pos,
						       func_name,
						       &except_scm);
	    }
	}
      else /* OBJ isn't anything we support.  */
	{
	  except_scm = gdbscm_make_type_error (func_name, obj_arg_pos, obj,
					       NULL);
	  value = NULL;
	}
    }
  catch (const gdb_exception &except)
    {
      except_scm = gdbscm_scm_from_gdb_exception (unpack (except));
    }

  if (gdbscm_is_true (except_scm))
    {
      gdb_assert (value == NULL);
      *except_scmp = except_scm;
    }

  return value;
}

/* Wrapper around vlscm_convert_typed_value_from_scheme for cases where there
   is no supplied type.  See vlscm_convert_typed_value_from_scheme for
   details.  */

struct value *
vlscm_convert_value_from_scheme (const char *func_name,
				 int obj_arg_pos, SCM obj,
				 SCM *except_scmp, struct gdbarch *gdbarch,
				 const struct language_defn *language)
{
  return vlscm_convert_typed_value_from_scheme (func_name, obj_arg_pos, obj,
						-1, SCM_UNDEFINED, NULL,
						except_scmp,
						gdbarch, language);
}

/* Initialize value math support.  */

static const scheme_function math_functions[] =
{
  { "value-add", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_add),
    "\
Return a + b." },

  { "value-sub", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_sub),
    "\
Return a - b." },

  { "value-mul", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_mul),
    "\
Return a * b." },

  { "value-div", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_div),
    "\
Return a / b." },

  { "value-rem", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_rem),
    "\
Return a % b." },

  { "value-mod", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_mod),
    "\
Return a mod b.  See Knuth 1.2.4." },

  { "value-pow", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_pow),
    "\
Return pow (x, y)." },

  { "value-not", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_not),
    "\
Return !a." },

  { "value-neg", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_neg),
    "\
Return -a." },

  { "value-pos", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_pos),
    "\
Return a." },

  { "value-abs", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_abs),
    "\
Return abs (a)." },

  { "value-lsh", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_lsh),
    "\
Return a << b." },

  { "value-rsh", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_rsh),
    "\
Return a >> b." },

  { "value-min", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_min),
    "\
Return min (a, b)." },

  { "value-max", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_max),
    "\
Return max (a, b)." },

  { "value-lognot", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_lognot),
    "\
Return ~a." },

  { "value-logand", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_logand),
    "\
Return a & b." },

  { "value-logior", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_logior),
    "\
Return a | b." },

  { "value-logxor", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_logxor),
    "\
Return a ^ b." },

  { "value=?", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_eq_p),
    "\
Return a == b." },

  { "value<?", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_lt_p),
    "\
Return a < b." },

  { "value<=?", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_le_p),
    "\
Return a <= b." },

  { "value>?", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_gt_p),
    "\
Return a > b." },

  { "value>=?", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_ge_p),
    "\
Return a >= b." },

  END_FUNCTIONS
};

void
gdbscm_initialize_math (void)
{
  gdbscm_define_functions (math_functions, 1);
}

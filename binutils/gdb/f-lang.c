/* Fortran language support routines for GDB, the GNU debugger.

   Copyright (C) 1993-2024 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C parser by Farooq Butt
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

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "gdbcore.h"
#include "f-lang.h"
#include "valprint.h"
#include "value.h"
#include "cp-support.h"
#include "charset.h"
#include "c-lang.h"
#include "target-float.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "f-array-walker.h"
#include "f-exp.h"

#include <math.h>

/* Whether GDB should repack array slices created by the user.  */
static bool repack_array_slices = false;

/* Implement 'show fortran repack-array-slices'.  */
static void
show_repack_array_slices (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Repacking of Fortran array slices is %s.\n"),
	      value);
}

/* Debugging of Fortran's array slicing.  */
static bool fortran_array_slicing_debug = false;

/* Implement 'show debug fortran-array-slicing'.  */
static void
show_fortran_array_slicing_debug (struct ui_file *file, int from_tty,
				  struct cmd_list_element *c,
				  const char *value)
{
  gdb_printf (file, _("Debugging of Fortran array slicing is %s.\n"),
	      value);
}

/* Local functions */

static value *fortran_prepare_argument (struct expression *exp,
					expr::operation *subexp,
					int arg_num, bool is_internal_call_p,
					struct type *func_type, enum noside noside);

/* Return the encoding that should be used for the character type
   TYPE.  */

const char *
f_language::get_encoding (struct type *type)
{
  const char *encoding;

  switch (type->length ())
    {
    case 1:
      encoding = target_charset (type->arch ());
      break;
    case 4:
      if (type_byte_order (type) == BFD_ENDIAN_BIG)
	encoding = "UTF-32BE";
      else
	encoding = "UTF-32LE";
      break;

    default:
      error (_("unrecognized character type"));
    }

  return encoding;
}

/* See language.h.  */

struct value *
f_language::value_string (struct gdbarch *gdbarch,
			  const char *ptr, ssize_t len) const
{
  struct type *type = language_string_char_type (this, gdbarch);
  return ::value_string (ptr, len, type);
}

/* A helper function for the "bound" intrinsics that checks that TYPE
   is an array.  LBOUND_P is true for lower bound; this is used for
   the error message, if any.  */

static void
fortran_require_array (struct type *type, bool lbound_p)
{
  type = check_typedef (type);
  if (type->code () != TYPE_CODE_ARRAY)
    {
      if (lbound_p)
	error (_("LBOUND can only be applied to arrays"));
      else
	error (_("UBOUND can only be applied to arrays"));
    }
}

/* Create an array containing the lower bounds (when LBOUND_P is true) or
   the upper bounds (when LBOUND_P is false) of ARRAY (which must be of
   array type).  GDBARCH is the current architecture.  */

static struct value *
fortran_bounds_all_dims (bool lbound_p,
			 struct gdbarch *gdbarch,
			 struct value *array)
{
  type *array_type = check_typedef (array->type ());
  int ndimensions = calc_f77_array_dims (array_type);

  /* Allocate a result value of the correct type.  */
  type_allocator alloc (gdbarch);
  struct type *range
    = create_static_range_type (alloc,
				builtin_f_type (gdbarch)->builtin_integer,
				1, ndimensions);
  struct type *elm_type = builtin_f_type (gdbarch)->builtin_integer;
  struct type *result_type = create_array_type (alloc, elm_type, range);
  struct value *result = value::allocate (result_type);

  /* Walk the array dimensions backwards due to the way the array will be
     laid out in memory, the first dimension will be the most inner.  */
  LONGEST elm_len = elm_type->length ();
  for (LONGEST dst_offset = elm_len * (ndimensions - 1);
       dst_offset >= 0;
       dst_offset -= elm_len)
    {
      LONGEST b;

      /* Grab the required bound.  */
      if (lbound_p)
	b = f77_get_lowerbound (array_type);
      else
	b = f77_get_upperbound (array_type);

      /* And copy the value into the result value.  */
      struct value *v = value_from_longest (elm_type, b);
      gdb_assert (dst_offset + v->type ()->length ()
		  <= result->type ()->length ());
      gdb_assert (v->type ()->length () == elm_len);
      v->contents_copy (result, dst_offset, 0, elm_len);

      /* Peel another dimension of the array.  */
      array_type = array_type->target_type ();
    }

  return result;
}

/* Return the lower bound (when LBOUND_P is true) or the upper bound (when
   LBOUND_P is false) for dimension DIM_VAL (which must be an integer) of
   ARRAY (which must be an array).  RESULT_TYPE corresponds to the type kind
   the function should be evaluated in.  */

static value *
fortran_bounds_for_dimension (bool lbound_p, value *array, value *dim_val,
			      type* result_type)
{
  /* Check the requested dimension is valid for this array.  */
  type *array_type = check_typedef (array->type ());
  int ndimensions = calc_f77_array_dims (array_type);
  long dim = value_as_long (dim_val);
  if (dim < 1 || dim > ndimensions)
    {
      if (lbound_p)
	error (_("LBOUND dimension must be from 1 to %d"), ndimensions);
      else
	error (_("UBOUND dimension must be from 1 to %d"), ndimensions);
    }

  /* Walk the dimensions backwards, due to the ordering in which arrays are
     laid out the first dimension is the most inner.  */
  for (int i = ndimensions - 1; i >= 0; --i)
    {
      /* If this is the requested dimension then we're done.  Grab the
	 bounds and return.  */
      if (i == dim - 1)
	{
	  LONGEST b;

	  if (lbound_p)
	    b = f77_get_lowerbound (array_type);
	  else
	    b = f77_get_upperbound (array_type);

	  return value_from_longest (result_type, b);
	}

      /* Peel off another dimension of the array.  */
      array_type = array_type->target_type ();
    }

  gdb_assert_not_reached ("failed to find matching dimension");
}

/* Return the number of dimensions for a Fortran array or string.  */

int
calc_f77_array_dims (struct type *array_type)
{
  int ndimen = 1;
  struct type *tmp_type;

  if ((array_type->code () == TYPE_CODE_STRING))
    return 1;

  if ((array_type->code () != TYPE_CODE_ARRAY))
    error (_("Can't get dimensions for a non-array type"));

  tmp_type = array_type;

  while ((tmp_type = tmp_type->target_type ()))
    {
      if (tmp_type->code () == TYPE_CODE_ARRAY)
	++ndimen;
    }
  return ndimen;
}

/* A class used by FORTRAN_VALUE_SUBARRAY when repacking Fortran array
   slices.  This is a base class for two alternative repacking mechanisms,
   one for when repacking from a lazy value, and one for repacking from a
   non-lazy (already loaded) value.  */
class fortran_array_repacker_base_impl
  : public fortran_array_walker_base_impl
{
public:
  /* Constructor, DEST is the value we are repacking into.  */
  fortran_array_repacker_base_impl (struct value *dest)
    : m_dest (dest),
      m_dest_offset (0)
  { /* Nothing.  */ }

  /* When we start processing the inner most dimension, this is where we
     will be creating values for each element as we load them and then copy
     them into the M_DEST value.  Set a value mark so we can free these
     temporary values.  */
  void start_dimension (struct type *index_type, LONGEST nelts, bool inner_p)
  {
    if (inner_p)
      {
	gdb_assert (!m_mark.has_value ());
	m_mark.emplace ();
      }
  }

  /* When we finish processing the inner most dimension free all temporary
     value that were created.  */
  void finish_dimension (bool inner_p, bool last_p)
  {
    if (inner_p)
      {
	gdb_assert (m_mark.has_value ());
	m_mark.reset ();
      }
  }

protected:
  /* Copy the contents of array element ELT into M_DEST at the next
     available offset.  */
  void copy_element_to_dest (struct value *elt)
  {
    elt->contents_copy (m_dest, m_dest_offset, 0,
			elt->type ()->length ());
    m_dest_offset += elt->type ()->length ();
  }

  /* The value being written to.  */
  struct value *m_dest;

  /* The byte offset in M_DEST at which the next element should be
     written.  */
  LONGEST m_dest_offset;

  /* Set and reset to handle removing intermediate values from the
     value chain.  */
  std::optional<scoped_value_mark> m_mark;
};

/* A class used by FORTRAN_VALUE_SUBARRAY when repacking Fortran array
   slices.  This class is specialised for repacking an array slice from a
   lazy array value, as such it does not require the parent array value to
   be loaded into GDB's memory; the parent value could be huge, while the
   slice could be tiny.  */
class fortran_lazy_array_repacker_impl
  : public fortran_array_repacker_base_impl
{
public:
  /* Constructor.  TYPE is the type of the slice being loaded from the
     parent value, so this type will correctly reflect the strides required
     to find all of the elements from the parent value.  ADDRESS is the
     address in target memory of value matching TYPE, and DEST is the value
     we are repacking into.  */
  explicit fortran_lazy_array_repacker_impl (struct type *type,
					     CORE_ADDR address,
					     struct value *dest)
    : fortran_array_repacker_base_impl (dest),
      m_addr (address)
  { /* Nothing.  */ }

  /* Create a lazy value in target memory representing a single element,
     then load the element into GDB's memory and copy the contents into the
     destination value.  */
  void process_element (struct type *elt_type, LONGEST elt_off,
			LONGEST index, bool last_p)
  {
    copy_element_to_dest (value_at_lazy (elt_type, m_addr + elt_off));
  }

private:
  /* The address in target memory where the parent value starts.  */
  CORE_ADDR m_addr;
};

/* A class used by FORTRAN_VALUE_SUBARRAY when repacking Fortran array
   slices.  This class is specialised for repacking an array slice from a
   previously loaded (non-lazy) array value, as such it fetches the
   element values from the contents of the parent value.  */
class fortran_array_repacker_impl
  : public fortran_array_repacker_base_impl
{
public:
  /* Constructor.  TYPE is the type for the array slice within the parent
     value, as such it has stride values as required to find the elements
     within the original parent value.  ADDRESS is the address in target
     memory of the value matching TYPE.  BASE_OFFSET is the offset from
     the start of VAL's content buffer to the start of the object of TYPE,
     VAL is the parent object from which we are loading the value, and
     DEST is the value into which we are repacking.  */
  explicit fortran_array_repacker_impl (struct type *type, CORE_ADDR address,
					LONGEST base_offset,
					struct value *val, struct value *dest)
    : fortran_array_repacker_base_impl (dest),
      m_base_offset (base_offset),
      m_val (val)
  {
    gdb_assert (!val->lazy ());
  }

  /* Extract an element of ELT_TYPE at offset (M_BASE_OFFSET + ELT_OFF)
     from the content buffer of M_VAL then copy this extracted value into
     the repacked destination value.  */
  void process_element (struct type *elt_type, LONGEST elt_off,
			LONGEST index, bool last_p)
  {
    struct value *elt
      = value_from_component (m_val, elt_type, (elt_off + m_base_offset));
    copy_element_to_dest (elt);
  }

private:
  /* The offset into the content buffer of M_VAL to the start of the slice
     being extracted.  */
  LONGEST m_base_offset;

  /* The parent value from which we are extracting a slice.  */
  struct value *m_val;
};


/* Evaluate FORTRAN_ASSOCIATED expressions.  Both GDBARCH and LANG are
   extracted from the expression being evaluated.  POINTER is the required
   first argument to the 'associated' keyword, and TARGET is the optional
   second argument, this will be nullptr if the user only passed one
   argument to their use of 'associated'.  */

static struct value *
fortran_associated (struct gdbarch *gdbarch, const language_defn *lang,
		    struct value *pointer, struct value *target = nullptr)
{
  struct type *result_type = language_bool_type (lang, gdbarch);

  /* All Fortran pointers should have the associated property, this is
     how we know the pointer is pointing at something or not.  */
  struct type *pointer_type = check_typedef (pointer->type ());
  if (TYPE_ASSOCIATED_PROP (pointer_type) == nullptr
      && pointer_type->code () != TYPE_CODE_PTR)
    error (_("ASSOCIATED can only be applied to pointers"));

  /* Get an address from POINTER.  Fortran (or at least gfortran) models
     array pointers as arrays with a dynamic data address, so we need to
     use two approaches here, for real pointers we take the contents of the
     pointer as an address.  For non-pointers we take the address of the
     content.  */
  CORE_ADDR pointer_addr;
  if (pointer_type->code () == TYPE_CODE_PTR)
    pointer_addr = value_as_address (pointer);
  else
    pointer_addr = pointer->address ();

  /* The single argument case, is POINTER associated with anything?  */
  if (target == nullptr)
    {
      bool is_associated = false;

      /* If POINTER is an actual pointer and doesn't have an associated
	 property then we need to figure out whether this pointer is
	 associated by looking at the value of the pointer itself.  We make
	 the assumption that a non-associated pointer will be set to 0.
	 This is probably true for most targets, but might not be true for
	 everyone.  */
      if (pointer_type->code () == TYPE_CODE_PTR
	  && TYPE_ASSOCIATED_PROP (pointer_type) == nullptr)
	is_associated = (pointer_addr != 0);
      else
	is_associated = !type_not_associated (pointer_type);
      return value_from_longest (result_type, is_associated ? 1 : 0);
    }

  /* The two argument case, is POINTER associated with TARGET?  */

  struct type *target_type = check_typedef (target->type ());

  struct type *pointer_target_type;
  if (pointer_type->code () == TYPE_CODE_PTR)
    pointer_target_type = pointer_type->target_type ();
  else
    pointer_target_type = pointer_type;

  struct type *target_target_type;
  if (target_type->code () == TYPE_CODE_PTR)
    target_target_type = target_type->target_type ();
  else
    target_target_type = target_type;

  if (pointer_target_type->code () != target_target_type->code ()
      || (pointer_target_type->code () != TYPE_CODE_ARRAY
	  && (pointer_target_type->length ()
	      != target_target_type->length ())))
    error (_("arguments to associated must be of same type and kind"));

  /* If TARGET is not in memory, or the original pointer is specifically
     known to be not associated with anything, then the answer is obviously
     false.  Alternatively, if POINTER is an actual pointer and has no
     associated property, then we have to check if its associated by
     looking the value of the pointer itself.  We make the assumption that
     a non-associated pointer will be set to 0.  This is probably true for
     most targets, but might not be true for everyone.  */
  if (target->lval () != lval_memory
      || type_not_associated (pointer_type)
      || (TYPE_ASSOCIATED_PROP (pointer_type) == nullptr
	  && pointer_type->code () == TYPE_CODE_PTR
	  && pointer_addr == 0))
    return value_from_longest (result_type, 0);

  /* See the comment for POINTER_ADDR above.  */
  CORE_ADDR target_addr;
  if (target_type->code () == TYPE_CODE_PTR)
    target_addr = value_as_address (target);
  else
    target_addr = target->address ();

  /* Wrap the following checks inside a do { ... } while (false) loop so
     that we can use `break' to jump out of the loop.  */
  bool is_associated = false;
  do
    {
      /* If the addresses are different then POINTER is definitely not
	 pointing at TARGET.  */
      if (pointer_addr != target_addr)
	break;

      /* If POINTER is a real pointer (i.e. not an array pointer, which are
	 implemented as arrays with a dynamic content address), then this
	 is all the checking that is needed.  */
      if (pointer_type->code () == TYPE_CODE_PTR)
	{
	  is_associated = true;
	  break;
	}

      /* We have an array pointer.  Check the number of dimensions.  */
      int pointer_dims = calc_f77_array_dims (pointer_type);
      int target_dims = calc_f77_array_dims (target_type);
      if (pointer_dims != target_dims)
	break;

      /* Now check that every dimension has the same upper bound, lower
	 bound, and stride value.  */
      int dim = 0;
      while (dim < pointer_dims)
	{
	  LONGEST pointer_lowerbound, pointer_upperbound, pointer_stride;
	  LONGEST target_lowerbound, target_upperbound, target_stride;

	  pointer_type = check_typedef (pointer_type);
	  target_type = check_typedef (target_type);

	  struct type *pointer_range = pointer_type->index_type ();
	  struct type *target_range = target_type->index_type ();

	  if (!get_discrete_bounds (pointer_range, &pointer_lowerbound,
				    &pointer_upperbound))
	    break;

	  if (!get_discrete_bounds (target_range, &target_lowerbound,
				    &target_upperbound))
	    break;

	  if (pointer_lowerbound != target_lowerbound
	      || pointer_upperbound != target_upperbound)
	    break;

	  /* Figure out the stride (in bits) for both pointer and target.
	     If either doesn't have a stride then we take the element size,
	     but we need to convert to bits (hence the * 8).  */
	  pointer_stride = pointer_range->bounds ()->bit_stride ();
	  if (pointer_stride == 0)
	    pointer_stride
	      = type_length_units (check_typedef
				     (pointer_type->target_type ())) * 8;
	  target_stride = target_range->bounds ()->bit_stride ();
	  if (target_stride == 0)
	    target_stride
	      = type_length_units (check_typedef
				     (target_type->target_type ())) * 8;
	  if (pointer_stride != target_stride)
	    break;

	  ++dim;
	}

      if (dim < pointer_dims)
	break;

      is_associated = true;
    }
  while (false);

  return value_from_longest (result_type, is_associated ? 1 : 0);
}

struct value *
eval_op_f_associated (struct type *expect_type,
		      struct expression *exp,
		      enum noside noside,
		      enum exp_opcode opcode,
		      struct value *arg1)
{
  return fortran_associated (exp->gdbarch, exp->language_defn, arg1);
}

struct value *
eval_op_f_associated (struct type *expect_type,
		      struct expression *exp,
		      enum noside noside,
		      enum exp_opcode opcode,
		      struct value *arg1,
		      struct value *arg2)
{
  return fortran_associated (exp->gdbarch, exp->language_defn, arg1, arg2);
}

/* Implement FORTRAN_ARRAY_SIZE expression, this corresponds to the 'SIZE'
   keyword.  RESULT_TYPE corresponds to the type kind the function should be
   evaluated in, ARRAY is the value that should be an array, though this will
   not have been checked before calling this function.  DIM is optional, if
   present then it should be an integer identifying a dimension of the
   array to ask about.  As with ARRAY the validity of DIM is not checked
   before calling this function.

   Return either the total number of elements in ARRAY (when DIM is
   nullptr), or the number of elements in dimension DIM.  */

static value *
fortran_array_size (value *array, value *dim_val, type *result_type)
{
  /* Check that ARRAY is the correct type.  */
  struct type *array_type = check_typedef (array->type ());
  if (array_type->code () != TYPE_CODE_ARRAY)
    error (_("SIZE can only be applied to arrays"));
  if (type_not_allocated (array_type) || type_not_associated (array_type))
    error (_("SIZE can only be used on allocated/associated arrays"));

  int ndimensions = calc_f77_array_dims (array_type);
  int dim = -1;
  LONGEST result = 0;

  if (dim_val != nullptr)
    {
      if (check_typedef (dim_val->type ())->code () != TYPE_CODE_INT)
	error (_("DIM argument to SIZE must be an integer"));
      dim = (int) value_as_long (dim_val);

      if (dim < 1 || dim > ndimensions)
	error (_("DIM argument to SIZE must be between 1 and %d"),
	       ndimensions);
    }

  /* Now walk over all the dimensions of the array totalling up the
     elements in each dimension.  */
  for (int i = ndimensions - 1; i >= 0; --i)
    {
      /* If this is the requested dimension then we're done.  Grab the
	 bounds and return.  */
      if (i == dim - 1 || dim == -1)
	{
	  LONGEST lbound, ubound;
	  struct type *range = array_type->index_type ();

	  if (!get_discrete_bounds (range, &lbound, &ubound))
	    error (_("failed to find array bounds"));

	  LONGEST dim_size = (ubound - lbound + 1);
	  if (result == 0)
	    result = dim_size;
	  else
	    result *= dim_size;

	  if (dim != -1)
	    break;
	}

      /* Peel off another dimension of the array.  */
      array_type = array_type->target_type ();
    }

  return value_from_longest (result_type, result);
}

/* See f-exp.h.  */

struct value *
eval_op_f_array_size (struct type *expect_type,
		      struct expression *exp,
		      enum noside noside,
		      enum exp_opcode opcode,
		      struct value *arg1)
{
  gdb_assert (opcode == FORTRAN_ARRAY_SIZE);

  type *result_type = builtin_f_type (exp->gdbarch)->builtin_integer;
  return fortran_array_size (arg1, nullptr, result_type);
}

/* See f-exp.h.  */

struct value *
eval_op_f_array_size (struct type *expect_type,
		      struct expression *exp,
		      enum noside noside,
		      enum exp_opcode opcode,
		      struct value *arg1,
		      struct value *arg2)
{
  gdb_assert (opcode == FORTRAN_ARRAY_SIZE);

  type *result_type = builtin_f_type (exp->gdbarch)->builtin_integer;
  return fortran_array_size (arg1, arg2, result_type);
}

/* See f-exp.h.  */

value *eval_op_f_array_size (type *expect_type, expression *exp, noside noside,
			     exp_opcode opcode, value *arg1, value *arg2,
			     type *kind_arg)
{
  gdb_assert (opcode == FORTRAN_ARRAY_SIZE);
  gdb_assert (kind_arg->code () == TYPE_CODE_INT);

  return fortran_array_size (arg1, arg2, kind_arg);
}

/* Implement UNOP_FORTRAN_SHAPE expression.  Both GDBARCH and LANG are
   extracted from the expression being evaluated.  VAL is the value on
   which 'shape' was used, this can be any type.

   Return an array of integers.  If VAL is not an array then the returned
   array should have zero elements.  If VAL is an array then the returned
   array should have one element per dimension, with the element
   containing the extent of that dimension from VAL.  */

static struct value *
fortran_array_shape (struct gdbarch *gdbarch, const language_defn *lang,
		     struct value *val)
{
  struct type *val_type = check_typedef (val->type ());

  /* If we are passed an array that is either not allocated, or not
     associated, then this is explicitly not allowed according to the
     Fortran specification.  */
  if (val_type->code () == TYPE_CODE_ARRAY
      && (type_not_associated (val_type) || type_not_allocated (val_type)))
    error (_("The array passed to SHAPE must be allocated or associated"));

  /* The Fortran specification allows non-array types to be passed to this
     function, in which case we get back an empty array.

     Calculate the number of dimensions for the resulting array.  */
  int ndimensions = 0;
  if (val_type->code () == TYPE_CODE_ARRAY)
    ndimensions = calc_f77_array_dims (val_type);

  /* Allocate a result value of the correct type.  */
  type_allocator alloc (gdbarch);
  struct type *range
    = create_static_range_type (alloc,
				builtin_type (gdbarch)->builtin_int,
				1, ndimensions);
  struct type *elm_type = builtin_f_type (gdbarch)->builtin_integer;
  struct type *result_type = create_array_type (alloc, elm_type, range);
  struct value *result = value::allocate (result_type);
  LONGEST elm_len = elm_type->length ();

  /* Walk the array dimensions backwards due to the way the array will be
     laid out in memory, the first dimension will be the most inner.

     If VAL was not an array then ndimensions will be 0, in which case we
     will never go around this loop.  */
  for (LONGEST dst_offset = elm_len * (ndimensions - 1);
       dst_offset >= 0;
       dst_offset -= elm_len)
    {
      LONGEST lbound, ubound;

      if (!get_discrete_bounds (val_type->index_type (), &lbound, &ubound))
	error (_("failed to find array bounds"));

      LONGEST dim_size = (ubound - lbound + 1);

      /* And copy the value into the result value.  */
      struct value *v = value_from_longest (elm_type, dim_size);
      gdb_assert (dst_offset + v->type ()->length ()
		  <= result->type ()->length ());
      gdb_assert (v->type ()->length () == elm_len);
      v->contents_copy (result, dst_offset, 0, elm_len);

      /* Peel another dimension of the array.  */
      val_type = val_type->target_type ();
    }

  return result;
}

/* See f-exp.h.  */

struct value *
eval_op_f_array_shape (struct type *expect_type, struct expression *exp,
		       enum noside noside, enum exp_opcode opcode,
		       struct value *arg1)
{
  gdb_assert (opcode == UNOP_FORTRAN_SHAPE);
  return fortran_array_shape (exp->gdbarch, exp->language_defn, arg1);
}

/* A helper function for UNOP_ABS.  */

struct value *
eval_op_f_abs (struct type *expect_type, struct expression *exp,
	       enum noside noside,
	       enum exp_opcode opcode,
	       struct value *arg1)
{
  struct type *type = arg1->type ();
  switch (type->code ())
    {
    case TYPE_CODE_FLT:
      {
	double d
	  = fabs (target_float_to_host_double (arg1->contents ().data (),
					       arg1->type ()));
	return value_from_host_double (type, d);
      }
    case TYPE_CODE_INT:
      {
	LONGEST l = value_as_long (arg1);
	l = llabs (l);
	return value_from_longest (type, l);
      }
    }
  error (_("ABS of type %s not supported"), TYPE_SAFE_NAME (type));
}

/* A helper function for BINOP_MOD.  */

struct value *
eval_op_f_mod (struct type *expect_type, struct expression *exp,
	       enum noside noside,
	       enum exp_opcode opcode,
	       struct value *arg1, struct value *arg2)
{
  struct type *type = arg1->type ();
  if (type->code () != arg2->type ()->code ())
    error (_("non-matching types for parameters to MOD ()"));
  switch (type->code ())
    {
    case TYPE_CODE_FLT:
      {
	double d1
	  = target_float_to_host_double (arg1->contents ().data (),
					 arg1->type ());
	double d2
	  = target_float_to_host_double (arg2->contents ().data (),
					 arg2->type ());
	double d3 = fmod (d1, d2);
	return value_from_host_double (type, d3);
      }
    case TYPE_CODE_INT:
      {
	LONGEST v1 = value_as_long (arg1);
	LONGEST v2 = value_as_long (arg2);
	if (v2 == 0)
	  error (_("calling MOD (N, 0) is undefined"));
	LONGEST v3 = v1 - (v1 / v2) * v2;
	return value_from_longest (arg1->type (), v3);
      }
    }
  error (_("MOD of type %s not supported"), TYPE_SAFE_NAME (type));
}

/* A helper function for the different FORTRAN_CEILING overloads.  Calculates
   CEILING for ARG1 (a float type) and returns it in the requested kind type
   RESULT_TYPE.  */

static value *
fortran_ceil_operation (value *arg1, type *result_type)
{
  if (arg1->type ()->code () != TYPE_CODE_FLT)
    error (_("argument to CEILING must be of type float"));
  double val = target_float_to_host_double (arg1->contents ().data (),
					    arg1->type ());
  val = ceil (val);
  return value_from_longest (result_type, val);
}

/* A helper function for FORTRAN_CEILING.  */

struct value *
eval_op_f_ceil (struct type *expect_type, struct expression *exp,
		enum noside noside,
		enum exp_opcode opcode,
		struct value *arg1)
{
  gdb_assert (opcode == FORTRAN_CEILING);
  type *result_type = builtin_f_type (exp->gdbarch)->builtin_integer;
  return fortran_ceil_operation (arg1, result_type);
}

/* A helper function for FORTRAN_CEILING.  */

value *
eval_op_f_ceil (type *expect_type, expression *exp, noside noside,
		exp_opcode opcode, value *arg1, type *kind_arg)
{
  gdb_assert (opcode == FORTRAN_CEILING);
  gdb_assert (kind_arg->code () == TYPE_CODE_INT);
  return fortran_ceil_operation (arg1, kind_arg);
}

/* A helper function for the different FORTRAN_FLOOR overloads.  Calculates
   FLOOR for ARG1 (a float type) and returns it in the requested kind type
   RESULT_TYPE.  */

static value *
fortran_floor_operation (value *arg1, type *result_type)
{
  if (arg1->type ()->code () != TYPE_CODE_FLT)
    error (_("argument to FLOOR must be of type float"));
  double val = target_float_to_host_double (arg1->contents ().data (),
					    arg1->type ());
  val = floor (val);
  return value_from_longest (result_type, val);
}

/* A helper function for FORTRAN_FLOOR.  */

struct value *
eval_op_f_floor (struct type *expect_type, struct expression *exp,
		enum noside noside,
		enum exp_opcode opcode,
		struct value *arg1)
{
  gdb_assert (opcode == FORTRAN_FLOOR);
  type *result_type = builtin_f_type (exp->gdbarch)->builtin_integer;
  return fortran_floor_operation (arg1, result_type);
}

/* A helper function for FORTRAN_FLOOR.  */

struct value *
eval_op_f_floor (type *expect_type, expression *exp, noside noside,
		 exp_opcode opcode, value *arg1, type *kind_arg)
{
  gdb_assert (opcode == FORTRAN_FLOOR);
  gdb_assert (kind_arg->code () == TYPE_CODE_INT);
  return fortran_floor_operation (arg1, kind_arg);
}

/* A helper function for BINOP_FORTRAN_MODULO.  */

struct value *
eval_op_f_modulo (struct type *expect_type, struct expression *exp,
		  enum noside noside,
		  enum exp_opcode opcode,
		  struct value *arg1, struct value *arg2)
{
  struct type *type = arg1->type ();
  if (type->code () != arg2->type ()->code ())
    error (_("non-matching types for parameters to MODULO ()"));
  /* MODULO(A, P) = A - FLOOR (A / P) * P */
  switch (type->code ())
    {
    case TYPE_CODE_INT:
      {
	LONGEST a = value_as_long (arg1);
	LONGEST p = value_as_long (arg2);
	LONGEST result = a - (a / p) * p;
	if (result != 0 && (a < 0) != (p < 0))
	  result += p;
	return value_from_longest (arg1->type (), result);
      }
    case TYPE_CODE_FLT:
      {
	double a
	  = target_float_to_host_double (arg1->contents ().data (),
					 arg1->type ());
	double p
	  = target_float_to_host_double (arg2->contents ().data (),
					 arg2->type ());
	double result = fmod (a, p);
	if (result != 0 && (a < 0.0) != (p < 0.0))
	  result += p;
	return value_from_host_double (type, result);
      }
    }
  error (_("MODULO of type %s not supported"), TYPE_SAFE_NAME (type));
}

/* A helper function for FORTRAN_CMPLX.  */

value *
eval_op_f_cmplx (type *expect_type, expression *exp, noside noside,
		 exp_opcode opcode, value *arg1)
{
  gdb_assert (opcode == FORTRAN_CMPLX);

  type *result_type = builtin_f_type (exp->gdbarch)->builtin_complex;

  if (arg1->type ()->code () == TYPE_CODE_COMPLEX)
    return value_cast (result_type, arg1);
  else
    return value_literal_complex (arg1,
				  value::zero (arg1->type (), not_lval),
				  result_type);
}

/* A helper function for FORTRAN_CMPLX.  */

struct value *
eval_op_f_cmplx (struct type *expect_type, struct expression *exp,
		 enum noside noside,
		 enum exp_opcode opcode,
		 struct value *arg1, struct value *arg2)
{
  if (arg1->type ()->code () == TYPE_CODE_COMPLEX
      || arg2->type ()->code () == TYPE_CODE_COMPLEX)
    error (_("Types of arguments for CMPLX called with more then one argument "
	     "must be REAL or INTEGER"));

  type *result_type = builtin_f_type (exp->gdbarch)->builtin_complex;
  return value_literal_complex (arg1, arg2, result_type);
}

/* A helper function for FORTRAN_CMPLX.  */

value *
eval_op_f_cmplx (type *expect_type, expression *exp, noside noside,
		 exp_opcode opcode, value *arg1, value *arg2, type *kind_arg)
{
  gdb_assert (kind_arg->code () == TYPE_CODE_COMPLEX);
  if (arg1->type ()->code () == TYPE_CODE_COMPLEX
      || arg2->type ()->code () == TYPE_CODE_COMPLEX)
    error (_("Types of arguments for CMPLX called with more then one argument "
	     "must be REAL or INTEGER"));

  return value_literal_complex (arg1, arg2, kind_arg);
}

/* A helper function for UNOP_FORTRAN_KIND.  */

struct value *
eval_op_f_kind (struct type *expect_type, struct expression *exp,
		enum noside noside,
		enum exp_opcode opcode,
		struct value *arg1)
{
  struct type *type = arg1->type ();

  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_MODULE:
    case TYPE_CODE_FUNC:
      error (_("argument to kind must be an intrinsic type"));
    }

  if (!type->target_type ())
    return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
			       type->length ());
  return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
			     type->target_type ()->length ());
}

/* A helper function for UNOP_FORTRAN_ALLOCATED.  */

struct value *
eval_op_f_allocated (struct type *expect_type, struct expression *exp,
		     enum noside noside, enum exp_opcode op,
		     struct value *arg1)
{
  struct type *type = check_typedef (arg1->type ());
  if (type->code () != TYPE_CODE_ARRAY)
    error (_("ALLOCATED can only be applied to arrays"));
  struct type *result_type
    = builtin_f_type (exp->gdbarch)->builtin_logical;
  LONGEST result_value = type_not_allocated (type) ? 0 : 1;
  return value_from_longest (result_type, result_value);
}

/* See f-exp.h.  */

struct value *
eval_op_f_rank (struct type *expect_type,
		struct expression *exp,
		enum noside noside,
		enum exp_opcode op,
		struct value *arg1)
{
  gdb_assert (op == UNOP_FORTRAN_RANK);

  struct type *result_type
    = builtin_f_type (exp->gdbarch)->builtin_integer;
  struct type *type = check_typedef (arg1->type ());
  if (type->code () != TYPE_CODE_ARRAY)
    return value_from_longest (result_type, 0);
  LONGEST ndim = calc_f77_array_dims (type);
  return value_from_longest (result_type, ndim);
}

/* A helper function for UNOP_FORTRAN_LOC.  */

struct value *
eval_op_f_loc (struct type *expect_type, struct expression *exp,
		     enum noside noside, enum exp_opcode op,
		     struct value *arg1)
{
  struct type *result_type;
  if (gdbarch_ptr_bit (exp->gdbarch) == 16)
    result_type = builtin_f_type (exp->gdbarch)->builtin_integer_s2;
  else if (gdbarch_ptr_bit (exp->gdbarch) == 32)
    result_type = builtin_f_type (exp->gdbarch)->builtin_integer;
  else
    result_type = builtin_f_type (exp->gdbarch)->builtin_integer_s8;

  LONGEST result_value = arg1->address ();
  return value_from_longest (result_type, result_value);
}

namespace expr
{

/* Called from evaluate to perform array indexing, and sub-range
   extraction, for Fortran.  As well as arrays this function also
   handles strings as they can be treated like arrays of characters.
   ARRAY is the array or string being accessed.  EXP and NOSIDE are as
   for evaluate.  */

value *
fortran_undetermined::value_subarray (value *array,
				      struct expression *exp,
				      enum noside noside)
{
  type *original_array_type = check_typedef (array->type ());
  bool is_string_p = original_array_type->code () == TYPE_CODE_STRING;
  const std::vector<operation_up> &ops = std::get<1> (m_storage);
  int nargs = ops.size ();

  /* Perform checks for ARRAY not being available.  The somewhat overly
     complex logic here is just to keep backward compatibility with the
     errors that we used to get before FORTRAN_VALUE_SUBARRAY was
     rewritten.  Maybe a future task would streamline the error messages we
     get here, and update all the expected test results.  */
  if (ops[0]->opcode () != OP_RANGE)
    {
      if (type_not_associated (original_array_type))
	error (_("no such vector element (vector not associated)"));
      else if (type_not_allocated (original_array_type))
	error (_("no such vector element (vector not allocated)"));
    }
  else
    {
      if (type_not_associated (original_array_type))
	error (_("array not associated"));
      else if (type_not_allocated (original_array_type))
	error (_("array not allocated"));
    }

  /* First check that the number of dimensions in the type we are slicing
     matches the number of arguments we were passed.  */
  int ndimensions = calc_f77_array_dims (original_array_type);
  if (nargs != ndimensions)
    error (_("Wrong number of subscripts"));

  /* This will be initialised below with the type of the elements held in
     ARRAY.  */
  struct type *inner_element_type;

  /* Extract the types of each array dimension from the original array
     type.  We need these available so we can fill in the default upper and
     lower bounds if the user requested slice doesn't provide that
     information.  Additionally unpacking the dimensions like this gives us
     the inner element type.  */
  std::vector<struct type *> dim_types;
  {
    dim_types.reserve (ndimensions);
    struct type *type = original_array_type;
    for (int i = 0; i < ndimensions; ++i)
      {
	dim_types.push_back (type);
	type = type->target_type ();
      }
    /* TYPE is now the inner element type of the array, we start the new
       array slice off as this type, then as we process the requested slice
       (from the user) we wrap new types around this to build up the final
       slice type.  */
    inner_element_type = type;
  }

  /* As we analyse the new slice type we need to understand if the data
     being referenced is contiguous.  Do decide this we must track the size
     of an element at each dimension of the new slice array.  Initially the
     elements of the inner most dimension of the array are the same inner
     most elements as the original ARRAY.  */
  LONGEST slice_element_size = inner_element_type->length ();

  /* Start off assuming all data is contiguous, this will be set to false
     if access to any dimension results in non-contiguous data.  */
  bool is_all_contiguous = true;

  /* The TOTAL_OFFSET is the distance in bytes from the start of the
     original ARRAY to the start of the new slice.  This is calculated as
     we process the information from the user.  */
  LONGEST total_offset = 0;

  /* A structure representing information about each dimension of the
     resulting slice.  */
  struct slice_dim
  {
    /* Constructor.  */
    slice_dim (LONGEST l, LONGEST h, LONGEST s, struct type *idx)
      : low (l),
	high (h),
	stride (s),
	index (idx)
    { /* Nothing.  */ }

    /* The low bound for this dimension of the slice.  */
    LONGEST low;

    /* The high bound for this dimension of the slice.  */
    LONGEST high;

    /* The byte stride for this dimension of the slice.  */
    LONGEST stride;

    struct type *index;
  };

  /* The dimensions of the resulting slice.  */
  std::vector<slice_dim> slice_dims;

  /* Process the incoming arguments.   These arguments are in the reverse
     order to the array dimensions, that is the first argument refers to
     the last array dimension.  */
  if (fortran_array_slicing_debug)
    debug_printf ("Processing array access:\n");
  for (int i = 0; i < nargs; ++i)
    {
      /* For each dimension of the array the user will have either provided
	 a ranged access with optional lower bound, upper bound, and
	 stride, or the user will have supplied a single index.  */
      struct type *dim_type = dim_types[ndimensions - (i + 1)];
      fortran_range_operation *range_op
	= dynamic_cast<fortran_range_operation *> (ops[i].get ());
      if (range_op != nullptr)
	{
	  enum range_flag range_flag = range_op->get_flags ();

	  LONGEST low, high, stride;
	  low = high = stride = 0;

	  if ((range_flag & RANGE_LOW_BOUND_DEFAULT) == 0)
	    low = value_as_long (range_op->evaluate0 (exp, noside));
	  else
	    low = f77_get_lowerbound (dim_type);
	  if ((range_flag & RANGE_HIGH_BOUND_DEFAULT) == 0)
	    high = value_as_long (range_op->evaluate1 (exp, noside));
	  else
	    high = f77_get_upperbound (dim_type);
	  if ((range_flag & RANGE_HAS_STRIDE) == RANGE_HAS_STRIDE)
	    stride = value_as_long (range_op->evaluate2 (exp, noside));
	  else
	    stride = 1;

	  if (stride == 0)
	    error (_("stride must not be 0"));

	  /* Get information about this dimension in the original ARRAY.  */
	  struct type *target_type = dim_type->target_type ();
	  struct type *index_type = dim_type->index_type ();
	  LONGEST lb = f77_get_lowerbound (dim_type);
	  LONGEST ub = f77_get_upperbound (dim_type);
	  LONGEST sd = index_type->bit_stride ();
	  if (sd == 0)
	    sd = target_type->length () * 8;

	  if (fortran_array_slicing_debug)
	    {
	      debug_printf ("|-> Range access\n");
	      std::string str = type_to_string (dim_type);
	      debug_printf ("|   |-> Type: %s\n", str.c_str ());
	      debug_printf ("|   |-> Array:\n");
	      debug_printf ("|   |   |-> Low bound: %s\n", plongest (lb));
	      debug_printf ("|   |   |-> High bound: %s\n", plongest (ub));
	      debug_printf ("|   |   |-> Bit stride: %s\n", plongest (sd));
	      debug_printf ("|   |   |-> Byte stride: %s\n", plongest (sd / 8));
	      debug_printf ("|   |   |-> Type size: %s\n",
			    pulongest (dim_type->length ()));
	      debug_printf ("|   |   '-> Target type size: %s\n",
			    pulongest (target_type->length ()));
	      debug_printf ("|   |-> Accessing:\n");
	      debug_printf ("|   |   |-> Low bound: %s\n",
			    plongest (low));
	      debug_printf ("|   |   |-> High bound: %s\n",
			    plongest (high));
	      debug_printf ("|   |   '-> Element stride: %s\n",
			    plongest (stride));
	    }

	  /* Check the user hasn't asked for something invalid.  */
	  if (high > ub || low < lb)
	    error (_("array subscript out of bounds"));

	  /* Calculate what this dimension of the new slice array will look
	     like.  OFFSET is the byte offset from the start of the
	     previous (more outer) dimension to the start of this
	     dimension.  E_COUNT is the number of elements in this
	     dimension.  REMAINDER is the number of elements remaining
	     between the last included element and the upper bound.  For
	     example an access '1:6:2' will include elements 1, 3, 5 and
	     have a remainder of 1 (element #6).  */
	  LONGEST lowest = std::min (low, high);
	  LONGEST offset = (sd / 8) * (lowest - lb);
	  LONGEST e_count = std::abs (high - low) + 1;
	  e_count = (e_count + (std::abs (stride) - 1)) / std::abs (stride);
	  LONGEST new_low = 1;
	  LONGEST new_high = new_low + e_count - 1;
	  LONGEST new_stride = (sd * stride) / 8;
	  LONGEST last_elem = low + ((e_count - 1) * stride);
	  LONGEST remainder = high - last_elem;
	  if (low > high)
	    {
	      offset += std::abs (remainder) * target_type->length ();
	      if (stride > 0)
		error (_("incorrect stride and boundary combination"));
	    }
	  else if (stride < 0)
	    error (_("incorrect stride and boundary combination"));

	  /* Is the data within this dimension contiguous?  It is if the
	     newly computed stride is the same size as a single element of
	     this dimension.  */
	  bool is_dim_contiguous = (new_stride == slice_element_size);
	  is_all_contiguous &= is_dim_contiguous;

	  if (fortran_array_slicing_debug)
	    {
	      debug_printf ("|   '-> Results:\n");
	      debug_printf ("|       |-> Offset = %s\n", plongest (offset));
	      debug_printf ("|       |-> Elements = %s\n", plongest (e_count));
	      debug_printf ("|       |-> Low bound = %s\n", plongest (new_low));
	      debug_printf ("|       |-> High bound = %s\n",
			    plongest (new_high));
	      debug_printf ("|       |-> Byte stride = %s\n",
			    plongest (new_stride));
	      debug_printf ("|       |-> Last element = %s\n",
			    plongest (last_elem));
	      debug_printf ("|       |-> Remainder = %s\n",
			    plongest (remainder));
	      debug_printf ("|       '-> Contiguous = %s\n",
			    (is_dim_contiguous ? "Yes" : "No"));
	    }

	  /* Figure out how big (in bytes) an element of this dimension of
	     the new array slice will be.  */
	  slice_element_size = std::abs (new_stride * e_count);

	  slice_dims.emplace_back (new_low, new_high, new_stride,
				   index_type);

	  /* Update the total offset.  */
	  total_offset += offset;
	}
      else
	{
	  /* There is a single index for this dimension.  */
	  LONGEST index
	    = value_as_long (ops[i]->evaluate_with_coercion (exp, noside));

	  /* Get information about this dimension in the original ARRAY.  */
	  struct type *target_type = dim_type->target_type ();
	  struct type *index_type = dim_type->index_type ();
	  LONGEST lb = f77_get_lowerbound (dim_type);
	  LONGEST ub = f77_get_upperbound (dim_type);
	  LONGEST sd = index_type->bit_stride () / 8;
	  if (sd == 0)
	    sd = target_type->length ();

	  if (fortran_array_slicing_debug)
	    {
	      debug_printf ("|-> Index access\n");
	      std::string str = type_to_string (dim_type);
	      debug_printf ("|   |-> Type: %s\n", str.c_str ());
	      debug_printf ("|   |-> Array:\n");
	      debug_printf ("|   |   |-> Low bound: %s\n", plongest (lb));
	      debug_printf ("|   |   |-> High bound: %s\n", plongest (ub));
	      debug_printf ("|   |   |-> Byte stride: %s\n", plongest (sd));
	      debug_printf ("|   |   |-> Type size: %s\n",
			    pulongest (dim_type->length ()));
	      debug_printf ("|   |   '-> Target type size: %s\n",
			    pulongest (target_type->length ()));
	      debug_printf ("|   '-> Accessing:\n");
	      debug_printf ("|       '-> Index: %s\n",
			    plongest (index));
	    }

	  /* If the array has actual content then check the index is in
	     bounds.  An array without content (an unbound array) doesn't
	     have a known upper bound, so don't error check in that
	     situation.  */
	  if (index < lb
	      || (dim_type->index_type ()->bounds ()->high.kind () != PROP_UNDEFINED
		  && index > ub)
	      || (array->lval () != lval_memory
		  && dim_type->index_type ()->bounds ()->high.kind () == PROP_UNDEFINED))
	    {
	      if (type_not_associated (dim_type))
		error (_("no such vector element (vector not associated)"));
	      else if (type_not_allocated (dim_type))
		error (_("no such vector element (vector not allocated)"));
	      else
		error (_("no such vector element"));
	    }

	  /* Calculate using the type stride, not the target type size.  */
	  LONGEST offset = sd * (index - lb);
	  total_offset += offset;
	}
    }

  /* Build a type that represents the new array slice in the target memory
     of the original ARRAY, this type makes use of strides to correctly
     find only those elements that are part of the new slice.  */
  struct type *array_slice_type = inner_element_type;
  for (const auto &d : slice_dims)
    {
      /* Create the range.  */
      dynamic_prop p_low, p_high, p_stride;

      p_low.set_const_val (d.low);
      p_high.set_const_val (d.high);
      p_stride.set_const_val (d.stride);

      type_allocator alloc (d.index->target_type ());
      struct type *new_range
	= create_range_type_with_stride (alloc,
					 d.index->target_type (),
					 &p_low, &p_high, 0, &p_stride,
					 true);
      array_slice_type
	= create_array_type (alloc, array_slice_type, new_range);
    }

  if (fortran_array_slicing_debug)
    {
      debug_printf ("'-> Final result:\n");
      debug_printf ("    |-> Type: %s\n",
		    type_to_string (array_slice_type).c_str ());
      debug_printf ("    |-> Total offset: %s\n",
		    plongest (total_offset));
      debug_printf ("    |-> Base address: %s\n",
		    core_addr_to_string (array->address ()));
      debug_printf ("    '-> Contiguous = %s\n",
		    (is_all_contiguous ? "Yes" : "No"));
    }

  /* Should we repack this array slice?  */
  if (!is_all_contiguous && (repack_array_slices || is_string_p))
    {
      /* Build a type for the repacked slice.  */
      struct type *repacked_array_type = inner_element_type;
      for (const auto &d : slice_dims)
	{
	  /* Create the range.  */
	  dynamic_prop p_low, p_high, p_stride;

	  p_low.set_const_val (d.low);
	  p_high.set_const_val (d.high);
	  p_stride.set_const_val (repacked_array_type->length ());

	  type_allocator alloc (d.index->target_type ());
	  struct type *new_range
	    = create_range_type_with_stride (alloc,
					     d.index->target_type (),
					     &p_low, &p_high, 0, &p_stride,
					     true);
	  repacked_array_type
	    = create_array_type (alloc, repacked_array_type, new_range);
	}

      /* Now copy the elements from the original ARRAY into the packed
	 array value DEST.  */
      struct value *dest = value::allocate (repacked_array_type);
      if (array->lazy ()
	  || (total_offset + array_slice_type->length ()
	      > check_typedef (array->type ())->length ()))
	{
	  fortran_array_walker<fortran_lazy_array_repacker_impl> p
	    (array_slice_type, array->address () + total_offset, dest);
	  p.walk ();
	}
      else
	{
	  fortran_array_walker<fortran_array_repacker_impl> p
	    (array_slice_type, array->address () + total_offset,
	     total_offset, array, dest);
	  p.walk ();
	}
      array = dest;
    }
  else
    {
      if (array->lval () == lval_memory)
	{
	  /* If the value we're taking a slice from is not yet loaded, or
	     the requested slice is outside the values content range then
	     just create a new lazy value pointing at the memory where the
	     contents we're looking for exist.  */
	  if (array->lazy ()
	      || (total_offset + array_slice_type->length ()
		  > check_typedef (array->type ())->length ()))
	    array = value_at_lazy (array_slice_type,
				   array->address () + total_offset);
	  else
	    array = value_from_contents_and_address
	      (array_slice_type, array->contents ().data () + total_offset,
	       array->address () + total_offset);
	}
      else if (!array->lazy ())
	array = value_from_component (array, array_slice_type, total_offset);
      else
	error (_("cannot subscript arrays that are not in memory"));
    }

  return array;
}

value *
fortran_undetermined::evaluate (struct type *expect_type,
				struct expression *exp,
				enum noside noside)
{
  value *callee = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  if (noside == EVAL_AVOID_SIDE_EFFECTS
      && is_dynamic_type (callee->type ()))
    callee = std::get<0> (m_storage)->evaluate (nullptr, exp, EVAL_NORMAL);
  struct type *type = check_typedef (callee->type ());
  enum type_code code = type->code ();

  if (code == TYPE_CODE_PTR)
    {
      /* Fortran always passes variable to subroutines as pointer.
	 So we need to look into its target type to see if it is
	 array, string or function.  If it is, we need to switch
	 to the target value the original one points to.  */
      struct type *target_type = check_typedef (type->target_type ());

      if (target_type->code () == TYPE_CODE_ARRAY
	  || target_type->code () == TYPE_CODE_STRING
	  || target_type->code () == TYPE_CODE_FUNC)
	{
	  callee = value_ind (callee);
	  type = check_typedef (callee->type ());
	  code = type->code ();
	}
    }

  switch (code)
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRING:
      return value_subarray (callee, exp, noside);

    case TYPE_CODE_PTR:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_INTERNAL_FUNCTION:
      {
	/* It's a function call.  Allocate arg vector, including
	   space for the function to be called in argvec[0] and a
	   termination NULL.  */
	const std::vector<operation_up> &actual (std::get<1> (m_storage));
	std::vector<value *> argvec (actual.size ());
	bool is_internal_func = (code == TYPE_CODE_INTERNAL_FUNCTION);
	for (int tem = 0; tem < argvec.size (); tem++)
	  argvec[tem] = fortran_prepare_argument (exp, actual[tem].get (),
						  tem, is_internal_func,
						  callee->type (),
						  noside);
	return evaluate_subexp_do_call (exp, noside, callee, argvec,
					nullptr, expect_type);
      }

    default:
      error (_("Cannot perform substring on this type"));
    }
}

value *
fortran_bound_1arg::evaluate (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  bool lbound_p = std::get<0> (m_storage) == FORTRAN_LBOUND;
  value *arg1 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  fortran_require_array (arg1->type (), lbound_p);
  return fortran_bounds_all_dims (lbound_p, exp->gdbarch, arg1);
}

value *
fortran_bound_2arg::evaluate (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  bool lbound_p = std::get<0> (m_storage) == FORTRAN_LBOUND;
  value *arg1 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  fortran_require_array (arg1->type (), lbound_p);

  /* User asked for the bounds of a specific dimension of the array.  */
  value *arg2 = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
  type *type_arg2 = check_typedef (arg2->type ());
  if (type_arg2->code () != TYPE_CODE_INT)
    {
      if (lbound_p)
	error (_("LBOUND second argument should be an integer"));
      else
	error (_("UBOUND second argument should be an integer"));
    }

  type *result_type = builtin_f_type (exp->gdbarch)->builtin_integer;
  return fortran_bounds_for_dimension (lbound_p, arg1, arg2, result_type);
}

value *
fortran_bound_3arg::evaluate (type *expect_type,
			      expression *exp,
			      noside noside)
{
  const bool lbound_p = std::get<0> (m_storage) == FORTRAN_LBOUND;
  value *arg1 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  fortran_require_array (arg1->type (), lbound_p);

  /* User asked for the bounds of a specific dimension of the array.  */
  value *arg2 = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
  type *type_arg2 = check_typedef (arg2->type ());
  if (type_arg2->code () != TYPE_CODE_INT)
    {
      if (lbound_p)
	error (_("LBOUND second argument should be an integer"));
      else
	error (_("UBOUND second argument should be an integer"));
    }

  type *kind_arg = std::get<3> (m_storage);
  gdb_assert (kind_arg->code () == TYPE_CODE_INT);

  return fortran_bounds_for_dimension (lbound_p, arg1, arg2, kind_arg);
}

/* Implement STRUCTOP_STRUCT for Fortran.  See operation::evaluate in
   expression.h for argument descriptions.  */

value *
fortran_structop_operation::evaluate (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  const char *str = std::get<1> (m_storage).c_str ();
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = lookup_struct_elt_type (arg1->type (), str, 1);

      if (type != nullptr && is_dynamic_type (type))
	arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, EVAL_NORMAL);
    }

  value *elt = value_struct_elt (&arg1, {}, str, NULL, "structure");

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *elt_type = elt->type ();
      if (is_dynamic_type (elt_type))
	{
	  const gdb_byte *valaddr = elt->contents_for_printing ().data ();
	  CORE_ADDR address = elt->address ();
	  gdb::array_view<const gdb_byte> view
	    = gdb::make_array_view (valaddr, elt_type->length ());
	  elt_type = resolve_dynamic_type (elt_type, view, address);
	}
      elt = value::zero (elt_type, elt->lval ());
    }

  return elt;
}

} /* namespace expr */

/* See language.h.  */

void
f_language::print_array_index (struct type *index_type, LONGEST index,
			       struct ui_file *stream,
			       const value_print_options *options) const
{
  struct value *index_value = value_from_longest (index_type, index);

  gdb_printf (stream, "(");
  value_print (index_value, stream, options);
  gdb_printf (stream, ") = ");
}

/* See language.h.  */

void
f_language::language_arch_info (struct gdbarch *gdbarch,
				struct language_arch_info *lai) const
{
  const struct builtin_f_type *builtin = builtin_f_type (gdbarch);

  /* Helper function to allow shorter lines below.  */
  auto add  = [&] (struct type * t)
  {
    lai->add_primitive_type (t);
  };

  add (builtin->builtin_character);
  add (builtin->builtin_logical);
  add (builtin->builtin_logical_s1);
  add (builtin->builtin_logical_s2);
  add (builtin->builtin_logical_s8);
  add (builtin->builtin_real);
  add (builtin->builtin_real_s8);
  add (builtin->builtin_real_s16);
  add (builtin->builtin_complex);
  add (builtin->builtin_complex_s8);
  add (builtin->builtin_void);

  lai->set_string_char_type (builtin->builtin_character);
  lai->set_bool_type (builtin->builtin_logical, "logical");
}

/* See language.h.  */

unsigned int
f_language::search_name_hash (const char *name) const
{
  return cp_search_name_hash (name);
}

/* See language.h.  */

struct block_symbol
f_language::lookup_symbol_nonlocal (const char *name,
				    const struct block *block,
				    const domain_enum domain) const
{
  return cp_lookup_symbol_nonlocal (this, name, block, domain);
}

/* See language.h.  */

symbol_name_matcher_ftype *
f_language::get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const
{
  return cp_get_symbol_name_matcher (lookup_name);
}

/* Single instance of the Fortran language class.  */

static f_language f_language_defn;

static struct builtin_f_type *
build_fortran_types (struct gdbarch *gdbarch)
{
  struct builtin_f_type *builtin_f_type = new struct builtin_f_type;

  builtin_f_type->builtin_void = builtin_type (gdbarch)->builtin_void;

  type_allocator alloc (gdbarch);

  builtin_f_type->builtin_character
    = alloc.new_type (TYPE_CODE_CHAR, TARGET_CHAR_BIT, "character");

  builtin_f_type->builtin_logical_s1
    = init_boolean_type (alloc, TARGET_CHAR_BIT, 1, "logical*1");

  builtin_f_type->builtin_logical_s2
    = init_boolean_type (alloc, gdbarch_short_bit (gdbarch), 1, "logical*2");

  builtin_f_type->builtin_logical
    = init_boolean_type (alloc, gdbarch_int_bit (gdbarch), 1, "logical*4");

  builtin_f_type->builtin_logical_s8
    = init_boolean_type (alloc, gdbarch_long_long_bit (gdbarch), 1,
			 "logical*8");

  builtin_f_type->builtin_integer_s1
    = init_integer_type (alloc, TARGET_CHAR_BIT, 0, "integer*1");

  builtin_f_type->builtin_integer_s2
    = init_integer_type (alloc, gdbarch_short_bit (gdbarch), 0, "integer*2");

  builtin_f_type->builtin_integer
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch), 0, "integer*4");

  builtin_f_type->builtin_integer_s8
    = init_integer_type (alloc, gdbarch_long_long_bit (gdbarch), 0,
			 "integer*8");

  builtin_f_type->builtin_real
    = init_float_type (alloc, gdbarch_float_bit (gdbarch),
		       "real*4", gdbarch_float_format (gdbarch));

  builtin_f_type->builtin_real_s8
    = init_float_type (alloc, gdbarch_double_bit (gdbarch),
		       "real*8", gdbarch_double_format (gdbarch));

  auto fmt = gdbarch_floatformat_for_type (gdbarch, "real(kind=16)", 128);
  if (fmt != nullptr)
    builtin_f_type->builtin_real_s16
      = init_float_type (alloc, 128, "real*16", fmt);
  else if (gdbarch_long_double_bit (gdbarch) == 128)
    builtin_f_type->builtin_real_s16
      = init_float_type (alloc, gdbarch_long_double_bit (gdbarch),
			 "real*16", gdbarch_long_double_format (gdbarch));
  else
    builtin_f_type->builtin_real_s16
      = alloc.new_type (TYPE_CODE_ERROR, 128, "real*16");

  builtin_f_type->builtin_complex
    = init_complex_type ("complex*4", builtin_f_type->builtin_real);

  builtin_f_type->builtin_complex_s8
    = init_complex_type ("complex*8", builtin_f_type->builtin_real_s8);

  if (builtin_f_type->builtin_real_s16->code () == TYPE_CODE_ERROR)
    builtin_f_type->builtin_complex_s16
      = alloc.new_type (TYPE_CODE_ERROR, 256, "complex*16");
  else
    builtin_f_type->builtin_complex_s16
      = init_complex_type ("complex*16", builtin_f_type->builtin_real_s16);

  return builtin_f_type;
}

static const registry<gdbarch>::key<struct builtin_f_type> f_type_data;

const struct builtin_f_type *
builtin_f_type (struct gdbarch *gdbarch)
{
  struct builtin_f_type *result = f_type_data.get (gdbarch);
  if (result == nullptr)
    {
      result = build_fortran_types (gdbarch);
      f_type_data.set (gdbarch, result);
    }

  return result;
}

/* Command-list for the "set/show fortran" prefix command.  */
static struct cmd_list_element *set_fortran_list;
static struct cmd_list_element *show_fortran_list;

void _initialize_f_language ();
void
_initialize_f_language ()
{
  add_setshow_prefix_cmd
    ("fortran", no_class,
     _("Prefix command for changing Fortran-specific settings."),
     _("Generic command for showing Fortran-specific settings."),
     &set_fortran_list, &show_fortran_list,
     &setlist, &showlist);

  add_setshow_boolean_cmd ("repack-array-slices", class_vars,
			   &repack_array_slices, _("\
Enable or disable repacking of non-contiguous array slices."), _("\
Show whether non-contiguous array slices are repacked."), _("\
When the user requests a slice of a Fortran array then we can either return\n\
a descriptor that describes the array in place (using the original array data\n\
in its existing location) or the original data can be repacked (copied) to a\n\
new location.\n\
\n\
When the content of the array slice is contiguous within the original array\n\
then the result will never be repacked, but when the data for the new array\n\
is non-contiguous within the original array repacking will only be performed\n\
when this setting is on."),
			   NULL,
			   show_repack_array_slices,
			   &set_fortran_list, &show_fortran_list);

  /* Debug Fortran's array slicing logic.  */
  add_setshow_boolean_cmd ("fortran-array-slicing", class_maintenance,
			   &fortran_array_slicing_debug, _("\
Set debugging of Fortran array slicing."), _("\
Show debugging of Fortran array slicing."), _("\
When on, debugging of Fortran array slicing is enabled."),
			    NULL,
			    show_fortran_array_slicing_debug,
			    &setdebuglist, &showdebuglist);
}

/* Ensures that function argument VALUE is in the appropriate form to
   pass to a Fortran function.  Returns a possibly new value that should
   be used instead of VALUE.

   When IS_ARTIFICIAL is true this indicates an artificial argument,
   e.g. hidden string lengths which the GNU Fortran argument passing
   convention specifies as being passed by value.

   When IS_ARTIFICIAL is false, the argument is passed by pointer.  If the
   value is already in target memory then return a value that is a pointer
   to VALUE.  If VALUE is not in memory (e.g. an integer literal), allocate
   space in the target, copy VALUE in, and return a pointer to the in
   memory copy.  */

static struct value *
fortran_argument_convert (struct value *value, bool is_artificial)
{
  if (!is_artificial)
    {
      /* If the value is not in the inferior e.g. registers values,
	 convenience variables and user input.  */
      if (value->lval () != lval_memory)
	{
	  struct type *type = value->type ();
	  const int length = type->length ();
	  const CORE_ADDR addr
	    = value_as_long (value_allocate_space_in_inferior (length));
	  write_memory (addr, value->contents ().data (), length);
	  struct value *val = value_from_contents_and_address
	    (type, value->contents ().data (), addr);
	  return value_addr (val);
	}
      else
	return value_addr (value); /* Program variables, e.g. arrays.  */
    }
    return value;
}

/* Prepare (and return) an argument value ready for an inferior function
   call to a Fortran function.  EXP and POS are the expressions describing
   the argument to prepare.  ARG_NUM is the argument number being
   prepared, with 0 being the first argument and so on.  FUNC_TYPE is the
   type of the function being called.

   IS_INTERNAL_CALL_P is true if this is a call to a function of type
   TYPE_CODE_INTERNAL_FUNCTION, otherwise this parameter is false.

   NOSIDE has its usual meaning for expression parsing (see eval.c).

   Arguments in Fortran are normally passed by address, we coerce the
   arguments here rather than in value_arg_coerce as otherwise the call to
   malloc (to place the non-lvalue parameters in target memory) is hit by
   this Fortran specific logic.  This results in malloc being called with a
   pointer to an integer followed by an attempt to malloc the arguments to
   malloc in target memory.  Infinite recursion ensues.  */

static value *
fortran_prepare_argument (struct expression *exp,
			  expr::operation *subexp,
			  int arg_num, bool is_internal_call_p,
			  struct type *func_type, enum noside noside)
{
  if (is_internal_call_p)
    return subexp->evaluate_with_coercion (exp, noside);

  bool is_artificial = ((arg_num >= func_type->num_fields ())
			? true
			: func_type->field (arg_num).is_artificial ());

  /* If this is an artificial argument, then either, this is an argument
     beyond the end of the known arguments, or possibly, there are no known
     arguments (maybe missing debug info).

     For these artificial arguments, if the user has prefixed it with '&'
     (for address-of), then lets always allow this to succeed, even if the
     argument is not actually in inferior memory.  This will allow the user
     to pass arguments to a Fortran function even when there's no debug
     information.

     As we already pass the address of non-artificial arguments, all we
     need to do if skip the UNOP_ADDR operator in the expression and mark
     the argument as non-artificial.  */
  if (is_artificial)
    {
      expr::unop_addr_operation *addrop
	= dynamic_cast<expr::unop_addr_operation *> (subexp);
      if (addrop != nullptr)
	{
	  subexp = addrop->get_expression ().get ();
	  is_artificial = false;
	}
    }

  struct value *arg_val = subexp->evaluate_with_coercion (exp, noside);
  return fortran_argument_convert (arg_val, is_artificial);
}

/* See f-lang.h.  */

struct type *
fortran_preserve_arg_pointer (struct value *arg, struct type *type)
{
  if (arg->type ()->code () == TYPE_CODE_PTR)
    return arg->type ();
  return type;
}

/* See f-lang.h.  */

CORE_ADDR
fortran_adjust_dynamic_array_base_address_hack (struct type *type,
						CORE_ADDR address)
{
  gdb_assert (type->code () == TYPE_CODE_ARRAY);

  /* We can't adjust the base address for arrays that have no content.  */
  if (type_not_allocated (type) || type_not_associated (type))
    return address;

  int ndimensions = calc_f77_array_dims (type);
  LONGEST total_offset = 0;

  /* Walk through each of the dimensions of this array type and figure out
     if any of the dimensions are "backwards", that is the base address
     for this dimension points to the element at the highest memory
     address and the stride is negative.  */
  struct type *tmp_type = type;
  for (int i = 0 ; i < ndimensions; ++i)
    {
      /* Grab the range for this dimension and extract the lower and upper
	 bounds.  */
      tmp_type = check_typedef (tmp_type);
      struct type *range_type = tmp_type->index_type ();
      LONGEST lowerbound, upperbound, stride;
      if (!get_discrete_bounds (range_type, &lowerbound, &upperbound))
	error ("failed to get range bounds");

      /* Figure out the stride for this dimension.  */
      struct type *elt_type = check_typedef (tmp_type->target_type ());
      stride = tmp_type->index_type ()->bounds ()->bit_stride ();
      if (stride == 0)
	stride = type_length_units (elt_type);
      else
	{
	  int unit_size
	    = gdbarch_addressable_memory_unit_size (elt_type->arch ());
	  stride /= (unit_size * 8);
	}

      /* If this dimension is "backward" then figure out the offset
	 adjustment required to point to the element at the lowest memory
	 address, and add this to the total offset.  */
      LONGEST offset = 0;
      if (stride < 0 && lowerbound < upperbound)
	offset = (upperbound - lowerbound) * stride;
      total_offset += offset;
      tmp_type = tmp_type->target_type ();
    }

  /* Adjust the address of this object and return it.  */
  address += total_offset;
  return address;
}

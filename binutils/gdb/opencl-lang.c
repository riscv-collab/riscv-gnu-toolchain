/* OpenCL language support for GDB, the GNU debugger.
   Copyright (C) 2010-2024 Free Software Foundation, Inc.

   Contributed by Ken Werner <ken.werner@de.ibm.com>.

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
#include "gdbtypes.h"
#include "symtab.h"
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "c-lang.h"
#include "gdbarch.h"
#include "c-exp.h"

/* Returns the corresponding OpenCL vector type from the given type code,
   the length of the element type, the unsigned flag and the amount of
   elements (N).  */

static struct type *
lookup_opencl_vector_type (struct gdbarch *gdbarch, enum type_code code,
			   unsigned int el_length, unsigned int flag_unsigned,
			   int n)
{
  unsigned int length;

  /* Check if n describes a valid OpenCL vector size (2, 3, 4, 8, 16).  */
  if (n != 2 && n != 3 && n != 4 && n != 8 && n != 16)
    error (_("Invalid OpenCL vector size: %d"), n);

  /* Triple vectors have the size of a quad vector.  */
  length = (n == 3) ?  el_length * 4 : el_length * n;

  auto filter = [&] (struct type *type)
  {
    LONGEST lowb, highb;

    return (type->code () == TYPE_CODE_ARRAY && type->is_vector ()
	    && get_array_bounds (type, &lowb, &highb)
	    && type->target_type ()->code () == code
	    && type->target_type ()->is_unsigned () == flag_unsigned
	    && type->target_type ()->length () == el_length
	    && type->length () == length
	    && highb - lowb + 1 == n);
  };
  const struct language_defn *lang = language_def (language_opencl);
  return language_lookup_primitive_type (lang, gdbarch, filter);
}

/* Returns nonzero if the array ARR contains duplicates within
     the first N elements.  */

static int
array_has_dups (int *arr, int n)
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      for (j = i + 1; j < n; j++)
	{
	  if (arr[i] == arr[j])
	    return 1;
	}
    }

  return 0;
}

/* The OpenCL component access syntax allows to create lvalues referring to
   selected elements of an original OpenCL vector in arbitrary order.  This
   structure holds the information to describe such lvalues.  */

struct lval_closure
{
  /* Reference count.  */
  int refc;
  /* The number of indices.  */
  int n;
  /* The element indices themselves.  */
  int *indices;
  /* A pointer to the original value.  */
  struct value *val;
};

/* Allocates an instance of struct lval_closure.  */

static struct lval_closure *
allocate_lval_closure (int *indices, int n, struct value *val)
{
  struct lval_closure *c = XCNEW (struct lval_closure);

  c->refc = 1;
  c->n = n;
  c->indices = XCNEWVEC (int, n);
  memcpy (c->indices, indices, n * sizeof (int));
  val->incref (); /* Increment the reference counter of the value.  */
  c->val = val;

  return c;
}

static void
lval_func_read (struct value *v)
{
  struct lval_closure *c = (struct lval_closure *) v->computed_closure ();
  struct type *type = check_typedef (v->type ());
  struct type *eltype = check_typedef (c->val->type ())->target_type ();
  LONGEST offset = v->offset ();
  LONGEST elsize = eltype->length ();
  int n, i, j = 0;
  LONGEST lowb = 0;
  LONGEST highb = 0;

  if (type->code () == TYPE_CODE_ARRAY
      && !get_array_bounds (type, &lowb, &highb))
    error (_("Could not determine the vector bounds"));

  /* Assume elsize aligned offset.  */
  gdb_assert (offset % elsize == 0);
  offset /= elsize;
  n = offset + highb - lowb + 1;
  gdb_assert (n <= c->n);

  for (i = offset; i < n; i++)
    memcpy (v->contents_raw ().data () + j++ * elsize,
	    c->val->contents ().data () + c->indices[i] * elsize,
	    elsize);
}

static void
lval_func_write (struct value *v, struct value *fromval)
{
  scoped_value_mark mark;

  struct lval_closure *c = (struct lval_closure *) v->computed_closure ();
  struct type *type = check_typedef (v->type ());
  struct type *eltype = check_typedef (c->val->type ())->target_type ();
  LONGEST offset = v->offset ();
  LONGEST elsize = eltype->length ();
  int n, i, j = 0;
  LONGEST lowb = 0;
  LONGEST highb = 0;

  if (type->code () == TYPE_CODE_ARRAY
      && !get_array_bounds (type, &lowb, &highb))
    error (_("Could not determine the vector bounds"));

  /* Assume elsize aligned offset.  */
  gdb_assert (offset % elsize == 0);
  offset /= elsize;
  n = offset + highb - lowb + 1;

  /* Since accesses to the fourth component of a triple vector is undefined we
     just skip writes to the fourth element.  Imagine something like this:
       int3 i3 = (int3)(0, 1, 2);
       i3.hi.hi = 5;
     In this case n would be 4 (offset=12/4 + 1) while c->n would be 3.  */
  if (n > c->n)
    n = c->n;

  for (i = offset; i < n; i++)
    {
      struct value *from_elm_val = value::allocate (eltype);
      struct value *to_elm_val = value_subscript (c->val, c->indices[i]);

      memcpy (from_elm_val->contents_writeable ().data (),
	      fromval->contents ().data () + j++ * elsize,
	      elsize);
      value_assign (to_elm_val, from_elm_val);
    }
}

/* Return true if bits in V from OFFSET and LENGTH represent a
   synthetic pointer.  */

static bool
lval_func_check_synthetic_pointer (const struct value *v,
				   LONGEST offset, int length)
{
  struct lval_closure *c = (struct lval_closure *) v->computed_closure ();
  /* Size of the target type in bits.  */
  int elsize =
    check_typedef (c->val->type ())->target_type ()->length () * 8;
  int startrest = offset % elsize;
  int start = offset / elsize;
  int endrest = (offset + length) % elsize;
  int end = (offset + length) / elsize;
  int i;

  if (endrest)
    end++;

  if (end > c->n)
    return false;

  for (i = start; i < end; i++)
    {
      int comp_offset = (i == start) ? startrest : 0;
      int comp_length = (i == end) ? endrest : elsize;

      if (!c->val->bits_synthetic_pointer (c->indices[i] * elsize + comp_offset,
					   comp_length))
	return false;
    }

  return true;
}

static void *
lval_func_copy_closure (const struct value *v)
{
  struct lval_closure *c = (struct lval_closure *) v->computed_closure ();

  ++c->refc;

  return c;
}

static void
lval_func_free_closure (struct value *v)
{
  struct lval_closure *c = (struct lval_closure *) v->computed_closure ();

  --c->refc;

  if (c->refc == 0)
    {
      c->val->decref (); /* Decrement the reference counter of the value.  */
      xfree (c->indices);
      xfree (c);
    }
}

static const struct lval_funcs opencl_value_funcs =
  {
    lval_func_read,
    lval_func_write,
    nullptr,
    NULL,	/* indirect */
    NULL,	/* coerce_ref */
    lval_func_check_synthetic_pointer,
    lval_func_copy_closure,
    lval_func_free_closure
  };

/* Creates a sub-vector from VAL.  The elements are selected by the indices of
   an array with the length of N.  Supported values for NOSIDE are
   EVAL_NORMAL and EVAL_AVOID_SIDE_EFFECTS.  */

static struct value *
create_value (struct gdbarch *gdbarch, struct value *val, enum noside noside,
	      int *indices, int n)
{
  struct type *type = check_typedef (val->type ());
  struct type *elm_type = type->target_type ();
  struct value *ret;

  /* Check if a single component of a vector is requested which means
     the resulting type is a (primitive) scalar type.  */
  if (n == 1)
    {
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	ret = value::zero (elm_type, not_lval);
      else
	ret = value_subscript (val, indices[0]);
    }
  else
    {
      /* Multiple components of the vector are requested which means the
	 resulting type is a vector as well.  */
      struct type *dst_type =
	lookup_opencl_vector_type (gdbarch, elm_type->code (),
				   elm_type->length (),
				   elm_type->is_unsigned (), n);

      if (dst_type == NULL)
	dst_type = init_vector_type (elm_type, n);

      make_cv_type (TYPE_CONST (type), TYPE_VOLATILE (type), dst_type, NULL);

      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	ret = value::allocate (dst_type);
      else
	{
	  /* Check whether to create a lvalue or not.  */
	  if (val->lval () != not_lval && !array_has_dups (indices, n))
	    {
	      struct lval_closure *c = allocate_lval_closure (indices, n, val);
	      ret = value::allocate_computed (dst_type, &opencl_value_funcs, c);
	    }
	  else
	    {
	      int i;

	      ret = value::allocate (dst_type);

	      /* Copy src val contents into the destination value.  */
	      for (i = 0; i < n; i++)
		memcpy (ret->contents_writeable ().data ()
			+ (i * elm_type->length ()),
			val->contents ().data ()
			+ (indices[i] * elm_type->length ()),
			elm_type->length ());
	    }
	}
    }
  return ret;
}

/* OpenCL vector component access.  */

static struct value *
opencl_component_ref (struct expression *exp, struct value *val,
		      const char *comps, enum noside noside)
{
  LONGEST lowb, highb;
  int src_len;
  struct value *v;
  int indices[16], i;
  int dst_len;

  if (!get_array_bounds (check_typedef (val->type ()), &lowb, &highb))
    error (_("Could not determine the vector bounds"));

  src_len = highb - lowb + 1;

  /* Throw an error if the amount of array elements does not fit a
     valid OpenCL vector size (2, 3, 4, 8, 16).  */
  if (src_len != 2 && src_len != 3 && src_len != 4 && src_len != 8
      && src_len != 16)
    error (_("Invalid OpenCL vector size"));

  if (strcmp (comps, "lo") == 0 )
    {
      dst_len = (src_len == 3) ? 2 : src_len / 2;

      for (i = 0; i < dst_len; i++)
	indices[i] = i;
    }
  else if (strcmp (comps, "hi") == 0)
    {
      dst_len = (src_len == 3) ? 2 : src_len / 2;

      for (i = 0; i < dst_len; i++)
	indices[i] = dst_len + i;
    }
  else if (strcmp (comps, "even") == 0)
    {
      dst_len = (src_len == 3) ? 2 : src_len / 2;

      for (i = 0; i < dst_len; i++)
	indices[i] = i*2;
    }
  else if (strcmp (comps, "odd") == 0)
    {
      dst_len = (src_len == 3) ? 2 : src_len / 2;

      for (i = 0; i < dst_len; i++)
	indices[i] = i*2+1;
    }
  else if (strncasecmp (comps, "s", 1) == 0)
    {
#define HEXCHAR_TO_INT(C) ((C >= '0' && C <= '9') ? \
			   C-'0' : ((C >= 'A' && C <= 'F') ? \
			   C-'A'+10 : ((C >= 'a' && C <= 'f') ? \
			   C-'a'+10 : -1)))

      dst_len = strlen (comps);
      /* Skip the s/S-prefix.  */
      dst_len--;

      for (i = 0; i < dst_len; i++)
	{
	  indices[i] = HEXCHAR_TO_INT(comps[i+1]);
	  /* Check if the requested component is invalid or exceeds
	     the vector.  */
	  if (indices[i] < 0 || indices[i] >= src_len)
	    error (_("Invalid OpenCL vector component accessor %s"), comps);
	}
    }
  else
    {
      dst_len = strlen (comps);

      for (i = 0; i < dst_len; i++)
	{
	  /* x, y, z, w */
	  switch (comps[i])
	  {
	  case 'x':
	    indices[i] = 0;
	    break;
	  case 'y':
	    indices[i] = 1;
	    break;
	  case 'z':
	    if (src_len < 3)
	      error (_("Invalid OpenCL vector component accessor %s"), comps);
	    indices[i] = 2;
	    break;
	  case 'w':
	    if (src_len < 4)
	      error (_("Invalid OpenCL vector component accessor %s"), comps);
	    indices[i] = 3;
	    break;
	  default:
	    error (_("Invalid OpenCL vector component accessor %s"), comps);
	    break;
	  }
	}
    }

  /* Throw an error if the amount of requested components does not
     result in a valid length (1, 2, 3, 4, 8, 16).  */
  if (dst_len != 1 && dst_len != 2 && dst_len != 3 && dst_len != 4
      && dst_len != 8 && dst_len != 16)
    error (_("Invalid OpenCL vector component accessor %s"), comps);

  v = create_value (exp->gdbarch, val, noside, indices, dst_len);

  return v;
}

/* Perform the unary logical not (!) operation.  */

struct value *
opencl_logical_not (struct type *expect_type, struct expression *exp,
		    enum noside noside, enum exp_opcode op,
		    struct value *arg)
{
  struct type *type = check_typedef (arg->type ());
  struct type *rettype;
  struct value *ret;

  if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    {
      struct type *eltype = check_typedef (type->target_type ());
      LONGEST lowb, highb;
      int i;

      if (!get_array_bounds (type, &lowb, &highb))
	error (_("Could not determine the vector bounds"));

      /* Determine the resulting type of the operation and allocate the
	 value.  */
      rettype = lookup_opencl_vector_type (exp->gdbarch, TYPE_CODE_INT,
					   eltype->length (), 0,
					   highb - lowb + 1);
      ret = value::allocate (rettype);

      for (i = 0; i < highb - lowb + 1; i++)
	{
	  /* For vector types, the unary operator shall return a 0 if the
	  value of its operand compares unequal to 0, and -1 (i.e. all bits
	  set) if the value of its operand compares equal to 0.  */
	  int tmp = value_logical_not (value_subscript (arg, i)) ? -1 : 0;
	  memset ((ret->contents_writeable ().data ()
		   + i * eltype->length ()),
		  tmp, eltype->length ());
	}
    }
  else
    {
      rettype = language_bool_type (exp->language_defn, exp->gdbarch);
      ret = value_from_longest (rettype, value_logical_not (arg));
    }

  return ret;
}

/* Perform a relational operation on two scalar operands.  */

static int
scalar_relop (struct value *val1, struct value *val2, enum exp_opcode op)
{
  int ret;

  switch (op)
    {
    case BINOP_EQUAL:
      ret = value_equal (val1, val2);
      break;
    case BINOP_NOTEQUAL:
      ret = !value_equal (val1, val2);
      break;
    case BINOP_LESS:
      ret = value_less (val1, val2);
      break;
    case BINOP_GTR:
      ret = value_less (val2, val1);
      break;
    case BINOP_GEQ:
      ret = value_less (val2, val1) || value_equal (val1, val2);
      break;
    case BINOP_LEQ:
      ret = value_less (val1, val2) || value_equal (val1, val2);
      break;
    case BINOP_LOGICAL_AND:
      ret = !value_logical_not (val1) && !value_logical_not (val2);
      break;
    case BINOP_LOGICAL_OR:
      ret = !value_logical_not (val1) || !value_logical_not (val2);
      break;
    default:
      error (_("Attempt to perform an unsupported operation"));
      break;
    }
  return ret;
}

/* Perform a relational operation on two vector operands.  */

static struct value *
vector_relop (struct expression *exp, struct value *val1, struct value *val2,
	      enum exp_opcode op)
{
  struct value *ret;
  struct type *type1, *type2, *eltype1, *eltype2, *rettype;
  int t1_is_vec, t2_is_vec, i;
  LONGEST lowb1, lowb2, highb1, highb2;

  type1 = check_typedef (val1->type ());
  type2 = check_typedef (val2->type ());

  t1_is_vec = (type1->code () == TYPE_CODE_ARRAY && type1->is_vector ());
  t2_is_vec = (type2->code () == TYPE_CODE_ARRAY && type2->is_vector ());

  if (!t1_is_vec || !t2_is_vec)
    error (_("Vector operations are not supported on scalar types"));

  eltype1 = check_typedef (type1->target_type ());
  eltype2 = check_typedef (type2->target_type ());

  if (!get_array_bounds (type1,&lowb1, &highb1)
      || !get_array_bounds (type2, &lowb2, &highb2))
    error (_("Could not determine the vector bounds"));

  /* Check whether the vector types are compatible.  */
  if (eltype1->code () != eltype2->code ()
      || eltype1->length () != eltype2->length ()
      || eltype1->is_unsigned () != eltype2->is_unsigned ()
      || lowb1 != lowb2 || highb1 != highb2)
    error (_("Cannot perform operation on vectors with different types"));

  /* Determine the resulting type of the operation and allocate the value.  */
  rettype = lookup_opencl_vector_type (exp->gdbarch, TYPE_CODE_INT,
				       eltype1->length (), 0,
				       highb1 - lowb1 + 1);
  ret = value::allocate (rettype);

  for (i = 0; i < highb1 - lowb1 + 1; i++)
    {
      /* For vector types, the relational, equality and logical operators shall
	 return 0 if the specified relation is false and -1 (i.e. all bits set)
	 if the specified relation is true.  */
      int tmp = scalar_relop (value_subscript (val1, i),
			      value_subscript (val2, i), op) ? -1 : 0;
      memset ((ret->contents_writeable ().data ()
	       + i * eltype1->length ()),
	      tmp, eltype1->length ());
     }

  return ret;
}

/* Perform a cast of ARG into TYPE.  There's sadly a lot of duplication in
   here from valops.c:value_cast, opencl is different only in the
   behaviour of scalar to vector casting.  As far as possibly we're going
   to try and delegate back to the standard value_cast function. */

struct value *
opencl_value_cast (struct type *type, struct value *arg)
{
  if (type != arg->type ())
    {
      /* Casting scalar to vector is a special case for OpenCL, scalar
	 is cast to element type of vector then replicated into each
	 element of the vector.  First though, we need to work out if
	 this is a scalar to vector cast; code lifted from
	 valops.c:value_cast.  */
      enum type_code code1, code2;
      struct type *to_type;
      int scalar;

      to_type = check_typedef (type);

      code1 = to_type->code ();
      code2 = check_typedef (arg->type ())->code ();

      if (code2 == TYPE_CODE_REF)
	code2 = check_typedef (coerce_ref(arg)->type ())->code ();

      scalar = (code2 == TYPE_CODE_INT || code2 == TYPE_CODE_BOOL
		|| code2 == TYPE_CODE_CHAR || code2 == TYPE_CODE_FLT
		|| code2 == TYPE_CODE_DECFLOAT || code2 == TYPE_CODE_ENUM
		|| code2 == TYPE_CODE_RANGE);

      if (code1 == TYPE_CODE_ARRAY && to_type->is_vector () && scalar)
	{
	  struct type *eltype;

	  /* Cast to the element type of the vector here as
	     value_vector_widen will error if the scalar value is
	     truncated by the cast.  To avoid the error, cast (and
	     possibly truncate) here.  */
	  eltype = check_typedef (to_type->target_type ());
	  arg = value_cast (eltype, arg);

	  return value_vector_widen (arg, type);
	}
      else
	/* Standard cast handler.  */
	arg = value_cast (type, arg);
    }
  return arg;
}

/* Perform a relational operation on two operands.  */

struct value *
opencl_relop (struct type *expect_type, struct expression *exp,
	      enum noside noside, enum exp_opcode op,
	      struct value *arg1, struct value *arg2)
{
  struct value *val;
  struct type *type1 = check_typedef (arg1->type ());
  struct type *type2 = check_typedef (arg2->type ());
  int t1_is_vec = (type1->code () == TYPE_CODE_ARRAY
		   && type1->is_vector ());
  int t2_is_vec = (type2->code () == TYPE_CODE_ARRAY
		   && type2->is_vector ());

  if (!t1_is_vec && !t2_is_vec)
    {
      int tmp = scalar_relop (arg1, arg2, op);
      struct type *type =
	language_bool_type (exp->language_defn, exp->gdbarch);

      val = value_from_longest (type, tmp);
    }
  else if (t1_is_vec && t2_is_vec)
    {
      val = vector_relop (exp, arg1, arg2, op);
    }
  else
    {
      /* Widen the scalar operand to a vector.  */
      struct value **v = t1_is_vec ? &arg2 : &arg1;
      struct type *t = t1_is_vec ? type2 : type1;

      if (t->code () != TYPE_CODE_FLT && !is_integral_type (t))
	error (_("Argument to operation not a number or boolean."));

      *v = opencl_value_cast (t1_is_vec ? type1 : type2, *v);
      val = vector_relop (exp, arg1, arg2, op);
    }

  return val;
}

/* A helper function for BINOP_ASSIGN.  */

struct value *
eval_opencl_assign (struct type *expect_type, struct expression *exp,
		    enum noside noside, enum exp_opcode op,
		    struct value *arg1, struct value *arg2)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;

  struct type *type1 = arg1->type ();
  if (arg1->deprecated_modifiable ()
      && arg1->lval () != lval_internalvar)
    arg2 = opencl_value_cast (type1, arg2);

  return value_assign (arg1, arg2);
}

namespace expr
{

value *
opencl_structop_operation::evaluate (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  struct type *type1 = check_typedef (arg1->type ());

  if (type1->code () == TYPE_CODE_ARRAY && type1->is_vector ())
    return opencl_component_ref (exp, arg1, std::get<1> (m_storage).c_str (),
				 noside);
  else
    {
      struct value *v = value_struct_elt (&arg1, {},
					  std::get<1> (m_storage).c_str (),
					  NULL, "structure");

      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	v = value::zero (v->type (), v->lval ());
      return v;
    }
}

value *
opencl_logical_binop_operation::evaluate (struct type *expect_type,
					  struct expression *exp,
					  enum noside noside)
{
  enum exp_opcode op = std::get<0> (m_storage);
  value *arg1 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);

  /* For scalar operations we need to avoid evaluating operands
     unnecessarily.  However, for vector operations we always need to
     evaluate both operands.  Unfortunately we only know which of the
     two cases apply after we know the type of the second operand.
     Therefore we evaluate it once using EVAL_AVOID_SIDE_EFFECTS.  */
  value *arg2 = std::get<2> (m_storage)->evaluate (nullptr, exp,
						   EVAL_AVOID_SIDE_EFFECTS);
  struct type *type1 = check_typedef (arg1->type ());
  struct type *type2 = check_typedef (arg2->type ());

  if ((type1->code () == TYPE_CODE_ARRAY && type1->is_vector ())
      || (type2->code () == TYPE_CODE_ARRAY && type2->is_vector ()))
    {
      arg2 = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);

      return opencl_relop (nullptr, exp, noside, op, arg1, arg2);
    }
  else
    {
      /* For scalar built-in types, only evaluate the right
	 hand operand if the left hand operand compares
	 unequal(&&)/equal(||) to 0.  */
      bool tmp = value_logical_not (arg1);

      if (op == BINOP_LOGICAL_OR)
	tmp = !tmp;

      if (!tmp)
	{
	  arg2 = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
	  tmp = value_logical_not (arg2);
	  if (op == BINOP_LOGICAL_OR)
	    tmp = !tmp;
	}

      type1 = language_bool_type (exp->language_defn, exp->gdbarch);
      return value_from_longest (type1, tmp);
    }
}

value *
opencl_ternop_cond_operation::evaluate (struct type *expect_type,
					struct expression *exp,
					enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  struct type *type1 = check_typedef (arg1->type ());
  if (type1->code () == TYPE_CODE_ARRAY && type1->is_vector ())
    {
      struct value *arg2, *arg3, *tmp, *ret;
      struct type *eltype2, *type2, *type3, *eltype3;
      int t2_is_vec, t3_is_vec, i;
      LONGEST lowb1, lowb2, lowb3, highb1, highb2, highb3;

      arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
      arg3 = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
      type2 = check_typedef (arg2->type ());
      type3 = check_typedef (arg3->type ());
      t2_is_vec
	= type2->code () == TYPE_CODE_ARRAY && type2->is_vector ();
      t3_is_vec
	= type3->code () == TYPE_CODE_ARRAY && type3->is_vector ();

      /* Widen the scalar operand to a vector if necessary.  */
      if (t2_is_vec || !t3_is_vec)
	{
	  arg3 = opencl_value_cast (type2, arg3);
	  type3 = arg3->type ();
	}
      else if (!t2_is_vec || t3_is_vec)
	{
	  arg2 = opencl_value_cast (type3, arg2);
	  type2 = arg2->type ();
	}
      else if (!t2_is_vec || !t3_is_vec)
	{
	  /* Throw an error if arg2 or arg3 aren't vectors.  */
	  error (_("\
Cannot perform conditional operation on incompatible types"));
	}

      eltype2 = check_typedef (type2->target_type ());
      eltype3 = check_typedef (type3->target_type ());

      if (!get_array_bounds (type1, &lowb1, &highb1)
	  || !get_array_bounds (type2, &lowb2, &highb2)
	  || !get_array_bounds (type3, &lowb3, &highb3))
	error (_("Could not determine the vector bounds"));

      /* Throw an error if the types of arg2 or arg3 are incompatible.  */
      if (eltype2->code () != eltype3->code ()
	  || eltype2->length () != eltype3->length ()
	  || eltype2->is_unsigned () != eltype3->is_unsigned ()
	  || lowb2 != lowb3 || highb2 != highb3)
	error (_("\
Cannot perform operation on vectors with different types"));

      /* Throw an error if the sizes of arg1 and arg2/arg3 differ.  */
      if (lowb1 != lowb2 || lowb1 != lowb3
	  || highb1 != highb2 || highb1 != highb3)
	error (_("\
Cannot perform conditional operation on vectors with different sizes"));

      ret = value::allocate (type2);

      for (i = 0; i < highb1 - lowb1 + 1; i++)
	{
	  tmp = value_logical_not (value_subscript (arg1, i)) ?
	    value_subscript (arg3, i) : value_subscript (arg2, i);
	  memcpy (ret->contents_writeable ().data () +
		  i * eltype2->length (), tmp->contents_all ().data (),
		  eltype2->length ());
	}

      return ret;
    }
  else
    {
      if (value_logical_not (arg1))
	return std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
      else
	return std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    }
}

} /* namespace expr */

/* Class representing the OpenCL language.  */

class opencl_language : public language_defn
{
public:
  opencl_language ()
    : language_defn (language_opencl)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "opencl"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "OpenCL C"; }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    /* Helper function to allow shorter lines below.  */
    auto add  = [&] (struct type * t) -> struct type *
    {
      lai->add_primitive_type (t);
      return t;
    };

/* Helper macro to create strings.  */
#define OCL_STRING(S) #S

/* This macro allocates and assigns the type struct pointers
   for the vector types.  */
#define BUILD_OCL_VTYPES(TYPE, ELEMENT_TYPE)			\
    do								\
      {								\
	struct type *tmp;					\
	tmp = add (init_vector_type (ELEMENT_TYPE, 2));		\
	tmp->set_name (OCL_STRING(TYPE ## 2));			\
	tmp = add (init_vector_type (ELEMENT_TYPE, 3));		\
	tmp->set_name (OCL_STRING(TYPE ## 3));			\
	tmp->set_length (4 * (ELEMENT_TYPE)->length ());	\
	tmp = add (init_vector_type (ELEMENT_TYPE, 4));		\
	tmp->set_name (OCL_STRING(TYPE ## 4));			\
	tmp = add (init_vector_type (ELEMENT_TYPE, 8));		\
	tmp->set_name (OCL_STRING(TYPE ## 8));			\
	tmp = init_vector_type (ELEMENT_TYPE, 16);		\
	tmp->set_name (OCL_STRING(TYPE ## 16));			\
      }								\
    while (false)

    struct type *el_type, *char_type, *int_type;

    type_allocator alloc (gdbarch);
    char_type = el_type = add (init_integer_type (alloc, 8, 0, "char"));
    BUILD_OCL_VTYPES (char, el_type);
    el_type = add (init_integer_type (alloc, 8, 1, "uchar"));
    BUILD_OCL_VTYPES (uchar, el_type);
    el_type = add (init_integer_type (alloc, 16, 0, "short"));
    BUILD_OCL_VTYPES (short, el_type);
    el_type = add (init_integer_type (alloc, 16, 1, "ushort"));
    BUILD_OCL_VTYPES (ushort, el_type);
    int_type = el_type = add (init_integer_type (alloc, 32, 0, "int"));
    BUILD_OCL_VTYPES (int, el_type);
    el_type = add (init_integer_type (alloc, 32, 1, "uint"));
    BUILD_OCL_VTYPES (uint, el_type);
    el_type = add (init_integer_type (alloc, 64, 0, "long"));
    BUILD_OCL_VTYPES (long, el_type);
    el_type = add (init_integer_type (alloc, 64, 1, "ulong"));
    BUILD_OCL_VTYPES (ulong, el_type);
    el_type = add (init_float_type (alloc, 16, "half", floatformats_ieee_half));
    BUILD_OCL_VTYPES (half, el_type);
    el_type = add (init_float_type (alloc, 32, "float", floatformats_ieee_single));
    BUILD_OCL_VTYPES (float, el_type);
    el_type = add (init_float_type (alloc, 64, "double", floatformats_ieee_double));
    BUILD_OCL_VTYPES (double, el_type);

    add (init_boolean_type (alloc, 8, 1, "bool"));
    add (init_integer_type (alloc, 8, 1, "unsigned char"));
    add (init_integer_type (alloc, 16, 1, "unsigned short"));
    add (init_integer_type (alloc, 32, 1, "unsigned int"));
    add (init_integer_type (alloc, 64, 1, "unsigned long"));
    add (init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 1, "size_t"));
    add (init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 0, "ptrdiff_t"));
    add (init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 0, "intptr_t"));
    add (init_integer_type (alloc, gdbarch_ptr_bit (gdbarch), 1, "uintptr_t"));
    add (builtin_type (gdbarch)->builtin_void);

    /* Type of elements of strings.  */
    lai->set_string_char_type (char_type);

    /* Specifies the return type of logical and relational operations.  */
    lai->set_bool_type (int_type, "int");
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
    /* We nearly always defer to C type printing, except that vector types
       are considered primitive in OpenCL, and should always be printed
       using their TYPE_NAME.  */
    if (show > 0)
      {
	type = check_typedef (type);
	if (type->code () == TYPE_CODE_ARRAY && type->is_vector ()
	    && type->name () != NULL)
	  show = 0;
      }

    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  enum macro_expansion macro_expansion () const override
  { return macro_expansion_c; }
};

/* Single instance of the OpenCL language class.  */

static opencl_language opencl_language_defn;

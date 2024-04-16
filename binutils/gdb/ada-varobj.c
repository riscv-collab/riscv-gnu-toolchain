/* varobj support for Ada.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "ada-lang.h"
#include "varobj.h"
#include "language.h"
#include "valprint.h"

/* Implementation principle used in this unit:

   For our purposes, the meat of the varobj object is made of two
   elements: The varobj's (struct) value, and the varobj's (struct)
   type.  In most situations, the varobj has a non-NULL value, and
   the type becomes redundant, as it can be directly derived from
   the value.  In the initial implementation of this unit, most
   routines would only take a value, and return a value.

   But there are many situations where it is possible for a varobj
   to have a NULL value.  For instance, if the varobj becomes out of
   scope.  Or better yet, when the varobj is the child of another
   NULL pointer varobj.  In that situation, we must rely on the type
   instead of the value to create the child varobj.

   That's why most functions below work with a (value, type) pair.
   The value may or may not be NULL.  But the type is always expected
   to be set.  When the value is NULL, then we work with the type
   alone, and keep the value NULL.  But when the value is not NULL,
   then we work using the value, because it provides more information.
   But we still always set the type as well, even if that type could
   easily be derived from the value.  The reason behind this is that
   it allows the code to use the type without having to worry about
   it being set or not.  It makes the code clearer.  */

static int ada_varobj_get_number_of_children (struct value *parent_value,
					      struct type *parent_type);

/* A convenience function that decodes the VALUE_PTR/TYPE_PTR couple:
   If there is a value (*VALUE_PTR not NULL), then perform the decoding
   using it, and compute the associated type from the resulting value.
   Otherwise, compute a static approximation of *TYPE_PTR, leaving
   *VALUE_PTR unchanged.

   The results are written in place.  */

static void
ada_varobj_decode_var (struct value **value_ptr, struct type **type_ptr)
{
  if (*value_ptr)
    *value_ptr = ada_get_decoded_value (*value_ptr);

  if (*value_ptr != nullptr)
    *type_ptr = ada_check_typedef ((*value_ptr)->type ());
  else
    *type_ptr = ada_get_decoded_type (*type_ptr);
}

/* Return a string containing an image of the given scalar value.
   VAL is the numeric value, while TYPE is the value's type.
   This is useful for plain integers, of course, but even more
   so for enumerated types.  */

static std::string
ada_varobj_scalar_image (struct type *type, LONGEST val)
{
  string_file buf;

  ada_print_scalar (type, val, &buf);
  return buf.release ();
}

/* Assuming that the (PARENT_VALUE, PARENT_TYPE) pair designates
   a struct or union, compute the (CHILD_VALUE, CHILD_TYPE) couple
   corresponding to the field number FIELDNO.  */

static void
ada_varobj_struct_elt (struct value *parent_value,
		       struct type *parent_type,
		       int fieldno,
		       struct value **child_value,
		       struct type **child_type)
{
  struct value *value = NULL;
  struct type *type = NULL;

  if (parent_value)
    {
      value = value_field (parent_value, fieldno);
      type = value->type ();
    }
  else
    type = parent_type->field (fieldno).type ();

  if (child_value)
    *child_value = value;
  if (child_type)
    *child_type = type;
}

/* Assuming that the (PARENT_VALUE, PARENT_TYPE) pair is a pointer or
   reference, return a (CHILD_VALUE, CHILD_TYPE) couple corresponding
   to the dereferenced value.  */

static void
ada_varobj_ind (struct value *parent_value,
		struct type *parent_type,
		struct value **child_value,
		struct type **child_type)
{
  struct value *value = NULL;
  struct type *type = NULL;

  if (ada_is_array_descriptor_type (parent_type))
    {
      /* This can only happen when PARENT_VALUE is NULL.  Otherwise,
	 ada_get_decoded_value would have transformed our parent_type
	 into a simple array pointer type.  */
      gdb_assert (parent_value == NULL);
      gdb_assert (parent_type->code () == TYPE_CODE_TYPEDEF);

      /* Decode parent_type by the equivalent pointer to (decoded)
	 array.  */
      while (parent_type->code () == TYPE_CODE_TYPEDEF)
	parent_type = parent_type->target_type ();
      parent_type = ada_coerce_to_simple_array_type (parent_type);
      parent_type = lookup_pointer_type (parent_type);
    }

  /* If parent_value is a null pointer, then only perform static
     dereferencing.  We cannot dereference null pointers.  */
  if (parent_value && value_as_address (parent_value) == 0)
    parent_value = NULL;

  if (parent_value)
    {
      value = ada_value_ind (parent_value);
      type = value->type ();
    }
  else
    type = parent_type->target_type ();

  if (child_value)
    *child_value = value;
  if (child_type)
    *child_type = type;
}

/* Assuming that the (PARENT_VALUE, PARENT_TYPE) pair is a simple
   array (TYPE_CODE_ARRAY), return the (CHILD_VALUE, CHILD_TYPE)
   pair corresponding to the element at ELT_INDEX.  */

static void
ada_varobj_simple_array_elt (struct value *parent_value,
			     struct type *parent_type,
			     int elt_index,
			     struct value **child_value,
			     struct type **child_type)
{
  struct value *value = NULL;
  struct type *type = NULL;

  if (parent_value)
    {
      struct value *index_value =
	value_from_longest (parent_type->index_type (), elt_index);

      value = ada_value_subscript (parent_value, 1, &index_value);
      type = value->type ();
    }
  else
    type = parent_type->target_type ();

  if (child_value)
    *child_value = value;
  if (child_type)
    *child_type = type;
}

/* Given the decoded value and decoded type of a variable object,
   adjust the value and type to those necessary for getting children
   of the variable object.

   The replacement is performed in place.  */

static void
ada_varobj_adjust_for_child_access (struct value **value,
				    struct type **type)
{
   /* Pointers to struct/union types are special: Instead of having
      one child (the struct), their children are the components of
      the struct/union type.  We handle this situation by dereferencing
      the (value, type) couple.  */
  if ((*type)->code () == TYPE_CODE_PTR
      && ((*type)->target_type ()->code () == TYPE_CODE_STRUCT
	  || (*type)->target_type ()->code () == TYPE_CODE_UNION)
      && *value != nullptr
      && value_as_address (*value) != 0
      && !ada_is_array_descriptor_type ((*type)->target_type ())
      && !ada_is_constrained_packed_array_type ((*type)->target_type ()))
    ada_varobj_ind (*value, *type, value, type);

  /* If this is a tagged type, we need to transform it a bit in order
     to be able to fetch its full view.  As always with tagged types,
     we can only do that if we have a value.  */
  if (*value != NULL && ada_is_tagged_type (*type, 1))
    {
      *value = ada_tag_value_at_base_address (*value);
      *type = (*value)->type ();
    }
}

/* Assuming that the (PARENT_VALUE, PARENT_TYPE) pair is an array
   (any type of array, "simple" or not), return the number of children
   that this array contains.  */

static int
ada_varobj_get_array_number_of_children (struct value *parent_value,
					 struct type *parent_type)
{
  LONGEST lo, hi;

  if (parent_value == NULL
      && is_dynamic_type (parent_type->index_type ()))
    {
      /* This happens when listing the children of an object
	 which does not exist in memory (Eg: when requesting
	 the children of a null pointer, which is allowed by
	 varobj).  The array index type being dynamic, we cannot
	 determine how many elements this array has.  Just assume
	 it has none.  */
      return 0;
    }

  if (!get_array_bounds (parent_type, &lo, &hi))
    {
      /* Could not get the array bounds.  Pretend this is an empty array.  */
      warning (_("unable to get bounds of array, assuming null array"));
      return 0;
    }

  /* Ada allows the upper bound to be less than the lower bound,
     in order to specify empty arrays...  */
  if (hi < lo)
    return 0;

  return hi - lo + 1;
}

/* Assuming that the (PARENT_VALUE, PARENT_TYPE) pair is a struct or
   union, return the number of children this struct contains.  */

static int
ada_varobj_get_struct_number_of_children (struct value *parent_value,
					  struct type *parent_type)
{
  int n_children = 0;
  int i;

  gdb_assert (parent_type->code () == TYPE_CODE_STRUCT
	      || parent_type->code () == TYPE_CODE_UNION);

  for (i = 0; i < parent_type->num_fields (); i++)
    {
      if (ada_is_ignored_field (parent_type, i))
	continue;

      if (ada_is_wrapper_field (parent_type, i))
	{
	  struct value *elt_value;
	  struct type *elt_type;

	  ada_varobj_struct_elt (parent_value, parent_type, i,
				 &elt_value, &elt_type);
	  if (ada_is_tagged_type (elt_type, 0))
	    {
	      /* We must not use ada_varobj_get_number_of_children
		 to determine is element's number of children, because
		 this function first calls ada_varobj_decode_var,
		 which "fixes" the element.  For tagged types, this
		 includes reading the object's tag to determine its
		 real type, which happens to be the parent_type, and
		 leads to an infinite loop (because the element gets
		 fixed back into the parent).  */
	      n_children += ada_varobj_get_struct_number_of_children
		(elt_value, elt_type);
	    }
	  else
	    n_children += ada_varobj_get_number_of_children (elt_value, elt_type);
	}
      else if (ada_is_variant_part (parent_type, i))
	{
	  /* In normal situations, the variant part of the record should
	     have been "fixed". Or, in other words, it should have been
	     replaced by the branch of the variant part that is relevant
	     for our value.  But there are still situations where this
	     can happen, however (Eg. when our parent is a NULL pointer).
	     We do not support showing this part of the record for now,
	     so just pretend this field does not exist.  */
	}
      else
	n_children++;
    }

  return n_children;
}

/* Assuming that the (PARENT_VALUE, PARENT_TYPE) pair designates
   a pointer, return the number of children this pointer has.  */

static int
ada_varobj_get_ptr_number_of_children (struct value *parent_value,
				       struct type *parent_type)
{
  struct type *child_type = parent_type->target_type ();

  /* Pointer to functions and to void do not have a child, since
     you cannot print what they point to.  */
  if (child_type->code () == TYPE_CODE_FUNC
      || child_type->code () == TYPE_CODE_VOID)
    return 0;

  /* Only show children for non-null pointers.  */
  if (parent_value == nullptr || value_as_address (parent_value) == 0)
    return 0;

  /* All other types have 1 child.  */
  return 1;
}

/* Return the number of children for the (PARENT_VALUE, PARENT_TYPE)
   pair.  */

static int
ada_varobj_get_number_of_children (struct value *parent_value,
				   struct type *parent_type)
{
  ada_varobj_decode_var (&parent_value, &parent_type);
  ada_varobj_adjust_for_child_access (&parent_value, &parent_type);

  /* A typedef to an array descriptor in fact represents a pointer
     to an unconstrained array.  These types always have one child
     (the unconstrained array).  */
  if (ada_is_access_to_unconstrained_array (parent_type))
    return 1;

  if (parent_type->code () == TYPE_CODE_ARRAY)
    return ada_varobj_get_array_number_of_children (parent_value,
						    parent_type);

  if (parent_type->code () == TYPE_CODE_STRUCT
      || parent_type->code () == TYPE_CODE_UNION)
    return ada_varobj_get_struct_number_of_children (parent_value,
						     parent_type);

  if (parent_type->code () == TYPE_CODE_PTR)
    return ada_varobj_get_ptr_number_of_children (parent_value,
						  parent_type);

  /* All other types have no child.  */
  return 0;
}

/* Describe the child of the (PARENT_VALUE, PARENT_TYPE) pair
   whose index is CHILD_INDEX:

     - If CHILD_NAME is not NULL, then a copy of the child's name
       is saved in *CHILD_NAME.  This copy must be deallocated
       with xfree after use.

     - If CHILD_VALUE is not NULL, then save the child's value
       in *CHILD_VALUE. Same thing for the child's type with
       CHILD_TYPE if not NULL.

     - If CHILD_PATH_EXPR is not NULL, then compute the child's
       path expression.  The resulting string must be deallocated
       after use with xfree.

       Computing the child's path expression requires the PARENT_PATH_EXPR
       to be non-NULL.  Otherwise, PARENT_PATH_EXPR may be null if
       CHILD_PATH_EXPR is NULL.

  PARENT_NAME is the name of the parent, and should never be NULL.  */

static void ada_varobj_describe_child (struct value *parent_value,
				       struct type *parent_type,
				       const char *parent_name,
				       const char *parent_path_expr,
				       int child_index,
				       std::string *child_name,
				       struct value **child_value,
				       struct type **child_type,
				       std::string *child_path_expr);

/* Same as ada_varobj_describe_child, but limited to struct/union
   objects.  */

static void
ada_varobj_describe_struct_child (struct value *parent_value,
				  struct type *parent_type,
				  const char *parent_name,
				  const char *parent_path_expr,
				  int child_index,
				  std::string *child_name,
				  struct value **child_value,
				  struct type **child_type,
				  std::string *child_path_expr)
{
  int fieldno;
  int childno = 0;

  gdb_assert (parent_type->code () == TYPE_CODE_STRUCT
	      || parent_type->code () == TYPE_CODE_UNION);

  for (fieldno = 0; fieldno < parent_type->num_fields (); fieldno++)
    {
      if (ada_is_ignored_field (parent_type, fieldno))
	continue;

      if (ada_is_wrapper_field (parent_type, fieldno))
	{
	  struct value *elt_value;
	  struct type *elt_type;
	  int elt_n_children;

	  ada_varobj_struct_elt (parent_value, parent_type, fieldno,
				 &elt_value, &elt_type);
	  if (ada_is_tagged_type (elt_type, 0))
	    {
	      /* Same as in ada_varobj_get_struct_number_of_children:
		 For tagged types, we must be careful to not call
		 ada_varobj_get_number_of_children, to prevent our
		 element from being fixed back into the parent.  */
	      elt_n_children = ada_varobj_get_struct_number_of_children
		(elt_value, elt_type);
	    }
	  else
	    elt_n_children =
	      ada_varobj_get_number_of_children (elt_value, elt_type);

	  /* Is the child we're looking for one of the children
	     of this wrapper field?  */
	  if (child_index - childno < elt_n_children)
	    {
	      if (ada_is_tagged_type (elt_type, 0))
		{
		  /* Same as in ada_varobj_get_struct_number_of_children:
		     For tagged types, we must be careful to not call
		     ada_varobj_describe_child, to prevent our element
		     from being fixed back into the parent.  */
		  ada_varobj_describe_struct_child
		    (elt_value, elt_type, parent_name, parent_path_expr,
		     child_index - childno, child_name, child_value,
		     child_type, child_path_expr);
		}
	      else
		ada_varobj_describe_child (elt_value, elt_type,
					   parent_name, parent_path_expr,
					   child_index - childno,
					   child_name, child_value,
					   child_type, child_path_expr);
	      return;
	    }

	  /* The child we're looking for is beyond this wrapper
	     field, so skip all its children.  */
	  childno += elt_n_children;
	  continue;
	}
      else if (ada_is_variant_part (parent_type, fieldno))
	{
	  /* In normal situations, the variant part of the record should
	     have been "fixed". Or, in other words, it should have been
	     replaced by the branch of the variant part that is relevant
	     for our value.  But there are still situations where this
	     can happen, however (Eg. when our parent is a NULL pointer).
	     We do not support showing this part of the record for now,
	     so just pretend this field does not exist.  */
	  continue;
	}

      if (childno == child_index)
	{
	  if (child_name)
	    {
	      /* The name of the child is none other than the field's
		 name, except that we need to strip suffixes from it.
		 For instance, fields with alignment constraints will
		 have an __XVA suffix added to them.  */
	      const char *field_name = parent_type->field (fieldno).name ();
	      int child_name_len = ada_name_prefix_len (field_name);

	      *child_name = string_printf ("%.*s", child_name_len, field_name);
	    }

	  if (child_value && parent_value)
	    ada_varobj_struct_elt (parent_value, parent_type, fieldno,
				   child_value, NULL);

	  if (child_type)
	    ada_varobj_struct_elt (parent_value, parent_type, fieldno,
				   NULL, child_type);

	  if (child_path_expr)
	    {
	      /* The name of the child is none other than the field's
		 name, except that we need to strip suffixes from it.
		 For instance, fields with alignment constraints will
		 have an __XVA suffix added to them.  */
	      const char *field_name = parent_type->field (fieldno).name ();
	      int child_name_len = ada_name_prefix_len (field_name);

	      *child_path_expr =
		string_printf ("(%s).%.*s", parent_path_expr,
			       child_name_len, field_name);
	    }

	  return;
	}

      childno++;
    }

  /* Something went wrong.  Either we miscounted the number of
     children, or CHILD_INDEX was too high.  But we should never
     reach here.  We don't have enough information to recover
     nicely, so just raise an assertion failure.  */
  gdb_assert_not_reached ("unexpected code path");
}

/* Same as ada_varobj_describe_child, but limited to pointer objects.

   Note that CHILD_INDEX is unused in this situation, but still provided
   for consistency of interface with other routines describing an object's
   child.  */

static void
ada_varobj_describe_ptr_child (struct value *parent_value,
			       struct type *parent_type,
			       const char *parent_name,
			       const char *parent_path_expr,
			       int child_index,
			       std::string *child_name,
			       struct value **child_value,
			       struct type **child_type,
			       std::string *child_path_expr)
{
  if (child_name)
    *child_name = string_printf ("%s.all", parent_name);

  if (child_value && parent_value)
    ada_varobj_ind (parent_value, parent_type, child_value, NULL);

  if (child_type)
    ada_varobj_ind (parent_value, parent_type, NULL, child_type);

  if (child_path_expr)
    *child_path_expr = string_printf ("(%s).all", parent_path_expr);
}

/* Same as ada_varobj_describe_child, limited to simple array objects
   (TYPE_CODE_ARRAY only).

   Assumes that the (PARENT_VALUE, PARENT_TYPE) pair is properly decoded.
   This is done by ada_varobj_describe_child before calling us.  */

static void
ada_varobj_describe_simple_array_child (struct value *parent_value,
					struct type *parent_type,
					const char *parent_name,
					const char *parent_path_expr,
					int child_index,
					std::string *child_name,
					struct value **child_value,
					struct type **child_type,
					std::string *child_path_expr)
{
  struct type *index_type;
  int real_index;

  gdb_assert (parent_type->code () == TYPE_CODE_ARRAY);

  index_type = parent_type->index_type ();
  real_index = child_index + ada_discrete_type_low_bound (index_type);

  if (child_name)
    *child_name = ada_varobj_scalar_image (index_type, real_index);

  if (child_value && parent_value)
    ada_varobj_simple_array_elt (parent_value, parent_type, real_index,
				 child_value, NULL);

  if (child_type)
    ada_varobj_simple_array_elt (parent_value, parent_type, real_index,
				 NULL, child_type);

  if (child_path_expr)
    {
      std::string index_img = ada_varobj_scalar_image (index_type, real_index);

      /* Enumeration litterals by themselves are potentially ambiguous.
	 For instance, consider the following package spec:

	    package Pck is
	       type Color is (Red, Green, Blue, White);
	       type Blood_Cells is (White, Red);
	    end Pck;

	 In this case, the litteral "red" for instance, or even
	 the fully-qualified litteral "pck.red" cannot be resolved
	 by itself.  Type qualification is needed to determine which
	 enumeration litterals should be used.

	 The following variable will be used to contain the name
	 of the array index type when such type qualification is
	 needed.  */
      const char *index_type_name = NULL;
      std::string decoded;

      /* If the index type is a range type, find the base type.  */
      while (index_type->code () == TYPE_CODE_RANGE)
	index_type = index_type->target_type ();

      if (index_type->code () == TYPE_CODE_ENUM
	  || index_type->code () == TYPE_CODE_BOOL)
	{
	  index_type_name = ada_type_name (index_type);
	  if (index_type_name)
	    {
	      decoded = ada_decode (index_type_name);
	      index_type_name = decoded.c_str ();
	    }
	}

      if (index_type_name != NULL)
	*child_path_expr =
	  string_printf ("(%s)(%.*s'(%s))", parent_path_expr,
			 ada_name_prefix_len (index_type_name),
			 index_type_name, index_img.c_str ());
      else
	*child_path_expr =
	  string_printf ("(%s)(%s)", parent_path_expr, index_img.c_str ());
    }
}

/* See description at declaration above.  */

static void
ada_varobj_describe_child (struct value *parent_value,
			   struct type *parent_type,
			   const char *parent_name,
			   const char *parent_path_expr,
			   int child_index,
			   std::string *child_name,
			   struct value **child_value,
			   struct type **child_type,
			   std::string *child_path_expr)
{
  /* We cannot compute the child's path expression without
     the parent's path expression.  This is a pre-condition
     for calling this function.  */
  if (child_path_expr)
    gdb_assert (parent_path_expr != NULL);

  ada_varobj_decode_var (&parent_value, &parent_type);
  ada_varobj_adjust_for_child_access (&parent_value, &parent_type);

  if (child_name)
    *child_name = std::string ();
  if (child_value)
    *child_value = NULL;
  if (child_type)
    *child_type = NULL;
  if (child_path_expr)
    *child_path_expr = std::string ();

  if (ada_is_access_to_unconstrained_array (parent_type))
    {
      ada_varobj_describe_ptr_child (parent_value, parent_type,
				     parent_name, parent_path_expr,
				     child_index, child_name,
				     child_value, child_type,
				     child_path_expr);
      return;
    }

  if (parent_type->code () == TYPE_CODE_ARRAY)
    {
      ada_varobj_describe_simple_array_child
	(parent_value, parent_type, parent_name, parent_path_expr,
	 child_index, child_name, child_value, child_type,
	 child_path_expr);
      return;
    }

  if (parent_type->code () == TYPE_CODE_STRUCT
      || parent_type->code () == TYPE_CODE_UNION)
    {
      ada_varobj_describe_struct_child (parent_value, parent_type,
					parent_name, parent_path_expr,
					child_index, child_name,
					child_value, child_type,
					child_path_expr);
      return;
    }

  if (parent_type->code () == TYPE_CODE_PTR)
    {
      ada_varobj_describe_ptr_child (parent_value, parent_type,
				     parent_name, parent_path_expr,
				     child_index, child_name,
				     child_value, child_type,
				     child_path_expr);
      return;
    }

  /* It should never happen.  But rather than crash, report dummy names
     and return a NULL child_value.  */
  if (child_name)
    *child_name = "???";
}

/* Return the name of the child number CHILD_INDEX of the (PARENT_VALUE,
   PARENT_TYPE) pair.  PARENT_NAME is the name of the PARENT.  */

static std::string
ada_varobj_get_name_of_child (struct value *parent_value,
			      struct type *parent_type,
			      const char *parent_name, int child_index)
{
  std::string child_name;

  ada_varobj_describe_child (parent_value, parent_type, parent_name,
			     NULL, child_index, &child_name, NULL,
			     NULL, NULL);
  return child_name;
}

/* Return the path expression of the child number CHILD_INDEX of
   the (PARENT_VALUE, PARENT_TYPE) pair.  PARENT_NAME is the name
   of the parent, and PARENT_PATH_EXPR is the parent's path expression.
   Both must be non-NULL.  */

static std::string
ada_varobj_get_path_expr_of_child (struct value *parent_value,
				   struct type *parent_type,
				   const char *parent_name,
				   const char *parent_path_expr,
				   int child_index)
{
  std::string child_path_expr;

  ada_varobj_describe_child (parent_value, parent_type, parent_name,
			     parent_path_expr, child_index, NULL,
			     NULL, NULL, &child_path_expr);

  return child_path_expr;
}

/* Return the value of child number CHILD_INDEX of the (PARENT_VALUE,
   PARENT_TYPE) pair.  PARENT_NAME is the name of the parent.  */

static struct value *
ada_varobj_get_value_of_child (struct value *parent_value,
			       struct type *parent_type,
			       const char *parent_name, int child_index)
{
  struct value *child_value;

  ada_varobj_describe_child (parent_value, parent_type, parent_name,
			     NULL, child_index, NULL, &child_value,
			     NULL, NULL);

  return child_value;
}

/* Return the type of child number CHILD_INDEX of the (PARENT_VALUE,
   PARENT_TYPE) pair.  */

static struct type *
ada_varobj_get_type_of_child (struct value *parent_value,
			      struct type *parent_type,
			      int child_index)
{
  struct type *child_type;

  ada_varobj_describe_child (parent_value, parent_type, NULL, NULL,
			     child_index, NULL, NULL, &child_type, NULL);

  return child_type;
}

/* Return a string that contains the image of the given VALUE, using
   the print options OPTS as the options for formatting the result.

   The resulting string must be deallocated after use with xfree.  */

static std::string
ada_varobj_get_value_image (struct value *value,
			    struct value_print_options *opts)
{
  string_file buffer;

  common_val_print (value, &buffer, 0, opts, current_language);
  return buffer.release ();
}

/* Assuming that the (VALUE, TYPE) pair designates an array varobj,
   return a string that is suitable for use in the "value" field of
   the varobj output.  Most of the time, this is the number of elements
   in the array inside square brackets, but there are situations where
   it's useful to add more info.

   OPTS are the print options used when formatting the result.

   The result should be deallocated after use using xfree.  */

static std::string
ada_varobj_get_value_of_array_variable (struct value *value,
					struct type *type,
					struct value_print_options *opts)
{
  const int numchild = ada_varobj_get_array_number_of_children (value, type);

  /* If we have a string, provide its contents in the "value" field.
     Otherwise, the only other way to inspect the contents of the string
     is by looking at the value of each element, as in any other array,
     which is not very convenient...  */
  if (value
      && ada_is_string_type (type)
      && (opts->format == 0 || opts->format == 's'))
    {
      std::string str = ada_varobj_get_value_image (value, opts);
      return string_printf ("[%d] %s", numchild, str.c_str ());
    }
  else
    return string_printf ("[%d]", numchild);
}

/* Return a string representation of the (VALUE, TYPE) pair, using
   the given print options OPTS as our formatting options.  */

static std::string
ada_varobj_get_value_of_variable (struct value *value,
				  struct type *type,
				  struct value_print_options *opts)
{
  ada_varobj_decode_var (&value, &type);

  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return "{...}";
    case TYPE_CODE_ARRAY:
      return ada_varobj_get_value_of_array_variable (value, type, opts);
    default:
      if (!value)
	return "";
      else
	return ada_varobj_get_value_image (value, opts);
    }
}

/* Ada specific callbacks for VAROBJs.  */

static int
ada_number_of_children (const struct varobj *var)
{
  return ada_varobj_get_number_of_children (var->value.get (), var->type);
}

static std::string
ada_name_of_variable (const struct varobj *parent)
{
  return c_varobj_ops.name_of_variable (parent);
}

static std::string
ada_name_of_child (const struct varobj *parent, int index)
{
  return ada_varobj_get_name_of_child (parent->value.get (), parent->type,
				       parent->name.c_str (), index);
}

static std::string
ada_path_expr_of_child (const struct varobj *child)
{
  const struct varobj *parent = child->parent;
  const char *parent_path_expr = varobj_get_path_expr (parent);

  return ada_varobj_get_path_expr_of_child (parent->value.get (),
					    parent->type,
					    parent->name.c_str (),
					    parent_path_expr,
					    child->index);
}

static struct value *
ada_value_of_child (const struct varobj *parent, int index)
{
  return ada_varobj_get_value_of_child (parent->value.get (), parent->type,
					parent->name.c_str (), index);
}

static struct type *
ada_type_of_child (const struct varobj *parent, int index)
{
  return ada_varobj_get_type_of_child (parent->value.get (), parent->type,
				       index);
}

static std::string
ada_value_of_variable (const struct varobj *var,
		       enum varobj_display_formats format)
{
  struct value_print_options opts;

  varobj_formatted_print_options (&opts, format);

  return ada_varobj_get_value_of_variable (var->value.get (), var->type,
					   &opts);
}

/* Implement the "value_is_changeable_p" routine for Ada.  */

static bool
ada_value_is_changeable_p (const struct varobj *var)
{
  struct type *type = (var->value != nullptr
		       ? var->value->type () : var->type);

  if (type->code () == TYPE_CODE_REF)
    type = type->target_type ();

  if (ada_is_access_to_unconstrained_array (type))
    {
      /* This is in reality a pointer to an unconstrained array.
	 its value is changeable.  */
      return true;
    }

  if (ada_is_string_type (type))
    {
      /* We display the contents of the string in the array's
	 "value" field.  The contents can change, so consider
	 that the array is changeable.  */
      return true;
    }

  return varobj_default_value_is_changeable_p (var);
}

/* Implement the "value_has_mutated" routine for Ada.  */

static bool
ada_value_has_mutated (const struct varobj *var, struct value *new_val,
		       struct type *new_type)
{
  int from = -1;
  int to = -1;

  /* If the number of fields have changed, then for sure the type
     has mutated.  */
  if (ada_varobj_get_number_of_children (new_val, new_type)
      != var->num_children)
    return true;

  /* If the number of fields have remained the same, then we need
     to check the name of each field.  If they remain the same,
     then chances are the type hasn't mutated.  This is technically
     an incomplete test, as the child's type might have changed
     despite the fact that the name remains the same.  But we'll
     handle this situation by saying that the child has mutated,
     not this value.

     If only part (or none!) of the children have been fetched,
     then only check the ones we fetched.  It does not matter
     to the frontend whether a child that it has not fetched yet
     has mutated or not. So just assume it hasn't.  */

  varobj_restrict_range (var->children, &from, &to);
  for (int i = from; i < to; i++)
    if (ada_varobj_get_name_of_child (new_val, new_type,
				      var->name.c_str (), i)
	!= var->children[i]->name)
      return true;

  return false;
}

/* varobj operations for ada.  */

const struct lang_varobj_ops ada_varobj_ops =
{
  ada_number_of_children,
  ada_name_of_variable,
  ada_name_of_child,
  ada_path_expr_of_child,
  ada_value_of_child,
  ada_type_of_child,
  ada_value_of_variable,
  ada_value_is_changeable_p,
  ada_value_has_mutated,
  varobj_default_is_path_expr_parent
};

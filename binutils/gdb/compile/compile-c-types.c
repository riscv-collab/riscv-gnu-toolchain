/* Convert types from GDB to GCC

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "compile-internal.h"
#include "compile-c.h"
#include "objfiles.h"

/* Convert a pointer type to its gcc representation.  */

static gcc_type
convert_pointer (compile_c_instance *context, struct type *type)
{
  gcc_type target = context->convert_type (type->target_type ());

  return context->plugin ().build_pointer_type (target);
}

/* Convert an array type to its gcc representation.  */

static gcc_type
convert_array (compile_c_instance *context, struct type *type)
{
  gcc_type element_type;
  struct type *range = type->index_type ();

  element_type = context->convert_type (type->target_type ());

  if (!range->bounds ()->low.is_constant ())
    return context->plugin ().error (_("array type with non-constant"
				       " lower bound is not supported"));
  if (range->bounds ()->low.const_val () != 0)
    return context->plugin ().error (_("cannot convert array type with "
				       "non-zero lower bound to C"));

  if (range->bounds ()->high.kind () == PROP_LOCEXPR
      || range->bounds ()->high.kind () == PROP_LOCLIST)
    {
      gcc_type result;

      if (type->is_vector ())
	return context->plugin ().error (_("variably-sized vector type"
					   " is not supported"));

      std::string upper_bound
	= c_get_range_decl_name (&range->bounds ()->high);
      result = context->plugin ().build_vla_array_type (element_type,
							upper_bound.c_str ());
      return result;
    }
  else
    {
      LONGEST low_bound, high_bound, count;

      if (!get_array_bounds (type, &low_bound, &high_bound))
	count = -1;
      else
	{
	  gdb_assert (low_bound == 0); /* Ensured above.  */
	  count = high_bound + 1;
	}

      if (type->is_vector ())
	return context->plugin ().build_vector_type (element_type, count);
      return context->plugin ().build_array_type (element_type, count);
    }
}

/* Convert a struct or union type to its gcc representation.  */

static gcc_type
convert_struct_or_union (compile_c_instance *context, struct type *type)
{
  int i;
  gcc_type result;

  /* First we create the resulting type and enter it into our hash
     table.  This lets recursive types work.  */
  if (type->code () == TYPE_CODE_STRUCT)
    result = context->plugin ().build_record_type ();
  else
    {
      gdb_assert (type->code () == TYPE_CODE_UNION);
      result = context->plugin ().build_union_type ();
    }
  context->insert_type (type, result);

  for (i = 0; i < type->num_fields (); ++i)
    {
      gcc_type field_type;
      unsigned long bitsize = type->field (i).bitsize ();

      field_type = context->convert_type (type->field (i).type ());
      if (bitsize == 0)
	bitsize = 8 * type->field (i).type ()->length ();
      context->plugin ().build_add_field (result,
					  type->field (i).name (),
					  field_type,
					  bitsize,
					  type->field (i).loc_bitpos ());
    }

  context->plugin ().finish_record_or_union (result, type->length ());
  return result;
}

/* Convert an enum type to its gcc representation.  */

static gcc_type
convert_enum (compile_c_instance *context, struct type *type)
{
  gcc_type int_type, result;
  int i;

  int_type = context->plugin ().int_type_v0 (type->is_unsigned (),
					     type->length ());

  result = context->plugin ().build_enum_type (int_type);
  for (i = 0; i < type->num_fields (); ++i)
    {
      context->plugin ().build_add_enum_constant
	(result, type->field (i).name (), type->field (i).loc_enumval ());
    }

  context->plugin ().finish_enum_type (result);

  return result;
}

/* Convert a function type to its gcc representation.  */

static gcc_type
convert_func (compile_c_instance *context, struct type *type)
{
  int i;
  gcc_type result, return_type;
  struct gcc_type_array array;
  int is_varargs = type->has_varargs () || !type->is_prototyped ();

  struct type *target_type = type->target_type ();

  /* Functions with no debug info have no return type.  Ideally we'd
     want to fallback to the type of the cast just before the
     function, like GDB's built-in expression parser, but we don't
     have access to that type here.  For now, fallback to int, like
     GDB's parser used to do.  */
  if (target_type == NULL)
    {
      target_type = builtin_type (type->arch ())->builtin_int;
      warning (_("function has unknown return type; assuming int"));
    }

  /* This approach means we can't make self-referential function
     types.  Those are impossible in C, though.  */
  return_type = context->convert_type (target_type);

  array.n_elements = type->num_fields ();
  std::vector<gcc_type> elements (array.n_elements);
  array.elements = elements.data ();
  for (i = 0; i < type->num_fields (); ++i)
    array.elements[i] = context->convert_type (type->field (i).type ());

  result = context->plugin ().build_function_type (return_type,
						   &array, is_varargs);

  return result;
}

/* Convert an integer type to its gcc representation.  */

static gcc_type
convert_int (compile_c_instance *context, struct type *type)
{
  if (context->plugin ().version () >= GCC_C_FE_VERSION_1)
    {
      if (type->has_no_signedness ())
	{
	  gdb_assert (type->length () == 1);
	  return context->plugin ().char_type ();
	}
      return context->plugin ().int_type (type->is_unsigned (),
					  type->length (),
					  type->name ());
    }
  else
    return context->plugin ().int_type_v0 (type->is_unsigned (),
					   type->length ());
}

/* Convert a floating-point type to its gcc representation.  */

static gcc_type
convert_float (compile_c_instance *context, struct type *type)
{
  if (context->plugin ().version () >= GCC_C_FE_VERSION_1)
    return context->plugin ().float_type (type->length (),
					  type->name ());
  else
    return context->plugin ().float_type_v0 (type->length ());
}

/* Convert the 'void' type to its gcc representation.  */

static gcc_type
convert_void (compile_c_instance *context, struct type *type)
{
  return context->plugin ().void_type ();
}

/* Convert a boolean type to its gcc representation.  */

static gcc_type
convert_bool (compile_c_instance *context, struct type *type)
{
  return context->plugin ().bool_type ();
}

/* Convert a qualified type to its gcc representation.  */

static gcc_type
convert_qualified (compile_c_instance *context, struct type *type)
{
  struct type *unqual = make_unqualified_type (type);
  gcc_type unqual_converted;
  gcc_qualifiers_flags quals = 0;

  unqual_converted = context->convert_type (unqual);

  if (TYPE_CONST (type))
    quals |= GCC_QUALIFIER_CONST;
  if (TYPE_VOLATILE (type))
    quals |= GCC_QUALIFIER_VOLATILE;
  if (TYPE_RESTRICT (type))
    quals |= GCC_QUALIFIER_RESTRICT;

  return context->plugin ().build_qualified_type (unqual_converted,
						  quals.raw ());
}

/* Convert a complex type to its gcc representation.  */

static gcc_type
convert_complex (compile_c_instance *context, struct type *type)
{
  gcc_type base = context->convert_type (type->target_type ());

  return context->plugin ().build_complex_type (base);
}

/* A helper function which knows how to convert most types from their
   gdb representation to the corresponding gcc form.  This examines
   the TYPE and dispatches to the appropriate conversion function.  It
   returns the gcc type.  */

static gcc_type
convert_type_basic (compile_c_instance *context, struct type *type)
{
  /* If we are converting a qualified type, first convert the
     unqualified type and then apply the qualifiers.  */
  if ((type->instance_flags () & (TYPE_INSTANCE_FLAG_CONST
				  | TYPE_INSTANCE_FLAG_VOLATILE
				  | TYPE_INSTANCE_FLAG_RESTRICT)) != 0)
    return convert_qualified (context, type);

  switch (type->code ())
    {
    case TYPE_CODE_PTR:
      return convert_pointer (context, type);

    case TYPE_CODE_ARRAY:
      return convert_array (context, type);

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return convert_struct_or_union (context, type);

    case TYPE_CODE_ENUM:
      return convert_enum (context, type);

    case TYPE_CODE_FUNC:
      return convert_func (context, type);

    case TYPE_CODE_INT:
      return convert_int (context, type);

    case TYPE_CODE_FLT:
      return convert_float (context, type);

    case TYPE_CODE_VOID:
      return convert_void (context, type);

    case TYPE_CODE_BOOL:
      return convert_bool (context, type);

    case TYPE_CODE_COMPLEX:
      return convert_complex (context, type);

    case TYPE_CODE_ERROR:
      {
	/* Ideally, if we get here due to a cast expression, we'd use
	   the cast-to type as the variable's type, like GDB's
	   built-in parser does.  For now, assume "int" like GDB's
	   built-in parser used to do, but at least warn.  */
	struct type *fallback = builtin_type (type->arch ())->builtin_int;
	warning (_("variable has unknown type; assuming int"));
	return convert_int (context, fallback);
      }
    }

  return context->plugin ().error (_("cannot convert gdb type to gcc type"));
}

/* Default compile flags for C.  */

const char *compile_c_instance::m_default_cflags = "-std=gnu11"
  /* Otherwise the .o file may need
     "_Unwind_Resume" and
     "__gcc_personality_v0".  */
  " -fno-exceptions"
  " -Wno-implicit-function-declaration";

/* See compile-c.h.  */

gcc_type
compile_c_instance::convert_type (struct type *type)
{
  /* We don't ever have to deal with typedefs in this code, because
     those are only needed as symbols by the C compiler.  */
  type = check_typedef (type);

  gcc_type result;
  if (get_cached_type (type, &result))
    return result;

  result = convert_type_basic (this, type);
  insert_type (type, result);
  return result;
}



/* C plug-in wrapper.  */

#define FORWARD(OP,...) m_context->c_ops->OP(m_context, ##__VA_ARGS__)
#define GCC_METHOD0(R, N) \
  R gcc_c_plugin::N () const \
  { return FORWARD (N); }
#define GCC_METHOD1(R, N, A) \
  R gcc_c_plugin::N (A a) const \
  { return FORWARD (N, a); }
#define GCC_METHOD2(R, N, A, B) \
  R gcc_c_plugin::N (A a, B b) const \
  { return FORWARD (N, a, b); }
#define GCC_METHOD3(R, N, A, B, C) \
  R gcc_c_plugin::N (A a, B b, C c)  const \
  { return FORWARD (N, a, b, c); }
#define GCC_METHOD4(R, N, A, B, C, D) \
  R gcc_c_plugin::N (A a, B b, C c, D d) const \
  { return FORWARD (N, a, b, c, d); }
#define GCC_METHOD5(R, N, A, B, C, D, E) \
  R gcc_c_plugin::N (A a, B b, C c, D d, E e) const \
  { return FORWARD (N, a, b, c, d, e); }
#define GCC_METHOD7(R, N, A, B, C, D, E, F, G) \
  R gcc_c_plugin::N (A a, B b, C c, D d, E e, F f, G g) const \
  { return FORWARD (N, a, b, c, d, e, f, g); }

#include "gcc-c-fe.def"

#undef GCC_METHOD0
#undef GCC_METHOD1
#undef GCC_METHOD2
#undef GCC_METHOD3
#undef GCC_METHOD4
#undef GCC_METHOD5
#undef GCC_METHOD7
#undef FORWARD

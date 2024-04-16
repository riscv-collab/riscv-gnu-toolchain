/* Scheme interface to types.

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
#include "top.h"
#include "arch-utils.h"
#include "value.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "language.h"
#include "bcache.h"
#include "dwarf2/loc.h"
#include "typeprint.h"
#include "guile-internal.h"

/* The <gdb:type> smob.
   The type is chained with all types associated with its objfile, if any.
   This lets us copy the underlying struct type when the objfile is
   deleted.  */

struct type_smob
{
  /* This always appears first.
     eqable_gdb_smob is used so that types are eq?-able.
     Also, a type object can be associated with an objfile.  eqable_gdb_smob
     lets us track the lifetime of all types associated with an objfile.
     When an objfile is deleted we need to invalidate the type object.  */
  eqable_gdb_smob base;

  /* The GDB type structure this smob is wrapping.  */
  struct type *type;
};

/* A field smob.  */

struct field_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* Backlink to the containing <gdb:type> object.  */
  SCM type_scm;

  /* The field number in TYPE_SCM.  */
  int field_num;
};

static const char type_smob_name[] = "gdb:type";
static const char field_smob_name[] = "gdb:field";

static const char not_composite_error[] =
  N_("type is not a structure, union, or enum type");

/* The tag Guile knows the type smob by.  */
static scm_t_bits type_smob_tag;

/* The tag Guile knows the field smob by.  */
static scm_t_bits field_smob_tag;

/* The "next" procedure for field iterators.  */
static SCM tyscm_next_field_x_proc;

/* Keywords used in argument passing.  */
static SCM block_keyword;

static int tyscm_copy_type_recursive (void **slot, void *info);

/* Called when an objfile is about to be deleted.
   Make a copy of all types associated with OBJFILE.  */

struct tyscm_deleter
{
  void operator() (htab_t htab)
  {
    if (!gdb_scheme_initialized)
      return;

    gdb_assert (htab != nullptr);
    htab_up copied_types = create_copied_types_hash ();
    htab_traverse_noresize (htab, tyscm_copy_type_recursive, copied_types.get ());
    htab_delete (htab);
  }
};

static const registry<objfile>::key<htab, tyscm_deleter>
     tyscm_objfile_data_key;

/* Hash table to uniquify global (non-objfile-owned) types.  */
static htab_t global_types_map;

static struct type *tyscm_get_composite (struct type *type);

/* Return the type field of T_SMOB.
   This exists so that we don't have to export the struct's contents.  */

struct type *
tyscm_type_smob_type (type_smob *t_smob)
{
  return t_smob->type;
}

/* Return the name of TYPE in expanded form.  If there's an error
   computing the name, throws the gdb exception with scm_throw.  */

static std::string
tyscm_type_name (struct type *type)
{
  SCM excp;
  try
    {
      string_file stb;

      current_language->print_type (type, "", &stb, -1, 0,
				    &type_print_raw_options);
      return stb.release ();
    }
  catch (const gdb_exception_forced_quit &except)
    {
      quit_force (NULL, 0);
    }
  catch (const gdb_exception &except)
    {
      excp = gdbscm_scm_from_gdb_exception (unpack (except));
    }

  gdbscm_throw (excp);
}

/* Administrivia for type smobs.  */

/* Helper function to hash a type_smob.  */

static hashval_t
tyscm_hash_type_smob (const void *p)
{
  const type_smob *t_smob = (const type_smob *) p;

  return htab_hash_pointer (t_smob->type);
}

/* Helper function to compute equality of type_smobs.  */

static int
tyscm_eq_type_smob (const void *ap, const void *bp)
{
  const type_smob *a = (const type_smob *) ap;
  const type_smob *b = (const type_smob *) bp;

  return (a->type == b->type
	  && a->type != NULL);
}

/* Return the struct type pointer -> SCM mapping table.
   If type is owned by an objfile, the mapping table is created if necessary.
   Otherwise, type is not owned by an objfile, and we use
   global_types_map.  */

static htab_t
tyscm_type_map (struct type *type)
{
  struct objfile *objfile = type->objfile_owner ();
  htab_t htab;

  if (objfile == NULL)
    return global_types_map;

  htab = tyscm_objfile_data_key.get (objfile);
  if (htab == NULL)
    {
      htab = gdbscm_create_eqable_gsmob_ptr_map (tyscm_hash_type_smob,
						 tyscm_eq_type_smob);
      tyscm_objfile_data_key.set (objfile, htab);
    }

  return htab;
}

/* The smob "free" function for <gdb:type>.  */

static size_t
tyscm_free_type_smob (SCM self)
{
  type_smob *t_smob = (type_smob *) SCM_SMOB_DATA (self);

  if (t_smob->type != NULL)
    {
      htab_t htab = tyscm_type_map (t_smob->type);

      gdbscm_clear_eqable_gsmob_ptr_slot (htab, &t_smob->base);
    }

  /* Not necessary, done to catch bugs.  */
  t_smob->type = NULL;

  return 0;
}

/* The smob "print" function for <gdb:type>.  */

static int
tyscm_print_type_smob (SCM self, SCM port, scm_print_state *pstate)
{
  type_smob *t_smob = (type_smob *) SCM_SMOB_DATA (self);
  std::string name = tyscm_type_name (t_smob->type);

  /* pstate->writingp = zero if invoked by display/~A, and nonzero if
     invoked by write/~S.  What to do here may need to evolve.
     IWBN if we could pass an argument to format that would we could use
     instead of writingp.  */
  if (pstate->writingp)
    gdbscm_printf (port, "#<%s ", type_smob_name);

  scm_puts (name.c_str (), port);

  if (pstate->writingp)
    scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* The smob "equal?" function for <gdb:type>.  */

static SCM
tyscm_equal_p_type_smob (SCM type1_scm, SCM type2_scm)
{
  type_smob *type1_smob, *type2_smob;
  struct type *type1, *type2;
  bool result = false;

  SCM_ASSERT_TYPE (tyscm_is_type (type1_scm), type1_scm, SCM_ARG1, FUNC_NAME,
		   type_smob_name);
  SCM_ASSERT_TYPE (tyscm_is_type (type2_scm), type2_scm, SCM_ARG2, FUNC_NAME,
		   type_smob_name);
  type1_smob = (type_smob *) SCM_SMOB_DATA (type1_scm);
  type2_smob = (type_smob *) SCM_SMOB_DATA (type2_scm);
  type1 = type1_smob->type;
  type2 = type2_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      result = types_deeply_equal (type1, type2);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return scm_from_bool (result);
}

/* Low level routine to create a <gdb:type> object.  */

static SCM
tyscm_make_type_smob (void)
{
  type_smob *t_smob = (type_smob *)
    scm_gc_malloc (sizeof (type_smob), type_smob_name);
  SCM t_scm;

  /* This must be filled in by the caller.  */
  t_smob->type = NULL;

  t_scm = scm_new_smob (type_smob_tag, (scm_t_bits) t_smob);
  gdbscm_init_eqable_gsmob (&t_smob->base, t_scm);

  return t_scm;
}

/* Return non-zero if SCM is a <gdb:type> object.  */

int
tyscm_is_type (SCM self)
{
  return SCM_SMOB_PREDICATE (type_smob_tag, self);
}

/* (type? object) -> boolean */

static SCM
gdbscm_type_p (SCM self)
{
  return scm_from_bool (tyscm_is_type (self));
}

/* Return the existing object that encapsulates TYPE, or create a new
   <gdb:type> object.  */

SCM
tyscm_scm_from_type (struct type *type)
{
  htab_t htab;
  eqable_gdb_smob **slot;
  type_smob *t_smob, t_smob_for_lookup;
  SCM t_scm;

  /* If we've already created a gsmob for this type, return it.
     This makes types eq?-able.  */
  htab = tyscm_type_map (type);
  t_smob_for_lookup.type = type;
  slot = gdbscm_find_eqable_gsmob_ptr_slot (htab, &t_smob_for_lookup.base);
  if (*slot != NULL)
    return (*slot)->containing_scm;

  t_scm = tyscm_make_type_smob ();
  t_smob = (type_smob *) SCM_SMOB_DATA (t_scm);
  t_smob->type = type;
  gdbscm_fill_eqable_gsmob_ptr_slot (slot, &t_smob->base);

  return t_scm;
}

/* Returns the <gdb:type> object in SELF.
   Throws an exception if SELF is not a <gdb:type> object.  */

static SCM
tyscm_get_type_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (tyscm_is_type (self), self, arg_pos, func_name,
		   type_smob_name);

  return self;
}

/* Returns a pointer to the type smob of SELF.
   Throws an exception if SELF is not a <gdb:type> object.  */

type_smob *
tyscm_get_type_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM t_scm = tyscm_get_type_arg_unsafe (self, arg_pos, func_name);
  type_smob *t_smob = (type_smob *) SCM_SMOB_DATA (t_scm);

  return t_smob;
}

/* Return the type field of T_SCM, an object of type <gdb:type>.
   This exists so that we don't have to export the struct's contents.  */

struct type *
tyscm_scm_to_type (SCM t_scm)
{
  type_smob *t_smob;

  gdb_assert (tyscm_is_type (t_scm));
  t_smob = (type_smob *) SCM_SMOB_DATA (t_scm);
  return t_smob->type;
}

/* Helper function to make a deep copy of the type.  */

static int
tyscm_copy_type_recursive (void **slot, void *info)
{
  type_smob *t_smob = (type_smob *) *slot;
  htab_t copied_types = (htab_t) info;
  htab_t htab;
  eqable_gdb_smob **new_slot;
  type_smob t_smob_for_lookup;

  htab_empty (copied_types);
  t_smob->type = copy_type_recursive (t_smob->type, copied_types);

  /* The eq?-hashtab that the type lived in is going away.
     Add the type to its new eq?-hashtab: Otherwise if/when the type is later
     garbage collected we'll assert-fail if the type isn't in the hashtab.
     PR 16612.

     Types now live in "arch space", and things like "char" that came from
     the objfile *could* be considered eq? with the arch "char" type.
     However, they weren't before the objfile got deleted, so making them
     eq? now is debatable.  */
  htab = tyscm_type_map (t_smob->type);
  t_smob_for_lookup.type = t_smob->type;
  new_slot = gdbscm_find_eqable_gsmob_ptr_slot (htab, &t_smob_for_lookup.base);
  gdb_assert (*new_slot == NULL);
  gdbscm_fill_eqable_gsmob_ptr_slot (new_slot, &t_smob->base);

  return 1;
}


/* Administrivia for field smobs.  */

/* The smob "print" function for <gdb:field>.  */

static int
tyscm_print_field_smob (SCM self, SCM port, scm_print_state *pstate)
{
  field_smob *f_smob = (field_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", field_smob_name);
  scm_write (f_smob->type_scm, port);
  gdbscm_printf (port, " %d", f_smob->field_num);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:field> object for field FIELD_NUM
   of type TYPE_SCM.  */

static SCM
tyscm_make_field_smob (SCM type_scm, int field_num)
{
  field_smob *f_smob = (field_smob *)
    scm_gc_malloc (sizeof (field_smob), field_smob_name);
  SCM result;

  f_smob->type_scm = type_scm;
  f_smob->field_num = field_num;
  result = scm_new_smob (field_smob_tag, (scm_t_bits) f_smob);
  gdbscm_init_gsmob (&f_smob->base);

  return result;
}

/* Return non-zero if SCM is a <gdb:field> object.  */

static int
tyscm_is_field (SCM self)
{
  return SCM_SMOB_PREDICATE (field_smob_tag, self);
}

/* (field? object) -> boolean */

static SCM
gdbscm_field_p (SCM self)
{
  return scm_from_bool (tyscm_is_field (self));
}

/* Create a new <gdb:field> object that encapsulates field FIELD_NUM
   in type TYPE_SCM.  */

SCM
tyscm_scm_from_field (SCM type_scm, int field_num)
{
  return tyscm_make_field_smob (type_scm, field_num);
}

/* Returns the <gdb:field> object in SELF.
   Throws an exception if SELF is not a <gdb:field> object.  */

static SCM
tyscm_get_field_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (tyscm_is_field (self), self, arg_pos, func_name,
		   field_smob_name);

  return self;
}

/* Returns a pointer to the field smob of SELF.
   Throws an exception if SELF is not a <gdb:field> object.  */

static field_smob *
tyscm_get_field_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM f_scm = tyscm_get_field_arg_unsafe (self, arg_pos, func_name);
  field_smob *f_smob = (field_smob *) SCM_SMOB_DATA (f_scm);

  return f_smob;
}

/* Returns a pointer to the type struct in F_SMOB
   (the type the field is in).  */

static struct type *
tyscm_field_smob_containing_type (field_smob *f_smob)
{
  type_smob *t_smob;

  gdb_assert (tyscm_is_type (f_smob->type_scm));
  t_smob = (type_smob *) SCM_SMOB_DATA (f_smob->type_scm);

  return t_smob->type;
}

/* Returns a pointer to the field struct of F_SMOB.  */

static struct field *
tyscm_field_smob_to_field (field_smob *f_smob)
{
  struct type *type = tyscm_field_smob_containing_type (f_smob);

  /* This should be non-NULL by construction.  */
  gdb_assert (type->fields () != NULL);

  return &type->field (f_smob->field_num);
}

/* Type smob accessors.  */

/* (type-code <gdb:type>) -> integer
   Return the code for this type.  */

static SCM
gdbscm_type_code (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  return scm_from_int (type->code ());
}

/* (type-fields <gdb:type>) -> list
   Return a list of all fields.  Each element is a <gdb:field> object.
   This also supports arrays, we return a field list of one element,
   the range type.  */

static SCM
gdbscm_type_fields (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;
  struct type *containing_type;
  SCM containing_type_scm, result;
  int i;

  containing_type = tyscm_get_composite (type);
  if (containing_type == NULL)
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _(not_composite_error));

  /* If SELF is a typedef or reference, we want the underlying type,
     which is what tyscm_get_composite returns.  */
  if (containing_type == type)
    containing_type_scm = self;
  else
    containing_type_scm = tyscm_scm_from_type (containing_type);

  result = SCM_EOL;
  for (i = 0; i < containing_type->num_fields (); ++i)
    result = scm_cons (tyscm_make_field_smob (containing_type_scm, i), result);

  return scm_reverse_x (result, SCM_EOL);
}

/* (type-tag <gdb:type>) -> string
   Return the type's tag, or #f.  */

static SCM
gdbscm_type_tag (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;
  const char *tagname = nullptr;

  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_ENUM)
    tagname = type->name ();

  if (tagname == nullptr)
    return SCM_BOOL_F;
  return gdbscm_scm_from_c_string (tagname);
}

/* (type-name <gdb:type>) -> string
   Return the type's name, or #f.  */

static SCM
gdbscm_type_name (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  if (!type->name ())
    return SCM_BOOL_F;
  return gdbscm_scm_from_c_string (type->name ());
}

/* (type-print-name <gdb:type>) -> string
   Return the print name of type.
   TODO: template support elided for now.  */

static SCM
gdbscm_type_print_name (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;
  std::string thetype = tyscm_type_name (type);
  SCM result = gdbscm_scm_from_c_string (thetype.c_str ());

  return result;
}

/* (type-sizeof <gdb:type>) -> integer
   Return the size of the type represented by SELF, in bytes.  */

static SCM
gdbscm_type_sizeof (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  try
    {
      check_typedef (type);
    }
  catch (const gdb_exception &except)
    {
    }

  /* Ignore exceptions.  */

  return scm_from_long (type->length ());
}

/* (type-strip-typedefs <gdb:type>) -> <gdb:type>
   Return the type, stripped of typedefs. */

static SCM
gdbscm_type_strip_typedefs (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      type = check_typedef (type);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (type);
}

/* Strip typedefs and pointers/reference from a type.  Then check that
   it is a struct, union, or enum type.  If not, return NULL.  */

static struct type *
tyscm_get_composite (struct type *type)
{

  for (;;)
    {
      gdbscm_gdb_exception exc {};
      try
	{
	  type = check_typedef (type);
	}
      catch (const gdb_exception &except)
	{
	  exc = unpack (except);
	}

      GDBSCM_HANDLE_GDB_EXCEPTION (exc);
      if (type->code () != TYPE_CODE_PTR
	  && type->code () != TYPE_CODE_REF)
	break;
      type = type->target_type ();
    }

  /* If this is not a struct, union, or enum type, raise TypeError
     exception.  */
  if (type->code () != TYPE_CODE_STRUCT
      && type->code () != TYPE_CODE_UNION
      && type->code () != TYPE_CODE_ENUM)
    return NULL;

  return type;
}

/* Helper for tyscm_array and tyscm_vector.  */

static SCM
tyscm_array_1 (SCM self, SCM n1_scm, SCM n2_scm, int is_vector,
	       const char *func_name)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, func_name);
  struct type *type = t_smob->type;
  long n1, n2 = 0;
  struct type *array = NULL;

  gdbscm_parse_function_args (func_name, SCM_ARG2, NULL, "l|l",
			      n1_scm, &n1, n2_scm, &n2);

  if (SCM_UNBNDP (n2_scm))
    {
      n2 = n1;
      n1 = 0;
    }

  if (n2 < n1 - 1) /* Note: An empty array has n2 == n1 - 1.  */
    {
      gdbscm_out_of_range_error (func_name, SCM_ARG3,
				 scm_cons (scm_from_long (n1),
					   scm_from_long (n2)),
				 _("Array length must not be negative"));
    }

  gdbscm_gdb_exception exc {};
  try
    {
      array = lookup_array_range_type (type, n1, n2);
      if (is_vector)
	make_vector_type (array);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (array);
}

/* (type-array <gdb:type> [low-bound] high-bound) -> <gdb:type>
   The array has indices [low-bound,high-bound].
   If low-bound is not provided zero is used.
   Return an array type.

   IWBN if the one argument version specified a size, not the high bound.
   It's too easy to pass one argument thinking it is the size of the array.
   The current semantics are for compatibility with the Python version.
   Later we can add #:size.  */

static SCM
gdbscm_type_array (SCM self, SCM n1, SCM n2)
{
  return tyscm_array_1 (self, n1, n2, 0, FUNC_NAME);
}

/* (type-vector <gdb:type> [low-bound] high-bound) -> <gdb:type>
   The array has indices [low-bound,high-bound].
   If low-bound is not provided zero is used.
   Return a vector type.

   IWBN if the one argument version specified a size, not the high bound.
   It's too easy to pass one argument thinking it is the size of the array.
   The current semantics are for compatibility with the Python version.
   Later we can add #:size.  */

static SCM
gdbscm_type_vector (SCM self, SCM n1, SCM n2)
{
  return tyscm_array_1 (self, n1, n2, 1, FUNC_NAME);
}

/* (type-pointer <gdb:type>) -> <gdb:type>
   Return a <gdb:type> object which represents a pointer to SELF.  */

static SCM
gdbscm_type_pointer (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      type = lookup_pointer_type (type);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (type);
}

/* (type-range <gdb:type>) -> (low high)
   Return the range of a type represented by SELF.  The return type is
   a list.  The first element is the low bound, and the second element
   is the high bound.  */

static SCM
gdbscm_type_range (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;
  SCM low_scm, high_scm;
  /* Initialize these to appease GCC warnings.  */
  LONGEST low = 0, high = 0;

  SCM_ASSERT_TYPE (type->code () == TYPE_CODE_ARRAY
		   || type->code () == TYPE_CODE_STRING
		   || type->code () == TYPE_CODE_RANGE,
		   self, SCM_ARG1, FUNC_NAME, _("ranged type"));

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRING:
    case TYPE_CODE_RANGE:
      if (type->bounds ()->low.is_constant ())
	low = type->bounds ()->low.const_val ();
      else
	low = 0;

      if (type->bounds ()->high.is_constant ())
	high = type->bounds ()->high.const_val ();
      else
	high = 0;
      break;
    }

  low_scm = gdbscm_scm_from_longest (low);
  high_scm = gdbscm_scm_from_longest (high);

  return scm_list_2 (low_scm, high_scm);
}

/* (type-reference <gdb:type>) -> <gdb:type>
   Return a <gdb:type> object which represents a reference to SELF.  */

static SCM
gdbscm_type_reference (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      type = lookup_lvalue_reference_type (type);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (type);
}

/* (type-target <gdb:type>) -> <gdb:type>
   Return a <gdb:type> object which represents the target type of SELF.  */

static SCM
gdbscm_type_target (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  SCM_ASSERT (type->target_type (), self, SCM_ARG1, FUNC_NAME);

  return tyscm_scm_from_type (type->target_type ());
}

/* (type-const <gdb:type>) -> <gdb:type>
   Return a const-qualified type variant.  */

static SCM
gdbscm_type_const (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      type = make_cv_type (1, 0, type, NULL);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (type);
}

/* (type-volatile <gdb:type>) -> <gdb:type>
   Return a volatile-qualified type variant.  */

static SCM
gdbscm_type_volatile (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      type = make_cv_type (0, 1, type, NULL);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (type);
}

/* (type-unqualified <gdb:type>) -> <gdb:type>
   Return an unqualified type variant.  */

static SCM
gdbscm_type_unqualified (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  gdbscm_gdb_exception exc {};
  try
    {
      type = make_cv_type (0, 0, type, NULL);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return tyscm_scm_from_type (type);
}

/* Field related accessors of types.  */

/* (type-num-fields <gdb:type>) -> integer
   Return number of fields.  */

static SCM
gdbscm_type_num_fields (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  type = tyscm_get_composite (type);
  if (type == NULL)
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _(not_composite_error));

  return scm_from_long (type->num_fields ());
}

/* (type-field <gdb:type> string) -> <gdb:field>
   Return the <gdb:field> object for the field named by the argument.  */

static SCM
gdbscm_type_field (SCM self, SCM field_scm)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  SCM_ASSERT_TYPE (scm_is_string (field_scm), field_scm, SCM_ARG2, FUNC_NAME,
		   _("string"));

  /* We want just fields of this type, not of base types, so instead of
     using lookup_struct_elt_type, portions of that function are
     copied here.  */

  type = tyscm_get_composite (type);
  if (type == NULL)
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _(not_composite_error));

  {
    gdb::unique_xmalloc_ptr<char> field = gdbscm_scm_to_c_string (field_scm);

    for (int i = 0; i < type->num_fields (); i++)
      {
	const char *t_field_name = type->field (i).name ();

	if (t_field_name && (strcmp_iw (t_field_name, field.get ()) == 0))
	  {
	    field.reset (nullptr);
	    return tyscm_make_field_smob (self, i);
	  }
      }
  }

  gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, field_scm,
			     _("Unknown field"));
}

/* (type-has-field? <gdb:type> string) -> boolean
   Return boolean indicating if type SELF has FIELD_SCM (a string).  */

static SCM
gdbscm_type_has_field_p (SCM self, SCM field_scm)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;

  SCM_ASSERT_TYPE (scm_is_string (field_scm), field_scm, SCM_ARG2, FUNC_NAME,
		   _("string"));

  /* We want just fields of this type, not of base types, so instead of
     using lookup_struct_elt_type, portions of that function are
     copied here.  */

  type = tyscm_get_composite (type);
  if (type == NULL)
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _(not_composite_error));

  {
    gdb::unique_xmalloc_ptr<char> field
      = gdbscm_scm_to_c_string (field_scm);

    for (int i = 0; i < type->num_fields (); i++)
      {
	const char *t_field_name = type->field (i).name ();

	if (t_field_name && (strcmp_iw (t_field_name, field.get ()) == 0))
	  return SCM_BOOL_T;
      }
  }

  return SCM_BOOL_F;
}

/* (make-field-iterator <gdb:type>) -> <gdb:iterator>
   Make a field iterator object.  */

static SCM
gdbscm_make_field_iterator (SCM self)
{
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = t_smob->type;
  struct type *containing_type;
  SCM containing_type_scm;

  containing_type = tyscm_get_composite (type);
  if (containing_type == NULL)
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _(not_composite_error));

  /* If SELF is a typedef or reference, we want the underlying type,
     which is what tyscm_get_composite returns.  */
  if (containing_type == type)
    containing_type_scm = self;
  else
    containing_type_scm = tyscm_scm_from_type (containing_type);

  return gdbscm_make_iterator (containing_type_scm, scm_from_int (0),
			       tyscm_next_field_x_proc);
}

/* (type-next-field! <gdb:iterator>) -> <gdb:field>
   Return the next field in the iteration through the list of fields of the
   type, or (end-of-iteration).
   SELF is a <gdb:iterator> object created by gdbscm_make_field_iterator.
   This is the next! <gdb:iterator> function, not exported to the user.  */

static SCM
gdbscm_type_next_field_x (SCM self)
{
  iterator_smob *i_smob;
  type_smob *t_smob;
  struct type *type;
  SCM it_scm, result, progress, object;
  int field;

  it_scm = itscm_get_iterator_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  i_smob = (iterator_smob *) SCM_SMOB_DATA (it_scm);
  object = itscm_iterator_smob_object (i_smob);
  progress = itscm_iterator_smob_progress (i_smob);

  SCM_ASSERT_TYPE (tyscm_is_type (object), object,
		   SCM_ARG1, FUNC_NAME, type_smob_name);
  t_smob = (type_smob *) SCM_SMOB_DATA (object);
  type = t_smob->type;

  SCM_ASSERT_TYPE (scm_is_signed_integer (progress,
					  0, type->num_fields ()),
		   progress, SCM_ARG1, FUNC_NAME, _("integer"));
  field = scm_to_int (progress);

  if (field < type->num_fields ())
    {
      result = tyscm_make_field_smob (object, field);
      itscm_set_iterator_smob_progress_x (i_smob, scm_from_int (field + 1));
      return result;
    }

  return gdbscm_end_of_iteration ();
}

/* Field smob accessors.  */

/* (field-name <gdb:field>) -> string
   Return the name of this field or #f if there isn't one.  */

static SCM
gdbscm_field_name (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct field *field = tyscm_field_smob_to_field (f_smob);

  if (field->name () != nullptr)
    return gdbscm_scm_from_c_string (field->name ());
  return SCM_BOOL_F;
}

/* (field-type <gdb:field>) -> <gdb:type>
   Return the <gdb:type> object of the field or #f if there isn't one.  */

static SCM
gdbscm_field_type (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct field *field = tyscm_field_smob_to_field (f_smob);

  /* A field can have a NULL type in some situations.  */
  if (field->type ())
    return tyscm_scm_from_type (field->type ());
  return SCM_BOOL_F;
}

/* (field-enumval <gdb:field>) -> integer
   For enum values, return its value as an integer.  */

static SCM
gdbscm_field_enumval (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct field *field = tyscm_field_smob_to_field (f_smob);
  struct type *type = tyscm_field_smob_containing_type (f_smob);

  SCM_ASSERT_TYPE (type->code () == TYPE_CODE_ENUM,
		   self, SCM_ARG1, FUNC_NAME, _("enum type"));

  return scm_from_long (field->loc_enumval ());
}

/* (field-bitpos <gdb:field>) -> integer
   For bitfields, return its offset in bits.  */

static SCM
gdbscm_field_bitpos (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct field *field = tyscm_field_smob_to_field (f_smob);
  struct type *type = tyscm_field_smob_containing_type (f_smob);

  SCM_ASSERT_TYPE (type->code () != TYPE_CODE_ENUM,
		   self, SCM_ARG1, FUNC_NAME, _("non-enum type"));

  return scm_from_long (field->loc_bitpos ());
}

/* (field-bitsize <gdb:field>) -> integer
   Return the size of the field in bits.  */

static SCM
gdbscm_field_bitsize (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct field *field = tyscm_field_smob_to_field (f_smob);

  return scm_from_long (field->loc_bitpos ());
}

/* (field-artificial? <gdb:field>) -> boolean
   Return #t if field is artificial.  */

static SCM
gdbscm_field_artificial_p (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct field *field = tyscm_field_smob_to_field (f_smob);

  return scm_from_bool (field->is_artificial ());
}

/* (field-baseclass? <gdb:field>) -> boolean
   Return #t if field is a baseclass.  */

static SCM
gdbscm_field_baseclass_p (SCM self)
{
  field_smob *f_smob
    = tyscm_get_field_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct type *type = tyscm_field_smob_containing_type (f_smob);

  if (type->code () == TYPE_CODE_STRUCT)
    return scm_from_bool (f_smob->field_num < TYPE_N_BASECLASSES (type));
  return SCM_BOOL_F;
}

/* Return the type named TYPE_NAME in BLOCK.
   Returns NULL if not found.
   This routine does not throw an error.  */

static struct type *
tyscm_lookup_typename (const char *type_name, const struct block *block)
{
  struct type *type = NULL;

  try
    {
      if (startswith (type_name, "struct "))
	type = lookup_struct (type_name + 7, NULL);
      else if (startswith (type_name, "union "))
	type = lookup_union (type_name + 6, NULL);
      else if (startswith (type_name, "enum "))
	type = lookup_enum (type_name + 5, NULL);
      else
	type = lookup_typename (current_language,
				type_name, block, 0);
    }
  catch (const gdb_exception &except)
    {
      return NULL;
    }

  return type;
}

/* (lookup-type name [#:block <gdb:block>]) -> <gdb:type>
   TODO: legacy template support left out until needed.  */

static SCM
gdbscm_lookup_type (SCM name_scm, SCM rest)
{
  SCM keywords[] = { block_keyword, SCM_BOOL_F };
  char *name;
  SCM block_scm = SCM_BOOL_F;
  int block_arg_pos = -1;
  const struct block *block = NULL;
  struct type *type;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#O",
			      name_scm, &name,
			      rest, &block_arg_pos, &block_scm);

  if (block_arg_pos != -1)
    {
      SCM exception;

      block = bkscm_scm_to_block (block_scm, block_arg_pos, FUNC_NAME,
				  &exception);
      if (block == NULL)
	{
	  xfree (name);
	  gdbscm_throw (exception);
	}
    }
  type = tyscm_lookup_typename (name, block);
  xfree (name);

  if (type != NULL)
    return tyscm_scm_from_type (type);
  return SCM_BOOL_F;
}

/* Initialize the Scheme type code.  */


static const scheme_integer_constant type_integer_constants[] =
{
  /* This is kept for backward compatibility.  */
  { "TYPE_CODE_BITSTRING", -1 },

#define OP(SYM) { #SYM, SYM },
#include "type-codes.def"
#undef OP

  END_INTEGER_CONSTANTS
};

static const scheme_function type_functions[] =
{
  { "type?", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_p),
    "\
Return #t if the object is a <gdb:type> object." },

  { "lookup-type", 1, 0, 1, as_a_scm_t_subr (gdbscm_lookup_type),
    "\
Return the <gdb:type> object representing string or #f if not found.\n\
If block is given then the type is looked for in that block.\n\
\n\
  Arguments: string [#:block <gdb:block>]" },

  { "type-code", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_code),
    "\
Return the code of the type" },

  { "type-tag", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_tag),
    "\
Return the tag name of the type, or #f if there isn't one." },

  { "type-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_name),
    "\
Return the name of the type as a string, or #f if there isn't one." },

  { "type-print-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_print_name),
    "\
Return the print name of the type as a string." },

  { "type-sizeof", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_sizeof),
    "\
Return the size of the type, in bytes." },

  { "type-strip-typedefs", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_type_strip_typedefs),
    "\
Return a type formed by stripping the type of all typedefs." },

  { "type-array", 2, 1, 0, as_a_scm_t_subr (gdbscm_type_array),
    "\
Return a type representing an array of objects of the type.\n\
\n\
  Arguments: <gdb:type> [low-bound] high-bound\n\
    If low-bound is not provided zero is used.\n\
    N.B. If only the high-bound parameter is specified, it is not\n\
    the array size.\n\
    Valid bounds for array indices are [low-bound,high-bound]." },

  { "type-vector", 2, 1, 0, as_a_scm_t_subr (gdbscm_type_vector),
    "\
Return a type representing a vector of objects of the type.\n\
Vectors differ from arrays in that if the current language has C-style\n\
arrays, vectors don't decay to a pointer to the first element.\n\
They are first class values.\n\
\n\
  Arguments: <gdb:type> [low-bound] high-bound\n\
    If low-bound is not provided zero is used.\n\
    N.B. If only the high-bound parameter is specified, it is not\n\
    the array size.\n\
    Valid bounds for array indices are [low-bound,high-bound]." },

  { "type-pointer", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_pointer),
    "\
Return a type of pointer to the type." },

  { "type-range", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_range),
    "\
Return (low high) representing the range for the type." },

  { "type-reference", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_reference),
    "\
Return a type of reference to the type." },

  { "type-target", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_target),
    "\
Return the target type of the type." },

  { "type-const", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_const),
    "\
Return a const variant of the type." },

  { "type-volatile", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_volatile),
    "\
Return a volatile variant of the type." },

  { "type-unqualified", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_unqualified),
    "\
Return a variant of the type without const or volatile attributes." },

  { "type-num-fields", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_num_fields),
    "\
Return the number of fields of the type." },

  { "type-fields", 1, 0, 0, as_a_scm_t_subr (gdbscm_type_fields),
    "\
Return the list of <gdb:field> objects of fields of the type." },

  { "make-field-iterator", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_make_field_iterator),
    "\
Return a <gdb:iterator> object for iterating over the fields of the type." },

  { "type-field", 2, 0, 0, as_a_scm_t_subr (gdbscm_type_field),
    "\
Return the field named by string of the type.\n\
\n\
  Arguments: <gdb:type> string" },

  { "type-has-field?", 2, 0, 0, as_a_scm_t_subr (gdbscm_type_has_field_p),
    "\
Return #t if the type has field named string.\n\
\n\
  Arguments: <gdb:type> string" },

  { "field?", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_p),
    "\
Return #t if the object is a <gdb:field> object." },

  { "field-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_name),
    "\
Return the name of the field." },

  { "field-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_type),
    "\
Return the type of the field." },

  { "field-enumval", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_enumval),
    "\
Return the enum value represented by the field." },

  { "field-bitpos", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_bitpos),
    "\
Return the offset in bits of the field in its containing type." },

  { "field-bitsize", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_bitsize),
    "\
Return the size of the field in bits." },

  { "field-artificial?", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_artificial_p),
    "\
Return #t if the field is artificial." },

  { "field-baseclass?", 1, 0, 0, as_a_scm_t_subr (gdbscm_field_baseclass_p),
    "\
Return #t if the field is a baseclass." },

  END_FUNCTIONS
};

void
gdbscm_initialize_types (void)
{
  type_smob_tag = gdbscm_make_smob_type (type_smob_name, sizeof (type_smob));
  scm_set_smob_free (type_smob_tag, tyscm_free_type_smob);
  scm_set_smob_print (type_smob_tag, tyscm_print_type_smob);
  scm_set_smob_equalp (type_smob_tag, tyscm_equal_p_type_smob);

  field_smob_tag = gdbscm_make_smob_type (field_smob_name,
					  sizeof (field_smob));
  scm_set_smob_print (field_smob_tag, tyscm_print_field_smob);

  gdbscm_define_integer_constants (type_integer_constants, 1);
  gdbscm_define_functions (type_functions, 1);

  /* This function is "private".  */
  tyscm_next_field_x_proc
    = scm_c_define_gsubr ("%type-next-field!", 1, 0, 0,
			  as_a_scm_t_subr (gdbscm_type_next_field_x));
  scm_set_procedure_property_x (tyscm_next_field_x_proc,
				gdbscm_documentation_symbol,
				gdbscm_scm_from_c_string ("\
Internal function to assist the type fields iterator."));

  block_keyword = scm_from_latin1_keyword ("block");

  global_types_map = gdbscm_create_eqable_gsmob_ptr_map (tyscm_hash_type_smob,
							 tyscm_eq_type_smob);
}

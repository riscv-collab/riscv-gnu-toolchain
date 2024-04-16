/* Support routines for manipulating internal types for GDB.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support, using pieces from other GDB modules.

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
#include "bfd.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "expression.h"
#include "language.h"
#include "target.h"
#include "value.h"
#include "demangle.h"
#include "complaints.h"
#include "gdbcmd.h"
#include "cp-abi.h"
#include "hashtab.h"
#include "cp-support.h"
#include "bcache.h"
#include "dwarf2/loc.h"
#include "dwarf2/read.h"
#include "gdbcore.h"
#include "floatformat.h"
#include "f-lang.h"
#include <algorithm>
#include "gmp-utils.h"
#include "rust-lang.h"
#include "ada-lang.h"

/* The value of an invalid conversion badness.  */
#define INVALID_CONVERSION 100

static struct dynamic_prop_list *
copy_dynamic_prop_list (struct obstack *, struct dynamic_prop_list *);

/* Initialize BADNESS constants.  */

const struct rank LENGTH_MISMATCH_BADNESS = {INVALID_CONVERSION,0};

const struct rank TOO_FEW_PARAMS_BADNESS = {INVALID_CONVERSION,0};
const struct rank INCOMPATIBLE_TYPE_BADNESS = {INVALID_CONVERSION,0};

const struct rank EXACT_MATCH_BADNESS = {0,0};

const struct rank INTEGER_PROMOTION_BADNESS = {1,0};
const struct rank FLOAT_PROMOTION_BADNESS = {1,0};
const struct rank BASE_PTR_CONVERSION_BADNESS = {1,0};
const struct rank CV_CONVERSION_BADNESS = {1, 0};
const struct rank INTEGER_CONVERSION_BADNESS = {2,0};
const struct rank FLOAT_CONVERSION_BADNESS = {2,0};
const struct rank INT_FLOAT_CONVERSION_BADNESS = {2,0};
const struct rank VOID_PTR_CONVERSION_BADNESS = {2,0};
const struct rank BOOL_CONVERSION_BADNESS = {3,0};
const struct rank BASE_CONVERSION_BADNESS = {2,0};
const struct rank REFERENCE_CONVERSION_BADNESS = {2,0};
const struct rank REFERENCE_SEE_THROUGH_BADNESS = {0,1};
const struct rank NULL_POINTER_CONVERSION_BADNESS = {2,0};
const struct rank NS_POINTER_CONVERSION_BADNESS = {10,0};
const struct rank NS_INTEGER_POINTER_CONVERSION_BADNESS = {3,0};
const struct rank VARARG_BADNESS = {4, 0};

/* Floatformat pairs.  */
const struct floatformat *floatformats_ieee_half[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_half_big,
  &floatformat_ieee_half_little
};
const struct floatformat *floatformats_ieee_single[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_single_big,
  &floatformat_ieee_single_little
};
const struct floatformat *floatformats_ieee_double[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_double_big,
  &floatformat_ieee_double_little
};
const struct floatformat *floatformats_ieee_quad[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_quad_big,
  &floatformat_ieee_quad_little
};
const struct floatformat *floatformats_ieee_double_littlebyte_bigword[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_double_big,
  &floatformat_ieee_double_littlebyte_bigword
};
const struct floatformat *floatformats_i387_ext[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_i387_ext,
  &floatformat_i387_ext
};
const struct floatformat *floatformats_m68881_ext[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_m68881_ext,
  &floatformat_m68881_ext
};
const struct floatformat *floatformats_arm_ext[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_arm_ext_big,
  &floatformat_arm_ext_littlebyte_bigword
};
const struct floatformat *floatformats_ia64_spill[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ia64_spill_big,
  &floatformat_ia64_spill_little
};
const struct floatformat *floatformats_vax_f[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_vax_f,
  &floatformat_vax_f
};
const struct floatformat *floatformats_vax_d[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_vax_d,
  &floatformat_vax_d
};
const struct floatformat *floatformats_ibm_long_double[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ibm_long_double_big,
  &floatformat_ibm_long_double_little
};
const struct floatformat *floatformats_bfloat16[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_bfloat16_big,
  &floatformat_bfloat16_little
};

/* Should opaque types be resolved?  */

static bool opaque_type_resolution = true;

/* See gdbtypes.h.  */

unsigned int overload_debug = 0;

/* A flag to enable strict type checking.  */

static bool strict_type_checking = true;

/* A function to show whether opaque types are resolved.  */

static void
show_opaque_type_resolution (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c, 
			     const char *value)
{
  gdb_printf (file, _("Resolution of opaque struct/class/union types "
		      "(if set before loading symbols) is %s.\n"),
	      value);
}

/* A function to show whether C++ overload debugging is enabled.  */

static void
show_overload_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging of C++ overloading is %s.\n"), 
	      value);
}

/* A function to show the status of strict type checking.  */

static void
show_strict_type_checking (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Strict type checking is %s.\n"), value);
}


/* Helper function to initialize a newly allocated type.  Set type code
   to CODE and initialize the type-specific fields accordingly.  */

static void
set_type_code (struct type *type, enum type_code code)
{
  type->set_code (code);

  switch (code)
    {
      case TYPE_CODE_STRUCT:
      case TYPE_CODE_UNION:
      case TYPE_CODE_NAMESPACE:
	INIT_CPLUS_SPECIFIC (type);
	break;
      case TYPE_CODE_FLT:
	TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_FLOATFORMAT;
	break;
      case TYPE_CODE_FUNC:
	INIT_FUNC_SPECIFIC (type);
	break;
      case TYPE_CODE_FIXED_POINT:
	INIT_FIXED_POINT_SPECIFIC (type);
	break;
    }
}

/* See gdbtypes.h.  */

type *
type_allocator::new_type ()
{
  if (m_smash)
    return m_data.type;

  obstack *obstack = (m_is_objfile
		      ? &m_data.objfile->objfile_obstack
		      : gdbarch_obstack (m_data.gdbarch));

  /* Alloc the structure and start off with all fields zeroed.  */
  struct type *type = OBSTACK_ZALLOC (obstack, struct type);
  TYPE_MAIN_TYPE (type) = OBSTACK_ZALLOC (obstack, struct main_type);
  TYPE_MAIN_TYPE (type)->m_lang = m_lang;

  if (m_is_objfile)
    {
      OBJSTAT (m_data.objfile, n_types++);
      type->set_owner (m_data.objfile);
    }
  else
    type->set_owner (m_data.gdbarch);

  /* Initialize the fields that might not be zero.  */
  type->set_code (TYPE_CODE_UNDEF);
  TYPE_CHAIN (type) = type;	/* Chain back to itself.  */

  return type;
}

/* See gdbtypes.h.  */

type *
type_allocator::new_type (enum type_code code, int bit, const char *name)
{
  struct type *type = new_type ();
  set_type_code (type, code);
  gdb_assert ((bit % TARGET_CHAR_BIT) == 0);
  type->set_length (bit / TARGET_CHAR_BIT);

  if (name != nullptr)
    {
      obstack *obstack = (m_is_objfile
			  ? &m_data.objfile->objfile_obstack
			  : gdbarch_obstack (m_data.gdbarch));
      type->set_name (obstack_strdup (obstack, name));
    }

  return type;
}

/* See gdbtypes.h.  */

gdbarch *
type_allocator::arch ()
{
  if (m_smash)
    return m_data.type->arch ();
  if (m_is_objfile)
    return m_data.objfile->arch ();
  return m_data.gdbarch;
}

/* See gdbtypes.h.  */

gdbarch *
type::arch () const
{
  struct gdbarch *arch;

  if (this->is_objfile_owned ())
    arch = this->objfile_owner ()->arch ();
  else
    arch = this->arch_owner ();

  /* The ARCH can be NULL if TYPE is associated with neither an objfile nor
     a gdbarch, however, this is very rare, and even then, in most cases
     that type::arch is called, we assume that a non-NULL value is
     returned.  */
  gdb_assert (arch != nullptr);
  return arch;
}

/* See gdbtypes.h.  */

struct type *
get_target_type (struct type *type)
{
  if (type != NULL)
    {
      type = type->target_type ();
      if (type != NULL)
	type = check_typedef (type);
    }

  return type;
}

/* See gdbtypes.h.  */

unsigned int
type_length_units (struct type *type)
{
  int unit_size = gdbarch_addressable_memory_unit_size (type->arch ());

  return type->length () / unit_size;
}

/* Alloc a new type instance structure, fill it with some defaults,
   and point it at OLDTYPE.  Allocate the new type instance from the
   same place as OLDTYPE.  */

static struct type *
alloc_type_instance (struct type *oldtype)
{
  struct type *type;

  /* Allocate the structure.  */

  if (!oldtype->is_objfile_owned ())
    type = GDBARCH_OBSTACK_ZALLOC (oldtype->arch_owner (), struct type);
  else
    type = OBSTACK_ZALLOC (&oldtype->objfile_owner ()->objfile_obstack,
			   struct type);

  TYPE_MAIN_TYPE (type) = TYPE_MAIN_TYPE (oldtype);

  TYPE_CHAIN (type) = type;	/* Chain back to itself for now.  */

  return type;
}

/* Clear all remnants of the previous type at TYPE, in preparation for
   replacing it with something else.  Preserve owner information.  */

static void
smash_type (struct type *type)
{
  bool objfile_owned = type->is_objfile_owned ();
  objfile *objfile = type->objfile_owner ();
  gdbarch *arch = type->arch_owner ();

  memset (TYPE_MAIN_TYPE (type), 0, sizeof (struct main_type));

  /* Restore owner information.  */
  if (objfile_owned)
    type->set_owner (objfile);
  else
    type->set_owner (arch);

  /* For now, delete the rings.  */
  TYPE_CHAIN (type) = type;

  /* For now, leave the pointer/reference types alone.  */
}

/* Lookup a pointer to a type TYPE.  TYPEPTR, if nonzero, points
   to a pointer to memory where the pointer type should be stored.
   If *TYPEPTR is zero, update it to point to the pointer type we return.
   We allocate new memory if needed.  */

struct type *
make_pointer_type (struct type *type, struct type **typeptr)
{
  struct type *ntype;	/* New type */
  struct type *chain;

  ntype = TYPE_POINTER_TYPE (type);

  if (ntype)
    {
      if (typeptr == 0)
	return ntype;		/* Don't care about alloc, 
				   and have new type.  */
      else if (*typeptr == 0)
	{
	  *typeptr = ntype;	/* Tracking alloc, and have new type.  */
	  return ntype;
	}
    }

  if (typeptr == 0 || *typeptr == 0)	/* We'll need to allocate one.  */
    {
      ntype = type_allocator (type).new_type ();
      if (typeptr)
	*typeptr = ntype;
    }
  else			/* We have storage, but need to reset it.  */
    {
      ntype = *typeptr;
      chain = TYPE_CHAIN (ntype);
      smash_type (ntype);
      TYPE_CHAIN (ntype) = chain;
    }

  ntype->set_target_type (type);
  TYPE_POINTER_TYPE (type) = ntype;

  /* FIXME!  Assumes the machine has only one representation for pointers!  */

  ntype->set_length (gdbarch_ptr_bit (type->arch ()) / TARGET_CHAR_BIT);
  ntype->set_code (TYPE_CODE_PTR);

  /* Mark pointers as unsigned.  The target converts between pointers
     and addresses (CORE_ADDRs) using gdbarch_pointer_to_address and
     gdbarch_address_to_pointer.  */
  ntype->set_is_unsigned (true);

  /* Update the length of all the other variants of this type.  */
  chain = TYPE_CHAIN (ntype);
  while (chain != ntype)
    {
      chain->set_length (ntype->length ());
      chain = TYPE_CHAIN (chain);
    }

  return ntype;
}

/* Given a type TYPE, return a type of pointers to that type.
   May need to construct such a type if this is the first use.  */

struct type *
lookup_pointer_type (struct type *type)
{
  return make_pointer_type (type, (struct type **) 0);
}

/* Lookup a C++ `reference' to a type TYPE.  TYPEPTR, if nonzero,
   points to a pointer to memory where the reference type should be
   stored.  If *TYPEPTR is zero, update it to point to the reference
   type we return.  We allocate new memory if needed. REFCODE denotes
   the kind of reference type to lookup (lvalue or rvalue reference).  */

struct type *
make_reference_type (struct type *type, struct type **typeptr,
		      enum type_code refcode)
{
  struct type *ntype;	/* New type */
  struct type **reftype;
  struct type *chain;

  gdb_assert (refcode == TYPE_CODE_REF || refcode == TYPE_CODE_RVALUE_REF);

  ntype = (refcode == TYPE_CODE_REF ? TYPE_REFERENCE_TYPE (type)
	   : TYPE_RVALUE_REFERENCE_TYPE (type));

  if (ntype)
    {
      if (typeptr == 0)
	return ntype;		/* Don't care about alloc, 
				   and have new type.  */
      else if (*typeptr == 0)
	{
	  *typeptr = ntype;	/* Tracking alloc, and have new type.  */
	  return ntype;
	}
    }

  if (typeptr == 0 || *typeptr == 0)	/* We'll need to allocate one.  */
    {
      ntype = type_allocator (type).new_type ();
      if (typeptr)
	*typeptr = ntype;
    }
  else			/* We have storage, but need to reset it.  */
    {
      ntype = *typeptr;
      chain = TYPE_CHAIN (ntype);
      smash_type (ntype);
      TYPE_CHAIN (ntype) = chain;
    }

  ntype->set_target_type (type);
  reftype = (refcode == TYPE_CODE_REF ? &TYPE_REFERENCE_TYPE (type)
	     : &TYPE_RVALUE_REFERENCE_TYPE (type));

  *reftype = ntype;

  /* FIXME!  Assume the machine has only one representation for
     references, and that it matches the (only) representation for
     pointers!  */

  ntype->set_length (gdbarch_ptr_bit (type->arch ()) / TARGET_CHAR_BIT);
  ntype->set_code (refcode);

  *reftype = ntype;

  /* Update the length of all the other variants of this type.  */
  chain = TYPE_CHAIN (ntype);
  while (chain != ntype)
    {
      chain->set_length (ntype->length ());
      chain = TYPE_CHAIN (chain);
    }

  return ntype;
}

/* Same as above, but caller doesn't care about memory allocation
   details.  */

struct type *
lookup_reference_type (struct type *type, enum type_code refcode)
{
  return make_reference_type (type, (struct type **) 0, refcode);
}

/* Lookup the lvalue reference type for the type TYPE.  */

struct type *
lookup_lvalue_reference_type (struct type *type)
{
  return lookup_reference_type (type, TYPE_CODE_REF);
}

/* Lookup the rvalue reference type for the type TYPE.  */

struct type *
lookup_rvalue_reference_type (struct type *type)
{
  return lookup_reference_type (type, TYPE_CODE_RVALUE_REF);
}

/* Lookup a function type that returns type TYPE.  TYPEPTR, if
   nonzero, points to a pointer to memory where the function type
   should be stored.  If *TYPEPTR is zero, update it to point to the
   function type we return.  We allocate new memory if needed.  */

struct type *
make_function_type (struct type *type, struct type **typeptr)
{
  struct type *ntype;	/* New type */

  if (typeptr == 0 || *typeptr == 0)	/* We'll need to allocate one.  */
    {
      ntype = type_allocator (type).new_type ();
      if (typeptr)
	*typeptr = ntype;
    }
  else			/* We have storage, but need to reset it.  */
    {
      ntype = *typeptr;
      smash_type (ntype);
    }

  ntype->set_target_type (type);

  ntype->set_length (1);
  ntype->set_code (TYPE_CODE_FUNC);

  INIT_FUNC_SPECIFIC (ntype);

  return ntype;
}

/* Given a type TYPE, return a type of functions that return that type.
   May need to construct such a type if this is the first use.  */

struct type *
lookup_function_type (struct type *type)
{
  return make_function_type (type, (struct type **) 0);
}

/* Given a type TYPE and argument types, return the appropriate
   function type.  If the final type in PARAM_TYPES is NULL, make a
   varargs function.  */

struct type *
lookup_function_type_with_arguments (struct type *type,
				     int nparams,
				     struct type **param_types)
{
  struct type *fn = make_function_type (type, (struct type **) 0);
  int i;

  if (nparams > 0)
    {
      if (param_types[nparams - 1] == NULL)
	{
	  --nparams;
	  fn->set_has_varargs (true);
	}
      else if (check_typedef (param_types[nparams - 1])->code ()
	       == TYPE_CODE_VOID)
	{
	  --nparams;
	  /* Caller should have ensured this.  */
	  gdb_assert (nparams == 0);
	  fn->set_is_prototyped (true);
	}
      else
	fn->set_is_prototyped (true);
    }

  fn->alloc_fields (nparams);
  for (i = 0; i < nparams; ++i)
    fn->field (i).set_type (param_types[i]);

  return fn;
}

/* Identify address space identifier by name -- return a
   type_instance_flags.  */

type_instance_flags
address_space_name_to_type_instance_flags (struct gdbarch *gdbarch,
					   const char *space_identifier)
{
  type_instance_flags type_flags;

  /* Check for known address space delimiters.  */
  if (!strcmp (space_identifier, "code"))
    return TYPE_INSTANCE_FLAG_CODE_SPACE;
  else if (!strcmp (space_identifier, "data"))
    return TYPE_INSTANCE_FLAG_DATA_SPACE;
  else if (gdbarch_address_class_name_to_type_flags_p (gdbarch)
	   && gdbarch_address_class_name_to_type_flags (gdbarch,
							space_identifier,
							&type_flags))
    return type_flags;
  else
    error (_("Unknown address space specifier: \"%s\""), space_identifier);
}

/* Identify address space identifier by type_instance_flags and return
   the string version of the adress space name.  */

const char *
address_space_type_instance_flags_to_name (struct gdbarch *gdbarch,
					   type_instance_flags space_flag)
{
  if (space_flag & TYPE_INSTANCE_FLAG_CODE_SPACE)
    return "code";
  else if (space_flag & TYPE_INSTANCE_FLAG_DATA_SPACE)
    return "data";
  else if ((space_flag & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL)
	   && gdbarch_address_class_type_flags_to_name_p (gdbarch))
    return gdbarch_address_class_type_flags_to_name (gdbarch, space_flag);
  else
    return NULL;
}

/* Create a new type with instance flags NEW_FLAGS, based on TYPE.

   If STORAGE is non-NULL, create the new type instance there.
   STORAGE must be in the same obstack as TYPE.  */

static struct type *
make_qualified_type (struct type *type, type_instance_flags new_flags,
		     struct type *storage)
{
  struct type *ntype;

  ntype = type;
  do
    {
      if (ntype->instance_flags () == new_flags)
	return ntype;
      ntype = TYPE_CHAIN (ntype);
    }
  while (ntype != type);

  /* Create a new type instance.  */
  if (storage == NULL)
    ntype = alloc_type_instance (type);
  else
    {
      /* If STORAGE was provided, it had better be in the same objfile
	 as TYPE.  Otherwise, we can't link it into TYPE's cv chain:
	 if one objfile is freed and the other kept, we'd have
	 dangling pointers.  */
      gdb_assert (type->objfile_owner () == storage->objfile_owner ());

      ntype = storage;
      TYPE_MAIN_TYPE (ntype) = TYPE_MAIN_TYPE (type);
      TYPE_CHAIN (ntype) = ntype;
    }

  /* Pointers or references to the original type are not relevant to
     the new type.  */
  TYPE_POINTER_TYPE (ntype) = (struct type *) 0;
  TYPE_REFERENCE_TYPE (ntype) = (struct type *) 0;

  /* Chain the new qualified type to the old type.  */
  TYPE_CHAIN (ntype) = TYPE_CHAIN (type);
  TYPE_CHAIN (type) = ntype;

  /* Now set the instance flags and return the new type.  */
  ntype->set_instance_flags (new_flags);

  /* Set length of new type to that of the original type.  */
  ntype->set_length (type->length ());

  return ntype;
}

/* Make an address-space-delimited variant of a type -- a type that
   is identical to the one supplied except that it has an address
   space attribute attached to it (such as "code" or "data").

   The space attributes "code" and "data" are for Harvard
   architectures.  The address space attributes are for architectures
   which have alternately sized pointers or pointers with alternate
   representations.  */

struct type *
make_type_with_address_space (struct type *type,
			      type_instance_flags space_flag)
{
  type_instance_flags new_flags = ((type->instance_flags ()
				    & ~(TYPE_INSTANCE_FLAG_CODE_SPACE
					| TYPE_INSTANCE_FLAG_DATA_SPACE
					| TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL))
				   | space_flag);

  return make_qualified_type (type, new_flags, NULL);
}

/* Make a "c-v" variant of a type -- a type that is identical to the
   one supplied except that it may have const or volatile attributes
   CNST is a flag for setting the const attribute
   VOLTL is a flag for setting the volatile attribute
   TYPE is the base type whose variant we are creating.

   If TYPEPTR and *TYPEPTR are non-zero, then *TYPEPTR points to
   storage to hold the new qualified type; *TYPEPTR and TYPE must be
   in the same objfile.  Otherwise, allocate fresh memory for the new
   type whereever TYPE lives.  If TYPEPTR is non-zero, set it to the
   new type we construct.  */

struct type *
make_cv_type (int cnst, int voltl, 
	      struct type *type, 
	      struct type **typeptr)
{
  struct type *ntype;	/* New type */

  type_instance_flags new_flags = (type->instance_flags ()
				   & ~(TYPE_INSTANCE_FLAG_CONST
				       | TYPE_INSTANCE_FLAG_VOLATILE));

  if (cnst)
    new_flags |= TYPE_INSTANCE_FLAG_CONST;

  if (voltl)
    new_flags |= TYPE_INSTANCE_FLAG_VOLATILE;

  if (typeptr && *typeptr != NULL)
    {
      /* TYPE and *TYPEPTR must be in the same objfile.  We can't have
	 a C-V variant chain that threads across objfiles: if one
	 objfile gets freed, then the other has a broken C-V chain.

	 This code used to try to copy over the main type from TYPE to
	 *TYPEPTR if they were in different objfiles, but that's
	 wrong, too: TYPE may have a field list or member function
	 lists, which refer to types of their own, etc. etc.  The
	 whole shebang would need to be copied over recursively; you
	 can't have inter-objfile pointers.  The only thing to do is
	 to leave stub types as stub types, and look them up afresh by
	 name each time you encounter them.  */
      gdb_assert ((*typeptr)->objfile_owner () == type->objfile_owner ());
    }
  
  ntype = make_qualified_type (type, new_flags, 
			       typeptr ? *typeptr : NULL);

  if (typeptr != NULL)
    *typeptr = ntype;

  return ntype;
}

/* Make a 'restrict'-qualified version of TYPE.  */

struct type *
make_restrict_type (struct type *type)
{
  return make_qualified_type (type,
			      (type->instance_flags ()
			       | TYPE_INSTANCE_FLAG_RESTRICT),
			      NULL);
}

/* Make a type without const, volatile, or restrict.  */

struct type *
make_unqualified_type (struct type *type)
{
  return make_qualified_type (type,
			      (type->instance_flags ()
			       & ~(TYPE_INSTANCE_FLAG_CONST
				   | TYPE_INSTANCE_FLAG_VOLATILE
				   | TYPE_INSTANCE_FLAG_RESTRICT)),
			      NULL);
}

/* Make a '_Atomic'-qualified version of TYPE.  */

struct type *
make_atomic_type (struct type *type)
{
  return make_qualified_type (type,
			      (type->instance_flags ()
			       | TYPE_INSTANCE_FLAG_ATOMIC),
			      NULL);
}

/* Replace the contents of ntype with the type *type.  This changes the
   contents, rather than the pointer for TYPE_MAIN_TYPE (ntype); thus
   the changes are propogated to all types in the TYPE_CHAIN.

   In order to build recursive types, it's inevitable that we'll need
   to update types in place --- but this sort of indiscriminate
   smashing is ugly, and needs to be replaced with something more
   controlled.  TYPE_MAIN_TYPE is a step in this direction; it's not
   clear if more steps are needed.  */

void
replace_type (struct type *ntype, struct type *type)
{
  struct type *chain;

  /* These two types had better be in the same objfile.  Otherwise,
     the assignment of one type's main type structure to the other
     will produce a type with references to objects (names; field
     lists; etc.) allocated on an objfile other than its own.  */
  gdb_assert (ntype->objfile_owner () == type->objfile_owner ());

  *TYPE_MAIN_TYPE (ntype) = *TYPE_MAIN_TYPE (type);

  /* The type length is not a part of the main type.  Update it for
     each type on the variant chain.  */
  chain = ntype;
  do
    {
      /* Assert that this element of the chain has no address-class bits
	 set in its flags.  Such type variants might have type lengths
	 which are supposed to be different from the non-address-class
	 variants.  This assertion shouldn't ever be triggered because
	 symbol readers which do construct address-class variants don't
	 call replace_type().  */
      gdb_assert (TYPE_ADDRESS_CLASS_ALL (chain) == 0);

      chain->set_length (type->length ());
      chain = TYPE_CHAIN (chain);
    }
  while (ntype != chain);

  /* Assert that the two types have equivalent instance qualifiers.
     This should be true for at least all of our debug readers.  */
  gdb_assert (ntype->instance_flags () == type->instance_flags ());
}

/* Implement direct support for MEMBER_TYPE in GNU C++.
   May need to construct such a type if this is the first use.
   The TYPE is the type of the member.  The DOMAIN is the type
   of the aggregate that the member belongs to.  */

struct type *
lookup_memberptr_type (struct type *type, struct type *domain)
{
  struct type *mtype;

  mtype = type_allocator (type).new_type ();
  smash_to_memberptr_type (mtype, domain, type);
  return mtype;
}

/* Return a pointer-to-method type, for a method of type TO_TYPE.  */

struct type *
lookup_methodptr_type (struct type *to_type)
{
  struct type *mtype;

  mtype = type_allocator (to_type).new_type ();
  smash_to_methodptr_type (mtype, to_type);
  return mtype;
}

/* See gdbtypes.h.  */

bool
operator== (const dynamic_prop &l, const dynamic_prop &r)
{
  if (l.kind () != r.kind ())
    return false;

  switch (l.kind ())
    {
    case PROP_UNDEFINED:
      return true;
    case PROP_CONST:
      return l.const_val () == r.const_val ();
    case PROP_ADDR_OFFSET:
    case PROP_LOCEXPR:
    case PROP_LOCLIST:
      return l.baton () == r.baton ();
    case PROP_VARIANT_PARTS:
      return l.variant_parts () == r.variant_parts ();
    case PROP_TYPE:
      return l.original_type () == r.original_type ();
    }

  gdb_assert_not_reached ("unhandled dynamic_prop kind");
}

/* See gdbtypes.h.  */

bool
operator== (const range_bounds &l, const range_bounds &r)
{
#define FIELD_EQ(FIELD) (l.FIELD == r.FIELD)

  return (FIELD_EQ (low)
	  && FIELD_EQ (high)
	  && FIELD_EQ (flag_upper_bound_is_count)
	  && FIELD_EQ (flag_bound_evaluated)
	  && FIELD_EQ (bias));

#undef FIELD_EQ
}

/* See gdbtypes.h.  */

struct type *
create_range_type (type_allocator &alloc, struct type *index_type,
		   const struct dynamic_prop *low_bound,
		   const struct dynamic_prop *high_bound,
		   LONGEST bias)
{
  /* The INDEX_TYPE should be a type capable of holding the upper and lower
     bounds, as such a zero sized, or void type makes no sense.  */
  gdb_assert (index_type->code () != TYPE_CODE_VOID);
  gdb_assert (index_type->length () > 0);

  struct type *result_type = alloc.new_type ();
  result_type->set_code (TYPE_CODE_RANGE);
  result_type->set_target_type (index_type);
  if (index_type->is_stub ())
    result_type->set_target_is_stub (true);
  else
    result_type->set_length (check_typedef (index_type)->length ());

  range_bounds *bounds
    = (struct range_bounds *) TYPE_ZALLOC (result_type, sizeof (range_bounds));
  bounds->low = *low_bound;
  bounds->high = *high_bound;
  bounds->bias = bias;
  bounds->stride.set_const_val (0);

  result_type->set_bounds (bounds);

  if (index_type->code () == TYPE_CODE_FIXED_POINT)
    result_type->set_is_unsigned (index_type->is_unsigned ());
  else if (index_type->is_unsigned ())
    {
      /* If the underlying type is unsigned, then the range
	 necessarily is.  */
      result_type->set_is_unsigned (true);
    }
  /* Otherwise, the signed-ness of a range type can't simply be copied
     from the underlying type.  Consider a case where the underlying
     type is 'int', but the range type can hold 0..65535, and where
     the range is further specified to fit into 16 bits.  In this
     case, if we copy the underlying type's sign, then reading some
     range values will cause an unwanted sign extension.  So, we have
     some heuristics here instead.  */
  else if (low_bound->is_constant () && low_bound->const_val () >= 0)
    {
      result_type->set_is_unsigned (true);
      /* Ada allows the declaration of range types whose upper bound is
	 less than the lower bound, so checking the lower bound is not
	 enough.  Make sure we do not mark a range type whose upper bound
	 is negative as unsigned.  */
      if (high_bound->is_constant () && high_bound->const_val () < 0)
	result_type->set_is_unsigned (false);
    }

  result_type->set_endianity_is_not_default
    (index_type->endianity_is_not_default ());

  return result_type;
}

/* See gdbtypes.h.  */

struct type *
create_range_type_with_stride (type_allocator &alloc,
			       struct type *index_type,
			       const struct dynamic_prop *low_bound,
			       const struct dynamic_prop *high_bound,
			       LONGEST bias,
			       const struct dynamic_prop *stride,
			       bool byte_stride_p)
{
  struct type *result_type = create_range_type (alloc, index_type, low_bound,
						high_bound, bias);

  gdb_assert (stride != nullptr);
  result_type->bounds ()->stride = *stride;
  result_type->bounds ()->flag_is_byte_stride = byte_stride_p;

  return result_type;
}

/* See gdbtypes.h.  */

struct type *
create_static_range_type (type_allocator &alloc, struct type *index_type,
			  LONGEST low_bound, LONGEST high_bound)
{
  struct dynamic_prop low, high;

  low.set_const_val (low_bound);
  high.set_const_val (high_bound);

  struct type *result_type = create_range_type (alloc, index_type,
						&low, &high, 0);

  return result_type;
}

/* Predicate tests whether BOUNDS are static.  Returns 1 if all bounds values
   are static, otherwise returns 0.  */

static bool
has_static_range (const struct range_bounds *bounds)
{
  /* If the range doesn't have a defined stride then its stride field will
     be initialized to the constant 0.  */
  return (bounds->low.is_constant ()
	  && bounds->high.is_constant ()
	  && bounds->stride.is_constant ());
}

/* See gdbtypes.h.  */

std::optional<LONGEST>
get_discrete_low_bound (struct type *type)
{
  type = check_typedef (type);
  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      {
	/* This function only works for ranges with a constant low bound.  */
	if (!type->bounds ()->low.is_constant ())
	  return {};

	LONGEST low = type->bounds ()->low.const_val ();

	if (type->target_type ()->code () == TYPE_CODE_ENUM)
	  {
	    std::optional<LONGEST> low_pos
	      = discrete_position (type->target_type (), low);

	    if (low_pos.has_value ())
	      low = *low_pos;
	  }

	return low;
      }

    case TYPE_CODE_ENUM:
      {
	if (type->num_fields () > 0)
	  {
	    /* The enums may not be sorted by value, so search all
	       entries.  */
	    LONGEST low = type->field (0).loc_enumval ();

	    for (int i = 0; i < type->num_fields (); i++)
	      {
		if (type->field (i).loc_enumval () < low)
		  low = type->field (i).loc_enumval ();
	      }

	    return low;
	  }
	else
	  return 0;
      }

    case TYPE_CODE_BOOL:
      return 0;

    case TYPE_CODE_INT:
      if (type->length () > sizeof (LONGEST))	/* Too big */
	return {};

      if (!type->is_unsigned ())
	return -(1 << (type->length () * TARGET_CHAR_BIT - 1));

      [[fallthrough]];
    case TYPE_CODE_CHAR:
      return 0;

    default:
      return {};
    }
}

/* See gdbtypes.h.  */

std::optional<LONGEST>
get_discrete_high_bound (struct type *type)
{
  type = check_typedef (type);
  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      {
	/* This function only works for ranges with a constant high bound.  */
	if (!type->bounds ()->high.is_constant ())
	  return {};

	LONGEST high = type->bounds ()->high.const_val ();

	if (type->target_type ()->code () == TYPE_CODE_ENUM)
	  {
	    std::optional<LONGEST> high_pos
	      = discrete_position (type->target_type (), high);

	    if (high_pos.has_value ())
	      high = *high_pos;
	  }

	return high;
      }

    case TYPE_CODE_ENUM:
      {
	if (type->num_fields () > 0)
	  {
	    /* The enums may not be sorted by value, so search all
	       entries.  */
	    LONGEST high = type->field (0).loc_enumval ();

	    for (int i = 0; i < type->num_fields (); i++)
	      {
		if (type->field (i).loc_enumval () > high)
		  high = type->field (i).loc_enumval ();
	      }

	    return high;
	  }
	else
	  return -1;
      }

    case TYPE_CODE_BOOL:
      return 1;

    case TYPE_CODE_INT:
      if (type->length () > sizeof (LONGEST))	/* Too big */
	return {};

      if (!type->is_unsigned ())
	{
	  LONGEST low = -(1 << (type->length () * TARGET_CHAR_BIT - 1));
	  return -low - 1;
	}

      [[fallthrough]];
    case TYPE_CODE_CHAR:
      {
	/* This round-about calculation is to avoid shifting by
	   type->length () * TARGET_CHAR_BIT, which will not work
	   if type->length () == sizeof (LONGEST).  */
	LONGEST high = 1 << (type->length () * TARGET_CHAR_BIT - 1);
	return (high - 1) | high;
      }

    default:
      return {};
    }
}

/* See gdbtypes.h.  */

bool
get_discrete_bounds (struct type *type, LONGEST *lowp, LONGEST *highp)
{
  std::optional<LONGEST> low = get_discrete_low_bound (type);
  if (!low.has_value ())
    return false;

  std::optional<LONGEST> high = get_discrete_high_bound (type);
  if (!high.has_value ())
    return false;

  *lowp = *low;
  *highp = *high;

  return true;
}

/* See gdbtypes.h  */

bool
get_array_bounds (struct type *type, LONGEST *low_bound, LONGEST *high_bound)
{
  struct type *index = type->index_type ();
  LONGEST low = 0;
  LONGEST high = 0;

  if (index == NULL)
    return false;

  if (!get_discrete_bounds (index, &low, &high))
    return false;

  if (low_bound)
    *low_bound = low;

  if (high_bound)
    *high_bound = high;

  return true;
}

/* Assuming that TYPE is a discrete type and VAL is a valid integer
   representation of a value of this type, save the corresponding
   position number in POS.

   Its differs from VAL only in the case of enumeration types.  In
   this case, the position number of the value of the first listed
   enumeration literal is zero; the position number of the value of
   each subsequent enumeration literal is one more than that of its
   predecessor in the list.

   Return 1 if the operation was successful.  Return zero otherwise,
   in which case the value of POS is unmodified.
*/

std::optional<LONGEST>
discrete_position (struct type *type, LONGEST val)
{
  if (type->code () == TYPE_CODE_RANGE)
    type = type->target_type ();

  if (type->code () == TYPE_CODE_ENUM)
    {
      int i;

      for (i = 0; i < type->num_fields (); i += 1)
	{
	  if (val == type->field (i).loc_enumval ())
	    return i;
	}

      /* Invalid enumeration value.  */
      return {};
    }
  else
    return val;
}

/* If the array TYPE has static bounds calculate and update its
   size, then return true.  Otherwise return false and leave TYPE
   unchanged.  */

static bool
update_static_array_size (struct type *type)
{
  gdb_assert (type->code () == TYPE_CODE_ARRAY);

  struct type *range_type = type->index_type ();

  if (type->dyn_prop (DYN_PROP_BYTE_STRIDE) == nullptr
      && has_static_range (range_type->bounds ())
      && (!type_not_associated (type)
	  && !type_not_allocated (type)))
    {
      LONGEST low_bound, high_bound;
      int stride;
      struct type *element_type;

      stride = type->bit_stride ();

      if (!get_discrete_bounds (range_type, &low_bound, &high_bound))
	low_bound = high_bound = 0;

      element_type = check_typedef (type->target_type ());
      /* Be careful when setting the array length.  Ada arrays can be
	 empty arrays with the high_bound being smaller than the low_bound.
	 In such cases, the array length should be zero.  */
      if (high_bound < low_bound)
	type->set_length (0);
      else if (stride != 0)
	{
	  /* Ensure that the type length is always positive, even in the
	     case where (for example in Fortran) we have a negative
	     stride.  It is possible to have a single element array with a
	     negative stride in Fortran (this doesn't mean anything
	     special, it's still just a single element array) so do
	     consider that case when touching this code.  */
	  LONGEST element_count = std::abs (high_bound - low_bound + 1);
	  type->set_length (((std::abs (stride) * element_count) + 7) / 8);
	}
      else
	type->set_length (element_type->length ()
			  * (high_bound - low_bound + 1));

      /* If this array's element is itself an array with a bit stride,
	 then we want to update this array's bit stride to reflect the
	 size of the sub-array.  Otherwise, we'll end up using the
	 wrong size when trying to find elements of the outer
	 array.  */
      if (element_type->code () == TYPE_CODE_ARRAY
	  && (stride != 0 || element_type->is_multi_dimensional ())
	  && element_type->length () != 0
	  && element_type->field (0).bitsize () != 0
	  && get_array_bounds (element_type, &low_bound, &high_bound)
	  && high_bound >= low_bound)
	type->field (0).set_bitsize
	  ((high_bound - low_bound + 1)
	   * element_type->field (0).bitsize ());

      return true;
    }

  return false;
}

/* See gdbtypes.h.  */

struct type *
create_array_type_with_stride (type_allocator &alloc,
			       struct type *element_type,
			       struct type *range_type,
			       struct dynamic_prop *byte_stride_prop,
			       unsigned int bit_stride)
{
  if (byte_stride_prop != nullptr && byte_stride_prop->is_constant ())
    {
      /* The byte stride is actually not dynamic.  Pretend we were
	 called with bit_stride set instead of byte_stride_prop.
	 This will give us the same result type, while avoiding
	 the need to handle this as a special case.  */
      bit_stride = byte_stride_prop->const_val () * 8;
      byte_stride_prop = NULL;
    }

  struct type *result_type = alloc.new_type ();

  result_type->set_code (TYPE_CODE_ARRAY);
  result_type->set_target_type (element_type);

  result_type->alloc_fields (1);
  result_type->set_index_type (range_type);
  if (byte_stride_prop != NULL)
    result_type->add_dyn_prop (DYN_PROP_BYTE_STRIDE, *byte_stride_prop);
  else if (bit_stride > 0)
    result_type->field (0).set_bitsize (bit_stride);

  if (!update_static_array_size (result_type))
    {
      /* This type is dynamic and its length needs to be computed
	 on demand.  In the meantime, avoid leaving the TYPE_LENGTH
	 undefined by setting it to zero.  Although we are not expected
	 to trust TYPE_LENGTH in this case, setting the size to zero
	 allows us to avoid allocating objects of random sizes in case
	 we accidently do.  */
      result_type->set_length (0);
    }

  /* TYPE_TARGET_STUB will take care of zero length arrays.  */
  if (result_type->length () == 0)
    result_type->set_target_is_stub (true);

  return result_type;
}

/* See gdbtypes.h.  */

struct type *
create_array_type (type_allocator &alloc,
		   struct type *element_type,
		   struct type *range_type)
{
  return create_array_type_with_stride (alloc, element_type,
					range_type, NULL, 0);
}

struct type *
lookup_array_range_type (struct type *element_type,
			 LONGEST low_bound, LONGEST high_bound)
{
  struct type *index_type;
  struct type *range_type;

  type_allocator alloc (element_type);
  index_type = builtin_type (element_type->arch ())->builtin_int;

  range_type = create_static_range_type (alloc, index_type,
					 low_bound, high_bound);

  return create_array_type (alloc, element_type, range_type);
}

/* See gdbtypes.h.  */

struct type *
create_string_type (type_allocator &alloc,
		    struct type *string_char_type,
		    struct type *range_type)
{
  struct type *result_type = create_array_type (alloc,
						string_char_type,
						range_type);
  result_type->set_code (TYPE_CODE_STRING);
  return result_type;
}

struct type *
lookup_string_range_type (struct type *string_char_type,
			  LONGEST low_bound, LONGEST high_bound)
{
  struct type *result_type;

  result_type = lookup_array_range_type (string_char_type,
					 low_bound, high_bound);
  result_type->set_code (TYPE_CODE_STRING);
  return result_type;
}

struct type *
create_set_type (type_allocator &alloc, struct type *domain_type)
{
  struct type *result_type = alloc.new_type ();

  result_type->set_code (TYPE_CODE_SET);
  result_type->alloc_fields (1);

  if (!domain_type->is_stub ())
    {
      LONGEST low_bound, high_bound, bit_length;

      if (!get_discrete_bounds (domain_type, &low_bound, &high_bound))
	low_bound = high_bound = 0;

      bit_length = high_bound - low_bound + 1;
      result_type->set_length ((bit_length + TARGET_CHAR_BIT - 1)
			       / TARGET_CHAR_BIT);
      if (low_bound >= 0)
	result_type->set_is_unsigned (true);
    }
  result_type->field (0).set_type (domain_type);

  return result_type;
}

/* Convert ARRAY_TYPE to a vector type.  This may modify ARRAY_TYPE
   and any array types nested inside it.  */

void
make_vector_type (struct type *array_type)
{
  struct type *inner_array, *elt_type;

  /* Find the innermost array type, in case the array is
     multi-dimensional.  */
  inner_array = array_type;
  while (inner_array->target_type ()->code () == TYPE_CODE_ARRAY)
    inner_array = inner_array->target_type ();

  elt_type = inner_array->target_type ();
  if (elt_type->code () == TYPE_CODE_INT)
    {
      type_instance_flags flags
	= elt_type->instance_flags () | TYPE_INSTANCE_FLAG_NOTTEXT;
      elt_type = make_qualified_type (elt_type, flags, NULL);
      inner_array->set_target_type (elt_type);
    }

  array_type->set_is_vector (true);
}

struct type *
init_vector_type (struct type *elt_type, int n)
{
  struct type *array_type;

  array_type = lookup_array_range_type (elt_type, 0, n - 1);
  make_vector_type (array_type);
  return array_type;
}

/* Internal routine called by TYPE_SELF_TYPE to return the type that TYPE
   belongs to.  In c++ this is the class of "this", but TYPE_THIS_TYPE is too
   confusing.  "self" is a common enough replacement for "this".
   TYPE must be one of TYPE_CODE_METHODPTR, TYPE_CODE_MEMBERPTR, or
   TYPE_CODE_METHOD.  */

struct type *
internal_type_self_type (struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	return NULL;
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_SELF_TYPE);
      return TYPE_MAIN_TYPE (type)->type_specific.self_type;
    case TYPE_CODE_METHOD:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	return NULL;
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FUNC);
      return TYPE_MAIN_TYPE (type)->type_specific.func_stuff->self_type;
    default:
      gdb_assert_not_reached ("bad type");
    }
}

/* Set the type of the class that TYPE belongs to.
   In c++ this is the class of "this".
   TYPE must be one of TYPE_CODE_METHODPTR, TYPE_CODE_MEMBERPTR, or
   TYPE_CODE_METHOD.  */

void
set_type_self_type (struct type *type, struct type *self_type)
{
  switch (type->code ())
    {
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_SELF_TYPE;
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_SELF_TYPE);
      TYPE_MAIN_TYPE (type)->type_specific.self_type = self_type;
      break;
    case TYPE_CODE_METHOD:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	INIT_FUNC_SPECIFIC (type);
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FUNC);
      TYPE_MAIN_TYPE (type)->type_specific.func_stuff->self_type = self_type;
      break;
    default:
      gdb_assert_not_reached ("bad type");
    }
}

/* Smash TYPE to be a type of pointers to members of SELF_TYPE with type
   TO_TYPE.  A member pointer is a wierd thing -- it amounts to a
   typed offset into a struct, e.g. "an int at offset 8".  A MEMBER
   TYPE doesn't include the offset (that's the value of the MEMBER
   itself), but does include the structure type into which it points
   (for some reason).

   When "smashing" the type, we preserve the objfile that the old type
   pointed to, since we aren't changing where the type is actually
   allocated.  */

void
smash_to_memberptr_type (struct type *type, struct type *self_type,
			 struct type *to_type)
{
  smash_type (type);
  type->set_code (TYPE_CODE_MEMBERPTR);
  type->set_target_type (to_type);
  set_type_self_type (type, self_type);
  /* Assume that a data member pointer is the same size as a normal
     pointer.  */
  type->set_length (gdbarch_ptr_bit (to_type->arch ()) / TARGET_CHAR_BIT);
}

/* Smash TYPE to be a type of pointer to methods type TO_TYPE.

   When "smashing" the type, we preserve the objfile that the old type
   pointed to, since we aren't changing where the type is actually
   allocated.  */

void
smash_to_methodptr_type (struct type *type, struct type *to_type)
{
  smash_type (type);
  type->set_code (TYPE_CODE_METHODPTR);
  type->set_target_type (to_type);
  set_type_self_type (type, TYPE_SELF_TYPE (to_type));
  type->set_length (cplus_method_ptr_size (to_type));
}

/* Smash TYPE to be a type of method of SELF_TYPE with type TO_TYPE.
   METHOD just means `function that gets an extra "this" argument'.

   When "smashing" the type, we preserve the objfile that the old type
   pointed to, since we aren't changing where the type is actually
   allocated.  */

void
smash_to_method_type (struct type *type, struct type *self_type,
		      struct type *to_type, struct field *args,
		      int nargs, int varargs)
{
  smash_type (type);
  type->set_code (TYPE_CODE_METHOD);
  type->set_target_type (to_type);
  set_type_self_type (type, self_type);
  type->set_fields (args);
  type->set_num_fields (nargs);

  if (varargs)
    type->set_has_varargs (true);

  /* In practice, this is never needed.  */
  type->set_length (1);
}

/* A wrapper of TYPE_NAME which calls error if the type is anonymous.
   Since GCC PR debug/47510 DWARF provides associated information to detect the
   anonymous class linkage name from its typedef.

   Parameter TYPE should not yet have CHECK_TYPEDEF applied, this function will
   apply it itself.  */

const char *
type_name_or_error (struct type *type)
{
  struct type *saved_type = type;
  const char *name;
  struct objfile *objfile;

  type = check_typedef (type);

  name = type->name ();
  if (name != NULL)
    return name;

  name = saved_type->name ();
  objfile = saved_type->objfile_owner ();
  error (_("Invalid anonymous type %s [in module %s], GCC PR debug/47510 bug?"),
	 name ? name : "<anonymous>",
	 objfile ? objfile_name (objfile) : "<arch>");
}

/* See gdbtypes.h.  */

struct type *
lookup_typename (const struct language_defn *language,
		 const char *name,
		 const struct block *block, int noerr)
{
  struct symbol *sym;

  sym = lookup_symbol_in_language (name, block, VAR_DOMAIN,
				   language->la_language, NULL).symbol;
  if (sym != NULL && sym->aclass () == LOC_TYPEDEF)
    {
      struct type *type = sym->type ();
      /* Ensure the length of TYPE is valid.  */
      check_typedef (type);
      return type;
    }

  if (noerr)
    return NULL;
  error (_("No type named %s."), name);
}

struct type *
lookup_unsigned_typename (const struct language_defn *language,
			  const char *name)
{
  std::string uns;
  uns.reserve (strlen (name) + strlen ("unsigned "));
  uns = "unsigned ";
  uns += name;

  return lookup_typename (language, uns.c_str (), NULL, 0);
}

struct type *
lookup_signed_typename (const struct language_defn *language, const char *name)
{
  /* In C and C++, "char" and "signed char" are distinct types.  */
  if (streq (name, "char"))
    name = "signed char";
  return lookup_typename (language, name, NULL, 0);
}

/* Lookup a structure type named "struct NAME",
   visible in lexical block BLOCK.  */

struct type *
lookup_struct (const char *name, const struct block *block)
{
  struct symbol *sym;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, 0).symbol;

  if (sym == NULL)
    {
      error (_("No struct type named %s."), name);
    }
  if (sym->type ()->code () != TYPE_CODE_STRUCT)
    {
      error (_("This context has class, union or enum %s, not a struct."),
	     name);
    }
  return (sym->type ());
}

/* Lookup a union type named "union NAME",
   visible in lexical block BLOCK.  */

struct type *
lookup_union (const char *name, const struct block *block)
{
  struct symbol *sym;
  struct type *t;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, 0).symbol;

  if (sym == NULL)
    error (_("No union type named %s."), name);

  t = sym->type ();

  if (t->code () == TYPE_CODE_UNION)
    return t;

  /* If we get here, it's not a union.  */
  error (_("This context has class, struct or enum %s, not a union."), 
	 name);
}

/* Lookup an enum type named "enum NAME",
   visible in lexical block BLOCK.  */

struct type *
lookup_enum (const char *name, const struct block *block)
{
  struct symbol *sym;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, 0).symbol;
  if (sym == NULL)
    {
      error (_("No enum type named %s."), name);
    }
  if (sym->type ()->code () != TYPE_CODE_ENUM)
    {
      error (_("This context has class, struct or union %s, not an enum."), 
	     name);
    }
  return (sym->type ());
}

/* Lookup a template type named "template NAME<TYPE>",
   visible in lexical block BLOCK.  */

struct type *
lookup_template_type (const char *name, struct type *type, 
		      const struct block *block)
{
  std::string nam;
  nam.reserve (strlen (name) + strlen (type->name ()) + strlen ("< >"));
  nam = name;
  nam += "<";
  nam += type->name ();
  nam += " >"; /* FIXME, extra space still introduced in gcc?  */

  symbol *sym = lookup_symbol (nam.c_str (), block, VAR_DOMAIN, 0).symbol;

  if (sym == NULL)
    {
      error (_("No template type named %s."), name);
    }
  if (sym->type ()->code () != TYPE_CODE_STRUCT)
    {
      error (_("This context has class, union or enum %s, not a struct."),
	     name);
    }
  return (sym->type ());
}

/* See gdbtypes.h.  */

struct_elt
lookup_struct_elt (struct type *type, const char *name, int noerr)
{
  int i;

  for (;;)
    {
      type = check_typedef (type);
      if (type->code () != TYPE_CODE_PTR
	  && type->code () != TYPE_CODE_REF)
	break;
      type = type->target_type ();
    }

  if (type->code () != TYPE_CODE_STRUCT
      && type->code () != TYPE_CODE_UNION)
    {
      std::string type_name = type_to_string (type);
      error (_("Type %s is not a structure or union type."),
	     type_name.c_str ());
    }

  for (i = type->num_fields () - 1; i >= TYPE_N_BASECLASSES (type); i--)
    {
      const char *t_field_name = type->field (i).name ();

      if (t_field_name && (strcmp_iw (t_field_name, name) == 0))
	{
	  return {&type->field (i), type->field (i).loc_bitpos ()};
	}
      else if (!t_field_name || *t_field_name == '\0')
	{
	  struct_elt elt
	    = lookup_struct_elt (type->field (i).type (), name, 1);
	  if (elt.field != NULL)
	    {
	      elt.offset += type->field (i).loc_bitpos ();
	      return elt;
	    }
	}
    }

  /* OK, it's not in this class.  Recursively check the baseclasses.  */
  for (i = TYPE_N_BASECLASSES (type) - 1; i >= 0; i--)
    {
      struct_elt elt = lookup_struct_elt (TYPE_BASECLASS (type, i), name, 1);
      if (elt.field != NULL)
	return elt;
    }

  if (noerr)
    return {nullptr, 0};

  std::string type_name = type_to_string (type);
  error (_("Type %s has no component named %s."), type_name.c_str (), name);
}

/* See gdbtypes.h.  */

struct type *
lookup_struct_elt_type (struct type *type, const char *name, int noerr)
{
  struct_elt elt = lookup_struct_elt (type, name, noerr);
  if (elt.field != NULL)
    return elt.field->type ();
  else
    return NULL;
}

/* Return the largest number representable by unsigned integer type TYPE.  */

ULONGEST
get_unsigned_type_max (struct type *type)
{
  unsigned int n;

  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_INT && type->is_unsigned ());
  gdb_assert (type->length () <= sizeof (ULONGEST));

  /* Written this way to avoid overflow.  */
  n = type->length () * TARGET_CHAR_BIT;
  return ((((ULONGEST) 1 << (n - 1)) - 1) << 1) | 1;
}

/* Store in *MIN, *MAX the smallest and largest numbers representable by
   signed integer type TYPE.  */

void
get_signed_type_minmax (struct type *type, LONGEST *min, LONGEST *max)
{
  unsigned int n;

  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_INT && !type->is_unsigned ());
  gdb_assert (type->length () <= sizeof (LONGEST));

  n = type->length () * TARGET_CHAR_BIT;
  *min = -((ULONGEST) 1 << (n - 1));
  *max = ((ULONGEST) 1 << (n - 1)) - 1;
}

/* Return the largest value representable by pointer type TYPE. */

CORE_ADDR
get_pointer_type_max (struct type *type)
{
  unsigned int n;

  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_PTR);
  gdb_assert (type->length () <= sizeof (CORE_ADDR));

  n = type->length () * TARGET_CHAR_BIT;
  return ((((CORE_ADDR) 1 << (n - 1)) - 1) << 1) | 1;
}

/* Internal routine called by TYPE_VPTR_FIELDNO to return the value of
   cplus_stuff.vptr_fieldno.

   cplus_stuff is initialized to cplus_struct_default which does not
   set vptr_fieldno to -1 for portability reasons (IWBN to use C99
   designated initializers).  We cope with that here.  */

int
internal_type_vptr_fieldno (struct type *type)
{
  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_STRUCT
	      || type->code () == TYPE_CODE_UNION);
  if (!HAVE_CPLUS_STRUCT (type))
    return -1;
  return TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_fieldno;
}

/* Set the value of cplus_stuff.vptr_fieldno.  */

void
set_type_vptr_fieldno (struct type *type, int fieldno)
{
  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_STRUCT
	      || type->code () == TYPE_CODE_UNION);
  if (!HAVE_CPLUS_STRUCT (type))
    ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_fieldno = fieldno;
}

/* Internal routine called by TYPE_VPTR_BASETYPE to return the value of
   cplus_stuff.vptr_basetype.  */

struct type *
internal_type_vptr_basetype (struct type *type)
{
  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_STRUCT
	      || type->code () == TYPE_CODE_UNION);
  gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_CPLUS_STUFF);
  return TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_basetype;
}

/* Set the value of cplus_stuff.vptr_basetype.  */

void
set_type_vptr_basetype (struct type *type, struct type *basetype)
{
  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_STRUCT
	      || type->code () == TYPE_CODE_UNION);
  if (!HAVE_CPLUS_STRUCT (type))
    ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_basetype = basetype;
}

/* Lookup the vptr basetype/fieldno values for TYPE.
   If found store vptr_basetype in *BASETYPEP if non-NULL, and return
   vptr_fieldno.  Also, if found and basetype is from the same objfile,
   cache the results.
   If not found, return -1 and ignore BASETYPEP.
   Callers should be aware that in some cases (for example,
   the type or one of its baseclasses is a stub type and we are
   debugging a .o file, or the compiler uses DWARF-2 and is not GCC),
   this function will not be able to find the
   virtual function table pointer, and vptr_fieldno will remain -1 and
   vptr_basetype will remain NULL or incomplete.  */

int
get_vptr_fieldno (struct type *type, struct type **basetypep)
{
  type = check_typedef (type);

  if (TYPE_VPTR_FIELDNO (type) < 0)
    {
      int i;

      /* We must start at zero in case the first (and only) baseclass
	 is virtual (and hence we cannot share the table pointer).  */
      for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
	{
	  struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
	  int fieldno;
	  struct type *basetype;

	  fieldno = get_vptr_fieldno (baseclass, &basetype);
	  if (fieldno >= 0)
	    {
	      /* If the type comes from a different objfile we can't cache
		 it, it may have a different lifetime.  PR 2384 */
	      if (type->objfile_owner () == basetype->objfile_owner ())
		{
		  set_type_vptr_fieldno (type, fieldno);
		  set_type_vptr_basetype (type, basetype);
		}
	      if (basetypep)
		*basetypep = basetype;
	      return fieldno;
	    }
	}

      /* Not found.  */
      return -1;
    }
  else
    {
      if (basetypep)
	*basetypep = TYPE_VPTR_BASETYPE (type);
      return TYPE_VPTR_FIELDNO (type);
    }
}

static void
stub_noname_complaint (void)
{
  complaint (_("stub type has NULL name"));
}

/* Return nonzero if TYPE has a DYN_PROP_BYTE_STRIDE dynamic property
   attached to it, and that property has a non-constant value.  */

static int
array_type_has_dynamic_stride (struct type *type)
{
  struct dynamic_prop *prop = type->dyn_prop (DYN_PROP_BYTE_STRIDE);

  return prop != nullptr && prop->is_constant ();
}

/* Worker for is_dynamic_type.  */

static int
is_dynamic_type_internal (struct type *type, int top_level)
{
  type = check_typedef (type);

  /* We only want to recognize references at the outermost level.  */
  if (top_level && type->code () == TYPE_CODE_REF)
    type = check_typedef (type->target_type ());

  /* Types that have a dynamic TYPE_DATA_LOCATION are considered
     dynamic, even if the type itself is statically defined.
     From a user's point of view, this may appear counter-intuitive;
     but it makes sense in this context, because the point is to determine
     whether any part of the type needs to be resolved before it can
     be exploited.  */
  if (TYPE_DATA_LOCATION (type) != NULL
      && (TYPE_DATA_LOCATION_KIND (type) == PROP_LOCEXPR
	  || TYPE_DATA_LOCATION_KIND (type) == PROP_LOCLIST))
    return 1;

  if (TYPE_ASSOCIATED_PROP (type))
    return 1;

  if (TYPE_ALLOCATED_PROP (type))
    return 1;

  struct dynamic_prop *prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
  if (prop != nullptr && prop->kind () != PROP_TYPE)
    return 1;

  if (TYPE_HAS_DYNAMIC_LENGTH (type))
    return 1;

  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      {
	/* A range type is obviously dynamic if it has at least one
	   dynamic bound.  But also consider the range type to be
	   dynamic when its subtype is dynamic, even if the bounds
	   of the range type are static.  It allows us to assume that
	   the subtype of a static range type is also static.  */
	return (!has_static_range (type->bounds ())
		|| is_dynamic_type_internal (type->target_type (), 0));
      }

    case TYPE_CODE_STRING:
      /* Strings are very much like an array of characters, and can be
	 treated as one here.  */
    case TYPE_CODE_ARRAY:
      {
	gdb_assert (type->num_fields () == 1);

	/* The array is dynamic if either the bounds are dynamic...  */
	if (is_dynamic_type_internal (type->index_type (), 0))
	  return 1;
	/* ... or the elements it contains have a dynamic contents...  */
	if (is_dynamic_type_internal (type->target_type (), 0))
	  return 1;
	/* ... or if it has a dynamic stride...  */
	if (array_type_has_dynamic_stride (type))
	  return 1;
	return 0;
      }

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	int i;

	bool is_cplus = HAVE_CPLUS_STRUCT (type);

	for (i = 0; i < type->num_fields (); ++i)
	  {
	    /* Static fields can be ignored here.  */
	    if (type->field (i).is_static ())
	      continue;
	    /* If the field has dynamic type, then so does TYPE.  */
	    if (is_dynamic_type_internal (type->field (i).type (), 0))
	      return 1;
	    /* If the field is at a fixed offset, then it is not
	       dynamic.  */
	    if (type->field (i).loc_kind () != FIELD_LOC_KIND_DWARF_BLOCK)
	      continue;
	    /* Do not consider C++ virtual base types to be dynamic
	       due to the field's offset being dynamic; these are
	       handled via other means.  */
	    if (is_cplus && BASETYPE_VIA_VIRTUAL (type, i))
	      continue;
	    return 1;
	  }
      }
      break;
    }

  return 0;
}

/* See gdbtypes.h.  */

int
is_dynamic_type (struct type *type)
{
  return is_dynamic_type_internal (type, 1);
}

static struct type *resolve_dynamic_type_internal
  (struct type *type, struct property_addr_info *addr_stack,
   const frame_info_ptr &frame, int top_level);

/* Given a dynamic range type (dyn_range_type) and a stack of
   struct property_addr_info elements, return a static version
   of that type.

   When RESOLVE_P is true then the returned static range is created by
   actually evaluating any dynamic properties within the range type, while
   when RESOLVE_P is false the returned static range has all of the bounds
   and stride information set to undefined.  The RESOLVE_P set to false
   case will be used when evaluating a dynamic array that is not
   allocated, or not associated, i.e. the bounds information might not be
   initialized yet.

   RANK is the array rank for which we are resolving this range, and is a
   zero based count.  The rank should never be negative.
*/

static struct type *
resolve_dynamic_range (struct type *dyn_range_type,
		       struct property_addr_info *addr_stack,
		       const frame_info_ptr &frame,
		       int rank, bool resolve_p = true)
{
  CORE_ADDR value;
  struct type *static_range_type, *static_target_type;
  struct dynamic_prop low_bound, high_bound, stride;

  gdb_assert (dyn_range_type->code () == TYPE_CODE_RANGE);
  gdb_assert (rank >= 0);

  const struct dynamic_prop *prop = &dyn_range_type->bounds ()->low;
  if (resolve_p && dwarf2_evaluate_property (prop, frame, addr_stack, &value,
					     { (CORE_ADDR) rank }))
    low_bound.set_const_val (value);
  else
    low_bound.set_undefined ();

  prop = &dyn_range_type->bounds ()->high;
  if (resolve_p && dwarf2_evaluate_property (prop, frame, addr_stack, &value,
					     { (CORE_ADDR) rank }))
    {
      high_bound.set_const_val (value);

      if (dyn_range_type->bounds ()->flag_upper_bound_is_count)
	high_bound.set_const_val
	  (low_bound.const_val () + high_bound.const_val () - 1);
    }
  else
    high_bound.set_undefined ();

  bool byte_stride_p = dyn_range_type->bounds ()->flag_is_byte_stride;
  prop = &dyn_range_type->bounds ()->stride;
  if (resolve_p && dwarf2_evaluate_property (prop, frame, addr_stack, &value,
					     { (CORE_ADDR) rank }))
    {
      stride.set_const_val (value);

      /* If we have a bit stride that is not an exact number of bytes then
	 I really don't think this is going to work with current GDB, the
	 array indexing code in GDB seems to be pretty heavily tied to byte
	 offsets right now.  Assuming 8 bits in a byte.  */
      struct gdbarch *gdbarch = dyn_range_type->arch ();
      int unit_size = gdbarch_addressable_memory_unit_size (gdbarch);
      if (!byte_stride_p && (value % (unit_size * 8)) != 0)
	error (_("bit strides that are not a multiple of the byte size "
		 "are currently not supported"));
    }
  else
    {
      stride.set_undefined ();
      byte_stride_p = true;
    }

  static_target_type
    = resolve_dynamic_type_internal (dyn_range_type->target_type (),
				     addr_stack, frame, 0);
  LONGEST bias = dyn_range_type->bounds ()->bias;
  type_allocator alloc (dyn_range_type);
  static_range_type = create_range_type_with_stride
    (alloc, static_target_type,
     &low_bound, &high_bound, bias, &stride, byte_stride_p);
  static_range_type->set_name (dyn_range_type->name ());
  static_range_type->bounds ()->flag_bound_evaluated = 1;
  return static_range_type;
}

/* Helper function for resolve_dynamic_array_or_string.  This function
   resolves the properties for a single array at RANK within a nested array
   of arrays structure.  The RANK value is greater than or equal to 0, and
   starts at it's maximum value and goes down by 1 for each recursive call
   to this function.  So, for a 3-dimensional array, the first call to this
   function has RANK == 2, then we call ourselves recursively with RANK ==
   1, than again with RANK == 0, and at that point we should return.

   TYPE is updated as the dynamic properties are resolved, and so, should
   be a copy of the dynamic type, rather than the original dynamic type
   itself.

   ADDR_STACK is a stack of struct property_addr_info to be used if needed
   during the dynamic resolution.

   When RESOLVE_P is true then the dynamic properties of TYPE are
   evaluated, otherwise the dynamic properties of TYPE are not evaluated,
   instead we assume the array is not allocated/associated yet.  */

static struct type *
resolve_dynamic_array_or_string_1 (struct type *type,
				   struct property_addr_info *addr_stack,
				   const frame_info_ptr &frame,
				   int rank, bool resolve_p)
{
  CORE_ADDR value;
  struct type *elt_type;
  struct type *range_type;
  struct type *ary_dim;
  struct dynamic_prop *prop;
  unsigned int bit_stride = 0;

  /* For dynamic type resolution strings can be treated like arrays of
     characters.  */
  gdb_assert (type->code () == TYPE_CODE_ARRAY
	      || type->code () == TYPE_CODE_STRING);

  /* As the rank is a zero based count we expect this to never be
     negative.  */
  gdb_assert (rank >= 0);

  /* Resolve the allocated and associated properties before doing anything
     else.  If an array is not allocated or not associated then (at least
     for Fortran) there is no guarantee that the data to define the upper
     bound, lower bound, or stride will be correct.  If RESOLVE_P is
     already false at this point then this is not the first dimension of
     the array and a more outer dimension has already marked this array as
     not allocated/associated, as such we just ignore this property.  This
     is fine as GDB only checks the allocated/associated on the outer most
     dimension of the array.  */
  prop = TYPE_ALLOCATED_PROP (type);
  if (prop != NULL && resolve_p
      && dwarf2_evaluate_property (prop, frame, addr_stack, &value))
    {
      prop->set_const_val (value);
      if (value == 0)
	resolve_p = false;
    }

  prop = TYPE_ASSOCIATED_PROP (type);
  if (prop != NULL && resolve_p
      && dwarf2_evaluate_property (prop, frame, addr_stack, &value))
    {
      prop->set_const_val (value);
      if (value == 0)
	resolve_p = false;
    }

  range_type = check_typedef (type->index_type ());
  range_type
    = resolve_dynamic_range (range_type, addr_stack, frame, rank, resolve_p);

  ary_dim = check_typedef (type->target_type ());
  if (ary_dim != NULL && ary_dim->code () == TYPE_CODE_ARRAY)
    {
      ary_dim = copy_type (ary_dim);
      elt_type = resolve_dynamic_array_or_string_1 (ary_dim, addr_stack,
						    frame, rank - 1,
						    resolve_p);
    }
  else
    elt_type = type->target_type ();

  prop = type->dyn_prop (DYN_PROP_BYTE_STRIDE);
  if (prop != NULL && resolve_p)
    {
      if (dwarf2_evaluate_property (prop, frame, addr_stack, &value))
	{
	  type->remove_dyn_prop (DYN_PROP_BYTE_STRIDE);
	  bit_stride = (unsigned int) (value * 8);
	}
      else
	{
	  /* Could be a bug in our code, but it could also happen
	     if the DWARF info is not correct.  Issue a warning,
	     and assume no byte/bit stride (leave bit_stride = 0).  */
	  warning (_("cannot determine array stride for type %s"),
		   type->name () ? type->name () : "<no name>");
	}
    }
  else
    bit_stride = type->field (0).bitsize ();

  type_allocator alloc (type, type_allocator::SMASH);
  return create_array_type_with_stride (alloc, elt_type, range_type, NULL,
					bit_stride);
}

/* Resolve an array or string type with dynamic properties, return a new
   type with the dynamic properties resolved to actual values.  The
   ADDR_STACK represents the location of the object being resolved.  */

static struct type *
resolve_dynamic_array_or_string (struct type *type,
				 struct property_addr_info *addr_stack,
				 const frame_info_ptr &frame)
{
  CORE_ADDR value;
  int rank = 0;

  /* For dynamic type resolution strings can be treated like arrays of
     characters.  */
  gdb_assert (type->code () == TYPE_CODE_ARRAY
	      || type->code () == TYPE_CODE_STRING);

  type = copy_type (type);

  /* Resolve the rank property to get rank value.  */
  struct dynamic_prop *prop = TYPE_RANK_PROP (type);
  if (dwarf2_evaluate_property (prop, frame, addr_stack, &value))
    {
      prop->set_const_val (value);
      rank = value;

      if (rank == 0)
	{
	  /* Rank is zero, if a variable is passed as an argument to a
	     function.  In this case the resolved type should not be an
	     array, but should instead be that of an array element.  */
	  struct type *dynamic_array_type = type;
	  type = copy_type (dynamic_array_type->target_type ());
	  struct dynamic_prop_list *prop_list
	    = TYPE_MAIN_TYPE (dynamic_array_type)->dyn_prop_list;
	  if (prop_list != nullptr)
	    {
	      struct obstack *obstack
		= &type->objfile_owner ()->objfile_obstack;
	      TYPE_MAIN_TYPE (type)->dyn_prop_list
		= copy_dynamic_prop_list (obstack, prop_list);
	    }
	  return type;
	}
      else if (type->code () == TYPE_CODE_STRING && rank != 1)
	{
	  /* What would this even mean?  A string with a dynamic rank
	     greater than 1.  */
	  error (_("unable to handle string with dynamic rank greater than 1"));
	}
      else if (rank > 1)
	{
	  /* Arrays with dynamic rank are initially just an array type
	     with a target type that is the array element.

	     However, now we know the rank of the array we need to build
	     the array of arrays structure that GDB expects, that is we
	     need an array type that has a target which is an array type,
	     and so on, until eventually, we have the element type at the
	     end of the chain.  Create all the additional array types here
	     by copying the top level array type.  */
	  struct type *element_type = type->target_type ();
	  struct type *rank_type = type;
	  for (int i = 1; i < rank; i++)
	    {
	      rank_type->set_target_type (copy_type (rank_type));
	      rank_type = rank_type->target_type ();
	    }
	  rank_type->set_target_type (element_type);
	}
    }
  else
    {
      rank = 1;

      for (struct type *tmp_type = check_typedef (type->target_type ());
	   tmp_type->code () == TYPE_CODE_ARRAY;
	   tmp_type = check_typedef (tmp_type->target_type ()))
	++rank;
    }

  /* The rank that we calculated above is actually a count of the number of
     ranks.  However, when we resolve the type of each individual array
     rank we should actually use a rank "offset", e.g. an array with a rank
     count of 1 (calculated above) will use the rank offset 0 in order to
     resolve the details of the first array dimension.  As a result, we
     reduce the rank by 1 here.  */
  --rank;

  return resolve_dynamic_array_or_string_1 (type, addr_stack, frame, rank,
					    true);
}

/* Resolve dynamic bounds of members of the union TYPE to static
   bounds.  ADDR_STACK is a stack of struct property_addr_info
   to be used if needed during the dynamic resolution.  */

static struct type *
resolve_dynamic_union (struct type *type,
		       struct property_addr_info *addr_stack,
		       const frame_info_ptr &frame)
{
  struct type *resolved_type;
  int i;
  unsigned int max_len = 0;

  gdb_assert (type->code () == TYPE_CODE_UNION);

  resolved_type = copy_type (type);
  resolved_type->copy_fields (type);
  for (i = 0; i < resolved_type->num_fields (); ++i)
    {
      struct type *t;

      if (type->field (i).is_static ())
	continue;

      t = resolve_dynamic_type_internal (resolved_type->field (i).type (),
					 addr_stack, frame, 0);
      resolved_type->field (i).set_type (t);

      struct type *real_type = check_typedef (t);
      if (real_type->length () > max_len)
	max_len = real_type->length ();
    }

  resolved_type->set_length (max_len);
  return resolved_type;
}

/* See gdbtypes.h.  */

bool
variant::matches (ULONGEST value, bool is_unsigned) const
{
  for (const discriminant_range &range : discriminants)
    if (range.contains (value, is_unsigned))
      return true;
  return false;
}

static void
compute_variant_fields_inner (struct type *type,
			      struct property_addr_info *addr_stack,
			      const variant_part &part,
			      std::vector<bool> &flags);

/* A helper function to determine which variant fields will be active.
   This handles both the variant's direct fields, and any variant
   parts embedded in this variant.  TYPE is the type we're examining.
   ADDR_STACK holds information about the concrete object.  VARIANT is
   the current variant to be handled.  FLAGS is where the results are
   stored -- this function sets the Nth element in FLAGS if the
   corresponding field is enabled.  ENABLED is whether this variant is
   enabled or not.  */

static void
compute_variant_fields_recurse (struct type *type,
				struct property_addr_info *addr_stack,
				const variant &variant,
				std::vector<bool> &flags,
				bool enabled)
{
  for (int field = variant.first_field; field < variant.last_field; ++field)
    flags[field] = enabled;

  for (const variant_part &new_part : variant.parts)
    {
      if (enabled)
	compute_variant_fields_inner (type, addr_stack, new_part, flags);
      else
	{
	  for (const auto &sub_variant : new_part.variants)
	    compute_variant_fields_recurse (type, addr_stack, sub_variant,
					    flags, enabled);
	}
    }
}

/* A helper function to determine which variant fields will be active.
   This evaluates the discriminant, decides which variant (if any) is
   active, and then updates FLAGS to reflect which fields should be
   available.  TYPE is the type we're examining.  ADDR_STACK holds
   information about the concrete object.  VARIANT is the current
   variant to be handled.  FLAGS is where the results are stored --
   this function sets the Nth element in FLAGS if the corresponding
   field is enabled.  */

static void
compute_variant_fields_inner (struct type *type,
			      struct property_addr_info *addr_stack,
			      const variant_part &part,
			      std::vector<bool> &flags)
{
  /* Evaluate the discriminant.  */
  std::optional<ULONGEST> discr_value;
  if (part.discriminant_index != -1)
    {
      int idx = part.discriminant_index;

      if (type->field (idx).loc_kind () != FIELD_LOC_KIND_BITPOS)
	error (_("Cannot determine struct field location"
		 " (invalid location kind)"));

      if (addr_stack->valaddr.data () != NULL)
	discr_value = unpack_field_as_long (type, addr_stack->valaddr.data (),
					    idx);
      else
	{
	  CORE_ADDR addr = (addr_stack->addr
			    + (type->field (idx).loc_bitpos ()
			       / TARGET_CHAR_BIT));

	  LONGEST bitsize = type->field (idx).bitsize ();
	  LONGEST size = bitsize / 8;
	  if (size == 0)
	    size = type->field (idx).type ()->length ();

	  gdb_byte bits[sizeof (ULONGEST)];
	  read_memory (addr, bits, size);

	  LONGEST bitpos = (type->field (idx).loc_bitpos ()
			    % TARGET_CHAR_BIT);

	  discr_value = unpack_bits_as_long (type->field (idx).type (),
					     bits, bitpos, bitsize);
	}
    }

  /* Go through each variant and see which applies.  */
  const variant *default_variant = nullptr;
  const variant *applied_variant = nullptr;
  for (const auto &variant : part.variants)
    {
      if (variant.is_default ())
	default_variant = &variant;
      else if (discr_value.has_value ()
	       && variant.matches (*discr_value, part.is_unsigned))
	{
	  applied_variant = &variant;
	  break;
	}
    }
  if (applied_variant == nullptr)
    applied_variant = default_variant;

  for (const auto &variant : part.variants)
    compute_variant_fields_recurse (type, addr_stack, variant,
				    flags, applied_variant == &variant);
}  

/* Determine which variant fields are available in TYPE.  The enabled
   fields are stored in RESOLVED_TYPE.  ADDR_STACK holds information
   about the concrete object.  PARTS describes the top-level variant
   parts for this type.  */

static void
compute_variant_fields (struct type *type,
			struct type *resolved_type,
			struct property_addr_info *addr_stack,
			const gdb::array_view<variant_part> &parts)
{
  /* Assume all fields are included by default.  */
  std::vector<bool> flags (resolved_type->num_fields (), true);

  /* Now disable fields based on the variants that control them.  */
  for (const auto &part : parts)
    compute_variant_fields_inner (type, addr_stack, part, flags);

  unsigned int nfields = std::count (flags.begin (), flags.end (), true);
  /* No need to zero-initialize the newly allocated fields, they'll be
     initialized by the copy in the loop below.  */
  resolved_type->alloc_fields (nfields, false);

  int out = 0;
  for (int i = 0; i < type->num_fields (); ++i)
    {
      if (!flags[i])
	continue;

      resolved_type->field (out) = type->field (i);
      ++out;
    }
}

/* Resolve dynamic bounds of members of the struct TYPE to static
   bounds.  ADDR_STACK is a stack of struct property_addr_info to
   be used if needed during the dynamic resolution.  */

static struct type *
resolve_dynamic_struct (struct type *type,
			struct property_addr_info *addr_stack,
			const frame_info_ptr &frame)
{
  struct type *resolved_type;
  int i;
  unsigned resolved_type_bit_length = 0;

  gdb_assert (type->code () == TYPE_CODE_STRUCT);

  resolved_type = copy_type (type);

  dynamic_prop *variant_prop = resolved_type->dyn_prop (DYN_PROP_VARIANT_PARTS);
  if (variant_prop != nullptr && variant_prop->kind () == PROP_VARIANT_PARTS)
    {
      compute_variant_fields (type, resolved_type, addr_stack,
			      *variant_prop->variant_parts ());
      /* We want to leave the property attached, so that the Rust code
	 can tell whether the type was originally an enum.  */
      variant_prop->set_original_type (type);
    }
  else
    {
      resolved_type->copy_fields (type);
    }

  for (i = 0; i < resolved_type->num_fields (); ++i)
    {
      unsigned new_bit_length;
      struct property_addr_info pinfo;

      if (resolved_type->field (i).is_static ())
	continue;

      if (resolved_type->field (i).loc_kind () == FIELD_LOC_KIND_DWARF_BLOCK)
	{
	  struct dwarf2_property_baton baton;
	  baton.property_type
	    = lookup_pointer_type (resolved_type->field (i).type ());
	  baton.locexpr = *resolved_type->field (i).loc_dwarf_block ();

	  struct dynamic_prop prop;
	  prop.set_locexpr (&baton);

	  CORE_ADDR addr;
	  if (dwarf2_evaluate_property (&prop, frame, addr_stack, &addr,
					{addr_stack->addr}))
	    resolved_type->field (i).set_loc_bitpos
	      (TARGET_CHAR_BIT * (addr - addr_stack->addr));
	}

      /* As we know this field is not a static field, the field's
	 field_loc_kind should be FIELD_LOC_KIND_BITPOS.  Verify
	 this is the case, but only trigger a simple error rather
	 than an internal error if that fails.  While failing
	 that verification indicates a bug in our code, the error
	 is not severe enough to suggest to the user he stops
	 his debugging session because of it.  */
      if (resolved_type->field (i).loc_kind () != FIELD_LOC_KIND_BITPOS)
	error (_("Cannot determine struct field location"
		 " (invalid location kind)"));

      pinfo.type = check_typedef (resolved_type->field (i).type ());
      size_t offset = resolved_type->field (i).loc_bitpos () / TARGET_CHAR_BIT;
      pinfo.valaddr = addr_stack->valaddr;
      if (!pinfo.valaddr.empty ())
	pinfo.valaddr = pinfo.valaddr.slice (offset);
      pinfo.addr = addr_stack->addr + offset;
      pinfo.next = addr_stack;

      resolved_type->field (i).set_type
	(resolve_dynamic_type_internal (resolved_type->field (i).type (),
					&pinfo, frame, 0));
      gdb_assert (resolved_type->field (i).loc_kind ()
		  == FIELD_LOC_KIND_BITPOS);

      new_bit_length = resolved_type->field (i).loc_bitpos ();
      if (resolved_type->field (i).bitsize () != 0)
	new_bit_length += resolved_type->field (i).bitsize ();
      else
	{
	  struct type *real_type
	    = check_typedef (resolved_type->field (i).type ());

	  new_bit_length += (real_type->length () * TARGET_CHAR_BIT);
	}

      /* Normally, we would use the position and size of the last field
	 to determine the size of the enclosing structure.  But GCC seems
	 to be encoding the position of some fields incorrectly when
	 the struct contains a dynamic field that is not placed last.
	 So we compute the struct size based on the field that has
	 the highest position + size - probably the best we can do.  */
      if (new_bit_length > resolved_type_bit_length)
	resolved_type_bit_length = new_bit_length;
    }

  /* The length of a type won't change for fortran, but it does for C and Ada.
     For fortran the size of dynamic fields might change over time but not the
     type length of the structure.  If we adapt it, we run into problems
     when calculating the element offset for arrays of structs.  */
  if (current_language->la_language != language_fortran)
    resolved_type->set_length ((resolved_type_bit_length + TARGET_CHAR_BIT - 1)
			       / TARGET_CHAR_BIT);

  /* The Ada language uses this field as a cache for static fixed types: reset
     it as RESOLVED_TYPE must have its own static fixed type.  */
  resolved_type->set_target_type (nullptr);

  return resolved_type;
}

/* Worker for resolved_dynamic_type.  */

static struct type *
resolve_dynamic_type_internal (struct type *type,
			       struct property_addr_info *addr_stack,
			       const frame_info_ptr &frame,
			       int top_level)
{
  struct type *real_type = check_typedef (type);
  struct type *resolved_type = nullptr;
  struct dynamic_prop *prop;
  CORE_ADDR value;

  if (!is_dynamic_type_internal (real_type, top_level))
    return type;

  std::optional<CORE_ADDR> type_length;
  prop = TYPE_DYNAMIC_LENGTH (type);
  if (prop != NULL
      && dwarf2_evaluate_property (prop, frame, addr_stack, &value))
    type_length = value;

  if (type->code () == TYPE_CODE_TYPEDEF)
    {
      resolved_type = copy_type (type);
      resolved_type->set_target_type
	(resolve_dynamic_type_internal (type->target_type (), addr_stack,
					frame, top_level));
    }
  else
    {
      /* Before trying to resolve TYPE, make sure it is not a stub.  */
      type = real_type;

      switch (type->code ())
	{
	case TYPE_CODE_REF:
	  {
	    struct property_addr_info pinfo;

	    pinfo.type = check_typedef (type->target_type ());
	    pinfo.valaddr = {};
	    if (addr_stack->valaddr.data () != NULL)
	      pinfo.addr = extract_typed_address (addr_stack->valaddr.data (),
						  type);
	    else
	      pinfo.addr = read_memory_typed_address (addr_stack->addr, type);
	    pinfo.next = addr_stack;

	    resolved_type = copy_type (type);
	    resolved_type->set_target_type
	      (resolve_dynamic_type_internal (type->target_type (),
					      &pinfo, frame, top_level));
	    break;
	  }

	case TYPE_CODE_STRING:
	  /* Strings are very much like an array of characters, and can be
	     treated as one here.  */
	case TYPE_CODE_ARRAY:
	  resolved_type = resolve_dynamic_array_or_string (type, addr_stack,
							   frame);
	  break;

	case TYPE_CODE_RANGE:
	  /* Pass 0 for the rank value here, which indicates this is a
	     range for the first rank of an array.  The assumption is that
	     this rank value is not actually required for the resolution of
	     the dynamic range, otherwise, we'd be resolving this range
	     within the context of a dynamic array.  */
	  resolved_type = resolve_dynamic_range (type, addr_stack, frame, 0);
	  break;

	case TYPE_CODE_UNION:
	  resolved_type = resolve_dynamic_union (type, addr_stack, frame);
	  break;

	case TYPE_CODE_STRUCT:
	  resolved_type = resolve_dynamic_struct (type, addr_stack, frame);
	  break;
	}
    }

  if (resolved_type == nullptr)
    return type;

  if (type_length.has_value ())
    {
      resolved_type->set_length (*type_length);
      resolved_type->remove_dyn_prop (DYN_PROP_BYTE_SIZE);
    }

  /* Resolve data_location attribute.  */
  prop = TYPE_DATA_LOCATION (resolved_type);
  if (prop != NULL
      && dwarf2_evaluate_property (prop, frame, addr_stack, &value))
    {
      /* Start of Fortran hack.  See comment in f-lang.h for what is going
	 on here.*/
      if (current_language->la_language == language_fortran
	  && resolved_type->code () == TYPE_CODE_ARRAY)
	value = fortran_adjust_dynamic_array_base_address_hack (resolved_type,
								value);
      /* End of Fortran hack.  */
      prop->set_const_val (value);
    }

  return resolved_type;
}

/* See gdbtypes.h  */

struct type *
resolve_dynamic_type (struct type *type,
		      gdb::array_view<const gdb_byte> valaddr,
		      CORE_ADDR addr,
		      const frame_info_ptr *in_frame)
{
  struct property_addr_info pinfo
    = {check_typedef (type), valaddr, addr, NULL};

  frame_info_ptr frame;
  if (in_frame != nullptr)
    frame = *in_frame;

  return resolve_dynamic_type_internal (type, &pinfo, frame, 1);
}

/* See gdbtypes.h  */

dynamic_prop *
type::dyn_prop (dynamic_prop_node_kind prop_kind) const
{
  dynamic_prop_list *node = this->main_type->dyn_prop_list;

  while (node != NULL)
    {
      if (node->prop_kind == prop_kind)
	return &node->prop;
      node = node->next;
    }
  return NULL;
}

/* See gdbtypes.h  */

void
type::add_dyn_prop (dynamic_prop_node_kind prop_kind, dynamic_prop prop)
{
  struct dynamic_prop_list *temp;

  gdb_assert (this->is_objfile_owned ());

  temp = XOBNEW (&this->objfile_owner ()->objfile_obstack,
		 struct dynamic_prop_list);
  temp->prop_kind = prop_kind;
  temp->prop = prop;
  temp->next = this->main_type->dyn_prop_list;

  this->main_type->dyn_prop_list = temp;
}

/* See gdbtypes.h.  */

void
type::remove_dyn_prop (dynamic_prop_node_kind kind)
{
  struct dynamic_prop_list *prev_node, *curr_node;

  curr_node = this->main_type->dyn_prop_list;
  prev_node = NULL;

  while (NULL != curr_node)
    {
      if (curr_node->prop_kind == kind)
	{
	  /* Update the linked list but don't free anything.
	     The property was allocated on objstack and it is not known
	     if we are on top of it.  Nevertheless, everything is released
	     when the complete objstack is freed.  */
	  if (NULL == prev_node)
	    this->main_type->dyn_prop_list = curr_node->next;
	  else
	    prev_node->next = curr_node->next;

	  return;
	}

      prev_node = curr_node;
      curr_node = curr_node->next;
    }
}

/* Find the real type of TYPE.  This function returns the real type,
   after removing all layers of typedefs, and completing opaque or stub
   types.  Completion changes the TYPE argument, but stripping of
   typedefs does not.

   Instance flags (e.g. const/volatile) are preserved as typedefs are
   stripped.  If necessary a new qualified form of the underlying type
   is created.

   NOTE: This will return a typedef if type::target_type for the typedef has
   not been computed and we're either in the middle of reading symbols, or
   there was no name for the typedef in the debug info.

   NOTE: Lookup of opaque types can throw errors for invalid symbol files.
   QUITs in the symbol reading code can also throw.
   Thus this function can throw an exception.

   If TYPE is a TYPE_CODE_TYPEDEF, its length is updated to the length of
   the target type.

   If this is a stubbed struct (i.e. declared as struct foo *), see if
   we can find a full definition in some other file.  If so, copy this
   definition, so we can use it in future.  There used to be a comment
   (but not any code) that if we don't find a full definition, we'd
   set a flag so we don't spend time in the future checking the same
   type.  That would be a mistake, though--we might load in more
   symbols which contain a full definition for the type.  */

struct type *
check_typedef (struct type *type)
{
  struct type *orig_type = type;

  gdb_assert (type);

  /* While we're removing typedefs, we don't want to lose qualifiers.
     E.g., const/volatile.  */
  type_instance_flags instance_flags = type->instance_flags ();

  while (type->code () == TYPE_CODE_TYPEDEF)
    {
      if (!type->target_type ())
	{
	  const char *name;
	  struct symbol *sym;

	  /* It is dangerous to call lookup_symbol if we are currently
	     reading a symtab.  Infinite recursion is one danger.  */
	  if (currently_reading_symtab)
	    return make_qualified_type (type, instance_flags, NULL);

	  name = type->name ();
	  /* FIXME: shouldn't we look in STRUCT_DOMAIN and/or
	     VAR_DOMAIN as appropriate?  */
	  if (name == NULL)
	    {
	      stub_noname_complaint ();
	      return make_qualified_type (type, instance_flags, NULL);
	    }
	  sym = lookup_symbol (name, 0, STRUCT_DOMAIN, 0).symbol;
	  if (sym)
	    type->set_target_type (sym->type ());
	  else					/* TYPE_CODE_UNDEF */
	    type->set_target_type (type_allocator (type->arch ()).new_type ());
	}
      type = type->target_type ();

      /* Preserve the instance flags as we traverse down the typedef chain.

	 Handling address spaces/classes is nasty, what do we do if there's a
	 conflict?
	 E.g., what if an outer typedef marks the type as class_1 and an inner
	 typedef marks the type as class_2?
	 This is the wrong place to do such error checking.  We leave it to
	 the code that created the typedef in the first place to flag the
	 error.  We just pick the outer address space (akin to letting the
	 outer cast in a chain of casting win), instead of assuming
	 "it can't happen".  */
      {
	const type_instance_flags ALL_SPACES
	  = (TYPE_INSTANCE_FLAG_CODE_SPACE
	     | TYPE_INSTANCE_FLAG_DATA_SPACE);
	const type_instance_flags ALL_CLASSES
	  = TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL;

	type_instance_flags new_instance_flags = type->instance_flags ();

	/* Treat code vs data spaces and address classes separately.  */
	if ((instance_flags & ALL_SPACES) != 0)
	  new_instance_flags &= ~ALL_SPACES;
	if ((instance_flags & ALL_CLASSES) != 0)
	  new_instance_flags &= ~ALL_CLASSES;

	instance_flags |= new_instance_flags;
      }
    }

  /* If this is a struct/class/union with no fields, then check
     whether a full definition exists somewhere else.  This is for
     systems where a type definition with no fields is issued for such
     types, instead of identifying them as stub types in the first
     place.  */

  if (TYPE_IS_OPAQUE (type) 
      && opaque_type_resolution 
      && !currently_reading_symtab)
    {
      const char *name = type->name ();
      struct type *newtype;

      if (name == NULL)
	{
	  stub_noname_complaint ();
	  return make_qualified_type (type, instance_flags, NULL);
	}
      newtype = lookup_transparent_type (name);

      if (newtype)
	{
	  /* If the resolved type and the stub are in the same
	     objfile, then replace the stub type with the real deal.
	     But if they're in separate objfiles, leave the stub
	     alone; we'll just look up the transparent type every time
	     we call check_typedef.  We can't create pointers between
	     types allocated to different objfiles, since they may
	     have different lifetimes.  Trying to copy NEWTYPE over to
	     TYPE's objfile is pointless, too, since you'll have to
	     move over any other types NEWTYPE refers to, which could
	     be an unbounded amount of stuff.  */
	  if (newtype->objfile_owner () == type->objfile_owner ())
	    type = make_qualified_type (newtype, type->instance_flags (), type);
	  else
	    type = newtype;
	}
    }
  /* Otherwise, rely on the stub flag being set for opaque/stubbed
     types.  */
  else if (type->is_stub () && !currently_reading_symtab)
    {
      const char *name = type->name ();
      /* FIXME: shouldn't we look in STRUCT_DOMAIN and/or VAR_DOMAIN
	 as appropriate?  */
      struct symbol *sym;

      if (name == NULL)
	{
	  stub_noname_complaint ();
	  return make_qualified_type (type, instance_flags, NULL);
	}
      sym = lookup_symbol (name, 0, STRUCT_DOMAIN, 0).symbol;
      if (sym)
	{
	  /* Same as above for opaque types, we can replace the stub
	     with the complete type only if they are in the same
	     objfile.  */
	  if (sym->type ()->objfile_owner () == type->objfile_owner ())
	    type = make_qualified_type (sym->type (),
					type->instance_flags (), type);
	  else
	    type = sym->type ();
	}
    }

  if (type->target_is_stub ())
    {
      struct type *target_type = check_typedef (type->target_type ());

      if (target_type->is_stub () || target_type->target_is_stub ())
	{
	  /* Nothing we can do.  */
	}
      else if (type->code () == TYPE_CODE_RANGE)
	{
	  type->set_length (target_type->length ());
	  type->set_target_is_stub (false);
	}
      else if (type->code () == TYPE_CODE_ARRAY
	       && update_static_array_size (type))
	type->set_target_is_stub (false);
    }

  type = make_qualified_type (type, instance_flags, NULL);

  /* Cache TYPE_LENGTH for future use.  */
  orig_type->set_length (type->length ());

  return type;
}

/* Parse a type expression in the string [P..P+LENGTH).  If an error
   occurs, silently return a void type.  */

static struct type *
safe_parse_type (struct gdbarch *gdbarch, const char *p, int length)
{
  struct type *type = NULL; /* Initialize to keep gcc happy.  */

  /* Suppress error messages.  */
  scoped_restore saved_gdb_stderr = make_scoped_restore (&gdb_stderr,
							 &null_stream);

  /* Call parse_and_eval_type() without fear of longjmp()s.  */
  try
    {
      type = parse_and_eval_type (p, length);
    }
  catch (const gdb_exception_error &except)
    {
      type = builtin_type (gdbarch)->builtin_void;
    }

  return type;
}

/* Ugly hack to convert method stubs into method types.

   He ain't kiddin'.  This demangles the name of the method into a
   string including argument types, parses out each argument type,
   generates a string casting a zero to that type, evaluates the
   string, and stuffs the resulting type into an argtype vector!!!
   Then it knows the type of the whole function (including argument
   types for overloading), which info used to be in the stab's but was
   removed to hack back the space required for them.  */

static void
check_stub_method (struct type *type, int method_id, int signature_id)
{
  struct gdbarch *gdbarch = type->arch ();
  struct fn_field *f;
  char *mangled_name = gdb_mangle_name (type, method_id, signature_id);
  gdb::unique_xmalloc_ptr<char> demangled_name
    = gdb_demangle (mangled_name, DMGL_PARAMS | DMGL_ANSI);
  char *argtypetext, *p;
  int depth = 0, argcount = 1;
  struct field *argtypes;
  struct type *mtype;

  /* Make sure we got back a function string that we can use.  */
  if (demangled_name)
    p = strchr (demangled_name.get (), '(');
  else
    p = NULL;

  if (demangled_name == NULL || p == NULL)
    error (_("Internal: Cannot demangle mangled name `%s'."), 
	   mangled_name);

  /* Now, read in the parameters that define this type.  */
  p += 1;
  argtypetext = p;
  while (*p)
    {
      if (*p == '(' || *p == '<')
	{
	  depth += 1;
	}
      else if (*p == ')' || *p == '>')
	{
	  depth -= 1;
	}
      else if (*p == ',' && depth == 0)
	{
	  argcount += 1;
	}

      p += 1;
    }

  /* If we read one argument and it was ``void'', don't count it.  */
  if (startswith (argtypetext, "(void)"))
    argcount -= 1;

  /* We need one extra slot, for the THIS pointer.  */

  argtypes = (struct field *)
    TYPE_ZALLOC (type, (argcount + 1) * sizeof (struct field));
  p = argtypetext;

  /* Add THIS pointer for non-static methods.  */
  f = TYPE_FN_FIELDLIST1 (type, method_id);
  if (TYPE_FN_FIELD_STATIC_P (f, signature_id))
    argcount = 0;
  else
    {
      argtypes[0].set_type (lookup_pointer_type (type));
      argcount = 1;
    }

  if (*p != ')')		/* () means no args, skip while.  */
    {
      depth = 0;
      while (*p)
	{
	  if (depth <= 0 && (*p == ',' || *p == ')'))
	    {
	      /* Avoid parsing of ellipsis, they will be handled below.
		 Also avoid ``void'' as above.  */
	      if (strncmp (argtypetext, "...", p - argtypetext) != 0
		  && strncmp (argtypetext, "void", p - argtypetext) != 0)
		{
		  argtypes[argcount].set_type
		    (safe_parse_type (gdbarch, argtypetext, p - argtypetext));
		  argcount += 1;
		}
	      argtypetext = p + 1;
	    }

	  if (*p == '(' || *p == '<')
	    {
	      depth += 1;
	    }
	  else if (*p == ')' || *p == '>')
	    {
	      depth -= 1;
	    }

	  p += 1;
	}
    }

  TYPE_FN_FIELD_PHYSNAME (f, signature_id) = mangled_name;

  /* Now update the old "stub" type into a real type.  */
  mtype = TYPE_FN_FIELD_TYPE (f, signature_id);
  /* MTYPE may currently be a function (TYPE_CODE_FUNC).
     We want a method (TYPE_CODE_METHOD).  */
  smash_to_method_type (mtype, type, mtype->target_type (),
			argtypes, argcount, p[-2] == '.');
  mtype->set_is_stub (false);
  TYPE_FN_FIELD_STUB (f, signature_id) = 0;
}

/* This is the external interface to check_stub_method, above.  This
   function unstubs all of the signatures for TYPE's METHOD_ID method
   name.  After calling this function TYPE_FN_FIELD_STUB will be
   cleared for each signature and TYPE_FN_FIELDLIST_NAME will be
   correct.

   This function unfortunately can not die until stabs do.  */

void
check_stub_method_group (struct type *type, int method_id)
{
  int len = TYPE_FN_FIELDLIST_LENGTH (type, method_id);
  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, method_id);

  for (int j = 0; j < len; j++)
    {
      if (TYPE_FN_FIELD_STUB (f, j))
	check_stub_method (type, method_id, j);
    }
}

/* Ensure it is in .rodata (if available) by working around GCC PR 44690.  */
const struct cplus_struct_type cplus_struct_default = { };

void
allocate_cplus_struct_type (struct type *type)
{
  if (HAVE_CPLUS_STRUCT (type))
    /* Structure was already allocated.  Nothing more to do.  */
    return;

  TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_CPLUS_STUFF;
  TYPE_RAW_CPLUS_SPECIFIC (type) = (struct cplus_struct_type *)
    TYPE_ZALLOC (type, sizeof (struct cplus_struct_type));
  *(TYPE_RAW_CPLUS_SPECIFIC (type)) = cplus_struct_default;
  set_type_vptr_fieldno (type, -1);
}

const struct gnat_aux_type gnat_aux_default =
  { NULL };

/* Set the TYPE's type-specific kind to TYPE_SPECIFIC_GNAT_STUFF,
   and allocate the associated gnat-specific data.  The gnat-specific
   data is also initialized to gnat_aux_default.  */

void
allocate_gnat_aux_type (struct type *type)
{
  TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_GNAT_STUFF;
  TYPE_GNAT_SPECIFIC (type) = (struct gnat_aux_type *)
    TYPE_ZALLOC (type, sizeof (struct gnat_aux_type));
  *(TYPE_GNAT_SPECIFIC (type)) = gnat_aux_default;
}

/* Helper function to verify floating-point format and size.
   BIT is the type size in bits; if BIT equals -1, the size is
   determined by the floatformat.  Returns size to be used.  */

static int
verify_floatformat (int bit, const struct floatformat *floatformat)
{
  gdb_assert (floatformat != NULL);

  if (bit == -1)
    bit = floatformat->totalsize;

  gdb_assert (bit >= 0);
  gdb_assert (bit >= floatformat->totalsize);

  return bit;
}

/* Return the floating-point format for a floating-point variable of
   type TYPE.  */

const struct floatformat *
floatformat_from_type (const struct type *type)
{
  gdb_assert (type->code () == TYPE_CODE_FLT);
  gdb_assert (TYPE_FLOATFORMAT (type));
  return TYPE_FLOATFORMAT (type);
}

/* See gdbtypes.h.  */

struct type *
init_integer_type (type_allocator &alloc,
		   int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = alloc.new_type (TYPE_CODE_INT, bit, name);
  if (unsigned_p)
    t->set_is_unsigned (true);

  TYPE_SPECIFIC_FIELD (t) = TYPE_SPECIFIC_INT;
  TYPE_MAIN_TYPE (t)->type_specific.int_stuff.bit_size = bit;
  TYPE_MAIN_TYPE (t)->type_specific.int_stuff.bit_offset = 0;

  return t;
}

/* See gdbtypes.h.  */

struct type *
init_character_type (type_allocator &alloc,
		     int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = alloc.new_type (TYPE_CODE_CHAR, bit, name);
  if (unsigned_p)
    t->set_is_unsigned (true);

  return t;
}

/* See gdbtypes.h.  */

struct type *
init_boolean_type (type_allocator &alloc,
		   int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = alloc.new_type (TYPE_CODE_BOOL, bit, name);
  if (unsigned_p)
    t->set_is_unsigned (true);

  TYPE_SPECIFIC_FIELD (t) = TYPE_SPECIFIC_INT;
  TYPE_MAIN_TYPE (t)->type_specific.int_stuff.bit_size = bit;
  TYPE_MAIN_TYPE (t)->type_specific.int_stuff.bit_offset = 0;

  return t;
}

/* See gdbtypes.h.  */

struct type *
init_float_type (type_allocator &alloc,
		 int bit, const char *name,
		 const struct floatformat **floatformats,
		 enum bfd_endian byte_order)
{
  if (byte_order == BFD_ENDIAN_UNKNOWN)
    {
      struct gdbarch *gdbarch = alloc.arch ();
      byte_order = gdbarch_byte_order (gdbarch);
    }
  const struct floatformat *fmt = floatformats[byte_order];
  struct type *t;

  bit = verify_floatformat (bit, fmt);
  t = alloc.new_type (TYPE_CODE_FLT, bit, name);
  TYPE_FLOATFORMAT (t) = fmt;

  return t;
}

/* See gdbtypes.h.  */

struct type *
init_decfloat_type (type_allocator &alloc, int bit, const char *name)
{
  return alloc.new_type (TYPE_CODE_DECFLOAT, bit, name);
}

/* Return true if init_complex_type can be called with TARGET_TYPE.  */

bool
can_create_complex_type (struct type *target_type)
{
  return (target_type->code () == TYPE_CODE_INT
	  || target_type->code () == TYPE_CODE_FLT);
}

/* Allocate a TYPE_CODE_COMPLEX type structure.  NAME is the type
   name.  TARGET_TYPE is the component type.  */

struct type *
init_complex_type (const char *name, struct type *target_type)
{
  struct type *t;

  gdb_assert (can_create_complex_type (target_type));

  if (TYPE_MAIN_TYPE (target_type)->flds_bnds.complex_type == nullptr)
    {
      if (name == nullptr && target_type->name () != nullptr)
	{
	  /* No zero-initialization required, initialized by strcpy/strcat
	     below.  */
	  char *new_name
	    = (char *) TYPE_ALLOC (target_type,
				   strlen (target_type->name ())
				   + strlen ("_Complex ") + 1);
	  strcpy (new_name, "_Complex ");
	  strcat (new_name, target_type->name ());
	  name = new_name;
	}

      t = type_allocator (target_type).new_type ();
      set_type_code (t, TYPE_CODE_COMPLEX);
      t->set_length (2 * target_type->length ());
      t->set_name (name);

      t->set_target_type (target_type);
      TYPE_MAIN_TYPE (target_type)->flds_bnds.complex_type = t;
    }

  return TYPE_MAIN_TYPE (target_type)->flds_bnds.complex_type;
}

/* See gdbtypes.h.  */

struct type *
init_pointer_type (type_allocator &alloc,
		   int bit, const char *name, struct type *target_type)
{
  struct type *t;

  t = alloc.new_type (TYPE_CODE_PTR, bit, name);
  t->set_target_type (target_type);
  t->set_is_unsigned (true);
  return t;
}

/* Allocate a TYPE_CODE_FIXED_POINT type structure associated with OBJFILE.
   BIT is the pointer type size in bits.
   UNSIGNED_P should be nonzero if the type is unsigned.
   NAME is the type name.  */

struct type *
init_fixed_point_type (type_allocator &alloc,
		       int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = alloc.new_type (TYPE_CODE_FIXED_POINT, bit, name);
  if (unsigned_p)
    t->set_is_unsigned (true);

  return t;
}

/* See gdbtypes.h.  */

unsigned
type_raw_align (struct type *type)
{
  if (type->align_log2 != 0)
    return 1 << (type->align_log2 - 1);
  return 0;
}

/* See gdbtypes.h.  */

unsigned
type_align (struct type *type)
{
  /* Check alignment provided in the debug information.  */
  unsigned raw_align = type_raw_align (type);
  if (raw_align != 0)
    return raw_align;

  /* Allow the architecture to provide an alignment.  */
  ULONGEST align = gdbarch_type_align (type->arch (), type);
  if (align != 0)
    return align;

  switch (type->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_INT:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_FLT:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_DECFLOAT:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      align = type_length_units (check_typedef (type));
      break;

    case TYPE_CODE_ARRAY:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_TYPEDEF:
      align = type_align (type->target_type ());
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	int number_of_non_static_fields = 0;
	for (unsigned i = 0; i < type->num_fields (); ++i)
	  {
	    if (!type->field (i).is_static ())
	      {
		number_of_non_static_fields++;
		ULONGEST f_align = type_align (type->field (i).type ());
		if (f_align == 0)
		  {
		    /* Don't pretend we know something we don't.  */
		    align = 0;
		    break;
		  }
		if (f_align > align)
		  align = f_align;
	      }
	  }
	/* A struct with no fields, or with only static fields has an
	   alignment of 1.  */
	if (number_of_non_static_fields == 0)
	  align = 1;
      }
      break;

    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
      /* Not sure what to do here, and these can't appear in C or C++
	 anyway.  */
      break;

    case TYPE_CODE_VOID:
      align = 1;
      break;

    case TYPE_CODE_ERROR:
    case TYPE_CODE_METHOD:
    default:
      break;
    }

  if ((align & (align - 1)) != 0)
    {
      /* Not a power of 2, so pass.  */
      align = 0;
    }

  return align;
}

/* See gdbtypes.h.  */

bool
set_type_align (struct type *type, ULONGEST align)
{
  /* Must be a power of 2.  Zero is ok.  */
  gdb_assert ((align & (align - 1)) == 0);

  unsigned result = 0;
  while (align != 0)
    {
      ++result;
      align >>= 1;
    }

  if (result >= (1 << TYPE_ALIGN_BITS))
    return false;

  type->align_log2 = result;
  return true;
}


/* Queries on types.  */

int
can_dereference (struct type *t)
{
  /* FIXME: Should we return true for references as well as
     pointers?  */
  t = check_typedef (t);
  return
    (t != NULL
     && t->code () == TYPE_CODE_PTR
     && t->target_type ()->code () != TYPE_CODE_VOID);
}

int
is_integral_type (struct type *t)
{
  t = check_typedef (t);
  return
    ((t != NULL)
     && !is_fixed_point_type (t)
     && ((t->code () == TYPE_CODE_INT)
	 || (t->code () == TYPE_CODE_ENUM)
	 || (t->code () == TYPE_CODE_FLAGS)
	 || (t->code () == TYPE_CODE_CHAR)
	 || (t->code () == TYPE_CODE_RANGE)
	 || (t->code () == TYPE_CODE_BOOL)));
}

int
is_floating_type (struct type *t)
{
  t = check_typedef (t);
  return
    ((t != NULL)
     && ((t->code () == TYPE_CODE_FLT)
	 || (t->code () == TYPE_CODE_DECFLOAT)));
}

/* Return true if TYPE is scalar.  */

int
is_scalar_type (struct type *type)
{
  type = check_typedef (type);

  if (is_fixed_point_type (type))
    return 0; /* Implemented as a scalar, but more like a floating point.  */

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
      return 0;
    default:
      return 1;
    }
}

/* Return true if T is scalar, or a composite type which in practice has
   the memory layout of a scalar type.  E.g., an array or struct with only
   one scalar element inside it, or a union with only scalar elements.  */

int
is_scalar_type_recursive (struct type *t)
{
  t = check_typedef (t);

  if (is_scalar_type (t))
    return 1;
  /* Are we dealing with an array or string of known dimensions?  */
  else if ((t->code () == TYPE_CODE_ARRAY
	    || t->code () == TYPE_CODE_STRING) && t->num_fields () == 1
	   && t->index_type ()->code () == TYPE_CODE_RANGE)
    {
      LONGEST low_bound, high_bound;
      struct type *elt_type = check_typedef (t->target_type ());

      if (get_discrete_bounds (t->index_type (), &low_bound, &high_bound))
	return (high_bound == low_bound
		&& is_scalar_type_recursive (elt_type));
      else
	return 0;
    }
  /* Are we dealing with a struct with one element?  */
  else if (t->code () == TYPE_CODE_STRUCT && t->num_fields () == 1)
    return is_scalar_type_recursive (t->field (0).type ());
  else if (t->code () == TYPE_CODE_UNION)
    {
      int i, n = t->num_fields ();

      /* If all elements of the union are scalar, then the union is scalar.  */
      for (i = 0; i < n; i++)
	if (!is_scalar_type_recursive (t->field (i).type ()))
	  return 0;

      return 1;
    }

  return 0;
}

/* Return true is T is a class or a union.  False otherwise.  */

int
class_or_union_p (const struct type *t)
{
  return (t->code () == TYPE_CODE_STRUCT
	  || t->code () == TYPE_CODE_UNION);
}

/* A helper function which returns true if types A and B represent the
   "same" class type.  This is true if the types have the same main
   type, or the same name.  */

int
class_types_same_p (const struct type *a, const struct type *b)
{
  return (TYPE_MAIN_TYPE (a) == TYPE_MAIN_TYPE (b)
	  || (a->name () && b->name ()
	      && !strcmp (a->name (), b->name ())));
}

/* If BASE is an ancestor of DCLASS return the distance between them.
   otherwise return -1;
   eg:

   class A {};
   class B: public A {};
   class C: public B {};
   class D: C {};

   distance_to_ancestor (A, A, 0) = 0
   distance_to_ancestor (A, B, 0) = 1
   distance_to_ancestor (A, C, 0) = 2
   distance_to_ancestor (A, D, 0) = 3

   If PUBLIC is 1 then only public ancestors are considered,
   and the function returns the distance only if BASE is a public ancestor
   of DCLASS.
   Eg:

   distance_to_ancestor (A, D, 1) = -1.  */

static int
distance_to_ancestor (struct type *base, struct type *dclass, int is_public)
{
  int i;
  int d;

  base = check_typedef (base);
  dclass = check_typedef (dclass);

  if (class_types_same_p (base, dclass))
    return 0;

  for (i = 0; i < TYPE_N_BASECLASSES (dclass); i++)
    {
      if (is_public && ! BASETYPE_VIA_PUBLIC (dclass, i))
	continue;

      d = distance_to_ancestor (base, TYPE_BASECLASS (dclass, i), is_public);
      if (d >= 0)
	return 1 + d;
    }

  return -1;
}

/* Check whether BASE is an ancestor or base class or DCLASS
   Return 1 if so, and 0 if not.
   Note: If BASE and DCLASS are of the same type, this function
   will return 1. So for some class A, is_ancestor (A, A) will
   return 1.  */

int
is_ancestor (struct type *base, struct type *dclass)
{
  return distance_to_ancestor (base, dclass, 0) >= 0;
}

/* Like is_ancestor, but only returns true when BASE is a public
   ancestor of DCLASS.  */

int
is_public_ancestor (struct type *base, struct type *dclass)
{
  return distance_to_ancestor (base, dclass, 1) >= 0;
}

/* A helper function for is_unique_ancestor.  */

static int
is_unique_ancestor_worker (struct type *base, struct type *dclass,
			   int *offset,
			   const gdb_byte *valaddr, int embedded_offset,
			   CORE_ADDR address, struct value *val)
{
  int i, count = 0;

  base = check_typedef (base);
  dclass = check_typedef (dclass);

  for (i = 0; i < TYPE_N_BASECLASSES (dclass) && count < 2; ++i)
    {
      struct type *iter;
      int this_offset;

      iter = check_typedef (TYPE_BASECLASS (dclass, i));

      this_offset = baseclass_offset (dclass, i, valaddr, embedded_offset,
				      address, val);

      if (class_types_same_p (base, iter))
	{
	  /* If this is the first subclass, set *OFFSET and set count
	     to 1.  Otherwise, if this is at the same offset as
	     previous instances, do nothing.  Otherwise, increment
	     count.  */
	  if (*offset == -1)
	    {
	      *offset = this_offset;
	      count = 1;
	    }
	  else if (this_offset == *offset)
	    {
	      /* Nothing.  */
	    }
	  else
	    ++count;
	}
      else
	count += is_unique_ancestor_worker (base, iter, offset,
					    valaddr,
					    embedded_offset + this_offset,
					    address, val);
    }

  return count;
}

/* Like is_ancestor, but only returns true if BASE is a unique base
   class of the type of VAL.  */

int
is_unique_ancestor (struct type *base, struct value *val)
{
  int offset = -1;

  return is_unique_ancestor_worker (base, val->type (), &offset,
				    val->contents_for_printing ().data (),
				    val->embedded_offset (),
				    val->address (), val) == 1;
}

/* See gdbtypes.h.  */

enum bfd_endian
type_byte_order (const struct type *type)
{
  bfd_endian byteorder = gdbarch_byte_order (type->arch ());
  if (type->endianity_is_not_default ())
    {
      if (byteorder == BFD_ENDIAN_BIG)
	return BFD_ENDIAN_LITTLE;
      else
	{
	  gdb_assert (byteorder == BFD_ENDIAN_LITTLE);
	  return BFD_ENDIAN_BIG;
	}
    }

  return byteorder;
}

/* See gdbtypes.h.  */

bool
is_nocall_function (const struct type *type)
{
  if (type->code () != TYPE_CODE_FUNC && type->code () != TYPE_CODE_METHOD)
    return false;

  return TYPE_CALLING_CONVENTION (type) == DW_CC_nocall;
}


/* Overload resolution.  */

/* Return the sum of the rank of A with the rank of B.  */

struct rank
sum_ranks (struct rank a, struct rank b)
{
  struct rank c;
  c.rank = a.rank + b.rank;
  c.subrank = a.subrank + b.subrank;
  return c;
}

/* Compare rank A and B and return:
   0 if a = b
   1 if a is better than b
  -1 if b is better than a.  */

int
compare_ranks (struct rank a, struct rank b)
{
  if (a.rank == b.rank)
    {
      if (a.subrank == b.subrank)
	return 0;
      if (a.subrank < b.subrank)
	return 1;
      if (a.subrank > b.subrank)
	return -1;
    }

  if (a.rank < b.rank)
    return 1;

  /* a.rank > b.rank */
  return -1;
}

/* Functions for overload resolution begin here.  */

/* Compare two badness vectors A and B and return the result.
   0 => A and B are identical
   1 => A and B are incomparable
   2 => A is better than B
   3 => A is worse than B  */

int
compare_badness (const badness_vector &a, const badness_vector &b)
{
  int i;
  int tmp;
  /* Any positives in comparison? */
  bool found_pos = false;
  /* Any negatives in comparison? */
  bool found_neg = false;
  /* Did A have any INVALID_CONVERSION entries.  */
  bool a_invalid = false;
  /* Did B have any INVALID_CONVERSION entries.  */
  bool b_invalid = false;

  /* differing sizes => incomparable */
  if (a.size () != b.size ())
    return 1;

  /* Subtract b from a */
  for (i = 0; i < a.size (); i++)
    {
      tmp = compare_ranks (b[i], a[i]);
      if (tmp > 0)
	found_pos = true;
      else if (tmp < 0)
	found_neg = true;
      if (a[i].rank >= INVALID_CONVERSION)
	a_invalid = true;
      if (b[i].rank >= INVALID_CONVERSION)
	b_invalid = true;
    }

  /* B will only be considered better than or incomparable to A if
     they both have invalid entries, or if neither does.  That is, if
     A has only valid entries, and B has an invalid entry, then A will
     be considered better than B, even if B happens to be better for
     some parameter.  */
  if (a_invalid != b_invalid)
    {
      if (a_invalid)
	return 3;		/* A > B */
      return 2;			/* A < B */
    }
  else if (found_pos)
    {
      if (found_neg)
	return 1;		/* incomparable */
      else
	return 3;		/* A > B */
    }
  else
    /* no positives */
    {
      if (found_neg)
	return 2;		/* A < B */
      else
	return 0;		/* A == B */
    }
}

/* Rank a function by comparing its parameter types (PARMS), to the
   types of an argument list (ARGS).  Return the badness vector.  This
   has ARGS.size() + 1 entries.  */

badness_vector
rank_function (gdb::array_view<type *> parms,
	       gdb::array_view<value *> args,
	       bool varargs)
{
  /* add 1 for the length-match rank.  */
  badness_vector bv;
  bv.reserve (1 + args.size ());

  /* First compare the lengths of the supplied lists.
     If there is a mismatch, set it to a high value.  */

  /* pai/1997-06-03 FIXME: when we have debug info about default
     arguments and ellipsis parameter lists, we should consider those
     and rank the length-match more finely.  */

  bv.push_back ((args.size () != parms.size ()
		 && (! varargs || args.size () < parms.size ()))
		? LENGTH_MISMATCH_BADNESS
		: EXACT_MATCH_BADNESS);

  /* Now rank all the parameters of the candidate function.  */
  size_t min_len = std::min (parms.size (), args.size ());

  for (size_t i = 0; i < min_len; i++)
    bv.push_back (rank_one_type (parms[i], args[i]->type (),
				 args[i]));

  /* If more arguments than parameters, add dummy entries.  */
  for (size_t i = min_len; i < args.size (); i++)
    bv.push_back (varargs ? VARARG_BADNESS : TOO_FEW_PARAMS_BADNESS);

  return bv;
}

/* Compare the names of two integer types, assuming that any sign
   qualifiers have been checked already.  We do it this way because
   there may be an "int" in the name of one of the types.  */

static int
integer_types_same_name_p (const char *first, const char *second)
{
  int first_p, second_p;

  /* If both are shorts, return 1; if neither is a short, keep
     checking.  */
  first_p = (strstr (first, "short") != NULL);
  second_p = (strstr (second, "short") != NULL);
  if (first_p && second_p)
    return 1;
  if (first_p || second_p)
    return 0;

  /* Likewise for long.  */
  first_p = (strstr (first, "long") != NULL);
  second_p = (strstr (second, "long") != NULL);
  if (first_p && second_p)
    return 1;
  if (first_p || second_p)
    return 0;

  /* Likewise for char.  */
  first_p = (strstr (first, "char") != NULL);
  second_p = (strstr (second, "char") != NULL);
  if (first_p && second_p)
    return 1;
  if (first_p || second_p)
    return 0;

  /* They must both be ints.  */
  return 1;
}

/* Compares type A to type B.  Returns true if they represent the same
   type, false otherwise.  */

bool
types_equal (struct type *a, struct type *b)
{
  /* Identical type pointers.  */
  /* However, this still doesn't catch all cases of same type for b
     and a.  The reason is that builtin types are different from
     the same ones constructed from the object.  */
  if (a == b)
    return true;

  /* Resolve typedefs */
  if (a->code () == TYPE_CODE_TYPEDEF)
    a = check_typedef (a);
  if (b->code () == TYPE_CODE_TYPEDEF)
    b = check_typedef (b);

  /* Check if identical after resolving typedefs.  */
  if (a == b)
    return true;

  /* If after resolving typedefs a and b are not of the same type
     code then they are not equal.  */
  if (a->code () != b->code ())
    return false;

  /* If a and b are both pointers types or both reference types then
     they are equal of the same type iff the objects they refer to are
     of the same type.  */
  if (a->code () == TYPE_CODE_PTR
      || a->code () == TYPE_CODE_REF)
    return types_equal (a->target_type (),
			b->target_type ());

  /* Well, damnit, if the names are exactly the same, I'll say they
     are exactly the same.  This happens when we generate method
     stubs.  The types won't point to the same address, but they
     really are the same.  */

  if (a->name () && b->name ()
      && strcmp (a->name (), b->name ()) == 0)
    return true;

  /* Two function types are equal if their argument and return types
     are equal.  */
  if (a->code () == TYPE_CODE_FUNC)
    {
      int i;

      if (a->num_fields () != b->num_fields ())
	return false;
      
      if (!types_equal (a->target_type (), b->target_type ()))
	return false;

      for (i = 0; i < a->num_fields (); ++i)
	if (!types_equal (a->field (i).type (), b->field (i).type ()))
	  return false;

      return true;
    }

  return false;
}

/* Deep comparison of types.  */

/* An entry in the type-equality bcache.  */

struct type_equality_entry
{
  type_equality_entry (struct type *t1, struct type *t2)
    : type1 (t1),
      type2 (t2)
  {
  }

  struct type *type1, *type2;
};

/* A helper function to compare two strings.  Returns true if they are
   the same, false otherwise.  Handles NULLs properly.  */

static bool
compare_maybe_null_strings (const char *s, const char *t)
{
  if (s == NULL || t == NULL)
    return s == t;
  return strcmp (s, t) == 0;
}

/* A helper function for check_types_worklist that checks two types for
   "deep" equality.  Returns true if the types are considered the
   same, false otherwise.  */

static bool
check_types_equal (struct type *type1, struct type *type2,
		   std::vector<type_equality_entry> *worklist)
{
  type1 = check_typedef (type1);
  type2 = check_typedef (type2);

  if (type1 == type2)
    return true;

  if (type1->code () != type2->code ()
      || type1->length () != type2->length ()
      || type1->is_unsigned () != type2->is_unsigned ()
      || type1->has_no_signedness () != type2->has_no_signedness ()
      || type1->endianity_is_not_default () != type2->endianity_is_not_default ()
      || type1->has_varargs () != type2->has_varargs ()
      || type1->is_vector () != type2->is_vector ()
      || TYPE_NOTTEXT (type1) != TYPE_NOTTEXT (type2)
      || type1->instance_flags () != type2->instance_flags ()
      || type1->num_fields () != type2->num_fields ())
    return false;

  if (!compare_maybe_null_strings (type1->name (), type2->name ()))
    return false;
  if (!compare_maybe_null_strings (type1->name (), type2->name ()))
    return false;

  if (type1->code () == TYPE_CODE_RANGE)
    {
      if (*type1->bounds () != *type2->bounds ())
	return false;
    }
  else
    {
      int i;

      for (i = 0; i < type1->num_fields (); ++i)
	{
	  const struct field *field1 = &type1->field (i);
	  const struct field *field2 = &type2->field (i);

	  if (field1->is_artificial () != field2->is_artificial ()
	      || field1->bitsize () != field2->bitsize ()
	      || field1->loc_kind () != field2->loc_kind ())
	    return false;
	  if (!compare_maybe_null_strings (field1->name (), field2->name ()))
	    return false;
	  switch (field1->loc_kind ())
	    {
	    case FIELD_LOC_KIND_BITPOS:
	      if (field1->loc_bitpos () != field2->loc_bitpos ())
		return false;
	      break;
	    case FIELD_LOC_KIND_ENUMVAL:
	      if (field1->loc_enumval () != field2->loc_enumval ())
		return false;
	      /* Don't compare types of enum fields, because they don't
		 have a type.  */
	      continue;
	    case FIELD_LOC_KIND_PHYSADDR:
	      if (field1->loc_physaddr () != field2->loc_physaddr ())
		return false;
	      break;
	    case FIELD_LOC_KIND_PHYSNAME:
	      if (!compare_maybe_null_strings (field1->loc_physname (),
					       field2->loc_physname ()))
		return false;
	      break;
	    case FIELD_LOC_KIND_DWARF_BLOCK:
	      {
		struct dwarf2_locexpr_baton *block1, *block2;

		block1 = field1->loc_dwarf_block ();
		block2 = field2->loc_dwarf_block ();
		if (block1->per_cu != block2->per_cu
		    || block1->size != block2->size
		    || memcmp (block1->data, block2->data, block1->size) != 0)
		  return false;
	      }
	      break;
	    default:
	      internal_error (_("Unsupported field kind "
						    "%d by check_types_equal"),
			      field1->loc_kind ());
	    }

	  worklist->emplace_back (field1->type (), field2->type ());
	}
    }

  if (type1->target_type () != NULL)
    {
      if (type2->target_type () == NULL)
	return false;

      worklist->emplace_back (type1->target_type (),
			      type2->target_type ());
    }
  else if (type2->target_type () != NULL)
    return false;

  return true;
}

/* Check types on a worklist for equality.  Returns false if any pair
   is not equal, true if they are all considered equal.  */

static bool
check_types_worklist (std::vector<type_equality_entry> *worklist,
		      gdb::bcache *cache)
{
  while (!worklist->empty ())
    {
      bool added;

      struct type_equality_entry entry = std::move (worklist->back ());
      worklist->pop_back ();

      /* If the type pair has already been visited, we know it is
	 ok.  */
      cache->insert (&entry, sizeof (entry), &added);
      if (!added)
	continue;

      if (!check_types_equal (entry.type1, entry.type2, worklist))
	return false;
    }

  return true;
}

/* Return true if types TYPE1 and TYPE2 are equal, as determined by a
   "deep comparison".  Otherwise return false.  */

bool
types_deeply_equal (struct type *type1, struct type *type2)
{
  std::vector<type_equality_entry> worklist;

  gdb_assert (type1 != NULL && type2 != NULL);

  /* Early exit for the simple case.  */
  if (type1 == type2)
    return true;

  gdb::bcache cache;
  worklist.emplace_back (type1, type2);
  return check_types_worklist (&worklist, &cache);
}

/* Allocated status of type TYPE.  Return zero if type TYPE is allocated.
   Otherwise return one.  */

int
type_not_allocated (const struct type *type)
{
  struct dynamic_prop *prop = TYPE_ALLOCATED_PROP (type);

  return prop != nullptr && prop->is_constant () && prop->const_val () == 0;
}

/* Associated status of type TYPE.  Return zero if type TYPE is associated.
   Otherwise return one.  */

int
type_not_associated (const struct type *type)
{
  struct dynamic_prop *prop = TYPE_ASSOCIATED_PROP (type);

  return prop != nullptr && prop->is_constant () && prop->const_val () == 0;
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_PTR.  */

static struct rank
rank_one_type_parm_ptr (struct type *parm, struct type *arg, struct value *value)
{
  struct rank rank = {0,0};

  switch (arg->code ())
    {
    case TYPE_CODE_PTR:

      /* Allowed pointer conversions are:
	 (a) pointer to void-pointer conversion.  */
      if (parm->target_type ()->code () == TYPE_CODE_VOID)
	return VOID_PTR_CONVERSION_BADNESS;

      /* (b) pointer to ancestor-pointer conversion.  */
      rank.subrank = distance_to_ancestor (parm->target_type (),
					   arg->target_type (),
					   0);
      if (rank.subrank >= 0)
	return sum_ranks (BASE_PTR_CONVERSION_BADNESS, rank);

      return INCOMPATIBLE_TYPE_BADNESS;
    case TYPE_CODE_ARRAY:
      {
	struct type *t1 = parm->target_type ();
	struct type *t2 = arg->target_type ();

	if (types_equal (t1, t2))
	  {
	    /* Make sure they are CV equal.  */
	    if (TYPE_CONST (t1) != TYPE_CONST (t2))
	      rank.subrank |= CV_CONVERSION_CONST;
	    if (TYPE_VOLATILE (t1) != TYPE_VOLATILE (t2))
	      rank.subrank |= CV_CONVERSION_VOLATILE;
	    if (rank.subrank != 0)
	      return sum_ranks (CV_CONVERSION_BADNESS, rank);
	    return EXACT_MATCH_BADNESS;
	  }
	return INCOMPATIBLE_TYPE_BADNESS;
      }
    case TYPE_CODE_FUNC:
      return rank_one_type (parm->target_type (), arg, NULL);
    case TYPE_CODE_INT:
      if (value != NULL && value->type ()->code () == TYPE_CODE_INT)
	{
	  if (value_as_long (value) == 0)
	    {
	      /* Null pointer conversion: allow it to be cast to a pointer.
		 [4.10.1 of C++ standard draft n3290]  */
	      return NULL_POINTER_CONVERSION_BADNESS;
	    }
	  else
	    {
	      /* If type checking is disabled, allow the conversion.  */
	      if (!strict_type_checking)
		return NS_INTEGER_POINTER_CONVERSION_BADNESS;
	    }
	}
      [[fallthrough]];
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_BOOL:
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_ARRAY.  */

static struct rank
rank_one_type_parm_array (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
      return rank_one_type (parm->target_type (),
			    arg->target_type (), NULL);
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_FUNC.  */

static struct rank
rank_one_type_parm_func (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_PTR:	/* funcptr -> func */
      return rank_one_type (parm, arg->target_type (), NULL);
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_INT.  */

static struct rank
rank_one_type_parm_int (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_INT:
      if (arg->length () == parm->length ())
	{
	  /* Deal with signed, unsigned, and plain chars and
	     signed and unsigned ints.  */
	  if (parm->has_no_signedness ())
	    {
	      /* This case only for character types.  */
	      if (arg->has_no_signedness ())
		return EXACT_MATCH_BADNESS;	/* plain char -> plain char */
	      else		/* signed/unsigned char -> plain char */
		return INTEGER_CONVERSION_BADNESS;
	    }
	  else if (parm->is_unsigned ())
	    {
	      if (arg->is_unsigned ())
		{
		  /* unsigned int -> unsigned int, or
		     unsigned long -> unsigned long */
		  if (integer_types_same_name_p (parm->name (),
						 arg->name ()))
		    return EXACT_MATCH_BADNESS;
		  else if (integer_types_same_name_p (arg->name (),
						      "int")
			   && integer_types_same_name_p (parm->name (),
							 "long"))
		    /* unsigned int -> unsigned long */
		    return INTEGER_PROMOTION_BADNESS;
		  else
		    /* unsigned long -> unsigned int */
		    return INTEGER_CONVERSION_BADNESS;
		}
	      else
		{
		  if (integer_types_same_name_p (arg->name (),
						 "long")
		      && integer_types_same_name_p (parm->name (),
						    "int"))
		    /* signed long -> unsigned int */
		    return INTEGER_CONVERSION_BADNESS;
		  else
		    /* signed int/long -> unsigned int/long */
		    return INTEGER_CONVERSION_BADNESS;
		}
	    }
	  else if (!arg->has_no_signedness () && !arg->is_unsigned ())
	    {
	      if (integer_types_same_name_p (parm->name (),
					     arg->name ()))
		return EXACT_MATCH_BADNESS;
	      else if (integer_types_same_name_p (arg->name (),
						  "int")
		       && integer_types_same_name_p (parm->name (),
						     "long"))
		return INTEGER_PROMOTION_BADNESS;
	      else
		return INTEGER_CONVERSION_BADNESS;
	    }
	  else
	    return INTEGER_CONVERSION_BADNESS;
	}
      else if (arg->length () < parm->length ())
	return INTEGER_PROMOTION_BADNESS;
      else
	return INTEGER_CONVERSION_BADNESS;
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_BOOL:
      if (arg->is_declared_class ())
	return INCOMPATIBLE_TYPE_BADNESS;
      return INTEGER_PROMOTION_BADNESS;
    case TYPE_CODE_FLT:
      return INT_FLOAT_CONVERSION_BADNESS;
    case TYPE_CODE_PTR:
      return NS_POINTER_CONVERSION_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_ENUM.  */

static struct rank
rank_one_type_parm_enum (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_ENUM:
      if (parm->is_declared_class () || arg->is_declared_class ())
	return INCOMPATIBLE_TYPE_BADNESS;
      return INTEGER_CONVERSION_BADNESS;
    case TYPE_CODE_FLT:
      return INT_FLOAT_CONVERSION_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_CHAR.  */

static struct rank
rank_one_type_parm_char (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_RANGE:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_ENUM:
      if (arg->is_declared_class ())
	return INCOMPATIBLE_TYPE_BADNESS;
      return INTEGER_CONVERSION_BADNESS;
    case TYPE_CODE_FLT:
      return INT_FLOAT_CONVERSION_BADNESS;
    case TYPE_CODE_INT:
      if (arg->length () > parm->length ())
	return INTEGER_CONVERSION_BADNESS;
      else if (arg->length () < parm->length ())
	return INTEGER_PROMOTION_BADNESS;
      [[fallthrough]];
    case TYPE_CODE_CHAR:
      /* Deal with signed, unsigned, and plain chars for C++ and
	 with int cases falling through from previous case.  */
      if (parm->has_no_signedness ())
	{
	  if (arg->has_no_signedness ())
	    return EXACT_MATCH_BADNESS;
	  else
	    return INTEGER_CONVERSION_BADNESS;
	}
      else if (parm->is_unsigned ())
	{
	  if (arg->is_unsigned ())
	    return EXACT_MATCH_BADNESS;
	  else
	    return INTEGER_PROMOTION_BADNESS;
	}
      else if (!arg->has_no_signedness () && !arg->is_unsigned ())
	return EXACT_MATCH_BADNESS;
      else
	return INTEGER_CONVERSION_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_RANGE.  */

static struct rank
rank_one_type_parm_range (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_ENUM:
      return INTEGER_CONVERSION_BADNESS;
    case TYPE_CODE_FLT:
      return INT_FLOAT_CONVERSION_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_BOOL.  */

static struct rank
rank_one_type_parm_bool (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
      /* n3290 draft, section 4.12.1 (conv.bool):

	 "A prvalue of arithmetic, unscoped enumeration, pointer, or
	 pointer to member type can be converted to a prvalue of type
	 bool.  A zero value, null pointer value, or null member pointer
	 value is converted to false; any other value is converted to
	 true.  A prvalue of type std::nullptr_t can be converted to a
	 prvalue of type bool; the resulting value is false."  */
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLT:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_PTR:
      return BOOL_CONVERSION_BADNESS;
    case TYPE_CODE_RANGE:
      return INCOMPATIBLE_TYPE_BADNESS;
    case TYPE_CODE_BOOL:
      return EXACT_MATCH_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_FLOAT.  */

static struct rank
rank_one_type_parm_float (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
    case TYPE_CODE_FLT:
      if (arg->length () < parm->length ())
	return FLOAT_PROMOTION_BADNESS;
      else if (arg->length () == parm->length ())
	return EXACT_MATCH_BADNESS;
      else
	return FLOAT_CONVERSION_BADNESS;
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_CHAR:
      return INT_FLOAT_CONVERSION_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_COMPLEX.  */

static struct rank
rank_one_type_parm_complex (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {		/* Strictly not needed for C++, but...  */
    case TYPE_CODE_FLT:
      return FLOAT_PROMOTION_BADNESS;
    case TYPE_CODE_COMPLEX:
      return EXACT_MATCH_BADNESS;
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_STRUCT.  */

static struct rank
rank_one_type_parm_struct (struct type *parm, struct type *arg, struct value *value)
{
  struct rank rank = {0, 0};

  switch (arg->code ())
    {
    case TYPE_CODE_STRUCT:
      /* Check for derivation */
      rank.subrank = distance_to_ancestor (parm, arg, 0);
      if (rank.subrank >= 0)
	return sum_ranks (BASE_CONVERSION_BADNESS, rank);
      [[fallthrough]];
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* rank_one_type helper for when PARM's type code is TYPE_CODE_SET.  */

static struct rank
rank_one_type_parm_set (struct type *parm, struct type *arg, struct value *value)
{
  switch (arg->code ())
    {
      /* Not in C++ */
    case TYPE_CODE_SET:
      return rank_one_type (parm->field (0).type (),
			    arg->field (0).type (), NULL);
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }
}

/* Compare one type (PARM) for compatibility with another (ARG).
 * PARM is intended to be the parameter type of a function; and
 * ARG is the supplied argument's type.  This function tests if
 * the latter can be converted to the former.
 * VALUE is the argument's value or NULL if none (or called recursively)
 *
 * Return 0 if they are identical types;
 * Otherwise, return an integer which corresponds to how compatible
 * PARM is to ARG.  The higher the return value, the worse the match.
 * Generally the "bad" conversions are all uniformly assigned
 * INVALID_CONVERSION.  */

struct rank
rank_one_type (struct type *parm, struct type *arg, struct value *value)
{
  struct rank rank = {0,0};

  /* Resolve typedefs */
  if (parm->code () == TYPE_CODE_TYPEDEF)
    parm = check_typedef (parm);
  if (arg->code () == TYPE_CODE_TYPEDEF)
    arg = check_typedef (arg);

  if (TYPE_IS_REFERENCE (parm) && value != NULL)
    {
      if (value->lval () == not_lval)
	{
	  /* Rvalues should preferably bind to rvalue references or const
	     lvalue references.  */
	  if (parm->code () == TYPE_CODE_RVALUE_REF)
	    rank.subrank = REFERENCE_CONVERSION_RVALUE;
	  else if (TYPE_CONST (parm->target_type ()))
	    rank.subrank = REFERENCE_CONVERSION_CONST_LVALUE;
	  else
	    return INCOMPATIBLE_TYPE_BADNESS;
	  return sum_ranks (rank, REFERENCE_CONVERSION_BADNESS);
	}
      else
	{
	  /* It's illegal to pass an lvalue as an rvalue.  */
	  if (parm->code () == TYPE_CODE_RVALUE_REF)
	    return INCOMPATIBLE_TYPE_BADNESS;
	}
    }

  if (types_equal (parm, arg))
    {
      struct type *t1 = parm;
      struct type *t2 = arg;

      /* For pointers and references, compare target type.  */
      if (parm->is_pointer_or_reference ())
	{
	  t1 = parm->target_type ();
	  t2 = arg->target_type ();
	}

      /* Make sure they are CV equal, too.  */
      if (TYPE_CONST (t1) != TYPE_CONST (t2))
	rank.subrank |= CV_CONVERSION_CONST;
      if (TYPE_VOLATILE (t1) != TYPE_VOLATILE (t2))
	rank.subrank |= CV_CONVERSION_VOLATILE;
      if (rank.subrank != 0)
	return sum_ranks (CV_CONVERSION_BADNESS, rank);
      return EXACT_MATCH_BADNESS;
    }

  /* See through references, since we can almost make non-references
     references.  */

  if (TYPE_IS_REFERENCE (arg))
    return (sum_ranks (rank_one_type (parm, arg->target_type (), NULL),
		       REFERENCE_SEE_THROUGH_BADNESS));
  if (TYPE_IS_REFERENCE (parm))
    return (sum_ranks (rank_one_type (parm->target_type (), arg, NULL),
		       REFERENCE_SEE_THROUGH_BADNESS));
  if (overload_debug)
    {
      /* Debugging only.  */
      gdb_printf (gdb_stderr,
		  "------ Arg is %s [%d], parm is %s [%d]\n",
		  arg->name (), arg->code (),
		  parm->name (), parm->code ());
    }

  /* x -> y means arg of type x being supplied for parameter of type y.  */

  switch (parm->code ())
    {
    case TYPE_CODE_PTR:
      return rank_one_type_parm_ptr (parm, arg, value);
    case TYPE_CODE_ARRAY:
      return rank_one_type_parm_array (parm, arg, value);
    case TYPE_CODE_FUNC:
      return rank_one_type_parm_func (parm, arg, value);
    case TYPE_CODE_INT:
      return rank_one_type_parm_int (parm, arg, value);
    case TYPE_CODE_ENUM:
      return rank_one_type_parm_enum (parm, arg, value);
    case TYPE_CODE_CHAR:
      return rank_one_type_parm_char (parm, arg, value);
    case TYPE_CODE_RANGE:
      return rank_one_type_parm_range (parm, arg, value);
    case TYPE_CODE_BOOL:
      return rank_one_type_parm_bool (parm, arg, value);
    case TYPE_CODE_FLT:
      return rank_one_type_parm_float (parm, arg, value);
    case TYPE_CODE_COMPLEX:
      return rank_one_type_parm_complex (parm, arg, value);
    case TYPE_CODE_STRUCT:
      return rank_one_type_parm_struct (parm, arg, value);
    case TYPE_CODE_SET:
      return rank_one_type_parm_set (parm, arg, value);
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }				/* switch (arg->code ()) */
}

/* End of functions for overload resolution.  */


/* Note the first arg should be the "this" pointer, we may not want to
   include it since we may get into a infinitely recursive
   situation.  */

static void
print_args (struct field *args, int nargs, int spaces)
{
  if (args != NULL)
    {
      int i;

      for (i = 0; i < nargs; i++)
	{
	  gdb_printf
	    ("%*s[%d] name '%s'\n", spaces, "", i,
	     args[i].name () != NULL ? args[i].name () : "<NULL>");
	  recursive_dump_type (args[i].type (), spaces + 2);
	}
    }
}

static void
dump_fn_fieldlists (struct type *type, int spaces)
{
  int method_idx;
  int overload_idx;
  struct fn_field *f;

  gdb_printf ("%*sfn_fieldlists %s\n", spaces, "",
	      host_address_to_string (TYPE_FN_FIELDLISTS (type)));
  for (method_idx = 0; method_idx < TYPE_NFN_FIELDS (type); method_idx++)
    {
      f = TYPE_FN_FIELDLIST1 (type, method_idx);
      gdb_printf
	("%*s[%d] name '%s' (%s) length %d\n", spaces + 2, "",
	 method_idx,
	 TYPE_FN_FIELDLIST_NAME (type, method_idx),
	 host_address_to_string (TYPE_FN_FIELDLIST_NAME (type, method_idx)),
	 TYPE_FN_FIELDLIST_LENGTH (type, method_idx));
      for (overload_idx = 0;
	   overload_idx < TYPE_FN_FIELDLIST_LENGTH (type, method_idx);
	   overload_idx++)
	{
	  gdb_printf
	    ("%*s[%d] physname '%s' (%s)\n",
	     spaces + 4, "", overload_idx,
	     TYPE_FN_FIELD_PHYSNAME (f, overload_idx),
	     host_address_to_string (TYPE_FN_FIELD_PHYSNAME (f,
							     overload_idx)));
	  gdb_printf
	    ("%*stype %s\n", spaces + 8, "",
	     host_address_to_string (TYPE_FN_FIELD_TYPE (f, overload_idx)));

	  recursive_dump_type (TYPE_FN_FIELD_TYPE (f, overload_idx),
			       spaces + 8 + 2);

	  gdb_printf
	    ("%*sargs %s\n", spaces + 8, "",
	     host_address_to_string (TYPE_FN_FIELD_ARGS (f, overload_idx)));
	  print_args (TYPE_FN_FIELD_ARGS (f, overload_idx),
		      TYPE_FN_FIELD_TYPE (f, overload_idx)->num_fields (),
		      spaces + 8 + 2);
	  gdb_printf
	    ("%*sfcontext %s\n", spaces + 8, "",
	     host_address_to_string (TYPE_FN_FIELD_FCONTEXT (f,
							     overload_idx)));

	  gdb_printf ("%*sis_const %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_CONST (f, overload_idx));
	  gdb_printf ("%*sis_volatile %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_VOLATILE (f, overload_idx));
	  gdb_printf ("%*sis_private %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_PRIVATE (f, overload_idx));
	  gdb_printf ("%*sis_protected %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_PROTECTED (f, overload_idx));
	  gdb_printf ("%*sis_stub %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_STUB (f, overload_idx));
	  gdb_printf ("%*sdefaulted %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_DEFAULTED (f, overload_idx));
	  gdb_printf ("%*sis_deleted %d\n", spaces + 8, "",
		      TYPE_FN_FIELD_DELETED (f, overload_idx));
	  gdb_printf ("%*svoffset %u\n", spaces + 8, "",
		      TYPE_FN_FIELD_VOFFSET (f, overload_idx));
	}
    }
}

static void
print_cplus_stuff (struct type *type, int spaces)
{
  gdb_printf ("%*svptr_fieldno %d\n", spaces, "",
	      TYPE_VPTR_FIELDNO (type));
  gdb_printf ("%*svptr_basetype %s\n", spaces, "",
	      host_address_to_string (TYPE_VPTR_BASETYPE (type)));
  if (TYPE_VPTR_BASETYPE (type) != NULL)
    recursive_dump_type (TYPE_VPTR_BASETYPE (type), spaces + 2);

  gdb_printf ("%*sn_baseclasses %d\n", spaces, "",
	      TYPE_N_BASECLASSES (type));
  gdb_printf ("%*snfn_fields %d\n", spaces, "",
	      TYPE_NFN_FIELDS (type));
  if (TYPE_NFN_FIELDS (type) > 0)
    {
      dump_fn_fieldlists (type, spaces);
    }

  gdb_printf ("%*scalling_convention %d\n", spaces, "",
	      TYPE_CPLUS_CALLING_CONVENTION (type));
}

/* Print the contents of the TYPE's type_specific union, assuming that
   its type-specific kind is TYPE_SPECIFIC_GNAT_STUFF.  */

static void
print_gnat_stuff (struct type *type, int spaces)
{
  struct type *descriptive_type = TYPE_DESCRIPTIVE_TYPE (type);

  if (descriptive_type == NULL)
    gdb_printf ("%*sno descriptive type\n", spaces + 2, "");
  else
    {
      gdb_printf ("%*sdescriptive type\n", spaces + 2, "");
      recursive_dump_type (descriptive_type, spaces + 4);
    }
}

/* Print the contents of the TYPE's type_specific union, assuming that
   its type-specific kind is TYPE_SPECIFIC_FIXED_POINT.  */

static void
print_fixed_point_type_info (struct type *type, int spaces)
{
  gdb_printf ("%*sscaling factor: %s\n", spaces + 2, "",
	      type->fixed_point_scaling_factor ().str ().c_str ());
}

static struct obstack dont_print_type_obstack;

/* Print the dynamic_prop PROP.  */

static void
dump_dynamic_prop (dynamic_prop const& prop)
{
  switch (prop.kind ())
    {
    case PROP_CONST:
      gdb_printf ("%s", plongest (prop.const_val ()));
      break;
    case PROP_UNDEFINED:
      gdb_printf ("(undefined)");
      break;
    case PROP_LOCEXPR:
    case PROP_LOCLIST:
      gdb_printf ("(dynamic)");
      break;
    default:
      gdb_assert_not_reached ("unhandled prop kind");
      break;
    }
}

/* Return a string that represents a type code.  */
static const char *
type_code_name (type_code code)
{
  switch (code)
    {
#define OP(X) case X: return # X;
#include "type-codes.def"
#undef OP

    case TYPE_CODE_UNDEF:
      return "TYPE_CODE_UNDEF";
    }

  gdb_assert_not_reached ("unhandled type_code");
}

void
recursive_dump_type (struct type *type, int spaces)
{
  int idx;

  if (spaces == 0)
    obstack_begin (&dont_print_type_obstack, 0);

  if (type->num_fields () > 0
      || (HAVE_CPLUS_STRUCT (type) && TYPE_NFN_FIELDS (type) > 0))
    {
      struct type **first_dont_print
	= (struct type **) obstack_base (&dont_print_type_obstack);

      int i = (struct type **) 
	obstack_next_free (&dont_print_type_obstack) - first_dont_print;

      while (--i >= 0)
	{
	  if (type == first_dont_print[i])
	    {
	      gdb_printf ("%*stype node %s", spaces, "",
			  host_address_to_string (type));
	      gdb_printf (_(" <same as already seen type>\n"));
	      return;
	    }
	}

      obstack_ptr_grow (&dont_print_type_obstack, type);
    }

  gdb_printf ("%*stype node %s\n", spaces, "",
	      host_address_to_string (type));
  gdb_printf ("%*sname '%s' (%s)\n", spaces, "",
	      type->name () ? type->name () : "<NULL>",
	      host_address_to_string (type->name ()));
  gdb_printf ("%*scode 0x%x ", spaces, "", type->code ());
  gdb_printf ("(%s)", type_code_name (type->code ()));
  gdb_puts ("\n");
  gdb_printf ("%*slength %s\n", spaces, "",
	      pulongest (type->length ()));
  if (type->is_objfile_owned ())
    gdb_printf ("%*sobjfile %s\n", spaces, "",
		host_address_to_string (type->objfile_owner ()));
  else
    gdb_printf ("%*sgdbarch %s\n", spaces, "",
		host_address_to_string (type->arch_owner ()));
  gdb_printf ("%*starget_type %s\n", spaces, "",
	      host_address_to_string (type->target_type ()));
  if (type->target_type () != NULL)
    {
      recursive_dump_type (type->target_type (), spaces + 2);
    }
  gdb_printf ("%*spointer_type %s\n", spaces, "",
	      host_address_to_string (TYPE_POINTER_TYPE (type)));
  gdb_printf ("%*sreference_type %s\n", spaces, "",
	      host_address_to_string (TYPE_REFERENCE_TYPE (type)));
  gdb_printf ("%*stype_chain %s\n", spaces, "",
	      host_address_to_string (TYPE_CHAIN (type)));
  gdb_printf ("%*sinstance_flags 0x%x", spaces, "", 
	      (unsigned) type->instance_flags ());
  if (TYPE_CONST (type))
    {
      gdb_puts (" TYPE_CONST");
    }
  if (TYPE_VOLATILE (type))
    {
      gdb_puts (" TYPE_VOLATILE");
    }
  if (TYPE_CODE_SPACE (type))
    {
      gdb_puts (" TYPE_CODE_SPACE");
    }
  if (TYPE_DATA_SPACE (type))
    {
      gdb_puts (" TYPE_DATA_SPACE");
    }
  if (TYPE_ADDRESS_CLASS_1 (type))
    {
      gdb_puts (" TYPE_ADDRESS_CLASS_1");
    }
  if (TYPE_ADDRESS_CLASS_2 (type))
    {
      gdb_puts (" TYPE_ADDRESS_CLASS_2");
    }
  if (TYPE_RESTRICT (type))
    {
      gdb_puts (" TYPE_RESTRICT");
    }
  if (TYPE_ATOMIC (type))
    {
      gdb_puts (" TYPE_ATOMIC");
    }
  gdb_puts ("\n");

  gdb_printf ("%*sflags", spaces, "");
  if (type->is_unsigned ())
    {
      gdb_puts (" TYPE_UNSIGNED");
    }
  if (type->has_no_signedness ())
    {
      gdb_puts (" TYPE_NOSIGN");
    }
  if (type->endianity_is_not_default ())
    {
      gdb_puts (" TYPE_ENDIANITY_NOT_DEFAULT");
    }
  if (type->is_stub ())
    {
      gdb_puts (" TYPE_STUB");
    }
  if (type->target_is_stub ())
    {
      gdb_puts (" TYPE_TARGET_STUB");
    }
  if (type->is_prototyped ())
    {
      gdb_puts (" TYPE_PROTOTYPED");
    }
  if (type->has_varargs ())
    {
      gdb_puts (" TYPE_VARARGS");
    }
  /* This is used for things like AltiVec registers on ppc.  Gcc emits
     an attribute for the array type, which tells whether or not we
     have a vector, instead of a regular array.  */
  if (type->is_vector ())
    {
      gdb_puts (" TYPE_VECTOR");
    }
  if (type->is_fixed_instance ())
    {
      gdb_puts (" TYPE_FIXED_INSTANCE");
    }
  if (type->stub_is_supported ())
    {
      gdb_puts (" TYPE_STUB_SUPPORTED");
    }
  if (TYPE_NOTTEXT (type))
    {
      gdb_puts (" TYPE_NOTTEXT");
    }
  gdb_puts ("\n");
  gdb_printf ("%*snfields %d ", spaces, "", type->num_fields ());
  if (TYPE_ASSOCIATED_PROP (type) != nullptr
      || TYPE_ALLOCATED_PROP (type) != nullptr)
    {
      gdb_printf ("%*s", spaces, "");
      if (TYPE_ASSOCIATED_PROP (type) != nullptr)
	{
	  gdb_printf ("associated ");
	  dump_dynamic_prop (*TYPE_ASSOCIATED_PROP (type));
	}
      if (TYPE_ALLOCATED_PROP (type) != nullptr)
	{
	  if (TYPE_ASSOCIATED_PROP (type) != nullptr)
	    gdb_printf ("  ");
	  gdb_printf ("allocated ");
	  dump_dynamic_prop (*TYPE_ALLOCATED_PROP (type));
	}
      gdb_printf ("\n");
    }
  gdb_printf ("%s\n", host_address_to_string (type->fields ()));
  for (idx = 0; idx < type->num_fields (); idx++)
    {
      field &fld = type->field (idx);
      if (type->code () == TYPE_CODE_ENUM)
	gdb_printf ("%*s[%d] enumval %s type ", spaces + 2, "",
		    idx, plongest (fld.loc_enumval ()));
      else
	gdb_printf ("%*s[%d] bitpos %s bitsize %d type ", spaces + 2, "",
		    idx, plongest (fld.loc_bitpos ()),
		    fld.bitsize ());
      gdb_printf ("%s name '%s' (%s)",
		  host_address_to_string (fld.type ()),
		  fld.name () != NULL
		  ? fld.name ()
		  : "<NULL>",
		  host_address_to_string (fld.name ()));
      if (fld.is_virtual ())
	gdb_printf (" virtual");

      if (fld.is_private ())
	gdb_printf (" private");
      else if (fld.is_protected ())
	gdb_printf (" protected");
      else if (fld.is_ignored ())
	gdb_printf (" ignored");

      gdb_printf ("\n");
      if (fld.type () != NULL)
	{
	  recursive_dump_type (fld.type (), spaces + 4);
	}
    }
  if (type->code () == TYPE_CODE_RANGE)
    {
      gdb_printf ("%*slow ", spaces, "");
      dump_dynamic_prop (type->bounds ()->low);
      gdb_printf ("  high ");
      dump_dynamic_prop (type->bounds ()->high);
      gdb_printf ("\n");
    }

  switch (TYPE_SPECIFIC_FIELD (type))
    {
    case TYPE_SPECIFIC_CPLUS_STUFF:
      gdb_printf ("%*scplus_stuff %s\n", spaces, "",
		  host_address_to_string (TYPE_CPLUS_SPECIFIC (type)));
      print_cplus_stuff (type, spaces);
      break;

    case TYPE_SPECIFIC_GNAT_STUFF:
      gdb_printf ("%*sgnat_stuff %s\n", spaces, "",
		  host_address_to_string (TYPE_GNAT_SPECIFIC (type)));
      print_gnat_stuff (type, spaces);
      break;

    case TYPE_SPECIFIC_FLOATFORMAT:
      gdb_printf ("%*sfloatformat ", spaces, "");
      if (TYPE_FLOATFORMAT (type) == NULL
	  || TYPE_FLOATFORMAT (type)->name == NULL)
	gdb_puts ("(null)");
      else
	gdb_puts (TYPE_FLOATFORMAT (type)->name);
      gdb_puts ("\n");
      break;

    case TYPE_SPECIFIC_FUNC:
      gdb_printf ("%*scalling_convention %d\n", spaces, "",
		  TYPE_CALLING_CONVENTION (type));
      /* tail_call_list is not printed.  */
      break;

    case TYPE_SPECIFIC_SELF_TYPE:
      gdb_printf ("%*sself_type %s\n", spaces, "",
		  host_address_to_string (TYPE_SELF_TYPE (type)));
      break;

    case TYPE_SPECIFIC_FIXED_POINT:
      gdb_printf ("%*sfixed_point_info ", spaces, "");
      print_fixed_point_type_info (type, spaces);
      gdb_puts ("\n");
      break;

    case TYPE_SPECIFIC_INT:
      if (type->bit_size_differs_p ())
	{
	  unsigned bit_size = type->bit_size ();
	  unsigned bit_off = type->bit_offset ();
	  gdb_printf ("%*s bit size = %u, bit offset = %u\n", spaces, "",
		      bit_size, bit_off);
	}
      break;
    }

  if (spaces == 0)
    obstack_free (&dont_print_type_obstack, NULL);
}

/* Trivial helpers for the libiberty hash table, for mapping one
   type to another.  */

struct type_pair
{
  type_pair (struct type *old_, struct type *newobj_)
    : old (old_), newobj (newobj_)
  {}

  struct type * const old, * const newobj;
};

static hashval_t
type_pair_hash (const void *item)
{
  const struct type_pair *pair = (const struct type_pair *) item;

  return htab_hash_pointer (pair->old);
}

static int
type_pair_eq (const void *item_lhs, const void *item_rhs)
{
  const struct type_pair *lhs = (const struct type_pair *) item_lhs;
  const struct type_pair *rhs = (const struct type_pair *) item_rhs;

  return lhs->old == rhs->old;
}

/* Allocate the hash table used by copy_type_recursive to walk
   types without duplicates.  */

htab_up
create_copied_types_hash ()
{
  return htab_up (htab_create_alloc (1, type_pair_hash, type_pair_eq,
				     htab_delete_entry<type_pair>,
				     xcalloc, xfree));
}

/* Recursively copy (deep copy) a dynamic attribute list of a type.  */

static struct dynamic_prop_list *
copy_dynamic_prop_list (struct obstack *storage,
			struct dynamic_prop_list *list)
{
  struct dynamic_prop_list *copy = list;
  struct dynamic_prop_list **node_ptr = &copy;

  while (*node_ptr != NULL)
    {
      struct dynamic_prop_list *node_copy;

      node_copy = ((struct dynamic_prop_list *)
		   obstack_copy (storage, *node_ptr,
				 sizeof (struct dynamic_prop_list)));
      node_copy->prop = (*node_ptr)->prop;
      *node_ptr = node_copy;

      node_ptr = &node_copy->next;
    }

  return copy;
}

/* Recursively copy (deep copy) TYPE, if it is associated with
   OBJFILE.  Return a new type owned by the gdbarch associated with the type, a
   saved type if we have already visited TYPE (using COPIED_TYPES), or TYPE if
   it is not associated with OBJFILE.  */

struct type *
copy_type_recursive (struct type *type, htab_t copied_types)
{
  void **slot;
  struct type *new_type;

  if (!type->is_objfile_owned ())
    return type;

  struct type_pair pair (type, nullptr);

  slot = htab_find_slot (copied_types, &pair, INSERT);
  if (*slot != NULL)
    return ((struct type_pair *) *slot)->newobj;

  new_type = type_allocator (type->arch ()).new_type ();

  /* We must add the new type to the hash table immediately, in case
     we encounter this type again during a recursive call below.  */
  struct type_pair *stored = new type_pair (type, new_type);

  *slot = stored;

  /* Copy the common fields of types.  For the main type, we simply
     copy the entire thing and then update specific fields as needed.  */
  *TYPE_MAIN_TYPE (new_type) = *TYPE_MAIN_TYPE (type);

  new_type->set_owner (type->arch ());

  if (type->name ())
    new_type->set_name (xstrdup (type->name ()));

  new_type->set_instance_flags (type->instance_flags ());
  new_type->set_length (type->length ());

  /* Copy the fields.  */
  if (type->num_fields ())
    {
      int i, nfields;

      nfields = type->num_fields ();
      new_type->alloc_fields (type->num_fields ());

      for (i = 0; i < nfields; i++)
	{
	  new_type->field (i).set_is_artificial
	    (type->field (i).is_artificial ());
	  new_type->field (i).set_bitsize (type->field (i).bitsize ());
	  if (type->field (i).type ())
	    new_type->field (i).set_type
	      (copy_type_recursive (type->field (i).type (), copied_types));
	  if (type->field (i).name ())
	    new_type->field (i).set_name (xstrdup (type->field (i).name ()));

	  switch (type->field (i).loc_kind ())
	    {
	    case FIELD_LOC_KIND_BITPOS:
	      new_type->field (i).set_loc_bitpos (type->field (i).loc_bitpos ());
	      break;
	    case FIELD_LOC_KIND_ENUMVAL:
	      new_type->field (i).set_loc_enumval (type->field (i).loc_enumval ());
	      break;
	    case FIELD_LOC_KIND_PHYSADDR:
	      new_type->field (i).set_loc_physaddr
		(type->field (i).loc_physaddr ());
	      break;
	    case FIELD_LOC_KIND_PHYSNAME:
	      new_type->field (i).set_loc_physname
		(xstrdup (type->field (i).loc_physname ()));
	      break;
	    case FIELD_LOC_KIND_DWARF_BLOCK:
	      new_type->field (i).set_loc_dwarf_block
		(type->field (i).loc_dwarf_block ());
	      break;
	    default:
	      internal_error (_("Unexpected type field location kind: %d"),
			      type->field (i).loc_kind ());
	    }
	}
    }

  /* For range types, copy the bounds information.  */
  if (type->code () == TYPE_CODE_RANGE)
    {
      range_bounds *bounds
	= ((struct range_bounds *) TYPE_ALLOC
	   (new_type, sizeof (struct range_bounds)));

      *bounds = *type->bounds ();
      new_type->set_bounds (bounds);
    }

  if (type->main_type->dyn_prop_list != NULL)
    new_type->main_type->dyn_prop_list
      = copy_dynamic_prop_list (gdbarch_obstack (new_type->arch_owner ()),
				type->main_type->dyn_prop_list);


  /* Copy pointers to other types.  */
  if (type->target_type ())
    new_type->set_target_type
      (copy_type_recursive (type->target_type (), copied_types));

  /* Maybe copy the type_specific bits.

     NOTE drow/2005-12-09: We do not copy the C++-specific bits like
     base classes and methods.  There's no fundamental reason why we
     can't, but at the moment it is not needed.  */

  switch (TYPE_SPECIFIC_FIELD (type))
    {
    case TYPE_SPECIFIC_NONE:
      break;
    case TYPE_SPECIFIC_FUNC:
      INIT_FUNC_SPECIFIC (new_type);
      TYPE_CALLING_CONVENTION (new_type) = TYPE_CALLING_CONVENTION (type);
      TYPE_NO_RETURN (new_type) = TYPE_NO_RETURN (type);
      TYPE_TAIL_CALL_LIST (new_type) = NULL;
      break;
    case TYPE_SPECIFIC_FLOATFORMAT:
      TYPE_FLOATFORMAT (new_type) = TYPE_FLOATFORMAT (type);
      break;
    case TYPE_SPECIFIC_CPLUS_STUFF:
      INIT_CPLUS_SPECIFIC (new_type);
      break;
    case TYPE_SPECIFIC_GNAT_STUFF:
      INIT_GNAT_SPECIFIC (new_type);
      break;
    case TYPE_SPECIFIC_SELF_TYPE:
      set_type_self_type (new_type,
			  copy_type_recursive (TYPE_SELF_TYPE (type),
					       copied_types));
      break;
    case TYPE_SPECIFIC_FIXED_POINT:
      INIT_FIXED_POINT_SPECIFIC (new_type);
      new_type->fixed_point_info ().scaling_factor
	= type->fixed_point_info ().scaling_factor;
      break;
    case TYPE_SPECIFIC_INT:
      TYPE_SPECIFIC_FIELD (new_type) = TYPE_SPECIFIC_INT;
      TYPE_MAIN_TYPE (new_type)->type_specific.int_stuff
	= TYPE_MAIN_TYPE (type)->type_specific.int_stuff;
      break;

    default:
      gdb_assert_not_reached ("bad type_specific_kind");
    }

  return new_type;
}

/* Make a copy of the given TYPE, except that the pointer & reference
   types are not preserved.  */

struct type *
copy_type (const struct type *type)
{
  struct type *new_type = type_allocator (type).new_type ();
  new_type->set_instance_flags (type->instance_flags ());
  new_type->set_length (type->length ());
  memcpy (TYPE_MAIN_TYPE (new_type), TYPE_MAIN_TYPE (type),
	  sizeof (struct main_type));
  if (type->main_type->dyn_prop_list != NULL)
    {
      struct obstack *storage = (type->is_objfile_owned ()
				 ? &type->objfile_owner ()->objfile_obstack
				 : gdbarch_obstack (type->arch_owner ()));
      new_type->main_type->dyn_prop_list
	= copy_dynamic_prop_list (storage, type->main_type->dyn_prop_list);
    }

  return new_type;
}

/* Helper functions to initialize architecture-specific types.  */

/* Allocate a TYPE_CODE_FLAGS type structure associated with GDBARCH.
   NAME is the type name.  BIT is the size of the flag word in bits.  */

struct type *
arch_flags_type (struct gdbarch *gdbarch, const char *name, int bit)
{
  struct type *type;

  type = type_allocator (gdbarch).new_type (TYPE_CODE_FLAGS, bit, name);
  type->set_is_unsigned (true);
  /* Pre-allocate enough space assuming every field is one bit.  */
  type->alloc_fields (bit);
  type->set_num_fields (0);

  return type;
}

/* Add field to TYPE_CODE_FLAGS type TYPE to indicate the bit at
   position BITPOS is called NAME.  Pass NAME as "" for fields that
   should not be printed.  */

void
append_flags_type_field (struct type *type, int start_bitpos, int nr_bits,
			 struct type *field_type, const char *name)
{
  int type_bitsize = type->length () * TARGET_CHAR_BIT;
  int field_nr = type->num_fields ();

  gdb_assert (type->code () == TYPE_CODE_FLAGS);
  gdb_assert (type->num_fields () + 1 <= type_bitsize);
  gdb_assert (start_bitpos >= 0 && start_bitpos < type_bitsize);
  gdb_assert (nr_bits >= 1 && (start_bitpos + nr_bits) <= type_bitsize);
  gdb_assert (name != NULL);

  type->set_num_fields (type->num_fields () + 1);
  type->field (field_nr).set_name (xstrdup (name));
  type->field (field_nr).set_type (field_type);
  type->field (field_nr).set_loc_bitpos (start_bitpos);
  type->field (field_nr).set_bitsize (nr_bits);
}

/* Special version of append_flags_type_field to add a flag field.
   Add field to TYPE_CODE_FLAGS type TYPE to indicate the bit at
   position BITPOS is called NAME.  */

void
append_flags_type_flag (struct type *type, int bitpos, const char *name)
{
  append_flags_type_field (type, bitpos, 1,
			   builtin_type (type->arch ())->builtin_bool,
			   name);
}

/* Allocate a TYPE_CODE_STRUCT or TYPE_CODE_UNION type structure (as
   specified by CODE) associated with GDBARCH.  NAME is the type name.  */

struct type *
arch_composite_type (struct gdbarch *gdbarch, const char *name,
		     enum type_code code)
{
  struct type *t;

  gdb_assert (code == TYPE_CODE_STRUCT || code == TYPE_CODE_UNION);
  t = type_allocator (gdbarch).new_type (code, 0, NULL);
  t->set_name (name);
  INIT_CPLUS_SPECIFIC (t);
  return t;
}

/* Add new field with name NAME and type FIELD to composite type T.
   Do not set the field's position or adjust the type's length;
   the caller should do so.  Return the new field.  */

struct field *
append_composite_type_field_raw (struct type *t, const char *name,
				 struct type *field)
{
  struct field *f;

  t->set_num_fields (t->num_fields () + 1);
  t->set_fields (XRESIZEVEC (struct field, t->fields (),
			     t->num_fields ()));
  f = &t->field (t->num_fields () - 1);
  memset (f, 0, sizeof f[0]);
  f[0].set_type (field);
  f[0].set_name (name);
  return f;
}

/* Add new field with name NAME and type FIELD to composite type T.
   ALIGNMENT (if non-zero) specifies the minimum field alignment.  */

void
append_composite_type_field_aligned (struct type *t, const char *name,
				     struct type *field, int alignment)
{
  struct field *f = append_composite_type_field_raw (t, name, field);

  if (t->code () == TYPE_CODE_UNION)
    {
      if (t->length () < field->length ())
	t->set_length (field->length ());
    }
  else if (t->code () == TYPE_CODE_STRUCT)
    {
      t->set_length (t->length () + field->length ());
      if (t->num_fields () > 1)
	{
	  f->set_loc_bitpos
	    (f[-1].loc_bitpos ()
	     + (f[-1].type ()->length () * TARGET_CHAR_BIT));

	  if (alignment)
	    {
	      int left;

	      alignment *= TARGET_CHAR_BIT;
	      left = f[0].loc_bitpos () % alignment;

	      if (left)
		{
		  f->set_loc_bitpos (f[0].loc_bitpos () + (alignment - left));
		  t->set_length
		    (t->length () + (alignment - left) / TARGET_CHAR_BIT);
		}
	    }
	}
    }
}

/* Add new field with name NAME and type FIELD to composite type T.  */

void
append_composite_type_field (struct type *t, const char *name,
			     struct type *field)
{
  append_composite_type_field_aligned (t, name, field, 0);
}



/* We manage the lifetimes of fixed_point_type_info objects by
   attaching them to the objfile.  Currently, these objects are
   modified during construction, and GMP does not provide a way to
   hash the contents of an mpq_t; so it's a bit of a pain to hash-cons
   them.  If we did do this, they could be moved to the per-BFD and
   shared across objfiles.  */
typedef std::vector<std::unique_ptr<fixed_point_type_info>>
    fixed_point_type_storage;

/* Key used for managing the storage of fixed-point type info.  */
static const struct registry<objfile>::key<fixed_point_type_storage>
    fixed_point_objfile_key;

/* See gdbtypes.h.  */

void
allocate_fixed_point_type_info (struct type *type)
{
  auto up = std::make_unique<fixed_point_type_info> ();
  fixed_point_type_info *info;

  if (type->is_objfile_owned ())
    {
      fixed_point_type_storage *storage
	= fixed_point_objfile_key.get (type->objfile_owner ());
      if (storage == nullptr)
	storage = fixed_point_objfile_key.emplace (type->objfile_owner ());
      info = up.get ();
      storage->push_back (std::move (up));
    }
  else
    {
      /* We just leak the memory, because that's what we do generally
	 for non-objfile-attached types.  */
      info = up.release ();
    }

  type->set_fixed_point_info (info);
}

/* See gdbtypes.h.  */

bool
is_fixed_point_type (struct type *type)
{
  while (check_typedef (type)->code () == TYPE_CODE_RANGE)
    type = check_typedef (type)->target_type ();
  type = check_typedef (type);

  return type->code () == TYPE_CODE_FIXED_POINT;
}

/* See gdbtypes.h.  */

struct type *
type::fixed_point_type_base_type ()
{
  struct type *type = this;

  while (check_typedef (type)->code () == TYPE_CODE_RANGE)
    type = check_typedef (type)->target_type ();
  type = check_typedef (type);

  gdb_assert (type->code () == TYPE_CODE_FIXED_POINT);
  return type;
}

/* See gdbtypes.h.  */

const gdb_mpq &
type::fixed_point_scaling_factor ()
{
  struct type *type = this->fixed_point_type_base_type ();

  return type->fixed_point_info ().scaling_factor;
}

/* See gdbtypes.h.  */

void
type::alloc_fields (unsigned int nfields, bool init)
{
  this->set_num_fields (nfields);

  if (nfields == 0)
    {
      this->main_type->flds_bnds.fields = nullptr;
      return;
    }

  size_t size = nfields * sizeof (*this->fields ());
  struct field *fields
    = (struct field *) (init
			? TYPE_ZALLOC (this, size)
			: TYPE_ALLOC (this, size));

  this->main_type->flds_bnds.fields = fields;
}

/* See gdbtypes.h.  */

void
type::copy_fields (struct type *src)
{
  unsigned int nfields = src->num_fields ();
  alloc_fields (nfields, false);
  if (nfields == 0)
    return;

  size_t size = nfields * sizeof (*this->fields ());
  memcpy (this->fields (), src->fields (), size);
}

/* See gdbtypes.h.  */

void
type::copy_fields (std::vector<struct field> &src)
{
  unsigned int nfields = src.size ();
  alloc_fields (nfields, false);
  if (nfields == 0)
    return;

  size_t size = nfields * sizeof (*this->fields ());
  memcpy (this->fields (), src.data (), size);
}

/* See gdbtypes.h.  */

bool
type::is_string_like ()
{
  const language_defn *defn = language_def (this->language ());
  return defn->is_string_type_p (this);
}

/* See gdbtypes.h.  */

bool
type::is_array_like ()
{
  if (code () == TYPE_CODE_ARRAY)
    return true;
  const language_defn *defn = language_def (this->language ());
  return defn->is_array_like (this);
}



static const registry<gdbarch>::key<struct builtin_type> gdbtypes_data;

static struct builtin_type *
create_gdbtypes_data (struct gdbarch *gdbarch)
{
  struct builtin_type *builtin_type = new struct builtin_type;

  type_allocator alloc (gdbarch);

  /* Basic types.  */
  builtin_type->builtin_void
    = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, "void");
  builtin_type->builtin_char
    = init_integer_type (alloc, TARGET_CHAR_BIT,
			 !gdbarch_char_signed (gdbarch), "char");
  builtin_type->builtin_char->set_has_no_signedness (true);
  builtin_type->builtin_signed_char
    = init_integer_type (alloc, TARGET_CHAR_BIT,
			 0, "signed char");
  builtin_type->builtin_unsigned_char
    = init_integer_type (alloc, TARGET_CHAR_BIT,
			 1, "unsigned char");
  builtin_type->builtin_short
    = init_integer_type (alloc, gdbarch_short_bit (gdbarch),
			 0, "short");
  builtin_type->builtin_unsigned_short
    = init_integer_type (alloc, gdbarch_short_bit (gdbarch),
			 1, "unsigned short");
  builtin_type->builtin_int
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch),
			 0, "int");
  builtin_type->builtin_unsigned_int
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch),
			 1, "unsigned int");
  builtin_type->builtin_long
    = init_integer_type (alloc, gdbarch_long_bit (gdbarch),
			 0, "long");
  builtin_type->builtin_unsigned_long
    = init_integer_type (alloc, gdbarch_long_bit (gdbarch),
			 1, "unsigned long");
  builtin_type->builtin_long_long
    = init_integer_type (alloc, gdbarch_long_long_bit (gdbarch),
			 0, "long long");
  builtin_type->builtin_unsigned_long_long
    = init_integer_type (alloc, gdbarch_long_long_bit (gdbarch),
			 1, "unsigned long long");
  builtin_type->builtin_half
    = init_float_type (alloc, gdbarch_half_bit (gdbarch),
		       "half", gdbarch_half_format (gdbarch));
  builtin_type->builtin_float
    = init_float_type (alloc, gdbarch_float_bit (gdbarch),
		       "float", gdbarch_float_format (gdbarch));
  builtin_type->builtin_bfloat16
    = init_float_type (alloc, gdbarch_bfloat16_bit (gdbarch),
		       "bfloat16", gdbarch_bfloat16_format (gdbarch));
  builtin_type->builtin_double
    = init_float_type (alloc, gdbarch_double_bit (gdbarch),
		       "double", gdbarch_double_format (gdbarch));
  builtin_type->builtin_long_double
    = init_float_type (alloc, gdbarch_long_double_bit (gdbarch),
		       "long double", gdbarch_long_double_format (gdbarch));
  builtin_type->builtin_complex
    = init_complex_type ("complex", builtin_type->builtin_float);
  builtin_type->builtin_double_complex
    = init_complex_type ("double complex", builtin_type->builtin_double);
  builtin_type->builtin_string
    = alloc.new_type (TYPE_CODE_STRING, TARGET_CHAR_BIT, "string");
  builtin_type->builtin_bool
    = init_boolean_type (alloc, TARGET_CHAR_BIT, 1, "bool");

  /* The following three are about decimal floating point types, which
     are 32-bits, 64-bits and 128-bits respectively.  */
  builtin_type->builtin_decfloat
    = init_decfloat_type (alloc, 32, "_Decimal32");
  builtin_type->builtin_decdouble
    = init_decfloat_type (alloc, 64, "_Decimal64");
  builtin_type->builtin_declong
    = init_decfloat_type (alloc, 128, "_Decimal128");

  /* "True" character types.  */
  builtin_type->builtin_true_char
    = init_character_type (alloc, TARGET_CHAR_BIT, 0, "true character");
  builtin_type->builtin_true_unsigned_char
    = init_character_type (alloc, TARGET_CHAR_BIT, 1, "true character");

  /* Fixed-size integer types.  */
  builtin_type->builtin_int0
    = init_integer_type (alloc, 0, 0, "int0_t");
  builtin_type->builtin_int8
    = init_integer_type (alloc, 8, 0, "int8_t");
  builtin_type->builtin_uint8
    = init_integer_type (alloc, 8, 1, "uint8_t");
  builtin_type->builtin_int16
    = init_integer_type (alloc, 16, 0, "int16_t");
  builtin_type->builtin_uint16
    = init_integer_type (alloc, 16, 1, "uint16_t");
  builtin_type->builtin_int24
    = init_integer_type (alloc, 24, 0, "int24_t");
  builtin_type->builtin_uint24
    = init_integer_type (alloc, 24, 1, "uint24_t");
  builtin_type->builtin_int32
    = init_integer_type (alloc, 32, 0, "int32_t");
  builtin_type->builtin_uint32
    = init_integer_type (alloc, 32, 1, "uint32_t");
  builtin_type->builtin_int64
    = init_integer_type (alloc, 64, 0, "int64_t");
  builtin_type->builtin_uint64
    = init_integer_type (alloc, 64, 1, "uint64_t");
  builtin_type->builtin_int128
    = init_integer_type (alloc, 128, 0, "int128_t");
  builtin_type->builtin_uint128
    = init_integer_type (alloc, 128, 1, "uint128_t");

  builtin_type->builtin_int8->set_instance_flags
    (builtin_type->builtin_int8->instance_flags ()
     | TYPE_INSTANCE_FLAG_NOTTEXT);

  builtin_type->builtin_uint8->set_instance_flags
    (builtin_type->builtin_uint8->instance_flags ()
     | TYPE_INSTANCE_FLAG_NOTTEXT);

  /* Wide character types.  */
  builtin_type->builtin_char16
    = init_integer_type (alloc, 16, 1, "char16_t");
  builtin_type->builtin_char32
    = init_integer_type (alloc, 32, 1, "char32_t");
  builtin_type->builtin_wchar
    = init_integer_type (alloc, gdbarch_wchar_bit (gdbarch),
			 !gdbarch_wchar_signed (gdbarch), "wchar_t");

  /* Default data/code pointer types.  */
  builtin_type->builtin_data_ptr
    = lookup_pointer_type (builtin_type->builtin_void);
  builtin_type->builtin_func_ptr
    = lookup_pointer_type (lookup_function_type (builtin_type->builtin_void));
  builtin_type->builtin_func_func
    = lookup_function_type (builtin_type->builtin_func_ptr);

  /* This type represents a GDB internal function.  */
  builtin_type->internal_fn
    = alloc.new_type (TYPE_CODE_INTERNAL_FUNCTION, 0,
		      "<internal function>");

  /* This type represents an xmethod.  */
  builtin_type->xmethod
    = alloc.new_type (TYPE_CODE_XMETHOD, 0, "<xmethod>");

  /* This type represents a type that was unrecognized in symbol read-in.  */
  builtin_type->builtin_error
    = alloc.new_type (TYPE_CODE_ERROR, 0, "<unknown type>");

  /* The following set of types is used for symbols with no
     debug information.  */
  builtin_type->nodebug_text_symbol
    = alloc.new_type (TYPE_CODE_FUNC, TARGET_CHAR_BIT,
		      "<text variable, no debug info>");

  builtin_type->nodebug_text_gnu_ifunc_symbol
    = alloc.new_type (TYPE_CODE_FUNC, TARGET_CHAR_BIT,
		      "<text gnu-indirect-function variable, no debug info>");
  builtin_type->nodebug_text_gnu_ifunc_symbol->set_is_gnu_ifunc (true);

  builtin_type->nodebug_got_plt_symbol
    = init_pointer_type (alloc, gdbarch_addr_bit (gdbarch),
			 "<text from jump slot in .got.plt, no debug info>",
			 builtin_type->nodebug_text_symbol);
  builtin_type->nodebug_data_symbol
    = alloc.new_type (TYPE_CODE_ERROR, 0, "<data variable, no debug info>");
  builtin_type->nodebug_unknown_symbol
    = alloc.new_type (TYPE_CODE_ERROR, 0,
		      "<variable (not text or data), no debug info>");
  builtin_type->nodebug_tls_symbol
    = alloc.new_type (TYPE_CODE_ERROR, 0,
		      "<thread local variable, no debug info>");

  /* NOTE: on some targets, addresses and pointers are not necessarily
     the same.

     The upshot is:
     - gdb's `struct type' always describes the target's
       representation.
     - gdb's `struct value' objects should always hold values in
       target form.
     - gdb's CORE_ADDR values are addresses in the unified virtual
       address space that the assembler and linker work with.  Thus,
       since target_read_memory takes a CORE_ADDR as an argument, it
       can access any memory on the target, even if the processor has
       separate code and data address spaces.

     In this context, builtin_type->builtin_core_addr is a bit odd:
     it's a target type for a value the target will never see.  It's
     only used to hold the values of (typeless) linker symbols, which
     are indeed in the unified virtual address space.  */

  builtin_type->builtin_core_addr
    = init_integer_type (alloc, gdbarch_addr_bit (gdbarch), 1,
			 "__CORE_ADDR");
  return builtin_type;
}

const struct builtin_type *
builtin_type (struct gdbarch *gdbarch)
{
  struct builtin_type *result = gdbtypes_data.get (gdbarch);
  if (result == nullptr)
    {
      result = create_gdbtypes_data (gdbarch);
      gdbtypes_data.set (gdbarch, result);
    }
  return result;
}

const struct builtin_type *
builtin_type (struct objfile *objfile)
{
  return builtin_type (objfile->arch ());
}

/* See gdbtypes.h.  */

CORE_ADDR
call_site::pc () const
{
  return per_objfile->relocate (m_unrelocated_pc);
}

void _initialize_gdbtypes ();
void
_initialize_gdbtypes ()
{
  add_setshow_zuinteger_cmd ("overload", no_class, &overload_debug,
			     _("Set debugging of C++ overloading."),
			     _("Show debugging of C++ overloading."),
			     _("When enabled, ranking of the "
			       "functions is displayed."),
			     NULL,
			     show_overload_debug,
			     &setdebuglist, &showdebuglist);

  /* Add user knob for controlling resolution of opaque types.  */
  add_setshow_boolean_cmd ("opaque-type-resolution", class_support,
			   &opaque_type_resolution,
			   _("Set resolution of opaque struct/class/union"
			     " types (if set before loading symbols)."),
			   _("Show resolution of opaque struct/class/union"
			     " types (if set before loading symbols)."),
			   NULL, NULL,
			   show_opaque_type_resolution,
			   &setlist, &showlist);

  /* Add an option to permit non-strict type checking.  */
  add_setshow_boolean_cmd ("type", class_support,
			   &strict_type_checking,
			   _("Set strict type checking."),
			   _("Show strict type checking."),
			   NULL, NULL,
			   show_strict_type_checking,
			   &setchecklist, &showchecklist);
}

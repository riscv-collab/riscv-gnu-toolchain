/* Abstraction of GNU v2 abi.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

   Contributed by Daniel Berlin <dberlin@redhat.com>

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
#include "value.h"
#include "demangle.h"
#include "gdb-demangle.h"
#include "cp-abi.h"
#include "cp-support.h"
#include <ctype.h>

static cp_abi_ops gnu_v2_abi_ops;

static int vb_match (struct type *, int, struct type *);

static enum dtor_kinds
gnuv2_is_destructor_name (const char *name)
{
  if ((name[0] == '_' && is_cplus_marker (name[1]) && name[2] == '_')
      || startswith (name, "__dt__"))
    return complete_object_dtor;
  else
    return (enum dtor_kinds) 0;
}

static enum ctor_kinds
gnuv2_is_constructor_name (const char *name)
{
  if ((name[0] == '_' && name[1] == '_'
       && (isdigit (name[2]) || strchr ("Qt", name[2])))
      || startswith (name, "__ct__"))
    return complete_object_ctor;
  else
    return (enum ctor_kinds) 0;
}

static int
gnuv2_is_vtable_name (const char *name)
{
  return (((name)[0] == '_'
	   && (((name)[1] == 'V' && (name)[2] == 'T')
	       || ((name)[1] == 'v' && (name)[2] == 't'))
	   && is_cplus_marker ((name)[3])) ||
	  ((name)[0] == '_' && (name)[1] == '_'
	   && (name)[2] == 'v' && (name)[3] == 't' && (name)[4] == '_'));
}

static int
gnuv2_is_operator_name (const char *name)
{
  return startswith (name, CP_OPERATOR_STR);
}


/* Return a virtual function as a value.
   ARG1 is the object which provides the virtual function
   table pointer.  *ARG1P is side-effected in calling this function.
   F is the list of member functions which contains the desired virtual
   function.
   J is an index into F which provides the desired virtual function.

   TYPE is the type in which F is located.  */
static struct value *
gnuv2_virtual_fn_field (struct value **arg1p, struct fn_field * f, int j,
			struct type * type, int offset)
{
  struct value *arg1 = *arg1p;
  struct type *type1 = check_typedef (arg1->type ());
  struct type *entry_type;
  /* First, get the virtual function table pointer.  That comes
     with a strange type, so cast it to type `pointer to long' (which
     should serve just fine as a function type).  Then, index into
     the table, and convert final value to appropriate function type.  */
  struct value *entry;
  struct value *vfn;
  struct value *vtbl;
  LONGEST vi = (LONGEST) TYPE_FN_FIELD_VOFFSET (f, j);
  struct type *fcontext = TYPE_FN_FIELD_FCONTEXT (f, j);
  struct type *context;
  struct type *context_vptr_basetype;
  int context_vptr_fieldno;

  if (fcontext == NULL)
    /* We don't have an fcontext (e.g. the program was compiled with
       g++ version 1).  Try to get the vtbl from the TYPE_VPTR_BASETYPE.
       This won't work right for multiple inheritance, but at least we
       should do as well as GDB 3.x did.  */
    fcontext = TYPE_VPTR_BASETYPE (type);
  context = lookup_pointer_type (fcontext);
  /* Now context is a pointer to the basetype containing the vtbl.  */
  if (context->target_type () != type1)
    {
      struct value *tmp = value_cast (context, value_addr (arg1));

      arg1 = value_ind (tmp);
      type1 = check_typedef (arg1->type ());
    }

  context = type1;
  /* Now context is the basetype containing the vtbl.  */

  /* This type may have been defined before its virtual function table
     was.  If so, fill in the virtual function table entry for the
     type now.  */
  context_vptr_fieldno = get_vptr_fieldno (context, &context_vptr_basetype);
  /* FIXME: What to do if vptr_fieldno is still -1?  */

  /* The virtual function table is now an array of structures
     which have the form { int16 offset, delta; void *pfn; }.  */
  vtbl = arg1->primitive_field (0, context_vptr_fieldno,
				context_vptr_basetype);

  /* With older versions of g++, the vtbl field pointed to an array
     of structures.  Nowadays it points directly to the structure.  */
  if (vtbl->type ()->code () == TYPE_CODE_PTR
      && vtbl->type ()->target_type ()->code () == TYPE_CODE_ARRAY)
    {
      /* Handle the case where the vtbl field points to an
	 array of structures.  */
      vtbl = value_ind (vtbl);

      /* Index into the virtual function table.  This is hard-coded because
	 looking up a field is not cheap, and it may be important to save
	 time, e.g. if the user has set a conditional breakpoint calling
	 a virtual function.  */
      entry = value_subscript (vtbl, vi);
    }
  else
    {
      /* Handle the case where the vtbl field points directly to a
	 structure.  */
      vtbl = value_ptradd (vtbl, vi);
      entry = value_ind (vtbl);
    }

  entry_type = check_typedef (entry->type ());

  if (entry_type->code () == TYPE_CODE_STRUCT)
    {
      /* Move the `this' pointer according to the virtual function table.  */
      arg1->set_offset (arg1->offset ()
			+ value_as_long (value_field (entry, 0)));

      if (!arg1->lazy ())
	{
	  arg1->set_lazy (true);
	  arg1->fetch_lazy ();
	}

      vfn = value_field (entry, 2);
    }
  else if (entry_type->code () == TYPE_CODE_PTR)
    vfn = entry;
  else
    error (_("I'm confused:  virtual function table has bad type"));
  /* Reinstantiate the function pointer with the correct type.  */
  vfn->deprecated_set_type (lookup_pointer_type (TYPE_FN_FIELD_TYPE (f, j)));

  *arg1p = arg1;
  return vfn;
}


static struct type *
gnuv2_value_rtti_type (struct value *v, int *full, LONGEST *top, int *using_enc)
{
  struct type *known_type;
  struct type *rtti_type;
  CORE_ADDR vtbl;
  struct bound_minimal_symbol minsym;
  char *p;
  const char *linkage_name;
  struct type *btype;
  struct type *known_type_vptr_basetype;
  int known_type_vptr_fieldno;

  if (full)
    *full = 0;
  if (top)
    *top = -1;
  if (using_enc)
    *using_enc = 0;

  /* Get declared type.  */
  known_type = v->type ();
  known_type = check_typedef (known_type);
  /* RTTI works only or class objects.  */
  if (known_type->code () != TYPE_CODE_STRUCT)
    return NULL;

  /* Plan on this changing in the future as i get around to setting
     the vtables properly for G++ compiled stuff.  Also, I'll be using
     the type info functions, which are always right.  Deal with it
     until then.  */

  /* Try to get the vptr basetype, fieldno.  */
  known_type_vptr_fieldno = get_vptr_fieldno (known_type,
					      &known_type_vptr_basetype);

  /* If we can't find it, give up.  */
  if (known_type_vptr_fieldno < 0)
    return NULL;

  /* Make sure our basetype and known type match, otherwise, cast
     so we can get at the vtable properly.  */
  btype = known_type_vptr_basetype;
  btype = check_typedef (btype);
  if (btype != known_type )
    {
      v = value_cast (btype, v);
      if (using_enc)
	*using_enc=1;
    }
  /* We can't use value_ind here, because it would want to use RTTI, and
     we'd waste a bunch of time figuring out we already know the type.
     Besides, we don't care about the type, just the actual pointer.  */
  if (value_field (v, known_type_vptr_fieldno)->address () == 0)
    return NULL;

  vtbl = value_as_address (value_field (v, known_type_vptr_fieldno));

  /* Try to find a symbol that is the vtable.  */
  minsym=lookup_minimal_symbol_by_pc(vtbl);
  if (minsym.minsym==NULL
      || (linkage_name=minsym.minsym->linkage_name ())==NULL
      || !is_vtable_name (linkage_name))
    return NULL;

  /* If we just skip the prefix, we get screwed by namespaces.  */
  gdb::unique_xmalloc_ptr<char> demangled_name
    = gdb_demangle(linkage_name,DMGL_PARAMS|DMGL_ANSI);
  p = strchr (demangled_name.get (), ' ');
  if (p)
    *p = '\0';

  /* Lookup the type for the name.  */
  /* FIXME: chastain/2003-11-26: block=NULL is bogus.  See pr gdb/1465.  */
  rtti_type = cp_lookup_rtti_type (demangled_name.get (), NULL);
  if (rtti_type == NULL)
    return NULL;

  if (TYPE_N_BASECLASSES(rtti_type) > 1 &&  full && (*full) != 1)
    {
      if (top)
	*top = TYPE_BASECLASS_BITPOS (rtti_type,
				      TYPE_VPTR_FIELDNO(rtti_type)) / 8;
      if (top && ((*top) >0))
	{
	  if (rtti_type->length () > known_type->length ())
	    {
	      if (full)
		*full=0;
	    }
	  else
	    {
	      if (full)
		*full=1;
	    }
	}
    }
  else
    {
      if (full)
	*full=1;
    }

  return rtti_type;
}

/* Return true if the INDEXth field of TYPE is a virtual baseclass
   pointer which is for the base class whose type is BASECLASS.  */

static int
vb_match (struct type *type, int index, struct type *basetype)
{
  struct type *fieldtype;
  const char *name = type->field (index).name ();
  const char *field_class_name = NULL;

  if (*name != '_')
    return 0;
  /* gcc 2.4 uses _vb$.  */
  if (name[1] == 'v' && name[2] == 'b' && is_cplus_marker (name[3]))
    field_class_name = name + 4;
  /* gcc 2.5 will use __vb_.  */
  if (name[1] == '_' && name[2] == 'v' && name[3] == 'b' && name[4] == '_')
    field_class_name = name + 5;

  if (field_class_name == NULL)
    /* This field is not a virtual base class pointer.  */
    return 0;

  /* It's a virtual baseclass pointer, now we just need to find out whether
     it is for this baseclass.  */
  fieldtype = type->field (index).type ();
  if (fieldtype == NULL
      || fieldtype->code () != TYPE_CODE_PTR)
    /* "Can't happen".  */
    return 0;

  /* What we check for is that either the types are equal (needed for
     nameless types) or have the same name.  This is ugly, and a more
     elegant solution should be devised (which would probably just push
     the ugliness into symbol reading unless we change the stabs format).  */
  if (fieldtype->target_type () == basetype)
    return 1;

  if (basetype->name () != NULL
      && fieldtype->target_type ()->name () != NULL
      && strcmp (basetype->name (),
		 fieldtype->target_type ()->name ()) == 0)
    return 1;
  return 0;
}

/* Compute the offset of the baseclass which is the INDEXth baseclass
   of class TYPE, for value at VALADDR (in host) at ADDRESS (in
   target).  The result is the offset of the baseclass value relative
   to (the address of)(ARG) + OFFSET.  */

static int
gnuv2_baseclass_offset (struct type *type, int index,
			const bfd_byte *valaddr, LONGEST embedded_offset,
			CORE_ADDR address, const struct value *val)
{
  struct type *basetype = TYPE_BASECLASS (type, index);

  if (BASETYPE_VIA_VIRTUAL (type, index))
    {
      /* Must hunt for the pointer to this virtual baseclass.  */
      int i, len = type->num_fields ();
      int n_baseclasses = TYPE_N_BASECLASSES (type);

      /* First look for the virtual baseclass pointer
	 in the fields.  */
      for (i = n_baseclasses; i < len; i++)
	{
	  if (vb_match (type, i, basetype))
	    {
	      struct type *field_type;
	      LONGEST field_offset;
	      int field_length;
	      CORE_ADDR addr;

	      field_type = check_typedef (type->field (i).type ());
	      field_offset = type->field (i).loc_bitpos () / 8;
	      field_length = field_type->length ();

	      if (!val->bytes_available (embedded_offset + field_offset,
					 field_length))
		throw_error (NOT_AVAILABLE_ERROR,
			     _("Virtual baseclass pointer is not available"));

	      addr = unpack_pointer (field_type,
				     valaddr + embedded_offset + field_offset);

	      return addr - (LONGEST) address + embedded_offset;
	    }
	}
      /* Not in the fields, so try looking through the baseclasses.  */
      for (i = index + 1; i < n_baseclasses; i++)
	{
	  /* Don't go through baseclass_offset, as that wraps
	     exceptions, thus, inner exceptions would be wrapped more
	     than once.  */
	  int boffset =
	    gnuv2_baseclass_offset (type, i, valaddr,
				    embedded_offset, address, val);

	  if (boffset)
	    return boffset;
	}

      error (_("Baseclass offset not found"));
    }

  /* Baseclass is easily computed.  */
  return TYPE_BASECLASS_BITPOS (type, index) / 8;
}

static void
init_gnuv2_ops (void)
{
  gnu_v2_abi_ops.shortname = "gnu-v2";
  gnu_v2_abi_ops.longname = "GNU G++ Version 2 ABI";
  gnu_v2_abi_ops.doc = "G++ Version 2 ABI";
  gnu_v2_abi_ops.is_destructor_name = gnuv2_is_destructor_name;
  gnu_v2_abi_ops.is_constructor_name = gnuv2_is_constructor_name;
  gnu_v2_abi_ops.is_vtable_name = gnuv2_is_vtable_name;
  gnu_v2_abi_ops.is_operator_name = gnuv2_is_operator_name;
  gnu_v2_abi_ops.virtual_fn_field = gnuv2_virtual_fn_field;
  gnu_v2_abi_ops.rtti_type = gnuv2_value_rtti_type;
  gnu_v2_abi_ops.baseclass_offset = gnuv2_baseclass_offset;
}

void _initialize_gnu_v2_abi ();
void
_initialize_gnu_v2_abi ()
{
  init_gnuv2_ops ();
  register_cp_abi (&gnu_v2_abi_ops);
}

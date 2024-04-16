/* Abstraction of GNU v3 abi.
   Contributed by Jim Blandy <jimb@redhat.com>

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "language.h"
#include "value.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "demangle.h"
#include "dwarf2.h"
#include "objfiles.h"
#include "valprint.h"
#include "c-lang.h"
#include "typeprint.h"
#include <algorithm>
#include "cli/cli-style.h"
#include "dwarf2/loc.h"
#include "inferior.h"

static struct cp_abi_ops gnu_v3_abi_ops;

/* A gdbarch key for std::type_info, in the event that it can't be
   found in the debug info.  */

static const registry<gdbarch>::key<struct type> std_type_info_gdbarch_data;


static int
gnuv3_is_vtable_name (const char *name)
{
  return startswith (name, "_ZTV");
}

static int
gnuv3_is_operator_name (const char *name)
{
  return startswith (name, CP_OPERATOR_STR);
}


/* To help us find the components of a vtable, we build ourselves a
   GDB type object representing the vtable structure.  Following the
   V3 ABI, it goes something like this:

   struct gdb_gnu_v3_abi_vtable {

     / * An array of virtual call and virtual base offsets.  The real
	 length of this array depends on the class hierarchy; we use
	 negative subscripts to access the elements.  Yucky, but
	 better than the alternatives.  * /
     ptrdiff_t vcall_and_vbase_offsets[0];

     / * The offset from a virtual pointer referring to this table
	 to the top of the complete object.  * /
     ptrdiff_t offset_to_top;

     / * The type_info pointer for this class.  This is really a
	 std::type_info *, but GDB doesn't really look at the
	 type_info object itself, so we don't bother to get the type
	 exactly right.  * /
     void *type_info;

     / * Virtual table pointers in objects point here.  * /

     / * Virtual function pointers.  Like the vcall/vbase array, the
	 real length of this table depends on the class hierarchy.  * /
     void (*virtual_functions[0]) ();

   };

   The catch, of course, is that the exact layout of this table
   depends on the ABI --- word size, endianness, alignment, etc.  So
   the GDB type object is actually a per-architecture kind of thing.

   vtable_type_gdbarch_data is a gdbarch per-architecture data pointer
   which refers to the struct type * for this structure, laid out
   appropriately for the architecture.  */
static const registry<gdbarch>::key<struct type> vtable_type_gdbarch_data;


/* Human-readable names for the numbers of the fields above.  */
enum {
  vtable_field_vcall_and_vbase_offsets,
  vtable_field_offset_to_top,
  vtable_field_type_info,
  vtable_field_virtual_functions
};


/* Return a GDB type representing `struct gdb_gnu_v3_abi_vtable',
   described above, laid out appropriately for ARCH.

   We use this function as the gdbarch per-architecture data
   initialization function.  */
static struct type *
get_gdb_vtable_type (struct gdbarch *arch)
{
  struct type *t;
  int offset;

  struct type *result = vtable_type_gdbarch_data.get (arch);
  if (result != nullptr)
    return result;

  struct type *void_ptr_type
    = builtin_type (arch)->builtin_data_ptr;
  struct type *ptr_to_void_fn_type
    = builtin_type (arch)->builtin_func_ptr;

  type_allocator alloc (arch);

  /* ARCH can't give us the true ptrdiff_t type, so we guess.  */
  struct type *ptrdiff_type
    = init_integer_type (alloc, gdbarch_ptr_bit (arch), 0, "ptrdiff_t");

  t = alloc.new_type (TYPE_CODE_STRUCT, 0, nullptr);

  /* We assume no padding is necessary, since GDB doesn't know
     anything about alignment at the moment.  If this assumption bites
     us, we should add a gdbarch method which, given a type, returns
     the alignment that type requires, and then use that here.  */

  /* Build the field list.  */
  t->alloc_fields (4);

  offset = 0;

  /* ptrdiff_t vcall_and_vbase_offsets[0]; */
  {
    struct field &field0 = t->field (0);
    field0.set_name ("vcall_and_vbase_offsets");
    field0.set_type (lookup_array_range_type (ptrdiff_type, 0, -1));
    field0.set_loc_bitpos (offset * TARGET_CHAR_BIT);
    offset += field0.type ()->length ();
  }

  /* ptrdiff_t offset_to_top; */
  {
    struct field &field1 = t->field (1);
    field1.set_name ("offset_to_top");
    field1.set_type (ptrdiff_type);
    field1.set_loc_bitpos (offset * TARGET_CHAR_BIT);
    offset += field1.type ()->length ();
  }

  /* void *type_info; */
  {
    struct field &field2 = t->field (2);
    field2.set_name ("type_info");
    field2.set_type (void_ptr_type);
    field2.set_loc_bitpos (offset * TARGET_CHAR_BIT);
    offset += field2.type ()->length ();
  }

  /* void (*virtual_functions[0]) (); */
  {
    struct field &field3 = t->field (3);
    field3.set_name ("virtual_functions");
    field3.set_type (lookup_array_range_type (ptr_to_void_fn_type, 0, -1));
    field3.set_loc_bitpos (offset * TARGET_CHAR_BIT);
    offset += field3.type ()->length ();
  }

  t->set_length (offset);

  t->set_name ("gdb_gnu_v3_abi_vtable");
  INIT_CPLUS_SPECIFIC (t);

  result = make_type_with_address_space (t, TYPE_INSTANCE_FLAG_CODE_SPACE);
  vtable_type_gdbarch_data.set (arch, result);
  return result;
}


/* Return the ptrdiff_t type used in the vtable type.  */
static struct type *
vtable_ptrdiff_type (struct gdbarch *gdbarch)
{
  struct type *vtable_type = get_gdb_vtable_type (gdbarch);

  /* The "offset_to_top" field has the appropriate (ptrdiff_t) type.  */
  return vtable_type->field (vtable_field_offset_to_top).type ();
}

/* Return the offset from the start of the imaginary `struct
   gdb_gnu_v3_abi_vtable' object to the vtable's "address point"
   (i.e., where objects' virtual table pointers point).  */
static int
vtable_address_point_offset (struct gdbarch *gdbarch)
{
  struct type *vtable_type = get_gdb_vtable_type (gdbarch);

  return (vtable_type->field (vtable_field_virtual_functions).loc_bitpos ()
	  / TARGET_CHAR_BIT);
}


/* Determine whether structure TYPE is a dynamic class.  Cache the
   result.  */

static int
gnuv3_dynamic_class (struct type *type)
{
  int fieldnum, fieldelem;

  type = check_typedef (type);
  gdb_assert (type->code () == TYPE_CODE_STRUCT
	      || type->code () == TYPE_CODE_UNION);

  if (type->code () == TYPE_CODE_UNION)
    return 0;

  if (TYPE_CPLUS_DYNAMIC (type))
    return TYPE_CPLUS_DYNAMIC (type) == 1;

  ALLOCATE_CPLUS_STRUCT_TYPE (type);

  for (fieldnum = 0; fieldnum < TYPE_N_BASECLASSES (type); fieldnum++)
    if (BASETYPE_VIA_VIRTUAL (type, fieldnum)
	|| gnuv3_dynamic_class (type->field (fieldnum).type ()))
      {
	TYPE_CPLUS_DYNAMIC (type) = 1;
	return 1;
      }

  for (fieldnum = 0; fieldnum < TYPE_NFN_FIELDS (type); fieldnum++)
    for (fieldelem = 0; fieldelem < TYPE_FN_FIELDLIST_LENGTH (type, fieldnum);
	 fieldelem++)
      {
	struct fn_field *f = TYPE_FN_FIELDLIST1 (type, fieldnum);

	if (TYPE_FN_FIELD_VIRTUAL_P (f, fieldelem))
	  {
	    TYPE_CPLUS_DYNAMIC (type) = 1;
	    return 1;
	  }
      }

  TYPE_CPLUS_DYNAMIC (type) = -1;
  return 0;
}

/* Find the vtable for a value of CONTAINER_TYPE located at
   CONTAINER_ADDR.  Return a value of the correct vtable type for this
   architecture, or NULL if CONTAINER does not have a vtable.  */

static struct value *
gnuv3_get_vtable (struct gdbarch *gdbarch,
		  struct type *container_type, CORE_ADDR container_addr)
{
  struct type *vtable_type = get_gdb_vtable_type (gdbarch);
  struct type *vtable_pointer_type;
  struct value *vtable_pointer;
  CORE_ADDR vtable_address;

  container_type = check_typedef (container_type);
  gdb_assert (container_type->code () == TYPE_CODE_STRUCT);

  /* If this type does not have a virtual table, don't read the first
     field.  */
  if (!gnuv3_dynamic_class (container_type))
    return NULL;

  /* We do not consult the debug information to find the virtual table.
     The ABI specifies that it is always at offset zero in any class,
     and debug information may not represent it.

     We avoid using value_contents on principle, because the object might
     be large.  */

  /* Find the type "pointer to virtual table".  */
  vtable_pointer_type = lookup_pointer_type (vtable_type);

  /* Load it from the start of the class.  */
  vtable_pointer = value_at (vtable_pointer_type, container_addr);
  vtable_address = value_as_address (vtable_pointer);

  /* Correct it to point at the start of the virtual table, rather
     than the address point.  */
  return value_at_lazy (vtable_type,
			vtable_address
			- vtable_address_point_offset (gdbarch));
}


static struct type *
gnuv3_rtti_type (struct value *value,
		 int *full_p, LONGEST *top_p, int *using_enc_p)
{
  struct gdbarch *gdbarch;
  struct type *values_type = check_typedef (value->type ());
  struct value *vtable;
  struct minimal_symbol *vtable_symbol;
  const char *vtable_symbol_name;
  const char *class_name;
  struct type *run_time_type;
  LONGEST offset_to_top;
  const char *atsign;

  /* We only have RTTI for dynamic class objects.  */
  if (values_type->code () != TYPE_CODE_STRUCT
      || !gnuv3_dynamic_class (values_type))
    return NULL;

  /* Determine architecture.  */
  gdbarch = values_type->arch ();

  if (using_enc_p)
    *using_enc_p = 0;

  vtable = gnuv3_get_vtable (gdbarch, values_type,
			     value_as_address (value_addr (value)));
  if (vtable == NULL)
    return NULL;

  /* Find the linker symbol for this vtable.  */
  vtable_symbol
    = lookup_minimal_symbol_by_pc (vtable->address ()
				   + vtable->embedded_offset ()).minsym;
  if (! vtable_symbol)
    return NULL;
  
  /* The symbol's demangled name should be something like "vtable for
     CLASS", where CLASS is the name of the run-time type of VALUE.
     If we didn't like this approach, we could instead look in the
     type_info object itself to get the class name.  But this way
     should work just as well, and doesn't read target memory.  */
  vtable_symbol_name = vtable_symbol->demangled_name ();
  if (vtable_symbol_name == NULL
      || !startswith (vtable_symbol_name, "vtable for "))
    {
      warning (_("can't find linker symbol for virtual table for `%s' value"),
	       TYPE_SAFE_NAME (values_type));
      if (vtable_symbol_name)
	warning (_("  found `%s' instead"), vtable_symbol_name);
      return NULL;
    }
  class_name = vtable_symbol_name + 11;

  /* Strip off @plt and version suffixes.  */
  atsign = strchr (class_name, '@');
  if (atsign != NULL)
    {
      char *copy;

      copy = (char *) alloca (atsign - class_name + 1);
      memcpy (copy, class_name, atsign - class_name);
      copy[atsign - class_name] = '\0';
      class_name = copy;
    }

  /* Try to look up the class name as a type name.  */
  /* FIXME: chastain/2003-11-26: block=NULL is bogus.  See pr gdb/1465.  */
  run_time_type = cp_lookup_rtti_type (class_name, NULL);
  if (run_time_type == NULL)
    return NULL;

  /* Get the offset from VALUE to the top of the complete object.
     NOTE: this is the reverse of the meaning of *TOP_P.  */
  offset_to_top
    = value_as_long (value_field (vtable, vtable_field_offset_to_top));

  if (full_p)
    *full_p = (- offset_to_top == value->embedded_offset ()
	       && (value->enclosing_type ()->length ()
		   >= run_time_type->length ()));
  if (top_p)
    *top_p = - offset_to_top;
  return run_time_type;
}

/* Return a function pointer for CONTAINER's VTABLE_INDEX'th virtual
   function, of type FNTYPE.  */

static struct value *
gnuv3_get_virtual_fn (struct gdbarch *gdbarch, struct value *container,
		      struct type *fntype, int vtable_index)
{
  struct value *vtable, *vfn;

  /* Every class with virtual functions must have a vtable.  */
  vtable = gnuv3_get_vtable (gdbarch, container->type (),
			     value_as_address (value_addr (container)));
  gdb_assert (vtable != NULL);

  /* Fetch the appropriate function pointer from the vtable.  */
  vfn = value_subscript (value_field (vtable, vtable_field_virtual_functions),
			 vtable_index);

  /* If this architecture uses function descriptors directly in the vtable,
     then the address of the vtable entry is actually a "function pointer"
     (i.e. points to the descriptor).  We don't need to scale the index
     by the size of a function descriptor; GCC does that before outputting
     debug information.  */
  if (gdbarch_vtable_function_descriptors (gdbarch))
    vfn = value_addr (vfn);

  /* Cast the function pointer to the appropriate type.  */
  vfn = value_cast (lookup_pointer_type (fntype), vfn);

  return vfn;
}

/* GNU v3 implementation of value_virtual_fn_field.  See cp-abi.h
   for a description of the arguments.  */

static struct value *
gnuv3_virtual_fn_field (struct value **value_p,
			struct fn_field *f, int j,
			struct type *vfn_base, int offset)
{
  struct type *values_type = check_typedef ((*value_p)->type ());
  struct gdbarch *gdbarch;

  /* Some simple sanity checks.  */
  if (values_type->code () != TYPE_CODE_STRUCT)
    error (_("Only classes can have virtual functions."));

  /* Determine architecture.  */
  gdbarch = values_type->arch ();

  /* Cast our value to the base class which defines this virtual
     function.  This takes care of any necessary `this'
     adjustments.  */
  if (vfn_base != values_type)
    *value_p = value_cast (vfn_base, *value_p);

  return gnuv3_get_virtual_fn (gdbarch, *value_p, TYPE_FN_FIELD_TYPE (f, j),
			       TYPE_FN_FIELD_VOFFSET (f, j));
}

/* Compute the offset of the baseclass which is
   the INDEXth baseclass of class TYPE,
   for value at VALADDR (in host) at ADDRESS (in target).
   The result is the offset of the baseclass value relative
   to (the address of)(ARG) + OFFSET.

   -1 is returned on error.  */

static int
gnuv3_baseclass_offset (struct type *type, int index,
			const bfd_byte *valaddr, LONGEST embedded_offset,
			CORE_ADDR address, const struct value *val)
{
  struct gdbarch *gdbarch;
  struct type *ptr_type;
  struct value *vtable;
  struct value *vbase_array;
  long int cur_base_offset, base_offset;

  /* Determine architecture.  */
  gdbarch = type->arch ();
  ptr_type = builtin_type (gdbarch)->builtin_data_ptr;

  /* If it isn't a virtual base, this is easy.  The offset is in the
     type definition.  */
  if (!BASETYPE_VIA_VIRTUAL (type, index))
    return TYPE_BASECLASS_BITPOS (type, index) / 8;

  /* If we have a DWARF expression for the offset, evaluate it.  */
  if (type->field (index).loc_kind () == FIELD_LOC_KIND_DWARF_BLOCK)
    {
      struct dwarf2_property_baton baton;
      baton.property_type
	= lookup_pointer_type (type->field (index).type ());
      baton.locexpr = *type->field (index).loc_dwarf_block ();

      struct dynamic_prop prop;
      prop.set_locexpr (&baton);

      struct property_addr_info addr_stack;
      addr_stack.type = type;
      /* Note that we don't set "valaddr" here.  Doing so causes
	 regressions.  FIXME.  */
      addr_stack.addr = address + embedded_offset;
      addr_stack.next = nullptr;

      CORE_ADDR result;
      if (dwarf2_evaluate_property (&prop, nullptr, &addr_stack, &result,
				    {addr_stack.addr}))
	return (int) (result - addr_stack.addr);
    }

  /* To access a virtual base, we need to use the vbase offset stored in
     our vtable.  Recent GCC versions provide this information.  If it isn't
     available, we could get what we needed from RTTI, or from drawing the
     complete inheritance graph based on the debug info.  Neither is
     worthwhile.  */
  cur_base_offset = TYPE_BASECLASS_BITPOS (type, index) / 8;
  if (cur_base_offset >= - vtable_address_point_offset (gdbarch))
    error (_("Expected a negative vbase offset (old compiler?)"));

  cur_base_offset = cur_base_offset + vtable_address_point_offset (gdbarch);
  if ((- cur_base_offset) % ptr_type->length () != 0)
    error (_("Misaligned vbase offset."));
  cur_base_offset = cur_base_offset / ((int) ptr_type->length ());

  vtable = gnuv3_get_vtable (gdbarch, type, address + embedded_offset);
  gdb_assert (vtable != NULL);
  vbase_array = value_field (vtable, vtable_field_vcall_and_vbase_offsets);
  base_offset = value_as_long (value_subscript (vbase_array, cur_base_offset));
  return base_offset;
}

/* Locate a virtual method in DOMAIN or its non-virtual base classes
   which has virtual table index VOFFSET.  The method has an associated
   "this" adjustment of ADJUSTMENT bytes.  */

static const char *
gnuv3_find_method_in (struct type *domain, CORE_ADDR voffset,
		      LONGEST adjustment)
{
  int i;

  /* Search this class first.  */
  if (adjustment == 0)
    {
      int len;

      len = TYPE_NFN_FIELDS (domain);
      for (i = 0; i < len; i++)
	{
	  int len2, j;
	  struct fn_field *f;

	  f = TYPE_FN_FIELDLIST1 (domain, i);
	  len2 = TYPE_FN_FIELDLIST_LENGTH (domain, i);

	  check_stub_method_group (domain, i);
	  for (j = 0; j < len2; j++)
	    if (TYPE_FN_FIELD_VOFFSET (f, j) == voffset)
	      return TYPE_FN_FIELD_PHYSNAME (f, j);
	}
    }

  /* Next search non-virtual bases.  If it's in a virtual base,
     we're out of luck.  */
  for (i = 0; i < TYPE_N_BASECLASSES (domain); i++)
    {
      int pos;
      struct type *basetype;

      if (BASETYPE_VIA_VIRTUAL (domain, i))
	continue;

      pos = TYPE_BASECLASS_BITPOS (domain, i) / 8;
      basetype = domain->field (i).type ();
      /* Recurse with a modified adjustment.  We don't need to adjust
	 voffset.  */
      if (adjustment >= pos && adjustment < pos + basetype->length ())
	return gnuv3_find_method_in (basetype, voffset, adjustment - pos);
    }

  return NULL;
}

/* Decode GNU v3 method pointer.  */

static int
gnuv3_decode_method_ptr (struct gdbarch *gdbarch,
			 const gdb_byte *contents,
			 CORE_ADDR *value_p,
			 LONGEST *adjustment_p)
{
  struct type *funcptr_type = builtin_type (gdbarch)->builtin_func_ptr;
  struct type *offset_type = vtable_ptrdiff_type (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR ptr_value;
  LONGEST voffset, adjustment;
  int vbit;

  /* Extract the pointer to member.  The first element is either a pointer
     or a vtable offset.  For pointers, we need to use extract_typed_address
     to allow the back-end to convert the pointer to a GDB address -- but
     vtable offsets we must handle as integers.  At this point, we do not
     yet know which case we have, so we extract the value under both
     interpretations and choose the right one later on.  */
  ptr_value = extract_typed_address (contents, funcptr_type);
  voffset = extract_signed_integer (contents,
				    funcptr_type->length (), byte_order);
  contents += funcptr_type->length ();
  adjustment = extract_signed_integer (contents,
				       offset_type->length (), byte_order);

  if (!gdbarch_vbit_in_delta (gdbarch))
    {
      vbit = voffset & 1;
      voffset = voffset ^ vbit;
    }
  else
    {
      vbit = adjustment & 1;
      adjustment = adjustment >> 1;
    }

  *value_p = vbit? voffset : ptr_value;
  *adjustment_p = adjustment;
  return vbit;
}

/* GNU v3 implementation of cplus_print_method_ptr.  */

static void
gnuv3_print_method_ptr (const gdb_byte *contents,
			struct type *type,
			struct ui_file *stream)
{
  struct type *self_type = TYPE_SELF_TYPE (type);
  struct gdbarch *gdbarch = self_type->arch ();
  CORE_ADDR ptr_value;
  LONGEST adjustment;
  int vbit;

  /* Extract the pointer to member.  */
  vbit = gnuv3_decode_method_ptr (gdbarch, contents, &ptr_value, &adjustment);

  /* Check for NULL.  */
  if (ptr_value == 0 && vbit == 0)
    {
      gdb_printf (stream, "NULL");
      return;
    }

  /* Search for a virtual method.  */
  if (vbit)
    {
      CORE_ADDR voffset;
      const char *physname;

      /* It's a virtual table offset, maybe in this class.  Search
	 for a field with the correct vtable offset.  First convert it
	 to an index, as used in TYPE_FN_FIELD_VOFFSET.  */
      voffset = ptr_value / vtable_ptrdiff_type (gdbarch)->length ();

      physname = gnuv3_find_method_in (self_type, voffset, adjustment);

      /* If we found a method, print that.  We don't bother to disambiguate
	 possible paths to the method based on the adjustment.  */
      if (physname)
	{
	  gdb::unique_xmalloc_ptr<char> demangled_name
	    = gdb_demangle (physname, DMGL_ANSI | DMGL_PARAMS);

	  gdb_printf (stream, "&virtual ");
	  if (demangled_name == NULL)
	    gdb_puts (physname, stream);
	  else
	    gdb_puts (demangled_name.get (), stream);
	  return;
	}
    }
  else if (ptr_value != 0)
    {
      /* Found a non-virtual function: print out the type.  */
      gdb_puts ("(", stream);
      c_print_type (type, "", stream, -1, 0, current_language->la_language,
		    &type_print_raw_options);
      gdb_puts (") ", stream);
    }

  /* We didn't find it; print the raw data.  */
  if (vbit)
    {
      gdb_printf (stream, "&virtual table offset ");
      print_longest (stream, 'd', 1, ptr_value);
    }
  else
    {
      struct value_print_options opts;

      get_user_print_options (&opts);
      print_address_demangle (&opts, gdbarch, ptr_value, stream, demangle);
    }

  if (adjustment)
    {
      gdb_printf (stream, ", this adjustment ");
      print_longest (stream, 'd', 1, adjustment);
    }
}

/* GNU v3 implementation of cplus_method_ptr_size.  */

static int
gnuv3_method_ptr_size (struct type *type)
{
  return 2 * builtin_type (type->arch ())->builtin_data_ptr->length ();
}

/* GNU v3 implementation of cplus_make_method_ptr.  */

static void
gnuv3_make_method_ptr (struct type *type, gdb_byte *contents,
		       CORE_ADDR value, int is_virtual)
{
  struct gdbarch *gdbarch = type->arch ();
  int size = builtin_type (gdbarch)->builtin_data_ptr->length ();
  enum bfd_endian byte_order = type_byte_order (type);

  /* FIXME drow/2006-12-24: The adjustment of "this" is currently
     always zero, since the method pointer is of the correct type.
     But if the method pointer came from a base class, this is
     incorrect - it should be the offset to the base.  The best
     fix might be to create the pointer to member pointing at the
     base class and cast it to the derived class, but that requires
     support for adjusting pointers to members when casting them -
     not currently supported by GDB.  */

  if (!gdbarch_vbit_in_delta (gdbarch))
    {
      store_unsigned_integer (contents, size, byte_order, value | is_virtual);
      store_unsigned_integer (contents + size, size, byte_order, 0);
    }
  else
    {
      store_unsigned_integer (contents, size, byte_order, value);
      store_unsigned_integer (contents + size, size, byte_order, is_virtual);
    }
}

/* GNU v3 implementation of cplus_method_ptr_to_value.  */

static struct value *
gnuv3_method_ptr_to_value (struct value **this_p, struct value *method_ptr)
{
  struct gdbarch *gdbarch;
  const gdb_byte *contents = method_ptr->contents ().data ();
  CORE_ADDR ptr_value;
  struct type *self_type, *final_type, *method_type;
  LONGEST adjustment;
  int vbit;

  self_type = TYPE_SELF_TYPE (check_typedef (method_ptr->type ()));
  final_type = lookup_pointer_type (self_type);

  method_type = check_typedef (method_ptr->type ())->target_type ();

  /* Extract the pointer to member.  */
  gdbarch = self_type->arch ();
  vbit = gnuv3_decode_method_ptr (gdbarch, contents, &ptr_value, &adjustment);

  /* First convert THIS to match the containing type of the pointer to
     member.  This cast may adjust the value of THIS.  */
  *this_p = value_cast (final_type, *this_p);

  /* Then apply whatever adjustment is necessary.  This creates a somewhat
     strange pointer: it claims to have type FINAL_TYPE, but in fact it
     might not be a valid FINAL_TYPE.  For instance, it might be a
     base class of FINAL_TYPE.  And if it's not the primary base class,
     then printing it out as a FINAL_TYPE object would produce some pretty
     garbage.

     But we don't really know the type of the first argument in
     METHOD_TYPE either, which is why this happens.  We can't
     dereference this later as a FINAL_TYPE, but once we arrive in the
     called method we'll have debugging information for the type of
     "this" - and that'll match the value we produce here.

     You can provoke this case by casting a Base::* to a Derived::*, for
     instance.  */
  *this_p = value_cast (builtin_type (gdbarch)->builtin_data_ptr, *this_p);
  *this_p = value_ptradd (*this_p, adjustment);
  *this_p = value_cast (final_type, *this_p);

  if (vbit)
    {
      LONGEST voffset;

      voffset = ptr_value / vtable_ptrdiff_type (gdbarch)->length ();
      return gnuv3_get_virtual_fn (gdbarch, value_ind (*this_p),
				   method_type, voffset);
    }
  else
    return value_from_pointer (lookup_pointer_type (method_type), ptr_value);
}

/* Objects of this type are stored in a hash table and a vector when
   printing the vtables for a class.  */

struct value_and_voffset
{
  /* The value representing the object.  */
  struct value *value;

  /* The maximum vtable offset we've found for any object at this
     offset in the outermost object.  */
  int max_voffset;
};

/* Hash function for value_and_voffset.  */

static hashval_t
hash_value_and_voffset (const void *p)
{
  const struct value_and_voffset *o = (const struct value_and_voffset *) p;

  return o->value->address () + o->value->embedded_offset ();
}

/* Equality function for value_and_voffset.  */

static int
eq_value_and_voffset (const void *a, const void *b)
{
  const struct value_and_voffset *ova = (const struct value_and_voffset *) a;
  const struct value_and_voffset *ovb = (const struct value_and_voffset *) b;

  return (ova->value->address () + ova->value->embedded_offset ()
	  == ovb->value->address () + ovb->value->embedded_offset ());
}

/* Comparison function for value_and_voffset.  */

static bool
compare_value_and_voffset (const struct value_and_voffset *va,
			   const struct value_and_voffset *vb)
{
  CORE_ADDR addra = (va->value->address ()
		     + va->value->embedded_offset ());
  CORE_ADDR addrb = (vb->value->address ()
		     + vb->value->embedded_offset ());

  return addra < addrb;
}

/* A helper function used when printing vtables.  This determines the
   key (most derived) sub-object at each address and also computes the
   maximum vtable offset seen for the corresponding vtable.  Updates
   OFFSET_HASH and OFFSET_VEC with a new value_and_voffset object, if
   needed.  VALUE is the object to examine.  */

static void
compute_vtable_size (htab_t offset_hash,
		     std::vector<value_and_voffset *> *offset_vec,
		     struct value *value)
{
  int i;
  struct type *type = check_typedef (value->type ());
  void **slot;
  struct value_and_voffset search_vo, *current_vo;

  gdb_assert (type->code () == TYPE_CODE_STRUCT);

  /* If the object is not dynamic, then we are done; as it cannot have
     dynamic base types either.  */
  if (!gnuv3_dynamic_class (type))
    return;

  /* Update the hash and the vec, if needed.  */
  search_vo.value = value;
  slot = htab_find_slot (offset_hash, &search_vo, INSERT);
  if (*slot)
    current_vo = (struct value_and_voffset *) *slot;
  else
    {
      current_vo = XNEW (struct value_and_voffset);
      current_vo->value = value;
      current_vo->max_voffset = -1;
      *slot = current_vo;
      offset_vec->push_back (current_vo);
    }

  /* Update the value_and_voffset object with the highest vtable
     offset from this class.  */
  for (i = 0; i < TYPE_NFN_FIELDS (type); ++i)
    {
      int j;
      struct fn_field *fn = TYPE_FN_FIELDLIST1 (type, i);

      for (j = 0; j < TYPE_FN_FIELDLIST_LENGTH (type, i); ++j)
	{
	  if (TYPE_FN_FIELD_VIRTUAL_P (fn, j))
	    {
	      int voffset = TYPE_FN_FIELD_VOFFSET (fn, j);

	      if (voffset > current_vo->max_voffset)
		current_vo->max_voffset = voffset;
	    }
	}
    }

  /* Recurse into base classes.  */
  for (i = 0; i < TYPE_N_BASECLASSES (type); ++i)
    compute_vtable_size (offset_hash, offset_vec, value_field (value, i));
}

/* Helper for gnuv3_print_vtable that prints a single vtable.  */

static void
print_one_vtable (struct gdbarch *gdbarch, struct value *value,
		  int max_voffset,
		  struct value_print_options *opts)
{
  int i;
  struct type *type = check_typedef (value->type ());
  struct value *vtable;
  CORE_ADDR vt_addr;

  vtable = gnuv3_get_vtable (gdbarch, type,
			     value->address ()
			     + value->embedded_offset ());
  vt_addr = value_field (vtable,
			 vtable_field_virtual_functions)->address ();

  gdb_printf (_("vtable for '%s' @ %s (subobject @ %s):\n"),
	      TYPE_SAFE_NAME (type),
	      paddress (gdbarch, vt_addr),
	      paddress (gdbarch, (value->address ()
				  + value->embedded_offset ())));

  for (i = 0; i <= max_voffset; ++i)
    {
      /* Initialize it just to avoid a GCC false warning.  */
      CORE_ADDR addr = 0;
      int got_error = 0;
      struct value *vfn;

      gdb_printf ("[%d]: ", i);

      vfn = value_subscript (value_field (vtable,
					  vtable_field_virtual_functions),
			     i);

      if (gdbarch_vtable_function_descriptors (gdbarch))
	vfn = value_addr (vfn);

      try
	{
	  addr = value_as_address (vfn);
	}
      catch (const gdb_exception_error &ex)
	{
	  fprintf_styled (gdb_stdout, metadata_style.style (),
			  _("<error: %s>"), ex.what ());
	  got_error = 1;
	}

      if (!got_error)
	print_function_pointer_address (opts, gdbarch, addr, gdb_stdout);
      gdb_printf ("\n");
    }
}

/* Implementation of the print_vtable method.  */

static void
gnuv3_print_vtable (struct value *value)
{
  struct gdbarch *gdbarch;
  struct type *type;
  struct value *vtable;
  struct value_print_options opts;
  int count;

  value = coerce_ref (value);
  type = check_typedef (value->type ());
  if (type->code () == TYPE_CODE_PTR)
    {
      value = value_ind (value);
      type = check_typedef (value->type ());
    }

  get_user_print_options (&opts);

  /* Respect 'set print object'.  */
  if (opts.objectprint)
    {
      value = value_full_object (value, NULL, 0, 0, 0);
      type = check_typedef (value->type ());
    }

  gdbarch = type->arch ();

  vtable = NULL;
  if (type->code () == TYPE_CODE_STRUCT)
    vtable = gnuv3_get_vtable (gdbarch, type,
			       value_as_address (value_addr (value)));

  if (!vtable)
    {
      gdb_printf (_("This object does not have a virtual function table\n"));
      return;
    }

  htab_up offset_hash (htab_create_alloc (1, hash_value_and_voffset,
					  eq_value_and_voffset,
					  xfree, xcalloc, xfree));
  std::vector<value_and_voffset *> result_vec;

  compute_vtable_size (offset_hash.get (), &result_vec, value);
  std::sort (result_vec.begin (), result_vec.end (),
	     compare_value_and_voffset);

  count = 0;
  for (value_and_voffset *iter : result_vec)
    {
      if (iter->max_voffset >= 0)
	{
	  if (count > 0)
	    gdb_printf ("\n");
	  print_one_vtable (gdbarch, iter->value, iter->max_voffset, &opts);
	  ++count;
	}
    }
}

/* Return a GDB type representing `struct std::type_info', laid out
   appropriately for ARCH.

   We use this function as the gdbarch per-architecture data
   initialization function.  */

static struct type *
build_std_type_info_type (struct gdbarch *arch)
{
  struct type *t;
  int offset;
  struct type *void_ptr_type
    = builtin_type (arch)->builtin_data_ptr;
  struct type *char_type
    = builtin_type (arch)->builtin_char;
  struct type *char_ptr_type
    = make_pointer_type (make_cv_type (1, 0, char_type, NULL), NULL);

  t = type_allocator (arch).new_type (TYPE_CODE_STRUCT, 0, nullptr);

  t->alloc_fields (2);

  offset = 0;

  /* The vtable.  */
  {
    struct field &field0 = t->field (0);
    field0.set_name ("_vptr.type_info");
    field0.set_type (void_ptr_type);
    field0.set_loc_bitpos (offset * TARGET_CHAR_BIT);
    offset += field0.type ()->length ();
  }

  /* The name.  */
  {
    struct field &field1 = t->field (1);
    field1.set_name ("__name");
    field1.set_type (char_ptr_type);
    field1.set_loc_bitpos (offset * TARGET_CHAR_BIT);
    offset += field1.type ()->length ();
  }

  t->set_length (offset);

  t->set_name ("gdb_gnu_v3_type_info");
  INIT_CPLUS_SPECIFIC (t);

  return t;
}

/* Implement the 'get_typeid_type' method.  */

static struct type *
gnuv3_get_typeid_type (struct gdbarch *gdbarch)
{
  struct symbol *typeinfo;
  struct type *typeinfo_type;

  typeinfo = lookup_symbol ("std::type_info", NULL, STRUCT_DOMAIN,
			    NULL).symbol;
  if (typeinfo == NULL)
    {
      typeinfo_type = std_type_info_gdbarch_data.get (gdbarch);
      if (typeinfo_type == nullptr)
	{
	  typeinfo_type = build_std_type_info_type (gdbarch);
	  std_type_info_gdbarch_data.set (gdbarch, typeinfo_type);
	}
    }
  else
    typeinfo_type = typeinfo->type ();

  return typeinfo_type;
}

/* Implement the 'get_typeid' method.  */

static struct value *
gnuv3_get_typeid (struct value *value)
{
  struct type *typeinfo_type;
  struct type *type;
  struct gdbarch *gdbarch;
  struct value *result;
  std::string type_name;
  gdb::unique_xmalloc_ptr<char> canonical;

  /* We have to handle values a bit trickily here, to allow this code
     to work properly with non_lvalue values that are really just
     disguised types.  */
  if (value->lval () == lval_memory)
    value = coerce_ref (value);

  type = check_typedef (value->type ());

  /* In the non_lvalue case, a reference might have slipped through
     here.  */
  if (type->code () == TYPE_CODE_REF)
    type = check_typedef (type->target_type ());

  /* Ignore top-level cv-qualifiers.  */
  type = make_cv_type (0, 0, type, NULL);
  gdbarch = type->arch ();

  type_name = type_to_string (type);
  if (type_name.empty ())
    error (_("cannot find typeinfo for unnamed type"));

  /* We need to canonicalize the type name here, because we do lookups
     using the demangled name, and so we must match the format it
     uses.  E.g., GDB tends to use "const char *" as a type name, but
     the demangler uses "char const *".  */
  canonical = cp_canonicalize_string (type_name.c_str ());
  const char *name = (canonical == nullptr
		      ? type_name.c_str ()
		      : canonical.get ());

  typeinfo_type = gnuv3_get_typeid_type (gdbarch);

  /* We check for lval_memory because in the "typeid (type-id)" case,
     the type is passed via a not_lval value object.  */
  if (type->code () == TYPE_CODE_STRUCT
      && value->lval () == lval_memory
      && gnuv3_dynamic_class (type))
    {
      struct value *vtable, *typeinfo_value;
      CORE_ADDR address = value->address () + value->embedded_offset ();

      vtable = gnuv3_get_vtable (gdbarch, type, address);
      if (vtable == NULL)
	error (_("cannot find typeinfo for object of type '%s'"),
	       name);
      typeinfo_value = value_field (vtable, vtable_field_type_info);
      result = value_ind (value_cast (make_pointer_type (typeinfo_type, NULL),
				      typeinfo_value));
    }
  else
    {
      std::string sym_name = std::string ("typeinfo for ") + name;
      bound_minimal_symbol minsym
	= lookup_minimal_symbol (sym_name.c_str (), NULL, NULL);

      if (minsym.minsym == NULL)
	error (_("could not find typeinfo symbol for '%s'"), name);

      result = value_at_lazy (typeinfo_type, minsym.value_address ());
    }

  return result;
}

/* Implement the 'get_typename_from_type_info' method.  */

static std::string
gnuv3_get_typename_from_type_info (struct value *type_info_ptr)
{
  struct gdbarch *gdbarch = type_info_ptr->type ()->arch ();
  struct bound_minimal_symbol typeinfo_sym;
  CORE_ADDR addr;
  const char *symname;
  const char *class_name;
  const char *atsign;

  addr = value_as_address (type_info_ptr);
  typeinfo_sym = lookup_minimal_symbol_by_pc (addr);
  if (typeinfo_sym.minsym == NULL)
    error (_("could not find minimal symbol for typeinfo address %s"),
	   paddress (gdbarch, addr));

#define TYPEINFO_PREFIX "typeinfo for "
#define TYPEINFO_PREFIX_LEN (sizeof (TYPEINFO_PREFIX) - 1)
  symname = typeinfo_sym.minsym->demangled_name ();
  if (symname == NULL || strncmp (symname, TYPEINFO_PREFIX,
				  TYPEINFO_PREFIX_LEN))
    error (_("typeinfo symbol '%s' has unexpected name"),
	   typeinfo_sym.minsym->linkage_name ());
  class_name = symname + TYPEINFO_PREFIX_LEN;

  /* Strip off @plt and version suffixes.  */
  atsign = strchr (class_name, '@');
  if (atsign != NULL)
    return std::string (class_name, atsign - class_name);
  return class_name;
}

/* Implement the 'get_type_from_type_info' method.  */

static struct type *
gnuv3_get_type_from_type_info (struct value *type_info_ptr)
{
  /* We have to parse the type name, since in general there is not a
     symbol for a type.  This is somewhat bogus since there may be a
     mis-parse.  Another approach might be to re-use the demangler's
     internal form to reconstruct the type somehow.  */
  std::string type_name = gnuv3_get_typename_from_type_info (type_info_ptr);
  expression_up expr (parse_expression (type_name.c_str ()));
  struct value *type_val = expr->evaluate_type ();
  return type_val->type ();
}

/* Determine if we are currently in a C++ thunk.  If so, get the address
   of the routine we are thunking to and continue to there instead.  */

static CORE_ADDR 
gnuv3_skip_trampoline (frame_info_ptr frame, CORE_ADDR stop_pc)
{
  CORE_ADDR real_stop_pc, method_stop_pc, func_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct bound_minimal_symbol thunk_sym, fn_sym;
  struct obj_section *section;
  const char *thunk_name, *fn_name;
  
  real_stop_pc = gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc);
  if (real_stop_pc == 0)
    real_stop_pc = stop_pc;

  /* Find the linker symbol for this potential thunk.  */
  thunk_sym = lookup_minimal_symbol_by_pc (real_stop_pc);
  section = find_pc_section (real_stop_pc);
  if (thunk_sym.minsym == NULL || section == NULL)
    return 0;

  /* The symbol's demangled name should be something like "virtual
     thunk to FUNCTION", where FUNCTION is the name of the function
     being thunked to.  */
  thunk_name = thunk_sym.minsym->demangled_name ();
  if (thunk_name == NULL || strstr (thunk_name, " thunk to ") == NULL)
    return 0;

  fn_name = strstr (thunk_name, " thunk to ") + strlen (" thunk to ");
  fn_sym = lookup_minimal_symbol (fn_name, NULL, section->objfile);
  if (fn_sym.minsym == NULL)
    return 0;

  method_stop_pc = fn_sym.value_address ();

  /* Some targets have minimal symbols pointing to function descriptors
     (powerpc 64 for example).  Make sure to retrieve the address
     of the real function from the function descriptor before passing on
     the address to other layers of GDB.  */
  func_addr = gdbarch_convert_from_func_ptr_addr
    (gdbarch, method_stop_pc, current_inferior ()->top_target ());
  if (func_addr != 0)
    method_stop_pc = func_addr;

  real_stop_pc = gdbarch_skip_trampoline_code
		   (gdbarch, frame, method_stop_pc);
  if (real_stop_pc == 0)
    real_stop_pc = method_stop_pc;

  return real_stop_pc;
}

/* A member function is in one these states.  */

enum definition_style
{
  DOES_NOT_EXIST_IN_SOURCE,
  DEFAULTED_INSIDE,
  DEFAULTED_OUTSIDE,
  DELETED,
  EXPLICIT,
};

/* Return how the given field is defined.  */

static definition_style
get_def_style (struct fn_field *fn, int fieldelem)
{
  if (TYPE_FN_FIELD_DELETED (fn, fieldelem))
    return DELETED;

  if (TYPE_FN_FIELD_ARTIFICIAL (fn, fieldelem))
    return DOES_NOT_EXIST_IN_SOURCE;

  switch (TYPE_FN_FIELD_DEFAULTED (fn, fieldelem))
    {
    case DW_DEFAULTED_no:
      return EXPLICIT;
    case DW_DEFAULTED_in_class:
      return DEFAULTED_INSIDE;
    case DW_DEFAULTED_out_of_class:
      return DEFAULTED_OUTSIDE;
    default:
      break;
    }

  return EXPLICIT;
}

/* Helper functions to determine whether the given definition style
   denotes that the definition is user-provided or implicit.
   Being defaulted outside the class decl counts as an explicit
   user-definition, while being defaulted inside is implicit.  */

static bool
is_user_provided_def (definition_style def)
{
  return def == EXPLICIT || def == DEFAULTED_OUTSIDE;
}

static bool
is_implicit_def (definition_style def)
{
  return def == DOES_NOT_EXIST_IN_SOURCE || def == DEFAULTED_INSIDE;
}

/* Helper function to decide if METHOD_TYPE is a copy/move
   constructor type for CLASS_TYPE.  EXPECTED is the expected
   type code for the "right-hand-side" argument.
   This function is supposed to be used by the IS_COPY_CONSTRUCTOR_TYPE
   and IS_MOVE_CONSTRUCTOR_TYPE functions below.  Normally, you should
   not need to call this directly.  */

static bool
is_copy_or_move_constructor_type (struct type *class_type,
				  struct type *method_type,
				  type_code expected)
{
  /* The method should take at least two arguments...  */
  if (method_type->num_fields () < 2)
    return false;

  /* ...and the second argument should be the same as the class
     type, with the expected type code...  */
  struct type *arg_type = method_type->field (1).type ();

  if (arg_type->code () != expected)
    return false;

  struct type *target = check_typedef (arg_type->target_type ());
  if (!(class_types_same_p (target, class_type)))
    return false;

  /* ...and if any of the remaining arguments don't have a default value
     then this is not a copy or move constructor, but just a
     constructor.  */
  for (int i = 2; i < method_type->num_fields (); i++)
    {
      arg_type = method_type->field (i).type ();
      /* FIXME aktemur/2019-10-31: As of this date, neither
	 clang++-7.0.0 nor g++-8.2.0 produce a DW_AT_default_value
	 attribute.  GDB is also not set to read this attribute, yet.
	 Hence, we immediately return false if there are more than
	 2 parameters.
	 GCC bug link:
	 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=42959
      */
      return false;
    }

  return true;
}

/* Return true if METHOD_TYPE is a copy ctor type for CLASS_TYPE.  */

static bool
is_copy_constructor_type (struct type *class_type,
			  struct type *method_type)
{
  return is_copy_or_move_constructor_type (class_type, method_type,
					   TYPE_CODE_REF);
}

/* Return true if METHOD_TYPE is a move ctor type for CLASS_TYPE.  */

static bool
is_move_constructor_type (struct type *class_type,
			  struct type *method_type)
{
  return is_copy_or_move_constructor_type (class_type, method_type,
					   TYPE_CODE_RVALUE_REF);
}

/* Return pass-by-reference information for the given TYPE.

   The rule in the v3 ABI document comes from section 3.1.1.  If the
   type has a non-trivial copy constructor or destructor, then the
   caller must make a copy (by calling the copy constructor if there
   is one or perform the copy itself otherwise), pass the address of
   the copy, and then destroy the temporary (if necessary).

   For return values with non-trivial copy/move constructors or
   destructors, space will be allocated in the caller, and a pointer
   will be passed as the first argument (preceding "this").

   We don't have a bulletproof mechanism for determining whether a
   constructor or destructor is trivial.  For GCC and DWARF5 debug
   information, we can check the calling_convention attribute,
   the 'artificial' flag, the 'defaulted' attribute, and the
   'deleted' attribute.  */

static struct language_pass_by_ref_info
gnuv3_pass_by_reference (struct type *type)
{
  int fieldnum, fieldelem;

  type = check_typedef (type);

  /* Start with the default values.  */
  struct language_pass_by_ref_info info;

  bool has_cc_attr = false;
  bool is_pass_by_value = false;
  bool is_dynamic = false;
  definition_style cctor_def = DOES_NOT_EXIST_IN_SOURCE;
  definition_style dtor_def = DOES_NOT_EXIST_IN_SOURCE;
  definition_style mctor_def = DOES_NOT_EXIST_IN_SOURCE;

  /* We're only interested in things that can have methods.  */
  if (type->code () != TYPE_CODE_STRUCT
      && type->code () != TYPE_CODE_UNION)
    return info;

  /* The compiler may have emitted the calling convention attribute.
     Note: GCC does not produce this attribute as of version 9.2.1.
     Bug link: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92418  */
  if (TYPE_CPLUS_CALLING_CONVENTION (type) == DW_CC_pass_by_value)
    {
      has_cc_attr = true;
      is_pass_by_value = true;
      /* Do not return immediately.  We have to find out if this type
	 is copy_constructible and destructible.  */
    }

  if (TYPE_CPLUS_CALLING_CONVENTION (type) == DW_CC_pass_by_reference)
    {
      has_cc_attr = true;
      is_pass_by_value = false;
    }

  /* A dynamic class has a non-trivial copy constructor.
     See c++98 section 12.8 Copying class objects [class.copy].  */
  if (gnuv3_dynamic_class (type))
    is_dynamic = true;

  for (fieldnum = 0; fieldnum < TYPE_NFN_FIELDS (type); fieldnum++)
    for (fieldelem = 0; fieldelem < TYPE_FN_FIELDLIST_LENGTH (type, fieldnum);
	 fieldelem++)
      {
	struct fn_field *fn = TYPE_FN_FIELDLIST1 (type, fieldnum);
	const char *name = TYPE_FN_FIELDLIST_NAME (type, fieldnum);
	struct type *fieldtype = TYPE_FN_FIELD_TYPE (fn, fieldelem);

	if (name[0] == '~')
	  {
	    /* We've found a destructor.
	       There should be at most one dtor definition.  */
	    gdb_assert (dtor_def == DOES_NOT_EXIST_IN_SOURCE);
	    dtor_def = get_def_style (fn, fieldelem);
	  }
	else if (is_constructor_name (TYPE_FN_FIELD_PHYSNAME (fn, fieldelem))
		 || TYPE_FN_FIELD_CONSTRUCTOR (fn, fieldelem))
	  {
	    /* FIXME drow/2007-09-23: We could do this using the name of
	       the method and the name of the class instead of dealing
	       with the mangled name.  We don't have a convenient function
	       to strip off both leading scope qualifiers and trailing
	       template arguments yet.  */
	    if (is_copy_constructor_type (type, fieldtype))
	      {
		/* There may be more than one cctors.  E.g.: one that
		   take a const parameter and another that takes a
		   non-const parameter.  Such as:

		   class K {
		     K (const K &k)...
		     K (K &k)...
		   };

		   It is sufficient for the type to be non-trivial
		   even only one of the cctors is explicit.
		   Therefore, update the cctor_def value in the
		   implicit -> explicit direction, not backwards.  */

		if (is_implicit_def (cctor_def))
		  cctor_def = get_def_style (fn, fieldelem);
	      }
	    else if (is_move_constructor_type (type, fieldtype))
	      {
		/* Again, there may be multiple move ctors.  Update the
		   mctor_def value if we found an explicit def and the
		   existing one is not explicit.  Otherwise retain the
		   existing value.  */
		if (is_implicit_def (mctor_def))
		  mctor_def = get_def_style (fn, fieldelem);
	      }
	  }
      }

  bool cctor_implicitly_deleted
    = (mctor_def != DOES_NOT_EXIST_IN_SOURCE
       && cctor_def == DOES_NOT_EXIST_IN_SOURCE);

  bool cctor_explicitly_deleted = (cctor_def == DELETED);

  if (cctor_implicitly_deleted || cctor_explicitly_deleted)
    info.copy_constructible = false;

  if (dtor_def == DELETED)
    info.destructible = false;

  info.trivially_destructible = is_implicit_def (dtor_def);

  info.trivially_copy_constructible
    = (is_implicit_def (cctor_def)
       && !is_dynamic);

  info.trivially_copyable
    = (info.trivially_copy_constructible
       && info.trivially_destructible
       && !is_user_provided_def (mctor_def));

  /* Even if all the constructors and destructors were artificial, one
     of them may have invoked a non-artificial constructor or
     destructor in a base class.  If any base class needs to be passed
     by reference, so does this class.  Similarly for members, which
     are constructed whenever this class is.  We do not need to worry
     about recursive loops here, since we are only looking at members
     of complete class type.  Also ignore any static members.  */
  for (fieldnum = 0; fieldnum < type->num_fields (); fieldnum++)
    if (!type->field (fieldnum).is_static ())
      {
	struct type *field_type = type->field (fieldnum).type ();

	/* For arrays, make the decision based on the element type.  */
	if (field_type->code () == TYPE_CODE_ARRAY)
	  field_type = check_typedef (field_type->target_type ());

	struct language_pass_by_ref_info field_info
	  = gnuv3_pass_by_reference (field_type);

	if (!field_info.copy_constructible)
	  info.copy_constructible = false;
	if (!field_info.destructible)
	  info.destructible = false;
	if (!field_info.trivially_copyable)
	  info.trivially_copyable = false;
	if (!field_info.trivially_copy_constructible)
	  info.trivially_copy_constructible = false;
	if (!field_info.trivially_destructible)
	  info.trivially_destructible = false;
      }

  /* Consistency check.  */
  if (has_cc_attr && info.trivially_copyable != is_pass_by_value)
    {
      /* DWARF CC attribute is not the same as the inferred value;
	 use the DWARF attribute.  */
      info.trivially_copyable = is_pass_by_value;
    }

  return info;
}

static void
init_gnuv3_ops (void)
{
  gnu_v3_abi_ops.shortname = "gnu-v3";
  gnu_v3_abi_ops.longname = "GNU G++ Version 3 ABI";
  gnu_v3_abi_ops.doc = "G++ Version 3 ABI";
  gnu_v3_abi_ops.is_destructor_name =
    (enum dtor_kinds (*) (const char *))is_gnu_v3_mangled_dtor;
  gnu_v3_abi_ops.is_constructor_name =
    (enum ctor_kinds (*) (const char *))is_gnu_v3_mangled_ctor;
  gnu_v3_abi_ops.is_vtable_name = gnuv3_is_vtable_name;
  gnu_v3_abi_ops.is_operator_name = gnuv3_is_operator_name;
  gnu_v3_abi_ops.rtti_type = gnuv3_rtti_type;
  gnu_v3_abi_ops.virtual_fn_field = gnuv3_virtual_fn_field;
  gnu_v3_abi_ops.baseclass_offset = gnuv3_baseclass_offset;
  gnu_v3_abi_ops.print_method_ptr = gnuv3_print_method_ptr;
  gnu_v3_abi_ops.method_ptr_size = gnuv3_method_ptr_size;
  gnu_v3_abi_ops.make_method_ptr = gnuv3_make_method_ptr;
  gnu_v3_abi_ops.method_ptr_to_value = gnuv3_method_ptr_to_value;
  gnu_v3_abi_ops.print_vtable = gnuv3_print_vtable;
  gnu_v3_abi_ops.get_typeid = gnuv3_get_typeid;
  gnu_v3_abi_ops.get_typeid_type = gnuv3_get_typeid_type;
  gnu_v3_abi_ops.get_type_from_type_info = gnuv3_get_type_from_type_info;
  gnu_v3_abi_ops.get_typename_from_type_info
    = gnuv3_get_typename_from_type_info;
  gnu_v3_abi_ops.skip_trampoline = gnuv3_skip_trampoline;
  gnu_v3_abi_ops.pass_by_reference = gnuv3_pass_by_reference;
}

void _initialize_gnu_v3_abi ();
void
_initialize_gnu_v3_abi ()
{
  init_gnuv3_ops ();

  register_cp_abi (&gnu_v3_abi_ops);
  set_cp_abi_as_auto_default (gnu_v3_abi_ops.shortname);
}

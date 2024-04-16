
/* Internal type definitions for GDB.

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

#if !defined (GDBTYPES_H)
#define GDBTYPES_H 1

/* * \page gdbtypes GDB Types

   GDB represents all the different kinds of types in programming
   languages using a common representation defined in gdbtypes.h.

   The main data structure is main_type; it consists of a code (such
   as #TYPE_CODE_ENUM for enumeration types), a number of
   generally-useful fields such as the printable name, and finally a
   field main_type::type_specific that is a union of info specific to
   particular languages or other special cases (such as calling
   convention).

   The available type codes are defined in enum #type_code.  The enum
   includes codes both for types that are common across a variety
   of languages, and for types that are language-specific.

   Most accesses to type fields go through macros such as
   #TYPE_CODE(thistype) and #TYPE_FN_FIELD_CONST(thisfn, n).  These are
   written such that they can be used as both rvalues and lvalues.
 */

#include "hashtab.h"
#include "gdbsupport/array-view.h"
#include "gdbsupport/gdb-hashtab.h"
#include <optional>
#include "gdbsupport/offset-type.h"
#include "gdbsupport/enum-flags.h"
#include "gdbsupport/underlying.h"
#include "gdbsupport/print-utils.h"
#include "gdbsupport/function-view.h"
#include "dwarf2.h"
#include "gdbsupport/gdb_obstack.h"
#include "gmp-utils.h"

/* Forward declarations for prototypes.  */
struct field;
struct block;
struct value_print_options;
struct language_defn;
struct dwarf2_per_cu_data;
struct dwarf2_per_objfile;
struct dwarf2_property_baton;

/* * Different kinds of data types are distinguished by the `code'
   field.  */

enum type_code
  {
    TYPE_CODE_UNDEF = 0,	/**< Not used; catches errors */

#define OP(X) X,
#include "type-codes.def"
#undef OP

  };

/* * Some bits for the type's instance_flags word.  See the macros
   below for documentation on each bit.  */

enum type_instance_flag_value : unsigned
{
  TYPE_INSTANCE_FLAG_CONST = (1 << 0),
  TYPE_INSTANCE_FLAG_VOLATILE = (1 << 1),
  TYPE_INSTANCE_FLAG_CODE_SPACE = (1 << 2),
  TYPE_INSTANCE_FLAG_DATA_SPACE = (1 << 3),
  TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1 = (1 << 4),
  TYPE_INSTANCE_FLAG_ADDRESS_CLASS_2 = (1 << 5),
  TYPE_INSTANCE_FLAG_NOTTEXT = (1 << 6),
  TYPE_INSTANCE_FLAG_RESTRICT = (1 << 7),
  TYPE_INSTANCE_FLAG_ATOMIC = (1 << 8)
};

DEF_ENUM_FLAGS_TYPE (enum type_instance_flag_value, type_instance_flags);

/* * Not textual.  By default, GDB treats all single byte integers as
   characters (or elements of strings) unless this flag is set.  */

#define TYPE_NOTTEXT(t)	(((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_NOTTEXT)

/* * Constant type.  If this is set, the corresponding type has a
   const modifier.  */

#define TYPE_CONST(t) ((((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_CONST) != 0)

/* * Volatile type.  If this is set, the corresponding type has a
   volatile modifier.  */

#define TYPE_VOLATILE(t) \
  ((((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_VOLATILE) != 0)

/* * Restrict type.  If this is set, the corresponding type has a
   restrict modifier.  */

#define TYPE_RESTRICT(t) \
  ((((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_RESTRICT) != 0)

/* * Atomic type.  If this is set, the corresponding type has an
   _Atomic modifier.  */

#define TYPE_ATOMIC(t) \
  ((((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_ATOMIC) != 0)

/* * True if this type represents either an lvalue or lvalue reference type.  */

#define TYPE_IS_REFERENCE(t) \
  ((t)->code () == TYPE_CODE_REF || (t)->code () == TYPE_CODE_RVALUE_REF)

/* * True if this type is allocatable.  */
#define TYPE_IS_ALLOCATABLE(t) \
  ((t)->dyn_prop (DYN_PROP_ALLOCATED) != NULL)

/* * True if this type has variant parts.  */
#define TYPE_HAS_VARIANT_PARTS(t) \
  ((t)->dyn_prop (DYN_PROP_VARIANT_PARTS) != nullptr)

/* * True if this type has a dynamic length.  */
#define TYPE_HAS_DYNAMIC_LENGTH(t) \
  ((t)->dyn_prop (DYN_PROP_BYTE_SIZE) != nullptr)

/* * Instruction-space delimited type.  This is for Harvard architectures
   which have separate instruction and data address spaces (and perhaps
   others).

   GDB usually defines a flat address space that is a superset of the
   architecture's two (or more) address spaces, but this is an extension
   of the architecture's model.

   If TYPE_INSTANCE_FLAG_CODE_SPACE is set, an object of the corresponding type
   resides in instruction memory, even if its address (in the extended
   flat address space) does not reflect this.

   Similarly, if TYPE_INSTANCE_FLAG_DATA_SPACE is set, then an object of the
   corresponding type resides in the data memory space, even if
   this is not indicated by its (flat address space) address.

   If neither flag is set, the default space for functions / methods
   is instruction space, and for data objects is data memory.  */

#define TYPE_CODE_SPACE(t) \
  ((((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_CODE_SPACE) != 0)

#define TYPE_DATA_SPACE(t) \
  ((((t)->instance_flags ()) & TYPE_INSTANCE_FLAG_DATA_SPACE) != 0)

/* * Address class flags.  Some environments provide for pointers
   whose size is different from that of a normal pointer or address
   types where the bits are interpreted differently than normal
   addresses.  The TYPE_INSTANCE_FLAG_ADDRESS_CLASS_n flags may be used in
   target specific ways to represent these different types of address
   classes.  */

#define TYPE_ADDRESS_CLASS_1(t) (((t)->instance_flags ()) \
				 & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1)
#define TYPE_ADDRESS_CLASS_2(t) (((t)->instance_flags ()) \
				 & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_2)
#define TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL \
  (TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1 | TYPE_INSTANCE_FLAG_ADDRESS_CLASS_2)
#define TYPE_ADDRESS_CLASS_ALL(t) (((t)->instance_flags ()) \
				   & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL)

/* * Information about a single discriminant.  */

struct discriminant_range
{
  /* * The range of values for the variant.  This is an inclusive
     range.  */
  ULONGEST low, high;

  /* * Return true if VALUE is contained in this range.  IS_UNSIGNED
     is true if this should be an unsigned comparison; false for
     signed.  */
  bool contains (ULONGEST value, bool is_unsigned) const
  {
    if (is_unsigned)
      return value >= low && value <= high;
    LONGEST valuel = (LONGEST) value;
    return valuel >= (LONGEST) low && valuel <= (LONGEST) high;
  }
};

struct variant_part;

/* * A single variant.  A variant has a list of discriminant values.
   When the discriminator matches one of these, the variant is
   enabled.  Each variant controls zero or more fields; and may also
   control other variant parts as well.  This struct corresponds to
   DW_TAG_variant in DWARF.  */

struct variant : allocate_on_obstack
{
  /* * The discriminant ranges for this variant.  */
  gdb::array_view<discriminant_range> discriminants;

  /* * The fields controlled by this variant.  This is inclusive on
     the low end and exclusive on the high end.  A variant may not
     control any fields, in which case the two values will be equal.
     These are indexes into the type's array of fields.  */
  int first_field;
  int last_field;

  /* * Variant parts controlled by this variant.  */
  gdb::array_view<variant_part> parts;

  /* * Return true if this is the default variant.  The default
     variant can be recognized because it has no associated
     discriminants.  */
  bool is_default () const
  {
    return discriminants.empty ();
  }

  /* * Return true if this variant matches VALUE.  IS_UNSIGNED is true
     if this should be an unsigned comparison; false for signed.  */
  bool matches (ULONGEST value, bool is_unsigned) const;
};

/* * A variant part.  Each variant part has an optional discriminant
   and holds an array of variants.  This struct corresponds to
   DW_TAG_variant_part in DWARF.  */

struct variant_part : allocate_on_obstack
{
  /* * The index of the discriminant field in the outer type.  This is
     an index into the type's array of fields.  If this is -1, there
     is no discriminant, and only the default variant can be
     considered to be selected.  */
  int discriminant_index;

  /* * True if this discriminant is unsigned; false if signed.  This
     comes from the type of the discriminant.  */
  bool is_unsigned;

  /* * The variants that are controlled by this variant part.  Note
     that these will always be sorted by field number.  */
  gdb::array_view<variant> variants;
};


enum dynamic_prop_kind
{
  PROP_UNDEFINED, /* Not defined.  */
  PROP_CONST,     /* Constant.  */
  PROP_ADDR_OFFSET, /* Address offset.  */
  PROP_LOCEXPR,   /* Location expression.  */
  PROP_LOCLIST,    /* Location list.  */
  PROP_VARIANT_PARTS, /* Variant parts.  */
  PROP_TYPE,	   /* Type.  */
  PROP_VARIABLE_NAME, /* Variable name.  */
};

union dynamic_prop_data
{
  /* Storage for constant property.  */

  LONGEST const_val;

  /* Storage for dynamic property.  */

  const dwarf2_property_baton *baton;

  /* Storage of variant parts for a type.  A type with variant parts
     has all its fields "linearized" -- stored in a single field
     array, just as if they had all been declared that way.  The
     variant parts are attached via a dynamic property, and then are
     used to control which fields end up in the final type during
     dynamic type resolution.  */

  const gdb::array_view<variant_part> *variant_parts;

  /* Once a variant type is resolved, we may want to be able to go
     from the resolved type to the original type.  In this case we
     rewrite the property's kind and set this field.  */

  struct type *original_type;

  /* Name of a variable to look up; the variable holds the value of
     this property.  */

  const char *variable_name;
};

/* * Used to store a dynamic property.  */

struct dynamic_prop
{
  dynamic_prop_kind kind () const
  {
    return m_kind;
  }

  void set_undefined ()
  {
    m_kind = PROP_UNDEFINED;
  }

  LONGEST const_val () const
  {
    gdb_assert (m_kind == PROP_CONST);

    return m_data.const_val;
  }

  void set_const_val (LONGEST const_val)
  {
    m_kind = PROP_CONST;
    m_data.const_val = const_val;
  }

  /* Return true if this property has a constant value, false
     otherwise.  */
  bool is_constant () const
  { return m_kind == PROP_CONST; }

  const dwarf2_property_baton *baton () const
  {
    gdb_assert (m_kind == PROP_LOCEXPR
		|| m_kind == PROP_LOCLIST
		|| m_kind == PROP_ADDR_OFFSET);

    return m_data.baton;
  }

  void set_locexpr (const dwarf2_property_baton *baton)
  {
    m_kind = PROP_LOCEXPR;
    m_data.baton = baton;
  }

  void set_loclist (const dwarf2_property_baton *baton)
  {
    m_kind = PROP_LOCLIST;
    m_data.baton = baton;
  }

  void set_addr_offset (const dwarf2_property_baton *baton)
  {
    m_kind = PROP_ADDR_OFFSET;
    m_data.baton = baton;
  }

  const gdb::array_view<variant_part> *variant_parts () const
  {
    gdb_assert (m_kind == PROP_VARIANT_PARTS);

    return m_data.variant_parts;
  }

  void set_variant_parts (gdb::array_view<variant_part> *variant_parts)
  {
    m_kind = PROP_VARIANT_PARTS;
    m_data.variant_parts = variant_parts;
  }

  struct type *original_type () const
  {
    gdb_assert (m_kind == PROP_TYPE);

    return m_data.original_type;
  }

  void set_original_type (struct type *original_type)
  {
    m_kind = PROP_TYPE;
    m_data.original_type = original_type;
  }

  /* Return the name of the variable that holds this property's value.
     Only valid for PROP_VARIABLE_NAME.  */
  const char *variable_name () const
  {
    gdb_assert (m_kind == PROP_VARIABLE_NAME);
    return m_data.variable_name;
  }

  /* Set the name of the variable that holds this property's value,
     and set this property to be of kind PROP_VARIABLE_NAME.  */
  void set_variable_name (const char *name)
  {
    m_kind = PROP_VARIABLE_NAME;
    m_data.variable_name = name;
  }

  /* Determine which field of the union dynamic_prop.data is used.  */
  enum dynamic_prop_kind m_kind;

  /* Storage for dynamic or static value.  */
  union dynamic_prop_data m_data;
};

/* Compare two dynamic_prop objects for equality.  dynamic_prop
   instances are equal iff they have the same type and storage.  */
extern bool operator== (const dynamic_prop &l, const dynamic_prop &r);

/* Compare two dynamic_prop objects for inequality.  */
static inline bool operator!= (const dynamic_prop &l, const dynamic_prop &r)
{
  return !(l == r);
}

/* * Define a type's dynamic property node kind.  */
enum dynamic_prop_node_kind
{
  /* A property providing a type's data location.
     Evaluating this field yields to the location of an object's data.  */
  DYN_PROP_DATA_LOCATION,

  /* A property representing DW_AT_allocated.  The presence of this attribute
     indicates that the object of the type can be allocated/deallocated.  */
  DYN_PROP_ALLOCATED,

  /* A property representing DW_AT_associated.  The presence of this attribute
     indicated that the object of the type can be associated.  */
  DYN_PROP_ASSOCIATED,

  /* A property providing an array's byte stride.  */
  DYN_PROP_BYTE_STRIDE,

  /* A property holding variant parts.  */
  DYN_PROP_VARIANT_PARTS,

  /* A property representing DW_AT_rank. The presence of this attribute
     indicates that the object is of assumed rank array type.  */
  DYN_PROP_RANK,

  /* A property holding the size of the type.  */
  DYN_PROP_BYTE_SIZE,
};

/* * List for dynamic type attributes.  */
struct dynamic_prop_list
{
  /* The kind of dynamic prop in this node.  */
  enum dynamic_prop_node_kind prop_kind;

  /* The dynamic property itself.  */
  struct dynamic_prop prop;

  /* A pointer to the next dynamic property.  */
  struct dynamic_prop_list *next;
};

/* * Determine which field of the union main_type.fields[x].loc is
   used.  */

enum field_loc_kind
  {
    FIELD_LOC_KIND_BITPOS,	/**< bitpos */
    FIELD_LOC_KIND_ENUMVAL,	/**< enumval */
    FIELD_LOC_KIND_PHYSADDR,	/**< physaddr */
    FIELD_LOC_KIND_PHYSNAME,	/**< physname */
    FIELD_LOC_KIND_DWARF_BLOCK	/**< dwarf_block */
  };

/* * A discriminant to determine which field in the
   main_type.type_specific union is being used, if any.

   For types such as TYPE_CODE_FLT, the use of this
   discriminant is really redundant, as we know from the type code
   which field is going to be used.  As such, it would be possible to
   reduce the size of this enum in order to save a bit or two for
   other fields of struct main_type.  But, since we still have extra
   room , and for the sake of clarity and consistency, we treat all fields
   of the union the same way.  */

enum type_specific_kind
{
  TYPE_SPECIFIC_NONE,
  TYPE_SPECIFIC_CPLUS_STUFF,
  TYPE_SPECIFIC_GNAT_STUFF,
  TYPE_SPECIFIC_FLOATFORMAT,
  /* Note: This is used by TYPE_CODE_FUNC and TYPE_CODE_METHOD.  */
  TYPE_SPECIFIC_FUNC,
  TYPE_SPECIFIC_SELF_TYPE,
  TYPE_SPECIFIC_INT,
  TYPE_SPECIFIC_FIXED_POINT,
};

union type_owner
{
  struct objfile *objfile;
  struct gdbarch *gdbarch;
};

union field_location
{
  /* * Position of this field, counting in bits from start of
     containing structure.  For big-endian targets, it is the bit
     offset to the MSB.  For little-endian targets, it is the bit
     offset to the LSB.  */

  LONGEST bitpos;

  /* * Enum value.  */
  LONGEST enumval;

  /* * For a static field, if TYPE_FIELD_STATIC_HAS_ADDR then
     physaddr is the location (in the target) of the static
     field.  Otherwise, physname is the mangled label of the
     static field.  */

  CORE_ADDR physaddr;
  const char *physname;

  /* * The field location can be computed by evaluating the
     following DWARF block.  Its DATA is allocated on
     objfile_obstack - no CU load is needed to access it.  */

  struct dwarf2_locexpr_baton *dwarf_block;
};

/* Accessibility of a member.  */
enum class accessibility : unsigned char
{
  /* It's important that this be 0 so that fields default to
     public.  */
  PUBLIC = 0,
  PROTECTED = 1,
  PRIVATE = 2,
};

struct field
{
  struct type *type () const
  {
    return this->m_type;
  }

  void set_type (struct type *type)
  {
    this->m_type = type;
  }

  const char *name () const
  {
    return m_name;
  }

  void set_name (const char *name)
  {
    m_name = name;
  }

  bool is_artificial () const
  {
    return m_artificial;
  }

  void set_is_artificial (bool is_artificial)
  {
    m_artificial = is_artificial;
  }

  unsigned int bitsize () const
  {
    return m_bitsize;
  }

  void set_bitsize (unsigned int bitsize)
  {
    m_bitsize = bitsize;
  }

  bool is_packed () const
  {
    return m_bitsize != 0;
  }

  /* Return true if this field is static; false if not.  */
  bool is_static () const
  {
    /* "static" fields are the fields whose location is not relative
       to the address of the enclosing struct.  It would be nice to
       have a dedicated flag that would be set for static fields when
       the type is being created.  But in practice, checking the field
       loc_kind should give us an accurate answer.  */
    return (m_loc_kind == FIELD_LOC_KIND_PHYSNAME
	    || m_loc_kind == FIELD_LOC_KIND_PHYSADDR);
  }

  /* Location getters / setters.  */

  field_loc_kind loc_kind () const
  {
    return m_loc_kind;
  }

  LONGEST loc_bitpos () const
  {
    gdb_assert (m_loc_kind == FIELD_LOC_KIND_BITPOS);
    return m_loc.bitpos;
  }

  void set_loc_bitpos (LONGEST bitpos)
  {
    m_loc_kind = FIELD_LOC_KIND_BITPOS;
    m_loc.bitpos = bitpos;
  }

  LONGEST loc_enumval () const
  {
    gdb_assert (m_loc_kind == FIELD_LOC_KIND_ENUMVAL);
    return m_loc.enumval;
  }

  void set_loc_enumval (LONGEST enumval)
  {
    m_loc_kind = FIELD_LOC_KIND_ENUMVAL;
    m_loc.enumval = enumval;
  }

  CORE_ADDR loc_physaddr () const
  {
    gdb_assert (m_loc_kind == FIELD_LOC_KIND_PHYSADDR);
    return m_loc.physaddr;
  }

  void set_loc_physaddr (CORE_ADDR physaddr)
  {
    m_loc_kind = FIELD_LOC_KIND_PHYSADDR;
    m_loc.physaddr = physaddr;
  }

  const char *loc_physname () const
  {
    gdb_assert (m_loc_kind == FIELD_LOC_KIND_PHYSNAME);
    return m_loc.physname;
  }

  void set_loc_physname (const char *physname)
  {
    m_loc_kind = FIELD_LOC_KIND_PHYSNAME;
    m_loc.physname = physname;
  }

  dwarf2_locexpr_baton *loc_dwarf_block () const
  {
    gdb_assert (m_loc_kind == FIELD_LOC_KIND_DWARF_BLOCK);
    return m_loc.dwarf_block;
  }

  void set_loc_dwarf_block (dwarf2_locexpr_baton *dwarf_block)
  {
    m_loc_kind = FIELD_LOC_KIND_DWARF_BLOCK;
    m_loc.dwarf_block = dwarf_block;
  }

  /* Set the field's accessibility.  */
  void set_accessibility (accessibility acc)
  { m_accessibility = acc; }

  /* Fetch the field's accessibility.  */
  enum accessibility accessibility () const
  { return m_accessibility; }

  /* True if this field is 'public'.  */
  bool is_public () const
  { return m_accessibility == accessibility::PUBLIC; }

  /* True if this field is 'private'.  */
  bool is_private () const
  { return m_accessibility == accessibility::PRIVATE; }

  /* True if this field is 'protected'.  */
  bool is_protected () const
  { return m_accessibility == accessibility::PROTECTED; }

  /* True if this field is 'virtual'.  */
  bool is_virtual () const
  { return m_virtual; }

  /* Set the field's "virtual" flag.  */
  void set_virtual ()
  { m_virtual = true; }

  /* True if this field is 'ignored'.  */
  bool is_ignored () const
  { return m_ignored; }

  /* Set the field's "ignored" flag.  Note that the 'ignored' bit is
     deprecated.  It was used by some unknown stabs generator, and has
     been replaced by the optimized-out approach -- however, it
     remains because the stabs reader was never updated.  */
  void set_ignored ()
  { m_ignored = true; }

  union field_location m_loc;

  /* * For a function or member type, this is 1 if the argument is
     marked artificial.  Artificial arguments should not be shown
     to the user.  For TYPE_CODE_RANGE it is set if the specific
     bound is not defined.  */

  unsigned int m_artificial : 1;

  /* Whether the field is 'virtual'.  */
  bool m_virtual : 1;
  /* Whether the field is 'ignored'.  */
  bool m_ignored : 1;

  /* * Discriminant for union field_location.  */

  ENUM_BITFIELD(field_loc_kind) m_loc_kind : 3;

  /* Accessibility of the field.  */
  enum accessibility m_accessibility;

  /* * Size of this field, in bits, or zero if not packed.
     If non-zero in an array type, indicates the element size in
     bits (used only in Ada at the moment).
     For an unpacked field, the field's type's length
     says how many bytes the field occupies.  */

  unsigned int m_bitsize;

  /* * In a struct or union type, type of this field.
     - In a function or member type, type of this argument.
     - In an array type, the domain-type of the array.  */

  struct type *m_type;

  /* * Name of field, value or argument.
     NULL for range bounds, array domains, and member function
     arguments.  */

  const char *m_name;
};

struct range_bounds
{
  ULONGEST bit_stride () const
  {
    if (this->flag_is_byte_stride)
      return this->stride.const_val () * 8;
    else
      return this->stride.const_val ();
  }

  /* * Low bound of range.  */

  struct dynamic_prop low;

  /* * High bound of range.  */

  struct dynamic_prop high;

  /* The stride value for this range.  This can be stored in bits or bytes
     based on the value of BYTE_STRIDE_P.  It is optional to have a stride
     value, if this range has no stride value defined then this will be set
     to the constant zero.  */

  struct dynamic_prop stride;

  /* * The bias.  Sometimes a range value is biased before storage.
     The bias is added to the stored bits to form the true value.  */

  LONGEST bias;

  /* True if HIGH range bound contains the number of elements in the
     subrange.  This affects how the final high bound is computed.  */

  unsigned int flag_upper_bound_is_count : 1;

  /* True if LOW or/and HIGH are resolved into a static bound from
     a dynamic one.  */

  unsigned int flag_bound_evaluated : 1;

  /* If this is true this STRIDE is in bytes, otherwise STRIDE is in bits.  */

  unsigned int flag_is_byte_stride : 1;
};

/* Compare two range_bounds objects for equality.  Simply does
   memberwise comparison.  */
extern bool operator== (const range_bounds &l, const range_bounds &r);

/* Compare two range_bounds objects for inequality.  */
static inline bool operator!= (const range_bounds &l, const range_bounds &r)
{
  return !(l == r);
}

union type_specific
{
  /* * CPLUS_STUFF is for TYPE_CODE_STRUCT.  It is initialized to
     point to cplus_struct_default, a default static instance of a
     struct cplus_struct_type.  */

  struct cplus_struct_type *cplus_stuff;

  /* * GNAT_STUFF is for types for which the GNAT Ada compiler
     provides additional information.  */

  struct gnat_aux_type *gnat_stuff;

  /* * FLOATFORMAT is for TYPE_CODE_FLT.  It is a pointer to a
     floatformat object that describes the floating-point value
     that resides within the type.  */

  const struct floatformat *floatformat;

  /* * For TYPE_CODE_FUNC and TYPE_CODE_METHOD types.  */

  struct func_type *func_stuff;

  /* * For types that are pointer to member types (TYPE_CODE_METHODPTR,
     TYPE_CODE_MEMBERPTR), SELF_TYPE is the type that this pointer
     is a member of.  */

  struct type *self_type;

  /* * For TYPE_CODE_FIXED_POINT types, the info necessary to decode
     values of that type.  */
  struct fixed_point_type_info *fixed_point_info;

  /* * An integer-like scalar type may be stored in just part of its
     enclosing storage bytes.  This structure describes this
     situation.  */
  struct
  {
    /* * The bit size of the integer.  This can be 0.  For integers
       that fill their storage (the ordinary case), this field holds
       the byte size times 8.  */
    unsigned short bit_size;
    /* * The bit offset of the integer.  This is ordinarily 0, and can
       only be non-zero if the bit size is less than the storage
       size.  */
    unsigned short bit_offset;
  } int_stuff;
};

/* * Main structure representing a type in GDB.

   This structure is space-critical.  Its layout has been tweaked to
   reduce the space used.  */

struct main_type
{
  /* * Code for kind of type.  */

  ENUM_BITFIELD(type_code) code : 8;

  /* * Flags about this type.  These fields appear at this location
     because they packs nicely here.  See the TYPE_* macros for
     documentation about these fields.  */

  unsigned int m_flag_unsigned : 1;
  unsigned int m_flag_nosign : 1;
  unsigned int m_flag_stub : 1;
  unsigned int m_flag_target_stub : 1;
  unsigned int m_flag_prototyped : 1;
  unsigned int m_flag_varargs : 1;
  unsigned int m_flag_vector : 1;
  unsigned int m_flag_stub_supported : 1;
  unsigned int m_flag_gnu_ifunc : 1;
  unsigned int m_flag_fixed_instance : 1;
  unsigned int m_flag_objfile_owned : 1;
  unsigned int m_flag_endianity_not_default : 1;

  /* * True if this type was declared with "class" rather than
     "struct".  */

  unsigned int m_flag_declared_class : 1;

  /* * True if this is an enum type with disjoint values.  This
     affects how the enum is printed.  */

  unsigned int m_flag_flag_enum : 1;

  /* * For TYPE_CODE_ARRAY, this is true if this type is part of a
     multi-dimensional array.  Multi-dimensional arrays are
     represented internally as arrays of arrays, and this flag lets
     gdb distinguish between multiple dimensions and an ordinary array
     of arrays.  The flag is set on each inner dimension, but not the
     outermost dimension.  */

  unsigned int m_multi_dimensional : 1;

  /* * A discriminant telling us which field of the type_specific
     union is being used for this type, if any.  */

  ENUM_BITFIELD(type_specific_kind) type_specific_field : 3;

  /* The language for this type.  */

  ENUM_BITFIELD(language) m_lang : LANGUAGE_BITS;

  /* * Number of fields described for this type.  This field appears
     at this location because it packs nicely here.  */

  unsigned int m_nfields;

  /* * Name of this type, or NULL if none.

     This is used for printing only.  For looking up a name, look for
     a symbol in the VAR_DOMAIN.  This is generally allocated in the
     objfile's obstack.  However coffread.c uses malloc.  */

  const char *name;

  /* * Every type is now associated with a particular objfile, and the
     type is allocated on the objfile_obstack for that objfile.  One
     problem however, is that there are times when gdb allocates new
     types while it is not in the process of reading symbols from a
     particular objfile.  Fortunately, these happen when the type
     being created is a derived type of an existing type, such as in
     lookup_pointer_type().  So we can just allocate the new type
     using the same objfile as the existing type, but to do this we
     need a backpointer to the objfile from the existing type.  Yes
     this is somewhat ugly, but without major overhaul of the internal
     type system, it can't be avoided for now.  */

  union type_owner m_owner;

  /* * For a pointer type, describes the type of object pointed to.
     - For an array type, describes the type of the elements.
     - For a function or method type, describes the type of the return value.
     - For a range type, describes the type of the full range.
     - For a complex type, describes the type of each coordinate.
     - For a special record or union type encoding a dynamic-sized type
     in GNAT, a memoized pointer to a corresponding static version of
     the type.
     - Unused otherwise.  */

  struct type *m_target_type;

  /* * For structure and union types, a description of each field.
     For set and pascal array types, there is one "field",
     whose type is the domain type of the set or array.
     For range types, there are two "fields",
     the minimum and maximum values (both inclusive).
     For enum types, each possible value is described by one "field".
     For a function or method type, a "field" for each parameter.
     For C++ classes, there is one field for each base class (if it is
     a derived class) plus one field for each class data member.  Member
     functions are recorded elsewhere.

     Using a pointer to a separate array of fields
     allows all types to have the same size, which is useful
     because we can allocate the space for a type before
     we know what to put in it.  */

  union 
  {
    struct field *fields;

    /* * Union member used for range types.  */

    struct range_bounds *bounds;

    /* If this is a scalar type, then this is its corresponding
       complex type.  */
    struct type *complex_type;

  } flds_bnds;

  /* * Slot to point to additional language-specific fields of this
     type.  */

  union type_specific type_specific;

  /* * Contains all dynamic type properties.  */
  struct dynamic_prop_list *dyn_prop_list;
};

/* * Number of bits allocated for alignment.  */

#define TYPE_ALIGN_BITS 8

/* * A ``struct type'' describes a particular instance of a type, with
   some particular qualification.  */

struct type
{
  /* Get the type code of this type. 

     Note that the code can be TYPE_CODE_TYPEDEF, so if you want the real
     type, you need to do `check_typedef (type)->code ()`.  */
  type_code code () const
  {
    return this->main_type->code;
  }

  /* Set the type code of this type.  */
  void set_code (type_code code)
  {
    this->main_type->code = code;
  }

  /* Get the name of this type.  */
  const char *name () const
  {
    return this->main_type->name;
  }

  /* Set the name of this type.  */
  void set_name (const char *name)
  {
    this->main_type->name = name;
  }

  /* Note that if thistype is a TYPEDEF type, you have to call check_typedef.
     But check_typedef does set the TYPE_LENGTH of the TYPEDEF type,
     so you only have to call check_typedef once.  Since value::allocate
     calls check_typedef, X->type ()->length () is safe.  */
  ULONGEST length () const
  {
    return this->m_length;
  }

  void set_length (ULONGEST length)
  {
    this->m_length = length;
  }

  /* Get the number of fields of this type.  */
  unsigned int num_fields () const
  {
    return this->main_type->m_nfields;
  }

  /* Set the number of fields of this type.  */
  void set_num_fields (unsigned int num_fields)
  {
    this->main_type->m_nfields = num_fields;
  }

  /* Get the fields array of this type.  */
  struct field *fields () const
  {
    return this->main_type->flds_bnds.fields;
  }

  /* Get the field at index IDX.  */
  struct field &field (int idx) const
  {
    gdb_assert (idx >= 0 && idx < num_fields ());
    return this->fields ()[idx];
  }

  /* Set the fields array of this type.  */
  void set_fields (struct field *fields)
  {
    this->main_type->flds_bnds.fields = fields;
  }

  /* Allocate the fields array of this type, with NFIELDS elements.  If INIT,
     zero-initialize the allocated memory.  */
  void alloc_fields (unsigned int nfields, bool init = true);

  /* Allocate the fields array of this type, and copy the fields from SRC.  */
  void copy_fields (struct type *src);
  void copy_fields (std::vector<struct field> &src);

  type *index_type () const
  {
    return this->field (0).type ();
  }

  struct type *target_type () const
  {
    return this->main_type->m_target_type;
  }

  void set_target_type (struct type *target_type)
  {
    this->main_type->m_target_type = target_type;
  }

  void set_index_type (type *index_type)
  {
    this->field (0).set_type (index_type);
  }

  /* Return the instance flags converted to the correct type.  */
  const type_instance_flags instance_flags () const
  {
    return (enum type_instance_flag_value) this->m_instance_flags;
  }

  /* Set the instance flags.  */
  void set_instance_flags (type_instance_flags flags)
  {
    this->m_instance_flags = flags;
  }

  /* Get the bounds bounds of this type.  The type must be a range type.  */
  range_bounds *bounds () const
  {
    switch (this->code ())
      {
      case TYPE_CODE_RANGE:
	return this->main_type->flds_bnds.bounds;

      case TYPE_CODE_ARRAY:
      case TYPE_CODE_STRING:
	return this->index_type ()->bounds ();

      default:
	gdb_assert_not_reached
	  ("type::bounds called on type with invalid code");
      }
  }

  /* Set the bounds of this type.  The type must be a range type.  */
  void set_bounds (range_bounds *bounds)
  {
    gdb_assert (this->code () == TYPE_CODE_RANGE);

    this->main_type->flds_bnds.bounds = bounds;
  }

  ULONGEST bit_stride () const
  {
    if (this->code () == TYPE_CODE_ARRAY && this->field (0).bitsize () != 0)
      return this->field (0).bitsize ();
    return this->bounds ()->bit_stride ();
  }

  /* Unsigned integer type.  If this is not set for a TYPE_CODE_INT,
     the type is signed (unless TYPE_NOSIGN is set).  */

  bool is_unsigned () const
  {
    return this->main_type->m_flag_unsigned;
  }

  void set_is_unsigned (bool is_unsigned)
  {
    this->main_type->m_flag_unsigned = is_unsigned;
  }

  /* No sign for this type.  In C++, "char", "signed char", and
     "unsigned char" are distinct types; so we need an extra flag to
     indicate the absence of a sign!  */

  bool has_no_signedness () const
  {
    return this->main_type->m_flag_nosign;
  }

  void set_has_no_signedness (bool has_no_signedness)
  {
    this->main_type->m_flag_nosign = has_no_signedness;
  }

  /* This appears in a type's flags word if it is a stub type (e.g.,
     if someone referenced a type that wasn't defined in a source file
     via (struct sir_not_appearing_in_this_film *)).  */

  bool is_stub () const
  {
    return this->main_type->m_flag_stub;
  }

  void set_is_stub (bool is_stub)
  {
    this->main_type->m_flag_stub = is_stub;
  }

  /* The target type of this type is a stub type, and this type needs
     to be updated if it gets un-stubbed in check_typedef.  Used for
     arrays and ranges, in which TYPE_LENGTH of the array/range gets set
     based on the TYPE_LENGTH of the target type.  Also, set for
     TYPE_CODE_TYPEDEF.  */

  bool target_is_stub () const
  {
    return this->main_type->m_flag_target_stub;
  }

  void set_target_is_stub (bool target_is_stub)
  {
    this->main_type->m_flag_target_stub = target_is_stub;
  }

  /* This is a function type which appears to have a prototype.  We
     need this for function calls in order to tell us if it's necessary
     to coerce the args, or to just do the standard conversions.  This
     is used with a short field.  */

  bool is_prototyped () const
  {
    return this->main_type->m_flag_prototyped;
  }

  void set_is_prototyped (bool is_prototyped)
  {
    this->main_type->m_flag_prototyped = is_prototyped;
  }

  /* FIXME drow/2002-06-03:  Only used for methods, but applies as well
     to functions.  */

  bool has_varargs () const
  {
    return this->main_type->m_flag_varargs;
  }

  void set_has_varargs (bool has_varargs)
  {
    this->main_type->m_flag_varargs = has_varargs;
  }

  /* Identify a vector type.  Gcc is handling this by adding an extra
     attribute to the array type.  We slurp that in as a new flag of a
     type.  This is used only in dwarf2read.c.  */

  bool is_vector () const
  {
    return this->main_type->m_flag_vector;
  }

  void set_is_vector (bool is_vector)
  {
    this->main_type->m_flag_vector = is_vector;
  }

  /* This debug target supports TYPE_STUB(t).  In the unsupported case
     we have to rely on NFIELDS to be zero etc., see TYPE_IS_OPAQUE().
     TYPE_STUB(t) with !TYPE_STUB_SUPPORTED(t) may exist if we only
     guessed the TYPE_STUB(t) value (see dwarfread.c).  */

  bool stub_is_supported () const
  {
    return this->main_type->m_flag_stub_supported;
  }

  void set_stub_is_supported (bool stub_is_supported)
  {
    this->main_type->m_flag_stub_supported = stub_is_supported;
  }

  /* Used only for TYPE_CODE_FUNC where it specifies the real function
     address is returned by this function call.  The target_type method
     determines the final returned function type to be presented to
     user.  */

  bool is_gnu_ifunc () const
  {
    return this->main_type->m_flag_gnu_ifunc;
  }

  void set_is_gnu_ifunc (bool is_gnu_ifunc)
  {
    this->main_type->m_flag_gnu_ifunc = is_gnu_ifunc;
  }

  /* The debugging formats (especially STABS) do not contain enough
     information to represent all Ada types---especially those whose
     size depends on dynamic quantities.  Therefore, the GNAT Ada
     compiler includes extra information in the form of additional type
     definitions connected by naming conventions.  This flag indicates
     that the type is an ordinary (unencoded) GDB type that has been
     created from the necessary run-time information, and does not need
     further interpretation.  Optionally marks ordinary, fixed-size GDB
     type.  */

  bool is_fixed_instance () const
  {
    return this->main_type->m_flag_fixed_instance;
  }

  void set_is_fixed_instance (bool is_fixed_instance)
  {
    this->main_type->m_flag_fixed_instance = is_fixed_instance;
  }

  /* A compiler may supply dwarf instrumentation that indicates the desired
     endian interpretation of the variable differs from the native endian
     representation. */

  bool endianity_is_not_default () const
  {
    return this->main_type->m_flag_endianity_not_default;
  }

  void set_endianity_is_not_default (bool endianity_is_not_default)
  {
    this->main_type->m_flag_endianity_not_default = endianity_is_not_default;
  }


  /* True if this type was declared using the "class" keyword.  This is
     only valid for C++ structure and enum types.  If false, a structure
     was declared as a "struct"; if true it was declared "class".  For
     enum types, this is true when "enum class" or "enum struct" was
     used to declare the type.  */

  bool is_declared_class () const
  {
    return this->main_type->m_flag_declared_class;
  }

  void set_is_declared_class (bool is_declared_class) const
  {
    this->main_type->m_flag_declared_class = is_declared_class;
  }

  /* True if this type is a "flag" enum.  A flag enum is one where all
     the values are pairwise disjoint when "and"ed together.  This
     affects how enum values are printed.  */

  bool is_flag_enum () const
  {
    return this->main_type->m_flag_flag_enum;
  }

  void set_is_flag_enum (bool is_flag_enum)
  {
    this->main_type->m_flag_flag_enum = is_flag_enum;
  }

  /* True if this array type is part of a multi-dimensional array.  */

  bool is_multi_dimensional () const
  {
    return this->main_type->m_multi_dimensional;
  }

  void set_is_multi_dimensional (bool value)
  {
    this->main_type->m_multi_dimensional = value;
  }

  /* * Assuming that THIS is a TYPE_CODE_FIXED_POINT, return a reference
     to this type's fixed_point_info.  */

  struct fixed_point_type_info &fixed_point_info () const
  {
    gdb_assert (this->code () == TYPE_CODE_FIXED_POINT);
    gdb_assert (this->main_type->type_specific.fixed_point_info != nullptr);

    return *this->main_type->type_specific.fixed_point_info;
  }

  /* * Assuming that THIS is a TYPE_CODE_FIXED_POINT, set this type's
     fixed_point_info to INFO.  */

  void set_fixed_point_info (struct fixed_point_type_info *info) const
  {
    gdb_assert (this->code () == TYPE_CODE_FIXED_POINT);

    this->main_type->type_specific.fixed_point_info = info;
  }

  /* * Assuming that THIS is a TYPE_CODE_FIXED_POINT, return its base type.

     In other words, this returns the type after having peeled all
     intermediate type layers (such as TYPE_CODE_RANGE, for instance).
     The TYPE_CODE of the type returned is guaranteed to be
     a TYPE_CODE_FIXED_POINT.  */

  struct type *fixed_point_type_base_type ();

  /* * Assuming that THIS is a TYPE_CODE_FIXED_POINT, return its scaling
     factor.  */

  const gdb_mpq &fixed_point_scaling_factor ();

  /* * Return the dynamic property of the requested KIND from this type's
     list of dynamic properties.  */
  dynamic_prop *dyn_prop (dynamic_prop_node_kind kind) const;

  /* * Given a dynamic property PROP of a given KIND, add this dynamic
     property to this type.

     This function assumes that this type is objfile-owned.  */
  void add_dyn_prop (dynamic_prop_node_kind kind, dynamic_prop prop);

  /* * Remove dynamic property of kind KIND from this type, if it exists.  */
  void remove_dyn_prop (dynamic_prop_node_kind kind);

  /* Return true if this type is owned by an objfile.  Return false if it is
     owned by an architecture.  */
  bool is_objfile_owned () const
  {
    return this->main_type->m_flag_objfile_owned;
  }

  /* Set the owner of the type to be OBJFILE.  */
  void set_owner (objfile *objfile)
  {
    gdb_assert (objfile != nullptr);

    this->main_type->m_owner.objfile = objfile;
    this->main_type->m_flag_objfile_owned = true;
  }

  /* Set the owner of the type to be ARCH.  */
  void set_owner (gdbarch *arch)
  {
    gdb_assert (arch != nullptr);

    this->main_type->m_owner.gdbarch = arch;
    this->main_type->m_flag_objfile_owned = false;
  }

  /* Return the objfile owner of this type.

     Return nullptr if this type is not objfile-owned.  */
  struct objfile *objfile_owner () const
  {
    if (!this->is_objfile_owned ())
      return nullptr;

    return this->main_type->m_owner.objfile;
  }

  /* Return the gdbarch owner of this type.

     Return nullptr if this type is not gdbarch-owned.  */
  gdbarch *arch_owner () const
  {
    if (this->is_objfile_owned ())
      return nullptr;

    return this->main_type->m_owner.gdbarch;
  }

  /* Return the type's architecture.  For types owned by an
     architecture, that architecture is returned.  For types owned by an
     objfile, that objfile's architecture is returned.

     The return value is always non-nullptr.  */
  gdbarch *arch () const;

  /* * Return true if this is an integer type whose logical (bit) size
     differs from its storage size; false otherwise.  Always return
     false for non-integer (i.e., non-TYPE_SPECIFIC_INT) types.  */
  bool bit_size_differs_p () const
  {
    return (main_type->type_specific_field == TYPE_SPECIFIC_INT
	    && main_type->type_specific.int_stuff.bit_size != 8 * length ());
  }

  /* * Return the logical (bit) size for this integer type.  Only
     valid for integer (TYPE_SPECIFIC_INT) types.  */
  unsigned short bit_size () const
  {
    gdb_assert (main_type->type_specific_field == TYPE_SPECIFIC_INT);
    return main_type->type_specific.int_stuff.bit_size;
  }

  /* * Return the bit offset for this integer type.  Only valid for
     integer (TYPE_SPECIFIC_INT) types.  */
  unsigned short bit_offset () const
  {
    gdb_assert (main_type->type_specific_field == TYPE_SPECIFIC_INT);
    return main_type->type_specific.int_stuff.bit_offset;
  }

  /* Return true if this is a pointer or reference type.  */
  bool is_pointer_or_reference () const
  {
    return this->code () == TYPE_CODE_PTR || TYPE_IS_REFERENCE (this);
  }

  /* Return true if this type is "string-like", according to its
     defining language.  */
  bool is_string_like ();

  /* Return true if this type is "array-like".  This includes arrays,
     but also some forms of structure type that are recognized as
     representations of arrays by the type's language.  */
  bool is_array_like ();

  /* Return the language that this type came from.  */
  enum language language () const
  { return main_type->m_lang; }

  /* * Type that is a pointer to this type.
     NULL if no such pointer-to type is known yet.
     The debugger may add the address of such a type
     if it has to construct one later.  */

  struct type *pointer_type;

  /* * C++: also need a reference type.  */

  struct type *reference_type;

  /* * A C++ rvalue reference type added in C++11. */

  struct type *rvalue_reference_type;

  /* * Variant chain.  This points to a type that differs from this
     one only in qualifiers and length.  Currently, the possible
     qualifiers are const, volatile, code-space, data-space, and
     address class.  The length may differ only when one of the
     address class flags are set.  The variants are linked in a
     circular ring and share MAIN_TYPE.  */

  struct type *chain;

  /* * The alignment for this type.  Zero means that the alignment was
     not specified in the debug info.  Note that this is stored in a
     funny way: as the log base 2 (plus 1) of the alignment; so a
     value of 1 means the alignment is 1, and a value of 9 means the
     alignment is 256.  */

  unsigned align_log2 : TYPE_ALIGN_BITS;

  /* * Flags specific to this instance of the type, indicating where
     on the ring we are.

     For TYPE_CODE_TYPEDEF the flags of the typedef type should be
     binary or-ed with the target type, with a special case for
     address class and space class.  For example if this typedef does
     not specify any new qualifiers, TYPE_INSTANCE_FLAGS is 0 and the
     instance flags are completely inherited from the target type.  No
     qualifiers can be cleared by the typedef.  See also
     check_typedef.  */
  unsigned m_instance_flags : 9;

  /* * Length of storage for a value of this type.  The value is the
     expression in host bytes of what sizeof(type) would return.  This
     size includes padding.  For example, an i386 extended-precision
     floating point value really only occupies ten bytes, but most
     ABI's declare its size to be 12 bytes, to preserve alignment.
     A `struct type' representing such a floating-point type would
     have a `length' value of 12, even though the last two bytes are
     unused.

     Since this field is expressed in host bytes, its value is appropriate
     to pass to memcpy and such (it is assumed that GDB itself always runs
     on an 8-bits addressable architecture).  However, when using it for
     target address arithmetic (e.g. adding it to a target address), the
     type_length_units function should be used in order to get the length
     expressed in target addressable memory units.  */

  ULONGEST m_length;

  /* * Core type, shared by a group of qualified types.  */

  struct main_type *main_type;
};

struct fn_fieldlist
{

  /* * The overloaded name.
     This is generally allocated in the objfile's obstack.
     However stabsread.c sometimes uses malloc.  */

  const char *name;

  /* * The number of methods with this name.  */

  int length;

  /* * The list of methods.  */

  struct fn_field *fn_fields;
};



struct fn_field
{
  /* * If is_stub is clear, this is the mangled name which we can look
     up to find the address of the method (FIXME: it would be cleaner
     to have a pointer to the struct symbol here instead).

     If is_stub is set, this is the portion of the mangled name which
     specifies the arguments.  For example, "ii", if there are two int
     arguments, or "" if there are no arguments.  See gdb_mangle_name
     for the conversion from this format to the one used if is_stub is
     clear.  */

  const char *physname;

  /* * The function type for the method.
	       
     (This comment used to say "The return value of the method", but
     that's wrong.  The function type is expected here, i.e. something
     with TYPE_CODE_METHOD, and *not* the return-value type).  */

  struct type *type;

  /* * For virtual functions.  First baseclass that defines this
     virtual function.  */

  struct type *fcontext;

  /* Attributes.  */

  unsigned int is_const:1;
  unsigned int is_volatile:1;
  unsigned int is_artificial:1;

  /* * A stub method only has some fields valid (but they are enough
     to reconstruct the rest of the fields).  */

  unsigned int is_stub:1;

  /* * True if this function is a constructor, false otherwise.  */

  unsigned int is_constructor : 1;

  /* * True if this function is deleted, false otherwise.  */

  unsigned int is_deleted : 1;

  /* * DW_AT_defaulted attribute for this function.  The value is one
     of the DW_DEFAULTED constants.  */

  ENUM_BITFIELD (dwarf_defaulted_attribute) defaulted : 2;

  /* Accessibility of the field.  */
  enum accessibility accessibility;

  /* * Index into that baseclass's virtual function table, minus 2;
     else if static: VOFFSET_STATIC; else: 0.  */

  unsigned int voffset:16;

#define VOFFSET_STATIC 1

};

struct decl_field
{
  /* * Unqualified name to be prefixed by owning class qualified
     name.  */

  const char *name;

  /* * Type this typedef named NAME represents.  */

  struct type *type;

  /* Accessibility of the field.  */
  enum accessibility accessibility;
};

/* * C++ language-specific information for TYPE_CODE_STRUCT and
   TYPE_CODE_UNION nodes.  */

struct cplus_struct_type
  {
    /* * Number of base classes this type derives from.  The
       baseclasses are stored in the first N_BASECLASSES fields
       (i.e. the `fields' field of the struct type).  The only fields
       of struct field that are used are: type, name, loc.bitpos.  */

    short n_baseclasses;

    /* * Field number of the virtual function table pointer in VPTR_BASETYPE.
       All access to this field must be through TYPE_VPTR_FIELDNO as one
       thing it does is check whether the field has been initialized.
       Initially TYPE_RAW_CPLUS_SPECIFIC has the value of cplus_struct_default,
       which for portability reasons doesn't initialize this field.
       TYPE_VPTR_FIELDNO returns -1 for this case.

       If -1, we were unable to find the virtual function table pointer in
       initial symbol reading, and get_vptr_fieldno should be called to find
       it if possible.  get_vptr_fieldno will update this field if possible.
       Otherwise the value is left at -1.

       Unused if this type does not have virtual functions.  */

    short vptr_fieldno;

    /* * Number of methods with unique names.  All overloaded methods
       with the same name count only once.  */

    short nfn_fields;

    /* * Number of template arguments.  */

    unsigned short n_template_arguments;

    /* * One if this struct is a dynamic class, as defined by the
       Itanium C++ ABI: if it requires a virtual table pointer,
       because it or any of its base classes have one or more virtual
       member functions or virtual base classes.  Minus one if not
       dynamic.  Zero if not yet computed.  */

    int is_dynamic : 2;

    /* * The calling convention for this type, fetched from the
       DW_AT_calling_convention attribute.  The value is one of the
       DW_CC constants.  */

    ENUM_BITFIELD (dwarf_calling_convention) calling_convention : 8;

    /* * The base class which defined the virtual function table pointer.  */

    struct type *vptr_basetype;

    /* * For classes, structures, and unions, a description of each
       field, which consists of an overloaded name, followed by the
       types of arguments that the method expects, and then the name
       after it has been renamed to make it distinct.

       fn_fieldlists points to an array of nfn_fields of these.  */

    struct fn_fieldlist *fn_fieldlists;

    /* * typedefs defined inside this class.  typedef_field points to
       an array of typedef_field_count elements.  */

    struct decl_field *typedef_field;

    unsigned typedef_field_count;

    /* * The nested types defined by this type.  nested_types points to
       an array of nested_types_count elements.  */

    struct decl_field *nested_types;

    unsigned nested_types_count;

    /* * The template arguments.  This is an array with
       N_TEMPLATE_ARGUMENTS elements.  This is NULL for non-template
       classes.  */

    struct symbol **template_arguments;
  };

/* * Struct used to store conversion rankings.  */

struct rank
  {
    short rank;

    /* * When two conversions are of the same type and therefore have
       the same rank, subrank is used to differentiate the two.

       Eg: Two derived-class-pointer to base-class-pointer conversions
       would both have base pointer conversion rank, but the
       conversion with the shorter distance to the ancestor is
       preferable.  'subrank' would be used to reflect that.  */

    short subrank;
  };

/* * Used for ranking a function for overload resolution.  */

typedef std::vector<rank> badness_vector;

/* * GNAT Ada-specific information for various Ada types.  */

struct gnat_aux_type
  {
    /* * Parallel type used to encode information about dynamic types
       used in Ada (such as variant records, variable-size array,
       etc).  */
    struct type* descriptive_type;
  };

/* * For TYPE_CODE_FUNC and TYPE_CODE_METHOD types.  */

struct func_type
  {
    /* * The calling convention for targets supporting multiple ABIs.
       Right now this is only fetched from the Dwarf-2
       DW_AT_calling_convention attribute.  The value is one of the
       DW_CC constants.  */

    ENUM_BITFIELD (dwarf_calling_convention) calling_convention : 8;

    /* * Whether this function normally returns to its caller.  It is
       set from the DW_AT_noreturn attribute if set on the
       DW_TAG_subprogram.  */

    unsigned int is_noreturn : 1;

    /* * Only those DW_TAG_call_site's in this function that have
       DW_AT_call_tail_call set are linked in this list.  Function
       without its tail call list complete
       (DW_AT_call_all_tail_calls or its superset
       DW_AT_call_all_calls) has TAIL_CALL_LIST NULL, even if some
       DW_TAG_call_site's exist in such function. */

    struct call_site *tail_call_list;

    /* * For method types (TYPE_CODE_METHOD), the aggregate type that
       contains the method.  */

    struct type *self_type;
  };

/* The type-specific info for TYPE_CODE_FIXED_POINT types.  */

struct fixed_point_type_info
{
  /* The fixed point type's scaling factor.  */
  gdb_mpq scaling_factor;
};

/* * The default value of TYPE_CPLUS_SPECIFIC(T) points to this shared
   static structure.  */

extern const struct cplus_struct_type cplus_struct_default;

extern void allocate_cplus_struct_type (struct type *);

#define INIT_CPLUS_SPECIFIC(type) \
  (TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_CPLUS_STUFF, \
   TYPE_RAW_CPLUS_SPECIFIC (type) = (struct cplus_struct_type*) \
   &cplus_struct_default)

#define ALLOCATE_CPLUS_STRUCT_TYPE(type) allocate_cplus_struct_type (type)

#define HAVE_CPLUS_STRUCT(type) \
  (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_CPLUS_STUFF \
   && TYPE_RAW_CPLUS_SPECIFIC (type) !=  &cplus_struct_default)

#define INIT_NONE_SPECIFIC(type) \
  (TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_NONE, \
   TYPE_MAIN_TYPE (type)->type_specific = {})

extern const struct gnat_aux_type gnat_aux_default;

extern void allocate_gnat_aux_type (struct type *);

#define INIT_GNAT_SPECIFIC(type) \
  (TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_GNAT_STUFF, \
   TYPE_GNAT_SPECIFIC (type) = (struct gnat_aux_type *) &gnat_aux_default)
#define ALLOCATE_GNAT_AUX_TYPE(type) allocate_gnat_aux_type (type)
/* * A macro that returns non-zero if the type-specific data should be
   read as "gnat-stuff".  */
#define HAVE_GNAT_AUX_INFO(type) \
  (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_GNAT_STUFF)

/* * True if TYPE is known to be an Ada type of some kind.  */
#define ADA_TYPE_P(type)					\
  (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_GNAT_STUFF	\
    || (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE	\
	&& (type)->is_fixed_instance ()))

#define INIT_FUNC_SPECIFIC(type)					       \
  (TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_FUNC,			       \
   TYPE_MAIN_TYPE (type)->type_specific.func_stuff = (struct func_type *)      \
     TYPE_ZALLOC (type,							       \
		  sizeof (*TYPE_MAIN_TYPE (type)->type_specific.func_stuff)))

/* "struct fixed_point_type_info" has a field that has a destructor.
   See allocate_fixed_point_type_info to understand how this is
   handled.  */
#define INIT_FIXED_POINT_SPECIFIC(type) \
  (TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_FIXED_POINT, \
   allocate_fixed_point_type_info (type))

#define TYPE_MAIN_TYPE(thistype) (thistype)->main_type
#define TYPE_POINTER_TYPE(thistype) (thistype)->pointer_type
#define TYPE_REFERENCE_TYPE(thistype) (thistype)->reference_type
#define TYPE_RVALUE_REFERENCE_TYPE(thistype) (thistype)->rvalue_reference_type
#define TYPE_CHAIN(thistype) (thistype)->chain

/* * Return the alignment of the type in target addressable memory
   units, or 0 if no alignment was specified.  */
#define TYPE_RAW_ALIGN(thistype) type_raw_align (thistype)

/* * Return the alignment of the type in target addressable memory
   units, or 0 if no alignment was specified.  */
extern unsigned type_raw_align (struct type *);

/* * Return the alignment of the type in target addressable memory
   units.  Return 0 if the alignment cannot be determined; but note
   that this makes an effort to compute the alignment even it it was
   not specified in the debug info.  */
extern unsigned type_align (struct type *);

/* * Set the alignment of the type.  The alignment must be a power of
   2.  Returns false if the given value does not fit in the available
   space in struct type.  */
extern bool set_type_align (struct type *, ULONGEST);

/* Property accessors for the type data location.  */
#define TYPE_DATA_LOCATION(thistype) \
  ((thistype)->dyn_prop (DYN_PROP_DATA_LOCATION))
#define TYPE_DATA_LOCATION_BATON(thistype) \
  TYPE_DATA_LOCATION (thistype)->data.baton
#define TYPE_DATA_LOCATION_ADDR(thistype) \
  (TYPE_DATA_LOCATION (thistype)->const_val ())
#define TYPE_DATA_LOCATION_KIND(thistype) \
  (TYPE_DATA_LOCATION (thistype)->kind ())
#define TYPE_DYNAMIC_LENGTH(thistype) \
  ((thistype)->dyn_prop (DYN_PROP_BYTE_SIZE))

/* Property accessors for the type allocated/associated.  */
#define TYPE_ALLOCATED_PROP(thistype) \
  ((thistype)->dyn_prop (DYN_PROP_ALLOCATED))
#define TYPE_ASSOCIATED_PROP(thistype) \
  ((thistype)->dyn_prop (DYN_PROP_ASSOCIATED))
#define TYPE_RANK_PROP(thistype) \
  ((thistype)->dyn_prop (DYN_PROP_RANK))

/* C++ */

#define TYPE_SELF_TYPE(thistype) internal_type_self_type (thistype)
/* Do not call this, use TYPE_SELF_TYPE.  */
extern struct type *internal_type_self_type (struct type *);
extern void set_type_self_type (struct type *, struct type *);

extern int internal_type_vptr_fieldno (struct type *);
extern void set_type_vptr_fieldno (struct type *, int);
extern struct type *internal_type_vptr_basetype (struct type *);
extern void set_type_vptr_basetype (struct type *, struct type *);
#define TYPE_VPTR_FIELDNO(thistype) internal_type_vptr_fieldno (thistype)
#define TYPE_VPTR_BASETYPE(thistype) internal_type_vptr_basetype (thistype)

#define TYPE_NFN_FIELDS(thistype) TYPE_CPLUS_SPECIFIC(thistype)->nfn_fields
#define TYPE_SPECIFIC_FIELD(thistype) \
  TYPE_MAIN_TYPE(thistype)->type_specific_field
/* We need this tap-dance with the TYPE_RAW_SPECIFIC because of the case
   where we're trying to print an Ada array using the C language.
   In that case, there is no "cplus_stuff", but the C language assumes
   that there is.  What we do, in that case, is pretend that there is
   an implicit one which is the default cplus stuff.  */
#define TYPE_CPLUS_SPECIFIC(thistype) \
   (!HAVE_CPLUS_STRUCT(thistype) \
    ? (struct cplus_struct_type*)&cplus_struct_default \
    : TYPE_RAW_CPLUS_SPECIFIC(thistype))
#define TYPE_RAW_CPLUS_SPECIFIC(thistype) TYPE_MAIN_TYPE(thistype)->type_specific.cplus_stuff
#define TYPE_CPLUS_CALLING_CONVENTION(thistype) \
  TYPE_MAIN_TYPE(thistype)->type_specific.cplus_stuff->calling_convention
#define TYPE_FLOATFORMAT(thistype) TYPE_MAIN_TYPE(thistype)->type_specific.floatformat
#define TYPE_GNAT_SPECIFIC(thistype) TYPE_MAIN_TYPE(thistype)->type_specific.gnat_stuff
#define TYPE_DESCRIPTIVE_TYPE(thistype) TYPE_GNAT_SPECIFIC(thistype)->descriptive_type
#define TYPE_CALLING_CONVENTION(thistype) TYPE_MAIN_TYPE(thistype)->type_specific.func_stuff->calling_convention
#define TYPE_NO_RETURN(thistype) TYPE_MAIN_TYPE(thistype)->type_specific.func_stuff->is_noreturn
#define TYPE_TAIL_CALL_LIST(thistype) TYPE_MAIN_TYPE(thistype)->type_specific.func_stuff->tail_call_list
#define TYPE_BASECLASS(thistype,index) ((thistype)->field (index).type ())
#define TYPE_N_BASECLASSES(thistype) TYPE_CPLUS_SPECIFIC(thistype)->n_baseclasses
#define TYPE_BASECLASS_NAME(thistype,index) (thistype->field (index).name ())
#define TYPE_BASECLASS_BITPOS(thistype,index) (thistype->field (index).loc_bitpos ())
#define BASETYPE_VIA_PUBLIC(thistype, index) \
  ((thistype)->field (index).is_public ())
#define TYPE_CPLUS_DYNAMIC(thistype) TYPE_CPLUS_SPECIFIC (thistype)->is_dynamic

#define BASETYPE_VIA_VIRTUAL(thistype, index) \
  ((thistype)->field (index).is_virtual ())

#define TYPE_FN_FIELDLISTS(thistype) TYPE_CPLUS_SPECIFIC(thistype)->fn_fieldlists
#define TYPE_FN_FIELDLIST(thistype, n) TYPE_CPLUS_SPECIFIC(thistype)->fn_fieldlists[n]
#define TYPE_FN_FIELDLIST1(thistype, n) TYPE_CPLUS_SPECIFIC(thistype)->fn_fieldlists[n].fn_fields
#define TYPE_FN_FIELDLIST_NAME(thistype, n) TYPE_CPLUS_SPECIFIC(thistype)->fn_fieldlists[n].name
#define TYPE_FN_FIELDLIST_LENGTH(thistype, n) TYPE_CPLUS_SPECIFIC(thistype)->fn_fieldlists[n].length

#define TYPE_N_TEMPLATE_ARGUMENTS(thistype) \
  TYPE_CPLUS_SPECIFIC (thistype)->n_template_arguments
#define TYPE_TEMPLATE_ARGUMENTS(thistype) \
  TYPE_CPLUS_SPECIFIC (thistype)->template_arguments
#define TYPE_TEMPLATE_ARGUMENT(thistype, n) \
  TYPE_CPLUS_SPECIFIC (thistype)->template_arguments[n]

#define TYPE_FN_FIELD(thisfn, n) (thisfn)[n]
#define TYPE_FN_FIELD_PHYSNAME(thisfn, n) (thisfn)[n].physname
#define TYPE_FN_FIELD_TYPE(thisfn, n) (thisfn)[n].type
#define TYPE_FN_FIELD_ARGS(thisfn, n) (((thisfn)[n].type)->fields ())
#define TYPE_FN_FIELD_CONST(thisfn, n) ((thisfn)[n].is_const)
#define TYPE_FN_FIELD_VOLATILE(thisfn, n) ((thisfn)[n].is_volatile)
#define TYPE_FN_FIELD_PRIVATE(thisfn, n) \
  ((thisfn)[n].accessibility == accessibility::PRIVATE)
#define TYPE_FN_FIELD_PROTECTED(thisfn, n) \
  ((thisfn)[n].accessibility == accessibility::PROTECTED)
#define TYPE_FN_FIELD_ARTIFICIAL(thisfn, n) ((thisfn)[n].is_artificial)
#define TYPE_FN_FIELD_STUB(thisfn, n) ((thisfn)[n].is_stub)
#define TYPE_FN_FIELD_CONSTRUCTOR(thisfn, n) ((thisfn)[n].is_constructor)
#define TYPE_FN_FIELD_FCONTEXT(thisfn, n) ((thisfn)[n].fcontext)
#define TYPE_FN_FIELD_VOFFSET(thisfn, n) ((thisfn)[n].voffset-2)
#define TYPE_FN_FIELD_VIRTUAL_P(thisfn, n) ((thisfn)[n].voffset > 1)
#define TYPE_FN_FIELD_STATIC_P(thisfn, n) ((thisfn)[n].voffset == VOFFSET_STATIC)
#define TYPE_FN_FIELD_DEFAULTED(thisfn, n) ((thisfn)[n].defaulted)
#define TYPE_FN_FIELD_DELETED(thisfn, n) ((thisfn)[n].is_deleted)

/* Accessors for typedefs defined by a class.  */
#define TYPE_TYPEDEF_FIELD_ARRAY(thistype) \
  TYPE_CPLUS_SPECIFIC (thistype)->typedef_field
#define TYPE_TYPEDEF_FIELD(thistype, n) \
  TYPE_CPLUS_SPECIFIC (thistype)->typedef_field[n]
#define TYPE_TYPEDEF_FIELD_NAME(thistype, n) \
  TYPE_TYPEDEF_FIELD (thistype, n).name
#define TYPE_TYPEDEF_FIELD_TYPE(thistype, n) \
  TYPE_TYPEDEF_FIELD (thistype, n).type
#define TYPE_TYPEDEF_FIELD_COUNT(thistype) \
  TYPE_CPLUS_SPECIFIC (thistype)->typedef_field_count
#define TYPE_TYPEDEF_FIELD_PROTECTED(thistype, n) \
  (TYPE_TYPEDEF_FIELD (thistype, n).accessibility == accessibility::PROTECTED)
#define TYPE_TYPEDEF_FIELD_PRIVATE(thistype, n)        \
  (TYPE_TYPEDEF_FIELD (thistype, n).accessibility == accessibility::PRIVATE)

#define TYPE_NESTED_TYPES_ARRAY(thistype)	\
  TYPE_CPLUS_SPECIFIC (thistype)->nested_types
#define TYPE_NESTED_TYPES_FIELD(thistype, n) \
  TYPE_CPLUS_SPECIFIC (thistype)->nested_types[n]
#define TYPE_NESTED_TYPES_FIELD_NAME(thistype, n) \
  TYPE_NESTED_TYPES_FIELD (thistype, n).name
#define TYPE_NESTED_TYPES_FIELD_TYPE(thistype, n) \
  TYPE_NESTED_TYPES_FIELD (thistype, n).type
#define TYPE_NESTED_TYPES_COUNT(thistype) \
  TYPE_CPLUS_SPECIFIC (thistype)->nested_types_count
#define TYPE_NESTED_TYPES_FIELD_PROTECTED(thistype, n) \
  (TYPE_NESTED_TYPES_FIELD (thistype, n).accessibility \
   == accessibility::PROTECTED)
#define TYPE_NESTED_TYPES_FIELD_PRIVATE(thistype, n)	\
  (TYPE_NESTED_TYPES_FIELD (thistype, n).accessibility \
   == accessibility::PRIVATE)

#define TYPE_IS_OPAQUE(thistype) \
  ((((thistype)->code () == TYPE_CODE_STRUCT) \
    || ((thistype)->code () == TYPE_CODE_UNION)) \
   && ((thistype)->num_fields () == 0) \
   && (!HAVE_CPLUS_STRUCT (thistype) \
       || TYPE_NFN_FIELDS (thistype) == 0) \
   && ((thistype)->is_stub () || !(thistype)->stub_is_supported ()))

/* * A helper macro that returns the name of a type or "unnamed type"
   if the type has no name.  */

#define TYPE_SAFE_NAME(type) \
  (type->name () != nullptr ? type->name () : _("<unnamed type>"))

/* * A helper macro that returns the name of an error type.  If the
   type has a name, it is used; otherwise, a default is used.  */

#define TYPE_ERROR_NAME(type) \
  (type->name () ? type->name () : _("<error type>"))

/* Given TYPE, return its floatformat.  */
const struct floatformat *floatformat_from_type (const struct type *type);

struct builtin_type
{
  /* Integral types.  */

  /* Implicit size/sign (based on the architecture's ABI).  */
  struct type *builtin_void = nullptr;
  struct type *builtin_char = nullptr;
  struct type *builtin_short = nullptr;
  struct type *builtin_int = nullptr;
  struct type *builtin_long = nullptr;
  struct type *builtin_signed_char = nullptr;
  struct type *builtin_unsigned_char = nullptr;
  struct type *builtin_unsigned_short = nullptr;
  struct type *builtin_unsigned_int = nullptr;
  struct type *builtin_unsigned_long = nullptr;
  struct type *builtin_bfloat16 = nullptr;
  struct type *builtin_half = nullptr;
  struct type *builtin_float = nullptr;
  struct type *builtin_double = nullptr;
  struct type *builtin_long_double = nullptr;
  struct type *builtin_complex = nullptr;
  struct type *builtin_double_complex = nullptr;
  struct type *builtin_string = nullptr;
  struct type *builtin_bool = nullptr;
  struct type *builtin_long_long = nullptr;
  struct type *builtin_unsigned_long_long = nullptr;
  struct type *builtin_decfloat = nullptr;
  struct type *builtin_decdouble = nullptr;
  struct type *builtin_declong = nullptr;

  /* "True" character types.
      We use these for the '/c' print format, because c_char is just a
      one-byte integral type, which languages less laid back than C
      will print as ... well, a one-byte integral type.  */
  struct type *builtin_true_char = nullptr;
  struct type *builtin_true_unsigned_char = nullptr;

  /* Explicit sizes - see C9X <intypes.h> for naming scheme.  The "int0"
     is for when an architecture needs to describe a register that has
     no size.  */
  struct type *builtin_int0 = nullptr;
  struct type *builtin_int8 = nullptr;
  struct type *builtin_uint8 = nullptr;
  struct type *builtin_int16 = nullptr;
  struct type *builtin_uint16 = nullptr;
  struct type *builtin_int24 = nullptr;
  struct type *builtin_uint24 = nullptr;
  struct type *builtin_int32 = nullptr;
  struct type *builtin_uint32 = nullptr;
  struct type *builtin_int64 = nullptr;
  struct type *builtin_uint64 = nullptr;
  struct type *builtin_int128 = nullptr;
  struct type *builtin_uint128 = nullptr;

  /* Wide character types.  */
  struct type *builtin_char16 = nullptr;
  struct type *builtin_char32 = nullptr;
  struct type *builtin_wchar = nullptr;

  /* Pointer types.  */

  /* * `pointer to data' type.  Some target platforms use an implicitly
     {sign,zero} -extended 32-bit ABI pointer on a 64-bit ISA.  */
  struct type *builtin_data_ptr = nullptr;

  /* * `pointer to function (returning void)' type.  Harvard
     architectures mean that ABI function and code pointers are not
     interconvertible.  Similarly, since ANSI, C standards have
     explicitly said that pointers to functions and pointers to data
     are not interconvertible --- that is, you can't cast a function
     pointer to void * and back, and expect to get the same value.
     However, all function pointer types are interconvertible, so void
     (*) () can server as a generic function pointer.  */

  struct type *builtin_func_ptr = nullptr;

  /* * `function returning pointer to function (returning void)' type.
     The final void return type is not significant for it.  */

  struct type *builtin_func_func = nullptr;

  /* Special-purpose types.  */

  /* * This type is used to represent a GDB internal function.  */

  struct type *internal_fn = nullptr;

  /* * This type is used to represent an xmethod.  */
  struct type *xmethod = nullptr;

  /* * This type is used to represent symbol addresses.  */
  struct type *builtin_core_addr = nullptr;

  /* * This type represents a type that was unrecognized in symbol
     read-in.  */
  struct type *builtin_error = nullptr;

  /* * Types used for symbols with no debug information.  */
  struct type *nodebug_text_symbol = nullptr;
  struct type *nodebug_text_gnu_ifunc_symbol = nullptr;
  struct type *nodebug_got_plt_symbol = nullptr;
  struct type *nodebug_data_symbol = nullptr;
  struct type *nodebug_unknown_symbol = nullptr;
  struct type *nodebug_tls_symbol = nullptr;
};

/* * Return the type table for the specified architecture.  */

extern const struct builtin_type *builtin_type (struct gdbarch *gdbarch);

/* * Return the type table for the specified objfile.  */

extern const struct builtin_type *builtin_type (struct objfile *objfile);
 
/* Explicit floating-point formats.  See "floatformat.h".  */
extern const struct floatformat *floatformats_ieee_half[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_ieee_single[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_ieee_double[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_ieee_quad[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_ieee_double_littlebyte_bigword[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_i387_ext[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_m68881_ext[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_arm_ext[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_ia64_spill[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_vax_f[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_vax_d[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_ibm_long_double[BFD_ENDIAN_UNKNOWN];
extern const struct floatformat *floatformats_bfloat16[BFD_ENDIAN_UNKNOWN];

/* Allocate space for storing data associated with a particular
   type.  We ensure that the space is allocated using the same
   mechanism that was used to allocate the space for the type
   structure itself.  I.e.  if the type is on an objfile's
   objfile_obstack, then the space for data associated with that type
   will also be allocated on the objfile_obstack.  If the type is
   associated with a gdbarch, then the space for data associated with that
   type will also be allocated on the gdbarch_obstack.

   If a type is not associated with neither an objfile or a gdbarch then
   you should not use this macro to allocate space for data, instead you
   should call xmalloc directly, and ensure the memory is correctly freed
   when it is no longer needed.  */

#define TYPE_ALLOC(t,size)                                              \
  (obstack_alloc (((t)->is_objfile_owned ()                             \
		   ? &((t)->objfile_owner ()->objfile_obstack)          \
		   : gdbarch_obstack ((t)->arch_owner ())),             \
		  size))


/* See comment on TYPE_ALLOC.  */

#define TYPE_ZALLOC(t,size) (memset (TYPE_ALLOC (t, size), 0, size))

/* * This returns the target type (or NULL) of TYPE, also skipping
   past typedefs.  */

extern struct type *get_target_type (struct type *type);

/* Return the equivalent of TYPE_LENGTH, but in number of target
   addressable memory units of the associated gdbarch instead of bytes.  */

extern unsigned int type_length_units (struct type *type);

/* An object of this type is passed when allocating certain types.  It
   determines where the new type is allocated.  Ultimately a type is
   either allocated on a on an objfile obstack or on a gdbarch
   obstack.  However, it's also possible to request that a new type be
   allocated on the same obstack as some existing type, or that a
   "new" type instead overwrite a supplied type object.  */

class type_allocator
{
public:

  /* Create new types on OBJFILE.  */
  type_allocator (objfile *objfile, enum language lang)
    : m_is_objfile (true),
      m_lang (lang)
  {
    m_data.objfile = objfile;
  }

  /* Create new types on GDBARCH.  */
  explicit type_allocator (gdbarch *gdbarch)
    : m_lang (language_minimal)
  {
    m_data.gdbarch = gdbarch;
  }

  /* This determines whether a passed-in type should be rewritten in
     place, or whether it should simply determine where the new type
     is created.  */
  enum type_allocator_kind
  {
    /* Allocate on same obstack as existing type.  */
    SAME = 0,
    /* Smash the existing type.  */
    SMASH = 1,
  };

  /* Create new types either on the same obstack as TYPE; or if SMASH
     is passed, overwrite TYPE.  */
  explicit type_allocator (struct type *type,
			   type_allocator_kind kind = SAME)
    : m_lang (type->language ())
  {
    if (kind == SAME)
      {
	if (type->is_objfile_owned ())
	  {
	    m_data.objfile = type->objfile_owner ();
	    m_is_objfile = true;
	  }
	else
	  m_data.gdbarch = type->arch_owner ();
      }
    else
      {
	m_smash = true;
	m_data.type = type;
      }
  }

  /* Create new types on the same obstack as TYPE.  */
  explicit type_allocator (const struct type *type)
    : m_is_objfile (type->is_objfile_owned ()),
      m_lang (type->language ())
  {
    if (type->is_objfile_owned ())
      m_data.objfile = type->objfile_owner ();
    else
      m_data.gdbarch = type->arch_owner ();
  }

  /* Create a new type on the desired obstack.  Note that a "new" type
     is not created if type-smashing was selected at construction.  */
  type *new_type ();

  /* Create a new type on the desired obstack, and fill in its code,
     length, and name.  If NAME is non-null, it is copied to the
     destination obstack first.  Note that a "new" type is not created
     if type-smashing was selected at construction.  */
  type *new_type (enum type_code code, int bit, const char *name);

  /* Return the architecture associated with this allocator.  This
     comes from whatever object was supplied to the constructor.  */
  gdbarch *arch ();

private:

  /* Where the type should wind up.  */
  union
  {
    struct objfile *objfile;
    struct gdbarch *gdbarch;
    struct type *type;
  } m_data {};

  /* True if this allocator uses the objfile field above.  */
  bool m_is_objfile = false;
  /* True if this allocator uses the type field above, indicating that
     the "allocation" should be done in-place.  */
  bool m_smash = false;
  /* The language for types created by this allocator.  */
  enum language m_lang;
};

/* Allocate a TYPE_CODE_INT type structure using ALLOC.  BIT is the
   type size in bits.  If UNSIGNED_P is non-zero, set the type's
   TYPE_UNSIGNED flag.  NAME is the type name.  */

extern struct type *init_integer_type (type_allocator &alloc, int bit,
				       int unsigned_p, const char *name);

/* Allocate a TYPE_CODE_CHAR type structure using ALLOC.  BIT is the
   type size in bits.  If UNSIGNED_P is non-zero, set the type's
   TYPE_UNSIGNED flag.  NAME is the type name.  */

extern struct type *init_character_type (type_allocator &alloc, int bit,
					 int unsigned_p, const char *name);

/* Allocate a TYPE_CODE_BOOL type structure using ALLOC.  BIT is the
   type size in bits.  If UNSIGNED_P is non-zero, set the type's
   TYPE_UNSIGNED flag.  NAME is the type name.  */

extern struct type *init_boolean_type (type_allocator &alloc, int bit,
				       int unsigned_p, const char *name);

/* Allocate a TYPE_CODE_FLT type structure using ALLOC.
   BIT is the type size in bits; if BIT equals -1, the size is
   determined by the floatformat.  NAME is the type name.  Set the
   TYPE_FLOATFORMAT from FLOATFORMATS.  BYTE_ORDER is the byte order
   to use.  If it is BFD_ENDIAN_UNKNOWN (the default), then the byte
   order of the objfile's architecture is used.  */

extern struct type *init_float_type
     (type_allocator &alloc, int bit, const char *name,
      const struct floatformat **floatformats,
      enum bfd_endian byte_order = BFD_ENDIAN_UNKNOWN);

/* Allocate a TYPE_CODE_DECFLOAT type structure using ALLOC.
   BIT is the type size in bits.  NAME is the type name.  */

extern struct type *init_decfloat_type (type_allocator &alloc, int bit,
					const char *name);

extern bool can_create_complex_type (struct type *);
extern struct type *init_complex_type (const char *, struct type *);

/* Allocate a TYPE_CODE_PTR type structure using ALLOC.
   BIT is the pointer type size in bits.  NAME is the type name.
   TARGET_TYPE is the pointer target type.  Always sets the pointer type's
   TYPE_UNSIGNED flag.  */

extern struct type *init_pointer_type (type_allocator &alloc, int bit,
				       const char *name,
				       struct type *target_type);

extern struct type *init_fixed_point_type (type_allocator &, int, int,
					   const char *);

/* Helper functions to construct a struct or record type.  An
   initially empty type is created using arch_composite_type().
   Fields are then added using append_composite_type_field*().  A union
   type has its size set to the largest field.  A struct type has each
   field packed against the previous.  */

extern struct type *arch_composite_type (struct gdbarch *gdbarch,
					 const char *name, enum type_code code);
extern void append_composite_type_field (struct type *t, const char *name,
					 struct type *field);
extern void append_composite_type_field_aligned (struct type *t,
						 const char *name,
						 struct type *field,
						 int alignment);
struct field *append_composite_type_field_raw (struct type *t, const char *name,
					       struct type *field);

/* Helper functions to construct a bit flags type.  An initially empty
   type is created using arch_flag_type().  Flags are then added using
   append_flag_type_field() and append_flag_type_flag().  */
extern struct type *arch_flags_type (struct gdbarch *gdbarch,
				     const char *name, int bit);
extern void append_flags_type_field (struct type *type,
				     int start_bitpos, int nr_bits,
				     struct type *field_type, const char *name);
extern void append_flags_type_flag (struct type *type, int bitpos,
				    const char *name);

extern void make_vector_type (struct type *array_type);
extern struct type *init_vector_type (struct type *elt_type, int n);

extern struct type *lookup_reference_type (struct type *, enum type_code);
extern struct type *lookup_lvalue_reference_type (struct type *);
extern struct type *lookup_rvalue_reference_type (struct type *);


extern struct type *make_reference_type (struct type *, struct type **,
					 enum type_code);

extern struct type *make_cv_type (int, int, struct type *, struct type **);

extern struct type *make_restrict_type (struct type *);

extern struct type *make_unqualified_type (struct type *);

extern struct type *make_atomic_type (struct type *);

extern void replace_type (struct type *, struct type *);

extern type_instance_flags address_space_name_to_type_instance_flags
  (struct gdbarch *, const char *);

extern const char *address_space_type_instance_flags_to_name
  (struct gdbarch *, type_instance_flags);

extern struct type *make_type_with_address_space
  (struct type *type, type_instance_flags space_identifier);

extern struct type *lookup_memberptr_type (struct type *, struct type *);

extern struct type *lookup_methodptr_type (struct type *);

extern void smash_to_method_type (struct type *type, struct type *self_type,
				  struct type *to_type, struct field *args,
				  int nargs, int varargs);

extern void smash_to_memberptr_type (struct type *, struct type *,
				     struct type *);

extern void smash_to_methodptr_type (struct type *, struct type *);

extern const char *type_name_or_error (struct type *type);

struct struct_elt
{
  /* The field of the element, or NULL if no element was found.  */
  struct field *field;

  /* The bit offset of the element in the parent structure.  */
  LONGEST offset;
};

/* Given a type TYPE, lookup the field and offset of the component named
   NAME.

   TYPE can be either a struct or union, or a pointer or reference to
   a struct or union.  If it is a pointer or reference, its target
   type is automatically used.  Thus '.' and '->' are interchangeable,
   as specified for the definitions of the expression element types
   STRUCTOP_STRUCT and STRUCTOP_PTR.

   If NOERR is nonzero, the returned structure will have field set to
   NULL if there is no component named NAME.

   If the component NAME is a field in an anonymous substructure of
   TYPE, the returned offset is a "global" offset relative to TYPE
   rather than an offset within the substructure.  */

extern struct_elt lookup_struct_elt (struct type *, const char *, int);

/* Given a type TYPE, lookup the type of the component named NAME.

   TYPE can be either a struct or union, or a pointer or reference to
   a struct or union.  If it is a pointer or reference, its target
   type is automatically used.  Thus '.' and '->' are interchangeable,
   as specified for the definitions of the expression element types
   STRUCTOP_STRUCT and STRUCTOP_PTR.

   If NOERR is nonzero, return NULL if there is no component named
   NAME.  */

extern struct type *lookup_struct_elt_type (struct type *, const char *, int);

extern struct type *make_pointer_type (struct type *, struct type **);

extern struct type *lookup_pointer_type (struct type *);

extern struct type *make_function_type (struct type *, struct type **);

extern struct type *lookup_function_type (struct type *);

extern struct type *lookup_function_type_with_arguments (struct type *,
							 int,
							 struct type **);

/* Create a range type using ALLOC.

   Indices will be of type INDEX_TYPE, and will range from LOW_BOUND
   to HIGH_BOUND, inclusive.  */

extern struct type *create_static_range_type (type_allocator &alloc,
					      struct type *index_type,
					      LONGEST low_bound,
					      LONGEST high_bound);

/* Create an array type using ALLOC.

   Elements will be of type ELEMENT_TYPE, the indices will be of type
   RANGE_TYPE.

   BYTE_STRIDE_PROP, when not NULL, provides the array's byte stride.
   This byte stride property is added to the resulting array type
   as a DYN_PROP_BYTE_STRIDE.  As a consequence, the BYTE_STRIDE_PROP
   argument can only be used to create types that are objfile-owned
   (see add_dyn_prop), meaning that either this function must be called
   with an objfile-owned RESULT_TYPE, or an objfile-owned RANGE_TYPE.

   BIT_STRIDE is taken into account only when BYTE_STRIDE_PROP is NULL.
   If BIT_STRIDE is not zero, build a packed array type whose element
   size is BIT_STRIDE.  Otherwise, ignore this parameter.  */

extern struct type *create_array_type_with_stride
     (type_allocator &alloc, struct type *element_type,
      struct type *range_type, struct dynamic_prop *byte_stride_prop,
      unsigned int bit_stride);

/* Create a range type using ALLOC with a dynamic range from LOW_BOUND
   to HIGH_BOUND, inclusive.  INDEX_TYPE is the underlying type.  BIAS
   is the bias to be applied when storing or retrieving values of this
   type.  */

extern struct type *create_range_type (type_allocator &alloc,
				       struct type *index_type,
				       const struct dynamic_prop *low_bound,
				       const struct dynamic_prop *high_bound,
				       LONGEST bias);

/* Like CREATE_RANGE_TYPE but also sets up a stride.  When BYTE_STRIDE_P
   is true the value in STRIDE is a byte stride, otherwise STRIDE is a bit
   stride.  */

extern struct type *create_range_type_with_stride
  (type_allocator &alloc, struct type *index_type,
   const struct dynamic_prop *low_bound,
   const struct dynamic_prop *high_bound, LONGEST bias,
   const struct dynamic_prop *stride, bool byte_stride_p);

/* Same as create_array_type_with_stride but with no bit_stride
   (BIT_STRIDE = 0), thus building an unpacked array.  */

extern struct type *create_array_type (type_allocator &alloc,
				       struct type *element_type,
				       struct type *range_type);

extern struct type *lookup_array_range_type (struct type *, LONGEST, LONGEST);

/* Create a string type using ALLOC.  String types are similar enough
   to array of char types that we can use create_array_type to build
   the basic type and then bash it into a string type.

   For fixed length strings, the range type contains 0 as the lower
   bound and the length of the string minus one as the upper bound.  */

extern struct type *create_string_type (type_allocator &alloc,
					struct type *string_char_type,
					struct type *range_type);

extern struct type *lookup_string_range_type (struct type *, LONGEST, LONGEST);

extern struct type *create_set_type (type_allocator &alloc,
				     struct type *domain_type);

extern struct type *lookup_unsigned_typename (const struct language_defn *,
					      const char *);

extern struct type *lookup_signed_typename (const struct language_defn *,
					    const char *);

extern ULONGEST get_unsigned_type_max (struct type *);

extern void get_signed_type_minmax (struct type *, LONGEST *, LONGEST *);

extern CORE_ADDR get_pointer_type_max (struct type *);

/* * Resolve all dynamic values of a type e.g. array bounds to static values.
   ADDR specifies the location of the variable the type is bound to.
   If TYPE has no dynamic properties return TYPE; otherwise a new type with
   static properties is returned.

   If FRAME is given, it is used when evaluating dynamic properties.
   This can be important when a static link is seen.  If not given,
   the selected frame is used.

   For an array type, if the element type is dynamic, then that will
   not be resolved.  This is done because each individual element may
   have a different type when resolved (depending on the contents of
   memory).  In this situation, 'is_dynamic_type' will still return
   true for the return value of this function.  */
extern struct type *resolve_dynamic_type
  (struct type *type, gdb::array_view<const gdb_byte> valaddr,
   CORE_ADDR addr, const frame_info_ptr *frame = nullptr);

/* * Predicate if the type has dynamic values, which are not resolved yet.
   See the caveat in 'resolve_dynamic_type' to understand a scenario
   where an apparently-resolved type may still be considered
   "dynamic".  */
extern int is_dynamic_type (struct type *type);

extern struct type *check_typedef (struct type *);

extern void check_stub_method_group (struct type *, int);

extern char *gdb_mangle_name (struct type *, int, int);

/* Lookup a typedef or primitive type named NAME, visible in lexical block
   BLOCK.  If NOERR is nonzero, return zero if NAME is not suitably
   defined.

   If this function finds a suitable type then check_typedef is called on
   the type, this ensures that if the type being returned is a typedef
   then the length of the type will be correct.  The original typedef will
   still be returned, not the result of calling check_typedef.  */

extern struct type *lookup_typename (const struct language_defn *language,
				     const char *name,
				     const struct block *block, int noerr);

extern struct type *lookup_template_type (const char *, struct type *,
					  const struct block *);

extern int get_vptr_fieldno (struct type *, struct type **);

/* Set *LOWP and *HIGHP to the lower and upper bounds of discrete type
   TYPE.

   Return true if the two bounds are available, false otherwise.  */

extern bool get_discrete_bounds (struct type *type, LONGEST *lowp,
				 LONGEST *highp);

/* If TYPE's low bound is a known constant, return it, else return nullopt.  */

extern std::optional<LONGEST> get_discrete_low_bound (struct type *type);

/* If TYPE's high bound is a known constant, return it, else return nullopt.  */

extern std::optional<LONGEST> get_discrete_high_bound (struct type *type);

/* Assuming TYPE is a simple, non-empty array type, compute its upper
   and lower bound.  Save the low bound into LOW_BOUND if not NULL.
   Save the high bound into HIGH_BOUND if not NULL.

   Return true if the operation was successful.  Return false otherwise,
   in which case the values of LOW_BOUND and HIGH_BOUNDS are unmodified.  */

extern bool get_array_bounds (struct type *type, LONGEST *low_bound,
			      LONGEST *high_bound);

extern std::optional<LONGEST> discrete_position (struct type *type,
						 LONGEST val);

extern int class_types_same_p (const struct type *, const struct type *);

extern int is_ancestor (struct type *, struct type *);

extern int is_public_ancestor (struct type *, struct type *);

extern int is_unique_ancestor (struct type *, struct value *);

/* Overload resolution */

/* * Badness if parameter list length doesn't match arg list length.  */
extern const struct rank LENGTH_MISMATCH_BADNESS;

/* * Dummy badness value for nonexistent parameter positions.  */
extern const struct rank TOO_FEW_PARAMS_BADNESS;
/* * Badness if no conversion among types.  */
extern const struct rank INCOMPATIBLE_TYPE_BADNESS;

/* * Badness of an exact match.  */
extern const struct rank EXACT_MATCH_BADNESS;

/* * Badness of integral promotion.  */
extern const struct rank INTEGER_PROMOTION_BADNESS;
/* * Badness of floating promotion.  */
extern const struct rank FLOAT_PROMOTION_BADNESS;
/* * Badness of converting a derived class pointer
   to a base class pointer.  */
extern const struct rank BASE_PTR_CONVERSION_BADNESS;
/* * Badness of integral conversion.  */
extern const struct rank INTEGER_CONVERSION_BADNESS;
/* * Badness of floating conversion.  */
extern const struct rank FLOAT_CONVERSION_BADNESS;
/* * Badness of integer<->floating conversions.  */
extern const struct rank INT_FLOAT_CONVERSION_BADNESS;
/* * Badness of conversion of pointer to void pointer.  */
extern const struct rank VOID_PTR_CONVERSION_BADNESS;
/* * Badness of conversion to boolean.  */
extern const struct rank BOOL_CONVERSION_BADNESS;
/* * Badness of converting derived to base class.  */
extern const struct rank BASE_CONVERSION_BADNESS;
/* * Badness of converting from non-reference to reference.  Subrank
   is the type of reference conversion being done.  */
extern const struct rank REFERENCE_CONVERSION_BADNESS;
extern const struct rank REFERENCE_SEE_THROUGH_BADNESS;
/* * Conversion to rvalue reference.  */
#define REFERENCE_CONVERSION_RVALUE 1
/* * Conversion to const lvalue reference.  */
#define REFERENCE_CONVERSION_CONST_LVALUE 2

/* * Badness of converting integer 0 to NULL pointer.  */
extern const struct rank NULL_POINTER_CONVERSION;
/* * Badness of cv-conversion.  Subrank is a flag describing the conversions
   being done.  */
extern const struct rank CV_CONVERSION_BADNESS;
#define CV_CONVERSION_CONST 1
#define CV_CONVERSION_VOLATILE 2

/* Non-standard conversions allowed by the debugger */

/* * Converting a pointer to an int is usually OK.  */
extern const struct rank NS_POINTER_CONVERSION_BADNESS;

/* * Badness of converting a (non-zero) integer constant
   to a pointer.  */
extern const struct rank NS_INTEGER_POINTER_CONVERSION_BADNESS;

extern struct rank sum_ranks (struct rank a, struct rank b);
extern int compare_ranks (struct rank a, struct rank b);

extern int compare_badness (const badness_vector &,
			    const badness_vector &);

extern badness_vector rank_function (gdb::array_view<type *> parms,
				     gdb::array_view<value *> args,
				     bool varargs = false);

extern struct rank rank_one_type (struct type *, struct type *,
				  struct value *);

extern void recursive_dump_type (struct type *, int);

/* printcmd.c */

extern void print_scalar_formatted (const gdb_byte *, struct type *,
				    const struct value_print_options *,
				    int, struct ui_file *);

extern int can_dereference (struct type *);

extern int is_integral_type (struct type *);

extern int is_floating_type (struct type *);

extern int is_scalar_type (struct type *type);

extern int is_scalar_type_recursive (struct type *);

extern int class_or_union_p (const struct type *);

extern void maintenance_print_type (const char *, int);

extern htab_up create_copied_types_hash ();

extern struct type *copy_type_recursive (struct type *type,
					 htab_t copied_types);

extern struct type *copy_type (const struct type *type);

extern bool types_equal (struct type *, struct type *);

extern bool types_deeply_equal (struct type *, struct type *);

extern int type_not_allocated (const struct type *type);

extern int type_not_associated (const struct type *type);

/* Return True if TYPE is a TYPE_CODE_FIXED_POINT or if TYPE is
   a range type whose base type is a TYPE_CODE_FIXED_POINT.  */
extern bool is_fixed_point_type (struct type *type);

/* Allocate a fixed-point type info for TYPE.  This should only be
   called by INIT_FIXED_POINT_SPECIFIC.  */
extern void allocate_fixed_point_type_info (struct type *type);

/* * When the type includes explicit byte ordering, return that.
   Otherwise, the byte ordering from gdbarch_byte_order for
   the type's arch is returned.  */

extern enum bfd_endian type_byte_order (const struct type *type);

/* A flag to enable printing of debugging information of C++
   overloading.  */

extern unsigned int overload_debug;

/* Return whether the function type represented by TYPE is marked as unsafe
   to call by the debugger.

   This usually indicates that the function does not follow the target's
   standard calling convention.  */

extern bool is_nocall_function (const struct type *type);

#endif /* GDBTYPES_H */

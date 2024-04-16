/* Definitions for values of C expressions, for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#if !defined (VALUE_H)
#define VALUE_H 1

#include "frame.h"
#include "extension.h"
#include "gdbsupport/gdb_ref_ptr.h"
#include "gmp-utils.h"

struct block;
struct expression;
struct regcache;
struct symbol;
struct type;
struct ui_file;
struct language_defn;
struct value_print_options;

/* Values can be partially 'optimized out' and/or 'unavailable'.
   These are distinct states and have different string representations
   and related error strings.

   'unavailable' has a specific meaning in this context.  It means the
   value exists in the program (at the machine level), but GDB has no
   means to get to it.  Such a value is normally printed as
   <unavailable>.  Examples of how to end up with an unavailable value
   would be:

    - We're inspecting a traceframe, and the memory or registers the
      debug information says the value lives on haven't been collected.

    - We're inspecting a core dump, the memory or registers the debug
      information says the value lives aren't present in the dump
      (that is, we have a partial/trimmed core dump, or we don't fully
      understand/handle the core dump's format).

    - We're doing live debugging, but the debug API has no means to
      get at where the value lives in the machine, like e.g., ptrace
      not having access to some register or register set.

    - Any other similar scenario.

  OTOH, "optimized out" is about what the compiler decided to generate
  (or not generate).  A chunk of a value that was optimized out does
  not actually exist in the program.  There's no way to get at it
  short of compiling the program differently.

  A register that has not been saved in a frame is likewise considered
  optimized out, except not-saved registers have a different string
  representation and related error strings.  E.g., we'll print them as
  <not-saved> instead of <optimized out>, as in:

    (gdb) p/x $rax
    $1 = <not saved>
    (gdb) info registers rax
    rax            <not saved>

  If the debug info describes a variable as being in such a register,
  we'll still print the variable as <optimized out>.  IOW, <not saved>
  is reserved for inspecting registers at the machine level.

  When comparing value contents, optimized out chunks, unavailable
  chunks, and valid contents data are all considered different.  See
  value_contents_eq for more info.
*/

extern bool overload_resolution;

/* Defines an [OFFSET, OFFSET + LENGTH) range.  */

struct range
{
  /* Lowest offset in the range.  */
  LONGEST offset;

  /* Length of the range.  */
  ULONGEST length;

  /* Returns true if THIS is strictly less than OTHER, useful for
     searching.  We keep ranges sorted by offset and coalesce
     overlapping and contiguous ranges, so this just compares the
     starting offset.  */

  bool operator< (const range &other) const
  {
    return offset < other.offset;
  }

  /* Returns true if THIS is equal to OTHER.  */
  bool operator== (const range &other) const
  {
    return offset == other.offset && length == other.length;
  }
};

/* A policy class to interface gdb::ref_ptr with struct value.  */

struct value_ref_policy
{
  static void incref (struct value *ptr);
  static void decref (struct value *ptr);
};

/* A gdb:;ref_ptr pointer to a struct value.  */

typedef gdb::ref_ptr<struct value, value_ref_policy> value_ref_ptr;

/* Note that the fields in this structure are arranged to save a bit
   of memory.  */

struct value
{
private:

  /* Values can only be created via "static constructors".  */
  explicit value (struct type *type_)
    : m_modifiable (true),
      m_lazy (true),
      m_initialized (true),
      m_stack (false),
      m_is_zero (false),
      m_in_history (false),
      m_type (type_),
      m_enclosing_type (type_)
  {
  }

  /* Values can only be destroyed via the reference-counting
     mechanism.  */
  ~value ();

  DISABLE_COPY_AND_ASSIGN (value);

public:

  /* Allocate a lazy value for type TYPE.  Its actual content is
     "lazily" allocated too: the content field of the return value is
     NULL; it will be allocated when it is fetched from the target.  */
  static struct value *allocate_lazy (struct type *type);

  /* Allocate a value and its contents for type TYPE.  */
  static struct value *allocate (struct type *type);

  /* Allocate a lazy value representing register REGNUM in the frame previous
     to NEXT_FRAME.  If TYPE is non-nullptr, use it as the value type.
     Otherwise, use `register_type` to obtain the type.  */
  static struct value *allocate_register_lazy (frame_info_ptr next_frame,
					  int regnum, type *type = nullptr);

  /* Same as `allocate_register_lazy`, but make the value non-lazy.
  
     The caller is responsible for filling the value's contents.  */
  static struct value *allocate_register (frame_info_ptr next_frame,
					  int regnum, type *type = nullptr);

  /* Create a computed lvalue, with type TYPE, function pointers
     FUNCS, and closure CLOSURE.  */
  static struct value *allocate_computed (struct type *type,
					  const struct lval_funcs *funcs,
					  void *closure);

  /* Allocate NOT_LVAL value for type TYPE being OPTIMIZED_OUT.  */
  static struct value *allocate_optimized_out (struct type *type);

  /* Create a value of type TYPE that is zero, and return it.  */
  static struct value *zero (struct type *type, enum lval_type lv);

  /* Return a copy of the value.  It contains the same contents, for
     the same memory address, but it's a different block of
     storage.  */
  struct value *copy () const;

  /* Type of the value.  */
  struct type *type () const
  { return m_type; }

  /* This is being used to change the type of an existing value, that
     code should instead be creating a new value with the changed type
     (but possibly shared content).  */
  void deprecated_set_type (struct type *type)
  { m_type = type; }

  /* Return the gdbarch associated with the value. */
  struct gdbarch *arch () const;

  /* Only used for bitfields; number of bits contained in them.  */
  LONGEST bitsize () const
  { return m_bitsize; }

  void set_bitsize (LONGEST bit)
  { m_bitsize = bit; }

  /* Only used for bitfields; position of start of field.  For
     little-endian targets, it is the position of the LSB.  For
     big-endian targets, it is the position of the MSB.  */
  LONGEST bitpos () const
  { return m_bitpos; }

  void set_bitpos (LONGEST bit)
  { m_bitpos = bit; }

  /* Only used for bitfields; the containing value.  This allows a
     single read from the target when displaying multiple
     bitfields.  */
  value *parent () const
  { return m_parent.get (); }

  void set_parent (struct value *parent)
  {  m_parent = value_ref_ptr::new_reference (parent); }

  /* Describes offset of a value within lval of a structure in bytes.
     If lval == lval_memory, this is an offset to the address.  If
     lval == lval_register, this is a further offset from
     location.address within the registers structure.  Note also the
     member embedded_offset below.  */
  LONGEST offset () const
  { return m_offset; }

  void set_offset (LONGEST offset)
  { m_offset = offset; }

  /* The comment from "struct value" reads: ``Is it modifiable?  Only
     relevant if lval != not_lval.''.  Shouldn't the value instead be
     not_lval and be done with it?  */
  bool deprecated_modifiable () const
  { return m_modifiable; }

  /* Set or clear the modifiable flag.  */
  void set_modifiable (bool val)
  { m_modifiable = val; }

  LONGEST pointed_to_offset () const
  { return m_pointed_to_offset; }

  void set_pointed_to_offset (LONGEST val)
  { m_pointed_to_offset = val; }

  LONGEST embedded_offset () const
  { return m_embedded_offset; }

  void set_embedded_offset (LONGEST val)
  { m_embedded_offset = val; }

  /* If false, contents of this value are in the contents field.  If
     true, contents are in inferior.  If the lval field is lval_memory,
     the contents are in inferior memory at location.address plus offset.
     The lval field may also be lval_register.

     WARNING: This field is used by the code which handles watchpoints
     (see breakpoint.c) to decide whether a particular value can be
     watched by hardware watchpoints.  If the lazy flag is set for some
     member of a value chain, it is assumed that this member of the
     chain doesn't need to be watched as part of watching the value
     itself.  This is how GDB avoids watching the entire struct or array
     when the user wants to watch a single struct member or array
     element.  If you ever change the way lazy flag is set and reset, be
     sure to consider this use as well!  */

  bool lazy () const
  { return m_lazy; }

  void set_lazy (bool val)
  { m_lazy = val; }

  /* If a value represents a C++ object, then the `type' field gives the
     object's compile-time type.  If the object actually belongs to some
     class derived from `type', perhaps with other base classes and
     additional members, then `type' is just a subobject of the real
     thing, and the full object is probably larger than `type' would
     suggest.

     If `type' is a dynamic class (i.e. one with a vtable), then GDB can
     actually determine the object's run-time type by looking at the
     run-time type information in the vtable.  When this information is
     available, we may elect to read in the entire object, for several
     reasons:

     - When printing the value, the user would probably rather see the
     full object, not just the limited portion apparent from the
     compile-time type.

     - If `type' has virtual base classes, then even printing `type'
     alone may require reaching outside the `type' portion of the
     object to wherever the virtual base class has been stored.

     When we store the entire object, `enclosing_type' is the run-time
     type -- the complete object -- and `embedded_offset' is the offset
     of `type' within that larger type, in bytes.  The contents()
     method takes `embedded_offset' into account, so most GDB code
     continues to see the `type' portion of the value, just as the
     inferior would.

     If `type' is a pointer to an object, then `enclosing_type' is a
     pointer to the object's run-time type, and `pointed_to_offset' is
     the offset in bytes from the full object to the pointed-to object
     -- that is, the value `embedded_offset' would have if we followed
     the pointer and fetched the complete object.  (I don't really see
     the point.  Why not just determine the run-time type when you
     indirect, and avoid the special case?  The contents don't matter
     until you indirect anyway.)

     If we're not doing anything fancy, `enclosing_type' is equal to
     `type', and `embedded_offset' is zero, so everything works
     normally.  */

  struct type *enclosing_type  () const
  { return m_enclosing_type; }

  void set_enclosing_type (struct type *new_type);

  bool stack () const
  { return m_stack; }

  void set_stack (bool val)
  { m_stack = val; }

  /* If this value is lval_computed, return its lval_funcs
     structure.  */
  const struct lval_funcs *computed_funcs () const;

  /* If this value is lval_computed, return its closure.  The meaning
     of the returned value depends on the functions this value
     uses.  */
  void *computed_closure () const;

  enum lval_type lval () const
  { return m_lval; }

  /* Set the 'lval' of this value.  */
  void set_lval (lval_type val)
  { m_lval = val; }

  /* Set or return field indicating whether a variable is initialized or
     not, based on debugging information supplied by the compiler.
     true = initialized; false = uninitialized.  */
  bool initialized () const
  { return m_initialized; }

  void set_initialized (bool value)
  { m_initialized = value; }

  /* If lval == lval_memory, return the address in the inferior.  If
     lval == lval_register, return the byte offset into the registers
     structure.  Otherwise, return 0.  The returned address
     includes the offset, if any.  */
  CORE_ADDR address () const;

  /* Like address, except the result does not include value's
     offset.  */
  CORE_ADDR raw_address () const;

  /* Set the address of a value.  */
  void set_address (CORE_ADDR);

  struct internalvar **deprecated_internalvar_hack ()
  { return &m_location.internalvar; }

  /* Return this value's next frame id.

     The value must be of lval == lval_register.  */
  frame_id next_frame_id ()
  {
    gdb_assert (m_lval == lval_register);

    return m_location.reg.next_frame_id;
  }

  /* Return this value's register number.

     The value must be of lval == lval_register.  */
  int regnum ()
  {
    gdb_assert (m_lval == lval_register);

    return m_location.reg.regnum;
  }


  /* contents() and contents_raw() both return the address of the gdb
     buffer used to hold a copy of the contents of the lval.
     contents() is used when the contents of the buffer are needed --
     it uses fetch_lazy() to load the buffer from the process being
     debugged if it hasn't already been loaded (contents_writeable()
     is used when a writeable but fetched buffer is required)..
     contents_raw() is used when data is being stored into the buffer,
     or when it is certain that the contents of the buffer are valid.

     Note: The contents pointer is adjusted by the offset required to
     get to the real subobject, if the value happens to represent
     something embedded in a larger run-time object.  */
  gdb::array_view<gdb_byte> contents_raw ();

  /* Actual contents of the value.  For use of this value; setting it
     uses the stuff above.  Not valid if lazy is nonzero.  Target
     byte-order.  We force it to be aligned properly for any possible
     value.  Note that a value therefore extends beyond what is
     declared here.  */
  gdb::array_view<const gdb_byte> contents ();

  /* The ALL variants of the above two methods do not adjust the
     returned pointer by the embedded_offset value.  */
  gdb::array_view<const gdb_byte> contents_all ();
  gdb::array_view<gdb_byte> contents_all_raw ();

  gdb::array_view<gdb_byte> contents_writeable ();

  /* Like contents_all, but does not require that the returned bits be
     valid.  This should only be used in situations where you plan to
     check the validity manually.  */
  gdb::array_view<const gdb_byte> contents_for_printing ();

  /* Like contents_for_printing, but accepts a constant value pointer.
     Unlike contents_for_printing however, the pointed value must
     _not_ be lazy.  */
  gdb::array_view<const gdb_byte> contents_for_printing () const;

  /* Load the actual content of a lazy value.  Fetch the data from the
     user's process and clear the lazy flag to indicate that the data in
     the buffer is valid.

     If the value is zero-length, we avoid calling read_memory, which
     would abort.  We mark the value as fetched anyway -- all 0 bytes of
     it.  */
  void fetch_lazy ();

  /* Compare LENGTH bytes of this value's contents starting at OFFSET1
     with LENGTH bytes of VAL2's contents starting at OFFSET2.

     Note that "contents" refers to the whole value's contents
     (value_contents_all), without any embedded offset adjustment.  For
     example, to compare a complete object value with itself, including
     its enclosing type chunk, you'd do:

     int len = check_typedef (val->enclosing_type ())->length ();
     val->contents_eq (0, val, 0, len);

     Returns true iff the set of available/valid contents match.

     Optimized-out contents are equal to optimized-out contents, and are
     not equal to non-optimized-out contents.

     Unavailable contents are equal to unavailable contents, and are not
     equal to non-unavailable contents.

     For example, if 'x's represent an unavailable byte, and 'V' and 'Z'
     represent different available/valid bytes, in a value with length
     16:

     offset:   0   4   8   12  16
     contents: xxxxVVVVxxxxVVZZ

     then:

     val->contents_eq(0, val, 8, 6) => true
     val->contents_eq(0, val, 4, 4) => false
     val->contents_eq(0, val, 8, 8) => false
     val->contents_eq(4, val, 12, 2) => true
     val->contents_eq(4, val, 12, 4) => true
     val->contents_eq(3, val, 4, 4) => true

     If 'x's represent an unavailable byte, 'o' represents an optimized
     out byte, in a value with length 8:

     offset:   0   4   8
     contents: xxxxoooo

     then:

     val->contents_eq(0, val, 2, 2) => true
     val->contents_eq(4, val, 6, 2) => true
     val->contents_eq(0, val, 4, 4) => true

     We only know whether a value chunk is unavailable or optimized out
     if we've tried to read it.  As this routine is used by printing
     routines, which may be printing values in the value history, long
     after the inferior is gone, it works with const values.  Therefore,
     this routine must not be called with lazy values.  */

  bool contents_eq (LONGEST offset1, const struct value *val2, LONGEST offset2,
		    LONGEST length) const;

  /* An overload of contents_eq that compares the entirety of both
     values.  */
  bool contents_eq (const struct value *val2) const;

  /* Given a value, determine whether the bits starting at OFFSET and
     extending for LENGTH bits are a synthetic pointer.  */

  bool bits_synthetic_pointer (LONGEST offset, LONGEST length) const;

  /* Increase this value's reference count.  */
  void incref ()
  { ++m_reference_count; }

  /* Decrease this value's reference count.  When the reference count
     drops to 0, it will be freed.  */
  void decref ();

  /* Given a value, determine whether the contents bytes starting at
     OFFSET and extending for LENGTH bytes are available.  This returns
     true if all bytes in the given range are available, false if any
     byte is unavailable.  */
  bool bytes_available (LONGEST offset, ULONGEST length) const;

  /* Given a value, determine whether the contents bits starting at
     OFFSET and extending for LENGTH bits are available.  This returns
     true if all bits in the given range are available, false if any
     bit is unavailable.  */
  bool bits_available (LONGEST offset, ULONGEST length) const;

  /* Like bytes_available, but return false if any byte in the
     whole object is unavailable.  */
  bool entirely_available ();

  /* Like entirely_available, but return false if any byte in the
     whole object is available.  */
  bool entirely_unavailable ()
  { return entirely_covered_by_range_vector (m_unavailable); }

  /* Mark this value's content bytes starting at OFFSET and extending
     for LENGTH bytes as unavailable.  */
  void mark_bytes_unavailable (LONGEST offset, ULONGEST length);

  /* Mark this value's content bits starting at OFFSET and extending
     for LENGTH bits as unavailable.  */
  void mark_bits_unavailable (LONGEST offset, ULONGEST length);

  /* If true, this is the value of a variable which does not actually
     exist in the program, at least partially.  If the value is lazy,
     this may fetch it now.  */
  bool optimized_out ();

  /* Given a value, return true if any of the contents bits starting at
     OFFSET and extending for LENGTH bits is optimized out, false
     otherwise.  */
  bool bits_any_optimized_out (int bit_offset, int bit_length) const;

  /* Like optimized_out, but return true iff the whole value is
     optimized out.  */
  bool entirely_optimized_out ()
  {
    return entirely_covered_by_range_vector (m_optimized_out);
  }

  /* Mark this value's content bytes starting at OFFSET and extending
     for LENGTH bytes as optimized out.  */
  void mark_bytes_optimized_out (int offset, int length);

  /* Mark this value's content bits starting at OFFSET and extending
     for LENGTH bits as optimized out.  */
  void mark_bits_optimized_out (LONGEST offset, LONGEST length);

  /* Return a version of this that is non-lvalue.  */
  struct value *non_lval ();

  /* Write contents of this value at ADDR and set its lval type to be
     LVAL_MEMORY.  */
  void force_lval (CORE_ADDR);

  /* Set this values's location as appropriate for a component of
     WHOLE --- regardless of what kind of lvalue WHOLE is.  */
  void set_component_location (const struct value *whole);

  /* Build a value wrapping and representing WORKER.  The value takes
     ownership of the xmethod_worker object.  */
  static struct value *from_xmethod (xmethod_worker_up &&worker);

  /* Return the type of the result of TYPE_CODE_XMETHOD value METHOD.  */
  struct type *result_type_of_xmethod (gdb::array_view<value *> argv);

  /* Call the xmethod corresponding to the TYPE_CODE_XMETHOD value
     METHOD.  */
  struct value *call_xmethod (gdb::array_view<value *> argv);

  /* Update this value before discarding OBJFILE.  COPIED_TYPES is
     used to prevent cycles / duplicates.  */
  void preserve (struct objfile *objfile, htab_t copied_types);

  /* Unpack a bitfield of BITSIZE bits found at BITPOS in the object
     at VALADDR + EMBEDDEDOFFSET that has the type of DEST_VAL and
     store the contents in DEST_VAL, zero or sign extending if the
     type of DEST_VAL is wider than BITSIZE.  VALADDR points to the
     contents of this value.  If this value's contents required to
     extract the bitfield from are unavailable/optimized out, DEST_VAL
     is correspondingly marked unavailable/optimized out.  */
  void unpack_bitfield (struct value *dest_val,
			LONGEST bitpos, LONGEST bitsize,
			const gdb_byte *valaddr, LONGEST embedded_offset)
    const;

  /* Copy LENGTH bytes of this value's (all) contents
     (value_contents_all) starting at SRC_OFFSET byte, into DST
     value's (all) contents, starting at DST_OFFSET.  If unavailable
     contents are being copied from this value, the corresponding DST
     contents are marked unavailable accordingly.  DST must not be
     lazy.  If this value is lazy, it will be fetched now.

     It is assumed the contents of DST in the [DST_OFFSET,
     DST_OFFSET+LENGTH) range are wholly available.  */
  void contents_copy (struct value *dst, LONGEST dst_offset,
		      LONGEST src_offset, LONGEST length);

  /* Given a value (offset by OFFSET bytes)
     of a struct or union type ARG_TYPE,
     extract and return the value of one of its (non-static) fields.
     FIELDNO says which field.  */
  struct value *primitive_field (LONGEST offset, int fieldno,
				 struct type *arg_type);

  /* Create a new value by extracting it from this value.  TYPE is the
     type of the new value.  BIT_OFFSET and BIT_LENGTH describe the
     offset and field width of the value to extract from this value --
     BIT_LENGTH may differ from TYPE's length in the case where this
     value's type is packed.

     When the value does come from a non-byte-aligned offset or field
     width, it will be marked non_lval.  */
  struct value *from_component_bitsize (struct type *type,
					LONGEST bit_offset,
					LONGEST bit_length);

  /* Record this value on the value history, and return its location
     in the history.  The value is removed from the value chain.  */
  int record_latest ();

private:

  /* Type of value; either not an lval, or one of the various
     different possible kinds of lval.  */
  enum lval_type m_lval = not_lval;

  /* Is it modifiable?  Only relevant if lval != not_lval.  */
  bool m_modifiable : 1;

  /* If false, contents of this value are in the contents field.  If
     true, contents are in inferior.  If the lval field is lval_memory,
     the contents are in inferior memory at location.address plus offset.
     The lval field may also be lval_register.

     WARNING: This field is used by the code which handles watchpoints
     (see breakpoint.c) to decide whether a particular value can be
     watched by hardware watchpoints.  If the lazy flag is set for
     some member of a value chain, it is assumed that this member of
     the chain doesn't need to be watched as part of watching the
     value itself.  This is how GDB avoids watching the entire struct
     or array when the user wants to watch a single struct member or
     array element.  If you ever change the way lazy flag is set and
     reset, be sure to consider this use as well!  */
  bool m_lazy : 1;

  /* If value is a variable, is it initialized or not.  */
  bool m_initialized : 1;

  /* If value is from the stack.  If this is set, read_stack will be
     used instead of read_memory to enable extra caching.  */
  bool m_stack : 1;

  /* True if this is a zero value, created by 'value::zero'; false
     otherwise.  */
  bool m_is_zero : 1;

  /* True if this a value recorded in value history; false otherwise.  */
  bool m_in_history : 1;

  /* Location of value (if lval).  */
  union
  {
    /* If lval == lval_memory, this is the address in the inferior  */
    CORE_ADDR address;

    /*If lval == lval_register, the value is from a register.  */
    struct
    {
      /* Register number.  */
      int regnum;

      /* Frame ID of the next physical (non-inline) frame to which a register
	 value is relative.  */
      frame_id next_frame_id;
    } reg;

    /* Pointer to internal variable.  */
    struct internalvar *internalvar;

    /* Pointer to xmethod worker.  */
    struct xmethod_worker *xm_worker;

    /* If lval == lval_computed, this is a set of function pointers
       to use to access and describe the value, and a closure pointer
       for them to use.  */
    struct
    {
      /* Functions to call.  */
      const struct lval_funcs *funcs;

      /* Closure for those functions to use.  */
      void *closure;
    } computed;
  } m_location {};

  /* Describes offset of a value within lval of a structure in target
     addressable memory units.  Note also the member embedded_offset
     below.  */
  LONGEST m_offset = 0;

  /* Only used for bitfields; number of bits contained in them.  */
  LONGEST m_bitsize = 0;

  /* Only used for bitfields; position of start of field.  For
     little-endian targets, it is the position of the LSB.  For
     big-endian targets, it is the position of the MSB.  */
  LONGEST m_bitpos = 0;

  /* The number of references to this value.  When a value is created,
     the value chain holds a reference, so REFERENCE_COUNT is 1.  If
     release_value is called, this value is removed from the chain but
     the caller of release_value now has a reference to this value.
     The caller must arrange for a call to value_free later.  */
  int m_reference_count = 1;

  /* Only used for bitfields; the containing value.  This allows a
     single read from the target when displaying multiple
     bitfields.  */
  value_ref_ptr m_parent;

  /* Type of the value.  */
  struct type *m_type;

  /* If a value represents a C++ object, then the `type' field gives
     the object's compile-time type.  If the object actually belongs
     to some class derived from `type', perhaps with other base
     classes and additional members, then `type' is just a subobject
     of the real thing, and the full object is probably larger than
     `type' would suggest.

     If `type' is a dynamic class (i.e. one with a vtable), then GDB
     can actually determine the object's run-time type by looking at
     the run-time type information in the vtable.  When this
     information is available, we may elect to read in the entire
     object, for several reasons:

     - When printing the value, the user would probably rather see the
     full object, not just the limited portion apparent from the
     compile-time type.

     - If `type' has virtual base classes, then even printing `type'
     alone may require reaching outside the `type' portion of the
     object to wherever the virtual base class has been stored.

     When we store the entire object, `enclosing_type' is the run-time
     type -- the complete object -- and `embedded_offset' is the
     offset of `type' within that larger type, in target addressable memory
     units.  The contents() method takes `embedded_offset' into account,
     so most GDB code continues to see the `type' portion of the value, just
     as the inferior would.

     If `type' is a pointer to an object, then `enclosing_type' is a
     pointer to the object's run-time type, and `pointed_to_offset' is
     the offset in target addressable memory units from the full object
     to the pointed-to object -- that is, the value `embedded_offset' would
     have if we followed the pointer and fetched the complete object.
     (I don't really see the point.  Why not just determine the
     run-time type when you indirect, and avoid the special case?  The
     contents don't matter until you indirect anyway.)

     If we're not doing anything fancy, `enclosing_type' is equal to
     `type', and `embedded_offset' is zero, so everything works
     normally.  */
  struct type *m_enclosing_type;
  LONGEST m_embedded_offset = 0;
  LONGEST m_pointed_to_offset = 0;

  /* Actual contents of the value.  Target byte-order.

     May be nullptr if the value is lazy or is entirely optimized out.
     Guaranteed to be non-nullptr otherwise.  */
  gdb::unique_xmalloc_ptr<gdb_byte> m_contents;

  /* Unavailable ranges in CONTENTS.  We mark unavailable ranges,
     rather than available, since the common and default case is for a
     value to be available.  This is filled in at value read time.
     The unavailable ranges are tracked in bits.  Note that a contents
     bit that has been optimized out doesn't really exist in the
     program, so it can't be marked unavailable either.  */
  std::vector<range> m_unavailable;

  /* Likewise, but for optimized out contents (a chunk of the value of
     a variable that does not actually exist in the program).  If LVAL
     is lval_register, this is a register ($pc, $sp, etc., never a
     program variable) that has not been saved in the frame.  Not
     saved registers and optimized-out program variables values are
     treated pretty much the same, except not-saved registers have a
     different string representation and related error strings.  */
  std::vector<range> m_optimized_out;

  /* This is only non-zero for values of TYPE_CODE_ARRAY and if the size of
     the array in inferior memory is greater than max_value_size.  If these
     conditions are met then, when the value is loaded from the inferior
     GDB will only load a portion of the array into memory, and
     limited_length will be set to indicate the length in octets that were
     loaded from the inferior.  */
  ULONGEST m_limited_length = 0;

  /* Allocate a value and its contents for type TYPE.  If CHECK_SIZE
     is true, then apply the usual max-value-size checks.  */
  static struct value *allocate (struct type *type, bool check_size);

  /* Helper for fetch_lazy when the value is a bitfield.  */
  void fetch_lazy_bitfield ();

  /* Helper for fetch_lazy when the value is in memory.  */
  void fetch_lazy_memory ();

  /* Helper for fetch_lazy when the value is in a register.  */
  void fetch_lazy_register ();

  /* Try to limit ourselves to only fetching the limited number of
     elements.  However, if this limited number of elements still
     puts us over max_value_size, then we still refuse it and
     return failure here, which will ultimately throw an error.  */
  bool set_limited_array_length ();

  /* Allocate the contents of this value if it has not been allocated
     yet.  If CHECK_SIZE is true, then apply the usual max-value-size
     checks.  */
  void allocate_contents (bool check_size);

  /* Helper function for value_contents_eq.  The only difference is that
     this function is bit rather than byte based.

     Compare LENGTH bits of this value's contents starting at OFFSET1
     bits with LENGTH bits of VAL2's contents starting at OFFSET2
     bits.  Return true if the available bits match.  */
  bool contents_bits_eq (int offset1, const struct value *val2, int offset2,
			 int length) const;

  void require_not_optimized_out () const;
  void require_available () const;

  /* Returns true if this value is entirely covered by RANGES.  If the
     value is lazy, it'll be read now.  Note that RANGE is a pointer
     to pointer because reading the value might change *RANGE.  */
  bool entirely_covered_by_range_vector (const std::vector<range> &ranges);

  /* Copy the ranges metadata from this value that overlaps
     [SRC_BIT_OFFSET, SRC_BIT_OFFSET+BIT_LENGTH) into DST,
     adjusted.  */
  void ranges_copy_adjusted (struct value *dst, int dst_bit_offset,
			     int src_bit_offset, int bit_length) const;

  /* Copy LENGTH target addressable memory units of this value's (all)
     contents (value_contents_all) starting at SRC_OFFSET, into DST
     value's (all) contents, starting at DST_OFFSET.  If unavailable
     contents are being copied from this, the corresponding DST
     contents are marked unavailable accordingly.  Neither DST nor
     this value may be lazy values.

     It is assumed the contents of DST in the [DST_OFFSET,
     DST_OFFSET+LENGTH) range are wholly available.  */
  void contents_copy_raw (struct value *dst, LONGEST dst_offset,
			  LONGEST src_offset, LONGEST length);

  /* A helper for value_from_component_bitsize that copies bits from
     this value to DEST.  */
  void contents_copy_raw_bitwise (struct value *dst, LONGEST dst_bit_offset,
				  LONGEST src_bit_offset, LONGEST bit_length);
};

inline void
value_ref_policy::incref (struct value *ptr)
{
  ptr->incref ();
}

inline void
value_ref_policy::decref (struct value *ptr)
{
  ptr->decref ();
}

/* Returns value_type or value_enclosing_type depending on
   value_print_options.objectprint.

   If RESOLVE_SIMPLE_TYPES is 0 the enclosing type will be resolved
   only for pointers and references, else it will be returned
   for all the types (e.g. structures).  This option is useful
   to prevent retrieving enclosing type for the base classes fields.

   REAL_TYPE_FOUND is used to inform whether the real type was found
   (or just static type was used).  The NULL may be passed if it is not
   necessary. */

extern struct type *value_actual_type (struct value *value,
				       int resolve_simple_types,
				       int *real_type_found);

/* For lval_computed values, this structure holds functions used to
   retrieve and set the value (or portions of the value).

   For each function, 'V' is the 'this' pointer: an lval_funcs
   function F may always assume that the V it receives is an
   lval_computed value, and has F in the appropriate slot of its
   lval_funcs structure.  */

struct lval_funcs
{
  /* Fill in VALUE's contents.  This is used to "un-lazy" values.  If
     a problem arises in obtaining VALUE's bits, this function should
     call 'error'.  If it is NULL value_fetch_lazy on "un-lazy"
     non-optimized-out value is an internal error.  */
  void (*read) (struct value *v);

  /* Handle an assignment TOVAL = FROMVAL by writing the value of
     FROMVAL to TOVAL's location.  The contents of TOVAL have not yet
     been updated.  If a problem arises in doing so, this function
     should call 'error'.  If it is NULL such TOVAL assignment is an error as
     TOVAL is not considered as an lvalue.  */
  void (*write) (struct value *toval, struct value *fromval);

  /* Return true if any part of V is optimized out, false otherwise.
     This will only be called for lazy values -- if the value has been
     fetched, then the value's optimized-out bits are consulted
     instead.  */
  bool (*is_optimized_out) (struct value *v);

  /* If non-NULL, this is used to implement pointer indirection for
     this value.  This method may return NULL, in which case value_ind
     will fall back to ordinary indirection.  */
  struct value *(*indirect) (struct value *value);

  /* If non-NULL, this is used to implement reference resolving for
     this value.  This method may return NULL, in which case coerce_ref
     will fall back to ordinary references resolving.  */
  struct value *(*coerce_ref) (const struct value *value);

  /* If non-NULL, this is used to determine whether the indicated bits
     of VALUE are a synthetic pointer.  */
  bool (*check_synthetic_pointer) (const struct value *value,
				   LONGEST offset, int length);

  /* Return a duplicate of VALUE's closure, for use in a new value.
     This may simply return the same closure, if VALUE's is
     reference-counted or statically allocated.

     This may be NULL, in which case VALUE's closure is re-used in the
     new value.  */
  void *(*copy_closure) (const struct value *v);

  /* Drop VALUE's reference to its closure.  Maybe this frees the
     closure; maybe this decrements a reference count; maybe the
     closure is statically allocated and this does nothing.

     This may be NULL, in which case no action is taken to free
     VALUE's closure.  */
  void (*free_closure) (struct value *v);
};

/* Throw an error complaining that the value has been optimized
   out.  */

extern void error_value_optimized_out (void);

/* Pointer to internal variable.  */
#define VALUE_INTERNALVAR(val) (*((val)->deprecated_internalvar_hack ()))

/* Return value after lval_funcs->coerce_ref (after check_typedef).  Return
   NULL if lval_funcs->coerce_ref is not applicable for whatever reason.  */

extern struct value *coerce_ref_if_computed (const struct value *arg);

/* Setup a new value type and enclosing value type for dereferenced value VALUE.
   ENC_TYPE is the new enclosing type that should be set.  ORIGINAL_TYPE and
   ORIGINAL_VAL are the type and value of the original reference or
   pointer.  ORIGINAL_VALUE_ADDRESS is the address within VALUE, that is
   the address that was dereferenced.

   Note, that VALUE is modified by this function.

   It is a common implementation for coerce_ref and value_ind.  */

extern struct value * readjust_indirect_value_type (struct value *value,
						    struct type *enc_type,
						    const struct type *original_type,
						    struct value *original_val,
						    CORE_ADDR original_value_address);

/* Convert a REF to the object referenced.  */

extern struct value *coerce_ref (struct value *value);

/* If ARG is an array, convert it to a pointer.
   If ARG is a function, convert it to a function pointer.

   References are dereferenced.  */

extern struct value *coerce_array (struct value *value);

/* Read LENGTH addressable memory units starting at MEMADDR into BUFFER,
   which is (or will be copied to) VAL's contents buffer offset by
   BIT_OFFSET bits.  Marks value contents ranges as unavailable if
   the corresponding memory is likewise unavailable.  STACK indicates
   whether the memory is known to be stack memory.  */

extern void read_value_memory (struct value *val, LONGEST bit_offset,
			       bool stack, CORE_ADDR memaddr,
			       gdb_byte *buffer, size_t length);

/* Cast SCALAR_VALUE to the element type of VECTOR_TYPE, then replicate
   into each element of a new vector value with VECTOR_TYPE.  */

struct value *value_vector_widen (struct value *scalar_value,
				  struct type *vector_type);



#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"

class frame_info_ptr;
struct fn_field;

extern int print_address_demangle (const struct value_print_options *,
				   struct gdbarch *, CORE_ADDR,
				   struct ui_file *, int);

/* Returns true if VAL is of floating-point type.  In addition,
   throws an error if the value is an invalid floating-point value.  */
extern bool is_floating_value (struct value *val);

extern LONGEST value_as_long (struct value *val);
extern CORE_ADDR value_as_address (struct value *val);

/* Extract the value from VAL as a MPZ.  This coerces arrays and
   handles various integer-like types as well.  */

extern gdb_mpz value_as_mpz (struct value *val);

extern LONGEST unpack_long (struct type *type, const gdb_byte *valaddr);
extern CORE_ADDR unpack_pointer (struct type *type, const gdb_byte *valaddr);

extern LONGEST unpack_field_as_long (struct type *type,
				     const gdb_byte *valaddr,
				     int fieldno);

/* Unpack a bitfield of the specified FIELD_TYPE, from the object at
   VALADDR, and store the result in *RESULT.
   The bitfield starts at BITPOS bits and contains BITSIZE bits; if
   BITSIZE is zero, then the length is taken from FIELD_TYPE.

   Extracting bits depends on endianness of the machine.  Compute the
   number of least significant bits to discard.  For big endian machines,
   we compute the total number of bits in the anonymous object, subtract
   off the bit count from the MSB of the object to the MSB of the
   bitfield, then the size of the bitfield, which leaves the LSB discard
   count.  For little endian machines, the discard count is simply the
   number of bits from the LSB of the anonymous object to the LSB of the
   bitfield.

   If the field is signed, we also do sign extension.  */

extern LONGEST unpack_bits_as_long (struct type *field_type,
				    const gdb_byte *valaddr,
				    LONGEST bitpos, LONGEST bitsize);

extern int unpack_value_field_as_long (struct type *type, const gdb_byte *valaddr,
				LONGEST embedded_offset, int fieldno,
				const struct value *val, LONGEST *result);

extern struct value *value_field_bitfield (struct type *type, int fieldno,
					   const gdb_byte *valaddr,
					   LONGEST embedded_offset,
					   const struct value *val);

extern void pack_long (gdb_byte *buf, struct type *type, LONGEST num);

extern struct value *value_from_longest (struct type *type, LONGEST num);
extern struct value *value_from_ulongest (struct type *type, ULONGEST num);
extern struct value *value_from_pointer (struct type *type, CORE_ADDR addr);
extern struct value *value_from_host_double (struct type *type, double d);
extern struct value *value_from_history_ref (const char *, const char **);
extern struct value *value_from_component (struct value *, struct type *,
					   LONGEST);

/* Convert the value V into a newly allocated value.  */
extern struct value *value_from_mpz (struct type *type, const gdb_mpz &v);

extern struct value *value_at (struct type *type, CORE_ADDR addr);

/* Return a new value given a type and an address.  The new value is
   lazy.  If FRAME is given, it is used when resolving dynamic
   properties.  */

extern struct value *value_at_lazy (struct type *type, CORE_ADDR addr,
				    frame_info_ptr frame = nullptr);

/* Like value_at, but ensures that the result is marked not_lval.
   This can be important if the memory is "volatile".  */
extern struct value *value_at_non_lval (struct type *type, CORE_ADDR addr);

extern struct value *value_from_contents_and_address_unresolved
     (struct type *, const gdb_byte *, CORE_ADDR);
extern struct value *value_from_contents_and_address
     (struct type *, const gdb_byte *, CORE_ADDR,
      frame_info_ptr frame = nullptr);
extern struct value *value_from_contents (struct type *, const gdb_byte *);

extern value *default_value_from_register (gdbarch *gdbarch, type *type,
					   int regnum,
					   const frame_info_ptr &this_frame);

extern struct value *value_from_register (struct type *type, int regnum,
					  frame_info_ptr frame);

extern CORE_ADDR address_from_register (int regnum,
					frame_info_ptr frame);

extern struct value *value_of_variable (struct symbol *var,
					const struct block *b);

extern struct value *address_of_variable (struct symbol *var,
					  const struct block *b);

/* Return a value with the contents of register REGNUM as found in the frame
   previous to NEXT_FRAME.  */

extern value *value_of_register (int regnum, frame_info_ptr next_frame);

/* Same as the above, but the value is not fetched.  */

extern value *value_of_register_lazy (frame_info_ptr next_frame, int regnum);

/* Return the symbol's reading requirement.  */

extern enum symbol_needs_kind symbol_read_needs (struct symbol *);

/* Return true if the symbol needs a frame.  This is a wrapper for
   symbol_read_needs that simply checks for SYMBOL_NEEDS_FRAME.  */

extern int symbol_read_needs_frame (struct symbol *);

extern struct value *read_var_value (struct symbol *var,
				     const struct block *var_block,
				     frame_info_ptr frame);

extern struct value *allocate_repeat_value (struct type *type, int count);

extern struct value *value_mark (void);

extern void value_free_to_mark (const struct value *mark);

/* A helper class that uses value_mark at construction time and calls
   value_free_to_mark in the destructor.  This is used to clear out
   temporary values created during the lifetime of this object.  */
class scoped_value_mark
{
 public:

  scoped_value_mark ()
    : m_value (value_mark ())
  {
  }

  ~scoped_value_mark ()
  {
    free_to_mark ();
  }

  scoped_value_mark (scoped_value_mark &&other) = default;

  DISABLE_COPY_AND_ASSIGN (scoped_value_mark);

  /* Free the values currently on the value stack.  */
  void free_to_mark ()
  {
    if (!m_freed)
      {
	value_free_to_mark (m_value);
	m_freed = true;
      }
  }

 private:

  const struct value *m_value;
  bool m_freed = false;
};

/* Create not_lval value representing a NULL-terminated C string.  The
   resulting value has type TYPE_CODE_ARRAY.  The string passed in should
   not include embedded null characters.

   PTR points to the string data; COUNT is number of characters (does
   not include the NULL terminator) pointed to by PTR, each character is of
   type (and size of) CHAR_TYPE.  */

extern struct value *value_cstring (const gdb_byte *ptr, ssize_t count,
				    struct type *char_type);

/* Specialisation of value_cstring above.  In this case PTR points to
   single byte characters.  CHAR_TYPE must have a length of 1.  */
inline struct value *value_cstring (const char *ptr, ssize_t count,
				    struct type *char_type)
{
  gdb_assert (char_type->length () == 1);
  return value_cstring ((const gdb_byte *) ptr, count, char_type);
}

/* Create a not_lval value with type TYPE_CODE_STRING, the resulting value
   has type TYPE_CODE_STRING.

   PTR points to the string data; COUNT is number of characters pointed to
   by PTR, each character has the type (and size of) CHAR_TYPE.

   Note that string types are like array of char types with a lower bound
   defined by the language (usually zero or one).  Also the string may
   contain embedded null characters.  */

extern struct value *value_string (const gdb_byte *ptr, ssize_t count,
				   struct type *char_type);

/* Specialisation of value_string above.  In this case PTR points to
   single byte characters.  CHAR_TYPE must have a length of 1.  */
inline struct value *value_string (const char *ptr, ssize_t count,
				   struct type *char_type)
{
  gdb_assert (char_type->length () == 1);
  return value_string ((const gdb_byte *) ptr, count, char_type);
}

extern struct value *value_array (int lowbound,
				  gdb::array_view<struct value *> elemvec);

extern struct value *value_concat (struct value *arg1, struct value *arg2);

extern struct value *value_binop (struct value *arg1, struct value *arg2,
				  enum exp_opcode op);

extern struct value *value_ptradd (struct value *arg1, LONGEST arg2);

extern LONGEST value_ptrdiff (struct value *arg1, struct value *arg2);

/* Return true if VAL does not live in target memory, but should in order
   to operate on it.  Otherwise return false.  */

extern bool value_must_coerce_to_target (struct value *arg1);

extern struct value *value_coerce_to_target (struct value *arg1);

extern struct value *value_coerce_array (struct value *arg1);

extern struct value *value_coerce_function (struct value *arg1);

extern struct value *value_ind (struct value *arg1);

extern struct value *value_addr (struct value *arg1);

extern struct value *value_ref (struct value *arg1, enum type_code refcode);

extern struct value *value_assign (struct value *toval,
				   struct value *fromval);

/* The unary + operation.  */
extern struct value *value_pos (struct value *arg1);

/* The unary - operation.  */
extern struct value *value_neg (struct value *arg1);

/* The unary ~ operation -- but note that it also implements the GCC
   extension, where ~ of a complex number is the complex
   conjugate.  */
extern struct value *value_complement (struct value *arg1);

extern struct value *value_struct_elt (struct value **argp,
				       std::optional<gdb::array_view <value *>> args,
				       const char *name, int *static_memfuncp,
				       const char *err);

extern struct value *value_struct_elt_bitpos (struct value **argp,
					      int bitpos,
					      struct type *field_type,
					      const char *err);

extern struct value *value_aggregate_elt (struct type *curtype,
					  const char *name,
					  struct type *expect_type,
					  int want_address,
					  enum noside noside);

extern struct value *value_static_field (struct type *type, int fieldno);

enum oload_search_type { NON_METHOD, METHOD, BOTH };

extern int find_overload_match (gdb::array_view<value *> args,
				const char *name,
				enum oload_search_type method,
				struct value **objp, struct symbol *fsym,
				struct value **valp, struct symbol **symp,
				int *staticp, const int no_adl,
				enum noside noside);

extern struct value *value_field (struct value *arg1, int fieldno);

extern struct type *value_rtti_indirect_type (struct value *, int *, LONGEST *,
					      int *);

extern struct value *value_full_object (struct value *, struct type *, int,
					int, int);

extern struct value *value_cast_pointers (struct type *, struct value *, int);

extern struct value *value_cast (struct type *type, struct value *arg2);

extern struct value *value_reinterpret_cast (struct type *type,
					     struct value *arg);

extern struct value *value_dynamic_cast (struct type *type, struct value *arg);

extern struct value *value_one (struct type *type);

extern struct value *value_repeat (struct value *arg1, int count);

extern struct value *value_subscript (struct value *array, LONGEST index);

/* Assuming VAL is array-like (see type::is_array_like), return an
   array form of VAL.  */
extern struct value *value_to_array (struct value *val);

extern struct value *value_bitstring_subscript (struct type *type,
						struct value *bitstring,
						LONGEST index);

extern struct value *register_value_being_returned (struct type *valtype,
						    struct regcache *retbuf);

extern int value_bit_index (struct type *type, const gdb_byte *addr,
			    int index);

extern enum return_value_convention
struct_return_convention (struct gdbarch *gdbarch, struct value *function,
			  struct type *value_type);

extern int using_struct_return (struct gdbarch *gdbarch,
				struct value *function,
				struct type *value_type);

extern value *evaluate_var_value (enum noside noside, const block *blk,
				  symbol *var);

extern value *evaluate_var_msym_value (enum noside noside,
				       struct objfile *objfile,
				       minimal_symbol *msymbol);

namespace expr { class operation; };
extern void fetch_subexp_value (struct expression *exp,
				expr::operation *op,
				struct value **valp, struct value **resultp,
				std::vector<value_ref_ptr> *val_chain,
				bool preserve_errors);

extern struct value *parse_and_eval (const char *exp, parser_flags flags = 0);

extern struct value *parse_to_comma_and_eval (const char **expp);

extern struct type *parse_and_eval_type (const char *p, int length);

extern CORE_ADDR parse_and_eval_address (const char *exp);

extern LONGEST parse_and_eval_long (const char *exp);

extern void unop_promote (const struct language_defn *language,
			  struct gdbarch *gdbarch,
			  struct value **arg1);

extern void binop_promote (const struct language_defn *language,
			   struct gdbarch *gdbarch,
			   struct value **arg1, struct value **arg2);

extern struct value *access_value_history (int num);

/* Return the number of items in the value history.  */

extern ULONGEST value_history_count ();

extern struct value *value_of_internalvar (struct gdbarch *gdbarch,
					   struct internalvar *var);

extern int get_internalvar_integer (struct internalvar *var, LONGEST *l);

extern void set_internalvar (struct internalvar *var, struct value *val);

extern void set_internalvar_integer (struct internalvar *var, LONGEST l);

extern void set_internalvar_string (struct internalvar *var,
				    const char *string);

extern void clear_internalvar (struct internalvar *var);

extern void set_internalvar_component (struct internalvar *var,
				       LONGEST offset,
				       LONGEST bitpos, LONGEST bitsize,
				       struct value *newvalue);

extern struct internalvar *lookup_only_internalvar (const char *name);

extern struct internalvar *create_internalvar (const char *name);

extern void complete_internalvar (completion_tracker &tracker,
				  const char *name);

/* An internalvar can be dynamically computed by supplying a vector of
   function pointers to perform various operations.  */

struct internalvar_funcs
{
  /* Compute the value of the variable.  The DATA argument passed to
     the function is the same argument that was passed to
     `create_internalvar_type_lazy'.  */

  struct value *(*make_value) (struct gdbarch *arch,
			       struct internalvar *var,
			       void *data);

  /* Update the agent expression EXPR with bytecode to compute the
     value.  VALUE is the agent value we are updating.  The DATA
     argument passed to this function is the same argument that was
     passed to `create_internalvar_type_lazy'.  If this pointer is
     NULL, then the internalvar cannot be compiled to an agent
     expression.  */

  void (*compile_to_ax) (struct internalvar *var,
			 struct agent_expr *expr,
			 struct axs_value *value,
			 void *data);
};

extern struct internalvar *create_internalvar_type_lazy (const char *name,
				const struct internalvar_funcs *funcs,
				void *data);

/* Compile an internal variable to an agent expression.  VAR is the
   variable to compile; EXPR and VALUE are the agent expression we are
   updating.  This will return 0 if there is no known way to compile
   VAR, and 1 if VAR was successfully compiled.  It may also throw an
   exception on error.  */

extern int compile_internalvar_to_ax (struct internalvar *var,
				      struct agent_expr *expr,
				      struct axs_value *value);

extern struct internalvar *lookup_internalvar (const char *name);

extern int value_equal (struct value *arg1, struct value *arg2);

extern int value_equal_contents (struct value *arg1, struct value *arg2);

extern int value_less (struct value *arg1, struct value *arg2);

/* Simulate the C operator ! -- return true if ARG1 contains zero.  */
extern bool value_logical_not (struct value *arg1);

/* Returns true if the value VAL represents a true value.  */
static inline bool
value_true (struct value *val)
{
  return !value_logical_not (val);
}

/* C++ */

extern struct value *value_of_this (const struct language_defn *lang);

extern struct value *value_of_this_silent (const struct language_defn *lang);

extern struct value *value_x_binop (struct value *arg1, struct value *arg2,
				    enum exp_opcode op,
				    enum exp_opcode otherop,
				    enum noside noside);

extern struct value *value_x_unop (struct value *arg1, enum exp_opcode op,
				   enum noside noside);

extern struct value *value_fn_field (struct value **arg1p, struct fn_field *f,
				     int j, struct type *type, LONGEST offset);

extern int binop_types_user_defined_p (enum exp_opcode op,
				       struct type *type1,
				       struct type *type2);

extern int binop_user_defined_p (enum exp_opcode op, struct value *arg1,
				 struct value *arg2);

extern int unop_user_defined_p (enum exp_opcode op, struct value *arg1);

extern int destructor_name_p (const char *name, struct type *type);

extern value_ref_ptr release_value (struct value *val);

extern void modify_field (struct type *type, gdb_byte *addr,
			  LONGEST fieldval, LONGEST bitpos, LONGEST bitsize);

extern void type_print (struct type *type, const char *varstring,
			struct ui_file *stream, int show);

extern std::string type_to_string (struct type *type);

extern gdb_byte *baseclass_addr (struct type *type, int index,
				 gdb_byte *valaddr,
				 struct value **valuep, int *errp);

extern void print_longest (struct ui_file *stream, int format,
			   int use_local, LONGEST val);

extern void print_floating (const gdb_byte *valaddr, struct type *type,
			    struct ui_file *stream);

extern void value_print (struct value *val, struct ui_file *stream,
			 const struct value_print_options *options);

/* Release values from the value chain and return them.  Values
   created after MARK are released.  If MARK is nullptr, or if MARK is
   not found on the value chain, then all values are released.  Values
   are returned in reverse order of creation; that is, newest
   first.  */

extern std::vector<value_ref_ptr> value_release_to_mark
    (const struct value *mark);

extern void common_val_print (struct value *val,
			      struct ui_file *stream, int recurse,
			      const struct value_print_options *options,
			      const struct language_defn *language);

extern int val_print_string (struct type *elttype, const char *encoding,
			     CORE_ADDR addr, int len,
			     struct ui_file *stream,
			     const struct value_print_options *options);

extern void print_variable_and_value (const char *name,
				      struct symbol *var,
				      frame_info_ptr frame,
				      struct ui_file *stream,
				      int indent);

extern void typedef_print (struct type *type, struct symbol *news,
			   struct ui_file *stream);

extern const char *internalvar_name (const struct internalvar *var);

extern void preserve_values (struct objfile *);

/* From values.c */

extern struct value *make_cv_value (int, int, struct value *);

/* From valops.c */

extern struct value *varying_to_slice (struct value *);

extern struct value *value_slice (struct value *, int, int);

/* Create a complex number.  The type is the complex type; the values
   are cast to the underlying scalar type before the complex number is
   created.  */

extern struct value *value_literal_complex (struct value *, struct value *,
					    struct type *);

/* Return the real part of a complex value.  */

extern struct value *value_real_part (struct value *value);

/* Return the imaginary part of a complex value.  */

extern struct value *value_imaginary_part (struct value *value);

extern struct value *find_function_in_inferior (const char *,
						struct objfile **);

extern struct value *value_allocate_space_in_inferior (int);

/* User function handler.  */

typedef struct value *(*internal_function_fn) (struct gdbarch *gdbarch,
					       const struct language_defn *language,
					       void *cookie,
					       int argc,
					       struct value **argv);

/* Add a new internal function.  NAME is the name of the function; DOC
   is a documentation string describing the function.  HANDLER is
   called when the function is invoked.  COOKIE is an arbitrary
   pointer which is passed to HANDLER and is intended for "user
   data".  */

extern void add_internal_function (const char *name, const char *doc,
				   internal_function_fn handler,
				   void *cookie);

/* This overload takes an allocated documentation string.  */

extern void add_internal_function (gdb::unique_xmalloc_ptr<char> &&name,
				   gdb::unique_xmalloc_ptr<char> &&doc,
				   internal_function_fn handler,
				   void *cookie);

struct value *call_internal_function (struct gdbarch *gdbarch,
				      const struct language_defn *language,
				      struct value *function,
				      int argc, struct value **argv);

const char *value_internal_function_name (struct value *);

/* Destroy the values currently allocated.  This is called when GDB is
   exiting (e.g., on quit_force).  */
extern void finalize_values ();

/* Convert VALUE to a gdb_mpq.  The caller must ensure that VALUE is
   of floating-point, fixed-point, or integer type.  */
extern gdb_mpq value_to_gdb_mpq (struct value *value);

/* Return true if LEN (in bytes) exceeds the max-value-size setting,
   otherwise, return false.  If the user has disabled (set to unlimited)
   the max-value-size setting then this function will always return false.  */
extern bool exceeds_max_value_size (ULONGEST length);

/* While an instance of this class is live, and array values that are
   created, that are larger than max_value_size, will be restricted in size
   to a particular number of elements.  */

struct scoped_array_length_limiting
{
  /* Limit any large array values to only contain ELEMENTS elements.  */
  scoped_array_length_limiting (int elements);

  /* Restore the previous array value limit.  */
  ~scoped_array_length_limiting ();

private:
  /* Used to hold the previous array value element limit.  */
  std::optional<int> m_old_value;
};

/* Helpers for building pseudo register values from raw registers.  */

/* Create a value for pseudo register PSEUDO_REG_NUM by using bytes from
   raw register RAW_REG_NUM starting at RAW_OFFSET.

   The size of the pseudo register specifies how many bytes to use.  The
   offset plus the size must not overflow the raw register's size.  */

value *pseudo_from_raw_part (frame_info_ptr next_frame, int pseudo_reg_num,
			     int raw_reg_num, int raw_offset);

/* Write PSEUDO_BUF, the contents of a pseudo register, to part of raw register
   RAW_REG_NUM starting at RAW_OFFSET.  */

void pseudo_to_raw_part (frame_info_ptr next_frame,
			 gdb::array_view<const gdb_byte> pseudo_buf,
			 int raw_reg_num, int raw_offset);

/* Create a value for pseudo register PSEUDO_REG_NUM by concatenating raw
   registers RAW_REG_1_NUM and RAW_REG_2_NUM.

   The sum of the sizes of raw registers must be equal to the size of the
   pseudo register.  */

value *pseudo_from_concat_raw (frame_info_ptr next_frame, int pseudo_reg_num,
			       int raw_reg_1_num, int raw_reg_2_num);

/* Write PSEUDO_BUF, the contents of a pseudo register, to the two raw registers
   RAW_REG_1_NUM and RAW_REG_2_NUM.  */

void pseudo_to_concat_raw (frame_info_ptr next_frame,
			   gdb::array_view<const gdb_byte> pseudo_buf,
			   int raw_reg_1_num, int raw_reg_2_num);

/* Same as the above, but with three raw registers.  */

value *pseudo_from_concat_raw (frame_info_ptr next_frame, int pseudo_reg_num,
			       int raw_reg_1_num, int raw_reg_2_num,
			       int raw_reg_3_num);

/* Write PSEUDO_BUF, the contents of a pseudo register, to the three raw
   registers RAW_REG_1_NUM, RAW_REG_2_NUM and RAW_REG_3_NUM.  */

void pseudo_to_concat_raw (frame_info_ptr next_frame,
			   gdb::array_view<const gdb_byte> pseudo_buf,
			   int raw_reg_1_num, int raw_reg_2_num,
			   int raw_reg_3_num);

#endif /* !defined (VALUE_H) */

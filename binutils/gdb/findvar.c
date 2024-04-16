/* Find a variable's value in memory, for GDB, the GNU debugger.

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

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "value.h"
#include "gdbcore.h"
#include "inferior.h"
#include "target.h"
#include "symfile.h"
#include "regcache.h"
#include "user-regs.h"
#include "block.h"
#include "objfiles.h"
#include "language.h"
#include "gdbsupport/selftest.h"

/* Basic byte-swapping routines.  All 'extract' functions return a
   host-format integer from a target-format integer at ADDR which is
   LEN bytes long.  */

#if TARGET_CHAR_BIT != 8 || HOST_CHAR_BIT != 8
  /* 8 bit characters are a pretty safe assumption these days, so we
     assume it throughout all these swapping routines.  If we had to deal with
     9 bit characters, we would need to make len be in bits and would have
     to re-write these routines...  */
you lose
#endif

template<typename T, typename>
T
extract_integer (gdb::array_view<const gdb_byte> buf, enum bfd_endian byte_order)
{
  typename std::make_unsigned<T>::type retval = 0;

  if (buf.size () > (int) sizeof (T))
    error (_("\
That operation is not available on integers of more than %d bytes."),
	   (int) sizeof (T));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      size_t i = 0;

      if (std::is_signed<T>::value)
	{
	  /* Do the sign extension once at the start.  */
	  retval = ((LONGEST) buf[i] ^ 0x80) - 0x80;
	  ++i;
	}
      for (; i < buf.size (); ++i)
	retval = (retval << 8) | buf[i];
    }
  else
    {
      ssize_t i = buf.size () - 1;

      if (std::is_signed<T>::value)
	{
	  /* Do the sign extension once at the start.  */
	  retval = ((LONGEST) buf[i] ^ 0x80) - 0x80;
	  --i;
	}
      for (; i >= 0; --i)
	retval = (retval << 8) | buf[i];
    }
  return retval;
}

/* Explicit instantiations.  */
template LONGEST extract_integer<LONGEST> (gdb::array_view<const gdb_byte> buf,
					   enum bfd_endian byte_order);
template ULONGEST extract_integer<ULONGEST>
  (gdb::array_view<const gdb_byte> buf, enum bfd_endian byte_order);

/* Sometimes a long long unsigned integer can be extracted as a
   LONGEST value.  This is done so that we can print these values
   better.  If this integer can be converted to a LONGEST, this
   function returns 1 and sets *PVAL.  Otherwise it returns 0.  */

int
extract_long_unsigned_integer (const gdb_byte *addr, int orig_len,
			       enum bfd_endian byte_order, LONGEST *pval)
{
  const gdb_byte *p;
  const gdb_byte *first_addr;
  int len;

  len = orig_len;
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = addr;
	   len > (int) sizeof (LONGEST) && p < addr + orig_len;
	   p++)
	{
	  if (*p == 0)
	    len--;
	  else
	    break;
	}
      first_addr = p;
    }
  else
    {
      first_addr = addr;
      for (p = addr + orig_len - 1;
	   len > (int) sizeof (LONGEST) && p >= addr;
	   p--)
	{
	  if (*p == 0)
	    len--;
	  else
	    break;
	}
    }

  if (len <= (int) sizeof (LONGEST))
    {
      *pval = (LONGEST) extract_unsigned_integer (first_addr,
						  sizeof (LONGEST),
						  byte_order);
      return 1;
    }

  return 0;
}


/* Treat the bytes at BUF as a pointer of type TYPE, and return the
   address it represents.  */
CORE_ADDR
extract_typed_address (const gdb_byte *buf, struct type *type)
{
  gdb_assert (type->is_pointer_or_reference ());
  return gdbarch_pointer_to_address (type->arch (), type, buf);
}

/* All 'store' functions accept a host-format integer and store a
   target-format integer at ADDR which is LEN bytes long.  */
template<typename T, typename>
void
store_integer (gdb::array_view<gdb_byte> dst, enum bfd_endian byte_order,
	       T val)
{
  gdb_byte *p;
  gdb_byte *startaddr = dst.data ();
  gdb_byte *endaddr = startaddr + dst.size ();

  /* Start at the least significant end of the integer, and work towards
     the most significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = endaddr - 1; p >= startaddr; --p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = startaddr; p < endaddr; ++p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
}

/* Explicit instantiations.  */
template void store_integer (gdb::array_view<gdb_byte> dst,
			     bfd_endian byte_order, LONGEST val);

template void store_integer (gdb::array_view<gdb_byte> dst,
			     bfd_endian byte_order, ULONGEST val);

/* Store the address ADDR as a pointer of type TYPE at BUF, in target
   form.  */
void
store_typed_address (gdb_byte *buf, struct type *type, CORE_ADDR addr)
{
  gdb_assert (type->is_pointer_or_reference ());
  gdbarch_address_to_pointer (type->arch (), type, buf, addr);
}

/* Copy a value from SOURCE of size SOURCE_SIZE bytes to DEST of size DEST_SIZE
   bytes.  If SOURCE_SIZE is greater than DEST_SIZE, then truncate the most
   significant bytes.  If SOURCE_SIZE is less than DEST_SIZE then either sign
   or zero extended according to IS_SIGNED.  Values are stored in memory with
   endianness BYTE_ORDER.  */

void
copy_integer_to_size (gdb_byte *dest, int dest_size, const gdb_byte *source,
		      int source_size, bool is_signed,
		      enum bfd_endian byte_order)
{
  signed int size_diff = dest_size - source_size;

  /* Copy across everything from SOURCE that can fit into DEST.  */

  if (byte_order == BFD_ENDIAN_BIG && size_diff > 0)
    memcpy (dest + size_diff, source, source_size);
  else if (byte_order == BFD_ENDIAN_BIG && size_diff < 0)
    memcpy (dest, source - size_diff, dest_size);
  else
    memcpy (dest, source, std::min (source_size, dest_size));

  /* Fill the remaining space in DEST by either zero extending or sign
     extending.  */

  if (size_diff > 0)
    {
      gdb_byte extension = 0;
      if (is_signed
	  && ((byte_order != BFD_ENDIAN_BIG && source[source_size - 1] & 0x80)
	      || (byte_order == BFD_ENDIAN_BIG && source[0] & 0x80)))
	extension = 0xff;

      /* Extend into MSBs of SOURCE.  */
      if (byte_order == BFD_ENDIAN_BIG)
	memset (dest, extension, size_diff);
      else
	memset (dest + source_size, extension, size_diff);
    }
}

/* See value.h.  */

value *
value_of_register (int regnum, frame_info_ptr next_frame)
{
  gdbarch *gdbarch = frame_unwind_arch (next_frame);

  /* User registers lie completely outside of the range of normal
     registers.  Catch them early so that the target never sees them.  */
  if (regnum >= gdbarch_num_cooked_regs (gdbarch))
    return value_of_user_reg (regnum, get_prev_frame_always (next_frame));

  value *reg_val = value_of_register_lazy (next_frame, regnum);
  reg_val->fetch_lazy ();
  return reg_val;
}

/* See value.h.  */

value *
value_of_register_lazy (frame_info_ptr next_frame, int regnum)
{
  gdbarch *gdbarch = frame_unwind_arch (next_frame);

  gdb_assert (regnum < gdbarch_num_cooked_regs (gdbarch));
  gdb_assert (next_frame != nullptr);

  return value::allocate_register_lazy (next_frame, regnum);
}

/* Given a pointer of type TYPE in target form in BUF, return the
   address it represents.  */
CORE_ADDR
unsigned_pointer_to_address (struct gdbarch *gdbarch,
			     struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = type_byte_order (type);

  return extract_unsigned_integer (buf, type->length (), byte_order);
}

CORE_ADDR
signed_pointer_to_address (struct gdbarch *gdbarch,
			   struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = type_byte_order (type);

  return extract_signed_integer (buf, type->length (), byte_order);
}

/* Given an address, store it as a pointer of type TYPE in target
   format in BUF.  */
void
unsigned_address_to_pointer (struct gdbarch *gdbarch, struct type *type,
			     gdb_byte *buf, CORE_ADDR addr)
{
  enum bfd_endian byte_order = type_byte_order (type);

  store_unsigned_integer (buf, type->length (), byte_order, addr);
}

void
address_to_signed_pointer (struct gdbarch *gdbarch, struct type *type,
			   gdb_byte *buf, CORE_ADDR addr)
{
  enum bfd_endian byte_order = type_byte_order (type);

  store_signed_integer (buf, type->length (), byte_order, addr);
}

/* See value.h.  */

enum symbol_needs_kind
symbol_read_needs (struct symbol *sym)
{
  if (SYMBOL_COMPUTED_OPS (sym) != NULL)
    return SYMBOL_COMPUTED_OPS (sym)->get_symbol_read_needs (sym);

  switch (sym->aclass ())
    {
      /* All cases listed explicitly so that gcc -Wall will detect it if
	 we failed to consider one.  */
    case LOC_COMPUTED:
      gdb_assert_not_reached ("LOC_COMPUTED variable missing a method");

    case LOC_REGISTER:
    case LOC_ARG:
    case LOC_REF_ARG:
    case LOC_REGPARM_ADDR:
    case LOC_LOCAL:
      return SYMBOL_NEEDS_FRAME;

    case LOC_UNDEF:
    case LOC_CONST:
    case LOC_STATIC:
    case LOC_TYPEDEF:

    case LOC_LABEL:
      /* Getting the address of a label can be done independently of the block,
	 even if some *uses* of that address wouldn't work so well without
	 the right frame.  */

    case LOC_BLOCK:
    case LOC_CONST_BYTES:
    case LOC_UNRESOLVED:
    case LOC_OPTIMIZED_OUT:
      return SYMBOL_NEEDS_NONE;
    }
  return SYMBOL_NEEDS_FRAME;
}

/* See value.h.  */

int
symbol_read_needs_frame (struct symbol *sym)
{
  return symbol_read_needs (sym) == SYMBOL_NEEDS_FRAME;
}

/* Assuming VAR is a symbol that can be reached from FRAME thanks to lexical
   rules, look for the frame that is actually hosting VAR and return it.  If,
   for some reason, we found no such frame, return NULL.

   This kind of computation is necessary to correctly handle lexically nested
   functions.

   Note that in some cases, we know what scope VAR comes from but we cannot
   reach the specific frame that hosts the instance of VAR we are looking for.
   For backward compatibility purposes (with old compilers), we then look for
   the first frame that can host it.  */

static frame_info_ptr
get_hosting_frame (struct symbol *var, const struct block *var_block,
		   frame_info_ptr frame)
{
  const struct block *frame_block = NULL;

  if (!symbol_read_needs_frame (var))
    return NULL;

  /* Some symbols for local variables have no block: this happens when they are
     not produced by a debug information reader, for instance when GDB creates
     synthetic symbols.  Without block information, we must assume they are
     local to FRAME. In this case, there is nothing to do.  */
  else if (var_block == NULL)
    return frame;

  /* We currently assume that all symbols with a location list need a frame.
     This is true in practice because selecting the location description
     requires to compute the CFA, hence requires a frame.  However we have
     tests that embed global/static symbols with null location lists.
     We want to get <optimized out> instead of <frame required> when evaluating
     them so return a frame instead of raising an error.  */
  else if (var_block->is_global_block () || var_block->is_static_block ())
    return frame;

  /* We have to handle the "my_func::my_local_var" notation.  This requires us
     to look for upper frames when we find no block for the current frame: here
     and below, handle when frame_block == NULL.  */
  if (frame != NULL)
    frame_block = get_frame_block (frame, NULL);

  /* Climb up the call stack until reaching the frame we are looking for.  */
  while (frame != NULL && frame_block != var_block)
    {
      /* Stacks can be quite deep: give the user a chance to stop this.  */
      QUIT;

      if (frame_block == NULL)
	{
	  frame = get_prev_frame (frame);
	  if (frame == NULL)
	    break;
	  frame_block = get_frame_block (frame, NULL);
	}

      /* If we failed to find the proper frame, fallback to the heuristic
	 method below.  */
      else if (frame_block->is_global_block ())
	{
	  frame = NULL;
	  break;
	}

      /* Assuming we have a block for this frame: if we are at the function
	 level, the immediate upper lexical block is in an outer function:
	 follow the static link.  */
      else if (frame_block->function () != nullptr)
	{
	  frame = frame_follow_static_link (frame);
	  if (frame != nullptr)
	    {
	      frame_block = get_frame_block (frame, nullptr);
	      if (frame_block == nullptr)
		frame = nullptr;
	    }
	}

      else
	/* We must be in some function nested lexical block.  Just get the
	   outer block: both must share the same frame.  */
	frame_block = frame_block->superblock ();
    }

  /* Old compilers may not provide a static link, or they may provide an
     invalid one.  For such cases, fallback on the old way to evaluate
     non-local references: just climb up the call stack and pick the first
     frame that contains the variable we are looking for.  */
  if (frame == NULL)
    {
      frame = block_innermost_frame (var_block);
      if (frame == NULL)
	{
	  if (var_block->function ()
	      && !var_block->inlined_p ()
	      && var_block->function ()->print_name ())
	    error (_("No frame is currently executing in block %s."),
		   var_block->function ()->print_name ());
	  else
	    error (_("No frame is currently executing in specified"
		     " block"));
	}
    }

  return frame;
}

/* See language.h.  */

struct value *
language_defn::read_var_value (struct symbol *var,
			       const struct block *var_block,
			       frame_info_ptr frame) const
{
  struct value *v;
  struct type *type = var->type ();
  CORE_ADDR addr;
  enum symbol_needs_kind sym_need;

  /* Call check_typedef on our type to make sure that, if TYPE is
     a TYPE_CODE_TYPEDEF, its length is set to the length of the target type
     instead of zero.  However, we do not replace the typedef type by the
     target type, because we want to keep the typedef in order to be able to
     set the returned value type description correctly.  */
  check_typedef (type);

  sym_need = symbol_read_needs (var);
  if (sym_need == SYMBOL_NEEDS_FRAME)
    gdb_assert (frame != NULL);
  else if (sym_need == SYMBOL_NEEDS_REGISTERS && !target_has_registers ())
    error (_("Cannot read `%s' without registers"), var->print_name ());

  if (frame != NULL)
    frame = get_hosting_frame (var, var_block, frame);

  if (SYMBOL_COMPUTED_OPS (var) != NULL)
    return SYMBOL_COMPUTED_OPS (var)->read_variable (var, frame);

  switch (var->aclass ())
    {
    case LOC_CONST:
      if (is_dynamic_type (type))
	{
	  gdb_byte bytes[sizeof (LONGEST)];

	  size_t len = std::min (sizeof (LONGEST), (size_t) type->length ());
	  store_unsigned_integer (bytes, len,
				  type_byte_order (type),
				  var->value_longest ());
	  gdb::array_view<const gdb_byte> view (bytes, len);

	  /* Value is a constant byte-sequence.  */
	  type = resolve_dynamic_type (type, view, /* Unused address.  */ 0);
	}
      /* Put the constant back in target format. */
      v = value::allocate (type);
      store_signed_integer (v->contents_raw ().data (), type->length (),
			    type_byte_order (type), var->value_longest ());
      v->set_lval (not_lval);
      return v;

    case LOC_LABEL:
      {
	/* Put the constant back in target format.  */
	if (overlay_debugging)
	  {
	    struct objfile *var_objfile = var->objfile ();
	    addr = symbol_overlayed_address (var->value_address (),
					     var->obj_section (var_objfile));
	  }
	else
	  addr = var->value_address ();

	/* First convert the CORE_ADDR to a function pointer type, this
	   ensures the gdbarch knows what type of pointer we are
	   manipulating when value_from_pointer is called.  */
	type = builtin_type (var->arch ())->builtin_func_ptr;
	v = value_from_pointer (type, addr);

	/* But we want to present the value as 'void *', so cast it to the
	   required type now, this will not change the values bit
	   representation.  */
	struct type *void_ptr_type
	  = builtin_type (var->arch ())->builtin_data_ptr;
	v = value_cast_pointers (void_ptr_type, v, 0);
	v->set_lval (not_lval);
	return v;
      }

    case LOC_CONST_BYTES:
      if (is_dynamic_type (type))
	{
	  gdb::array_view<const gdb_byte> view (var->value_bytes (),
						type->length ());

	  /* Value is a constant byte-sequence.  */
	  type = resolve_dynamic_type (type, view, /* Unused address.  */ 0);
	}
      v = value::allocate (type);
      memcpy (v->contents_raw ().data (), var->value_bytes (),
	      type->length ());
      v->set_lval (not_lval);
      return v;

    case LOC_STATIC:
      if (overlay_debugging)
	addr
	  = symbol_overlayed_address (var->value_address (),
				      var->obj_section (var->objfile ()));
      else
	addr = var->value_address ();
      break;

    case LOC_ARG:
      addr = get_frame_args_address (frame);
      if (!addr)
	error (_("Unknown argument list address for `%s'."),
	       var->print_name ());
      addr += var->value_longest ();
      break;

    case LOC_REF_ARG:
      {
	struct value *ref;
	CORE_ADDR argref;

	argref = get_frame_args_address (frame);
	if (!argref)
	  error (_("Unknown argument list address for `%s'."),
		 var->print_name ());
	argref += var->value_longest ();
	ref = value_at (lookup_pointer_type (type), argref);
	addr = value_as_address (ref);
	break;
      }

    case LOC_LOCAL:
      addr = get_frame_locals_address (frame);
      addr += var->value_longest ();
      break;

    case LOC_TYPEDEF:
      error (_("Cannot look up value of a typedef `%s'."),
	     var->print_name ());
      break;

    case LOC_BLOCK:
      if (overlay_debugging)
	addr = symbol_overlayed_address
	  (var->value_block ()->entry_pc (),
	   var->obj_section (var->objfile ()));
      else
	addr = var->value_block ()->entry_pc ();
      break;

    case LOC_REGISTER:
    case LOC_REGPARM_ADDR:
      {
	int regno = SYMBOL_REGISTER_OPS (var)
		      ->register_number (var, get_frame_arch (frame));
	struct value *regval;

	if (var->aclass () == LOC_REGPARM_ADDR)
	  {
	    regval = value_from_register (lookup_pointer_type (type),
					  regno,
					  frame);

	    if (regval == NULL)
	      error (_("Value of register variable not available for `%s'."),
		     var->print_name ());

	    addr = value_as_address (regval);
	  }
	else
	  {
	    regval = value_from_register (type, regno, frame);

	    if (regval == NULL)
	      error (_("Value of register variable not available for `%s'."),
		     var->print_name ());
	    return regval;
	  }
      }
      break;

    case LOC_COMPUTED:
      gdb_assert_not_reached ("LOC_COMPUTED variable missing a method");

    case LOC_UNRESOLVED:
      {
	struct obj_section *obj_section;
	bound_minimal_symbol bmsym;

	gdbarch_iterate_over_objfiles_in_search_order
	  (var->arch (),
	   [var, &bmsym] (objfile *objfile)
	     {
		bmsym = lookup_minimal_symbol (var->linkage_name (), nullptr,
					       objfile);

		/* Stop if a match is found.  */
		return bmsym.minsym != nullptr;
	     },
	   var->objfile ());

	/* If we can't find the minsym there's a problem in the symbol info.
	   The symbol exists in the debug info, but it's missing in the minsym
	   table.  */
	if (bmsym.minsym == nullptr)
	  {
	    const char *flavour_name
	      = objfile_flavour_name (var->objfile ());

	    /* We can't get here unless we've opened the file, so flavour_name
	       can't be NULL.  */
	    gdb_assert (flavour_name != NULL);
	    error (_("Missing %s symbol \"%s\"."),
		   flavour_name, var->linkage_name ());
	  }

	obj_section = bmsym.minsym->obj_section (bmsym.objfile);
	/* Relocate address, unless there is no section or the variable is
	   a TLS variable. */
	if (obj_section == NULL
	    || (obj_section->the_bfd_section->flags & SEC_THREAD_LOCAL) != 0)
	  addr = CORE_ADDR (bmsym.minsym->unrelocated_address ());
	else
	  addr = bmsym.value_address ();
	if (overlay_debugging)
	  addr = symbol_overlayed_address (addr, obj_section);
	/* Determine address of TLS variable. */
	if (obj_section
	    && (obj_section->the_bfd_section->flags & SEC_THREAD_LOCAL) != 0)
	  addr = target_translate_tls_address (obj_section->objfile, addr);
      }
      break;

    case LOC_OPTIMIZED_OUT:
      if (is_dynamic_type (type))
	type = resolve_dynamic_type (type, {}, /* Unused address.  */ 0);
      return value::allocate_optimized_out (type);

    default:
      error (_("Cannot look up value of a botched symbol `%s'."),
	     var->print_name ());
      break;
    }

  v = value_at_lazy (type, addr);
  return v;
}

/* Calls VAR's language read_var_value hook with the given arguments.  */

struct value *
read_var_value (struct symbol *var, const struct block *var_block,
		frame_info_ptr frame)
{
  const struct language_defn *lang = language_def (var->language ());

  gdb_assert (lang != NULL);

  return lang->read_var_value (var, var_block, frame);
}

/* Install default attributes for register values.  */

value *
default_value_from_register (gdbarch *gdbarch, type *type, int regnum,
			     const frame_info_ptr &this_frame)
{
  value *value
    = value::allocate_register (get_next_frame_sentinel_okay (this_frame),
				regnum, type);

  /* Any structure stored in more than one register will always be
     an integral number of registers.  Otherwise, you need to do
     some fiddling with the last register copied here for little
     endian machines.  */
  if (type_byte_order (type) == BFD_ENDIAN_BIG
      && type->length () < register_size (gdbarch, regnum))
    /* Big-endian, and we want less than full size.  */
    value->set_offset (register_size (gdbarch, regnum) - type->length ());
  else
    value->set_offset (0);

  return value;
}

/* VALUE must be an lval_register value.  If regnum is the value's
   associated register number, and len the length of the value's type,
   read one or more registers in VALUE's frame, starting with register REGNUM,
   until we've read LEN bytes.

   If any of the registers we try to read are optimized out, then mark the
   complete resulting value as optimized out.  */

static void
read_frame_register_value (value *value)
{
  gdb_assert (value->lval () == lval_register);

  frame_info_ptr next_frame = frame_find_by_id (value->next_frame_id ());
  gdb_assert (next_frame != nullptr);

  gdbarch *gdbarch = frame_unwind_arch (next_frame);
  LONGEST offset = 0;
  LONGEST reg_offset = value->offset ();
  int regnum = value->regnum ();
  int len = type_length_units (check_typedef (value->type ()));

  /* Skip registers wholly inside of REG_OFFSET.  */
  while (reg_offset >= register_size (gdbarch, regnum))
    {
      reg_offset -= register_size (gdbarch, regnum);
      regnum++;
    }

  /* Copy the data.  */
  while (len > 0)
    {
      struct value *regval = frame_unwind_register_value (next_frame, regnum);
      int reg_len = type_length_units (regval->type ()) - reg_offset;

      /* If the register length is larger than the number of bytes
	 remaining to copy, then only copy the appropriate bytes.  */
      if (reg_len > len)
	reg_len = len;

      regval->contents_copy (value, offset, reg_offset, reg_len);

      offset += reg_len;
      len -= reg_len;
      reg_offset = 0;
      regnum++;
    }
}

/* Return a value of type TYPE, stored in register REGNUM, in frame FRAME.  */

struct value *
value_from_register (struct type *type, int regnum, frame_info_ptr frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct type *type1 = check_typedef (type);
  struct value *v;

  if (gdbarch_convert_register_p (gdbarch, regnum, type1))
    {
      int optim, unavail, ok;

      /* The ISA/ABI need to something weird when obtaining the
	 specified value from this register.  It might need to
	 re-order non-adjacent, starting with REGNUM (see MIPS and
	 i386).  It might need to convert the [float] register into
	 the corresponding [integer] type (see Alpha).  The assumption
	 is that gdbarch_register_to_value populates the entire value
	 including the location.  */
      v = value::allocate_register (get_next_frame_sentinel_okay (frame),
				    regnum, type);
      ok = gdbarch_register_to_value (gdbarch, frame, regnum, type1,
				      v->contents_raw ().data (), &optim,
				      &unavail);

      if (!ok)
	{
	  if (optim)
	    v->mark_bytes_optimized_out (0, type->length ());
	  if (unavail)
	    v->mark_bytes_unavailable (0, type->length ());
	}
    }
  else
    {
      /* Construct the value.  */
      v = gdbarch_value_from_register (gdbarch, type, regnum, frame);

      /* Get the data.  */
      read_frame_register_value (v);
    }

  return v;
}

/* Return contents of register REGNUM in frame FRAME as address.
   Will abort if register value is not available.  */

CORE_ADDR
address_from_register (int regnum, frame_info_ptr frame)
{
  type *type = builtin_type (get_frame_arch (frame))->builtin_data_ptr;
  value_ref_ptr v = release_value (value_from_register (type, regnum, frame));

  if (v->optimized_out ())
    {
      /* This function is used while computing a location expression.
	 Complain about the value being optimized out, rather than
	 letting value_as_address complain about some random register
	 the expression depends on not being saved.  */
      error_value_optimized_out ();
    }

  return value_as_address (v.get ());
}

#if GDB_SELF_TEST
namespace selftests {
namespace findvar_tests {

/* Function to test copy_integer_to_size.  Store SOURCE_VAL with size
   SOURCE_SIZE to a buffer, making sure no sign extending happens at this
   stage.  Copy buffer to a new buffer using copy_integer_to_size.  Extract
   copied value and compare to DEST_VALU.  Copy again with a signed
   copy_integer_to_size and compare to DEST_VALS.  Do everything for both
   LITTLE and BIG target endians.  Use unsigned values throughout to make
   sure there are no implicit sign extensions.  */

static void
do_cint_test (ULONGEST dest_valu, ULONGEST dest_vals, int dest_size,
	      ULONGEST src_val, int src_size)
{
  for (int i = 0; i < 2 ; i++)
    {
      gdb_byte srcbuf[sizeof (ULONGEST)] = {};
      gdb_byte destbuf[sizeof (ULONGEST)] = {};
      enum bfd_endian byte_order = i ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;

      /* Fill the src buffer (and later the dest buffer) with non-zero junk,
	 to ensure zero extensions aren't hidden.  */
      memset (srcbuf, 0xaa, sizeof (srcbuf));

      /* Store (and later extract) using unsigned to ensure there are no sign
	 extensions.  */
      store_unsigned_integer (srcbuf, src_size, byte_order, src_val);

      /* Test unsigned.  */
      memset (destbuf, 0xaa, sizeof (destbuf));
      copy_integer_to_size (destbuf, dest_size, srcbuf, src_size, false,
			    byte_order);
      SELF_CHECK (dest_valu == extract_unsigned_integer (destbuf, dest_size,
							 byte_order));

      /* Test signed.  */
      memset (destbuf, 0xaa, sizeof (destbuf));
      copy_integer_to_size (destbuf, dest_size, srcbuf, src_size, true,
			    byte_order);
      SELF_CHECK (dest_vals == extract_unsigned_integer (destbuf, dest_size,
							 byte_order));
    }
}

static void
copy_integer_to_size_test ()
{
  /* Destination is bigger than the source, which has the signed bit unset.  */
  do_cint_test (0x12345678, 0x12345678, 8, 0x12345678, 4);
  do_cint_test (0x345678, 0x345678, 8, 0x12345678, 3);

  /* Destination is bigger than the source, which has the signed bit set.  */
  do_cint_test (0xdeadbeef, 0xffffffffdeadbeef, 8, 0xdeadbeef, 4);
  do_cint_test (0xadbeef, 0xffffffffffadbeef, 8, 0xdeadbeef, 3);

  /* Destination is smaller than the source.  */
  do_cint_test (0x5678, 0x5678, 2, 0x12345678, 3);
  do_cint_test (0xbeef, 0xbeef, 2, 0xdeadbeef, 3);

  /* Destination and source are the same size.  */
  do_cint_test (0x8765432112345678, 0x8765432112345678, 8, 0x8765432112345678,
		8);
  do_cint_test (0x432112345678, 0x432112345678, 6, 0x8765432112345678, 6);
  do_cint_test (0xfeedbeaddeadbeef, 0xfeedbeaddeadbeef, 8, 0xfeedbeaddeadbeef,
		8);
  do_cint_test (0xbeaddeadbeef, 0xbeaddeadbeef, 6, 0xfeedbeaddeadbeef, 6);

  /* Destination is bigger than the source.  Source is bigger than 32bits.  */
  do_cint_test (0x3412345678, 0x3412345678, 8, 0x3412345678, 6);
  do_cint_test (0xff12345678, 0xff12345678, 8, 0xff12345678, 6);
  do_cint_test (0x432112345678, 0x432112345678, 8, 0x8765432112345678, 6);
  do_cint_test (0xff2112345678, 0xffffff2112345678, 8, 0xffffff2112345678, 6);
}

} // namespace findvar_test
} // namespace selftests

#endif

void _initialize_findvar ();
void
_initialize_findvar ()
{
#if GDB_SELF_TEST
  selftests::register_test
    ("copy_integer_to_size",
     selftests::findvar_tests::copy_integer_to_size_test);
#endif
}

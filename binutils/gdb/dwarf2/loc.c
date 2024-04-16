/* DWARF 2 location expression support for GDB.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Contributed by Daniel Jacobowitz, MontaVista Software, Inc.

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
#include "ui-out.h"
#include "value.h"
#include "frame.h"
#include "gdbcore.h"
#include "target.h"
#include "inferior.h"
#include "ax.h"
#include "ax-gdb.h"
#include "regcache.h"
#include "objfiles.h"
#include "block.h"
#include "gdbcmd.h"
#include "complaints.h"
#include "dwarf2.h"
#include "dwarf2/expr.h"
#include "dwarf2/loc.h"
#include "dwarf2/read.h"
#include "dwarf2/frame.h"
#include "dwarf2/leb.h"
#include "compile/compile.h"
#include "gdbsupport/selftest.h"
#include <algorithm>
#include <vector>
#include <unordered_set>
#include "gdbsupport/underlying.h"
#include "gdbsupport/byte-vector.h"

static struct value *dwarf2_evaluate_loc_desc_full
  (struct type *type, frame_info_ptr frame, const gdb_byte *data,
   size_t size, dwarf2_per_cu_data *per_cu, dwarf2_per_objfile *per_objfile,
   struct type *subobj_type, LONGEST subobj_byte_offset, bool as_lval = true);

/* Until these have formal names, we define these here.
   ref: http://gcc.gnu.org/wiki/DebugFission
   Each entry in .debug_loc.dwo begins with a byte that describes the entry,
   and is then followed by data specific to that entry.  */

enum debug_loc_kind
{
  /* Indicates the end of the list of entries.  */
  DEBUG_LOC_END_OF_LIST = 0,

  /* This is followed by an unsigned LEB128 number that is an index into
     .debug_addr and specifies the base address for all following entries.  */
  DEBUG_LOC_BASE_ADDRESS = 1,

  /* This is followed by two unsigned LEB128 numbers that are indices into
     .debug_addr and specify the beginning and ending addresses, and then
     a normal location expression as in .debug_loc.  */
  DEBUG_LOC_START_END = 2,

  /* This is followed by an unsigned LEB128 number that is an index into
     .debug_addr and specifies the beginning address, and a 4 byte unsigned
     number that specifies the length, and then a normal location expression
     as in .debug_loc.  */
  DEBUG_LOC_START_LENGTH = 3,

  /* This is followed by two unsigned LEB128 operands. The values of these
     operands are the starting and ending offsets, respectively, relative to
     the applicable base address.  */
  DEBUG_LOC_OFFSET_PAIR = 4,

  /* An internal value indicating there is insufficient data.  */
  DEBUG_LOC_BUFFER_OVERFLOW = -1,

  /* An internal value indicating an invalid kind of entry was found.  */
  DEBUG_LOC_INVALID_ENTRY = -2
};

/* Helper function which throws an error if a synthetic pointer is
   invalid.  */

void
invalid_synthetic_pointer (void)
{
  error (_("access outside bounds of object "
	   "referenced via synthetic pointer"));
}

/* Decode the addresses in a non-dwo .debug_loc entry.
   A pointer to the next byte to examine is returned in *NEW_PTR.
   The encoded low,high addresses are return in *LOW,*HIGH.
   The result indicates the kind of entry found.  */

static enum debug_loc_kind
decode_debug_loc_addresses (const gdb_byte *loc_ptr, const gdb_byte *buf_end,
			    const gdb_byte **new_ptr,
			    unrelocated_addr *lowp, unrelocated_addr *highp,
			    enum bfd_endian byte_order,
			    unsigned int addr_size,
			    int signed_addr_p)
{
  CORE_ADDR base_mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));

  if (buf_end - loc_ptr < 2 * addr_size)
    return DEBUG_LOC_BUFFER_OVERFLOW;

  CORE_ADDR low, high;
  if (signed_addr_p)
    low = extract_signed_integer (loc_ptr, addr_size, byte_order);
  else
    low = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
  loc_ptr += addr_size;

  if (signed_addr_p)
    high = extract_signed_integer (loc_ptr, addr_size, byte_order);
  else
    high = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
  loc_ptr += addr_size;

  *new_ptr = loc_ptr;
  *lowp = (unrelocated_addr) low;
  *highp = (unrelocated_addr) high;

  /* A base-address-selection entry.  */
  if ((low & base_mask) == base_mask)
    return DEBUG_LOC_BASE_ADDRESS;

  /* An end-of-list entry.  */
  if (low == 0 && high == 0)
    return DEBUG_LOC_END_OF_LIST;

  /* We want the caller to apply the base address, so we must return
     DEBUG_LOC_OFFSET_PAIR here.  */
  return DEBUG_LOC_OFFSET_PAIR;
}

/* Decode the addresses in .debug_loclists entry.
   A pointer to the next byte to examine is returned in *NEW_PTR.
   The encoded low,high addresses are return in *LOW,*HIGH.
   The result indicates the kind of entry found.  */

static enum debug_loc_kind
decode_debug_loclists_addresses (dwarf2_per_cu_data *per_cu,
				 dwarf2_per_objfile *per_objfile,
				 const gdb_byte *loc_ptr,
				 const gdb_byte *buf_end,
				 const gdb_byte **new_ptr,
				 unrelocated_addr *low,
				 unrelocated_addr *high,
				 enum bfd_endian byte_order,
				 unsigned int addr_size,
				 int signed_addr_p)
{
  uint64_t u64;

  if (loc_ptr == buf_end)
    return DEBUG_LOC_BUFFER_OVERFLOW;

  switch (*loc_ptr++)
    {
    case DW_LLE_base_addressx:
      *low = {};
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	 return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = dwarf2_read_addr_index (per_cu, per_objfile, u64);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_BASE_ADDRESS;

    case DW_LLE_startx_length:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	 return DEBUG_LOC_BUFFER_OVERFLOW;

      *low = dwarf2_read_addr_index (per_cu, per_objfile, u64);
      *high = *low;
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	 return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = (unrelocated_addr) ((uint64_t) *high + u64);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_START_LENGTH;

    case DW_LLE_start_length:
      if (buf_end - loc_ptr < addr_size)
	 return DEBUG_LOC_BUFFER_OVERFLOW;

      if (signed_addr_p)
	*low = (unrelocated_addr) extract_signed_integer (loc_ptr, addr_size,
							  byte_order);
      else
	*low = (unrelocated_addr) extract_unsigned_integer (loc_ptr, addr_size,
							    byte_order);

      loc_ptr += addr_size;
      *high = *low;

      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	 return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = (unrelocated_addr) ((uint64_t) *high + u64);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_START_LENGTH;

    case DW_LLE_end_of_list:
      *new_ptr = loc_ptr;
      return DEBUG_LOC_END_OF_LIST;

    case DW_LLE_base_address:
      if (loc_ptr + addr_size > buf_end)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      if (signed_addr_p)
	*high = (unrelocated_addr) extract_signed_integer (loc_ptr, addr_size,
							   byte_order);
      else
	*high = (unrelocated_addr) extract_unsigned_integer (loc_ptr, addr_size,
							     byte_order);

      loc_ptr += addr_size;
      *new_ptr = loc_ptr;
      return DEBUG_LOC_BASE_ADDRESS;

    case DW_LLE_offset_pair:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *low = (unrelocated_addr) u64;
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = (unrelocated_addr) u64;
      *new_ptr = loc_ptr;
      return DEBUG_LOC_OFFSET_PAIR;

    case DW_LLE_start_end:
      if (loc_ptr + 2 * addr_size > buf_end)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      if (signed_addr_p)
	*low = (unrelocated_addr) extract_signed_integer (loc_ptr, addr_size,
							  byte_order);
      else
	*low = (unrelocated_addr) extract_unsigned_integer (loc_ptr, addr_size,
							    byte_order);

      loc_ptr += addr_size;
      if (signed_addr_p)
	*high = (unrelocated_addr) extract_signed_integer (loc_ptr, addr_size,
							   byte_order);
      else
	*high = (unrelocated_addr) extract_unsigned_integer (loc_ptr, addr_size,
							     byte_order);

      loc_ptr += addr_size;
      *new_ptr = loc_ptr;
      return DEBUG_LOC_START_END;

    /* Following cases are not supported yet.  */
    case DW_LLE_startx_endx:
    case DW_LLE_default_location:
    default:
      return DEBUG_LOC_INVALID_ENTRY;
    }
}

/* Decode the addresses in .debug_loc.dwo entry.
   A pointer to the next byte to examine is returned in *NEW_PTR.
   The encoded low,high addresses are return in *LOW,*HIGH.
   The result indicates the kind of entry found.  */

static enum debug_loc_kind
decode_debug_loc_dwo_addresses (dwarf2_per_cu_data *per_cu,
				dwarf2_per_objfile *per_objfile,
				const gdb_byte *loc_ptr,
				const gdb_byte *buf_end,
				const gdb_byte **new_ptr,
				unrelocated_addr *low,
				unrelocated_addr *high,
				enum bfd_endian byte_order)
{
  uint64_t low_index, high_index;

  if (loc_ptr == buf_end)
    return DEBUG_LOC_BUFFER_OVERFLOW;

  switch (*loc_ptr++)
    {
    case DW_LLE_GNU_end_of_list_entry:
      *new_ptr = loc_ptr;
      return DEBUG_LOC_END_OF_LIST;

    case DW_LLE_GNU_base_address_selection_entry:
      *low = {};
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &high_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = dwarf2_read_addr_index (per_cu, per_objfile, high_index);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_BASE_ADDRESS;

    case DW_LLE_GNU_start_end_entry:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &low_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *low = dwarf2_read_addr_index (per_cu, per_objfile, low_index);
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &high_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = dwarf2_read_addr_index (per_cu, per_objfile, high_index);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_START_END;

    case DW_LLE_GNU_start_length_entry:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &low_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *low = dwarf2_read_addr_index (per_cu, per_objfile, low_index);
      if (loc_ptr + 4 > buf_end)
	return DEBUG_LOC_BUFFER_OVERFLOW;

      *high = *low;
      *high = (unrelocated_addr) ((CORE_ADDR) *high
				  + extract_unsigned_integer (loc_ptr, 4,
							      byte_order));
      *new_ptr = loc_ptr + 4;
      return DEBUG_LOC_START_LENGTH;

    default:
      return DEBUG_LOC_INVALID_ENTRY;
    }
}

/* A function for dealing with location lists.  Given a
   symbol baton (BATON) and a pc value (PC), find the appropriate
   location expression, set *LOCEXPR_LENGTH, and return a pointer
   to the beginning of the expression.  Returns NULL on failure.

   For now, only return the first matching location expression; there
   can be more than one in the list.  */

const gdb_byte *
dwarf2_find_location_expression (const dwarf2_loclist_baton *baton,
				 size_t *locexpr_length, const CORE_ADDR pc,
				 bool at_entry)
{
  dwarf2_per_objfile *per_objfile = baton->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int addr_size = baton->per_cu->addr_size ();
  int signed_addr_p = bfd_get_sign_extend_vma (objfile->obfd.get ());
  /* Adjustment for relocatable objects.  */
  CORE_ADDR text_offset = baton->per_objfile->objfile->text_section_offset ();
  unrelocated_addr unrel_pc = (unrelocated_addr) (pc - text_offset);
  unrelocated_addr base_address = baton->base_address;
  const gdb_byte *loc_ptr, *buf_end;

  loc_ptr = baton->data;
  buf_end = baton->data + baton->size;

  while (1)
    {
      unrelocated_addr low = {}, high = {}; /* init for gcc -Wall */
      int length;
      enum debug_loc_kind kind;
      const gdb_byte *new_ptr = NULL; /* init for gcc -Wall */

      if (baton->per_cu->version () < 5 && baton->from_dwo)
	kind = decode_debug_loc_dwo_addresses (baton->per_cu,
					       baton->per_objfile,
					       loc_ptr, buf_end, &new_ptr,
					       &low, &high, byte_order);
      else if (baton->per_cu->version () < 5)
	kind = decode_debug_loc_addresses (loc_ptr, buf_end, &new_ptr,
					   &low, &high,
					   byte_order, addr_size,
					   signed_addr_p);
      else
	kind = decode_debug_loclists_addresses (baton->per_cu,
						baton->per_objfile,
						loc_ptr, buf_end, &new_ptr,
						&low, &high, byte_order,
						addr_size, signed_addr_p);

      loc_ptr = new_ptr;
      switch (kind)
	{
	case DEBUG_LOC_END_OF_LIST:
	  *locexpr_length = 0;
	  return NULL;

	case DEBUG_LOC_BASE_ADDRESS:
	  base_address = high;
	  continue;

	case DEBUG_LOC_START_END:
	case DEBUG_LOC_START_LENGTH:
	case DEBUG_LOC_OFFSET_PAIR:
	  break;

	case DEBUG_LOC_BUFFER_OVERFLOW:
	case DEBUG_LOC_INVALID_ENTRY:
	  error (_("dwarf2_find_location_expression: "
		   "Corrupted DWARF expression."));

	default:
	  gdb_assert_not_reached ("bad debug_loc_kind");
	}

      /* Otherwise, a location expression entry.
	 If the entry is from a DWO, don't add base address: the entry is from
	 .debug_addr which already has the DWARF "base address". We still add
	 text offset in case we're debugging a PIE executable. However, if the
	 entry is DW_LLE_offset_pair from a DWO, add the base address as the
	 operands are offsets relative to the applicable base address.
	 If the entry is DW_LLE_start_end or DW_LLE_start_length, then
	 it already is an address, and we don't need to add the base.  */
      if (!baton->from_dwo && kind == DEBUG_LOC_OFFSET_PAIR)
	{
	  low = (unrelocated_addr) ((CORE_ADDR) low + (CORE_ADDR) base_address);
	  high = (unrelocated_addr) ((CORE_ADDR) high + (CORE_ADDR) base_address);
	}

      if (baton->per_cu->version () < 5)
	{
	  length = extract_unsigned_integer (loc_ptr, 2, byte_order);
	  loc_ptr += 2;
	}
      else
	{
	  unsigned int bytes_read;

	  length = read_unsigned_leb128 (NULL, loc_ptr, &bytes_read);
	  loc_ptr += bytes_read;
	}

      if (low == high && unrel_pc == low && at_entry)
	{
	  /* This is entry PC record present only at entry point
	     of a function.  Verify it is really the function entry point.  */

	  const struct block *pc_block = block_for_pc (pc);
	  struct symbol *pc_func = NULL;

	  if (pc_block)
	    pc_func = pc_block->linkage_function ();

	  if (pc_func && pc == pc_func->value_block ()->entry_pc ())
	    {
	      *locexpr_length = length;
	      return loc_ptr;
	    }
	}

      if (unrel_pc >= low && unrel_pc < high)
	{
	  *locexpr_length = length;
	  return loc_ptr;
	}

      loc_ptr += length;
    }
}

/* Implement find_frame_base_location method for LOC_BLOCK functions using
   DWARF expression for its DW_AT_frame_base.  */

static void
locexpr_find_frame_base_location (struct symbol *framefunc, CORE_ADDR pc,
				  const gdb_byte **start, size_t *length)
{
  struct dwarf2_locexpr_baton *symbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (framefunc);

  *length = symbaton->size;
  *start = symbaton->data;
}

/* Implement the struct symbol_block_ops::get_frame_base method for
   LOC_BLOCK functions using a DWARF expression as its DW_AT_frame_base.  */

static CORE_ADDR
locexpr_get_frame_base (struct symbol *framefunc, frame_info_ptr frame)
{
  struct gdbarch *gdbarch;
  struct type *type;
  struct dwarf2_locexpr_baton *dlbaton;
  const gdb_byte *start;
  size_t length;
  struct value *result;

  /* If this method is called, then FRAMEFUNC is supposed to be a DWARF block.
     Thus, it's supposed to provide the find_frame_base_location method as
     well.  */
  gdb_assert (SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location != NULL);

  gdbarch = get_frame_arch (frame);
  type = builtin_type (gdbarch)->builtin_data_ptr;
  dlbaton = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (framefunc);

  SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location
    (framefunc, get_frame_pc (frame), &start, &length);
  result = dwarf2_evaluate_loc_desc (type, frame, start, length,
				     dlbaton->per_cu, dlbaton->per_objfile);

  /* The DW_AT_frame_base attribute contains a location description which
     computes the base address itself.  However, the call to
     dwarf2_evaluate_loc_desc returns a value representing a variable at
     that address.  The frame base address is thus this variable's
     address.  */
  return result->address ();
}

/* Vector for inferior functions as represented by LOC_BLOCK, if the inferior
   function uses DWARF expression for its DW_AT_frame_base.  */

const struct symbol_block_ops dwarf2_block_frame_base_locexpr_funcs =
{
  locexpr_find_frame_base_location,
  locexpr_get_frame_base
};

/* Implement find_frame_base_location method for LOC_BLOCK functions using
   DWARF location list for its DW_AT_frame_base.  */

static void
loclist_find_frame_base_location (struct symbol *framefunc, CORE_ADDR pc,
				  const gdb_byte **start, size_t *length)
{
  struct dwarf2_loclist_baton *symbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (framefunc);

  *start = dwarf2_find_location_expression (symbaton, length, pc);
}

/* Implement the struct symbol_block_ops::get_frame_base method for
   LOC_BLOCK functions using a DWARF location list as its DW_AT_frame_base.  */

static CORE_ADDR
loclist_get_frame_base (struct symbol *framefunc, frame_info_ptr frame)
{
  struct gdbarch *gdbarch;
  struct type *type;
  struct dwarf2_loclist_baton *dlbaton;
  const gdb_byte *start;
  size_t length;
  struct value *result;

  /* If this method is called, then FRAMEFUNC is supposed to be a DWARF block.
     Thus, it's supposed to provide the find_frame_base_location method as
     well.  */
  gdb_assert (SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location != NULL);

  gdbarch = get_frame_arch (frame);
  type = builtin_type (gdbarch)->builtin_data_ptr;
  dlbaton = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (framefunc);

  SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location
    (framefunc, get_frame_pc (frame), &start, &length);
  result = dwarf2_evaluate_loc_desc (type, frame, start, length,
				     dlbaton->per_cu, dlbaton->per_objfile);

  /* The DW_AT_frame_base attribute contains a location description which
     computes the base address itself.  However, the call to
     dwarf2_evaluate_loc_desc returns a value representing a variable at
     that address.  The frame base address is thus this variable's
     address.  */
  return result->address ();
}

/* Vector for inferior functions as represented by LOC_BLOCK, if the inferior
   function uses DWARF location list for its DW_AT_frame_base.  */

const struct symbol_block_ops dwarf2_block_frame_base_loclist_funcs =
{
  loclist_find_frame_base_location,
  loclist_get_frame_base
};

/* See dwarf2/loc.h.  */

void
func_get_frame_base_dwarf_block (struct symbol *framefunc, CORE_ADDR pc,
				 const gdb_byte **start, size_t *length)
{
  if (SYMBOL_BLOCK_OPS (framefunc) != NULL)
    {
      const struct symbol_block_ops *ops_block = SYMBOL_BLOCK_OPS (framefunc);

      ops_block->find_frame_base_location (framefunc, pc, start, length);
    }
  else
    *length = 0;

  if (*length == 0)
    error (_("Could not find the frame base for \"%s\"."),
	   framefunc->natural_name ());
}

/* See loc.h.  */

value *
compute_var_value (const char *name)
{
  struct block_symbol sym = lookup_symbol (name, nullptr, VAR_DOMAIN,
					   nullptr);
  if (sym.symbol != nullptr)
    return value_of_variable (sym.symbol, sym.block);
  return nullptr;
}

/* See dwarf2/loc.h.  */

unsigned int entry_values_debug = 0;

/* Helper to set entry_values_debug.  */

static void
show_entry_values_debug (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Entry values and tail call frames debugging is %s.\n"),
	      value);
}

/* See gdbtypes.h.  */

void
call_site_target::iterate_over_addresses
     (struct gdbarch *call_site_gdbarch,
      const struct call_site *call_site,
      frame_info_ptr caller_frame,
      iterate_ftype callback) const
{
  switch (m_loc_kind)
    {
    case call_site_target::DWARF_BLOCK:
      {
	struct dwarf2_locexpr_baton *dwarf_block;
	struct value *val;
	struct type *caller_core_addr_type;
	struct gdbarch *caller_arch;

	dwarf_block = m_loc.dwarf_block;
	if (dwarf_block == NULL)
	  {
	    struct bound_minimal_symbol msym;
	    
	    msym = lookup_minimal_symbol_by_pc (call_site->pc () - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("DW_AT_call_target is not specified at %s in %s"),
			 paddress (call_site_gdbarch, call_site->pc ()),
			 (msym.minsym == NULL ? "???"
			  : msym.minsym->print_name ()));
			
	  }
	if (caller_frame == NULL)
	  {
	    struct bound_minimal_symbol msym;
	    
	    msym = lookup_minimal_symbol_by_pc (call_site->pc () - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("DW_AT_call_target DWARF block resolving "
			   "requires known frame which is currently not "
			   "available at %s in %s"),
			 paddress (call_site_gdbarch, call_site->pc ()),
			 (msym.minsym == NULL ? "???"
			  : msym.minsym->print_name ()));
			
	  }
	caller_arch = get_frame_arch (caller_frame);
	caller_core_addr_type = builtin_type (caller_arch)->builtin_func_ptr;
	val = dwarf2_evaluate_loc_desc (caller_core_addr_type, caller_frame,
					dwarf_block->data, dwarf_block->size,
					dwarf_block->per_cu,
					dwarf_block->per_objfile);
	/* DW_AT_call_target is a DWARF expression, not a DWARF location.  */
	if (val->lval () == lval_memory)
	  callback (val->address ());
	else
	  callback (value_as_address (val));
      }
      break;

    case call_site_target::PHYSNAME:
      {
	const char *physname;
	struct bound_minimal_symbol msym;

	physname = m_loc.physname;

	/* Handle both the mangled and demangled PHYSNAME.  */
	msym = lookup_minimal_symbol (physname, NULL, NULL);
	if (msym.minsym == NULL)
	  {
	    msym = lookup_minimal_symbol_by_pc (call_site->pc () - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("Cannot find function \"%s\" for a call site target "
			   "at %s in %s"),
			 physname, paddress (call_site_gdbarch, call_site->pc ()),
			 (msym.minsym == NULL ? "???"
			  : msym.minsym->print_name ()));
			
	  }

	CORE_ADDR addr = (gdbarch_convert_from_func_ptr_addr
			  (call_site_gdbarch, msym.value_address (),
			   current_inferior ()->top_target ()));
	callback (addr);
      }
      break;

    case call_site_target::PHYSADDR:
      {
	dwarf2_per_objfile *per_objfile = call_site->per_objfile;

	callback (per_objfile->relocate (m_loc.physaddr));
      }
      break;

    case call_site_target::ADDRESSES:
      {
	dwarf2_per_objfile *per_objfile = call_site->per_objfile;

	for (unsigned i = 0; i < m_loc.addresses.length; ++i)
	  callback (per_objfile->relocate (m_loc.addresses.values[i]));
      }
      break;

    default:
      internal_error (_("invalid call site target kind"));
    }
}

/* Convert function entry point exact address ADDR to the function which is
   compliant with TAIL_CALL_LIST_COMPLETE condition.  Throw
   NO_ENTRY_VALUE_ERROR otherwise.  */

static struct symbol *
func_addr_to_tail_call_list (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  struct symbol *sym = find_pc_function (addr);
  struct type *type;

  if (sym == NULL || sym->value_block ()->entry_pc () != addr)
    throw_error (NO_ENTRY_VALUE_ERROR,
		 _("DW_TAG_call_site resolving failed to find function "
		   "name for address %s"),
		 paddress (gdbarch, addr));

  type = sym->type ();
  gdb_assert (type->code () == TYPE_CODE_FUNC);
  gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FUNC);

  return sym;
}

/* Verify function with entry point exact address ADDR can never call itself
   via its tail calls (incl. transitively).  Throw NO_ENTRY_VALUE_ERROR if it
   can call itself via tail calls.

   If a funtion can tail call itself its entry value based parameters are
   unreliable.  There is no verification whether the value of some/all
   parameters is unchanged through the self tail call, we expect if there is
   a self tail call all the parameters can be modified.  */

static void
func_verify_no_selftailcall (struct gdbarch *gdbarch, CORE_ADDR verify_addr)
{
  CORE_ADDR addr;

  /* The verification is completely unordered.  Track here function addresses
     which still need to be iterated.  */
  std::vector<CORE_ADDR> todo;

  /* Track here CORE_ADDRs which were already visited.  */
  std::unordered_set<CORE_ADDR> addr_hash;

  todo.push_back (verify_addr);
  while (!todo.empty ())
    {
      struct symbol *func_sym;
      struct call_site *call_site;

      addr = todo.back ();
      todo.pop_back ();

      func_sym = func_addr_to_tail_call_list (gdbarch, addr);

      for (call_site = TYPE_TAIL_CALL_LIST (func_sym->type ());
	   call_site; call_site = call_site->tail_call_next)
	{
	  /* CALLER_FRAME with registers is not available for tail-call jumped
	     frames.  */
	  call_site->iterate_over_addresses (gdbarch, nullptr,
					     [&] (CORE_ADDR target_addr)
	    {
	      if (target_addr == verify_addr)
		{
		  struct bound_minimal_symbol msym;

		  msym = lookup_minimal_symbol_by_pc (verify_addr);
		  throw_error (NO_ENTRY_VALUE_ERROR,
			       _("DW_OP_entry_value resolving has found "
				 "function \"%s\" at %s can call itself via tail "
				 "calls"),
			       (msym.minsym == NULL ? "???"
				: msym.minsym->print_name ()),
			       paddress (gdbarch, verify_addr));
		}

	      if (addr_hash.insert (target_addr).second)
		todo.push_back (target_addr);
	    });
	}
    }
}

/* Print user readable form of CALL_SITE->PC to gdb_stdlog.  Used only for
   ENTRY_VALUES_DEBUG.  */

static void
tailcall_dump (struct gdbarch *gdbarch, const struct call_site *call_site)
{
  CORE_ADDR addr = call_site->pc ();
  struct bound_minimal_symbol msym = lookup_minimal_symbol_by_pc (addr - 1);

  gdb_printf (gdb_stdlog, " %s(%s)", paddress (gdbarch, addr),
	      (msym.minsym == NULL ? "???"
	       : msym.minsym->print_name ()));

}

/* Intersect RESULTP with CHAIN to keep RESULTP unambiguous, keep in RESULTP
   only top callers and bottom callees which are present in both.  GDBARCH is
   used only for ENTRY_VALUES_DEBUG.  RESULTP is NULL after return if there are
   no remaining possibilities to provide unambiguous non-trivial result.
   RESULTP should point to NULL on the first (initialization) call.  Caller is
   responsible for xfree of any RESULTP data.  */

static void
chain_candidate (struct gdbarch *gdbarch,
		 gdb::unique_xmalloc_ptr<struct call_site_chain> *resultp,
		 const std::vector<struct call_site *> &chain)
{
  long length = chain.size ();
  int callers, callees, idx;

  if (*resultp == NULL)
    {
      /* Create the initial chain containing all the passed PCs.  */

      struct call_site_chain *result
	= ((struct call_site_chain *)
	   xmalloc (sizeof (*result)
		    + sizeof (*result->call_site) * (length - 1)));
      result->length = length;
      result->callers = result->callees = length;
      if (!chain.empty ())
	memcpy (result->call_site, chain.data (),
		sizeof (*result->call_site) * length);
      resultp->reset (result);

      if (entry_values_debug)
	{
	  gdb_printf (gdb_stdlog, "tailcall: initial:");
	  for (idx = 0; idx < length; idx++)
	    tailcall_dump (gdbarch, result->call_site[idx]);
	  gdb_putc ('\n', gdb_stdlog);
	}

      return;
    }

  if (entry_values_debug)
    {
      gdb_printf (gdb_stdlog, "tailcall: compare:");
      for (idx = 0; idx < length; idx++)
	tailcall_dump (gdbarch, chain[idx]);
      gdb_putc ('\n', gdb_stdlog);
    }

  /* Intersect callers.  */

  callers = std::min ((long) (*resultp)->callers, length);
  for (idx = 0; idx < callers; idx++)
    if ((*resultp)->call_site[idx] != chain[idx])
      {
	(*resultp)->callers = idx;
	break;
      }

  /* Intersect callees.  */

  callees = std::min ((long) (*resultp)->callees, length);
  for (idx = 0; idx < callees; idx++)
    if ((*resultp)->call_site[(*resultp)->length - 1 - idx]
	!= chain[length - 1 - idx])
      {
	(*resultp)->callees = idx;
	break;
      }

  if (entry_values_debug)
    {
      gdb_printf (gdb_stdlog, "tailcall: reduced:");
      for (idx = 0; idx < (*resultp)->callers; idx++)
	tailcall_dump (gdbarch, (*resultp)->call_site[idx]);
      gdb_puts (" |", gdb_stdlog);
      for (idx = 0; idx < (*resultp)->callees; idx++)
	tailcall_dump (gdbarch,
		       (*resultp)->call_site[(*resultp)->length
					     - (*resultp)->callees + idx]);
      gdb_putc ('\n', gdb_stdlog);
    }

  if ((*resultp)->callers == 0 && (*resultp)->callees == 0)
    {
      /* There are no common callers or callees.  It could be also a direct
	 call (which has length 0) with ambiguous possibility of an indirect
	 call - CALLERS == CALLEES == 0 is valid during the first allocation
	 but any subsequence processing of such entry means ambiguity.  */
      resultp->reset (NULL);
      return;
    }

  /* See call_site_find_chain_1 why there is no way to reach the bottom callee
     PC again.  In such case there must be two different code paths to reach
     it.  CALLERS + CALLEES equal to LENGTH in the case of self tail-call.  */
  gdb_assert ((*resultp)->callers + (*resultp)->callees <= (*resultp)->length);
}

/* Recursively try to construct the call chain.  GDBARCH, RESULTP, and
   CHAIN are passed to chain_candidate.  ADDR_HASH tracks which
   addresses have already been seen along the current chain.
   CALL_SITE is the call site to visit, and CALLEE_PC is the PC we're
   trying to "reach".  Returns false if an error has already been
   detected and so an early return can be done.  If it makes sense to
   keep trying (even if no answer has yet been found), returns
   true.  */

static bool
call_site_find_chain_2
     (struct gdbarch *gdbarch,
      gdb::unique_xmalloc_ptr<struct call_site_chain> *resultp,
      std::vector<struct call_site *> &chain,
      std::unordered_set<CORE_ADDR> &addr_hash,
      struct call_site *call_site,
      CORE_ADDR callee_pc)
{
  std::vector<CORE_ADDR> addresses;
  bool found_exact = false;
  call_site->iterate_over_addresses (gdbarch, nullptr,
				     [&] (CORE_ADDR addr)
    {
      if (addr == callee_pc)
	found_exact = true;
      else
	addresses.push_back (addr);
    });

  if (found_exact)
    {
      chain_candidate (gdbarch, resultp, chain);
      /* If RESULTP was reset, then chain_candidate failed, and so we
	 can tell our callers to early-return.  */
      return *resultp != nullptr;
    }

  for (CORE_ADDR target_func_addr : addresses)
    {
      struct symbol *target_func
	= func_addr_to_tail_call_list (gdbarch, target_func_addr);
      for (struct call_site *target_call_site
	     = TYPE_TAIL_CALL_LIST (target_func->type ());
	   target_call_site != nullptr;
	   target_call_site = target_call_site->tail_call_next)
	{
	  if (addr_hash.insert (target_call_site->pc ()).second)
	    {
	      /* Successfully entered TARGET_CALL_SITE.  */
	      chain.push_back (target_call_site);

	      if (!call_site_find_chain_2 (gdbarch, resultp, chain,
					   addr_hash, target_call_site,
					   callee_pc))
		return false;

	      size_t removed = addr_hash.erase (target_call_site->pc ());
	      gdb_assert (removed == 1);
	      chain.pop_back ();
	    }
	}
    }

  return true;
}

/* Create and return call_site_chain for CALLER_PC and CALLEE_PC.  All
   the assumed frames between them use GDBARCH.  Any unreliability
   results in thrown NO_ENTRY_VALUE_ERROR.  */

static gdb::unique_xmalloc_ptr<call_site_chain>
call_site_find_chain_1 (struct gdbarch *gdbarch, CORE_ADDR caller_pc,
			CORE_ADDR callee_pc)
{
  CORE_ADDR save_callee_pc = callee_pc;
  gdb::unique_xmalloc_ptr<struct call_site_chain> retval;
  struct call_site *call_site;

  /* CHAIN contains only the intermediate CALL_SITEs.  Neither CALLER_PC's
     call_site nor any possible call_site at CALLEE_PC's function is there.
     Any CALL_SITE in CHAIN will be iterated to its siblings - via
     TAIL_CALL_NEXT.  This is inappropriate for CALLER_PC's call_site.  */
  std::vector<struct call_site *> chain;

  /* A given call site may have multiple associated addresses.  This
     can happen if, e.g., the caller is split by hot/cold
     partitioning.  This vector tracks the ones we haven't visited
     yet.  */
  std::vector<std::vector<CORE_ADDR>> unvisited_addresses;

  /* We are not interested in the specific PC inside the callee function.  */
  callee_pc = get_pc_function_start (callee_pc);
  if (callee_pc == 0)
    throw_error (NO_ENTRY_VALUE_ERROR, _("Unable to find function for PC %s"),
		 paddress (gdbarch, save_callee_pc));

  /* Mark CALL_SITEs so we do not visit the same ones twice.  */
  std::unordered_set<CORE_ADDR> addr_hash;

  /* Do not push CALL_SITE to CHAIN.  Push there only the first tail call site
     at the target's function.  All the possible tail call sites in the
     target's function will get iterated as already pushed into CHAIN via their
     TAIL_CALL_NEXT.  */
  call_site = call_site_for_pc (gdbarch, caller_pc);
  /* No need to check the return value here, because we no longer care
     about possible early returns.  */
  call_site_find_chain_2 (gdbarch, &retval, chain, addr_hash, call_site,
			  callee_pc);

  if (retval == NULL)
    {
      struct bound_minimal_symbol msym_caller, msym_callee;
      
      msym_caller = lookup_minimal_symbol_by_pc (caller_pc);
      msym_callee = lookup_minimal_symbol_by_pc (callee_pc);
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("There are no unambiguously determinable intermediate "
		     "callers or callees between caller function \"%s\" at %s "
		     "and callee function \"%s\" at %s"),
		   (msym_caller.minsym == NULL
		    ? "???" : msym_caller.minsym->print_name ()),
		   paddress (gdbarch, caller_pc),
		   (msym_callee.minsym == NULL
		    ? "???" : msym_callee.minsym->print_name ()),
		   paddress (gdbarch, callee_pc));
    }

  return retval;
}

/* Create and return call_site_chain for CALLER_PC and CALLEE_PC.  All the
   assumed frames between them use GDBARCH.  If valid call_site_chain cannot be
   constructed return NULL.  */

gdb::unique_xmalloc_ptr<call_site_chain>
call_site_find_chain (struct gdbarch *gdbarch, CORE_ADDR caller_pc,
		      CORE_ADDR callee_pc)
{
  gdb::unique_xmalloc_ptr<call_site_chain> retval;

  try
    {
      retval = call_site_find_chain_1 (gdbarch, caller_pc, callee_pc);
    }
  catch (const gdb_exception_error &e)
    {
      if (e.error == NO_ENTRY_VALUE_ERROR)
	{
	  if (entry_values_debug)
	    exception_print (gdb_stdout, e);

	  return NULL;
	}
      else
	throw;
    }

  return retval;
}

/* Return 1 if KIND and KIND_U match PARAMETER.  Return 0 otherwise.  */

static int
call_site_parameter_matches (struct call_site_parameter *parameter,
			     enum call_site_parameter_kind kind,
			     union call_site_parameter_u kind_u)
{
  if (kind == parameter->kind)
    switch (kind)
      {
      case CALL_SITE_PARAMETER_DWARF_REG:
	return kind_u.dwarf_reg == parameter->u.dwarf_reg;

      case CALL_SITE_PARAMETER_FB_OFFSET:
	return kind_u.fb_offset == parameter->u.fb_offset;

      case CALL_SITE_PARAMETER_PARAM_OFFSET:
	return kind_u.param_cu_off == parameter->u.param_cu_off;
      }
  return 0;
}

/* See loc.h.  */

struct call_site_parameter *
dwarf_expr_reg_to_entry_parameter (frame_info_ptr frame,
				   enum call_site_parameter_kind kind,
				   union call_site_parameter_u kind_u,
				   dwarf2_per_cu_data **per_cu_return,
				   dwarf2_per_objfile **per_objfile_return)
{
  CORE_ADDR func_addr, caller_pc;
  struct gdbarch *gdbarch;
  frame_info_ptr caller_frame;
  struct call_site *call_site;
  int iparams;
  /* Initialize it just to avoid a GCC false warning.  */
  struct call_site_parameter *parameter = NULL;
  CORE_ADDR target_addr;

  while (get_frame_type (frame) == INLINE_FRAME)
    {
      frame = get_prev_frame (frame);
      gdb_assert (frame != NULL);
    }

  func_addr = get_frame_func (frame);
  gdbarch = get_frame_arch (frame);
  caller_frame = get_prev_frame (frame);
  if (gdbarch != frame_unwind_arch (frame))
    {
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol_by_pc (func_addr);
      struct gdbarch *caller_gdbarch = frame_unwind_arch (frame);

      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("DW_OP_entry_value resolving callee gdbarch %s "
		     "(of %s (%s)) does not match caller gdbarch %s"),
		   gdbarch_bfd_arch_info (gdbarch)->printable_name,
		   paddress (gdbarch, func_addr),
		   (msym.minsym == NULL ? "???"
		    : msym.minsym->print_name ()),
		   gdbarch_bfd_arch_info (caller_gdbarch)->printable_name);
    }

  if (caller_frame == NULL)
    {
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol_by_pc (func_addr);

      throw_error (NO_ENTRY_VALUE_ERROR, _("DW_OP_entry_value resolving "
					   "requires caller of %s (%s)"),
		   paddress (gdbarch, func_addr),
		   (msym.minsym == NULL ? "???"
		    : msym.minsym->print_name ()));
    }
  caller_pc = get_frame_pc (caller_frame);
  call_site = call_site_for_pc (gdbarch, caller_pc);

  bool found = false;
  unsigned count = 0;
  call_site->iterate_over_addresses (gdbarch, caller_frame,
				     [&] (CORE_ADDR addr)
    {
      /* Preserve any address.  */
      target_addr = addr;
      ++count;
      if (addr == func_addr)
	found = true;
    });
  if (!found)
    {
      struct minimal_symbol *target_msym, *func_msym;

      target_msym = lookup_minimal_symbol_by_pc (target_addr).minsym;
      func_msym = lookup_minimal_symbol_by_pc (func_addr).minsym;
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("DW_OP_entry_value resolving expects callee %s at %s %s"
		     "but the called frame is for %s at %s"),
		   (target_msym == NULL ? "???"
					: target_msym->print_name ()),
		   paddress (gdbarch, target_addr),
		   (count > 0
		    ? _("(but note there are multiple addresses not listed)")
		    : ""),
		   func_msym == NULL ? "???" : func_msym->print_name (),
		   paddress (gdbarch, func_addr));
    }

  /* No entry value based parameters would be reliable if this function can
     call itself via tail calls.  */
  func_verify_no_selftailcall (gdbarch, func_addr);

  for (iparams = 0; iparams < call_site->parameter_count; iparams++)
    {
      parameter = &call_site->parameter[iparams];
      if (call_site_parameter_matches (parameter, kind, kind_u))
	break;
    }
  if (iparams == call_site->parameter_count)
    {
      struct minimal_symbol *msym
	= lookup_minimal_symbol_by_pc (caller_pc).minsym;

      /* DW_TAG_call_site_parameter will be missing just if GCC could not
	 determine its value.  */
      throw_error (NO_ENTRY_VALUE_ERROR, _("Cannot find matching parameter "
					   "at DW_TAG_call_site %s at %s"),
		   paddress (gdbarch, caller_pc),
		   msym == NULL ? "???" : msym->print_name ()); 
    }

  *per_cu_return = call_site->per_cu;
  *per_objfile_return = call_site->per_objfile;
  return parameter;
}

/* Return value for PARAMETER matching DEREF_SIZE.  If DEREF_SIZE is -1, return
   the normal DW_AT_call_value block.  Otherwise return the
   DW_AT_call_data_value (dereferenced) block.

   TYPE and CALLER_FRAME specify how to evaluate the DWARF block into returned
   struct value.

   Function always returns non-NULL, non-optimized out value.  It throws
   NO_ENTRY_VALUE_ERROR if it cannot resolve the value for any reason.  */

static struct value *
dwarf_entry_parameter_to_value (struct call_site_parameter *parameter,
				CORE_ADDR deref_size, struct type *type,
				frame_info_ptr caller_frame,
				dwarf2_per_cu_data *per_cu,
				dwarf2_per_objfile *per_objfile)
{
  const gdb_byte *data_src;
  size_t size;

  data_src = deref_size == -1 ? parameter->value : parameter->data_value;
  size = deref_size == -1 ? parameter->value_size : parameter->data_value_size;

  /* DEREF_SIZE size is not verified here.  */
  if (data_src == NULL)
    throw_error (NO_ENTRY_VALUE_ERROR,
		 _("Cannot resolve DW_AT_call_data_value"));

  return dwarf2_evaluate_loc_desc (type, caller_frame, data_src, size, per_cu,
				   per_objfile, false);
}

/* VALUE must be of type lval_computed with entry_data_value_funcs.  Perform
   the indirect method on it, that is use its stored target value, the sole
   purpose of entry_data_value_funcs..  */

static struct value *
entry_data_value_coerce_ref (const struct value *value)
{
  struct type *checked_type = check_typedef (value->type ());
  struct value *target_val;

  if (!TYPE_IS_REFERENCE (checked_type))
    return NULL;

  target_val = (struct value *) value->computed_closure ();
  target_val->incref ();
  return target_val;
}

/* Implement copy_closure.  */

static void *
entry_data_value_copy_closure (const struct value *v)
{
  struct value *target_val = (struct value *) v->computed_closure ();

  target_val->incref ();
  return target_val;
}

/* Implement free_closure.  */

static void
entry_data_value_free_closure (struct value *v)
{
  struct value *target_val = (struct value *) v->computed_closure ();

  target_val->decref ();
}

/* Vector for methods for an entry value reference where the referenced value
   is stored in the caller.  On the first dereference use
   DW_AT_call_data_value in the caller.  */

static const struct lval_funcs entry_data_value_funcs =
{
  NULL,	/* read */
  NULL,	/* write */
  nullptr,
  NULL,	/* indirect */
  entry_data_value_coerce_ref,
  NULL,	/* check_synthetic_pointer */
  entry_data_value_copy_closure,
  entry_data_value_free_closure
};

/* See dwarf2/loc.h.  */
struct value *
value_of_dwarf_reg_entry (struct type *type, frame_info_ptr frame,
			  enum call_site_parameter_kind kind,
			  union call_site_parameter_u kind_u)
{
  struct type *checked_type = check_typedef (type);
  struct type *target_type = checked_type->target_type ();
  frame_info_ptr caller_frame = get_prev_frame (frame);
  struct value *outer_val, *target_val, *val;
  struct call_site_parameter *parameter;
  dwarf2_per_cu_data *caller_per_cu;
  dwarf2_per_objfile *caller_per_objfile;

  parameter = dwarf_expr_reg_to_entry_parameter (frame, kind, kind_u,
						 &caller_per_cu,
						 &caller_per_objfile);

  outer_val = dwarf_entry_parameter_to_value (parameter, -1 /* deref_size */,
					      type, caller_frame,
					      caller_per_cu,
					      caller_per_objfile);

  /* Check if DW_AT_call_data_value cannot be used.  If it should be
     used and it is not available do not fall back to OUTER_VAL - dereferencing
     TYPE_CODE_REF with non-entry data value would give current value - not the
     entry value.  */

  if (!TYPE_IS_REFERENCE (checked_type)
      || checked_type->target_type () == NULL)
    return outer_val;

  target_val = dwarf_entry_parameter_to_value (parameter,
					       target_type->length (),
					       target_type, caller_frame,
					       caller_per_cu,
					       caller_per_objfile);

  val = value::allocate_computed (type, &entry_data_value_funcs,
				 release_value (target_val).release ());

  /* Copy the referencing pointer to the new computed value.  */
  memcpy (val->contents_raw ().data (),
	  outer_val->contents_raw ().data (),
	  checked_type->length ());
  val->set_lazy (false);

  return val;
}

/* Read parameter of TYPE at (callee) FRAME's function entry.  DATA and
   SIZE are DWARF block used to match DW_AT_location at the caller's
   DW_TAG_call_site_parameter.

   Function always returns non-NULL value.  It throws NO_ENTRY_VALUE_ERROR if it
   cannot resolve the parameter for any reason.  */

static struct value *
value_of_dwarf_block_entry (struct type *type, frame_info_ptr frame,
			    const gdb_byte *block, size_t block_len)
{
  union call_site_parameter_u kind_u;

  kind_u.dwarf_reg = dwarf_block_to_dwarf_reg (block, block + block_len);
  if (kind_u.dwarf_reg != -1)
    return value_of_dwarf_reg_entry (type, frame, CALL_SITE_PARAMETER_DWARF_REG,
				     kind_u);

  if (dwarf_block_to_fb_offset (block, block + block_len, &kind_u.fb_offset))
    return value_of_dwarf_reg_entry (type, frame, CALL_SITE_PARAMETER_FB_OFFSET,
				     kind_u);

  /* This can normally happen - throw NO_ENTRY_VALUE_ERROR to get the message
     suppressed during normal operation.  The expression can be arbitrary if
     there is no caller-callee entry value binding expected.  */
  throw_error (NO_ENTRY_VALUE_ERROR,
	       _("DWARF-2 expression error: DW_OP_entry_value is supported "
		 "only for single DW_OP_reg* or for DW_OP_fbreg(*)"));
}

/* Fetch a DW_AT_const_value through a synthetic pointer.  */

static struct value *
fetch_const_value_from_synthetic_pointer (sect_offset die, LONGEST byte_offset,
					  dwarf2_per_cu_data *per_cu,
					  dwarf2_per_objfile *per_objfile,
					  struct type *type)
{
  struct value *result = NULL;
  const gdb_byte *bytes;
  LONGEST len;

  auto_obstack temp_obstack;
  bytes = dwarf2_fetch_constant_bytes (die, per_cu, per_objfile,
				       &temp_obstack, &len);

  if (bytes != NULL)
    {
      if (byte_offset >= 0
	  && byte_offset + type->target_type ()->length () <= len)
	{
	  bytes += byte_offset;
	  result = value_from_contents (type->target_type (), bytes);
	}
      else
	invalid_synthetic_pointer ();
    }
  else
    result = value::allocate_optimized_out (type->target_type ());

  return result;
}

/* See loc.h.  */

struct value *
indirect_synthetic_pointer (sect_offset die, LONGEST byte_offset,
			    dwarf2_per_cu_data *per_cu,
			    dwarf2_per_objfile *per_objfile,
			    frame_info_ptr frame, struct type *type,
			    bool resolve_abstract_p)
{
  /* Fetch the location expression of the DIE we're pointing to.  */
  auto get_frame_address_in_block_wrapper = [frame] ()
    {
     return get_frame_address_in_block (frame);
    };
  struct dwarf2_locexpr_baton baton
    = dwarf2_fetch_die_loc_sect_off (die, per_cu, per_objfile,
				     get_frame_address_in_block_wrapper,
				     resolve_abstract_p);

  /* Get type of pointed-to DIE.  */
  struct type *orig_type = dwarf2_fetch_die_type_sect_off (die, per_cu,
							   per_objfile);
  if (orig_type == NULL)
    invalid_synthetic_pointer ();

  /* If pointed-to DIE has a DW_AT_location, evaluate it and return the
     resulting value.  Otherwise, it may have a DW_AT_const_value instead,
     or it may've been optimized out.  */
  if (baton.data != NULL)
    return dwarf2_evaluate_loc_desc_full (orig_type, frame, baton.data,
					  baton.size, baton.per_cu,
					  baton.per_objfile,
					  type->target_type (),
					  byte_offset);
  else
    return fetch_const_value_from_synthetic_pointer (die, byte_offset, per_cu,
						     per_objfile, type);
}

/* Evaluate a location description, starting at DATA and with length
   SIZE, to find the current location of variable of TYPE in the
   context of FRAME.  If SUBOBJ_TYPE is non-NULL, return instead the
   location of the subobject of type SUBOBJ_TYPE at byte offset
   SUBOBJ_BYTE_OFFSET within the variable of type TYPE.  */

static struct value *
dwarf2_evaluate_loc_desc_full (struct type *type, frame_info_ptr frame,
			       const gdb_byte *data, size_t size,
			       dwarf2_per_cu_data *per_cu,
			       dwarf2_per_objfile *per_objfile,
			       struct type *subobj_type,
			       LONGEST subobj_byte_offset,
			       bool as_lval)
{
  if (subobj_type == NULL)
    {
      subobj_type = type;
      subobj_byte_offset = 0;
    }
  else if (subobj_byte_offset < 0)
    invalid_synthetic_pointer ();

  if (size == 0)
    return value::allocate_optimized_out (subobj_type);

  dwarf_expr_context ctx (per_objfile, per_cu->addr_size ());

  value *retval;
  scoped_value_mark free_values;

  try
    {
      retval = ctx.evaluate (data, size, as_lval, per_cu, frame, nullptr,
			     type, subobj_type, subobj_byte_offset);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  free_values.free_to_mark ();
	  retval = value::allocate (subobj_type);
	  retval->mark_bytes_unavailable (0,
					  subobj_type->length ());
	  return retval;
	}
      else if (ex.error == NO_ENTRY_VALUE_ERROR)
	{
	  if (entry_values_debug)
	    exception_print (gdb_stdout, ex);
	  free_values.free_to_mark ();
	  return value::allocate_optimized_out (subobj_type);
	}
      else
	throw;
    }

  /* We need to clean up all the values that are not needed any more.
     The problem with a value_ref_ptr class is that it disconnects the
     RETVAL from the value garbage collection, so we need to make
     a copy of that value on the stack to keep everything consistent.
     The value_ref_ptr will clean up after itself at the end of this block.  */
  value_ref_ptr value_holder = value_ref_ptr::new_reference (retval);
  free_values.free_to_mark ();

  return retval->copy ();
}

/* The exported interface to dwarf2_evaluate_loc_desc_full; it always
   passes 0 as the byte_offset.  */

struct value *
dwarf2_evaluate_loc_desc (struct type *type, frame_info_ptr frame,
			  const gdb_byte *data, size_t size,
			  dwarf2_per_cu_data *per_cu,
			  dwarf2_per_objfile *per_objfile, bool as_lval)
{
  return dwarf2_evaluate_loc_desc_full (type, frame, data, size, per_cu,
					per_objfile, NULL, 0, as_lval);
}

/* Evaluates a dwarf expression and stores the result in VAL,
   expecting that the dwarf expression only produces a single
   CORE_ADDR.  FRAME is the frame in which the expression is
   evaluated.  ADDR_STACK is a context (location of a variable) and
   might be needed to evaluate the location expression.

   PUSH_VALUES is an array of values to be pushed to the expression stack
   before evaluation starts.  PUSH_VALUES[0] is pushed first, then
   PUSH_VALUES[1], and so on.

   Returns 1 on success, 0 otherwise.  */

static int
dwarf2_locexpr_baton_eval (const struct dwarf2_locexpr_baton *dlbaton,
			   frame_info_ptr frame,
			   const struct property_addr_info *addr_stack,
			   CORE_ADDR *valp,
			   gdb::array_view<CORE_ADDR> push_values,
			   bool *is_reference)
{
  if (dlbaton == NULL || dlbaton->size == 0)
    return 0;

  dwarf2_per_objfile *per_objfile = dlbaton->per_objfile;
  dwarf2_per_cu_data *per_cu = dlbaton->per_cu;
  dwarf_expr_context ctx (per_objfile, per_cu->addr_size ());

  value *result;
  scoped_value_mark free_values;

  /* Place any initial values onto the expression stack.  */
  for (const auto &val : push_values)
    ctx.push_address (val, false);

  try
    {
      result = ctx.evaluate (dlbaton->data, dlbaton->size,
			     true, per_cu, frame, addr_stack);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  return 0;
	}
      else if (ex.error == NO_ENTRY_VALUE_ERROR)
	{
	  if (entry_values_debug)
	    exception_print (gdb_stdout, ex);
	  return 0;
	}
      else
	throw;
    }

  if (result->optimized_out ())
    return 0;

  if (result->lval () == lval_memory)
    *valp = result->address ();
  else
    {
      if (result->lval () == not_lval)
	*is_reference = false;

      *valp = value_as_address (result);
    }

  return 1;
}

/* See dwarf2/loc.h.  */

bool
dwarf2_evaluate_property (const struct dynamic_prop *prop,
			  frame_info_ptr frame,
			  const struct property_addr_info *addr_stack,
			  CORE_ADDR *value,
			  gdb::array_view<CORE_ADDR> push_values)
{
  if (prop == NULL)
    return false;

  /* Evaluating a property should not change the current language.
     Without this here this could happen if the code below selects a
     frame.  */
  scoped_restore_current_language save_language;

  if (frame == NULL && has_stack_frames ())
    frame = get_selected_frame (NULL);

  switch (prop->kind ())
    {
    case PROP_LOCEXPR:
      {
	const struct dwarf2_property_baton *baton = prop->baton ();
	gdb_assert (baton->property_type != NULL);

	bool is_reference = baton->locexpr.is_reference;
	if (dwarf2_locexpr_baton_eval (&baton->locexpr, frame, addr_stack,
				       value, push_values, &is_reference))
	  {
	    if (is_reference)
	      {
		struct value *val = value_at (baton->property_type, *value);
		*value = value_as_address (val);
	      }
	    else
	      {
		gdb_assert (baton->property_type != NULL);

		struct type *type = check_typedef (baton->property_type);
		if (type->length () < sizeof (CORE_ADDR)
		    && !type->is_unsigned ())
		  {
		    /* If we have a valid return candidate and it's value
		       is signed, we have to sign-extend the value because
		       CORE_ADDR on 64bit machine has 8 bytes but address
		       size of an 32bit application is bytes.  */
		    const int addr_size
		      = (baton->locexpr.per_cu->addr_size ()
			 * TARGET_CHAR_BIT);
		    const CORE_ADDR neg_mask
		      = (~((CORE_ADDR) 0) <<  (addr_size - 1));

		    /* Check if signed bit is set and sign-extend values.  */
		    if (*value & neg_mask)
		      *value |= neg_mask;
		  }
	      }
	    return true;
	  }
      }
      break;

    case PROP_LOCLIST:
      {
	const dwarf2_property_baton *baton = prop->baton ();
	CORE_ADDR pc;
	const gdb_byte *data;
	struct value *val;
	size_t size;

	if (frame == NULL
	    || !get_frame_address_in_block_if_available (frame, &pc))
	  return false;

	data = dwarf2_find_location_expression (&baton->loclist, &size, pc);
	if (data != NULL)
	  {
	    val = dwarf2_evaluate_loc_desc (baton->property_type, frame, data,
					    size, baton->loclist.per_cu,
					    baton->loclist.per_objfile);
	    if (!val->optimized_out ())
	      {
		*value = value_as_address (val);
		return true;
	      }
	  }
      }
      break;

    case PROP_CONST:
      *value = prop->const_val ();
      return true;

    case PROP_ADDR_OFFSET:
      {
	const dwarf2_property_baton *baton = prop->baton ();
	const struct property_addr_info *pinfo;
	struct value *val;

	for (pinfo = addr_stack; pinfo != NULL; pinfo = pinfo->next)
	  {
	    /* This approach lets us avoid checking the qualifiers.  */
	    if (TYPE_MAIN_TYPE (pinfo->type)
		== TYPE_MAIN_TYPE (baton->property_type))
	      break;
	  }
	if (pinfo == NULL)
	  error (_("cannot find reference address for offset property"));
	if (pinfo->valaddr.data () != NULL)
	  val = value_from_contents
		  (baton->offset_info.type,
		   pinfo->valaddr.data () + baton->offset_info.offset);
	else
	  val = value_at (baton->offset_info.type,
			  pinfo->addr + baton->offset_info.offset);
	*value = value_as_address (val);
	return true;
      }

    case PROP_VARIABLE_NAME:
      {
	struct value *val = compute_var_value (prop->variable_name ());
	if (val != nullptr)
	  {
	    *value = value_as_long (val);
	    return true;
	  }
      }
      break;
    }

  return false;
}

/* See dwarf2/loc.h.  */

void
dwarf2_compile_property_to_c (string_file *stream,
			      const char *result_name,
			      struct gdbarch *gdbarch,
			      std::vector<bool> &registers_used,
			      const struct dynamic_prop *prop,
			      CORE_ADDR pc,
			      struct symbol *sym)
{
  const dwarf2_property_baton *baton = prop->baton ();
  const gdb_byte *data;
  size_t size;
  dwarf2_per_cu_data *per_cu;
  dwarf2_per_objfile *per_objfile;

  if (prop->kind () == PROP_LOCEXPR)
    {
      data = baton->locexpr.data;
      size = baton->locexpr.size;
      per_cu = baton->locexpr.per_cu;
      per_objfile = baton->locexpr.per_objfile;
    }
  else
    {
      gdb_assert (prop->kind () == PROP_LOCLIST);

      data = dwarf2_find_location_expression (&baton->loclist, &size, pc);
      per_cu = baton->loclist.per_cu;
      per_objfile = baton->loclist.per_objfile;
    }

  compile_dwarf_bounds_to_c (stream, result_name, prop, sym, pc,
			     gdbarch, registers_used,
			     per_cu->addr_size (),
			     data, data + size, per_cu, per_objfile);
}

/* Compute the correct symbol_needs_kind value for the location
   expression in EXPR.

   Implemented by traversing the logical control flow graph of the
   expression.  */

static enum symbol_needs_kind
dwarf2_get_symbol_read_needs (gdb::array_view<const gdb_byte> expr,
			      dwarf2_per_cu_data *per_cu,
			      dwarf2_per_objfile *per_objfile,
			      bfd_endian byte_order,
			      int addr_size,
			      int ref_addr_size,
			      int depth = 0)
{
  enum symbol_needs_kind symbol_needs = SYMBOL_NEEDS_NONE;

  /* If the expression is empty, we have nothing to do.  */
  if (expr.empty ())
    return symbol_needs;

  const gdb_byte *expr_end = expr.data () + expr.size ();

  /* List of operations to visit.  Operations in this list are not visited yet,
     so are not in VISITED_OPS (and vice-versa).  */
  std::vector<const gdb_byte *> ops_to_visit;

  /* Operations already visited.  */
  std::unordered_set<const gdb_byte *> visited_ops;

  /* Insert OP in OPS_TO_VISIT if it is within the expression's range and
     hasn't been visited yet.  */
  auto insert_in_ops_to_visit
    = [expr_end, &visited_ops, &ops_to_visit] (const gdb_byte *op_ptr)
      {
	if (op_ptr >= expr_end)
	  return;

	if (visited_ops.find (op_ptr) != visited_ops.end ())
	  return;

	ops_to_visit.push_back (op_ptr);
      };

  /* Expressions can invoke other expressions with DW_OP_call*.  Protect against
     a loop of calls.  */
  const int max_depth = 256;

  if (depth > max_depth)
    error (_("DWARF-2 expression error: Loop detected."));

  depth++;

  /* Initialize the to-visit list with the first operation.  */
  insert_in_ops_to_visit (&expr[0]);

  while (!ops_to_visit.empty ())
    {
      /* Pop one op to visit, mark it as visited.  */
      const gdb_byte *op_ptr = ops_to_visit.back ();
      ops_to_visit.pop_back ();
      gdb_assert (visited_ops.find (op_ptr) == visited_ops.end ());
      visited_ops.insert (op_ptr);

      dwarf_location_atom op = (dwarf_location_atom) *op_ptr;

      /* Most operations have a single possible following operation
	 (they are not conditional branches).  The code below updates
	 OP_PTR to point to that following operation, which is pushed
	 back to OPS_TO_VISIT, if needed, at the bottom.  Here, leave
	 OP_PTR pointing just after the operand.  */
      op_ptr++;

      /* The DWARF expression might have a bug causing an infinite
	 loop.  In that case, quitting is the only way out.  */
      QUIT;

      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	case DW_OP_stack_value:
	case DW_OP_dup:
	case DW_OP_drop:
	case DW_OP_swap:
	case DW_OP_over:
	case DW_OP_rot:
	case DW_OP_deref:
	case DW_OP_abs:
	case DW_OP_neg:
	case DW_OP_not:
	case DW_OP_and:
	case DW_OP_div:
	case DW_OP_minus:
	case DW_OP_mod:
	case DW_OP_mul:
	case DW_OP_or:
	case DW_OP_plus:
	case DW_OP_shl:
	case DW_OP_shr:
	case DW_OP_shra:
	case DW_OP_xor:
	case DW_OP_le:
	case DW_OP_ge:
	case DW_OP_eq:
	case DW_OP_lt:
	case DW_OP_gt:
	case DW_OP_ne:
	case DW_OP_GNU_push_tls_address:
	case DW_OP_nop:
	case DW_OP_GNU_uninit:
	case DW_OP_push_object_address:
	  break;

	case DW_OP_form_tls_address:
	  if (symbol_needs <= SYMBOL_NEEDS_REGISTERS)
	    symbol_needs = SYMBOL_NEEDS_REGISTERS;
	  break;

	case DW_OP_convert:
	case DW_OP_GNU_convert:
	case DW_OP_reinterpret:
	case DW_OP_GNU_reinterpret:
	case DW_OP_addrx:
	case DW_OP_GNU_addr_index:
	case DW_OP_GNU_const_index:
	case DW_OP_constu:
	case DW_OP_plus_uconst:
	case DW_OP_piece:
	  op_ptr = safe_skip_leb128 (op_ptr, expr_end);
	  break;

	case DW_OP_consts:
	  op_ptr = safe_skip_leb128 (op_ptr, expr_end);
	  break;

	case DW_OP_bit_piece:
	  op_ptr = safe_skip_leb128 (op_ptr, expr_end);
	  op_ptr = safe_skip_leb128 (op_ptr, expr_end);
	  break;

	case DW_OP_deref_type:
	case DW_OP_GNU_deref_type:
	  op_ptr++;
	  op_ptr = safe_skip_leb128 (op_ptr, expr_end);
	  break;

	case DW_OP_addr:
	  op_ptr += addr_size;
	  break;

	case DW_OP_const1u:
	case DW_OP_const1s:
	  op_ptr += 1;
	  break;

	case DW_OP_const2u:
	case DW_OP_const2s:
	  op_ptr += 2;
	  break;

	case DW_OP_const4s:
	case DW_OP_const4u:
	  op_ptr += 4;
	  break;

	case DW_OP_const8s:
	case DW_OP_const8u:
	  op_ptr += 8;
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	case DW_OP_regx:
	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	case DW_OP_bregx:
	case DW_OP_fbreg:
	case DW_OP_call_frame_cfa:
	case DW_OP_entry_value:
	case DW_OP_GNU_entry_value:
	case DW_OP_GNU_parameter_ref:
	case DW_OP_regval_type:
	case DW_OP_GNU_regval_type:
	  symbol_needs = SYMBOL_NEEDS_FRAME;
	  break;

	case DW_OP_implicit_value:
	  {
	    uint64_t uoffset;
	    op_ptr = safe_read_uleb128 (op_ptr, expr_end, &uoffset);
	    op_ptr += uoffset;
	    break;
	  }

	case DW_OP_implicit_pointer:
	case DW_OP_GNU_implicit_pointer:
	  op_ptr += ref_addr_size;
	  op_ptr = safe_skip_leb128 (op_ptr, expr_end);
	  break;

	case DW_OP_deref_size:
	case DW_OP_pick:
	  op_ptr++;
	  break;

	case DW_OP_skip:
	  {
	    int64_t offset = extract_signed_integer (op_ptr, 2, byte_order);
	    op_ptr += 2;
	    op_ptr += offset;
	    break;
	  }

	case DW_OP_bra:
	  {
	    /* This is the only operation that pushes two operations in
	       the to-visit list, so handle it all here.  */
	    LONGEST offset = extract_signed_integer (op_ptr, 2, byte_order);
	    op_ptr += 2;

	    insert_in_ops_to_visit (op_ptr + offset);
	    insert_in_ops_to_visit (op_ptr);
	    continue;
	  }

	case DW_OP_call2:
	case DW_OP_call4:
	  {
	    unsigned int len = op == DW_OP_call2 ? 2 : 4;
	    cu_offset cu_off
	      = (cu_offset) extract_unsigned_integer (op_ptr, len, byte_order);
	    op_ptr += len;

	    auto get_frame_pc = [&symbol_needs] ()
	      {
		symbol_needs = SYMBOL_NEEDS_FRAME;
		return 0;
	      };

	    struct dwarf2_locexpr_baton baton
	      = dwarf2_fetch_die_loc_cu_off (cu_off, per_cu,
					     per_objfile,
					     get_frame_pc);

	    /* If SYMBOL_NEEDS_FRAME is returned from the previous call,
	       we dont have to check the baton content.  */
	    if (symbol_needs != SYMBOL_NEEDS_FRAME)
	      {
		gdbarch *arch = baton.per_objfile->objfile->arch ();
		gdb::array_view<const gdb_byte> sub_expr (baton.data,
							  baton.size);
		symbol_needs
		  = dwarf2_get_symbol_read_needs (sub_expr,
						  baton.per_cu,
						  baton.per_objfile,
						  gdbarch_byte_order (arch),
						  baton.per_cu->addr_size (),
						  baton.per_cu->ref_addr_size (),
						  depth);
	      }
	    break;
	  }

	case DW_OP_GNU_variable_value:
	  {
	    sect_offset sect_off
	      = (sect_offset) extract_unsigned_integer (op_ptr,
							ref_addr_size,
							byte_order);
	    op_ptr += ref_addr_size;

	    struct type *die_type
	      = dwarf2_fetch_die_type_sect_off (sect_off, per_cu,
						per_objfile);

	    if (die_type == NULL)
	      error (_("Bad DW_OP_GNU_variable_value DIE."));

	    /* Note: Things still work when the following test is
	       removed.  This test and error is here to conform to the
	       proposed specification.  */
	    if (die_type->code () != TYPE_CODE_INT
	       && die_type->code () != TYPE_CODE_PTR)
	      error (_("Type of DW_OP_GNU_variable_value DIE must be "
		       "an integer or pointer."));

	    auto get_frame_pc = [&symbol_needs] ()
	      {
		symbol_needs = SYMBOL_NEEDS_FRAME;
		return 0;
	      };

	    struct dwarf2_locexpr_baton baton
	      = dwarf2_fetch_die_loc_sect_off (sect_off, per_cu,
					       per_objfile,
					       get_frame_pc, true);

	    /* If SYMBOL_NEEDS_FRAME is returned from the previous call,
	       we dont have to check the baton content.  */
	    if (symbol_needs != SYMBOL_NEEDS_FRAME)
	      {
		gdbarch *arch = baton.per_objfile->objfile->arch ();
		gdb::array_view<const gdb_byte> sub_expr (baton.data,
							  baton.size);
		symbol_needs
		  = dwarf2_get_symbol_read_needs (sub_expr,
						  baton.per_cu,
						  baton.per_objfile,
						  gdbarch_byte_order (arch),
						  baton.per_cu->addr_size (),
						  baton.per_cu->ref_addr_size (),
						  depth);
	      }
	    break;
	  }

	case DW_OP_const_type:
	case DW_OP_GNU_const_type:
	  {
	    uint64_t uoffset;
	    op_ptr = safe_read_uleb128 (op_ptr, expr_end, &uoffset);
	    gdb_byte offset = *op_ptr++;
	    op_ptr += offset;
	    break;
	  }

	default:
	  error (_("Unhandled DWARF expression opcode 0x%x"), op);
	}

      /* If it is known that a frame information is
	 needed we can stop parsing the expression.  */
      if (symbol_needs == SYMBOL_NEEDS_FRAME)
	break;

      insert_in_ops_to_visit (op_ptr);
    }

  return symbol_needs;
}

/* A helper function that throws an unimplemented error mentioning a
   given DWARF operator.  */

static void ATTRIBUTE_NORETURN
unimplemented (unsigned int op)
{
  const char *name = get_DW_OP_name (op);

  if (name)
    error (_("DWARF operator %s cannot be translated to an agent expression"),
	   name);
  else
    error (_("Unknown DWARF operator 0x%02x cannot be translated "
	     "to an agent expression"),
	   op);
}

/* See dwarf2/loc.h.

   This is basically a wrapper on gdbarch_dwarf2_reg_to_regnum so that we
   can issue a complaint, which is better than having every target's
   implementation of dwarf2_reg_to_regnum do it.  */

int
dwarf_reg_to_regnum (struct gdbarch *arch, int dwarf_reg)
{
  int reg = gdbarch_dwarf2_reg_to_regnum (arch, dwarf_reg);

  if (reg == -1)
    {
      complaint (_("bad DWARF register number %d"), dwarf_reg);
    }
  return reg;
}

/* Subroutine of dwarf_reg_to_regnum_or_error to simplify it.
   Throw an error because DWARF_REG is bad.  */

static void
throw_bad_regnum_error (ULONGEST dwarf_reg)
{
  /* Still want to print -1 as "-1".
     We *could* have int and ULONGEST versions of dwarf2_reg_to_regnum_or_error
     but that's overkill for now.  */
  if ((int) dwarf_reg == dwarf_reg)
    error (_("Unable to access DWARF register number %d"), (int) dwarf_reg);
  error (_("Unable to access DWARF register number %s"),
	 pulongest (dwarf_reg));
}

/* See dwarf2/loc.h.  */

int
dwarf_reg_to_regnum_or_error (struct gdbarch *arch, ULONGEST dwarf_reg)
{
  int reg;

  if (dwarf_reg > INT_MAX)
    throw_bad_regnum_error (dwarf_reg);
  /* Yes, we will end up issuing a complaint and an error if DWARF_REG is
     bad, but that's ok.  */
  reg = dwarf_reg_to_regnum (arch, (int) dwarf_reg);
  if (reg == -1)
    throw_bad_regnum_error (dwarf_reg);
  return reg;
}

/* A helper function that emits an access to memory.  ARCH is the
   target architecture.  EXPR is the expression which we are building.
   NBITS is the number of bits we want to read.  This emits the
   opcodes needed to read the memory and then extract the desired
   bits.  */

static void
access_memory (struct gdbarch *arch, struct agent_expr *expr, ULONGEST nbits)
{
  ULONGEST nbytes = (nbits + 7) / 8;

  gdb_assert (nbytes > 0 && nbytes <= sizeof (LONGEST));

  if (expr->tracing)
    ax_trace_quick (expr, nbytes);

  if (nbits <= 8)
    ax_simple (expr, aop_ref8);
  else if (nbits <= 16)
    ax_simple (expr, aop_ref16);
  else if (nbits <= 32)
    ax_simple (expr, aop_ref32);
  else
    ax_simple (expr, aop_ref64);

  /* If we read exactly the number of bytes we wanted, we're done.  */
  if (8 * nbytes == nbits)
    return;

  if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG)
    {
      /* On a bits-big-endian machine, we want the high-order
	 NBITS.  */
      ax_const_l (expr, 8 * nbytes - nbits);
      ax_simple (expr, aop_rsh_unsigned);
    }
  else
    {
      /* On a bits-little-endian box, we want the low-order NBITS.  */
      ax_zero_ext (expr, nbits);
    }
}

/* Compile a DWARF location expression to an agent expression.
   
   EXPR is the agent expression we are building.
   LOC is the agent value we modify.
   ARCH is the architecture.
   ADDR_SIZE is the size of addresses, in bytes.
   OP_PTR is the start of the location expression.
   OP_END is one past the last byte of the location expression.
   
   This will throw an exception for various kinds of errors -- for
   example, if the expression cannot be compiled, or if the expression
   is invalid.  */

static void
dwarf2_compile_expr_to_ax (struct agent_expr *expr, struct axs_value *loc,
			   unsigned int addr_size, const gdb_byte *op_ptr,
			   const gdb_byte *op_end,
			   dwarf2_per_cu_data *per_cu,
			   dwarf2_per_objfile *per_objfile)
{
  gdbarch *arch = expr->gdbarch;
  std::vector<int> dw_labels, patches;
  const gdb_byte * const base = op_ptr;
  const gdb_byte *previous_piece = op_ptr;
  enum bfd_endian byte_order = gdbarch_byte_order (arch);
  ULONGEST bits_collected = 0;
  unsigned int addr_size_bits = 8 * addr_size;
  bool bits_big_endian = byte_order == BFD_ENDIAN_BIG;

  std::vector<int> offsets (op_end - op_ptr, -1);

  /* By default we are making an address.  */
  loc->kind = axs_lvalue_memory;

  while (op_ptr < op_end)
    {
      enum dwarf_location_atom op = (enum dwarf_location_atom) *op_ptr;
      uint64_t uoffset, reg;
      int64_t offset;
      int i;

      offsets[op_ptr - base] = expr->buf.size ();
      ++op_ptr;

      /* Our basic approach to code generation is to map DWARF
	 operations directly to AX operations.  However, there are
	 some differences.

	 First, DWARF works on address-sized units, but AX always uses
	 LONGEST.  For most operations we simply ignore this
	 difference; instead we generate sign extensions as needed
	 before division and comparison operations.  It would be nice
	 to omit the sign extensions, but there is no way to determine
	 the size of the target's LONGEST.  (This code uses the size
	 of the host LONGEST in some cases -- that is a bug but it is
	 difficult to fix.)

	 Second, some DWARF operations cannot be translated to AX.
	 For these we simply fail.  See
	 http://sourceware.org/bugzilla/show_bug.cgi?id=11662.  */
      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  ax_const_l (expr, op - DW_OP_lit0);
	  break;

	case DW_OP_addr:
	  uoffset = extract_unsigned_integer (op_ptr, addr_size, byte_order);
	  op_ptr += addr_size;
	  /* Some versions of GCC emit DW_OP_addr before
	     DW_OP_GNU_push_tls_address.  In this case the value is an
	     index, not an address.  We don't support things like
	     branching between the address and the TLS op.  */
	  if (op_ptr >= op_end || *op_ptr != DW_OP_GNU_push_tls_address)
	    uoffset += per_objfile->objfile->text_section_offset ();
	  ax_const_l (expr, uoffset);
	  break;

	case DW_OP_const1u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 1, byte_order));
	  op_ptr += 1;
	  break;

	case DW_OP_const1s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 1, byte_order));
	  op_ptr += 1;
	  break;

	case DW_OP_const2u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 2, byte_order));
	  op_ptr += 2;
	  break;

	case DW_OP_const2s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 2, byte_order));
	  op_ptr += 2;
	  break;

	case DW_OP_const4u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 4, byte_order));
	  op_ptr += 4;
	  break;

	case DW_OP_const4s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 4, byte_order));
	  op_ptr += 4;
	  break;

	case DW_OP_const8u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 8, byte_order));
	  op_ptr += 8;
	  break;

	case DW_OP_const8s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 8, byte_order));
	  op_ptr += 8;
	  break;

	case DW_OP_constu:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &uoffset);
	  ax_const_l (expr, uoffset);
	  break;

	case DW_OP_consts:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  ax_const_l (expr, offset);
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  loc->u.reg = dwarf_reg_to_regnum_or_error (arch, op - DW_OP_reg0);
	  loc->kind = axs_lvalue_register;
	  break;

	case DW_OP_regx:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  loc->u.reg = dwarf_reg_to_regnum_or_error (arch, reg);
	  loc->kind = axs_lvalue_register;
	  break;

	case DW_OP_implicit_value:
	  {
	    uint64_t len;

	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &len);
	    if (op_ptr + len > op_end)
	      error (_("DW_OP_implicit_value: too few bytes available."));
	    if (len > sizeof (ULONGEST))
	      error (_("Cannot translate DW_OP_implicit_value of %d bytes"),
		     (int) len);

	    ax_const_l (expr, extract_unsigned_integer (op_ptr, len,
							byte_order));
	    op_ptr += len;
	    dwarf_expr_require_composition (op_ptr, op_end,
					    "DW_OP_implicit_value");

	    loc->kind = axs_rvalue;
	  }
	  break;

	case DW_OP_stack_value:
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_stack_value");
	  loc->kind = axs_rvalue;
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  i = dwarf_reg_to_regnum_or_error (arch, op - DW_OP_breg0);
	  ax_reg (expr, i);
	  if (offset != 0)
	    {
	      ax_const_l (expr, offset);
	      ax_simple (expr, aop_add);
	    }
	  break;

	case DW_OP_bregx:
	  {
	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	    i = dwarf_reg_to_regnum_or_error (arch, reg);
	    ax_reg (expr, i);
	    if (offset != 0)
	      {
		ax_const_l (expr, offset);
		ax_simple (expr, aop_add);
	      }
	  }
	  break;

	case DW_OP_fbreg:
	  {
	    const gdb_byte *datastart;
	    size_t datalen;
	    const struct block *b;
	    struct symbol *framefunc;

	    b = block_for_pc (expr->scope);

	    if (!b)
	      error (_("No block found for address"));

	    framefunc = b->linkage_function ();

	    if (!framefunc)
	      error (_("No function found for block"));

	    func_get_frame_base_dwarf_block (framefunc, expr->scope,
					     &datastart, &datalen);

	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	    dwarf2_compile_expr_to_ax (expr, loc, addr_size, datastart,
				       datastart + datalen, per_cu,
				       per_objfile);
	    if (loc->kind == axs_lvalue_register)
	      require_rvalue (expr, loc);

	    if (offset != 0)
	      {
		ax_const_l (expr, offset);
		ax_simple (expr, aop_add);
	      }

	    loc->kind = axs_lvalue_memory;
	  }
	  break;

	case DW_OP_dup:
	  ax_simple (expr, aop_dup);
	  break;

	case DW_OP_drop:
	  ax_simple (expr, aop_pop);
	  break;

	case DW_OP_pick:
	  offset = *op_ptr++;
	  ax_pick (expr, offset);
	  break;

	case DW_OP_swap:
	  ax_simple (expr, aop_swap);
	  break;

	case DW_OP_over:
	  ax_pick (expr, 1);
	  break;

	case DW_OP_rot:
	  ax_simple (expr, aop_rot);
	  break;

	case DW_OP_deref:
	case DW_OP_deref_size:
	  {
	    int size;

	    if (op == DW_OP_deref_size)
	      size = *op_ptr++;
	    else
	      size = addr_size;

	    if (size != 1 && size != 2 && size != 4 && size != 8)
	      error (_("Unsupported size %d in %s"),
		     size, get_DW_OP_name (op));
	    access_memory (arch, expr, size * TARGET_CHAR_BIT);
	  }
	  break;

	case DW_OP_abs:
	  /* Sign extend the operand.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_dup);
	  ax_const_l (expr, 0);
	  ax_simple (expr, aop_less_signed);
	  ax_simple (expr, aop_log_not);
	  i = ax_goto (expr, aop_if_goto);
	  /* We have to emit 0 - X.  */
	  ax_const_l (expr, 0);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_sub);
	  ax_label (expr, i, expr->buf.size ());
	  break;

	case DW_OP_neg:
	  /* No need to sign extend here.  */
	  ax_const_l (expr, 0);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_sub);
	  break;

	case DW_OP_not:
	  /* Sign extend the operand.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_bit_not);
	  break;

	case DW_OP_plus_uconst:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  /* It would be really weird to emit `DW_OP_plus_uconst 0',
	     but we micro-optimize anyhow.  */
	  if (reg != 0)
	    {
	      ax_const_l (expr, reg);
	      ax_simple (expr, aop_add);
	    }
	  break;

	case DW_OP_and:
	  ax_simple (expr, aop_bit_and);
	  break;

	case DW_OP_div:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_div_signed);
	  break;

	case DW_OP_minus:
	  ax_simple (expr, aop_sub);
	  break;

	case DW_OP_mod:
	  ax_simple (expr, aop_rem_unsigned);
	  break;

	case DW_OP_mul:
	  ax_simple (expr, aop_mul);
	  break;

	case DW_OP_or:
	  ax_simple (expr, aop_bit_or);
	  break;

	case DW_OP_plus:
	  ax_simple (expr, aop_add);
	  break;

	case DW_OP_shl:
	  ax_simple (expr, aop_lsh);
	  break;

	case DW_OP_shr:
	  ax_simple (expr, aop_rsh_unsigned);
	  break;

	case DW_OP_shra:
	  ax_simple (expr, aop_rsh_signed);
	  break;

	case DW_OP_xor:
	  ax_simple (expr, aop_bit_xor);
	  break;

	case DW_OP_le:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* Note no swap here: A <= B is !(B < A).  */
	  ax_simple (expr, aop_less_signed);
	  ax_simple (expr, aop_log_not);
	  break;

	case DW_OP_ge:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  /* A >= B is !(A < B).  */
	  ax_simple (expr, aop_less_signed);
	  ax_simple (expr, aop_log_not);
	  break;

	case DW_OP_eq:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* No need for a second swap here.  */
	  ax_simple (expr, aop_equal);
	  break;

	case DW_OP_lt:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_less_signed);
	  break;

	case DW_OP_gt:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* Note no swap here: A > B is B < A.  */
	  ax_simple (expr, aop_less_signed);
	  break;

	case DW_OP_ne:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* No need for a swap here.  */
	  ax_simple (expr, aop_equal);
	  ax_simple (expr, aop_log_not);
	  break;

	case DW_OP_call_frame_cfa:
	  {
	    int regnum;
	    CORE_ADDR text_offset;
	    LONGEST off;
	    const gdb_byte *cfa_start, *cfa_end;

	    if (dwarf2_fetch_cfa_info (arch, expr->scope, per_cu,
				       &regnum, &off,
				       &text_offset, &cfa_start, &cfa_end))
	      {
		/* Register.  */
		ax_reg (expr, regnum);
		if (off != 0)
		  {
		    ax_const_l (expr, off);
		    ax_simple (expr, aop_add);
		  }
	      }
	    else
	      {
		/* Another expression.  */
		ax_const_l (expr, text_offset);
		dwarf2_compile_expr_to_ax (expr, loc, addr_size, cfa_start,
					   cfa_end, per_cu, per_objfile);
	      }

	    loc->kind = axs_lvalue_memory;
	  }
	  break;

	case DW_OP_GNU_push_tls_address:
	case DW_OP_form_tls_address:
	  unimplemented (op);
	  break;

	case DW_OP_push_object_address:
	  unimplemented (op);
	  break;

	case DW_OP_skip:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  i = ax_goto (expr, aop_goto);
	  dw_labels.push_back (op_ptr + offset - base);
	  patches.push_back (i);
	  break;

	case DW_OP_bra:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  /* Zero extend the operand.  */
	  ax_zero_ext (expr, addr_size_bits);
	  i = ax_goto (expr, aop_if_goto);
	  dw_labels.push_back (op_ptr + offset - base);
	  patches.push_back (i);
	  break;

	case DW_OP_nop:
	  break;

	case DW_OP_piece:
	case DW_OP_bit_piece:
	  {
	    uint64_t size;

	    if (op_ptr - 1 == previous_piece)
	      error (_("Cannot translate empty pieces to agent expressions"));
	    previous_piece = op_ptr - 1;

	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &size);
	    if (op == DW_OP_piece)
	      {
		size *= 8;
		uoffset = 0;
	      }
	    else
	      op_ptr = safe_read_uleb128 (op_ptr, op_end, &uoffset);

	    if (bits_collected + size > 8 * sizeof (LONGEST))
	      error (_("Expression pieces exceed word size"));

	    /* Access the bits.  */
	    switch (loc->kind)
	      {
	      case axs_lvalue_register:
		ax_reg (expr, loc->u.reg);
		break;

	      case axs_lvalue_memory:
		/* Offset the pointer, if needed.  */
		if (uoffset > 8)
		  {
		    ax_const_l (expr, uoffset / 8);
		    ax_simple (expr, aop_add);
		    uoffset %= 8;
		  }
		access_memory (arch, expr, size);
		break;
	      }

	    /* For a bits-big-endian target, shift up what we already
	       have.  For a bits-little-endian target, shift up the
	       new data.  Note that there is a potential bug here if
	       the DWARF expression leaves multiple values on the
	       stack.  */
	    if (bits_collected > 0)
	      {
		if (bits_big_endian)
		  {
		    ax_simple (expr, aop_swap);
		    ax_const_l (expr, size);
		    ax_simple (expr, aop_lsh);
		    /* We don't need a second swap here, because
		       aop_bit_or is symmetric.  */
		  }
		else
		  {
		    ax_const_l (expr, size);
		    ax_simple (expr, aop_lsh);
		  }
		ax_simple (expr, aop_bit_or);
	      }

	    bits_collected += size;
	    loc->kind = axs_rvalue;
	  }
	  break;

	case DW_OP_GNU_uninit:
	  unimplemented (op);

	case DW_OP_call2:
	case DW_OP_call4:
	  {
	    struct dwarf2_locexpr_baton block;
	    int size = (op == DW_OP_call2 ? 2 : 4);

	    uoffset = extract_unsigned_integer (op_ptr, size, byte_order);
	    op_ptr += size;

	    auto get_frame_pc_from_expr = [expr] ()
	      {
		return expr->scope;
	      };
	    cu_offset cuoffset = (cu_offset) uoffset;
	    block = dwarf2_fetch_die_loc_cu_off (cuoffset, per_cu, per_objfile,
						 get_frame_pc_from_expr);

	    /* DW_OP_call_ref is currently not supported.  */
	    gdb_assert (block.per_cu == per_cu);

	    dwarf2_compile_expr_to_ax (expr, loc, addr_size, block.data,
				       block.data + block.size, per_cu,
				       per_objfile);
	  }
	  break;

	case DW_OP_call_ref:
	  unimplemented (op);

	case DW_OP_GNU_variable_value:
	  unimplemented (op);

	default:
	  unimplemented (op);
	}
    }

  /* Patch all the branches we emitted.  */
  for (int i = 0; i < patches.size (); ++i)
    {
      int targ = offsets[dw_labels[i]];
      if (targ == -1)
	internal_error (_("invalid label"));
      ax_label (expr, patches[i], targ);
    }
}


/* Return the value of SYMBOL in FRAME using the DWARF-2 expression
   evaluator to calculate the location.  */
static struct value *
locexpr_read_variable (struct symbol *symbol, frame_info_ptr frame)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);
  struct value *val;

  val = dwarf2_evaluate_loc_desc (symbol->type (), frame, dlbaton->data,
				  dlbaton->size, dlbaton->per_cu,
				  dlbaton->per_objfile);

  return val;
}

/* Return the value of SYMBOL in FRAME at (callee) FRAME's function
   entry.  SYMBOL should be a function parameter, otherwise NO_ENTRY_VALUE_ERROR
   will be thrown.  */

static struct value *
locexpr_read_variable_at_entry (struct symbol *symbol, frame_info_ptr frame)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);

  return value_of_dwarf_block_entry (symbol->type (), frame, dlbaton->data,
				     dlbaton->size);
}

/* Implementation of get_symbol_read_needs from
   symbol_computed_ops.  */

static enum symbol_needs_kind
locexpr_get_symbol_read_needs (struct symbol *symbol)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);

  gdbarch *arch = dlbaton->per_objfile->objfile->arch ();
  gdb::array_view<const gdb_byte> expr (dlbaton->data, dlbaton->size);

  return dwarf2_get_symbol_read_needs (expr,
				       dlbaton->per_cu,
				       dlbaton->per_objfile,
				       gdbarch_byte_order (arch),
				       dlbaton->per_cu->addr_size (),
				       dlbaton->per_cu->ref_addr_size ());
}

/* Return true if DATA points to the end of a piece.  END is one past
   the last byte in the expression.  */

static int
piece_end_p (const gdb_byte *data, const gdb_byte *end)
{
  return data == end || data[0] == DW_OP_piece || data[0] == DW_OP_bit_piece;
}

/* Helper for locexpr_describe_location_piece that finds the name of a
   DWARF register.  */

static const char *
locexpr_regname (struct gdbarch *gdbarch, int dwarf_regnum)
{
  int regnum;

  /* This doesn't use dwarf_reg_to_regnum_or_error on purpose.
     We'd rather print *something* here than throw an error.  */
  regnum = dwarf_reg_to_regnum (gdbarch, dwarf_regnum);
  /* gdbarch_register_name may just return "", return something more
     descriptive for bad register numbers.  */
  if (regnum == -1)
    {
      /* The text is output as "$bad_register_number".
	 That is why we use the underscores.  */
      return _("bad_register_number");
    }
  return gdbarch_register_name (gdbarch, regnum);
}

/* Nicely describe a single piece of a location, returning an updated
   position in the bytecode sequence.  This function cannot recognize
   all locations; if a location is not recognized, it simply returns
   DATA.  If there is an error during reading, e.g. we run off the end
   of the buffer, an error is thrown.  */

static const gdb_byte *
locexpr_describe_location_piece (struct symbol *symbol, struct ui_file *stream,
				 CORE_ADDR addr, dwarf2_per_cu_data *per_cu,
				 dwarf2_per_objfile *per_objfile,
				 const gdb_byte *data, const gdb_byte *end,
				 unsigned int addr_size)
{
  objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  size_t leb128_size;

  if (data[0] >= DW_OP_reg0 && data[0] <= DW_OP_reg31)
    {
      gdb_printf (stream, _("a variable in $%s"),
		  locexpr_regname (gdbarch, data[0] - DW_OP_reg0));
      data += 1;
    }
  else if (data[0] == DW_OP_regx)
    {
      uint64_t reg;

      data = safe_read_uleb128 (data + 1, end, &reg);
      gdb_printf (stream, _("a variable in $%s"),
		  locexpr_regname (gdbarch, reg));
    }
  else if (data[0] == DW_OP_fbreg)
    {
      const struct block *b;
      struct symbol *framefunc;
      int frame_reg = 0;
      int64_t frame_offset;
      const gdb_byte *base_data, *new_data, *save_data = data;
      size_t base_size;
      int64_t base_offset = 0;

      new_data = safe_read_sleb128 (data + 1, end, &frame_offset);
      if (!piece_end_p (new_data, end))
	return data;
      data = new_data;

      b = block_for_pc (addr);

      if (!b)
	error (_("No block found for address for symbol \"%s\"."),
	       symbol->print_name ());

      framefunc = b->linkage_function ();

      if (!framefunc)
	error (_("No function found for block for symbol \"%s\"."),
	       symbol->print_name ());

      func_get_frame_base_dwarf_block (framefunc, addr, &base_data, &base_size);

      if (base_data[0] >= DW_OP_breg0 && base_data[0] <= DW_OP_breg31)
	{
	  const gdb_byte *buf_end;
	  
	  frame_reg = base_data[0] - DW_OP_breg0;
	  buf_end = safe_read_sleb128 (base_data + 1, base_data + base_size,
				       &base_offset);
	  if (buf_end != base_data + base_size)
	    error (_("Unexpected opcode after "
		     "DW_OP_breg%u for symbol \"%s\"."),
		   frame_reg, symbol->print_name ());
	}
      else if (base_data[0] >= DW_OP_reg0 && base_data[0] <= DW_OP_reg31)
	{
	  /* The frame base is just the register, with no offset.  */
	  frame_reg = base_data[0] - DW_OP_reg0;
	  base_offset = 0;
	}
      else
	{
	  /* We don't know what to do with the frame base expression,
	     so we can't trace this variable; give up.  */
	  return save_data;
	}

      gdb_printf (stream,
		  _("a variable at frame base reg $%s offset %s+%s"),
		  locexpr_regname (gdbarch, frame_reg),
		  plongest (base_offset), plongest (frame_offset));
    }
  else if (data[0] >= DW_OP_breg0 && data[0] <= DW_OP_breg31
	   && piece_end_p (data, end))
    {
      int64_t offset;

      data = safe_read_sleb128 (data + 1, end, &offset);

      gdb_printf (stream,
		  _("a variable at offset %s from base reg $%s"),
		  plongest (offset),
		  locexpr_regname (gdbarch, data[0] - DW_OP_breg0));
    }

  /* The location expression for a TLS variable looks like this (on a
     64-bit LE machine):

     DW_AT_location    : 10 byte block: 3 4 0 0 0 0 0 0 0 e0
			(DW_OP_addr: 4; DW_OP_GNU_push_tls_address)

     0x3 is the encoding for DW_OP_addr, which has an operand as long
     as the size of an address on the target machine (here is 8
     bytes).  Note that more recent version of GCC emit DW_OP_const4u
     or DW_OP_const8u, depending on address size, rather than
     DW_OP_addr.  0xe0 is the encoding for DW_OP_GNU_push_tls_address.
     The operand represents the offset at which the variable is within
     the thread local storage.  */

  else if (data + 1 + addr_size < end
	   && (data[0] == DW_OP_addr
	       || (addr_size == 4 && data[0] == DW_OP_const4u)
	       || (addr_size == 8 && data[0] == DW_OP_const8u))
	   && (data[1 + addr_size] == DW_OP_GNU_push_tls_address
	       || data[1 + addr_size] == DW_OP_form_tls_address)
	   && piece_end_p (data + 2 + addr_size, end))
    {
      ULONGEST offset;
      offset = extract_unsigned_integer (data + 1, addr_size,
					 gdbarch_byte_order (gdbarch));

      gdb_printf (stream, 
		  _("a thread-local variable at offset 0x%s "
		    "in the thread-local storage for `%s'"),
		  phex_nz (offset, addr_size), objfile_name (objfile));

      data += 1 + addr_size + 1;
    }

  /* With -gsplit-dwarf a TLS variable can also look like this:
     DW_AT_location    : 3 byte block: fc 4 e0
			(DW_OP_GNU_const_index: 4;
			 DW_OP_GNU_push_tls_address)  */
  else if (data + 3 <= end
	   && data + 1 + (leb128_size = skip_leb128 (data + 1, end)) < end
	   && data[0] == DW_OP_GNU_const_index
	   && leb128_size > 0
	   && (data[1 + leb128_size] == DW_OP_GNU_push_tls_address
	       || data[1 + leb128_size] == DW_OP_form_tls_address)
	   && piece_end_p (data + 2 + leb128_size, end))
    {
      uint64_t offset;

      data = safe_read_uleb128 (data + 1, end, &offset);
      offset = (uint64_t) dwarf2_read_addr_index (per_cu, per_objfile, offset);
      gdb_printf (stream, 
		  _("a thread-local variable at offset 0x%s "
		    "in the thread-local storage for `%s'"),
		  phex_nz (offset, addr_size), objfile_name (objfile));
      ++data;
    }

  else if (data[0] >= DW_OP_lit0
	   && data[0] <= DW_OP_lit31
	   && data + 1 < end
	   && data[1] == DW_OP_stack_value)
    {
      gdb_printf (stream, _("the constant %d"), data[0] - DW_OP_lit0);
      data += 2;
    }

  return data;
}

/* Disassemble an expression, stopping at the end of a piece or at the
   end of the expression.  Returns a pointer to the next unread byte
   in the input expression.  If ALL is nonzero, then this function
   will keep going until it reaches the end of the expression.
   If there is an error during reading, e.g. we run off the end
   of the buffer, an error is thrown.  */

static const gdb_byte *
disassemble_dwarf_expression (struct ui_file *stream,
			      struct gdbarch *arch, unsigned int addr_size,
			      int offset_size, const gdb_byte *start,
			      const gdb_byte *data, const gdb_byte *end,
			      int indent, int all,
			      dwarf2_per_cu_data *per_cu,
			      dwarf2_per_objfile *per_objfile)
{
  while (data < end
	 && (all
	     || (data[0] != DW_OP_piece && data[0] != DW_OP_bit_piece)))
    {
      enum dwarf_location_atom op = (enum dwarf_location_atom) *data++;
      uint64_t ul;
      int64_t l;
      const char *name;

      name = get_DW_OP_name (op);

      if (!name)
	error (_("Unrecognized DWARF opcode 0x%02x at %ld"),
	       op, (long) (data - 1 - start));
      gdb_printf (stream, "  %*ld: %s", indent + 4,
		  (long) (data - 1 - start), name);

      switch (op)
	{
	case DW_OP_addr:
	  ul = extract_unsigned_integer (data, addr_size,
					 gdbarch_byte_order (arch));
	  data += addr_size;
	  gdb_printf (stream, " 0x%s", phex_nz (ul, addr_size));
	  break;

	case DW_OP_const1u:
	  ul = extract_unsigned_integer (data, 1, gdbarch_byte_order (arch));
	  data += 1;
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_const1s:
	  l = extract_signed_integer (data, 1, gdbarch_byte_order (arch));
	  data += 1;
	  gdb_printf (stream, " %s", plongest (l));
	  break;

	case DW_OP_const2u:
	  ul = extract_unsigned_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_const2s:
	  l = extract_signed_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  gdb_printf (stream, " %s", plongest (l));
	  break;

	case DW_OP_const4u:
	  ul = extract_unsigned_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_const4s:
	  l = extract_signed_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  gdb_printf (stream, " %s", plongest (l));
	  break;

	case DW_OP_const8u:
	  ul = extract_unsigned_integer (data, 8, gdbarch_byte_order (arch));
	  data += 8;
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_const8s:
	  l = extract_signed_integer (data, 8, gdbarch_byte_order (arch));
	  data += 8;
	  gdb_printf (stream, " %s", plongest (l));
	  break;

	case DW_OP_constu:
	  data = safe_read_uleb128 (data, end, &ul);
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_consts:
	  data = safe_read_sleb128 (data, end, &l);
	  gdb_printf (stream, " %s", plongest (l));
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  gdb_printf (stream, " [$%s]",
		      locexpr_regname (arch, op - DW_OP_reg0));
	  break;

	case DW_OP_regx:
	  data = safe_read_uleb128 (data, end, &ul);
	  gdb_printf (stream, " %s [$%s]", pulongest (ul),
		      locexpr_regname (arch, (int) ul));
	  break;

	case DW_OP_implicit_value:
	  data = safe_read_uleb128 (data, end, &ul);
	  data += ul;
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  data = safe_read_sleb128 (data, end, &l);
	  gdb_printf (stream, " %s [$%s]", plongest (l),
		      locexpr_regname (arch, op - DW_OP_breg0));
	  break;

	case DW_OP_bregx:
	  data = safe_read_uleb128 (data, end, &ul);
	  data = safe_read_sleb128 (data, end, &l);
	  gdb_printf (stream, " register %s [$%s] offset %s",
		      pulongest (ul),
		      locexpr_regname (arch, (int) ul),
		      plongest (l));
	  break;

	case DW_OP_fbreg:
	  data = safe_read_sleb128 (data, end, &l);
	  gdb_printf (stream, " %s", plongest (l));
	  break;

	case DW_OP_xderef_size:
	case DW_OP_deref_size:
	case DW_OP_pick:
	  gdb_printf (stream, " %d", *data);
	  ++data;
	  break;

	case DW_OP_plus_uconst:
	  data = safe_read_uleb128 (data, end, &ul);
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_skip:
	  l = extract_signed_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  gdb_printf (stream, " to %ld",
		      (long) (data + l - start));
	  break;

	case DW_OP_bra:
	  l = extract_signed_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  gdb_printf (stream, " %ld",
		      (long) (data + l - start));
	  break;

	case DW_OP_call2:
	  ul = extract_unsigned_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  gdb_printf (stream, " offset %s", phex_nz (ul, 2));
	  break;

	case DW_OP_call4:
	  ul = extract_unsigned_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  gdb_printf (stream, " offset %s", phex_nz (ul, 4));
	  break;

	case DW_OP_call_ref:
	  ul = extract_unsigned_integer (data, offset_size,
					 gdbarch_byte_order (arch));
	  data += offset_size;
	  gdb_printf (stream, " offset %s", phex_nz (ul, offset_size));
	  break;

	case DW_OP_piece:
	  data = safe_read_uleb128 (data, end, &ul);
	  gdb_printf (stream, " %s (bytes)", pulongest (ul));
	  break;

	case DW_OP_bit_piece:
	  {
	    uint64_t offset;

	    data = safe_read_uleb128 (data, end, &ul);
	    data = safe_read_uleb128 (data, end, &offset);
	    gdb_printf (stream, " size %s offset %s (bits)",
			pulongest (ul), pulongest (offset));
	  }
	  break;

	case DW_OP_implicit_pointer:
	case DW_OP_GNU_implicit_pointer:
	  {
	    ul = extract_unsigned_integer (data, offset_size,
					   gdbarch_byte_order (arch));
	    data += offset_size;

	    data = safe_read_sleb128 (data, end, &l);

	    gdb_printf (stream, " DIE %s offset %s",
			phex_nz (ul, offset_size),
			plongest (l));
	  }
	  break;

	case DW_OP_deref_type:
	case DW_OP_GNU_deref_type:
	  {
	    int deref_addr_size = *data++;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset offset = (cu_offset) ul;
	    type = dwarf2_get_die_type (offset, per_cu, per_objfile);
	    gdb_printf (stream, "<");
	    type_print (type, "", stream, -1);
	    gdb_printf (stream, " [0x%s]> %d",
			phex_nz (to_underlying (offset), 0),
			deref_addr_size);
	  }
	  break;

	case DW_OP_const_type:
	case DW_OP_GNU_const_type:
	  {
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset type_die = (cu_offset) ul;
	    type = dwarf2_get_die_type (type_die, per_cu, per_objfile);
	    gdb_printf (stream, "<");
	    type_print (type, "", stream, -1);
	    gdb_printf (stream, " [0x%s]>",
			phex_nz (to_underlying (type_die), 0));

	    int n = *data++;
	    gdb_printf (stream, " %d byte block:", n);
	    for (int i = 0; i < n; ++i)
	      gdb_printf (stream, " %02x", data[i]);
	    data += n;
	  }
	  break;

	case DW_OP_regval_type:
	case DW_OP_GNU_regval_type:
	  {
	    uint64_t reg;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &reg);
	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset type_die = (cu_offset) ul;

	    type = dwarf2_get_die_type (type_die, per_cu, per_objfile);
	    gdb_printf (stream, "<");
	    type_print (type, "", stream, -1);
	    gdb_printf (stream, " [0x%s]> [$%s]",
			phex_nz (to_underlying (type_die), 0),
			locexpr_regname (arch, reg));
	  }
	  break;

	case DW_OP_convert:
	case DW_OP_GNU_convert:
	case DW_OP_reinterpret:
	case DW_OP_GNU_reinterpret:
	  {
	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset type_die = (cu_offset) ul;

	    if (to_underlying (type_die) == 0)
	      gdb_printf (stream, "<0>");
	    else
	      {
		struct type *type;

		type = dwarf2_get_die_type (type_die, per_cu, per_objfile);
		gdb_printf (stream, "<");
		type_print (type, "", stream, -1);
		gdb_printf (stream, " [0x%s]>",
			    phex_nz (to_underlying (type_die), 0));
	      }
	  }
	  break;

	case DW_OP_entry_value:
	case DW_OP_GNU_entry_value:
	  data = safe_read_uleb128 (data, end, &ul);
	  gdb_putc ('\n', stream);
	  disassemble_dwarf_expression (stream, arch, addr_size, offset_size,
					start, data, data + ul, indent + 2,
					all, per_cu, per_objfile);
	  data += ul;
	  continue;

	case DW_OP_GNU_parameter_ref:
	  ul = extract_unsigned_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  gdb_printf (stream, " offset %s", phex_nz (ul, 4));
	  break;

	case DW_OP_addrx:
	case DW_OP_GNU_addr_index:
	  data = safe_read_uleb128 (data, end, &ul);
	  ul = (uint64_t) dwarf2_read_addr_index (per_cu, per_objfile, ul);
	  gdb_printf (stream, " 0x%s", phex_nz (ul, addr_size));
	  break;

	case DW_OP_GNU_const_index:
	  data = safe_read_uleb128 (data, end, &ul);
	  ul = (uint64_t) dwarf2_read_addr_index (per_cu, per_objfile, ul);
	  gdb_printf (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_GNU_variable_value:
	  ul = extract_unsigned_integer (data, offset_size,
					 gdbarch_byte_order (arch));
	  data += offset_size;
	  gdb_printf (stream, " offset %s", phex_nz (ul, offset_size));
	  break;
	}

      gdb_printf (stream, "\n");
    }

  return data;
}

static bool dwarf_always_disassemble;

static void
show_dwarf_always_disassemble (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Whether to always disassemble "
		"DWARF expressions is %s.\n"),
	      value);
}

/* Describe a single location, which may in turn consist of multiple
   pieces.  */

static void
locexpr_describe_location_1 (struct symbol *symbol, CORE_ADDR addr,
			     struct ui_file *stream,
			     const gdb_byte *data, size_t size,
			     unsigned int addr_size,
			     int offset_size, dwarf2_per_cu_data *per_cu,
			     dwarf2_per_objfile *per_objfile)
{
  const gdb_byte *end = data + size;
  int first_piece = 1, bad = 0;
  objfile *objfile = per_objfile->objfile;

  while (data < end)
    {
      const gdb_byte *here = data;
      int disassemble = 1;

      if (first_piece)
	first_piece = 0;
      else
	gdb_printf (stream, _(", and "));

      if (!dwarf_always_disassemble)
	{
	  data = locexpr_describe_location_piece (symbol, stream,
						  addr, per_cu, per_objfile,
						  data, end, addr_size);
	  /* If we printed anything, or if we have an empty piece,
	     then don't disassemble.  */
	  if (data != here
	      || data[0] == DW_OP_piece
	      || data[0] == DW_OP_bit_piece)
	    disassemble = 0;
	}
      if (disassemble)
	{
	  gdb_printf (stream, _("a complex DWARF expression:\n"));
	  data = disassemble_dwarf_expression (stream,
					       objfile->arch (),
					       addr_size, offset_size, data,
					       data, end, 0,
					       dwarf_always_disassemble,
					       per_cu, per_objfile);
	}

      if (data < end)
	{
	  int empty = data == here;
	      
	  if (disassemble)
	    gdb_printf (stream, "   ");
	  if (data[0] == DW_OP_piece)
	    {
	      uint64_t bytes;

	      data = safe_read_uleb128 (data + 1, end, &bytes);

	      if (empty)
		gdb_printf (stream, _("an empty %s-byte piece"),
			    pulongest (bytes));
	      else
		gdb_printf (stream, _(" [%s-byte piece]"),
			    pulongest (bytes));
	    }
	  else if (data[0] == DW_OP_bit_piece)
	    {
	      uint64_t bits, offset;

	      data = safe_read_uleb128 (data + 1, end, &bits);
	      data = safe_read_uleb128 (data, end, &offset);

	      if (empty)
		gdb_printf (stream,
			    _("an empty %s-bit piece"),
			    pulongest (bits));
	      else
		gdb_printf (stream,
			    _(" [%s-bit piece, offset %s bits]"),
			    pulongest (bits), pulongest (offset));
	    }
	  else
	    {
	      bad = 1;
	      break;
	    }
	}
    }

  if (bad || data > end)
    error (_("Corrupted DWARF2 expression for \"%s\"."),
	   symbol->print_name ());
}

/* Print a natural-language description of SYMBOL to STREAM.  This
   version is for a symbol with a single location.  */

static void
locexpr_describe_location (struct symbol *symbol, CORE_ADDR addr,
			   struct ui_file *stream)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);
  unsigned int addr_size = dlbaton->per_cu->addr_size ();
  int offset_size = dlbaton->per_cu->offset_size ();

  locexpr_describe_location_1 (symbol, addr, stream,
			       dlbaton->data, dlbaton->size,
			       addr_size, offset_size,
			       dlbaton->per_cu, dlbaton->per_objfile);
}

/* Describe the location of SYMBOL as an agent value in VALUE, generating
   any necessary bytecode in AX.  */

static void
locexpr_tracepoint_var_ref (struct symbol *symbol, struct agent_expr *ax,
			    struct axs_value *value)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);
  unsigned int addr_size = dlbaton->per_cu->addr_size ();

  if (dlbaton->size == 0)
    value->optimized_out = 1;
  else
    dwarf2_compile_expr_to_ax (ax, value, addr_size, dlbaton->data,
			       dlbaton->data + dlbaton->size, dlbaton->per_cu,
			       dlbaton->per_objfile);
}

/* symbol_computed_ops 'generate_c_location' method.  */

static void
locexpr_generate_c_location (struct symbol *sym, string_file *stream,
			     struct gdbarch *gdbarch,
			     std::vector<bool> &registers_used,
			     CORE_ADDR pc, const char *result_name)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (sym);
  unsigned int addr_size = dlbaton->per_cu->addr_size ();

  if (dlbaton->size == 0)
    error (_("symbol \"%s\" is optimized out"), sym->natural_name ());

  compile_dwarf_expr_to_c (stream, result_name,
			   sym, pc, gdbarch, registers_used, addr_size,
			   dlbaton->data, dlbaton->data + dlbaton->size,
			   dlbaton->per_cu, dlbaton->per_objfile);
}

/* The set of location functions used with the DWARF-2 expression
   evaluator.  */
const struct symbol_computed_ops dwarf2_locexpr_funcs = {
  locexpr_read_variable,
  locexpr_read_variable_at_entry,
  locexpr_get_symbol_read_needs,
  locexpr_describe_location,
  0,	/* location_has_loclist */
  locexpr_tracepoint_var_ref,
  locexpr_generate_c_location
};


/* Wrapper functions for location lists.  These generally find
   the appropriate location expression and call something above.  */

/* Return the value of SYMBOL in FRAME using the DWARF-2 expression
   evaluator to calculate the location.  */
static struct value *
loclist_read_variable (struct symbol *symbol, frame_info_ptr frame)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  struct value *val;
  const gdb_byte *data;
  size_t size;
  CORE_ADDR pc = frame ? get_frame_address_in_block (frame) : 0;

  data = dwarf2_find_location_expression (dlbaton, &size, pc);
  val = dwarf2_evaluate_loc_desc (symbol->type (), frame, data, size,
				  dlbaton->per_cu, dlbaton->per_objfile);

  return val;
}

/* Read variable SYMBOL like loclist_read_variable at (callee) FRAME's function
   entry.  SYMBOL should be a function parameter, otherwise NO_ENTRY_VALUE_ERROR
   will be thrown.

   Function always returns non-NULL value, it may be marked optimized out if
   inferior frame information is not available.  It throws NO_ENTRY_VALUE_ERROR
   if it cannot resolve the parameter for any reason.  */

static struct value *
loclist_read_variable_at_entry (struct symbol *symbol, frame_info_ptr frame)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  const gdb_byte *data;
  size_t size;
  CORE_ADDR pc;

  if (frame == NULL || !get_frame_func_if_available (frame, &pc))
    return value::allocate_optimized_out (symbol->type ());

  data = dwarf2_find_location_expression (dlbaton, &size, pc, true);
  if (data == NULL)
    return value::allocate_optimized_out (symbol->type ());

  return value_of_dwarf_block_entry (symbol->type (), frame, data, size);
}

/* Implementation of get_symbol_read_needs from
   symbol_computed_ops.  */

static enum symbol_needs_kind
loclist_symbol_needs (struct symbol *symbol)
{
  /* If there's a location list, then assume we need to have a frame
     to choose the appropriate location expression.  With tracking of
     global variables this is not necessarily true, but such tracking
     is disabled in GCC at the moment until we figure out how to
     represent it.  */

  return SYMBOL_NEEDS_FRAME;
}

/* Print a natural-language description of SYMBOL to STREAM.  This
   version applies when there is a list of different locations, each
   with a specified address range.  */

static void
loclist_describe_location (struct symbol *symbol, CORE_ADDR addr,
			   struct ui_file *stream)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  const gdb_byte *loc_ptr, *buf_end;
  dwarf2_per_objfile *per_objfile = dlbaton->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int addr_size = dlbaton->per_cu->addr_size ();
  int offset_size = dlbaton->per_cu->offset_size ();
  int signed_addr_p = bfd_get_sign_extend_vma (objfile->obfd.get ());
  unrelocated_addr base_address = dlbaton->base_address;
  int done = 0;

  loc_ptr = dlbaton->data;
  buf_end = dlbaton->data + dlbaton->size;

  gdb_printf (stream, _("multi-location:\n"));

  /* Iterate through locations until we run out.  */
  while (!done)
    {
      unrelocated_addr low = {}, high = {}; /* init for gcc -Wall */
      int length;
      enum debug_loc_kind kind;
      const gdb_byte *new_ptr = NULL; /* init for gcc -Wall */

      if (dlbaton->per_cu->version () < 5 && dlbaton->from_dwo)
	kind = decode_debug_loc_dwo_addresses (dlbaton->per_cu,
					       per_objfile,
					       loc_ptr, buf_end, &new_ptr,
					       &low, &high, byte_order);
      else if (dlbaton->per_cu->version () < 5)
	kind = decode_debug_loc_addresses (loc_ptr, buf_end, &new_ptr,
					   &low, &high,
					   byte_order, addr_size,
					   signed_addr_p);
      else
	kind = decode_debug_loclists_addresses (dlbaton->per_cu,
						per_objfile,
						loc_ptr, buf_end, &new_ptr,
						&low, &high, byte_order,
						addr_size, signed_addr_p);
      loc_ptr = new_ptr;
      switch (kind)
	{
	case DEBUG_LOC_END_OF_LIST:
	  done = 1;
	  continue;

	case DEBUG_LOC_BASE_ADDRESS:
	  base_address = high;
	  gdb_printf (stream, _("  Base address %s"),
		      paddress (gdbarch, (CORE_ADDR) base_address));
	  continue;

	case DEBUG_LOC_START_END:
	case DEBUG_LOC_START_LENGTH:
	case DEBUG_LOC_OFFSET_PAIR:
	  break;

	case DEBUG_LOC_BUFFER_OVERFLOW:
	case DEBUG_LOC_INVALID_ENTRY:
	  error (_("Corrupted DWARF expression for symbol \"%s\"."),
		 symbol->print_name ());

	default:
	  gdb_assert_not_reached ("bad debug_loc_kind");
	}

      /* Otherwise, a location expression entry.  */
      if (!dlbaton->from_dwo && kind == DEBUG_LOC_OFFSET_PAIR)
	{
	  low = (unrelocated_addr) ((CORE_ADDR) low
				    + (CORE_ADDR) base_address);
	  high = (unrelocated_addr) ((CORE_ADDR) high
				     + (CORE_ADDR) base_address);
	}

      CORE_ADDR low_reloc = per_objfile->relocate (low);
      CORE_ADDR high_reloc = per_objfile->relocate (high);

      if (dlbaton->per_cu->version () < 5)
	 {
	   length = extract_unsigned_integer (loc_ptr, 2, byte_order);
	   loc_ptr += 2;
	 }
      else
	 {
	   unsigned int bytes_read;
	   length = read_unsigned_leb128 (NULL, loc_ptr, &bytes_read);
	   loc_ptr += bytes_read;
	 }

      /* (It would improve readability to print only the minimum
	 necessary digits of the second number of the range.)  */
      gdb_printf (stream, _("  Range %s-%s: "),
		  paddress (gdbarch, low_reloc),
		  paddress (gdbarch, high_reloc));

      /* Now describe this particular location.  */
      locexpr_describe_location_1 (symbol, low_reloc, stream, loc_ptr, length,
				   addr_size, offset_size,
				   dlbaton->per_cu, per_objfile);

      gdb_printf (stream, "\n");

      loc_ptr += length;
    }
}

/* Describe the location of SYMBOL as an agent value in VALUE, generating
   any necessary bytecode in AX.  */
static void
loclist_tracepoint_var_ref (struct symbol *symbol, struct agent_expr *ax,
			    struct axs_value *value)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  const gdb_byte *data;
  size_t size;
  unsigned int addr_size = dlbaton->per_cu->addr_size ();

  data = dwarf2_find_location_expression (dlbaton, &size, ax->scope);
  if (size == 0)
    value->optimized_out = 1;
  else
    dwarf2_compile_expr_to_ax (ax, value, addr_size, data, data + size,
			       dlbaton->per_cu, dlbaton->per_objfile);
}

/* symbol_computed_ops 'generate_c_location' method.  */

static void
loclist_generate_c_location (struct symbol *sym, string_file *stream,
			     struct gdbarch *gdbarch,
			     std::vector<bool> &registers_used,
			     CORE_ADDR pc, const char *result_name)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (sym);
  unsigned int addr_size = dlbaton->per_cu->addr_size ();
  const gdb_byte *data;
  size_t size;

  data = dwarf2_find_location_expression (dlbaton, &size, pc);
  if (size == 0)
    error (_("symbol \"%s\" is optimized out"), sym->natural_name ());

  compile_dwarf_expr_to_c (stream, result_name,
			   sym, pc, gdbarch, registers_used, addr_size,
			   data, data + size,
			   dlbaton->per_cu,
			   dlbaton->per_objfile);
}

/* The set of location functions used with the DWARF-2 expression
   evaluator and location lists.  */
const struct symbol_computed_ops dwarf2_loclist_funcs = {
  loclist_read_variable,
  loclist_read_variable_at_entry,
  loclist_symbol_needs,
  loclist_describe_location,
  1,	/* location_has_loclist */
  loclist_tracepoint_var_ref,
  loclist_generate_c_location
};

void _initialize_dwarf2loc ();
void
_initialize_dwarf2loc ()
{
  add_setshow_zuinteger_cmd ("entry-values", class_maintenance,
			     &entry_values_debug,
			     _("Set entry values and tail call frames "
			       "debugging."),
			     _("Show entry values and tail call frames "
			       "debugging."),
			     _("When non-zero, the process of determining "
			       "parameter values from function entry point "
			       "and tail call frames will be printed."),
			     NULL,
			     show_entry_values_debug,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("always-disassemble", class_obscure,
			   &dwarf_always_disassemble, _("\
Set whether `info address' always disassembles DWARF expressions."), _("\
Show whether `info address' always disassembles DWARF expressions."), _("\
When enabled, DWARF expressions are always printed in an assembly-like\n\
syntax.  When disabled, expressions will be printed in a more\n\
conversational style, when possible."),
			   NULL,
			   show_dwarf_always_disassemble,
			   &set_dwarf_cmdlist,
			   &show_dwarf_cmdlist);
}

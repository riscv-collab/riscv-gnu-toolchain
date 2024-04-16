/* DWARF aranges handling

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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
#include "dwarf2/aranges.h"
#include "dwarf2/read.h"

/* See aranges.h.  */

bool
read_addrmap_from_aranges (dwarf2_per_objfile *per_objfile,
			   dwarf2_section_info *section,
			   addrmap *mutable_map,
			   deferred_warnings *warn)
{
  /* Caller must ensure that the section has already been read.  */
  gdb_assert (section->readin);
  if (section->empty ())
    return false;

  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  struct gdbarch *gdbarch = objfile->arch ();
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  std::unordered_map<sect_offset,
		     dwarf2_per_cu_data *,
		     gdb::hash_enum<sect_offset>>
    debug_info_offset_to_per_cu;
  for (const auto &per_cu : per_bfd->all_units)
    {
      /* A TU will not need aranges, and skipping them here is an easy
	 way of ignoring .debug_types -- and possibly seeing a
	 duplicate section offset -- entirely.  The same applies to
	 units coming from a dwz file.  */
      if (per_cu->is_debug_types || per_cu->is_dwz)
	continue;

      const auto insertpair
	= debug_info_offset_to_per_cu.emplace (per_cu->sect_off,
					       per_cu.get ());

      /* Assume no duplicate offsets in all_units.  */
      gdb_assert (insertpair.second);
    }

  std::set<sect_offset> debug_info_offset_seen;
  const bfd_endian dwarf5_byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *addr = section->buffer;
  while (addr < section->buffer + section->size)
    {
      const gdb_byte *const entry_addr = addr;
      unsigned int bytes_read;

      const LONGEST entry_length = read_initial_length (abfd, addr,
							&bytes_read);
      addr += bytes_read;

      const gdb_byte *const entry_end = addr + entry_length;
      const bool dwarf5_is_dwarf64 = bytes_read != 4;
      const uint8_t offset_size = dwarf5_is_dwarf64 ? 8 : 4;
      if (addr + entry_length > section->buffer + section->size)
	{
	  warn->warn (_("Section .debug_aranges in %s entry at offset %s "
			"length %s exceeds section length %s, "
			"ignoring .debug_aranges."),
		      objfile_name (objfile),
		      plongest (entry_addr - section->buffer),
		      plongest (bytes_read + entry_length),
		      pulongest (section->size));
	  return false;
	}

      /* The version number.  */
      const uint16_t version = read_2_bytes (abfd, addr);
      addr += 2;
      if (version != 2)
	{
	  warn->warn
	    (_("Section .debug_aranges in %s entry at offset %s "
	       "has unsupported version %d, ignoring .debug_aranges."),
	     objfile_name (objfile),
	     plongest (entry_addr - section->buffer), version);
	  return false;
	}

      const uint64_t debug_info_offset
	= extract_unsigned_integer (addr, offset_size, dwarf5_byte_order);
      addr += offset_size;
      const auto per_cu_it
	= debug_info_offset_to_per_cu.find (sect_offset (debug_info_offset));
      if (per_cu_it == debug_info_offset_to_per_cu.cend ())
	{
	  warn->warn (_("Section .debug_aranges in %s entry at offset %s "
			"debug_info_offset %s does not exists, "
			"ignoring .debug_aranges."),
		      objfile_name (objfile),
		      plongest (entry_addr - section->buffer),
		      pulongest (debug_info_offset));
	  return false;
	}
      const auto insertpair
	= debug_info_offset_seen.insert (sect_offset (debug_info_offset));
      if (!insertpair.second)
	{
	  warn->warn (_("Section .debug_aranges in %s has duplicate "
			"debug_info_offset %s, ignoring .debug_aranges."),
		      objfile_name (objfile),
		      sect_offset_str (sect_offset (debug_info_offset)));
	  return false;
	}
      dwarf2_per_cu_data *const per_cu = per_cu_it->second;

      const uint8_t address_size = *addr++;
      if (address_size < 1 || address_size > 8)
	{
	  warn->warn
	    (_("Section .debug_aranges in %s entry at offset %s "
	       "address_size %u is invalid, ignoring .debug_aranges."),
	     objfile_name (objfile),
	     plongest (entry_addr - section->buffer), address_size);
	  return false;
	}

      const uint8_t segment_selector_size = *addr++;
      if (segment_selector_size != 0)
	{
	  warn->warn (_("Section .debug_aranges in %s entry at offset %s "
			"segment_selector_size %u is not supported, "
			"ignoring .debug_aranges."),
		      objfile_name (objfile),
		      plongest (entry_addr - section->buffer),
		      segment_selector_size);
	  return false;
	}

      /* Must pad to an alignment boundary that is twice the address
	 size.  It is undocumented by the DWARF standard but GCC does
	 use it.  However, not every compiler does this.  We can see
	 whether it has happened by looking at the total length of the
	 contents of the aranges for this CU -- it if isn't a multiple
	 of twice the address size, then we skip any leftover
	 bytes.  */
      addr += (entry_end - addr) % (2 * address_size);

      while (addr < entry_end)
	{
	  if (addr + 2 * address_size > entry_end)
	    {
	      warn->warn (_("Section .debug_aranges in %s entry at offset %s "
			    "address list is not properly terminated, "
			    "ignoring .debug_aranges."),
			  objfile_name (objfile),
			  plongest (entry_addr - section->buffer));
	      return false;
	    }
	  ULONGEST start = extract_unsigned_integer (addr, address_size,
						     dwarf5_byte_order);
	  addr += address_size;
	  ULONGEST length = extract_unsigned_integer (addr, address_size,
						      dwarf5_byte_order);
	  addr += address_size;
	  if (start == 0 && length == 0)
	    {
	      /* This can happen on some targets with --gc-sections.
		 This pair of values is also used to mark the end of
		 the entries for a given CU, but we ignore it and
		 instead handle termination using the check at the top
		 of the loop.  */
	      continue;
	    }
	  if (start == 0 && !per_bfd->has_section_at_zero)
	    {
	      /* Symbol was eliminated due to a COMDAT group.  */
	      continue;
	    }
	  ULONGEST end = start + length;
	  start = (ULONGEST) per_objfile->adjust ((unrelocated_addr) start);
	  end = (ULONGEST) per_objfile->adjust ((unrelocated_addr) end);
	  mutable_map->set_empty (start, end - 1, per_cu);
	}

      per_cu->addresses_seen = true;
    }

  return true;
}

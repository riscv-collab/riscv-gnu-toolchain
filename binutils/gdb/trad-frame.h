/* Traditional frame unwind support, for GDB the GNU Debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef TRAD_FRAME_H
#define TRAD_FRAME_H

#include "frame.h"

class frame_info_ptr;
struct regcache_map_entry;
struct trad_frame_cache;

/* A simple, or traditional frame cache.

   The entire cache is populated in a single pass and then generic
   routines are used to extract the various cache values.  */

struct trad_frame_cache *trad_frame_cache_zalloc (frame_info_ptr);

/* This frame's ID.  */
void trad_frame_set_id (struct trad_frame_cache *this_trad_cache,
			struct frame_id this_id);
void trad_frame_get_id (struct trad_frame_cache *this_trad_cache,
			struct frame_id *this_id);
void trad_frame_set_this_base (struct trad_frame_cache *this_trad_cache,
			       CORE_ADDR this_base);
CORE_ADDR trad_frame_get_this_base (struct trad_frame_cache *this_trad_cache);

void trad_frame_set_reg_realreg (struct trad_frame_cache *this_trad_cache,
				 int regnum, int realreg);
void trad_frame_set_reg_addr (struct trad_frame_cache *this_trad_cache,
			      int regnum, CORE_ADDR addr);
void trad_frame_set_reg_regmap (struct trad_frame_cache *this_trad_cache,
				const struct regcache_map_entry *regmap,
				CORE_ADDR addr, size_t size);
void trad_frame_set_reg_value (struct trad_frame_cache *this_cache,
			       int regnum, LONGEST val);

/* Given the cache in THIS_TRAD_CACHE, set the value of REGNUM to the bytes
   contained in BYTES with size SIZE.  */
void trad_frame_set_reg_value_bytes (struct trad_frame_cache *this_trad_cache,
				     int regnum,
				     gdb::array_view<const gdb_byte> bytes);

struct value *trad_frame_get_register (struct trad_frame_cache *this_trad_cache,
				       frame_info_ptr this_frame,
				       int regnum);

/* Describes the kind of encoding a stored register has.  */
enum class trad_frame_saved_reg_kind
{
  /* Register value is unknown.  */
  UNKNOWN = 0,
  /* Register value is a constant.  */
  VALUE,
  /* Register value is in another register.  */
  REALREG,
  /* Register value is at an address.  */
  ADDR,
  /* Register value is a sequence of bytes.  */
  VALUE_BYTES
};

/* A struct that describes a saved register in a frame.  */

struct trad_frame_saved_reg
{
  /* Setters */

  /* Encode that the saved register's value is constant VAL in the
     trad-frame.  */
  void set_value (LONGEST val)
  {
    m_kind = trad_frame_saved_reg_kind::VALUE;
    m_reg.value = val;
  }

  /* Encode that the saved register's value is stored in register REALREG.  */
  void set_realreg (int realreg)
  {
    m_kind = trad_frame_saved_reg_kind::REALREG;
    m_reg.realreg = realreg;
  }

  /* Encode that the saved register's value is stored in memory at ADDR.  */
  void set_addr (LONGEST addr)
  {
    m_kind = trad_frame_saved_reg_kind::ADDR;
    m_reg.addr = addr;
  }

  /* Encode that the saved register's value is unknown.  */
  void set_unknown ()
  {
    m_kind = trad_frame_saved_reg_kind::UNKNOWN;
  }

  /* Encode that the saved register's value is stored as a sequence of bytes.
     This is useful when the value is larger than what primitive types
     can hold.  */
  void set_value_bytes (gdb::array_view<const gdb_byte> bytes)
  {
    /* Allocate the space and copy the data bytes.  */
    gdb_byte *data = FRAME_OBSTACK_CALLOC (bytes.size (), gdb_byte);
    memcpy (data, bytes.data (), bytes.size ());

    m_kind = trad_frame_saved_reg_kind::VALUE_BYTES;
    m_reg.value_bytes = data;
  }

  /* Getters */

  LONGEST value () const
  {
    gdb_assert (m_kind == trad_frame_saved_reg_kind::VALUE);
    return m_reg.value;
  }

  int realreg () const
  {
    gdb_assert (m_kind == trad_frame_saved_reg_kind::REALREG);
    return m_reg.realreg;
  }

  LONGEST addr () const
  {
    gdb_assert (m_kind == trad_frame_saved_reg_kind::ADDR);
    return m_reg.addr;
  }

  const gdb_byte *value_bytes () const
  {
    gdb_assert (m_kind == trad_frame_saved_reg_kind::VALUE_BYTES);
    return m_reg.value_bytes;
  }

  /* Convenience functions, return true if the register has been
     encoded as specified.  Return false otherwise.  */
  bool is_value () const
  {
    return m_kind == trad_frame_saved_reg_kind::VALUE;
  }

  bool is_realreg () const
  {
    return m_kind == trad_frame_saved_reg_kind::REALREG;
  }

  bool is_addr () const
  {
    return m_kind == trad_frame_saved_reg_kind::ADDR;
  }

  bool is_unknown () const
  {
    return m_kind == trad_frame_saved_reg_kind::UNKNOWN;
  }

  bool is_value_bytes () const
  {
    return m_kind == trad_frame_saved_reg_kind::VALUE_BYTES;
  }

private:

  trad_frame_saved_reg_kind m_kind;

  union {
    LONGEST value;
    int realreg;
    LONGEST addr;
    const gdb_byte *value_bytes;
  } m_reg;
};

/* Reset the saved regs cache, setting register values to REALREG.  */
void trad_frame_reset_saved_regs (struct gdbarch *gdbarch,
				  trad_frame_saved_reg *regs);

/* Return a freshly allocated (and initialized) trad_frame array.  */
trad_frame_saved_reg *trad_frame_alloc_saved_regs (frame_info_ptr);
trad_frame_saved_reg *trad_frame_alloc_saved_regs (struct gdbarch *);

/* Given the trad_frame info, return the location of the specified
   register.  */
struct value *trad_frame_get_prev_register (frame_info_ptr this_frame,
					    trad_frame_saved_reg this_saved_regs[],
					    int regnum);

#endif

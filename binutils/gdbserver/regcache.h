/* Register support routines for the remote server for GDB.
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

#ifndef GDBSERVER_REGCACHE_H
#define GDBSERVER_REGCACHE_H

#include "gdbsupport/common-regcache.h"

struct thread_info;
struct target_desc;

/* The data for the register cache.  Note that we have one per
   inferior; this is primarily for simplicity, as the performance
   benefit is minimal.  */

struct regcache : public reg_buffer_common
{
  /* The regcache's target description.  */
  const struct target_desc *tdesc = nullptr;

  /* Whether the REGISTERS buffer's contents are valid.  If false, we
     haven't fetched the registers from the target yet.  Not that this
     register cache is _not_ pass-through, unlike GDB's.  Note that
     "valid" here is unrelated to whether the registers are available
     in a traceframe.  For that, check REGISTER_STATUS below.  */
  int registers_valid = 0;
  int registers_owned = 0;
  unsigned char *registers = nullptr;
#ifndef IN_PROCESS_AGENT
  /* One of REG_UNAVAILABLE or REG_VALID.  */
  unsigned char *register_status = nullptr;
#endif

  /* See gdbsupport/common-regcache.h.  */
  enum register_status get_register_status (int regnum) const override;

  /* See gdbsupport/common-regcache.h.  */
  void raw_supply (int regnum, gdb::array_view<const gdb_byte> src) override;

  /* See gdbsupport/common-regcache.h.  */
  void raw_collect (int regnum, gdb::array_view<gdb_byte> dst) const override;

  /* See gdbsupport/common-regcache.h.  */
  bool raw_compare (int regnum, const void *buf, int offset) const override;
};

struct regcache *init_register_cache (struct regcache *regcache,
				      const struct target_desc *tdesc,
				      unsigned char *regbuf);

void regcache_cpy (struct regcache *dst, struct regcache *src);

/* Create a new register cache for INFERIOR.  */

struct regcache *new_register_cache (const struct target_desc *tdesc);

struct regcache *get_thread_regcache (struct thread_info *thread, int fetch);

/* Release all memory associated with the register cache for INFERIOR.  */

void free_register_cache (struct regcache *regcache);

/* Invalidate cached registers for one thread.  */

void regcache_invalidate_thread (struct thread_info *);

/* Invalidate cached registers for all threads of the given process.  */

void regcache_invalidate_pid (int pid);

/* Invalidate cached registers for all threads of the current
   process.  */

void regcache_invalidate (void);

/* Invalidate and release the register cache of all threads of the
   current process.  */

void regcache_release (void);

/* Convert all registers to a string in the currently specified remote
   format.  */

void registers_to_string (struct regcache *regcache, char *buf);

/* Convert a string to register values and fill our register cache.  */

void registers_from_string (struct regcache *regcache, char *buf);

/* For regcache_read_pc see gdbsupport/common-regcache.h.  */

void regcache_write_pc (struct regcache *regcache, CORE_ADDR pc);

int register_cache_size (const struct target_desc *tdesc);

int register_size (const struct target_desc *tdesc, int n);

/* No throw version of find_regno.  If NAME is not a known register, return
   an empty value.  */
std::optional<int> find_regno_no_throw (const struct target_desc *tdesc,
					const char *name);

int find_regno (const struct target_desc *tdesc, const char *name);

void supply_register (struct regcache *regcache, int n, const void *buf);

void supply_register_zeroed (struct regcache *regcache, int n);

void supply_register_by_name (struct regcache *regcache,
			      const char *name, const void *buf);

void supply_register_by_name_zeroed (struct regcache *regcache,
				     const char *name);

void supply_regblock (struct regcache *regcache, const void *buf);

void collect_register (struct regcache *regcache, int n, void *buf);

void collect_register_as_string (struct regcache *regcache, int n, char *buf);

void collect_register_by_name (struct regcache *regcache,
			       const char *name, void *buf);

/* Read a raw register as an unsigned integer.  Convenience wrapper
   around regcache_raw_get_unsigned that takes a register name instead
   of a register number.  */

ULONGEST regcache_raw_get_unsigned_by_name (struct regcache *regcache,
					    const char *name);

#endif /* GDBSERVER_REGCACHE_H */

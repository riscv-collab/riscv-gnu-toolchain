/* Cache and manage the values of registers for GDB, the GNU debugger.

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

#ifndef REGCACHE_H
#define REGCACHE_H

#include "gdbsupport/array-view.h"
#include "gdbsupport/common-regcache.h"
#include "gdbsupport/function-view.h"

struct regcache;
struct regset;
struct gdbarch;
class thread_info;
struct process_stratum_target;
struct inferior;

extern struct regcache *get_thread_regcache (process_stratum_target *target,
					     ptid_t ptid);

/* Get the regcache of THREAD.  */
extern struct regcache *get_thread_regcache (thread_info *thread);

extern regcache *get_thread_arch_regcache (inferior *inf_for_target_calls,
					   ptid_t ptid, gdbarch *arch);

extern enum register_status
  regcache_raw_read_signed (struct regcache *regcache,
			    int regnum, LONGEST *val);

extern void regcache_raw_write_signed (struct regcache *regcache,
				       int regnum, LONGEST val);
extern void regcache_raw_write_unsigned (struct regcache *regcache,
					 int regnum, ULONGEST val);

/* Return the register's value in signed or throw if it's not
   available.  */

extern LONGEST regcache_raw_get_signed (struct regcache *regcache,
					int regnum);

/* Read a register as a signed/unsigned quantity.  */
extern enum register_status
  regcache_cooked_read_signed (struct regcache *regcache,
			       int regnum, LONGEST *val);
extern enum register_status
  regcache_cooked_read_unsigned (struct regcache *regcache,
				 int regnum, ULONGEST *val);
extern void regcache_cooked_write_signed (struct regcache *regcache,
					  int regnum, LONGEST val);
extern void regcache_cooked_write_unsigned (struct regcache *regcache,
					    int regnum, ULONGEST val);

/* Special routines to read/write the PC.  */

/* For regcache_read_pc see gdbsupport/common-regcache.h.  */
extern void regcache_write_pc (struct regcache *regcache, CORE_ADDR pc);

/* Mapping between register numbers and offsets in a buffer, for use
   in the '*regset' functions below and with traditional frame caches.
   In an array of 'regcache_map_entry' each element is interpreted
   like follows:

   - If 'regno' is a register number: Map register 'regno' to the
     current offset (starting with 0) and increase the current offset
     by 'size' (or the register's size, if 'size' is zero).  Repeat
     this with consecutive register numbers up to 'regno+count-1'.

     For each described register, if 'size' is larger than the
     register's size, the register's value is assumed to be stored in
     the first N (where N is the register size) bytes at the current
     offset.  The remaining 'size' - N bytes are filled with zeroes by
     'regcache_collect_regset' and ignored by other consumers.

     If 'size' is smaller than the register's size, only the first
     'size' bytes of a register's value are assumed to be stored at
     the current offset.  'regcache_collect_regset' copies the first
     'size' bytes of a register's value to the output buffer.
     'regcache_supply_regset' copies the bytes from the input buffer
     into the first 'size' bytes of the register's value leaving the
     remaining bytes of the register's value unchanged.  Frame caches
     read the 'size' bytes from the stack frame and zero extend them
     to generate the register's value.

   - If 'regno' is REGCACHE_MAP_SKIP: Add 'count*size' to the current
     offset.

   - If count=0: End of the map.  */

struct regcache_map_entry
{
  int count;
  int regno;
  int size;
};

/* Special value for the 'regno' field in the struct above.  */

enum
  {
    REGCACHE_MAP_SKIP = -1,
  };

/* Calculate and return the total size of all the registers in a
   regcache_map_entry.  */

static inline int
regcache_map_entry_size (const struct regcache_map_entry *map)
{
  int size = 0;
  for (int i = 0; map[i].count != 0; i++)
    size += (map[i].count * map[i].size);
  return size;
}

/* Transfer a set of registers (as described by REGSET) between
   REGCACHE and BUF.  If REGNUM == -1, transfer all registers
   belonging to the regset, otherwise just the register numbered
   REGNUM.  The REGSET's 'regmap' field must point to an array of
   'struct regcache_map_entry'.

   These functions are suitable for the 'regset_supply' and
   'regset_collect' fields in a regset structure.  */

extern void regcache_supply_regset (const struct regset *regset,
				    struct regcache *regcache,
				    int regnum, const void *buf,
				    size_t size);
extern void regcache_collect_regset (const struct regset *regset,
				     const struct regcache *regcache,
				     int regnum, void *buf, size_t size);


/* Return true if a set of registers contains the value of the
   register numbered REGNUM.  The size of the set of registers is
   given in SIZE, and the layout of the set of registers is described
   by MAP.  */

extern bool regcache_map_supplies (const struct regcache_map_entry *map,
				   int regnum, struct gdbarch *gdbarch,
				   size_t size);

/* The type of a register.  This function is slightly more efficient
   then its gdbarch vector counterpart since it returns a precomputed
   value stored in a table.  */

extern struct type *register_type (struct gdbarch *gdbarch, int regnum);


/* Return the size of register REGNUM.  All registers should have only
   one size.  */
   
extern int register_size (struct gdbarch *gdbarch, int regnum);

using register_read_ftype
  = gdb::function_view<register_status (int, gdb::array_view<gdb_byte>)>;

/* A (register_number, register_value) pair.  */

struct cached_reg_t
{
  int num;
  gdb::unique_xmalloc_ptr<gdb_byte> data;

  cached_reg_t () = default;
  cached_reg_t (cached_reg_t &&rhs) = default;
};

/* Buffer of registers.  */

class reg_buffer : public reg_buffer_common
{
public:
  reg_buffer (gdbarch *gdbarch, bool has_pseudo);

  DISABLE_COPY_AND_ASSIGN (reg_buffer);

  /* Return regcache's architecture.  */
  gdbarch *arch () const;

  /* See gdbsupport/common-regcache.h.  */
  enum register_status get_register_status (int regnum) const override;

  /* See gdbsupport/common-regcache.h.  */
  void raw_collect (int regnum, gdb::array_view<gdb_byte> dst) const override;

  /* Deprecated overload of the above.  */
  void raw_collect (int regnum, void *dst) const;

  /* Collect register REGNUM from REGCACHE.  Store collected value as an integer
     at address ADDR, in target endian, with length ADDR_LEN and sign IS_SIGNED.
     If ADDR_LEN is greater than the register size, then the integer will be
     sign or zero extended.  If ADDR_LEN is smaller than the register size, then
     the most significant bytes of the integer will be truncated.  */
  void raw_collect_integer (int regnum, gdb_byte *addr, int addr_len,
			    bool is_signed) const;

  /* Collect part of register REGNUM from this register buffer.  Start at OFFSET
     in register.  The size is given by the size of DST.  */
  void raw_collect_part (int regnum, int offset,
			 gdb::array_view<gdb_byte> dst) const;

  /* Deprecated overload of the above.  */
  void raw_collect_part (int regnum, int offset, int len, gdb_byte *dst) const
  { raw_collect_part (regnum, offset, gdb::make_array_view (dst, len)); }

  /* See gdbsupport/common-regcache.h.  */
  void raw_supply (int regnum, gdb::array_view<const gdb_byte> src) override;

  /* Deprecated overload of the above.  */
  void raw_supply (int regnum, const void *src);

  void raw_supply (int regnum, const reg_buffer &src)
  { raw_supply (regnum, src.register_buffer (regnum)); }

  /* Supply register REGNUM to REGCACHE.  Value to supply is an integer stored
     at address ADDR, in target endian, with length ADDR_LEN and sign IS_SIGNED.
     If the register size is greater than ADDR_LEN, then the integer will be
     sign or zero extended.  If the register size is smaller than the integer,
     then the most significant bytes of the integer will be truncated.  */
  void raw_supply_integer (int regnum, const gdb_byte *addr, int addr_len,
			   bool is_signed);

  /* Supply register REGNUM with zeroed value to REGCACHE.  This is not the same
     as calling raw_supply with NULL (which will set the state to
     unavailable).  */
  void raw_supply_zeroed (int regnum);

  /* Supply part of register REGNUM to this register buffer.  Start at OFFSET in
     the register.  The size is given by the size of SRC.  The rest of the
     register left untouched.  */
  void raw_supply_part (int regnum, int offset,
			gdb::array_view<const gdb_byte> src);

  void invalidate (int regnum);

  virtual ~reg_buffer () = default;

  /* See gdbsupport/common-regcache.h.  */
  bool raw_compare (int regnum, const void *buf, int offset) const override;

protected:
  /* Assert on the range of REGNUM.  */
  void assert_regnum (int regnum) const;

  int num_raw_registers () const;

  /* Return a view on register REGNUM's buffer cache.  */
  template <typename ElemType>
  gdb::array_view<ElemType> register_buffer (int regnum) const;
  gdb::array_view<const gdb_byte> register_buffer (int regnum) const;
  gdb::array_view<gdb_byte> register_buffer (int regnum);

  /* Save a register cache.  The set of registers saved into the
     regcache determined by the save_reggroup.  COOKED_READ returns
     zero iff the register's value can't be returned.  */
  void save (register_read_ftype cooked_read);

  struct regcache_descr *m_descr;

  bool m_has_pseudo;
  /* The register buffers.  */
  std::unique_ptr<gdb_byte[]> m_registers;
  /* Register cache status.  */
  std::unique_ptr<register_status[]> m_register_status;

  friend class regcache;
  friend class detached_regcache;
};

/* An abstract class which only has methods doing read.  */

class readable_regcache : public reg_buffer
{
public:
  readable_regcache (gdbarch *gdbarch, bool has_pseudo)
    : reg_buffer (gdbarch, has_pseudo)
  {}

  /* Transfer a raw register [0..NUM_REGS) from core-gdb to this regcache,
     return its value in *BUF and return its availability status.  */
  register_status raw_read (int regnum, gdb::array_view<gdb_byte> dst);

  /* Deprecated overload of the above.  */
  register_status raw_read (int regnum, gdb_byte *dst);

  template<typename T, typename = RequireLongest<T>>
  register_status raw_read (int regnum, T *val);

  /* Partial transfer of raw registers.  Return the status of the register.  */
  register_status raw_read_part (int regnum, int offset,
				 gdb::array_view<gdb_byte> dst);

  /* Deprecated overload of the above.  */
  register_status raw_read_part (int regnum, int offset, int len,
				 gdb_byte *dst)
  { return raw_read_part (regnum, offset, gdb::make_array_view (dst, len)); }

  /* Make certain that the register REGNUM is up-to-date.  */
  virtual void raw_update (int regnum) = 0;

  /* Transfer a raw register [0..NUM_REGS+NUM_PSEUDO_REGS) from core-gdb to
     this regcache, return its value in DST and return its availability status.  */
  register_status cooked_read (int regnum, gdb::array_view<gdb_byte> dst);
  register_status cooked_read (int regnum, gdb_byte *dst);

  template<typename T, typename = RequireLongest<T>>
  register_status cooked_read (int regnum, T *val);

  /* Partial transfer of a cooked register.  */
  register_status cooked_read_part (int regnum, int offset,
				    gdb::array_view<gdb_byte> dst);

  /* Deprecated overload of the above.  */
  register_status cooked_read_part (int regnum, int offset, int len, gdb_byte *src)
  { return cooked_read_part (regnum, offset, gdb::make_array_view (src, len)); }

  /* Read register REGNUM from the regcache and return a new value.  This
     will call mark_value_bytes_unavailable as appropriate.  */
  struct value *cooked_read_value (int regnum);

protected:

  /* Perform a partial register transfer using a read, modify, write
     operation.  Will fail if register is currently invalid.  */
  register_status read_part (int regnum, int offset,
			     gdb::array_view<gdb_byte> dst, bool is_raw);
};

/* Buffer of registers, can be read and written.  */

class detached_regcache : public readable_regcache
{
public:
  detached_regcache (gdbarch *gdbarch, bool has_pseudo)
    : readable_regcache (gdbarch, has_pseudo)
  {}

  void raw_update (int regnum) override
  {}

  DISABLE_COPY_AND_ASSIGN (detached_regcache);
};

class readonly_detached_regcache;

/* The register cache for storing raw register values.  */

class regcache : public detached_regcache
{
public:
  DISABLE_COPY_AND_ASSIGN (regcache);

  /* Restore 'this' regcache.  The set of registers restored into
     the regcache determined by the restore_reggroup.
     Writes to regcache will go through to the target.  SRC is a
     read-only register cache.  */
  void restore (readonly_detached_regcache *src);

  /* Update the value of raw register REGNUM (in the range [0..NUM_REGS)) and
     transfer its value to core-gdb.  */

  void raw_write (int regnum, gdb::array_view<const gdb_byte> src);

  /* Deprecated overload of the above.  */
  void raw_write (int regnum, const gdb_byte *src);

  template<typename T, typename = RequireLongest<T>>
  void raw_write (int regnum, T val);

  /* Transfer of pseudo-registers.  */
  void cooked_write (int regnum, gdb::array_view<const gdb_byte> src);

  /* Deprecated overload of the above.  */
  void cooked_write (int regnum, const gdb_byte *src);

  template<typename T, typename = RequireLongest<T>>
  void cooked_write (int regnum, T val);

  void raw_update (int regnum) override;

  /* Partial transfer of raw registers.  Perform read, modify, write style
     operations.  */
  void raw_write_part (int regnum, int offset,
		       gdb::array_view<const gdb_byte> src);

  /* Deprecated overload of the above.  */
  void raw_write_part (int regnum, int offset, int len, const gdb_byte *src)
  { raw_write_part (regnum, offset, gdb::make_array_view (src, len)); }

  /* Partial transfer of a cooked register.  Perform read, modify, write style
     operations.  */
  void cooked_write_part (int regnum, int offset,
			  gdb::array_view<const gdb_byte> src);

  /* Deprecated overload of the above.  */
  void cooked_write_part (int regnum, int offset, int len, const gdb_byte *src)
  { cooked_write_part (regnum, offset, gdb::make_array_view (src, len)); }

  /* Transfer a set of registers (as described by REGSET) between
     REGCACHE and BUF.  If REGNUM == -1, transfer all registers
     belonging to the regset, otherwise just the register numbered
     REGNUM.  The REGSET's 'regmap' field must point to an array of
     'struct regcache_map_entry'.  The valid register numbers in each
     entry in 'struct regcache_map_entry' are offset by REGBASE.  */

  void supply_regset (const struct regset *regset, int regbase,
		      int regnum, const void *buf, size_t size);

  void collect_regset (const struct regset *regset, int regbase, int regnum,
		       void *buf, size_t size) const;

  /* Same as the above, but with REGBASE == 0.  */

  void supply_regset (const struct regset *regset,
		      int regnum, const void *buf, size_t size)
  {
    supply_regset (regset, 0, regnum, buf, size);
  }

  void collect_regset (const struct regset *regset, int regnum,
		       void *buf, size_t size) const
  {
    collect_regset (regset, 0, regnum, buf, size);
  }

  /* Return REGCACHE's ptid.  */

  ptid_t ptid () const
  {
    gdb_assert (m_ptid != minus_one_ptid);

    return m_ptid;
  }

  void set_ptid (const ptid_t ptid)
  {
    this->m_ptid = ptid;
  }

/* Dump the contents of a register from the register cache to the target
   debug.  */
  void debug_print_register (const char *func, int regno);

protected:
  regcache (inferior *inf_for_target_calls, gdbarch *gdbarch);

private:

  /* Helper function for transfer_regset.  Copies across a single register.  */
  void transfer_regset_register (struct regcache *out_regcache, int regnum,
				 const gdb_byte *in_buf, gdb_byte *out_buf,
				 int slot_size, int offs) const;

  /* Transfer a single or all registers belonging to a certain register
     set to or from a buffer.  This is the main worker function for
     regcache_supply_regset and regcache_collect_regset.  */
  void transfer_regset (const struct regset *regset, int regbase,
			struct regcache *out_regcache,
			int regnum, const gdb_byte *in_buf,
			gdb_byte *out_buf, size_t size) const;

  /* Perform a partial register transfer using a read, modify, write
     operation.  */
  register_status write_part (int regnum, int offset,
			      gdb::array_view<const gdb_byte> src,
			      bool is_raw);

  /* The inferior to switch to, to make target calls.

     This may not be the inferior of thread M_PTID.  For instance, this
     regcache might be for a fork child we are about to detach, so there will
     never be an inferior for that thread / process.  Nevertheless, we need to
     be able to switch to the target stack that can handle register reads /
     writes for this regcache, and that's what this inferior is for.  */
  inferior *m_inf_for_target_calls;

  /* If this is a read-write cache, which thread's registers is
     it connected to?  */
  ptid_t m_ptid;

  friend regcache *get_thread_arch_regcache (inferior *inf_for_target_calls,
					     ptid_t ptid, gdbarch *gdbarch);
};

using regcache_up = std::unique_ptr<regcache>;

class readonly_detached_regcache : public readable_regcache
{
public:
  readonly_detached_regcache (regcache &src);

  /* Create a readonly regcache by getting contents from COOKED_READ.  */

  readonly_detached_regcache (gdbarch *gdbarch, register_read_ftype cooked_read)
    : readable_regcache (gdbarch, true)
  {
    save (cooked_read);
  }

  DISABLE_COPY_AND_ASSIGN (readonly_detached_regcache);

  void raw_update (int regnum) override
  {}
};

extern void registers_changed (void);
extern void registers_changed_ptid (process_stratum_target *target,
				    ptid_t ptid);

/* Indicate that registers of THREAD may have changed, so invalidate
   the cache.  */
extern void registers_changed_thread (thread_info *thread);

/* An abstract base class for register dump.  */

class register_dump
{
public:
  void dump (ui_file *file);
  virtual ~register_dump () = default;

protected:
  register_dump (gdbarch *arch)
    : m_gdbarch (arch)
  {}

  /* Dump the register REGNUM contents.  If REGNUM is -1, print the
     header.  */
  virtual void dump_reg (ui_file *file, int regnum) = 0;

  gdbarch *m_gdbarch;
};

#endif /* REGCACHE_H */

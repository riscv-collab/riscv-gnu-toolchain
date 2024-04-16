/* Native-dependent code for SPARC.

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

#ifndef SPARC_NAT_H
#define SPARC_NAT_H 1

#include "target.h"

struct sparc_gregmap;
struct sparc_fpregmap;

extern const struct sparc_gregmap *sparc_gregmap;
extern const struct sparc_fpregmap *sparc_fpregmap;
extern void (*sparc_supply_gregset) (const struct sparc_gregmap *,
				     struct regcache *, int , const void *);
extern void (*sparc_collect_gregset) (const struct sparc_gregmap *,
				      const struct regcache *, int, void *);
extern void (*sparc_supply_fpregset) (const struct sparc_fpregmap *,
				      struct regcache *, int , const void *);
extern void (*sparc_collect_fpregset) (const struct sparc_fpregmap *,
				       const struct regcache *, int , void *);
extern int (*sparc_gregset_supplies_p) (struct gdbarch *gdbarch, int);
extern int (*sparc_fpregset_supplies_p) (struct gdbarch *gdbarch, int);

extern int sparc32_gregset_supplies_p (struct gdbarch *gdbarch, int regnum);
extern int sparc32_fpregset_supplies_p (struct gdbarch *gdbarch, int regnum);

extern void sparc_fetch_inferior_registers (process_stratum_target *proc_target,
					    regcache *, int);
extern void sparc_store_inferior_registers (process_stratum_target *proc_target,
					    regcache *, int);

extern target_xfer_status sparc_xfer_wcookie (enum target_object object,
					      const char *annex,
					      gdb_byte *readbuf,
					      const gdb_byte *writebuf,
					      ULONGEST offset,
					      ULONGEST len,
					      ULONGEST *xfered_len);

/* A prototype generic SPARC target.  The client can override
   it with local methods.  */

template<typename BaseTarget>
struct sparc_target : public BaseTarget
{
  void fetch_registers (struct regcache *regcache, int regnum) override
  { sparc_fetch_inferior_registers (this, regcache, regnum); }

  void store_registers (struct regcache *regcache, int regnum) override
  { sparc_store_inferior_registers (this, regcache, regnum); }

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override
  {
    if (object == TARGET_OBJECT_WCOOKIE)
      return sparc_xfer_wcookie (object, annex, readbuf, writebuf,
				 offset, len, xfered_len);

    return BaseTarget::xfer_partial (object, annex, readbuf, writebuf,
				     offset, len, xfered_len);
  }
};

#endif /* sparc-nat.h */

/* Generic GNU/Linux target using traditional ptrace register access.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef LINUX_NAT_TRAD_H
#define LINUX_NAT_TRAD_H

#include "linux-nat.h"

/* A prototype GNU/Linux target using traditional ptrace register
   access.  A concrete type should override REGISTER_U_OFFSET.  */

class linux_nat_trad_target : public linux_nat_target
{
public:
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

protected:
  /* Return the offset within the user area where a particular
     register is stored.  */
  virtual CORE_ADDR register_u_offset (struct gdbarch *gdbarch,
				       int regnum, int store) = 0;

private:
  /* Helpers.  See definition.  */
  void fetch_register (struct regcache *regcache, int regnum);
  void store_register (const struct regcache *regcache, int regnum);
};

#endif /* LINUX_NAT_TRAD_H */

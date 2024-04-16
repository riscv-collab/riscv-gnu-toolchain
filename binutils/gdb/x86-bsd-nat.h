/* Native-dependent code for x86 BSD's.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef X86_BSD_NAT_H
#define X86_BSD_NAT_H

#include "x86-nat.h"

/* A prototype *BSD/x86 target.  */

#ifdef HAVE_PT_GETDBREGS
template<typename BaseTarget>
class x86bsd_nat_target : public x86_nat_target<BaseTarget>
{
  using base_class = x86_nat_target<BaseTarget>;
public:
  void mourn_inferior () override
  {
    x86_cleanup_dregs ();
    base_class::mourn_inferior ();
  }
};
#else /* !HAVE_PT_GETDBREGS */
template<typename BaseTarget>
class x86bsd_nat_target : public BaseTarget
{
};
#endif /* HAVE_PT_GETDBREGS */

#endif /* x86-bsd-nat.h */

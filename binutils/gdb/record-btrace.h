/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <tim.wiederhake@intel.com>

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

#ifndef RECORD_BTRACE_H
#define RECORD_BTRACE_H

/* Push the record_btrace target.  */
extern void record_btrace_push_target (void);

/* Return the cpu configured by the user via "set btrace cpu".  Returns
   NULL if the cpu was configured as auto.  */
extern const struct btrace_cpu *record_btrace_get_cpu (void);

#endif /* RECORD_BTRACE_H */

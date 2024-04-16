/* Data structures associated with breakpoints shared in both GDB and
   GDBserver.
   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_BREAK_COMMON_H
#define COMMON_BREAK_COMMON_H

enum target_hw_bp_type
  {
    hw_write   = 0, 		/* Common  HW watchpoint */
    hw_read    = 1, 		/* Read    HW watchpoint */
    hw_access  = 2, 		/* Access  HW watchpoint */
    hw_execute = 3		/* Execute HW breakpoint */
  };

#endif /* COMMON_BREAK_COMMON_H */

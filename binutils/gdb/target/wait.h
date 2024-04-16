/* Target wait definitions and prototypes.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#ifndef TARGET_WAIT_H
#define TARGET_WAIT_H

#include "gdbsupport/enum-flags.h"

/* Options that can be passed to target_wait.  */

enum target_wait_flag : unsigned
{
  /* Return immediately if there's no event already queued.  If this
     options is not requested, target_wait blocks waiting for an
     event.  */
  TARGET_WNOHANG = 1,
};

DEF_ENUM_FLAGS_TYPE (enum target_wait_flag, target_wait_flags);

#endif /* TARGET_WAIT_H */

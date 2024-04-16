/* Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_LINUX_AARCH32_TDESC_H
#define GDBSERVER_LINUX_AARCH32_TDESC_H

/* Return the AArch32 target description.  */

const target_desc * aarch32_linux_read_description ();

/* Return true if TDESC is the AArch32 target description.  */

bool is_aarch32_linux_description (const target_desc *tdesc);

#endif /* linux-aarch32-tdesc.h.  */

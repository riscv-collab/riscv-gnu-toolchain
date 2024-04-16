/* Low level support for aarch64, shared between gdbserver and IPA.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_LINUX_AARCH64_TDESC_H
#define GDBSERVER_LINUX_AARCH64_TDESC_H

#include "arch/aarch64.h"

const target_desc *
  aarch64_linux_read_description (const aarch64_features &features);

#endif /* GDBSERVER_LINUX_AARCH64_TDESC_H */

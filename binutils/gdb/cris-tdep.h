/* Target dependent code for CRIS, for GDB, the GNU debugger.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

   Contributed by Axis Communications AB.
   Written by Hendrik Ruijter, Stefan Andersson, and Orjan Friberg.

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

#ifndef CRIS_TDEP_H
#define CRIS_TDEP_H

#include "gdbarch.h"

/* CRIS architecture specific information.  */
struct cris_gdbarch_tdep : gdbarch_tdep_base
{
  unsigned int cris_version = 0;
  const char *cris_mode = nullptr;
  int cris_dwarf2_cfi = 0;
};

#endif /* CRIS_TDEP_H */

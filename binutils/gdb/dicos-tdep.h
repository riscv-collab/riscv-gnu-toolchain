/* Target-dependent, architecture-independent code for DICOS, for GDB.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef DICOS_TDEP_H
#define DICOS_TDEP_H

extern void dicos_init_abi (struct gdbarch *gdbarch);
extern int dicos_load_module_p (bfd *abfd, int header_size);

#endif /* dicos-tdep.h */

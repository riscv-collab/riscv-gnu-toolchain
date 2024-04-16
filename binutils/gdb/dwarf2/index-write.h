/* DWARF index writing support for GDB.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef DWARF_INDEX_WRITE_H
#define DWARF_INDEX_WRITE_H

#include "dwarf2/read.h"
#include "dwarf2/public.h"

/* Create index files for OBJFILE in the directory DIR.

   An index file is created for OBJFILE itself, and is created for its
   associated dwz file, if it has one.

   BASENAME is the desired filename base for OBJFILE's index.  An extension
   derived from INDEX_KIND is added to this base name.  DWZ_BASENAME is the
   same, but for the dwz file's index.  */

extern void write_dwarf_index
  (dwarf2_per_bfd *per_bfd, const char *dir, const char *basename,
   const char *dwz_basename, dw_index_kind index_kind);

#endif /* DWARF_INDEX_WRITE_H */

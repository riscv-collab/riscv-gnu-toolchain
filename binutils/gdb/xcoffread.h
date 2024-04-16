/* Read AIX xcoff symbol tables and convert to internal format, for GDB.
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

#ifndef XCOFFREAD_H
#define XCOFFREAD_H

extern CORE_ADDR xcoff_get_toc_offset (struct objfile *);

extern int xcoff_get_n_import_files (bfd *abfd);

#endif /* XCOFFREAD_H */

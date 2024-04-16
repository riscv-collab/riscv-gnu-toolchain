/* Handle different target file systems for GDB, the GNU Debugger.
   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

extern const char file_system_kind_auto[];
extern const char file_system_kind_unix[];
extern const char file_system_kind_dos_based[];

extern const char *target_file_system_kind;

/* Same as IS_DIR_SEPARATOR but with file system kind KIND's
   semantics, instead of host semantics.  */

#define IS_TARGET_DIR_SEPARATOR(kind, c)				\
  (((kind) == file_system_kind_dos_based) ? IS_DOS_DIR_SEPARATOR (c) \
   : IS_UNIX_DIR_SEPARATOR (c))

/* Same as IS_ABSOLUTE_PATH but with file system kind KIND's
   semantics, instead of host semantics.  */

#define IS_TARGET_ABSOLUTE_PATH(kind, p)				\
  (((kind) == file_system_kind_dos_based) ? IS_DOS_ABSOLUTE_PATH (p) \
   : IS_UNIX_ABSOLUTE_PATH (p))

/* Same as HAS_DRIVE_SPEC but with file system kind KIND's semantics,
   instead of host semantics.  */

#define HAS_TARGET_DRIVE_SPEC(kind, p)					\
  (((kind) == file_system_kind_dos_based) ? HAS_DOS_DRIVE_SPEC (p) \
   : 0)

/* Same as lbasename, but with file system kind KIND's semantics,
   instead of host semantics.  */
extern const char *target_lbasename (const char *kind, const char *name);

/* The effective setting of "set target-file-system-kind", with "auto"
   resolved to the real kind.  That is, you never see "auto" as a
   result from this function.  */
extern const char *effective_target_file_system_kind (void);

#endif

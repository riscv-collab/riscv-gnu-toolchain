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

#include "defs.h"
#include "filesystem.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "inferior.h"

const char file_system_kind_auto[] = "auto";
const char file_system_kind_unix[] = "unix";
const char file_system_kind_dos_based[] = "dos-based";
const char *const target_file_system_kinds[] =
{
  file_system_kind_auto,
  file_system_kind_unix,
  file_system_kind_dos_based,
  NULL
};
const char *target_file_system_kind = file_system_kind_auto;

const char *
effective_target_file_system_kind (void)
{
  if (target_file_system_kind == file_system_kind_auto)
    {
      if (gdbarch_has_dos_based_file_system (current_inferior ()->arch ()))
	return file_system_kind_dos_based;
      else
	return file_system_kind_unix;
    }
  else
    return target_file_system_kind;
}

const char *
target_lbasename (const char *kind, const char *name)
{
  if (kind == file_system_kind_dos_based)
    return dos_lbasename (name);
  else
    return unix_lbasename (name);
}

static void
show_target_file_system_kind_command (struct ui_file *file,
				      int from_tty,
				      struct cmd_list_element *c,
				      const char *value)
{
  if (target_file_system_kind == file_system_kind_auto)
    gdb_printf (file, _("\
The assumed file system kind for target reported file names \
is \"%s\" (currently \"%s\").\n"),
		value,
		effective_target_file_system_kind ());
  else
    gdb_printf (file, _("\
The assumed file system kind for target reported file names \
is \"%s\".\n"),
		value);
}

void _initialize_filesystem ();
void
_initialize_filesystem ()
{
  add_setshow_enum_cmd ("target-file-system-kind",
			class_files,
			target_file_system_kinds,
			&target_file_system_kind, _("\
Set assumed file system kind for target reported file names."), _("\
Show assumed file system kind for target reported file names."),
			_("\
If `unix', target file names (e.g., loaded shared library file names)\n\
starting the forward slash (`/') character are considered absolute,\n\
and the directory separator character is the forward slash (`/').  If\n\
`dos-based', target file names starting with a drive letter followed\n\
by a colon (e.g., `c:'), are also considered absolute, and the\n\
backslash (`\\') is also considered a directory separator.  Set to\n\
`auto' (which is the default), to let GDB decide, based on its\n\
knowledge of the target operating system."),
			NULL, /* setfunc */
			show_target_file_system_kind_command,
			&setlist, &showlist);
}

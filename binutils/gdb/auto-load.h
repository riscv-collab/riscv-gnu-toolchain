/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#ifndef AUTO_LOAD_H
#define AUTO_LOAD_H 1

struct objfile;
struct program_space;
struct auto_load_pspace_info;
struct extension_language_defn;

namespace gdb {
namespace observers {
struct token;
} /* namespace observers */
} /* namespace gdb */

/* Value of the 'set debug auto-load' configuration variable.  */

extern bool debug_auto_load;

/* Print an "auto-load" debug statement.  */

#define auto_load_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_auto_load, "auto-load", fmt, ##__VA_ARGS__)

extern bool global_auto_load;

extern bool auto_load_local_gdbinit;
extern char *auto_load_local_gdbinit_pathname;
extern bool auto_load_local_gdbinit_loaded;

/* Token used for the auto_load_new_objfile observer, so other observers can
   specify it as a dependency. */
extern gdb::observers::token auto_load_new_objfile_observer_token;

extern struct auto_load_pspace_info *
  get_auto_load_pspace_data_for_loading (struct program_space *pspace);
extern void auto_load_objfile_script (struct objfile *objfile,
				      const struct extension_language_defn *);
extern void load_auto_scripts_for_objfile (struct objfile *objfile);
extern char auto_load_info_scripts_pattern_nl[];
extern void auto_load_info_scripts (program_space *pspace, const char *pattern,
				    int from_tty,
				    const extension_language_defn *);

extern struct cmd_list_element **auto_load_set_cmdlist_get (void);
extern struct cmd_list_element **auto_load_show_cmdlist_get (void);
extern struct cmd_list_element **auto_load_info_cmdlist_get (void);

/* Return true if FILENAME is located in one of the directories of
   AUTO_LOAD_SAFE_PATH.  Otherwise call warning and return false.  FILENAME does
   not have to be an absolute path.

   Existence of FILENAME is not checked.  Function will still give a warning
   even if the caller would quietly skip non-existing file in unsafe
   directory.  */

extern bool file_is_auto_load_safe (const char *filename);

/* Return true if auto-loading gdb scripts is enabled.  */

extern bool auto_load_gdb_scripts_enabled
  (const struct extension_language_defn *extlang);

#endif /* AUTO_LOAD_H */

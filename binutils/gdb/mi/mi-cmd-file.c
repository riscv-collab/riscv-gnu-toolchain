/* MI Command Set - file commands.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "mi-cmds.h"
#include "mi-getopt.h"
#include "mi-interp.h"
#include "ui-out.h"
#include "symtab.h"
#include "source.h"
#include "objfiles.h"
#include "solib.h"
#include "solist.h"
#include "gdbsupport/gdb_regex.h"

/* Return to the client the absolute path and line number of the 
   current file being executed.  */

void
mi_cmd_file_list_exec_source_file (const char *command,
				   const char *const *argv, int argc)
{
  struct symtab_and_line st;
  struct ui_out *uiout = current_uiout;
  
  if (!mi_valid_noargs ("-file-list-exec-source-file", argc, argv))
    error (_("-file-list-exec-source-file: Usage: No args"));

  /* Set the default file and line, also get them.  */
  set_default_source_symtab_and_line ();
  st = get_current_source_symtab_and_line ();

  /* We should always get a symtab.  Apparently, filename does not
     need to be tested for NULL.  The documentation in symtab.h
     suggests it will always be correct.  */
  if (!st.symtab)
    error (_("-file-list-exec-source-file: No symtab"));

  /* Print to the user the line, filename and fullname.  */
  uiout->field_signed ("line", st.line);
  uiout->field_string ("file", symtab_to_filename_for_display (st.symtab));

  uiout->field_string ("fullname", symtab_to_fullname (st.symtab));

  uiout->field_signed ("macro-info",
		       st.symtab->compunit ()->macro_table () != NULL);
}

/* Implement -file-list-exec-source-files command.  */

void
mi_cmd_file_list_exec_source_files (const char *command,
				    const char *const *argv, int argc)
{
  enum opt
    {
      GROUP_BY_OBJFILE_OPT,
      MATCH_BASENAME_OPT,
      MATCH_DIRNAME_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"-group-by-objfile", GROUP_BY_OBJFILE_OPT, 0},
    {"-basename", MATCH_BASENAME_OPT, 0},
    {"-dirname", MATCH_DIRNAME_OPT, 0},
    { 0, 0, 0 }
  };

  /* Parse arguments.  */
  int oind = 0;
  const char *oarg;

  bool group_by_objfile = false;
  bool match_on_basename = false;
  bool match_on_dirname = false;

  while (1)
    {
      int opt = mi_getopt ("-file-list-exec-source-files", argc, argv,
			   opts, &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case GROUP_BY_OBJFILE_OPT:
	  group_by_objfile = true;
	  break;
	case MATCH_BASENAME_OPT:
	  match_on_basename = true;
	  break;
	case MATCH_DIRNAME_OPT:
	  match_on_dirname = true;
	  break;
	}
    }

  if ((argc - oind > 1) || (match_on_basename && match_on_dirname))
    error (_("-file-list-exec-source-files: Usage: [--group-by-objfile] [--basename | --dirname] [--] REGEXP"));

  const char *regexp = nullptr;
  if (argc - oind == 1)
    regexp = argv[oind];

  info_sources_filter::match_on match_type;
  if (match_on_dirname)
    match_type = info_sources_filter::match_on::DIRNAME;
  else if (match_on_basename)
    match_type = info_sources_filter::match_on::BASENAME;
  else
    match_type = info_sources_filter::match_on::FULLNAME;

  info_sources_filter filter (match_type, regexp);
  info_sources_worker (current_uiout, group_by_objfile, filter);
}

/* See mi-cmds.h.  */

void
mi_cmd_file_list_shared_libraries (const char *command,
				   const char *const *argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  const char *pattern;

  switch (argc)
    {
    case 0:
      pattern = NULL;
      break;
    case 1:
      pattern = argv[0];
      break;
    default:
      error (_("Usage: -file-list-shared-libraries [REGEXP]"));
    }

  if (pattern != NULL)
    {
      const char *re_err = re_comp (pattern);

      if (re_err != NULL)
	error (_("Invalid regexp: %s"), re_err);
    }

  update_solib_list (1);

  /* Print the table header.  */
  ui_out_emit_list list_emitter (uiout, "shared-libraries");

  for (const shobj &so : current_program_space->solibs ())
    {
      if (so.so_name.empty ())
	continue;

      if (pattern != nullptr && !re_exec (so.so_name.c_str ()))
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);
      mi_output_solib_attribs (uiout, so);
    }
}

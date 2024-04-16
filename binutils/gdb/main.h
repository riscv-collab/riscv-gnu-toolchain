/* Main interface for GDB, the GNU debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#ifndef MAIN_H
#define MAIN_H

struct captured_main_args
{
  int argc;
  char **argv;
  const char *interpreter_p;
};

extern int gdb_main (struct captured_main_args *);

/* From main.c.  */
extern int return_child_result;
extern int return_child_result_value;
extern int batch_silent;
extern int batch_flag;

/* * The name of the interpreter if specified on the command line.  */
extern std::string interpreter_p;

/* From mingw-hdep.c, used by main.c.  */

/* Return argv[0] in absolute form, if possible, or ARGV0 if not.  The
   return value is in malloc'ed storage.  */
extern char *windows_get_absolute_argv0 (const char *argv0);

/* Return read only pointer to the name of gdb as it was invoked.  This
   might have been expanded to an absolute path if required by the
   platform.  Could return NULL if called before gdb has had a chance to
   parse the argv array.  */
extern const char *get_gdb_program_name (void);

extern void set_gdb_data_directory (const char *new_data_dir);

#endif

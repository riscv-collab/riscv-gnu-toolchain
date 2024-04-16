/* Stack manipulation commands, for GDB the GNU Debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef STACK_H
#define STACK_H

gdb::unique_xmalloc_ptr<char> find_frame_funname (frame_info_ptr frame,
						  enum language *funlang,
						  struct symbol **funcp);

typedef gdb::function_view<void (const char *print_name, struct symbol *sym)>
     iterate_over_block_arg_local_vars_cb;

void iterate_over_block_arg_vars (const struct block *block,
				  iterate_over_block_arg_local_vars_cb cb);

void iterate_over_block_local_vars (const struct block *block,
				    iterate_over_block_arg_local_vars_cb cb);

/* Initialize *WHAT to be a copy of the user desired print what frame info.
   If !WHAT.has_value (), the printing function chooses a default set of
   information to print, otherwise the printing function should print
   the relevant information.  */

void get_user_print_what_frame_info (std::optional<enum print_what> *what);

/* Return true if we should display the address in addition to the location,
   because we are in the middle of a statement.  */

bool frame_show_address (frame_info_ptr frame, struct symtab_and_line sal);

/* Forget the last sal we displayed.  */

void clear_last_displayed_sal (void);

/* Is our record of the last sal we displayed valid?  If not, the
   get_last_displayed_* functions will return NULL or 0, as appropriate.  */

bool last_displayed_sal_is_valid (void);

/* Get the pspace of the last sal we displayed, if it's valid, otherwise
   return nullptr.  */

struct program_space* get_last_displayed_pspace (void);

/* Get the address of the last sal we displayed, if it's valid, otherwise
   return an address of 0.  */

CORE_ADDR get_last_displayed_addr (void);

/* Get the symtab of the last sal we displayed, if it's valid, otherwise
   return nullptr.  */

struct symtab* get_last_displayed_symtab (void);

/* Get the line of the last sal we displayed, if it's valid, otherwise
   return 0.  */

int get_last_displayed_line (void);

/* Get the last sal we displayed, if it's valid, otherwise return a
   symtab_and_line constructed in its default state.  */

symtab_and_line get_last_displayed_sal ();

/* Completer for the "frame apply all" command.  */
void frame_apply_all_cmd_completer (struct cmd_list_element *ignore,
				    completion_tracker &tracker,
				    const char *text, const char */*word*/);

#endif /* #ifndef STACK_H */

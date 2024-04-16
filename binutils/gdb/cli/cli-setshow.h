/* Header file for GDB CLI set and show commands implementation.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef CLI_CLI_SETSHOW_H
#define CLI_CLI_SETSHOW_H

#include <string>

struct cmd_list_element;

/* Parse ARG, an option to a boolean variable.
   Returns 1 for true, 0 for false, and -1 if invalid.  */
extern int parse_cli_boolean_value (const char *arg);

/* Same as above, but work with a pointer to pointer.  ARG is advanced
   past a successfully parsed value.  */
extern int parse_cli_boolean_value (const char **arg);

/* Parse ARG, an option to a var_uinteger, var_integer or var_pinteger
   variable.  Return the parsed value on success or throw an error.  If
   EXTRA_LITERALS is non-null, then interpret those literals accordingly.
   If EXPRESSION is true, *ARG is parsed as an expression; otherwise,
   it is parsed with get_ulongest.  It's not possible to parse the
   integer as an expression when there may be valid input after the
   integer, such as when parsing command options.  E.g., "print
   -elements NUMBER -obj --".  In such case, parsing as an expression
   would parse "-obj --" as part of the expression as well.  */
extern LONGEST parse_cli_var_integer (var_types var_type,
				      const literal_def *extra_literals,
				      const char **arg,
				      bool expression);

/* Parse ARG, an option to a var_enum variable.  ENUM is a
   null-terminated array of possible values. Either returns the parsed
   value on success or throws an error.  ARG is advanced past the
   parsed value.  */
const char *parse_cli_var_enum (const char **args,
				const char *const *enums);

extern void do_set_command (const char *arg, int from_tty,
			    struct cmd_list_element *c);
extern void do_show_command (const char *arg, int from_tty,
			     struct cmd_list_element *c);

/* Get a string version of VAR's value.  */
extern std::string get_setshow_command_value_string (const setting &var);

extern void cmd_show_list (struct cmd_list_element *list, int from_tty);

#endif /* CLI_CLI_SETSHOW_H */

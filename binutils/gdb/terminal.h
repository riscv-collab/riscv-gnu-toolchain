/* Terminal interface definitions for GDB, the GNU Debugger.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#if !defined (TERMINAL_H)
#define TERMINAL_H 1

struct inferior;

extern void new_tty_prefork (std::string ttyname);

extern void new_tty (void);

extern void new_tty_postfork (void);

extern void copy_terminal_info (struct inferior *to, struct inferior *from);

/* Exchange the terminal info and state between inferiors A and B.  */
extern void swap_terminal_info (inferior *a, inferior *b);

extern pid_t create_tty_session (void);

/* Set up a serial structure describing standard input.  In inflow.c.  */
extern void initialize_stdin_serial (void);

extern void gdb_save_tty_state (void);

/* Take a snapshot of our initial tty state before readline/ncurses
   have had a chance to alter it.  */
extern void set_initial_gdb_ttystate (void);

#endif /* !defined (TERMINAL_H) */

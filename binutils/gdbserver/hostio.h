/* Host file transfer support for gdbserver.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_HOSTIO_H
#define GDBSERVER_HOSTIO_H

/* Per-connection setup.  */
extern void hostio_handle_new_gdb_connection (void);

extern int handle_vFile (char *, int, int *);

#endif /* GDBSERVER_HOSTIO_H */

/* List of target connections for GDB.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef TARGET_CONNECTION_H
#define TARGET_CONNECTION_H

#include <string>

struct process_stratum_target;

/* Add a process target to the connection list, if not already
   added.  */
void connection_list_add (process_stratum_target *t);

/* Remove a process target from the connection list.  */
void connection_list_remove (process_stratum_target *t);

/* Make a target connection string for T.  This is usually T's
   shortname, but it includes the result of
   process_stratum_target::connection_string() too if T supports
   it.  */
std::string make_target_connection_string (process_stratum_target *t);

#endif /* TARGET_CONNECTION_H */
